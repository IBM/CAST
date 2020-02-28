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

#include <unistd.h>

#include <boost/range/iterator_range.hpp>
#include <boost/system/error_code.hpp>

#include "BBTagInfo.h"

#include "bberror.h"
#include "bbinternal.h"
#include "BBLV_Info.h"
#include "bbserver_flightlog.h"
#include "bbwrkqmgr.h"
#include "HandleFile.h"
#include "logging.h"
#include "LVUuidFile.h"
#include "TagInfo.h"

using namespace std;
using namespace boost::archive;
namespace bfs = boost::filesystem;
namespace bs = boost::system;

//
// BBTagInfo class
//

//
// BBTagInfo - static methods
//

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
        unsigned char str[sizeof(crc_data)];
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

int BBTagInfo::getTransferHandle(const LVKey* pLVKey, uint64_t& pHandle, BBTagInfo* &pTagInfo, const BBJob pJob, const uint64_t pTag, const uint64_t pNumContrib, const uint32_t pContrib[])
{
    int rc = 0;
    stringstream errorText;
    bool l_Continue = true;

    int l_TransferQueueWasUnlocked = 0;
    int l_LocalMetadataLocked = 0;
    int l_TagInfoLocked = 0;
    uint64_t l_Handle = UNDEFINED_HANDLE;

    // First ensure that the jobstepid directory exists for the xbbServer metadata
    rc = BBTagInfo::update_xbbServerAddData(pLVKey, pJob);

    if (!rc)
    {
        bfs::path l_JobStepPath(g_BBServer_Metadata_Path);
        l_JobStepPath /= bfs::path(to_string(pJob.getJobId()));
        l_JobStepPath /= bfs::path(to_string(pJob.getJobStepId()));
        vector<uint32_t> l_ExpectContrib;
        while (l_Continue)
        {
            l_Continue = false;
            uint32_t l_BumpCount = 0;
            if (l_Handle == UNDEFINED_HANDLE)
            {
                for (size_t i=0; i<(size_t)pNumContrib; ++i)
                {
                    l_ExpectContrib.push_back(pContrib[i]);
                }
                genTransferHandle(l_Handle, pJob, pTag, l_ExpectContrib);
            }

            try
            {
                // Serialize threads within this server to read the bump count file
                l_TransferQueueWasUnlocked = unlockTransferQueueIfNeeded(pLVKey, "BBTagInfo::getTransferHandle");
                l_LocalMetadataLocked = lockLocalMetadataIfNeeded(pLVKey, "BBTagInfo::getTransferHandle");
                // Perform the necessary locking across bbServers to read the bump count file
                rc = TagInfo::lock(l_JobStepPath);
                if (!rc)
                {
                    l_TagInfoLocked = 1;
                    rc = TagInfo::readBumpCountFile(l_JobStepPath.string(), l_BumpCount);
                    if (!rc)
                    {
                        l_TagInfoLocked = 0;
                        TagInfo::unlock();
                        if (l_LocalMetadataLocked)
                        {
                            l_LocalMetadataLocked = 0;
                            unlockLocalMetadata(pLVKey, "BBTagInfo::getTransferHandle");
                        }
                        if (l_TransferQueueWasUnlocked)
                        {
                            l_TransferQueueWasUnlocked = 0;
                            lockTransferQueue(pLVKey, "BBTagInfo::getTransferHandle");
                        }

                        // Process this proposed handle value for this job, jobstep, tag, and expectContrib vector.
                        int rc2 = processNewHandle(pLVKey, pJob, pTag, l_ExpectContrib, l_Handle, l_BumpCount);

                        // NOTE:  Upon return, the l_Handle value could have been modified by processNewHandle()
                        switch (rc2)
                        {
                            case 0:
                            {
                                // Newly generated handle value is currently unused for this job, jobstep, tag, and expectContrib vector.
                                // Use the newly generated handle value for this job, jobstep, tag, and expectContrib vector.
                            }
                            break;

                            case 1:
                            {
                                // Tag value for this expectContrib vector has already been assigned a handle for this job, jobstep.
                                // Use the returned handle value passed back in l_Handle.
                            }
                            break;

                            case 2:
                            {
                                // Newly generated handle value is already in use for this job and jobstepid, but for a different tag value.
                                // Assign a new handle value and try to process the new handle value again.
                                do
                                {
                                    bumpTransferHandle(l_Handle);
                                } while(l_Handle == 0);
                                if (!TagInfo::incrBumpCountFile(l_JobStepPath))
                                {
                                    l_Continue = true;
                                }
                                else
                                {
                                    // Error from incrBumpCount()
                                    rc = -1;
                                    errorText << "Unexpected error occurred during the generation of a handle value. Could not increment the bump count.";
                                    LOG_ERROR_TEXT_RC(errorText, rc);
                                }
                            }
                            break;

                            case -2:
                            {
                                // Tag value has already been used for this job and jobstep for a different expectContrib vector.  Error condition.
                                rc = -1;
                                errorText << "Tag value " << pTag << " has already been used for jobid " << pJob.getJobId() << ", jobstepid " << pJob.getJobStepId() \
                                          << " but for a different contrib vector. Another tag value must be specified.";
                                LOG_ERROR_TEXT_RC(errorText, rc);
                            }
                            break;

                            case -3:
                            {
                                // Bump count did not match...  Just start over...
                                l_Continue = true;
                            }
                            break;

                            default:
                            {
                                // Some other error
                                rc = -1;
                                errorText << "Unexpected error occurred during the generation of a handle value";
                                LOG_ERROR_TEXT_RC(errorText, rc);
                            }
                        }
                    }
                    else
                    {
                        // Error from reading the bump count file
                        rc = -1;
                        errorText << "Unexpected error occurred during the generation of a handle value.  Could not retrieve/read the bump count file/value at " << l_JobStepPath.string();
                        LOG_ERROR_TEXT_RC(errorText, rc);
                    }
                }
                else
                {
                    // Error locking taginfo file
                    rc = -1;
                    errorText << "Unexpected error occurred during the generation of a handle value.  Could not lock the TagInfo file at " << l_JobStepPath.string();
                    LOG_ERROR_TEXT_RC(errorText, rc);
                }
            }
            catch(ExceptionBailout& e) { }
            catch(exception& e)
            {
                rc = -1;
                LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
            }
        }

        if (rc >= 0)
        {
            pHandle = l_Handle;
            pTagInfo = new BBTagInfo(l_Handle, pNumContrib, pContrib);
        }
    }
    else
    {
        rc = -1;
        errorText << "Unexpected error occurred when attempting during the generation of a handle value.";
        LOG_ERROR_TEXT_RC(errorText, rc);
    }

    if (l_TagInfoLocked)
    {
        l_TagInfoLocked = 0;
        TagInfo::unlock();
    }
    if (l_LocalMetadataLocked)
    {
        l_LocalMetadataLocked = 0;
        unlockLocalMetadata(pLVKey, "BBTagInfo::getTransferHandle on exit");
    }
    if (l_TransferQueueWasUnlocked)
    {
        l_TransferQueueWasUnlocked = 0;
        lockTransferQueue(pLVKey, "BBTagInfo::getTransferHandle on exit");
    }

    return rc;
}

int BBTagInfo::processNewHandle(const LVKey* pLVKey, const BBJob pJob, const uint64_t pTag, vector<uint32_t>& pExpectContrib, uint64_t& l_Handle, const uint32_t pBumpCount)
{
    return TagInfo::addTagHandle(pLVKey, pJob, pTag, pExpectContrib, l_Handle, pBumpCount);
}

int BBTagInfo::update_xbbServerAddData(const LVKey* pLVKey, const BBJob pJob)
{
    int rc = 0;
    stringstream errorText;

    try
    {
        bfs::path jobstepid(g_BBServer_Metadata_Path);
        jobstepid = jobstepid / bfs::path(to_string(pJob.getJobId())) / bfs::path(to_string(pJob.getJobStepId()));

        if (access(jobstepid.c_str(), F_OK))
        {
            // Job step directory does not exist
            // NOTE:  There is a window between creating the job directory and
            //        performing the chmod to the correct uid:gid.  Therefore, if
            //        create_directories() returns EACCESS (permission denied), keep
            //        attempting for 2 minutes.
            bs::error_code l_ErrorCode;
            int l_Attempts = 120;
            while (l_Attempts-- > 0)
            {
                // Attempt to create the jobstepid directory
                bfs::create_directories(jobstepid, l_ErrorCode);
                if (l_ErrorCode.value() == EACCES)
                {
                    usleep((useconds_t)1000000);    // Delay 1 second
                }
                else
                {
                    // Jobstepid directory created
                    l_Attempts = -1;
                }
            }

            if (l_Attempts == 0)
            {
                // Error returned via create_directories...
                // Attempt one more time, without the error code.
                // On error, the appropriate boost exception will be thrown...
                LOG(bb,debug) << "BBTagInfoMap::update_xbbServerAddData(): l_Attempts " << l_Attempts << ", l_ErrorCode.value() " << l_ErrorCode.value();
                bfs::create_directories(jobstepid);
            }

            // Perform a chmod to 0770 for the jobstepid directory.

            // NOTE:  This is done for completeness, as all access is via the parent directory (jobid) and access to the files
            //        contained in this tree is controlled there.
            rc = chmod(jobstepid.c_str(), 0770);
            if (rc)
            {
                errorText << "chmod failed";
                bberror << err("error.path", jobstepid.c_str());
                LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, errno);
            }

            // Create the lock file for the taginfo
            rc = TagInfo::createLockFile(jobstepid.string());
            if (rc) BAIL;
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    return rc;
}


//
// BBTagInfo - non-static methods
//

BBTagInfo::BBTagInfo(uint64_t& pHandle, const uint64_t pNumContrib, const uint32_t pContrib[]) :
    flags(0),
    transferHandle(pHandle) {
    for (size_t i=0; i<(size_t)pNumContrib; ++i)
    {
        expectContrib.push_back(pContrib[i]);
    }
    parts = BBTagParts();

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

    unlockTransferQueue(pLVKey, "BBTagInfo::addTransferDef");
    int l_TransferQueueUnlocked = 1;

    lockLocalMetadata(pLVKey, "BBTagInfo::addTransferDef");
    int l_LocalMetadataLocked = 1;

    // NOTE: The handle file is locked exclusive here to serialize amongst multiple bbServers
    //       adding transfer definitions...
    HANDLEFILE_LOCK_FEEDBACK l_LockFeedback;
    rc = HandleFile::loadHandleFile(l_HandleFile, l_HandleFileName, pTagId.getJobId(), pTagId.getJobStepId(), pHandle, LOCK_HANDLEFILE, &l_LockFeedback);
    if (!rc)
    {
        rc = update_xbbServerAddData(pLVKey, l_HandleFile, l_HandleFileName, pJob, pLV_Info, pContribId, pHandle, pTransferDef);

        // Unlock the handle file
        // NOTE: In most cases, we are done updating the handle file after the above invocation of update_xbbServerAddData().
        //       In those cases where we have a transfer definition with no files, the handle file may then need to be updated.
        //       If required, the handle file lock is later re-acquired.
        // NOTE: Only if the return code was returned as zero can the handle file be locked by the invocation of loadHandleFile() above.
        l_HandleFile->close(l_LockFeedback);

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
                if (!rc)
                {
                    // NOTE:  The following checks are required here in case the transfer definition just added
                    //        had no files but completed the criteria being checked...
                    if ((expectContrib.size() == getNumberOfTransferDefs()) && (!parts.anyStoppedTransferDefinitions(this)))
                    {
                        setAllContribsReported(pLVKey, pJob.getJobId(), pJob.getJobStepId(), pHandle);
                    }
                    if (pTransferDef->allExtentsTransferred())
                    {
                        pLV_Info->sendTransferCompleteForContribIdMsg(pConnectionName, pLVKey, pHandle, pContribId, pTransferDef);

                        int l_NewStatus = 0;
                        Extent l_Extent = Extent();
                        ExtentInfo l_ExtentInfo = ExtentInfo(pHandle, pContribId, &l_Extent, this, pTransferDef);
                        pLV_Info->updateTransferStatus(pLVKey, l_ExtentInfo, pTagId, pContribId, l_NewStatus, 0);
                        if (l_NewStatus)
                        {
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
                }
                else
                {
                    rc = -1;
                    errorText << "BBTagInfo::addTransferDef: Failure from addTransferDef(), rc = " << rc;
                    LOG_ERROR_TEXT_RC(errorText, rc);
                }
            }
        }
        else
        {
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

    if (l_LocalMetadataLocked)
    {
        l_LocalMetadataLocked = 0;
        unlockLocalMetadata(pLVKey, "BBTagInfo::addTransferDef - Exit");
    }

    if (l_TransferQueueUnlocked)
    {
        l_TransferQueueUnlocked = 0;
        lockTransferQueue(pLVKey, "BBTagInfo::addTransferDef - Exit");
    }

    if (l_HandleFileName)
    {
        delete[] l_HandleFileName;
        l_HandleFileName = 0;
    }
    if (l_HandleFile)
    {
        delete l_HandleFile;
        l_HandleFile = 0;
    }

    return rc;
}

void BBTagInfo::calcCanceled(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle)
{
    int l_CanceledTransferDefinitions = parts.anyCanceledTransferDefinitions(this);
    if (canceled() != l_CanceledTransferDefinitions)
    {
        setCanceledForHandle(pLVKey, pJobId, pJobStepId, pHandle, UNDEFINED_CONTRIBID, l_CanceledTransferDefinitions);
    }
    return;
}

void BBTagInfo::calcStopped(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle)
{
    int l_StoppedTransferDefinitions = parts.anyStoppedTransferDefinitions(this);
    if (stopped() != l_StoppedTransferDefinitions)
    {
        setStopped(pLVKey, pJobId, pJobStepId, pHandle, UNDEFINED_CONTRIBID, l_StoppedTransferDefinitions);
    }
    return;
}

void BBTagInfo::dump(const char* pSev)
{
    if (wrkqmgr.checkLoggingLevel(pSev))
    {
        stringstream l_Temp;
        expectContribToSS(l_Temp);
        if (!strcmp(pSev,"debug"))
        {
            LOG(bb,debug) << hex << uppercase << setfill('0') << "Transfer Handle: 0x" << setw(16) << transferHandle << setfill(' ') << nouppercase << dec << " (" << transferHandle << ")";
            LOG(bb,debug) << "Expect Contrib:  " << l_Temp.str();
            parts.dump(pSev);
        }
        else if (!strcmp(pSev,"info"))
        {
            LOG(bb,info) << hex << uppercase << setfill('0') << "Transfer Handle: 0x" << setw(16) << transferHandle << setfill(' ') << nouppercase << dec << " (" << transferHandle << ")";
            LOG(bb,info) << "Expect Contrib:  " << l_Temp.str();
            parts.dump(pSev);
        }
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
        int l_Attempts = 1;
        bool l_AllDone = false;
        while (!l_AllDone)
        {
            l_AllDone = true;

            // Make sure that the HandleFile indicates that the handle has at least
            // one transfer definition that is stopped
            rc = 1;
            uint64_t l_OriginalDeclareServerDeadCount = wrkqmgr.getDeclareServerDeadCount(pJob, pHandle, pContribId);
            uint64_t l_Continue = l_OriginalDeclareServerDeadCount;
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

                if (rc)
                {
                    if (l_Continue)
                    {
                        if (((l_OriginalDeclareServerDeadCount - l_Continue) % 15) == 5)
                        {
                            // Display this message every 15 seconds, after an initial wait of 5 seconds...
                            FL_Write6(FLDelay, PrepareForRestart, "Attempting to restart a transfer definition for jobid %ld, jobstepid %ld, handle %ld, contribid %ld. Delay of 1 second before retry. %ld seconds remain waiting for the original bbServer to act before an unconditional stop is performed.",
                                      (uint64_t)pJob.getJobId(), (uint64_t)pJob.getJobStepId(), (uint64_t)pHandle, (uint64_t)pContribId, (uint64_t)l_Continue, 0);
                            LOG(bb,info) << ">>>>> DELAY <<<<< BBTagInfo::prepareForRestart: Attempting to restart a transfer definition for jobid " << pJob.getJobId() \
                                         << ", jobstepid " << pJob.getJobStepId() << ", handle " << pHandle << ", contribid " << pContribId \
                                         << ". Waiting for the handle to be marked as stopped. Delay of 1 second before retry. " << l_Continue \
                                         << " seconds remain waiting for the original bbServer to act before an unconditional stop is performed.";
                        }
                        unlockLocalMetadata(pLVKey, "BBTagInfo::prepareForRestart - Waiting for transfer definition to be marked as stopped");
                        {
                            usleep((useconds_t)1000000);    // Delay 1 second
                        }
                        lockLocalMetadata(pLVKey, "BBTagInfo::prepareForRestart - Waiting for transfer definition to be marked as stopped");
                    }

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
                if (l_Attempts--)
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
                // Next, reset the flags for the local cached handle information.
                // NOTE: The handle file should already be recalculated as it's status is updated
                //       as the ContribIdFile is updated (code above).  These attributes for the
                //       handle file will again be recalculated as the local attributes for the handle
                //       are reset, but it does no harm.
                // NOTE: We cannot unconditionally reset the canceled, failed, and stopped attributes
                //       for the local handle information because we could have more than one contributor
                //       being restarted for a given CN.  In this case, the attribute in the local handle
                //       information will be reset when the last of those restarted transfer definitions
                //       for the CN is processed by restart.
                setAllExtentsTransferred(pLVKey, pJob.getJobId(), pJob.getJobStepId(), pHandle, 0);
                calcCanceled(pLVKey, pJob.getJobId(), pJob.getJobStepId(), pHandle);
                calcStopped(pLVKey, pJob.getJobId(), pJob.getJobStepId(), pHandle);
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

        // NOTE: We do not update the metadata handle status here because the number of reporting contributors for the
        //       handle file is not updated until the extents are enqueued for the transfer definition.  At that time, the
        //       handle status will be updated.
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

        // NOTE: We do not update the metadata handle status here because the status will not change based
        //       on this attribute alone.
    }

    return;
}

void BBTagInfo::setCanceledForHandle(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId, const int pValue)
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
        if (HandleFile::update_xbbServerHandleFile(pLVKey, pJobId, pJobStepId, pHandle, pContribId, BBTD_Canceled, pValue))
        {
            LOG(bb,error) << "BBTagInfo::setCanceled():  Failure when attempting to update the cross bbServer handle file for jobid " << pJobId \
                          << ", jobstepid " << pJobStepId << ", handle " << pHandle;
        }
    }

    return;
}

void BBTagInfo::setStopped(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId, const int pValue)
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
        if (HandleFile::update_xbbServerHandleFile(pLVKey, pJobId, pJobStepId, pHandle, pContribId, BBTD_Stopped, pValue))
        {
            LOG(bb,error) << "BBTagInfo::setStopped():  Failure when attempting to update the cross bbServer handle file status for jobid " << pJobId \
                          << ", jobstepid " << pJobStepId << ", handle " << pHandle;
        }
    }

    return;
}

int BBTagInfo::stopTransfer(const LVKey* pLVKey, BBLV_Info* pLV_Info, const string& pHostName, const string& pCN_HostName, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId, LOCAL_METADATA_RELEASED& pLockWasReleased)
{
    int rc = 0;

    // NOTE: pLockWasReleased intentionally not initialized

    if (pHandle == transferHandle)
    {
        rc = parts.stopTransfer(pLVKey, pHostName, pCN_HostName, pLV_Info, pJobId, pJobStepId, pHandle, pContribId, pLockWasReleased);
        if (rc == 1)
        {
            int l_Value = 1;
            // Set the stopped indicator in the local metadata...
            setStopped(pLVKey, pJobId, pJobStepId, pHandle, pContribId, l_Value);
        }
    }

    return rc;
}

int BBTagInfo::update_xbbServerAddData(const LVKey* pLVKey, HandleFile* pHandleFile, const char* pHandleFileName, const BBJob pJob, BBLV_Info* pLV_Info, const uint32_t pContribId, const uint64_t pHandle, BBTransferDef* &pTransferDef)
{
    int rc = 0;
    stringstream errorText;

    Uuid lv_uuid = pLVKey->second;
    char lv_uuid_str[LENGTH_UUID_STR] = {'\0'};
    lv_uuid.copyTo(lv_uuid_str);

    ContribFile* l_ContribFile = 0;
    ContribIdFile* l_ContribIdFile = 0;
    ContribIdFile* l_NewContribIdFile = 0;
    ContribIdFile* l_ContribIdFileToProcess = 0;

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, TI_AddData, "BBTagInfo server add data, counter=%ld, jobid=%ld, handle=%ld, contribid=%ld", l_FL_Counter, pJob.getJobId(), pHandle, pContribId);

    bfs::path l_Temp = bfs::path(pHandleFileName);
    bfs::path handle = l_Temp.parent_path();
    try
    {
        // NOTE: If this is a restart for a transfer definition, we verify that the ContribIdFile
        //       already exists.  In that case rc=1 will be returned...
        // NOTE: Vector contribHasReported in the HandleFile is maintained so a quick lookup
        //       of already reporting contribs can be performed without having to open/read/close
        //       all the 'contribs' files.
        // NOTE: We always interrogate the 'contribs' for a restart scenario.  This is because for a normal
        //       start transfer, the handle file is first created, followed by the contribid file, and then
        //       later the vector of reporting contribs is updated when the extents are enqueued.
        //       In the restart case, we want to find the contribid file if it exists.  So, we close the
        //       window by always searching the 'contribs' in the restart case.
        bool l_ContribFileFoundForLVUuid = false;
        bool l_ContribIdAlreadyExists = (bool)(pHandleFile->contribHasReported(pContribId));
        for (auto& lvuuid: boost::make_iterator_range(bfs::directory_iterator(handle), {}))
        {
            if (string(lv_uuid_str) == lvuuid.path().filename().string())
            {
                // NOTE:  If the lvuuid directory exists, the 'contribs' file should also exist.
                //        We rely on this fact for the early exit in this 'normal' start transfer path
                //        for multiple contributors for a given CN (lvuuid).
                l_ContribFileFoundForLVUuid = true;
                if ((!l_ContribIdAlreadyExists) && (!pTransferDef->builtViaRetrieveTransferDefinition()))
                {
                    // Non-restart for new reporting contributor.  Found the matching LVUuid.  Exit now...
                    break;
                }
            }
            if (l_ContribIdFile || (!pathIsDirectory(lvuuid))) continue;
            if (l_ContribIdAlreadyExists || pTransferDef->builtViaRetrieveTransferDefinition())
            {
                // We know this contributor had previously reported -or- this is a restart scenario.
                // Search under this LVUuid for the contributor...
                bfs::path l_ContribFilePath = lvuuid.path() / CONTRIBS_FILENAME;
                rc = ContribFile::loadContribFile(l_ContribFile, l_ContribFilePath);
                if (!rc)
                {
                    for (map<uint32_t,ContribIdFile>::iterator ce = l_ContribFile->contribs.begin(); ce != l_ContribFile->contribs.end(); ce++)
                    {
                        if (ce->first == pContribId)
                        {
                            if (string(lv_uuid_str) == lvuuid.path().filename().string())
                            {
                                l_ContribIdFile = new ContribIdFile(ce->second);
                                LOG(bb,info) << "xbbServer: Logical volume with a uuid of " << lv_uuid_str << " is already registered and currently has " << l_ContribFile->numberOfContribs() << " non-stopped contributor(s)";
                                break;
                            }
                            else
                            {
                                // Even for restart, the lvuuid for the already registered contribid must match the lvuuid for the transfer definition being added
                                rc = -1;
                                errorText << "Contribid " << pContribId << " is already registered under lvuuid " << lvuuid.path().filename().string() << " for job " << pJob.getJobId() << ", jobstepid " << pJob.getJobStepId() << ", handle " << pHandle;
                                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                            }
                        }
                    }
                }
                else
                {
                    // ContribFile could not be loaded
                    rc = -1;
                    errorText << "For job " << pJob.getJobId() << ", jobstepid " << pJob.getJobStepId() << ", handle " << pHandle \
                              << ", could not load the contribfile associated with " << lvuuid.path().filename().string() << " when processing contribid " << pContribId;
                    LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                }

                if (l_ContribFile)
                {
                    delete l_ContribFile;
                    l_ContribFile = 0;
                }
            }
        }

        if (!l_ContribIdFile)
        {
            if (!pTransferDef->builtViaRetrieveTransferDefinition())
            {
                if (!l_ContribFileFoundForLVUuid)
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
                    else if (pContribId == NO_CONTRIBID)
                    {
                        rc = -1;
                        errorText << "BBTagInfo::update_xbbServerAddData(): Attempt to add invalid contribid of " << NO_CONTRIBID << " to the cross bbServer metadata";
                        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                    }
                    else if (pContribId == UNDEFINED_CONTRIBID)
                    {
                        rc = -1;
                        errorText << "BBTagInfo::update_xbbServerAddData(): Attempt to add invalid contribid of " << UNDEFINED_CONTRIBID << " to the cross bbServer metadata";
                        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                    }

                    bfs::path l_JobStepPath(g_BBServer_Metadata_Path);
                    l_JobStepPath /= bfs::path(to_string(pJob.getJobId()));
                    l_JobStepPath /= bfs::path(to_string(pJob.getJobStepId()));
                    int l_JobStepDirectoryExists = access(l_JobStepPath.c_str(), F_OK) ? 0 : 1;

                    bfs::path l_ToplevelHandleDirectoryPath = l_JobStepPath / bfs::path(HandleFile::getToplevelHandleName(pHandle));
                    int l_ToplevelHandleDirectoryExists = access(l_ToplevelHandleDirectoryPath.c_str(), F_OK) ? 0 : 1;

                    LOG(bb,info) << "xbbServer: For job " << pJob.getJobId() << ", jobstepid " << pJob.getJobStepId() << ", handle " << pHandle \
                                 << ", a logical volume with a uuid of " << lv_uuid_str << " is not currently registered.  It will be added.";
                    bfs::path l_LVUuidPath = handle / bfs::path(lv_uuid_str);
                    bfs::create_directories(l_LVUuidPath);

                    if (!l_JobStepDirectoryExists)
                    {
                        // Unconditionally perform a chmod to 0770 for the jobstepid directory.
                        // NOTE:  This is done for completeness, as all access is via the great-grandparent directory (jobid) and access to the files
                        //        contained in this tree is controlled there.
                        rc = chmod(l_JobStepPath.c_str(), 0770);
                        if (rc)
                        {
                            errorText << "chmod failed";
                            bberror << err("error.path", l_JobStepPath.string());
                            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, errno);
                        }

                        // Create the lock file for the taginfo
                        rc = TagInfo::createLockFile(l_JobStepPath.string());
                        if (rc) BAIL;
                    }

                    if (!l_ToplevelHandleDirectoryExists)
                    {
                        // Unconditionally perform a chmod to 0770 for the toplevel handle directory.
                        // NOTE:  This is done for completeness, as all access is via the great-grandparent directory (jobid) and access to the files
                        //        contained in this tree is controlled there.
                        rc = chmod(l_ToplevelHandleDirectoryPath.c_str(), 0770);
                        if (rc)
                        {
                            errorText << "chmod failed";
                            bberror << err("error.path", l_ToplevelHandleDirectoryPath.string());
                            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, errno);
                        }
                    }

                    // Unconditionally perform a chmod to 0770 for the lvuuid directory.
                    // NOTE:  This is done for completeness, as all access is via the great-grandparent directory (jobid) and access to the files
                    //        contained in this tree is controlled there.
                    rc = chmod(l_LVUuidPath.c_str(), 0770);
                    if (rc)
                    {
                        errorText << "chmod failed";
                        bberror << err("error.path", l_LVUuidPath.string());
                        LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, errno);
                    }

                    LVUuidFile l_LVUuidFile((*pLVKey).first, pLV_Info->getHostName());
                    bfs::path l_LVUuidFilePathName = l_LVUuidPath / bfs::path("^" + string(lv_uuid_str));
                    rc = l_LVUuidFile.save(l_LVUuidFilePathName.string());
                    if (rc) BAIL;

                    ContribFile l_ContribFileStg;
                    bfs::path l_ContribFilePath = l_LVUuidPath / CONTRIBS_FILENAME;
                    rc = l_ContribFileStg.save(l_ContribFilePath.string());
                    if (rc) BAIL;
                }

                // Create a new ContribIdFile for this contributor
                l_NewContribIdFile = new ContribIdFile(pTransferDef);
                l_ContribIdFileToProcess = l_NewContribIdFile;
            }
            else
            {
                // For a restart, the ContribIdFile must already exist
                rc = -1;
                errorText << "BBTagInfo::update_xbbServerAddData(): For a restart transfer definition operation, could not find the ContribIdFile for " \
                          << *pLVKey << ", contribid " << pContribId << ", using handle path " << handle.string();
                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
            }
        }
        else
        {
            // ContribIdFile already exists
            rc = 1;
            if (!pTransferDef->builtViaRetrieveTransferDefinition())
            {
                if (l_ContribIdFile->extentsAreEnqueued())
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
                    rc = 0;
                }
            }
            l_ContribIdFileToProcess = l_ContribIdFile;
        }

        bool l_UpdateHandleStatus = false;
        if (!pTransferDef->hasFilesInRequest())
        {
            // No files in the request
            l_UpdateHandleStatus = true;
            uint64_t l_OriginalFileFlags = l_ContribIdFileToProcess->flags;
            pTransferDef->setExtentsEnqueued();
            SET_FLAG_VAR(l_ContribIdFileToProcess->flags, l_ContribIdFileToProcess->flags, BBTD_Extents_Enqueued, 1);
            pTransferDef->setAllExtentsTransferred();
            SET_FLAG_VAR(l_ContribIdFileToProcess->flags, l_ContribIdFileToProcess->flags, BBTD_All_Extents_Transferred, 1);
            pTransferDef->setAllFilesClosed();
            SET_FLAG_VAR(l_ContribIdFileToProcess->flags, l_ContribIdFileToProcess->flags, BBTD_All_Files_Closed, 1);
            if (l_OriginalFileFlags != l_ContribIdFileToProcess->flags)
            {
                LOG(bb,debug) << "xbbServer: For " << *pLVKey << ", handle " << pHandle << ", contribid " << pContribId << ":";
                LOG(bb,debug) << "           ContribId flags changing from 0x" << hex << uppercase << l_OriginalFileFlags << " to 0x" << l_ContribIdFileToProcess->flags << nouppercase << dec << ".";
            }
        }

        int rc2 = ContribIdFile::saveContribIdFile(l_ContribIdFileToProcess, pLVKey, handle, pContribId);

        if (rc2)
        {
            rc = rc2;
            SET_RC_AND_BAIL(rc);
        }

        if (l_UpdateHandleStatus)
        {
            rc = HandleFile::update_xbbServerHandleStatus(pLVKey, pJob.getJobId(), pJob.getJobStepId(), pHandle, pContribId, 1, 0, NORMAL_SCAN);
            if (rc)
            {
                LOG(bb,error) << "BBTagInfo::update_xbbServerAddData():  Failure when attempting to update the cross bbServer handle status for jobid " << pJob.getJobId() \
                              << ", jobstepid " << pJob.getJobStepId() << ", handle " << pHandle << ", contribid " << pContribId;
            }
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    if (l_ContribIdFile)
    {
        delete l_ContribIdFile;
        l_ContribIdFile = 0;
    }

    if (l_NewContribIdFile)
    {
        delete l_NewContribIdFile;
        l_NewContribIdFile = 0;
    }

    if (l_ContribFile)
    {
        delete l_ContribFile;
        l_ContribFile = 0;
    }

    FL_Write6(FLMetaData, TI_AddData_End, "BBTagInfo server add data, counter=%ld, jobid=%ld, handle=%ld, contribid=%ld, rc=%ld", l_FL_Counter, pJob.getJobId(), pHandle, pContribId, rc, 0);

    return rc;
}

int BBTagInfo::xbbServerIsHandleUnique(const BBJob& pJob, const uint64_t pHandle)
{
    bfs::path handle(g_BBServer_Metadata_Path);
    handle /= bfs::path(to_string(pJob.getJobId()));
    handle /= bfs::path(to_string(pJob.getJobStepId()));
    handle /= bfs::path(HandleFile::getToplevelHandleName(pHandle));
    handle /= bfs::path(to_string(pHandle));

    return (access(handle.c_str(), F_OK) ? 1 : 0);
}
