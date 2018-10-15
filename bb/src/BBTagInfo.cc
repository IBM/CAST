/*******************************************************************************
 |    BBTagInfo.cc
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

#include <boost/range/iterator_range.hpp>

#include "BBTagInfo.h"

using namespace std;
using namespace boost::archive;

#include "bberror.h"
#include "bbinternal.h"
#include "BBLV_Info.h"
#include "bbserver_flightlog.h"
#include "bbwrkqmgr.h"
#include "HandleFile.h"
#include "logging.h"
#include "LVUuidFile.h"

//
// BBTagInfo class
//

//
// BBTagInfo - static methods
//

int BBTagInfo::compareContrib(const uint64_t pNumContrib, const uint32_t pContrib[], vector<uint32_t>& pContribVector) {
    int rc = 0;

    uint64_t l_NumExpectContrib = (uint64_t)pContribVector.size();
    if (pNumContrib == l_NumExpectContrib) {
        for(uint64_t i=0; i<l_NumExpectContrib; ++i) {
            if (pContrib[i] != pContribVector[i]) {
                rc = 1;
                break;
            }
        }
    } else {
        rc = 1;
    }

    return rc;
}

void BBTagInfo::genTransferHandle(uint64_t& pHandle, const BBJob pJob, const uint64_t pTag, vector<uint32_t>& pContrib) {
    // NOTE:  It is the invoker's responsibility to check for any collisions
    //        of the returned handle value with existing handles...

    typedef struct
    {
        uint64_t jobid;
        uint64_t jobstepid;
        uint64_t tag;
        uint32_t expectContrib[];
    } crc_data;

    typedef union
    {
        crc_data data;
        unsigned char str[];
    } crc_t;

    uint64_t l_SizeOfCRC = sizeof(((crc_data*)0)->jobid) + sizeof(((crc_data*)0)->jobstepid) + sizeof(((crc_data*)0)->tag) + (pContrib.size()*sizeof(uint32_t));
    crc_t* l_CRC = (crc_t*)(new char[l_SizeOfCRC]);
    memset(&(l_CRC->str[0]), 0, l_SizeOfCRC);
    l_CRC->data.jobid = pJob.getJobId();
    l_CRC->data.jobstepid = pJob.getJobStepId();
    l_CRC->data.tag = pTag;
    for (size_t i=0; i<pContrib.size(); ++i)
    {
        l_CRC->data.expectContrib[i] = pContrib[i];
    }

#if 0
    // Log the input string...
    char l_Temp[(sizeof(l_CRC->str)*2)+1] = {'\0'};
    for (size_t i = 0; i < sizeof(l_CRC.str); ++i) {
        snprintf(&(l_Temp[i*2]), 3, "%02X", l_CRC->str[i]);
    }
    LOG(bb,debug) << "crc.str=0x" << l_Temp;
#endif

    unsigned long crcValue = 0;
    crcValue = Crc32n(crcValue, l_CRC->str, l_SizeOfCRC);

#if 0
     // Log the output string...
    LOG(bb,debug) << "crcValue=0x" << hex << uppercase << crcValue << nouppercase << dec << " (" << crcValue << ")";
#endif

    delete[] l_CRC;
    l_CRC = 0;

    pHandle = (uint64_t)crcValue;

    return;
}


//
// BBTagInfo - non-static methods
//

BBTagInfo::BBTagInfo(BBTagInfoMap* pTagInfo, const uint64_t pNumContrib, const uint32_t pContrib[], const BBJob pJob, const uint64_t pTag, int& pGeneratedHandle) :
    flags(0) {
    for (size_t i=0; i<(size_t)pNumContrib; ++i) {
        expectContrib.push_back(pContrib[i]);
    }
    uint64_t l_Handle = 0;
    getTransferHandle(l_Handle, pTagInfo, pJob, pTag, pGeneratedHandle);
    setTransferHandle(l_Handle);

    return;
}

void BBTagInfo::accumulateTotalLocalContributorInfo(const uint64_t pHandle, size_t& pTotalContributors, size_t& pTotalLocalReportingContributors)
{
    if (pHandle == transferHandle)
    {
        pTotalContributors = getTotalContributors();
        pTotalLocalReportingContributors += getTotalLocalReportingContributors();
    }

    return;
}

int BBTagInfo::addTransferDef(const std::string& pConnectionName, const LVKey* pLVKey, const BBJob pJob, BBLV_Info* pLV_Info, const BBTagID pTagId, const uint32_t pContribId, const uint64_t pHandle, BBTransferDef* &pTransferDef) {
    int rc = 0;
    stringstream errorText;

    HandleFile* l_HandleFile = 0;
    char* l_HandleFileName = 0;

    // NOTE: The handle file is locked exclusive here to serialize amongst multiple bbServers
    //       adding transfer definitions...
    rc = HandleFile::loadHandleFile(l_HandleFile, l_HandleFileName, pTagId.getJobId(), pTagId.getJobStepId(), pHandle, LOCK_HANDLEFILE);
    if (!rc)
    {
        rc = update_xbbServerAddData(pLVKey, pJob, pLV_Info, pContribId, pHandle, pTransferDef);
        if (rc >= 0)
        {
            // NOTE:  rc=0 means that the contribid was added to the ContribFile.
            //        rc=1 means that the contribid already existed in the ContribFile.
            //             This is normal for the restart of a transfer definition.
            if (rc == 1)
            {
                if (pTransferDef->builtViaRetrieveTransferDefinition())
                {
                    rc = 0;
                }
                else
                {
                    rc = -1;
                    errorText << "BBTagInfo::addTransferDef: For " << *pLVKey << ", handle " << pHandle << ", contribid " << pContribId \
                              << " was already known to the cross-bbServer metadata.";
                    LOG_ERROR_TEXT_RC(errorText, rc);
                }
            }

            if (!rc)
            {
                rc = parts.addTransferDef(pLVKey, pHandle, pContribId, pTransferDef);
                if (!rc) {
                    // NOTE:  The following checks are required here in case the transfer definition just added
                    //        had no files but completed the criteria being checked...
                    if ((expectContrib.size() == getNumberOfTransferDefs()) && (!parts.anyStoppedTransferDefinitions())) {
                        setAllContribsReported(pLVKey, pJob.getJobId(), pJob.getJobStepId(), pHandle);
                    }

                    if (pTransferDef->allExtentsTransferred()) {
                        pLV_Info->sendTransferCompleteForContribIdMsg(pConnectionName, pLVKey, pHandle, pContribId, pTransferDef);

                        int l_NewStatus = 0;
                        Extent l_Extent = Extent();
                        ExtentInfo l_ExtentInfo = ExtentInfo(pHandle, pContribId, &l_Extent, pTransferDef);
                        pLV_Info->updateTransferStatus(pLVKey, l_ExtentInfo, pTagId, pContribId, l_NewStatus, 0);
                        if (l_NewStatus) {
                            // Status changed for transfer handle...
                            // Send the transfer is complete for this handle message to bbProxy
                            string l_HostName;
                            activecontroller->gethostname(l_HostName);
                            metadata.sendTransferCompleteForHandleMsg(l_HostName, pTransferDef->getHostName(), pHandle);

                            // Check/update the status for the LVKey
                            // NOTE:  If the status changes at the LVKey level, the updateTransferStatus() routine will send the message for the LVKey...
                            pLV_Info->updateTransferStatus(pConnectionName, pLVKey, 0);
                        }
                    }
                } else {
                    rc = -1;
                    errorText << "BBTagInfo::addTransferDef: Failure from addTransferDef(), rc = " << rc;
                    LOG_ERROR_TEXT_RC(errorText, rc);
                }
            }
        } else {
            errorText << "BBTagInfo::addTransferDef: Failure from update_xbbServerAddData(), rc = " << rc;
            LOG_ERROR(errorText);
        }
    }
    else
    {
        rc = -1;
        errorText << "BBTagInfo::addTransferDef: Handle file could not be loaded for " << *pLVKey << ", handle " << pHandle << ", contribid " << pContribId;
        LOG_ERROR(errorText);
    }

    if (l_HandleFileName)
    {
        delete[] l_HandleFileName;
        l_HandleFileName = 0;
    }
    if (l_HandleFile)
    {
        l_HandleFile->close();
        delete l_HandleFile;
        l_HandleFile = 0;
    }

    return rc;
}

void BBTagInfo::bumpTransferHandle(uint64_t& pHandle) {
    union {
        struct {
            uint8_t reserved1;
            uint8_t reserved2;
            uint8_t reserved3;
            uint8_t collision;
            uint32_t crc;
        } internal;
        uint64_t external;
    } handle;

    handle.external = pHandle;
    ++handle.internal.collision;
    pHandle = handle.external;

    return;
}

void BBTagInfo::dump(const char* pSev) {
    stringstream l_Temp;
    expectContribToSS(l_Temp);
    if (!strcmp(pSev,"debug")) {
        LOG(bb,debug) << hex << uppercase << setfill('0') << "Transfer Handle: 0x" << setw(16) << transferHandle << setfill(' ') << nouppercase << dec << " (" << transferHandle << ")";
        LOG(bb,debug) << "Expect Contrib:  " << l_Temp.str();
        parts.dump(pSev);
    } else if (!strcmp(pSev,"info")) {
        LOG(bb,info) << hex << uppercase << setfill('0') << "Transfer Handle: 0x" << setw(16) << transferHandle << setfill(' ') << nouppercase << dec << " (" << transferHandle << ")";
        LOG(bb,info) << "Expect Contrib:  " << l_Temp.str();
        parts.dump(pSev);
    }

    return;
}

void BBTagInfo::expectContribToSS(stringstream& pSS) const {
    pSS << "(";
    if (expectContrib.size()) {
        uint64_t j = expectContrib.size();
        for(uint64_t i=0; i<j; ++i) {
            if (i!=j-1) {
                pSS << expectContrib[i] << ",";
            } else {
                pSS << expectContrib[i];
            }
        }
    }
    pSS << ")";

    return;
}

uint64_t BBTagInfo::get_xbbServerHandle(const BBJob& pJob, const uint64_t pTag)
{
    uint64_t l_Handle = 0;
    uint32_t* l_ContribArray = 0;
    HandleFile* l_HandleFile = 0;
    uint64_t l_NumOfContribsInArray = 0;

    bfs::path jobstep(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
    jobstep /= bfs::path(to_string(pJob.getJobId()));
    jobstep /= bfs::path(to_string(pJob.getJobStepId()));

    if(!bfs::exists(jobstep)) return 0;

    for(auto& handle : boost::make_iterator_range(bfs::directory_iterator(jobstep), {}))
    {
        bfs::path handlefile = handle.path() / bfs::path(handle.path().filename().string());
        int rc = HandleFile::loadHandleFile(l_HandleFile, handlefile.string().c_str());
        if (!rc)
        {
            if (l_HandleFile->tag == pTag)
            {
                // Tags match...  Now, compare the list of contribs...
                l_HandleFile->getContribArray(l_NumOfContribsInArray, l_ContribArray);

                if (!compareContrib(l_NumOfContribsInArray, l_ContribArray))
                {
                    l_Handle = stoul(handle.path().filename().string());
                }
                else
                {
                    LOG(bb,debug) << "BBTagInfo::get_xbbServerHandle(): For job (" << pJob.getJobId() << "," << pJob.getJobStepId() << "), exsting handle " << handlefile.string() << ", contributor vectors do not match";
                }
                delete[] l_ContribArray;
                l_ContribArray = 0;
            }
            else
            {
                LOG(bb,debug) << "BBTagInfo::get_xbbServerHandle(): For job (" << pJob.getJobId() << "," << pJob.getJobStepId() << "), exsting handle " << handlefile.string() << ", tags do not match. Input tag is " << pTag << ", existing handle tag is " << l_HandleFile->tag;
            }

            if (l_Handle) break;
        }
        else
        {
            LOG(bb,debug) << "BBTagInfo::get_xbbServerHandle(): For job (" << pJob.getJobId() << "," << pJob.getJobStepId() << "), existing handle file " << handlefile.string() << " could not be loaded, rc=" << rc;
        }

        if (l_HandleFile)
        {
            delete l_HandleFile;
            l_HandleFile = 0;
        }
    }

    return l_Handle;
}

BBSTATUS BBTagInfo::getStatus(const int pStageOutStarted) {
    BBSTATUS l_Status = BBNONE;

    if (!stopped()) {
        if (!failed()) {
            if (!canceled()) {
                if (allContribsReported()) {
                    if (!allExtentsTransferred()) {
                        l_Status = BBINPROGRESS;
                    } else {
                        l_Status = BBFULLSUCCESS;
                    }
                } else {
                    if (!parts.getNumberOfParts()) {
                        l_Status = BBNOTSTARTED;
                    } else {
                        if (!allExtentsTransferred()) {
                            l_Status = BBINPROGRESS;
                        } else {
                            l_Status = BBPARTIALSUCCESS;
                        }
                    }
                }
            } else {
                l_Status = BBCANCELED;
            }
        } else {
            l_Status = BBFAILED;
        }
    } else {
        l_Status = BBSTOPPED;
    }

    return l_Status;
}

void BBTagInfo::getTransferHandle(uint64_t& pHandle, BBTagInfoMap* pTagInfo, const BBJob pJob, const uint64_t pTag, int& pGeneratedHandle) {
    pGeneratedHandle = 0;
    pHandle = get_xbbServerHandle(pJob, pTag);
    if (!pHandle) {
        pGeneratedHandle = 1;
        genTransferHandle(pHandle, pJob, pTag);
#if 0  // \todo need real fix to timing window with concurrent handle generation. Disabling the following for now
        while (pHandle == 0 || (!(xbbServerIsHandleUnique(pJob, pHandle)))) {
            bumpTransferHandle(pHandle);
        }
#endif
    }

    return;
}

int BBTagInfo::inExpectContrib(const uint32_t pContribId) {
    int rc = 0;

    for (size_t i=0; i<expectContrib.size(); ++i) {
        if (pContribId == expectContrib[i]) {
            rc = 1;
            break;
        }
    }

    return rc;
}

int BBTagInfo::prepareForRestart(const std::string& pConnectionName, const LVKey* pLVKey, const BBJob pJob, const uint64_t pHandle, const int32_t pContribId, BBTransferDef* l_OrigTransferDef, BBTransferDef* pRebuiltTransferDef, const int pPass)
{
    int rc = 0;

    LOG(bb,debug) << "BBTagInfo::prepareForRestart(): Pass " << pPass;

    if (pPass == FIRST_PASS)
    {
        int l_Attempts = 2;
        bool l_AllDone = false;
        while (!l_AllDone)
        {
            l_AllDone = true;

            // Make sure that the HandleFile indicates that the handle has at least
            // one transfer definition that is stopped
            rc = 1;
            uint64_t l_Continue = wrkqmgr.getDeclareServerDeadCount();
            while ((rc == 1) && (l_Continue--))
            {
                BBSTATUS l_Status;
                rc = HandleFile::get_xbbServerHandleStatus(l_Status, pLVKey, pJob.getJobId(), pJob.getJobStepId(), pHandle);
                if (!rc)
                {
                    if (l_Status != BBSTOPPED)
                    {
                        rc = 1;
                    }
                    else
                    {
                        // Stopped, rc is already 0, will exit both loops
                    }
                }
                else
                {
                    // Indicate to not restart this transfer definition
                    rc = 1;
                    l_Continue = 0;
                    LOG(bb,info) << "Error when getting the handle status for " << *pLVKey << ", jobid " << pJob.getJobId() << ", jobstepid " << pJob.getJobStepId() \
                                 << ", handle " << pHandle << ". Cannot restart any transfer definitions under this handle.";
                }

                if (rc && l_Continue)
                {
                    if (((wrkqmgr.getDeclareServerDeadCount() - l_Continue) % 15) == 1)
                    {
                        // Display this message every 15 seconds...
                        FL_Write6(FLDelay, PrepareForRestart, "Attempting to restart a transfer definition for jobid %ld, jobstepid %ld, handle %ld, contribid %ld. Delay of 1 second before retry. %ld seconds remain waiting for the original bbServer to act before an unconditional stop is performed.",
                                  (uint64_t)pJob.getJobId(), (uint64_t)pJob.getJobStepId(), (uint64_t)pHandle, (uint64_t)pContribId, (uint64_t)l_Continue, 0);
                        LOG(bb,info) << ">>>>> DELAY <<<<< BBTagInfo::prepareForRestart: Attempting to restart a transfer definition for jobid " << pJob.getJobId() \
                                     << ", jobstepid " << pJob.getJobStepId() << ", handle " << pHandle << ", contribid " << pContribId \
                                     << ". Waiting for the handle to be marked as stopped. Delay of 1 second before retry. " << l_Continue \
                                     << " seconds remain waiting for the original bbServer to act before an unconditional stop is performed.";
                    }
                    unlockTransferQueue(pLVKey, "BBTagInfo::prepareForRestart - Waiting for transfer definition to be marked as stopped");
                    {
                        usleep((useconds_t)1000000);    // Delay 1 second
                    }
                    lockTransferQueue(pLVKey, "BBTagInfo::prepareForRestart - Waiting for transfer definition to be marked as stopped");

                    // Check to make sure the job still exists after releasing/re-acquiring the lock
                    if (!jobStillExists(pConnectionName, pLVKey, (BBLV_Info*)0, this, pJob.getJobId(), pContribId))
                    {
                        rc = -1;
                        l_Continue = 0;
                    }
                }
            }

            if (rc > 0)
            {
                if (--l_Attempts)
                {
                    rc = prepareForRestartOriginalServerDead(pConnectionName, pLVKey, pHandle, pJob, pContribId);
                    switch (rc)
                    {
                        case 1:
                        {
                            // Reset of cross bbServer metadata was successful...  Continue...
                            rc = 0;
                            LOG(bb,info) << "ContribId " << pContribId << " was found in the cross bbServer metadata and was successfully stopped" \
                                         << " after the original bbServer was unresponsive";
                            l_AllDone = false;
                        }
                        break;

                        case 2:
                        {
                            // Indicate to not restart this transfer definition
                            rc = 1;
                            LOG(bb,info) << "ContribId " << pContribId << " was found in the cross bbServer metadata, but no file associated with the transfer definition needed to be restarted." \
                                         << " Most likely, the transfer completed for the contributor or was canceled. Therefore, the transfer definition cannot be restarted. See any previous messages.";
                        }
                        break;

                        default:
                        {
                            // Indicate to not restart this transfer definition
                            rc = 1;
                            LOG(bb,error) << "Attempt to reset the cross bbServer metadata for the transfer definition associated with contribid " << pContribId << " to stopped failed." \
                                          << " Therefore, the transfer definition cannot be restarted. See any previous messages.";
                        }
                        break;
                    }
                }
                else
                {
                    // Indicate to not restart this transfer definition
                    rc = 1;
                    LOG(bb,error) << *pLVKey << ", jobid " << pJob.getJobId() << ", jobstepid " << pJob.getJobStepId() \
                                  << ", handle " << pHandle << " is not is a stopped state. Cannot restart any transfer definitions under this handle.";
                }
            }
        }
    }

    if (!rc)
    {
        // Prepare the new transfer definition for restart...
        if (l_OrigTransferDef)
        {
            rc = l_OrigTransferDef->prepareForRestart(pLVKey, pJob, pHandle, pContribId, pRebuiltTransferDef, pPass);
        }

        if (!rc)
        {
            if (pPass == THIRD_PASS)
            {
                // Next, reset the flags for the handle and HandleFile...
                setAllExtentsTransferred(pLVKey, pJob.getJobId(), pJob.getJobStepId(), pHandle, 0);
                setCanceled(pLVKey, pJob.getJobId(), pJob.getJobStepId(), pHandle, 0);
                setFailed(pLVKey, pJob.getJobId(), pJob.getJobStepId(), pHandle, 0);
                setStopped(pLVKey, pJob.getJobId(), pJob.getJobStepId(), pHandle, 0);
                HandleFile::update_xbbServerHandleResetStatus(pLVKey, pJob.getJobId(), pJob.getJobStepId(), pHandle);

                // Update the handle status to recalculate the new status
                HandleFile::update_xbbServerHandleStatus(pLVKey, pJob.getJobId(), pJob.getJobStepId(), pHandle, 0);
            }
        }
    }

    return rc;
}

int BBTagInfo::retrieveTransfers(BBTransferDefs& pTransferDefs, BBLV_ExtentInfo* pExtentInfo) {
    int rc = 0;

    if (pTransferDefs.getHandle() == UNDEFINED_HANDLE || pTransferDefs.getHandle() == transferHandle)
    {
        rc = parts.retrieveTransfers(pTransferDefs, pExtentInfo);
    }

    return rc;
}

void BBTagInfo::sendTransferCompleteForHandleMsg(const string& pHostName, const string& pCN_HostName, const string& pConnectionName, const LVKey* pLVKey, BBLV_Info* pLV_Info, const BBTagID pTagId, const uint64_t pHandle, int& pAppendAsyncRequestFlag, const BBSTATUS pStatus)
{
    if (pHandle == transferHandle)
    {
        pLV_Info->sendTransferCompleteForHandleMsg(pHostName, pCN_HostName, pConnectionName, pLVKey, pTagId, pHandle, pAppendAsyncRequestFlag, pStatus);
    }

    return;
}

void BBTagInfo::setAllContribsReported(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const int pValue)
{
    if (pHandle == transferHandle)
    {
        if (pValue)
        {
            LOG(bb,info) << "All contributors reported for " << *pLVKey << ", jobid " << pJobId << ", jobstepid " << pJobStepId << ", transfer handle " << pHandle;
        }

        if ((((flags & BBTI_All_Contribs_Reported) == 0) && pValue) || ((flags & BBTI_All_Contribs_Reported) && (!pValue)))
        {
            LOG(bb,debug) << "BBTagInfo::setAllContribsReported(): Jobid " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle \
                          << " -> Changing from: " << ((flags & BBTI_All_Contribs_Reported) ? "true" : "false") << " to " << (pValue ? "true" : "false");
        }
        SET_FLAG(BBTI_All_Contribs_Reported, pValue);

        // Now update the status for the Handle file in the xbbServer data...
        if (HandleFile::update_xbbServerHandleStatus(pLVKey, pJobId, pJobStepId, pHandle, 0))
        {
            LOG(bb,error) << "BBTagInfo::setAllContribsReported():  Failure when attempting to update the cross bbServer handle file statusfor jobid " << pJobId \
                          << ", jobstepid " << pJobStepId << ", handle " << pHandle;
        }
    }

    return;
}

void BBTagInfo::setAllExtentsTransferred(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const int pValue)
{
    if (pHandle == transferHandle)
    {
        if (pValue && (!allExtentsTransferred()))
        {
            LOG(bb,debug) << "Processing complete for " << *pLVKey << ", transfer handle 0x" << hex << uppercase << setfill('0') \
                          << setw(16) << transferHandle << setfill(' ') << nouppercase << dec << " (" << transferHandle << ")";
        }

        if ((((flags & BBTD_All_Extents_Transferred) == 0) && pValue) || ((flags & BBTD_All_Extents_Transferred) && (!pValue)))
        {
            LOG(bb,debug) << "BBTagInfo::setAllExtentsTransferred(): Jobid " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle \
                          << " -> Changing from: " << ((flags & BBTD_All_Extents_Transferred) ? "true" : "false") << " to " << (pValue ? "true" : "false");
        }
        SET_FLAG(BBTD_All_Extents_Transferred, pValue);

        // Now update the status for the Handle file in the xbbServer data...
        if (HandleFile::update_xbbServerHandleStatus(pLVKey, pJobId, pJobStepId, pHandle, 0))
        {
            LOG(bb,error) << "BBTagInfo::setAllExtentsTransferred():  Failure when attempting to update the cross bbServer handle file status for jobid " << pJobId \
                          << ", jobstepid " << pJobStepId << ", handle " << pHandle;
        }
    }

    return;
}

void BBTagInfo::setCanceled(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const int pValue)
{
    if (pHandle == transferHandle)
    {
        if (pValue && (!stopped()) && (!canceled()))
        {
            LOG(bb,info) << "All extents canceled for " << *pLVKey << ", transfer handle 0x" << hex << uppercase << setfill('0') \
                         << setw(16) << transferHandle << setfill(' ') << nouppercase << dec << " (" << transferHandle << ")";
        }

        if ((((flags & BBTD_Canceled) == 0) && pValue) || ((flags & BBTD_Canceled) && (!pValue)))
        {
            LOG(bb,debug) << "BBTagInfo::setCanceled(): Jobid " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle \
                          << " -> Changing from: " << ((flags & BBTD_Canceled) ? "true" : "false") << " to " << (pValue ? "true" : "false");
        }

        if (pValue && stopped() && canceled())
        {
            SET_FLAG(BBTD_Stopped, 0);
            LOG(bb,info) << "BBTagInfo::setCanceled(): Jobid " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle \
                         << " was previously stopped. It will now be canceled and all underlying transfer definitions will no longer be restartable.";
        }
        SET_FLAG(BBTD_Canceled, pValue);

        // Now update the status for the Handle file in the xbbServer data...
        if (HandleFile::update_xbbServerHandleFile(pLVKey, pJobId, pJobStepId, pHandle, BBTD_Canceled, pValue))
        {
            LOG(bb,error) << "BBTagInfo::setCanceled():  Failure when attempting to update the cross bbServer handle file for jobid " << pJobId \
                          << ", jobstepid " << pJobStepId << ", handle " << pHandle;
        }
    }

    return;
}

// NOTE: No code sets the handle status to BBFAILED today...  @DLH
void BBTagInfo::setFailed(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const int pValue)
{
    if (pHandle == transferHandle)
    {
        if ((((flags & BBTD_Failed) == 0) && pValue) || ((flags & BBTD_Failed) && (!pValue)))
        {
            LOG(bb,debug) << "BBTagInfo::setFailed(): Jobid " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle \
                          << " -> Changing from: " << ((flags & BBTD_Failed) ? "true" : "false") << " to " << (pValue ? "true" : "false");
        }
        SET_FLAG(BBTD_Failed, pValue);

        // Now update the status for the Handle file in the xbbServer data...
        if (HandleFile::update_xbbServerHandleFile(pLVKey, pJobId, pJobStepId, pHandle, BBTD_Failed, pValue))
        {
            LOG(bb,error) << "BBTagInfo::setFailed():  Failure when attempting to update the cross bbServer handle file for jobid " << pJobId \
                          << ", jobstepid " << pJobStepId << ", handle " << pHandle;
        }
    }

    return;
}

void BBTagInfo::setStopped(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const int pValue)
{
    if (pHandle == transferHandle)
    {
        if (pValue)
        {
            LOG(bb,debug) << "All extents stopped for " << *pLVKey << ", transfer handle 0x" << hex << uppercase << setfill('0') \
                          << setw(16) << transferHandle << setfill(' ') << nouppercase << dec << " (" << transferHandle << ")";
        }

        if ((((flags & BBTD_Stopped) == 0) && pValue) || ((flags & BBTD_Stopped) && (!pValue)))
        {
            LOG(bb,debug) << "BBTagInfo::setStopped(): Jobid " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle \
                          << " -> Changing from: " << ((flags & BBTD_Stopped) ? "true" : "false") << " to " << (pValue ? "true" : "false");
        }
        SET_FLAG(BBTD_Stopped, pValue);

        // Now update the status for the Handle file in the xbbServer data...
        if (HandleFile::update_xbbServerHandleStatus(pLVKey, pJobId, pJobStepId, pHandle, 0))
        {
            LOG(bb,error) << "BBTagInfo::setStopped():  Failure when attempting to update the cross bbServer handle file status for jobid " << pJobId \
                          << ", jobstepid " << pJobStepId << ", handle " << pHandle;
        }
    }

    return;
}

int BBTagInfo::stopTransfer(const LVKey* pLVKey, BBLV_Info* pLV_Info, const string& pHostName, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId)
{
    int rc = 0;


    if (pHandle == transferHandle)
    {
        rc = parts.stopTransfer(pLVKey, pHostName, pJobId, pJobStepId, pHandle, pContribId);
        if (rc == 1)
        {
            int l_Value = 1;
            // Set the stopped indicator in the local metadata...
            setStopped(pLVKey, pJobId, pJobStepId, pHandle, l_Value);
        }
    }

    return rc;
}

int BBTagInfo::update_xbbServerAddData(const LVKey* pLVKey, const BBJob pJob, BBLV_Info* pLV_Info, const uint32_t pContribId, const uint64_t pHandle, BBTransferDef* &pTransferDef)
{
    int rc = 0;
    stringstream errorText;

    Uuid lv_uuid = pLVKey->second;
    char lv_uuid_str[LENGTH_UUID_STR] = {'\0'};
    lv_uuid.copyTo(lv_uuid_str);

    ContribFile* l_ContribFile = 0;
    ContribIdFile* l_ExistingContribFile = 0;
    ContribIdFile* l_NewContribIdFile = 0;

    try
    {
        // NOTE: If this is a restart for a transfer definition, we verify that the ContribIdFile
        //       already exists.  In that case rc=1 will be returned...
        bfs::path handle(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
        handle /= bfs::path(to_string(pJob.getJobId()));
        handle /= bfs::path(to_string(pJob.getJobStepId()));
        handle /= bfs::path(to_string(pHandle));
        if (bfs::exists(handle))
        {
            for (auto& lvuuid: boost::make_iterator_range(bfs::directory_iterator(handle), {}))
            {
                if ((l_ExistingContribFile) || (!bfs::is_directory(lvuuid))) continue;
                bfs::path l_ContribFilePath = lvuuid.path() / "contribs";
                rc = ContribFile::loadContribFile(l_ContribFile, l_ContribFilePath);
                if (!rc)
                {
                    for (map<uint32_t,ContribIdFile>::iterator ce = l_ContribFile->contribs.begin(); ce != l_ContribFile->contribs.end(); ce++)
                    {
                        if (ce->first == pContribId)
                        {
                            l_ExistingContribFile = new ContribIdFile(ce->second);
                            rc = 1;
                            break;
                        }
                    }
                }

                if (l_ExistingContribFile)
                {
                    if (string(lv_uuid_str) == lvuuid.path().filename().string())
                    {
                        LOG(bb,info) << "xbbServer: Logical volume with a uuid of " << lv_uuid_str << " is already registered and currently has " << l_ContribFile->numberOfContribs() << " non-stopped contributor(s)";
                    }
                    else
                    {
                        // Even for restart, the lvuuid for the already registered contribid must match the lvuuid for the transfer definition being added
                        rc = -1;
                        errorText << "Contribid " << pContribId << " is already registered under lvuuid " << lvuuid.path().filename().string() << " for job " << pJob.getJobId() << ", jobstepid " << pJob.getJobStepId() << ", handle " << pHandle;
                        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                    }
                }

                if (l_ContribFile)
                {
                    delete l_ContribFile;
                    l_ContribFile = 0;
                }
            }

            if (!l_ExistingContribFile)
            {
                // Validate any data prior to creating the directories in the cross bbServer metadata
                if (pJob.getJobId() == UNDEFINED_JOBID)
                {
                    rc = -1;
                    errorText << "BBTagInfo::update_xbbServerAddData(): Attempt to add invalid jobid of " << UNDEFINED_JOBID << " to the cross bbServer metadata";
                    LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                }
                else if (pJob.getJobStepId() == UNDEFINED_JOBSTEPID)
                {
                    rc = -1;
                    errorText << "BBTagInfo::update_xbbServerAddData(): Attempt to add invalid jobstepid of " << UNDEFINED_JOBSTEPID << " to the cross bbServer metadata";
                    LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                }

                LOG(bb,info) << "xbbServer: Logical volume with a uuid of " << lv_uuid_str << " is not already registered.  It will be added.";
                bfs::path l_LVUuidPath = handle / bfs::path(lv_uuid_str);
                bfs::create_directories(l_LVUuidPath);

                // Unconditionally perform a chmod to 0770 for the lvuuid directory.
                // NOTE:  This is done for completeness, as all access is via the great-grandparent directory (jobid) and access to the files
                //        contained in this tree is controlled there.
                rc = chmod(l_LVUuidPath.c_str(), 0770);
                if (rc)
                {
                    stringstream errorText;
                    errorText << "chmod failed";
                    bberror << err("error.path", l_LVUuidPath.string());
                    LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
                }

                LVUuidFile l_LVUuidFile((*pLVKey).first, pLV_Info->getHostName());
                bfs::path l_LVUuidFilePathName = l_LVUuidPath / bfs::path(lv_uuid_str);
                rc = l_LVUuidFile.save(l_LVUuidFilePathName.string());
                if (rc) BAIL;

                ContribFile l_ContribFile;
                bfs::path l_ContribFilePath = l_LVUuidPath / "contribs";
                rc = l_ContribFile.save(l_ContribFilePath.string());
                if (rc) BAIL;
            }
        }
        else
        {
            rc = -1;
            errorText << "Handle file directory " << handle.string() << " does not exist for " << pJob.getJobId() << ", jobstepid " << pJob.getJobStepId() << ", handle " << pHandle;
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Create a new ContribIdFile for this contributor
        ContribIdFile l_NewContribIdFileStg = ContribIdFile(pTransferDef);

        if (!pTransferDef->builtViaRetrieveTransferDefinition())
        {
            // Use the newly created ContribIdFile
            l_NewContribIdFile = &l_NewContribIdFileStg;

            ContribIdFile* l_ContribIdFilePtr = 0;
            rc = ContribIdFile::loadContribIdFile(l_ContribIdFilePtr, pLVKey, handle, pContribId);
            switch (rc)
            {
                case 0:
                    break;

                case 1:
                {
                    if (l_ContribIdFilePtr->extentsAreEnqueued())
                    {
                        // Extents have already been enqueued for this contributor...
                        rc = -1;
                        errorText << "ContribId " << pContribId << " is already registered for " << *pLVKey << ", using handle path " << handle.string() << " and has already had extents enqueued for transfer.  This transfer definition cannot be started.";
                        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                    }
                    else
                    {
                        // Extents have not been enqueued yet...
                        // NOTE:  Allow this to continue...  This is probably the case where a start transfer got far enough along
                        //        on bbServer to create all of the metadata (first volley message), but the second volley either failed
                        //        or bbProxy failed before/during the send of the second volley message.
                        // NOTE:  Start transfer processing DOES NOT backout any metadata changes made for a partially completed
                        //        operation.
                        LOG(bb,info) << "ContribId " << pContribId << " already exists in contrib file for " << *pLVKey << ", using handle path " << handle.string() \
                                     << ", but extents have never been enqueued for the transfer definition. ContribIdFile for " << pContribId << " will be reused.";
                        l_NewContribIdFile = l_ExistingContribFile;
                        rc = 0;
                    }

                    break;
                }

                default:
                {
                    errorText << "Failure when attempting to load the contrib file for " << *pLVKey << ", contribid " << pContribId << ", using handle path " << handle.string();
                    LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                }
            }

            if (l_ContribIdFilePtr)
            {
                delete l_ContribIdFilePtr;
                l_ContribIdFilePtr = 0;
            }
        }
        else
        {
            // For a restart, the ContribIdFile already exists...
            // Use the already existing ContribIdFile...
            if (l_ExistingContribFile)
            {
                l_NewContribIdFile = l_ExistingContribFile;
            }
            else
            {
                rc = -1;
                errorText << "BBTagInfo::update_xbbServerAddData(): For a restart transfer definition operation, could not find the ContribIdFile for " \
                          << *pLVKey << ", contribid " << pContribId << ", using handle path " << handle.string();
                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
            }
        }

        if (rc < 0) bberror << bailout;

        if (!pTransferDef->hasFilesInRequest())
        {
            // No files in the request
            uint64_t l_OriginalFileFlags = l_NewContribIdFile->flags;
            SET_FLAG_VAR(l_NewContribIdFile->flags, l_NewContribIdFile->flags, BBTD_Extents_Enqueued, 1);
            SET_FLAG_VAR(l_NewContribIdFile->flags, l_NewContribIdFile->flags, BBTD_All_Extents_Transferred, 1);
            SET_FLAG_VAR(l_NewContribIdFile->flags, l_NewContribIdFile->flags, BBTD_All_Files_Closed, 1);
            if (l_OriginalFileFlags != l_NewContribIdFile->flags)
            {
                LOG(bb,info) << "xbbServer: For " << *pLVKey << ", handle " << pHandle << ", contribid " << pContribId << ":";
                LOG(bb,info) << "           ContribId flags changing from 0x" << hex << uppercase << l_OriginalFileFlags << " to 0x" << l_NewContribIdFile->flags << nouppercase << dec << ".";
            }
        }

        int rc2 = ContribIdFile::saveContribIdFile(l_NewContribIdFile, pLVKey, handle, pContribId);

        if (rc2)
        {
            rc = rc2;
            bberror << bailout;
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    if (l_ExistingContribFile)
    {
        delete l_ExistingContribFile;
        l_ExistingContribFile = 0;
    }

    if (l_ContribFile)
    {
        delete l_ContribFile;
        l_ContribFile = 0;
    }

    return rc;
}

int BBTagInfo::xbbServerIsHandleUnique(const BBJob& pJob, const uint64_t pHandle)
{
    bfs::path handle(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
    handle /= bfs::path(to_string(pJob.getJobId()));
    handle /= bfs::path(to_string(pJob.getJobStepId()));
    handle /= bfs::path(to_string(pHandle));

    return (!bfs::exists(handle));
}
