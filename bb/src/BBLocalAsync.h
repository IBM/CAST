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

#include <map>
#include <queue>
#include <string>

#include <semaphore.h>

#include "usage.h"


/*******************************************************************************
 | Forward declarations
 *******************************************************************************/
class BBLocalRequest;


/*******************************************************************************
 | Constants
 *******************************************************************************/


/*******************************************************************************
 | Enumerators
 *******************************************************************************/
enum LOCAL_ASYNC_REQUEST_PRIORITY
{
    NONE   = 0,
    HIGH   = 10,
    MEDIUM = 50,
    LOW    = 90
};
typedef enum LOCAL_ASYNC_REQUEST_PRIORITY LOCAL_ASYNC_REQUEST_PRIORITY;

enum LOCAL_ASYNC_REQUEST_LOG_RESULT
{
    DO_NOT_LOG_RESULT   = 0,
    LOG_RESULT          = 1
};
typedef enum LOCAL_ASYNC_REQUEST_LOG_RESULT LOCAL_ASYNC_REQUEST_LOG_RESULT;


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

    // Static data

    // Inlined static methods

    // Inlined non-static methods
    inline LOCAL_ASYNC_REQUEST_PRIORITY getPriority()
    {
        return priority;
    }

    // Inlined virtual methods
    inline virtual void doit() { return; };

    // Static methods
    static std::string getPriorityStr(LOCAL_ASYNC_REQUEST_PRIORITY pPriority);

    // Virtual methods
    virtual void dump(const char* pPrefix="");

    // Non-virtual methods
    void dumpRequest(stringstream& pStream);

    // Data members
    std::string                  name;
    LOCAL_ASYNC_REQUEST_PRIORITY priority;
};

//
// Classes derived from BBLocalRequest
//
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
        BBLocalRequest("BBPruneMetadataBranch", MEDIUM),
        path(pPath) {
    };

    /**
     * \brief Destructor
     */
    virtual ~BBPruneMetadataBranch() { };

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

    // Inlined static methods

    // Inlined non-static methods
  private:
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

  public:
    // Static methods
    static void* asyncRequestWorker(void* ptr);

    // Non-static methods
    int dispatchFromThisQueue(LOCAL_ASYNC_REQUEST_PRIORITY pPriority);
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
