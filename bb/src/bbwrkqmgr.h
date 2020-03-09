/*******************************************************************************
 |    bbwrkqmgr.h
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


#ifndef BB_BBWRKQMGR_H_
#define BB_BBWRKQMGR_H_

#include <map>
#include <utility>

#include "time.h"

#include "bbinternal.h"
#include "BBTagID.h"
#include "bbwrkqe.h"
#include "CnxSock.h"
#include "LVKey.h"
#include "util.h"
#include "Uuid.h"
#include "xfer.h"

using namespace std;


/*******************************************************************************
 | Forward declarations
 *******************************************************************************/
class Extent;
class WorkID;


/*******************************************************************************
 | Constants
 *******************************************************************************/
const time_t ASYNC_REQUEST_FILE_PRUNE_TIME = 3600;   // In seconds, default 1 hour
//const time_t ASYNC_REQUEST_FILE_PRUNE_TIME = 300;   // In seconds, default 5 minutes
const uint64_t MAXIMUM_ASYNC_REQUEST_FILE_SIZE = 16 * 1024 * 1024;  // Default 16M
//const uint64_t MAXIMUM_ASYNC_REQUEST_FILE_SIZE = 32 * 1024;
const int DEFAULT_ALLOW_DUMP_OF_WORKQUEUE_MGR = 1;  // Default, allow dump of wrkqmgr
const int DEFAULT_DUMP_MGR_ON_REMOVE_WORK_ITEM = 0; // Default, do not dump wrkqmgr based on work items being removed
const int DEFAULT_DUMP_MGR_ON_DELAY = 0;    // Default, do not dump wrkqmgr when it 'delays'
const int DEFAULT_RETRY_VALUE = 10;         // Default, retry value for fread, fwrite, fseek, and ftell
const int DEFAULT_TURBO_CLIP_VALUE = 3;     // Downshift the async read request rate based upon this clip value
const double DEFAULT_DUMP_MGR_TIME_INTERVAL = 30.0;    // In seconds, default is to dump wrkqmgr every 30 seconds
const uint32_t DEFAULT_NUMBER_OF_ALLOWED_SKIPPED_DUMP_REQUESTS = 120;   // Default, if no activity, dump every hour
                                                                        // NOTE:  120*30 = 3600 seconds
const double DEFAULT_TURBO_FACTOR = 0.1;    // Upshift or downshift the async request read rate by this factor

const uint64_t DEFAULT_DUMP_MGR_ON_REMOVE_WORK_ITEM_INTERVAL = 1000;
const string XBBSERVER_ASYNC_REQUEST_BASE_FILENAME = "asyncRequests";


/*******************************************************************************
 | Enumerators
 *******************************************************************************/
enum DUMP_OPTION
{
    DUMP_ALWAYS             = 0,
    DUMP_ONLY_IF_THROTTLING = 1,
    DUMP_UNCONDITIONALLY    = 2
};
typedef enum DUMP_OPTION DUMP_OPTION;

enum MAINTENANCE_OPTION
{
    NO_MAINTENANCE = 0,
    FORCE_REOPEN = 1,
    MINIMAL_MAINTENANCE = 2,
    FULL_MAINTENANCE = 3,
    START_BBSERVER = 4,
    CREATE_NEW_FILE = 5
};
typedef MAINTENANCE_OPTION MAINTENANCE_OPTION;


/*******************************************************************************
 | External variables
 *******************************************************************************/
extern const LVKey* HPWrkQE_LVKey;
extern WRKQE* HPWrkQE;


/*******************************************************************************
 | Classes
 *******************************************************************************/
//
// asyncRequest class
//
class AsyncRequest
{
  public:
    static const size_t MAX_DATA_LENGTH = 1024;
    static const size_t MAX_HOSTNAME_LENGTH = 64;

    AsyncRequest()
    {
        init();
    };

    AsyncRequest(const char* pBuffer)
    {
        init();
        string l_HostName;
        activecontroller->gethostname(l_HostName);
        l_HostName.copy(hostname, sizeof(hostname));
        strCpy(data, pBuffer, sizeof(data));
    };

    AsyncRequest(const char* pHostName, const char* pData)
    {
        init();
        strCpy(hostname, pHostName, sizeof(hostname));
        strCpy(data, pData, sizeof(data));
    };

    AsyncRequest& operator=(const AsyncRequest& pSource)
    {
        memcpy(hostname, pSource.hostname, sizeof(hostname));
        memcpy(data, pSource.data, sizeof(data));

        return *this;
    };

    inline char* getData()
    {
        return data;
    };

    inline char* getHostName()
    {
        return hostname;
    }

    inline void init()
    {
        memset(hostname, 0, sizeof(hostname));
        memset(data, 0, sizeof(data));

        return;
    };

    inline int sameHostName()
    {
        string l_HostName;
        activecontroller->gethostname(l_HostName);

        return (strstr(hostname, l_HostName.c_str()) ? 1 : 0);
    }

    inline int str(char* pBuffer, size_t pSize)
    {
        int rc = 0;

        if (pSize >= sizeof(hostname)+sizeof(data)+1)
        {
            memcpy(pBuffer, hostname, sizeof(hostname));
            memcpy(pBuffer+sizeof(hostname), data, sizeof(data));
            memset(pBuffer+sizeof(hostname)+sizeof(data), 0, 1);
        }
        else
        {
            rc = -1;
            memset(pBuffer, 0, 1);
        }

        return rc;
    };

    AsyncRequest(const AsyncRequest& pSource)
    {
        *this = pSource;
    };

    ~AsyncRequest() {};

    char hostname[MAX_HOSTNAME_LENGTH];
    char data[MAX_DATA_LENGTH];
};


//
// HeartbeatEntry class
//
class HeartbeatEntry
{
  public:
    HeartbeatEntry() :
        count(0)
    {
        currentTime = timeval {.tv_sec=0, .tv_usec=0};
        serverCurrentTime = "";
    }

    HeartbeatEntry(const uint64_t pCount, const struct timeval& pCurrentTime, const string& pServerCurrentTime) :
        count(pCount),
        currentTime(pCurrentTime),
        serverCurrentTime(pServerCurrentTime) {
    }

    static void getCurrentTime(struct timeval& pTime);
    static string getHeartbeatCurrentTimeStr();

    inline uint64_t getCount()
    {
        return count;
    };

    inline string getServerTime()
    {
        return serverCurrentTime;
    }

    inline struct timeval getTime()
    {
        return currentTime;
    }

    HeartbeatEntry(const HeartbeatEntry& pSource)
    {
        this->count = pSource.count;
        this->currentTime = pSource.currentTime;
        this->serverCurrentTime = pSource.serverCurrentTime;
    };

    ~HeartbeatEntry() {};

    int serverDeclaredDead(const uint64_t pAllowedNumberOfSeconds);

    uint64_t count;             // Incremented when ANY command is received
                                // from a given bbServer.
    struct timeval currentTime; // Timestamp of when ANY command was last received
                                // from a given bbServer.
    string serverCurrentTime;   // Timestamp provided by the reporting bbServer.
                                // Only updated when a real heartbeat command
                                // is received from a given bbServer.
};


//
// WRKQMGR class
//
class WRKQMGR
{
  public:
    /**
     * \brief Constructor
     */
    WRKQMGR() :
        throttleMode(0),
        throttleTimerCount(0),
        throttleTimerPoppedCount(0),
        asyncRequestReadTimerCount(0),
        asyncRequestReadTimerPoppedCount(0),
        asyncRequestReadTurboFactor(1.0),
        asyncRequestReadConsecutiveNoNewRequests(0),
        allowDump(DEFAULT_ALLOW_DUMP_OF_WORKQUEUE_MGR),
        dumpOnDelay(DEFAULT_DUMP_MGR_ON_DELAY),
        dumpOnRemoveWorkItem(DEFAULT_DUMP_MGR_ON_REMOVE_WORK_ITEM),
        useAsyncRequestReadTurboMode(DEFAULT_USE_ASYNC_REQUEST_READ_TURBO_MODE),
        delayMsgSent(0),
        asyncRequestFileSeqNbr(0),
        numberOfAllowedSkippedDumpRequests(DEFAULT_NUMBER_OF_ALLOWED_SKIPPED_DUMP_REQUESTS),
        numberOfSkippedDumpRequests(0),
        numberOfAllowedConcurrentCancelRequests(0),
        numberOfConcurrentCancelRequests(0),
        numberOfAllowedConcurrentHPRequests(0),
        numberOfConcurrentHPRequests(0),
        dumpOnRemoveWorkItemInterval(DEFAULT_DUMP_MGR_ON_REMOVE_WORK_ITEM_INTERVAL),
        dumpTimerCount(0),
        heartbeatDumpCount(0),
        heartbeatTimerCount(0),
        dumpTimerPoppedCount(0),
        heartbeatDumpPoppedCount(0),
        heartbeatTimerPoppedCount(0),
        declareServerDeadCount(0),
        numberOfWorkQueueItemsProcessed(0),
        lastDumpedNumberOfWorkQueueItemsProcessed(0),
        offsetToNextAsyncRequest(0),
        lastOffsetProcessed(0)
        {
            lastQueueProcessed = LVKey();
            lastQueueWithEntries = LVKey();
            loggingLevel = "";
            wrkqs = map<LVKey, WRKQE*>();
            heartbeatData = map<string, HeartbeatEntry>();
            outOfOrderOffsets = vector<uint64_t>();
            checkForCanceledExtents = 0;
            lock_workQueueMgr = PTHREAD_MUTEX_INITIALIZER;
            workQueueMgrLocked = 0;
        };

    /**
     * \brief Destructor
     */
    virtual ~WRKQMGR() {};

    // Static data

    // Inlined static methods

    inline static void wait()
    {
        sem_wait(&sem_workqueue);

        return;
    }

    // Inlined non-static methods

    inline int crossingAsyncFileBoundary(const uint64_t pOffset)
    {
        return (pOffset < MAXIMUM_ASYNC_REQUEST_FILE_SIZE ? 0 : 1);
    }

    inline int delayMessageSent()
    {
        return delayMsgSent;
    }

    inline int getCheckForCanceledExtents()
    {
        return checkForCanceledExtents;
    };

    inline uint64_t getDeclareServerDeadCount()
    {
        return declareServerDeadCount;
    };

    inline int getDumpOnDelay()
    {
        return dumpOnDelay;
    };

    inline int getDumpOnRemoveWorkItem()
    {
        return dumpOnRemoveWorkItem;
    };

    inline uint64_t getDumpOnRemoveWorkItemInterval()
    {
        return dumpOnRemoveWorkItemInterval;
    };

    inline int getDumpTimerCount()
    {
        return dumpTimerCount;
    }

    inline int getDumpTimerPoppedCount()
    {
        return dumpTimerPoppedCount;
    }

    inline void getOffsetToNextAsyncRequest(int &pSeqNbr, uint64_t &pOffset)
    {
        pSeqNbr = asyncRequestFileSeqNbr;
        pOffset = offsetToNextAsyncRequest;

        return;
    }

    inline uint64_t getLastDumpedNumberOfWorkQueueItemsProcessed()
    {
        return lastDumpedNumberOfWorkQueueItemsProcessed;
    }

    inline uint32_t getNumberOfAllowedConcurrentCancelRequests()
    {
        return numberOfAllowedConcurrentCancelRequests;
    }

    inline uint32_t getNumberOfAllowedConcurrentHPRequests()
    {
        return numberOfAllowedConcurrentHPRequests;
    }

    inline uint32_t getNumberOfConcurrentCancelRequests()
    {
        return numberOfConcurrentCancelRequests;
    }

    inline uint32_t getNumberOfConcurrentHPRequests()
    {
        return numberOfConcurrentHPRequests;
    }

    inline uint64_t getNumberOfWorkQueueItemsProcessed()
    {
        return numberOfWorkQueueItemsProcessed;
    }

    inline string getServerLoggingLevel()
    {
        return loggingLevel;
    }

    inline int getThrottleTimerCount()
    {
        return throttleTimerCount;
    }

    inline int getThrottleTimerPoppedCount()
    {
        return throttleTimerPoppedCount;
    }

    inline int getUseAsyncRequestReadTurboMode()
    {
        return useAsyncRequestReadTurboMode;
    }

    inline int highPriorityWorkQueueIsEmpty(const LVKey* pLVKey)
    {
        return (HPWrkQE->getWrkQ_Size() == 0 ? 1 : 0);
    }

    inline void incrementNumberOfConcurrentCancelRequests()
    {
        ++numberOfConcurrentCancelRequests;

        return;
    }

    inline void incrementNumberOfConcurrentHPRequests()
    {
        ++numberOfConcurrentHPRequests;

        return;
    }

    inline void incrementNumberOfWorkItemsProcessed()
    {
        ++numberOfWorkQueueItemsProcessed;

        return;
    }

    inline void incrementNumberOfHP_WorkItemsProcessed(const uint64_t pOffset)
    {
        HPWrkQE->incrementNumberOfWorkItemsProcessed();
        lastOffsetProcessed = pOffset;
        if (g_LogAllAsyncRequestActivity)
        {
            LOG(bb,info) << "AsyncRequest -> incrementNumberOfHP_WorkItemsProcessed(): numberOfWorkQueueItemsProcessed " << HPWrkQE->getNumberOfWorkItemsProcessed() \
                         << ", lastOffsetProcessed 0x" << hex << uppercase << setfill('0') << setw(8) << lastOffsetProcessed << setfill(' ') << nouppercase << dec;
        }

        return;
    };

    inline int inThrottleMode()
    {
        return throttleMode;
    }

    inline void setAllowDumpOfWorkQueueMgr(const int pValue)
    {
        allowDump = pValue;

        return;
    }

    inline void setCheckForCanceledExtents(const int pValue)
    {
        checkForCanceledExtents = pValue;

        return;
    }

    inline void setDelayMessageSent(const int pValue=1)
    {
        delayMsgSent = pValue;

        return;
    }

    inline void setDumpOnDelay(const int pValue)
    {
        dumpOnDelay = pValue;

        return;
    };

    inline void setDumpOnRemoveWorkItem(const int pValue)
    {
        dumpOnRemoveWorkItem = pValue;

        return;
    };

    inline void setDumpOnRemoveWorkItemInterval(const uint64_t pValue)
    {
        dumpOnRemoveWorkItemInterval = pValue;

        return;
    };

    inline void setDumpTimerCount(const int pValue)
    {
        dumpTimerCount = pValue;

        return;
    }

    inline void setLastDumpedNumberOfWorkQueueItemsProcessed(const int pValue)
    {
        lastDumpedNumberOfWorkQueueItemsProcessed = pValue;

        return;
    }

    inline void setLastQueueProcessed(LVKey* pLVKey)
    {
        LOG(bb,debug) << "WRKQMGR::setLastQueueProcessed(): lastQueueProcessed changing from = " << lastQueueProcessed << " to " << *pLVKey;
        lastQueueProcessed = *pLVKey;

        return;
    }

    inline void setLastQueueWithEntries(LVKey pLVKey)
    {
        if (lastQueueWithEntries != pLVKey)
        {
            LOG(bb,debug) << "WRKQMGR::setLastQueueWithEntries(): lastQueueWithEntries changing from = " << lastQueueWithEntries << " to " << pLVKey;
        }

        lastQueueWithEntries = pLVKey;

        return;
    }

    inline void setNumberOfAllowedConcurrentCancelRequests(const uint32_t pValue)
    {
        numberOfAllowedConcurrentCancelRequests = pValue;

        return;
    }

    inline void setNumberOfAllowedConcurrentHPRequests(const uint32_t pValue)
    {
        numberOfAllowedConcurrentHPRequests = pValue;

        return;
    }

    inline void setNumberOfAllowedSkippedDumpRequests(const uint32_t pValue)
    {
        numberOfAllowedSkippedDumpRequests = pValue;

        return;
    }

    inline void setNumberOfConcurrentCancelRequests(const uint32_t pValue)
    {
        numberOfConcurrentCancelRequests = pValue;

        return;
    }

    inline void setNumberOfConcurrentHPRequests(const uint32_t pValue)
    {
        numberOfConcurrentHPRequests = pValue;

        return;
    }

    inline void setOffsetToNextAsyncRequest(const int pSeqNbr, const uint64_t pOffset)
    {
        asyncRequestFileSeqNbr = pSeqNbr;
        offsetToNextAsyncRequest = pOffset;

        return;
    }

    inline void setServerLoggingLevel(const string pValue)
    {
        loggingLevel = pValue;

        return;
    }

    inline void setThrottleTimerCount(const int pValue)
    {
        throttleTimerCount = pValue;

        return;
    }

    inline void setThrottleTimerPoppedCount(const int pValue)
    {
        throttleTimerPoppedCount = pValue;

        return;
    }

    inline void setUseAsyncRequestReadTurboMode(const int pValue)
    {
        useAsyncRequestReadTurboMode = pValue;

        return;
    }

    inline bool workQueueMgrIsLocked()
    {
        return (workQueueMgrLocked == pthread_self());
    }

    // Methods
    void addHPWorkItem(LVKey* pLVKey, BBTagID& pTagId);
    int addWrkQ(const LVKey* pLVKey, BBLV_Info* pLV_Info, const uint64_t pJobId, const int pSuspendIndicator);
    int appendAsyncRequest(AsyncRequest& pRequest);
    void calcLastWorkQueueWithEntries();
    void calcThrottleMode();
    uint64_t checkForNewHPWorkItems();
    int checkLoggingLevel(const char* pSev);
    void checkThrottleTimer();
    int createAsyncRequestFile(const char* pAsyncRequestFileName);
    void decrementNumberOfConcurrentCancelRequests();
    void decrementNumberOfConcurrentHPRequests();
    void dump(const char* pSev, const char* pPrefix, DUMP_OPTION pOption=DUMP_ALWAYS);
    void dump(queue<WorkID>* l_WrkQ, WRKQE* l_WrkQE, const char* pSev, const char* pPostfix);
    void endProcessingHP_Request(AsyncRequest& pRequest);
    int findOffsetToNextAsyncRequest(int &pSeqNbr, int64_t &pOffset);
    void dumpHeartbeatData(const char* pSev, const char* pPrefix=0);
    int findWork(const LVKey* pLVKey, WRKQE* &pWrkQE);
    int getAsyncRequest(WorkID& pWorkItem, AsyncRequest& pRequest);
    int getAsyncRequestReadTimerPoppedCount();
    HeartbeatEntry* getHeartbeatEntry(const string& pHostName);
    uint64_t getDeclareServerDeadCount(const BBJob pJob, const uint64_t pHandle, const int32_t pContribId);
    size_t getNumberOfWorkQueues();
    size_t getSizeOfAllWorkQueues();
    int getThrottleRate(LVKey* pLVKey, uint64_t& pRate);
    int getWrkQE(const LVKey* pLVKey, WRKQE* &pWrkQE);
    void getWrkQE_WithCanceledExtents(WRKQE* &pWrkQE);
    void incrementNumberOfWorkItemsProcessed(WRKQE* pWrkQE, const WorkID& pWorkItem);
    int isServerDead(const BBJob pJob, const uint64_t pHandle, const int32_t pContribId);
    void loadBuckets();
    void lockWorkQueueMgr(const LVKey* pLVKey, const char* pMethod, int* pLocalMetadataUnlockedInd=0);
    int lockWorkQueueMgrIfNeeded(const LVKey* pLVKey, const char* pMethod, int* pLocalMetadataUnlockedInd=0);
    void manageWorkItemsProcessed(const WorkID& pWorkItem);
    FILE* openAsyncRequestFile(const char* pOpenOption, int &pSeqNbr, const MAINTENANCE_OPTION pMaintenanceOption=NO_MAINTENANCE);
    string peekAtNextAsyncRequest(WorkID& pWorkItem);
    void post();
    void post_multiple(const size_t pCount);
    void processAllOutstandingHP_Requests(const LVKey* pLVKey);
    void processThrottle(LVKey* pLVKey, WRKQE* pWrkQE, BBLV_Info* pLV_Info, BBTagID& pTagId, ExtentInfo& pExtentInfo, Extent* pExtent, double& pThreadDelay, double& pTotalDelay);
    void processTurboFactorForFoundRequest();
    void processTurboFactorForNotFoundRequest();
    void removeWorkItem(WRKQE* pWrkQE, WorkID& pWorkItem, bool& pLastWorkItemRemoved);
    int rmvWrkQ(const LVKey* pLVKey);
    void setAsyncRequestReadTimerPoppedCount(const double pTimerInterval);
    void setDumpTimerPoppedCount(const double pTimerInterval);
    void setHeartbeatDumpPoppedCount(const double pTimerInterval);
    void setHeartbeatTimerPoppedCount(const double pTimerInterval);
    int setSuspended(const LVKey* pLVKey, LOCAL_METADATA_RELEASED &pLocal_Metadata_Lock_Released, const int pValue);
    int setThrottleRate(const LVKey* pLVKey, const uint64_t pRate);
    void setThrottleTimerPoppedCount(const double pTimerInterval);
    int startProcessingHP_Request(AsyncRequest& pRequest);
    void unlockWorkQueueMgr(const LVKey* pLVKey, const char* pMethod, int* pLocalMetadataUnlockedInd=0);
    int unlockWorkQueueMgrIfNeeded(const LVKey* pLVKey, const char* pMethod);
    void updateHeartbeatData(const string& pHostName);
    void updateHeartbeatData(const string& pHostName, const string& pServerTimeStamp);
    void verify();
    int verifyAsyncRequestFile(char* &pAsyncRequestFileName, int &pSeqNbr, const MAINTENANCE_OPTION pMaintenanceOption=FULL_MAINTENANCE);

    // Data members
    //
    // NOTE:  Unless otherwise noted, data member access is serialized
    //        with the work queue manager lock (lock_workQueueMgr)
    int                 throttleMode;
    volatile int        throttleTimerCount;
    int                 throttleTimerPoppedCount;
    volatile int        asyncRequestReadTimerCount;
    int                 asyncRequestReadTimerPoppedCount;
    volatile double     asyncRequestReadTurboFactor;
    volatile uint64_t   asyncRequestReadConsecutiveNoNewRequests;
    int                 allowDump;
    int                 dumpOnDelay;
    int                 dumpOnRemoveWorkItem;
    int                 useAsyncRequestReadTurboMode;
    volatile int        delayMsgSent;
    volatile int        asyncRequestFileSeqNbr;
    uint32_t            numberOfAllowedSkippedDumpRequests;
    volatile uint32_t   numberOfSkippedDumpRequests;
    uint32_t            numberOfAllowedConcurrentCancelRequests;
    volatile uint32_t   numberOfConcurrentCancelRequests;       // Access is serialized with the
                                                                // HPWrkQE transfer queue lock
    uint32_t            numberOfAllowedConcurrentHPRequests;
    volatile uint32_t   numberOfConcurrentHPRequests;           // Access is serialized with the
                                                                // HPWrkQE transfer queue lock
    uint64_t            dumpOnRemoveWorkItemInterval;
    volatile int64_t    dumpTimerCount;
    volatile int64_t    heartbeatDumpCount;
    volatile int64_t    heartbeatTimerCount;                    // Access is serialized with the
                                                                // HPWrkQE transfer queue lock
    int64_t             dumpTimerPoppedCount;
    int64_t             heartbeatDumpPoppedCount;
    int64_t             heartbeatTimerPoppedCount;
    int64_t             declareServerDeadCount;                 // In seconds
    volatile uint64_t   numberOfWorkQueueItemsProcessed;
    volatile uint64_t   lastDumpedNumberOfWorkQueueItemsProcessed;
    volatile uint64_t   offsetToNextAsyncRequest;               // Access is serialized with the
                                                                // HPWrkQE transfer queue lock
    volatile uint64_t   lastOffsetProcessed;                    // Access is serialized with the
                                                                // HPWrkQE transfer queue lock
    LVKey               lastQueueProcessed;
    LVKey               lastQueueWithEntries;
    string              loggingLevel;

    map<LVKey, WRKQE*>  wrkqs;
    map<string, HeartbeatEntry> heartbeatData;  // Access is serialized with the
                                                // HPWrkQE transfer queue lock
    vector<uint64_t>    outOfOrderOffsets;      // Access is serialized with the
                                                // HPWrkQE transfer queue lock
    vector<string>      inflightHP_Requests;    // Access is serialized with the
                                                // HPWrkQE transfer queue lock
  private:
    volatile int        checkForCanceledExtents;
    pthread_mutex_t     lock_workQueueMgr;
    pthread_t           workQueueMgrLocked;
};

#endif /* BB_BBWRKQMGR_H_ */
