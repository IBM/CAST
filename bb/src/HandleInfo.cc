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


/*
 * Static methods
 */
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
