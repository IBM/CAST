/*******************************************************************************
 |    BBTransferDef.h
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

#ifndef BB_BBTRANSFERDEF_H_
#define BB_BBTRANSFERDEF_H_

#include <iostream>
#include <map>
#include <vector>
#include <sstream>

// #include <boost/archive/binary_oarchive.hpp>
// #include <boost/archive/binary_iarchive.hpp>
// #include <boost/serialization/binary_object.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
// #include <boost/filesystem.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>

#include "bbapi_types.h"
#include "bbapi2.h"
#include "bbflags.h"
#include "bbinternal.h"
#include "BBJob.h"
#include "Extent.h"
#include "ExtentInfo.h"
#include "fh.h"
#include "logging.h"
#include "LVKey.h"
#include "Msg.h"

/*******************************************************************************
 | Macro definitions
 *******************************************************************************/
#define DUMP_TRANSDEF(SEV,JOBSTR) { \
    LOG(bb,SEV) << "    sVrsn: " << serializeVersion << " oVrsn: " << objectVersion; \
    LOG(bb,SEV) << "      Job: " << JOBSTR; \
    LOG(bb,SEV) << " Hostname: " << hostname \
                << hex << uppercase << setfill('0'); \
    LOG(bb,SEV) << "      Uid: 0x" << setw(8) << uid << "   Gid: 0x" << setw(8) << gid; \
    LOG(bb,SEV) << "   Handle: 0x" << setw(16) << transferHandle << setfill(' ') << nouppercase << dec << " (" << transferHandle << ")" << hex << uppercase << setfill('0'); \
    LOG(bb,SEV) << "ContribId: " << contribid << hex << uppercase << setfill('0') << "   Tag: 0x" << setw(8) << tag << " Flags: 0x" << setw(16) << flags \
                << setfill(' ') << nouppercase << dec; \
    if (files.size()) { \
        LOG(bb,SEV) << ">>>>> Start: " << files.size() << (files.size()==1 ? " file <<<<<" : " files <<<<<"); \
        uint32_t i = 0; \
        for (auto& f : files) { \
            LOG(bb,SEV) << i++ << ": " << f; \
        } \
        LOG(bb,SEV) << ">>>>>   End: " << files.size() << (files.size()==1 ? " file <<<<<" : " files <<<<<"); \
    } \
    if (keyvalues.size()) { \
        LOG(bb,SEV) << ">>>>> Start: " << keyvalues.size() << (keyvalues.size()==1 ? " keyvalue <<<<<" : " keyvalues <<<<<"); \
        uint32_t i = 0; \
        for (auto& kv : keyvalues) { \
            LOG(bb,SEV) << i++ << ": (" << kv.first << "," << kv.second << ")"; \
        } \
        LOG(bb,SEV) << ">>>>>   End: " << keyvalues.size() << (keyvalues.size()==1 ? " keyvalue <<<<<" : " keyvalues <<<<<"); \
    } \
    if (iomap.size()) { \
        LOG(bb,SEV) << ">>>>> Start: " << iomap.size() << (iomap.size()==1 ? " I/O object <<<<<" : " I/O objects <<<<<"); \
        uint32_t i = 0; \
        for (auto& io : iomap) { \
            LOG(bb,SEV) << i++ << ": (" << io.first << "," << io.second << ")"; \
        } \
        LOG(bb,SEV) << ">>>>>   End: " << iomap.size() << (iomap.size()==1 ? " I/O object <<<<<" : " I/O objects <<<<<"); \
    } \
    if (sizeTransferred.size()) { \
        LOG(bb,SEV) << ">>>>> Start: " << sizeTransferred.size() << (sizeTransferred.size()==1 ? " Size transferred <<<<<" : " Sizes transferred <<<<<"); \
        uint32_t i = 0; \
        for (auto& sz : sizeTransferred) { \
            LOG(bb,SEV) << i++ << ": " << sz; \
        } \
        LOG(bb,SEV) << ">>>>>   End: " << sizeTransferred.size() << (sizeTransferred.size()==1 ? " Size transferred <<<<<" : " Sizes transferred <<<<<"); \
    } \
}

/*******************************************************************************
 | Forward declarations
 *******************************************************************************/
class BBIO;
class BBLVKey_ExtentInfo;

/*******************************************************************************
 | Constants
 *******************************************************************************/
const uint32_t ARCHIVE_TRANSFERDEFS_VERSION = 1;
const uint32_t ARCHIVE_TRANSFERDEF_VERSION = 1;

/*******************************************************************************
 | Classes
 *******************************************************************************/

/**
 * \class BBTransferDefs
 * Defines the structure that is archived as the return value on Coral_RetrieveTransfers()
 */
class BBTransferDefs
{
public:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& pArchive, const uint32_t pVersion)
    {
        serializeVersion = pVersion;
        pArchive & serializeVersion;
        pArchive & objectVersion;
        pArchive & hostname;
        pArchive & jobid;
        pArchive & jobstepid;
        pArchive & handle;
        pArchive & contribid;
        pArchive & flags;
        pArchive & transferdefs;

        return;
    }

    BBTransferDefs() :
        serializeVersion(0),
        objectVersion(ARCHIVE_TRANSFERDEFS_VERSION),
        hostname(""),
        jobid(0),
        jobstepid(0),
        handle(0),
        contribid(0),
        flags((BB_RTV_TRANSFERDEFS_FLAGS)0)
        {
            transferdefs = vector<BBTransferDef*>();
        };

    BBTransferDefs(const string& pHostName, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId, const BB_RTV_TRANSFERDEFS_FLAGS pFlags) :
        serializeVersion(0),
        objectVersion(ARCHIVE_TRANSFERDEFS_VERSION),
        hostname(pHostName),
        jobid(pJobId),
        jobstepid(pJobStepId),
        handle(pHandle),
        contribid(pContribId),
        flags(pFlags)
        {
            transferdefs = vector<BBTransferDef*>();
        }

    /* Static methods                   */
#if BBSERVER
    static int xbbServerRetrieveTransfers(BBTransferDefs& pTransferDefs);
#endif

    /* Non-static methods               */

    int add(BBTransferDef* pTransferDef);
    void clear();
    void demarshall(string& pMarshalledTransferDefs);
    void dump(const char* pSev, const char* pPrefix=0);
    string marshall();
#if BBPROXY
    void restartTransfers(const string& pHostName, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId, uint32_t& pNumRestartedTransferDefs);
#endif
#if BBSERVER
    void stopTransfers(const string& pHostName, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId, uint32_t& pNumStoppedTransferDefs);
#endif

    /* Inlined methods                  */

    inline uint32_t getContribId() {
        return contribid;
    }

    inline BB_RTV_TRANSFERDEFS_FLAGS getFlags() {
        return flags;
    }

    inline string getHostName() {
        return hostname;
    }

    inline uint64_t getJobId() {
        return jobid;
    }

    inline uint64_t getJobStepId() {
        return jobstepid;
    }

    inline uint64_t getHandle() {
        return handle;
    }

    inline size_t getNumberOfDefinitions() {
        return transferdefs.size();
    }

    inline void compactTransferDefs() {
        transferdefs.resize(transferdefs.size());
        return;
    }

    virtual ~BBTransferDefs();

    // Non-static methods

    // Data members
    uint32_t                    serializeVersion;
    uint32_t                    objectVersion;
    string                      hostname;           // Echoed input
    uint64_t                    jobid;              // Echoed input
    uint64_t                    jobstepid;          // Echoed input
    uint64_t                    handle;             // Echoed input
    uint32_t                    contribid;          // Echoed input
    BB_RTV_TRANSFERDEFS_FLAGS   flags;              // Echoed input
    vector<BBTransferDef*>      transferdefs;
};

/**
 * \class BBTransferDef
 * Defines a set of files to be transferred from a given contributor
 */
class BBTransferDef
{
  public:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& pArchive, const uint32_t pVersion)
    {
        serializeVersion = pVersion;
        pArchive & serializeVersion;
        pArchive & objectVersion;
        pArchive & job;
        pArchive & uid;
        pArchive & gid;
        pArchive & contribid;
        pArchive & flags;
        pArchive & tag;
        pArchive & transferHandle;
        pArchive & hostname;
        pArchive & files;
        pArchive & keyvalues;
        pArchive & extents;
        pArchive & sizeTransferred;

        return;
    }

    BBTransferDef() :
        serializeVersion(0),
        objectVersion(ARCHIVE_TRANSFERDEF_VERSION),
        job(BBJob()),
        uid(0),
        gid(0),
        contribid(0),
        flags(0),
        tag(0),
        transferHandle(UNDEFINED_HANDLE),
        hostname(UNDEFINED_HOSTNAME),
        update(PTHREAD_MUTEX_INITIALIZER),
        files(),
        keyvalues(),
        iomap(),
        extents(),
        sizeTransferred()
        {
            iomap = map<uint16_t, BBIO*>();
            extents = vector<Extent>();
            sizeTransferred = vector<size_t>();
        }

    BBTransferDef(const BBTransferDef& src) :
        serializeVersion(src.serializeVersion),
        objectVersion(src.objectVersion),
        job(src.job),
        uid(src.uid),
        gid(src.gid),
        contribid(src.contribid),
        flags(src.flags),
        tag(src.tag),
        transferHandle(src.transferHandle),
        hostname(src.hostname),
        update(PTHREAD_MUTEX_INITIALIZER) {
        files = src.files;
        keyvalues = src.keyvalues;
        iomap = src.iomap;
        extents = src.extents;
        sizeTransferred = src.sizeTransferred;
    }

    ~BBTransferDef();

    // Static inline methods
    static inline uint32_t getSourceIndex(const uint32_t pTargetIndex) {
        return pTargetIndex ^ 1;
    }

    static inline uint32_t getTargetIndex(const uint32_t pSourceIndex) {
        return pSourceIndex ^ 1;
    }

    // Static methods
    static int demarshallTransferDef(txp::Msg* msg, BBTransferDef* transfer);
    static size_t getTotalTransferSize(BBTransferDef* pTransferDef);
    static size_t getTotalTransferSize(BBTransferDef* pTransferDef, const uint32_t pSourceIndex);
    static int marshallTransferDef(txp::Msg* msg, BBTransferDef* transfer, vector<txp::CharArray> &arr);

    // Non-static methods
    void cleanUp();
    void cleanUpIOMap();
    #if BBSERVER
    void copyExtentsForRetrieveTransferDefinitions(BBTransferDef* pSourceTransferDef, BBLVKey_ExtentInfo* pExtentInfo);
    int copyForRetrieveTransferDefinitions(BBTransferDefs& pTransferDefs, BBLVKey_ExtentInfo* pExtentInfo);
#endif
    void dumpExtents(const char* pSev, const char* pPrefix=0) const;
    void dump(const char* pSev, const char* pPrefix=0);
#if BBSERVER
    Extent* getAnyExtent(const uint32_t pSourceIndex);
    BBFILESTATUS getFileStatus(const LVKey* pLVKey, ExtentInfo& pExtentInfo);
#endif
    BBSTATUS getStatus();
    void incrSizeTransferred(Extent* pExtent);
    void lock();
#if BBSERVER
    void markAsStopped(const LVKey* pLVKey, const uint64_t pHandle, const uint32_t pContribId);
    int prepareForRestart(const LVKey* pLVKey, const BBJob pJob, const uint64_t pHandle, const int32_t pContribId, BBTransferDef* pRebuiltTransferDef, const int pPass);
#endif
    int replaceExtentVector(vector<Extent>* pNewList);
    int replaceExtentVector(BBTransferDef* pTransferDef);
    uint64_t retrieveJobId();
    uint64_t retrieveJobStepId();
#if BBSERVER
    int retrieveTransfers(BBTransferDefs& pTransferDefs, BBLVKey_ExtentInfo* pExtentInfo);
    void setAllExtentsTransferred(const LVKey* pLVKey, const uint64_t pHandle, const uint32_t pContribId, const int pValue=1);
    void setCanceled(const LVKey* pLVKey, const uint64_t pHandle, const uint32_t pContribId, const int pValue=1);
    void setExtentsEnqueued(const LVKey* pLVKey, const uint64_t pHandle, const uint32_t pContribId, const int pValue=1);
    void setFailed(const LVKey* pLVKey, const uint64_t pHandle, const uint32_t pContribId, const int pValue=1);
#endif
    void setJob();
#if BBSERVER
    void setStopped(const LVKey* pLVKey, const uint64_t pHandle, const uint32_t pContribId, const int pValue=1);
    int stopTransfer(const LVKey* pLVKey, const string& pHostName, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId);
#endif
    void unlock();

    inline int allExtentsTransferred() {
        RETURN_FLAG(BBTD_All_Extents_Transferred);
    }

    inline int all_CN_CP_TransfersInDefinition() {
        RETURN_FLAG(BBTD_All_CN_CP_Transfers);
    }

    inline int BSCFS_InRequest() {
        RETURN_FLAG(BBTD_BSCFS_In_Request);
    }

    inline int builtViaRetrieveTransferDefinition() {
        RETURN_FLAG(BBTD_Built_Via_Retrieve_Transfer_Definition);
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

    inline uint32_t getContribId() {
        return contribid;
    }

    inline gid_t getGroupId() {
        return gid;
    }

    inline string getHostName() {
        return hostname;
    }

    inline BBJob getJob() {
        return job;
    }

    inline uint64_t getJobId() {
        return job.getJobId();
    }

    inline uint64_t getJobObjectVersion() {
        return job.getObjectVersion();
    }

    inline uint64_t getJobSerializeVersion() {
        return job.getSerializeVersion();
    }

    inline uint64_t getJobStepId() {
        return job.getJobStepId();
    }

    inline size_t getNumberOfExtents() {
        return extents.size();
    }

    inline size_t getNumberOfKeys() {
        return keyvalues.size();
    }

    inline uint64_t getObjectVersion() {
        return objectVersion;
    }

    inline uint64_t getSerializeVersion() {
        return serializeVersion;
    }

    inline size_t getSizeTransferred(const uint32_t pSourceIndex) {
        return sizeTransferred[pSourceIndex];
    }

    inline uint64_t getTag() {
        return tag;
    }

    inline uint64_t getTransferHandle() {
        return transferHandle;
    }

    inline uid_t getUserId() {
        return uid;
    }

    inline int hasBSCFS_InRequest() {
        RETURN_FLAG(BBTD_BSCFS_In_Request);
    }

    inline int hasFilesInRequest() {
        return files.size();
    }

    inline uint64_t mergeFileFlags(uint64_t pFlags) {
        return pFlags | (flags & 0x000000000000FFFF);
    }

    inline int noStageinOrStageoutTransfersInDefinition() {
        RETURN_FLAG(BBTD_No_Stagein_Or_Stageout_Transfers);
    }

    inline int resizeLogicalVolumeDuringStageOut() {
        RETURN_FLAG(BBTD_Resize_Logical_Volume_During_Stageout);
    }

    inline void setAll_CN_CP_TransfersInDefinition(const int pValue=1) {
        SET_FLAG_AND_RETURN(BBTD_All_CN_CP_Transfers,pValue);
    }

    inline void setBSCFS_InRequest(const int pValue=1) {
        SET_FLAG_AND_RETURN(BBTD_BSCFS_In_Request,pValue);
    }

    inline void setBuiltViaRetrieveTransferDefinition(const int pValue=1) {
        SET_FLAG_AND_RETURN(BBTD_Built_Via_Retrieve_Transfer_Definition,pValue);
    }

    inline void setContribId(const uint32_t pContribId) {
        contribid = pContribId;
        return;
    }

    inline void setHostName(const string& pHostName) {
        hostname = pHostName;
        return;
    }

    inline void setJob(const BBJob pJob) {
        job = pJob;
        return;
    }

    inline void setJob(const uint64_t pJobId, const uint64_t pJobStepId) {
        job.jobid = pJobId;
        job.jobstepid = pJobStepId;
        return;
    }

    inline void setJob(const uint32_t pSerializeVersion, const uint32_t pObjectVersion, const uint64_t pJobId, const uint64_t pJobStepId) {
        job.serializeVersion = pSerializeVersion;
        job.objectVersion = pObjectVersion;
        job.jobid = pJobId;
        job.jobstepid = pJobStepId;
        return;
    }

    inline void setNoStageinOrStageoutTransfersInDefinition(const int pValue=1) {
        SET_FLAG_AND_RETURN(BBTD_No_Stagein_Or_Stageout_Transfers,pValue);
    }

    inline void setResizeLogicalVolumeDuringStageout(const int pValue=1) {
        SET_FLAG_AND_RETURN(BBTD_Resize_Logical_Volume_During_Stageout,pValue);
    }

    inline void setTag(const uint64_t pTag) {
        tag = pTag;
        return;
    }

    inline void setTransferHandle(const uint64_t pHandle) {
        transferHandle = pHandle;
        return;
    }

    inline int stopped() {
        RETURN_FLAG(BBTD_Stopped);
    }

    uint32_t                serializeVersion;
    uint32_t                objectVersion;
    BBJob                   job;
    uid_t                   uid;
    gid_t                   gid;
    uint32_t                contribid;          // Only set/valid for transfer definitions
                                                // in the metadata on bbServer or rebuilt
                                                // definitions from the metadata
    uint64_t                flags;
    uint64_t                tag;                // Only set/valid for transfer definitions
                                                // in the metadata on bbServer or rebuilt
                                                // definitions from the metadata
    uint64_t                transferHandle;     // Only set/valid for transfer definitions
                                                // in the metadata on bbServer or rebuilt
                                                // definitions from the metadata
    string                  hostname;           // Only set/valid for transfer definitions
                                                // in the metadata on bbServer or rebuilt
                                                // definitions from the metadata
    pthread_mutex_t         update;             ///< Mutex acquired by BB_AddFiles and BB_AddKeys
    vector<string>          files;              ///< Vector of the files to-be-transferred
    map<string, string>     keyvalues;          ///< Map of the key-values associated with the transfer
    map<uint16_t, BBIO*>    iomap;              ///< Map of bundleID to BBIO() object
    vector<Extent>          extents;
    vector<size_t>          sizeTransferred;    ///< Vector of the amount of data transferred.
                                                ///< Only maintained on bbServer, in real time for source file indices.
#if BBAPI
    map<string,string>      tgt_src_whole_file; ///< for whole copy of target to source, watch for unique source
    int checkOneCPSourceToDest(const std::string& src, const std::string& tgt){
        auto it = tgt_src_whole_file.find(tgt);
        if (it == tgt_src_whole_file.end() ) {
            tgt_src_whole_file[tgt]=src;
        }
        else return -1;
        return 0;
    }
#endif
};

#endif /* BB_BBTRANSFERDEF_H_ */
