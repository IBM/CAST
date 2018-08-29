/*******************************************************************************
 |    BBTagInfo2.cc
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
#include "bbserver_flightlog.h"
#include "BBTagInfo2.h"
#include "BBTransferDef.h"
#include "bbwrkqmgr.h"
#include "ContribIdFile.h"
#include "fh.h"
#include "HandleFile.h"
#include "logging.h"
#include "xfer.h"

//
// BBTagInfo2 class
//

void BBTagInfo2::accumulateTotalLocalContributorInfo(const uint64_t pHandle, size_t& pTotalContributors, size_t& pTotalLocalReportingContributors)
{
    tagInfoMap.accumulateTotalLocalContributorInfo(pHandle, pTotalContributors, pTotalLocalReportingContributors);

    return;
}

int BBTagInfo2::allContribsReported(const uint64_t pHandle, const BBTagID& pTagId)
{
    int rc = 0;

    // Check the cross bbServer metadata and return the flag indicator for BBTI_All_Contribs_Reported.
    HandleFile* l_HandleFile = 0;
    char* l_HandleFileName = 0;
    rc = HandleFile::loadHandleFile(l_HandleFile, l_HandleFileName, pTagId.getJobId(), pTagId.getJobStepId(), pHandle);
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

int BBTagInfo2::allExtentsTransferred(const BBTagID& pTagId)
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

void BBTagInfo2::cancelExtents(const LVKey* pLVKey, uint64_t* pHandle, uint32_t* pContribId, const int pRemoveOption)
{
    // Sort the extents, moving the canceled extents to the front of
    // the work queue so they are immediately removed...
    extentInfo.sortExtents(pLVKey, pHandle, pContribId);

    // Indicate that next findWork() needs to look for canceled extents
    wrkqmgr.setCheckForCanceledExtents(1);

    // If we are to perform remove operations for target PFS files, do so now...
    if (pRemoveOption == REMOVE_TARGET_PFS_FILES)
    {
        // Wait for the canceled extents to be processed
        uint64_t l_Attempts = 1;
        while (1)
        {
            if (wrkqmgr.getCheckForCanceledExtents())
            {
                if ((l_Attempts % 15) == 0)
                {
                    // Display this message every 15 seconds...
                    LOG(bb,info) << ">>>>> DELAY <<<<< BBTagInfo2::cancelExtents: For " << *pLVKey << ", handle " << *pHandle << ", contribid " << *pContribId \
                                 << ", waiting for all canceled extents to finished being processed.  Delay of 1 second before retry.";
                }

                unlockTransferQueue(pLVKey, "cancelExtents - Waiting for the canceled extents to be processed");
                {
                    usleep((useconds_t)1000000);    // Delay 1 second
                }
                lockTransferQueue(pLVKey, "cancelExtents - Waiting for the canceled extents to be processed");
                ++l_Attempts;
            }
            else
            {
                break;
            }
        }

        // Remove the target files
        LOG(bb,info) << "Start: Removing target files associated with transfer " << *pLVKey << ", handle " << *pHandle << ", contribid " << *pContribId;
        removeTargetFiles(pLVKey, *pHandle, *pContribId);
        LOG(bb,info) << "Completed: Removing target files associated with transfer " << *pLVKey << ", handle " << *pHandle << ", contribid " << *pContribId;
    }

    return;
}

void BBTagInfo2::changeServer()
{

    return;
}


void BBTagInfo2::cleanUpAll(const LVKey* pLVKey)
{
    // Cleanup the LVKey...
    tagInfoMap.cleanUpAll(pLVKey);

    return;
}

void BBTagInfo2::dump(char* pSev, const char* pPrefix)
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

void BBTagInfo2::ensureStageOutEnded(const LVKey* pLVKey)
{

    if (!stageOutEnded()) {
        LOG(bb,info) << "taginfo: Stageout end processing being initiated for jobid " << jobid << ", for " << *pLVKey;

        // NOTE: if stageoutEnd() fails, it fills in errstate.  However, we are not setting a rc here...
        if (stageoutEnd(string(""), pLVKey, FORCED))
        {
            LOG(bb,error) << "BBTagInfo2::ensureStageOutEnded():  Failure from stageoutEnd() for LVKey " << *pLVKey;
        }
    } else {
        uint32_t i = 0;
        while (!stageOutEndedComplete())
        {
            unlockTransferQueue(pLVKey, "ensureStageOutEnded - Waiting for stageout end to complete");
            {
                if (!(i++ % 30))
                {
                    LOG(bb,info) << ">>>>> DELAY <<<<< ensureStageOutEnded(): Waiting for stageout end processing to complete for " << *pLVKey;
                }
                usleep((useconds_t)2000000);
           }
           lockTransferQueue(pLVKey, "ensureStageOutEnded - Waiting for stageout end to complete");
        }
    }

    return;
}

BBSTATUS BBTagInfo2::getStatus(const uint64_t pHandle, BBTagInfo* pTagInfo)
{
    BBSTATUS l_Status = BBNONE;

    if (extentInfo.moreExtentsToTransfer((int64_t)pHandle, (int32_t)(-1), 0)) {
        l_Status = BBINPROGRESS;
    } else {
        l_Status = pTagInfo->getStatus(extentInfo.stageOutStarted());
    }

    return l_Status;
}

BBSTATUS BBTagInfo2::getStatus(const uint64_t pHandle, const uint32_t pContribId, BBTagInfo* pTagInfo)
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

int BBTagInfo2::getTransferHandle(uint64_t& pHandle, const LVKey* pLVKey, const BBJob pJob, const uint64_t pTag, const uint64_t pNumContrib, const uint32_t pContrib[])
{
    int rc = 0;
    stringstream errorText;

    BBTagID l_TagId = BBTagID(pJob, pTag);
    BBTagInfo* l_TagInfo = tagInfoMap.getTagInfo(l_TagId);

    stringstream l_JobStr;
    l_TagId.getJob().getStr(l_JobStr);

    if(!l_TagInfo)
    {
        int l_GeneratedHandle = 0;
        BBTagInfo l_NewTagInfo = BBTagInfo(&tagInfoMap, pNumContrib, pContrib, pJob, pTag, l_GeneratedHandle);
        rc = tagInfoMap.addTagInfo(pLVKey, pJob, l_TagId, l_NewTagInfo, l_GeneratedHandle);
        if (!rc) {
            l_TagInfo = tagInfoMap.getTagInfo(l_TagId);
            LOG(bb,debug) << "taginfo: Adding TagID(" << l_JobStr.str() << "," << l_TagId.getTag() << ") with handle 0x" \
                          << hex << uppercase << setfill('0') << setw(16) << l_TagInfo->transferHandle \
                          << setfill(' ') << nouppercase << dec << " (" << l_TagInfo->transferHandle << ") to " << *pLVKey;
        } else {
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

int BBTagInfo2::prepareForRestart(const string& pConnectionName, const LVKey* pLVKey, BBTagInfo* pTagInfo, const BBJob pJob, const uint64_t pHandle, const int32_t pContribId, BBTransferDef* pOrigTransferDef, BBTransferDef* pRebuiltTransferDef, const int pPass)
{
    int rc = 0;

    LOG(bb,debug) << "BBTagInfo2::prepareForRestart(): Pass " << pPass;

    // First, perform handle related processing to prepare for the restart
    rc = pTagInfo->prepareForRestart(pConnectionName, pLVKey, pJob, pHandle, pContribId, pOrigTransferDef, pRebuiltTransferDef, pPass);

    if (pPass == THIRD_PASS)
    {
        // NOTE:  Today, the stageout started flag is NOT turned on for an LVKey/jobid/handle.
        //        Therefore we do not attempt to replicate that flag, and related ones,
        //        in the BBLVKey_ExtentInfo flags data contained within BBTagInfo2.
        //        Upon restart to a new bbServer, we *should* copy those flag values
        //        from the cross bbServer metadata to the newly constructed local
        //        metadata.
        // NOTE:  For restart, we should not be concerned with the stageout end
        //        and stage out end completed flags as they should always be off
        //        for a restart scenario.  Those flags are only used as part of
        //        remove logical volume processing.
        // \todo - If we ever become dependent upon these flags, need to copy those
        //         values (not exactly sure where in the restart path...) -or-
        //         never rely on the local cached flag values and always go out
        //         to the cross bbServer metadata for those flag vlaues.

        // If last pass, set the appropriate flags in the LVKey related local metadata
        extentInfo.setAllExtentsTransferred(pConnectionName, pLVKey, 0);
    }

    return rc;
}

int BBTagInfo2::recalculateFlags(const string& pConnectionName, const LVKey* pLVKey, BBTagInfoMap* pTagInfoMap, BBTagInfo* pTagInfo, const int64_t pHandle, const int32_t pContribId)
{
    int rc = 0;

    // NOTE: This may not be needed if when resetting the local metadata,
    //       the cross-bbServer metadata is updated accordingly...

    return rc;
}

void BBTagInfo2::removeFromInFlight(const string& pConnectionName, const LVKey* pLVKey, BBTagInfo* pTagInfo, ExtentInfo& pExtentInfo)
{
    const uint32_t THIS_EXTENT_IS_IN_THE_INFLIGHT_QUEUE = 1;
    bool l_UpdateTransferStatus = false;

    // Check to see if this is the last extent to be transferred for the source file
    // NOTE:  isCP_Transfer() indicates this is a transfer performed via cp either locally on
    //        the compute node or remotely I/O node to the PFS.
    if ( (!(pExtentInfo.getExtent()->isCP_Transfer())) )
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
            while (extentInfo.moreExtentsToTransferForFile(pExtentInfo.getHandle(), pExtentInfo.getContrib(), pExtentInfo.getSourceIndex(), THIS_EXTENT_IS_IN_THE_INFLIGHT_QUEUE, l_DumpOption))
            {
                unlockTransferQueue(pLVKey, "removeFromInFlight - Waiting for inflight queue to clear");
                {
                    // NOTE: Currently set to send info to console after 1 second of not being able to clear, and every 10 seconds thereafter...
                    if ((i++ % 40) == 4)
                    {
                        LOG(bb,info) << ">>>>> DELAY <<<<< removeFromInFlight(): Processing last extent, waiting for in-flight queue to clear of extents for handle " << pExtentInfo.getHandle() \
                                     << ", contribid " << pExtentInfo.getContrib() << ", sourceindex " << pExtentInfo.getSourceIndex();
                    }
                    usleep((useconds_t)250000);
                    // NOTE: Currently set to dump after 3 seconds of not being able to clear, and every 10 seconds thereafter...
                    if ((i % 40) == 12)
                    {
                        l_DumpOption = MORE_EXTENTS_TO_TRANSFER_FOR_FILE;
                    }
                    else
                    {
                        l_DumpOption = DO_NOT_DUMP_QUEUES_ON_VALUE;
                    }
                }
                lockTransferQueue(pLVKey, "removeFromInFlight - Waiting for inflight queue to clear");
            }

            if (!pExtentInfo.getTransferDef()->stopped())
            {
                uint16_t l_BundleId = pExtentInfo.getExtent()->getBundleID();
                BBIO* l_IO = pExtentInfo.getTransferDef()->iomap[l_BundleId];
                if (l_IO)
                {
                    // Perform any necessary syncing of the data for the target file
                    unlockTransferQueue(pLVKey, "removeFromInFlight - Before fsync");

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

                    lockTransferQueue(pLVKey, "removeFromInFlight - After fsync");
                }
                else
                {
                    LOG(bb,error) << "removeFromInFlight: Could not retrieve the BBIO object for extent " << pExtentInfo.getExtent();
                }
            }

            // Update the status for the file in xbbServer data
            ContribIdFile::update_xbbServerFileStatus(pLVKey, pExtentInfo.getTransferDef(), pExtentInfo.getHandle(), pExtentInfo.getContrib(), pExtentInfo.getExtent(), BBTD_All_Extents_Transferred);

            l_UpdateTransferStatus = true;
        }
    }
    else
    {
        l_UpdateTransferStatus = true;
    }

    if (l_UpdateTransferStatus)
    {
        // Update any/all transfer status
        updateAllTransferStatus(pConnectionName, pLVKey, pExtentInfo, THIS_EXTENT_IS_IN_THE_INFLIGHT_QUEUE);

        // Update handle status
        if (HandleFile::update_xbbServerHandleStatus(pLVKey, pExtentInfo.getTransferDef()->getJobId(), pExtentInfo.getTransferDef()->getJobStepId(), pExtentInfo.getHandle(), 0))
        {
            LOG(bb,error) << "BBTagInfo2::removeFromInFlight():  Failure when attempting to update the cross bbServer handle status for jobid " << pExtentInfo.getTransferDef()->getJobId() \
                          << ", jobstepid " << pExtentInfo.getTransferDef()->getJobStepId() << ", handle " << pExtentInfo.getHandle() << ", contribid " << pExtentInfo.getContrib();
        }
    }

    // NOTE:  Removing the extent from the in-flight queue has to be done AFTER
    //        any metadata updates above.  When running with multiple transfer threads,
    //        this entry must remain in the queue as the lock on the transfer queue
    //        can be dropped and re-acquired during the processing above.  Other
    //        threads must still see that the processing for this extent is not complete
    //        during that time.
    // Remove the extent from the in-flight queue...
    extentInfo.removeFromInFlight(pLVKey, pExtentInfo);

    return;
}

int BBTagInfo2::retrieveTransfers(BBTransferDefs& pTransferDefs)
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
        //        request on the 'original' bbServer which could find some trnasfer information
        //        in its local metadata which will not reflect reality once the new bbServer
        //        starts processing the transfered work.
        //        \todo - Need to verify and determine the exposure of having this local metadata
        //                on the original bbServer...  @DLH
        if (!rc)
        {
            rc = 2;
        }
    }

    return rc;
}

void BBTagInfo2::sendTransferCompleteForContribIdMsg(const string& pConnectionName, const LVKey* pLVKey, const int64_t pHandle, const int32_t pContribId, BBTransferDef* pTransferDef)
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

    LOG(bb,info) << "->bbproxy: Transfer " << l_TransferStatusStr << " for contribid " << pContribId \
                 << ":  " << *pLVKey << ", handle " << pHandle << ", status " << l_StatusStr;

    // NOTE:  The char array is copied to heap by addAttribute and the storage for
    //        the logical volume uuid attribute is owned by the message facility.
    //        Our copy can then go out of scope...
    l_Complete->addAttribute(txp::uuid, lv_uuid_str, sizeof(lv_uuid_str), txp::COPY_TO_HEAP);
    l_Complete->addAttribute(txp::handle, pHandle);
    l_Complete->addAttribute(txp::contribid, pContribId);
    l_Complete->addAttribute(txp::status, (int64_t)l_Status);

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

void BBTagInfo2::sendTransferCompleteForFileMsg(const string& pConnectionName, const LVKey* pLVKey, ExtentInfo& pExtentInfo, BBTransferDef* pTransferDef)
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
    size_t l_SizeTransferred = 0;
    if (!((pExtentInfo.getExtent())->flags & BBI_TargetSSDSSD))
    {
        if (!((pExtentInfo.getExtent())->flags & BBI_TargetPFSPFS))
        {
            strCpy(l_OperationStr, "Transfer ", sizeof(l_OperationStr));
            l_SizeTransferred = pTransferDef->getSizeTransferred(pExtentInfo.getSourceIndex());
        }
        else
        {
            strCpy(l_OperationStr, "Remote PFS cp command ", sizeof(l_OperationStr));
            l_SizeTransferred = pExtentInfo.getExtent()->getLength();
        }

        l_FileStatus = pTransferDef->getFileStatus(pLVKey, pExtentInfo);
        getStrFromBBFileStatus(l_FileStatus, l_FileStatusStr, sizeof(l_FileStatusStr));
    }
    else
    {
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
                 << pTransferDef->files[pExtentInfo.getSourceIndex()] << ", " << *pLVKey << ",";
    LOG(bb,info) << "           handle " << pExtentInfo.getHandle() << ", contribid " << pExtentInfo.getContrib() << ", sourceindex " \
                 << pExtentInfo.getSourceIndex() << ", file status " << l_FileStatusStr << ",";
    LOG(bb,info) << "           transfer type " << l_TransferType << ", size transferred is " << l_SizeTransferred << ".";

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

    bool l_LockTransferQueue = false;
    if (wrkqmgr.transferQueueIsLocked())
    {
        l_LockTransferQueue = true;
        unlockTransferQueue(pLVKey, "sendTransferCompleteForFileMsg");
    }

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


    if (l_LockTransferQueue)
    {
        lockTransferQueue(pLVKey, "sendTransferCompleteForFileMsg");
    }

    if (rc)
    {
        markTransferFailed(pLVKey, pTransferDef, pExtentInfo.getHandle(), pExtentInfo.getContrib());
        ContribIdFile::update_xbbServerFileStatus(pLVKey, pExtentInfo.getTransferDef(), pExtentInfo.getHandle(), pExtentInfo.getContrib(), pExtentInfo.getExtent(), BBTD_Failed);
    }

    ContribIdFile::update_xbbServerFileStatus(pLVKey, pTransferDef, pExtentInfo.getHandle(), pExtentInfo.getContrib(), pExtentInfo.getExtent(), BBTD_All_Files_Closed);


    delete l_Complete;

    return;
}

void BBTagInfo2::sendTransferCompleteForHandleMsg(const string& pHostName, const string& pCN_HostName, const string& pConnectionName, const LVKey* pLVKey, const BBTagID pTagId, const uint64_t pHandle, int& pAppendAsyncRequestFlag, const BBSTATUS pStatus)
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
                     << ":  " << *pLVKey << ", status " << l_StatusStr;

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
                    snprintf(l_AsyncCmd, sizeof(l_AsyncCmd), "handle %lu %lu %lu 0 0 %s %s", pTagId.getJobId(), pTagId.getJobStepId(), pHandle, pCN_HostName.c_str(), l_TransferStatusStr);
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

void BBTagInfo2::setAllExtentsTransferred(const LVKey* pLVKey, const uint64_t pHandle, const BBLVKey_ExtentInfo& pLVKey_ExtentInfo, const BBTagID pTagId, const int pValue)
{
    BBTagInfo* l_TagInfo = tagInfoMap.getTagInfo(pTagId);
    if (l_TagInfo)
    {
        l_TagInfo->setAllExtentsTransferred(pLVKey, pTagId.getJobId(), pTagId.getJobStepId(), pHandle, pValue);
    } else {
        // Send error message...
    }

    return;
}

void BBTagInfo2::setCanceled(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, uint64_t pHandle, const int pRemoveOption)
{
    if (jobid == pJobId)
    {
        tagInfoMap.setCanceled(pLVKey, pJobId, pJobStepId, pHandle);

        // Sort the extents, moving the canceled extents to the front of
        // the work queue so they are immediately removed...
        uint32_t l_ContribId = UNDEFINED_CONTRIBID;
        cancelExtents(pLVKey, &pHandle, &l_ContribId, pRemoveOption);
    }

    return;
}

int BBTagInfo2::setSuspended(const LVKey* pLVKey, const string& pHostName, const int pValue)
{
    int rc = 0;

    if (pHostName == UNDEFINED_HOSTNAME || pHostName == hostname)
    {
        if (!stageOutStarted())
        {
            rc = wrkqmgr.setSuspended(pLVKey, pValue);
            switch (rc)
            {
                case 0:
                {
                    if ((((flags & BBTI2_Suspended) == 0) && pValue) || ((flags & BBTI2_Suspended) && (!pValue)))
                    {
                        LOG(bb,info) << "BBTagInfo2::setSuspended(): For hostname " << pHostName << ", connection " \
                                     << connectionName << ", " << *pLVKey << ", jobid " << jobid \
                                     << " -> Changing from: " << ((flags & BBTI2_Suspended) ? "true" : "false") << " to " << (pValue ? "true" : "false");
                    }
                    SET_FLAG(BBTI2_Suspended, pValue);
                }
                break;

                case -2:
                {
                    // NOTE: For failover cases, it is possible for a setSuspended() request to be issued to this bbServer before any request
                    //       has 'used' the LVKey and required the work queue to be present.  We simply tolerate the condition...
                    string l_Temp = "resume";
                    if (pValue)
                    {
                        // Connection being suspended
                        l_Temp = "suspend";
                    }
                    LOG(bb,info) << "BBTagInfo2::setSuspended(): For hostname " << pHostName << ", connection " \
                                 << connectionName << ", jobid " << jobid << ", work queue not present for " << *pLVKey \
                                 << ". Tolerated condition for a " << l_Temp << " operation.";
                }
                break;

                case 2:
                    break;

                default:
                    LOG(bb,info) << "BBTagInfo2::setSuspended(): Unexpected return code " << rc \
                                 << " received for hostname " << pHostName << ", connection " \
                                 << connectionName << ", jobid " << jobid << ", " << *pLVKey \
                                 << " when attempting the suspend or resume operation on the work queue.";
                    rc = -1;
                    break;
            }
        }
        else
        {
            // Stageout end processing has started.  Therefore, the ability to do anything using this LVKey
            // will soon be, or has already, been removed.  (i.e. the local cache of data is being/or has been
            // torn down...)  Therefore, the only meaningful thing left to be done is remove job information.
            // Return an error message.
            rc = -1;
            LOG(bb,error) << "BBTagInfo2::setSuspended(): For hostname " << pHostName << ", connection " \
                          << connectionName << ", jobid " << jobid \
                          << ", the remove logical volume request has been run, or is currently running" \
                          << " for " << *pLVKey << ". Suspend or resume operations are not allowed for this environment.";
        }
    }
    else
    {
        rc = 1;
    }

    return rc;
}

int BBTagInfo2::stopTransfer(const LVKey* pLVKey, const string& pHostName, const uint64_t pJobId, const uint64_t pJobStepId, uint64_t pHandle, uint32_t pContribId)
{
    int rc = 0;

    if ((pHostName == UNDEFINED_HOSTNAME || pHostName == hostname) && (pJobId == UNDEFINED_JOBID || pJobId == jobid))
    {
        if (!stageOutStarted())
        {
            // NOTE: If we are using multiple transfer threads, we have to make sure that there are
            //       no extents for this transfer definition currently in-flight on this bbServer...
            //       If so, delay for a bit...
            // NOTE: In the normal case, the work queue for the CN hostname on this bbServer should
            //       be suspended, so no new extents should start processing when we release/re-acquire
            //       the lock below...
            uint32_t i = 0;
            while (extentInfo.moreInFlightExtentsForTransferDefinition(pHandle, pContribId))
            {
                unlockTransferQueue(pLVKey, "stopTransfer - Waiting for inflight queue to clear");
                {
                    // NOTE: Currently set to send info to console after 1 second of not being able to clear, and every 10 seconds thereafter...
                    if ((i++ % 40) == 4)
                    {
                        LOG(bb,info) << ">>>>> DELAY <<<<< stopTransfer():Waiting for in-flight queue to clear of extents for handle " << pHandle \
                                     << ", contribid " << pContribId;
                    }
                    usleep((useconds_t)250000);
                }
                lockTransferQueue(pLVKey, "stopTransfer - Waiting for inflight queue to clear");
            }

            rc = tagInfoMap.stopTransfer(pLVKey, this, pHostName, pJobId, pJobStepId, pHandle, pContribId);

            if (rc == 1)
            {
                // Transfer definition was successfully stopped...
                //
                // Sort the extents, moving the canceled extents to the front of
                // the work queue so they are immediately removed...
                cancelExtents(pLVKey, &pHandle, &pContribId, DO_NOT_REMOVE_TARGET_PFS_FILES);
            }
        }
        else
        {
            // Stageout end processing has started.  Therefore, the ability to do anything using this LVKey
            // will soon be, or has already, been removed.  (i.e. the local cache of data is being/or has been
            // torn down...)  Therefore, the only meaningful thing left to be done is remove job information.
            // Return an error message.
            rc = -1;
            LOG(bb,error) << "BBTagInfo2::stopTransfer(): For hostname " << pHostName << ", connection " \
                          << connectionName << ", jobid " << pJobId << ", jobidstep " << pJobStepId \
                          << ", handle " << pHandle << ", contribid " << pContribId \
                          << ", the remove logical volume request has been run, or is currently running for " << *pLVKey \
                          << ". Suspend or resume operations are not allowed for this environment.";
        }
    }

    return rc;
}

void BBTagInfo2::updateAllContribsReported(const LVKey* pLVKey)
{
    int l_AllContribsReported = 0;
    tagInfoMap.updateAllContribsReported(pLVKey, l_AllContribsReported);
    if (l_AllContribsReported) {
        setAllContribsReported(pLVKey);
    }

    return;
}

int BBTagInfo2::updateAllTransferStatus(const string& pConnectionName, const LVKey* pLVKey, ExtentInfo& pExtentInfo, uint32_t pNumberOfExpectedInFlight)
{
    int rc = 0;

    BBTransferDef* l_TransferDef = pExtentInfo.getTransferDef();

    LOG(bb,debug) << "updateAllTransferStatus():   Connection name=" << pConnectionName;

    // Check/update the status for the transfer definition
    int l_NewStatus = 0;
    int l_ExtentsRemainForSourceIndex = 1;

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
    if (!l_ExtentsRemainForSourceIndex)
    {
        ContribIdFile::update_xbbServerFileStatus(pLVKey, l_TransferDef, pExtentInfo.getHandle(), pExtentInfo.getContrib(), pExtentInfo.getExtent(), BBTD_All_Files_Closed);
    }

    // Now, the l_NewStatus flag properly indicates if the ContribId complete message should be sent
    if (l_NewStatus)
    {
        // NOTE: \todo - Not sure about the following long-term...  @DLH
        if (!l_TransferDef->stopped())
        {
            sendTransferCompleteForContribIdMsg(pConnectionName, pLVKey, pExtentInfo.getHandle(), pExtentInfo.getContrib(), l_TransferDef);
        }
        // Status changed for transfer definition...
        // Check/update the status for the transferHandle
        l_NewStatus = 0;
        BBTagID l_TagId = BBTagID(l_TransferDef->getJob(), l_TransferDef->getTag());
        updateTransferStatus(pLVKey, pExtentInfo, l_TagId, pExtentInfo.getContrib(), l_NewStatus, pNumberOfExpectedInFlight);

        if (l_NewStatus && (!l_TransferDef->stopped()))
        {

            // Status changed for transfer handle...
            // Send the transfer is complete for this handle message to bbProxy
            string l_HostName;
            activecontroller->gethostname(l_HostName);
            metadata.sendTransferCompleteForHandleMsg(l_HostName, l_TransferDef->getHostName(), pExtentInfo.getHandle());

            // Check/update the status for the LVKey
            // NOTE:  If the status changes at the LVKey level, the updateTransferStatus() routine will send the message...
            updateTransferStatus(pConnectionName, pLVKey, pNumberOfExpectedInFlight);
        }
    }

    return rc;
}

void BBTagInfo2::updateTransferStatus(const LVKey* pLVKey, ExtentInfo& pExtentInfo, const BBTagID& pTagId, const int32_t pContribId, int& pNewStatus, uint32_t pNumberOfExpectedInFlight)
{
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
                setAllExtentsTransferred(pLVKey, pExtentInfo.getHandle(), extentInfo, pTagId);
                break;
            }
        }
    }

    return;
}

