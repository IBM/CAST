/*******************************************************************************
 |    BBTagInfoMap2.cc
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
#include <boost/range/iterator_range.hpp>

#include "bberror.h"
#include "bbinternal.h"
#include "bbwrkqmgr.h"
#include "BBTagInfoMap2.h"

namespace bfs = boost::filesystem;

//
// BBTagInfoMap2 class
//

//
// BBTagInfoMap2 - Static members
//

int BBTagInfoMap2::update_xbbServerAddData(const uint64_t pJobId)
{
    int rc = 0;
    stringstream errorText;

    try
    {
        if (pJobId != UNDEFINED_JOBID)
        {
            bfs::path job(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
            job /= bfs::path(to_string(pJobId));

            if(bfs::exists(job))
            {
                // NOTE: This is the normal case for the restart of a transfer definition...
                unsigned count = 0;
                for(auto& elements: boost::make_iterator_range(bfs::directory_iterator(job), {}))
                {
                    LOG(bb,info) << "elements: " << elements;
                    count++;
                }
                LOG(bb,info) << "xbbServer: JobId " << pJobId << " is already registered and currently has " << count << " associated job step(s)";
            }
            else
            {
                bfs::create_directories(job);
                // Unconditionally perform a chmod to 0770 for the jobid directory.
                // This is required so that only root, the uid, and any user belonging to the gid can access this 'job'
                rc = chmod(job.c_str(), 0770);
                if (rc)
                {
                    errorText << "chmod failed";
                    bberror << err("error.path", job.c_str());
                    LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
                }
            }
        }
        else
        {
            rc = -1;
            errorText << "BBTagInfoMap2::update_xbbServerAddData(): Attempt to add invalid jobid of " << UNDEFINED_JOBID << " to the cross bbServer metadata";
            bberror << err("error.jobid", UNDEFINED_JOBID);
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
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

int BBTagInfoMap2::update_xbbServerRemoveData(const uint64_t pJobId) {
    int rc = 0;
    stringstream errorText;

    try
    {
        bfs::path job(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
        job /= bfs::path(to_string(pJobId));

        if(bfs::exists(job))
        {
            bfs::remove_all(job);
            LOG(bb,info) << "JobId " << pJobId << " was removed from the cross-bbServer metadata";
        }
        else
        {
            rc = -2;
            errorText << "JobId " << pJobId << " was not found in the cross-bbServer metadata";
            LOG(bb,info) << errorText.str();
            bberror << err("error.text", errorText.str()) << errloc(rc);
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        // NOTE: There is a window between checking for the job above and subsequently removing
        //       the job.  This is the most likely exception...  Return -2...
        rc = -2;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    return rc;
}

//
// BBTagInfoMap2 - Non-static members
//

void BBTagInfoMap2::accumulateTotalLocalContributorInfo(const uint64_t pHandle, size_t& pTotalContributors, size_t& pTotalLocalReportingContributors)
{
    pTotalContributors = 0;
    pTotalLocalReportingContributors = 0;

    for(auto it = tagInfoMap2.begin(); it != tagInfoMap2.end(); ++it)
    {
        it->second.accumulateTotalLocalContributorInfo(pHandle, pTotalContributors, pTotalLocalReportingContributors);
    }

    return;
}

int BBTagInfoMap2::addLVKey(const string& pHostName, const LVKey* pLVKey, const uint64_t pJobId, BBTagInfo2& pTagInfo2, const TOLERATE_ALREADY_EXISTS_OPTION pTolerateAlreadyExists)
{
    int rc = 0;
    stringstream errorText;

    for(auto it = tagInfoMap2.begin(); it != tagInfoMap2.end(); ++it)
    {
        if (it->first == *pLVKey)
        {
            errorText << *pLVKey << " already exists.  Registration failed with bbserver.";
            if (!pTolerateAlreadyExists)
            {
                rc = -1;
                LOG_ERROR_TEXT(errorText);
            }
            else
            {
                rc = -2;
                LOG(bb,info) << errorText << " Tolerated exception.";
            }
            break;
        }
        else
        {
            if ((it->first).first == pLVKey->first && (it->second).getJobId() == pJobId)
            {
                errorText << *pLVKey << " already exists with the same connection for the jobid " << pJobId << ".  Registration failed with bbserver.";
                if (!pTolerateAlreadyExists)
                {
                    rc = -1;
                    LOG_ERROR_TEXT(errorText);
                }
                else
                {
                    rc = -2;
                    LOG(bb,info) << errorText << " Tolerated exception.";
                }
                break;
            }
            else
            {
                // If we are not simply registering an LVKey for restart scenarios, if any LVKeys associated with the same hostname
                // are suspended, prevent the add.
                // NOTE:  bbServer cannot catch the case of a suspended hostname and this is the first to be added.
                //        We rely on bbProxy to prevent that from happening...
                if ((!pTolerateAlreadyExists) && (it->second).isSuspended() && pHostName == (it->second).getHostName())
                {
                    errorText << "Hostname " << (it->second).getHostName() << " is currently suspended. Therefore " << *pLVKey << " cannot be added.  Registration failed with bbserver." \
                              << " Attempt to retry the create logical volume request when the connection is not suspended.";
                    LOG_ERROR_TEXT(errorText);
                    rc = 1;
                    break;
                }
            }
        }
    }

    if (!rc)
    {
        LOG(bb,debug) << "taginfo: Adding " << *pLVKey << " from host " << pTagInfo2.getHostName() << " for jobid " << pJobId;
        tagInfoMap2[*pLVKey] = pTagInfo2;
        rc = update_xbbServerAddData(pJobId);
    }
    else if (rc == -2)
    {
        // Reset return code for tolerated exceptions
        rc = 0;
    }

    return rc;
}

int BBTagInfoMap2::cleanLVKeyOnly(const LVKey* pLVKey) {
    int rc = 0;

    BBTagInfo2* l_TagInfo2 = getTagInfo2(pLVKey);
    if (l_TagInfo2) {
        tagInfoMap2.erase(*pLVKey);
    } else {
        rc = -1;
    }

    return rc;
}

void BBTagInfoMap2::dump(char* pSev, const char* pPrefix) {
    if (tagInfoMap2.size()) {
        char l_Temp[LENGTH_UUID_STR] = {'\0'};
        if (!strcmp(pSev,"debug")) {
            LOG(bb,debug) << "";
            LOG(bb,debug) << ">>>>> Start: " << (pPrefix ? pPrefix : "taginfo2") << ", " \
                          << tagInfoMap2.size() << (tagInfoMap2.size()==1 ? " entry <<<<<" : " entries <<<<<");
            for (auto& it : tagInfoMap2) {
                const_cast <Uuid*> (&(it.first.second))->copyTo(l_Temp);
                LOG(bb,debug) << "LVKey -> Local Port: " << it.first.first << "   Uuid: " << l_Temp;
                it.second.dump(pSev);
            }
            LOG(bb,debug) << ">>>>>   End: " << (pPrefix ? pPrefix : "taginfo2") << ", " \
                          << tagInfoMap2.size() << (tagInfoMap2.size()==1 ? " entry <<<<<" : " entries <<<<<");
            LOG(bb,debug) << "";
        } else if (!strcmp(pSev,"info")) {
            LOG(bb,info) << "";
            LOG(bb,info) << ">>>>> Start: " << (pPrefix ? pPrefix : "taginfo2") << ", " \
                         << tagInfoMap2.size() << (tagInfoMap2.size()==1 ? " entry <<<<<" : " entries <<<<<");
            for (auto& it : tagInfoMap2) {
                const_cast <Uuid*> (&(it.first.second))->copyTo(l_Temp);
                LOG(bb,info) << "LVKey -> Local Port: " << it.first.first << "   Uuid: " << l_Temp;
                it.second.dump(pSev);
            }
            LOG(bb,info) << ">>>>>   End: " << (pPrefix ? pPrefix : "taginfo2") << ", " \
                         << tagInfoMap2.size() << (tagInfoMap2.size()==1 ? " entry <<<<<" : " entries <<<<<");
            LOG(bb,info) << "";
        }
    }
}

void BBTagInfoMap2::ensureStageOutEnded(const LVKey* pLVKey) {

    // Ensure stage-out ended for the given LVKey
    for(auto it = tagInfoMap2.begin(); it != tagInfoMap2.end(); ++it) {
        if((it->first) == *pLVKey) {
            (it->second).ensureStageOutEnded(&(it->first));
            break;
        }
    }

    return;
}

void BBTagInfoMap2::ensureStageOutEnded(const uint64_t pJobId) {

    // Ensure stage-out ended for all LVKeys under the job
    for (auto it = tagInfoMap2.begin(); it != tagInfoMap2.end(); ++it) {
        if ((it->second).getJobId() == pJobId) {
            (it->second).ensureStageOutEnded(&(it->first));
        }
    }

    return;
}

// NOTE:  This method returns any LVKey with the input LV Uuid and jobid...
int BBTagInfoMap2::getAnyLVKeyForUuidAndJobId(LVKey* &pLVKeyOut, LVKey* &pLVKeyIn, const uint64_t pJobId) {
    int rc = -2;    // LVKey not registered with bbserver
    for(auto it = tagInfoMap2.begin(); it != tagInfoMap2.end(); ++it)
    {
        if ((pLVKeyIn == NULL || pLVKeyIn->second == (it->first).second) && (pJobId == UNDEFINED_JOBID || pJobId == (it->second).getJobId()))
        {
            // Matching LV Uuid and acceptable jobid...
            rc = 1;
            *pLVKeyOut = it->first;
            break;
        }
    }

    return rc;
}

BBTagInfo2* BBTagInfoMap2::getAnyTagInfo2ForUuid(const LVKey* pLVKey) const {
    for(auto it =  tagInfoMap2.begin(); it != tagInfoMap2.end(); ++it) {
        if ((it->first).second == pLVKey->second) {
            return const_cast <BBTagInfo2*> (&(it->second));
        }
    }

    return (BBTagInfo2*)0;
}

int BBTagInfoMap2::getInfo(const std::string& pConnectionName, LVKey& pLVKey, BBTagInfo2* &pTagInfo2, BBTagInfo* &pTagInfo, BBTagID &pTagId, const BBJob pJob, std::vector<uint32_t>*& pContrib, const uint64_t pHandle, const uint32_t pContribId) {
    int rc = 0;
    LVKey l_LVKey;
    BBTagID l_TagId;
    uint64_t l_NumContrib = 0;
    uint64_t l_Handle = 0;
    BBTagInfo* l_TagInfo = (BBTagInfo*)0;
    std::vector<uint32_t>* l_Contrib = 0;
    uint32_t* l_ContribArray = 0;

    // Returning a rc = 1, -> found
    //             rc = 0, -> not found
    //             rc < 0, -> error

    bool l_HandleWasAdded = false;
    for(auto it = tagInfoMap2.begin(); it != tagInfoMap2.end(); ++it) {
        if((it->second).getTagInfo(pHandle, pContribId, l_TagId, l_TagInfo)) {
            // We found the handle that is currently associated with 'some' LVKey.
            if ((it->second).getJobId() == pJob.getJobId()) {
                // This handle is associated with the correct jobid...
                // NOTE: We do not verify any jobstepid criteria...
                if((it->first).first == pConnectionName) {
                    // Correct LVKey...  Set the return data...
                    pLVKey = it->first;
                    pTagInfo2 = &(it->second);
                    pTagInfo = l_TagInfo;
                    pTagId = l_TagId;
                    pContrib = pTagInfo->getExpectContrib();
                    rc = 1;
                    break;
                } else {
                    l_Contrib = l_TagInfo->getExpectContrib();
                    l_TagInfo = 0;
                    l_NumContrib = (uint64_t)l_Contrib->size();
                    l_ContribArray = (uint32_t*)(new char[sizeof(uint32_t)*l_NumContrib]);
                    for(uint64_t i=0; i<l_NumContrib; ++i)
                    {
                        l_ContribArray[i] = (*l_Contrib)[i];
                    }
                    l_Handle = pHandle;
                    // Add this handle under the LVKey associated with the connection and jobid...
                    for(auto it2 = tagInfoMap2.begin(); it2 != tagInfoMap2.end(); ++it2) {
                        if((it2->first).first == pConnectionName && (it2->second).getJobId() == pJob.getJobId()) {
                            // NOTE: We use the LVKey value from the current entry and
                            //       we use the tag value from that returned by getTagInfo
                            //       above for the 'incorrect' LVKey....
                            l_LVKey = it2->first;
                            BBTransferDef* l_TransferDef = 0;
                            uint32_t l_Dummy = 0;

                            rc = queueTransfer(pConnectionName, &l_LVKey, pJob, l_TagId.getTag(), l_TransferDef, (int32_t)(-1), l_NumContrib, l_ContribArray, l_Handle, 0, l_Dummy, (vector<struct stat*>*)0);
                            if (!rc) {
                                l_HandleWasAdded = true;
                            } else {
                                // NOTE:  errstate already filled in...
                                LOG(bb,error) << "Handle " << pHandle << " could not be added to the LVKey metadata for the compute node.";
                            }
                        }
                    }
                    delete[] l_ContribArray;
                    l_ContribArray = 0;
                }
            }
        }
    }

    if (!rc && l_HandleWasAdded) {
        // The handle was added to a new LVKey...  Find that LVKey and set the return data...
        // NOTE: If we didn't find the handle at all above, we won't find it this time either
        //       and return that indication.
        for(auto it = tagInfoMap2.begin(); it != tagInfoMap2.end(); ++it) {
            if((it->first).first == pConnectionName && (it->second).getJobId() == pJob.getJobId()) {
                if(((it->second).getTagInfo(pHandle, pContribId, l_TagId, l_TagInfo))) {
                    pLVKey = it->first;
                    pTagInfo2 = &(it->second);
                    pTagInfo = l_TagInfo;
                    pTagId = l_TagId;
                    pContrib = pTagInfo->getExpectContrib();
                    rc = 1;
                    break;
                }
            }
        }
    }

    return rc;
}

// NOTE:  This method only returns the LVKey given the jobid and contribid...
int BBTagInfoMap2::getLVKey(const std::string& pConnectionName, LVKey* &pLVKey, const uint64_t pJobId, const uint32_t pContribId) {
    int rc = -2;    // LVKey not registered with bbserver
    for(auto it = tagInfoMap2.begin(); it != tagInfoMap2.end(); ++it)
    {
        // NOTE:  Connection name can come in as an empty string when invoked as part of processAsyncRequest()...
        if ((pConnectionName.size() == 0 || (it->first).first == pConnectionName) && ((it->second).getJobId() == pJobId)) {
            // Correct connection and correct jobid...
            if ((it->second).hasContribId(pContribId)) {
                rc = 1;
                *pLVKey = it->first;
                break;
            }
        }
    }

    return rc;
}

// NOTE:  This method returns the LVKey and BBTagInfo given the jobid, jobstepid, tab, numcontrib and contrib[] values...
int BBTagInfoMap2::getLVKey(const std::string& pConnectionName, LVKey* &pLVKey, BBTagInfo* &pTagInfo, const BBJob pJob, const uint64_t pTag, const uint64_t pNumContrib, const uint32_t pContrib[]) {
    int rc = -2;    // LVKey not registered with bbserver
    for(auto it = tagInfoMap2.begin(); it != tagInfoMap2.end(); ++it) {
        if (((it->first).first == pConnectionName) && ((it->second).getJobId() == pJob.getJobId())) {
            // Correct connection and correct jobid...
            rc = (it->second).getTagInfo(pTagInfo, pJob, pTag, pNumContrib, pContrib);
            // Return code values:
            // -1 = Contrib list did not match
            //  0 = Tag value did not match
            //  1 = Tag and contrib list matched
            *pLVKey = it->first;
            break;
        }
    }

    return rc;
}

BBTagInfo2* BBTagInfoMap2::getTagInfo2(const LVKey* pLVKey) const {
    for(auto it =  tagInfoMap2.begin(); it != tagInfoMap2.end(); ++it) {
        if (it->first == *pLVKey) {
            return const_cast <BBTagInfo2*> (&(it->second));
        }
    }

    return (BBTagInfo2*)0;
}

size_t BBTagInfoMap2::getTotalTransferSize(const LVKey& pLVKey) {
    if (tagInfoMap2.find(pLVKey) != tagInfoMap2.end()) {
        return tagInfoMap2[pLVKey].getTotalTransferSize();
    } else {
        return 0;
    }
}

int BBTagInfoMap2::getTransferHandle(uint64_t& pHandle, const LVKey* pLVKey, const BBJob pJob, const uint64_t pTag, const uint64_t pNumContrib, const uint32_t pContrib[]) {
    int rc = 0;

    if (tagInfoMap2.find(*pLVKey) != tagInfoMap2.end()) {
        rc = tagInfoMap2[*pLVKey].getTransferHandle(pHandle, pLVKey, pJob, pTag, pNumContrib, pContrib);
    } else {
        pHandle = 0;
    }

    return rc;
}

void BBTagInfoMap2::getTransferHandles(std::vector<uint64_t>& pHandles, const BBJob pJob, const BBSTATUS pMatchStatus) {
    for(auto it =  tagInfoMap2.begin(); it != tagInfoMap2.end(); ++it) {
        it->second.getTransferHandles(pHandles, pJob, pMatchStatus, it->second.stageOutStarted());
    }

    return;
}

void BBTagInfoMap2::removeAllLogicalVolumesForUuid(const string& pHostName, const LVKey* pLVKey, const uint64_t pJobId)
{
    int rc = 0;
    stringstream errorText;

    LVKey l_LVKeyStg = LVKey();
    LVKey* l_LVKey = &l_LVKeyStg;
    LVKey l_LVKeyStg2;
    LVKey* l_LVKey2 = &l_LVKeyStg2;

    try
    {
        if (pLVKey)
        {
            l_LVKey = const_cast<LVKey*>(pLVKey);
            do
            {
                rc = getAnyLVKeyForUuidAndJobId(l_LVKey2, l_LVKey, pJobId);
                switch (rc)
                {
                    case -2:
                    {
                        // Cannot find an LVKey registered for this jobid
                        // NOTE: This is normal for the processAsyncRequest() case when this
                        //       LVKey was never associated with this bbServer...  Just exit...
                        rc = 0;
                        BAIL;
                        break;
                    }

                    case 1:
                    {
                        // LVKey was found on this bbServer...
                        rc = 0;
                        break;
                    }

                    default:
                    {
                        errorText << "Error when attempting to find a local LVKey with same LV uuid as " << *l_LVKey << ", for jobid " << pJobId;
                        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                    }
                }

                // Perform any cleanup of the local bbServer metadata
                ensureStageOutEnded(l_LVKey2);

                // Remove the LVKey value...
                removeLVKey(pJobId, l_LVKey2);
            }
            while (1);
        }
        else
        {
            rc = -1;
            errorText << "BBTagInfoMap2::removeAllLogicalVolumesForUuid(): LVKey passed as NULL with jobid " << pJobId;
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    if ((!rc) && sameHostName(pHostName))
    {
        // NOTE: It is possible for a given LVKey to be found in more than one bbServer.
        //       Append the remove logical volume operation to the async request file.
        char l_lvuuid_str[LENGTH_UUID_STR] = {'\0'};
        (*l_LVKey).second.copyTo(l_lvuuid_str);
        char l_AsyncCmd[AsyncRequest::MAX_DATA_LENGTH] = {'\0'};
        snprintf(l_AsyncCmd, sizeof(l_AsyncCmd), "removelogicalvolume %lu 0 0 0 0 %s %s", pJobId, pHostName.c_str(), l_lvuuid_str);
        AsyncRequest l_Request = AsyncRequest(l_AsyncCmd);
        wrkqmgr.appendAsyncRequest(l_Request);
    }

    return;
}

void BBTagInfoMap2::removeLVKey(const uint64_t pJobId, const LVKey* pLVKey)
{
    LOG(bb,info) << "taginfo: Removing " << *pLVKey << " for jobid " << pJobId;
    tagInfoMap2.erase(*pLVKey);

    return;
}

int BBTagInfoMap2::retrieveTransfers(BBTransferDefs& pTransferDefs)
{
    int rc = 0;

    if (pTransferDefs.getHostName() != UNDEFINED_HOSTNAME)
    {
        // For now, we only attempt to get the data from the local metadata if
        // we have a unique hostname provided
        bool l_HostNameFound = false;
        for (auto it = tagInfoMap2.begin(); (!rc) && it != tagInfoMap2.end(); ++it)
        {
            rc = it->second.retrieveTransfers(pTransferDefs);
            if (rc == 2 || pTransferDefs.getNumberOfDefinitions() > 0)
            {
                l_HostNameFound = true;
                rc = 0;
            }
        }

        if (rc == 0)
        {
            if (!l_HostNameFound)
            {
                // Must use cross-bbserver metadata
                rc = 1;
            }
        }

        if (rc != 0)
        {
            // We have to use the cross-bbserver metadata to
            // attempt to satisfy this query -or-
            // we took an error.  Empty any result we may have
            // collected.
            pTransferDefs.clear();
        }
    }
    else
    {
        // Must use cross-bbserver metadata
        rc = 1;
    }

    return rc;
}

void BBTagInfoMap2::sendTransferCompleteForHandleMsg(const string& pHostName, const string& pCN_HostName, const uint64_t pHandle, const BBSTATUS pStatus)
{
    int l_AppendAsyncRequestFlag = ASYNC_REQUEST_HAS_NOT_BEEN_APPENDED;
    for(auto it = tagInfoMap2.begin(); it != tagInfoMap2.end(); ++it)
    {
        it->second.sendTransferCompleteForHandleMsg(pHostName, pCN_HostName, &(it->first), pHandle, l_AppendAsyncRequestFlag, pStatus);
    }

    return;
}

void BBTagInfoMap2::setCanceled(const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle)
{
    for(auto it = tagInfoMap2.begin(); it != tagInfoMap2.end(); ++it)
    {
        it->second.setCanceled(&(it->first), pJobId, pJobStepId, pHandle);
    }

    return;
}

int BBTagInfoMap2::setSuspended(const string& pHostName, const string& pCN_HostName, const int pValue)
{
    int rc = 0;
    uint32_t l_NumberAlreadySet = 0;
    uint32_t l_NumberOfQueuesNotMatchingHostNameCriteria = 0;
    uint32_t l_NumberOfQueuesNotFoundForLVKey = 0;
    uint32_t l_NumberSet = 0;
    uint32_t l_NumberFailed = 0;

    for(auto it = tagInfoMap2.begin(); it != tagInfoMap2.end(); ++it)
    {
        rc = it->second.setSuspended(&(it->first), pCN_HostName, pValue);
        switch (rc)
        {
            case 2:
            {
                // Value was already set for this work queue.
                // Continue to the next LVKey...
                LOG(bb,info) << "BBTagInfoMap2::setSuspended(): Queue for hostname " << it->second.getHostName() << ", " << it->first \
                             << " was already " << (pValue ? "inactive" : "active");
                ++l_NumberAlreadySet;

                break;
            }

            case 1:
            {
                // Hostname did not match...  Continue to the next LVKey...
                LOG(bb,debug) << "BBTagInfoMap2::setSuspended(): Queue for hostname " << it->second.getHostName() << ", " << it->first \
                              << " did not match the host name criteria";
                ++l_NumberOfQueuesNotMatchingHostNameCriteria;

                break;
            }

            case 0:
            {
                // If the hostname matched, the suspended value was successfully set
                // for the work queue.  Already logged...  Continue to the next LVKey...
                ++l_NumberSet;

                break;
            }

            case -2:
            {
                // Work quque not found for hostname/LVKey...  Continue to the next LVKey...
                LOG(bb,debug) << "BBTagInfoMap2::setSuspended(): Work queue for hostname " << it->second.getHostName() << ", " << it->first \
                              << " was not found";
                ++l_NumberOfQueuesNotFoundForLVKey;

                break;
            }

            default:
            {
                // Error occurred....  It was already logged...  Continue...
                ++l_NumberFailed;

                break;
            }
        }
        rc = 0;
    }

    string l_HostNamePrt = "For CN host name " + pCN_HostName + ", ";
    if (!pCN_HostName.size())
    {
        l_HostNamePrt = "For all CN host names, ";
    }

    string l_Operation;
    l_Operation = (pValue ? "suspended" : "resumed");
    bberror.errdirect("out.queuesAlreadySet", l_NumberAlreadySet);
    bberror.errdirect("out.queuesSet", l_NumberSet);
    bberror.errdirect("out.queuesNotFoundForLVKey", l_NumberOfQueuesNotFoundForLVKey);
    bberror.errdirect("out.queuesNotMatchingHostNameCriteria", l_NumberOfQueuesNotMatchingHostNameCriteria);
    bberror.errdirect("out.queuesFailed", l_NumberFailed);
    bberror.errdirect("out.operation", l_Operation);
    LOG(bb,info) << l_HostNamePrt << l_NumberSet << " work queue(s) were " << l_Operation \
                 << ", " << l_NumberAlreadySet << " work queue(s) were already in a " << l_Operation << " state, "
                 << l_NumberOfQueuesNotMatchingHostNameCriteria << " work queue(s) did not match the hostname selection criteria, " \
                 << l_NumberOfQueuesNotFoundForLVKey << " expected work queues were not found for the LVKey, and "
                 << l_NumberFailed << " work queue(s) failed. See previous messages for additional details.";

    if (sameHostName(pHostName))
    {
        // NOTE: It is possible for a given hostname to be found in more than one bbServer.
        //       Append the suspend operation to the async request file.
        char l_AsyncCmd[AsyncRequest::MAX_DATA_LENGTH] = {'\0'};
        snprintf(l_AsyncCmd, sizeof(l_AsyncCmd), "setsuspend %lu 0 0 0 0 %s None", (uint64_t)pValue, (pCN_HostName.size() ? pCN_HostName.c_str() : "''"));
        AsyncRequest l_Request = AsyncRequest(l_AsyncCmd);
        wrkqmgr.appendAsyncRequest(l_Request);
    }

    return rc;
}

int BBTagInfoMap2::stopTransfer(const string& pHostName, const string& pCN_HostName, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId)
{
    int rc = 0;

    string l_ServerHostName;
    activecontroller->gethostname(l_ServerHostName);
    string l_Result = ", the transfer definition was not found on the bbServer at " + l_ServerHostName;

    for(auto it = tagInfoMap2.begin(); ((!rc) && it != tagInfoMap2.end()); ++it)
    {
        rc = it->second.stopTransfer(&(it->first), pCN_HostName, pJobId, pJobStepId, pHandle, pContribId);
        switch (rc)
        {
            case 0:
            {
                // The transfer definition being searched for could not be found with this LVKey.
                // Continue to the next LVKey...

                break;
            }

            case 1:
            {
                // Found the transfer definition on this bbServer.
                // It was processed, and operation logged.
                // The cross bbserver metadata was also appropriately reset
                // as part of the operation.
                l_Result = ", the transfer definition was successfully stopped.";

                break;
            }

            case 2:
            {
                // Found the transfer definition on this bbServer.
                // However, no extents were left to be transferred.
                // Situation was logged, and nothing more to do...
                l_Result = ", the transfer definition was found and had already finished.";

                break;
            }

            case 3:
            {
                // Found the transfer definition on this bbServer.
                // However, it was already in a stopped state.
                // Situation was logged, and nothing more to do...
                l_Result = ", the transfer definition was already stopped.";

                break;
            }

            case 4:
            {
                // Found the transfer definition on this bbServer.
                // However, it was already in a canceled state.
                // Situation was logged, and nothing more to do...
                l_Result = ", the transfer definition was already canceled.";

                break;
            }

            case 5:
            {
                // Found the transfer definition on this bbServer.
                // However, extents had not yet been scheduled.
                // Situation was logged, and nothing more to do...
                l_Result = ", the transfer definition did not yet have any extents scheduled for transfer. A start transfer request was caught in mid-flight and the original request was issued to the new bbServer to complete the trasnfer request.";

                break;
            }

            default:
            {
                // Error occurred....  Log it and continue...
                l_Result = ", processing during the search for the transfer definition caused a failure.";
                LOG(bb,error) << "Failed when processing a stop transfer request for CN hostname " << pCN_HostName << ", jobid " << pJobId << ", jobstepid " << pJobStepId
                              << ", handle " << pHandle << ", contribId " << pContribId << ", rc=" << rc << ". Stop transfer processing continues for the remaining transfer definitions in the set.";

                break;
            }
        }
    }

    LOG(bb,info) << "For host name " << pCN_HostName << ", jobid " << pJobId << ", jobstepid " << pJobStepId \
                 << ", handle " << pHandle << ", and contribid " << pContribId << l_Result;

    if (sameHostName(pHostName))
    {
        // NOTE: It is possible for a given hostname to be found in more than one bbServer.
        //       Append the stop operation for the stop transfer request to the async request file.
        char l_AsyncCmd[AsyncRequest::MAX_DATA_LENGTH] = {'\0'};
        snprintf(l_AsyncCmd, sizeof(l_AsyncCmd), "cancel %lu %lu %lu %u %lu stoprequest %s", pJobId, pJobStepId, pHandle, pContribId, (uint64_t)BBSCOPETRANSFER, (pCN_HostName.size() ? pCN_HostName.c_str() : "''"));
        AsyncRequest l_Request = AsyncRequest(l_AsyncCmd);
        wrkqmgr.appendAsyncRequest(l_Request);
    }

    return rc;
}

// NOTE:  This method verifies that the input jobid exists for some LVKey...
int BBTagInfoMap2::verifyJobIdExists(const std::string& pConnectionName, const LVKey* pLVKey, const uint64_t pJobId)
{
    int rc = 0;
    for (auto it = tagInfoMap2.begin(); it != tagInfoMap2.end(); ++it)
    {
        if ((it->second).getJobId() == pJobId)
        {
            rc = 1;
            break;
        }
    }

    return rc;
}

