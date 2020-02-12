/*******************************************************************************
 |    HandleInfo.cc
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
#include "HandleInfo.h"
#include "TagInfo.h"
#include "tracksyscall.h"

using namespace boost::archive;
namespace bfs = boost::filesystem;

/*******************************************************************************
 | External data
 *******************************************************************************/
thread_local int HandleBucketLockFd = -1;


/*
 * Static data
 */


/*
 * Static methods
 */
int HandleInfo::createLockFile(const string& pFilePath)
{
    int rc = 0;

    char l_LockFileName[PATH_MAX+64] = {'\0'};
    snprintf(l_LockFileName, sizeof(l_LockFileName), "%s/%s", pFilePath.c_str(), LOCK_HANDLE_BUCKET_FILENAME);

    bfs::ofstream l_LockFile{l_LockFileName};

    return rc;
}

int HandleInfo::load(HandleInfo* &pHandleInfo, const bfs::path& pHandleInfoName)
{
    int rc = 0;

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, HI_Load, "loadHandleInfo, counter=%ld", l_FL_Counter, 0, 0, 0);

    pHandleInfo = NULL;
    HandleInfo* l_HandleInfo = new HandleInfo(pHandleInfoName.string());

    if(bfs::exists(pHandleInfoName))
    {
        struct timeval l_StartTime = timeval {.tv_sec=0, .tv_usec=0}, l_StopTime = timeval {.tv_sec=0, .tv_usec=0};
        bool l_AllDone = false;
        int l_Attempts = 0;
        int l_ElapsedTime = 0;
        int l_LastConsoleOutput = -1;

        while ((!l_AllDone) && (l_ElapsedTime < MAXIMUM_HANDLEINFO_LOADTIME))
        {
            rc = 0;
            l_AllDone = true;
            ++l_Attempts;
            try
            {
                LOG(bb,debug) << "Reading:" << pHandleInfoName;
                ifstream l_ArchiveFile{pHandleInfoName.c_str()};
                text_iarchive l_Archive{l_ArchiveFile};
                l_Archive >> *l_HandleInfo;
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
                                    << " when attempting to load archive " << pHandleInfoName.c_str() << ". Elapsed time=" << l_ElapsedTime << " second(s). Retrying...";
                }
            }
            catch(exception& e)
            {
                rc = -1;
                LOG(bb,error) << "Exception thrown in " << __func__ << " was " << e.what() << " when attempting to load archive " << pHandleInfoName.c_str();
            }
        }

        if (l_LastConsoleOutput > 0)
        {
           gettimeofday(&l_StopTime, NULL);
           if (!rc)
            {
                LOG(bb,warning) << "Loading " << pHandleInfoName.c_str() << " became successful after " << (l_StopTime.tv_sec - l_StartTime.tv_sec) << " second(s)" \
                                << " after recovering from archive exception(s)";
            }
            else
            {
                LOG(bb,error) << "Loading " << pHandleInfoName.c_str() << " failed after " << (l_StopTime.tv_sec - l_StartTime.tv_sec) << " second(s)" \
                              << " when attempting to recover from archive exception(s).  The most likely cause is due to the job being ended and/or removed.";
            }
        }

        FL_Write(FLMetaData, HI_Load_End, "loadHandleInfo, counter=%ld, rc=%ld", l_FL_Counter, rc, 0, 0);
    }

    if (!rc)
    {
        pHandleInfo = l_HandleInfo;
    }
    else
    {
        if (l_HandleInfo)
        {
            delete l_HandleInfo;
            l_HandleInfo = NULL;
        }
    }

    return rc;
}

#define ATTEMPTS 200
int HandleInfo::lockHandleBucket(const bfs::path& pHandleBucketPath, const uint64_t pHandleBucketNumber)
{
    int rc = -2;
    int rc2 = 0;
    int fd = -1;
    stringstream errorText;

    if (!bfs::exists(pHandleBucketPath))
    {
        // Create the handle bucket directory
        bfs::create_directories(pHandleBucketPath);

        // Unconditionally perform a chmod to 0770 for the handle bucket directory.
        // This is required so that only root, the uid, and any user belonging to the gid can access this 'job'
        rc2 = chmod(pHandleBucketPath.c_str(), 0770);
        if (!rc2)
        {
            rc2 = createLockFile(pHandleBucketPath.string());
            if (rc2)
            {
                errorText << "Creation of the lockfile failed for the handle bucket directory " << pHandleBucketPath.string();
                bberror << err("error.path", pHandleBucketPath.c_str());
                LOG_ERROR_TEXT(errorText);
            }
        }
        else
        {
            errorText << "chmod failed for the handle bucket directory " << pHandleBucketPath.string();
            bberror << err("error.path", pHandleBucketPath.c_str());
            LOG_ERROR_TEXT_ERRNO(errorText, errno);
        }
    }

    if (!rc2)
    {
        string l_HandleBucketLockFileName = pHandleBucketPath.string() + "/" + LOCK_HANDLE_BUCKET_FILENAME;
        if (HandleBucketLockFd == -1)
        {
            int l_Attempts = ATTEMPTS;
            while (l_Attempts--)
            {
                uint64_t l_FL_Counter = metadataCounter.getNext();
                FL_Write(FLMetaData, HB_Lock, "lock HB, counter=%ld", l_FL_Counter, 0, 0, 0);

                uint64_t l_FL_Counter2 = metadataCounter.getNext();
                FL_Write(FLMetaData, HB_Open, "open HB, counter=%ld", l_FL_Counter2, 0, 0, 0);

                threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::opensyscall, l_HandleBucketLockFileName, __LINE__);
                fd = open(l_HandleBucketLockFileName.c_str(), O_WRONLY);
                threadLocalTrackSyscallPtr->clearTrack();

                FL_Write(FLMetaData, HB_Open_End, "open HB, counter=%ld, fd=%ld", l_FL_Counter2, fd, 0, 0);

                if (fd >= 0)
                {
                    // Exclusive lock and this will block if needed
                    LOG(bb,debug) << "lock(): Open issued for handle bucket lockfile " << l_HandleBucketLockFileName << ", fd=" << fd;
                    struct flock l_LockOptions;
                    l_LockOptions.l_whence = SEEK_SET;
                    l_LockOptions.l_start = 0;
                    l_LockOptions.l_len = 0;    // Lock entire file for writing
                    l_LockOptions.l_type = F_WRLCK;
                    threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::fcntlsyscall, l_HandleBucketLockFileName, __LINE__);
                    rc = ::fcntl(fd, F_SETLKW, &l_LockOptions);
                    threadLocalTrackSyscallPtr->clearTrack();
                }

                switch(rc)
                {
                    case -2:
                    {
                        if (l_Attempts)
                        {
                            // Delay fifty milliseconds and try again
                            // NOTE: We may have hit the window between the creation of the jobstep/handle_bucket directory
                            //       and creating the lockfile in that jobstep/handle_bucket directory.
                            usleep((useconds_t)50000);
                        }
                        else
                        {
                            rc = -1;
                            errorText << "Could not open handle bucket lockfile " << l_HandleBucketLockFileName << " for locking, errno=" << errno << ": " << strerror(errno);
                            LOG_ERROR_TEXT_ERRNO(errorText, errno);
                        }
                    }
                    break;

                    case -1:
                    {
                        errorText << "Could not exclusively lock handle bucket lockfile " << l_HandleBucketLockFileName << ", errno=" << errno << ":" << strerror(errno);
                        LOG_ERROR_TEXT_ERRNO(errorText, errno);

                        if (fd >= 0)
                        {
                            LOG(bb,debug) << "lock(): Issue close for handle bucket fd " << fd;
                            uint64_t l_FL_Counter = metadataCounter.getNext();
                            FL_Write(FLMetaData, HB_CouldNotLockExcl, "open HB, could not lock exclusive, performing close, counter=%ld", l_FL_Counter, 0, 0, 0);
                            ::close(fd);
                            FL_Write(FLMetaData, HB_CouldNotLockExcl_End, "open HB, could not lock exclusive, performing close, counter=%ld, fd=%ld", l_FL_Counter, fd, 0, 0);
                        }
                        l_Attempts = 0;
                    }
                    break;

                    default:
                    {
                        // Successful lock...
                        HandleBucketLockFd = fd;
                        pthread_mutex_lock(&HandleBucketMutex[pHandleBucketNumber]);
                        l_Attempts = 0;
                    }
                    break;
                }

                FL_Write(FLMetaData, HB_Lock_End, "lock HB, counter=%ld, fd=%ld, rc=%ld, errno=%ld", l_FL_Counter, fd, rc, errno);
            }
        }
        else
        {
            // NOTE:  Should never be the case...
            rc = -1;
            errorText << "Handle bucket lockfile " << HandleBucketLockFd << " is currently locked";
            LOG_ERROR_TEXT_RC(errorText, rc);
        }
    }
    else
    {
        rc = -1;
        SET_RC(rc);
    }

    return rc;
}
#undef ATTEMPTS

void HandleInfo::unlockHandleBucket(const uint64_t pHandleBucketNumber)
{
    if (HandleBucketLockFd != -1)
    {
        pthread_mutex_unlock(&HandleBucketMutex[pHandleBucketNumber]);
        unlockHandleBucket(HandleBucketLockFd);
        HandleBucketLockFd = -1;
    }

    return;
}

void HandleInfo::unlockHandleBucket(const int pFd)
{
    stringstream errorText;

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, HB_Unlock, "unlock HB, counter=%ld, fd=%ld", l_FL_Counter, pFd, 0, 0);

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
                LOG(bb,warning) << "Could not exclusively unlock handle bucket fd " << pFd << ", errno=" << errno << ":" << strerror(errno);
            }
            LOG(bb,debug) << "close(): Issue close for handle bucket fd " << pFd;

            ::close(pFd);

            FL_Write(FLMetaData, HB_Close_End, "close HB, counter=%ld, fd=%ld", l_FL_Counter, pFd, 0, 0);
        }
        catch(exception& e)
        {
            LOG(bb,info) << "Exception caught " << __func__ << "@" << __FILE__ << ":" << __LINE__ << " what=" << e.what();
        }
    }

    FL_Write(FLMetaData, HB_Unlock_End, "unlock HB, counter=%ld, fd=%ld", l_FL_Counter, pFd, 0, 0);

    return;
}


/*
 * Non-static methods
 */
int HandleInfo::save()
{
    int rc = 0;

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, HI_Save, "saveHandleInfo, counter=%ld", l_FL_Counter, 0, 0, 0);

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

    FL_Write(FLMetaData, HI_Save_End, "saveHandleInfo, counter=%ld, rc=%ld", l_FL_Counter, rc, 0, 0);

    return rc;
}
