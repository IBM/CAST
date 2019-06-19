/*******************************************************************************
 |    bbwrkqe.cc
 |
 |  © Copyright IBM Corporation 2015,2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/

#include <sys/syscall.h>

#include "bbinternal.h"
#include "BBLV_Info.h"
#include "BBLV_Metadata.h"
#include "bbwrkqe.h"
#include "bbserver_flightlog.h"
#include "Extent.h"
#include "ExtentInfo.h"
#include "util.h"
#include "xfer.h"


void WRKQE::addWorkItem(WorkID& pWorkItem, const bool pValidateQueue)
{
    wrkq->push(pWorkItem);
    // NOTE: Unless we are debugging a problem, pValidateQueue always comes in as false.
    //       Therefore, we never hit the endOnError() below in production...
    if (pValidateQueue && this != HPWrkQE)
    {
        // NOTE:  This method exists to make sure that the number of work queue entries
        //        matches the number of entries in the vector of extents to transfer
        //        for this LVKey when we are adding an entry to the work queue.
        BBLV_Info* l_LV_Info = pWorkItem.getLV_Info();
        if (l_LV_Info && (getWrkQ_Size() != l_LV_Info->getNumberOfExtents()))
        {
            LOG(bb,error) << "WRKQE::addWorkItem(): Mismatch between number of elements on work queue (" << getWrkQ_Size() << ") and number of extents in the vector of extents (" \
                          << l_LV_Info->getNumberOfExtents() << ")";
            endOnError();
        }
    }

    incrementNumberOfWorkItems();

    return;
};

void WRKQE::dump(const char* pSev, const char* pPrefix)
{
    string l_JobId = to_string(jobid);
    string l_JobStepId = "";
    string l_Handle = "";
    string l_ContribId = "";
    string l_Rate = "";
    string l_Bucket = "";
    string l_ThrottleWait = "";
    string l_WorkQueueReturnedWithNegativeBucket = "";
    string l_Suspended = (suspended ? "Y" : "N");
    string l_Output = "Job " + l_JobId + ", Susp " + l_Suspended;

    if (wrkq)
    {
        int l_TransferQueueLocked = lockTransferQueueIfNeeded((LVKey*)0, "WRKQE::dump - entry");

        if (getWrkQ_Size())
        {
            BBLV_Info* l_LV_Info = getLV_Info();
            if (l_LV_Info)
            {
                // NOTE: The high priority work queue will not fall into this leg...
                ExtentInfo l_ExtentInfo = l_LV_Info->getNextExtentInfo();
                l_JobStepId = to_string(l_ExtentInfo.getTransferDef()->getJobStepId());
                l_Handle = to_string(l_ExtentInfo.getHandle());
                l_ContribId = to_string(l_ExtentInfo.getContrib());
                if (rate)
                {
                    l_Rate = to_string(rate);
                    l_Bucket = to_string(bucket);
                    l_ThrottleWait = to_string(throttleWait);
                    l_WorkQueueReturnedWithNegativeBucket = to_string(workQueueReturnedWithNegativeBucket);
                }
            }
        }

        if (l_JobStepId.size())
        {
            l_Output += ", Jobstep " + l_JobStepId;
        }
        if (l_Handle.size())
        {
            l_Output += ", Hdl " + l_Handle;
        }
        if (l_ContribId.size())
        {
            l_Output += ", Cntb " + l_ContribId;
        }
        if (rate)
        {
            if (l_Rate.size())
            {
                l_Output += ", Rate " + l_Rate;
            }
            if (l_Bucket.size())
            {
                l_Output += ", Bkt " + l_Bucket;
            }
            if (l_ThrottleWait.size())
            {
                l_Output += ", TW " + l_ThrottleWait;
            }
            if (l_WorkQueueReturnedWithNegativeBucket.size())
            {
                l_Output += ", WQRNegB " + l_WorkQueueReturnedWithNegativeBucket;
            }
        }

        if (!strcmp(pSev,"debug"))
        {
            LOG(bb,debug) << pPrefix << lvKey << ", " << l_Output << ", #Items " << getNumberOfWorkItems() << ", #Proc'd " << getNumberOfWorkItemsProcessed() << ", CurSize " << getWrkQ_Size();
        }
        else if (!strcmp(pSev,"info"))
        {
            LOG(bb,info) << pPrefix << lvKey << ", " << l_Output << ", #Items " << getNumberOfWorkItems() << ", #Proc'd " << getNumberOfWorkItemsProcessed() << ", CurSize " << getWrkQ_Size();
        }

        if (l_TransferQueueLocked)
        {
            unlockTransferQueue((LVKey*)0, "WRKQE::dump - exit");
        }
    }
    else
    {
        LOG(bb,error) << pPrefix << lvKey << ", " << l_Output << ", #Items " << getNumberOfWorkItems() << ", #Proc'd " << getNumberOfWorkItemsProcessed() << ", wrkq is NULL";
    }

    return;
}

int WRKQE::getIssuingWorkItem()
{
    return issuingWorkItem;
}

void WRKQE::loadBucket()
{
    dump("debug", "loadBucket(): Before bucket modification: ");
    bucket = (int64_t)(MIN(bucket+rate, rate));
    if (bucket >= 0)
    {
        workQueueReturnedWithNegativeBucket = 0;
    }
    dump("debug", "loadBucket(): After bucket modification: ");

    return;
}

// NOTE: pLVKey is not currently used, but can come in as null.
void WRKQE::lock(const LVKey* pLVKey, const char* pMethod)
{
    stringstream errorText;

    if (!transferQueueIsLocked())
    {
        // NOTE: We must obtain the lock before we verify the lock protocol.
        //       Otherwise, the issuingWorkItem check will fail...
        pthread_mutex_lock(&lock_transferqueue);
        transferQueueLocked = pthread_self();

        // Verify lock protocol
        if (issuingWorkItem)
        {
            FL_Write(FLError, lockPV_TQLock, "WRKQE::lock: Transfer queue lock being obtained while a work item is being issued",0,0,0,0);
            errorText << "WRKQE::lock: Transfer queue lock being obtained while a work item is being issued";
            LOG_ERROR_TEXT_AND_RAS(errorText, bb.internal.lockprotocol.locktq)
            endOnError();
        }

        if (strstr(pMethod, "%") == NULL)
        {
            if (l_LockDebugLevel == "info")
            {
                LOG(bb,info) << "TRNFR_Q:   LOCK <- " << pMethod << ", jobid " << jobid << ", " << lvKey;
            }
            else
            {
                LOG(bb,debug) << "TRNFR_Q:   LOCK <- " << pMethod << ", jobid " << jobid << ", " << lvKey;
            }
        }

        pid_t tid = syscall(SYS_gettid);  // \todo eventually remove this.  incurs syscall for each log entry
        FL_Write(FLMutex, lockTransferQ, "lockTransfer.  threadid=%ld",tid,0,0,0);
    }
    else
    {
        FL_Write(FLError, lockTransferQERROR, "lockTransferQueue called when lock already owned by thread",0,0,0,0);
        flightlog_Backtrace(__LINE__);
        // For now, also to the console...
        LOG(bb,error) << "TRNFR_Q: Request made to lock the transfer queue by " << pMethod << ", but the lock is already owned.";
        logBacktrace();
    }

    return;
}

double WRKQE::processBucket(BBTagID& pTagId, ExtentInfo& pExtentInfo)
{
    double l_Delay = 0;
    BBLV_Info* l_LV_Info = 0;
    BBTransferDef* l_TransferDef = 0;
    bool l_MadeBucketModification = false;

    if (rate)
    {
        // Must be a throttled work queue...
        if (getWrkQ_Size())
        {
            Extent* l_Extent = pExtentInfo.getExtent();
            LOG(bb,debug) << "processBucket(): For extent with length " << l_Extent->getLength();
            dump("debug", "processBucket(): Before bucket modification: ");

            // NOTE: The code below is very similar to the code in transferExtent() in xfer.cc.  The code there
            //       logs messages if an extent is not going to be transferred for some reason.  Here, we simply
            //       want to check for those similar conditions, determining if we should decrement the bucket value.
            l_LV_Info = getLV_Info();
            if ((!(l_Extent->isCP_Transfer())) && l_Extent->getLength())
            {
                BBTagInfo* l_TagInfo = pExtentInfo.getTagInfo();
                if (l_TagInfo)
                {
                    if (!l_LV_Info->stageOutEnded())
                    {
                        if (!(l_LV_Info->resizeLogicalVolumeDuringStageOut() && l_LV_Info->stageOutStarted() && (l_Extent->flags & BBI_TargetSSD)))
                        {
                            l_TransferDef = pExtentInfo.transferDef;
                            if (!l_TransferDef->failed())
                            {
                                if (!l_TagInfo->canceled())
                                {
                                    //  Handle not marked as canceled
                                    if (!l_TransferDef->canceled())
                                    {
                                        // This extent will be transferred...
                                        // NOTE: We are guaranteed that this work queue is throttled, so
                                        //       even if this bucket has a value of zero, it must be decremented
                                        uint64_t l_Length = (uint64_t)(l_Extent->getLength());
                                        if (bucket >= 0)
                                        {
                                            // No delay
                                            l_MadeBucketModification = true;
                                            bucket -= (int64_t)l_Length;
                                        }
                                        else
                                        {
                                            // With delay
                                            l_Delay = max((Throttle_TimeInterval-Throttle_Timer.getCurrentElapsedTimeInterval())*1000000,(double)0);
                                            LOG(bb,debug) << "processBucket(): Delay for " << (float)l_Delay/1000000.0 << " seconds";
                                        }
                                        if (bucket < 0)
                                        {
                                            workQueueReturnedWithNegativeBucket = 1;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            // All extents have been processed.  Set the bucket value to zero.
            if (bucket)
            {
                bucket = 0;
            }
        }
        setThrottleWait(0);
    }

    if (l_MadeBucketModification)
    {
        dump("debug", "processBucket(): After bucket modification: ");
    }
    else
    {
        dump("debug", "processBucket(): No bucket modification: ");
    }

    return l_Delay;
}

void WRKQE::removeWorkItem(WorkID& pWorkItem, const bool pValidateQueue)
{
    // NOTE: Unless we are debugging a problem, pValidateQueue always comes in as false.
    //       Therefore, we never hit the endOnError() below in production...
    if (pValidateQueue && this != HPWrkQE)
    {
        // NOTE:  This method exists to make sure that the number of work queue entries
        //        matches the number of entries in the vector of extents to transfer
        //        for this LVKey when we are removing the first entry from the work queue.
        LVKey l_Key = (wrkq->front()).getLVKey();
        if (getWrkQ_Size() != getLV_Info()->getNumberOfExtents())
        {
            LOG(bb,error) << "WRKQE::removeWorkItem(): Mismatch between number of elements on work queue (" << getWrkQ_Size() << ") and number of extents in the vector of extents (" << getLV_Info()->getNumberOfExtents() << ") to transfer for " << l_Key;
            endOnError();
        }
    }

    pWorkItem = wrkq->front();
    wrkq->pop();

    return;
};

void WRKQE::setIssuingWorkItem(const int pValue)
{
    issuingWorkItem = pValue;

    return;
}

// NOTE: pLVKey is not currently used, but can come in as null.
void WRKQE::unlock(const LVKey* pLVKey, const char* pMethod)
{
    stringstream errorText;

    if (transferQueueIsLocked())
    {
        // Verify lock protocol
        if (issuingWorkItem)
        {
            FL_Write(FLError, lockPV_TQUnlock, "WRKQE::unlock: Transfer queue lock being released while a work item is being issued",0,0,0,0);
            errorText << "WRKQE::unlock: Transfer queue lock being released while a work item is being issued";
            LOG_ERROR_TEXT_AND_RAS(errorText, bb.internal.lockprotocol.unlocktq)
            endOnError();
        }

        pid_t tid = syscall(SYS_gettid);  // \todo eventually remove this.  incurs syscall for each log entry
        FL_Write(FLMutex, unlockTransferQ, "unlockTransfer.  threadid=%ld",tid,0,0,0);

        if (strstr(pMethod, "%") == NULL)
        {
            if (l_LockDebugLevel == "info")
            {
                LOG(bb,info) << "TRNFR_Q: UNLOCK <- " << pMethod << ", jobid " << jobid << ", " << lvKey;
            }
            else
            {
                LOG(bb,debug) << "TRNFR_Q: UNLOCK <- " << pMethod << ", jobid " << jobid << ", " << lvKey;
            }
        }

        transferQueueLocked = 0;
        pthread_mutex_unlock(&lock_transferqueue);
    }
    else
    {
        FL_Write(FLError, unlockTransferQERROR, "unlockTransferQueue called when lock not owned by thread",0,0,0,0);
        flightlog_Backtrace(__LINE__);
        // For now, also to the console...
        LOG(bb,error) << "TRNFR_Q: Request made to unlock the transfer queue by " << pMethod << ", but the lock is not owned.";
        logBacktrace();
    }

    return;
}
