/*******************************************************************************
 |    HandleFile.cc
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

#include <fcntl.h>
#include <unistd.h>

#include "BBTagInfo.h"
#include "BBTagInfoMap.h"
#include "bbserver_flightlog.h"
#include "ContribIdFile.h"
#include "HandleFile.h"
#include "LVUuidFile.h"
#include "tracksyscall.h"
#include <dirent.h>


/*******************************************************************************
 | External data
 *******************************************************************************/
thread_local int handleFileLockFd = -1;


/*
 * Static methods
 */

bool accessDir(const std::string& pF)
{
    DIR* dirp=opendir(pF.c_str());
    if (!dirp) {
        LOG(bb,debug) << pF.c_str()<<" accessDir opendir errno="<<errno<<":"<<strerror(errno);
        return false;
    }
    closedir(dirp);
    return true;
}


int HandleFile::createLockFile(const char* pFilePath)
{
    int rc = 0;

    char l_LockFileName[PATH_MAX+64] = {'\0'};
    snprintf(l_LockFileName, sizeof(l_LockFileName), "%s/%s", pFilePath, LOCK_FILENAME);

    bfs::ofstream l_LockFile{l_LockFileName};

    return rc;
}

int HandleFile::getTransferKeys(const uint64_t pJobId, const uint64_t pHandle, uint64_t& pLengthOfTransferKeys, uint64_t& pBufferSize, char* pBuffer)
{
    int rc = 0;

    string l_TransferKeys = "";
    pLengthOfTransferKeys = 0;

    rc = HandleFile::get_xbbServerHandleTransferKeys(l_TransferKeys, pJobId, pHandle);

    if (!rc)
    {
        if (pBuffer && pBufferSize)
        {
            pBuffer[0] = '\0';
            if (l_TransferKeys.length())
            {
                if (pBufferSize >= l_TransferKeys.length()+1)
                {
                    l_TransferKeys.copy(pBuffer, l_TransferKeys.length());
                    pBuffer[l_TransferKeys.length()] = '\0';
                }
                else
                {
                    rc = -2;
                }
            }
        }
        pLengthOfTransferKeys = l_TransferKeys.length();
    }

    return rc;
}

#define ATTEMPTS 10
int HandleFile::get_xbbServerGetCurrentJobIds(vector<string>& pJobIds)
{
    int rc = 0;
    stringstream errorText;

    int l_catch_count=ATTEMPTS;

    pJobIds.clear();

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, HF_GetCurrentJobIds, "get current jobids, counter=%ld", l_FL_Counter, 0, 0, 0);

    bfs::path datastore(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
    if(bfs::is_directory(datastore))
    {
        // Build a vector of jobids that the current uid/gid is authorized to access.
        // Our invoker will iterate over these jobids in reverse order, as it is almost always
        // the case that the jobid we want is the last one...
        bool l_AllDone = false;
        while (!l_AllDone)
        {
            l_AllDone = true;
            try
            {
                for(auto& job : boost::make_iterator_range(bfs::directory_iterator(datastore), {}))
                {
                    if (!bfs::is_directory(job)) continue;
                    if (!accessDir(job.path().string())) continue;
                    pJobIds.push_back(job.path().string());
                }
            }
            catch(exception& e)
            {

                if (--l_catch_count)
                {
                    // NOTE:  'No entry' is an expected error due to a concurrent removeJobInfo.
                    //        If not that, log the error and retry.
                    if (errno != ENOENT)
                    {
                        LOG(bb,warning) << "Exception caught " << __func__ << "@" << __FILE__ << ":" << __LINE__ << " what=" << e.what() \
                                        << ". Attempting to rebuild the vector of jobids again...";
                    }
                    pJobIds.clear();
                    l_AllDone = false;
                }
                else //RAS
                {
                    rc = -1;
                    LOG(bb,error) << "Exception caught " << __func__ << "@" << __FILE__ << ":" << __LINE__ << " what=" << e.what();
                    errorText << "get_xbbServerGetCurrentJobIds(): exception when building the vector of jobids";
                    LOG_ERROR_TEXT_ERRNO_AND_RAS(errorText, errno, bb.internal.handleinfo);
                    LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
                }
                break;
            }
        }
    }
    else
    {
        rc = -1;
        errorText << "get_xbbServerGetCurrentJobIds(): Could not find the BB metadata store at " << datastore.string();
        LOG_ERROR_TEXT_RC(errorText, rc);
    }

    FL_Write(FLMetaData, HF_GetCurrentJobIds_End, "get current jobids, counter=%ld, attempts=%ld, errno=%ld, rc=%ld",
             l_FL_Counter, (uint64_t)(l_catch_count == ATTEMPTS ? 1 : ATTEMPTS-l_catch_count), errno, rc);

    return rc;
}

int HandleFile::get_xbbServerGetJobForHandle(uint64_t& pJobId, uint64_t& pJobStepId, const uint64_t pHandle) {
    int rc = 0;
    stringstream errorText;

    pJobId = UNDEFINED_JOBID;
    pJobStepId = UNDEFINED_JOBSTEPID;

    int l_catch_count=ATTEMPTS;
    vector<string> l_PathJobIds;
    l_PathJobIds.reserve(100);

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, HF_GetJobForHandle, "get jobid for handle, counter=%ld, handle=%ld", l_FL_Counter, pHandle, 0, 0);

    // First, build a vector of jobids that the current uid/gid is authorized to access.
    // We will iterate over these jobids in reverse order, as it is almost always
    // the case that the jobid we want is the last one...
    //
    // NOTE: If we take an exception in the loop below, we do not rebuild this vector
    //       of jobids.  The job could go away, but any jobid expected by the code
    //       below should be in the vector.
    rc = get_xbbServerGetCurrentJobIds(l_PathJobIds);

    if (!rc)
    {
        rc = -1;
        bool l_AllDone = false;
        while (!l_AllDone)
        {
            l_AllDone = true;
            for(vector<string>::reverse_iterator rit = l_PathJobIds.rbegin(); rit != l_PathJobIds.rend(); ++rit)
            {
                try
                {
                    if (!rc) continue;
                    bfs::path job = bfs::path(*rit);
                    for (auto& jobstep : boost::make_iterator_range(bfs::directory_iterator(job), {}))
                    {
                        if ((!rc) || (!accessDir(jobstep.path().string()))) continue;
                        for (auto& handle : boost::make_iterator_range(bfs::directory_iterator(jobstep), {}))
                        {
                            if (handle.path().filename().string() == to_string(pHandle))
                            {
                                rc = 0;
                                pJobId = stoul(job.filename().string());
                                pJobStepId = stoul(jobstep.path().filename().string());
                                break;
                            }
                        }
                    }
                }
                catch(exception& e)
                {
                    if (--l_catch_count)
                    {
                        // NOTE:  'No entry' is an expected error due to a concurrent removeJobInfo.
                        //        If not that, log the error and retry.  Before retrying, remove the
                        //        jobid that failed...
                        if (errno != ENOENT)
                        {
                            LOG(bb,warning) << "Exception caught " << __func__ << "@" << __FILE__ << ":" << __LINE__ << " what=" << e.what() \
                                            << ". Retrying the operation...";
                        }
    		        	advance(rit, 1);
    			        l_PathJobIds.erase(rit.base());
                        l_AllDone = false;
                     }
                     else //RAS
                     {
                         rc=-1;
                         LOG(bb,error) << "Exception caught " << __func__ << "@" << __FILE__ << ":" << __LINE__ << " what=" << e.what();
                         errorText << "get_xbbServerGetJobForHandle(): exception in looking for handle " << pHandle;
                         LOG_ERROR_TEXT_ERRNO_AND_RAS(errorText, errno, bb.internal.getjobforhandle);
                         LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
                     }
                     break;
                }
            }
        }
    }
    else
    {
        // bberror has already been filled in
    }

    FL_Write6(FLMetaData, HF_GetJobForHandle_End, "get jobid for handle, counter=%ld, handle=%ld, jobid=%ld, attempts=%ld, rc=%ld",
              l_FL_Counter, pHandle, pJobId, (uint64_t)(l_catch_count == ATTEMPTS ? 1 : ATTEMPTS-l_catch_count), rc, 0);

    return rc;
}

int HandleFile::get_xbbServerGetHandle(BBJob& pJob, uint64_t pTag, vector<uint32_t>& pContrib, uint64_t& pHandle)
{
    int rc = 0;
    stringstream errorText;

    uint32_t* l_ContribArray = 0;
    HandleFile* l_HandleFile = 0;
    uint64_t l_NumOfContribsInArray = 0;
    int l_catch_count=ATTEMPTS;
    vector<string> l_PathJobIds;
    l_PathJobIds.reserve(100);

    pHandle = 0;

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, HF_GetHandle, "get handle, counter=%ld, jobid=%ld", l_FL_Counter, pJob.getJobId(), 0, 0);

    // First, build a vector of jobids that the current uid/gid is authorized to access.
    // We will iterate over these jobids in reverse order, as it is almost always
    // the case that the jobid we want is the last one...
    //
    // NOTE: If we take an exception in the loop below, we do not rebuild this vector
    //       of jobids.  The job could go away, but any jobid expected by the code
    //       below should be in the vector.
    rc = get_xbbServerGetCurrentJobIds(l_PathJobIds);

    if (!rc)
    {
        bool l_AllDone = false;
        while (!l_AllDone)
        {
            l_AllDone = true;
            for(vector<string>::reverse_iterator rit = l_PathJobIds.rbegin(); rit != l_PathJobIds.rend(); ++rit)
            {
                if(*rit == to_string(pJob.getJobId()))
                {
                    try
                    {
                        bfs::path job = bfs::path(*rit);
                        for(auto& jobstep : boost::make_iterator_range(bfs::directory_iterator(job), {}))
                        {
                            if(!accessDir(jobstep.path().string())) continue;
                            if(jobstep.path().filename().string() == to_string(pJob.getJobStepId()))
                            {
                                for(auto& handle : boost::make_iterator_range(bfs::directory_iterator(jobstep), {}))
                                {
                                    bfs::path handlefile = handle.path() / bfs::path(handle.path().filename());
                                    rc = loadHandleFile(l_HandleFile, handlefile.string().c_str());
                                    if (!rc)
                                    {
                                        if (l_HandleFile->tag == pTag)
                                        {
                                            // Tags match...  Now, compare the list of contribs...
                                            l_HandleFile->getContribArray(l_NumOfContribsInArray, l_ContribArray);
                                            if (!BBTagInfo::compareContrib(l_NumOfContribsInArray, l_ContribArray, pContrib))
                                            {
                                                rc = 1;
                                                pHandle = stoul(handle.path().filename().string());
                                            }

                                            delete[] l_ContribArray;
                                            l_ContribArray = 0;
                                        }
                                    }
                                    else
                                    {
                                        LOG(bb,error) << "Could not load the handle file for jobid " << pJob.getJobId() << ", jobstepid " << pJob.getJobStepId() << " using handle path " << handle.path().string();
                                    }

                                    if (l_HandleFile)
                                    {
                                        delete l_HandleFile;
                                        l_HandleFile = 0;
                                    }
                                }
                            }
                        }
                    }
                    catch(exception& e)
                    {
                        if (--l_catch_count)
                        {
                            // NOTE:  'No entry' is an expected error due to a concurrent removeJobInfo.
                            //        If not that, log the error and retry.  Before retrying, remove the
                            //        jobid that failed...
                            if (errno != ENOENT)
                            {
                                LOG(bb,warning) << "Exception caught " << __func__ << "@" << __FILE__ << ":" << __LINE__ << " what=" << e.what() \
                                                << ". Retrying the operation...";
                            }
    	    	        	advance(rit, 1);
    	    		        l_PathJobIds.erase(rit.base());
                            l_AllDone = false;
                        }
                        else //RAS
                        {
                            rc=-1;
                            LOG(bb,error) << "Exception caught " << __func__ << "@" << __FILE__ << ":" << __LINE__ << " what=" << e.what();
                            errorText << "get_xbbServerGetHandle(): exception in looking for handle for jobid " << pJob.getJobId() << ", jobstepid " \
                                      << pJob.getJobStepId()  << ", tag " << pTag << ", number of contribs " << pContrib.size();
                            LOG_ERROR_TEXT_ERRNO_AND_RAS(errorText, errno, bb.internal.gethandle);
                            LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
                        }
                        break;
                    }
                }
            }
        }
    }
    else
    {
        // bberror has already been filled in
    }

    FL_Write6(FLMetaData, HF_GetHandle_End, "get handle, counter=%ld, jobid=%ld, handle=%ld, attempts=%ld, rc=%ld",
              l_FL_Counter, pJob.getJobId(), pHandle, (uint64_t)(l_catch_count == ATTEMPTS ? 1 : ATTEMPTS-l_catch_count), rc, 0);

    return rc;
}

int HandleFile::get_xbbServerHandleInfo(uint64_t& pJobId, uint64_t& pJobStepId, uint64_t& pNumberOfReportingContribs, HandleFile* &pHandleFile, ContribIdFile* &pContribIdFile, const uint64_t pHandle, const uint32_t pContribId)
{
    int rc = 0;
    stringstream errorText;

    pJobId = 0;
    pJobStepId = 0;
    pNumberOfReportingContribs = 0;
    pHandleFile = 0;
    pContribIdFile = 0;

    uint64_t l_JobId = pJobId;
    uint64_t l_JobStepId = pJobStepId;
//    char* l_HandleFileName = 0;
    int l_catch_count=ATTEMPTS;
    vector<string> l_PathJobIds;
    l_PathJobIds.reserve(100);

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, HF_GetHandleInfo, "get handle info, counter=%ld, jobid=%ld, handle=%ld, contribid=%ld", l_FL_Counter, pJobId, pHandle, pContribId);

    // NOTE: The only case where this method will return a non-zero return code is if the xbbServer data store
    //       cannot be found/loaded.  Otherwise, the invoker MUST check the returned pHandleFile and pContribIdFile
    //       pointers for success/information.

    // First, build a vector of jobids that the current uid/gid is authorized to access.
    // We will iterate over these jobids in reverse order, as it is almost always
    // the case that the jobid we want is the last one...
    //
    // NOTE: If we take an exception in the loop below, we do not rebuild this vector
    //       of jobids.  The job could go away, but any jobid expected by the code
    //       below should be in the vector.
    rc = get_xbbServerGetCurrentJobIds(l_PathJobIds);

    if (!rc)
    {
        bool l_AllDone = false;
        while (!l_AllDone)
        {
            l_AllDone = true;
            for(vector<string>::reverse_iterator rit = l_PathJobIds.rbegin(); rit != l_PathJobIds.rend(); ++rit)
            {
                try
                {
                    bfs::path job = bfs::path(*rit);
                    l_JobId = stoull(job.filename().string());
                    for(auto& jobstep : boost::make_iterator_range(bfs::directory_iterator(job), {}))
                    {
                        if(!accessDir(jobstep.path().string())) continue;
                        l_JobStepId = stoull(jobstep.path().filename().string());
                        for(auto& handle : boost::make_iterator_range(bfs::directory_iterator(jobstep), {}))
                        {
                            if(handle.path().filename().string() == to_string(pHandle))
                            {
                                bfs::path handlefile = handle.path() / bfs::path(handle.path().filename());
                                rc = loadHandleFile(pHandleFile, handlefile.string().c_str());
//                                rc = loadHandleFile(pHandleFile, l_HandleFileName, l_JobId, l_JobStepId, pHandle, TEST_FOR_HANDLEFILE_LOCK);
                                if (!rc)
                                {
                                    // Store the jobid and jobstepid values in the return variables...
                                    pJobId = l_JobId;
                                    pJobStepId = l_JobStepId;

                                    uint64_t l_NumberOfLVUuidReportingContribs = 0;
                                    rc = ContribIdFile::loadContribIdFile(pContribIdFile, pNumberOfReportingContribs, l_NumberOfLVUuidReportingContribs, handle.path(), pContribId);
                                    switch (rc)
                                    {
                                        case 0:
                                        case 1:
                                        {
                                            rc = 0;
                                            break;
                                        }
                                        default:
                                        {
                                            LOG(bb,warning) << "Could not load the contribid file for jobid " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", contribid " << pContribId << ", using handle path " << handle.path().string();
                                        }
                                    }
                                }
                                else
                                {
                                    LOG(bb,warning) << "Could not load the handle file for jobid " << l_JobId << ", jobstepid " << l_JobStepId << ", handle " << pHandle << ", using handle path " << handle.path().string();
                                }
                            }
                        }
                    }
                }
                catch(exception& e)
                {
                    if (pContribIdFile)
                    {
                        delete pContribIdFile;
                        pContribIdFile = 0;
                    }
#if 0
                    if (l_HandleFileName)
                    {
                        delete[] l_HandleFileName;
                        l_HandleFileName = 0;
                    }
#endif
                    if (pHandleFile)
                    {
                        delete pHandleFile;
                        pHandleFile = 0;
                    }
                    if (--l_catch_count)
                    {
                        // NOTE:  'No entry' is an expected error due to a concurrent removeJobInfo.
                        //        If not that, log the error and retry.  Before retrying, remove the
                        //        jobid that failed...
                        if (errno != ENOENT)
                        {
                            LOG(bb,warning) << "Exception caught " << __func__ << "@" << __FILE__ << ":" << __LINE__ << " what=" << e.what() \
                                            << ". Retrying the operation...";
                        }
    		        	advance(rit, 1);
    			        l_PathJobIds.erase(rit.base());
                        l_AllDone = false;
                    }
                    else //RAS
                    {
                        rc=-1;
                        LOG(bb,error) << "Exception caught " << __func__ << "@" << __FILE__ << ":" << __LINE__ << " what=" << e.what();
                        errorText << "get_xbbServerHandleInfo(): exception in looking for handle " << pHandle << ", contribid " << pContribId;
                        LOG_ERROR_TEXT_ERRNO_AND_RAS(errorText, errno, bb.internal.handleinfo);
                        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
                    }
                    break;
                }
            }
        }
    }
    else
    {
        // bberror has already been filled in
    }

    if (rc)
    {
        pNumberOfReportingContribs = 0;

        if (pContribIdFile)
        {
            delete pContribIdFile;
            pContribIdFile = 0;
        }
#if 0
        if (l_HandleFileName)
        {
            delete[] l_HandleFileName;
            l_HandleFileName = 0;
        }
#endif
        if (pHandleFile)
        {
            delete pHandleFile;
            pHandleFile = 0;
        }
    }

    FL_Write6(FLMetaData, HF_GetHandleInfo_End, "get handle info, counter=%ld, jobid=%ld, handle=%ld, contribid=%ld, attempts=%ld, rc=%ld",
              l_FL_Counter, pJobId, pHandle, pContribId, (uint64_t)(l_catch_count == ATTEMPTS ? 1 : ATTEMPTS-l_catch_count), rc);

    return rc;
}
#undef ATTEMPTS

int HandleFile::get_xbbServerHandleList(std::vector<uint64_t>& pHandles, const BBJob pJob, const BBSTATUS pMatchStatus)
{
    int rc = 0;

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, HF_GetHandleList, "get handle list, counter=%ld, jobid=%ld, status=%ld", l_FL_Counter, pJob.getJobId(), (uint64_t)pMatchStatus, 0);

    try
    {
        uint64_t l_JobStepId = pJob.getJobStepId();
        if (l_JobStepId == 0)
        {
            bfs::path job(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
            job /= bfs::path(to_string(pJob.getJobId()));
            if(!bfs::is_directory(job)) BAIL;
            for(auto& jobstep : boost::make_iterator_range(bfs::directory_iterator(job), {}))
            {
                rc = processTransferHandleForJobStep(pHandles, jobstep.path().string().c_str(), pMatchStatus);
                if (rc)
                {
                    break;
                }
            }
        }
        else
        {
            bfs::path jobstep(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
            jobstep /= bfs::path(to_string(pJob.getJobId()));
            jobstep /= bfs::path(to_string(l_JobStepId));
            if(!bfs::is_directory(jobstep)) BAIL;
            rc = processTransferHandleForJobStep(pHandles, jobstep.string().c_str(), pMatchStatus);
        }

        if (rc)
        {
            LOG(bb,error) << "Failure from processTransferHandleForJobStep()";
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    FL_Write6(FLMetaData, HF_GetHandleList_End, "get handle list, counter=%ld, jobid=%ld, status=%ld, number returned=%ld, rc=%ld",
              l_FL_Counter, pJob.getJobId(), (uint64_t)pMatchStatus, (uint64_t)pHandles.size(), rc, 0);

    return rc;
}

int HandleFile::get_xbbServerHandleStatus(BBSTATUS& pStatus, const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle)
{
    int rc = 0;
    stringstream errorText;

    pStatus = BBNONE;

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, HF_GetHandleStatus, "get handle status, counter=%ld, jobid=%ld, handle=%ld", l_FL_Counter, pJobId, pHandle, 0);

    HandleFile* l_HandleFile = 0;
    char* l_HandleFileName = 0;
    rc = loadHandleFile(l_HandleFile, l_HandleFileName, pJobId, pJobStepId, pHandle, DO_NOT_LOCK_HANDLEFILE);
//    rc = loadHandleFile(l_HandleFile, l_HandleFileName, pJobId, pJobStepId, pHandle, TEST_FOR_HANDLEFILE_LOCK);
    if (!rc)
    {
        pStatus = (BBSTATUS)l_HandleFile->status;
    }
    else
    {
        errorText << "Failure when attempting to load the handle file for jobid " << pJobId << ", jobstepid " << pJobStepId  << ", handle " << pHandle;
        LOG_ERROR_TEXT_RC(errorText, rc);
    }

    if (l_HandleFileName)
    {
        delete[] l_HandleFileName;
        l_HandleFileName = 0;
    }

    if (l_HandleFile) {
        delete l_HandleFile;
        l_HandleFile = 0;
    }

    FL_Write6(FLMetaData, HF_GetHandleStatus_End, "get handle status, counter=%ld, jobid=%ld, handle=%ld, status=%ld, rc=%ld", l_FL_Counter, pJobId, pHandle, pStatus, rc, 0);

    return rc;
}

#define ATTEMPTS 10
int HandleFile::get_xbbServerHandleTransferKeys(string& pTransferKeys, const uint64_t pJobId, const uint64_t pHandle)
{
    int rc = 0;
    stringstream errorText;

    pTransferKeys = "";

    bfs::path datastore(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
    if(!bfs::is_directory(datastore)) return rc;
    int l_catch_count=ATTEMPTS;
    vector<string> l_PathJobIds;
    l_PathJobIds.reserve(100);

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, HF_GetHandleKeys, "get handle transfer keys, counter=%ld, jobid=%ld, handle=%ld", l_FL_Counter, pJobId, pHandle, 0);

    // First, build a vector of jobids that the current uid/gid is authorized to access.
    // We will iterate over these jobids in reverse order, as it is almost always
    // the case that the jobid we want is the last one...
    //
    // NOTE: If we take an exception in the loop below, we do not rebuild this vector
    //       of jobids.  The job could go away, but any jobid expected by the code
    //       below should be in the vector.
    rc = get_xbbServerGetCurrentJobIds(l_PathJobIds);

    if (!rc)
    {
        bool l_AllDone = false;
        while (!l_AllDone)
        {
            l_AllDone = true;
            for(vector<string>::reverse_iterator rit = l_PathJobIds.rbegin(); rit != l_PathJobIds.rend(); ++rit)
            {
                if(*rit == to_string(pJobId))
                {
                    try
                    {
                        bfs::path job = bfs::path(*rit);
                        for(auto& jobstep : boost::make_iterator_range(bfs::directory_iterator(job), {}))
                        {
                            if(!accessDir(jobstep.path().string()) ) continue;
                            for(auto& handle : boost::make_iterator_range(bfs::directory_iterator(jobstep), {}))
                            {
                                if(!bfs::is_directory(handle)) continue;
                                if(handle.path().filename().string() == to_string(pHandle))
                                {
                                    bfs::path handlefile = handle.path() / bfs::path(handle.path().filename());
                                    HandleFile* l_HandleFile = 0;
                                    rc = loadHandleFile(l_HandleFile, handlefile.string().c_str());
                                    if (!rc)
                                    {
                                        // The string is terminated with a line feed...  Don't copy the last character...
                                        pTransferKeys = (l_HandleFile->transferKeys).substr(0,((l_HandleFile->transferKeys).length()-1));
                                    }
                                    else
                                    {
                                        errorText << "Failure when attempting to load the handle file for jobid " << pJobId << ", handle " << pHandle;
                                        LOG_ERROR_TEXT_RC(errorText, rc);
                                    }

                                    if (l_HandleFile)
                                    {
                                        delete l_HandleFile;
                                        l_HandleFile = 0;
                                    }
                                }
                            }
                        }
                    }
                    catch(exception& e)
                    {
                        if (--l_catch_count)
                        {
                            // NOTE:  'No entry' is an expected error due to a concurrent removeJobInfo.
                            //        If not that, log the error and retry.  Before retrying, remove the
                            //        jobid that failed...
                            if (errno != ENOENT)
                            {
                                LOG(bb,warning) << "Exception caught " << __func__ << "@" << __FILE__ << ":" << __LINE__ << " what=" << e.what() \
                                                << ". Retrying the operation...";
                            }
        		        	advance(rit, 1);
        			        l_PathJobIds.erase(rit.base());
                            l_AllDone = false;
                        }
                        else //RAS
                        {
                            rc=-1;
                            LOG(bb,error) << "Exception caught " << __func__ << "@" << __FILE__ << ":" << __LINE__ << " what=" << e.what();
                            errorText << "get_xbbServerHandleTransferKeys(): exception looking for transfer keys for jobid " << pJobId << ", handle " << pHandle;
                            LOG_ERROR_TEXT_ERRNO_AND_RAS(errorText, errno, bb.internal.xferkeys);
                            LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
                        }
                        break;
                    }
                }
            }
        }
    }
    else
    {
        // bberror has already been filled in
    }

    FL_Write6(FLMetaData, HF_GetHandleKeys_End, "get handle transfer keys, counter=%ld, jobid=%ld, handle=%ld, attempts=%ld, rc=%ld",
              l_FL_Counter, pJobId, pHandle, (uint64_t)(l_catch_count == ATTEMPTS ? 1 : ATTEMPTS-l_catch_count), rc, 0);

    return rc;
}
#undef ATTEMPTS

int HandleFile::loadHandleFile(HandleFile* &pHandleFile, const char* pHandleFileName)
{
    int rc;

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, HF_Load, "loadHandleFile, counter=%ld", l_FL_Counter, 0, 0, 0);

    pHandleFile = NULL;
    HandleFile* l_HandleFile = new HandleFile();

    struct timeval l_StartTime, l_StopTime;
    bool l_AllDone = false;
    int l_Attempts = 0;
    int l_ElapsedTime = 0;
    int l_LastConsoleOutput = -1;

    l_StartTime.tv_sec = 0; // resolve gcc optimizer complaint

    while ((!l_AllDone) && (l_ElapsedTime < MAXIMUM_HANDLEFILE_LOADTIME))
    {
        rc = 0;
        l_AllDone = true;
        ++l_Attempts;
        try
        {
            ifstream l_ArchiveFile{pHandleFileName};
            text_iarchive l_Archive{l_ArchiveFile};
            l_Archive >> *l_HandleFile;
            pHandleFile = l_HandleFile;
        }
        catch(archive_exception& e)
        {
            // NOTE: If we take an 'archieve exception' we do not delay before attempting the next
            //       read of the archive file.  More than likely, we just had a concurrent update
            //       to the handle file.
            rc = -1;
            l_AllDone = false;

            gettimeofday(&l_StopTime, NULL);
            if (l_Attempts == 1)
            {
                l_StartTime = l_StopTime;
            }
            l_ElapsedTime = int(l_StopTime.tv_sec - l_StartTime.tv_sec);

            if (l_ElapsedTime && (l_ElapsedTime % 3 == 0) && (l_ElapsedTime != l_LastConsoleOutput))
            {
                l_LastConsoleOutput = l_ElapsedTime;
                LOG(bb,warning) << "Archive exception thrown in " << __func__ << " was " << e.what() \
                                << " when attempting to load archive " << pHandleFileName << ". Elapsed time=" << l_ElapsedTime << " second(s). Retrying...";
            }
        }
        catch(exception& e)
        {
            rc = -1;
            LOG(bb,error) << "Exception thrown in " << __func__ << " was " << e.what() << " when attempting to load archive " << pHandleFileName;
        }
    }

    if (l_LastConsoleOutput > 0)
    {
       gettimeofday(&l_StopTime, NULL);
       if (!rc)
        {
            LOG(bb,warning) << "Loading " << pHandleFileName << " became successful after " << (l_StopTime.tv_sec - l_StartTime.tv_sec) << " second(s)" << " after recovering from archive exception(s)";
        }
        else
        {
            LOG(bb,error) << "Loading " << pHandleFileName << " failed after " << (l_StopTime.tv_sec - l_StartTime.tv_sec) << " second(s)" << " when attempting to recover from archive exception(s)";
        }
    }

    if (!rc)
    {
        if (pHandleFile)
        {
            // NOTE: After the initial load, we always set the lockfd
            //       value to -1.  A prior fd value could have been saved.
            //       If this load request has also locked the Handlefile,
            //       then the lockfd value will be filled in by our invoker.
            //
            // NOTE: This value isn't really used anymore...  Has been replaed with
            //       thread_local handleFileLockFd.
            pHandleFile->lockfd = -1;
        }
    }
    else
    {
        if (l_HandleFile)
        {
            delete l_HandleFile;
            l_HandleFile = NULL;
        }
    }

    FL_Write(FLMetaData, HF_Load_End, "loadHandleFile, counter=%ld, attempts=%ld, rc=%ld", l_FL_Counter, l_Attempts, rc, 0);

    return rc;
}

int HandleFile::loadHandleFile(HandleFile* &pHandleFile, char* &pHandleFileName, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const HANDLEFILE_LOCK_OPTION pLockOption, HANDLEFILE_LOCK_FEEDBACK* pLockFeedback)
{
    int rc = 0;
    stringstream errorText;

    int fd = -1;
    char l_ArchivePath[PATH_MAX-64] = {'\0'};
    char* l_ArchivePathWithName = new char[PATH_MAX];

    string l_DataStorePath = config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH);
    snprintf(l_ArchivePath, PATH_MAX-64, "%s/%lu/%lu/%lu", l_DataStorePath.c_str(), pJobId, pJobStepId, pHandle);
    snprintf(l_ArchivePathWithName, PATH_MAX, "%s/%lu", l_ArchivePath, pHandle);

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, HF_Load_wLock, "load HF with lock option, counter=%ld, handle=%ld, lock option=%ld", l_FL_Counter, pHandle, (uint64_t)pLockOption, 0);

    int l_LockOption = pLockOption;
    if (pLockFeedback)
    {
        *pLockFeedback = HANDLEFILE_WAS_NOT_LOCKED;
    }

    if (handleFileLockFd != -1)
    {
        // Handle file already locked - downgrade lock option if necessary
        // NOTE:  We have to downgrade, because if we were to open/close
        //        the lockfile again, the first lock would be released on the
        //        close.
        switch (pLockOption)
        {
            case TEST_FOR_HANDLEFILE_LOCK:          // Not currently used
            case LOCK_HANDLEFILE:
            case LOCK_HANDLEFILE_WITH_TEST_FIRST:   // Not currnetly used
            {
                l_LockOption = DO_NOT_LOCK_HANDLEFILE;
            }

            case DO_NOT_LOCK_HANDLEFILE:
            default:
            {
                // Nothing to do
            }
        }
    }

    switch (l_LockOption)
    {
        case LOCK_HANDLEFILE:
        {
            fd = lock(l_ArchivePath);
        }
        break;

        case TEST_FOR_HANDLEFILE_LOCK:
        {
            // The invoked routine dumps info to the console log.
            // This is a test path only (not in production) and the
            // return code value is not important here...
            testForLock(l_ArchivePath);
        }
        break;

        // NOTE: The following option is not currently used as
        //       it isn't clear this technique works with GPFS.

        case LOCK_HANDLEFILE_WITH_TEST_FIRST:
        {
            rc = testForLock(l_ArchivePath);
            while (rc == 1)
            {
                usleep((useconds_t)1000000);    // Delay 1 second
                rc = testForLock(l_ArchivePath);
            }
            if (!rc)
            {
                fd = lock(l_ArchivePath);
            }
        }
        break;

        default:
            break;
    }

    if (!rc)
    {
        if (l_LockOption == DO_NOT_LOCK_HANDLEFILE ||
            (l_LockOption == LOCK_HANDLEFILE && fd >= 0) ||
            (l_LockOption == LOCK_HANDLEFILE_WITH_TEST_FIRST && fd >= 0))
        {
            rc = loadHandleFile(pHandleFile, l_ArchivePathWithName);
            if (!rc)
            {
                pHandleFileName = l_ArchivePathWithName;
                switch (l_LockOption)
                {
                    case LOCK_HANDLEFILE:
                    case LOCK_HANDLEFILE_WITH_TEST_FIRST:
                    {
                        pHandleFile->lockfd = fd;
                        handleFileLockFd = fd;
                        if (pLockFeedback)
                        {
                            LOG(bb,debug) << ">>>>>>>>>> Handle file " << l_ArchivePathWithName << ", fd " << fd << " locked. Transfer queue locked: " << wrkqmgr.transferQueueIsLocked();
                            *pLockFeedback = HANDLEFILE_WAS_LOCKED;
                        }
                    }
                    break;

                    default:
                        break;
                }
            }
        }
        else
        {
            rc = -1;
        }
    }

    if (pHandleFileName != l_ArchivePathWithName)
    {
        delete[] l_ArchivePathWithName;
        l_ArchivePathWithName = 0;
    }

    if (rc && fd >= 0)
    {
        if (l_LockOption == LOCK_HANDLEFILE || l_LockOption == LOCK_HANDLEFILE_WITH_TEST_FIRST)
        {
            unlock(fd);
        }
        LOG(bb,debug) << "loadHandleFile(): Issue close for handle file fd " << fd;
        ::close(fd);
    }

    FL_Write6(FLMetaData, HF_Load_wLock_End, "load HF with lock option, counter=%ld, handle=%ld, lock option=%ld, lock feedback=%ld, rc=%ld",
              l_FL_Counter, pHandle, (uint64_t)pLockOption, (pLockFeedback ? (uint64_t)(*pLockFeedback) : 0), rc, 0);

    return rc;
}

int HandleFile::lock(const char* pFilePath)
{
    int rc = -2;
    int fd = -1;
    stringstream errorText;

    char l_LockFile[PATH_MAX] = {'\0'};
    snprintf(l_LockFile, PATH_MAX, "%s/%s", pFilePath, LOCK_FILENAME);

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, HF_Lock, "lock HF, counter=%ld", l_FL_Counter, 0, 0, 0);

    uint64_t l_FL_Counter2 = metadataCounter.getNext();
    FL_Write(FLMetaData, HF_Open, "open HF, counter=%ld", l_FL_Counter2, 0, 0, 0);

    threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::opensyscall, l_LockFile, __LINE__);
    fd = open(l_LockFile, O_WRONLY);
    threadLocalTrackSyscallPtr->clearTrack();

    FL_Write(FLMetaData, HF_Open_End, "open HF, counter=%ld, fd=%ld", l_FL_Counter2, fd, 0, 0);

    if (fd >= 0)
    {
        // Exclusive lock and this will block if needed
        LOG(bb,debug) << "lock(): Open issued for handle file " << l_LockFile << ", fd=" << fd;
        struct flock l_LockOptions;
        l_LockOptions.l_whence = SEEK_SET;
        l_LockOptions.l_start = 0;
        l_LockOptions.l_len = 0;    // Lock entire file for writing
        l_LockOptions.l_type = F_WRLCK;
        threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::fcntlsyscall, l_LockFile, __LINE__);
        rc = ::fcntl(fd, F_SETLKW, &l_LockOptions);
        threadLocalTrackSyscallPtr->clearTrack();
    }

    switch(rc)
    {
        case -2:
        {
            errorText << "Could not open handle file " << l_LockFile << " for locking, errno=" << errno << ":" << strerror(errno);
            LOG_ERROR_TEXT_ERRNO(errorText, errno);
        }
        break;

        case -1:
        {
            errorText << "Could not exclusively lock handle file " << l_LockFile << ", errno=" << errno << ":" << strerror(errno);
            LOG_ERROR_TEXT_ERRNO(errorText, errno);

            if (fd >= 0)
            {
                LOG(bb,debug) << "lock(): Issue close for handle file fd " << fd;
                uint64_t l_FL_Counter = metadataCounter.getNext();
                FL_Write(FLMetaData, HF_CouldNotLockExcl, "open HF, could not lock exclusive, performing close, counter=%ld", l_FL_Counter, 0, 0, 0);
                ::close(fd);
                FL_Write(FLMetaData, HF_CouldNotLockExcl_End, "open HF, could not lock exclusive, performing close, counter=%ld, fd=%ld", l_FL_Counter, fd, 0, 0);
            }
            fd = -1;
        }
        break;

        default:
            // Successful lock...
            LOG(bb,debug) << "lock(): Handle file " << l_LockFile << " locked, fd " << fd;
            break;
    }

    FL_Write(FLMetaData, HF_Lock_End, "lock HF, counter=%ld, fd=%ld, rc=%ld, errno=%ld", l_FL_Counter, fd, rc, errno);

    return fd;
}

int HandleFile::processTransferHandleForJobStep(std::vector<uint64_t>& pHandles, const char* pDataStoreName, const BBSTATUS pMatchStatus)
{
    int rc = 0;
    HandleFile* l_HandleFile = 0;

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, HF_GetHandleForJobStep, "process handle for jobstep, counter=%ld, status=%ld", l_FL_Counter, pMatchStatus, 0, 0);

    bfs::path jobstep(pDataStoreName);
    if(!bfs::is_directory(jobstep)) return rc;
    for(auto& handle : boost::make_iterator_range(bfs::directory_iterator(jobstep), {}))
    {
        bfs::path handlefile = handle.path() / bfs::path(handle.path().filename());
        int rc = loadHandleFile(l_HandleFile, handlefile.string().c_str());
        if ((!rc) && l_HandleFile)
        {
            if (BBSTATUS_AND(pMatchStatus, l_HandleFile->status) != BBNONE)
            {
                pHandles.push_back(stoul(handle.path().filename().string()));
            }
        }

        if (l_HandleFile)
        {
            delete l_HandleFile;
            l_HandleFile = 0;
        }

        if (rc)
        {
            break;
        }
    }

    FL_Write(FLMetaData, HF_GetHandleForJobStep_End, "process handle for jobstep, counter=%ld, number of handles returned=%ld, status=%ld, rc=%ld", l_FL_Counter, pHandles.size(), pMatchStatus, rc);

    return rc;
}

int HandleFile::saveHandleFile(HandleFile* &pHandleFile, const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pTag, BBTagInfo& pTagInfo, const uint64_t pHandle)
{
    int rc = 0;
    char l_ArchivePath[PATH_MAX-64] = {'\0'};
    char l_ArchivePathWithName[PATH_MAX] = {'\0'};

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, HF_Save1, "saveHandleFile, counter=%ld, jobid=%ld, handle=%ld", l_FL_Counter, pJobId, pHandle, 0);

    string l_DataStorePath = config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH);
    snprintf(l_ArchivePath, sizeof(l_ArchivePath), "%s/%lu/%lu/%lu", l_DataStorePath.c_str(), pJobId, pJobStepId, pHandle);
    snprintf(l_ArchivePathWithName, sizeof(l_ArchivePathWithName), "%s/%lu", l_ArchivePath, pHandle);
    LOG(bb,debug) << "saveHandleFile (created): l_ArchiveName=" << l_ArchivePathWithName;
    ofstream l_ArchiveFile{l_ArchivePathWithName};
    text_oarchive ha{l_ArchiveFile};

    try
    {
        if (pHandleFile)
        {
            rc = saveHandleFile(pHandleFile, pLVKey, pJobId, pJobStepId, pHandle);
        }
        else
        {
            HandleFile l_HandleFile = HandleFile(pTag, pTagInfo);
            ha << l_HandleFile;
//            l_HandleFile.dump("xbbServer: Saved handle file contents (created)");

            // Create file used to serialize access to the HandleFile
            // NOTE: We need to lock the HandleFile if it can be updated within
            //       a given code path.  Multiple bbServers may be attempting to
            //       update the file concurrently which requires the lock.
            //       We lock the file exclusively in the update path.  Otherwise,
            //       no lock is obtained on the HandleFile for read only.
            // NOTE: We cannot lock the handle file directly, so we create a "lockfile"
            //       in the handle directory.  We cannot lock the handle file directly
            //       because when the load archive code is run it performs a close
            //       for the file which prematurely drops the file lock.
            rc = createLockFile(l_ArchivePath);
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    if (!pHandleFile)
    {
        FL_Write(FLMetaData, HF_Save1_End, "saveHandleFile, counter=%ld, new handle file saved for jobid=%ld, handle=%ld, rc=%ld", l_FL_Counter, pJobId, pHandle, rc);
    }
    else
    {
        FL_Write6(FLMetaData, HF_Save1b_End, "saveHandleFile, counter=%ld, handle=%ld, flags=0x%lx, transfer size=%ld, reporting contribs=%ld, rc=%ld",
                  l_FL_Counter, pHandle, pHandleFile->flags, pHandleFile->totalTransferSize, (uint64_t)pHandleFile->numReportingContribs, rc);
    }

    return rc;
}

int HandleFile::saveHandleFile(HandleFile* &pHandleFile, const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle)
{
    int rc = 0;
    stringstream errorText;
    char l_ArchiveName[PATH_MAX] = {'\0'};

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, HF_Save2, "saveHandleFile, counter=%ld, jobid=%ld, handle=%ld", l_FL_Counter, pJobId, pHandle, 0);

    string l_DataStorePath = config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH);
    snprintf(l_ArchiveName, sizeof(l_ArchiveName), "%s/%lu/%lu/%lu/%lu", l_DataStorePath.c_str(), pJobId, pJobStepId, pHandle, pHandle);
    LOG(bb,debug) << "saveHandleFile (passed): l_ArchiveName=" << l_ArchiveName;
    ofstream l_ArchiveFile{l_ArchiveName};
    text_oarchive l_Archive{l_ArchiveFile};

    if (pHandleFile)
    {
        try
        {
            l_Archive << *pHandleFile;
//            pHandleFile->dump("xbbServer: Saved handle file contents (passed)");
        }
        catch(ExceptionBailout& e) { }
        catch(exception& e)
        {
            rc = -1;
            LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
        }
    } else {
        rc = -1;
        errorText << "Failure when attempting to save the handle file for job " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle \
                  << ". Pointer to the handle file was passed as NULL.";
        LOG_ERROR_TEXT_RC(errorText, rc);
    }

    if (pHandleFile)
    {
        FL_Write6(FLMetaData, HF_Save2_End, "saveHandleFile, counter=%ld, handle=%ld, flags=0x%lx, transfer size=%ld, reporting contribs=%ld, rc=%ld",
                  l_FL_Counter, pHandle, pHandleFile->flags, pHandleFile->totalTransferSize, (uint64_t)pHandleFile->numReportingContribs, rc);
    }
    else
    {
        FL_Write(FLMetaData, HF_Save2_ErrEnd, "saveHandleFile, counter=%ld, jobid=%ld, handle=%ld", l_FL_Counter, pJobId, pHandle, 0);
    }

    return rc;
}

int HandleFile::testForLock(const char* pFilePath)
{
    int rc = -2;
    int fd = -1;
    stringstream errorText;

    char l_LockFile[PATH_MAX] = {'\0'};
    snprintf(l_LockFile, PATH_MAX, "%s/%s", pFilePath, LOCK_FILENAME);

    struct flock l_LockOptions;

    threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::opensyscall, l_LockFile, __LINE__);
    fd = open(l_LockFile, O_WRONLY);
    threadLocalTrackSyscallPtr->clearTrack();
    if (fd >= 0)
    {
        // Test for the entire file lock
        LOG(bb,debug) << "testForLock(): Open issued for handle file " << l_LockFile << ", fd=" << fd;
        l_LockOptions.l_whence = SEEK_SET;
        l_LockOptions.l_start = 0;
        l_LockOptions.l_len = 0;    // Test for lock of entire file for writing
        l_LockOptions.l_type = F_WRLCK;
        threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::fcntlsyscall, l_LockFile, __LINE__);
        rc = ::fcntl(fd, F_GETLK, &l_LockOptions);
        threadLocalTrackSyscallPtr->clearTrack();
    }

    switch(rc)
    {
        case 0:
        {
            if (l_LockOptions.l_type == F_UNLCK)
            {
//                bberror << err("out.handlefilelocked", "false");
                LOG(bb,debug) << ">>>>> Handle file " << l_LockFile << " is NOT locked exclusive for writing";
            }
            else
            {
                rc = 1;
//                bberror << err("out.handlefilelocked", "true");
                LOG(bb,debug) << ">>>>> Handle file " << l_LockFile << " is locked exclusive for writing";
            }
        }
        break;

        case -2:
        {
            bberror << err("out.handlefilelocked", "Could not open");
            errorText << "Could not open handle file " << l_LockFile << " to test for file lock, errno=" << errno << ":" << strerror(errno);
            LOG_ERROR_TEXT_ERRNO(errorText, errno);
        }
        break;

        default:
        {
            rc = -1;
            bberror << err("out.handlefilelocked", "Could not test");
            errorText << "Could not test for exclusive lock on handle file " << l_LockFile << ", errno=" << errno << ":" << strerror(errno);
            LOG_ERROR_TEXT_ERRNO(errorText, errno);
        }
        break;
    }

    if (fd >= 0)
    {
        LOG(bb,debug) << "testForLock(): Issue close for handle file fd " << fd;
        ::close(fd);
    }

    return rc;
}

void HandleFile::unlock(const int pFd)
{
    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, HF_Unlock, "unlock HF, counter=%ld, fd=%ld", l_FL_Counter, pFd, 0, 0);

    if (pFd >= 0)
    {
        try
        {
            // Unlock the file
            struct flock l_LockOptions;
            l_LockOptions.l_whence = SEEK_SET;
            l_LockOptions.l_start = 0;
            l_LockOptions.l_len = 0;    // Unlock entire file
            l_LockOptions.l_type = F_UNLCK;
            LOG(bb,debug) << "unlock(): Issue unlock for handle file fd " << pFd;
            threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::fcntlsyscall, pFd, __LINE__);
            int rc = ::fcntl(pFd, F_SETLK, &l_LockOptions);
            threadLocalTrackSyscallPtr->clearTrack();
            if (!rc)
            {
                // Successful unlock...
                LOG(bb,debug) << "<<<<<<<<<< Handle file fd " << pFd << " unlocked";
            }
            else
            {
                LOG(bb,warning) << "Could not exclusively unlock handle file fd " << pFd << ", errno=" << errno << ":" << strerror(errno);
            }
        }
        catch(exception& e)
        {
            LOG(bb,info) << "Exception caught " << __func__ << "@" << __FILE__ << ":" << __LINE__ << " what=" << e.what();
        }
    }

    FL_Write(FLMetaData, HF_Unlock_End, "unlock HF, counter=%ld, fd=%ld", l_FL_Counter, pFd, 0, 0);

    return;
}

int HandleFile::update_xbbServerHandleFile(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId, const uint64_t pFlags, const int pValue)
{
    int rc = 0;
    stringstream errorText;
    HandleFile* l_HandleFile = 0;
    char* l_HandleFileName = 0;

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, HF_UpdateFile, "update handle file, counter=%ld, jobid=%ld, handle=%ld", l_FL_Counter, pJobId, pHandle, 0);

    // NOTE: The Handlefile is locked exclusive here to serialize amongst all bbServers that may
    //       be updating simultaneously
    HANDLEFILE_LOCK_FEEDBACK l_LockFeedback;
    rc = loadHandleFile(l_HandleFile, l_HandleFileName, pJobId, pJobStepId, pHandle, LOCK_HANDLEFILE, &l_LockFeedback);
    if (!rc) {
        uint64_t l_Flags = l_HandleFile->flags;
        uint64_t l_NewFlags = 0;
        SET_FLAG_VAR(l_NewFlags, l_Flags, pFlags, (uint32_t)pValue);

        if (!pValue) {
            if (pFlags & BBTD_All_Extents_Transferred) {
                // If we are turning off BBTD_All_Extents_Transferred, also turn off BBTD_All_Files_Closed.
                // NOTE: This occurs during restart for a transfer definition.  This bit should already
                //       be off, as we cannot restart any transfer definition if all extents were already
                //       sent for the handle.  But, it is done here for completeness and to match the logic
                //       that maintains the ContribId file.
                SET_FLAG_VAR(l_NewFlags, l_NewFlags, BBTD_All_Files_Closed, (uint32_t)pValue);
            }
        }

        if (l_Flags != l_NewFlags)
        {
            LOG(bb,info) << "xbbServer: For " << *pLVKey << ", handle " << pHandle << ":";
            LOG(bb,info) << "           Handle flags changing from 0x" << hex << uppercase << l_Flags << " to 0x" << l_NewFlags << nouppercase << dec << ".";
        }

        l_HandleFile->flags = l_NewFlags;

        // Save the handle file...
        rc = saveHandleFile(l_HandleFile, pLVKey, pJobId, pJobStepId, pHandle);

        if (!rc)
        {
            // Only update the handle file status if the status could change
            if ((!pValue) || pFlags & BB_UpdateHandleStatusMask2 || l_NewFlags & BB_UpdateHandleStatusMask3)
            {
                // Update the handle status
                // NOTE:  If turning an attribute off, perform a FULL_SCAN when updating the handle status.
                //        Performing the full scan will re-calculate the attribute value across all bbServers.
                // NOTE:  We can unconditionally indicate that the number of contribs bump value is zero.
                //        We never update the 'extents_enqueued' flag via this update_xbbServerHandleFile() interface.
                rc = update_xbbServerHandleStatus(pLVKey, pJobId, pJobStepId, pHandle, pContribId, 0, 0, ((!pValue) ? FULL_SCAN : NORMAL_SCAN));
            }
        }
    }
    else
    {
        errorText << "Failure when attempting to load the handle file for jobid " << pJobId << ", jobstepid " << pJobStepId  << ", handle " << pHandle;
        LOG_ERROR_TEXT_RC(errorText, rc);
    }

    if (l_HandleFile)
    {
        l_HandleFile->close(l_LockFeedback);

        FL_Write6(FLMetaData, HF_UpdateFile_End, "update handle file, counter=%ld, handle=%ld, flags=0x%lx, transfer size=%ld, reporting contribs=%ld, rc=%ld",
                  l_FL_Counter, pHandle, l_HandleFile->flags, l_HandleFile->totalTransferSize, l_HandleFile->numReportingContribs, rc);
        delete l_HandleFile;
        l_HandleFile = 0;
    }
    else
    {
        FL_Write(FLMetaData, HF_UpdateFile_ErrEnd, "update handle file, counter=%ld, handle=%ld, rc=%ld",
                 l_FL_Counter, pHandle, rc, 0);
    }

    if (l_HandleFileName)
    {
        delete[] l_HandleFileName;
        l_HandleFileName = 0;
    }

    return rc;
}

int HandleFile::update_xbbServerHandleStatus(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId, const int32_t pNumOfContribsBump, const int64_t pSize, const HANDLEFILE_SCAN_OPTION pScanOption)
{
    int rc = 0;
    stringstream errorText;
    HANDLEFILE_SCAN_OPTION l_ScanOption = pScanOption;

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write6(FLMetaData, HF_UpdateStatus, "update handle status, counter=%ld, jobid=%ld, handle=%ld, size=%ld, scan option=%ld",
              l_FL_Counter, pJobId, pHandle, (uint64_t)pSize, pScanOption, 0);

    HandleFile* l_HandleFile = 0;
    char* l_HandleFileName = 0;
    // NOTE: The Handlefile is locked exclusive here to serialize amongst all bbServers that may
    //       be updating simultaneously
    HANDLEFILE_LOCK_FEEDBACK l_LockFeedback;
    rc = loadHandleFile(l_HandleFile, l_HandleFileName, pJobId, pJobStepId, pHandle, LOCK_HANDLEFILE, &l_LockFeedback);
    if (!rc)
    {
        uint64_t l_StartingFlags = l_HandleFile->flags;
        uint64_t l_StartingStatus = l_HandleFile->status;
        uint64_t l_StartingTotalTransferSize = l_HandleFile->totalTransferSize;
        //  NOTE: Full success and canceled are the only final states so we don't have to recalculate the handle status.
        //  NOTE: A canceled handle is a final state because it only becomes canceled after all associated files are
        //        closed and the handle itself is marked as canceled.  No additional transitions can occur.
        //  NOTE: Partial success and stopped are temporary final states until the status is reset during restart processing.
        //        We will ALWAYS do a FULL scan if the current status is BBSTOPPED or BBPARTIALSUCCESS.  Multiple contributors
        //        could be causing the BBSTOPPED or BBPARTIALSUCCESS state, so we always have to do a FULL scan to determine
        //        when, and if, the state should transition to BBINPROGRESS.
        //  NOTE: For a transfer definition having it's status set to failed, canceled, or stopped, update_xbbServerContribIdFile()
        //        first updates the appropriate ContribIdFile.  After updating the ContribIdFile, this method is then invoked to update
        //        the handle status.  It is invoked with a scan option of FULL_SCAN so that the handle file status is properly set.
        //  NOTE: Anytime any attribute flag is being turned off for a transfer definition, this method is invoked to update
        //        the corresponding handle attribute.  It is invoked with a scan option of FULL_SCAN so that the handle attribute
        //        is properly calculated and set.
        //  NOTE: As discussed in the previous note, restart logic is the primary reason attribute flags are turned off for the
        //        appropriate contribid and handle files.  These code paths will all specify a FULL_SCAN so that the proper
        //        corresponding handle file attributes are properly calculated and set.
        //  NOTE: A stopped handle can become canceled;  A failed handle can become stopped;
        //  NOTE: For restart scenarios, a partially successful status can transition to stopped and then to in-progress.
        if (l_StartingStatus == BBPARTIALSUCCESS || l_StartingStatus == BBSTOPPED)
        {
            l_ScanOption = FULL_SCAN;
        }
        if (l_ScanOption == FULL_SCAN)
        {
            FL_Write(FLMetaData, HF_UpdateStatusFullScan, "update handle status, counter=%ld, starting status=%ld, original scan option=%ld, performing FULL_SCAN",
                     l_FL_Counter, (uint64_t)l_StartingStatus, (uint64_t)pScanOption, 0);
        }

        if ((!(l_StartingStatus == BBFULLSUCCESS)) &&
            (!(l_StartingStatus == BBCANCELED)))
        {
            bfs::path handle(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
            handle /= bfs::path(to_string(pJobId));
            handle /= bfs::path(to_string(pJobStepId));
            handle /= bfs::path(to_string(pHandle));

            // Now, determine if all extents have been sent for each contributor
            // and other status indications...
            uint64_t l_AllFilesClosed = 1;           // Optimistic coding...
            uint64_t l_AllExtentsTransferred = 1;    // Optimistic coding...
            uint64_t l_OrigNumberOfHandleReportingContribs = l_HandleFile->getNumOfContribsReported();
            bool l_CanceledDefinitions = false;
            bool l_FailedDefinitions = false;
            bool l_StoppedDefinitions = false;
            bool l_ExitEarly = false;
            for (auto& lvuuid : boost::make_iterator_range(bfs::directory_iterator(handle), {}))
            {
                if(!bfs::is_directory(lvuuid)) continue;
                bfs::path contribs_file = lvuuid.path() / bfs::path("contribs");
                ContribFile* l_ContribFile = 0;
                rc = ContribFile::loadContribFile(l_ContribFile, contribs_file.c_str());
                if (!rc)
                {
                    for (map<uint32_t,ContribIdFile>::iterator ce = l_ContribFile->contribs.begin(); ce != l_ContribFile->contribs.end(); ce++)
                    {
                        if ((ce->second.flags & BBTD_All_Extents_Transferred) == 0)
                        {
                            // Not all extents have been transferred yet...
                            l_AllExtentsTransferred = 0;
                            LOG(bb,debug) << "update_xbbServerHandleStatus(): Contribid " << ce->first << " not all extents transferred";
                            if (l_ScanOption == NORMAL_SCAN)
                            {
                                l_ExitEarly = true;
                                break;
                            }
                        }
                        if ((ce->second.flags & BBTD_All_Files_Closed) == 0)
                        {
                            // Not all files have been closed yet...
                            l_AllFilesClosed = 0;
                            LOG(bb,debug) << "update_xbbServerHandleStatus(): Contribid " << ce->first << " not closed";
                        }
                        if (ce->second.flags & BBTD_Stopped)
                        {
                            l_StoppedDefinitions = true;
                            LOG(bb,debug) << "update_xbbServerHandleStatus(): Contribid " << ce->first << " stopped";
                        }
                        if (ce->second.flags & BBTD_Failed)
                        {
                            l_FailedDefinitions = true;
                            LOG(bb,debug) << "update_xbbServerHandleStatus(): Contribid " << ce->first << " failed";
                        }
                        if (ce->second.flags & BBTD_Canceled)
                        {
                            l_CanceledDefinitions = true;
                            LOG(bb,debug) << "update_xbbServerHandleStatus(): Contribid " << ce->first << " canceled";
                        }
                    }
                    if (l_ExitEarly)
                    {
                        break;
                    }
                }
                else
                {
                    errorText << "update_xbbServerHandleStatus(): Could not load the contrib file from file " << contribs_file.c_str();
                    LOG_ERROR_TEXT_RC(errorText, rc);
                }

                if (l_ContribFile)
                {
                    delete l_ContribFile;
                    l_ContribFile = NULL;
                }

                if (rc || l_ExitEarly)
                {
                    break;
                }
            }

            if (!l_ExitEarly)
            {
                // Reset the appropriate attribute flags
                l_HandleFile->flags &= BB_ResetHandleFileAttributesFlagsMask;

                // Set the attribute flags based upon the scan results above
                if (l_AllExtentsTransferred)
                {
                    l_HandleFile->flags |= BBTD_All_Extents_Transferred;
                }
                if (l_AllFilesClosed)
                {
                    l_HandleFile->flags |= BBTD_All_Files_Closed;
                }
                if (l_StoppedDefinitions)
                {
                    l_HandleFile->flags |= BBTD_Stopped;
                }
            }

            if (pNumOfContribsBump)
            {
                l_HandleFile->numReportingContribs += pNumOfContribsBump;
                l_HandleFile->reportingContribs.push_back(pContribId);
            }

            if (!rc)
            {
                // NOTE: A handle with any failed or individually canceled transfer definitions will remain
                //       in the BBINPROGRESS state until all files are closed under that handle.
                //       Such a handle will then transition to the BBPARTIALSUCCESS state, even if no
                //       data was successfully transferred under that handle.
                //
                //       A handle with any stopped transfer definition(s) will immediately transition to the
                //       BBSTOPPED state, awaiting for the proper restart operation(s) to eventually transition
                //       the handle back to the BBINPROGRESS state.  The handle is transitioned to the STOPPED
                //       state directly by the code setting an individual transfer definition as STOPPED.
                if (!l_StoppedDefinitions)
                {
                    if (l_HandleFile->numReportingContribs == l_HandleFile->numContrib)
                    {
                        // All contributors have reported...
                        l_HandleFile->flags |= BBTI_All_Contribs_Reported;
                        if (!l_AllExtentsTransferred)
                        {
                            l_HandleFile->status = BBINPROGRESS;
                        }
                        else
                        {
                            // All extents have been transferred
                            // NOTE: This indication is turned on in the cross-bbServer metadata
                            //       before it is turned on in the local metadata for the 'last'
                            //       bbServer to do processing for this handle.  It will be turned
                            //       on in the local metadata when BBTagInfo::setAllExtentsTransferred()
                            //       is invoked a little later on in this code path during the processing
                            //       for the last extent.  It is turned on here as this is the only
                            //       place where we determine when all extents have been processed
                            //       across ALL bbServers associated with this handle.
                            if (l_AllFilesClosed)
                            {
                                // All files have been marked as closed (but maybe not successfully,
                                // but then those files are marked as BBFAILED...)
                                if (!l_FailedDefinitions)
                                {
                                    if (!l_CanceledDefinitions)
                                    {
                                        l_HandleFile->status = BBFULLSUCCESS;
                                    }
                                    else if (l_HandleFile->flags & BBTD_Canceled)
                                    {
                                        l_HandleFile->status = BBCANCELED;
                                    }
                                    else
                                    {
                                        l_HandleFile->status = BBPARTIALSUCCESS;
                                    }
                                }
                                else
                                {
                                    l_HandleFile->status = BBPARTIALSUCCESS;
                                }
                            }
                            else
                            {
                                l_HandleFile->status = BBINPROGRESS;
                            }
                        }
                    }
                    else
                    {
                        // Not all contributors have reported
                        if (!l_HandleFile->numReportingContribs)
                        {
                            l_HandleFile->status = BBNOTSTARTED;
                        }
                        else
                        {
                            l_HandleFile->status = BBINPROGRESS;
                        }
                    }
                }
                else
                {
                    l_HandleFile->status = BBSTOPPED;
                }
            }

            if (!rc)
            {
                int64_t l_Size = (int64_t)(l_HandleFile->totalTransferSize) + pSize;
                l_HandleFile->totalTransferSize = (uint64_t)l_Size;

                uint64_t l_EndingStatus = l_HandleFile->status;
                if ( !(pNumOfContribsBump == 0 && l_StartingTotalTransferSize == l_HandleFile->totalTransferSize && l_StartingFlags == l_HandleFile->flags && l_StartingStatus == l_EndingStatus) )
                {
                    LOG(bb,info) << "xbbServer: For jobid " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle << ":";
                    if (pNumOfContribsBump)
                    {
                        if (l_OrigNumberOfHandleReportingContribs < l_HandleFile->numReportingContribs)
                        {
                            LOG(bb,info) << "           Number of reporting contributors increasing from " << l_OrigNumberOfHandleReportingContribs << " to " << l_HandleFile->numReportingContribs << ".";
                        }
                        else
                        {
                            LOG(bb,info) << "           Number of reporting contributors decreasing from " << l_OrigNumberOfHandleReportingContribs << " to " << l_HandleFile->numReportingContribs << ".";
                        }
                    }

                    if (l_StartingTotalTransferSize != l_HandleFile->totalTransferSize)
                    {
                        if (l_StartingTotalTransferSize < l_HandleFile->totalTransferSize)
                        {
                            LOG(bb,info) << "           Handle transferred size increasing from " << l_StartingTotalTransferSize << " to " << l_HandleFile->totalTransferSize << ".";
                        }
                        else
                        {
                            LOG(bb,info) << "           Handle transferred size decreasing from " << l_StartingTotalTransferSize << " to " << l_HandleFile->totalTransferSize << ".";
                        }
                    }

                    if (l_StartingFlags != l_HandleFile->flags)
                    {
                        LOG(bb,info) << "           Handle flags changing from 0x" << hex << uppercase << l_StartingFlags << " to 0x" << l_HandleFile->flags << nouppercase << dec << ".";
                    }

                    if (l_StartingStatus != l_EndingStatus)
                    {
                        char l_StartingStatusStr[64] = {'\0'};
                        char l_EndingStatusStr[64] = {'\0'};
                        getStrFromBBStatus((BBSTATUS)l_StartingStatus, l_StartingStatusStr, sizeof(l_StartingStatusStr));
                        getStrFromBBStatus((BBSTATUS)l_EndingStatus, l_EndingStatusStr, sizeof(l_EndingStatusStr));
                        LOG(bb,info) << "           Status changing from " << l_StartingStatusStr << " to " << l_EndingStatusStr << ".";
                    }

                    // Save the handle file...
                    if (saveHandleFile(l_HandleFile, pLVKey, pJobId, pJobStepId, pHandle))
                    {
                        errorText << "update_xbbServerHandleStatus(): Failure when attempting to save the handle file for " << pLVKey << ", jobid " << pJobId << ", jobstepid " \
                                  << pJobStepId << ", handle " << pHandle;
                        LOG_ERROR_TEXT_RC(errorText, rc);
                    }
                }
            }
        }
        else
        {
            // Final status already set for the handle.  The status value cannot change, nor
            // can any other attribute associated with this handle file change.
        }
    }
    else
    {
        errorText << "update_xbbServerHandleStatus(): Failure when attempting to load the handle file for jobid " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle;
        LOG_ERROR_TEXT_RC(errorText, rc);
    }

    if (l_HandleFile)
    {
        l_HandleFile->close(l_LockFeedback);

        FL_Write6(FLMetaData, HF_UpdateStatus_End, "update handle status, counter=%ld, handle=%ld, flags=0x%lx, transfer size=%ld, reporting contribs=%ld, rc=%ld",
                  l_FL_Counter, pHandle, l_HandleFile->flags, l_HandleFile->totalTransferSize, l_HandleFile->numReportingContribs, rc);
        delete l_HandleFile;
        l_HandleFile = 0;
    }
    else
    {
        FL_Write(FLMetaData, HF_UpdateStatus_ErrEnd, "update handle status, counter=%ld, handle=%ld, rc=%ld",
                 l_FL_Counter, pHandle, rc, 0);
    }

    if (l_HandleFileName)
    {
        delete[] l_HandleFileName;
        l_HandleFileName = 0;
    }

    return rc;
}

#define ATTEMPTS 10
int HandleFile::update_xbbServerHandleTransferKeys(BBTransferDef* pTransferDef, const LVKey* pLVKey, const BBJob pJob, const uint64_t pHandle)
{
    int rc = 0;
    stringstream errorText;

    HandleFile* l_HandleFile = 0;
    char* l_HandleFileName = 0;
    uint64_t l_JobId = 0;
    uint64_t l_JobStepId = 0;
    HANDLEFILE_LOCK_FEEDBACK l_LockFeedback = HANDLEFILE_WAS_NOT_LOCKED;

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, HF_UpdateTransferKeys, "update handle transfer keys, counter=%ld, jobid=%ld, handle=%ld", l_FL_Counter, pJob.getJobId(), pHandle, 0);

    bfs::path datastore(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
    if(!bfs::is_directory(datastore)) return rc;
    int l_catch_count=10;
    vector<string> l_PathJobIds;
    l_PathJobIds.reserve(100);

    // NOTE: The only case where this method will return a non-zero return code is if the xbbServer data store
    //       cannot be found/loaded.  Otherwise, the invoker MUST check the returned pHandleFile and pContribIdFile
    //       pointers for success/information.

    // First, build a vector of jobids that the current uid/gid is authorized to access.
    // We will iterate over these jobids in reverse order, as it is almost always
    // the case that the jobid we want is the last one...
    //
    // NOTE: If we take an exception in the loop below, we do not rebuild this vector
    //       of jobids.  The job could go away, but any jobid expected by the code
    //       below should be in the vector.
    rc = get_xbbServerGetCurrentJobIds(l_PathJobIds);

    if (!rc)
    {
        bool l_AllDone = false;
        while (!l_AllDone)
        {
            l_AllDone = true;
            for(vector<string>::reverse_iterator rit = l_PathJobIds.rbegin(); rit != l_PathJobIds.rend(); ++rit)
            {
                try
                {
                    bfs::path job = bfs::path(*rit);
                    l_JobId = stoull(job.filename().string());
                    for (auto& jobstep : boost::make_iterator_range(bfs::directory_iterator(job), {}))
                    {
                        if (!accessDir(jobstep.path().string()) ) continue;
                        l_JobStepId = stoull(jobstep.path().filename().string());
                        for (auto& handle : boost::make_iterator_range(bfs::directory_iterator(jobstep), {}))
                        {
                            if (handle.path().filename().string() == to_string(pHandle))
                            {
                                // NOTE: The Handlefile is locked exclusive here to serialize amongst all bbServers that may
                                //       be updating simultaneously
                                rc = loadHandleFile(l_HandleFile, l_HandleFileName, l_JobId, l_JobStepId, pHandle, LOCK_HANDLEFILE, &l_LockFeedback);
                                if (!rc)
                                {
                                    string l_TransferKeys = l_HandleFile->transferKeys;
                                    LOG(bb,debug) << "update_xbbServerHandleTransferKeys() after load: l_TransferKeys=" << l_TransferKeys;

                                    boost::property_tree::ptree l_PropertyTree;
                                    if (l_TransferKeys.length())
                                    {
                                        std::istringstream l_InputStream(l_TransferKeys);
                                        boost::property_tree::read_json(l_InputStream, l_PropertyTree);
                                    }

                                    for(auto& e : pTransferDef->keyvalues)
                                    {
                                        l_PropertyTree.put((char*)e.first.c_str(), (char*)e.second.c_str());
                                    }

                                    std::ostringstream l_OutputStream;
                                    boost::property_tree::write_json(l_OutputStream, l_PropertyTree, false);
                                    l_TransferKeys = l_OutputStream.str();
                                    LOG(bb,debug) << "update_xbbServerHandleTransferKeys() after insert: l_TransferKeys=" << l_TransferKeys;
                                    l_HandleFile->transferKeys = l_TransferKeys;
                                    LOG(bb,debug) << "update_xbbServerHandleTransferKeys() after insert: l_HandleFile->transferKeys=" << l_HandleFile->transferKeys;

                                    rc = saveHandleFile(l_HandleFile, pLVKey, pJob.getJobId(), pJob.getJobStepId(), pHandle);
                                }
                                else
                                {
                                    errorText << "Failure when attempting to load the handle file for jobid " << pJob.getJobId() << ", jobstepid " << pJob.getJobStepId() << ", handle " << pHandle;
                                    LOG_ERROR_TEXT_RC(errorText, rc);
                                }
                            }
                        }
                    }
                }
                catch(exception& e)
                {
                    if (l_HandleFileName)
                    {
                        delete[] l_HandleFileName;
                        l_HandleFileName = 0;
                    }
                    if (l_HandleFile)
                    {
                        l_HandleFile->close(l_LockFeedback);

                        delete l_HandleFile;
                        l_HandleFile = 0;
                    }
                    if (--l_catch_count)
                    {
                        // NOTE:  'No entry' is an expected error due to a concurrent removeJobInfo.
                        //        If not that, log the error and retry.  Before retrying, remove the
                        //        jobid that failed...
                        if (errno != ENOENT)
                        {
                            LOG(bb,warning) << "Exception caught " << __func__ << "@" << __FILE__ << ":" << __LINE__ << " what=" << e.what() \
                                            << ". Retrying the operation...";
                        }
    		        	advance(rit, 1);
    			        l_PathJobIds.erase(rit.base());
                        l_AllDone = false;
                    }
                    else //RAS
                    {
                        rc=-1;
                        LOG(bb,error) << "Exception caught " << __func__ << "@" << __FILE__ << ":" << __LINE__ << " what=" << e.what();
                        errorText << "update_xbbServerHandleTransferKeys(): exception updating transfer keys for jobid " << pJob.getJobId() << ", jobstepid " << pJob.getJobStepId() << ", handle " << pHandle;
                        LOG_ERROR_TEXT_ERRNO_AND_RAS(errorText, errno, bb.internal.updatexferkeys);
                        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
                    }

                    break;
                }
            }
        }
    }
    else
    {
        // bberror has already been filled in
    }

    if (l_HandleFile)
    {
        l_HandleFile->close(l_LockFeedback);

        delete l_HandleFile;
        l_HandleFile = 0;
    }

    if (l_HandleFileName)
    {
        delete[] l_HandleFileName;
        l_HandleFileName = 0;
    }

    FL_Write6(FLMetaData, HF_UpdateTransferKeys_End, "update handle transfer keys, counter=%ld, jobid=%ld, handle=%ld, attempts=%ld, rc=%ld",
              l_FL_Counter, pJob.getJobId(), pHandle, (uint64_t)(l_catch_count == ATTEMPTS ? 1 : ATTEMPTS-l_catch_count), rc, 0);

    return rc;
}
#undef ATTEMPTS

/*
 * Non-static methods
 */

void HandleFile::close(HANDLEFILE_LOCK_FEEDBACK pLockFeedback)
{
    if (pLockFeedback == HANDLEFILE_WAS_LOCKED)
    {
        close(handleFileLockFd);
    }
}

void HandleFile::close(const int pFd)
{
    if (pFd >= 0)
    {
        uint64_t l_FL_Counter = metadataCounter.getNext();
        FL_Write(FLMetaData, HF_Close, "close HF, counter=%ld, fd=%ld", l_FL_Counter, pFd, 0, 0);

        if (pFd == handleFileLockFd)
        {
            unlock();
        }
        LOG(bb,debug) << "close(): Issue close for handle file fd " << pFd;

        ::close(pFd);

        FL_Write(FLMetaData, HF_Close_End, "close HF, counter=%ld, fd=%ld", l_FL_Counter, pFd, 0, 0);
    }
}

void HandleFile::getContribArray(uint64_t &pNumContribsInArray, uint32_t* &pContribArray)
{
    if (!pContribArray)
    {
        uint64_t l_LengthOfContribArray = expectContrib.length()-2;         // Do not include the parens...
        uint64_t l_NumOfContribs = (l_LengthOfContribArray/2)+1;            // Number of contributors
        LOG(bb,debug) << "HandleFile::getContribArray(): l_NumOfContribs=" << l_NumOfContribs << ", l_LengthOfContribArray=" << l_LengthOfContribArray << ", expectContrib=" << expectContrib;
        pContribArray = (uint32_t*)(new char[l_NumOfContribs*sizeof(*pContribArray)]);
        stringstream l_ContribSS(expectContrib.substr(1, l_LengthOfContribArray));

        int32_t l_Contributor;
        uint64_t l_Index = 0;
        while (l_ContribSS >> l_Contributor)
        {
            pContribArray[l_Index++] = l_Contributor;
            if (l_ContribSS.peek() == ',')
            {
                l_ContribSS.ignore();
            }
        }

        pNumContribsInArray = l_Index;
    }
    else
    {
        LOG(bb,error) << "HandleFile::getContribArray(): Invoked with non-null pContribArray";
    }

    return;
}

BBSTATUS HandleFile::getLocalStatus(const uint64_t pNumberOfReportingContribs, ContribIdFile* pContribIdFile)
{
    BBSTATUS l_LocalStatus = BBNONE;

    if (!pContribIdFile->anyFilesStopped()) {
        if (!pContribIdFile->anyFilesFailed()) {
            if (!pContribIdFile->anyFilesCanceled()) {
                if (pNumberOfReportingContribs) {
                    if (pContribIdFile->flags & BBTD_All_Files_Closed) {
                        l_LocalStatus = BBFULLSUCCESS;
                    } else {
                        if ((BBSTATUS)status == BBNOTSTARTED) {
                            l_LocalStatus = BBNOTSTARTED;
                        } else {
                            l_LocalStatus = BBINPROGRESS;
                        }
                    }
                } else {
                    l_LocalStatus = BBNOTSTARTED;
                }
            } else {
                l_LocalStatus = BBCANCELED;
            }
        } else {
            l_LocalStatus = BBFAILED;
        }
    } else {
        l_LocalStatus = BBSTOPPED;
    }

    return l_LocalStatus;
}

void HandleFile::unlock()
{
    if (handleFileLockFd != -1)
    {
        unlock(handleFileLockFd);
        handleFileLockFd = -1;
        lockfd = -1;
    }
}
