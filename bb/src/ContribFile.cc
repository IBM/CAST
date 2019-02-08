/*******************************************************************************
 |    ContribFile.cc
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

#include "ContribFile.h"


/*
 * Static methods
 */

int ContribFile::loadContribFile(ContribFile* &pContribFile, const bfs::path& pContribFileName)
{
    int rc;

    pContribFile = NULL;
    ContribFile* l_ContribFile = new ContribFile();

    bool l_AllDone = false;
    bool l_FirstAttempt = true;
    struct timeval l_StartTime, l_StopTime;
    int l_ElapsedTime = 0;
    int l_LastConsoleOutput = -1;

    l_StartTime.tv_sec = 0; // resolve gcc optimizer complaint

    while ((!l_AllDone) && (l_ElapsedTime < MAXIMUM_CONTRIBFILE_LOADTIME))
    {
        rc = 0;
        l_AllDone = true;
        try
        {
            ifstream l_ArchiveFile{pContribFileName.c_str()};
            text_iarchive l_Archive{l_ArchiveFile};
            l_Archive >> *l_ContribFile;
            pContribFile = l_ContribFile;
        }
        catch(archive_exception& e)
        {
            // NOTE: If we take an 'archieve exception' we do not delay before attempting the next
            //       read of the archive file.  More than likely, we just had a concurrent update
            //       to the contrib file.
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
                                << " when attempting to load archive " << pContribFileName.c_str() << ". Elapsed time=" << l_ElapsedTime << " second(s). Retrying...";
            }
        }
        catch(exception& e)
        {
            rc = -1;
            LOG(bb,error) << "Exception thrown in " << __func__ << " was " << e.what() << " when attempting to load archive " << pContribFileName.c_str();
        }
        l_FirstAttempt = false;
    }

    if (l_LastConsoleOutput > 0)
    {
       gettimeofday(&l_StopTime, NULL);
       if (!rc)
        {
            LOG(bb,warning) << "Loading " << pContribFileName.c_str() << " became successful after " << (l_StopTime.tv_sec - l_StartTime.tv_sec) << " second(s)" << " after recovering from archive exception(s)";
        }
        else
        {
            LOG(bb,error) << "Loading " << pContribFileName.c_str() << " failed after " << (l_StopTime.tv_sec - l_StartTime.tv_sec) << " second(s)" << " when attempting to recover from archive exception(s)";
        }
    }

    if (rc)
    {
        if (l_ContribFile)
        {
            delete l_ContribFile;
            l_ContribFile = NULL;
        }
    }

    return rc;
}

/*
 * Non-static methods
 */

int ContribFile::save(const string& pContribFileName)
{
    int rc = 0;

    try
    {
        LOG(bb,debug) << "Writing:" << pContribFileName;
        ofstream l_ArchiveFile{pContribFileName};
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
