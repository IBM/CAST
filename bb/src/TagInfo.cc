/*******************************************************************************
 |    TagInfo.cc
 |
 |   Copyright IBM Corporation 2015,2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/

#include "bbserver_flightlog.h"
#include "TagInfo.h"
#include "tracksyscall.h"

using namespace boost::archive;
namespace bfs = boost::filesystem;


/*******************************************************************************
 | External data
 *******************************************************************************/
thread_local int TagInfoLockFd = -1;


/*
 * Static methods
 */

int TagInfo::addTagHandle(const LVKey* pLVKey, const BBJob pJob, const uint64_t pTag, vector<uint32_t>& pExpectContrib, uint64_t& pHandle, const uint32_t pBumpCount)
{
    int rc = 0;
    stringstream errorText;

    TagInfo* l_TagInfo = 0;
    HandleInfo* l_HandleInfo = 0;
    int l_TransferQueueWasUnlocked = 0;
    int l_LocalMetadataLocked = 0;
    int l_TagInfoLocked = 0;
    int l_HandleBucketLocked = 0;

    try
    {
        bfs::path l_JobStepPath(g_BBServer_Metadata_Path);
        l_JobStepPath /= bfs::path(to_string(pJob.getJobId()));
        l_JobStepPath /= bfs::path(to_string(pJob.getJobStepId()));
        if(!bfs::is_directory(l_JobStepPath))
        {
            rc = -1;
            errorText << "BBTagInfo::addTagHandle(): Attempt to load taginfo file failed because the jobstep directory " << l_JobStepPath.string() << " could not be found";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        uint64_t l_BucketNumber = pHandle % g_Number_Handlefile_Buckets;

        char l_TagInfoName[64] = {'\0'};
        snprintf(l_TagInfoName, sizeof(l_TagInfoName), "%s%lu", TAGINFONAME, l_BucketNumber);
        bfs::path l_TagInfoPath = l_JobStepPath / l_TagInfoName;
        char l_HandleInfoName[64] = {'\0'};
        snprintf(l_HandleInfoName, sizeof(l_HandleInfoName), "%s%lu", HANDLEINFONAME, l_BucketNumber);
        bfs::path l_HandleInfoPath = l_JobStepPath / l_HandleInfoName;
        char l_HandleBucketName[64] = {'\0'};
        snprintf(l_HandleBucketName, sizeof(l_HandleBucketName), "%s%lu", TOPLEVEL_HANDLEFILE_NAME, l_BucketNumber);
        bfs::path l_HandleBucketPath = l_JobStepPath / l_HandleBucketName;
        BBTagHandle l_TagHandle = BBTagHandle(pTag, pHandle, pExpectContrib);

        if (!rc)
        {
            rc = HandleInfo::lockHandleBucket(l_HandleBucketPath, l_BucketNumber);
            if (!rc)
            {
                l_HandleBucketLocked = 1;

                l_TransferQueueWasUnlocked = unlockTransferQueueIfNeeded(pLVKey, "TagInfo::addTagHandle");
                // This lock serializes amongst request/transfer threads on this bbServer...
                l_LocalMetadataLocked = lockLocalMetadataIfNeeded(pLVKey, "TagInfo::addTagHandle");
                // Perform the necessary locking across bbServers to read the taginfo
                rc = TagInfo::lock(l_JobStepPath);
                if (!rc)
                {
                    l_TagInfoLocked = 1;
                    rc = TagInfo::load(l_TagInfo, l_TagInfoPath);
                    if (!rc)
                    {
                        l_TagInfoLocked = 0;
                        TagInfo::unlock();
                        if (l_LocalMetadataLocked)
                        {
                            l_LocalMetadataLocked = 0;
                            unlockLocalMetadata(pLVKey, "TagInfo::addTagHandle");
                        }
                        if (l_TransferQueueWasUnlocked)
                        {
                            l_TransferQueueWasUnlocked = 0;
                            lockTransferQueue(pLVKey, "TagInfo::addTagHandle");
                        }
                        for (size_t i=0; (i<l_TagInfo->tagHandles.size() && (!rc)); i++)
                        {
                            if (l_TagInfo->tagHandles[i].tag == pTag)
                            {
                                // Same tag value
                                if (!(l_TagInfo->tagHandles[i].compareContrib(pExpectContrib)))
                                {
                                    // Same contrib vector
                                    // Handle value has already been assigned for this tag/contrib vector.
                                    // Return the already assigned handle value for this tag/contrib vector.
                                    pHandle = l_TagInfo->tagHandles[i].handle;
                                    rc = 1;
                                }
                                else
                                {
                                    // ERROR - Tag value has already been used for a different contrib vector.
                                    stringstream l_Temp;
                                    contribToString(l_Temp, pExpectContrib);
                                    stringstream l_Temp2;
                                    contribToString(l_Temp2, (l_TagInfo->tagHandles[i]).expectContrib);
                                    LOG(bb,error) << "For jobid " << pJob.getJobId() << ", jobstepid " << pJob.getJobStepId() << ", handle " << pHandle << ", tag " << pTag \
                                                  << ", the expected contrib is " << l_Temp.str() << ". Existing contrib for handle and tag is " << l_Temp2.str() << ".";
                                    rc = -2;
                                }
                            }
                        }

                        if (!rc)
                        {
                            // The passed in tag was not found.  Now, determine if we have a duplicate
                            // handle value.
                            rc = HandleInfo::load(l_HandleInfo, l_HandleInfoPath);
                            if (!rc)
                            {
                                for (size_t i=0; (i<l_HandleInfo->handles.size() && (rc == 0 || rc == 2)); i++)
                                {
                                    if (l_HandleInfo->handles[i] == pHandle)
                                    {
                                        // Same handle value.  Indicate to re-generate the handle vlaue and retry.
                                        rc = 2;
                                    }
                                }

                                if (!rc)
                                {
                                    rc = TagInfo::update(pLVKey, l_JobStepPath, l_TagInfoPath, l_HandleInfoPath, pBumpCount, l_TagHandle, pHandle);
                                    switch (rc)
                                    {
                                        case 0:
                                        {
                                            // Update was successful
                                        }
                                        break;

                                        case 1:
                                        {
                                            // Bump count mismatch...
                                            rc = -3;
                                        }
                                        break;

                                        default:
                                        {
                                            // Could not update the TagInfo or HandleInfo file
                                            // NOTE: error state already filled in...
                                            SET_RC(rc);
                                        }
                                    }
                                }
                            }
                            else
                            {
                                // Could not load the handleinfo
                                // NOTE: error state already filled in...
                                SET_RC(rc);
                            }
                        }
                    }
                    else
                    {
                        // Could not load the taginfo
                        // NOTE: error state already filled in...
                        SET_RC(rc);
                    }
                }
                else
                {
                    // Could not lock the tag lockfile
                    // NOTE: error state already filled in...
                    SET_RC(rc);
                }
            }
            else
            {
                // Could not lock the handle bucket
                // NOTE: error state already filled in...
                SET_RC(rc);
            }
        }
        else
        {
            // Could not lock the taginfo
            // NOTE: error state already filled in...
            SET_RC(rc);
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    if (l_TagInfoLocked)
    {
        l_TagInfoLocked = 0;
        TagInfo::unlock();
    }

    if (l_LocalMetadataLocked)
    {
        l_LocalMetadataLocked = 0;
        unlockLocalMetadata(pLVKey, "TagInfo::addTagHandle on exit");
    }
    if (l_TransferQueueWasUnlocked)
    {
        l_TransferQueueWasUnlocked = 0;
        lockTransferQueue(pLVKey, "TagInfo::addTagHandle on exit");
    }

    if (l_HandleBucketLocked)
    {
        l_HandleBucketLocked = 0;
        HandleInfo::unlockHandleBucket(pHandle % g_Number_Handlefile_Buckets);
    }

    if (l_HandleInfo)
    {
        delete l_HandleInfo;
        l_HandleInfo = 0;
    }

    if (l_TagInfo)
    {
        delete l_TagInfo;
        l_TagInfo = 0;
    }

    return rc;
}

int TagInfo::createLockFile(const string& pFilePath)
{
    int rc = 0;

    char l_LockFileName[PATH_MAX+64] = {'\0'};
    snprintf(l_LockFileName, sizeof(l_LockFileName), "%s/%s", pFilePath.c_str(), LOCK_TAG_FILENAME);

    bfs::ofstream l_LockFile{l_LockFileName};

    writeBumpCountFile(pFilePath, 1);

    return rc;
}

int TagInfo::incrBumpCountFile(const string& pFilePath)
{
    int rc = 0;
    uint32_t l_BumpCount;

    rc = TagInfo::readBumpCountFile(pFilePath, l_BumpCount);
    if (!rc)
    {
        do {
            l_BumpCount += 1;
        } while (l_BumpCount == 0);
        rc = TagInfo::writeBumpCountFile(pFilePath, l_BumpCount);
    }

    if (rc)
    {
        LOG(bb,error) << "Failed to increment bump count file located at " << pFilePath;
    }

    return rc;
}

// NOTE: TagInfo lock MUST be held when invoking this method
int TagInfo::load(TagInfo* &pTagInfo, const bfs::path& pTagInfoName)
{
    int rc = 0;

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, TF_Load, "loadTagInfo, counter=%ld", l_FL_Counter, 0, 0, 0);

    pTagInfo = NULL;
    TagInfo* l_TagInfo = new TagInfo(pTagInfoName.string());

    if(bfs::exists(pTagInfoName))
    {
        struct timeval l_StartTime = timeval {.tv_sec=0, .tv_usec=0}, l_StopTime = timeval {.tv_sec=0, .tv_usec=0};
        bool l_AllDone = false;
        int l_Attempts = 0;
        int l_ElapsedTime = 0;
        int l_LastConsoleOutput = -1;

        while ((!l_AllDone) && (l_ElapsedTime < MAXIMUM_TAGINFO_LOADTIME))
        {
            rc = 0;
            l_AllDone = true;
            ++l_Attempts;
            try
            {
                LOG(bb,debug) << "Reading:" << pTagInfoName;
                ifstream l_ArchiveFile{pTagInfoName.c_str()};
                text_iarchive l_Archive{l_ArchiveFile};
                l_Archive >> *l_TagInfo;
            }
            catch(archive_exception& e)
            {
                // NOTE: If we take an 'archieve exception' we do not delay before attempting the next
                //       read of the archive file.
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
                                    << " when attempting to load archive " << pTagInfoName.c_str() << ". Elapsed time=" << l_ElapsedTime << " second(s). Retrying...";
                }
            }
            catch(exception& e)
            {
                rc = -1;
                LOG(bb,error) << "Exception thrown in " << __func__ << " was " << e.what() << " when attempting to load archive " << pTagInfoName.c_str();
            }
        }

        if (l_LastConsoleOutput > 0)
        {
           gettimeofday(&l_StopTime, NULL);
           if (!rc)
            {
                LOG(bb,warning) << "Loading " << pTagInfoName.c_str() << " became successful after " << (l_StopTime.tv_sec - l_StartTime.tv_sec) << " second(s)" \
                                << " after recovering from archive exception(s)";
            }
            else
            {
                LOG(bb,error) << "Loading " << pTagInfoName.c_str() << " failed after " << (l_StopTime.tv_sec - l_StartTime.tv_sec) << " second(s)" \
                              << " when attempting to recover from archive exception(s).  The most likely cause is due to the job being ended and/or removed.";
            }
        }

        FL_Write(FLMetaData, TF_Load_End, "loadTagInfo, counter=%ld, rc=%ld", l_FL_Counter, rc, 0, 0);
    }

    if (!rc)
    {
        pTagInfo = l_TagInfo;
    }
    else
    {
        if (l_TagInfo)
        {
            delete l_TagInfo;
            l_TagInfo = NULL;
        }
    }

    return rc;
}

#define ATTEMPTS 200
int TagInfo::lock(const bfs::path& pJobStepPath)
{
    int rc = -2;
    int fd = -1;
    stringstream errorText;

    std::string l_TagLockFileName = (pJobStepPath / LOCK_TAG_FILENAME).string();
    if (TagInfoLockFd == -1)
    {
        int l_Attempts = ATTEMPTS;
        while (l_Attempts--)
        {
            uint64_t l_FL_Counter = metadataCounter.getNext();
            FL_Write(FLMetaData, TI_Lock, "lock TI, counter=%ld", l_FL_Counter, 0, 0, 0);

            uint64_t l_FL_Counter2 = metadataCounter.getNext();
            FL_Write(FLMetaData, TI_Open, "open TI, counter=%ld", l_FL_Counter2, 0, 0, 0);

            threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::opensyscall, l_TagLockFileName, __LINE__);
            fd = open(l_TagLockFileName.c_str(), O_WRONLY);
            threadLocalTrackSyscallPtr->clearTrack();

            FL_Write(FLMetaData, TI_Open_End, "open TI, counter=%ld, fd=%ld", l_FL_Counter2, fd, 0, 0);

            if (fd >= 0)
            {
                // Exclusive lock and this will block if needed
                LOG(bb,debug) << "lock(): Open issued for tag lockfile " << l_TagLockFileName << ", fd=" << fd;
                struct flock l_LockOptions;
                l_LockOptions.l_whence = SEEK_SET;
                l_LockOptions.l_start = 0;
                l_LockOptions.l_len = 0;    // Lock entire file for writing
                l_LockOptions.l_type = F_WRLCK;
                threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::fcntlsyscall, l_TagLockFileName, __LINE__);
                rc = ::fcntl(fd, F_SETLKW, &l_LockOptions);
                threadLocalTrackSyscallPtr->clearTrack();
            }

            switch(rc)
            {
                case 0:
                {
                    // Successful lock...
                    TagInfoLockFd = fd;
                    l_Attempts = 0;
                }
                break;

                case -2:
                {
                    if (l_Attempts)
                    {
                        // Delay fifty milliseconds and try again
                        // NOTE: We may have hit the window between the creation of the jobstep directory
                        //       and creating the lockfile/taginfo in that jobstep directory.
                        usleep((useconds_t)50000);
                    }
                    else
                    {
                        rc = -1;
                        errorText << "Could not open tag lockfile " << l_TagLockFileName << " for locking, errno=" << errno << ": " << strerror(errno);
                        LOG_ERROR_TEXT_ERRNO(errorText, errno);
                    }
                }
                break;

                case -1:
                default:
                {
                    errorText << "Could not exclusively lock tag lockfile " << l_TagLockFileName << ", errno=" << errno << ":" << strerror(errno);
                    LOG_ERROR_TEXT_ERRNO(errorText, errno);

                    if (fd >= 0)
                    {
                        LOG(bb,debug) << "lock(): Issue close for tag lockfile fd " << fd;
                        uint64_t l_FL_Counter = metadataCounter.getNext();
                        FL_Write(FLMetaData, TI_CouldNotLockExcl, "open TI, could not lock exclusive, performing close, counter=%ld", l_FL_Counter, 0, 0, 0);
                        ::close(fd);
                        FL_Write(FLMetaData, TI_CouldNotLockExcl_End, "open TI, could not lock exclusive, performing close, counter=%ld, fd=%ld", l_FL_Counter, fd, 0, 0);
                    }
                    l_Attempts = 0;
                    rc = -1;
                }
                break;
            }

            FL_Write(FLMetaData, TI_Lock_End, "lock TI, counter=%ld, fd=%ld, rc=%ld, errno=%ld", l_FL_Counter, fd, rc, errno);
        }
    }
    else
    {
        // NOTE:  Should never be the case...
        rc = -1;
        errorText << "Tag lockfile " << l_TagLockFileName << " is currently locked";
        LOG_ERROR_TEXT_RC(errorText, rc);
    }

    return rc;
}

// NOTE: TagInfo lock MUST be held when invoking this method
int TagInfo::readBumpCountFile(const string& pFilePath, uint32_t& pBumpCount)
{
    int rc = 0;
    stringstream errorText;

    pBumpCount = 0;
    string l_Line;

    char l_BumpCountFileName[PATH_MAX+64] = {'\0'};
    snprintf(l_BumpCountFileName, sizeof(l_BumpCountFileName), "%s/%s", pFilePath.c_str(), BUMP_COUNT_FILENAME);

    try
    {
        int l_Attempts = ATTEMPTS;
        while (l_Attempts-- && (!rc))
        {
            ifstream l_BumpCountFile(l_BumpCountFileName);
            if (l_BumpCountFile.is_open())
            {
                getline(l_BumpCountFile, l_Line);
                if (l_Line.length() > 0 && all_of(l_Line.begin(), l_Line.end(), ::isdigit))
                {
                    pBumpCount = stoi(l_Line);
                    l_Attempts = 0;
                }
                else
                {
                    if (l_Attempts)
                    {
                        // Delay fifty milliseconds and try again
                        // NOTE: We may have hit the window between the creation of the jobstep directory
                        //       and creating/populating of the bump count file in that jobstep directory.
                        usleep((useconds_t)50000);
                    }
                    else
                    {
                        rc = -1;
                        LOG(bb,error) << "Bump count file at " << l_BumpCountFileName << " has invalid contents of |" << l_Line.c_str() << "|";
                    }
                }
                l_BumpCountFile.close();
            }
            else
            {
                if (l_Attempts)
                {
                    // Delay fifty milliseconds and try again
                    // NOTE: We may have hit the window between the creation of the jobstep directory
                    //       and creating/populating of the bump count file in that jobstep directory.
                    usleep((useconds_t)50000);
                }
                else
                {
                    rc = -1;
                    errorText << "Failed to open bump count file " << l_BumpCountFileName << ", errno=" << errno << ": " << strerror(errno);
                    LOG_ERROR_TEXT_ERRNO(errorText, errno);
                }
            }
        }
    }
    catch(exception& e)
    {
        rc = -1;
        LOG(bb,error) << "Exception caught " << __func__ << "@" << __FILE__ << ":" << __LINE__ << " what=" << e.what();
    }

    return rc;
};
#undef ATTEMPTS

void TagInfo::unlock()
{
    if (TagInfoLockFd != -1)
    {
        unlock(TagInfoLockFd);
        TagInfoLockFd = -1;
    }

    return;
}

void TagInfo::unlock(const int pFd)
{
    stringstream errorText;

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, TI_Unlock, "unlock TI, counter=%ld, fd=%ld", l_FL_Counter, pFd, 0, 0);

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
            LOG(bb,debug) << "unlock(): Issue unlock for tag lockfile fd " << pFd;
            threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::fcntlsyscall, pFd, __LINE__);
            int rc = ::fcntl(pFd, F_SETLK, &l_LockOptions);
            threadLocalTrackSyscallPtr->clearTrack();
            if (!rc)
            {
                // Successful unlock...
            }
            else
            {
                LOG(bb,warning) << "Could not exclusively unlock tag lockfile fd " << pFd << ", errno=" << errno << ":" << strerror(errno);
            }
            LOG(bb,debug) << "close(): Issue close for tag lockfile fd " << pFd;

            ::close(pFd);

            FL_Write(FLMetaData, TI_Close_End, "close TI, counter=%ld, fd=%ld", l_FL_Counter, pFd, 0, 0);
        }
        catch(exception& e)
        {
            LOG(bb,info) << "Exception caught " << __func__ << "@" << __FILE__ << ":" << __LINE__ << " what=" << e.what();
        }
    }

    FL_Write(FLMetaData, TI_Unlock_End, "unlock TI, counter=%ld, fd=%ld", l_FL_Counter, pFd, 0, 0);

    return;
}

int TagInfo::update(const LVKey* pLVKey, const bfs::path& pJobStepPath, const bfs::path& pTagInfoPath, const bfs::path& pHandleInfoPath, const uint32_t pBumpCount, BBTagHandle& pTagHandle, uint64_t pHandle)
{
    int rc = 0;

    TagInfo* l_TagInfo = 0;
    HandleInfo* l_HandleInfo = 0;
    int l_TransferQueueWasUnlocked = 0;
    int l_LocalMetadataLocked = 0;
    int l_TagInfoLocked = 0;

    try
    {
        l_TransferQueueWasUnlocked = unlockTransferQueueIfNeeded(pLVKey, "TagInfo::update");
        // This lock serializes amongst request/transfer threads on this bbServer...
        l_LocalMetadataLocked = lockLocalMetadataIfNeeded(pLVKey, "TagInfo::update");

        // This lock serializes amongst bbServers...
        rc = TagInfo::lock(pJobStepPath);
        if (!rc)
        {
            l_TagInfoLocked = 1;

            uint32_t l_BumpCount = 0;
            rc = readBumpCountFile(pJobStepPath.string(), l_BumpCount);

            if (l_BumpCount == pBumpCount)
            {
                // Add the passed in values for tag/contrib vector/handle as a new TagHandle
                // in the taginfo file
                rc = TagInfo::load(l_TagInfo, pTagInfoPath);
                if (!rc)
                {
                    rc = HandleInfo::load(l_HandleInfo, pHandleInfoPath);
                    if (!rc)
                    {
                        // Add the taghandle value to the taginfo file
                        l_TagInfo->tagHandles.push_back(pTagHandle);
                        l_TagInfo->save();

                        // Add the handle value to the handleinfo file
                        l_HandleInfo->handles.push_back(pHandle);
                        l_HandleInfo->save();
                    }
                    else
                    {
                        rc = -1;
                        LOG(bb,error) << "Could not load handleinfo file " << pHandleInfoPath.c_str();
                    }
                }
                else
                {
                    rc = -1;
                    LOG(bb,error) << "Could not load taginfo file " << pTagInfoPath.c_str();
                }
            }
            else
            {
                // Bump count mismatch...  Indicate to invoker to retry...
                rc = 1;
            }
        }
        else
        {
            rc = -1;
            LOG(bb,error) << "Could not lock the taginfo lockfile located at " << pJobStepPath.c_str();
        }
    }
    catch(exception& e)
    {
        rc = -1;
        LOG(bb,error) << "Exception caught " << __func__ << "@" << __FILE__ << ":" << __LINE__ << " what=" << e.what();
    }

    if (l_TagInfoLocked)
    {
        // Unlock the TagInfo file
        l_TagInfoLocked = 0;
        TagInfo::unlock();
    }

    if (l_LocalMetadataLocked)
    {
        l_LocalMetadataLocked = 0;
        unlockLocalMetadata(pLVKey, "TagInfo::update");
    }
    if (l_TransferQueueWasUnlocked)
    {
        l_TransferQueueWasUnlocked = 0;
        lockTransferQueue(pLVKey, "TagInfo::update");
    }

    if (l_HandleInfo)
    {
        delete l_HandleInfo;
        l_HandleInfo = 0;
    }

    if (l_TagInfo)
    {
        delete l_TagInfo;
        l_TagInfo = 0;
    }

    return rc;
}


/*
 * Non-static methods
 */
int BBTagHandle::compareContrib(vector<uint32_t>& pContribVector)
{
    int rc = 0;

    size_t l_NumExpectContrib = pContribVector.size();
    if (l_NumExpectContrib == expectContrib.size())
    {
        for (size_t i=0; i<l_NumExpectContrib; ++i)
        {
            if (pContribVector[i] != expectContrib[i])
            {
                rc = 1;
                break;
            }
        }
    }
    else
    {
        rc = 1;
    }

    return rc;
}

// NOTE: TagInfo lock MUST be held when invoking this method
int TagInfo::save()
{
    int rc = 0;

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, TF_Save, "saveTagInfo, counter=%ld", l_FL_Counter, 0, 0, 0);

    try
    {
        LOG(bb,debug) << "Writing:" << filename;
        ofstream l_ArchiveFile{filename};
        text_oarchive l_Archive{l_ArchiveFile};
        l_Archive << *this;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    FL_Write(FLMetaData, TF_Save_End, "saveTagInfo, counter=%ld, rc=%ld", l_FL_Counter, rc, 0, 0);

    return rc;
}

int TagInfo::writeBumpCountFile(const string& pFilePath, const uint32_t pValue)
{
    int rc = 0;

    char l_BumpCountFileName[PATH_MAX+64] = {'\0'};
    snprintf(l_BumpCountFileName, sizeof(l_BumpCountFileName), "%s/%s", pFilePath.c_str(), BUMP_COUNT_FILENAME);

    try
    {
        ofstream l_BumpCountFile(l_BumpCountFileName);
        if(l_BumpCountFile.is_open())
        {
            l_BumpCountFile << pValue << std::endl;
            l_BumpCountFile.flush();
            l_BumpCountFile.close();
        }
        else
        {
            rc = -1;
            LOG(bb,error) << "Failed to open bump count file " << l_BumpCountFileName << ", errno=" << errno << ": " << strerror(errno);
        }
    }
    catch(exception& e)
    {
        rc = -1;
        LOG(bb,error) << "Exception caught " << __func__ << "@" << __FILE__ << ":" << __LINE__ << " what=" << e.what();
    }

    return rc;
}