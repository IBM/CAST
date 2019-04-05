/*******************************************************************************
 |    bbwrkqmgr.cc
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

#include <boost/filesystem.hpp>
#include <sys/stat.h>
#include <sys/syscall.h>

#include <stdio.h>
#include <string.h>

#include "time.h"

#include "bbinternal.h"
#include "BBLV_Info.h"
#include "BBLV_Metadata.h"
#include "bbserver_flightlog.h"
#include "bbwrkqmgr.h"
#include "identity.h"
#include "tracksyscall.h"
#include "Uuid.h"

namespace bfs = boost::filesystem;

FL_SetName(FLError, "Errordata flightlog")
FL_SetSize(FLError, 16384)

FL_SetName(FLAsyncRqst, "Async Request Flightlog")
FL_SetSize(FLAsyncRqst, 16384)

#ifndef GPFS_SUPER_MAGIC
#define GPFS_SUPER_MAGIC 0x47504653
#endif


/*
 * Static data
 */
static int asyncRequestFile_ReadSeqNbr = 0;
static FILE* asyncRequestFile_Read  = (FILE*)0;
static LVKey LVKey_Null = LVKey();


/*
 * Helper methods
 */
int isGpfsFile(const char* pFileName, bool& pValue)
{
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;
    stringstream errorText;

    pValue = false;
    struct statfs l_Statbuf;

    bfs::path l_Path(pFileName);
    rc = statfs(pFileName, &l_Statbuf);
    while ((rc) && ((errno == ENOENT)))
    {
        l_Path = l_Path.parent_path();
        if (l_Path.string() == "")
        {
            break;
        }
        rc = statfs(l_Path.string().c_str(), &l_Statbuf);
    }

    if (rc)
    {
        FL_Write(FLServer, StatfsFailedGpfs, "Statfs failed", 0, 0 ,0, 0);
        errorText << "Unable to statfs file " << l_Path.string();
        LOG_ERROR_TEXT_ERRNO(errorText, errno);
    }

    if((l_Statbuf.f_type == GPFS_SUPER_MAGIC))
    {
        pValue = true;
    }

    FL_Write(FLServer, Statfs_isGpfsFile, "rc=%ld, isGpfsFile=%ld, magic=%lx", rc, pValue, l_Statbuf.f_type, 0);

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}


/*
 * Static methods
 */
string HeartbeatEntry::getHeartbeatCurrentTime()
{
	char l_Buffer[20] = {'\0'};
	char l_ReturnTime[27] = {'\0'};

    timeval l_CurrentTime;
    gettimeofday(&l_CurrentTime, NULL);
    unsigned long l_Micro = l_CurrentTime.tv_usec;

    // localtime is not thread safe
    // NOTE:  No spaces in the formatted timestamp, because this can be sent as part of an
    //        async request message.  That processing uses scanf to parse the data...
    strftime(l_Buffer, sizeof(l_Buffer), "%Y-%m-%d_%H:%M:%S", localtime((const time_t*)&l_CurrentTime.tv_sec));
    snprintf(l_ReturnTime, sizeof(l_ReturnTime), "%s.%06lu", l_Buffer, l_Micro);

    return string(l_ReturnTime);
}


/*
 * Non-static methods
 */

void WRKQMGR::addHPWorkItem(LVKey* pLVKey, BBTagID& pTagId)
{
    // Build the high priority work item
    WorkID l_WorkId(*pLVKey, pTagId);

    // Push the work item onto the HP work queue and post
    l_WorkId.dump("debug", "addHPWorkItem() ");
    HPWrkQE->addWorkItem(l_WorkId, DO_NOT_VALIDATE_WORK_QUEUE);
    WRKQMGR::post();

    return;
}

int WRKQMGR::addWrkQ(const LVKey* pLVKey, const uint64_t pJobId, const int pSuspendIndicator)
{
    int rc = 0;

    stringstream l_Prefix;
    l_Prefix << " - addWrkQ() before adding " << *pLVKey << " for jobid " << pJobId << ", suspend indicator " << pSuspendIndicator;
    wrkqmgr.dump("debug", l_Prefix.str().c_str(), DUMP_UNCONDITIONALLY);

    std::map<LVKey,WRKQE*>::iterator it = wrkqs.find(*pLVKey);
    if (it == wrkqs.end())
    {
        WRKQE* l_WrkQE = new WRKQE(pLVKey, pJobId, pSuspendIndicator);
        l_WrkQE->setDumpOnRemoveWorkItem(config.get("bb.bbserverDumpWorkQueueOnRemoveWorkItem", DEFAULT_DUMP_QUEUE_ON_REMOVE_WORK_ITEM));
        wrkqs.insert(std::pair<LVKey,WRKQE*>(*pLVKey, l_WrkQE));
    }
    else
    {
        rc = -1;
        stringstream errorText;
        errorText << " Failure when attempting to add workqueue for " << *pLVKey << " for jobid " << pJobId;
        wrkqmgr.dump("info", errorText.str().c_str(), DUMP_UNCONDITIONALLY);
        LOG_ERROR_TEXT_RC(errorText, rc);
    }

    l_Prefix << " - addWrkQ() after adding " << *pLVKey << " for jobid " << pJobId;
    wrkqmgr.dump("debug", l_Prefix.str().c_str(), DUMP_UNCONDITIONALLY);

    return rc;
}

int WRKQMGR::appendAsyncRequest(AsyncRequest& pRequest)
{
    int rc = 0;
    size_t l_Size;

    LOG(bb,debug) << "appendAsyncRequest(): Attempting an append to the async request file...";

    uid_t l_Uid = getuid();
    gid_t l_Gid = getgid();

  	becomeUser(0, 0);

    MAINTENANCE_OPTION l_Maintenance = NO_MAINTENANCE;
    char l_Cmd[AsyncRequest::MAX_DATA_LENGTH] = {'\0'};
    int rc2 = sscanf(pRequest.getData(), "%s", l_Cmd);
    if (rc2 == 1 && strstr(l_Cmd, "heartbeat"))
    {
        // NOTE:  Only perform async request file pruning maintenance
        //        for a heartbeat operation, which is only every 5 minutes...
        l_Maintenance = MINIMAL_MAINTENANCE;
    }

    int l_Retry = DEFAULT_RETRY_VALUE;
    while (l_Retry--)
    {
        rc = 0;
        l_Size = 0;
        try
        {
            stringstream errorText;

            int l_SeqNbr = 0;
            FILE* fd = openAsyncRequestFile("ab", l_SeqNbr, l_Maintenance);
            if (fd != NULL)
            {
                char l_Buffer[sizeof(AsyncRequest)+1] = {'\0'};
                pRequest.str(l_Buffer, sizeof(l_Buffer));

                threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::fwritesyscall, fd, __LINE__, sizeof(AsyncRequest));
                l_Size = ::fwrite(l_Buffer, sizeof(char), sizeof(AsyncRequest), fd);
                threadLocalTrackSyscallPtr->clearTrack();
                FL_Write(FLAsyncRqst, Append, "Append to async request file having seqnbr %ld for %ld bytes. File pointer %p, %ld bytes written.", l_SeqNbr, sizeof(AsyncRequest), (uint64_t)(void*)fd, l_Size);

                if (l_Size == sizeof(AsyncRequest))
                {
                    l_Retry = 0;
                }
                else
                {
                    // NOTE: If the size written is anything but zero, we are in a world of hurt.
                    //       Current async request file would now be hosed.
                    FL_Write(FLAsyncRqst, AppendFail, "Failed fwrite() request, sequence number %ld. Wrote %ld bytes, expected %ld bytes, ferror %ld.", l_SeqNbr, l_Size, sizeof(AsyncRequest), ferror(fd));
                    if (l_Size)
                    {
                        // Don't retry if a partial result was written
                        // \todo - What to do...  @@DLH
                        l_Retry = 0;
                    }
                    errorText << "appendAsyncRequest(): Failed fwrite() request, sequence number " << l_SeqNbr << ". Wrote " << l_Size \
                              << " bytes, expected " << sizeof(AsyncRequest) << " bytes." \
                              << (l_Retry ? " Request will be retried." : "");
                    LOG_ERROR_TEXT_ERRNO(errorText, ferror(fd));
                    clearerr(fd);
                    rc = -1;
                }
                FL_Write(FLAsyncRqst, CloseAfterAppend, "Close for async request file having seqnbr %ld, file pointer %p.", l_SeqNbr, (uint64_t)(void*)fd, 0, 0);
                ::fclose(fd);
                fd = NULL;
            }
            else
            {
                errorText << "appendAsyncRequest(): Failed open request, sequence number " << l_SeqNbr \
                             << (l_Retry ? ". Request will be retried." : "");
                LOG_ERROR_TEXT_ERRNO(errorText, errno);
                rc = -1;
            }
        }
        catch(exception& e)
        {
            LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
        }
    }

    // Reset the heartbeat timer count...
    heartbeatTimerCount = 0;

  	becomeUser(l_Uid, l_Gid);

    if (!rc)
    {
        if (strstr(pRequest.data, "heartbeat"))
        {
            LOG(bb,debug) << "appendAsyncRequest(): Host name " << pRequest.hostname << " => " << pRequest.data;
        }
        else
        {
            LOG(bb,info) << "appendAsyncRequest(): Host name " << pRequest.hostname << " => " << pRequest.data;
        }
    }
    else
    {
        LOG(bb,error) << "appendAsyncRequest(): Could not append to the async request file. Failing append was: Host name " << pRequest.hostname << " => " << pRequest.data;
    }

    return rc;
}

void WRKQMGR::calcThrottleMode()
{
    int l_NewThrottleMode = 0;
    for (map<LVKey,WRKQE*>::iterator qe = wrkqs.begin(); qe != wrkqs.end(); ++qe)
    {
        if (qe->second->getRate())
        {
            l_NewThrottleMode = 1;
            break;
        }
    }

    if (throttleMode != l_NewThrottleMode)
    {
        LOG(bb,info) << "calcThrottleMode(): Throttle mode changing from " << (throttleMode ? "true" : "false") << " to " << (l_NewThrottleMode ? "true" : "false");
        if (l_NewThrottleMode)
        {
            Throttle_Timer.forcePop();
            loadBuckets();
        }

        throttleMode = l_NewThrottleMode;
    }

    return;
}

uint64_t WRKQMGR::checkForNewHPWorkItems()
{
    uint64_t l_CurrentNumber = HPWrkQE->getNumberOfWorkItems();
    uint64_t l_NumberAdded = 0;

    int l_AsyncRequestFileSeqNbr = 0;
    int64_t l_OffsetToNextAsyncRequest = 0;

    int rc = findOffsetToNextAsyncRequest(l_AsyncRequestFileSeqNbr, l_OffsetToNextAsyncRequest);
    if (!rc)
    {
        if (l_AsyncRequestFileSeqNbr > 0 && l_OffsetToNextAsyncRequest >= 0)
        {
            bool l_FirstFile = true;
            int l_CurrentAsyncRequestFileSeqNbr = 0;
            uint64_t l_CurrentOffsetToNextAsyncRequest = 0;
            uint64_t l_TargetOffsetToNextAsyncRequest = 0;
            getOffsetToNextAsyncRequest(l_CurrentAsyncRequestFileSeqNbr, l_CurrentOffsetToNextAsyncRequest);
            while (l_CurrentAsyncRequestFileSeqNbr <= l_AsyncRequestFileSeqNbr)
            {
                l_TargetOffsetToNextAsyncRequest = (l_CurrentAsyncRequestFileSeqNbr < l_AsyncRequestFileSeqNbr ? MAXIMUM_ASYNC_REQUEST_FILE_SIZE : (uint64_t)l_OffsetToNextAsyncRequest);
                if (!l_FirstFile)
                {
                    // We crossed the boundary to a new async request file...  Start at offset zero...
                    l_CurrentOffsetToNextAsyncRequest = 0;
                    LOG(bb,info) << "Starting to process async requests from async request file sequence number " << l_CurrentAsyncRequestFileSeqNbr;
                }
                while (l_CurrentOffsetToNextAsyncRequest < l_TargetOffsetToNextAsyncRequest)
                {
                    // Enqueue the data items to the high priority work queue
                    const string l_ConnectionName = "None";
                    LVKey l_LVKey = std::pair<string, Uuid>(l_ConnectionName, Uuid(HP_UUID));
                    size_t l_Offset = l_CurrentOffsetToNextAsyncRequest;
                    BBTagID l_TagId(BBJob(), l_Offset);

                    // Build/push the work item onto the HP work queue and post
                    addHPWorkItem(&l_LVKey, l_TagId);
                    l_CurrentOffsetToNextAsyncRequest += sizeof(AsyncRequest);
                    ++l_NumberAdded;
                }
                l_FirstFile = false;
                ++l_CurrentAsyncRequestFileSeqNbr;
            }

            if (l_NumberAdded)
            {
                // NOTE:  Set the file seqnbr/offset to that of the request file we obtained above,
                //        as we have now enqueued everything up to this point from that request file.
                wrkqmgr.setOffsetToNextAsyncRequest(l_AsyncRequestFileSeqNbr, l_OffsetToNextAsyncRequest);
                LOG(bb,debug) << "checkForNewHPWorkItems(): Found " << l_NumberAdded << " new async requests";
            }
        }
        else
        {
            LOG(bb,error) << "Error occured when attempting to read the cross bbserver async request file, l_AsyncRequestFileSeqNbr = " \
                          << l_AsyncRequestFileSeqNbr << ", l_OffsetToNextAsyncRequest = " << l_OffsetToNextAsyncRequest;
        }
    }
    else
    {
        LOG(bb,error) << "Error occured when attempting to read the cross bbserver async request file, rc = " << rc;
    }

    return l_CurrentNumber + l_NumberAdded;
}

void WRKQMGR::checkThrottleTimer()
{
    if (Throttle_Timer.popped(Throttle_TimeInterval))
    {
        LOG(bb,off) << "WRKQMGR::checkThrottleTimer(): Popped";

        // Check/add new high priority work items from the cross bbserver metadata
        checkForNewHPWorkItems();

        // See if it is time to dump the work manager
        if (dumpTimerPoppedCount && (++dumpTimerCount >= dumpTimerPoppedCount))
        {
            dump("info", " Work Queue Mgr (Not an error - Timer Interval)", DUMP_ALWAYS);
        }

        // See if it is time to reload the work queue throttle buckets
        if (++throttleTimerCount >= throttleTimerPoppedCount)
        {
            // If any workqueue in throttle mode, load the buckets
            if (inThrottleMode())
            {
                loadBuckets();
            }
            throttleTimerCount = 0;
            setDelayMessageSent(false);
        }

        // See if it is time to have a heartbeat for this bbServer
        if (++heartbeatTimerCount >= heartbeatTimerPoppedCount)
        {
            // Tell the world this bbServer is still alive...
            char l_AsyncCmd[AsyncRequest::MAX_DATA_LENGTH] = {'\0'};
            string l_CurrentTime = HeartbeatEntry::getHeartbeatCurrentTime();
            snprintf(l_AsyncCmd, sizeof(l_AsyncCmd), "heartbeat 0 0 0 0 0 None %s", l_CurrentTime.c_str());
            AsyncRequest l_Request = AsyncRequest(l_AsyncCmd);
            appendAsyncRequest(l_Request);
        }

        // See if it is time to dump the heartbeat information
        if (heartbeatDumpPoppedCount && (++heartbeatDumpCount >= heartbeatDumpPoppedCount))
        {
            dumpHeartbeatData("info");
        }
    }

    return;
}

int WRKQMGR::createAsyncRequestFile(const char* pAsyncRequestFileName)
{
    int rc = 0;

    char l_Cmd[PATH_MAX+64] = {'\0'};
    snprintf(l_Cmd, sizeof(l_Cmd), "touch %s 2>&1;", pAsyncRequestFileName);

    for (auto&l_Line : runCommand(l_Cmd)) {
        // No expected output...
        LOG(bb,error) << l_Line;
        rc = -1;
    }

    if (!rc)
    {
        // NOTE:  This could be slightly misleading.  We have a race condition between multiple
        //        bbServers possibly attempting to create the same named async request file.
        //        But, touch is being used.  So if the file were to be created just before this bbServer
        //        attempts to create it, all should still be OK with just the last reference dates
        //        being updated.  However, both bbServers will claim to have created the file.
        LOG(bb,info) << "WRKQMGR: New async request file " << pAsyncRequestFileName << " created";
    }

    return rc;
}

void WRKQMGR::dump(const char* pSev, const char* pPostfix, DUMP_OPTION pDumpOption) {
    // NOTE: We early exit based on the logging level because we don't want to 'reset'
    //       dump counters, etc. if the logging facility filters out an entry.
    if (pSev == loggingLevel)
    {
        const char* l_PostfixOverride = " Work Queue Mgr (Not an error - Skip Interval)";
        char* l_PostfixStr = const_cast<char*>(pPostfix);

        if (allowDump)
        {
            if (pDumpOption == DUMP_UNCONDITIONALLY || pDumpOption == DUMP_ALWAYS || inThrottleMode())
            {
                bool l_DumpIt = false;
                if (pDumpOption != DUMP_UNCONDITIONALLY)
                {
                    if (numberOfWorkQueueItemsProcessed != lastDumpedNumberOfWorkQueueItemsProcessed)
                    {
                        l_DumpIt = true;
                    }
                    else
                    {
                        if (numberOfAllowedSkippedDumpRequests && numberOfSkippedDumpRequests >= numberOfAllowedSkippedDumpRequests)
                        {
                            l_DumpIt = true;
                            l_PostfixStr = const_cast<char*>(l_PostfixOverride);
                        }
                    }
                }
                else
                {
                    l_DumpIt = true;
                }

                if (l_DumpIt)
                {
                    stringstream l_OffsetStr;
                    if (outOfOrderOffsets.size())
                    {
                        // Build an output stream for the out of order async request offsets...
                        l_OffsetStr << "(";
                        size_t l_NumberOfOffsets = outOfOrderOffsets.size();
                        for(size_t i=0; i<l_NumberOfOffsets; ++i)
                        {
                            if (i!=l_NumberOfOffsets-1) {
                                l_OffsetStr << hex << uppercase << setfill('0') << outOfOrderOffsets[i] << setfill(' ') << nouppercase << dec << ",";
                            } else {
                                l_OffsetStr << hex << uppercase << setfill('0') << outOfOrderOffsets[i] << setfill(' ') << nouppercase << dec;
                            }
                        }
                        l_OffsetStr << ")";
                    }

                    int l_CheckForCanceledExtents = checkForCanceledExtents;
                    if (!strcmp(pSev,"debug"))
                    {
                        LOG(bb,debug) << ">>>>> Start: WRKQMGR" << l_PostfixStr << " <<<<<";
//                        LOG(bb,debug) << "                 Throttle Mode: " << (throttleMode ? "true" : "false") << "  TransferQueue Locked: " << (transferQueueLocked ? "true" : "false");
                        LOG(bb,debug) << "                 Throttle Mode: " << (throttleMode ? "true" : "false") << "  Number of Workqueue Items Processed: " << numberOfWorkQueueItemsProcessed \
                                      << "  Check Canceled Extents: " << (l_CheckForCanceledExtents ? "true" : "false") << "  Snoozing: " << (Throttle_Timer.isSnoozing() ? "true" : "false");
                        if (l_CheckForCanceledExtents)
                        {
                            LOG(bb,debug) << "      ConcurrentCancelRequests: " << numberOfConcurrentCancelRequests << "  AllowedConcurrentCancelRequests: " << numberOfAllowedConcurrentCancelRequests;
                        }
//                        LOG(bb,debug) << "          Throttle Timer Count: " << throttleTimerCount << "  Throttle Timer Popped Count: " << throttleTimerPoppedCount;
//                        LOG(bb,debug) << "         Heartbeat Timer Count: " << dumpTimerCount << " Heartbeat Timer Popped Count: " << dumpTimerPoppedCount;
//                        LOG(bb,debug) << "          Heartbeat Dump Count: " << heartbeatDumpCount << "  Heartbeat Dump Popped Count: " << heartbeatDumpPoppedCount;
//                        LOG(bb,debug) << "              Dump Timer Count: " << heartbeatTimerCount << "          Dump Timer Popped Count: " << heartbeatTimerPoppedCount;
//                        LOG(bb,debug) << "     Declare Server Dead Count: " << declareServerDeadCount;
                        LOG(bb,debug) << "          Last Queue Processed: " << lastQueueProcessed << "  Last Queue With Entries: " << lastQueueWithEntries;
                        LOG(bb,debug) << "          Async Seq#: " << asyncRequestFileSeqNbr << "  LstOff: 0x" << hex << uppercase << setfill('0') \
                                      << setw(8) << lastOffsetProcessed << "  NxtOff: 0x" << setw(8) << offsetToNextAsyncRequest \
                                      << setfill(' ') << nouppercase << dec << "  #OutOfOrd " << outOfOrderOffsets.size();
                        if (outOfOrderOffsets.size())
                        {
                            LOG(bb,debug) << " Out of Order Offsets (in hex): " << l_OffsetStr.str();
                        }
                        LOG(bb,debug) << "   Number of Workqueue Entries: " << wrkqs.size();
                        for (map<LVKey,WRKQE*>::iterator qe = wrkqs.begin(); qe != wrkqs.end(); ++qe)
                        {
                            qe->second->dump(pSev, "          ");
                        }
                        LOG(bb,debug) << ">>>>>   End: WRKQMGR" << l_PostfixStr << " <<<<<";
                    }
                    else if (!strcmp(pSev,"info"))
                    {
                        LOG(bb,info) << ">>>>> Start: WRKQMGR" << l_PostfixStr << " <<<<<";
//                        LOG(bb,info) << "                 Throttle Mode: " << (throttleMode ? "true" : "false") << "  TransferQueue Locked: " << (transferQueueLocked ? "true" : "false");
                        LOG(bb,info) << "                 Throttle Mode: " << (throttleMode ? "true" : "false") << "  Number of Workqueue Items Processed: " << numberOfWorkQueueItemsProcessed \
                                     << "  Check Canceled Extents: " << (l_CheckForCanceledExtents ? "true" : "false") << "  Snoozing: " << (Throttle_Timer.isSnoozing() ? "true" : "false");
                        if (l_CheckForCanceledExtents)
                        {
                            LOG(bb,info) << "      ConcurrentCancelRequests: " << numberOfConcurrentCancelRequests << "  AllowedConcurrentCancelRequests: " << numberOfAllowedConcurrentCancelRequests;
                        }
//                        LOG(bb,info) << "          Throttle Timer Count: " << throttleTimerCount << "  Throttle Timer Popped Count: " << throttleTimerPoppedCount;
//                        LOG(bb,info) << "         Heartbeat Timer Count: " << dumpTimerCount << " Heartbeat Timer Popped Count: " << dumpTimerPoppedCount;
//                        LOG(bb,info) << "          Heartbeat Dump Count: " << heartbeatDumpCount << "  Heartbeat Dump Popped Count: " << heartbeatDumpPoppedCount;
//                        LOG(bb,info) << "              Dump Timer Count: " << dumpTimerCount << "      Dump Timer Popped Count: " << dumpTimerPoppedCount;
//                        LOG(bb,info) << "     Declare Server Dead Count: " << declareServerDeadCount;
                        LOG(bb,info) << "          Last Queue Processed: " << lastQueueProcessed << "  Last Queue With Entries: " << lastQueueWithEntries;
                        LOG(bb,info) << "          Async Seq#: " << asyncRequestFileSeqNbr << "  LstOff: 0x" << hex << uppercase << setfill('0') \
                                     << setw(8) << lastOffsetProcessed << "  NxtOff: 0x" << setw(8) << offsetToNextAsyncRequest \
                                     << setfill(' ') << nouppercase << dec << "  #OutOfOrd " << outOfOrderOffsets.size();
                        if (outOfOrderOffsets.size())
                        {
                            LOG(bb,info) << " Out of Order Offsets (in hex): " << l_OffsetStr.str();
                        }
                        LOG(bb,info) << "   Number of Workqueue Entries: " << wrkqs.size();
                        for (map<LVKey,WRKQE*>::iterator qe = wrkqs.begin(); qe != wrkqs.end(); ++qe)
                        {
                            qe->second->dump(pSev, "          ");
                        }
                        LOG(bb,info) << ">>>>>   End: WRKQMGR" << l_PostfixStr << " <<<<<";
                    }
                    lastDumpedNumberOfWorkQueueItemsProcessed = numberOfWorkQueueItemsProcessed;
                    numberOfSkippedDumpRequests = 0;

                    // NOTE:  Only reset the dumpTimerCount if we actually dumped the work queue manager.
                    //        Then, if no transfer activity for a while, we will dump the work queue manager
                    //        the next time the throttle timer pops.
                    dumpTimerCount = 0;
                }
                else
                {
                    // Not dumped...
                    ++numberOfSkippedDumpRequests;
                }
            }
        }
    }

    return;
}

void WRKQMGR::dump(queue<WorkID>* l_WrkQ, WRKQE* l_WrkQE, const char* pSev, const char* pPrefix)
{
    char l_Temp[64] = {'\0'};
    if (wrkqs.size() == 1)
    {
        strCpy(l_Temp, " queue exists: ", sizeof(l_Temp));
    }
    else
    {
        strCpy(l_Temp, " queues exist: ", sizeof(l_Temp));
    }

    l_WrkQE->dump(pSev, pPrefix);

    return;
}

void WRKQMGR::dumpHeartbeatData(const char* pSev, const char* pPrefix)
{
    if (heartbeatData.size())
    {
        int i = 1;
        if (!strcmp(pSev,"debug"))
        {
            LOG(bb,debug) << ">>>>> Start: " << (pPrefix ? pPrefix : "") << heartbeatData.size() \
                          << (heartbeatData.size()==1 ? " reporting bbServer <<<<<" : " reporting bbServers <<<<<");
            for (auto it=heartbeatData.begin(); it!=heartbeatData.end(); ++it)
            {
                LOG(bb,debug) << i << ") " << it->first << " -> Count " << (it->second).getCount() \
                              << ", time reported: " << (it->second).getTime() \
                              << ", timestamp from bbServer: " << (it->second).getServerTime();
                ++i;
            }
            LOG(bb,debug) << ">>>>>   End: " << heartbeatData.size() \
                          << (heartbeatData.size()==1 ? " reporting bbServer <<<<<" : " reporting bbServers <<<<<");
        }
        else if (!strcmp(pSev,"info"))
        {
            LOG(bb,info) << ">>>>> Start: " << (pPrefix ? pPrefix : "") << heartbeatData.size() \
                         << (heartbeatData.size()==1 ? " reporting bbServer <<<<<" : " reporting bbServers <<<<<");
            for (auto it=heartbeatData.begin(); it!=heartbeatData.end(); ++it)
            {
                LOG(bb,info) << i << ") " << it->first << " -> Count " << (it->second).getCount() \
                              << ", time reported: " << (it->second).getTime() \
                              << ", timestamp from bbServer: " << (it->second).getServerTime();
                ++i;
            }
            LOG(bb,info) << ">>>>>   End: " << heartbeatData.size() \
                         << (heartbeatData.size()==1 ? " reporting bbServer <<<<<" : " reporting bbServers <<<<<");
        }
    }
    else
    {
        if (!strcmp(pSev,"debug"))
        {
            LOG(bb,debug) << ">>>>>   No other reporting bbServers";
        }
        else if (!strcmp(pSev,"info"))
        {
            LOG(bb,info) << ">>>>>   No other reporting bbServers";
        }
    }

    heartbeatDumpCount = 0;

    return;
}

int WRKQMGR::findOffsetToNextAsyncRequest(int &pSeqNbr, int64_t &pOffset)
{
    int rc = 0;

    pSeqNbr = 0;
    pOffset = 0;

    uid_t l_Uid = getuid();
    gid_t l_Gid = getgid();
    MAINTENANCE_OPTION l_Maintenance = NO_MAINTENANCE;

  	becomeUser(0, 0);

    int l_Retry = DEFAULT_RETRY_VALUE;
    while (l_Retry--)
    {
        rc = 0;
        try
        {
            stringstream errorText;
            FILE* fd = openAsyncRequestFile("rb", pSeqNbr, l_Maintenance);
            if (fd != NULL)
            {
                threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::fseeksyscall, fd, __LINE__);
                rc = ::fseek(fd, 0, SEEK_END);
                threadLocalTrackSyscallPtr->clearTrack();
                FL_Write(FLAsyncRqst, SeekEnd, "Seeking the end of async request file having seqnbr %ld, rc %ld.", pSeqNbr, rc, 0, 0);
                if (!rc)
                {
                    int64_t l_Offset = -1;
                    threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::ftellsyscall, fd, __LINE__);
                    l_Offset = (int64_t)::ftell(fd);
                    threadLocalTrackSyscallPtr->clearTrack();
                    if (l_Offset >= 0)
                    {
                        l_Retry = 0;
                        pOffset = l_Offset;
                        FL_Write(FLAsyncRqst, RtvEndOffsetForFind, "End of async request file with seqnbr %ld is at offset %ld.", pSeqNbr, pOffset, 0, 0);
                        LOG(bb,debug) << "findOffsetToNextAsyncRequest(): SeqNbr: " << pSeqNbr << ", Offset: " << pOffset;
                    }
                    else
                    {
                        FL_Write(FLAsyncRqst, RtvEndOffsetForFindFail, "Failed ftell() request, sequence number %ld, errno %ld.", pSeqNbr, errno, 0, 0);
                        errorText << "findOffsetToNextAsyncRequest(): Failed ftell() request, sequence number " << pSeqNbr \
                                  << (l_Retry ? ". Request will be retried." : "");
                        LOG_ERROR_TEXT_ERRNO(errorText, errno);
                        rc = -1;
                    }
                }
                else
                {
                    FL_Write(FLAsyncRqst, SeekEndFail, "Failed fseek() request for end of file, sequence number %ld, ferror %ld.", pSeqNbr, ferror(fd), 0, 0);
                    errorText << "findOffsetToNextAsyncRequest(): Failed fseek() request for end of file, sequence number " << pSeqNbr \
                              << (l_Retry ? ". Request will be retried." : "");
                    LOG_ERROR_TEXT_ERRNO(errorText, ferror(fd));
                    clearerr(fd);
                    rc = -1;
                }
            }
            else
            {
                errorText << "findOffsetToNextAsyncRequest(): Failed open request, sequence number " << pSeqNbr \
                             << (l_Retry ? ". Request will be retried." : "");
                LOG_ERROR_TEXT_ERRNO(errorText, errno);
                rc = -1;
            }
        }
        catch(exception& e)
        {
            LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
        }

        l_Maintenance = FORCE_REOPEN;
    }

  	becomeUser(l_Uid, l_Gid);

    return rc;
}

int WRKQMGR::findWork(const LVKey* pLVKey, WRKQE* &pWrkQE)
{
    int rc = 0;
    pWrkQE = 0;

    if (pLVKey)
    {
        // Request to return a specific work queue
        rc = getWrkQE(pLVKey, pWrkQE);
    }
    else
    {
        // Next, determine if we need to look for work queues with canceled extents.
        // NOTE: This takes a higher priority than taking any work off the high priority
        //       work queue because in restart cases we want to clear these extents as
        //       quickly as possible.  If we first took work off the high priorty work
        //       queue, it is possible to consume all the transfer threads waiting for
        //       canceled extents to be removed (due to stop transfer) without any
        //       transfer threads available to actually perform the remove of the canceled
        //       extents, yielding deadlock.
        if (getCheckForCanceledExtents())
        {
            // Search for any LVKey work queue with a canceled extent at the front...
            rc = getWrkQE_WithCanceledExtents(pWrkQE);
        }

        if (!rc)
        {
            if (!pWrkQE)
            {
                // No work queue exists with canceled extents.
                // Next, check the high priority work queue...
                // NOTE: If we have high priority work items available and we have reached the maximum number of concurrent
                //       cancel requests, we don't check the contents of the next high priority work item.
                //       We 'assume' that it is a cancel request and wait until at least one additional thread is not
                //       working on a cancel request before dequeuing that high priority work item.
                //       Also, for this case, we are guaranteed to have work on one or more LVKey work queues.
                if (HPWrkQE->getWrkQ()->size() && getNumberOfConcurrentCancelRequests() < getNumberOfAllowedConcurrentCancelRequests())
                {
                    // High priority work exists...  Pass the high priority queue back...
                    pWrkQE = HPWrkQE;
                }
                else
                {
                    // No high priorty work exists that we want to schedule.
                    // Find 'real' work on one of the LVKey work queues...
                    rc = getWrkQE((LVKey*)0, pWrkQE);
                }
            }
            else
            {
                // Currently, rc will always be returned as zero by getWrkQE_WithCanceledExtents().
                // If we get here, at least one work queue has canceled extents...
            }
        }
        else
        {
            // Can't get here today...
        }
    }

    return rc;
}

int WRKQMGR::getAsyncRequest(WorkID& pWorkItem, AsyncRequest& pRequest)
{
    int rc = 0;

    char l_Buffer[sizeof(AsyncRequest)+1] = {'\0'};

    uid_t l_Uid = getuid();
    gid_t l_Gid = getgid();
    MAINTENANCE_OPTION l_Maintenance = NO_MAINTENANCE;

  	becomeUser(0, 0);

    // Default is to open the currrent async request file
    int l_SeqNbr = asyncRequestFileSeqNbr;
    if (pWorkItem.getTag() >= offsetToNextAsyncRequest)
    {
        // We need to open the prior async request file...
        l_SeqNbr -= 1;
    }

    int l_Retry = DEFAULT_RETRY_VALUE;
    while (l_Retry--)
    {
        rc = 0;
        try
        {
            stringstream errorText;

            FILE* fd = openAsyncRequestFile("rb", l_SeqNbr, l_Maintenance);
            if (fd != NULL)
            {
                threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::fseeksyscall, fd, __LINE__);
                rc = ::fseek(fd, pWorkItem.getTag(), SEEK_SET);
                threadLocalTrackSyscallPtr->clearTrack();
                FL_Write(FLAsyncRqst, Position, "Positioning async request file having seqnbr %ld to offset %ld, rc %ld.", l_SeqNbr, pWorkItem.getTag(), rc, 0);
                if (!rc)
                {
                    threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::freadsyscall, fd, __LINE__, sizeof(AsyncRequest), pWorkItem.getTag());
                    size_t l_Size = ::fread(l_Buffer, sizeof(char), sizeof(AsyncRequest), fd);
                    threadLocalTrackSyscallPtr->clearTrack();
                    FL_Write6(FLAsyncRqst, Read, "Read async request file having seqnbr %ld starting at offset %ld for %ld bytes. File pointer %p, %ld bytes read.", l_SeqNbr, pWorkItem.getTag(), sizeof(AsyncRequest), (uint64_t)(void*)fd, l_Size, 0);

                    if (l_Size == sizeof(AsyncRequest))
                    {
                        l_Retry = 0;
                        pRequest = AsyncRequest(l_Buffer, l_Buffer+AsyncRequest::MAX_HOSTNAME_LENGTH);
                    }
                    else
                    {
                        FL_Write6(FLAsyncRqst, ReadFail, "Failed fread() request, sequence number %ld, offset %ld. Read %ld bytes, expected %ld bytes, errno %ld.", l_SeqNbr, pWorkItem.getTag(), l_Size, sizeof(AsyncRequest), errno, 0);
                        errorText << "getAsyncRequest(): Failed fread() request, sequence number " << l_SeqNbr << " for offset " \
                                  << pWorkItem.getTag() << ". Read " << l_Size << " bytes, expected " << sizeof(AsyncRequest) << " bytes." \
                                  << (l_Retry ? " Request will be retried." : "");
                        LOG_ERROR_TEXT_ERRNO(errorText, errno);
                        rc = -1;
                    }
                }
                else
                {
                    FL_Write(FLAsyncRqst, PositionFail, "Failed fseek() request for end of file, sequence number %ld, offset %ld, errno %ld.", l_SeqNbr, pWorkItem.getTag(), ferror(fd), 0);
                    errorText << "getAsyncRequest(): Failed fseek() request, sequence number " << l_SeqNbr << " for offset " << pWorkItem.getTag() \
                              << (l_Retry ? ". Request will be retried." : "");
                    LOG_ERROR_TEXT_ERRNO(errorText, ferror(fd));
                    clearerr(fd);
                    rc = -1;
                }
            }
            else
            {
                errorText << "getAsyncRequest(): Failed open request, sequence number " << l_SeqNbr \
                          << (l_Retry ? ". Request will be retried." : "");
                LOG_ERROR_TEXT_ERRNO(errorText, errno);
                rc = -1;
            }
        }
        catch(exception& e)
        {
            LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
        }

        l_Maintenance = FORCE_REOPEN;
    }

  	becomeUser(l_Uid, l_Gid);

    return rc;
}

int WRKQMGR::getThrottleRate(LVKey* pLVKey, uint64_t& pRate)
{
    int rc = 0;
    pRate = 0;

    std::map<LVKey,WRKQE*>::iterator it = wrkqs.find(*pLVKey);
    if (it != wrkqs.end())
    {
        pRate = it->second->getRate();
    }
    else
    {
        // NOTE: This may be tolerated...  Set rc to -2
        rc = -2;
    }

    return rc;
}

int WRKQMGR::getWrkQE(const LVKey* pLVKey, WRKQE* &pWrkQE)
{
    int rc = 0;

    pWrkQE = 0;
    // NOTE: In the 'normal' case, we many be looking for a non-existent work queue in either
    //       the null or non-null LVKey paths.
    //
    //       If a job is ended, the extents are removed from the appropriate work queue, but the counting
    //       semaphore is not reduced by the number of extents removed.  Therefore, we may be sent into
    //       this routine without any remaining work queue entries for a given work queue, for a workqueue that
    //       no longer exists, or where there are no existing work queues.
    //
    //       In these cases, pWorkQE is always returned as NULL.  If ANY work queue exists with at least
    //       one entry, rc is returned as zero.  If no entry exists on any work queue
    //       (except for the high priority work queue), rc is returned as -1.
    //
    //       A zero return code indicates that even if a work queue entry was not found, a repost should
    //       be performed to the semaphore.  A return code of -1 indicates that a repost is not necessary.
    //
    // NOTE: Suspended work queues are returned by this method.  The reason for this is that an entry can
    //       still be removed from a suspended work queue if the extent is for a canceled transfer definition.
    //       Thus, we must return suspended work queues from this method.

    if (pLVKey == NULL || (pLVKey->second).is_null())
    {
//        verify();
        int64_t l_BucketValue = -1;
        bool l_RecalculateLastQueueWithEntries = (lastQueueWithEntries == LVKey_Null ? true : false);
        // NOTE: The HP work queue is always present, so there must be at least two work queues...
        if (wrkqs.size() > 1)
        {
            // We have one or more workqueues...
            LVKey l_LVKey;
            WRKQE* l_WrkQE = 0;

            bool l_SelectNext = false;
            bool l_FoundFirstPositiveWorkQueueInMap = false;
            bool l_EarlyExit = false;
            for (map<LVKey,WRKQE*>::iterator qe = wrkqs.begin(); ((qe != wrkqs.end()) && (!l_EarlyExit)); ++qe)
            {
                // The value for lastQueueWithEntries is only useful for the NEXT
                // invocation of this method if the work queue returned on THIS invocation
                // was the 'last queue with entries' in the map.  In that case, we immediately
                // return the next 'non-negative bucket' work queue on that next invocation,
                // as that completes the round-robin distribution of returned queues
                // back to the top of the map.
                //
                // Therefore, only if
                //   A) We 'encounter' the queue currently in lastQueueWithEntries
                //      during this search -or-
                //   B) We go through the entire map without seeing the queue
                //      currently in lastQueueWithEntries -or-
                //   C) The value for lastQueueWithEntries is not currently set and
                //      we return a work queue on this invocation
                //      *AND*
                //   D) If returning a work queue entry, it has a non-negative bucket value.
                // If the above it true, we will recalculate/reset the lastQueueWithEntries
                // value.
                //
                // NOTE: If we are returning a work queue with a negative bucket value, we
                //       do not recalculate the lastQueueWithEntries value because we will
                //       go into a delay returning that work queue and then all worker threads
                //       come back to this method as part of finding their next work item.
                //       When this method is then invoked, at least one of the work queues will
                //       have a positive bucket value and then 'normal' logic will determine
                //       if the lastQueueWithEntries value needs to be recalculated.
                //
                // Stated another way, if we didn't encounter the current lastQueueWithEntries
                // value during this search, it can't effect the work queue to be returned
                // on the next invocation of this method.  Thus, it doesn't need to be
                // recalculated at the end of the current invocation of this method.
                if (qe->first == lastQueueWithEntries)
                {
                    l_RecalculateLastQueueWithEntries = true;
                }

                bool l_Switch = false;
                if (qe->second != HPWrkQE)
                {
                    // Not the HP workqueue...
                    if (qe->second->getWrkQ_Size())
                    {
                        // This workqueue has at least one entry...
                        int64_t l_CurrentQueueBucketValue = qe->second->getBucket();
                        if (l_WrkQE)
                        {
                            // We already have a workqueue to return.  Only switch to this
                            // workqueue if the current saved bucket value is not positive
                            // and this workqueue has a better 'bucket' value.
                            // NOTE: The 'switch' logic below is generally only for throttled
                            //       work queues.  Non-throttled work queues are simply
                            //       round-robined.
                            // NOTE: If all of the workqueues end up having a negative bucket
                            //       value, we return the workqueue with the least negative
                            //       bucket value.  We want to 'delay' the smallest amount
                            //       of time.
                            // NOTE: If we end up delaying due to throttling, all threads
                            //       will be delaying on the same workqueue.  However, when the
                            //       delay time has expired, all of the threads will again return
                            //       to this routine to get the 'next' workqueue.
                            //       Returning to get the 'next' workqueue after a throttle delay
                            //       prevents a huge I/O spike for an individual workqueue.
                            if (l_BucketValue <= 0 && l_BucketValue < l_CurrentQueueBucketValue)
                            {
                                l_Switch = true;
                            }
                        }
                        else
                        {
                            // We don't have a workqueue to return.
                            // If we can't find a 'next' WRKQE to return,
                            // we will return this one unless we find a better one...
                            l_Switch = true;
                        }

                        if (l_Switch)
                        {
                            // This is a better workqueue to return if we
                            // can't find a 'next' WRKQE...
                            l_LVKey = qe->first;
                            l_WrkQE = qe->second;
                            l_BucketValue = l_CurrentQueueBucketValue;

                            if (!l_FoundFirstPositiveWorkQueueInMap)
                            {
                                if (l_BucketValue >= 0)
                                {
                                    if (lastQueueProcessed == lastQueueWithEntries)
                                    {
                                        // Return this queue now as it is the next in round robin order
                                        l_EarlyExit = true;
//                                        LOG(bb,info) << "WRKQMGR::getWrkQE(): Early exit, top of map";
                                    }
                                }
                                l_FoundFirstPositiveWorkQueueInMap = true;
                            }
                        }

                        // Return this queue now as it is the next in round robin order
                        if (l_SelectNext && l_CurrentQueueBucketValue >= 0)
                        {
                            // Switch to this workqueue and return it...
                            l_LVKey = qe->first;
                            l_WrkQE = qe->second;
                            l_BucketValue = l_CurrentQueueBucketValue;
                            l_EarlyExit = true;
//                            LOG(bb,info) << "WRKQMGR::getWrkQE(): Early exit, next after last returned";
                        }
                    }

                    if ((!l_SelectNext) && qe->first == lastQueueProcessed)
                    {
                        // Just found the entry we last returned.  Return the next workqueue that has a positive bucket value...
//                        LOG(bb,info) << "WRKQMGR::getWrkQE(): l_SelectNext = true";
                        l_SelectNext = true;
                    }
                }
            }

            if (l_WrkQE)
            {
                // NOTE: We don't update the last queue processed here because our invoker may choose to not take action on our returned
                //       data.  The last queue processed is updated just before an item of work is removed from a queue.
                pWrkQE = l_WrkQE;
                if ((!l_RecalculateLastQueueWithEntries) && (!l_EarlyExit))
                {
                    l_RecalculateLastQueueWithEntries = true;
                }
            }
            else
            {
                // WRKQMGR::getWrkQE(): No extents left on any workqueue
                rc = -1;
//                LOG(bb,info) << "WRKQMGR::getWrkQE(): No extents left on any workqueue";
            }
        }
        else
        {
            // WRKQMGR::getWrkQE(): No workqueue entries exist
            rc = -1;
//            LOG(bb,info) << "WRKQMGR::getWrkQE(): No workqueue entries exist";
        }

        if (!rc)
        {
            if (pWrkQE)
            {
                if (l_RecalculateLastQueueWithEntries && (l_BucketValue >= 0))
                {
                    for (map<LVKey,WRKQE*>::reverse_iterator qe = wrkqs.rbegin(); qe != wrkqs.rend(); ++qe)
                    {
                        if (qe->second->getWrkQ_Size())
                        {
                            // Update the last work with entries
                            setLastQueueWithEntries(qe->first);
                            break;
                        }
                    }
//                    LOG(bb,info) << "WRKQMGR::getWrkQE(): Recalculated lastQueueWithEntries to " << lastQueueWithEntries;
                }
#if 0
                if (l_BucketValue >= 0)
                {
                    dump("info", " Work Queue Mgr (Work queue entry returned with non-negative bucket)", DUMP_ALWAYS);
                }
#endif
            }
            else
            {
                // Update the last work with entries as NONE
                setLastQueueWithEntries(LVKey_Null);
            }
        }
        else
        {
            // Update the last work with entries as NONE
            setLastQueueWithEntries(LVKey_Null);
        }
    }
    else
    {
        std::map<LVKey,WRKQE*>::iterator it = wrkqs.find(*pLVKey);
        if (it != wrkqs.end())
        {
            // NOTE: In this path since a specific LVKey was passed, we do not interrogate the
            //       rate/bucket values.  We simply return the work queue associated with the LVKey.
            pWrkQE = it->second;
        }
        else
        {
            // WRKQMGR::getWrkQE(): Workqueue no longer exists
            //
            // NOTE: rc is returned as a zero in this case.  We do not know if
            //       any other workqueue has an entry...
            LOG(bb,info) << "WRKQMGR::getWrkQE(): Workqueue for " << *pLVKey << " no longer exists";
            dump("info", " Work Queue Mgr (Specific workqueue not found)", DUMP_ALWAYS);
        }
    }

    return rc;
}

int WRKQMGR::getWrkQE_WithCanceledExtents(WRKQE* &pWrkQE)
{
    int rc = 0;
    LVKey l_Key;
    BBLV_Info* l_LV_Info = 0;

    pWrkQE = 0;
    if (wrkqs.size() > 1)
    {
        for (map<LVKey,WRKQE*>::iterator qe = wrkqs.begin(); qe != wrkqs.end(); ++qe)
        {
            // For each of the work queues...
            if (qe->second != HPWrkQE)
            {
                // Not the HP workqueue...
                if (qe->second->wrkq->size())
                {
                    // This workqueue has at least one entry.
                    // Get the LVKey and taginfo2 for this work item...
                    l_Key = (qe->second->getWrkQ()->front()).getLVKey();
                    l_LV_Info = metadata.getLV_Info(&l_Key);
                    if (l_LV_Info && ((l_LV_Info->getNextExtentInfo().getExtent()->isCanceled())))
                    {
                        // Next extent is canceled...  Don't look any further
                        // and simply return this work queue.
                        pWrkQE = qe->second;
                        pWrkQE->dump("info", "getWrkQE_WithCanceledExtents(): Extent being cancelled ");
                        break;
                    }
                }
            }
        }
    }

    if (!pWrkQE)
    {
        // Indicate that we no longer need to look for canceled extents
        setCheckForCanceledExtents(0);
    }

    return rc;
}

void WRKQMGR::loadBuckets()
{
    for (map<LVKey,WRKQE*>::iterator qe = wrkqs.begin(); qe != wrkqs.end(); ++qe)
    {
        if (qe->second != HPWrkQE)
        {
            qe->second->loadBucket();
        }
    }

    return;
}

// NOTE: pLVKey is not currently used, but can come in as null.
void WRKQMGR::lock(const LVKey* pLVKey, const char* pMethod)
{
    if (!transferQueueIsLocked())
    {
        pthread_mutex_lock(&lock_transferqueue);
        if (strstr(pMethod, "%") == NULL)
        {
            if (pLVKey)
            {
                LOG(bb,debug) << "TRNFR_Q:   LOCK <- " << pMethod << ", " << *pLVKey;
            }
            else
            {
                LOG(bb,debug) << "TRNFR_Q:   LOCK <- " << pMethod << ", unknown LVKey";
            }
        }
        transferQueueLocked = pthread_self();

        pid_t tid = syscall(SYS_gettid);  // \todo eventually remove this.  incurs syscall for each log entry
        FL_Write(FLMutex, lockTransferQ, "lockTransfer.  threadid=%ld",tid,0,0,0);
    }
    else
    {
        if (lockPinned)
        {
            // The lock is already owned, but the lock is pinned...  So, all OK...
            LOG(bb,debug) << "TRNFR_Q: Request made to lock the transfer queue by " << pMethod << ", but the lock is already pinned.";
        }
        else
        {
            FL_Write(FLError, lockTransferQERROR, "lockTransferQueue called when lock already owned by thread",0,0,0,0);
            flightlog_Backtrace(__LINE__);
            // For now, also to the console...
            LOG(bb,error) << "TRNFR_Q: Request made to lock the transfer queue by " << pMethod << ", but the lock is already owned.";
            logBacktrace();
        }
    }

    return;
}

void WRKQMGR::manageWorkItemsProcessed(const WorkID& pWorkItem)
{
    // NOTE: This must be for the HPWrkQ.
    // NOTE: The ordering for processing of async requests is NOT guaranteed.
    //       This is because high priority work items are dispatched to multiple
    //       threads and there is no guarantee as to the order of their finish.
    //       Command requests can ensure that all prior high priority requests are
    //       completed before processing for their request starts by invoking the
    //       processAllOutstandingHP_Requests() method.
    //       Therefore, the purpose of this routine is to only record that a given
    //       work item (high priorty request) is complete if all prior work items
    //       are also complete. This processing enforces the promise given by the
    //       processAllOutstandingHP_Requests() method.
    // NOTE: The processing below only works if we assume that the number of
    //       outstanding out of order async requests does not exceed tne number
    //       of async requests that can be contained within a given async request
    //       file.  Otherwise, we would have a duplicate offset in the outOfOrderOffsets
    //       vector.
    //
    // Determine the next offset to be marked as complete
    uint64_t l_TargetOffset = lastOffsetProcessed + sizeof(AsyncRequest);
    if (crossingAsyncFileBoundary(l_TargetOffset))
    {
        // New async request file...  Offset will be zero
        l_TargetOffset = 0;
    }
    if (l_TargetOffset == pWorkItem.getTag())
    {
        // The work item just finished is for the next offset
        incrementNumberOfHP_WorkItemsProcessed(l_TargetOffset);

        // Now see if any work items that came out of order should also be marked as complete
        bool l_AllDone = false;
        while (!l_AllDone)
        {
            l_AllDone = true;
            if (outOfOrderOffsets.size())
            {
                l_TargetOffset = lastOffsetProcessed + sizeof(AsyncRequest);
                if (crossingAsyncFileBoundary(l_TargetOffset))
                {
                    l_TargetOffset = 0;
                }
                LOG(bb,debug) << "manageWorkItemsProcessed(): TargetOffset 0x" << hex << uppercase << setfill('0') << setw(8) << l_TargetOffset << setfill(' ') << nouppercase << dec;
                for (auto it=outOfOrderOffsets.begin(); it!=outOfOrderOffsets.end(); ++it) {
                    if (*it == l_TargetOffset)
                    {
                        l_AllDone = false;
                        outOfOrderOffsets.erase(it);
                        incrementNumberOfHP_WorkItemsProcessed(l_TargetOffset);
                        LOG(bb,info) << "manageWorkItemsProcessed(): Offset 0x" << hex << uppercase << setfill('0') << l_TargetOffset << setfill(' ') << nouppercase << dec << " removed from the outOfOrderOffsets vector";
                        break;
                    }
                }
            }
        }
    }
    else
    {
        // Work item completed out of order...  Keep it for later processing...
        outOfOrderOffsets.push_back(pWorkItem.getTag());
        LOG(bb,info) << "manageWorkItemsProcessed(): Offset 0x" << hex << uppercase << setfill('0') << pWorkItem.getTag() << setfill(' ') << nouppercase << dec << " pushed onto the outOfOrderOffsets vector";
    }

    return;
}

FILE* WRKQMGR::openAsyncRequestFile(const char* pOpenOption, int &pSeqNbr, const MAINTENANCE_OPTION pMaintenanceOption)
{
    FILE* l_FilePtr = 0;
    char* l_AsyncRequestFileNamePtr = 0;
    stringstream errorText;

    int rc = verifyAsyncRequestFile(l_AsyncRequestFileNamePtr, pSeqNbr, pMaintenanceOption);
    if ((!rc) && l_AsyncRequestFileNamePtr)
    {
        bool l_AllDone = false;
        while (!l_AllDone)
        {
            l_AllDone = true;
            if (pOpenOption[0] != 'a')
            {
                if (pMaintenanceOption != FORCE_REOPEN && pSeqNbr == asyncRequestFile_ReadSeqNbr)
                {
                    l_FilePtr = asyncRequestFile_Read;
                }
            }

            if (l_FilePtr == NULL)
            {
                threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::fopensyscall, l_AsyncRequestFileNamePtr, __LINE__);
                l_FilePtr = ::fopen(l_AsyncRequestFileNamePtr, pOpenOption);
                threadLocalTrackSyscallPtr->clearTrack();
                if (pOpenOption[0] != 'a')
                {
                    if (l_FilePtr != NULL)
                    {
                        FL_Write(FLAsyncRqst, OpenRead, "Open async request file having seqnbr %ld using mode 'rb', maintenance option %ld. File pointer returned is %p.", pSeqNbr, pMaintenanceOption, (uint64_t)(void*)l_FilePtr, 0);
                        // NOTE:  All closes for fds opened for read are done via the cached
                        //        fd that has been stashed in asyncRequestFile_Read.
                        if (asyncRequestFile_Read != NULL)
                        {
                            LOG(bb,debug) << "openAsyncRequestFile(): Close cached file for read";
                            FL_Write(FLAsyncRqst, CloseForRead, "Close async request file having seqnbr %ld using mode 'rb', maintenance option %ld. File pointer is %p.", pSeqNbr, (uint64_t)(void*)asyncRequestFile_Read, pMaintenanceOption, 0);
                            ::fclose(asyncRequestFile_Read);
                            asyncRequestFile_Read = NULL;
                        }
                        LOG(bb,debug) << "openAsyncRequestFile(): Cache open for read, seqnbr " << pSeqNbr;
                        asyncRequestFile_ReadSeqNbr = pSeqNbr;
                        asyncRequestFile_Read = l_FilePtr;
                    }
                    else
                    {
                        FL_Write(FLAsyncRqst, OpenReadFailed, "Open async request file having seqnbr %ld using mode 'rb', maintenance option %ld failed, errno %ld.", pSeqNbr, pMaintenanceOption, errno, 0);
                        errorText << "Open async request file having seqnbr " << pSeqNbr << " using mode 'rb', maintenance option " << pMaintenanceOption << " failed.";
                        LOG_ERROR_TEXT_ERRNO(errorText, errno);
                    }
                }
                else
                {
                    if (l_FilePtr != NULL)
                    {
                        FL_Write(FLAsyncRqst, OpenAppend, "Open async request file having seqnbr %ld using mode 'ab', maintenance option %ld. File pointer returned is %p.", pSeqNbr, pMaintenanceOption, (uint64_t)(void*)l_FilePtr, 0);
                        // Append mode...  Check the file size...
                        threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::ftellsyscall, l_FilePtr, __LINE__);
                        int64_t l_Offset = (int64_t)::ftell(l_FilePtr);
                        threadLocalTrackSyscallPtr->clearTrack();
                        if (l_Offset >= 0)
                        {
                            FL_Write(FLAsyncRqst, RtvEndOffset, "Check if it is time to create a new async request file. Current file has seqnbr %ld, ending offset %ld.", pSeqNbr, l_Offset, 0, 0);
                            if (crossingAsyncFileBoundary(l_Offset))
                            {
                                // Time for a new async request file...
                                delete [] l_AsyncRequestFileNamePtr;
                                l_AsyncRequestFileNamePtr = 0;
                                rc = verifyAsyncRequestFile(l_AsyncRequestFileNamePtr, pSeqNbr, CREATE_NEW_FILE);
                                if (!rc)
                                {
                                    // Close the file...
                                    FL_Write(FLAsyncRqst, CloseForNewFile, "Close async request file having seqnbr %ld using mode 'ab', maintenance option %ld. File pointer is %p.", pSeqNbr, (uint64_t)(void*)l_FilePtr, pMaintenanceOption, 0);
                                    ::fclose(l_FilePtr);
                                    l_FilePtr = NULL;

                                    // Iterate to open the new file...
                                    l_AllDone = false;
                                }
                                else
                                {
                                    // Error case...  Just return this file...
                                }
                            }
                        }
                        else
                        {
                            FL_Write(FLAsyncRqst, RtvEndOffsetFail, "Failed ftell() request, sequence number %ld, errno %ld.", pSeqNbr, errno, 0, 0);
                            errorText << "openAsyncRequestFile(): Failed ftell() request, sequence number " << pSeqNbr;
                            LOG_ERROR_TEXT_ERRNO(errorText, errno);
                        }
                    }
                    else
                    {
                        FL_Write(FLAsyncRqst, OpenAppendFailed, "Open async request file having seqnbr %ld using mode 'ab', maintenance option %ld failed.", pSeqNbr, pMaintenanceOption, 0, 0);
                        errorText << "Open async request file having seqnbr " << pSeqNbr << " using mode 'ab', maintenance option " << pMaintenanceOption << " failed.";
                        LOG_ERROR_TEXT_ERRNO(errorText, errno);
                    }
                }
            }
        }
    }

    if (l_AsyncRequestFileNamePtr)
    {
        delete [] l_AsyncRequestFileNamePtr;
        l_AsyncRequestFileNamePtr = 0;
    }

    return l_FilePtr;
}

// NOTE: pLVKey is not currently used, but can come in as null.
void WRKQMGR::pinLock(const LVKey* pLVKey, const char* pMethod)
{
    if(transferQueueIsLocked())
    {
        if (!lockPinned)
        {
            lockPinned = 1;
            if (strstr(pMethod, "%") == NULL)
            {
                if (pLVKey)
                {
                    LOG(bb,info) << "TRNFR_Q:   LOCK PINNED <- " << pMethod << ", " << *pLVKey;
                }
                else
                {
                    LOG(bb,info) << "TRNFR_Q:   LOCK PINNED <- " << pMethod << ", unknown LVKey";
                }
            }
        }
        else
        {
            LOG(bb,error) << "TRNFR_Q:   Request made to pin the transfer queue lock by " << pMethod << ", " << *pLVKey << ", but the pin is already in place.";
        }
    }
    else
    {
        LOG(bb,error) << "TRNFR_Q:   Request made to pin the transfer queue lock by " << pMethod << ", " << *pLVKey << ", but the lock is not held.";
    }

    return;
}

void WRKQMGR::processAllOutstandingHP_Requests(const LVKey* pLVKey)
{
    // NOTE: We currently hold the lock transfer queue lock.  Therefore, we essentially process all of the
    //       outstanding async requests in FIFO order and even if bbServer is multi-threaded, we serialize
    //       the processing for each of these requests.  This is true even if the processing for an individual
    //       request releases and re-acquires the lock as part of its processing.
    uint32_t i = 0;
    bool l_AllDone= false;

    // First, check for any new appended HP work queue items...
    uint64_t l_NumberToProcess = checkForNewHPWorkItems();
    HPWrkQE->dump("info", "processAllOutstandingHP_Requests(): ");

    // Now, process all enqueued high priority work items...
    // NOTE: We assume it is not possible for so many async requests to be appended (and continue to be appended...)
    //       that we never process them all...
    while (!l_AllDone)
    {
        // NOTE: Currently set to log after 5 seconds of not being able to process all async requests, and every 10 seconds thereafter...
        if ((i % 20) == 10)
        {
            LOG(bb,info) << "processAllOutstandingHP_Requests(): HPWrkQE->getNumberOfWorkItemsProcessed() " << HPWrkQE->getNumberOfWorkItemsProcessed() << ", l_NumberToProcess " << l_NumberToProcess \
                         << ", Async Seq# " << asyncRequestFileSeqNbr << ", LstOff 0x" << hex << uppercase << setfill('0') << setw(8) << lastOffsetProcessed \
                         << ", NxtOff 0x" << setw(8) << offsetToNextAsyncRequest << setfill(' ') << nouppercase << dec << "  #OutOfOrd " << outOfOrderOffsets.size();
        }
        if (HPWrkQE->getNumberOfWorkItemsProcessed() >= l_NumberToProcess)
        {
            if (i)
            {
                LOG(bb,info) << "processAllOutstandingHP_Requests(): Completed -> HPWrkQE->getNumberOfWorkItemsProcessed() = " << HPWrkQE->getNumberOfWorkItemsProcessed() << ", l_NumberToProcess = " << l_NumberToProcess;
            }
            l_AllDone = true;
        }
        else
        {
            unlockTransferQueue(pLVKey, "processAllOutstandingHP_Requests");
            {
                // NOTE: Currently set to log after 5 seconds of not being able to process all async requests, and every 10 seconds thereafter...
                if ((i++ % 20) == 10)
                {
                    FL_Write(FLDelay, OutstandingHP_Requests, "Processing all outstanding async requests. %ld of %ld work items processed.", (uint64_t)HPWrkQE->getNumberOfWorkItemsProcessed(), (uint64_t)l_NumberToProcess, 0, 0);
                    LOG(bb,info) << ">>>>> DELAY <<<<< processAllOutstandingHP_Requests(): Processing all outstanding async requests...";
                }
                usleep((useconds_t)500000);
            }
            lockTransferQueue(pLVKey, "processAllOutstandingHP_Requests");
        }
    }

    return;
}

void WRKQMGR::processThrottle(LVKey* pLVKey, WRKQE* pWrkQE, BBLV_Info* pLV_Info, BBTagID& pTagId, ExtentInfo& pExtentInfo, Extent* pExtent, double& pThreadDelay, double& pTotalDelay)
{
    pThreadDelay = 0;
    pTotalDelay = 0;

    // Check to see if the throttle timer has popped.
    // If so, any new high priority work items are pushed onto the
    // high priority work queue from the cross bbserver metadata.
    // If we are in throttle mode, the buckets are also (re)loaded.
    checkThrottleTimer();

    if (inThrottleMode())
    {
        if(pWrkQE)
        {
            pThreadDelay = pWrkQE->processBucket(pLV_Info, pTagId, pExtentInfo);
            pTotalDelay = (pThreadDelay ? ((double)(throttleTimerPoppedCount-throttleTimerCount-1) * (Throttle_TimeInterval*1000000)) + pThreadDelay : 0);
        }
    }

    return;
}

void WRKQMGR::removeWorkItem(WRKQE* pWrkQE, WorkID& pWorkItem)
{
    if (pWrkQE)
    {
        // Perform any dump operations...
        if (pWrkQE->getDumpOnRemoveWorkItem())
        {
            // NOTE: Only dump out the found work if for the high priority work queue -or-
            //       this is the last entry in the queue -or- there this a multiple of the number of entries interval
            queue<WorkID>* l_WrkQ = pWrkQE->getWrkQ();
            if ((l_WrkQ == HPWrkQE->getWrkQ()) || (pWrkQE->getWrkQ_Size() == 1) ||
                (getDumpOnRemoveWorkItemInterval() && getNumberOfWorkQueueItemsProcessed() % getDumpOnRemoveWorkItemInterval() == 0))
            {
                pWrkQE->dump("info", "Start: Current work item -> ");
                if (dumpOnRemoveWorkItem && (l_WrkQ != HPWrkQE->getWrkQ()))
                {
                    dump("info", " Work Queue Mgr (Not an error - Count Interval)", DUMP_ALWAYS);
                }
            }
            else
            {
                pWrkQE->dump("debug", "Start: Current work item -> ");
                if (dumpOnRemoveWorkItem && (pWrkQE != HPWrkQE))
                {
                    dump("debug", " Work Queue Mgr (Debug)", DUMP_ALWAYS);
                }
            }
        }

        // Remove the work item from the work queue
        pWrkQE->removeWorkItem(pWorkItem, VALIDATE_WORK_QUEUE);

        // Update the last processed work queue in the manager
        setLastQueueProcessed(pWrkQE->getLVKey());

        // If work item is not for the HP work queue, increment number of work items processed.
        // NOTE: We don't want to trigger the timer interval dump based solely on async requests.
        if (pWrkQE != HPWrkQE)
        {
            incrementNumberOfWorkItemsProcessed();
        }
    }

    return;
}

int WRKQMGR::rmvWrkQ(const LVKey* pLVKey)
{
    int rc = 0;

    stringstream l_Prefix;
    l_Prefix << " - rmvWrkQ() before removing" << *pLVKey;
    wrkqmgr.dump("debug", l_Prefix.str().c_str(), DUMP_UNCONDITIONALLY);

    std::map<LVKey,WRKQE*>::iterator it = wrkqs.find(*pLVKey);
    if (it != wrkqs.end())
    {
        // Remove the work queue from the map
        WRKQE* l_WrkQE = it->second;
        wrkqs.erase(it);

        // If the work queue being removed is currently
        // identified as the last work queue in the map
        // with work items (not likely...), then reset
        // lastQueueWithEntries so it is recalculated
        // next time we try to findWork().
        if (*pLVKey == lastQueueWithEntries)
        {
            lastQueueWithEntries = LVKey_Null;
        }

        if (l_WrkQE)
        {
            // Delete the work queue entry
            if (l_WrkQE->getRate())
            {
                // Removing a work queue that had a transfer rate.
                // Re-calculate the indication of throttle mode...
                calcThrottleMode();
            }
            delete l_WrkQE;
        }
    }
    else
    {
        rc = -1;
        stringstream errorText;
        errorText << " Failure when attempting to remove workqueue for " << *pLVKey;
        wrkqmgr.dump("info", errorText.str().c_str(), DUMP_UNCONDITIONALLY);
        LOG_ERROR_TEXT_RC(errorText, rc);
    }

    l_Prefix << " - rmvWrkQ() after removing " << *pLVKey;
    wrkqmgr.dump("debug", l_Prefix.str().c_str(), DUMP_UNCONDITIONALLY);

    return rc;
}

void WRKQMGR::setDumpTimerPoppedCount(const double pTimerInterval)
{
    double l_DumpTimeInterval = config.get("bb.bbserverDumpWorkQueueMgr_TimeInterval", DEFAULT_DUMP_MGR_TIME_INTERVAL);
    dumpTimerPoppedCount = (int64_t)(l_DumpTimeInterval/pTimerInterval);
    if (((double)dumpTimerPoppedCount)*pTimerInterval != (double)(l_DumpTimeInterval))
    {
        ++dumpTimerPoppedCount;
        LOG(bb,warning) << "Dump timer interval of " << to_string(l_DumpTimeInterval) << " second(s) is not a common multiple of " << pTimerInterval << " second(s).  Any dumpping rates may be implemented as slightly less than what is specified.";
    }

    return;
}

void WRKQMGR::setHeartbeatDumpPoppedCount(const double pTimerInterval)
{
    double l_HeartbeatDumpInterval = config.get("bb.bbserverHeartbeat_DumpInterval", DEFAULT_BBSERVER_HEARTBEAT_DUMP_INTERVAL);
    heartbeatDumpPoppedCount = (int64_t)(l_HeartbeatDumpInterval/pTimerInterval);
    if (((double)heartbeatDumpPoppedCount)*pTimerInterval != (double)(l_HeartbeatDumpInterval))
    {
        ++heartbeatDumpPoppedCount;
        LOG(bb,warning) << "Dump timer interval of " << to_string(l_HeartbeatDumpInterval) << " second(s) is not a common multiple of " << pTimerInterval << " second(s).  Any heartbeat dump rates may be implemented as slightly less than what is specified.";
    }

    return;
}

void WRKQMGR::setHeartbeatTimerPoppedCount(const double pTimerInterval)
{
    double l_HeartbeatTimeInterval = config.get("bb.bbserverHeartbeat_TimeInterval", DEFAULT_BBSERVER_HEARTBEAT_TIME_INTERVAL);
    heartbeatTimerPoppedCount = (int64_t)(l_HeartbeatTimeInterval/pTimerInterval);
    if (((double)heartbeatTimerPoppedCount)*pTimerInterval != (double)(l_HeartbeatTimeInterval))
    {
        ++heartbeatTimerPoppedCount;
        LOG(bb,warning) << "Dump timer interval of " << to_string(l_HeartbeatTimeInterval) << " second(s) is not a common multiple of " << pTimerInterval << " second(s).  Any heartbeat rates may be implemented as slightly less than what is specified.";
    }

    // Currently, for a restart transfer operation, we will wait a total
    // of twice the bbServer heartbeat before declaring a bbServer dead
    // because the transfer definition is not marked as being stopped.
    // Value stored in seconds.
    declareServerDeadCount = max((uint64_t)(l_HeartbeatTimeInterval * 2), MINIMUM_BBSERVER_DECLARE_SERVER_DEAD_VALUE);

    return;
}

int WRKQMGR::setSuspended(const LVKey* pLVKey, const int pValue)
{
    int rc = 0;

    if (pLVKey)
    {
        std::map<LVKey,WRKQE*>::iterator it = wrkqs.find(*pLVKey);
        if (it != wrkqs.end())
        {
            if ((pValue && (!it->second->isSuspended())) || ((!pValue) && it->second->isSuspended()))
            {
                if (it->second)
                {
                    it->second->setSuspended(pValue);
                }
                else
                {
                    rc = -2;
                }
            }
            else
            {
                rc = 2;
            }
        }
        else
        {
            rc = -2;
        }
    }
    else
    {
        rc = -2;
    }

    return rc;
}


int WRKQMGR::setThrottleRate(const LVKey* pLVKey, const uint64_t pRate)
{
    int rc = 0;

    std::map<LVKey,WRKQE*>::iterator it = wrkqs.find(*pLVKey);
    if (it != wrkqs.end())
    {
        it->second->setRate(pRate);
        calcThrottleMode();
    }
    else
    {
        rc = -2;
    }

    return rc;
}

void WRKQMGR::setThrottleTimerPoppedCount(const double pTimerInterval)
{
    throttleTimerPoppedCount = (int)(1.0/pTimerInterval);
    if (((double)throttleTimerPoppedCount)*pTimerInterval != (double)1.0)
    {
        ++throttleTimerPoppedCount;
        LOG(bb,warning) << "Throttle timer interval of " << pTimerInterval << " second is not a common multiple of 1.0 second.  Any throttling rates may be implemented as slightly less than what is specified.";
    }

    return;
}

// NOTE: pLVKey is not currently used, but can come in as null.
void WRKQMGR::unlock(const LVKey* pLVKey, const char* pMethod)
{
    if (transferQueueIsLocked())
    {
        if (!lockPinned)
        {
            pid_t tid = syscall(SYS_gettid);  // \todo eventually remove this.  incurs syscall for each log entry
            FL_Write(FLMutex, unlockTransferQ, "unlockTransfer.  threadid=%ld",tid,0,0,0);

            transferQueueLocked = 0;
            if (strstr(pMethod, "%") == NULL)
            {
                if (pLVKey)
                {
                    LOG(bb,debug) << "TRNFR_Q: UNLOCK <- " << pMethod << ", " << *pLVKey;
                }
                else
                {
                    LOG(bb,debug) << "TRNFR_Q: UNLOCK <- " << pMethod << ", unknown LVKey";
                }
            }
            pthread_mutex_unlock(&lock_transferqueue);
        }
        else
        {
            LOG(bb,debug) << "TRNFR_Q: Request made to unlock the transfer queue by " << pMethod << ", but the lock is pinned.";
        }
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

// NOTE: pLVKey is not currently used, but can come in as null.
void WRKQMGR::unpinLock(const LVKey* pLVKey, const char* pMethod)
{
    if(transferQueueIsLocked())
    {
        if (lockPinned)
        {
            lockPinned = 0;
            if (strstr(pMethod, "%") == NULL)
            {
                if (pLVKey)
                {
                    LOG(bb,info) << "TRNFR_Q:   LOCK UNPINNED <- " << pMethod << ", " << *pLVKey;
                }
                else
                {
                    LOG(bb,info) << "TRNFR_Q:   LOCK UNPINNED <- " << pMethod << ", unknown LVKey";
                }
            }
        }
        else
        {
            LOG(bb,error) << "TRNFR_Q:   Request made to unpin the transfer queue lock by " << pMethod << ", " << *pLVKey << ", but the pin is not in place.";
        }
    }
    else
    {
        LOG(bb,error) << "TRNFR_Q:   Request made to unpin the transfer queue lock by " << pMethod << ", " << *pLVKey << ", but the lock is not held.";
    }

    return;
}

// NOTE: Stageout End processing can 'discard' extents from a work queue.  Therefore, the number of
//       posts can exceed the current number of extents.  Such posts will eventually be consumed by the worker
//       threads and treated as no-ops.  Note that the stageout end processing cannot re-initialize
//       the semaphore to the 'correct' value as you cannot re-init a counting semaphore...  @DLH
void WRKQMGR::verify()
{
    int l_TotalExtents = getSizeOfAllWorkQueues();

    int l_NumberOfPosts = 0;
    sem_getvalue(&sem_workqueue, &l_NumberOfPosts);

    // NOTE: l_NumberOfPosts+1 because for us to be invoking verify(), the current thread has already been dispatched...
    if (l_NumberOfPosts+1 != l_TotalExtents)
    {
        LOG(bb,info) << "WRKQMGR::verify(): MISMATCH: l_NumberOfPosts=" << l_NumberOfPosts << ", l_TotalExtents=" << l_TotalExtents;
        dump(const_cast<char*>("info"), " - After failed verification");
    }

    return;
}

void WRKQMGR::updateHeartbeatData(const string& pHostName)
{
    uint64_t l_Count = 0;

    string l_CurrentTime = HeartbeatEntry::getHeartbeatCurrentTime();
    map<string, HeartbeatEntry>::iterator it = heartbeatData.find(pHostName);
    if (it != heartbeatData.end())
    {
        l_Count = (it->second).getCount();
        heartbeatData[pHostName] = HeartbeatEntry(++l_Count, l_CurrentTime, (it->second).getServerTime());
    }
    else
    {
        heartbeatData[pHostName] = HeartbeatEntry(++l_Count, l_CurrentTime, "");
    }

    return;
}

void WRKQMGR::updateHeartbeatData(const string& pHostName, const string& pServerTimeStamp)
{
    uint64_t l_Count = 0;

    map<string, HeartbeatEntry>::iterator it = heartbeatData.find(pHostName);
    if (it != heartbeatData.end())
    {
        l_Count = (it->second).getCount();
    }

    string l_CurrentTime = HeartbeatEntry::getHeartbeatCurrentTime();
    HeartbeatEntry l_Entry = HeartbeatEntry(++l_Count, l_CurrentTime, pServerTimeStamp);

    heartbeatData[pHostName] = l_Entry;

    return;
}

int WRKQMGR::verifyAsyncRequestFile(char* &pAsyncRequestFileName, int &pSeqNbr, const MAINTENANCE_OPTION pMaintenanceOption)
{
    int rc = 0;
    stringstream errorText;

    int l_SeqNbr = 0;
    int l_CurrentSeqNbr = 0;

    pAsyncRequestFileName = new char[PATH_MAX+1];
    string l_DataStorePath = config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH);

    bfs::path datastore(l_DataStorePath);
    if(bfs::is_directory(datastore))
    {
        for(auto& asyncfile : boost::make_iterator_range(bfs::directory_iterator(datastore), {}))
        {
            if(bfs::is_directory(asyncfile)) continue;

            int l_Count = sscanf(asyncfile.path().filename().c_str(),"asyncRequests_%d", &l_CurrentSeqNbr);
            // NOTE: If pSeqNbr is passed in, that is the file we want to open...
            if (l_Count == 1 && ((pSeqNbr && pSeqNbr == l_CurrentSeqNbr) || ((!pSeqNbr) && l_CurrentSeqNbr > l_SeqNbr)))
            {
                l_SeqNbr = l_CurrentSeqNbr;
                strCpy(pAsyncRequestFileName, asyncfile.path().c_str(), PATH_MAX+1);
            }
        }
    }
    else
    {
        bfs::create_directories(datastore);
        LOG(bb,info) << "WRKQMGR: Directory " << datastore << " created to house the cross bbServer metadata";
    }

    if (!l_SeqNbr)
    {
        l_SeqNbr = 1;
        snprintf(pAsyncRequestFileName, PATH_MAX+1, "%s/%s_%d", l_DataStorePath.c_str(), XBBSERVER_ASYNC_REQUEST_BASE_FILENAME.c_str(), l_SeqNbr);
        rc = createAsyncRequestFile(pAsyncRequestFileName);
        if (rc)
        {
            errorText << "Failure when attempting to create new cross bbserver async request file";
            bberror << err("error.filename", pAsyncRequestFileName);
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }
    }

    if (!rc)
    {
        try
        {
            switch (pMaintenanceOption)
            {
                case CREATE_NEW_FILE:
                {
                    l_SeqNbr += 1;
                    snprintf(pAsyncRequestFileName, PATH_MAX+1, "%s/%s_%d", l_DataStorePath.c_str(), XBBSERVER_ASYNC_REQUEST_BASE_FILENAME.c_str(), l_SeqNbr);
                    rc = createAsyncRequestFile(pAsyncRequestFileName);
                    if (rc)
                    {
                        errorText << "Failure when attempting to create new cross bbserver async request file";
                        bberror << err("error.filename", pAsyncRequestFileName);
                        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                    }
                }
                // Fall through...

                case START_BBSERVER:
                case FULL_MAINTENANCE:
                {
                    if (pMaintenanceOption == START_BBSERVER)
                    {
                        // Ensure that the bbServer metadata is on a parallel file system
                        // NOTE:  We invoke isGpfsFile() even if we are not to enforce the condition so that
                        //        we flightlog the statfs() result...
                        bool l_GpfsMount = false;
                        rc = isGpfsFile(pAsyncRequestFileName, l_GpfsMount);
                        if (!rc)
                        {
                            if (!l_GpfsMount)
                            {
                                if (config.get("bb.requireMetadataOnParallelFileSystem", DEFAULT_REQUIRE_BBSERVER_METADATA_ON_PARALLEL_FILE_SYSTEM))
                                {
                                    rc = -1;
                                    errorText << "bbServer metadata is required to be on a parallel file system. Current data store path is " << l_DataStorePath \
                                              << ". Set bb.bbserverMetadataPath properly in the configuration.";
                                    bberror << err("error.asyncRequestFile", pAsyncRequestFileName);
                                    LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
                                }
                                else
                                {
                                    LOG(bb,info) << "WRKQMGR: bbServer metadata is NOT on a parallel file system, but is currently allowed";
                                }
                            }
                        }
                        else
                        {
                            // bberror was filled in...
                            BAIL;
                        }

                        // Unconditionally perform a chown to root:root for the cross-bbServer metatdata root directory.
                        rc = chown(l_DataStorePath.c_str(), 0, 0);
                        if (rc)
                        {
                            errorText << "chown failed";
                            bberror << err("error.path", l_DataStorePath.c_str());
                            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
                        }

                        // Unconditionally perform a chmod to 0755 for the cross-bbServer metatdata root directory.
                        // NOTE:  root:root will insert jobid directories into this directory and then ownership
                        //        of those jobid directories will be changed to the uid:gid of the mountpoint.
                        //        The mode of the jobid directories is also changed to be 0700.
                        rc = chmod(l_DataStorePath.c_str(), 0755);
                        if (rc)
                        {
                            errorText << "chmod failed";
                            bberror << err("error.path", l_DataStorePath.c_str());
                            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
                        }

                        // Verify the correct permissions for all individual directories in the
                        // cross-bbServer metadata path.
                        bfs::path l_Path = datastore.parent_path();
                        while (l_Path.string().length())
                        {
                            if (((bfs::status(l_Path)).permissions() & (bfs::others_read|bfs::others_exe)) != (bfs::others_read|bfs::others_exe))
                            {
                                rc = -1;
                                stringstream l_Temp;
                                l_Temp << "0" << oct << (bfs::status(l_Path)).permissions();
                                errorText << "Verification of permissions failed for bbServer metadata directory " << l_Path.c_str() \
                                          << ". Requires read and execute for all users, but permissions are " \
                                          << l_Temp.str() << ".";
                                bberror << err("error.path", l_Path.c_str()) << err("error.permissions", l_Temp.str());
                                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                            }
                            l_Path = l_Path.parent_path();
                        }
                    }

                    // Unconditionally perform a chown to root:root for the async request file.
                    rc = chown(pAsyncRequestFileName, 0, 0);
                    if (rc)
                    {
                        errorText << "chown failed";
                        bberror << err("error.path", pAsyncRequestFileName);
                        LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
                    }

                    // Unconditionally perform a chmod to 0700 for the async request file.
                    // NOTE:  root is the only user of the async request file.
                    rc = chmod(pAsyncRequestFileName, 0700);
                    if (rc)
                    {
                        errorText << "chmod failed";
                        bberror << err("error.path", pAsyncRequestFileName);
                        LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
                    }

                    // Log where this instance of bbServer will start processing async requests
                    int l_AsyncRequestFileSeqNbr = 0;
                    int64_t l_OffsetToNextAsyncRequest = 0;
                    rc = wrkqmgr.findOffsetToNextAsyncRequest(l_AsyncRequestFileSeqNbr, l_OffsetToNextAsyncRequest);
                    if (!rc)
                    {
                        if (l_AsyncRequestFileSeqNbr > 0 && l_OffsetToNextAsyncRequest >= 0)
                        {
                            if (pMaintenanceOption == START_BBSERVER)
                            {
                                wrkqmgr.setOffsetToNextAsyncRequest(l_AsyncRequestFileSeqNbr, l_OffsetToNextAsyncRequest);
                                if (l_OffsetToNextAsyncRequest)
                                {
                                    // Set the last offset processed to one entry less than the next offset set above.
                                    lastOffsetProcessed = l_OffsetToNextAsyncRequest - (uint64_t)sizeof(AsyncRequest);
                                }
                                else
                                {
                                    // Set the last offset processed to the maximum async request file size.
                                    // NOTE: This will cause the first expected target offset to be zero in method
                                    //       manageWorkItemsProcessed().
                                    lastOffsetProcessed = MAXIMUM_ASYNC_REQUEST_FILE_SIZE;
                                }
                            }
                            LOG(bb,info) << "WRKQMGR: Current async request file " << pAsyncRequestFileName \
                                         << hex << uppercase << setfill('0') << ", LstOff 0x" << setw(8) << lastOffsetProcessed \
                                         << ", NxtOff 0x" << setw(8) << l_OffsetToNextAsyncRequest \
                                         << setfill(' ') << nouppercase << dec << "  #OutOfOrd " << outOfOrderOffsets.size();
                        }
                        else
                        {
                            rc = -1;
                            errorText << "Failure when attempting to open the cross bbserver async request file, l_AsyncRequestFileSeqNbr = " \
                                      << l_AsyncRequestFileSeqNbr << ", l_OffsetToNextAsyncRequest = " << l_OffsetToNextAsyncRequest;
                            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                        }
                    }
                    else
                    {
                        rc = -1;
                        errorText << "Failure when attempting to open the cross bbserver async request file, rc = " << rc;
                        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                    }
                }
                // Fall through...

                case MINIMAL_MAINTENANCE:
                {
                    bfs::path datastore(l_DataStorePath);
                    if(bfs::is_directory(datastore))
                    {
                        for(auto& asyncfile : boost::make_iterator_range(bfs::directory_iterator(datastore), {}))
                        {
                            if(bfs::is_directory(asyncfile)) continue;

                            if (asyncfile.path().filename().string() == XBBSERVER_ASYNC_REQUEST_BASE_FILENAME)
                            {
                                // Old style....  Simply delete it...
                                bfs::remove(asyncfile.path());
                                LOG(bb,info) << "WRKQMGR: Deprecated async request file " << asyncfile.path().c_str() << " removed";
                                continue;
                            }

                            int l_Count = sscanf(asyncfile.path().filename().c_str(),"asyncRequests_%d", &l_CurrentSeqNbr);
                            if (l_Count == 1 && l_CurrentSeqNbr < l_SeqNbr)
                            {
                                try
                                {
                                    // Old async file....  If old enough, delete it...
                                    struct stat l_Statinfo;
                                    threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::statsyscall, asyncfile.path().c_str(), __LINE__);
                                    int rc2 = stat(asyncfile.path().c_str(), &l_Statinfo);
                                    threadLocalTrackSyscallPtr->clearTrack();
                                    FL_Write(FLAsyncRqst, Stat, "Get stats for async request file having seqnbr %ld for aging purposes, rc %ld.", l_CurrentSeqNbr, rc2, 0, 0);
                                    if (!rc2)
                                    {
                                        time_t l_CurrentTime = time(0);
                                        if (difftime(l_CurrentTime, l_Statinfo.st_atime) > ASYNC_REQUEST_FILE_PRUNE_TIME)
                                        {
                                            bfs::remove(asyncfile.path());
                                            LOG(bb,info) << "WRKQMGR: Async request file " << asyncfile.path() << " removed";
                                        }
                                    }
                                }
                                catch(exception& e)
                                {
                                    // NOTE:  It is possible for multiple bbServers to be performing maintenance at the same time.
                                    //        Tolerate any exception and continue...
                                }
                            }
                        }
                    }
                }
                // Fall through...

                case FORCE_REOPEN:
                case NO_MAINTENANCE:
                default:
                    break;
            }
       }
       catch(ExceptionBailout& e) { }
       catch(exception& e)
       {
           rc = -1;
           LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
       }
    }

    LOG(bb,debug) << "verifyAsyncRequestFile(): File: " << pAsyncRequestFileName << ", SeqNbr: " << l_SeqNbr << ", Option: " << pMaintenanceOption;

    if (rc)
    {
        if (pAsyncRequestFileName)
        {
            delete [] pAsyncRequestFileName;
            pAsyncRequestFileName = 0;
        }
    }
    else
    {
        pSeqNbr = l_SeqNbr;
    }

    return rc;
}
