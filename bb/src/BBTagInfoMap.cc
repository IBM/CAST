/*******************************************************************************
 |    BBTagInfoMap.cc
 |
 |  � Copyright IBM Corporation 2015,2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/

#include "BBTagInfoMap.h"
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/system/error_code.hpp>

using namespace std;
using namespace boost::archive;
namespace bfs = boost::filesystem;
namespace bs = boost::system;

#include "bberror.h"
#include "BBLV_ExtentInfo.h"
#include "BBTransferDef.h"
#include "bbinternal.h"
#include "bbwrkqmgr.h"


//
// BBTagInfoMap class
//

//
// BBTagInfoMap - Static methods
//

//
// BBTagInfoMap - Non-static methods
//

void BBTagInfoMap::accumulateTotalLocalContributorInfo(const uint64_t pHandle, size_t& pTotalContributors, size_t& pTotalLocalReportingContributors)
{
    for (auto it = tagInfoMap.begin(); it != tagInfoMap.end(); ++it)
    {
        it->second.accumulateTotalLocalContributorInfo(pHandle, pTotalContributors, pTotalLocalReportingContributors);
    }

    return;
}

int BBTagInfoMap::addTagInfo(const LVKey* pLVKey, const BBJob pJob, const BBTagID& pTagId, BBTagInfo* &pTagInfo, uint64_t& pGeneratedHandle)
{
    int rc = 0;

    LOG(bb,debug) << "BBTagInfoMap::addTagInfo(): LVKey " << *pLVKey << ", job (" << pJob.getJobId() << "," << pJob.getJobStepId() << "), tagid " << pTagId.getTag() << ", generated handle " << pGeneratedHandle;

    // It is possible to enter this section of code without the transfer queue locked.
    // Inserting into a std::map is not thread safe, so we must acquire the lock around
    // the insert.
    int l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded(pLVKey, "BBTagInfoMap::addTagInfo");

    tagInfoMap[pTagId] = *pTagInfo;
    pTagInfo = &tagInfoMap[pTagId];

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata(pLVKey, "BBTagInfoMap::addTagInfo");
    }

    if (pGeneratedHandle) {
        rc = update_xbbServerAddData(pLVKey, pJob, pTagId.getTag(), pTagInfo);
    }

    return rc;
}

void BBTagInfoMap::cleanUpAll(const LVKey* pLVKey)
{
    stringstream l_JobStr;

    // For each TagId, cleanup each transfer definition...
    auto it = tagInfoMap.begin();
    while (it != tagInfoMap.end()) {
        it->second.cleanUpAll(pLVKey, it->first);
        if (!l_JobStr.str().size()) {
            it->first.getJob().getStr(l_JobStr);
        }
        LOG(bb,debug) << "taginfo: TagId(" << l_JobStr.str() << "," << it->first.getTag() << ") with handle 0x" \
                      << hex << uppercase << setfill('0') << setw(16) << it->second.transferHandle \
                      << setfill(' ') << nouppercase << dec << " (" << it->second.transferHandle \
                      << ") removed from " << *pLVKey;
        it = tagInfoMap.erase(it);
    }

    return;
}

void BBTagInfoMap::dump(char* pSev, const char* pPrefix)
{
    if (wrkqmgr.checkLoggingLevel(pSev))
    {
        int l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded((LVKey*)0, "BBTagInfoMap::dump");

        if (tagInfoMap.size())
        {
            if (!strcmp(pSev,"debug"))
            {
                LOG(bb,debug) << ">>>>> Start: " << (pPrefix ? pPrefix : "taginfo") << ", " \
                              << tagInfoMap.size() << (tagInfoMap.size()==1 ? " entry <<<<<" : " entries <<<<<");
                for (auto& it : tagInfoMap)
                {
                    const_cast <BBTagID*>(&(it.first))->dump(pSev);
                    it.second.dump(pSev);
                }
                LOG(bb,debug) << ">>>>>   End: " << (pPrefix ? pPrefix : "taginfo") << ", " \
                              << tagInfoMap.size() << (tagInfoMap.size()==1 ? " entry <<<<<" : " entries <<<<<");
            }
            else if (!strcmp(pSev,"info"))
            {
                LOG(bb,info) << ">>>>> Start: " << (pPrefix ? pPrefix : "taginfo") << ", " \
                             << tagInfoMap.size() << (tagInfoMap.size()==1 ? " entry <<<<<" : " entries <<<<<");
                for (auto& it : tagInfoMap)
                {
                    const_cast <BBTagID*>(&(it.first))->dump(pSev);
                    it.second.dump(pSev);
                }
                LOG(bb,info) << ">>>>>   End: " << (pPrefix ? pPrefix : "taginfo") << ", " \
                             << tagInfoMap.size() << (tagInfoMap.size()==1 ? " entry <<<<<" : " entries <<<<<");
            }
        }

        if (l_LocalMetadataWasLocked)
        {
            unlockLocalMetadata((LVKey*)0, "BBTagInfoMap::dump");
        }
    }

    return;
}

size_t BBTagInfoMap::getNumberOfTransferDefs(const BBTagID& pTagId)
{
    size_t l_Count = 0;

    BBTagParts* l_TagParts = getParts(pTagId);
    if (l_TagParts) {
        l_Count = l_TagParts->getNumberOfParts();
    }

    return l_Count;
}

BBTagParts* BBTagInfoMap::getParts(const BBTagID& pTagId)
{
    BBTagInfo* l_TagInfo = getTagInfo(pTagId);
    if (l_TagInfo) {
        return l_TagInfo->getParts();
    } else {
        return (BBTagParts*)0;
    }
}

int BBTagInfoMap::getTagInfo(const uint64_t pHandle, const uint32_t pContribId, BBTagID& pTagId, BBTagInfo* &pTagInfo)
{
    int rc = 0;

    for (auto it = tagInfoMap.begin(); it != tagInfoMap.end(); ++it) {
        if (pHandle == it->second.getTransferHandle() && it->second.inExpectContrib(pContribId)) {
            pTagId = it->first;
            pTagInfo = &(it->second);
            rc = 1;
            break;
        }
    }

    return rc;
}

BBTagInfo* BBTagInfoMap::getTagInfo(const BBTagID& pTagId)
{
    BBTagInfo* l_TagInfo = (BBTagInfo*)0;

    int l_TransferQueueWasUnlocked = unlockTransferQueueIfNeeded((LVKey*)0, "BBTagInfoMap::getTagInfo");
    int l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded((LVKey*)0, "BBTagInfoMap::getTagInfo");

    for (auto it = tagInfoMap.begin(); it != tagInfoMap.end(); ++it) {
        if (it->first == pTagId) {
            l_TagInfo = &(it->second);
            break;
        }
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata((LVKey*)0, "BBTagInfoMap::getTagInfo");
    }

    if (l_TransferQueueWasUnlocked)
    {
        lockTransferQueue((LVKey*)0, "BBTagInfoMap::getTagInfo");
    }

    return l_TagInfo;
}

int BBTagInfoMap::getTagInfo(BBTagInfo* &pTagInfo, const BBJob pJob, const uint64_t pTag, const uint64_t pNumContrib, const uint32_t pContrib[])
{
    int rc = 0;

    for (auto it = tagInfoMap.begin(); it != tagInfoMap.end(); ++it) {
        if (it->first.getJob() == pJob && it->first.getTag() == pTag) {
            if (!(it->second.compareContrib(pNumContrib, pContrib))) {
                // Contrib list matched
                pTagInfo = &(it->second);
                rc = 1;
                break;
            } else {
                // Contrib list did not match
                rc = -1;
                break;
            }
        }
    }

    return rc;
}

size_t BBTagInfoMap::getTotalTransferSize()
{
    size_t l_TotalSize = 0;

    for (auto it = tagInfoMap.begin(); it != tagInfoMap.end(); ++it) {
        l_TotalSize += it->second.getTotalTransferSize();
    }

    return l_TotalSize;
}

void BBTagInfoMap::getTransferHandles(std::vector<uint64_t>& pHandles, const BBJob pJob, const BBSTATUS pMatchStatus, const int pStageOutStarted)
{
    for (auto it = tagInfoMap.begin(); it != tagInfoMap.end(); ++it) {
        bool l_Match = false;
        if (pJob.getJobStepId() == 0) {
            if (pJob.getJobId() == it->first.getJobId()) {
                l_Match = true;
            }
        } else {
            if (pJob == it->first.getJob()) {
                l_Match = true;
            }
        }
        if (l_Match) {
            if ( BBSTATUS_AND(pMatchStatus, it->second.getStatus(pStageOutStarted)) != BBNONE ) {
                pHandles.push_back(it->second.getTransferHandle());
            }
        }
    }

    return;
}

int BBTagInfoMap::hasContribId(const uint32_t pContribId)
{
    int rc = 0;

    int l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded((LVKey*)0, "BBTagInfoMap::hasContribId");

    for (auto it = tagInfoMap.begin(); rc == 0 && it != tagInfoMap.end(); ++it) {
        rc = it->second.inExpectContrib(pContribId);
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata((LVKey*)0, "BBTagInfoMap::hasContribId");
    }

    return rc;
}

int BBTagInfoMap::isUniqueHandle(uint64_t pHandle)
{
    int rc = 1;

    int l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded((LVKey*)0, "BBTagInfoMap::isUniqueHandle");

    for (auto it = tagInfoMap.begin(); it != tagInfoMap.end(); ++it) {
        if (pHandle == it->second.getTransferHandle()) {
            rc = 0;
            break;
        }
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata((LVKey*)0, "BBTagInfoMap::isUniqueHandle");
    }

    return rc;

}

void BBTagInfoMap::removeTargetFiles(const LVKey* pLVKey, const uint64_t pHandle, const uint32_t pContribId)
{
    for(auto it = tagInfoMap.begin(); it != tagInfoMap.end(); ++it)
    {
        it->second.removeTargetFiles(pLVKey, pHandle, pContribId);
    }

    return;
}

int BBTagInfoMap::retrieveTransfers(BBTransferDefs& pTransferDefs, BBLV_ExtentInfo* pExtentInfo)
{
    int rc = 0;

    for(auto it = tagInfoMap.begin(); (!rc) && it != tagInfoMap.end(); ++it)
    {
        rc = it->second.retrieveTransfers(pTransferDefs, pExtentInfo);
    }

    return rc;
}

void BBTagInfoMap::sendTransferCompleteForHandleMsg(const string& pHostName, const string& pCN_HostName, const string& pConnectionName, const LVKey* pLVKey, BBLV_Info* pLV_Info, const uint64_t pHandle, int& pAppendAsyncRequestFlag, const BBSTATUS pStatus)
{
    for (auto it = tagInfoMap.begin(); it != tagInfoMap.end(); ++it)
    {
        it->second.sendTransferCompleteForHandleMsg(pHostName, pCN_HostName, pConnectionName, pLVKey, pLV_Info, it->first, pHandle, pAppendAsyncRequestFlag, pStatus);
    }

    return;
}

void BBTagInfoMap::setCanceled(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle)
{
    for (auto it = tagInfoMap.begin(); it != tagInfoMap.end(); ++it)
    {
        it->second.setCanceledForHandle(pLVKey, pJobId, pJobStepId, pHandle, UNDEFINED_CONTRIBID);
    }

    return;
}

int BBTagInfoMap::stopTransfer(const LVKey* pLVKey, BBLV_Info* pLV_Info, const string& pHostName, const string& pCN_HostName, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId, LOCAL_METADATA_RELEASED& pLockWasReleased)
{
    int rc = 0;

    // NOTE: pLockWasReleased intentionally not initialized

    for (auto it = tagInfoMap.begin(); ((!rc) && it != tagInfoMap.end()); ++it)
    {
         rc = it->second.stopTransfer(pLVKey, pLV_Info, pHostName, pCN_HostName, pJobId, pJobStepId, pHandle, pContribId, pLockWasReleased);
    }

    return rc;
}

void BBTagInfoMap::updateAllContribsReported(const LVKey* pLVKey, int& pAllReported)
{
    pAllReported = 0;

    int l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded(pLVKey, "BBTagInfoMap::updateAllContribsReported");

    int l_AllReported = 1;
    for (auto it = tagInfoMap.begin(); it != tagInfoMap.end(); ++it) {
        if (!(it->second.allContribsReported())) {
            l_AllReported = 0;
            break;
        }
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata(pLVKey, "BBTagInfoMap::updateAllContribsReported");
    }

    pAllReported = l_AllReported;

    return;
}

int BBTagInfoMap::updateAllTransferHandleStatus(const string& pConnectionName, const LVKey* pLVKey, const uint64_t pJobId, BBLV_ExtentInfo& pLVKey_ExtentInfo, uint32_t pNumberOfExpectedInFlight)
{
    int rc = 0;

    int l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded(pLVKey, "BBTagInfoMap::updateAllTransferHandleStatus");

    // Update the status for all transferHandles
    int l_AllContribsReportedForAllTransferHandles = 1;
    int l_AllExtentsSentForAllTransferHandles = 1;
    for (auto it = tagInfoMap.begin(); (!rc) && it != tagInfoMap.end(); ++it) {
        if (it->second.allContribsReported()) {
            if (!(it->second.allExtentsTransferred())) {
                if (pLVKey_ExtentInfo.moreExtentsToTransfer((int64_t)(it->second.getTransferHandle()), (int32_t)(-1), pNumberOfExpectedInFlight)) {
                    l_AllExtentsSentForAllTransferHandles = 0;
                } else {
                    it->second.setAllExtentsTransferred(pLVKey, pJobId, it->first.getJobStepId(), it->second.getTransferHandle());
                }
            }
        } else {
            rc = 0;
            l_AllContribsReportedForAllTransferHandles = 0;
            l_AllExtentsSentForAllTransferHandles = 0;
        }
    }

    if (l_AllContribsReportedForAllTransferHandles) {
        pLVKey_ExtentInfo.setAllContribsReported(pLVKey);
    }

    if (l_AllExtentsSentForAllTransferHandles) {
        pLVKey_ExtentInfo.updateTransferStatus(pConnectionName, pLVKey, pNumberOfExpectedInFlight);
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata(pLVKey, "BBTagInfoMap::updateAllTransferHandleStatus");
    }

    return rc;
}

int BBTagInfoMap::update_xbbServerAddData(const LVKey* pLVKey, const BBJob pJob, const uint64_t pTag, BBTagInfo* &pTagInfo)
{
    int rc = 0;
    stringstream errorText;

    try
    {
        bfs::path jobstepid(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
        jobstepid = jobstepid / bfs::path(to_string(pJob.getJobId())) / bfs::path(to_string(pJob.getJobStepId()));
        bfs::path handle = jobstepid / bfs::path(to_string(pTagInfo->getTransferHandle()));

        // NOTE:  There is a window between creating the job directory and
        //        performing the chmod to the correct uid:gid.  Therefore, if
        //        create_directories() returns EACCESS (permission denied), keep
        //        attempting for 2 minutes.
        bs::error_code l_ErrorCode;
        bool l_AllDone = false;
        bool l_JobStepDirectoryAlreadyExists = false;
        int l_Attempts = 120;
        while ((!l_AllDone) && l_Attempts-- > 0)
        {
            if(!bfs::exists(handle))
            {
                // Note if the jobstepid directory exists...
                if(bfs::exists(jobstepid))
                {
                    l_JobStepDirectoryAlreadyExists = true;
                }

                // On first attempt, log the creation of the handle directory...
                if (l_Attempts == 119)
                {
                    LOG(bb,info) << "xbbServer: Handle " << pTagInfo->getTransferHandle() << " is not already registered.  It will be added.";
                }

                // Attempt to create the handle directory
                bfs::create_directories(handle, l_ErrorCode);
                if (l_ErrorCode.value() == EACCES)
                {
                    usleep((useconds_t)1000000);    // Delay 1 second
                }
                else
                {
                    // Handle directory created
                    l_Attempts = -1;
                }
            }
            else
            {
                // Handle directory already exists
                l_AllDone = true;
                LOG(bb,debug) << "BBTagInfoMap::update_xbbServerAddData(): Handle file " << handle.c_str() << " already exists";
            }
        }

        if (!l_AllDone)
        {
            if (l_Attempts == 0)
            {
                // Error returned via create_directories...
                // Attempt one more time, without the error code.
                // On error, the appropriate boost exception will be thrown...
                LOG(bb,debug) << "BBTagInfoMap::update_xbbServerAddData(): l_Attempts " << l_Attempts << ", l_ErrorCode.value() " << l_ErrorCode.value();
                bfs::create_directories(handle);
            }

            // Archive a file for this handle...
            // NOTE: We want the creation of the directory and the handle file as close together as possible.
            //       The handle file is 'assumed' to exist if the directory exists...
            HandleFile* l_HandleFile = 0;
            rc = HandleFile::saveHandleFile(l_HandleFile, pLVKey, pJob.getJobId(), pJob.getJobStepId(), pTag, *pTagInfo, pTagInfo->getTransferHandle());
            if (!rc)
            {
                if (!l_JobStepDirectoryAlreadyExists)
                {
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
                }

                // Unconditionally perform a chmod to 0770 for the handle directory.
                // NOTE:  This is done for completeness, as all access is via the grandparent directory (jobid) and access to the files
                //        contained in this tree is controlled there.
                rc = chmod(handle.c_str(), 0770);
                if (rc)
                {
                    errorText << "chmod failed";
                    bberror << err("error.path", handle.c_str());
                    LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, errno);
                }
            }
            else
            {
                // Back out the creation of the handle directory
                bfs::remove(handle);
            }
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
