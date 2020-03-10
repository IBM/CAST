/*******************************************************************************
 |    BBLV_Metadata.cc
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
#include <identity.h>
#include <unistd.h>

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

#include "bberror.h"
#include "bbinternal.h"
#include "BBLV_Metadata.h"
#include "bbserver_flightlog.h"
#include "bbwrkqmgr.h"
#include "LVKey.h"
#include "LVUuidFile.h"
#include "Uuid.h"


namespace bfs = boost::filesystem;

//
// BBLV_Metadata class
//

//
// BBLV_Metadata - Static data/members
//

void BBLV_Metadata::appendAsyncRequestForStopTransfer(const string& pCN_HostName, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId, const uint64_t pCancelScope)
{
    char l_AsyncCmd[AsyncRequest::MAX_DATA_LENGTH] = {'\0'};
    snprintf(l_AsyncCmd, sizeof(l_AsyncCmd), "cancel %lu %lu %lu %u %lu stoprequest %s", pJobId, pJobStepId, pHandle, pContribId, pCancelScope, (pCN_HostName.size() ? pCN_HostName.c_str() : "''"));
    AsyncRequest l_Request = AsyncRequest(l_AsyncCmd);
    wrkqmgr.appendAsyncRequest(l_Request);

    return;
}

int BBLV_Metadata::update_xbbServerAddData(txp::Msg* pMsg, const uint64_t pJobId)
{
    int rc = 0;
    stringstream errorText;

    int l_SwitchUsers = 0;

    try
    {
        if (pJobId != UNDEFINED_JOBID)
        {
            bfs::path job(g_BBServer_Metadata_Path);
            job /= bfs::path(to_string(pJobId));

            if (!access(job.c_str(), F_OK))
            {
                // NOTE: This is the normal case for the restart of a transfer definition...
                unsigned count = 0;
                for(auto& elements: boost::make_iterator_range(bfs::directory_iterator(job), {}))
                {
                    LOG(bb,debug) << "update_xbbServerAddData(): Found jobstep" << elements;
                    ++count;
                }
                LOG(bb,info) << "xbbServer: JobId " << pJobId << " is already registered and currently has " << count << " associated job step(s)";
            }
            else
            {
                // Switch to root:root
                // NOTE:  Must do this so we can insert into the cross bbserver
                //        metadata directory, which has permissions of 755.
                l_SwitchUsers = 1;
                becomeUser(0,0);

                // Create the jobid directory
                // NOTE: umask of 0027 yields permissions of 0750 for jobid directory
                rc = mkdir(job.c_str(), (mode_t)0777);

                if (!rc)
                {
                    // Unconditionally perform a chown for the jobid directory to the uid:gid of the mountpoint.
                    int rc = chown(job.c_str(), (uid_t)((txp::Attr_uint32*)pMsg->retrieveAttrs()->at(txp::mntptuid))->getData(),
                                   (gid_t)((txp::Attr_uint32*)pMsg->retrieveAttrs()->at(txp::mntptgid))->getData());
                    if (rc)
                    {
                        rc = -1;
                        int l_Errno = errno;
                        bfs::remove_all(job);
                        bberror << err("error.path", job.string());
                        errorText << "chown failed for the jobid directory at " << job.string() \
                                  << ", errno " << l_Errno << " (" << strerror(l_Errno) << ")";
                        LOG_ERROR_TEXT_ERRNO(errorText, l_Errno);
                        SET_RC_AND_BAIL(rc);
                    }
                }
                else
                {
                    if (errno != EEXIST)
                    {
                        rc = -1;
                        bberror << err("error.path", job.c_str());
                        errorText << "mkdir failed for jobid directory at " << job.string() \
                                  << ", errno " << errno << " (" << strerror(errno) << ")";
                        LOG_ERROR_TEXT_ERRNO(errorText, errno);
                        SET_RC_AND_BAIL(rc);
                    }
                    else
                    {
                        // Tolerate if the jobid directory already exists.  There exists a possible
                        // race condition between CNs when registering their logical volumes.
                    }
                }

                // Switch back to the correct uid:gid
                switchIdsToMountPoint(pMsg);
                l_SwitchUsers = 0;
            }
        }
        else
        {
            rc = -1;
            errorText << "BBLV_Metadata::update_xbbServerAddData(): Attempt to add invalid jobid of " << UNDEFINED_JOBID << " to the cross bbServer metadata";
            bberror << err("error.jobid", UNDEFINED_JOBID);
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    if (l_SwitchUsers)
    {
        switchIdsToMountPoint(pMsg);
    }

    return rc;
}

int BBLV_Metadata::update_xbbServerRemoveData(const uint64_t pJobId) {
    int rc = 0;
    stringstream errorText;

    try
    {
        bfs::path job(g_BBServer_Metadata_Path);
        job /= bfs::path(to_string(pJobId));

        if (!g_AsyncRemoveJobInfo)
        {
            unlockLocalMetadata((LVKey*)0, "update_xbbServerRemoveData - bfs::remove_all(job)");
            bfs::remove_all(job);
            lockLocalMetadata((LVKey*)0, "update_xbbServerRemoveData - bfs::remove_all(job)");
        }
        else
        {
            bfs::path hidejob(g_BBServer_Metadata_Path);
            string hidejobname = "." + to_string(pJobId);
            hidejob /= bfs::path(hidejobname);
            rc = rename(job.c_str(), hidejob.c_str());
            if (rc)
            {
                // NOTE: There is a window between checking for the job above and subsequently renaming/removing
                //       the job directory.  This is the most likely exception...  Return -2...
                //       Also, if a script sends a RemoveJobInfo command to each CN, it is a big race condition
                //       as to which bbServer actually removes the job from the cross bbServer metadata and
                //       which servers 'may' take an exception trying to concurrently remove the data.
                //       Simply log this as an info...
                rc = -2;
                errorText << "JobId " << pJobId << " was not found in the cross-bbServer metadata";
                LOG_INFO_TEXT_RC(errorText, rc);
            }
        }
        LOG(bb,info) << "JobId " << pJobId << " was removed from the cross-bbServer metadata";
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        // NOTE: There is a window between checking for the job above and subsequently renaming/removing
        //       the job directory.  This is the most likely exception...  Return -2...
        //       Also, if a script sends a RemoveJobInfo command to each CN, it is a big race condition
        //       as to which bbServer actually removes the job from the cross bbServer metadata and
        //       which servers 'may' take an exception trying to concurrently remove the data.
        //       Simply log this as an info...
        rc = -2;
        errorText << "JobId " << pJobId << " was not found in the cross-bbServer metadata (via exception)";
        LOG_INFO_TEXT_RC(errorText, rc);
        // LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    return rc;
}

//
// BBLV_Metadata - Non-static members
//

void BBLV_Metadata::accumulateTotalLocalContributorInfo(const uint64_t pHandle, size_t& pTotalContributors, size_t& pTotalLocalReportingContributors)
{
    pTotalContributors = 0;
    pTotalLocalReportingContributors = 0;

    int l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded((LVKey*)0, "BBLV_Metadata::accumulateTotalLocalContributorInfo");

    for(auto it = metaDataMap.begin(); it != metaDataMap.end(); ++it)
    {
        it->second.accumulateTotalLocalContributorInfo(pHandle, pTotalContributors, pTotalLocalReportingContributors);
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata((LVKey*)0, "BBLV_Metadata::accumulateTotalLocalContributorInfo");
    }

    return;
}

int BBLV_Metadata::addLVKey(const string& pHostName, txp::Msg* pMsg, const LVKey* pLVKey, const uint64_t pJobId, BBLV_Info& pLV_Info, const TOLERATE_ALREADY_EXISTS_OPTION pTolerateAlreadyExists)
{
    int rc = 0;
    stringstream errorText;

    for(auto it = metaDataMap.begin(); it != metaDataMap.end(); ++it)
    {
        if (it->first == *pLVKey)
        {
            errorText << *pLVKey << " already exists.  Registration failed with bbserver.";
            if (!pTolerateAlreadyExists)
            {
                rc = -1;
                LOG_ERROR_TEXT_RC(errorText, rc);
            }
            else
            {
                rc = -2;
                LOG(bb,info) << errorText.str() << " Tolerated exception.";
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
                    LOG_ERROR_TEXT_RC(errorText, rc);
                }
                else
                {
                    rc = -2;
                    LOG(bb,info) << errorText.str() << " Tolerated exception.";
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
                    LOG_INFO_TEXT(errorText);
                    rc = 1;
                    break;
                }
            }
        }
    }

    if (!rc)
    {
        metaDataMap[*pLVKey] = pLV_Info;
        // NOTE: Return a pointer to the version of LV_Info in metaDataMap
        BBLV_Info* l_LV_Info = getLV_Info(pLVKey);
        if (l_LV_Info)
        {
            // NOTE: We overload the TOLERATE_ALREADY_EXISTS_OPTION option that is passed in to this method.
            //       This option is only passed in as non-zero if this work queue is being added for a restart case.
            //       Therefore, if non-zero, we indicate on the addWrkQ() invocation to create the work queue as suspended.
            // NOTE: In the restart case, we create the work queue as suspended directly, as the setSuspended() invocation
            //       could fail (not likely) and the local metadata would not get updated.  In the whole scheme of things,
            //       that is not critical as a resume operation for the host (that is likely coming in the future...)
            //       should then enable the work queue object.  While any restarted transfer definitions would fail (local
            //       metadata would indicate the host is not suspended), once officially resumed, any new start transfer
            //       requests would succeed.
            // NOTE: In the restart case, if the work queue already exists (fail over and then back for the same LVKey),
            //       the setSuspended() invocation will set the metadata and the work queue object as suspended.
            int l_SuspendOption = (pTolerateAlreadyExists ? 1 : 0);
            rc = wrkqmgr.addWrkQ(pLVKey, l_LV_Info, pJobId, l_SuspendOption);
            LOCAL_METADATA_RELEASED l_Local_Metadata_Lock_Released;
            pLV_Info.getExtentInfo()->setSuspended(pLVKey, l_LV_Info->getHostName(), pJobId, l_Local_Metadata_Lock_Released, l_SuspendOption);
            if (!rc)
            {
                // NOTE: If necessary, errstate will be filled in by update_xbbServerAddData()
                rc = update_xbbServerAddData(pMsg, pJobId);
            }
        }
        else
        {
            rc = -1;
            errorText << "Could not find the local metadata for " << *pLVKey << ".  Registration failed with bbserver.";
            LOG_ERROR_TEXT_RC(errorText, rc);
        }
    }
    else if (rc == -2)
    {
        // Reset return code for tolerated exceptions
        rc = 0;
    }

    if (rc == 0 || rc == -2)
    {
        LOG(bb,info) << "BBLV_Metadata::addLVKey(): Adding " << *pLVKey << " from host " << pLV_Info.getHostName() \
                     << " for jobid " << pJobId << ". LVKey entry was " << (rc ? "found and reused from" : "created new into") << " the local cache.";
    }

    return rc;
}

#define ATTEMPTS 10
int BBLV_Metadata::attemptToUnconditionallyStopThisTransferDefinition(const string& pHostName, const string& pCN_HostName, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId)
{
    int rc = 0;
    stringstream errorText;

    HandleFile* l_HandleFile = NULL;
    ContribIdFile* l_ContribIdFile = NULL;
    char* l_HandleFileName = NULL;
    int l_Attempts = ATTEMPTS;

    std::string l_HostName;
    activecontroller->gethostname(l_HostName);

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, LVM_UnconditionallyStop, "BBLV_Metadata attempt to unconditionally stop transfer definition, counter=%ld, jobid=%ld, handle=%ld, contribid=%ld", l_FL_Counter, pJobId, pHandle, pContribId);

    // NOTE: The handle file is locked exclusive here to serialize between this bbServer and another
    //       bbServer that is marking the handle/contribid file as 'stopped'
    HANDLEFILE_LOCK_FEEDBACK l_LockFeedback;
    rc = HandleFile::loadHandleFile(l_HandleFile, l_HandleFileName, pJobId, pJobStepId, pHandle, LOCK_HANDLEFILE, &l_LockFeedback);
    if (!rc)
    {
        bfs::path l_HandleFilePath = bfs::path(string(l_HandleFileName)).parent_path();

        bool l_AllDone = false;
        while ((!l_AllDone) && --l_Attempts)
        {
            l_AllDone = true;
            // Iterate through the logical volumes...
            bool l_ContribIdFound = false;
            for (auto& l_LVUuid : boost::make_iterator_range(bfs::directory_iterator(l_HandleFilePath), {}))
            {
                try
                {
                    if (!pathIsDirectory(l_LVUuid)) continue;
                    bfs::path lvuuidfile = l_LVUuid.path() / bfs::path("^" + l_LVUuid.path().filename().string());
                    LVUuidFile l_LVUuidFile;
                    rc = l_LVUuidFile.load(lvuuidfile.string());
                    if (!rc)
                    {
                        if (l_LVUuidFile.hostname == pCN_HostName)
                        {
                            // CN of interest
                            LVKey l_LVKey = std::pair<string, Uuid>(l_LVUuidFile.connectionName, Uuid(l_LVUuid.path().filename().c_str()));
                            if (l_ContribIdFile)
                            {
                                delete l_ContribIdFile;
                                l_ContribIdFile = NULL;
                            }
                            rc = ContribIdFile::loadContribIdFile(l_ContribIdFile, &l_LVKey, l_HandleFilePath, pContribId);
                            if (rc == 1)
                            {
                                rc = 0;
                                if (l_ContribIdFile)
                                {
                                    // Valid contribid file
                                    l_ContribIdFound = true;
                                    if (l_ContribIdFile->hostname == l_HostName)
                                    {
                                        // This bbServer previously serviced this transfer definition
                                        if (!l_ContribIdFile->stopped())
                                        {
                                            // Contribid is not marked as stopped
                                            if (!l_ContribIdFile->notRestartable())
                                            {
                                                // Contribid is restartable
                                                rc = doForceStopTransfer(&l_LVKey, l_ContribIdFile, pJobId, pJobStepId, pHandle, pContribId);
                                            }
                                        }
                                        else
                                        {
                                            // Transfer definition is already stopped
                                            // NOTE:  This probably means that the 'new' bbServer has already declared this bbServer as dead
                                            //        and has unconditionaly stopped this transfer definition.
                                            LOG(bb,info) << "attemptToUnconditionallyStopThisTransferDefinition(): Transfer definition is already stopped. " \
                                                         << "Input: hostname " << pHostName << ", CN_hostname " << pCN_HostName << ", jobid " << pJobId \
                                                         << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", contribid " << pContribId;
                                        }
                                    }
                                    else
                                    {
                                        // This bbServer was not the servicer for this transfer definition
                                        LOG(bb,debug) << "attemptToUnconditionallyStopThisTransferDefinition(): This bbServer was not the servicer for this transfer definition. " \
                                                      << "Input: hostname " << pHostName << ", CN_hostname " << pCN_HostName << ", jobid " << pJobId \
                                                      << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", contribid " << pContribId;
                                    }
                                }
                                else
                                {
                                    // ContribIdFile could not be loaded
                                    LOG(bb,info) << "attemptToUnconditionallyStopThisTransferDefinition(): ContribIdFile could not be loaded (NULL pointer returned). " \
                                                 << "Input: hostname " << pHostName << ", CN_hostname " << pCN_HostName << ", jobid " << pJobId \
                                                 << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", contribid " << pContribId;
                                }
                            }
                            else
                            {
                                // ContribIdFile could not be loaded
                                LOG(bb,info) << "attemptToUnconditionallyStopThisTransferDefinition(): ContribIdFile could not be loaded (rc " << rc << "). " \
                                             << "Input: hostname " << pHostName << ", CN_hostname " << pCN_HostName << ", jobid " << pJobId \
                                             << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", contribid " << pContribId;
                            }
                        }
                        else
                        {
                            // This LV was not for the CN hostname (likely, normal)
                        }
                    }
                    else
                    {
                        l_AllDone = false;
                        rc = 0;
                        break;
                    }
                }
                catch(ExceptionBailout& e) { }
                catch(exception& e)
                {
                    // More than likely, the cross-bbServer data was altered under us...  Restart the walk of the LVUuid directories...
                    l_AllDone = false;
                    rc = 0;
                    break;
                }

                if (l_ContribIdFound)
                {
                    l_AllDone = true;
                    break;
                }
            }
        }
    }
    else
    {
        // Could not lock the handle file
        LOG(bb,error) << "attemptToUnconditionallyStopThisTransferDefinition(): Could not lock the handle file for jobid " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", contribid " << pContribId;
    }

    if (l_HandleFile)
    {
        l_HandleFile->close(l_LockFeedback);
        delete l_HandleFile;
        l_HandleFile = NULL;
    }

    if (l_HandleFileName)
    {
        delete[] l_HandleFileName;
        l_HandleFileName = 0;
    }

    if (l_ContribIdFile)
    {
        delete l_ContribIdFile;
        l_ContribIdFile = NULL;
    }

    FL_Write6(FLMetaData, LVM_UnconditionallyStop_End, "BBLV_Metadata attempt to unconditionally stop transfer definition, counter=%ld, jobid=%ld, handle=%ld, contribid=%ld, attempts=%ld, rc=%ld",
              l_FL_Counter, pJobId, pHandle, pContribId, (uint64_t)(ATTEMPTS-l_Attempts), rc);

    return rc;
}
#undef ATTEMPTS

int BBLV_Metadata::cleanLVKeyOnly(const LVKey* pLVKey) {
    int rc = 0;

    BBLV_Info* l_LV_Info = getLV_Info(pLVKey);
    if (l_LV_Info) {
        metaDataMap.erase(*pLVKey);
    } else {
        rc = -1;
    }

    return rc;
}

void BBLV_Metadata::cleanUpAll(const uint64_t pJobId) {

    // Ensure stage-out ended for all LVKeys under the job
    LOCAL_METADATA_RELEASED l_LockWasReleased = LOCAL_METADATA_LOCK_NOT_RELEASED;

    bool l_Restart = true;
    while (l_Restart)
    {
        l_Restart = false;
        l_LockWasReleased = LOCAL_METADATA_LOCK_NOT_RELEASED;
        for (auto it = metaDataMap.begin(); it != metaDataMap.end(); ++it)
        {
            if ((it->second).getJobId() == pJobId)
            {
                (it->second).ensureStageOutEnded(&(it->first), l_LockWasReleased);
                if (l_LockWasReleased == LOCAL_METADATA_LOCK_NOT_RELEASED)
                {
                    it = metaDataMap.erase(it);
                }

                // NOTE: Reset CurrentWrkQE for the next iteration...
                CurrentWrkQE = NULL;

                l_Restart = true;
                break;
            }
        }
    }

    return;
}

void BBLV_Metadata::dump(char* pSev, const char* pPrefix)
{
    if (wrkqmgr.checkLoggingLevel(pSev))
    {
        int l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded((LVKey*)0, "BBLV_Metadata::dump");

        if (metaDataMap.size())
        {
            char l_Temp[LENGTH_UUID_STR] = {'\0'};
            if (!strcmp(pSev,"debug"))
            {
                LOG(bb,debug) << "";
                LOG(bb,debug) << ">>>>> Start: " << (pPrefix ? pPrefix : "BBLV_Metadata") << ", " \
                              << metaDataMap.size() << (metaDataMap.size()==1 ? " entry <<<<<" : " entries <<<<<");
                for (auto& it : metaDataMap)
                {
                    const_cast <Uuid*> (&(it.first.second))->copyTo(l_Temp);
                    LOG(bb,debug) << "LVKey -> Local Port: " << it.first.first << "   Uuid: " << l_Temp;
                    it.second.dump(pSev);
                }
                LOG(bb,debug) << ">>>>>   End: " << (pPrefix ? pPrefix : "BBLV_Metadata") << ", " \
                              << metaDataMap.size() << (metaDataMap.size()==1 ? " entry <<<<<" : " entries <<<<<");
                LOG(bb,debug) << "";
            }
            else if (!strcmp(pSev,"info"))
            {
                LOG(bb,info) << "";
                LOG(bb,info) << ">>>>> Start: " << (pPrefix ? pPrefix : "BBLV_Metadata") << ", " \
                             << metaDataMap.size() << (metaDataMap.size()==1 ? " entry <<<<<" : " entries <<<<<");
                for (auto& it : metaDataMap)
                {
                    const_cast <Uuid*> (&(it.first.second))->copyTo(l_Temp);
                    LOG(bb,info) << "LVKey -> Local Port: " << it.first.first << "   Uuid: " << l_Temp;
                    it.second.dump(pSev);
                }
                LOG(bb,info) << ">>>>>   End: " << (pPrefix ? pPrefix : "BBLV_Metadata") << ", " \
                             << metaDataMap.size() << (metaDataMap.size()==1 ? " entry <<<<<" : " entries <<<<<");
                LOG(bb,info) << "";
            }
        }

        if (l_LocalMetadataWasLocked)
        {
            unlockLocalMetadata((LVKey*)0, "BBLV_Metadata::dump");
        }
    }
}

void BBLV_Metadata::ensureStageOutEnded(const LVKey* pLVKey) {

    // Ensure stage-out ended for the given LVKey
    LOCAL_METADATA_RELEASED l_LockWasReleased = LOCAL_METADATA_LOCK_NOT_RELEASED;
    for (auto it = metaDataMap.begin(); it != metaDataMap.end(); ++it)
    {
        if ((it->first) == *pLVKey)
        {
            (it->second).ensureStageOutEnded(&(it->first), l_LockWasReleased);
            break;
        }
    }

    return;
}

BBLV_Info* BBLV_Metadata::getAnyLV_InfoForUuid(const LVKey* pLVKey) const {
    BBLV_Info* l_TagInfo = (BBLV_Info*)0;

    int l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded(pLVKey, "BBLV_Metadata::getAnyLV_InfoForUuid");
    for(auto it = metaDataMap.begin(); it != metaDataMap.end(); ++it) {
        if ((it->first).second == pLVKey->second) {
            l_TagInfo = const_cast <BBLV_Info*> (&(it->second));
            break;
        }
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata(pLVKey, "BBLV_Metadata::getAnyLV_InfoForUuid");
    }

    return l_TagInfo;
}

// NOTE:  This method returns any LVKey with the input LV Uuid and jobid...
int BBLV_Metadata::getAnyLVKeyForUuidAndJobId(LVKey* &pLVKeyOut, LVKey* &pLVKeyIn, const uint64_t pJobId) {
    int rc = -2;    // LVKey not registered with bbserver

    int l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded((LVKey*)0, "BBLV_Metadata::getAnyLVKeyForUuidAndJobId");
    for(auto it = metaDataMap.begin(); it != metaDataMap.end(); ++it)
    {
        if ((pLVKeyIn == NULL || pLVKeyIn->second == (it->first).second) && (pJobId == UNDEFINED_JOBID || pJobId == (it->second).getJobId()))
        {
            // Matching LV Uuid and acceptable jobid...
            rc = 1;
            *pLVKeyOut = it->first;
            break;
        }
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata((LVKey*)0, "BBLV_Metadata::getAnyLVKeyForUuidAndJobId");
    }

    return rc;
}

int BBLV_Metadata::getInfo(const std::string& pConnectionName, LVKey& pLVKey, BBLV_Info* &pLV_Info, BBTagInfo* &pTagInfo, BBTagID &pTagId, const BBJob pJob, std::vector<uint32_t>*& pContrib, const uint64_t pHandle, const uint32_t pContribId) {
    int rc = 0;
    LVKey l_LVKey;
    BBTagID l_TagId;
    uint64_t l_NumContrib = 0;
    uint64_t l_Handle = UNDEFINED_HANDLE;
    BBTagInfo* l_TagInfo = (BBTagInfo*)0;
    std::vector<uint32_t>* l_Contrib = 0;
    uint32_t* l_ContribArray = 0;

    // Returning a rc = 1, -> found
    //             rc = 0, -> not found
    //             rc < 0, -> error

    bool l_HandleWasAdded = false;
    uint64_t l_JobId = pJob.getJobId();

    int l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded(&pLVKey, "BBLV_Metadata::getInfo");

    for (auto it = metaDataMap.begin(); it != metaDataMap.end() && (!rc) && (!l_HandleWasAdded); ++it)
    {
        if ((it->second).getJobId() == l_JobId)
        {
            if ((it->second).getTagInfo(pHandle, pContribId, l_TagId, l_TagInfo))
            {
                // We found the handle that is currently associated with 'some' LVKey.
                if ((it->first).first == pConnectionName)
                {
                    // Correct LVKey...  Set the return data...
                    pLVKey = it->first;
                    pLV_Info = &(it->second);
                    pTagInfo = l_TagInfo;
                    pTagId = l_TagId;
                    pContrib = pTagInfo->getExpectContrib();
                    rc = 1;
                    break;
                }
                else
                {
                    l_Contrib = l_TagInfo->getExpectContrib();
                    l_TagInfo = 0;
                    l_NumContrib = (uint64_t)l_Contrib->size();
                    l_ContribArray = (uint32_t*)(new char[sizeof(uint32_t)*l_NumContrib]);
                    for (uint64_t i=0; i<l_NumContrib; ++i)
                    {
                        l_ContribArray[i] = (*l_Contrib)[i];
                    }
                    l_Handle = pHandle;
                    // Add this handle under the LVKey associated with the connection and jobid...
                    for (auto it2 = metaDataMap.begin(); it2 != metaDataMap.end(); ++it2)
                    {
                        if ((it2->first).first == pConnectionName && (it2->second).getJobId() == l_JobId)
                        {
                            // NOTE: We use the LVKey value from the current entry and
                            //       we use the tag value from that returned by getTagInfo
                            //       above for the 'incorrect' LVKey....
                            l_LVKey = it2->first;
                            BBTransferDef* l_TransferDef = 0;
                            uint32_t l_PerformOperationDummy = 0;

                            rc = queueTransfer(pConnectionName, &l_LVKey, pJob, l_TagId.getTag(), l_TransferDef, (int32_t)(-1), l_NumContrib, l_ContribArray, l_Handle, l_PerformOperationDummy, (vector<struct stat*>*)0);
                            if (!rc)
                            {
                                l_HandleWasAdded = true;
                            }
                            else
                            {
                                // NOTE:  errstate already filled in...
                                LOG(bb,error) << "Handle " << pHandle << " could not be added to the LVKey metadata for the compute node.";
                            }
                            break;
                        }
                    }
                    delete[] l_ContribArray;
                    l_ContribArray = 0;
                }
            }
        }
    }

    if (!rc && l_HandleWasAdded)
    {
        // The handle was added to a new LVKey...  Find that LVKey and set the return data...
        // NOTE: If we didn't find the handle at all above, we won't find it this time either
        //       and return that indication.
        for (auto it = metaDataMap.begin(); it != metaDataMap.end(); ++it)
        {
            if ((it->first).first == pConnectionName && (it->second).getJobId() == l_JobId)
            {
                if (((it->second).getTagInfo(pHandle, pContribId, l_TagId, l_TagInfo)))
                {
                    pLVKey = it->first;
                    pLV_Info = &(it->second);
                    pTagInfo = l_TagInfo;
                    pTagId = l_TagId;
                    pContrib = pTagInfo->getExpectContrib();
                    rc = 1;
                    break;
                }
            }
        }
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata(&pLVKey, "BBLV_Metadata::getInfo");
    }

    return rc;
}

// NOTE:  This method only returns the LVKey given the jobid and contribid...
int BBLV_Metadata::getLVKey(const std::string& pConnectionName, LVKey* &pLVKey, const uint64_t pJobId, const uint32_t pContribId) {
    int rc = -2;    // LVKey not registered with bbserver

    int l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded(pLVKey, "BBLV_Metadata::getLVKey_1");

    for(auto it = metaDataMap.begin(); it != metaDataMap.end(); ++it)
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

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata(pLVKey, "BBLV_Metadata::getLVKey_1");
    }

    return rc;
}

// NOTE:  This method returns the LVKey and BBTagInfo given the jobid, jobstepid, tab, numcontrib and contrib[] values...
int BBLV_Metadata::getLVKey(const std::string& pConnectionName, LVKey* &pLVKey, BBTagInfo* &pTagInfo, BBJob pJob, const uint64_t pTag, const uint64_t pNumContrib, const uint32_t pContrib[]) {
    int rc = -2;    // LVKey not registered with bbserver

    bool l_ConnectionNameFound = false;
    int l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded(pLVKey, "BBLV_Metadata::getLVKey_2");

    for (auto it = metaDataMap.begin(); it != metaDataMap.end(); ++it)
    {
        if ((it->first).first == pConnectionName)
        {
            l_ConnectionNameFound = true;
            if ((it->second).getJobId() == pJob.getJobId())
            {
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
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata(pLVKey, "BBLV_Metadata::getLVKey_2");
    }

    if (rc == -2)
    {
        stringstream l_JobStr;
        pJob.getStr(l_JobStr);
        LOG(bb,debug) << "BBLV_Metadata::getLVKey(): LVKey not found for connection " << pConnectionName \
                      << ", job" << l_JobStr.str() << ", tag " << pTag << ", number of contribs " << pNumContrib \
                      << ". Connection name was " << (l_ConnectionNameFound ? "" : "not ") << "found in the local cache.";
    }

    return rc;
}

BBLV_Info* BBLV_Metadata::getLV_Info(const LVKey* pLVKey) const {
    BBLV_Info* l_BBLV_Info = 0;

    int l_TransferQueueWasUnlocked = unlockTransferQueueIfNeeded(pLVKey, "BBLV_Metadata::getLV_Info");
    int l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded(pLVKey, "BBLV_Metadata::getLV_Info");

    for(auto it = metaDataMap.begin(); it != metaDataMap.end(); ++it) {
        if (it->first == *pLVKey) {
            l_BBLV_Info = const_cast <BBLV_Info*> (&(it->second));
            break;
        }
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata(pLVKey, "BBLV_Metadata::getLV_Info");
    }

    if (l_TransferQueueWasUnlocked)
    {
        lockTransferQueue(pLVKey, "BBLV_Metadata::getLV_Info");
    }

    return l_BBLV_Info;
}

size_t BBLV_Metadata::getTotalTransferSize(const LVKey& pLVKey) {
    size_t l_Size = 0;

    int l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded(&pLVKey, "BBLV_Metadata::getTotalTransferSize");

    if (metaDataMap.find(pLVKey) != metaDataMap.end()) {
        l_Size = metaDataMap[pLVKey].getTotalTransferSize();
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata(&pLVKey, "BBTagInfoMap::getTotalTransferSize");
    }

    return l_Size;
}

int BBLV_Metadata::getTransferHandle(uint64_t& pHandle, const LVKey* pLVKey, const BBJob pJob, const uint64_t pTag, const uint64_t pNumContrib, const uint32_t pContrib[]) {
    int rc = 0;

    int l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded(pLVKey, "BBLV_Metadata::getTransferHandle");

    if (metaDataMap.find(*pLVKey) != metaDataMap.end()) {
        rc = metaDataMap[*pLVKey].getTransferHandle(pHandle, pLVKey, pJob, pTag, pNumContrib, pContrib);
    } else {
        pHandle = UNDEFINED_HANDLE;
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata(pLVKey, "BBTagInfoMap::getTransferHandle");
    }

    return rc;
}

void BBLV_Metadata::getTransferHandles(std::vector<uint64_t>& pHandles, const BBJob pJob, const BBSTATUS pMatchStatus) {
    int l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded((LVKey*)0, "BBLV_Metadata::getTransferHandles");

    for(auto it = metaDataMap.begin(); it != metaDataMap.end(); ++it) {
        it->second.getTransferHandles(pHandles, pJob, pMatchStatus, it->second.stageOutStarted());
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata((LVKey*)0, "BBLV_Metadata::getTransferHandles");
    }

    return;
}

int BBLV_Metadata::hasLVKey(const LVKey* pLVKey, const uint64_t pJobId)
{
    int rc = 0;

    int l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded(pLVKey, "BBLV_Metadata::hasLVKey");

    for (auto it = metaDataMap.begin(); it != metaDataMap.end(); ++it)
    {
        if (it->first == *pLVKey && (it->second).getJobId() == pJobId)
        {
            rc = 1;
            break;
        }
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata(pLVKey, "BBLV_Metadata::hasLVKey");
    }

    return rc;
}

void BBLV_Metadata::removeAllLogicalVolumesForUuid(const string& pHostName, const LVKey* pLVKey, const uint64_t pJobId)
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
            errorText << "BBLV_Metadata::removeAllLogicalVolumesForUuid(): LVKey passed as NULL with jobid " << pJobId;
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

void BBLV_Metadata::removeLVKey(const uint64_t pJobId, const LVKey* pLVKey)
{
    LOG(bb,debug) << "taginfo: Removing " << *pLVKey << " for jobid " << pJobId;
    metaDataMap.erase(*pLVKey);

    return;
}

int BBLV_Metadata::retrieveTransfers(BBTransferDefs& pTransferDefs)
{
    int rc = 0;

    int l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded((LVKey*)0, "BBLV_Metadata::retrieveTransfers");

    if (pTransferDefs.getHostName() != UNDEFINED_HOSTNAME)
    {
        // For now, we only attempt to get the data from the local metadata if
        // we have a unique hostname provided
        bool l_HostNameFound = false;
        for (auto it = metaDataMap.begin(); (!rc) && it != metaDataMap.end(); ++it)
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

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata((LVKey*)0, "BBLV_Metadata::retrieveTransfers");
    }

    return rc;
}

int BBLV_Metadata::sendTransferCompleteForHandleMsg(const string& pHostName, const string& pCN_HostName, const uint64_t pHandle, const BBSTATUS pStatus)
{
    int rc = 0;
    int l_AppendAsyncRequestFlag = ASYNC_REQUEST_HAS_NOT_BEEN_APPENDED;

    int l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded((LVKey*)0, "BBLV_Metadata::sendTransferCompleteForHandleMsg");

    for (auto it = metaDataMap.begin(); it != metaDataMap.end(); ++it)
    {
        int rc2 = it->second.sendTransferCompleteForHandleMsg(pHostName, pCN_HostName, &(it->first), pHandle, l_AppendAsyncRequestFlag, pStatus);
        rc = (rc2 ? rc2 : rc);
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata((LVKey*)0, "BBLV_Metadata::sendTransferCompleteForHandleMsg");
    }

    return rc;
}

void BBLV_Metadata::setCanceled(const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const int pRemoveOption)
{
    LOCAL_METADATA_RELEASED l_LockWasReleased = LOCAL_METADATA_LOCK_NOT_RELEASED;

    bool l_Restart = true;
    while (l_Restart)
    {
        l_Restart = false;
        l_LockWasReleased = LOCAL_METADATA_LOCK_NOT_RELEASED;
        for (auto it = metaDataMap.begin(); it != metaDataMap.end(); ++it)
        {
            it->second.setCanceled(&(it->first), pJobId, pJobStepId, pHandle, l_LockWasReleased, pRemoveOption);

            // NOTE: Reset CurrentWrkQE for the next iteration...
            CurrentWrkQE = NULL;

            if (l_LockWasReleased == LOCAL_METADATA_LOCK_RELEASED)
            {
                l_Restart = true;
                break;
            }
        }
    }

    return;
}

int BBLV_Metadata::setSuspended(const string& pHostName, const string& pCN_HostName, LOCAL_METADATA_RELEASED &pLocal_Metadata_Lock_Released, const int pValue)
{
    int rc = 0;
    uint32_t l_NumberAlreadySet = 0;
    uint32_t l_NumberOfQueuesNotMatchingHostNameCriteria = 0;
    uint32_t l_NumberOfQueuesNotFoundForLVKey = 0;
    uint32_t l_NumberSet = 0;
    uint32_t l_NumberFailed = 0;
    bool l_AllDone = false;
    vector<LVKey> l_LVKeysProcessed = vector<LVKey>();
    vector<LVKey>::iterator it2;

    while (!l_AllDone)
    {
        l_AllDone = true;
        for(auto it = metaDataMap.begin(); it != metaDataMap.end(); ++it)
        {
            if (find(l_LVKeysProcessed.begin(), l_LVKeysProcessed.end(), it->first) == l_LVKeysProcessed.end())
            {
                rc = it->second.setSuspended(&(it->first), pCN_HostName, pLocal_Metadata_Lock_Released, pValue);
                switch (rc)
                {
                    case 2:
                    {
                        // Value was already set for this work queue.
                        // Continue to the next LVKey...
                        LOG(bb,info) << "BBLV_Metadata::setSuspended(): Queue for hostname " << it->second.getHostName() << ", " << it->first \
                                     << " was already " << (pValue ? "inactive" : "active");
                        ++l_NumberAlreadySet;

                        break;
                    }

                    case 1:
                    {
                        // Hostname did not match...  Continue to the next LVKey...
                        LOG(bb,debug) << "BBLV_Metadata::setSuspended(): Queue for hostname " << it->second.getHostName() << ", " << it->first \
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
                        LOG(bb,debug) << "BBLV_Metadata::setSuspended(): Work queue for hostname " << it->second.getHostName() << ", " << it->first \
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
                l_LVKeysProcessed.push_back(it->first);
                if (pLocal_Metadata_Lock_Released == LOCAL_METADATA_LOCK_RELEASED)
                {
                    l_AllDone = false;
                    break;
                }
                rc = 0;
            }
        }
    }

    if (l_NumberSet)
    {
        // Reset lastDumpedNumberOfWorkQueueItemsProcessed so that the
        // work queue manager is dumped to the console log 'next time'...
        int l_LocalMetadataUnlockedInd = 0;
        wrkqmgr.lockWorkQueueMgr((LVKey*)0, "BBLV_Metadata::setSuspended", &l_LocalMetadataUnlockedInd);
        wrkqmgr.setLastDumpedNumberOfWorkQueueItemsProcessed(0);
        wrkqmgr.unlockWorkQueueMgr((LVKey*)0, "BBLV_Metadata::setSuspended", &l_LocalMetadataUnlockedInd);
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

int BBLV_Metadata::stopTransfer(const string pHostName, const string pCN_HostName, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId)
{
    int rc = 0;
    LOCAL_METADATA_RELEASED l_LockWasReleased = LOCAL_METADATA_LOCK_NOT_RELEASED;

    string l_ServerHostName;
    activecontroller->gethostname(l_ServerHostName);
    string l_Result = ", the transfer definition was not found on the bbServer at " + l_ServerHostName;
    string l_ServicedByHostname = ContribIdFile::isServicedBy(BBJob(pJobId, pJobStepId), pHandle, pContribId);
    if (l_ServerHostName == l_ServicedByHostname)
    {
        bool l_Restart = true;
        bool l_Continue = true;
        while (l_Restart && l_Continue)
        {
            l_Restart = false;
            l_LockWasReleased = LOCAL_METADATA_LOCK_NOT_RELEASED;
            for(auto it = metaDataMap.begin(); ((!rc) && it != metaDataMap.end()); ++it)
            {
                l_Continue = false;
                rc = it->second.stopTransfer(&(it->first), pHostName, pCN_HostName, pJobId, pJobStepId, pHandle, pContribId, l_LockWasReleased);
                switch (rc)
                {
                    case 0:
                    {
                        // The transfer definition being searched for could not be found with this LVKey.
                        // Continue to the next LVKey...
                        l_Continue = true;

                        break;
                    }

                    case 1:
                    {
                        // Found the transfer definition on this bbServer.
                        // It was processed, and operation logged.
                        // The cross bbserver metadata was also appropriately reset
                        // as part of the operation.
                        l_Result = ", the transfer definition was successfully stopped using information from this bbServer's local cache.";

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
                        l_Result = ", the transfer definition did not yet have any extents scheduled for transfer. A start transfer request was caught in mid-flight and the original request was issued to the new bbServer to complete the transfer request.";

                        break;
                    }

                    default:
                    {
                        // Error occurred....  Log it and continue...
                        l_Result = ", processing during the search for the transfer definition caused a failure.";
                        LOG(bb,error) << "Failed when processing a stop transfer request for CN hostname " << pCN_HostName << ", jobid " << pJobId << ", jobstepid " << pJobStepId
                                      << ", handle " << pHandle << ", contribId " << pContribId << ", rc=" << rc << ". Stop transfer processing continues for the remaining transfer definitions in the set.";
                        rc = 0;
                        l_Continue = true;

                        break;
                    }
                }

                // NOTE: Reset CurrentWrkQE for the next iteration...
                CurrentWrkQE = NULL;

                if (l_LockWasReleased == LOCAL_METADATA_LOCK_RELEASED)
                {
                    l_Restart = true;
                    break;
                }

                if (!l_Continue)
                {
                    break;
                }
            }
        }

        if (!rc)
        {
            rc = attemptToUnconditionallyStopThisTransferDefinition(pHostName, pCN_HostName, pJobId, pJobStepId, pHandle, pContribId);
            if (rc == 1)
            {
                // Transfer definition was previously serviced by this bbServer.
                // However, this bbServer was previously restarted so this transfer
                // definition was not found in the local metadata.  This transfer
                // definition was unconditionally stopped.
                l_Result = ", the transfer definition was successfully stopped using information from the cross bbServer metadata.";
            }
        }

        LOG(bb,info) << "For host name " << pCN_HostName << ", jobid " << pJobId << ", jobstepid " << pJobStepId \
                     << ", handle " << pHandle << ", and contribid " << pContribId << l_Result;
    }
    else
    {
        // The transfer definition being searched for is not being serviced by this bbServer.
        // Categorize as the transfer definition was not found on this bbServer.
        // NOTE: The transfer definition could actually be present on this bbServer, if
        //       this happens to be the second failover processing for the transfer definition.
        //       However, such a transfer definition should have had all of its extents already
        //       processed (stopped or completed) and a restart operation is more than likely
        //       to follow this stop request on this bbServer to restart and reuse the transfer
        //       definition.
    }

    if (sameHostName(pHostName) && rc <= 0)
    {
        // NOTE: It is possible for a given hostname to be found in more than one bbServer.
        //       Append the stop operation for the stop transfer request to the async request file.
        appendAsyncRequestForStopTransfer(pCN_HostName, pJobId, pJobStepId, pHandle, pContribId, (uint64_t)BBSCOPETRANSFER);
    }

    return rc;
}

// NOTE:  This method verifies that the input jobid exists for some LVKey...
int BBLV_Metadata::verifyJobIdExists(const std::string& pConnectionName, const LVKey* pLVKey, const uint64_t pJobId)
{
    int rc = 0;

    int l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded(pLVKey, "BBLV_Metadata::verifyJobIdExists");

    for (auto it = metaDataMap.begin(); it != metaDataMap.end(); ++it)
    {
        if ((it->second).getJobId() == pJobId)
        {
            rc = 1;
            break;
        }
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata(pLVKey, "BBLV_Metadata::verifyJobIdExists");
    }

    return rc;
}
