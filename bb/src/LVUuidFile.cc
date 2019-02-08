/*******************************************************************************
 |    LVUuidFile.cc
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

#include "LVUuidFile.h"


/*
 * Static methods
 */
int LVUuidFile::update_xbbServerLVUuidFile(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pFlags, const int pValue)
{
    int rc = 0;
    Uuid lv_uuid = pLVKey->second;
    char lv_uuid_str[LENGTH_UUID_STR] = {'\0'};
    lv_uuid.copyTo(lv_uuid_str);

    bfs::path job(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
    job /= bfs::path(to_string(pJobId));
    if(!bfs::is_directory(job)) return -2;

    for(auto& jobstep : boost::make_iterator_range(bfs::directory_iterator(job), {}))
    {
        if(!bfs::is_directory(jobstep)) continue;
        for(auto& handle : boost::make_iterator_range(bfs::directory_iterator(jobstep), {}))
        {
            if(!bfs::is_directory(handle)) continue;
            for(auto& lvuuid : boost::make_iterator_range(bfs::directory_iterator(handle), {}))
            {
                if(lvuuid.path().filename() == lv_uuid_str)
                {
                    bfs::path metafile = lvuuid.path();
                    metafile /= bfs::path(lv_uuid_str);

                    LVUuidFile l_LVUuidFile;
                    int rc2 = l_LVUuidFile.load(metafile.string());
                    if (!rc2)
                    {
                        uint64_t l_Flags = l_LVUuidFile.flags;
                        uint64_t l_NewFlags = 0;
                        SET_FLAG_VAR(l_NewFlags, l_Flags, pFlags, pValue);
                        if (l_Flags != l_NewFlags)
                        {
                            LOG(bb,info) << "xbbServer: For " << *pLVKey << ", handle " << handle.path().filename() << ":";
                            LOG(bb,info) << "           LVUuid flags changing from 0x" << hex << uppercase << setfill('0') << l_Flags << " to 0x" << l_NewFlags <<nouppercase << dec << ".";
                        }
                        l_LVUuidFile.flags = l_NewFlags;

                        rc = l_LVUuidFile.save(metafile.string());
                    }
                    else
                    {
                    	rc = -1;
                    }
                }
            }
        }
    }

    return rc;
}


/*
 * Non-static methods
 */
int LVUuidFile::load(const string& pLVUuidFileName)
{
    int rc;

    bool l_AllDone = false;
    bool l_FirstAttempt = true;
    struct timeval l_StartTime, l_StopTime;
    int l_ElapsedTime = 0;
    int l_LastConsoleOutput = -1;

    l_StartTime.tv_sec = 0; // resolve gcc optimizer complaint

    while ((!l_AllDone) && (l_ElapsedTime < MAXIMUM_LVUUIDFILE_LOADTIME))
    {
        rc = 0;
        l_AllDone = true;
        try
        {
            ifstream l_ArchiveFile{pLVUuidFileName};
            text_iarchive l_Archive{l_ArchiveFile};
            l_Archive >> *this;
        }
        catch(archive_exception& e)
        {
            // NOTE: If we take an 'archieve exception' we do not delay before attempting the next
            //       read of the archive file.  More than likely, we just had a concurrent update
            //       to the LVUuid file.
            rc = -1;
            l_AllDone = false;

            gettimeofday(&l_StopTime, NULL);
            if (l_FirstAttempt)
            {
                l_StartTime = l_StopTime;
            }
            l_ElapsedTime = int(l_StopTime.tv_sec - l_StartTime.tv_sec);

            if (l_ElapsedTime && (l_ElapsedTime % 3 == 0) && (l_ElapsedTime != l_LastConsoleOutput))
            {
                l_LastConsoleOutput = l_ElapsedTime;
                LOG(bb,warning) << "Archive exception thrown in " << __func__ << " was " << e.what() \
                                << " when attempting to load archive " << pLVUuidFileName << ". Elapsed time=" << l_ElapsedTime << " second(s). Retrying...";
            }
        }
        catch(exception& e)
        {
            rc = -1;
            LOG(bb,error) << "Exception thrown in " << __func__ << " was " << e.what() << " when attempting to load archive " << pLVUuidFileName;
        }
        l_FirstAttempt = false;
    }

    if (l_LastConsoleOutput > 0)
    {
       gettimeofday(&l_StopTime, NULL);
       if (!rc)
        {
            LOG(bb,warning) << "Loading " << pLVUuidFileName << " became successful after " << (l_StopTime.tv_sec - l_StartTime.tv_sec) << " second(s)" << " after recovering from archive exception(s)";
        }
        else
        {
            LOG(bb,error) << "Loading " << pLVUuidFileName << " failed after " << (l_StopTime.tv_sec - l_StartTime.tv_sec) << " second(s)" << " when attempting to recover from archive exception(s)";
        }
    }

    return rc;
}

int LVUuidFile::save(const string& pLVUuidFileName)
{
    int rc = 0;

    try
    {
        LOG(bb,debug) << "Writing:" << pLVUuidFileName;
        ofstream l_ArchiveFile{pLVUuidFileName};
        text_oarchive l_Archive{l_ArchiveFile};
        l_Archive << *this;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    return rc;
}
