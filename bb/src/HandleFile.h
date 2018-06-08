/*******************************************************************************
 |    HandleFile.h
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

#ifndef BB_HANDLEFILE_H_
#define BB_HANDLEFILE_H_

#include <sstream>

#include <stdio.h>
#include <string.h>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/filesystem.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/string.hpp>

#include "bbinternal.h"
#include "BBStatus.h"
#include "BBTagInfo.h"

using namespace boost::archive;
namespace bfs = boost::filesystem;

/*******************************************************************************
 | Forward declarations
 *******************************************************************************/

/*******************************************************************************
 | Constants
 *******************************************************************************/
const uint32_t ARCHIVE_HANDLE_VERSION = 1;
const char LOCK_FILENAME[] = "lockfile";


/*******************************************************************************
 | Classes
 *******************************************************************************/

class HandleFile
{
public:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& pArchive, const uint32_t pVersion)
    {
        serializeVersion = pVersion;
        pArchive & objectVersion;
        pArchive & tag;
        pArchive & flags;
        pArchive & status;
        pArchive & totalTransferSize;
        pArchive & numContrib;
        pArchive & expectContrib;
        pArchive & transferKeys;
        pArchive & lockfd;          // NOTE: On load, we will either overlay this with the fd
                                    //       of the handle file locked or reset this value to -1

        return;
    }

    HandleFile() :
        serializeVersion(0),
        objectVersion(ARCHIVE_HANDLE_VERSION),
        tag(0),
        flags(0),
        status((uint64_t)BBNOTSTARTED),
        totalTransferSize(0),
        numContrib(0),
        expectContrib(""),
        transferKeys(""),
        lockfd(-1) {}

    HandleFile (const uint64_t pTag, BBTagInfo& pTagInfo) :
        serializeVersion(0),
        objectVersion(ARCHIVE_HANDLE_VERSION) {
        tag = pTag;
        flags = 0;
        status = ((uint64_t)BBNOTSTARTED);
        totalTransferSize = 0;
        vector<uint32_t>* l_ExpectContrib = pTagInfo.getExpectContrib();
        numContrib = l_ExpectContrib->size();
        stringstream l_Temp;
        pTagInfo.expectContribToSS(l_Temp);
        expectContrib = l_Temp.str();
        transferKeys = "";
        lockfd = -1;
    }


    // Static methods

//    static int calculate_xbbServerHandleStatus(HandleFile* pHandleFile, const char* pHandleFilePath, uint64_t& pStatus);

    static int createLockFile(const char* pFilePath);
    static int getTransferKeys(const uint64_t pJobId, const uint64_t pHandle, uint64_t& pBufferSize, char* pBuffer);
    static int get_xbbServerGetJobForHandle(uint64_t& pJobId, uint64_t& pJobStepId, const uint64_t pHandle);
    static int get_xbbServerGetHandle(BBJob& pJob, uint64_t pTag, vector<uint32_t>& pContrib, uint64_t& pHandle);
    static int get_xbbServerHandleInfo(uint64_t& pJobId, uint64_t& pJobStepId, uint64_t& pNumberOfReportingContribs, HandleFile* &pHandleFile, ContribIdFile* &pContribIdFile, const uint64_t pHandle, const uint32_t pContribId);
    static int get_xbbServerHandleList(vector<uint64_t>& pHandles, const BBJob pJob, const BBSTATUS pMatchStatus);
    static int get_xbbServerHandleStatus(BBSTATUS& pStatus, const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle);
    static int get_xbbServerHandleTransferKeys(string& pTransferKeys, const uint64_t pJobId, const uint64_t pHandle);
    static int loadHandleFile(HandleFile* &pHandleFile, const char* pHandleFileName);
    static int loadHandleFile(HandleFile* &pHandleFile, char* &pHandleFileName, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const int pLockOption=DO_NOT_LOCK_HANDLEFILE);
    static int lock(const char* pFile);
    static int processTransferHandleForJobStep(vector<uint64_t>& pHandles, const char* pDataStoreName, const BBSTATUS pMatchStatus);
    static int saveHandleFile(HandleFile* &pHandleFile, const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pTag, BBTagInfo& pTagInfo, const uint64_t pHandle);
    static int saveHandleFile(HandleFile* &pHandleFile, const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle);
    static void testForLock(const char* pFile);
    static void unlock(const int pFd);
    static int update_xbbServerHandleFile(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint64_t pFlags, const int pValue=1);
    static int update_xbbServerHandleResetStatus(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle);
    static int update_xbbServerHandleStatus(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const int64_t pSize);
    static int update_xbbServerHandleTransferKeys(BBTransferDef* pTransferDef, const LVKey* pLVKey, const BBJob pJob, const uint64_t pHandle);

    // Non-static methods

    void close();
    void close(const int pFd);
    void getContribArray(uint64_t &pNumContribsInArray, uint32_t* &pContribArray);
    BBSTATUS getLocalStatus(const uint64_t pNumberOfReportingContribs, ContribIdFile* pContribIdFile);
    void unlock();


    // Inlined methods

    inline int allContribsReported()
    {
        RETURN_FLAG(BBTI_All_Contribs_Reported);
    }

    inline int allExtentsTransferred()
    {
        RETURN_FLAG(BBTD_All_Extents_Transferred);
    }

    inline void dump(const char* pPrefix) {
        stringstream l_Line;
        char l_Status[64] = {'\0'};

        if (pPrefix) {
            l_Line << pPrefix << ": ";
        }
        getStrFromBBStatus((BBSTATUS)status, l_Status, sizeof(l_Status));
//        l_Line << "sVrsn=" << serializeVersion
//               << ", oVrsn=" << objectVersion
        l_Line << "tagId=" << tag \
               << hex << uppercase << ", flags=0x" << flags << nouppercase << dec \
               << ", status=" << l_Status \
               << ", totalTransferSize=" << totalTransferSize \
               << ", numContrib=" << numContrib \
               << ", expectContrib=" << expectContrib;
        if (transferKeys.size())
        {
            l_Line << ", transferKeys=" << transferKeys;
        }
        LOG(bb,debug) << l_Line.str();

        return;
    }

    inline uint64_t getNumContrib()
    {
        return numContrib;
    }

    inline int stopped()
    {
        RETURN_FLAG(BBTD_Stopped);
    }

    virtual ~HandleFile()
    {
        close();
    }

    uint32_t serializeVersion;
    uint32_t objectVersion;
    uint64_t tag;
    uint64_t flags;
    uint64_t status;
    uint64_t totalTransferSize;
    uint64_t numContrib;
    string   expectContrib;
    string   transferKeys;
    int      lockfd;
};

#endif /* BB_HANDLEFILE_H_ */


