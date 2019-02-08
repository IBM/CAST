/*******************************************************************************
 |    LVUuidFile.h
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

#ifndef BB_LVUUIDFILE_H_
#define BB_LVUUIDFILE_H_

#include <sstream>

#include <stdio.h>
#include <string.h>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/filesystem.hpp>
#include <boost/serialization/serialization.hpp>

#include "bbflags.h"
#include "bbinternal.h"
#include "LVKey.h"

using namespace boost::archive;
namespace bfs = boost::filesystem;

/*******************************************************************************
 | Forward declarations
 *******************************************************************************/

/*******************************************************************************
 | Constants
 *******************************************************************************/
const uint32_t ARCHIVE_LVUUID_VERSION = 1;
const int MAXIMUM_LVUUIDFILE_LOADTIME = 30;

/*******************************************************************************
 | Classes
 *******************************************************************************/

class LVUuidFile
{
public:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& pArchive, const uint32_t pVersion)
    {
        serializeVersion = pVersion;
        pArchive & objectVersion;
        pArchive & flags;
        pArchive & connectionName;
        pArchive & hostname;

        return;
    }

    LVUuidFile() :
        serializeVersion(0),
        objectVersion(ARCHIVE_LVUUID_VERSION),
        flags(0),
        connectionName(""),
        hostname("") {}

    LVUuidFile(string pConnectionName, string pHostName) :
        serializeVersion(0),
        objectVersion(ARCHIVE_LVUUID_VERSION),
        flags(0),
        connectionName(pConnectionName),
        hostname(pHostName) {}

    virtual ~LVUuidFile() {}

    /*
     * Static methods
     */
//    static int loadLVUuidFile(LVUuidFile* &pLVUuidFile, const char* pLVUuidFileName);
//    static int loadLVUuidFile(LVUuidFile* &pLVUuidFile, const LVKey* pLVKey, uint64_t pJobId, uint64_t pJobStepId, uint64_t pHandle);
//    static int saveLVUuidFile(LVUuidFile* &pLVUuidFile, const LVKey* pLVKey, uint64_t pJobId, uint64_t pJobStepId, uint64_t pHandle);
    static int update_xbbServerLVUuidFile(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pFlags, const int pValue=1);

    /*
     * Inlined methods
     */
    inline void dump(const char* pPrefix)
    {
        stringstream l_Line;

        if (pPrefix)
        {
            l_Line << pPrefix << ": ";
        }
    //    l_Line << "sVrsn=" << serializeVersion
    //           << ", oVrsn=" << objectVersion
        l_Line << hex << uppercase << ", flags=0x" << flags << nouppercase << dec \
               << ", connection name " << connectionName \
               << ", hostname " << hostname;
        LOG(bb,debug) << l_Line.str();

        return;
    }

    /*
     * Non-static methods
     */
    int load(const string& pLVUuidFileName);
    int save(const string& pLVUuidFileName);

    /*
     * Data members
     */
    uint32_t serializeVersion;
    uint32_t objectVersion;
    uint64_t flags;
    string   connectionName;
    string   hostname;
};

#endif /* BB_LVUUIDFILE_H_ */
