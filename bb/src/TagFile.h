/*******************************************************************************
 |    TagFile.h
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

#ifndef BB_TAGFILE_H_
#define BB_TAGFILE_H_

#include <sstream>
#include <vector>

#include <stdio.h>
#include <string.h>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/filesystem.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>

#include "bbinternal.h"
#include "BBJob.h"
#include "LVKey.h"


namespace bfs = boost::filesystem;


/*******************************************************************************
 | Forward declarations
 *******************************************************************************/


/*******************************************************************************
 | Constants
 *******************************************************************************/
const uint32_t ARCHIVE_TAG_VERSION_1 = 1;

const char LOCK_TAG_FILENAME[] = "lockfile";
const char TAGFILENAME[] = "tagfile";
const int MAXIMUM_TAGFILE_LOADTIME = 10;     // In seconds


/*******************************************************************************
 | External methods
 *******************************************************************************/
extern void lockLocalMetadata(const LVKey* pLVKey, const char* pMethod);
extern int unlockLocalMetadataIfNeeded(const LVKey* pLVKey, const char* pMethod);


/*******************************************************************************
 | External data
 *******************************************************************************/
extern thread_local int tagFileLockFd;


/*******************************************************************************
 | Classes
 *******************************************************************************/

class BBTagHandle
{
public:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& pArchive, const uint32_t pVersion)
    {
        pArchive & tag;
        pArchive & handle;
        pArchive & expectContrib;

        return;
    }

    BBTagHandle() :
        tag(0),
        handle(UNDEFINED_HANDLE) {
        expectContrib = vector<uint32_t>();
    }

    BBTagHandle(const uint64_t pTag, const uint64_t pHandle, const vector<uint32_t> pExpectContrib) :
        tag(pTag),
        handle(pHandle) {
        expectContrib = pExpectContrib;
    }

    int compareContrib(vector<uint32_t>& pContribVector);

    /*
     * Data members
     */
    uint64_t tag;
    uint64_t handle;
    vector<uint32_t> expectContrib;
};


class TagFile
{
public:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& pArchive, const uint32_t pVersion)
    {
        serializeVersion = pVersion;
        pArchive & objectVersion;
        pArchive & filename;
        pArchive & tagHandles;

        return;
    }

    TagFile() :
        serializeVersion(0),
        objectVersion(ARCHIVE_TAG_VERSION_1),
        filename("") {
        tagHandles = std::vector<BBTagHandle>();
    }

    TagFile(const std::string pFileName) :
        serializeVersion(0),
        objectVersion(ARCHIVE_TAG_VERSION_1),
        filename(pFileName) {
        tagHandles = std::vector<BBTagHandle>();
    }

    virtual ~TagFile()
    {
        // NOTE:  We do not unlock/close the tagfile here as the
        //        tagfile being deleted is a local copy.
        //        If the tagfile in the metadata is deleted,
        //        it should already be unlocked/closed.
    }

    /*
     * Static methods
     */
    static int addTagHandle(const LVKey* pLVKey, const BBJob pJob, const uint64_t pTag, vector<uint32_t> pExpectContrib, uint64_t& pHandle);
    static int createLockFile(const string pFilePath);
    static int load(TagFile* &pTagFile, const bfs::path& pTagFileName);
    static int lock(const bfs::path& pJobStepPath);
    static void unlock();
    static void unlock(const int pFd);

    /*
     * Inlined methods
     */
#if 0
    inline void dump(const char* pPrefix)
    {
        stringstream l_Line;

        if (pPrefix) {
            l_Line << pPrefix << ": ";
        }
//        l_Line << "sVrsn=" << serializeVersion
//               << ", oVrsn=" << objectVersion
        l_Line << "TagHandles filename=" << filename;
        LOG(bb,info) << l_Line.str();
        l_Line << " Start: " << tagHandles.size() << " tag-handle pairs";
        LOG(bb,info) << l_Line.str();

        if (tagHandles.size())
        {
            l_Line << ", ";
            dumpTagHandles(l_Line, strlen(pPrefix)+2+10);
            l_Line << " >";
        }
        LOG(bb,info) << l_Line.str();

        l_Line << "   End: " << tagHandles.size() << " tag-handle pairs";
        LOG(bb,info) << l_Line.str();

        return;
    }

    inline void dumpTagHandles(stringstream& l_Line, size_t pPrefixLen)
    {
        stringstream l_SS_SkipChars, l_SS_SkipChars2;
        for (size_t i=0; i<pPrefixLen-1; i++) l_SS_SkipChars << " ";
        string l_SkipChars = l_SS_SkipChars.str();
        for (size_t i=0; i<pPrefixLen; i++) l_SS_SkipChars2 << " ";
        string l_SkipChars2 = l_SS_SkipChars2.str();

        bool l_FirstEntry = true;
        char l_Prefix[64] = {'\0'};
        for (std::vector<BBTagHandle>::iterator th = tagHandles.begin(); th != tagHandles.end(); th++)
        {
            if (!l_FirstEntry)
            {
                l_Line << ",";
            }
            else
            {
                l_Line << endl << l_SkipChars << "Vector <";
                l_FirstEntry = false;
            }
            l_Line << endl << l_SkipChars2;
            snprintf(l_Prefix, sizeof(l_Prefix), "Tag %ull -> handle %ull", thp->tag, thp->handle);
        }
        return;
    }
#endif

    /*
     * Non-static methods
     */
    int save();

    /*
     * Data members
     */
    uint32_t serializeVersion;
    uint32_t objectVersion;
    std::string filename;
    std::vector<BBTagHandle> tagHandles;
};

#endif /* BB_TAGFILE_H_ */


