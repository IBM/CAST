/*******************************************************************************
 |    ContribFile.h
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

#ifndef BB_CONTRIBFILE_H_
#define BB_CONTRIBFILE_H_

#include <map>
#include <sstream>

#include <stdio.h>
#include <string.h>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/filesystem.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/serialization.hpp>

#include "bbinternal.h"
#include "ContribIdFile.h"

using namespace boost::archive;
namespace bfs = boost::filesystem;

/*******************************************************************************
 | Forward declarations
 *******************************************************************************/

/*******************************************************************************
 | Constants
 *******************************************************************************/
const uint32_t ARCHIVE_CONTRIB_VERSION = 1;
const int MAXIMUM_CONTRIBFILE_LOADTIME = 10;    // In seconds


/*******************************************************************************
 | Classes
 *******************************************************************************/

class ContribFile
{
public:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& pArchive, const uint32_t pVersion)
    {
        serializeVersion = pVersion;
        pArchive & objectVersion;
        pArchive & contribs;

        return;
    }

    ContribFile() :
        serializeVersion(0),
        objectVersion(ARCHIVE_CONTRIB_VERSION) {
        contribs = map<uint32_t,ContribIdFile>();
    }

    virtual ~ContribFile() {}

    /*
     * Static methods
     */
    static int loadContribFile(ContribFile* &pContribFile, const bfs::path& pContribFileName);

    /*
     * Inlined methods
     */
    inline void dump(const char* pPrefix)
    {
        stringstream l_Line;

        if (pPrefix) {
            l_Line << pPrefix << ": ";
        }
//        l_Line << "sVrsn=" << serializeVersion
//               << ", oVrsn=" << objectVersion
        l_Line << " Start: " << contribs.size() << " ContribFile(s) in ContribFile";
        LOG(bb,info) << l_Line.str();

        if (contribs.size())
        {
            l_Line << ", ";
            dumpContribs(l_Line, strlen(pPrefix)+2+10);
            l_Line << " >";
        }
        LOG(bb,info) << l_Line.str();

        l_Line << "   End: " << contribs.size() << " ContribFile(s) in ContribFile";
        LOG(bb,info) << l_Line.str();
        return;
    }

    inline void dumpContribs(stringstream& l_Line, size_t pPrefixLen)
    {
        stringstream l_SS_SkipChars, l_SS_SkipChars2;
        for (size_t i=0; i<pPrefixLen-1; i++) l_SS_SkipChars << " ";
        string l_SkipChars = l_SS_SkipChars.str();
        for (size_t i=0; i<pPrefixLen; i++) l_SS_SkipChars2 << " ";
        string l_SkipChars2 = l_SS_SkipChars2.str();

        bool l_FirstEntry = true;
        char l_Prefix[64] = {'\0'};
        for (map<uint32_t,ContribIdFile>::iterator ce = contribs.begin(); ce != contribs.end(); ce++)
        {
            if (!l_FirstEntry) {
                l_Line << ",";
            } else {
                l_Line << endl << l_SkipChars << "Map <";
                l_FirstEntry = false;
            }
            l_Line << endl << l_SkipChars2;
            snprintf(l_Prefix, sizeof(l_Prefix), "ContribId %d", ce->first);
            (ce->second).dump(l_Prefix);
        }

        return;
    }

    inline size_t numberOfContribs()
    {
        size_t l_Count = 0;

        for (map<uint32_t,ContribIdFile>::iterator ce = contribs.begin(); ce != contribs.end(); ce++)
        {
            if (!((ce->second).stopped()))
            {
                ++l_Count;
            }
        }

        return l_Count;
    }

    /*
     * Non-inlined methods
     */
    int save(const string& pContribFileName);

    /*
     * Data members
     */
    uint32_t serializeVersion;
    uint32_t objectVersion;
    map<uint32_t,ContribIdFile> contribs;
};

#endif /* BB_CONTRIBFILE_H_ */
