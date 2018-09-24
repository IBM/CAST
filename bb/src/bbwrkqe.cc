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

#include "bbinternal.h"
#include "BBLV_Info.h"
#include "BBLV_Metadata.h"
#include "bbwrkqe.h"
#include "Extent.h"
#include "ExtentInfo.h"
#include "util.h"


void WRKQE::addWorkItem(WorkID& pWorkItem, const bool pValidateQueue)
{
    wrkq->push(pWorkItem);
    // NOTE: Unless we are debugging a problem, pValidateQueue always comes in as false.
    //       Therefore, we never hit the abort() below in production...
    if (pValidateQueue && this != HPWrkQE)
    {
        // NOTE:  This method exists to make sure that the number of work queue entries
        //        matches the number of entries in the vector of extents to transfer
        //        for this LVKey when we are adding an entry to the work queue.
        LVKey l_Key = pWorkItem.getLVKey();
        BBTagInfo2* l_TagInfo2 = metadata.getTagInfo2(&l_Key);
        if (getWrkQ_Size() != l_TagInfo2->getNumberOfExtents())
        {
            LOG(bb,error) << "WRKQE::addWorkItem(): Mismatch between number of elements on work queue (" << getWrkQ_Size() << ") and number of extents in the vector of extents (" << l_TagInfo2->getNumberOfExtents() << ") to transfer for " << l_Key;
            // \todo - Need to figure out what to do here for production...  @DLH
            abort();
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
    string l_Suspended = (suspended ? "Y" : "N");
    string l_Output = "Job " + l_JobId + ", Susp " + l_Suspended;

    if (wrkq)
    {
        if (getWrkQ_Size())
        {
            BBTagInfo2* l_WorkItemTagInfo2 = metadata.getTagInfo2(&lvKey);
            if (l_WorkItemTagInfo2)
            {
                // NOTE: The high priority work queue will not fall into this leg...
                ExtentInfo l_ExtentInfo = l_WorkItemTagInfo2->getNextExtentInfo();
                l_JobStepId = to_string(l_ExtentInfo.getTransferDef()->getJobStepId());
                l_Handle = to_string(l_ExtentInfo.getHandle());
                l_ContribId = to_string(l_ExtentInfo.getContrib());
                l_Rate = to_string(rate);
                l_Bucket = to_string(bucket);
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
        if (l_Rate.size())
        {
            l_Output += ", Rate " + l_Rate;
        }
        if (l_Bucket.size())
        {
            l_Output += ", Bkt " + l_Bucket;
        }

        if (!strcmp(pSev,"debug"))
        {
            LOG(bb,debug) << pPrefix << lvKey << ", " << l_Output << ", #Items " << getNumberOfWorkItems() << ", #Proc'd " << getNumberOfWorkItemsProcessed() << ", CurSize " << getWrkQ_Size();
        }
        else if (!strcmp(pSev,"info"))
        {
            LOG(bb,info) << pPrefix << lvKey << ", " << l_Output << ", #Items " << getNumberOfWorkItems() << ", #Proc'd " << getNumberOfWorkItemsProcessed() << ", CurSize " << getWrkQ_Size();
        }
    }
    else
    {
        LOG(bb,error) << pPrefix << lvKey << ", " << l_Output << ", #Items " << getNumberOfWorkItems() << ", #Proc'd " << getNumberOfWorkItemsProcessed() << ", wrkq is NULL";
    }

    return;
}

void WRKQE::loadBucket()
{
    dump("debug", "loadBucket(): Before bucket modification: ");
    bucket = (int64_t)(MIN(bucket+rate, rate));
    dump("debug", "loadBucket(): After bucket modification: ");

    return;
}

double WRKQE::processBucket(BBTagInfo2* pTagInfo2, BBTagID& pTagId, ExtentInfo& pExtentInfo)
{
    double l_Delay = 0;
    BBTransferDef* l_TransferDef = 0;
    bool l_MadeBucketModification = false;

    if (rate)
    {
        Extent* l_Extent = pExtentInfo.getExtent();
        LOG(bb,debug) << "processBucket(): For extent with length " << l_Extent->getLength();
        dump("debug", "processBucket(): Before bucket modification: ");

        // NOTE: The code below is very similar to the code in transferExtent() in xfer.cc.  The code there
        //       logs messages if an extent is not going to be transferred for some reason.  Here, we simply
        //       want to check for those similar conditions.
        if (pTagInfo2)
        {
            if ((!(l_Extent->isCP_Transfer())) && l_Extent->getLength())
            {
                BBTagInfo* l_TagInfo = pTagInfo2->getTagInfo(pTagId);
                if (l_TagInfo)
                {
                    if (!pTagInfo2->stageOutEnded())
                    {
                        if (!(pTagInfo2->resizeLogicalVolumeDuringStageOut() && pTagInfo2->stageOutStarted() && (l_Extent->flags & BBI_TargetSSD)))
                        {
                            l_TransferDef = pExtentInfo.transferDef;
                            if (!l_TransferDef->failed())
                            {
                                if (!l_TagInfo->failed())
                                {
                                    //  Handle not marked as failed...
                                    if (!l_TagInfo->canceled())
                                    {
                                        //  Handle not marked as canceled
                                        if (!l_TransferDef->canceled())
                                        {
                                            //  This extent will be transferred...
                                            uint64_t l_Length = (uint64_t)(l_Extent->getLength());
                                            if (bucket > 0)
                                            {
                                                l_MadeBucketModification = true;
                                                bucket -= (int64_t)l_Length;
                                            }
                                            else
                                            {
                                                l_Delay = max((Throttle_TimeInterval-Throttle_Timer.getCurrentElapsedTimeInterval())*1000000,(double)0);
                                                LOG(bb,debug) << "processBucket(): Delay for " << (float)l_Delay/1000000.0 << " seconds";
                                            }
                                       }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
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
    //       Therefore, we never hit the abort() below in production...
    if (pValidateQueue && this != HPWrkQE)
    {
        // NOTE:  This method exists to make sure that the number of work queue entries
        //        matches the number of entries in the vector of extents to transfer
        //        for this LVKey when we are removing the first entry from the work queue.
        LVKey l_Key = (wrkq->front()).getLVKey();
        BBTagInfo2* l_TagInfo2 = metadata.getTagInfo2(&l_Key);
        if (getWrkQ_Size() != l_TagInfo2->getNumberOfExtents())
        {
            LOG(bb,error) << "WRKQE::removeWorkItem(): Mismatch between number of elements on work queue (" << getWrkQ_Size() << ") and number of extents in the vector of extents (" << l_TagInfo2->getNumberOfExtents() << ") to transfer for " << l_Key;
            // \todo - Need to figure out what to do here for production...  @DLH
            abort();
        }
    }

    pWorkItem = wrkq->front();
    wrkq->pop();

    return;
};
