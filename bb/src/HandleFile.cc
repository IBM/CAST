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
#include "ContribIdFile.h"
#include "HandleFile.h"
#include "LVUuidFile.h"
#include "tracksyscall.h"
#include <dirent.h>


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

int HandleFile::get_xbbServerGetJobForHandle(uint64_t& pJobId, uint64_t& pJobStepId, const uint64_t pHandle) {
    int rc = -1;
    stringstream errorText;

    pJobId = 0;
    pJobStepId = 0;

    bfs::path datastore(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
    if(bfs::is_directory(datastore))
    {
        bool l_AllDone = false;
        int l_catch_count=10;

        while (!l_AllDone)
        {
            l_AllDone = true;
            for (auto& job : boost::make_iterator_range(bfs::directory_iterator(datastore), {}))
            {
               if (!rc) continue;
               try
               {
                   if ((!rc) || (!accessDir( job.path().string() ) ) ) continue;
                   for (auto& jobstep : boost::make_iterator_range(bfs::directory_iterator(job), {}))
                   {if(!accessDir(jobstep.path().string()) ) continue;
                       if ((!rc) || (!accessDir(jobstep.path().string()) ) ) continue;
                       for (auto& handle : boost::make_iterator_range(bfs::directory_iterator(jobstep), {}))
                       {
                           if (handle.path().filename().string() == to_string(pHandle))
                           {
                               rc = 0;
                               pJobId = stoul(job.path().filename().string());
                               pJobStepId = stoul(jobstep.path().filename().string());
                               break;
                           }
                       }
                   }
               }
               catch(exception& e)
               {
                   LOG(bb,info) << "Exception caught "<<__func__<<"@"<<__FILE__<<":"<<__LINE__<<" what="<<e.what();
                   if (l_catch_count--)
                   {
                       l_AllDone = false;
                   }
                   else //RAS
                   {
                       rc=-1;
                       errorText << "get_xbbServerGetJobForHandle(): exception in looking for handle " << pHandle;
                       LOG_ERROR_TEXT_ERRNO_AND_RAS(errorText, errno, bb.internal.getjobforhandle);
                       LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
                   }
                   break;
               }
            }
        }
    }

    return rc;
}

int HandleFile::get_xbbServerGetHandle(BBJob& pJob, uint64_t pTag, vector<uint32_t>& pContrib, uint64_t& pHandle)
{
    int rc = 0;
    stringstream errorText;

    uint32_t* l_ContribArray = 0;
    HandleFile* l_HandleFile = 0;
    uint64_t l_NumOfContribsInArray = 0;

    pHandle = 0;

    bfs::path datastore(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
    if(bfs::is_directory(datastore))
    {
        bool l_AllDone = false;
        int l_catch_count=10;

        while (!l_AllDone)
        {
            l_AllDone = true;
            for(auto& job : boost::make_iterator_range(bfs::directory_iterator(datastore), {}))
            {
                if(!accessDir( job.path().string() ) ) continue;
                try
                {
                    if(job.path().filename().string() == to_string(pJob.getJobId()))
                    {
                        for(auto& jobstep : boost::make_iterator_range(bfs::directory_iterator(job), {}))
                        {
                            if(!accessDir(jobstep.path().string()) ) continue;
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
                }
                catch(exception& e)
                {
                    LOG(bb,info) << "Exception caught "<<__func__<<"@"<<__FILE__<<":"<<__LINE__<<" what="<<e.what();
                    if (l_catch_count--)
                    {
                        l_AllDone = false;
                    }
                    else //RAS
                    {
                        rc=-1;
                        errorText << "get_xbbServerGetHandle(): exception in looking for handle for jobid " << pJob.getJobId() << ", jobstepid " << pJob.getJobStepId()  << ", tag " << pTag << ", number of contribs " << pContrib.size();
                        LOG_ERROR_TEXT_ERRNO_AND_RAS(errorText, errno, bb.internal.gethandle);
                        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
                    }
                    break;
                }
            }
        }
    }
    else
    {
        rc = -1;
        errorText << "get_xbbServerGetHandle(): Could not find handle for jobid " << pJob.getJobId() << ", jobstepid " << pJob.getJobStepId()  << ", tag " << pTag << ", number of contribs " << pContrib.size();
        LOG_ERROR_TEXT_RC(errorText, rc);
    }

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

    uint64_t l_JobId = 0;
    uint64_t l_JobStepId = 0;
//    char* l_HandleFileName = 0;

    // NOTE: The only case where this method will return a non-zero return code is if the xbbServer data store
    //       cannot be found/loaded.  Otherwise, the invoker MUST check the returned pHandleFile and pContribIdFile
    //       pointers for success/information.

    bfs::path datastore(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
    if(bfs::is_directory(datastore))
    {
        bool l_AllDone = false;
        int l_catch_count=10;
        while (!l_AllDone)
        {
            l_AllDone = true;
            for(auto& job : boost::make_iterator_range(bfs::directory_iterator(datastore), {}))
            {
                if (!accessDir( job.path().string() ) )continue;
                try
                {
                    l_JobId = stoull(job.path().filename().string());
                    for(auto& jobstep : boost::make_iterator_range(bfs::directory_iterator(job), {}))
                    {
                        if(!accessDir(jobstep.path().string()) ) continue;
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
                                            LOG(bb,error) << "Could not load the contribid file for jobid " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", contribid " << pContribId << ", using handle path " << handle.path().string();
                                        }
                                    }
                                }
                                else
                                {
                                    LOG(bb,error) << "Could not load the handle file for jobid " << l_JobId << ", jobstepid " << l_JobStepId << ", handle " << pHandle << ", using handle path " << handle.path().string();
                                }
                            }
                        }
                    }
                }
                catch(exception& e)
                {
                    LOG(bb,info) << "Exception caught "<<__func__<<"@"<<__FILE__<<":"<<__LINE__<<" what="<<e.what();

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
                    if (l_catch_count--)
                    {
                        l_AllDone = false;
                    }
                    else //RAS
                    {
                        rc=-1;
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
        rc = -1;
        errorText << "get_xbbServerHandleInfo(): Could not find handle " << pHandle << ", contribid " << pContribId;
        LOG_ERROR_TEXT_RC(errorText, rc);
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

    return rc;
}

int HandleFile::get_xbbServerHandleList(std::vector<uint64_t>& pHandles, const BBJob pJob, const BBSTATUS pMatchStatus)
{
    int rc = 0;

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

    return rc;
}

int HandleFile::get_xbbServerHandleStatus(BBSTATUS& pStatus, const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle) {
    int rc = 0;
    stringstream errorText;

    pStatus = BBNONE;

    HandleFile* l_HandleFile = 0;
    char* l_HandleFileName = 0;
    rc = loadHandleFile(l_HandleFile, l_HandleFileName, pJobId, pJobStepId, pHandle);
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

    return rc;
}


int HandleFile::get_xbbServerHandleTransferKeys(string& pTransferKeys, const uint64_t pJobId, const uint64_t pHandle)
{
    int rc = 0;
    stringstream errorText;

    pTransferKeys = "";

    bfs::path datastore(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
    if(!bfs::is_directory(datastore)) return rc;
    bool l_AllDone = false;
    int l_catch_count=10;

    while (!l_AllDone)
    {
        l_AllDone = true;

        for(auto& job : boost::make_iterator_range(bfs::directory_iterator(datastore), {}))
        {
            if (!accessDir( job.path().string() ) )continue;
            try
            {
                if(job.path().filename().string() == to_string(pJobId))
                {
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
            }
            catch(exception& e)
            {
                LOG(bb,info) << "Exception caught "<<__func__<<"@"<<__FILE__<<":"<<__LINE__<<" what="<<e.what();
                if (l_catch_count--)
                {
                    l_AllDone = false;
                }
                else //RAS
                {
                    rc=-1;
                    errorText << "get_xbbServerHandleTransferKeys(): exception looking for transfer keys for jobid " << pJobId << ", handle " << pHandle;
                    LOG_ERROR_TEXT_ERRNO_AND_RAS(errorText, errno, bb.internal.xferkeys);
                    LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
                }
                break;
            }
        }
    }

    return rc;
}

int HandleFile::loadHandleFile(HandleFile* &ptr, const char* filename)
{
    HandleFile* tmparchive = NULL;
    ptr = NULL;

    int rc = -1;
    bool doover;
    bool didretry = false;
    struct timeval start, stop;
    int l_LastConsoleOutput = -1;

    start.tv_sec = 0; // resolve gcc optimizer complaint
    LOG(bb,debug) << __func__ << "  ArchiveName=" << filename;

    do
    {
        doover = false;
        try
        {
            ifstream l_ArchiveFile{filename};
            text_iarchive l_Archive{l_ArchiveFile};
            if (!tmparchive)
            {
                tmparchive = new HandleFile();
            }
            l_Archive >> *tmparchive;
            ptr = tmparchive;
            rc = 0;
        }
        catch(ExceptionBailout& e)
        {
            rc = -1;
            if(tmparchive)
            {
                delete tmparchive;
            }
            ptr = tmparchive = NULL;
        }
        catch(archive_exception& e)
        {
            rc = -1;

            gettimeofday(&stop, NULL);
            if (didretry == false)
            {
                start = stop;
                didretry = true;
            }

            int l_Time = int(stop.tv_sec - start.tv_sec);
            if (l_Time < 30)
            {
                doover = true;
            }

            if (((l_Time % 5) == 0) && (l_Time != l_LastConsoleOutput))
            {
                l_LastConsoleOutput = l_Time;
                LOG(bb,warning) << "Archive exception thrown in " << __func__ << " was " << e.what() \
                                << " when attempting to load archive " << filename << "  Retrying..." << " time=" << l_Time << " second(s)";
            }
        }
        catch(exception& e)
        {
            rc = -1;
            LOG(bb,error) << "Exception thrown in " << __func__ << " was " << e.what() << " when attempting to load archive " << filename;
        }

        if (doover)
        {
            usleep(250000);
        }
    }
    while (rc && doover);

    if (!rc)
    {
        if (ptr)
        {
            // NOTE: After the initial load, we always set the lockfd
            //       value to -1.  A prior fd value could have been saved.
            //       If this load request has also locked the Handlefile,
            //       then the lockfd value will be filled in by our invoker.
            ptr->lockfd = -1;
        }
        if (didretry)
        {
            gettimeofday(&stop, NULL);
            LOG(bb,info) << __func__ << " became successful after recovering from exception after " << (stop.tv_sec-start.tv_sec) << " second(s)";
        }
    }

    if (rc && tmparchive)
    {
        delete tmparchive;
        tmparchive = NULL;
    }

    return rc;
}

int HandleFile::loadHandleFile(HandleFile* &pHandleFile, char* &pHandleFileName, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const int pLockOption)
{
    int rc = 0;
    stringstream errorText;

    int fd = -1;
    char l_ArchivePath[PATH_MAX-64] = {'\0'};
    char* l_ArchivePathWithName = new char[PATH_MAX];

    string l_DataStorePath = config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH);
    snprintf(l_ArchivePath, PATH_MAX-64, "%s/%lu/%lu/%lu", l_DataStorePath.c_str(), pJobId, pJobStepId, pHandle);
    snprintf(l_ArchivePathWithName, PATH_MAX, "%s/%lu", l_ArchivePath, pHandle);

    switch (pLockOption)
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
        if (pLockOption == DO_NOT_LOCK_HANDLEFILE ||
            (pLockOption == LOCK_HANDLEFILE && fd >= 0) ||
            (pLockOption == LOCK_HANDLEFILE_WITH_TEST_FIRST && fd >= 0))
        {
            rc = loadHandleFile(pHandleFile, l_ArchivePathWithName);
            if (!rc)
            {
                pHandleFileName = l_ArchivePathWithName;
                switch (pLockOption)
                {
                    case LOCK_HANDLEFILE:
                    case LOCK_HANDLEFILE_WITH_TEST_FIRST:
                    {
                        pHandleFile->lockfd = fd;
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
        if (pLockOption == LOCK_HANDLEFILE || pLockOption == LOCK_HANDLEFILE_WITH_TEST_FIRST)
        {
            unlock(fd);
        }
        LOG(bb,debug) << "loadHandleFile(): Issue close for handle file fd " << fd;
        ::close(fd);
    }

    return rc;
}

int HandleFile::lock(const char* pFilePath)
{
    int rc = -2;
    int fd = -1;
    stringstream errorText;

    char l_LockFile[PATH_MAX] = {'\0'};
    snprintf(l_LockFile, PATH_MAX, "%s/%s", pFilePath, LOCK_FILENAME);

    threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::opensyscall, l_LockFile, __LINE__);
    fd = open(l_LockFile, O_WRONLY);
    threadLocalTrackSyscallPtr->clearTrack();
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
                ::close(fd);
            }
            fd = -1;
        }
        break;

        default:
            // Successful lock...
            LOG(bb,debug) << "lock(): Handle file " << l_LockFile << " locked, fd " << fd;
            break;
    }

    return fd;
}

int HandleFile::processTransferHandleForJobStep(std::vector<uint64_t>& pHandles, const char* pDataStoreName, const BBSTATUS pMatchStatus)
{
    int rc = 0;
    HandleFile* l_HandleFile = 0;

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

    return rc;
}

int HandleFile::saveHandleFile(HandleFile* &pHandleFile, const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pTag, BBTagInfo& pTagInfo, const uint64_t pHandle)
{
    int rc = 0;
    char l_ArchivePath[PATH_MAX-64] = {'\0'};
    char l_ArchivePathWithName[PATH_MAX] = {'\0'};

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

    return rc;
}

int HandleFile::saveHandleFile(HandleFile* &pHandleFile, const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle)
{
    int rc = 0;
    stringstream errorText;
    char l_ArchiveName[PATH_MAX] = {'\0'};

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

        // If the Handlefile was locked, this close will drop that lock
        pHandleFile->close();

    } else {
        rc = -1;
        errorText << "Failure when attempting to save the handle file for job " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle \
                  << ". Pointer to the handle file was passed as NULL.";
        LOG_ERROR_TEXT_RC(errorText, rc);
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
                LOG(bb,debug) << "unlock(): Handle file fd " << pFd << " unlocked";
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
}

int HandleFile::update_xbbServerHandleFile(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint64_t pFlags, const int pValue)
{
    int rc = 0;
    stringstream errorText;
    HandleFile* l_HandleFile = 0;
    char* l_HandleFileName = 0;

    // NOTE: The Handlefile is locked exclusive here to serialize amongst all bbServers that may
    //       be updating simultaneously
    rc = loadHandleFile(l_HandleFile, l_HandleFileName, pJobId, pJobStepId, pHandle, LOCK_HANDLEFILE);
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
            // Update the handle status
            rc = update_xbbServerHandleStatus(pLVKey, pJobId, pJobStepId, pHandle, 0);
        }
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
    if (l_HandleFile)
    {
        // The lock on the handle file should have been released by the save code
        // above.  We ensure that it is unlocked here (e.g. the save code didn't run above)
        l_HandleFile->close();

        delete l_HandleFile;
        l_HandleFile = 0;
    }

    return rc;
}

int HandleFile::update_xbbServerHandleResetStatus(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle)
{
    int rc = 0;
    stringstream errorText;

    HandleFile* l_HandleFile = 0;
    char* l_HandleFileName = 0;
    // NOTE: The Handlefile is locked exclusive here to serialize amongst all bbServers that may
    //       be updating simultaneously
    rc = loadHandleFile(l_HandleFile, l_HandleFileName, pJobId, pJobStepId, pHandle, LOCK_HANDLEFILE);
    if (!rc)
    {
        l_HandleFile->status = BBNONE;
        l_HandleFile->flags &= BB_ResetHandleFileForRestartFlagsMask;
        LOG(bb,info) << "xbbServer: Flags reset prior to restart for " << *pLVKey << ", handle " << pHandle;
        rc = saveHandleFile(l_HandleFile, pLVKey, pJobId, pJobStepId, pHandle);
    }

    if (l_HandleFileName)
    {
        delete[] l_HandleFileName;
        l_HandleFileName = 0;
    }
    if (l_HandleFile)
    {
        // The lock on the handle file should have been released by the save code
        // above.  We ensure that it is unlocked here (e.g. the save code didn't run above)
        l_HandleFile->close();

        delete l_HandleFile;
        l_HandleFile = 0;
    }

    return rc;
}

int HandleFile::update_xbbServerHandleStatus(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const int64_t pSize)
{
    int rc = 0;
    stringstream errorText;

    HandleFile* l_HandleFile = 0;
    char* l_HandleFileName = 0;
    // NOTE: The Handlefile is locked exclusive here to serialize amongst all bbServers that may
    //       be updating simultaneously
    rc = loadHandleFile(l_HandleFile, l_HandleFileName, pJobId, pJobStepId, pHandle, LOCK_HANDLEFILE);
    if (!rc)
    {
        uint64_t l_StartingFlags = l_HandleFile->flags;
        uint64_t l_StartingStatus = l_HandleFile->status;
        uint64_t l_StartingTotalTransferSize = l_HandleFile->totalTransferSize;
        //  NOTE: Full success and canceled are the only final states so we don't have to recalculate the handle state.
        //  NOTE: A canceled handle is a final state because it only becomes canceled after all associated files are
        //        closed and the handle itself is marked as canceled.  No additional transitions can occur.
        //  NOTE: A stopped handle can become canceled;  A failed handle can become stopped;
        //  NOTE: For restart scenarios, a partially successful status can transition to stopped and then to in-progress.
        if ( (!(l_StartingStatus == BBFULLSUCCESS)) && (!(l_StartingStatus == BBCANCELED)) )
        {
            uint64_t l_AllFilesClosed;
            uint64_t l_AllExtentsTransferred;
#ifndef __clang_analyzer__
            l_AllFilesClosed = l_HandleFile->flags & BBTD_All_Files_Closed;
            l_AllExtentsTransferred = l_HandleFile->flags & BBTD_All_Extents_Transferred;
#endif

            bfs::path handle(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
            handle /= bfs::path(to_string(pJobId));
            handle /= bfs::path(to_string(pJobStepId));
            handle /= bfs::path(to_string(pHandle));

            uint64_t l_NumberOfHandleReportingContribs = 0;
            uint64_t l_NumberOfLVUuidReportingContribs = 0;
            ContribIdFile* l_ContribIdFile = 0;
            // NOTE:  The following call is made to obtain the number of reporting contributors for the handle.  Regardless of what contribid
            //        value is passed, the number of reporting contributors for the handle is returned.  Therefore, we simply pass 0 for the
            //        contribid value.
            rc = ContribIdFile::loadContribIdFile(l_ContribIdFile, l_NumberOfHandleReportingContribs, l_NumberOfLVUuidReportingContribs, handle, 0);
            switch (rc)
            {
                case 0:
                case 1:
                {
                    rc = 0;

                    // Now, determine if all extents have been sent for each contributor
                    // and other status indications...
                    l_AllFilesClosed = 1;           // Optimistic coding...
                    l_AllExtentsTransferred = 1;    // Optimistic coding...
                    bool l_CanceledDefinitions = false;
                    bool l_FailedDefinitions = false;
                    bool l_StoppedDefinitions = false;
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
                                if ((ce->second.flags & BBTD_All_Files_Closed) == 0)
                                {
                                    l_AllFilesClosed = 0;
                                    LOG(bb,debug) << "update_xbbServerHandleStatus(): Contribid " << ce->first << " not closed";
                                }
                                if ((ce->second.flags & BBTD_All_Extents_Transferred) == 0)
                                {
                                    l_AllExtentsTransferred = 0;
                                    LOG(bb,debug) << "update_xbbServerHandleStatus(): Contribid " << ce->first << " not all extents transferred";
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

                        if (rc)
                        {
                            break;
                        }
                    }

                    if (!rc)
                    {
                        // NOTE: A handle with any failed or individually canceled transfer definitions will remain
                        //       in the BBINPROGRESS state until all files are closed under that handle.
                        //       Such a handle will then transition to the BBPARTIALSUCCESS state, even if no
                        //       data was successfully transferred under that handle.
                        //       A handle with any stopped transfer definition(s) will immediately transition to the
                        //       BBSTOPPED state, awaiting for the proper restart operation(s) to eventually transition
                        //       the handle back to the BBINPROGRESS state.
                        l_HandleFile->status = BBNONE;
                        if ((!(l_HandleFile->flags & BBTD_Stopped)) && (!l_StoppedDefinitions))
                        {
                            // NOTE: BBFAILED is not currently set for a handle...
                            if (!(l_HandleFile->flags & BBTD_Failed))
                            {
                                if (l_NumberOfHandleReportingContribs == l_HandleFile->numContrib)
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
                                        l_HandleFile->flags |= BBTD_All_Extents_Transferred;
                                        if (l_AllFilesClosed)
                                        {
                                            // All files have been marked as closed (but maybe not successfully,
                                            // but then those files are marked as BBFAILED...)
                                            l_HandleFile->flags |= BBTD_All_Files_Closed;
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
                                    if (!l_NumberOfHandleReportingContribs)
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
                                // NOTE:  Not currently set for the HandleFile...
                                l_HandleFile->status = BBFAILED;
                            }
                        }
                        else
                        {
                            // Mark the HandleFile as soon as we see any 'underlying' file marked as stopped
                            l_HandleFile->flags |= BBTD_Stopped;
                            l_HandleFile->status = BBSTOPPED;
                        }
                    }

                    break;
                }

                default:
                {
                    errorText << "update_xbbServerHandleStatus(): Could not load the contribid file from file " << handle.string();
                    LOG_ERROR_TEXT_RC(errorText, rc);
                }
            }

            if (l_ContribIdFile)
            {
                delete l_ContribIdFile;
                l_ContribIdFile = 0;
            }

            if (!rc)
            {
                int64_t l_Size = (int64_t)(l_HandleFile->totalTransferSize) + pSize;
                l_HandleFile->totalTransferSize = (uint64_t)l_Size;

                uint64_t l_EndingStatus = l_HandleFile->status;
                if ( !(l_StartingTotalTransferSize == l_HandleFile->totalTransferSize && l_StartingFlags == l_HandleFile->flags && l_StartingStatus == l_EndingStatus) )
                {
                    LOG(bb,info) << "xbbServer: For jobid " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle << ":";
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
        else
        {
            // Status already set for handle...
        }
    }
    else
    {
        errorText << "update_xbbServerHandleStatus(): Failure when attempting to load the handle file for jobid " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle;
        LOG_ERROR_TEXT_RC(errorText, rc);
    }

    if (l_HandleFileName)
    {
        delete[] l_HandleFileName;
        l_HandleFileName = 0;
    }
    if (l_HandleFile)
    {
        // The lock on the handle file should have been released by the save code
        // above.  We ensure that it is unlocked here (e.g. the save code didn't run above)
        l_HandleFile->close();

        delete l_HandleFile;
        l_HandleFile = 0;
    }

    return rc;
}

int HandleFile::update_xbbServerHandleTransferKeys(BBTransferDef* pTransferDef, const LVKey* pLVKey, const BBJob pJob, const uint64_t pHandle)
{
    int rc = 0;
    stringstream errorText;

    HandleFile* l_HandleFile = 0;
    char* l_HandleFileName = 0;
    uint64_t l_JobId = 0;
    uint64_t l_JobStepId = 0;

    bfs::path datastore(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
    if(!bfs::is_directory(datastore)) return rc;
    bool l_AllDone = false;
    int l_catch_count=10;

    while (!l_AllDone)
    {
        l_AllDone = true;

        for(auto& job : boost::make_iterator_range(bfs::directory_iterator(datastore), {}))
        {
            if (!accessDir( job.path().string() ) )continue;
            try
            {
                l_JobId = stoull(job.path().filename().string());
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
                            rc = loadHandleFile(l_HandleFile, l_HandleFileName, l_JobId, l_JobStepId, pHandle, LOCK_HANDLEFILE);
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
                LOG(bb,info) << "Exception caught "<<__func__<<"@"<<__FILE__<<":"<<__LINE__<<" what="<<e.what();
                if (l_HandleFileName)
                {
                    delete[] l_HandleFileName;
                    l_HandleFileName = 0;
                }
                if (l_HandleFile)
                {
                    // The lock on the handle file should have been released by the save code
                    // above.  We ensure that it is unlocked here (e.g. the save code didn't run above)
                    l_HandleFile->close();

                    delete l_HandleFile;
                    l_HandleFile = 0;
                }
                if (l_catch_count--)
                {
                    l_AllDone = false;
                }
                else //RAS
                {
                    rc=-1;
                    errorText << "update_xbbServerHandleTransferKeys(): exception updating transfer keys for jobid " << pJob.getJobId() << ", jobstepid " << pJob.getJobStepId() << ", handle " << pHandle;
                    LOG_ERROR_TEXT_ERRNO_AND_RAS(errorText, errno, bb.internal.updatexferkeys);
                    LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
                }

                break;
            }
        }
    }

    if (l_HandleFileName)
    {
        delete[] l_HandleFileName;
        l_HandleFileName = 0;
    }
    if (l_HandleFile)
    {
        // The lock on the handle file should have been released by the save code
        // above.  We ensure that it is unlocked here (e.g. the save code didn't run above)
        l_HandleFile->close();

        delete l_HandleFile;
        l_HandleFile = 0;
    }

    return rc;
}

/*
 * Non-static methods
 */

void HandleFile::close()
{
    close(lockfd);
}

void HandleFile::close(const int pFd)
{
    if (pFd >= 0)
    {
        if (pFd == lockfd)
        {
            unlock();
        }
        LOG(bb,debug) << "close(): Issue close for handle file fd " << pFd;
        ::close(pFd);
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
    unlock(lockfd);
    lockfd = -1;
}
