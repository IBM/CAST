/*******************************************************************************
 |    xfer.cc
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

#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>

#include <iomanip>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

#include <sys/socket.h>
#include <sys/stat.h>

#include "connections.h"

using namespace std;

#include <boost/filesystem.hpp>
namespace bfs = boost::filesystem;
namespace bs  = boost::system;

#include "bbapi.h"
#include "bbinternal.h"
#include "bbio_regular.h"
#include "bbio_BSCFS.h"
#include "BBLV_Info.h"
#include "BBLV_Metadata.h"
#include "bbserver_flightlog.h"
#include "BBTagID.h"
#include "BBTransferDef.h"
#include "bbwrkqmgr.h"
#include "BBTagID.h"
#include "BBTagInfoMap.h"
#include "ContribIdFile.h"
#include "ExtentInfo.h"
#include "identity.h"
#include "Msg.h"
#include "WorkID.h"
#include "xfer.h"
#include "tracksyscall.h"

// Control elements for wrkqmgr
pthread_mutex_t lock_transferqueue = PTHREAD_MUTEX_INITIALIZER;
sem_t sem_workqueue;
size_t  transferBufferSize   = 0;
pthread_once_t startThreadsControl = PTHREAD_ONCE_INIT;

string getDeviceBySerial(string serial);
string getSerialByDevice(string device);
string getNVMeByIndex(uint32_t index);

FL_SetName(FLXfer, "Transfer Flightlog")
FL_SetSize(FLXfer, 16384)

FL_SetName(FLDelay, "Server Delay Flightlog")
FL_SetSize(FLDelay, 16384)

/*
 * Static data
 */
static LVKey LVKey_Null = LVKey();


void lockTransferQueue(const LVKey* pLVKey, const char* pMethod)
{
    ENTRY(__FILE__,__FUNCTION__);

    wrkqmgr.lock(pLVKey, pMethod);

    EXIT(__FILE__,__FUNCTION__);
    return;
}

void unlockTransferQueue(const LVKey* pLVKey, const char* pMethod)
{
    ENTRY(__FILE__,__FUNCTION__);

    wrkqmgr.unlock(pLVKey, pMethod);

    EXIT(__FILE__,__FUNCTION__);
    return;
}

void pinTransferQueueLock(const LVKey* pLVKey, const char* pMethod)
{
    ENTRY(__FILE__,__FUNCTION__);

    wrkqmgr.pinLock(pLVKey, pMethod);

    EXIT(__FILE__,__FUNCTION__);
    return;
}

void unpinTransferQueueLock(const LVKey* pLVKey, const char* pMethod)
{
    ENTRY(__FILE__,__FUNCTION__);

    wrkqmgr.unpinLock(pLVKey, pMethod);

    EXIT(__FILE__,__FUNCTION__);
    return;
}

void processAsyncRequest(WorkID& pWorkItem)
{
    ENTRY(__FILE__,__FUNCTION__);

    int rc = 0;

    AsyncRequest l_Request = AsyncRequest();
    rc = wrkqmgr.getAsyncRequest(pWorkItem, l_Request);

    if (!rc)
    {
        if (!l_Request.sameHostName())
        {
            char l_Cmd[AsyncRequest::MAX_DATA_LENGTH] = {'\0'};
            char l_Str1[64] = {'\0'};
            char l_Str2[64] = {'\0'};
            uint64_t l_JobId = UNDEFINED_JOBID;
            uint64_t l_JobStepId = UNDEFINED_JOBSTEPID;
            uint64_t l_Handle = UNDEFINED_HANDLE;
            uint32_t l_ContribId = UNDEFINED_CONTRIBID;
            uint64_t l_CancelScope = 0;

            // Process the request
            bool l_LogAsInfo = true;
            rc = sscanf(l_Request.getData(), "%s %lu %lu %lu %u %lu %s %s", l_Cmd, &l_JobId, &l_JobStepId, &l_Handle, &l_ContribId, &l_CancelScope, l_Str1, l_Str2);
            if (rc == 8)
            {
                if (strstr(l_Cmd, "heartbeat"))
                {
                    l_LogAsInfo = false;
                    LOG(bb,debug) << "Start processing async request: Offset 0x" << hex << uppercase << setfill('0') \
                                  << pWorkItem.getTag() << setfill(' ') << nouppercase << dec \
                                  << ", from hostname " << l_Request.getHostName() << " => " << l_Request.getData();
                }
                else
                {
                    LOG(bb,info) << "Start processing async request: Offset 0x" << hex << uppercase << setfill('0') \
                                 << pWorkItem.getTag() << setfill(' ') << nouppercase << dec \
                                 << ", from hostname " << l_Request.getHostName() << " => " << l_Request.getData();
                }

                rc = 0;
                if (strcmp(l_Str1, "''") == 0)
                {
                    l_Str1[0] = '\0';
                }
                if (strcmp(l_Str2, "''") == 0)
                {
                    l_Str2[0] = '\0';
                }
                if (strstr(l_Cmd, "cancel"))
                {
                    // Process cancel request...
                    if (strstr(l_Str1, "cancelrequest"))
                    {
                        // This was a direct cancel request from a given compute node to the bbServer servicing
                        // that CN at the request's hostname.  That request is now being propagated to all other bbServers...
                        rc = cancelTransferForHandle(l_Request.getHostName(), l_JobId, l_JobStepId, l_Handle, REMOVE_TARGET_PFS_FILES);
                    }
                    else if (strstr(l_Str1, "stoprequest"))
                    {
                        // This was a direct stop transfers request that is now being propagated to all other bbServers...
                        rc = metadata.stopTransfer(l_Request.getHostName(), l_Str2, l_JobId, l_JobStepId, l_Handle, l_ContribId);
                    }
                    else
                    {
                        LOG(bb,error) << "Invalid data indicating type of cancel operation from request data " << l_Request.getData() << " to this bbServer";
                    }

                    // NOTE: The rc value could be returned as position indicating a non-error...
                    if (rc < 0)
                    {
                        LOG(bb,error) << "Failure when attempting to propagate cancel operation " << l_Request.getData() << " to this bbServer, rc=" << rc;
                    }
                    wrkqmgr.updateHeartbeatData(l_Request.getHostName());
                }

                else if (strstr(l_Cmd, "handle"))
                {
                    // Process the handle status request...
                    // NOTE:  We send the completion message here because some other bbServer has updated the
                    //        status for the handle.  This is the mechanism used to notify handle status changes
                    //        to other bbServers that have already finished processing a handle.  However,
                    //        if a bbServer still has extents on a work queue for the handle in question, a status
                    //        change will be sent here and also might be sent again when this bbServer processes
                    //        that last extent for the handle.  This is probably only in the status case of BBFAILED.
                    BBSTATUS l_Status = getBBStatusFromStr(l_Str2);
                    metadata.sendTransferCompleteForHandleMsg(l_Request.getHostName(), l_Str1, l_Handle, l_Status);
                    wrkqmgr.updateHeartbeatData(l_Request.getHostName());
                }

                else if (strstr(l_Cmd, "heartbeat"))
                {
                    // Process a heartbeat from another bbServer...
                    wrkqmgr.updateHeartbeatData(l_Request.getHostName(), l_Str2);
                }

                else if (strstr(l_Cmd, "removejobinfo"))
                {
                    rc = removeJobInfo(l_Request.getHostName(), l_JobId);
                    if (rc)
                    {
                        if (rc != -2)
                        {
                            LOG(bb,error) << "Failure when attempting to propagate " << l_Request.getData() << " to this bbServer, rc=" << rc;
                        }
                    }
                    wrkqmgr.updateHeartbeatData(l_Request.getHostName());
                }

                else if (strstr(l_Cmd, "removelogicalvolume"))
                {
                    LVKey l_LVKeyStg = LVKey("", Uuid(l_Str2));
                    LVKey* l_LVKey = &l_LVKeyStg;
                    metadata.removeAllLogicalVolumesForUuid(l_Request.getHostName(), l_LVKey, l_JobId);
                    wrkqmgr.updateHeartbeatData(l_Request.getHostName());
                }

                else if (strstr(l_Cmd, "setsuspend"))
                {
                    // NOTE: In this path l_JobId is the pValue
                    rc = metadata.setSuspended(l_Request.getHostName(), l_Str1, (int)l_JobId);
                    if (rc)
                    {
                        LOG(bb,error) << "Failure when attempting to propagate " << l_Request.getData() << " to this bbServer, rc=" << rc;
                    }
                    wrkqmgr.updateHeartbeatData(l_Request.getHostName());
                }
                else
                {
                    LOG(bb,error) << "Unknown async request command from hostname " << l_Request.getHostName() << " => " << l_Request.getData();
                }
            }
            else
            {
                // Failure when attempting to parse the request...  Log it and continue...
                LOG(bb,error) << "Failure when attempting to process async request from hostname " << l_Request.getHostName() << ", number of successfully parsed items " << rc << " => " << l_Request.getData();
            }

            if (l_LogAsInfo)
            {
                LOG(bb,info) << "End processing async request: Offset 0x" << hex << uppercase << setfill('0') \
                             << pWorkItem.getTag() << setfill(' ') << nouppercase << dec;
            }
            else
            {
                LOG(bb,debug) << "End processing async request: Offset 0x" << hex << uppercase << setfill('0') \
                              << pWorkItem.getTag() << setfill(' ') << nouppercase << dec;
            }
        }
        else
        {
            LOG(bb,debug) << "Skipping async request because it is from this bbServer host: Tag: " << pWorkItem.getTag() << ", from hostname " << l_Request.getHostName() << " => " << l_Request.getData();
        }
    }
    else
    {
        // Error when retrieving the request
        LOG(bb,error) << "Error when attempting to retrieve the async request at offset " << pWorkItem.getTag();
    }

    EXIT(__FILE__,__FUNCTION__);
    return;
}

int cancelTransferForHandle(const string& pHostName, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const int pRemoveOption)
{
    ENTRY(__FILE__,__FUNCTION__);

    stringstream errorText;
    int rc = 0;

    try
    {
        metadata.setCanceled(pJobId, pJobStepId, pHandle, pRemoveOption);

        // Determine if this cancel request should be appended to the async file
        // to be consumed by other bbServers
        if (sameHostName(pHostName))
        {
            size_t l_TotalContributors = 0;
            size_t l_TotalLocalReportingContributors = 0;
            metadata.accumulateTotalLocalContributorInfo(pHandle, l_TotalContributors, l_TotalLocalReportingContributors);
            LOG(bb,debug) << "cancelRequest: pHandle=" << pHandle << ", l_TotalContributors=" << l_TotalContributors << ", l_TotalLocalReportingContributors=" << l_TotalLocalReportingContributors;
            if (l_TotalContributors > 1)
            {
                if (l_TotalLocalReportingContributors != l_TotalContributors)
                {
                    // Communicate this handle status change to other bbServers by
                    // posting this request to the async request file.
                    // NOTE: Regardless of the rc, post the async request...
                    // NOTE: No need to catch the return code.  If the append doesn't work,
                    //       appendAsyncRequest() will log the failure...
                    char l_AsyncCmd[AsyncRequest::MAX_DATA_LENGTH] = {'\0'};
                    snprintf(l_AsyncCmd, sizeof(l_AsyncCmd), "cancel %lu %lu %lu %u %lu cancelrequest None", pJobId, pJobStepId, pHandle, UNDEFINED_CONTRIBID, (uint64_t)BBSCOPETAG);
                    AsyncRequest l_Request = AsyncRequest(l_AsyncCmd);
                    wrkqmgr.appendAsyncRequest(l_Request);
                }
                else
                {
                    LOG(bb,debug) << "cancelRequest: No need to append an async request as all " << l_TotalContributors << " contributors for handle " << pHandle << " are local to this bbServer";
                }
            }
            else
            {
                LOG(bb,debug) << "cancelRequest: No need to append an async request as there is only a single contributor for handle " << pHandle;
            }
        }
    }
    catch (ExceptionBailout& e) { }
    catch (exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}

int contribIdStopped(const std::string& pConnectionName, const LVKey* pLVKey, BBLV_Info* pLV_Info, BBTransferDef* pOrigTransferDef, BBTransferDef* pRebuiltTransferDef, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId)
{
    stringstream errorText;
    int rc = 0;

    HandleFile* l_HandleFile = 0;
    char* l_HandleFileName = 0;

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, XF_ContribIdStopped, "xfer contribIdStopped, counter=%ld, jobid=%ld, handle=%ld, contribid=%ld", l_FL_Counter, pJobId, pHandle, pContribId);

    // First, ensure that this handle/transfer definition is marked as stopped in the cross bbServer metadata...
    // NOTE:  This must be a restart scenario...
    ContribIdFile* l_ContribIdFile = 0;
    bfs::path l_HandleFilePath(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
    l_HandleFilePath /= bfs::path(to_string(pJobId));
    l_HandleFilePath /= bfs::path(to_string(pJobStepId));
    l_HandleFilePath /= bfs::path(to_string(pHandle));

    int l_Attempts = 1;
    int l_RetryAttempts = 0;
    bool l_AllDone = false;
    while (!l_AllDone)
    {
        l_AllDone = true;

        rc = 2;
        uint64_t l_Continue = wrkqmgr.getDeclareServerDeadCount();
        while ((rc != 1) && (l_Continue--))
        {
            ++l_RetryAttempts;
            // NOTE: The handle file is locked exclusive here to serialize between this bbServer and another
            //       bbServer that is marking the handle/contribid file as 'stopped'
            // NOTE: The lock on the handle file is obtained by first polling for the lock being held so that we do
            //       not generate RAS messages indicating that we are blocked waiting on the handle file lock.
            //       When stopping the transfer definition, processing may have to hold the lock for an extended period.
            HANDLEFILE_LOCK_FEEDBACK l_LockFeedback;
            rc = HandleFile::loadHandleFile(l_HandleFile, l_HandleFileName, pJobId, pJobStepId, pHandle, LOCK_HANDLEFILE, &l_LockFeedback);
            if (!rc)
            {
                rc = ContribIdFile::loadContribIdFile(l_ContribIdFile, l_HandleFilePath, pContribId);
                if (rc >= 0)
                {
                    // Contribid file successfully loaded
                    // NOTE: We process the contribid file first to see if this contributor
                    //       is restartable.  If not, we early exit without having to wait
                    //       for the handle to be marked as stopped.  In the case where a contributor
                    //       is not restartable, the handle may never transition to being stopped.
                    //       Otherwise, we then wait for the contributor/handle to be
                    //       marked as stopped.
                    //
                    // Process the contribid file
                    if (rc == 1 && l_ContribIdFile)
                    {
                        if (l_ContribIdFile->allExtentsTransferred())
                        {
                            // All extents have been processed...
                            if (!l_ContribIdFile->stopped())
                            {
                                // Contribid is not marked as stopped
                                if (l_ContribIdFile->allFilesClosed())
                                {
                                    // All files are marked as closed (but, some could have failed...)
                                    if (l_ContribIdFile->notRestartable())
                                    {
                                        // All extents have been processed, all files closed, no failed files -or- canceled and
                                        // not marked as stopped.  Therefore, no need to restart this transfer definition.
                                        rc = 0;
                                        l_Continue = 0;
                                        LOG(bb,info) << "msgin_starttransfer(): All extents have been transferred for contribId " << pContribId \
                                                     << ", but it is not marked as being stopped.   All file transfers for this contributor have already finished or were canceled." \
                                                     << " See previous messages.";
                                    }
                                    else
                                    {
                                        // At least one of the files failed.  (It may have been marked failed after processing the
                                        // last extent for the transfer definition...)
                                        //
                                        // First ensure that the handle is marked as stopped...
                                        if (l_HandleFile->stopped())
                                        {
                                            // Mark the transfer definition as stopped...
                                            uint64_t l_StartingFlags = l_ContribIdFile->flags;
                                            SET_FLAG_VAR(l_ContribIdFile->flags, l_ContribIdFile->flags, BBTD_Stopped, 1);

                                            LOG(bb,info) << "xbbServer: For " << *pLVKey << ", handle " << pHandle << ", contribid " << pContribId << ":";
                                            LOG(bb,info) << "           ContribId flags changing from 0x" << hex << uppercase << l_StartingFlags << " to 0x" << l_ContribIdFile->flags << nouppercase << dec << ".";

                                            // Save the contribid file
                                            rc = ContribIdFile::saveContribIdFile(l_ContribIdFile, pLVKey, l_HandleFilePath, pContribId);
                                            if (!rc)
                                            {
                                                // Update the handle status
                                                rc = HandleFile::update_xbbServerHandleStatus(pLVKey, pJobId, pJobStepId, pHandle, 0);
                                                if (!rc)
                                                {
                                                    // Indicate to restart this transfer definition as it is now marked as stopped
                                                    rc = 1;
                                                    LOG(bb,info) << "contribIdStopped():  At least one file transfer failed for the transfer definition. " \
                                                                 << "Transfer definition marked as stopped for jobid " << pJobId \
                                                                 << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", contribid " << pContribId;
                                                }
                                                else
                                                {
                                                    // Indicate to not restart this transfer definition
                                                    rc = 0;
                                                    l_Continue = 0;
                                                    LOG(bb,error) << "contribIdStopped():  Failure when attempting to update the cross bbServer handle status for jobid " << pJobId \
                                                                  << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", contribid " << pContribId;
                                                }
                                            }
                                            else
                                            {
                                                // Indicate to not restart this transfer definition
                                                rc = 0;
                                                l_Continue = 0;
                                                LOG(bb,error) << "contribIdStopped():  Failure when attempting to save the cross bbServer contribs file for jobid " << pJobId \
                                                              << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", contribid " << pContribId;
                                            }
                                        }
                                        else
                                        {
                                            // Handle file is not marked as stopped.
                                            // NOTE: If any contributor is marked as stopped, the handle file should
                                            //       eventually be marked as stopped also.  We will wait for the handle
                                            //       file to be marked.
                                            rc = 0;     //  Assume that we will keep spinning...
                                            if (!l_Continue)
                                            {
                                                //  We have waited long enough...  Processing for all prior enqueued extents has not occurred...
                                                rc = -2;
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    // Not all of the files have been marked as closed.
                                    rc = 0;     //  Assume that we will keep spinning...
                                    if (!l_Continue)
                                    {
                                        //  We have waited long enough...  Processing for all prior enqueued extents has not occurred...
                                        rc = -2;
                                    }
                                }
                            }
                            else
                            {
                                // Contribid is marked as stopped
                                // First ensure that the handle is also marked as stopped...
                                if (!l_HandleFile->stopped())
                                {
                                    rc = 0;     //  Assume that we will keep spinning...
                                    if (!l_Continue)
                                    {
                                        //  We have waited long enough...  Processing for all prior enqueued extents has not occurred...
                                        rc = -2;
                                    }
                                }
                                else
                                {
                                    // Transfer definition and handle are both  stopped and all previously enqueued extents have been processed.
                                    // Will exit both loops...  rc is already 1
                                }
                            }
                        }
                        else
                        {
                            // Not all previously enqueued extents have been processed
                            rc = 0;     //  Assume that we will keep spinning...
                            if (!l_Continue)
                            {
                                //  We have waited long enough...  Processing for all prior enqueued extents has not occurred...
                                rc = -2;
                            }
                        }
                    }
                    else
                    {
                        rc = 0;
                        l_Continue = 0;
                        LOG(bb,info) << "contribIdStopped(): ContribId " << pContribId << " was not found in the cross bbServer metadata (ContribIdFile pointer is NULL)." \
                                     << " All transfers for this contributor may have already finished.  See previous messages.";
                    }
                }
                else
                {
                    rc = 0;
                    l_Continue = 0;
                    LOG(bb,info) << "contribIdStopped(): Error occurred when attempting to load the contrib file for contribid " << pContribId \
                                 << " (Negative rc from loadContribIdFile()). All transfers for this contributor may have already finished.  See previous messages.";
                }
            }
            else
            {
                rc = 0;
                l_Continue = 0;
                LOG(bb,info) << "contribIdStopped(): Error occurred when attempting to load the handle file for " << *pLVKey << ", handle " << pHandle \
                             << " (Negative rc from loadHandleFile()). All transfers for this contributor may have already finished.  See previous messages.";
            }

            if (l_HandleFileName)
            {
                delete[] l_HandleFileName;
                l_HandleFileName = 0;
            }

            if (l_HandleFile)
            {
                // The lock on the handle file was already released by the close above
                l_HandleFile->close(l_LockFeedback);
                delete l_HandleFile;
                l_HandleFile = 0;
            }

            if (l_ContribIdFile)
            {
                delete l_ContribIdFile;
                l_ContribIdFile = 0;
            }

            if ((!rc) && l_Continue)
            {
                // Spin, waiting for the transfer definition to be marked as stopped and for all of the prior enqueued
                // extents to be processed for the transfer definition.
                // NOTE: If for some reason I/O is 'stuck' and does not return, the following is an infinite loop...
                //       \todo - What to do???  @DLH
                // NOTE: Currently set to log after 1 second of not being able to clear, and every 15 seconds thereafter...
                if (((wrkqmgr.getDeclareServerDeadCount() - l_Continue) % 15) == 1)
                {
                    FL_Write6(FLDelay, RestartWaitForStop2, "Attempting to restart a transfer definition for jobid %ld, jobstepid %ld, handle %ld, contribid %ld. Delay of 1 second before retry. %ld seconds remain waiting for the original bbServer to act before an unconditional stop is performed.",
                              (uint64_t)pJobId, (uint64_t)pJobStepId, (uint64_t)pHandle, (uint64_t)pContribId, (uint64_t)l_Continue, 0);
                    LOG(bb,info) << ">>>>> DELAY <<<<< contribIdStopped(): Attempting to restart a transfer definition for jobid " << pJobId \
                                 << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", contribid " << pContribId \
                                 << ". Waiting for all extents to finish being processed on the prior bbServer" \
                                 << " and the transfer definition to be marked as stopped. Delay of 1 second before retry. " \
                                 << l_Continue << " seconds remain waiting for the original bbServer to act before an unconditional stop is performed.";
                    if (pOrigTransferDef)
                    {
                        // The prior bbServer is this same bbServer...
                        // Dump out the extents in question...
                        pLV_Info->getExtentInfo()->dump("info", "contribIdStopped(): Waiting for all extents to be processed");
                    }
                }
                unlockTransferQueue(pLVKey, "contribIdStopped - Waiting to determine if a transfer definition is restartable");
                {
                    usleep((useconds_t)1000000);
                }
                lockTransferQueue(pLVKey, "contribIdStopped - Waiting to determine if a transfer definition is restartable");

                // Check to make sure the job still exists after releasing/re-acquiring the lock
                // NOTE: The connection name is optional, and is potentially different from what
                //       is in our local metadata if we are processing this via the async request file.
                string l_ConnectionName = string();
                if (!jobStillExists(l_ConnectionName, pLVKey, pLV_Info, (BBTagInfo*)0, pJobId, pContribId))
                {
                    rc = -1;
                    l_Continue = 0;
                }
            }
        }

        if (rc == -2)
        {
            if (++l_Attempts <= 2)
            {
                rc = prepareForRestartOriginalServerDead(pConnectionName, pLVKey, pHandle, BBJob(pJobId, pJobStepId), pContribId);
                switch (rc)
                {
                    case 1:
                    {
                        // Reset of cross bbServer metadata was successful...  Continue...
                        LOG(bb,info) << "ContribId " << pContribId << " was found in the cross bbServer metadata and was successfully stopped" \
                                  << " after the original bbServer was unresponsive";
                        l_AllDone = false;
                    }
                    break;

                    case 2:
                    {
                        // Indicate not to restart this transfer definition
                        rc = 0;
                        LOG(bb,info) << "ContribId " << pContribId << " was found in the cross bbServer metadata, but no file associated with the transfer definition needed to be restarted." \
                                     << " Most likely, the transfer completed for the contributor or was canceled. Therefore, the transfer definition cannot be restarted. See any previous messages.";
                    }
                    break;

                    default:
                    {
                        // Indicate not to restart this transfer definition
                        rc = 0;
                        LOG(bb,error) << "Attempt to reset the cross bbServer metadata for the transfer definition associated with contribid " << pContribId << " to stopped failed." \
                                      << " Therefore, the transfer definition cannot be restarted. See any previous messages.";
                    }
                    break;
                }
            }
            else
            {
                // Indicate not to restart this transfer definition
                rc = 0;
                LOG(bb,error) << "ContribId " << pContribId << " found in the cross bbServer metadata, but it does not have a status of stopped." \
                              << " Most likely, the transfer completed for the contributor or was canceled. Therefore, the transfer definition cannot be restarted. See any previous messages.";
            }
        }
    }

    FL_Write6(FLMetaData, XF_ContribIdStopped_End, "xfer contribIdStopped, counter=%ld, jobid=%ld, handle=%ld, contribid=%ld, attempts=%ld, rc=%ld", l_FL_Counter, pJobId, pHandle, pContribId, l_RetryAttempts, rc);

    return rc;
}

int doForceStopTransfer(const LVKey* pLVKey, ContribIdFile* pContribIdFile, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId)
{
    int rc = becomeUser(pContribIdFile->getUserId(), pContribIdFile->getGroupId());
    if (!rc)
    {
        // We mark this transfer definition as stopped
        // Update the status for the ContribId and Handle files in the xbbServer data...
        rc = ContribIdFile::update_xbbServerContribIdFile(pLVKey, pJobId, pJobStepId, pHandle, pContribId, BBTD_Stopped, 1);

        // We mark this transfer definition as canceled
        // Now update the status for the ContribId and Handle files in the xbbServer data...
        if (!rc)
        {
            rc = ContribIdFile::update_xbbServerContribIdFile(pLVKey, pJobId, pJobStepId, pHandle, pContribId, BBTD_Canceled, 1);
            // We mark this transfer definition as all extents processed
            // Now update the status for the ContribId and Handle files in the xbbServer data...
            if (!rc)
            {
                rc = ContribIdFile::update_xbbServerContribIdFile(pLVKey, pJobId, pJobStepId, pHandle, pContribId, BBTD_All_Extents_Transferred, 1);
                // We mark this transfer definition as all files closed
                // Now update the status for the ContribId and Handle files in the xbbServer data...
                if (!rc)
                {
                    rc = ContribIdFile::update_xbbServerContribIdFile(pLVKey, pJobId, pJobStepId, pHandle, pContribId, BBTD_All_Files_Closed, 1);
                }
            }
        }

        if (!rc)
        {
            // Indicate success...  Restart the transfer definition...
            rc = 1;
        }

        becomeUser(0,0);
    }
    else
    {
        rc = -1;
        stringstream errorText;
        errorText << "becomeUser failed when attempting to force stop the transfer definition associated with an unknown CN hostname, jobid " \
                  << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", contribId " << pContribId \
                  << " when attempting to become uid=" << pContribIdFile->getUserId() << ", gid=" << pContribIdFile->getGroupId();
        bberror << err("error.uid", pContribIdFile->getUserId()) << err("error.gid", pContribIdFile->getGroupId());
        LOG_ERROR_TEXT_RC(errorText, rc);
    }

    return rc;
}

int doTransfer(LVKey& pKey, const uint64_t pHandle, const uint32_t pContribId, BBTransferDef* pTransferDef, Extent* pExtent)
{
    ENTRY(__FILE__,__FUNCTION__);

    int rc = 0;
    stringstream errorText;

    if (pExtent->flags & BBI_TargetSSD || pExtent->flags & BBI_TargetPFS)
    {
        if ((!pExtent->isCP_Transfer()) && pExtent->getLength())
        {
            LOG(bb,debug) << "Performing asynchronous transfer of extent";

            // Honor transfer order boundaries within non-zero bundle ids
            uint16_t l_BundleId = pExtent->getBundleID();
            if (l_BundleId)
            {
                // Non-zero bundle id
                vector<uint32_t> l_ClosedFiles;
                vector<uint32_t>::iterator it;
                bfs::path l_HandleFilePath;
                ContribIdFile* l_ContribIdFile = 0;
                uint32_t l_SourceIndex;
                uint16_t l_BundleId2;
                uint16_t l_TransferOrder = pExtent->getTransferOrder();
                uint16_t l_TransferOrder2;

                bool l_AllDone = false;
                uint32_t i = 0;
                while (!l_AllDone)
                {
                    // Inspect each extent within the transfer definition, looking for the same bundle id
                    // and a higher priority transfer order...
                    l_AllDone = true;
                    for (auto& e : pTransferDef->extents)
                    {
                        if (!rc)
                        {
                            l_BundleId2 = e.getBundleID();
                            l_TransferOrder2 = e.getTransferOrder();
                            if (l_BundleId == l_BundleId2 && l_TransferOrder > l_TransferOrder2) {
                                // Bundle id of extent to be transferred is the same as the extent being inspected -and-
                                // transfer order of extent to be transfered has a lower priority (greater value)
                                // than the extent being inspected.
                                //
                                // See if we have already recorded the associated source file as closed.
                                l_SourceIndex = e.sourceindex;
                                it = find(l_ClosedFiles.begin(), l_ClosedFiles.end(), l_SourceIndex);
                                if (it == l_ClosedFiles.end())
                                {
                                    // Source file for the extent being inspected has not already been
                                    // recorded as closed.
                                    // If not already done, load the contribid file.
                                    if (!l_ContribIdFile)
                                    {
                                        if (l_HandleFilePath.string().empty())
                                        {
                                            l_HandleFilePath = config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH);
                                            l_HandleFilePath /= bfs::path(to_string(pTransferDef->getJobId()));
                                            l_HandleFilePath /= bfs::path(to_string(pTransferDef->getJobStepId()));
                                            l_HandleFilePath /= bfs::path(to_string(pHandle));
                                        }
                                        int rc2 = ContribIdFile::loadContribIdFile(l_ContribIdFile, &pKey, l_HandleFilePath, pContribId);
                                        switch (rc2)
                                        {
                                            case 1:
                                            {
                                                // NOTE: There is is a single set of flags for the source/target file pair in the
                                                //       ContribId file.  Therefore, we cannot use the source/target index directly
                                                //       into the array of files and it must be adjusted.
                                                if ((l_ContribIdFile->files[l_SourceIndex/2]).flags & BBTD_All_Files_Closed)
                                                {
                                                    // Source file for extent being inspected has been closed.
                                                    // Record that fact.
                                                    l_ClosedFiles.push_back(l_SourceIndex);
                                                }
                                                else
                                                {
                                                    // Source file for extent being inspected has NOT been closed.
                                                    // Delay a bit for it to clear the in-flight queue and for the file to be closed...
                                                    // NOTE: If for some reason I/O is 'stuck' and does not return, the following is an infinite loop...
                                                    //       \todo - What to do???  @DLH
                                                    unlockTransferQueue(&pKey, "doTransfer - Waiting for the close of source file having higher priority transfer order");
                                                    {
                                                        // NOTE: Currently set to log after 1 second of not being able to clear, and every 10 seconds thereafter...
                                                        if ((i++ % 40) == 4)
                                                        {
                                                            FL_Write(FLDelay, BundleId, "Attempting to transfer extent from bundle id %ld with a transfer order of %ld. Waiting for the source file with a source index of %ld and transfer order of %ld to be closed. Delay of 250 milliseconds.",
                                                                     (uint64_t)l_BundleId, (uint64_t)l_TransferOrder, (uint64_t)l_SourceIndex, (uint64_t)l_TransferOrder2);
                                                            LOG(bb,info) << ">>>>> DELAY <<<<< doTransfer(): Attempting to transfer extent from bundle id " << l_BundleId << " with a transfer order of " << l_TransferOrder << ". Waiting for the source file with a source index of " << l_SourceIndex \
                                                                         << " and transfer order of " << l_TransferOrder2 << " to be closed. Delay of 250 milliseconds.";
                                                        }
                                                        usleep((useconds_t)250000);

                                                        // Delete the loaded contribid file so that we pick up new flag values for next iteration...
                                                        delete l_ContribIdFile;
                                                        l_ContribIdFile = 0;

                                                        // Setup to iterate through the extents again...
                                                        l_AllDone = false;
                                                    }
                                                    lockTransferQueue(&pKey, "doTransfer - Waiting for the close of source file having higher priority transfer order");
                                                }

                                                break;
                                            }
                                            case 0:
                                            {
                                                rc = -1;
                                                errorText << "ContribId " << pContribId << " could not be found in the contrib file for jobid " << pTransferDef->getJobId() << ", jobstepid " << pTransferDef->getJobStepId() << ", handle " << pHandle << ", " << pKey << ", using handle path " << l_HandleFilePath.string();
                                                LOG_ERROR_TEXT_RC(errorText, rc);
                                                break;
                                            }
                                            default:
                                            {
                                                rc = -1;
                                                errorText << "Could not load the contribid file for jobid " << pTransferDef->getJobId() << ", jobstepid " << pTransferDef->getJobStepId() << ", handle " << pHandle << ", " << pKey << ", contribid " << pContribId << ", using handle path " << l_HandleFilePath.string();
                                                LOG_ERROR_TEXT_RC(errorText, rc);
                                            }
                                        }
                                    }
                                }   // Source file has already been recorded as closed...
                            }   // Bundle id for extent to be transferred is not equal -or- extent has higher priority transfer order
                        }
                    }
                }
                if (l_ContribIdFile)
                {
                    delete l_ContribIdFile;
                    l_ContribIdFile = 0;
                }
            }   // Bundle id for extent to be transferred is zero.  No transfer order boundary to honor.

            if (!rc)
            {
                BBIO* l_IO = pTransferDef->iomap[l_BundleId];
                if (l_IO)
                {
                    LOG(bb,debug) << "BBIO: performIO(): tdef=" << hex << uppercase << setfill('0') << pTransferDef << setfill(' ') << nouppercase << dec \
                                  << ", handle=" << pHandle << ", contribid=" << pContribId;
                    LOG(bb,debug) << "                   " << *pExtent;
                    rc = l_IO->performIO(pKey, pExtent);
                    if (!rc)
                    {
                        pTransferDef->incrSizeTransferred(pExtent);
                    }
                }
                else
                {
                    rc = -1;
                    errorText << "Could not perform I/O for bundleID " << (uint64_t)l_BundleId;
                    bberror << err("error.bundleID", (uint64_t)l_BundleId);
                    LOG_ERROR_TEXT_RC_AND_RAS(errorText, rc, bb.internal.unkBndl);
                }
            }
        }
        else
        {
            // Dummy extent for file with no extents
            ContribIdFile::update_xbbServerFileStatus(&pKey, pTransferDef, pHandle, pContribId, pExtent, (BBTD_Extents_Enqueued | BBTD_All_Extents_Transferred | BBTD_All_Files_Closed));
        }
    }

    else if(pExtent->flags & BBI_TargetPFSPFS)
    {
        BBSTATUS l_Status = BBFULLSUCCESS;

        bs::error_code err;
        bfs::copy_file(bfs::path(pTransferDef->files[pExtent->sourceindex]), bfs::path(pTransferDef->files[pExtent->targetindex]), bfs::copy_option::overwrite_if_exists, err);

        if (err.value())
        {
            l_Status = BBFAILED;
            pTransferDef->setFailed(&pKey, pHandle, pContribId);
        }

        switch(l_Status) {
            case BBFULLSUCCESS:
                LOG(bb,info) << "PFS copy complete for file " << pTransferDef->files[pExtent->sourceindex] \
                             << ", handle = " << pHandle << ", contribid = " << pContribId << ", sourceindex = " << pExtent->sourceindex;
                break;

            case BBFAILED:
                pExtent->len = 0;
                LOG(bb,info) << "PFS copy failed for file " << pTransferDef->files[pExtent->sourceindex] \
                             << ", handle = " << pHandle << ", contribid = " << pContribId << ", sourceindex = " << pExtent->sourceindex;
                break;

            case BBCANCELED:
            case BBSTOPPED:
            default:
                // Not possible...
                break;
        }

        // Dummy extent for file with no extents
        ContribIdFile::update_xbbServerFileStatus(&pKey, pTransferDef, pHandle, pContribId, pExtent, (BBTD_Extents_Enqueued | BBTD_All_Extents_Transferred | BBTD_All_Files_Closed));
    }

    else if(pExtent->flags & BBI_TargetSSDSSD)
    {
        if (pExtent->flags & BBTD_Failed)
        {
            pTransferDef->setFailed(&pKey, pHandle, pContribId);
            LOG(bb,info) << "Local compute node SSD copy failed for file " << pTransferDef->files[pExtent->sourceindex] \
                         << ", handle = " << pHandle << ", contribid = " << pContribId << ", sourceindex = " << pExtent->sourceindex;
        }
        else
        {
            LOG(bb,info) << "Local compute node SSD copy complete for file " << pTransferDef->files[pExtent->sourceindex] \
                         << ", handle = " << pHandle << ", contribid = " << pContribId << ", sourceindex = " << pExtent->sourceindex;
        }

        // Dummy extent for file with no extents
        ContribIdFile::update_xbbServerFileStatus(&pKey, pTransferDef, pHandle, pContribId, pExtent, (BBTD_Extents_Enqueued | BBTD_All_Extents_Transferred | BBTD_All_Files_Closed));
    }
    else
    {
        rc = -1;
        errorText << "Invalid flag setting of " << hex << uppercase << setfill('0')
                  << pExtent->flags << setfill(' ') << nouppercase << dec << " for an extent";
        bberror << err("error.flags", pExtent->flags);
        LOG_ERROR_TEXT_RC_AND_RAS(errorText, rc, bb.internal.unkFlags);
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}

int fileToBeRestarted(const LVKey* pLVKey, BBTransferDef* pTransferDef, Extent pExtent, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId)
{
    stringstream errorText;
    int rc = 0;

    // Check the cross bbServer metadata to see if this transfer definition
    // has a stopped status...
    // NOTE: For now, we always use the cross bbServer metadata because we have to
    //       see if the file is stopped.  That indication is currently only in the
    //       cross bbServer metadata.
    ContribIdFile* l_ContribIdFile = 0;
    bfs::path l_HandleFilePath(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
    l_HandleFilePath /= bfs::path(to_string(pJobId));
    l_HandleFilePath /= bfs::path(to_string(pJobStepId));
    l_HandleFilePath /= bfs::path(to_string(pHandle));
    rc = ContribIdFile::loadContribIdFile(l_ContribIdFile, l_HandleFilePath, pContribId);
    if (rc >= 0)
    {
        if (rc == 1 && l_ContribIdFile)
        {
            rc = l_ContribIdFile->fileToBeRestarted(pExtent);
        }
        else
        {
            rc = 0;
            LOG(bb,info) << "fileToBeRestarted(): ContribId " << pContribId << " was not found in the cross bbServer metadata (ContribIdFile pointer is NULL)." \
                         << " All transfers for this contributor may have already finished.  See previous messages.";
        }
    }
    else
    {
        rc = 0;
        LOG(bb,info) << "fileToBeRestarted(): Error occurred when attempting to load the contrib file for contribid " << pContribId \
                     << " (Negative rc from loadContribIdFile()). All transfers for this contributor may have already finished.  See previous messages.";
    }

    if (l_ContribIdFile)
    {
        delete l_ContribIdFile;
        l_ContribIdFile = 0;
    }

    return rc;
}

int forceStopTransfer(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId)
{
    int rc = 0;
    bool l_StopDefinition = false;

    ContribIdFile* l_ContribIdFile = 0;
    bfs::path l_HandleFilePath(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
    l_HandleFilePath /= bfs::path(to_string(pJobId));
    l_HandleFilePath /= bfs::path(to_string(pJobStepId));
    l_HandleFilePath /= bfs::path(to_string(pHandle));
    int l_RC = ContribIdFile::loadContribIdFile(l_ContribIdFile, pLVKey, l_HandleFilePath, pContribId);
    switch (l_RC)
    {
        case 1:
        {
            // We stop any transfer definition that does not have all of its files transferred/closed -or-
            // has a failed transfer
            if ((!l_ContribIdFile->allFilesClosed()) || l_ContribIdFile->anyFilesFailed())
            {
                l_StopDefinition = true;
            }

            break;
        }
        case 0:
        {
            rc = -1;
            LOG(bb,error) << "ContribId " << pContribId << "could not be found in the contribid file for jobid " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", " << *pLVKey << ", using handle path " << l_HandleFilePath;

            break;
        }
        default:
        {
            rc = -1;
            LOG(bb,error) << "Could not load the contribid file for jobid " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", contribid " << pContribId << ", " << *pLVKey << ", using handle path " << l_HandleFilePath;
        }
    }

    if (l_StopDefinition)
    {
        rc = doForceStopTransfer(pLVKey, l_ContribIdFile, pJobId, pJobStepId, pHandle, pContribId);
    }
    else
    {
        if (!rc)
        {
            // ContribIdFile was loaded, but it was not marked as stopped.  No files need to be restarted...
            rc = 2;
            LOG(bb,info) << "A stop transfer request was made for the transfer definition associated with " << *pLVKey << ", jobid " << pJobId << ", jobstepid " << pJobStepId \
                         << ", handle " << pHandle << ", contribid " << pContribId << ", however no extents are left to be transferred (via BBTransferDef), rc = " << rc \
                         << ". Stop transfer request ignored.";
        }
        else
        {
            // Indicate not to restart this transfer definition
            rc = 0;
        }
    }

    if (l_ContribIdFile)
    {
        delete l_ContribIdFile;
        l_ContribIdFile = NULL;
    }

    return rc;
}

void markTransferFailed(const LVKey* pLVKey, BBTransferDef* pTransferDef, BBLV_Info* pLV_Info, uint64_t pHandle, uint32_t pContribId)
{
    if (pTransferDef)
    {
        // Mark the transfer definition failed
        pTransferDef->setFailed(pLVKey, pHandle, pContribId);

        // Sort the extents, moving the extents for the failed file, and all other files
        // for the transfer definition, to the front of the work queue so they are immediately removed...
        TRANSFER_QUEUE_RELEASED l_LockWasReleased = TRANSFER_QUEUE_LOCK_NOT_RELEASED;
        pLV_Info->cancelExtents(pLVKey, &pHandle, &pContribId, l_LockWasReleased, DO_NOT_REMOVE_TARGET_PFS_FILES);
    }
    else
    {
        LOG(bb,error) << "Could not mark the handle as failed at (3) for " << *pLVKey \
                      << ", handle " << pHandle << ", contribid " << pContribId \
                      << " because the pointer to the transfer definition was passed as NULL.";
    }

    return;
}

int prepareForRestart(const std::string& pConnectionName, const LVKey* pLVKey, BBLV_Info* pLV_Info, BBTagInfoMap* pTagInfoMap, BBTagInfo* pTagInfo,
                      const uint64_t pHandle, BBTagID pTagId, BBJob pJob, const int32_t pContribId, BBTransferDef* pOrigTransferDef, BBTransferDef* pRebuiltTransferDef, const int pPass)
{
    ENTRY(__FILE__,__FUNCTION__);

    int rc = 1;     // Default is to not restart the transfer definition...
    stringstream errorText;

    LOG(bb,debug) << "xfer::prepareForRestart(): Pass " << pPass;

    if (pPass == FIRST_PASS)
    {
        if (pTagInfo)
        {
            // NOTE: If pOrigTransferDef is NULL (restart on different bbServer), contribIdStopped() will use the cross-bbServer metadata
            //       to determine if this contribid has been successfully stopped.
            // NOTE: If the cross-bbServer metadata is used to determine the stopped state, this method call may block for a while
            //       waiting for another bbServer to set the stopped state for the transfer definition.
            rc = contribIdStopped(pConnectionName, pLVKey, pLV_Info, pOrigTransferDef, pRebuiltTransferDef, pJob.getJobId(), pJob.getJobStepId(), pHandle, pContribId);
            switch (rc)
            {
                case 1:
                {
                    // Restart this transfer definition
                    rc = pLV_Info->prepareForRestart(pConnectionName, pLVKey, pTagInfo, pJob, pHandle, pContribId, pOrigTransferDef, pRebuiltTransferDef, pPass);
                }
                break;

                case 0:
                    // Do not restart this transfer definition...
                    rc = 1;
                    break;

                default:
                    // Some other error...  Return the error...
                    break;
            }
        }
        else
        {
            rc = -1;
            errorText << "Local taginfo not found for transfer definition associated with jobid " << pJob.getJobId() << ", jobstepid " << pJob.getJobStepId() \
                      << ", handle " << pHandle << ", contribid " << pContribId << ".";
            LOG_ERROR_TEXT_RC(errorText, rc);
        }
    }
    else
    {
        rc = pLV_Info->prepareForRestart(pConnectionName, pLVKey, pTagInfo, pJob, pHandle, pContribId, pOrigTransferDef, pRebuiltTransferDef, pPass);
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}

int jobStillExists(const std::string& pConnectionName, const LVKey* pLVKey, BBLV_Info* pLV_Info, BBTagInfo* pTagInfo, const uint64_t pJobId, const uint32_t pContribId)
{
    int rc = 0;

    // NOTE: We know that the local cache of metadata for a jobid is still
    //       valid if we can find the LVKey for the jobid in the metadata...
    rc = metadata.verifyJobIdExists(pConnectionName, pLVKey, pJobId);
    if (!rc)
    {
        LOG(bb,info) << "jobStillExists(): JobId " << pJobId << " no longer exists for input connection " << pConnectionName \
                     << ", " << *pLVKey << ", contribid " << pContribId \
                     << ", BBLV_Info* 0x" << pLV_Info << ", BBTagInfo* 0x" << pTagInfo;
    }

    return rc;
}

int prepareForRestartOriginalServerDead(const std::string& pConnectionName, const LVKey* pLVKey, const uint64_t pHandle, BBJob pJob, const int32_t pContribId)
{
    return forceStopTransfer(pLVKey, pJob.getJobId(), pJob.getJobStepId(), pHandle, pContribId);
}

int sendTransferProgressMsg(const string& pConnectionName, const LVKey* pLVKey, const uint64_t pJobId, const uint32_t pCount, const Extent* pExtent)
{
    ENTRY(__FILE__,__FUNCTION__);

    int rc = 0;
    txp::Msg* l_Progress = 0;
    txp::Msg::buildMsg(txp::BB_TRANSFER_PROGRESS, l_Progress);

    Uuid lv_uuid = pLVKey->second;
    char lv_uuid_str[LENGTH_UUID_STR] = {'\0'};
    lv_uuid.copyTo(lv_uuid_str);

    // NOTE:  The char array is copied to heap by addAttribute and the storage for
    //        the logical volume uuid attribute is owned by the message facility.
    //        Our copy can then go out of scope...
    l_Progress->addAttribute(txp::uuid, lv_uuid_str, sizeof(lv_uuid_str), txp::COPY_TO_HEAP);
    l_Progress->addAttribute(txp::jobid, pJobId);
    l_Progress->addAttribute(txp::count, pCount);
    l_Progress->addAttribute(txp::maxlba, (pExtent ? pExtent->lba.maxkey : 0));
    l_Progress->addAttribute(txp::offset, (pExtent ? pExtent->lba.start : 0));
    l_Progress->addAttribute(txp::length, (pExtent ? (uint64_t)pExtent->len : 0));

    // Send the progress message
    try
    {
        rc = sendMessage(pConnectionName,l_Progress);
        if (rc) LOG(bb,info) << "name not found for name="<<pConnectionName<<" @"<<  __func__;
    }
    catch(exception& e)
    {
        LOG(bb,warning) << "sendTransferProgressMsg(): Exception thrown: " << e.what();
        assert(strlen(e.what())==0);
    }

    delete l_Progress;

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}

// If invoked as part of the stageout end processing, no extents will actually
// be transferred.  But rather, the extents will be added/removed
// to/from the inflight queue.  As part of removing from the inflight queue,
// the necessary metadata bookkeeping is performed, including unlocking
// files and sending messages to bbproxy which will cause files to be
// unlocked on the compute node.  The unlocking of the files on the
// compute node allows for the ensuing Remove Logical Volume, which
// will first unmount the file system from the logical volume.
//
// Also, once we make it to this method, it is imperative that we
// remove the extent from the vector of extents to transfer for this
// LVKey.  This is because we have already removed this extent
// from the LVKey's work queue.
void transferExtent(WorkID& pWorkItem, ExtentInfo& pExtentInfo)
{
    ENTRY(__FILE__,__FUNCTION__);

    BBTransferDef* l_TransferDef = 0;
    BBTagInfo* l_TagInfo = 0;
    Extent* l_Extent = 0;
    bool l_ExtentRemovedFromVectorOfExtents = false;

    // Get connection...
    string l_ConnectionName = pWorkItem.getConnectionName();

    LVKey l_Key = pWorkItem.getLVKey();

//    pWorkItem.dump("info", "transferExtent(): Processing ");

    // Process request...
    BBTagID l_TagId = pWorkItem.getTagId();
    BBLV_Info* l_LV_Info = metadata.getLV_Info(&l_Key);
    if (l_LV_Info)
    {
        bool l_MarkFailed = false;
        bool l_MarkTransferDefinitionCanceled = false;
        l_Extent = pExtentInfo.extent;
        l_TransferDef = pExtentInfo.transferDef;
        l_TagInfo = l_LV_Info->getTagInfo(l_TagId);
        if (l_TagInfo)
        {
            try
            {
                // Add this extent to the in-flight list...
                l_LV_Info->addToInFlight(l_ConnectionName, &l_Key, pExtentInfo);

                // Remove this extent from the vector of extents to work on...
                l_LV_Info->extentInfo.removeExtent(l_Extent);
                l_ExtentRemovedFromVectorOfExtents = true;

                // NOTE: The code below determines if an extent will be passed to doTransfer().  If this code
                //       is modified, the similar code in WRKQE::processBucket() should also be checked.  @DLH
                // NOTE: We do not have to consider the 'stopped' flag here, as those transfer definitions are
                //       also always marked as 'canceled'.
                if (!l_LV_Info->stageOutEnded())
                {
                    if (!(l_LV_Info->resizeLogicalVolumeDuringStageOut() && l_LV_Info->stageOutStarted() && (l_Extent->flags & BBI_TargetSSD)))
                    {
                        if (l_TransferDef->failed()) bberror << bailout;
                        //  Transfer definition not marked as failed...
                        if (!l_TagInfo->canceled())
                        {
                            //  Handle not marked as canceled...
                            if (l_TransferDef->canceled()) bberror << bailout;
                            //  Transfer definition not marked as canceled...
                            try
                            {
                                // Perform the transfer of data
                                int rc = doTransfer(l_Key, pExtentInfo.handle, pExtentInfo.contrib, l_TransferDef, l_Extent);
                                if (rc)
                                {
                                    stringstream errorText;
                                    errorText << "transferExtent(): doTransfer() returned rc " << rc;
                                    LOG_ERROR_TEXT_RC_AND_RAS(errorText, rc, bb.internal.rcDoXfer);
                                    l_MarkFailed = true;
                                }
                            }
                            catch(ExceptionBailout& e) { }
                            catch(exception& e)
                            {
                                LOG_ERROR_WITH_EXCEPTION_AND_RAS(__FILE__, __FUNCTION__, __LINE__, e, bb.internal.expDoXfer);
                                l_MarkFailed = true;
                            }
                        }
                        else
                        {
                            if (!l_TransferDef->canceled())
                            {
                                l_MarkTransferDefinitionCanceled = true;
                            }
                        }
                    }
                    else
                    {
                        // The job's logical volume is being resized during stageout and stageout has started.
                        // Therefore, the file system has been torn down and the current transfer operation
                        // is to transfer data to the SSD.  Since the file system has been torn down and
                        // the target extent must be for the job's logical volume, there is no need to do
                        // this transfer any longer.
                        // NOTE:  \todo Is this a failure or cancel case??? @DLH
                        LOG(bb,info) << "Stageout started... Skipping transfer to job's logical volume from pfs"
                                     << std::hex << std::uppercase << setfill('0')
                                     << " src offset=0x" << setw(8) << l_Extent->start
                                     << " dst offset=0x" << setw(8) << l_Extent->lba.start
                                     << " size=0x" << setw(8) << l_Extent->len
                                     << setfill(' ') << std::nouppercase << std::dec;
                    }
                }
                else
                {
                    // Stageout processing has already ended, so there is no need to do
                    // this transfer any longer.
                    LOG(bb,info) << "Stageout ended... Skipping transfer from/to job's logical volume"
                                 << std::hex << std::uppercase << setfill('0')
                                 << " src offset=0x" << setw(8) << l_Extent->start
                                 << " dst offset=0x" << setw(8) << l_Extent->lba.start
                                 << " size=0x" << setw(8) << l_Extent->len
                                 << setfill(' ') << std::nouppercase << std::dec;
                    l_MarkFailed = true;
                }
            }
            catch(ExceptionBailout& e) { }
            catch(exception& e)
            {
                LOG(bb,error) << "Exception thrown in transferExtent() when attempting to transfer an extent";
                LOG_ERROR_WITH_EXCEPTION_AND_RAS(__FILE__, __FUNCTION__, __LINE__, e, bb.internal.te_1);
                l_MarkFailed = true;
            }

            try
            {
                if (!l_ExtentRemovedFromVectorOfExtents)
                {
#ifndef __clang_analyzer__
                    l_ExtentRemovedFromVectorOfExtents = true;
#endif
                    l_LV_Info->extentInfo.removeExtent(l_Extent);
                }
            }
            catch(ExceptionBailout& e) { }
            catch(exception& e)
            {
                LOG(bb,error) << "Exception thrown in transferExtent() when attempting to remove an extent from the vector of extents to transfer for an LVKey";
                LOG_ERROR_WITH_EXCEPTION_AND_RAS(__FILE__, __FUNCTION__, __LINE__, e, bb.internal.te_2);
                l_MarkFailed = true;
            }
            try
            {
                if (l_MarkFailed)
                {
                    // Took an error on the transfer...
                    l_MarkFailed = false;
                    // Mark the transfer definition and associated handle as failed
                    markTransferFailed(&l_Key, l_TransferDef, l_LV_Info, pExtentInfo.handle, pExtentInfo.contrib);
                }

                if (l_MarkTransferDefinitionCanceled)
                {
                    // Cancel has been issued for the handle...
#ifndef __clang_analyzer__
                    l_MarkTransferDefinitionCanceled = false;
#endif
                    // Mark the transfer definition
                    l_TransferDef->setCanceled(&l_Key, pExtentInfo.handle, pExtentInfo.contrib);
                    // NOTE: If the handle is marked as stopped, mark the transfer definition as stopped also...
                    //       The reason the handle might be marked as canceled is because it is stopped.
                    if (l_TagInfo->stopped())
                    {
                        l_TransferDef->setStopped(&l_Key, pExtentInfo.handle, pExtentInfo.contrib);
                    }
                }
            }
            catch(ExceptionBailout& e) { }
            catch(exception& e)
            {
                LOG(bb,error) << "Exception thrown in transferExtent() when attempting to mark a transfer definition as failed or cancelled";
                LOG_ERROR_WITH_EXCEPTION_AND_RAS(__FILE__, __FUNCTION__, __LINE__, e, bb.internal.te_3);
            }

            try
            {
                // Remove this extent from the in-flight list...
                l_LV_Info->removeFromInFlight(l_ConnectionName, &l_Key, l_TagInfo, pExtentInfo);
            }
            catch(ExceptionBailout& e) { }
            catch(exception& e)
            {
                LOG(bb,error) << "Exception thrown in transferExtent() when attempting to remove an extent from the inflight queue";
                LOG_ERROR_WITH_EXCEPTION_AND_RAS(__FILE__, __FUNCTION__, __LINE__, e, bb.internal.te_4);
            }

            try
            {
                // For now, here is where we send progress messages back to bbproxy...
                // NOTE:  To send a progress message, we must have sorted the extents -and-
                //        the logical volume is to be resized during stageout
                // NOTE:  \todo - Need to update this for multiple CN having different LVKeys...  @DLH
                // NOTE:  \todo - Need to think about this processing WRT 'stopped' processing for a transfer definition...  @DLH
                size_t l_NumberOfExtents = l_LV_Info->getNumberOfExtents();
                if (l_LV_Info->resizeLogicalVolumeDuringStageOut() &&
                    l_LV_Info->stageOutStarted() &&
                    ((!l_NumberOfExtents) || ResizeSSD_Timer.popped(ResizeSSD_TimeInterval)))
                {
                    // NOTE:  getMinimumTrimExtent() returns a pointer to the extent with the LBA value that we can return to bbProxy.
                    //        Because it might be a pointer to an extent in the in-flight list, send the message here with the transfer queue
                    //        lock held...
                    if (sendTransferProgressMsg(l_ConnectionName, &l_Key, l_LV_Info->getJobId(), (uint32_t)l_NumberOfExtents, l_LV_Info->extentInfo.getMinimumTrimExtent()))
                    {
                        LOG(bb,warning) << "Error occurred when sending transfer progress message back to bbproxy";
                    }
                }
            }
            catch(ExceptionBailout& e) { }
            catch(exception& e)
            {
                LOG(bb,error) << "Exception thrown in transferExtent() when attempting to send a transfer progress message to bbProxy";
                LOG_ERROR_WITH_EXCEPTION_AND_RAS(__FILE__, __FUNCTION__, __LINE__, e, bb.internal.te_5);
            }
        }
        else
        {
            stringstream errorText;
            errorText << "Exception thrown in transferExtent() when attempting to access the local metadata (taginfo)";
            LOG_ERROR_TEXT_AND_RAS(errorText, bb.internal.te_6);
        }
    }
    else
    {
        stringstream errorText;
        errorText << "Exception thrown in transferExtent() when attempting to access the local metadata (taginfo2)";
        LOG_ERROR_TEXT_AND_RAS(errorText, bb.internal.te_7);
    }

    EXIT(__FILE__,__FUNCTION__);
    return;
}

void* transferWorker(void* ptr)
{
    queue<WorkID>* l_WrkQ = 0;
    WRKQE* l_WrkQE = 0;
    Extent* l_Extent = 0;
    WorkID l_WorkItem;
    LVKey l_Key = LVKey_Null;
    BBTagID l_TagId;
    ExtentInfo l_ExtentInfo;
    BBLV_Info* l_LV_Info;
    stringstream errorText;

    double l_ThreadDelay = 0;
    double l_TotalDelay = 0;
    int l_ConsecutiveSuspendedWorkQueuesNotProcessed = 0;
    bool l_Repost = false;
    threadLocalTrackSyscallPtr = getSysCallTracker();

    while (1) {
  	    becomeUser(0,0);

        wrkqmgr.wait();
        lockTransferQueue((LVKey*)0, "%transferWorker - Prep to transfer extent");

        l_WrkQ = 0;
        l_WrkQE = 0;
        l_LV_Info = 0;
        l_Repost = true;

        try
        {
            // Find some work to do...
            // NOTE: The findWork() invocation is the only time we will
            //       look for work on the high priority work queue.
            //       If there is work on the high priority work queue,
            //       the high priority work queue is returned.
            // NOTE: The findWork() invocation will return suspended work
            //       queues.
            bool l_WorkRemains = true;
            if (!wrkqmgr.findWork((LVKey*)0, l_WrkQE))
            {
                if (l_WrkQE)
                {
                    // Get the LVKey for this work item...
                    l_Key = (l_WrkQE->getWrkQ()->front()).getLVKey();
                    if (l_WrkQE != HPWrkQE)
                    {
                        // An LVKey work queue was returned
                        if (l_WrkQE->getWrkQ_Size())
                        {
                            // Workqueue entries exist...
                            l_WorkItem = l_WrkQE->getWrkQ()->front();
                            l_LV_Info = metadata.getLV_Info(&l_Key);
                            if (l_LV_Info)
                            {
                                l_TagId = l_WorkItem.getTagId();
                                l_ExtentInfo = l_LV_Info->getNextExtentInfo();
                                l_Extent = l_ExtentInfo.extent;
                                l_ThreadDelay = 0;  // in micro-seconds
                                l_TotalDelay = 0;   // in micro-seconds
                                wrkqmgr.processThrottle(&l_Key, l_WrkQE, l_LV_Info, l_TagId, l_ExtentInfo, l_Extent, l_ThreadDelay, l_TotalDelay);
                                if (l_ThreadDelay > 0)
                                {
                                    LOG(bb,debug)  << "transferWorker(): l_ThreadDelay = " << l_ThreadDelay << ", l_TotalDelay = " << l_TotalDelay;
                                    if (!wrkqmgr.delayMessageSent())
                                    {
                                        wrkqmgr.setDelayMessageSent(true);
                                        FL_Write6(FLDelay, Throttle, "Total delay for %ld usecs, thread delay for %ld usecs, %ld work queues, throttled work queue %p currently has %ld entries. Throttle rate is %ld bytes/sec.",
                                                  (uint64_t)l_TotalDelay, (uint64_t)l_ThreadDelay, (uint64_t)wrkqmgr.getNumberOfWorkQueues(), (uint64_t)l_WrkQE, (uint64_t)l_WrkQE->getWrkQ()->size(), (uint64_t)l_WrkQE->getRate());
                                        LOG(bb,debug)  << ">>>>> DELAY <<<<< transferWorker(): For LVKey " << l_Key << " with " << l_WrkQE->getWrkQ()->size() \
                                                       << " work queue entrie(s) for " << (float)l_TotalDelay/1000000.0 << " seconds.";
                                        LOG(bb,debug)  << "                                    Throttle rate is " << l_WrkQE->getRate() << " bytes/sec and current bucket value is " << l_WrkQE->getBucket() << ".";
                                        LOG(bb,debug)  << "                                    " << l_LV_Info->getNumberOfExtents() << " extents are ready to transfer and the current length of extent to transfer is " \
                                                       << l_Extent->getLength() << " bytes.";
                                        LOG(bb,debug)  << "                                    Thread delay is " << (float)l_ThreadDelay/1000000.0 << " seconds.";
                                        if (wrkqmgr.getDumpOnDelay())
                                        {
                                            wrkqmgr.dump("info", " Work Queue Mgr @ Delay", DUMP_ALWAYS);
                                        }
                                    }
                                    unlockTransferQueue(&l_Key, "%transferWorker - Before throttle delay sending extents");
                                    {
                                        usleep((unsigned int)l_ThreadDelay);
                                    }
                                    lockTransferQueue(&l_Key, "%transferWorker - After throttle delay sending extents");
                                }
                                else
                                {
                                    l_WrkQ = l_WrkQE->getWrkQ();
                                }
                            }
                            else
                            {
                                errorText << "Error occurred in transferExtent() when attempting to access the local metadata (taginfo2)";
                                LOG_ERROR_TEXT_AND_RAS(errorText, bb.internal.tw_1);
                            }
                        }
                        else
                        {
                            errorText << "Error occurred in transferExtent() when processing a work queue with no entries";
                            LOG_ERROR_TEXT_AND_RAS(errorText, bb.internal.tw_2);
                        }
                    }
                    else
                    {
                        // The high priority work queue was returned
                        l_WrkQ = l_WrkQE->getWrkQ();
                    }
                }
                else
                {
                    errorText << "Null work queue returned when attempting to find additional work";
                    LOG_ERROR_TEXT_AND_RAS(errorText, bb.internal.tw_3);
                }

                //  NOTE:  When we get here, l_WrkQ addresses a workqueue with one or more extents
                //         left to transfer -or- the high priority work queue.
                //         If l_WrkQ is not set, we either were not handed back a work queue,
                //         or a work queue with no entries (both unlikely), or we had a throttle delay
                //         (very likely).
                //
                //         Simply, fall out, and find more work...
                if (l_WrkQ)
                {
                    bool l_ProcessNextWorkItem = true;
                    if (l_WrkQE->isSuspended())
                    {
                        // Work queue is suspended
                        // NOTE:  This will never be the case for the high-priority
                        //        work queue (async requests)
                        //
                        // Peek at the work item.  If for a canceled transfer definition
                        // (most likely because it was 'stopped'), process the entry as
                        // we would any other entry.  Because it is for a canceled
                        // transfer definition, the extent will simply be removed from the
                        // work queue and any metadata is updated as necessary.
                        if (l_LV_Info && (!(l_LV_Info->getNextExtentInfo().getTransferDef()->canceled())))
                        {
                            // Not for a canceled transfer definition.  Do not process
                            // the next work item.
                            l_ProcessNextWorkItem = false;
                        }
                    }

                    if (l_ProcessNextWorkItem)
                    {
                        // Transfer the next extent...
                        // NOTE: If this is for a non-HPWrkQE, after we have removed the
                        //       the work item we MUST make it through transferExtent() to
                        //       process the extent and remove it from the vector of extents
                        //       to transfer.  If not, the number of elements on the work queue
                        //       will not match the elements in the vector of extents to transfer.
                        //       This is regardless of whether the current work item could be
                        //       successfully processed by transferExtent() or not.
                        l_ConsecutiveSuspendedWorkQueuesNotProcessed = 0;
                        wrkqmgr.removeWorkItem(l_WrkQE, l_WorkItem);

                        l_Repost = false;

                        // Clear bberror...
                        bberror.clear(l_WorkItem.getConnectionName());

                        if (l_WrkQ != HPWrkQE->getWrkQ())
                        {
                            // Perform the transfer
//                            l_WrkQE->dump("info", " Extent being transferred -> ");
                            transferExtent(l_WorkItem, l_ExtentInfo);
                        }
                        else
                        {
                            // Process the async request
                            processAsyncRequest(l_WorkItem);
                        }
                        wrkqmgr.incrementNumberOfWorkItemsProcessed(l_WrkQE, l_WorkItem);
                    }
                    else
                    {
                        // Advance the last work queue processed so non-suspended work continues
                        // NOTE: We don't increment the number of work items processed for the manager...
                        wrkqmgr.setLastQueueProcessed(l_WrkQE->getLVKey());

                        if (++l_ConsecutiveSuspendedWorkQueuesNotProcessed >= CONSECUTIVE_SUSPENDED_WORK_QUEUES_NOT_PROCESSED_THRESHOLD)
                        {
                            // If we have not processed several suspended work queues consecutively, then treat
                            // this situation as if no work remains.  We will delay below for a while before retrying
                            // to find additional work.
                            l_ConsecutiveSuspendedWorkQueuesNotProcessed = 0;
                            l_WorkRemains = false;
                        }
                    }

                    LOG(bb,debug) << "End: Previous current work item";

                }

                // If needed, repost to the semaphore...
                if (l_Repost)
                {
                    // NOTE: At least one workqueue entry exists on one workqueue, so repost to the semaphore...
                    l_Repost = false;
                    wrkqmgr.post();
                }
            }
            else
            {
                l_WorkRemains = false;
            }

            if (!l_WorkRemains)
            {
                // No work remains...  We need to post to the semaphore at least every time interval
                // so that we can check for requests in the async file...
                // NOTE: If the Throttle_Timer is snoozing, that indicates another thread is
                //       already delaying to post to the semaphore.  We only need a single thread
                //       to perform this function at one time.
                if (!Throttle_Timer.isSnoozing())
                {
                    // A transfer thread is not already snoozing waiting to post...
                    double l_Delay = max((Throttle_TimeInterval-Throttle_Timer.getCurrentElapsedTimeInterval())*1000000,(double)0);
                    if (l_Delay > 0)
                    {
                        Throttle_Timer.setSnooze(true);
                        unlockTransferQueue(&l_Key, "%transferWorker - Before snoozing");
                        {
//                            FL_Write(FLDelay, Snooze, "Snoozing for %ld usecs waiting for additional work", (uint64_t)l_Delay, 0, 0, 0);
                            LOG(bb,off) << ">>>>> DELAY <<<<< transferWorker(): Snoozing for " << (float)l_Delay/1000000.0 << " seconds waiting for additional work";
                            usleep((unsigned int)l_Delay);
                        }
                        lockTransferQueue(&l_Key, "%transferWorker - After snoozing");
                        Throttle_Timer.setSnooze(false);
                    }
                    // Check the throttle timer and repost
                    wrkqmgr.checkThrottleTimer();
                    wrkqmgr.post();
                }
                else
                {
                    // Another thread is already snoozing...
                    LOG(bb,debug) << "Already snoozing...";
                }
            }

            unlockTransferQueue(&l_Key, "%transferWorker - After remove extent from in-flight list");

        }
        catch(ExceptionBailout& e)
        {
            stringstream errorText;
            errorText << "Exception thrown in transferExtent() to bailout";
            LOG_ERROR_TEXT_AND_RAS(errorText, bb.internal.tw_4);

            if (l_Repost)
            {
                // NOTE: At least one workqueue entry exists, so repost to the semaphore...
                l_Repost = false;
                wrkqmgr.post();
            }

            //  NOTE:  unlockTransferQueue() will only unlock the mutex if this thread took an exception
            //         while it held the mutex.
            unlockTransferQueue(&l_Key, "transferWorker - Bailout exception handler");
        }
        catch(exception& e)
        {
            LOG_ERROR_WITH_EXCEPTION_AND_RAS(__FILE__, __FUNCTION__, __LINE__, e, bb.internal.tw_5);

            if (l_Repost)
            {
                // NOTE: At least one workqueue entry exists, so repost to the semaphore...
                l_Repost = false;
                wrkqmgr.post();
            }

            //  NOTE:  unlockTransferQueue() will only unlock the mutex if this thread took an exception
            //         while it held the mutex.
            unlockTransferQueue(&l_Key, "transferWorker - General exception handler");
        }
    }

    return NULL;
}

void startThreads(void)
{
    int rc;
    stringstream errorText;
    unsigned int x;
    unsigned int numthreads = config.get(resolveServerConfigKey("numTransferThreads"), 1);

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    LOG(bb,info) << "Starting " << numthreads << " transfer threads";

    unsigned numbuffers = config.get(resolveServerConfigKey("numTransferBuffers"), numthreads);
    transferBufferSize   = config.get(resolveServerConfigKey("workerBufferSize"), 16*1024*1024);
    for(x=0; x<numbuffers; x++)
    {
        void* buffer = mmap(NULL, transferBufferSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        returnTransferBuffer(buffer);
    }

    for (x=0; x<numthreads; x++)
    {
        rc = pthread_create(&tid, &attr, transferWorker, NULL);
        if (rc)
        {
            errorText << "Error occurred in startThreads()";
            bberror << err("error.numthreads", numthreads);
            LOG_ERROR_TEXT_RC_AND_RAS_AND_BAIL(errorText, rc, bb.sc.pthread_create);
        }
    }

    return;
}

int queueTagInfo(const std::string& pConnectionName, LVKey* pLVKey, BBLV_Info* pLV_Info, BBTagInfoMap* pTagInfoMap,
                 BBTagID& pTagId, BBJob pJob, BBTransferDef* &pTransferDef, int32_t pContribId, uint64_t pNumContrib,
                 uint32_t pContrib[], uint64_t& pHandle, vector<struct stat*>* pStats, const uint32_t pPerformOperation,
                 uint32_t &pMarkFailedFromProxy)
{
    ENTRY(__FILE__,__FUNCTION__);

    int rc = 0;
    stringstream errorText;
    stringstream l_JobStr;
    pJob.getStr(l_JobStr);

    BBTransferDef* l_OrigInputTransferDef = pTransferDef;
    bool l_TransferDefinitionBuiltViaRetrieve = false;  // NOTE:  Only set/used if (!pPerformOperation)
                                                        //        Otherwise, take from local metadata transfer definition.
    bool l_RestartToSameServer = false;

    // NOTE:  We used to initialize the pHandle value.  However, this can come in as valid and if this routine takes an
    //        error, we do not want to wipe out this value before returning.

    // Finish building the metadata related to the LVKey/transfer definition...
    BBTagInfo* l_TagInfo = pTagInfoMap->getTagInfo(pTagId);
    if(!l_TagInfo)
    {
        int l_GeneratedHandle = 0;
        BBTagInfo l_NewTagInfo = BBTagInfo(pTagInfoMap, pNumContrib, pContrib, pJob, pTagId.getTag(), l_GeneratedHandle);
        rc = pTagInfoMap->addTagInfo(pLVKey, pJob, pTagId, l_NewTagInfo, l_GeneratedHandle);
        if (!rc)
        {
            l_TagInfo = pTagInfoMap->getTagInfo(pTagId);
            if (pConnectionName.size())
            {
                LOG(bb,debug) << "taginfo: Adding TagID(" << l_JobStr.str() << "," << pTagId.getTag() << ") with handle 0x" \
                              << hex << uppercase << setfill('0') << setw(16) << l_TagInfo->transferHandle \
                              << setfill(' ') << nouppercase << dec << " (" << l_TagInfo->transferHandle << ") to " << *pLVKey;
            } else {
                LOG(bb,debug) << "taginfo: Adding TagID(" << l_JobStr.str() << "," << pTagId.getTag() << ")";
            }
        } else {
            // NOTE: errstate already filled in...
            // This condition overrides any failure detected on bbProxy...
            pMarkFailedFromProxy = 0;

            errorText << "queueTagInfo: Failure from addTagInfo(), rc = " << rc;
            LOG_ERROR(errorText);
        }
    } else {
        bool l_Error = false;
        if ((l_TagInfo->expectContrib).size() == pNumContrib)
        {
            for (size_t i=0; (!l_Error) && i<pNumContrib; ++i)
            {
                if (pContrib[i] != l_TagInfo->expectContrib[i])
                {
                    l_Error = true;
                }
            }
        } else {
            l_Error = true;
        }

        if (l_Error) {
            // This condition overrides any failure detected on bbProxy...
            pMarkFailedFromProxy = 0;

            rc = -1;
            stringstream l_Temp;
            l_TagInfo->expectContribToSS(l_Temp);
            errorText << "taginfo: Expect contrib array mismatch for contribid " << pContribId << ", TagID(" << l_JobStr.str() << "," << pTagId.getTag()
                      << "). Existing expect contrib array is " << l_Temp.str() << ". A different tag value must be used for this transfer definition.";
            LOG_ERROR_TEXT_RC(errorText, rc);
        }
    }

    if (!rc)
    {
        if (pTransferDef)
        {
            BBTransferDef* l_OrigTransferDef = l_TagInfo->getTransferDef((uint32_t)pContribId);

            // For first pass, perform any cleanup of the metadata (this could be a restart for the transfer definition)
            if (!pPerformOperation)
            {
                // NOTE: This indicator is being set from the input transfer definition
                l_TransferDefinitionBuiltViaRetrieve = (pTransferDef->builtViaRetrieveTransferDefinition() ? true : false);

                if (l_TransferDefinitionBuiltViaRetrieve)
                {
                    // This is a transfer definition that was rebuilt from bbServer metadata...
                    // NOTE:  l_OrigTransferDef may or may not be NULL here.  If we are restarting on the same bbServer, then l_OrigTransferDef will be set
                    //        to the 'old' definition.  If this bbServer has not seen this transfer definition before (it was submitted and serviced
                    //        by a different bbServer), then l_OrigTransferDef will be NULL.
                    // NOTE:  The ensuing method call ensures that the handle is at a stopped state.  This method call may block for a while
                    //        waiting for another bbServer to set the stopped state for the transfer handle.
                    rc = prepareForRestart(pConnectionName, pLVKey, pLV_Info, pTagInfoMap, l_TagInfo, pHandle, pTagId, pJob, pContribId, l_OrigTransferDef, pTransferDef, FIRST_PASS);
                    switch (rc)
                    {
                        case 1:
                        {
                            // This condition overrides any failure detected on bbProxy...
                            pMarkFailedFromProxy = 0;

                            // We will not restart this transfer definition
                            errorText << "Transfer definition associated with jobid " << pJob.getJobId() << ", jobstepid " << pJob.getJobStepId() \
                                      << ", handle " << pHandle << ", contribid " << pContribId \
                                      << " was not restarted because it was not in a stopped state." \
                                      << " If a prior attempt was made to stop the transfer, it may not have been stopped" \
                                      << " because the transfer for all extents had already completed.  See prior messages for this handle.";
                            LOG_ERROR_TEXT_RC(errorText, -2);

                            break;
                        }

                        case 0:
                        {
                            if (!pMarkFailedFromProxy)
                            {
                                // We will restart this transfer definition
                                rc = prepareForRestart(pConnectionName, pLVKey, pLV_Info, pTagInfoMap, l_TagInfo, pHandle, pTagId, pJob, pContribId, l_OrigTransferDef, pTransferDef, SECOND_PASS);
                            }

                            break;
                        }

                        default:
                        {
                            // Error case...  errstate already set

                            // This condition overrides any failure detected on bbProxy...
                            pMarkFailedFromProxy = 0;

                            break;
                        }
                    }
                }
            }

            if (!rc)
            {
                stringstream l_JobStr;
                pTagId.getJob().getStr(l_JobStr);

                if (!pPerformOperation)
                {
                    if (!l_OrigTransferDef)
                    {
                        if (pConnectionName.size())
                        {
                            LOG(bb,debug) << "taginfo: Adding Contrib(" << pContribId << ") to TagID(" << l_JobStr.str() << "," << pTagId.getTag() \
                                          << ") having a transfer definition with " << pTransferDef->getNumberOfExtents() << " extents to " << *pLVKey;
                        }
                        else
                        {
                            LOG(bb,debug) << "taginfo: Adding Contrib(" << pContribId << ") to TagID(" << l_JobStr.str() << "," << pTagId.getTag() \
                                          << ") having a transfer definition with " << pTransferDef->getNumberOfExtents() << " extents";
                        }

                        // NOTE: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                        // NOTE: * pTransferDef currently addresses the input transfer definition.  This would be the rebuilt
                        // NOTE:   transfer definition from the retrieve archive for a restart transfer definition scenario.
                        // NOTE: * addTransferDef() resets pTransferDef to the local metadata copy of the transfer definition
                        // NOTE:   in BBTagInfo/BBTagParts.
                        // NOTE: * After the invocation of addTransferDef(), l_OrigInputTransferDef must be used to address the input
                        // NOTE:   transfer definition
                        // NOTE: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                        rc = l_TagInfo->addTransferDef(pConnectionName, pLVKey, pJob, pLV_Info, pTagId, (uint32_t)pContribId, l_TagInfo->transferHandle, pTransferDef);
                        if (rc)
                        {
                            // This condition overrides any failure detected on bbProxy...
                            pMarkFailedFromProxy = 0;

                            errorText << "queueTagInfo: Failure from addTransferDef() for TagID(" << l_JobStr.str() << "," << pTagId.getTag() << ") for contribid " << pContribId << ", rc=" << rc;
                            LOG_ERROR(errorText);
                        }
                    }
                    else
                    {
                        if (!l_TransferDefinitionBuiltViaRetrieve)
                        {
                            if (!l_OrigTransferDef->extentsAreEnqueued())
                            {
                                // Extents have not been enqueued yet...
                                // NOTE:  Allow this to continue...  This is probably the case where a start transfer got far enough along
                                //        on bbServer to create all of the metadata (first volley message), but the second volley either failed
                                //        or bbProxy failed before/during the send of the second volley message.
                                // NOTE:  Start transfer processing DOES NOT backout any metadata changes made for a partially completed
                                //        operation.
                                LOG(bb,info) << "Transfer definition for contribid " << pContribId << " already exists for " << *pLVKey \
                                             << ", TagID(" << l_JobStr.str() << "," << pTagId.getTag() << "), handle " << l_TagInfo->transferHandle \
                                             << ", but extents have never been enqueued for the transfer definition. Transfer definition will be reused.";

                                // Cleanup the I/O map
                                l_OrigTransferDef->cleanUpIOMap();

                                // Now, swap in the extent vector from the new transfer definition
                                l_OrigTransferDef->replaceExtentVector(pTransferDef);

                                // Set pTransferDef to the version of the transfer definition in BBTagInfo/BBTagParts...
                                // NOTE: This is just like the call to addTransferDef() in the then leg.
                                pTransferDef = l_OrigTransferDef;
                            }
                            else
                            {
                                // Extents have already been enqueued for this transfer definition...
                                // This condition overrides any failure detected on bbProxy...
                                pMarkFailedFromProxy = 0;

                                rc = -1;
                                errorText << "Transfer definition for contribid " << pContribId << " already exists for " << *pLVKey \
                                          << ", TagID(" << l_JobStr.str() << "," << pTagId.getTag() << "), handle " << l_TagInfo->transferHandle \
                                          << ". Extents have already been enqueued for the transfer definition. Most likely, an incorrect contribid" \
                                          << " was specified or a different tag should be used for the transfer.";
                                LOG_ERROR_TEXT_RC(errorText, rc);
                            }
                        }
                        else
                        {
                            // Set addressability to the transfer definition in BBTagInfo/BBTagParts
                            l_RestartToSameServer = true;

                            // Cleanup the I/O map
                            l_OrigTransferDef->cleanUpIOMap();

                            // Now, swap in the extent vector from the new transfer definition
                            l_OrigTransferDef->replaceExtentVector(pTransferDef);

                            // Set pTransferDef to the version of the transfer definition in BBTagInfo/BBTagParts...
                            // NOTE: This is just like the call to addTransferDef() in the then leg.
                            pTransferDef = l_OrigTransferDef;
                        }
                    }

                    if (l_TransferDefinitionBuiltViaRetrieve)
                    {
                        // Set the 'built via retrieve transfer definition' indicator
                        // in the transfer definition in BBTagInfo/BBTagParts
                        pTransferDef->setBuiltViaRetrieveTransferDefinition();
                    }

                    if (!rc)
                    {
                        if (!pMarkFailedFromProxy)
                        {
                            // NOTE:  The extents we are iterating through here is a single extent per file.
                            //        The second pass will have the actual extents to be transferred.
                            //        But, we can open and get stats for the PFS files...
                            bool l_SkipFile = false;
                            BBTransferDef* l_TempTransferDef = 0;
                            if (l_RestartToSameServer)
                            {
                                // If we need to check to see if this contribid was stopped, we
                                // can use the transfer definition in the local metadata because
                                // the same bbServer is being used for the restart.
                                l_TempTransferDef = pTransferDef;
                            }

                            uint32_t l_NextSourceIndexToProcess = 0;
                            map<uint32_t, int> l_AlreadyOpened;
                            bool l_AtLeastOneFileRestarted = false;
                            for(auto& e : pTransferDef->extents)
                            {
                                l_SkipFile = false;
                                if (l_TransferDefinitionBuiltViaRetrieve)
                                {
                                    int rc2 = fileToBeRestarted(pLVKey, l_TempTransferDef, e, pJob.getJobId(), pJob.getJobStepId(), pHandle, pContribId);
                                    if (!rc2)
                                    {
                                        // File is not to be restarted.  Indicate to skip this file...
                                        l_SkipFile = true;
                                    }
                                    else
                                    {
                                        if (rc2 == 1)
                                        {
                                            // File is stopped.  Need to restart this file...
                                        }
                                        else
                                        {
                                            // Error when attempting to determine if this file was stopped....
                                            rc = -1;
                                            errorText << "On first pass, could not determine if the file with source index " << e.sourceindex << " was stopped for handle " << pHandle << ", contribid " << pContribId << ", TagID(" << l_JobStr.str() << "," << pTagId.getTag() << ".";
                                            LOG_ERROR_TEXT_RC(errorText, rc);
                                        }
                                    }
                                }

                                if (l_TransferDefinitionBuiltViaRetrieve)
                                {
                                    // bbproxy will skip all source/target file pairs from
                                    // the last pair processed up through the current source/target file pair...
                                    // NOTE: We have a dependency that bbProxy will always insert extents into
                                    //       the extent vector in ascending sourcefile index order...
                                    while ((l_NextSourceIndexToProcess) < e.sourceindex)
                                    {
                                        LOG(bb,info) << "bbproxy is not restarting the transfer for the source file associated with jobid " \
                                                     << pJob.getJobId() << ", jobstepid " << pJob.getJobStepId() << ", handle " << pHandle << ", contrib " \
                                                     << (uint32_t)pContribId << ", source index " << l_NextSourceIndexToProcess << ", transfer type UNKNOWN";
                                        l_NextSourceIndexToProcess += 2;
                                    }
                                }

                                if (!l_SkipFile)
                                {
                                    l_AtLeastOneFileRestarted = true;
                                    uint16_t l_BundleID = e.getBundleID();
                                    BBIO* l_IO = pTransferDef->iomap[l_BundleID];
                                    if (l_IO == NULL)
                                    {
                                        switch(e.getBundleType())
                                        {
                                            case BBTransferTypeBSCFS:
                                                l_IO = new BBIO_BSCFS(pContribId, pTransferDef);
                                                break;
                                            case BBTransferTypeRegular:
                                                l_IO = new BBIO_Regular(pContribId, pTransferDef);
                                                break;
                                            default:
                                                break;
                                        }
                                        pTransferDef->iomap[l_BundleID] = l_IO;
                                    }

                                    if (l_IO)
                                    {
                                        if ((e.flags & BBI_TargetSSD) || (e.flags & BBI_TargetPFS))
                                        {
                                            uint32_t pfs_idx = (e.flags & BBI_TargetSSD) ? e.sourceindex : e.targetindex;
                                            if (!l_AlreadyOpened[pfs_idx])
                                            {
                                                LOG(bb,debug) << "BBIO:  Opening index " << pfs_idx << "  file=" << pTransferDef->files[pfs_idx];

                                                mode_t l_Mode = 0;
                                                if (e.flags & BBI_TargetPFS)
                                                {
                                                    // NOTE: bbProxy filled in the stats for the source files that reside on the SSD.
                                                    //       We obtain the mode from those source file stats to pass on the open in case
                                                    //       the PFS file needs to be created.
                                                    // NOTE: The index is pfs_idx-1 because the sourceindex immediately precedes the target index.
                                                    l_Mode = ((*pStats)[pfs_idx-1])->st_mode;
                                                }

                                                rc = l_IO->open(pfs_idx, e.flags, pTransferDef->files[pfs_idx], l_Mode);
                                                if (rc)
                                                {
                                                    errorText << "Unable to open file associated with jobid=" << pJob.getJobId() << ", handle=" << pHandle << ", contrib=" << (uint32_t)pContribId << ", index=" << pfs_idx;
                                                    bberror << err("error.caller", __PRETTY_FUNCTION__);
                                                    LOG_ERROR(errorText);
                                                    break;
                                                }
                                                l_AlreadyOpened[pfs_idx] = 1;

                                                struct stat l_Stat;
                                                if (((*pStats)[pfs_idx]) == 0)
                                                {
                                                    if (e.flags & BBI_TargetSSD)
                                                    {
                                                        // Stagein processing...  Need stats from the PFS file...
                                                        rc = l_IO->fstat(pfs_idx, &l_Stat);
                                                        if (!rc)
                                                        {
                                                            (*pStats)[pfs_idx] = new(struct stat);
                                                            memcpy((void*)((*pStats)[pfs_idx]), &l_Stat, sizeof(l_Stat));
                                                        }
                                                        else
                                                        {
                                                            errorText << "Unable to get stats for file associated with jobid=" << pJob.getJobId() << ", handle=" << pHandle << ", contrib=" << (uint32_t)pContribId << ", index=" << pfs_idx << ", transfer type PFS_to_SSD";
                                                            bberror << err("error.caller", __PRETTY_FUNCTION__);
                                                            LOG_ERROR(errorText);
                                                            break;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        else if (e.flags & BBI_TargetPFSPFS)
                                        {
                                            uint32_t pfs_idx = e.sourceindex;
                                            mode_t l_Mode = 0;
                                            if (!l_AlreadyOpened[pfs_idx])
                                            {
                                                LOG(bb,debug) << "BBIO:  Opening index " << pfs_idx << "  file=" << pTransferDef->files[pfs_idx];

                                                rc = l_IO->open(pfs_idx, e.flags | BBI_TargetSSD, pTransferDef->files[pfs_idx],l_Mode); //open readonly
                                                if (rc)
                                                {
                                                    errorText << "Unable to open file associated with jobid=" << pJob.getJobId() << ", handle=" << pHandle << ", contrib=" << (uint32_t)pContribId << ", index=" << pfs_idx;
                                                    bberror << err("error.caller", __PRETTY_FUNCTION__);
                                                    LOG_ERROR(errorText);
                                                    break;
                                                }
                                                l_AlreadyOpened[pfs_idx] = 1;

                                                (*pStats)[pfs_idx] = new(struct stat);
                                                rc = l_IO->fstat(pfs_idx, (*pStats)[pfs_idx]);
                                                if (rc)
                                                {
                                                    delete (*pStats)[pfs_idx];
                                                    (*pStats)[pfs_idx]=NULL;

                                                    errorText << "Unable to stat file associated with jobid=" << pJob.getJobId() << ", handle=" << pHandle << ", contrib=" << (uint32_t)pContribId << ", index=" << pfs_idx;
                                                    bberror << err("error.caller", __PRETTY_FUNCTION__);
                                                    LOG_ERROR(errorText);
                                                    break;
                                                }

                                            }
                                            if  ( (*pStats)[e.sourceindex] ) l_Mode = ((*pStats)[e.sourceindex])->st_mode;
                                            pfs_idx = e.targetindex;


                                            if (!l_AlreadyOpened[pfs_idx])
                                            {
                                                LOG(bb,debug) << "BBIO:  Opening index " << pfs_idx << "  file=" << pTransferDef->files[pfs_idx];

                                                rc = l_IO->open(pfs_idx, e.flags | BBI_TargetPFS, pTransferDef->files[pfs_idx],l_Mode); //open trunc/creat/wronly
                                                if (rc)
                                                {
                                                    errorText << "Unable to open file associated with jobid=" << pJob.getJobId() << ", handle=" << pHandle << ", contrib=" << (uint32_t)pContribId << ", index=" << pfs_idx;
                                                    bberror << err("error.caller", __PRETTY_FUNCTION__);
                                                    LOG_ERROR(errorText);
                                                    break;
                                                }
                                                l_AlreadyOpened[pfs_idx] = 1;

                                                (*pStats)[pfs_idx] = new(struct stat);
                                                rc = l_IO->fstat(pfs_idx, (*pStats)[pfs_idx]);
                                                if (rc)
                                                {
                                                    delete (*pStats)[pfs_idx];
                                                    (*pStats)[pfs_idx]=NULL;

                                                    errorText << "Unable to stat file associated with jobid=" << pJob.getJobId() << ", handle=" << pHandle << ", contrib=" << (uint32_t)pContribId << ", index=" << pfs_idx;
                                                    bberror << err("error.caller", __PRETTY_FUNCTION__);
                                                    LOG_ERROR(errorText);
                                                    break;
                                                }

                                            }


                                        }

                                        if (l_TransferDefinitionBuiltViaRetrieve)
                                        {
                                            char l_TransferType[64] = {'\0'};
                                            getStrFromTransferType(e.flags, l_TransferType, sizeof(l_TransferType));
                                            LOG(bb,info) << "Indicating to bbproxy to restart the transfer for the source file associated with jobid " \
                                                         << pJob.getJobId() << ", jobstepid " << pJob.getJobStepId() << ", handle " << pHandle << ", contribid " \
                                                         << (uint32_t)pContribId << ", source index " << e.sourceindex << ", transfer type " << l_TransferType;
                                        }
                                    }
                                    else
                                    {
                                        rc = -1;
                                        errorText << "Unable to obtain/create I/O object for file associated with jobid=" << pJob.getJobId() \
                                                  << ", handle=" << pHandle << ", contrib=" << (uint32_t)pContribId << ", bundle type=" << e.getBundleType();
                                        LOG_ERROR_TEXT_RC(errorText, rc);
                                        break;
                                    }
                                }
                                else
                                {
                                    // Indicate to bbproxy to skip this source/target file pair
                                    // NOTE: We have a dependency that bbProxy will always insert extents into
                                    //       the extent vector in ascending sourcefile index order...
                                    if (!(*pStats)[l_NextSourceIndexToProcess])
                                    {
                                        (*pStats)[l_NextSourceIndexToProcess] = new(struct stat);
                                    }
                                    ((*pStats)[l_NextSourceIndexToProcess])->st_dev = DO_NOT_TRANSFER_FILE;
                                    ((*pStats)[l_NextSourceIndexToProcess])->st_ino = DO_NOT_TRANSFER_FILE;
                                    char l_TransferType[64] = {'\0'};
                                    getStrFromTransferType(e.flags, l_TransferType, sizeof(l_TransferType));
                                    LOG(bb,info) << "Indicating to bbproxy to not restart the transfer for the source file associated with jobid " \
                                                 << pJob.getJobId() << ", jobstepid " << pJob.getJobStepId() << ", handle " << pHandle << ", contribid " \
                                                 << (uint32_t)pContribId << ", source index " << l_NextSourceIndexToProcess << ", transfer type " << l_TransferType;
                                }
                                l_NextSourceIndexToProcess = e.sourceindex + 2;
                            }

                            // All extents have been processed.  If this is a transfer definition built via retrieve,
                            // perform the last updates to metadata to prepare for the restart.
                            if (l_TransferDefinitionBuiltViaRetrieve)
                            {
                                // bbproxy will skip all source/target file pairs from
                                // the last pair processed up through the end of the file list...
                                // NOTE: We have a dependency that bbProxy will always insert extents into
                                //       the extent vector in ascending sourcefile index order...
                                while ((size_t)l_NextSourceIndexToProcess < (pTransferDef->files).size())
                                {
                                    LOG(bb,info) << "bbproxy will not restart the transfer for the source file associated with jobid " \
                                                 << pJob.getJobId() << ", jobstepid " << pJob.getJobStepId() << ", handle " << pHandle << ", contrib " \
                                                 << (uint32_t)pContribId << ", source index " << l_NextSourceIndexToProcess << ", transfer type UNKNOWN";
                                    l_NextSourceIndexToProcess += 2;
                                }

                                if (l_AtLeastOneFileRestarted)
                                {
                                    // NOTE: For the third invocation of prepareForRestart(), pass pTransferDef as the 'old' transfer definition.
                                    rc = prepareForRestart(pConnectionName, pLVKey, pLV_Info, pTagInfoMap, l_TagInfo, pHandle, pTagId, pJob, pContribId, pTransferDef, l_OrigInputTransferDef, THIRD_PASS);
                                }
                                else
                                {
                                    // Indicate to not restart this transfer definition
                                    rc = 1;
                                    LOG(bb,info) << "A restart transfer request was made for the transfer definition associated with " << *pLVKey \
                                                 << ", jobid " << pJob.getJobId() << ", jobstepid " << pJob.getJobStepId() \
                                                 << ", handle " << pHandle << ", contribid " << (uint32_t)pContribId \
                                                 << ", however no extents are left to be transferred for any file." \
                                                 << " A restart for this transfer definition is not necessary.";
                                    LOG_ERROR_TEXT_RC(errorText, rc);
                                }
                            }
                        }
                        else
                        {
                            // Failed on bbProxy side...  Mark the handle/contribid as failed
                            markTransferFailed(pLVKey, l_OrigTransferDef, pLV_Info, pHandle, pContribId);
                        }
                    }
                }
                else
                {
                    if (!pMarkFailedFromProxy)
                    {
                        if (l_OrigTransferDef)
                        {
                            rc = l_TagInfo->replaceExtentVector((uint32_t)pContribId, pTransferDef);
                        }
                        else
                        {
                            // Inconsistency with metadata....
                            rc = -1;
                            errorText << "On second pass, transfer definition could not be found for contribid " << pContribId << ", TagID(" << l_JobStr.str() << "," << pTagId.getTag() << ".";
                            LOG_ERROR_TEXT_RC(errorText, rc);
                        }
                    }
                    else
                    {
                        // Failed on bbProxy side...  Mark the handle/contribid as failed
                        markTransferFailed(pLVKey, l_OrigTransferDef, pLV_Info, pHandle, pContribId);
                    }
                }
            }
            else
            {
                // Error text already filled in
            }
        }

        // Return the transfer handle
        if ((!rc) && (!pMarkFailedFromProxy) && (!pHandle))
        {
            pHandle = l_TagInfo->getTransferHandle();
        }

    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}

// NOTE: pHandle comes in with a handle value in all cases but for the gethandle() path.  In that path, the handle comes in as zero.
int queueTransfer(const std::string& pConnectionName, LVKey* pLVKey, BBJob pJob, const uint64_t pTag, BBTransferDef* &pTransferDef,
                  const int32_t pContribId, uint64_t pNumContrib, uint32_t pContrib[], uint64_t& pHandle, const uint32_t pPerformOperation,
                  uint32_t &pMarkFailedFromProxy, vector<struct stat*>* pStats)
{
    ENTRY(__FILE__,__FUNCTION__);

    FL_Write(FLXfer, DoQueueTransfer, "Starting queueTransfer",0,0,0,0);

    // Transfer mode by_extent...
    int rc = 0;
    stringstream errorText;
    //std::string pConnectionName=getConnectionName(pConnection); // $$$proto

    size_t l_CurrentNumberOfExtents = 0;
    BBTagID l_TagId(pJob, pTag);
    BBLV_Info* l_LV_Info = 0;;
    BBTransferDef* l_TransferDef = 0;

    stringstream l_JobStr;
    pJob.getStr(l_JobStr);

    // First, find the LVKey value...
    l_LV_Info = metadata.getLV_Info(pLVKey);
    if (l_LV_Info)
    {
        if (pTransferDef)
        {
            if (l_LV_Info->stageOutStarted())
            {
                // Stageout has already started for this LVKey...
                if (pTransferDef->builtViaRetrieveTransferDefinition() && (!(l_LV_Info->stageOutEnded())))
                {
                    if (!pTransferDef->resizeLogicalVolumeDuringStageOut())
                    {
                        // This is for restart...  So allow the start transfer...
                        LOG(bb,info) << "Stageout start has already been received for " << *pLVKey << ", but this transfer definition is being restarted from bbServer metadata.  The start transfer operation will be allowed to proceed.";
                    }
                    else
                    {
                        // This condition overrides any failure detected on bbProxy...
                        pMarkFailedFromProxy = 0;

                        rc = -1;
                        errorText << "Stageout start has already been received for " << *pLVKey << " and the CN logical volume is being resized during stageout.  Additional transfers cannot be scheduled for restart.";
                        LOG_ERROR_TEXT_RC(errorText, rc);
                    }
                }
                else
                {
                    // This condition overrides any failure detected on bbProxy...
                    pMarkFailedFromProxy = 0;

                    rc = -1;
                    errorText << "Stageout start has already been received for " << *pLVKey << ".  Additional transfers cannot be scheduled for start.";
                    LOG_ERROR_TEXT_RC(errorText, rc);
                }
            }
        }
    }
    else
    {
        // LVKey could not be found in taginfo2...
        // This condition overrides any failure detected on bbProxy...
        pMarkFailedFromProxy = 0;

        rc = -1;
        errorText << "queueTransfer():" << *pLVKey << " could not be found in taginfo2";
        LOG_ERROR_TEXT_RC(errorText, rc);
    }

    if (!rc)
    {
        // Queue the taginfo...
        rc = queueTagInfo(pConnectionName, pLVKey, l_LV_Info, l_LV_Info->getTagInfoMap(), l_TagId, pJob, pTransferDef, pContribId, pNumContrib, pContrib, pHandle, pStats, pPerformOperation, pMarkFailedFromProxy);
        if (!rc)
        {
            l_CurrentNumberOfExtents = l_LV_Info->getNumberOfExtents();

            if (!pMarkFailedFromProxy)
            {
                if (pTransferDef)
                {
                    if (pPerformOperation)
                    {
                        // Merge in the flags from the transfer definition to the extent info...
                        l_LV_Info->mergeFlags(pTransferDef->flags);

                        BBTagInfo* l_TagInfo = l_LV_Info->getTagInfoMap()->getTagInfo(l_TagId);
                        if (l_TagInfo)
                        {
                            l_TransferDef = l_TagInfo->getTransferDef((uint32_t)pContribId);
                            if (l_TransferDef)
                            {
                                // Add the extents from the transfer definition to allExtents for this LVKey...
                                // NOTE: l_TransferDef is the transfer definition that is associated with BBTagInfo.
                                //       It is that transfer definition that has addressability to the BBIO objects.
                                //       Therefore, it is that transfer definition that is used when constructing the
                                //       ExtentInfo() objects.
                                size_t l_PreviousNumberOfExtents = l_TransferDef->getNumberOfExtents();
                                rc = l_LV_Info->addExtents(pHandle, (uint32_t)pContribId, l_TransferDef, pStats);
                                if (!rc)
                                {
                                    LOG(bb,info) << "For " << *pLVKey << ", the number of extents for the transfer definition associated with Contrib(" \
                                                 << pContribId << "), TagID(" << l_JobStr.str()<< "," << l_TagId.getTag() << ") handle " << pHandle \
                                                 << " is being changed from " << l_PreviousNumberOfExtents << " to " << l_TransferDef->getNumberOfExtents() \
                                                 << " extents";
                                    // If necessary, sort the extents...
                                    rc = l_LV_Info->sortExtents(pLVKey);
                                    if (!rc)
                                    {
                                        WRKQE* l_WrkQE = 0;
                                        rc = wrkqmgr.getWrkQE(pLVKey, l_WrkQE);
                                        if ((!rc) && l_WrkQE)
                                        {
                                            l_WrkQE->dump("debug", "Before pushing work onto this queue ");
                                            bool l_ValidateOption = DO_NOT_VALIDATE_WORK_QUEUE;
                                            for (size_t i=0; i<(l_TransferDef->extents).size(); ++i)
                                            {
                                                LOG(bb,off) << "adding extent with flags " << l_TransferDef->extents[i].flags;
                                                // Queue a WorkID object for every extent to the work queue
                                                WorkID l_WorkId(*pLVKey, l_TagId);
                                                if (i+1 == (l_TransferDef->extents).size())
                                                {
                                                    // Validate work queue on last add...
                                                    l_ValidateOption = VALIDATE_WORK_QUEUE;
                                                }
                                                l_WrkQE->addWorkItem(l_WorkId, l_ValidateOption);
                                            }
                                            l_WrkQE->dump("info", "After pushing work onto this queue ");

                                            // If extents were added to be transferred, make sure the 'all extents transferred flag' is now off for the extentinfo...
                                            if ((l_TransferDef->extents).size())
                                            {
                                                l_LV_Info->extentInfo.setAllExtentsTransferred(pConnectionName, pLVKey, 0);
                                            }
                                        }
                                        else
                                        {
                                            // Work queue not found....
                                            rc = -1;
                                            errorText << "Work queue for " << *pLVKey << ", jobid " << pJob.getJobId() << ", jobstepid " << pJob.getJobStepId() \
                                                      << ", handle " << pHandle << ", could not be found. The job may have ended.  The transfer is not scheduled.";
                                            LOG_ERROR_TEXT_RC(errorText, rc);
                                            wrkqmgr.dump("info", " Start transfer - Work queue not found", DUMP_UNCONDITIONALLY);
                                        }

                                        if (!rc)
                                        {
                                            // If all contribs have reported for this LVKey, then check to make sure
                                            // we still have extents to transfer.  If not, then indicate that all
                                            // extents have been transferred for this LVKey.
                                            if (l_TagInfo->allContribsReported())
                                            {
                                                if (!(l_LV_Info->extentInfo.moreExtentsToTransfer((int64_t)l_TagInfo->getTransferHandle(), (int32_t)(-1), 0)))
                                                {
                                                    l_TagInfo->setAllExtentsTransferred(pLVKey, pJob.getJobId(), pJob.getJobStepId(), pHandle);
                                                }
                                            }

                                            // Reset the extent for the minimum trim anchor point...
                                            l_LV_Info->resetMinTrimAnchorExtent();

                                            if (config.get(resolveServerConfigKey("bringup.dumpTransferMetadataAfterQueue"), 0))
                                            {
                                                metadata.dump(const_cast<char*>("info"));
                                            }
                                        }
                                    }
                                    else
                                    {
                                        // Sort failed
                                        errorText << "queueTransfer(): sortExtents() failed, rc = " << rc;
                                        LOG_ERROR_TEXT_RC(errorText, rc);
                                    }
                                }
                                else
                                {
                                    // Add extents failed
                                    errorText << "addExtents() failed, rc = " << rc;
                                    LOG_ERROR(errorText);
                                }

                                if (!rc)
                                {
                                    // Indicate in the transfer definition/ContribId file that the extents are now enqueued
                                    l_TransferDef->setExtentsEnqueued(pLVKey, pHandle, pContribId);
                                }
                            }
                            else
                            {
                                // Inconsistency with metadata....
                                rc = -1;
                                errorText << "queueTransfer(): Could not resolve to the transfer definition in the local metadata";
                                LOG_ERROR_TEXT_RC(errorText, rc);
                            }
                        }
                        else
                        {
                            // Inconsistency with metadata....
                            rc = -1;
                            errorText << "queueTransfer(): Could not resolve to BBTagInfo object in the local metadata";
                            LOG_ERROR_TEXT_RC(errorText, rc);
                        }
                    }
                }
            }
        }
        else
        {
            // errstate data already set
        }
    }
    else
    {
        // errstate data already set
    }

    if (!rc)
    {
        if (!pMarkFailedFromProxy)
        {
            if (pPerformOperation)
            {
                // NOTE: pHandle was either already set coming into this routine or set by queueTagInfo()...
                LOG(bb,debug) << "Previously " << l_CurrentNumberOfExtents << ", now " << l_LV_Info->getNumberOfExtents() << " extents queued on workqueue for " << *pLVKey;

                // Post each new extent...
                WorkID l_WorkId;
                if (!rc)
                {
                    size_t l_NewPosts = l_LV_Info->getNumberOfExtents() - l_CurrentNumberOfExtents;
                    wrkqmgr.post_multiple(l_NewPosts);
                }
            }
        }
    }

    FL_Write(FLXfer, DoQueueTransferCMP, "Finishing queueTransfer  rc=%ld",rc,0,0,0);

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}

void startTransferThreads()
{
    pthread_once(&startThreadsControl, startThreads);

    // Post to the semaphore so that the threads start looking
    // for async requests
    wrkqmgr.post();

    return;
}

int addLogicalVolume(const std::string& pConnectionName, const string& pHostName, txp::Msg* pMsg, const LVKey* pLVKey, const uint64_t pJobId, const TOLERATE_ALREADY_EXISTS_OPTION pTolerateAlreadyExists)
{
    ENTRY(__FILE__,__FUNCTION__);

    int rc = 0;

    try
    {
        // NOTE:  If a non-zero return code comes back, error information was also filled in...
        BBLV_Info empty(pConnectionName, pHostName, pJobId);
        // NOTE:  The LVKey could already exist in the case where a given job spans the failover
        //        to a backup and then back to the primary bbServer.
        rc = metadata.addLVKey(pHostName, pMsg, pLVKey, pJobId, empty, pTolerateAlreadyExists);
    }
    catch (ExceptionBailout& e) { }
    catch (exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}

int getHandle(const std::string& pConnectionName, LVKey* &pLVKey, BBJob pJob, const uint64_t pTag, uint64_t pNumContrib, uint32_t pContrib[], uint64_t& pHandle, int pLockTransferQueueOption)
{
    ENTRY(__FILE__,__FUNCTION__);

    int rc = 0;
    stringstream errorText;
    bool l_LockAcquired = false;

    stringstream l_JobStr;
    pJob.getStr(l_JobStr);

    if (pLockTransferQueueOption)
    {
        lockTransferQueue(pLVKey, "getHandle");
        l_LockAcquired = true;
    }

    try
    {
        BBTagInfo* l_TagInfo = 0;
        rc = metadata.getLVKey(pConnectionName, pLVKey, l_TagInfo, pJob, pTag, pNumContrib, pContrib);
        switch (rc) {
            case -2:
            {
                // Cannot find an LVKey registered for this jobid (may be tolerated by invoker...)
                BAIL;
                break;
            }
            case -1:
            {
                // Contrib list did not match...
                errorText << "LVKey could be found for " << l_JobStr.str() << ", but the contrib list did not match for this tag value.  Correct the contrib list or a different tag value must be specified.";
                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                break;
            }

            case 0:
            {
                // Job/tag value could not be found...
                BBTransferDef* l_TransferDef = 0;
                uint32_t l_Dummy = 0;
                rc = queueTransfer(pConnectionName, pLVKey, pJob, pTag, l_TransferDef, (int32_t)(-1), pNumContrib, pContrib, pHandle, 0, l_Dummy, (vector<struct stat*>*)0);
                if (rc) {
                    // NOTE:  errstate already filled in...
                    errorText << "For " << l_JobStr.str() << ", handle " << pHandle << " could not be added to " << *pLVKey << " for the compute node.";
                    LOG_ERROR_AND_BAIL(errorText);
                }
                break;
            }

            case 1:
            {
                // Retrieve the transfer handle value
                rc = metadata.getTransferHandle(pHandle, pLVKey, pJob, pTag, pNumContrib, pContrib);
                break;
            }

            default:
                // Should never get here...
                break;
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    if (l_LockAcquired)
    {
#ifndef __clang_analyzer__
        l_LockAcquired = false;
#endif
        unlockTransferQueue(pLVKey, "getHandle");
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}

int getThrottleRate(const std::string& pConnectionName, LVKey* pLVKey, uint64_t& pRate)
{
    ENTRY(__FILE__,__FUNCTION__);

    int rc = 0;
    stringstream errorText;
    int l_Continue = 120;

    lockTransferQueue(pLVKey, "getThrottleRate");
    bool l_LockHeld = true;

    try
    {
        // NOTE: For failover cases, it is possible for a getThrottleRate() request to be issued to this
        //       bbServer before the activate code has registered all of the LVKeys for bbProxy.
        //       Thus, we wait for a total of 2 minutes if neither an LVKey (nor a work queue) is present
        //       for an LVKey.
        while ((!rc) && l_Continue--)
        {
            rc = wrkqmgr.getThrottleRate(pLVKey, pRate);
            if (rc)
            {
                if (rc == -2)
                {
                    rc = 0;
#ifndef __clang_analyzer__
                    l_LockHeld = false;
#endif
                    unlockTransferQueue(pLVKey, "getThrottleRate - Waiting for LVKey to be registered");
                    {
                        usleep((useconds_t)1000000);    // Delay 1 second
                    }
                    lockTransferQueue(pLVKey, "getThrottleRate - Waiting for LVKey to be registered");
                    l_LockHeld = true;
                }
                else
                {
                    rc = -1;
                    errorText << "Failure when trying to get throttle rate for " << pLVKey;
                    LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                }
            }
            else
            {
                l_Continue = 0;
            }
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    if (l_LockHeld)
    {
#ifndef __clang_analyzer__
        l_LockHeld = false;
#endif
        unlockTransferQueue(pLVKey, "getThrottleRate");
    }

    if (rc && (!l_Continue))
    {
        rc = -1;
        LOG(bb,error) << "Failure when trying to get throttle rate for " << pLVKey << ".  The particular LVKey is not present in the metadata for this bbServer.";
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}

int removeJobInfo(const string& pHostName, const uint64_t pJobId)
{
    ENTRY(__FILE__,__FUNCTION__);

    int rc = 0;

    LOG(bb,info) << "RemoveJobInfo received for jobid=" << pJobId;

    // Perform any cleanup of the local bbServer metadata
    metadata.ensureStageOutEnded(pJobId);

    if (sameHostName(pHostName))
    {
        rc = metadata.update_xbbServerRemoveData(pJobId);

        // Communicate this removejobinfo to other bbServers by
        // posting this request to the async request file.
        // NOTE: Regardless of the rc, post the async request...
        // NOTE: No need to catch the return code.  If the append doesn't work,
        //       appendAsyncRequest() will log the failure...
        char l_AsyncCmd[AsyncRequest::MAX_DATA_LENGTH] = {'\0'};
        snprintf(l_AsyncCmd, sizeof(l_AsyncCmd), "removejobinfo %lu 0 0 0 0 None None", pJobId);
        AsyncRequest l_Request = AsyncRequest(l_AsyncCmd);
        wrkqmgr.appendAsyncRequest(l_Request);
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}

int removeLogicalVolume(const std::string& pConnectionName, const LVKey* pLVKey)
{
    ENTRY(__FILE__,__FUNCTION__);

    int rc = 0;
    BBLV_Info* l_LV_Info = 0;

    lockTransferQueue(pLVKey, "removeLogicalVolume_1");

    try
    {
        l_LV_Info = metadata.getAnyTagInfo2ForUuid(pLVKey);
        if (l_LV_Info)
        {
            // At least one LVKey value was found for the logical volume to be removed...
            // NOTE:  The same LV Uuid could have been registered multiple times with different
            //        connections.  All of those registrations will be for the same jobid.
            uint64_t l_JobId = l_LV_Info->getJobId();

            // Remove the information for the logical volume
            string l_HostName;
            activecontroller->gethostname(l_HostName);
            metadata.removeAllLogicalVolumesForUuid(l_HostName, pLVKey, l_JobId);
        }
        else
        {
            // LVKey could not be found in taginfo2...
            LOG(bb,warning) << "removeLogicalVolume: Logical volume to be removed by bbproxy, however " << *pLVKey << " was not found in the bbserver metadata.";
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    unlockTransferQueue(pLVKey, "removeLogicalVolume_1");

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}

int setThrottleRate(const std::string& pConnectionName, LVKey* pLVKey, uint64_t pRate)
{
    ENTRY(__FILE__,__FUNCTION__);

    int rc = 0;
    stringstream errorText;
    int l_Continue = 120;

    lockTransferQueue(pLVKey, "setThrottleRate");
    bool l_LockHeld = true;

    try
    {
        // NOTE: For failover cases, it is possible for a setThrottleRate() request to be issued to this
        //       bbServer before the activate code has registered all of the LVKeys for bbProxy.
        //       Thus, we wait for a total of 2 minutes if neither an LVKey (nor a work queue) is present
        //       for an LVKey.
        while ((!rc) && l_Continue--)
        {
            rc = wrkqmgr.setThrottleRate(pLVKey, pRate);
            if (rc)
            {
                if (rc == -2)
                {
                    rc = 0;
#ifndef __clang_analyzer__
                    l_LockHeld = false;
#endif
                    unlockTransferQueue(pLVKey, "setThrottleRate - Waiting for LVKey to be registered");
                    {
                        usleep((useconds_t)1000000);    // Delay 1 second
                    }
                    lockTransferQueue(pLVKey, "setThrottleRate - Waiting for LVKey to be registered");
                    l_LockHeld = true;
                }
                else
                {
                    rc = -1;
                    errorText << "Failure when trying to set throttle rate for " << pLVKey;
                    LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                }
            }
            else
            {
                l_Continue = 0;
            }
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    if (l_LockHeld)
    {
#ifndef __clang_analyzer__
        l_LockHeld = false;
#endif
        unlockTransferQueue(pLVKey, "setThrottleRate");
    }

    if (rc && (!l_Continue))
    {
        rc = -1;
        LOG(bb,error) << "Failure when trying to set throttle rate for " << pLVKey << ".  The particular LVKey is not present in the metadata for this bbServer.";
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}

int stageoutEnd(const std::string& pConnectionName, const LVKey* pLVKey, const FORCE_OPTION pForced)
{
    ENTRY(__FILE__,__FUNCTION__);

    // NOTE:  This command is used to simulate when the LV has been removed from the BB volume group on the CN.
    //        With respect to the metadata, everything is performed except that the LVKey is not removed.  This
    //        is so testing can reuse the logical volume.
    int rc = 0;
    stringstream errorText;
    queue<WorkID>* l_WrkQ = 0;
    WRKQE* l_WrkQE = 0;
    BBLV_Info* l_LV_Info;

    l_LV_Info = metadata.getLV_Info(pLVKey);
    if (l_LV_Info)
    {
        // LVKey value found in taginfo2...

        // Check to see if stgout_start has been called...  If not, send warning and continue...
        if (!l_LV_Info->stageOutStarted()) {
            // Stageout start was never received...
            LOG(bb,debug) << "Stageout end received for " << *pLVKey << " without preceding stageout start.  Continuing...";
        }

        if (!(l_LV_Info->stageOutEnded())) {

            // ** NOT USED **
            // In an attempt to make this stageoutEnd() processing look 'atomic' to other threads on THIS bbServer,
            // we pin the transfer queue lock.  This means that the lock will be held throughout the following
            // processing without the lock being dropped.  This means that we WILL NOT open up any processing
            // windows for other transfer threads to perform work until this processing is complete.
            //
            // \todo - Not sure about this strategy long-term, and it does not solve all of the potential issues
            //         with concurrent stageout end and remove logical volume(?) issues across bbServers
            //         (i.e., multiple requests come concurrently from different bbProxies to different bbServers,
            //          and all processed concurrently).
            //         But, this is a potential solution for a given bbServer...    @DLH
            // ** NOT USED **

//            pinTransferQueueLock(pLVKey, "stageoutEnd");

            try
            {
                // NOTE:  First, mark TagInfo2 as StageOutEnded before processing any extents associated with the jobid.
                //        Doing so will ensure that such extents are not actually transferred.
                l_LV_Info->setStageOutEnded(pLVKey, l_LV_Info->getJobId());

                // Next, wait for all in-flight I/O to complete
                // NOTE: If for some reason I/O is 'stuck' and does not return, the following is an infinite loop...
                //       \todo - What to do???  @DLH
                uint32_t i = 0;
                size_t l_CurrentNumberOfInFlightExtents = l_LV_Info->getNumberOfInFlightExtents();
                while (l_CurrentNumberOfInFlightExtents)
                {
                    LOG(bb,info) << "stageoutEnd(): " << l_CurrentNumberOfInFlightExtents << " extents are still inflight for " << *pLVKey;
                    // Source file for extent being inspected has NOT been closed.
                    // Delay a bit for it to clear the in-flight queue and be closed...
                    // NOTE: Currently set to log after 1 second of not being able to clear, and every 10 seconds thereafter...
                    if ((i++ % 40) == 4)
                    {
                        FL_Write(FLDelay, InFlight, "%ld extents are still inflight for jobid %ld. Waiting for the in-flight queue to clear during stageout end processing. Delay of 250 milliseconds.",
                                 (uint64_t)l_CurrentNumberOfInFlightExtents, (uint64_t)l_LV_Info->getJobId(), 0, 0);
                        LOG(bb,info) << ">>>>> DELAY <<<<< stageoutEnd(): Waiting for the in-flight queue to clear.  Delay of 250 milliseconds.";
                        l_LV_Info->getExtentInfo()->dumpInFlight("info");
                        l_LV_Info->getExtentInfo()->dumpExtents("info", "stageoutEnd()");
                    }
                    unlockTransferQueue(pLVKey, "stageoutEnd - Waiting for the in-flight queue to clear");
                    {
                        usleep((useconds_t)250000);
                    }
                    lockTransferQueue(pLVKey, "stageoutEnd - Waiting for the in-flight queue to clear");
                    l_CurrentNumberOfInFlightExtents = l_LV_Info->getNumberOfInFlightExtents();
                }

                // Now, process the remaining extents for this jobid
                size_t l_CurrentNumberOfExtents = l_LV_Info->getNumberOfExtents();
                if (l_CurrentNumberOfExtents) {
                    // Extents still left to be transferred...
                    LOG(bb,info) << "stageoutEnd(): " << l_CurrentNumberOfExtents << " extents still remain on workqueue for " << *pLVKey;

                    int rc2 = wrkqmgr.getWrkQE(pLVKey, l_WrkQE);
                    if ((!rc2) && l_WrkQE)
                    {
                        l_WrkQ = l_WrkQE->getWrkQ();
                        WorkID l_WorkId;
                        queue<WorkID> l_Temp;
                        queue<WorkID> l_Temp2;

                        // The workqueue is currently locked and must remain locked during the following
                        // unload and reload of the workqueue.
                        //
                        // Unload the current workqueue into l_Temp
                        LOG(bb,info) << "stageoutEnd(): " << l_WrkQE->getWrkQ_Size() << " extents remaining on workqueue for " << *pLVKey << " before the reload processing";
                        while (l_WrkQE->getWrkQ_Size()) {
                            LOOP_COUNT(__FILE__,__FUNCTION__,"stageoutEnd_unload_workqueue");
                            l_WorkId = l_WrkQ->front();
                            l_WrkQ->pop();
                            l_Temp.push(l_WorkId);
                        }

                        // Reload the current workqueue
                        // For every work item that is not associated with the jobid that is having
                        // stageout ended, simply push the work item back on the workqueue.
                        //
                        // Note, that the extents will not be reordered as part of this unload/reload
                        // of the workqueue.  Also, because we push the extents that we will eventually
                        // invoke transferExtent() with onto l_Temp2, those extents will also be processed
                        // in the original order.  This is important because the first/last extent flags
                        // need to be processed in the correct order.
                        LVKey l_Key;
                        BBLV_Info* l_WorkItemLV_Info;
                        uint64_t l_JobId = l_LV_Info->getJobId();
                        bool l_ValidateOption = DO_NOT_VALIDATE_WORK_QUEUE;
                        while (l_Temp.size()) {
                            LOOP_COUNT(__FILE__,__FUNCTION__,"stageoutEnd_unload_workqueue");
                            l_WorkId = l_Temp.front();
                            l_Temp.pop();
                            l_Key = l_WorkId.getLVKey();
                            l_WorkItemLV_Info = metadata.getLV_Info(&l_Key);
                            if (l_WorkItemLV_Info)
                            {
                                if (l_WorkItemLV_Info->getJobId() == l_JobId) {
                                    l_Temp2.push(l_WorkId);
                                } else {
                                    LOOP_COUNT(__FILE__,__FUNCTION__,"stageoutEnd_reload_workqueue");
                                    if (!l_Temp.size())
                                    {
                                        // Validate work queue on last add...
                                        l_ValidateOption = VALIDATE_WORK_QUEUE;
                                    }
                                    l_WrkQE->addWorkItem(l_WorkId, l_ValidateOption);
                                }
                            }
                            else
                            {
                                // Not sure how we could get here...  Do not set rc...  Plow ahead...
                                LOG(bb,warning) << "stageoutEnd(): Failure when attempting to remove remaining extents to be transferred for " << *pLVKey \
                                                << ". " << l_Key << " could not be found in the metadata when reloading the work queue with unrelated work items.";
                                l_WorkId.dump("info", "Failure when reloading work queue ");
                            }
                        }

                        // NOTE: We do not adjust the counting semaphore because we cannot re-init it...  We will just leave the
                        //       excess posts there and the worker threads will discard those as no-ops.  @DLH
                        LOG(bb,info) << "stageoutEnd(): " << l_WrkQE->getWrkQ_Size() << " extents are now on the workqueue for " << *pLVKey << " after the reload processing";

                        // Unload and reload is complete.
                        //
                        // For every work item that is associated with the job that is having stageout
                        // ended, invoke transferExtent() to process the extent.  Note, that
                        // the extent will not be transferred, but rather, added and removed
                        // from the inflight queue.  Proper metadata updates, file unlocking,
                        // and messages sent to bbproxy will be performed as part removing the work
                        // items from the inflight queue.  The message(s) that are sent to bbproxy will
                        // eventually cause the files on the compute node to be unlocked.  The unlocking
                        // of the files on the compute node is necessary so that the ensuing Remove
                        // Logical Volume processing can unmount the file system from the logical
                        // volume.
                        //
                        // NOTE: Calling transferExtent() may cause the transfer queue lock to be dropped and
                        //       re-acquired...
                        while (l_Temp2.size()) {
                            LOOP_COUNT(__FILE__,__FUNCTION__,"stageoutEnd_process_workitem");
                            l_WorkId = l_Temp2.front();
                            l_Temp2.pop();
                            l_Key = l_WorkId.getLVKey();
                            l_WorkItemLV_Info = metadata.getLV_Info(&l_Key);
                            if (l_WorkItemLV_Info)
                            {
                                ExtentInfo l_ExtentInfo = l_WorkItemLV_Info->getNextExtentInfo();
                                transferExtent(l_WorkId, l_ExtentInfo);
                            }
                            else
                            {
                                // Not sure how we could get here...  Do not set rc...  Plow ahead...
                                LOG(bb,warning) << "stageoutEnd(): Failure when attempting to remove remaining extents to be transferred for " << *pLVKey \
                                                << ". " << l_Key << " could not be found in the metadata when attempting to process the related work items.";
                                l_WorkId.dump("info", "Failure when processing work items to remove ");
                            }
                        }
                    }
                    else
                    {
                        // Do not set rc...  Plow ahead...
                        LOG(bb,warning) << "stageoutEnd(): Failure when attempting to resolve to the work queue entry for " << *pLVKey;
                    }
                }

                LOG(bb,info) << "Stageout: Ended:   " << *pLVKey << " for jobid " << l_LV_Info->getJobId();

                // Remove the work queue
                rc = wrkqmgr.rmvWrkQ(pLVKey);
                if (rc)
                {
                    // Failure when attempting to remove the work queue for the LVKey value...
                    LOG(bb,warning) << "stageoutEnd: Failure occurred when attempting to remove the work queue for " << *pLVKey;
                    rc = 0;
                }

                // Perform cleanup for the LVKey value
                l_LV_Info->cleanUpAll(pLVKey);

                if (!pForced) {
                    // Remove taginfo2 that is currently associated with this LVKey value...
                    LOG(bb,info) << "stageoutEnd(): Removing all transfer definitions and clearing the allExtents vector for " << *pLVKey << " for jobid = " << l_LV_Info->getJobId();
                    metadata.cleanLVKeyOnly(pLVKey);
                }

                l_LV_Info->setStageOutEndedComplete(pLVKey, l_LV_Info->getJobId());
            }
            catch (ExceptionBailout& e) { }
            catch (exception& e)
            {
                LOG(bb,error) << "stageoutEnd(): Exception thrown: " << e.what();
            }

//            unpinTransferQueueLock(pLVKey, "stageoutEnd");

        } else {
            // Stageout end was already received...
            rc = -2;
            if (!(l_LV_Info->stageOutEndedComplete())) {
                errorText << "Stageout end was received for " << *pLVKey << ", but stageout end has already been received and is currently being processed";
                LOG_ERROR_TEXT(errorText);
            } else {
                errorText << "Stageout end was received for " << *pLVKey << ", but stageout end has already been received and has finished being processed";
                LOG_ERROR_TEXT(errorText);
            }
        }
    } else {
        // LVKey could not be found in taginfo2...
        rc = -2;
        errorText << "Stageout end was received, but no transfer handles can be found for " << *pLVKey << ". Cleanup of metadata not required.";
        LOG_ERROR_TEXT(errorText);
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}

int stageoutStart(const std::string& pConnectionName, const LVKey* pLVKey)
{
    ENTRY(__FILE__,__FUNCTION__);

    // NOTE:  This command is used to simulate when the file system has been unmounted on the CN for the LFKey value...
    int rc = 0;
    stringstream errorText;
    BBLV_Info* l_LV_Info;

    lockTransferQueue(pLVKey, "stageoutStart");

    try
    {
        l_LV_Info = metadata.getLV_Info(pLVKey);
        if (l_LV_Info)
        {
            // Key found in taginfo2...
            if (!(l_LV_Info->stageOutStarted()))
            {
                LOG(bb,info) << "Stageout: Started: " << *pLVKey << " for jobid " << l_LV_Info->getJobId();
                l_LV_Info->setStageOutStarted(pLVKey, l_LV_Info->getJobId());

                // Update any/all transfer status
                l_LV_Info->updateAllTransferHandleStatus(pConnectionName, pLVKey, l_LV_Info->jobid, l_LV_Info->extentInfo, 0);

                size_t l_CurrentNumberOfExtents = l_LV_Info->getNumberOfExtents();
                if (!l_CurrentNumberOfExtents)
                {
                    size_t l_CurrentNumberOfInFlightExtents = l_LV_Info->getNumberOfInFlightExtents();
                    if (l_CurrentNumberOfInFlightExtents)
                    {
                        // In-flight extents are still being transferred...
                        LOG(bb,info) << l_CurrentNumberOfInFlightExtents << " extents are still being transferred for " << *pLVKey;
                    }
                } else {
                    // Extents are left to be transferred...
                    LOG(bb,info) << l_CurrentNumberOfExtents << " extents remaining on workqueue for " << *pLVKey;
                    // NOTE:  To send a progress message, we must have sorted the extents -and-
                    //        the logical volume is to be resized during stageout -and-
                    //        BSCFS cannot be involved -and-
                    //        stageout has started
                    if (l_LV_Info->resizeLogicalVolumeDuringStageOut() &&
                        ((!l_CurrentNumberOfExtents) || ResizeSSD_Timer.popped(ResizeSSD_TimeInterval)))
                    {
                        if (sendTransferProgressMsg(pConnectionName, pLVKey, l_LV_Info->getJobId(), (uint32_t)l_CurrentNumberOfExtents, NULL))
                        {
                            LOG(bb,warning) << "stageoutStart: Error occurred when sending transfer progress message back to bbproxy";
                        }
                    }
                }
            } else {
                // Stageout start was already received...
                rc = -1;
                errorText << "Stageout start was received for " << *pLVKey << ", but stageout start has already been received and processed";
                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
            }
        } else {
            // LVKey could not be found in taginfo2...
            rc = -1;
            errorText << "Stageout start was received, but " << *pLVKey << " could not be found in taginfo2";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    unlockTransferQueue(pLVKey, "stageoutStart");

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}

void switchIdsToMountPoint(txp::Msg* pMsg)
{
    const uid_t l_Owner = (uid_t)((txp::Attr_uint32*)pMsg->retrieveAttrs()->at(txp::mntptuid))->getData();
    const gid_t l_Group = (gid_t)((txp::Attr_uint32*)pMsg->retrieveAttrs()->at(txp::mntptgid))->getData();

    int rc = becomeUser(l_Owner, l_Group);
    if (!rc)
    {
        bberror << err("in.misc.mntptuid", l_Owner) << err("in.misc.mntptgid", l_Group);
    }
    else
    {
        stringstream errorText;
        errorText << "becomeUser failed";
    	bberror << err("error.uid", l_Owner) << err("error.gid", l_Group);
        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
    }

    return;
}

