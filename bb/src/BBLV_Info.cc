/*******************************************************************************
 |    BBLV_Info.cc
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

#include "bberror.h"
#include "bbinternal.h"
#include "bbio.h"
#include "BBLV_ExtentInfo.h"
#include "BBLV_Info.h"
#include "bbserver_flightlog.h"
#include "BBTransferDef.h"
#include "bbwrkqmgr.h"
#include "ContribIdFile.h"
#include "fh.h"
#include "HandleFile.h"
#include "logging.h"
#include "xfer.h"

//
// BBLV_Info class
//

void BBLV_Info::accumulateTotalLocalContributorInfo(const uint64_t pHandle, size_t& pTotalContributors, size_t& pTotalLocalReportingContributors)
{
    tagInfoMap.accumulateTotalLocalContributorInfo(pHandle, pTotalContributors, pTotalLocalReportingContributors);

    return;
}

int BBLV_Info::allContribsReported(const uint64_t pHandle, const BBTagID& pTagId)
{
    int rc = 0;

    // Check the cross bbServer metadata and return the flag indicator for BBTI_All_Contribs_Reported.
    HandleFile* l_HandleFile = 0;
    char* l_HandleFileName = 0;
    rc = HandleFile::loadHandleFile(l_HandleFile, l_HandleFileName, pTagId.getJobId(), pTagId.getJobStepId(), pHandle, DO_NOT_LOCK_HANDLEFILE);
    if (!rc)
    {
        rc = l_HandleFile->allContribsReported();
    }
    if (l_HandleFileName)
    {
        delete[] l_HandleFileName;
    }

    if (l_HandleFile)
    {
        delete l_HandleFile;
    }

    return rc;
}

int BBLV_Info::allExtentsTransferred(const BBTagID& pTagId)
{
    int rc = 0;

    BBTagInfo* l_TagInfo = tagInfoMap.getTagInfo(pTagId);
    if (l_TagInfo) {
        rc = l_TagInfo->allExtentsTransferred();
    } else {
        rc = -1;
    }

    return rc;
}

void BBLV_Info::cancelExtents(const LVKey* pLVKey, uint64_t* pHandle, uint32_t* pContribId, uint32_t pNumberOfExpectedInFlight, LOCAL_METADATA_RELEASED& pLockWasReleased, const int pRemoveOption)
{
    // Sort the extents, moving the canceled extents to the front of
    // the work queue so they are immediately removed...

    // NOTE: pLockWasReleased intentionally not initialized

    // NOTE: CurrentWrkQE must be set prior to sortExtents()
    if (!CurrentWrkQE)
    {
        wrkqmgr.getWrkQE(pLVKey, CurrentWrkQE);
    }
    if (CurrentWrkQE)
    {
        size_t l_NumberOfNewExtentsCanceled = 0;
        extentInfo.sortExtents(pLVKey, l_NumberOfNewExtentsCanceled, pHandle, pContribId);

        if (l_NumberOfNewExtentsCanceled)
        {
            int l_LocalMetadataUnlockedInd = 0;
            wrkqmgr.lockWorkQueueMgr(pLVKey, "cancelExtents - before increment of concurrent", &l_LocalMetadataUnlockedInd);

            // Indicate that the next findWork() needs to look for canceled extents
            wrkqmgr.setCheckForCanceledExtents(1);

            // Increment the number of concurrent cancel reqeusts
            wrkqmgr.incrementNumberOfConcurrentCancelRequests();

            wrkqmgr.unlockWorkQueueMgr(pLVKey, "cancelExtents - after increment of concurrent", &l_LocalMetadataUnlockedInd);

            // Wait for the canceled extents to be processed
            uint32_t i = 0;
            int l_DumpOption = DO_NOT_DUMP_QUEUES_ON_VALUE;
            int l_DelayMsgLogged = 0;
            while (extentInfo.moreExtentsToTransfer((int64_t)(*pHandle), (int32_t)(*pContribId), pNumberOfExpectedInFlight, l_DumpOption))
            {
                unlockLocalMetadata(pLVKey, "cancelExtents - Waiting for the canceled extents to be processed");
                {
                    // NOTE: Currently set to send info to console after 12 seconds of not being able to clear, and every 15 seconds thereafter...
                    if ((i++ % 60) == 48)
                    {
                        FL_Write(FLDelay, RemoveTargetFiles, "Attempting to remove the target files after a cancel operation for handle %ld, contribid %ld. Waiting for the canceled extents to be processed. Delay of 1 second before retry.",
                                 (uint64_t)pHandle, (uint64_t)pContribId, 0, 0);
                        LOG(bb,info) << ">>>>> DELAY <<<<< BBLV_Info::cancelExtents: For " << *pLVKey << ", handle " << *pHandle << ", contribid " << *pContribId \
                                     << ", waiting for all canceled extents to finished being processed.  Delay of 1 second before retry.";
                        l_DelayMsgLogged = 1;
                    }
                    pLockWasReleased = LOCAL_METADATA_LOCK_RELEASED;
                    usleep((useconds_t)250000);
                    // NOTE: Currently set to dump after 12 seconds of not being able to clear, and every 15 seconds thereafter...
                    if ((i % 60) == 48)
                    {
                        l_DumpOption = MORE_EXTENTS_TO_TRANSFER;
                    }
                    else
                    {
                        l_DumpOption = DO_NOT_DUMP_QUEUES_ON_VALUE;
                    }
                }
                lockLocalMetadata(pLVKey, "cancelExtents - Waiting for the canceled extents to be processed");
            }

            if (l_DelayMsgLogged)
            {
                LOG(bb,info) << ">>>>> RESUME <<<<< BBLV_Info::cancelExtents: For " << *pLVKey << ", handle " << *pHandle << ", contribid " << *pContribId \
                             << ", all canceled extents are now processed.";
            }

            wrkqmgr.lockWorkQueueMgr(pLVKey, "cancelExtents - before decrement of concurrent", &l_LocalMetadataUnlockedInd);

            // Decrement the number of concurrent cancel requests
            wrkqmgr.decrementNumberOfConcurrentCancelRequests();

            wrkqmgr.unlockWorkQueueMgr(pLVKey, "cancelExtents - after decrement of concurrent", &l_LocalMetadataUnlockedInd);
        }

        // If we are to perform remove operations for target PFS files, do so now...
        if (pRemoveOption == REMOVE_TARGET_PFS_FILES)
        {
            // Remove the target files
            if (*pContribId != UNDEFINED_CONTRIBID)
            {
                LOG(bb,info) << "Start: Removing target files associated with transfer " << *pLVKey << ", handle " << *pHandle << ", contribid " << *pContribId;
            }
            else
            {
                LOG(bb,info) << "Start: Removing target files associated with transfer " << *pLVKey << ", handle " << *pHandle << ", all contributors.";
            }
            removeTargetFiles(pLVKey, *pHandle, *pContribId);
            if (*pContribId != UNDEFINED_CONTRIBID)
            {
                LOG(bb,info) << "Completed: Removing target files associated with transfer " << *pLVKey << ", handle " << *pHandle << ", contribid " << *pContribId;
            }
            else
            {
                LOG(bb,info) << "Completed: Removing target files associated with transfer " << *pLVKey << ", handle " << *pHandle << ", all contributors.";
            }
        }
    }
    else
    {
        // Plow ahead...
        LOG(bb,warning) << "cancelExtents(): Failure when attempting to resolve to the work queue entry for " << *pLVKey;
    }

    return;
}

void BBLV_Info::cleanUpAll(const LVKey* pLVKey)
{
    // Cleanup the LVKey...
    tagInfoMap.cleanUpAll(pLVKey);

    return;
}

void BBLV_Info::dump(char* pSev, const char* pPrefix)
{
    if (!strcmp(pSev,"debug")) {
        LOG(bb,debug) << ">>>>> Start: TagInfo2 <<<<<";
        LOG(bb,debug) << "JobId: 0x" << hex << uppercase << setfill('0') << setw(4) \
                      << jobid << setfill(' ') << nouppercase << dec;
        extentInfo.dump(pSev);
        tagInfoMap.dump(pSev);
        LOG(bb,debug) << ">>>>>   End: TagInfo2 <<<<<";
    } else if (!strcmp(pSev,"info")) {
        LOG(bb,info) << ">>>>> Start: TagInfo2 <<<<<";
        LOG(bb,info) << "JobId: 0x" << hex << uppercase << setfill('0') << setw(4) \
                     << jobid << setfill(' ') << nouppercase << dec;
        extentInfo.dump(pSev);
        tagInfoMap.dump(pSev);
        LOG(bb,info) << ">>>>>   End: TagInfo2 <<<<<";
    }

    return;
}

void BBLV_Info::ensureStageOutEnded(const LVKey* pLVKey, LOCAL_METADATA_RELEASED& pLockWasReleased)
{
    // NOTE: pLockWasReleased intentionally not initialized

    if (!stageOutEnded())
    {
        LOG(bb,debug) << "taginfo: Stageout end processing being initiated for jobid " << jobid << ", for " << *pLVKey;

        // NOTE: if stageoutEnd() fails, it fills in errstate.  However, we are not setting a rc here...
        if (stageoutEnd(string(""), pLVKey, FORCED))
        {
            LOG(bb,error) << "BBLV_Info::ensureStageOutEnded():  Failure from stageoutEnd() for LVKey " << *pLVKey;
        }
    }
    else
    {
        if (!stageOutEndedComplete())
        {
            // NOTE: Once we let go of the lock, we have to return and re-iterate
            //       through the map of local metadata
            FL_Write(FLDelay, StageOutEnd, "Waiting for stageout end processing to complete for jobid %ld.",
                     jobid, 0, 0, 0);
            LOG(bb,info) << ">>>>> DELAY <<<<< ensureStageOutEnded(): Waiting for stageout end processing to complete for " << *pLVKey << ", jobid " << jobid;
            pLockWasReleased = LOCAL_METADATA_LOCK_RELEASED;

            unlockLocalMetadata(pLVKey, "ensureStageOutEnded - Waiting for stageout end to complete");
            {
                usleep((useconds_t)3000000);
            }
            lockLocalMetadata(pLVKey, "ensureStageOutEnded - Waiting for stageout end to complete");
        }
    }

    return;
}

BBSTATUS BBLV_Info::getStatus(const uint64_t pHandle, BBTagInfo* pTagInfo)
{
    BBSTATUS l_Status = BBNONE;

    if (extentInfo.moreExtentsToTransfer((int64_t)pHandle, (int32_t)(-1), 0)) {
        l_Status = BBINPROGRESS;
    } else {
        l_Status = pTagInfo->getStatus(extentInfo.stageOutStarted());
    }

    return l_Status;
}

BBSTATUS BBLV_Info::getStatus(const uint64_t pHandle, const uint32_t pContribId, BBTagInfo* pTagInfo)
{
    BBSTATUS l_Status = BBNONE;

    if (pTagInfo->inExpectContrib(pContribId)) {
        if (extentInfo.moreExtentsToTransfer((int64_t)pHandle, (int32_t)pContribId, 0)) {
            l_Status = BBINPROGRESS;
        } else {
            l_Status = pTagInfo->getStatus(pContribId);
        }
    } else {
        l_Status = BBNOTACONTRIB;
    }

    return l_Status;
}

int BBLV_Info::getTransferHandle(uint64_t& pHandle, const LVKey* pLVKey, const BBJob pJob, const uint64_t pTag, const uint64_t pNumContrib, const uint32_t pContrib[])
{
    int rc = 0;
    stringstream errorText;

    BBTagID l_TagId = BBTagID(pJob, pTag);
    BBTagInfo* l_TagInfo = tagInfoMap.getTagInfo(l_TagId);

    stringstream l_JobStr;
    l_TagId.getJob().getStr(l_JobStr);

    if(!l_TagInfo)
    {
        int l_GeneratedHandle = UNDEFINED_HANDLE;
        BBTagInfo l_NewTagInfo = BBTagInfo(&tagInfoMap, pNumContrib, pContrib, pJob, pTag, l_GeneratedHandle);
        BBTagInfo* l_NewTagInfoPtr = &l_NewTagInfo;
        rc = tagInfoMap.addTagInfo(pLVKey, pJob, l_TagId, l_NewTagInfoPtr, l_GeneratedHandle);
        if (!rc)
        {
            // NOTE:  addTagInfo() returns addressability to the newly inserted BBTagInfo object in BBTagInfoMap
            l_TagInfo = l_NewTagInfoPtr;
            LOG(bb,debug) << "taginfo: Adding TagID(" << l_JobStr.str() << "," << l_TagId.getTag() << ") with handle 0x" \
                          << hex << uppercase << setfill('0') << setw(16) << l_TagInfo->transferHandle \
                          << setfill(' ') << nouppercase << dec << " (" << l_TagInfo->transferHandle << ") to " << *pLVKey;
        }
        else
        {
            // NOTE: errstate already filled in...
            errorText << "getTransferHandle: Failure from addTagInfo(), rc = " << rc;
            LOG_ERROR(errorText);
        }
    }

    if(l_TagInfo)
    {
        pHandle = l_TagInfo->getTransferHandle();
    }

    return rc;
}

int BBLV_Info::prepareForRestart(const string& pConnectionName, const LVKey* pLVKey, BBTagInfo* pTagInfo, const BBJob pJob, const uint64_t pHandle, const int32_t pContribId, BBTransferDef* pOrigTransferDef, BBTransferDef* pRebuiltTransferDef, const int pPass)
{
    int rc = 0;

    LOG(bb,debug) << "BBLV_Info::prepareForRestart(): Pass " << pPass;

    // First, perform handle related processing to prepare for the restart
    rc = pTagInfo->prepareForRestart(pConnectionName, pLVKey, pJob, pHandle, pContribId, pOrigTransferDef, pRebuiltTransferDef, pPass);

    if (pPass == THIRD_PASS)
    {
        // NOTE:  Today, the stageout started flag is NOT turned on for an LVKey/jobid/handle.
        //        Therefore we do not attempt to replicate that flag, and related ones,
        //        in the BBLV_ExtentInfo flags data contained within BBLV_Info.
        //        Upon restart to a new bbServer, we *should* copy those flag values
        //        from the cross bbServer metadata to the newly constructed local
        //        metadata.
        // NOTE:  For restart, we should not be concerned with the stageout end
        //        and stage out end completed flags as they should always be off
        //        for a restart scenario.  Those flags are only used as part of
        //        remove logical volume processing.

        // Last pass, set the appropriate flags in the LVKey related local metadata
        extentInfo.setAllExtentsTransferred(pConnectionName, pLVKey, 0);
    }

    return rc;
}

int BBLV_Info::recalculateFlags(const string& pConnectionName, const LVKey* pLVKey, BBTagInfoMap* pTagInfoMap, BBTagInfo* pTagInfo, const int64_t pHandle, const int32_t pContribId)
{
    int rc = 0;

    // NOTE: This may not be needed if when resetting the local metadata,
    //       the cross-bbServer metadata is updated accordingly...

    return rc;
}

void BBLV_Info::removeFromInFlight(const string& pConnectionName, const LVKey* pLVKey, BBTagInfo* pTagInfo, ExtentInfo& pExtentInfo)
{
    stringstream errorText;

    const uint32_t THIS_EXTENT_IS_IN_THE_INFLIGHT_QUEUE = 1;
    bool l_UpdateTransferStatus = false;
    bool l_LocalMetadataLocked = false;
    bool l_LocalMetadataUnlocked = false;

    // Check to see if this is the last extent to be transferred for the source file
    // NOTE:  isCP_Transfer() indicates this is a transfer performed via cp, either locally on
    //        the compute node or remotely on the I/O node.  A single extent is enqueued
    //        for a local/remote cp for a file.
    if (!(pExtentInfo.getExtent()->isCP_Transfer()))
    {
        // An actual transfer of data was performed...
        // NOTE:  However, the length could be zero for a file with no extents
        //        or the last extent was truncated to zero.  We send extents with
        //        a length of zero down this path so that the file status is properly
        //        updated.
        if (pExtentInfo.getExtent()->flags & BBI_Last_Extent)
        {
            // Last extent

            // NOTE: If we are using multiple transfer threads, we have to make sure that there are
            //       no extents for this file currently in-flight...  If so, delay for a bit...
            uint32_t i = 0;
            int l_DumpOption = DO_NOT_DUMP_QUEUES_ON_VALUE;
            int l_DelayMsgLogged = 0;
            while (extentInfo.moreExtentsToTransferForFile((int64_t)pExtentInfo.getHandle(), (int32_t)pExtentInfo.getContrib(), pExtentInfo.getSourceIndex(), THIS_EXTENT_IS_IN_THE_INFLIGHT_QUEUE, l_DumpOption))
            {
                unlockTransferQueue(pLVKey, "removeFromInFlight - Waiting for in-flight queue to clear");
                l_LocalMetadataUnlocked = unlockLocalMetadataIfNeeded(pLVKey, "removeFromInFlight - Waiting for in-flight queue to clear");

                {
                    // NOTE: Currently set to send info to console after 12 seconds of not being able to clear, and every 15 seconds thereafter...
                    if ((i++ % 60) == 48)
                    {
                        FL_Write(FLDelay, RemoveFromInFlight, "Processing last extent, waiting for in-flight queue to clear of extents for handle %ld, contribid %ld, sourceindex %ld.",
                                 pExtentInfo.getHandle(), pExtentInfo.getContrib(), pExtentInfo.getSourceIndex(), 0);
                        LOG(bb,info) << ">>>>> DELAY <<<<< removeFromInFlight(): Processing last extent, waiting for in-flight queue to clear of extents for handle " << pExtentInfo.getHandle() \
                                     << ", contribid " << pExtentInfo.getContrib() << ", sourceindex " << pExtentInfo.getSourceIndex();
                        l_DelayMsgLogged = 1;
                    }
                    usleep((useconds_t)250000);
                    // NOTE: Currently set to dump after 12 seconds of not being able to clear, and every 15 seconds thereafter...
                    if ((i % 60) == 48)
                    {
                        l_DumpOption = MORE_EXTENTS_TO_TRANSFER;
                    }
                    else
                    {
                        l_DumpOption = DO_NOT_DUMP_QUEUES_ON_VALUE;
                    }
                }

                if (l_LocalMetadataUnlocked)
                {
                    l_LocalMetadataUnlocked = false;
                    lockLocalMetadata(pLVKey, "removeFromInFlight - Waiting for in-flight queue to clear");
                }
                lockTransferQueue(pLVKey, "removeFromInFlight - Waiting for in-flight queue to clear");
            }

            if (l_DelayMsgLogged)
            {
                LOG(bb,info) << ">>>>> RESUME <<<<< removeFromInFlight(): Processing last extent, in-flight queue is now clear of all extents for handle " << pExtentInfo.getHandle() \
                             << ", contribid " << pExtentInfo.getContrib() << ", sourceindex " << pExtentInfo.getSourceIndex();
            }

            unlockTransferQueue(pLVKey, "removeFromInFlight - Last extent for file transfer, before fsync");

            if ((!pExtentInfo.getTransferDef()->stopped()) && (!pExtentInfo.getTransferDef()->canceled()))
            {
                uint16_t l_BundleId = pExtentInfo.getExtent()->getBundleID();
                BBIO* l_IO = pExtentInfo.getTransferDef()->iomap[l_BundleId];
                if (l_IO)
                {
                    // Perform any necessary syncing of the data for the target file
                    l_LocalMetadataUnlocked = unlockLocalMetadataIfNeeded(pLVKey, "removeFromInFlight - Last extent for file transfer, before fsync");

                    try
                    {
                        if (pExtentInfo.getExtent()->flags & BBI_TargetSSD)
                        {
                            if (pExtentInfo.getExtent()->len)
                            {
                                // Target is SSD...
                                int ssd_fd = l_IO->getReadFdByExtent(pExtentInfo.getExtent());
                                if (ssd_fd >= 0)
                                {
                                    LOG(bb,debug) << "Final SSD fsync start: targetindex " << pExtentInfo.getExtent()->targetindex << ", ssd fd " << ssd_fd;
                                    FL_Write(FLTInf2, FSYNC_SSD, "Performing SSD fsync.  fd=%ld", ssd_fd,0,0,0);
                                    ::fsync(ssd_fd);
                                    FL_Write(FLTInf2, FSYNC_SSDCMP, "Performed SSD fsync.  fd=%ld", ssd_fd,0,0,0);
                                    LOG(bb,debug) << "Final SSD fsync: targetindex " << pExtentInfo.getExtent()->targetindex << ", ssd fd " << ssd_fd;
                                }
                                else
                                {
                                    LOG(bb,error) << "getReadFdByExtent: targetindex " << pExtentInfo.getExtent()->targetindex << ", ssd fd " << ssd_fd;
                                }
                            }

                        }
                        else if (pExtentInfo.getExtent()->flags & BBI_TargetPFS)
                        {
                            // Target is PFS...
                            LOG(bb,debug) << "Final PFS fsync start: targetindex=" << pExtentInfo.getExtent()->targetindex << ", transdef=" << pExtentInfo.getTransferDef() << ", handle=" << pExtentInfo.getHandle() << ", contribid=" << pExtentInfo.getContrib();
                            FL_Write(FLTInf2, FSYNC_PFS, "Performing PFS fsync.  Target index=%ld", pExtentInfo.getExtent()->targetindex,0,0,0);
                            l_IO->fsync(pExtentInfo.getExtent()->targetindex);
                            FL_Write(FLTInf2, FSYNC_PFSCMP, "Performed PFS fsync.  Target index=%ld", pExtentInfo.getExtent()->targetindex,0,0,0);
                            LOG(bb,debug) << "Final PFS fsync end: targetindex=" << pExtentInfo.getExtent()->targetindex;
                        }
                    }
                    catch (ExceptionBailout& e) { }
                    catch (exception& e)
                    {
                        LOG(bb,error) << "removeFromInFlight(): Exception thrown when attempting to sync the data: " << e.what();
                        pExtentInfo.getExtent()->dump("info", "Exception thrown when attempting to sync the data");
                    }

                    if (l_LocalMetadataUnlocked)
                    {
                        l_LocalMetadataUnlocked = false;
                        lockLocalMetadata(pLVKey, "removeFromInFlight - Last extent for file transfer, before fsync");
                    }
                }
                else
                {
                    LOG(bb,error) << "removeFromInFlight: Could not retrieve the BBIO object for extent " << pExtentInfo.getExtent();
                }
            }
            // Update the status for the file in xbbServer data
            l_LocalMetadataLocked = lockLocalMetadataIfNeeded(pLVKey, "removeFromInFlight - Last extent for file transfer");
            lockTransferQueue(pLVKey, "removeFromInFlight - Last extent for file transfer");

            ContribIdFile::update_xbbServerFileStatus(pLVKey, pExtentInfo.getTransferDef(), pExtentInfo.getHandle(), pExtentInfo.getContrib(), pExtentInfo.getExtent(), BBTD_All_Extents_Transferred);
            l_UpdateTransferStatus = true;
        }
        else
        {
            if (!localMetadataIsLocked())
            {
                unlockTransferQueue(pLVKey, "removeFromInFlight - Not last extent for file transfer");
                l_LocalMetadataLocked = lockLocalMetadataIfNeeded(pLVKey, "removeFromInFlight - Not last extent for file transfer");
                lockTransferQueue(pLVKey, "removeFromInFlight - Not last extent for file transfer");
            }
        }
    }
    else
    {
        // Update the status for the file in xbbServer data
        if (!localMetadataIsLocked())
        {
            unlockTransferQueue(pLVKey, "removeFromInFlight - cp");
            l_LocalMetadataLocked = lockLocalMetadataIfNeeded(pLVKey, "removeFromInFlight - cp");
            lockTransferQueue(pLVKey, "removeFromInFlight - cp");
        }

        ContribIdFile::update_xbbServerFileStatus(pLVKey, pExtentInfo.getTransferDef(), pExtentInfo.getHandle(), pExtentInfo.getContrib(), pExtentInfo.getExtent(), BBTD_All_Extents_Transferred);
        l_UpdateTransferStatus = true;
    }

    if (l_UpdateTransferStatus)
    {
        // Update any/all transfer status
        updateAllTransferStatus(pConnectionName, pLVKey, pExtentInfo, THIS_EXTENT_IS_IN_THE_INFLIGHT_QUEUE);

        // NOTE: The handle status does not need to be updated here, as it is updated as part of updating the ContribIdFile
        //       when updateAllTransferStatus() is invoked above.
    }

    // NOTE:  Removing the extent from the in-flight queue has to be done AFTER
    //        any metadata updates above.  When running with multiple transfer threads,
    //        this entry must remain in the queue as the lock on the transfer queue
    //        can be dropped and re-acquired during the processing above.  Other
    //        threads must still see that the processing for this extent is not complete
    //        during that time.
    // Remove the extent from the in-flight queue...
    extentInfo.removeFromInFlight(pLVKey, pExtentInfo);

    // We have to return with the same lock states as when we entered this code
    if (l_LocalMetadataLocked)
    {
        unlockTransferQueue(pLVKey, "removeFromInFlight - After status updates, after removeFromInFlight()");
        unlockLocalMetadata(pLVKey, "removeFromInFlight - After status updates, after removeFromInFlight()");
        lockTransferQueue(pLVKey, "removeFromInFlight - On return");
    }

    return;
}

int BBLV_Info::retrieveTransfers(BBTransferDefs& pTransferDefs)
{
    int rc = 0;

    if (pTransferDefs.getHostName() == UNDEFINED_HOSTNAME || pTransferDefs.getHostName() == hostname)
    {
        if (pTransferDefs.getJobId() == UNDEFINED_JOBID || pTransferDefs.getJobId() == jobid)
        {
            rc = tagInfoMap.retrieveTransfers(pTransferDefs, &extentInfo);
        }

        // If no error, indicate that we found the specific hostname and
        // we do not have to search the cross-bbserver metadata...
        // NOTE:  This could be an issue if we truly can have activity from two bbServers
        //        for a given bbProxy.  For switch/failover, current design is to halt all
        //        requests to the 'original' bbServer before moving the work from that bbServer
        //        to the new bbServer.  Therefore, we should NOT be processing any retrieve transfer
        //        request on the 'original' bbServer which could find some transfer information
        //        in its local metadata which will not reflect reality once the new bbServer
        //        starts processing the transfered work.
        if (!rc)
        {
            rc = 2;
        }
    }

    return rc;
}

void BBLV_Info::sendTransferCompleteForContribIdMsg(const string& pConnectionName, const LVKey* pLVKey, const int64_t pHandle, const int32_t pContribId, BBTransferDef* pTransferDef)
{
    txp::Msg* l_Complete = 0;
    txp::Msg::buildMsg(txp::BB_TRANSFER_COMPLETE_FOR_CONTRIBID, l_Complete);

    //  NOTE:  BBSTOPPED takes precedence over BBFAILED,
    //         which takes precedence over BBCANCELED
    BBSTATUS l_Status = BBFULLSUCCESS;
    if (pTransferDef->stopped()) {
        l_Status = BBSTOPPED;
    } else if (pTransferDef->failed()) {
        l_Status = BBFAILED;
    } else if (pTransferDef->canceled()) {
        l_Status = BBCANCELED;
    }
    char l_StatusStr[64] = {'\0'};
    getStrFromBBStatus(l_Status, l_StatusStr, sizeof(l_StatusStr));

    char l_TransferStatusStr[64] = {'\0'};
    switch (l_Status) {
        case BBSTOPPED:
        {
            strCpy(l_TransferStatusStr, "stopped", sizeof(l_TransferStatusStr));
            break;
        }
        case BBFAILED:
        {
            strCpy(l_TransferStatusStr, "failed", sizeof(l_TransferStatusStr));
            break;
        }
        case BBCANCELED:
        {
            strCpy(l_TransferStatusStr, "canceled", sizeof(l_TransferStatusStr));
            break;
        }
        default:
        {
            strCpy(l_TransferStatusStr, "completed", sizeof(l_TransferStatusStr));
            break;
        }
    }

    Uuid lv_uuid = pLVKey->second;
    char lv_uuid_str[LENGTH_UUID_STR] = {'\0'};
    lv_uuid.copyTo(lv_uuid_str);

    // Calculate the total processing time for this transfer definition
    pTransferDef->calcProcessingTime(pTransferDef->processingTime);

    LOG(bb,info) << "->bbproxy: Transfer " << l_TransferStatusStr << " for contribid " << pContribId << ", " \
                 << *pLVKey << ", handle " << pHandle << ", status " << l_StatusStr \
                 << ", total processing time " << (double)pTransferDef->processingTime/(double)g_TimeBaseScale << " seconds";

    // NOTE:  The char array is copied to heap by addAttribute and the storage for
    //        the logical volume uuid attribute is owned by the message facility.
    //        Our copy can then go out of scope...
    l_Complete->addAttribute(txp::uuid, lv_uuid_str, sizeof(lv_uuid_str), txp::COPY_TO_HEAP);
    l_Complete->addAttribute(txp::handle, pHandle);
    l_Complete->addAttribute(txp::contribid, pContribId);
    l_Complete->addAttribute(txp::status, (int64_t)l_Status);
    l_Complete->addAttribute(txp::totalProcessingTime, pTransferDef->processingTime);

    //    std::string pConnectionName=getConnectionName(pConnection); // $$$mea
    try{
       int rc = sendMessage(pConnectionName,l_Complete);
       if (rc) LOG(bb,info) << "sendMessage rc="<<rc<<" @ func="<<__func__<<":"<<__LINE__;
    }

    catch(exception& e)
    {
        LOG(bb,warning) << "Exception thrown when attempting to send completion for contributor " << pContribId << ": " << e.what();
        assert(strlen(e.what())==0);
    }

    delete l_Complete;

    return;
}

void BBLV_Info::sendTransferCompleteForFileMsg(const string& pConnectionName, const LVKey* pLVKey, ExtentInfo& pExtentInfo, BBTransferDef* pTransferDef)
{
    int rc = 0;

    // NOTE: If this extent is a dummy extent indicating a local copy for BBI_TargetSSDSSD,
    //       a failed or canceled status for that local copy DOES NOT currently effect the
    //       overall status for the transfer definition and/or handle.
    //       \todo - Not sure what the strategy should be here...
    //               Maybe we reflect that failed/canceled status in the dummy extent
    //               sent to bbServer...  @DLH
    txp::Msg* l_Complete = 0;
    txp::Msg::buildMsg(txp::BB_TRANSFER_COMPLETE_FOR_FILE, l_Complete);

    char l_OperationStr[64] = {'\0'};
    BBFILESTATUS l_FileStatus = BBFILE_NONE;
    char l_FileStatusStr[64] = {'\0'};
    char l_SizePhrase[64] = {'\0'};
    size_t l_SizeTransferred = 0;
    if (!((pExtentInfo.getExtent())->flags & BBI_TargetSSDSSD))
    {
        if (!((pExtentInfo.getExtent())->flags & BBI_TargetPFSPFS))
        {
            strCpy(l_SizePhrase, ", size transferred is ", sizeof(l_SizePhrase));
            strCpy(l_OperationStr, "Transfer ", sizeof(l_OperationStr));
            l_SizeTransferred = pTransferDef->getSizeTransferred(pExtentInfo.getSourceIndex());
        }
        else
        {
            strCpy(l_SizePhrase, ", remote size copied is ", sizeof(l_SizePhrase));
            strCpy(l_OperationStr, "Remote PFS cp command ", sizeof(l_OperationStr));
            l_SizeTransferred = pExtentInfo.getExtent()->getLength();
        }

        l_FileStatus = pTransferDef->getFileStatus(pLVKey, pExtentInfo);
        getStrFromBBFileStatus(l_FileStatus, l_FileStatusStr, sizeof(l_FileStatusStr));
    }
    else
    {
        strCpy(l_SizePhrase, ", local size copied is ", sizeof(l_SizePhrase));
        strCpy(l_OperationStr, "Previous cp command on the local compute node ", sizeof(l_OperationStr));

        l_FileStatus = BBFILE_SUCCESS;
        l_SizeTransferred = pExtentInfo.getExtent()->getLength();
        if ((pExtentInfo.getExtent())->flags & BBTD_Stopped)
        {
            l_FileStatus = BBFILE_STOPPED;
            l_SizeTransferred = 0;
        }
        if ((pExtentInfo.getExtent())->flags & BBTD_Failed)
        {
            l_FileStatus = BBFILE_FAILED;
            l_SizeTransferred = 0;
        }
        if ((pExtentInfo.getExtent())->flags & BBTD_Canceled)
        {
            l_FileStatus = BBFILE_CANCELED;
            l_SizeTransferred = 0;
        }
        getStrFromBBFileStatus(l_FileStatus, l_FileStatusStr, sizeof(l_FileStatusStr));
    }

    char l_TransferStatusStr[64] = {'\0'};
    switch (l_FileStatus) {
        case BBFILE_NONE:
        {
            strCpy(l_TransferStatusStr, "has an unknown status", sizeof(l_TransferStatusStr));
            l_SizeTransferred = 0;
            break;
        }
        case BBFILE_STOPPED:
        {
            strCpy(l_TransferStatusStr, "stopped", sizeof(l_TransferStatusStr));
            l_SizeTransferred = 0;
            break;
        }
        case BBFILE_FAILED:
        {
            strCpy(l_TransferStatusStr, "failed", sizeof(l_TransferStatusStr));
            l_SizeTransferred = 0;
            break;
        }
        case BBFILE_CANCELED:
        {
            strCpy(l_TransferStatusStr, "canceled", sizeof(l_TransferStatusStr));
            l_SizeTransferred = 0;
            break;
        }
        default:
        {
            strCpy(l_TransferStatusStr, "completed", sizeof(l_TransferStatusStr));
            break;
        }
    }

    Uuid lv_uuid = pLVKey->second;
    char lv_uuid_str[LENGTH_UUID_STR] = {'\0'};
    lv_uuid.copyTo(lv_uuid_str);
    char l_TransferType[64] = {'\0'};
    getStrFromTransferType((pExtentInfo.getExtent())->flags, l_TransferType, sizeof(l_TransferType));

    LOG(bb,info) << "->bbproxy: " << l_OperationStr << l_TransferStatusStr << " for file " \
                 << pTransferDef->files[pExtentInfo.getSourceIndex()] << ", " << *pLVKey \
                 << ", handle " << pExtentInfo.getHandle() << ", contribid " << pExtentInfo.getContrib() << ", sourceindex " \
                 << pExtentInfo.getSourceIndex() << ", file status " << l_FileStatusStr << ", transfer type " << l_TransferType \
                 << l_SizePhrase << l_SizeTransferred << " bytes, read count/cumulative time " \
                 << pTransferDef->readOperations[pExtentInfo.getSourceIndex()].first << "/" \
                 << (double)pTransferDef->readOperations[pExtentInfo.getSourceIndex()].second/(double)g_TimeBaseScale \
                 << " seconds, write count/cumulative time " << pTransferDef->writeOperations[pExtentInfo.getSourceIndex()].first << "/" \
                 << (double)pTransferDef->writeOperations[pExtentInfo.getSourceIndex()].second/(double)g_TimeBaseScale << " seconds";

    // NOTE:  The char array is copied to heap by addAttribute and the storage for
    //        the logical volume uuid attribute is owned by the message facility.
    //        Our copy can then go out of scope...
    l_Complete->addAttribute(txp::uuid, lv_uuid_str, sizeof(lv_uuid_str), txp::COPY_TO_HEAP);
    l_Complete->addAttribute(txp::jobid, pTransferDef->getJobId());
    l_Complete->addAttribute(txp::handle, pExtentInfo.getHandle());
    l_Complete->addAttribute(txp::contribid, pExtentInfo.getContrib());
    l_Complete->addAttribute(txp::sourceindex, pExtentInfo.getSourceIndex());
    l_Complete->addAttribute(txp::flags, (pExtentInfo.getExtent())->flags);
    l_Complete->addAttribute(txp::sourcefile, pTransferDef->files[pExtentInfo.getSourceIndex()].c_str(), pTransferDef->files[pExtentInfo.getSourceIndex()].size()+1);
    l_Complete->addAttribute(txp::status, (int64_t)l_FileStatus);
    l_Complete->addAttribute(txp::sizetransferred, (int64_t)l_SizeTransferred);
    l_Complete->addAttribute(txp::readcount, pTransferDef->readOperations[pExtentInfo.getSourceIndex()].first);
    l_Complete->addAttribute(txp::readtime, pTransferDef->readOperations[pExtentInfo.getSourceIndex()].second);
    l_Complete->addAttribute(txp::writecount, pTransferDef->writeOperations[pExtentInfo.getSourceIndex()].first);
    l_Complete->addAttribute(txp::writetime, pTransferDef->writeOperations[pExtentInfo.getSourceIndex()].second);

    unlockTransferQueue(pLVKey, "sendTransferCompleteForFileMsg");
    unlockLocalMetadata(pLVKey, "sendTransferCompleteForFileMsg");

    // Send the message and wait for reply
    try
    {
        rc = sendMsgAndWaitForReturnCode(pConnectionName, l_Complete);
    }
    catch(exception& e)
    {
        rc = -1;
        LOG(bb,warning) << "Exception thrown when attempting to send completion for file " << pTransferDef->files[pExtentInfo.getSourceIndex()] << ": " << e.what();
        assert(strlen(e.what())==0);
    }

    lockLocalMetadata(pLVKey, "sendTransferCompleteForFileMsg");
    lockTransferQueue(pLVKey, "sendTransferCompleteForFileMsg");

    if (rc)
    {
        markTransferFailed(pLVKey, pTransferDef, this, pExtentInfo.getHandle(), pExtentInfo.getContrib());
        ContribIdFile::update_xbbServerFileStatus(pLVKey, pExtentInfo.getTransferDef(), pExtentInfo.getHandle(), pExtentInfo.getContrib(), pExtentInfo.getExtent(), BBTD_Failed);
    }

    delete l_Complete;

    return;
}

void BBLV_Info::sendTransferCompleteForHandleMsg(const string& pHostName, const string& pCN_HostName, const string& pConnectionName, const LVKey* pLVKey, const BBTagID pTagId, const uint64_t pHandle, int& pAppendAsyncRequestFlag, const BBSTATUS pStatus)
{
    txp::Msg* l_Complete = 0;
    txp::Msg::buildMsg(txp::BB_TRANSFER_COMPLETE_FOR_HANDLE, l_Complete);

    BBSTATUS l_Status;
    char l_StatusStr[64] = {'\0'};
    bool l_SendMessage = true;

    if (pStatus == BBNONE)
    {
        // NOTE:  We don't catch the RC here...  @DLH
        HandleFile::get_xbbServerHandleStatus(l_Status, pLVKey, pTagId.getJobId(), pTagId.getJobStepId(), pHandle);
    }
    else
    {
        l_Status = pStatus;
    }

    getStrFromBBStatus(l_Status, l_StatusStr, sizeof(l_StatusStr));

    char l_TransferStatusStr[64] = {'\0'};
    switch (l_Status)
    {
        case BBSTOPPED:
        {
            strCpy(l_TransferStatusStr, "stopped", sizeof(l_TransferStatusStr));
            break;
        }
        case BBFAILED:
        {
            strCpy(l_TransferStatusStr, "failed", sizeof(l_TransferStatusStr));
            break;
        }
        case BBCANCELED:
        {
            strCpy(l_TransferStatusStr, "canceled", sizeof(l_TransferStatusStr));
            break;
        }
        case BBFULLSUCCESS:
        case BBPARTIALSUCCESS:
        {
            strCpy(l_TransferStatusStr, "completed", sizeof(l_TransferStatusStr));
            break;
        }
        default:
        {
            l_SendMessage = false;
        }
    }

    if (l_SendMessage)
    {
        Uuid lv_uuid = pLVKey->second;
        char lv_uuid_str[LENGTH_UUID_STR] = {'\0'};
        lv_uuid.copyTo(lv_uuid_str);

        LOG(bb,info) << "->bbproxy: Transfer " << l_TransferStatusStr << " for handle " << pHandle \
                     << ", " << *pLVKey << ", status " << l_StatusStr;

        // NOTE:  The char array is copied to heap by addAttribute and the storage for
        //        the logical volume uuid attribute is owned by the message facility.
        //        Our copy can then go out of scope...
        l_Complete->addAttribute(txp::uuid, lv_uuid_str, sizeof(lv_uuid_str), txp::COPY_TO_HEAP);
        l_Complete->addAttribute(txp::handle, pHandle);
        l_Complete->addAttribute(txp::status, (int64_t)l_Status);

        // Send the message
        int rc = sendMessage(pConnectionName,l_Complete);
        if (rc) LOG(bb,info) << "sendMessage rc="<<rc<<" @ func="<<__func__<<":"<<__LINE__;

        // Determine if this status update for the handle should be appended to the async file
        // to be consumed by other bbServers
        if (sameHostName(pHostName) && pAppendAsyncRequestFlag == ASYNC_REQUEST_HAS_NOT_BEEN_APPENDED)
        {
            size_t l_TotalContributors = 0;
            size_t l_TotalLocalReportingContributors = 0;
            metadata.accumulateTotalLocalContributorInfo(pHandle, l_TotalContributors, l_TotalLocalReportingContributors);
            LOG(bb,debug) << "sendTransferCompleteForHandleMsg: pHandle=" << pHandle << ", l_TotalContributors=" << l_TotalContributors << ", l_TotalLocalReportingContributors=" << l_TotalLocalReportingContributors;
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
                    snprintf(l_AsyncCmd, sizeof(l_AsyncCmd), "handle %lu %lu %lu 0 0 %s %s", pTagId.getJobId(), pTagId.getJobStepId(), pHandle, pCN_HostName.c_str(), l_StatusStr);
                    AsyncRequest l_Request = AsyncRequest(l_AsyncCmd);
                    wrkqmgr.appendAsyncRequest(l_Request);
                    pAppendAsyncRequestFlag = ASYNC_REQUEST_HAS_BEEN_APPENDED;
                }
                else
                {
                    LOG(bb,debug) << "sendTransferCompleteForHandleMsg(): No need to append an async request as all " << l_TotalContributors << " contributors for handle " << pHandle << " are local to this bbServer";
                }
            }
            else
            {
                LOG(bb,debug) << "sendTransferCompleteForHandleMsg(): No need to append an async request as there is only a single contributor for handle " << pHandle;
            }
        }
    }

    delete l_Complete;

    return;
}

void BBLV_Info::setAllExtentsTransferred(const LVKey* pLVKey, const uint64_t pHandle, ExtentInfo& pExtentInfo, const BBTagID pTagId, const int pValue)
{
    pExtentInfo.getTagInfo()->setAllExtentsTransferred(pLVKey, pTagId.getJobId(), pTagId.getJobStepId(), pHandle, pValue);

    return;
}

void BBLV_Info::setCanceled(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, uint64_t pHandle, LOCAL_METADATA_RELEASED& pLockWasReleased, const int pRemoveOption)
{
    // NOTE: pLockWasReleased intentionally not initialized

    if (jobid == pJobId)
    {
        tagInfoMap.setCanceled(pLVKey, pJobId, pJobStepId, pHandle);

        // Sort the extents, moving the canceled extents to the front of
        // the work queue so they are immediately removed...
        uint32_t l_ContribId = UNDEFINED_CONTRIBID;
        cancelExtents(pLVKey, &pHandle, &l_ContribId, 0, pLockWasReleased, pRemoveOption);
    }

    return;
}

int BBLV_Info::setSuspended(const LVKey* pLVKey, const string& pHostName, const int pValue)
{
    int rc = 0;

    if (pHostName == UNDEFINED_HOSTNAME || pHostName == hostname)
    {
        rc = extentInfo.setSuspended(pLVKey, hostname, jobid, pValue);
    }
    else
    {
        rc = 1;
    }

    return rc;
}

int BBLV_Info::stopTransfer(const LVKey* pLVKey, const string& pHostName, const string& pCN_HostName, const uint64_t pJobId, const uint64_t pJobStepId, uint64_t pHandle, uint32_t pContribId, LOCAL_METADATA_RELEASED& pLockWasReleased)
{
    int rc = 0;

    // NOTE: pLockWasReleased intentionally not initialized

    if ((pCN_HostName == UNDEFINED_HOSTNAME || pCN_HostName == hostname) && (pJobId == UNDEFINED_JOBID || pJobId == jobid))
    {
        if (!stageOutStarted())
        {
            rc = tagInfoMap.stopTransfer(pLVKey, this, pHostName, pCN_HostName, jobid, pJobStepId, pHandle, pContribId, pLockWasReleased);

            if (rc == 1)
            {
                // Transfer definition was successfully stopped...
                //
                // Sort the extents, moving the canceled extents to the front of
                // the work queue so they are immediately removed...
                cancelExtents(pLVKey, &pHandle, &pContribId, 0, pLockWasReleased, DO_NOT_REMOVE_TARGET_PFS_FILES);
            }
        }
        else
        {
            // Stageout end processing has started.  Therefore, the ability to do anything using this LVKey
            // will soon be, or has already, been removed.  (i.e. the local cache of data is being/or has been
            // torn down...)  Therefore, the only meaningful thing left to be done is remove job information.
            // Return an error message.
            rc = -1;
            LOG(bb,error) << "BBLV_Info::stopTransfer(): For pCN_HostName " << pCN_HostName << ", connection " \
                          << connectionName << ", jobid " << jobid << ", jobidstep " << pJobStepId \
                          << ", handle " << pHandle << ", contribid " << pContribId \
                          << ", the remove logical volume request has been run, or is currently running for " << *pLVKey \
                          << ". Suspend or resume operations are not allowed for this environment.";
        }
    }

    return rc;
}

void BBLV_Info::updateAllContribsReported(const LVKey* pLVKey)
{
    int l_AllContribsReported = 0;
    tagInfoMap.updateAllContribsReported(pLVKey, l_AllContribsReported);
    if (l_AllContribsReported) {
        setAllContribsReported(pLVKey);
    }

    return;
}

int BBLV_Info::updateAllTransferStatus(const string& pConnectionName, const LVKey* pLVKey, ExtentInfo& pExtentInfo, uint32_t pNumberOfExpectedInFlight)
{
    int rc = 0;

    BBTransferDef* l_TransferDef = pExtentInfo.getTransferDef();

    LOG(bb,debug) << "updateAllTransferStatus():   Connection name=" << pConnectionName;

    // Check/update the status for the transfer definition
    int l_NewStatus = 0;
    int l_ExtentsRemainForSourceIndex = 1;
    bool l_SetFileClosedIndicator = false;

    int l_LocalMetadataLocked = lockLocalMetadataIfNeeded(pLVKey, "BBLV_Info::updateAllTransferStatus");

    updateTransferStatus(pLVKey, pExtentInfo, l_TransferDef, l_NewStatus, l_ExtentsRemainForSourceIndex, pNumberOfExpectedInFlight);
    if (!l_ExtentsRemainForSourceIndex)
    {
        // Send message to bbproxy indicating the transfer for the source index file is complete.
        sendTransferCompleteForFileMsg(pConnectionName, pLVKey, pExtentInfo, l_TransferDef);

        // Perform any necessary closes
        Extent* l_Extent = pExtentInfo.getExtent();
        uint16_t l_BundleId = l_Extent->getBundleID();
        BBIO* l_IO = l_TransferDef->iomap[l_BundleId];
        if (l_IO)
        {
            if ((l_Extent->flags & BBI_TargetSSD) || (l_Extent->flags & BBI_TargetPFS))
            {
                uint32_t pfs_idx = (l_Extent->flags & BBI_TargetSSD) ? l_Extent->sourceindex : l_Extent->targetindex;
                LOG(bb,debug) << "BBIO:  Closing index " << pfs_idx;
                l_IO->close(pfs_idx);
            }
        }
        else
        {
            LOG(bb,error) << "updateAllTransferStatus: Could not retrieve the BBIO object for extent " << *l_Extent;
        }
        l_SetFileClosedIndicator = true;
    }

    // NOTE: The transfer queue lock is released and re-acquired as part of the send message
    //       operation for the file.  Therefore, the l_NewStatus flag that was calculated above
    //       cannot be trusted to determine if the completion message for the contribid should
    //       be sent.  Another thread could get in during the send message operation
    //       and remove their last extent for a different file from the in-flight queue.
    //       The l_NewStatus flag above would be off when this really is the processing
    //       for the last extent, for the last file for this transfer definition.
    //       The update performed below is not for this file's status, but simply to
    //       determine if all files for the definition are now complete, to accurately
    //       update the cross bbserver metadata, and indicate if the completion
    //       message for the contribid should be sent.

    // Perform updates of metadata again because of multiple transfer threads and the transfer queue lock possibly being
    // dropped and re-acquired as part of sending the file complete message above.
    updateTransferStatus(pLVKey, pExtentInfo, l_TransferDef, l_NewStatus, l_ExtentsRemainForSourceIndex, pNumberOfExpectedInFlight);

    // NOTE:  The update of the BBTD_All_Files_Closed flag for the overall ContribId file cannot be calculated properly
    //        until the above update of the transfer status is finally performed. The update_xbbServerFileStatus() was already
    //        performed by sendTransferCompleteForFileMsg() above, but because the updateTransferStatus() has to
    //        be issued again because the transfer queue lock was dropped and re-acquired as part of sending the transfer
    //        complete for the file, this update must also be issued again.
    if (l_SetFileClosedIndicator)
    {
        ContribIdFile::update_xbbServerFileStatus(pLVKey, l_TransferDef, pExtentInfo.getHandle(), pExtentInfo.getContrib(), pExtentInfo.getExtent(), BBTD_All_Files_Closed);
    }

    // Now, the l_NewStatus flag properly indicates if the ContribId complete message should be sent
    if (l_NewStatus)
    {
        if (!l_TransferDef->stopped())
        {
            if (l_LocalMetadataLocked)
            {
                unlockLocalMetadata(pLVKey, "BBLV_Info::updateAllTransferStatus - before sendTransferCompleteForContribIdMsg");
            }
            sendTransferCompleteForContribIdMsg(pConnectionName, pLVKey, pExtentInfo.getHandle(), pExtentInfo.getContrib(), l_TransferDef);
            l_LocalMetadataLocked = lockLocalMetadataIfNeeded(pLVKey, "BBLV_Info::updateAllTransferStatus - after sendTransferCompleteForContribIdMsg");
        }

        // Status changed for transfer definition...
        // Check/update the status for the transferHandle
        l_NewStatus = 0;
        BBTagID l_TagId = BBTagID(l_TransferDef->getJob(), l_TransferDef->getTag());
        updateTransferStatus(pLVKey, pExtentInfo, l_TagId, pExtentInfo.getContrib(), l_NewStatus, pNumberOfExpectedInFlight);

        if (l_NewStatus)
        {
            if (!l_TransferDef->stopped())
            {
                // Status changed for transfer handle...
                // Send the transfer is complete for this handle message to bbProxy
                string l_HostName;
                activecontroller->gethostname(l_HostName);
                metadata.sendTransferCompleteForHandleMsg(l_HostName, l_TransferDef->getHostName(), pExtentInfo.getHandle());
            }

            // Check/update the status for the LVKey
            // NOTE:  If the status changes at the LVKey level, the updateTransferStatus() routine will send the message...
            updateTransferStatus(pConnectionName, pLVKey, pNumberOfExpectedInFlight);
        }
    }

    if (l_LocalMetadataLocked)
    {
        unlockLocalMetadata(pLVKey, "BBLV_Info::updateAllTransferStatus");
    }

    return rc;
}

void BBLV_Info::updateTransferStatus(const LVKey* pLVKey, ExtentInfo& pExtentInfo, const BBTagID& pTagId, const int32_t pContribId, int& pNewStatus, uint32_t pNumberOfExpectedInFlight)
{
    int l_LocalMetadataLocked = lockLocalMetadataIfNeeded(pLVKey, "BBLV_Info::updateTransferStatus");

    // NOTE:  The following method retrieves its information from the xbbServer metadata...
    if (allContribsReported(pExtentInfo.getHandle(), pTagId)) {
        // NOTE:  The following method retrieves its information from the xbbServer metadata...
        int rc = ContribIdFile::allExtentsTransferredButThisContribId(pExtentInfo.getHandle(), pTagId, pContribId);
        switch (rc)
        {
            case -1:
                LOG(bb,error) << "updateAllTransferStatus: Error when attempting to determine if all extents have been transferred";
                break;
            case 0:
                break;
            case 1:
            {
                pNewStatus = 1;
                setAllExtentsTransferred(pLVKey, pExtentInfo.getHandle(), pExtentInfo, pTagId);
                break;
            }
        }
    }

    if (l_LocalMetadataLocked)
    {
        unlockLocalMetadata(pLVKey, "BBLV_Info::updateTransferStatus");
    }

    return;
}

