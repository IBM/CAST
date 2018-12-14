/*******************************************************************************
 |    ContribIdFile.h
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

#ifndef BB_CONTRIBIDFILE_H_
#define BB_CONTRIBIDFILE_H_

#include <sstream>
#include <vector>

#include <stdio.h>
#include <string.h>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/filesystem.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>

#include "bbflags.h"
#include "bbinternal.h"
#include "BBTagID.h"
#include "BBTransferDef.h"
#include "ExtentInfo.h"
#include "LVKey.h"

using namespace boost::archive;
namespace bfs = boost::filesystem;


/*******************************************************************************
 | Forward declarations
 *******************************************************************************/

/*******************************************************************************
 | Constants
 *******************************************************************************/
const uint32_t ARCHIVE_CONTRIBID_VERSION = 1;


/*******************************************************************************
 | Classes
 *******************************************************************************/

/**
 * \class ContribIdFile
 * Defines the cross bbserver metadata that is associated with a contributor/transfer definition.
 */
class ContribIdFile
{
public:
    friend class boost::serialization::access;

    class FileData
    {
    public:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive& pArchive, const uint32_t pVersion)
        {
            pArchive & sourceIndex;
            pArchive & sourceFile;
            pArchive & targetFile;
            pArchive & flags;
        }

        FileData() :
            sourceIndex(0),
            sourceFile(""),
            targetFile(""),
            flags(0) {}

        FileData (uint32_t pSrcIndex, string& pSrcFile, string& pTgtFile, BBTransferDef* pTransferDef) :
            sourceIndex(pSrcIndex),
            sourceFile(pSrcFile),
            targetFile(pTgtFile),
            flags(0) {}

        inline int closed() {
            RETURN_FLAG(BBTD_All_Files_Closed);
        }

        inline void dump(stringstream& l_Line) {
            l_Line << " (sourceIndex=" << sourceIndex \
                   << ", sourceFile=" << sourceFile \
                   << ", targetFile=" << targetFile \
                   << hex << uppercase << ", flags=0x" << flags << nouppercase << dec << ")";

            return;
        }

        inline int failed() {
            RETURN_FLAG(BBTD_Failed);
        }

        inline int fileToBeRestarted() {
            return (stopped() | failed() | (!closed()));
        }

        inline int stopped() {
            RETURN_FLAG(BBTD_Stopped);
        }

        virtual ~FileData() {}

        uint32_t    sourceIndex;
        string      sourceFile;
        string      targetFile;
        uint64_t    flags;
    };

    template<class Archive>
    void serialize(Archive& pArchive, const uint32_t pVersion)
    {
        serializeVersion = pVersion;
        pArchive & serializeVersion;
        pArchive & objectVersion;
        pArchive & uid;
        pArchive & gid;
        pArchive & tag;
        pArchive & flags;
        pArchive & totalTransferSize;
        pArchive & files;

        return;
    }

    ContribIdFile() :
        serializeVersion(0),
        objectVersion(ARCHIVE_CONTRIBID_VERSION),
        uid(0),
        gid(0),
        tag(0),
        flags(0),
        totalTransferSize(0) {
        files = vector<FileData>();
    };

    ContribIdFile (BBTransferDef* pTransferDef) :
        serializeVersion(0),
        objectVersion(ARCHIVE_CONTRIBID_VERSION) {
        tag = pTransferDef->getTag();
        flags = 0;
        totalTransferSize = 0;
        uid = pTransferDef->getUserId();
        gid = pTransferDef->getGroupId();
        createFileData(files, pTransferDef);
    };

    static int allExtentsTransferredButThisContribId(const uint64_t pHandle, const BBTagID& pTagId, const uint32_t pContribId);
    static int isStopped(const BBJob pJob, const uint64_t pHandle, const uint32_t pContribId);
    static int loadContribIdFile(ContribIdFile* &pContribIdFile, const bfs::path& pHandleFilePath, const uint32_t pContribId, Uuid* pUuid=0);
    static int loadContribIdFile(ContribIdFile* &pContribIdFile, const LVKey* pLVKey, const bfs::path& pHandleFilePath, const uint32_t pContribId);
    static int loadContribIdFile(ContribIdFile* &pContribIdFile, uint64_t& pNumHandleContribs, uint64_t& pNumLVUuidContribs, const bfs::path& pHandleFilePath, const uint32_t pContribId);
    static int update_xbbServerContribIdFile(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId, const uint64_t pFlags, const int pValue=1);
    static int update_xbbServerFileStatus(const LVKey* pLVKey, BBTransferDef* pTransferDef, ExtentInfo& pExtentInfo, const uint64_t pFlags, const int pValue=1);
    static int update_xbbServerFileStatus(const LVKey* pLVKey, BBTransferDef* pTransferDef, uint64_t pHandle, uint32_t pContribId, Extent* pExtent, const uint64_t pFlags, const int pValue=1);
    static int update_xbbServerFileStatusForRestart(const LVKey* pLVKey, BBTransferDef* pRebuiltTransferDef, uint64_t pHandle, uint32_t pContribId, int64_t &pSize);
    static int saveContribIdFile(ContribIdFile* &pContribIdFile, const LVKey* pLVKey, const bfs::path& pHandleFilePath, const uint32_t pContribId);

    inline int allExtentsTransferred() {
        RETURN_FLAG(BBTD_All_Extents_Transferred);
    }

    inline int canceled() {
        RETURN_FLAG(BBTD_Canceled);
    }

    inline int extentsAreEnqueued() {
        RETURN_FLAG(BBTD_Extents_Enqueued);
    }

    inline int failed() {
        RETURN_FLAG(BBTD_Failed);
    }

    inline int stopped() {
        RETURN_FLAG(BBTD_Stopped);
    }

    inline int allFilesClosed() {
        int rc = 1;

        for (auto& fdata : files) {
            if (!(fdata.flags & BBTD_All_Files_Closed)) {
                rc = 0;
                break;
            }
        }

        return rc;
    }

    inline int anyFilesCanceled() {
        int rc = 0;

        for (auto& fdata : files) {
            if (fdata.flags & BBTD_Canceled) {
                rc = 1;
                break;
            }
        }

        return rc;
    }

    inline int anyFilesFailed() {
        int rc = 0;

        for (auto& fdata : files) {
            if (fdata.flags & BBTD_Failed) {
                rc = 1;
                break;
            }
        }

        return rc;
    }

    inline int anyFilesStopped() {
        int rc = 0;

        for (auto& fdata : files) {
            if (fdata.flags & BBTD_Stopped) {
                rc = 1;
                break;
            }
        }

        return rc;
    }

    inline void createFileData(vector<FileData>& pFiles, BBTransferDef* pTransferDef)
    {
        string l_Source = "";
        string l_Target = "";
        uint32_t l_SrcIndex = 0;
        for (auto& fileName : pTransferDef->files)
        {
            if (l_Source == "")
            {
                l_Source = fileName;
            }
            else
            {
                l_Target = fileName;
                FileData l_FileData(l_SrcIndex, l_Source, l_Target, pTransferDef);
                pFiles.push_back(l_FileData);
                l_SrcIndex += 2;
                l_Source = "";
                l_Target = "";
            }
            flags = 0;
        }

        return;
    }

    inline void dump(const char* pPrefix) {
        stringstream l_Line;

        if (pPrefix) {
            l_Line << pPrefix << ": ";
        }
//        l_Line << "sVrsn=" << serializeVersion
//               << ", oVrsn=" << objectVersion
        l_Line << hex << uppercase << " flags=0x" << flags << nouppercase << dec \
               << ", uid=" << uid << ", gid=" << gid \
               << ", totalTransferSize=" << totalTransferSize;
        if (files.size())
        {
            l_Line << ", ";
            dumpFiles(files, l_Line, strlen(pPrefix)+2+10);
            l_Line << " >";
        }
        LOG(bb,info) << l_Line.str();

        return;
    }

    inline void dumpFiles(vector<FileData>& pFiles, stringstream& l_Line, size_t pPrefixLen) {
        stringstream l_SS_SkipChars, l_SS_SkipChars2;
        for (size_t i=0; i<pPrefixLen-1; i++) l_SS_SkipChars << " ";
        string l_SkipChars = l_SS_SkipChars.str();
        for (size_t i=0; i<pPrefixLen; i++) l_SS_SkipChars2 << " ";
        string l_SkipChars2 = l_SS_SkipChars2.str();

        bool l_FirstEntry = true;
        for (auto& fdata : pFiles) {
            if (!l_FirstEntry) {
                l_Line << ",";
            } else {
                l_Line << endl << l_SkipChars << "Vector <";
                l_FirstEntry = false;
            }
            l_Line << endl << l_SkipChars2;
            fdata.dump(l_Line);
        }

        return;
    }

    inline int fileToBeRestarted(Extent& pExtent)
    {
        int rc = 0;

        if (stopped())
        {
            // Only if the contributor (transfer definition) is stopped,
            // can any of the files be restarted...
            for (auto& fdata : files)
            {
                if (pExtent.getSourceIndex() == fdata.sourceIndex)
                {
                    rc = fdata.fileToBeRestarted();
                    break;
                }
            }
        }

        return rc;
    }

    inline uid_t getGroupId()
    {
        return gid;
    }

    inline uid_t getUserId()
    {
        return uid;
    }

    inline int notRestartable()
    {
        // Not restartable if no files in transfer definition -or- finished normally -or- canceled
        return ((files.size() == 0) || (allExtentsTransferred() && allFilesClosed() && (!anyFilesFailed())) || (canceled() && (!stopped())));
    }

    int copyForRetrieveTransferDefinitions(BBTransferDefs& pTransferDefs, const string& pHostName, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId, const string& pTransferKeys);

    virtual ~ContribIdFile() {}

    uint32_t            serializeVersion;
    uint32_t            objectVersion;
    uid_t               uid;
    gid_t               gid;
    uint64_t            tag;
    uint64_t            flags;
    uint64_t            totalTransferSize;
    vector<FileData>    files;
};

#endif /* BB_CONTRIBIDFILE_H_ */
