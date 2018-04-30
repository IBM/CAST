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

int ContribFile::loadContribFile(ContribFile* &ptr, const bfs::path& filename)
{
    ContribFile* tmparchive = NULL;
    ptr = NULL;

    int rc = -1;
    bool doover;
    bool didretry = false;
    struct timeval start, stop;
    int l_LastConsoleOutput = -1;

    LOG(bb,debug) << __func__ << "  ArchiveName=" << filename;

    do
    {
        doover = false;
        try
        {
            ifstream l_ArchiveFile(filename.c_str());
            text_iarchive l_Archive(l_ArchiveFile);
            if (!tmparchive)
            {
                tmparchive = new ContribFile();
            }
            l_Archive >> *tmparchive;
            ptr = tmparchive;
            rc = 0;
//          ptr->dump("xbbServer: Loaded contrib file contents");
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
            if(didretry == false)
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

        if(doover)
        {
            usleep(250000);
        }
    }
    while(rc && doover);

    if (!rc)
    {
        if (didretry)
        {
            gettimeofday(&stop, NULL);
            LOG(bb,info) << __func__ << " became successful after recovering from exception after " << (stop.tv_sec-start.tv_sec) << " second(s)";
//            ptr->dump("xbbServer: Loaded contrib file contents after recovering from exception");
        }
    }
    else
    {
//        ptr->dump("xbbServer: Loaded contrib file contents with no exception");
    }

    if (rc && tmparchive)
    {
        delete tmparchive;
        tmparchive = NULL;
    }

    return rc;
}
