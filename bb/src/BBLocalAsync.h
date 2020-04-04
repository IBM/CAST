/*******************************************************************************
 |    BBLocalAsync.h
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

#ifndef BB_BBLOCALASYNC_H_
#define BB_BBLOCALASYNC_H_

#include <atomic>
#include <map>
#include <queue>
#include <string>

#include <semaphore.h>

#include "BBTagID.h"
#include "LVKey.h"


/*
 * NOTE:  When adding a new request type, which represents the
 *        priority grouping of a set of BBLocalRequests together
 *        for scheduling purposes, update these three elements:
 *        1) enum LOCAL_ASYNC_REQUEST_PRIORITY in BBLocalAsync.h
 *        2) static class BBLocalAsync in BBLocalAsync.h
 *        3) method getLocalAsyncPriorityStr() in BBLocalAsync.cc
 */


/*******************************************************************************
 | Forward declarations
 *******************************************************************************/
class BBLocalRequest;
class BBLV_Metadata;


/*******************************************************************************
 | External data
 *******************************************************************************/
extern BBLV_Metadata metadata;
extern int64_t g_IBStatsLowActivityClipValue;
extern uint64_t g_AsyncRemoveJobInfoNumberPerGroup;


/*******************************************************************************
 | Constants
 *******************************************************************************/


/*******************************************************************************
 | Enumerators
 *******************************************************************************/
enum LOCAL_ASYNC_REQUEST_PRIORITY
{
    NONE           = 0,
    HIGH           = 10,
    MEDIUM_HIGH    = 30,
    MEDIUM         = 50,
    MEDIUM_LOW     = 70,
    LOW            = 90,
    VERY_LOW       = 91
};
typedef enum LOCAL_ASYNC_REQUEST_PRIORITY LOCAL_ASYNC_REQUEST_PRIORITY;


//
// BBAsyncRequestType class
//
class BBAsyncRequestType
{
  public:
    /**
     * \brief Constructor
     */
    BBAsyncRequestType(std::string pName, LOCAL_ASYNC_REQUEST_PRIORITY pPriority, double pPercentageOfThreads) :
        name(pName),
        priority(pPriority),
        percentage_of_threads(pPercentageOfThreads) {
    };

    /**
     * \brief Destructor
     */
    virtual ~BBAsyncRequestType() {};

    // Inlined non-static methods

    // Static methods

    // Non-static methods

    // Data members
    std::string                     name;
    LOCAL_ASYNC_REQUEST_PRIORITY    priority;
    double                          percentage_of_threads;
};


//
// BBAsyncRequestData class
//
class BBAsyncRequestData
{
  public:
    /**
     * \brief Constructor
     */
    BBAsyncRequestData(LOCAL_ASYNC_REQUEST_PRIORITY pPriority, uint64_t pMaximumConcurrentRunning) :
        priority(pPriority),
        lastRequestNumberIssued(0),
        lastRequestNumberDispatched(0),
        lastRequestNumberProcessed(0),
        maximumConcurrentRunning(pMaximumConcurrentRunning),
        mutex(PTHREAD_MUTEX_INITIALIZER) {
    };

    /**
     * \brief Destructor
     */
    virtual ~BBAsyncRequestData() {};

    // Inlined non-static methods
    inline int64_t getLastRequestNumberProcessed()
    {
        return lastRequestNumberProcessed;
    }

    inline int64_t getMaximumConcurrentRunning()
    {
        return maximumConcurrentRunning;
    }

    inline void lock()
    {
        pthread_mutex_lock(&mutex);

        return;
    }

    inline void unlock()
    {
        pthread_mutex_unlock(&mutex);

        return;
    }

    // Static methods

    // Non-static methods
  private:
    int64_t increment(int64_t& pNumber);
  public:
    void dump(const char* pPrefix="");
    size_t numberOfNonDispatchedRequests();
    void recordRequestCompletion(int64_t pRequestNumber);
    int64_t addRequest(BBLocalRequest* pRequest);
    int64_t getNumberOfInFlightRequests();
    int64_t removeNextRequest(BBLocalRequest* &pRequest);

    // Data members
    LOCAL_ASYNC_REQUEST_PRIORITY    priority;
    int64_t                         lastRequestNumberIssued;
    int64_t                         lastRequestNumberDispatched;
    int64_t                         lastRequestNumberProcessed;
    int64_t                         maximumConcurrentRunning;
    pthread_mutex_t                 mutex;
    queue<BBLocalRequest*>          requests;
    vector<int64_t>                 outOfSequenceRequests;
};

//
// BBController class
//
class BBController
{
  public:
    /**
     * \brief Constructor
     */
    BBController() :
        count(0),
        fired(0),
        poppedCount(0) { };

    /**
     * \brief Destructor
     */
    virtual ~BBController() { };

    // Static data

    // Inlined methods

    inline int alreadyFired()
    {
        return fired;
    };

    inline int getCount()
    {
        return count;
    };

    inline void lock()
    {
        pthread_mutex_lock(&mutex);

        return;
    };

    inline void setCount(const int pValue)
    {
        lock();
        count = pValue;
        unlock();

        return;
    };

    inline void setTimerFired(const int pValue)
    {
        lock();
        fired = pValue;
        unlock();

        return;
    };

    inline void unlock()
    {
        pthread_mutex_unlock(&mutex);

        return;
    };

    // Inlined virtual methods
    virtual inline int getTimerPoppedCount()
    {
        return poppedCount;
    };

    virtual inline int timeToFire()
    {
        return (!fired) && poppedCount && (++count >= poppedCount);
    };

    // Virtual methods
    virtual void checkTimeToPerform() { return; };
    virtual void init(const double pTimerInterval) { return; };

    // Non-virtual methods

    // Data members
    pthread_mutex_t     mutex;
    volatile int64_t    count;
    volatile int        fired;
    int64_t             poppedCount;
};

class AsyncRemoveJobInfo_Controller : public BBController
{
  public:
    /**
     * \brief Constructor
     */
    AsyncRemoveJobInfo_Controller() :
        BBController() {
    };

    /**
     * \brief Destructor
     */
    virtual ~AsyncRemoveJobInfo_Controller() { };

    // Virtual methods
    virtual void checkTimeToPerform();
    virtual void init(const double pTimerInterval);
    virtual inline int timeToFire();

    // Data members
};

class BBIB_Stats_Controller : public BBController
{
  public:
    /**
     * \brief Constructor
     */
    BBIB_Stats_Controller() :
        BBController() {
    };

    /**
     * \brief Destructor
     */
    virtual ~BBIB_Stats_Controller() { };

    // Virtual methods
    virtual void checkTimeToPerform();
    virtual void init(const double pTimerInterval);

    // Data members
};

class BBIO_Stats_Controller : public BBController
{
  public:
    /**
     * \brief Constructor
     */
    BBIO_Stats_Controller() :
        BBController() {
    };

    /**
     * \brief Destructor
     */
    virtual ~BBIO_Stats_Controller() { };

    // Virtual methods
    virtual void checkTimeToPerform();
    virtual void init(const double pTimerInterval);

    // Data members
};

class CycleActivities_Controller : public BBController
{
  public:
    /**
     * \brief Constructor
     */
    CycleActivities_Controller() :
        BBController() {
    };

    /**
     * \brief Destructor
     */
    virtual ~CycleActivities_Controller() { };

    // Virtual methods

    // Data members
};

class Dump_Counters_Controller : public BBController
{
  public:
    /**
     * \brief Constructor
     */
    Dump_Counters_Controller() :
        BBController() {
    };

    /**
     * \brief Destructor
     */
    virtual ~Dump_Counters_Controller() { };

    // Virtual methods
    virtual void checkTimeToPerform();
    virtual void init(const double pTimerInterval);

    // Data members
};

class Dump_Heartbeat_Data_Controller : public BBController
{
  public:
    /**
     * \brief Constructor
     */
    Dump_Heartbeat_Data_Controller() :
        BBController() {
    };

    /**
     * \brief Destructor
     */
    virtual ~Dump_Heartbeat_Data_Controller() { };

    // Virtual methods
    virtual void checkTimeToPerform();
    virtual void init(const double pTimerInterval);

    // Data members
};

class Dump_Local_Async_Controller : public BBController
{
  public:
    /**
     * \brief Constructor
     */
    Dump_Local_Async_Controller() :
        BBController() {
    };

    /**
     * \brief Destructor
     */
    virtual ~Dump_Local_Async_Controller() { };

    // Virtual methods
    virtual void checkTimeToPerform();
    virtual void init(const double pTimerInterval);

    // Data members
};

class Dump_WrkQMgr_Controller : public BBController
{
  public:
    /**
     * \brief Constructor
     */
    Dump_WrkQMgr_Controller() :
        BBController() {
    };

    /**
     * \brief Destructor
     */
    virtual ~Dump_WrkQMgr_Controller() { };

    // Virtual methods
    virtual void checkTimeToPerform();
    virtual void init(const double pTimerInterval);

    // Data members
};

class Heartbeat_Controller : public BBController
{
  public:
    /**
     * \brief Constructor
     */
    Heartbeat_Controller() :
        BBController() {
    };

    /**
     * \brief Destructor
     */
    virtual ~Heartbeat_Controller() { };

    // Virtual methods
    virtual void checkTimeToPerform();
    virtual void init(const double pTimerInterval);

    // Data members
};

class RemoteAsyncRequest_Controller : public BBController
{
  public:
    /**
     * \brief Constructor
     */
    RemoteAsyncRequest_Controller() :
        BBController() {
    };

    /**
     * \brief Destructor
     */
    virtual ~RemoteAsyncRequest_Controller() { };

    // Virtual methods
    virtual int getTimerPoppedCount();
    virtual void init(const double pTimerInterval);

    // Data members
};

class ThrottleBucket_Controller : public BBController
{
  public:
    /**
     * \brief Constructor
     */
    ThrottleBucket_Controller() :
        BBController() {
    };

    /**
     * \brief Destructor
     */
    virtual ~ThrottleBucket_Controller() { };

    // Virtual methods
    virtual void init(const double pTimerInterval);

    // Data members
};

//
// BBLocalRequest class
//
class BBLocalRequest
{
  public:
    /**
     * \brief Constructor
     */
    BBLocalRequest(std::string pName, LOCAL_ASYNC_REQUEST_PRIORITY pPriority=HIGH) :
        name(pName),
        priority(pPriority) {
    };

    /**
     * \brief Destructor
     */
    virtual ~BBLocalRequest() { };

    // Inlined static methods

    // Inlined non-static methods
    inline LOCAL_ASYNC_REQUEST_PRIORITY getPriority()
    {
        return priority;
    };

    // Inlined virtual methods
    inline virtual void doit() { return; };
    inline virtual int dumpOnAdd() { return 0; };
    inline virtual int dumpOnDelete() { return 0; };
    inline virtual int dumpOnRemove() { return 0; };

    // Static methods
    static std::string getPriorityStr(LOCAL_ASYNC_REQUEST_PRIORITY pPriority);

    // Virtual methods
    virtual void dump(const char* pPrefix="");

    // Non-virtual methods
    void dumpRequest(stringstream& pStream);
    void end_logging();
    void start_logging();

    // Data members
    std::string                  name;
    LOCAL_ASYNC_REQUEST_PRIORITY priority;
};

//
// Classes derived from BBLocalRequest
//
class BBAsyncRemoveJobInfo : public BBLocalRequest
{
  public:
    /**
     * \brief Constructor
     */
    BBAsyncRemoveJobInfo() :
        BBLocalRequest("BBAsyncRemoveJobInfo", VERY_LOW) {
    };

    /**
     * \brief Destructor
     */
    virtual ~BBAsyncRemoveJobInfo() { };

    // Inlined virtual methods
//    inline virtual int dumpOnAdd() { return 1; };
//    inline virtual int dumpOnDelete() { return 1; };
//    inline virtual int dumpOnRemove() { return 1; };

    // Static methods
    static int64_t getLastRequestNumberProcessed();

    // Inlined methods

    // Virtual methods
    virtual void doit();

    // Data members
};

class BBCheckCycleActivities : public BBLocalRequest
{
  public:
    /**
     * \brief Constructor
     */
    BBCheckCycleActivities() :
        BBLocalRequest("BBCheckCycleActivities", HIGH) {
    };

    /**
     * \brief Destructor
     */
    virtual ~BBCheckCycleActivities() { };

    // Inlined virtual methods
//    inline virtual int dumpOnAdd() { return 1; };
//    inline virtual int dumpOnDelete() { return 1; };
//    inline virtual int dumpOnRemove() { return 1; };

    // Static methods
    static int64_t getLastRequestNumberProcessed();

    // Inlined methods

    // Virtual methods
    virtual void doit();

    // Data members
};

class BBCleanUpContribId : public BBLocalRequest
{
  public:
    /**
     * \brief Constructor
     */
    BBCleanUpContribId(std::string pConnectionName, LVKey pLVKey, BBTagID pTagId, uint64_t pHandle, uint32_t pContribId) :
        BBLocalRequest("BBCleanUpContribId", MEDIUM_HIGH),
        connection_name(pConnectionName),
        lvkey(pLVKey),
        tagid(pTagId),
        handle(pHandle),
        contribid(pContribId) {
    };

    /**
     * \brief Destructor
     */
    virtual ~BBCleanUpContribId() { };

    // Static methods
    static int64_t getLastRequestNumberProcessed();

    // Inlined methods

    // Virtual methods
    virtual void doit();
    virtual void dump(const char* pPrefix="");

    // Data members
    std::string     connection_name;
    LVKey           lvkey;
    BBTagID         tagid;
    uint64_t        handle;
    uint32_t        contribid;
};

class BBCleanUpTagInfo : public BBLocalRequest
{
  public:
    /**
     * \brief Constructor
     */
    BBCleanUpTagInfo(std::string pConnectionName, LVKey pLVKey, BBTagID pTagId) :
        BBLocalRequest("BBCleanUpTagInfo", MEDIUM_HIGH),
        connection_name(pConnectionName),
        lvkey(pLVKey),
        tagid(pTagId) {
    };

    /**
     * \brief Destructor
     */
    virtual ~BBCleanUpTagInfo() { };

    // Inlined virtual methods
//    inline virtual int dumpOnAdd() { return 1; };
//    inline virtual int dumpOnDelete() { return 1; };
//    inline virtual int dumpOnRemove() { return 1; };

    // Static methods
    static int64_t getLastRequestNumberProcessed();

    // Inlined methods

    // Virtual methods
    virtual void doit();
    virtual void dump(const char* pPrefix="");

    // Data members
    std::string     connection_name;
    LVKey           lvkey;
    BBTagID         tagid;
};

class BBCounters : public BBLocalRequest
{
  public:
    /**
     * \brief Constructor
     */
    BBCounters() :
        BBLocalRequest("BBCounters", MEDIUM) {
    };

    /**
     * \brief Destructor
     */
    virtual ~BBCounters() { };

    // Inlined virtual methods
//    inline virtual int dumpOnAdd() { return 1; };
//    inline virtual int dumpOnDelete() { return 1; };
//    inline virtual int dumpOnRemove() { return 1; };

    // Static methods
    static int64_t getLastRequestNumberProcessed();

    // Inlined methods

    // Virtual methods
    virtual void doit();

    // Data members
};

class BBDumpHeartbeatData : public BBLocalRequest
{
  public:
    /**
     * \brief Constructor
     */
    BBDumpHeartbeatData() :
        BBLocalRequest("BBDumpHeartbeatData", MEDIUM) {
    };

    /**
     * \brief Destructor
     */
    virtual ~BBDumpHeartbeatData() { };

    // Inlined virtual methods
//    inline virtual int dumpOnAdd() { return 1; };
//    inline virtual int dumpOnDelete() { return 1; };
//    inline virtual int dumpOnRemove() { return 1; };

    // Static methods
    static int64_t getLastRequestNumberProcessed();

    // Inlined methods

    // Virtual methods
    virtual void doit();

    // Data members
};

class BBDumpLocalAsync : public BBLocalRequest
{
  public:
    /**
     * \brief Constructor
     */
    BBDumpLocalAsync() :
        BBLocalRequest("BBDumpLocalAsync", MEDIUM) {
    };

    /**
     * \brief Destructor
     */
    virtual ~BBDumpLocalAsync() { };

    // Inlined virtual methods
//    inline virtual int dumpOnAdd() { return 1; };
//    inline virtual int dumpOnDelete() { return 1; };
//    inline virtual int dumpOnRemove() { return 1; };

    // Static methods
    static int64_t getLastRequestNumberProcessed();

    // Inlined methods

    // Virtual methods
    virtual void doit();

    // Data members
};

class BBDumpWrkQMgr : public BBLocalRequest
{
  public:
    /**
     * \brief Constructor
     */
    BBDumpWrkQMgr() :
        BBLocalRequest("BBDumpWrkQMgr", MEDIUM) {
    };

    /**
     * \brief Destructor
     */
    virtual ~BBDumpWrkQMgr() { };

    // Inlined virtual methods
//    inline virtual int dumpOnAdd() { return 1; };
//    inline virtual int dumpOnDelete() { return 1; };
//    inline virtual int dumpOnRemove() { return 1; };

    // Static methods
    static int64_t getLastRequestNumberProcessed();

    // Inlined methods

    // Virtual methods
    virtual void doit();

    // Data members
};

class BBIB_Stats : public BBLocalRequest
{
  public:
    /**
     * \brief Constructor
     */
    BBIB_Stats() :
        BBLocalRequest("BBIB_Stats", MEDIUM) {
    };

    /**
     * \brief Destructor
     */
    virtual ~BBIB_Stats() { };

    // Inlined virtual methods
//    inline virtual int dumpOnAdd() { return 1; };
//    inline virtual int dumpOnDelete() { return 1; };
//    inline virtual int dumpOnRemove() { return 1; };

    // Static methods
    static int64_t getLastRequestNumberProcessed();

    // Inlined methods

    // Virtual methods
    virtual void doit();

    // Data members
};

class BBIO_Stats : public BBLocalRequest
{
  public:
    /**
     * \brief Constructor
     */
    BBIO_Stats() :
        BBLocalRequest("BBIO_Stats", MEDIUM) {
    };

    /**
     * \brief Destructor
     */
    virtual ~BBIO_Stats() { };

    // Inlined virtual methods
//    inline virtual int dumpOnAdd() { return 1; };
//    inline virtual int dumpOnDelete() { return 1; };
//    inline virtual int dumpOnRemove() { return 1; };

    // Static methods
    static int64_t getLastRequestNumberProcessed();

    // Inlined methods

    // Virtual methods
    virtual void doit();

    // Data members
};

class BBLogIt : public BBLocalRequest
{
  public:
    /**
     * \brief Constructor
     */
    BBLogIt(std::string pData) :
        BBLocalRequest("BBLogIt", HIGH),
        data(pData) {
    };

    /**
     * \brief Destructor
     */
    virtual ~BBLogIt() { };

    // Inlined virtual methods
//    inline virtual int dumpOnAdd() { return 1; };
//    inline virtual int dumpOnDelete() { return 1; };
//    inline virtual int dumpOnRemove() { return 1; };

    // Static methods
    static int64_t getLastRequestNumberProcessed();

    // Inlined methods

    // Virtual methods
    virtual void doit();
    virtual void dump(const char* pPrefix="");

    // Data members
    std::string data;
};

class BBPruneMetadata : public BBLocalRequest
{
  public:
    /**
     * \brief Constructor
     */
    BBPruneMetadata(std::string pPath) :
        BBLocalRequest("BBPruneMetadata", LOW),
        path(pPath) {
    };

    /**
     * \brief Destructor
     */
    virtual ~BBPruneMetadata() { };

    // Inlined virtual methods
//    inline virtual int dumpOnAdd() { return 1; };
//    inline virtual int dumpOnDelete() { return 1; };
//    inline virtual int dumpOnRemove() { return 1; };

    // Static methods
    static int64_t getLastRequestNumberProcessed();

    // Inlined methods

    // Virtual methods
    virtual void doit();
    virtual void dump(const char* pPrefix="");

    // Data members
    std::string path;
};

class BBPruneMetadataBranch : public BBLocalRequest
{
  public:
    /**
     * \brief Constructor
     */
    BBPruneMetadataBranch(std::string pPath) :
        BBLocalRequest("BBPruneMetadataBranch", MEDIUM_LOW),
        path(pPath) {
    };

    /**
     * \brief Destructor
     */
    virtual ~BBPruneMetadataBranch() { };

    // Inlined virtual methods
//    inline virtual int dumpOnAdd() { return 1; };
//    inline virtual int dumpOnDelete() { return 1; };
//    inline virtual int dumpOnRemove() { return 1; };

    // Static methods
    static int64_t getLastRequestNumberProcessed();

    // Inlined methods

    // Virtual methods
    virtual void doit();
    virtual void dump(const char* pPrefix="");

    // Data members
    std::string path;
};


/*
 * BBLocalAsync class
 */
class BBLocalAsync
{
  public:
    /**
     * \brief Constructor
     */
    BBLocalAsync() :
        mutex(PTHREAD_MUTEX_INITIALIZER) {
    };

    /**
     * \brief Destructor
     */
    virtual ~BBLocalAsync() {};

    // Static data
    vector<BBAsyncRequestType> requestType = {
        BBAsyncRequestType(string("HIGH"), HIGH, (double)0),    // With 48 async threads, leaves 2 threads
        BBAsyncRequestType(string("MEDIUM_HIGH"), MEDIUM_HIGH, (double)18/(double)48),
        BBAsyncRequestType(string("MEDIUM"), MEDIUM, (double)8/(double)48),
        BBAsyncRequestType(string("MEDIUM_LOW"), MEDIUM_LOW, (double)16/(double)48),
        BBAsyncRequestType(string("LOW"), LOW, (double)2/(double)48),
        BBAsyncRequestType(string("VERY_LOW"), VERY_LOW, (double)2/(double)48)
    };

    // Inlined static methods

    // Inlined non-static methods
    inline void lock()
    {
        pthread_mutex_lock(&mutex);

        return;
    };

    inline void unlock()
    {
        pthread_mutex_unlock(&mutex);

        return;
    };

  public:
    // Static methods
    static void* asyncRequestWorker(void* ptr);

    // Non-static methods
    int dispatchFromThisQueue(LOCAL_ASYNC_REQUEST_PRIORITY pPriority);
    void dump(const char* pPrefix="");
    int64_t getLastRequestNumberProcessed(BBLocalRequest* pRequest);
    int64_t getNextRequest(BBLocalRequest* &pRequest);
    int init();
    int64_t issueAsyncRequest(BBLocalRequest* pRequest);
    void recordRequestCompletion(int64_t pRequestNumber, BBLocalRequest* pRequest);

  private:
    // Data members
    pthread_mutex_t             mutex;
    sem_t                       work;
    map<LOCAL_ASYNC_REQUEST_PRIORITY, BBAsyncRequestData*>   requestData;
};

#endif /* BB_BBLOCALASYNC_H_ */
