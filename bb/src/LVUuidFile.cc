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
