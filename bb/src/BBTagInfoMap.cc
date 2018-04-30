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

using namespace std;
using namespace boost::archive;
namespace bfs = boost::filesystem;

#include "bberror.h"
#include "BBLVKey_ExtentInfo.h"
#include "BBTransferDef.h"
#include "bbinternal.h"


// Static data...
// NOTE: This lock is not used today.  Serialization of all metadata is controlled by the transfer queue lock
// pthread_mutex_t MetadataMutex = PTHREAD_MUTEX_INITIALIZER;


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

int BBTagInfoMap::addTagInfo(const LVKey* pLVKey, const BBJob pJob, const BBTagID pTagId, BBTagInfo& pTagInfo, int& pGeneratedHandle)
{
    int rc = 0;

    LOG(bb,debug) << "BBTagInfoMap::addTagInfo(): LVKey " << *pLVKey << ", job (" << pJob.getJobId() << "," << pJob.getJobStepId() << "), tagid " << pTagId.getTag() << ", generated handle " << pGeneratedHandle;
    tagInfoMap[pTagId] = pTagInfo;
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
        LOG(bb,info) << "taginfo: TagId(" << l_JobStr.str() << "," << it->first.getTag() << ") with handle 0x" \
                     << hex << uppercase << setfill('0') << setw(16) << it->second.transferHandle \
                     << setfill(' ') << nouppercase << dec << " (" << it->second.transferHandle \
                     << ") removed from " << *pLVKey;
        it = tagInfoMap.erase(it);
    }
}

void BBTagInfoMap::dump(char* pSev, const char* pPrefix)
{
    if (tagInfoMap.size()) {
        if (!strcmp(pSev,"debug")) {
            LOG(bb,debug) << ">>>>> Start: " << (pPrefix ? pPrefix : "taginfo") << ", " \
                          << tagInfoMap.size() << (tagInfoMap.size()==1 ? " entry <<<<<" : " entries <<<<<");
            for (auto& it : tagInfoMap) {
                const_cast <BBTagID*>(&(it.first))->dump(pSev);
                it.second.dump(pSev);
            }
            LOG(bb,debug) << ">>>>>   End: " << (pPrefix ? pPrefix : "taginfo") << ", " \
                          << tagInfoMap.size() << (tagInfoMap.size()==1 ? " entry <<<<<" : " entries <<<<<");
        } else if (!strcmp(pSev,"info")) {
            LOG(bb,info) << ">>>>> Start: " << (pPrefix ? pPrefix : "taginfo") << ", " \
                         << tagInfoMap.size() << (tagInfoMap.size()==1 ? " entry <<<<<" : " entries <<<<<");
            for (auto& it : tagInfoMap) {
                const_cast <BBTagID*>(&(it.first))->dump(pSev);
                it.second.dump(pSev);
            }
            LOG(bb,info) << ">>>>>   End: " << (pPrefix ? pPrefix : "taginfo") << ", " \
                         << tagInfoMap.size() << (tagInfoMap.size()==1 ? " entry <<<<<" : " entries <<<<<");
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

    if (hasContribId(pContribId)) {
        for (auto it = tagInfoMap.begin(); it != tagInfoMap.end(); ++it) {
            if (pHandle == it->second.getTransferHandle()) {
                pTagId = it->first;
                pTagInfo = &(it->second);
                rc = 1;
                break;
            }
        }
    }

    return rc;
}

BBTagInfo* BBTagInfoMap::getTagInfo(const BBTagID& pTagId)
{
    BBTagInfo* l_TagInfo = (BBTagInfo*)0;

    for (auto it = tagInfoMap.begin(); it != tagInfoMap.end(); ++it) {
        if (it->first == pTagId) {
            l_TagInfo = &(it->second);
            break;
        }
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

    for (auto it = tagInfoMap.begin(); rc == 0 && it != tagInfoMap.end(); ++it) {
        rc = it->second.inExpectContrib(pContribId);
    }

    return rc;
}


int BBTagInfoMap::isUniqueHandle(uint64_t pHandle)
{
    int rc = 1;
    for (auto it = tagInfoMap.begin(); it != tagInfoMap.end(); ++it) {
        if (pHandle == it->second.getTransferHandle()) {
            rc = 0;
            break;
        }
    }

    return rc;

}

void BBTagInfoMap::removeTransferDef(const BBTagID& pTagId, const uint32_t pContribId)
{
    BBTagParts* l_TagParts = getParts(pTagId);
    if (l_TagParts) {
        l_TagParts->removeTransferDef(pContribId);
    }

    return;
}

int BBTagInfoMap::retrieveTransfers(BBTransferDefs& pTransferDefs, BBLVKey_ExtentInfo* pExtentInfo)
{
    int rc = 0;

    for(auto it = tagInfoMap.begin(); (!rc) && it != tagInfoMap.end(); ++it)
    {
        rc = it->second.retrieveTransfers(pTransferDefs, pExtentInfo);
    }

    return rc;
}

void BBTagInfoMap::sendTransferCompleteForHandleMsg(const string& pHostName, const string& pConnectionName, const LVKey* pLVKey, BBTagInfo2* pTagInfo2, const uint64_t pHandle, const BBSTATUS pStatus)
{
    for (auto it = tagInfoMap.begin(); it != tagInfoMap.end(); ++it)
    {
        it->second.sendTransferCompleteForHandleMsg(pHostName, pConnectionName, pLVKey, pTagInfo2, it->first, pHandle, pStatus);
    }

    return;
}

void BBTagInfoMap::setCanceled(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle)
{
    for (auto it = tagInfoMap.begin(); it != tagInfoMap.end(); ++it)
    {
        it->second.setCanceled(pLVKey, pJobId, pJobStepId, pHandle);
    }

    return;
}

int BBTagInfoMap::stopTransfer(const LVKey* pLVKey, BBTagInfo2* pTagInfo2, const string& pHostName, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId)
{
    int rc = 0;

    for (auto it = tagInfoMap.begin(); ((!rc) && it != tagInfoMap.end()); ++it)
    {
         rc = it->second.stopTransfer(pLVKey, pTagInfo2, pHostName, pJobId, pJobStepId, pHandle, pContribId);
    }

    return rc;
}

void BBTagInfoMap::updateAllContribsReported(const LVKey* pLVKey, int& pAllReported)
{
    pAllReported = 0;

    int l_AllReported = 1;
    for (auto it = tagInfoMap.begin(); it != tagInfoMap.end(); ++it) {
        if (!(it->second.allContribsReported())) {
            l_AllReported = 0;
            break;
        }
    }

    pAllReported = l_AllReported;

    return;
}

int BBTagInfoMap::updateAllTransferHandleStatus(const string& pConnectionName, const LVKey* pLVKey, const uint64_t pJobId, BBLVKey_ExtentInfo& pLVKey_ExtentInfo, uint32_t pNumberOfExpectedInFlight)
{
    int rc = 0;

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

    return rc;
}

int BBTagInfoMap::update_xbbServerAddData(const LVKey* pLVKey, const BBJob pJob, const uint64_t pTag, BBTagInfo& pTagInfo)
{
    int rc = 0;
    stringstream errorText;

    try
    {
        bfs::path jobstepid(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
        jobstepid = jobstepid / bfs::path(to_string(pJob.getJobId())) / bfs::path(to_string(pJob.getJobStepId()));
        bfs::path handle = jobstepid / bfs::path(to_string(pTagInfo.getTransferHandle()));
        if(!bfs::exists(handle))
        {
            bool l_JobStepDirectoryAlreadyExists = false;
            if(bfs::exists(jobstepid))
            {
                l_JobStepDirectoryAlreadyExists = true;
            }

            LOG(bb,info) << "xbbServer: Handle " << pTagInfo.getTransferHandle() << " is not already registered.  It will be added.";
            bfs::create_directories(handle);

            if (!l_JobStepDirectoryAlreadyExists)
            {
                // Perform a chmod to 0770 for the jobstepid directory.
                // NOTE:  This is done for completeness, as all access is via the parent directory (jobid) and access to the files
                //        contained in this tree is controlled there.
                rc = chmod(jobstepid.c_str(), 0770);
                if (rc)
                {
                    stringstream errorText;
                    errorText << "chmod failed";
                    bberror << err("error.path", jobstepid.c_str());
                    LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
                }
            }

            // Unconditionally perform a chmod to 0770 for the handle directory.
            // NOTE:  This is done for completeness, as all access is via the grandparent directory (jobid) and access to the files
            //        contained in this tree is controlled there.
            rc = chmod(handle.c_str(), 0770);
            if (rc)
            {
                stringstream errorText;
                errorText << "chmod failed";
                bberror << err("error.path", handle.c_str());
                LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
            }

            // Archive a file for this handle...
            HandleFile* l_HandleFile = 0;
            rc = HandleFile::saveHandleFile(l_HandleFile, pLVKey, pJob.getJobId(), pJob.getJobStepId(), pTag, pTagInfo, pTagInfo.getTransferHandle());
        }
        else
        {
            LOG(bb,debug) << "BBTagInfoMap::update_xbbServerAddData(): Handle file " << handle.c_str() << " already exists";
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
