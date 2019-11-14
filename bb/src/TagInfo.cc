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

int TagInfo::addTagHandle(const LVKey* pLVKey, const BBJob pJob, const uint64_t pTag, vector<uint32_t> pExpectContrib, uint64_t& pHandle)
{
    int rc = 0;
    stringstream errorText;

    TagInfo* l_TagInfo = 0;
    HandleInfo* l_HandleInfo = 0;
    int l_TransferQueueWasUnlocked = 0;
    int l_LocalMetadataLocked = 0;
    int l_TagInfoLocked = 0;

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

        l_TransferQueueWasUnlocked = unlockTransferQueueIfNeeded(pLVKey, "addTagHandle");
        // This lock serializes amongst request/transfer threads on this bbServer...
        l_LocalMetadataLocked = lockLocalMetadataIfNeeded(pLVKey, "addTagHandle");
        // This lock serializes amongst bbServers...
        rc = TagInfo::lock(l_JobStepPath);

        if (!rc)
        {
            l_TagInfoLocked = 1;
            char l_TagInfoName[64] = {'\0'};
            snprintf(l_TagInfoName, sizeof(l_TagInfoName), "%s%lu", TAGINFONAME, (pTag % NUMBER_OF_TAGINFO_BUCKETS));
            bfs::path l_TagInfoPath = l_JobStepPath / l_TagInfoName;
            rc = TagInfo::load(l_TagInfo, l_TagInfoPath);
            if (!rc)
            {
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
                            // Different contrib vector
                            // ERROR - Tag value has already been used for a different contrib vector.
                            rc = -2;
                        }
                    }
                }

                if (!rc)
                {
                    // The passed in tag was not found.  Now, determine if we have a duplicate
                    // handle value.
                    char l_HandleInfoName[64] = {'\0'};
                    snprintf(l_HandleInfoName, sizeof(l_HandleInfoName), "%s%lu", HANDLEINFONAME, (pHandle % NUMBER_OF_HANDLEINFO_BUCKETS));
                    bfs::path l_HandleInfoPath = l_JobStepPath / l_HandleInfoName;
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
                            // Add the passed in values for tag/contrib vector/handle as a new TagHandle
                            // in the taginfo file
                            BBTagHandle l_TagHandle = BBTagHandle(pTag, pHandle, pExpectContrib);
                            l_TagInfo->tagHandles.push_back(l_TagHandle);
                            l_TagInfo->save();
                            // Add the passed in handle value to the handleinfo file
                            l_HandleInfo->handles.push_back(pHandle);
                            l_HandleInfo->save();
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
        unlockLocalMetadata(pLVKey, "addTagHandle");
    }

    if (l_TransferQueueWasUnlocked)
    {
        lockTransferQueue(pLVKey, "addTagHandle");
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

int TagInfo::createLockFile(const string pFilePath)
{
    int rc = 0;

    char l_LockFileName[PATH_MAX+64] = {'\0'};
    snprintf(l_LockFileName, sizeof(l_LockFileName), "%s/%s", pFilePath.c_str(), LOCK_TAG_FILENAME);

    bfs::ofstream l_LockFile{l_LockFileName};

    return rc;
}

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

#define ATTEMPTS 10
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
            FL_Write(FLMetaData, TF_Lock, "lock TF, counter=%ld", l_FL_Counter, 0, 0, 0);

            uint64_t l_FL_Counter2 = metadataCounter.getNext();
            FL_Write(FLMetaData, TF_Open, "open TF, counter=%ld", l_FL_Counter2, 0, 0, 0);

            threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::opensyscall, l_TagLockFileName, __LINE__);
            fd = open(l_TagLockFileName.c_str(), O_WRONLY);
            threadLocalTrackSyscallPtr->clearTrack();

            FL_Write(FLMetaData, TF_Open_End, "open TF, counter=%ld, fd=%ld", l_FL_Counter2, fd, 0, 0);

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
                case -2:
                {
                    if (!l_Attempts)
                    {
                        rc = -1;
                        errorText << "Could not open tag lockfile " << l_TagLockFileName << " for locking, errno=" << errno << ": " << strerror(errno) \
                                  << ". The most likely cause is due to the job being ended and/or removed.";
                        LOG_ERROR_TEXT_ERRNO(errorText, errno);
                    }
                    else
                    {
                        // Delay one second and try again
                        // NOTE: We may have hit the window between the creation of the jobstep directory
                        //       and creating the lockfile/taginfo in that jobstep directory.
                        usleep((useconds_t)1000000);
                    }
                }
                break;

                case -1:
                {
                    errorText << "Could not exclusively lock tag lockfile " << l_TagLockFileName << ", errno=" << errno << ":" << strerror(errno);
                    LOG_ERROR_TEXT_ERRNO(errorText, errno);

                    if (fd >= 0)
                    {
                        LOG(bb,debug) << "lock(): Issue close for taginfo fd " << fd;
                        uint64_t l_FL_Counter = metadataCounter.getNext();
                        FL_Write(FLMetaData, TF_CouldNotLockExcl, "open TF, could not lock exclusive, performing close, counter=%ld", l_FL_Counter, 0, 0, 0);
                        ::close(fd);
                        FL_Write(FLMetaData, TF_CouldNotLockExcl_End, "open TF, could not lock exclusive, performing close, counter=%ld, fd=%ld", l_FL_Counter, fd, 0, 0);
                    }
                    l_Attempts = 0;
                }
                break;

                default:
                {
                    // Successful lock...
                    TagInfoLockFd = fd;
                    l_Attempts = 0;
                }
                break;
            }

            FL_Write(FLMetaData, TF_Lock_End, "lock TF, counter=%ld, fd=%ld, rc=%ld, errno=%ld", l_FL_Counter, fd, rc, errno);
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
    FL_Write(FLMetaData, TF_Unlock, "unlock TF, counter=%ld, fd=%ld", l_FL_Counter, pFd, 0, 0);

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
            LOG(bb,debug) << "unlock(): Issue unlock for taginfo fd " << pFd;
            threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::fcntlsyscall, pFd, __LINE__);
            int rc = ::fcntl(pFd, F_SETLK, &l_LockOptions);
            threadLocalTrackSyscallPtr->clearTrack();
            if (!rc)
            {
                // Successful unlock...
            }
            else
            {
                LOG(bb,warning) << "Could not exclusively unlock taginfo fd " << pFd << ", errno=" << errno << ":" << strerror(errno);
            }
            LOG(bb,debug) << "close(): Issue close for taginfo fd " << pFd;

            ::close(pFd);

            FL_Write(FLMetaData, TF_Close_End, "close TF, counter=%ld, fd=%ld", l_FL_Counter, pFd, 0, 0);
        }
        catch(exception& e)
        {
            LOG(bb,info) << "Exception caught " << __func__ << "@" << __FILE__ << ":" << __LINE__ << " what=" << e.what();
        }
    }

    FL_Write(FLMetaData, TF_Unlock_End, "unlock TF, counter=%ld, fd=%ld", l_FL_Counter, pFd, 0, 0);

    return;
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
