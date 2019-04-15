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
const uint32_t DEFAULT_NUMBER_OF_ALLOWED_SKIPPED_DUMP_REQUESTS = 20;    // Default, if no activity, dump every hour
const double DEFAULT_DUMP_MGR_TIME_INTERVAL = 180.0;    // In seconds, default is to dump wrkqmgr every 3 minutes

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
        count(0),
        currentTime(""),
        serverCurrentTime("") {
    }

    HeartbeatEntry(const uint64_t pCount, const string& pCurrentTime, const string& pServerCurrentTime) :
        count(pCount),
        currentTime(pCurrentTime),
        serverCurrentTime(pServerCurrentTime) {
    }

    static string getHeartbeatCurrentTime();

    inline uint64_t getCount()
    {
        return count;
    };

    inline string getServerTime()
    {
        return serverCurrentTime;
    }

    inline string getTime()
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

    uint64_t count;             // Incremented when ANY command is received
                                // from a given bbServer.
    string currentTime;         // Timestamp of when ANY command was last received
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
        allowDump(DEFAULT_ALLOW_DUMP_OF_WORKQUEUE_MGR),
        dumpOnDelay(DEFAULT_DUMP_MGR_ON_DELAY),
        dumpOnRemoveWorkItem(DEFAULT_DUMP_MGR_ON_REMOVE_WORK_ITEM),
        delayMsgSent(0),
        asyncRequestFileSeqNbr(0),
        numberOfAllowedSkippedDumpRequests(DEFAULT_NUMBER_OF_ALLOWED_SKIPPED_DUMP_REQUESTS),
        numberOfSkippedDumpRequests(0),
        numberOfAllowedConcurrentCancelRequests(0),
        numberOfConcurrentCancelRequests(0),
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
            lockPinned = 0;
            checkForCanceledExtents = 0;
            transferQueueLocked = 0;
        };

    /**
     * \brief Destructor
     */
    virtual ~WRKQMGR() {};

    // Static data

    // Inlined static methods

    inline static void post_multiple(const size_t pCount)
    {
        for (size_t i=0; i<pCount; i++)
        {
            WRKQMGR::post();
        }
//        verify();

        return;
    }

    inline static void post()
    {
        sem_post(&sem_workqueue);

        return;
    }

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

    inline void decrementNumberOfConcurrentCancelRequests()
    {
        --numberOfConcurrentCancelRequests;

        return;
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

    inline size_t getNumberOfWorkQueues()
    {
        return wrkqs.size();
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

    inline uint32_t getNumberOfConcurrentCancelRequests()
    {
        return numberOfConcurrentCancelRequests;
    }

    inline uint64_t getNumberOfWorkQueueItemsProcessed()
    {
        return numberOfWorkQueueItemsProcessed;
    }

    inline string getServerLoggingLevel()
    {
        return loggingLevel;
    }

    inline size_t getSizeOfAllWorkQueues()
    {
        size_t l_TotalSize = 0;

        for (map<LVKey,WRKQE*>::iterator qe = wrkqs.begin(); qe != wrkqs.end(); qe++)
        {
            l_TotalSize += qe->second->getWrkQ_Size();
        }

        return l_TotalSize;
    }

    inline int getThrottleTimerCount()
    {
        return throttleTimerCount;
    }

    inline int getThrottleTimerPoppedCount()
    {
        return throttleTimerPoppedCount;
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

    inline void incrementNumberOfWorkItemsProcessed()
    {
        ++numberOfWorkQueueItemsProcessed;

        return;
    }

    inline void incrementNumberOfHP_WorkItemsProcessed(const uint64_t pOffset)
    {
        HPWrkQE->incrementNumberOfWorkItemsProcessed();
        lastOffsetProcessed = pOffset;

        return;
    };

    inline void incrementNumberOfWorkItemsProcessed(WRKQE* pWrkQE, const WorkID& pWorkItem)
    {
        if (pWrkQE != HPWrkQE)
        {
            // Not the high priority work queue.
            // Simply, increment the number of work items processed.
            pWrkQE->incrementNumberOfWorkItemsProcessed();
        }
        else
        {
            // For the high priority work queue, we have to manage
            // how work items are recorded as being complete.
            manageWorkItemsProcessed(pWorkItem);
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

    inline bool transferQueueIsLocked()
    {
        return (transferQueueLocked == pthread_self());
    }

    // Methods
    void addHPWorkItem(LVKey* pLVKey, BBTagID& pTagId);
    int addWrkQ(const LVKey* pLVKey, const uint64_t pJobId, const int pSuspendIndicator);
    int appendAsyncRequest(AsyncRequest& pRequest);
    void calcThrottleMode();
    uint64_t checkForNewHPWorkItems();
    void checkThrottleTimer();
    int createAsyncRequestFile(const char* pAsyncRequestFileName);
    void dump(const char* pSev, const char* pPrefix, DUMP_OPTION pOption=DUMP_ALWAYS);
    void dump(queue<WorkID>* l_WrkQ, WRKQE* l_WrkQE, const char* pSev, const char* pPostfix);
    void endProcessingHP_Request(AsyncRequest& pRequest);
    int findOffsetToNextAsyncRequest(int &pSeqNbr, int64_t &pOffset);
    void dumpHeartbeatData(const char* pSev, const char* pPrefix=0);
    int findWork(const LVKey* pLVKey, WRKQE* &pWrkQE);
    int getAsyncRequest(WorkID& pWorkItem, AsyncRequest& pRequest);
    int getThrottleRate(LVKey* pLVKey, uint64_t& pRate);
    int getWrkQE(const LVKey* pLVKey, WRKQE* &pWrkQE);
    int getWrkQE_WithCanceledExtents(WRKQE* &pWrkQE);
    void loadBuckets();
    void lock(const LVKey* pLVKey, const char* pMethod);
    void manageWorkItemsProcessed(const WorkID& pWorkItem);
    FILE* openAsyncRequestFile(const char* pOpenOption, int &pSeqNbr, const MAINTENANCE_OPTION pMaintenanceOption=NO_MAINTENANCE);
    void pinLock(const LVKey* pLVKey, const char* pMethod);
    void processAllOutstandingHP_Requests(const LVKey* pLVKey);
    void processThrottle(LVKey* pLVKey, WRKQE* pWrkQE, BBLV_Info* pLV_Info, BBTagID& pTagId, ExtentInfo& pExtentInfo, Extent* pExtent, double& pThreadDelay, double& pTotalDelay);
    void removeWorkItem(WRKQE* pWrkQE, WorkID& pWorkItem);
    int rmvWrkQ(const LVKey* pLVKey);
    void setDumpTimerPoppedCount(const double pTimerInterval);
    void setHeartbeatDumpPoppedCount(const double pTimerInterval);
    void setHeartbeatTimerPoppedCount(const double pTimerInterval);
    int setSuspended(const LVKey* pLVKey, const int pValue);
    int setThrottleRate(const LVKey* pLVKey, const uint64_t pRate);
    void setThrottleTimerPoppedCount(const double pTimerInterval);
    int startProcessingHP_Request(AsyncRequest& pRequest);
    void unlock(const LVKey* pLVKey, const char* pMethod);
    void unpinLock(const LVKey* pLVKey, const char* pMethod);
    void updateHeartbeatData(const string& pHostName);
    void updateHeartbeatData(const string& pHostName, const string& pServerTimeStamp);
    void verify();
    int verifyAsyncRequestFile(char* &pAsyncRequestFileName, int &pSeqNbr, const MAINTENANCE_OPTION pMaintenanceOption=FULL_MAINTENANCE);

    // Data members
    int                 throttleMode;
    volatile int        throttleTimerCount;
    int                 throttleTimerPoppedCount;
    int                 allowDump;
    int                 dumpOnDelay;
    int                 dumpOnRemoveWorkItem;
    volatile int        delayMsgSent;
    volatile int        asyncRequestFileSeqNbr;
    uint32_t            numberOfAllowedSkippedDumpRequests;
    volatile uint32_t   numberOfSkippedDumpRequests;
    uint32_t            numberOfAllowedConcurrentCancelRequests;
    volatile uint32_t   numberOfConcurrentCancelRequests;
    uint64_t            dumpOnRemoveWorkItemInterval;
    volatile int64_t    dumpTimerCount;
    volatile int64_t    heartbeatDumpCount;
    volatile int64_t    heartbeatTimerCount;
    int64_t             dumpTimerPoppedCount;
    int64_t             heartbeatDumpPoppedCount;
    int64_t             heartbeatTimerPoppedCount;
    int64_t             declareServerDeadCount;     // In seconds
    volatile uint64_t   numberOfWorkQueueItemsProcessed;
    volatile uint64_t   lastDumpedNumberOfWorkQueueItemsProcessed;
    volatile uint64_t   offsetToNextAsyncRequest;
    volatile uint64_t   lastOffsetProcessed;
    LVKey               lastQueueProcessed;
    LVKey               lastQueueWithEntries;
    string              loggingLevel;

    map<LVKey, WRKQE*>  wrkqs;
    map<string, HeartbeatEntry> heartbeatData;
    vector<uint64_t>    outOfOrderOffsets;
    vector<string>      inflightHP_Requests;
  private:
    int                 lockPinned;
    volatile int        checkForCanceledExtents;
    pthread_t           transferQueueLocked;
};

#endif /* BB_BBWRKQMGR_H_ */
