/*******************************************************************************
 |    BBLocalAsync.cc
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

#include "time.h"

#include "bbcounters.h"
#include "bbinternal.h"
#include "BBLV_Info.h"
#include "BBLV_Metadata.h"
#include "bbserver_flightlog.h"
#include "BBTagInfoMap.h"
#include "bbwrkqmgr.h"
#include "tracksyscall.h"
#include "usage.h"
#include "util.h"
#include "xfer.h"

#include <boost/filesystem.hpp>

using namespace std;
using namespace boost;
namespace bfs = boost::filesystem;


/*
 * Global data
 */
map<std::string, std::string> diskStatCache;

unsigned long bbcounters_shadow[BB_COUNTER_MAX];

atomic<int64_t> g_Last_Port_Rcv_Data_Delta(-1);
atomic<int64_t> g_Last_Port_Xmit_Data_Delta(-1);
string g_IB_NVMe_Adapter = "mlx5_0";

AsyncRemoveJobInfo_Controller g_AsyncRemoveJobInfo_Controller = AsyncRemoveJobInfo_Controller();
CycleActivities_Controller g_CycleActivities_Controller = CycleActivities_Controller();
BBIB_Stats_Controller g_BBIB_Stats_Controller = BBIB_Stats_Controller();
BBIO_Stats_Controller g_BBIO_Stats_Controller = BBIO_Stats_Controller();
Dump_Counters_Controller g_Dump_Counters_Controller = Dump_Counters_Controller();
Dump_Heartbeat_Data_Controller g_Dump_Heartbeat_Data_Controller = Dump_Heartbeat_Data_Controller();
Dump_Local_Async_Controller g_Dump_Local_Async_Controller = Dump_Local_Async_Controller();
Dump_WrkQMgr_Controller g_Dump_WrkQMgr_Controller = Dump_WrkQMgr_Controller();
Heartbeat_Controller g_Heartbeat_Controller = Heartbeat_Controller();
RemoteAsyncRequest_Controller g_RemoteAsyncRequest_Controller = RemoteAsyncRequest_Controller();
RemoveAsyncRequestFile_Controller g_RemoveAsyncRequestFile_Controller = RemoveAsyncRequestFile_Controller();
SwapAsyncRequestFile_Controller g_SwapAsyncRequestFile_Controller = SwapAsyncRequestFile_Controller();
ThrottleBucket_Controller g_ThrottleBucket_Controller = ThrottleBucket_Controller();

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t log_mutex_locked = 0;


/*
 * Helper methods
 */
std::string getHeartbeatCurrentTimeStr()
{
    struct timeval l_CurrentTime = timeval {.tv_sec=0, .tv_usec=0};
    gettimeofday(&l_CurrentTime, NULL);

    return timevalToStr(l_CurrentTime);
}

std::string getLocalAsyncPriorityStr(LOCAL_ASYNC_REQUEST_PRIORITY pPriority)
{
    std::string l_Priority;
    switch (pPriority)
    {
        case NONE:
            l_Priority = "NONE";
            break;

        case HIGH:
            l_Priority = "HIGH";
            break;

        case MEDIUM_HIGH:
            l_Priority = "MEDIUM_HIGH";
            break;

        case MEDIUM:
            l_Priority = "MEDIUM";
            break;

        case MEDIUM_LOW:
            l_Priority = "MEDIUM_LOW";
            break;

        case LOW:
            l_Priority = "LOW";
            break;

        case VERY_LOW:
            l_Priority = "VERY_LOW";
            break;

        default:
            l_Priority = "Unknown";
            break;
    }

    return l_Priority;
}

int highIB_Activity()
{
    int rc = 0;

    if (g_Last_Port_Rcv_Data_Delta > 0)
    {
        if (g_Last_Port_Rcv_Data_Delta < g_IBStatsLowActivityClipValue)
        {
            if (g_Last_Port_Xmit_Data_Delta > 0)
            {
                if (g_Last_Port_Xmit_Data_Delta >= g_IBStatsLowActivityClipValue)
                {
                    rc = 1;
                }
            }
            else
            {
                // Undetermined IB activity
                rc = -1;
            }
        }
        else
        {
            // High IB activity
            rc = 1;
        }
    }
    else
    {
        // Undetermined IB activity
        rc = -1;
    }

    return rc;
}


/*
 * BBAsyncRequestData class methods
 */
int64_t BBAsyncRequestData::addRequest(BBLocalRequest* pRequest)
{
    int64_t rc = -1;

    lock();
    try
    {
        requests.push(pRequest);
        rc = increment(lastRequestNumberIssued);
        if (pRequest->dumpOnAdd())
        {
            pRequest->dump("BBLocalAsync::addRequest(): Enqueuing: ");
        }
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }
    unlock();

    return rc;
}

void BBAsyncRequestData::dump(const char* pPrefix)
{
    lock();
    LOG(bb,info) << "  Priority " << setw(11) << getLocalAsyncPriorityStr(priority) << ": #Iss'd " << lastRequestNumberIssued \
                 << ", #Disptch'd " << lastRequestNumberDispatched << ", #Proc'd " << lastRequestNumberProcessed \
                 << ", #MaxConcur " << maximumConcurrentRunning << " #Inflt " << (lastRequestNumberDispatched - lastRequestNumberProcessed) - size(outOfSequenceRequests) \
                 << ", #Wait " << lastRequestNumberIssued - lastRequestNumberDispatched << ", #_OutOfSeq " << size(outOfSequenceRequests);
    unlock();

    return;
}

int64_t BBAsyncRequestData::getNumberOfInFlightRequests()
{
    int64_t l_NumberOfRequests = 0;

    lock();
    l_NumberOfRequests = (lastRequestNumberDispatched - lastRequestNumberProcessed) - size(outOfSequenceRequests);
    unlock();

    return l_NumberOfRequests;
}

int64_t BBAsyncRequestData::getNumberOfOutOfSequenceRequests()
{
    int64_t l_NumberOfRequests = 0;

    lock();
    l_NumberOfRequests = size(outOfSequenceRequests);
    unlock();

    return l_NumberOfRequests;
}

int64_t BBAsyncRequestData::getNumberOfWaitingRequests()
{
    int64_t l_NumberOfRequests = 0;

    lock();
    l_NumberOfRequests = lastRequestNumberIssued - lastRequestNumberDispatched;
    unlock();

    return l_NumberOfRequests;
}

int64_t BBAsyncRequestData::increment(int64_t& pNumber)
{
    do
    {
        ++pNumber;
    } while (!pNumber);

    return pNumber;
}

size_t BBAsyncRequestData::numberOfNonDispatchedRequests()
{
    size_t l_NumberOfRequests = 0;

    lock();
    l_NumberOfRequests = requests.size();
    unlock();

    return l_NumberOfRequests;
}

void BBAsyncRequestData::recordRequestCompletion(int64_t pRequestNumber)
{
    lock();
    if (lastRequestNumberProcessed + 1 == pRequestNumber)
    {
        ++lastRequestNumberProcessed;
        if (outOfSequenceRequests.size())
        {
            bool l_AllDone = false;
            while (!l_AllDone)
            {
                l_AllDone = true;
                vector<int64_t>::iterator it;
                for (it = outOfSequenceRequests.begin(); it != outOfSequenceRequests.end(); ++it)
                {
                    if (lastRequestNumberProcessed + 1 == *it)
                    {
                        ++lastRequestNumberProcessed;
                        outOfSequenceRequests.erase(it);
                        l_AllDone = false;
                        LOG(bb,debug) << "BBLocalAsync::recordRequestCompletion(): Priority " << getLocalAsyncPriorityStr(priority) \
                                      << ", removed request number " << *it << " from outOfSequenceRequests";
                        break;
                    }
                }
            }
        }
    }
    else
    {
        outOfSequenceRequests.push_back(pRequestNumber);
        LOG(bb,debug) << "BBLocalAsync::recordRequestCompletion(): Priority " << getLocalAsyncPriorityStr(priority) \
                      << ", pushed request number " << pRequestNumber << " onto outOfSequenceRequests";
    }

    LOG(bb,debug) << "BBLocalAsync::recordRequestCompletion(): Priority " << getLocalAsyncPriorityStr(priority) \
                  << ", completed request, lastRequestNumberProcessed " << lastRequestNumberProcessed;

    unlock();

    return;
}

int64_t BBAsyncRequestData::removeNextRequest(BBLocalRequest* &pRequest)
{
    int64_t rc = -1;

    lock();
    try
    {
        pRequest = requests.front();
        if (pRequest)
        {
            requests.pop();
            rc = increment(lastRequestNumberDispatched);
            if (pRequest->dumpOnRemove())
            {
                pRequest->dump("BBLocalAsync::removeNextRequest(): Dequeuing: ");
            }
        }
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }
    unlock();

    return rc;
}


/*
 * BBLocalRequest class methods
 */
void BBLocalRequest::dumpRequest(stringstream& pStream)
{
    pStream << "Name " << name << ", priority " << getLocalAsyncPriorityStr(priority);

    return;
}

void BBLocalRequest::end_logging()
{
    if (log_mutex_locked)
    {
        log_mutex_locked = 0;
        pthread_mutex_unlock(&log_mutex);
    }

    return;
};

void BBLocalRequest::start_logging()
{
    pthread_mutex_lock(&log_mutex);
    log_mutex_locked = pthread_self();

    return;
};


/*
 * BBLocalAsync class methods
 */
void* BBLocalAsync::asyncRequestWorker(void* ptr)
{
    BBLocalRequest* l_Request;

    while (1)
    {
        l_Request = 0;
        int64_t l_RequestNumber = 0;

        try
        {
            verifyInitLockState();

            l_RequestNumber = g_LocalAsync.getNextRequest(l_Request);
            if (l_RequestNumber > 0 && l_Request)
            {
                l_Request->doit();
            }
        }
        catch (ExceptionBailout& e) { }
        catch (std::exception& e)
        {
            LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
        }

        if (l_Request)
        {
            try
            {
                g_LocalAsync.recordRequestCompletion(l_RequestNumber, l_Request);
            }
            catch (std::exception& e)
            {
                // Tolerate everything
            }
            try
            {
                if (l_Request->dumpOnDelete())
                {
                    l_Request->dump("BBLocalAsync::asyncRequestWorker(): Deleting: ");
                }
            }
            catch (std::exception& e)
            {
                // Tolerate everything
            }
            delete l_Request;
            l_Request = 0;
        }
    }

    return NULL;
}

int BBLocalAsync::dispatchFromThisQueue(LOCAL_ASYNC_REQUEST_PRIORITY pPriority)
{
    int rc = 0;

    int64_t l_NumberOfNonDispatchedRequests = requestData[pPriority]->numberOfNonDispatchedRequests();
    int64_t l_MaximumConcurrentRunning = 0;
    int64_t l_NumberOfInFlightRequests = 0;

    if (l_NumberOfNonDispatchedRequests)
    {
        l_MaximumConcurrentRunning = (int64_t)requestData[pPriority]->getMaximumConcurrentRunning();
        if (l_MaximumConcurrentRunning)
        {
            l_NumberOfInFlightRequests = (int64_t)requestData[pPriority]->getNumberOfInFlightRequests();
            if (l_NumberOfInFlightRequests < l_MaximumConcurrentRunning)
            {
                rc = 1;
            }
        }
        else
        {
            rc = 1;
        }
    }

    if (rc == 1)
    {
        LOG(bb,debug) << "BBLocalAsync::dispatchFromThisQueue(): ==> Scheduling async request having priority " << getLocalAsyncPriorityStr(pPriority) \
                      << ", NumberOfNonDispatchedRequests " << l_NumberOfNonDispatchedRequests \
                      << ", NumberOfInFlightRequests " << l_NumberOfInFlightRequests \
                      << ", MaximumConcurrentRunning " << l_MaximumConcurrentRunning;
    }

    return rc;
}

void BBLocalAsync::dump(const char* pPrefix)
{
    lock();
    try
    {
        LOG(bb,info) << ">>>>> Start: Local Async Mgr <<<<<";

        for (map<LOCAL_ASYNC_REQUEST_PRIORITY, BBAsyncRequestData*>::iterator rd = requestData.begin(); rd != requestData.end(); ++rd)
        {
            rd->second->dump();
        }

        LOG(bb,info) << ">>>>>   End: Local Async Mgr <<<<<";
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }
    unlock();

    return;
}

int64_t BBLocalAsync::getLastRequestNumberProcessed(BBLocalRequest* pRequest)
{
    int64_t l_RequestNumber = -1;

    if (pRequest)
    {
        l_RequestNumber = requestData[pRequest->getPriority()]->getLastRequestNumberProcessed();
    }

    return l_RequestNumber;
}

int64_t BBLocalAsync::getNextRequest(BBLocalRequest* &pRequest)
{
    int64_t rc = -1;

    sem_wait(&work);

    lock();
    try
    {
        pRequest = (BBLocalRequest*)0;
        LOCAL_ASYNC_REQUEST_PRIORITY l_Priority = NONE;
        vector<BBAsyncRequestType>::iterator it;
        for (it = requestType.begin(); it != requestType.end(); ++it)
        {
            if (dispatchFromThisQueue(it->priority))
            {
                l_Priority = it->priority;
                break;
            }
        }

        if (l_Priority != NONE)
        {
            rc = requestData[l_Priority]->removeNextRequest(pRequest);
        }

        if (rc < 0 || l_Priority == NONE)
        {
            // Cannot dispatch new work...  Delay 250 milliseconds and retry...
            LOG(bb,debug) << "BBLocalAsync::getNextRequest(): Cannot dispatch new work...";
            unlock();
            usleep(250000);
            lock();
            sem_post(&work);
        }
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }
    unlock();

    return rc;
}

int BBLocalAsync::init()
{
    int rc = 0;
    stringstream errorText;

    try
    {
        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        LOG(bb,info) << "Starting " << g_NumberOfAsyncRequestsThreads << " async request threads";

        for (unsigned int i=0; i<g_NumberOfAsyncRequestsThreads; i++)
        {
            rc = pthread_create(&tid, &attr, BBLocalAsync::asyncRequestWorker, NULL);
            if (rc)
            {
                errorText << "Error occurred in BBLocalAsync::init()";
                bberror << err("error.numthreads", g_NumberOfAsyncRequestsThreads);
                LOG_ERROR_TEXT_RC_RAS_AND_BAIL(errorText, rc, bb.lar.pthread_create);
            }
        }

        // Setup the request data for each supported priority...
        vector<BBAsyncRequestType>::iterator it;
        for (it = requestType.begin(); it != requestType.end(); ++it)
        {
            requestData[it->priority] = new BBAsyncRequestData(it->priority, round(g_NumberOfAsyncRequestsThreads*(it->percentage_of_threads)));
        }

        // Initialize the controller classes
        // Basic check to perform other activities
        g_CycleActivities_Controller.init(Throttle_TimeInterval);

        // Work queue throttling
        g_ThrottleBucket_Controller.init(Throttle_TimeInterval);

        // Remote async requests and heartbeats/heartbeat data dumps
        g_RemoteAsyncRequest_Controller.init(Throttle_TimeInterval);
        g_Heartbeat_Controller.init(Throttle_TimeInterval);
        g_Dump_Heartbeat_Data_Controller.init(Throttle_TimeInterval);

        // Local async request dumps and work queue manager dumps
        g_Dump_Local_Async_Controller.init(Throttle_TimeInterval);
        g_Dump_WrkQMgr_Controller.init(Throttle_TimeInterval);

        // IBSTAT and IOSTAT dumps, counter dumps
        g_BBIB_Stats_Controller.init(Throttle_TimeInterval);
        g_BBIO_Stats_Controller.init(Throttle_TimeInterval);
        g_Dump_Counters_Controller.init(Throttle_TimeInterval);

        // Async remove job information
        g_AsyncRemoveJobInfo_Controller.init(Throttle_TimeInterval);

        // Async request file maintenance
        g_SwapAsyncRequestFile_Controller.init(Throttle_TimeInterval);
        g_RemoveAsyncRequestFile_Controller.init(Throttle_TimeInterval);
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    return rc;
}

// NOTE pRequest must be allocated out of heap.
int64_t BBLocalAsync::issueAsyncRequest(BBLocalRequest* pRequest)
{
    int64_t rc = -1;

    lock();
    try
    {
        // NOTE: Once the request has been added, the BBLocalAsync object
        //       owns the heap that was allocated for the request.
        //       BBLocalAsync will delete this space when the request is complete.
        rc = requestData[pRequest->getPriority()]->addRequest(pRequest);
        if (rc > 0)
        {
            sem_post(&work);
        }
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }
    unlock();

    return rc;
}

void BBLocalAsync::recordRequestCompletion(int64_t l_RequestNumber, BBLocalRequest* l_Request)
{
    requestData[l_Request->getPriority()]->recordRequestCompletion(l_RequestNumber);
    LOG(bb,debug) << "BBLocalAsync::recordRequestCompletion(): Completed local async request having priority " \
                  << getLocalAsyncPriorityStr(l_Request->getPriority()) \
                  << ", NumberOfNonDispatchedRequests " << requestData[l_Request->getPriority()]->numberOfNonDispatchedRequests() \
                  << ", NumberOfInFlightRequests " << requestData[l_Request->getPriority()]->getNumberOfInFlightRequests() \
                  << ", MaximumConcurrentRunning " << requestData[l_Request->getPriority()]->getMaximumConcurrentRunning();

    return;
}


/*
 * BBLocalRequest class methods
 */
void BBLocalRequest::doitEnd(BBController* pController)
{
    // NOTE: Within normal error processing, whatever is
    //       placed in this method must complete without
    //       throwing an exception
    pController->setTimerFired(0);
    pController->setCount(0);

    return;
}

void BBLocalRequest::doitStart(BBController* pController)
{
    // NOTE: Within normal error processing, whatever is
    //       placed in this method must complete without
    //       throwing an exception
    // NOTE: It is only when the fired indicator is turned
    //       on by the requestor, do we know we can proceed
    //       with processing this request.
    while (!(pController->fired))
    {
        usleep(250000);
    }

    return;
}


/*
 * Classes derived from BBController checkTimeToPerform methods
 */
void AsyncRemoveJobInfo_Controller::checkTimeToPerform()
{
    if (timeToFire())
    {
        BBAsyncRemoveJobInfo* l_Request = new BBAsyncRemoveJobInfo();
        g_LocalAsync.issueAsyncRequest(l_Request);
        setTimerFired(1);
    }

    return;
}

void BBIB_Stats_Controller::checkTimeToPerform()
{
    if (timeToFire())
    {
        BBIB_Stats* l_Request = new BBIB_Stats();
        g_LocalAsync.issueAsyncRequest(l_Request);
        setTimerFired(1);
    }

    return;
}

void BBIO_Stats_Controller::checkTimeToPerform()
{
    if (timeToFire())
    {
        BBIO_Stats* l_Request = new BBIO_Stats();
        g_LocalAsync.issueAsyncRequest(l_Request);
        setTimerFired(1);
    }

    return;
}

void Dump_Counters_Controller::checkTimeToPerform()
{
    if (timeToFire())
    {
        BBCounters* l_Request = new BBCounters();
        g_LocalAsync.issueAsyncRequest(l_Request);
        setTimerFired(1);
    }

    return;
}

void Dump_Heartbeat_Data_Controller::checkTimeToPerform()
{
    if (timeToFire())
    {
        BBDumpHeartbeatData* l_Request = new BBDumpHeartbeatData();
        g_LocalAsync.issueAsyncRequest(l_Request);
        setTimerFired(1);
    }

    return;
}

void Dump_Local_Async_Controller::checkTimeToPerform()
{
    if (timeToFire())
    {
        BBDumpLocalAsync* l_Request = new BBDumpLocalAsync();
        g_LocalAsync.issueAsyncRequest(l_Request);
        setTimerFired(1);
    }

    return;
}

void Dump_WrkQMgr_Controller::checkTimeToPerform()
{
    if (timeToFire())
    {
        BBDumpWrkQMgr* l_Request = new BBDumpWrkQMgr();
        g_LocalAsync.issueAsyncRequest(l_Request);
        setTimerFired(1);
    }

    return;
}

void Heartbeat_Controller::checkTimeToPerform()
{
    if (timeToFire())
    {
        // Tell the world this bbServer is still alive...
        char l_AsyncCmd[AsyncRequest::MAX_DATA_LENGTH] = {'\0'};
        string l_CurrentTime = getHeartbeatCurrentTimeStr();
        snprintf(l_AsyncCmd, sizeof(l_AsyncCmd), "heartbeat 0 0 0 0 0 None %s", l_CurrentTime.c_str());
        AsyncRequest l_Request = AsyncRequest(l_AsyncCmd);
        wrkqmgr.appendAsyncRequest(l_Request);
    }

    return;
}

void RemoveAsyncRequestFile_Controller::checkTimeToPerform()
{
    if (timeToFire())
    {
        BBRemoveAsyncRequestFile* l_Request = new BBRemoveAsyncRequestFile();
        g_LocalAsync.issueAsyncRequest(l_Request);
        setTimerFired(1);
    }

    return;
}

void SwapAsyncRequestFile_Controller::checkTimeToPerform()
{
    if (timeToFire())
    {
        BBSwapAsyncRequestFile* l_Request = new BBSwapAsyncRequestFile();
        g_LocalAsync.issueAsyncRequest(l_Request);
        setTimerFired(1);
    }

    return;
}


/*
 * Classes derived from BBController getTimerCount methods
 */
uint64_t RemoteAsyncRequest_Controller::getTimerPoppedCount()
{
    return (uint64_t)((double)poppedCount * wrkqmgr.getAsyncRequestReadTurboFactor());
};


/*
 * Classes derived from BBController timeToFire methods
 */
int AsyncRemoveJobInfo_Controller::timeToFire()
{
    return (g_AsyncRemoveJobInfo ? BBController::timeToFire() : 0);
};


/*
 * Classes derived from BBController init methods
 */
void AsyncRemoveJobInfo_Controller::init(const double pTimerInterval)
{
    if (g_AsyncRemoveJobInfo)
    {
        double l_AsyncRemoveJobInfoInterval = max(config.get("bb.bbserverAsyncRemoveJobInfoInterval", DEFAULT_ASYNC_REMOVEJOBINFO_INTERVAL_VALUE), DEFAULT_ASYNC_REMOVEJOBINFO_MINIMUM_INTERVAL_VALUE);
//        l_AsyncRemoveJobInfoInterval = 60;
        poppedCount = (uint64_t)(l_AsyncRemoveJobInfoInterval/pTimerInterval);
        if (((double)poppedCount)*pTimerInterval != (l_AsyncRemoveJobInfoInterval))
        {
            if (poppedCount < 1)
            {
                LOG(bb,warning) << "Async rmvjobinfo timer interval of " << to_string(l_AsyncRemoveJobInfoInterval) << " second(s) is not a common multiple of " << pTimerInterval << " second(s).  Any async rmvjobinfo rates may be implemented as slightly less than what is specified.";
            }
            else
            {
                LOG(bb,warning) << "Async rmvjobinfo timer interval of " << to_string(l_AsyncRemoveJobInfoInterval) << " second(s) is not a common multiple of " << pTimerInterval << " second(s).  Any async rmvjobinfo rates may be implemented as slightly more than what is specified.";
            }
            ++poppedCount;
        }
        LOG(bb,always) << "Timer interval is set to " << pTimerInterval << " second(s) with a multiplier of " << poppedCount << " to implement async rmvjobinfo " \
                       << pTimerInterval*poppedCount << " second intervals. Job information will be removed in groups of " << g_AsyncRemoveJobInfoNumberPerGroup << ".";
    }
    else
    {
        LOG(bb,always) << "Removal of bbServer job information metadata will be done synchronous with the bbcmd_removejobinfo and/or BB_RemoveJobInfo() API.";
    }

    return;
}

void BBIB_Stats_Controller::init(const double pTimerInterval)
{
    double l_IB_StatsTimeInterval = DEFAULT_BBSERVER_IBSTATS_TIME_INTERVAL;
    poppedCount = (uint64_t)(l_IB_StatsTimeInterval/pTimerInterval);
    if (((double)poppedCount)*pTimerInterval != (double)(l_IB_StatsTimeInterval))
    {
        if (poppedCount < 1)
        {
            LOG(bb,warning) << "IB stats interval of " << to_string(l_IB_StatsTimeInterval) << " second(s) is not a common multiple of " << pTimerInterval << " second(s).  Any IB stats dump counter rates may be implemented as slightly less than what is specified.";
        }
        else
        {
            LOG(bb,warning) << "IB stats interval of " << to_string(l_IB_StatsTimeInterval) << " second(s) is not a common multiple of " << pTimerInterval << " second(s).  Any IB stats dump counter rates may be implemented as slightly more than what is specified.";
        }
        ++poppedCount;
    }

    // NOTE: Pop this 'event' on next pass so we can determine the rcv/xmit delta values quicker
    count = poppedCount;

    LOG(bb,always) << "Timer interval is set to " << pTimerInterval << " second(s) with a multiplier of " << poppedCount << " to implement an IB stats dump rate with " \
                   << pTimerInterval*poppedCount << " second intervals.";

    return;
}

void BBIO_Stats_Controller::init(const double pTimerInterval)
{
    double l_IO_StatsTimeInterval = DEFAULT_BBSERVER_IOSTATS_TIME_INTERVAL;
    poppedCount = (uint64_t)(l_IO_StatsTimeInterval/pTimerInterval);
    if (((double)poppedCount)*pTimerInterval != (double)(l_IO_StatsTimeInterval))
    {
        if (poppedCount < 1)
        {
            LOG(bb,warning) << "IO stats interval of " << to_string(l_IO_StatsTimeInterval) << " second(s) is not a common multiple of " << pTimerInterval << " second(s).  Any IO stats dump counter rates may be implemented as slightly less than what is specified.";
        }
        else
        {
            LOG(bb,warning) << "IO stats interval of " << to_string(l_IO_StatsTimeInterval) << " second(s) is not a common multiple of " << pTimerInterval << " second(s).  Any IO stats dump counter rates may be implemented as slightly more than what is specified.";
        }
        ++poppedCount;
    }

    LOG(bb,always) << "Timer interval is set to " << pTimerInterval << " second(s) with a multiplier of " << poppedCount << " to implement an IO stats dump rate with " \
                   << pTimerInterval*poppedCount << " second intervals.";

    return;
}

void Dump_Counters_Controller::init(const double pTimerInterval)
{
    double l_DumpCountersTimeInterval = DEFAULT_BBSERVER_DUMP_COUNTERS_TIME_INTERVAL;
    poppedCount = (uint64_t)(l_DumpCountersTimeInterval/pTimerInterval);
    if (((double)poppedCount)*pTimerInterval != (double)(l_DumpCountersTimeInterval))
    {
        if (poppedCount < 1)
        {
            LOG(bb,warning) << "WRKQMGR dump timer interval of " << to_string(l_DumpCountersTimeInterval) << " second(s) is not a common multiple of " << pTimerInterval << " second(s).  Any dump counter rates may be implemented as slightly less than what is specified.";
        }
        else
        {
            LOG(bb,warning) << "WRKQMGR dump timer interval of " << to_string(l_DumpCountersTimeInterval) << " second(s) is not a common multiple of " << pTimerInterval << " second(s).  Any dump counter rates may be implemented as slightly more than what is specified.";
        }
        ++poppedCount;
    }
    LOG(bb,always) << "Timer interval is set to " << pTimerInterval << " second(s) with a multiplier of " << poppedCount << " to implement a counter dump rate with " \
                   << pTimerInterval*poppedCount << " second intervals.";

    return;
}

void Dump_Heartbeat_Data_Controller::init(const double pTimerInterval)
{
    double l_HeartbeatDumpInterval = config.get("bb.bbserverHeartbeat_DumpInterval", DEFAULT_BBSERVER_HEARTBEAT_DUMP_INTERVAL);
    poppedCount = (uint64_t)(l_HeartbeatDumpInterval/pTimerInterval);
    if (((double)poppedCount)*pTimerInterval != (double)(l_HeartbeatDumpInterval))
    {
        if (poppedCount < 1)
        {
            LOG(bb,warning) << "Heartbeat dump timer interval of " << to_string(l_HeartbeatDumpInterval) << " second(s) is not a common multiple of " << pTimerInterval << " second(s).  Any heartbeat dump rates may be implemented as slightly less than what is specified.";
        }
        else
        {
            LOG(bb,warning) << "Heartbeat dump timer interval of " << to_string(l_HeartbeatDumpInterval) << " second(s) is not a common multiple of " << pTimerInterval << " second(s).  Any heartbeat dump rates may be implemented as slightly more than what is specified.";
        }
        ++poppedCount;
    }
    LOG(bb,always) << "Timer interval is set to " << pTimerInterval << " second(s) with a multiplier of " << poppedCount << " to implement a heartbeat dump rate with " \
                   << pTimerInterval*poppedCount << " second intervals.";

    return;
}

void Dump_Local_Async_Controller::init(const double pTimerInterval)
{
    double l_DumpLocalAsyncTimeInterval = config.get("bb.bbserverDumpLocalAsyncMgrTimeInterval", DEFAULT_BBSERVER_DUMP_LOCAL_ASYNC_TIME_INTERVAL);
    poppedCount = (uint64_t)(l_DumpLocalAsyncTimeInterval/pTimerInterval);
    if (((double)poppedCount)*pTimerInterval != (double)(l_DumpLocalAsyncTimeInterval))
    {
        if (poppedCount < 1)
        {
            LOG(bb,warning) << "WRKQMGR dump timer interval of " << to_string(l_DumpLocalAsyncTimeInterval) << " second(s) is not a common multiple of " << pTimerInterval << " second(s).  Any LocalAsync manager dump rates may be implemented as slightly less than what is specified.";
        }
        else
        {
            LOG(bb,warning) << "WRKQMGR dump timer interval of " << to_string(l_DumpLocalAsyncTimeInterval) << " second(s) is not a common multiple of " << pTimerInterval << " second(s).  Any LocalAsync manager dump rates may be implemented as slightly more than what is specified.";
        }
        ++poppedCount;
    }
    LOG(bb,always) << "Timer interval is set to " << pTimerInterval << " second(s) with a multiplier of " << poppedCount << " to implement a local async request manager dump rate with " \
                   << pTimerInterval*poppedCount << " second intervals.";

    return;
}

void Dump_WrkQMgr_Controller::init(const double pTimerInterval)
{
    double l_DumpTimeInterval = config.get("bb.bbserverDumpWorkQueueMgr_TimeInterval", DEFAULT_DUMP_MGR_TIME_INTERVAL);
    poppedCount = (uint64_t)(l_DumpTimeInterval/pTimerInterval);
    if (((double)poppedCount)*pTimerInterval != (double)(l_DumpTimeInterval))
    {
        if (poppedCount < 1)
        {
            LOG(bb,warning) << "WRKQMGR dump timer interval of " << to_string(l_DumpTimeInterval) << " second(s) is not a common multiple of " << pTimerInterval << " second(s).  Any WRKQMGR dump rates may be implemented as slightly less than what is specified.";
        }
        else
        {
            LOG(bb,warning) << "WRKQMGR dump timer interval of " << to_string(l_DumpTimeInterval) << " second(s) is not a common multiple of " << pTimerInterval << " second(s).  Any WRKQMGR dump rates may be implemented as slightly more than what is specified.";
        }
        ++poppedCount;
    }
    LOG(bb,always) << "Timer interval is set to " << pTimerInterval << " second(s) with a multiplier of " << poppedCount << " to implement a work queue manager dump rate with " \
                   << pTimerInterval*poppedCount << " second intervals.";

    return;
}

void Heartbeat_Controller::init(const double pTimerInterval)
{
    double l_HeartbeatTimeInterval = config.get("bb.bbserverHeartbeat_TimeInterval", DEFAULT_BBSERVER_HEARTBEAT_TIME_INTERVAL);
    poppedCount = (uint64_t)(l_HeartbeatTimeInterval/pTimerInterval);
    if (((double)poppedCount)*pTimerInterval != (double)(l_HeartbeatTimeInterval))
    {
        if (poppedCount < 1)
        {
            LOG(bb,warning) << "Heartbeat timer interval of " << to_string(l_HeartbeatTimeInterval) << " second(s) is not a common multiple of " << pTimerInterval << " second(s).  Any heartbeat rates may be implemented as slightly less than what is specified.";
        }
        else
        {
            LOG(bb,warning) << "Heartbeat timer interval of " << to_string(l_HeartbeatTimeInterval) << " second(s) is not a common multiple of " << pTimerInterval << " second(s).  Any heartbeat rates may be implemented as slightly more than what is specified.";
        }
        ++poppedCount;
    }

    // Currently, for a restart transfer operation, we will wait a total of:
    // min( max( bbServer heartbeat interval + 30 seconds, minimum declare server dead value ), maximum declare server dead value )
    // Value(s) stored in seconds.
    wrkqmgr.setDeclareServerDeadCount(min( max( (uint64_t)(l_HeartbeatTimeInterval + 30), MINIMUM_BBSERVER_DECLARE_SERVER_DEAD_VALUE ), MAXIMUM_BBSERVER_DECLARE_SERVER_DEAD_VALUE ));

    LOG(bb,always) << "Timer interval is set to " << pTimerInterval << " second(s) with a multiplier of " << poppedCount << " to implement a heartbeat rate with " \
                   << pTimerInterval*poppedCount << " second intervals.";
    LOG(bb,always) << "For failover situations, declare server dead time set to " << wrkqmgr.getDeclareServerDeadCount() << " seconds.";
    return;
}

void RemoteAsyncRequest_Controller::init(const double pTimerInterval)
{
    AsyncRequestRead_TimeInterval = min(config.get("bb.bbserverAsyncRequestRead_TimeInterval", DEFAULT_BBSERVER_ASYNC_REQUEST_READ_TIME_INTERVAL), MAXIMUM_BBSERVER_ASYNC_REQUEST_READ_TIME_INTERVAL);
    poppedCount = (uint64_t)(AsyncRequestRead_TimeInterval/pTimerInterval);
    if (((double)poppedCount)*pTimerInterval != AsyncRequestRead_TimeInterval)
    {
        if (poppedCount < 1)
        {
            LOG(bb,warning) << "AsyncRequestRead timer interval of " << to_string(AsyncRequestRead_TimeInterval) << " second(s) is not a common multiple of " << pTimerInterval << " second(s).  Any async request read rates may be implemented as slightly less than what is specified.";
        }
        else
        {
            LOG(bb,warning) << "AsyncRequestRead timer interval of " << to_string(AsyncRequestRead_TimeInterval) << " second(s) is not a common multiple of " << pTimerInterval << " second(s).  Any async request read rates may be implemented as slightly more than what is specified.";
        }
        ++poppedCount;
    }
    LOG(bb,always) << "Timer interval is set to " << pTimerInterval << " second(s) with a multiplier of " << poppedCount << " to implement a remote async request read rate with " \
                   << pTimerInterval*poppedCount << " second intervals.";

    return;
}

void RemoveAsyncRequestFile_Controller::init(const double pTimerInterval)
{
    double RemoveAsyncRequestFile_TimeInterval = 900;   // Check every 15 minutes
//    RemoveAsyncRequestFile_TimeInterval = 60;   // Check every minute
    poppedCount = (uint64_t)(RemoveAsyncRequestFile_TimeInterval/pTimerInterval);
    if (((double)poppedCount)*pTimerInterval != RemoveAsyncRequestFile_TimeInterval)
    {
        if (poppedCount < 1)
        {
            LOG(bb,warning) << "RemoveAsyncRequestFile timer interval of " << to_string(RemoveAsyncRequestFile_TimeInterval) << " second(s) is not a common multiple of " << pTimerInterval << " second(s).  Any check for async request file removal rates may be implemented as slightly less than what is specified.";
        }
        else
        {
            LOG(bb,warning) << "AsyncRequestRead timer interval of " << to_string(RemoveAsyncRequestFile_TimeInterval) << " second(s) is not a common multiple of " << pTimerInterval << " second(s).  Any check for async request file removal rates may be implemented as slightly more than what is specified.";
        }
        ++poppedCount;
    }

    LOG(bb,always) << "Timer interval is set to " << pTimerInterval << " second(s) with a multiplier of " << poppedCount << " to implement a check for async request file removal rate with " \
                   << pTimerInterval*poppedCount << " second intervals.";

    LOG(bb,always) << "Async request files will swapped to a new file after growing to a size of " << ASYNC_REQUEST_FILE_SIZE_FOR_SWAP << " bytes.";
    LOG(bb,always) << "After being swapped for a new file, async request files will be considered expired and eligible for removal after " << ASYNC_REQUEST_FILE_PRUNE_TIME << " seconds.";

    return;
}

void SwapAsyncRequestFile_Controller::init(const double pTimerInterval)
{
    double SwapAsyncRequestFile_TimeInterval = 300;   // Check every 5 minutes
//    SwapAsyncRequestFile_TimeInterval = 60;   // Check every minute
    poppedCount = (uint64_t)(SwapAsyncRequestFile_TimeInterval/pTimerInterval);
    if (((double)poppedCount)*pTimerInterval != SwapAsyncRequestFile_TimeInterval)
    {
        if (poppedCount < 1)
        {
            LOG(bb,warning) << "SwapAsyncRequestFile timer interval of " << to_string(SwapAsyncRequestFile_TimeInterval) << " second(s) is not a common multiple of " << pTimerInterval << " second(s).  Any check for swapping of async request file rates may be implemented as slightly less than what is specified.";
        }
        else
        {
            LOG(bb,warning) << "AsyncRequestRead timer interval of " << to_string(SwapAsyncRequestFile_TimeInterval) << " second(s) is not a common multiple of " << pTimerInterval << " second(s).  Any check for swapping of async request file rates may be implemented as slightly more than what is specified.";
        }
        ++poppedCount;
    }

    // NOTE: Pop this 'event' on next pass so we check to see if it is time to swap to a new async request file immediately
    count = poppedCount;

    LOG(bb,always) << "Timer interval is set to " << pTimerInterval << " second(s) with a multiplier of " << poppedCount << " to implement a check for swapping async request file rate with " \
                   << pTimerInterval*poppedCount << " second intervals.";

    return;
}

void ThrottleBucket_Controller::init(const double pTimerInterval)
{
    poppedCount = (uint64_t)(1.0/pTimerInterval);
    if (((double)poppedCount)*pTimerInterval != (double)1.0)
    {
        if (poppedCount < 1)
        {
            LOG(bb,warning) << "Throttle timer interval of " << pTimerInterval << " second(s) is not a common multiple of 1.0 second.  Any throttling rates may be implemented as slightly less than what is specified.";
        }
        else
        {
            LOG(bb,warning) << "Throttle timer interval of " << pTimerInterval << " second(s) is not a common multiple of 1.0 second.  Any throttling rates may be implemented as slightly more than what is specified.";
        }
        ++poppedCount;
    }
    LOG(bb,always) << "Timer interval is set to " << pTimerInterval << " second(s) with a multiplier of " << poppedCount << " to implement a throttle rate with " \
                   << pTimerInterval*poppedCount << " second intervals.";

    return;
}


/*
 * Classes derived from BBLocalRequest doit() methods
 */
void BBAsyncRemoveJobInfo::doit()
{
    const uint64_t l_JobsToSchedulePerPass = g_AsyncRemoveJobInfoNumberPerGroup;
    uint64_t l_JobsScheduled = 0;

    vector<string> l_PathJobIds;
    l_PathJobIds.reserve(100);

    doitStart(&g_AsyncRemoveJobInfo_Controller);

    try
    {
        bool l_AllDone = false;
        while (!l_AllDone)
        {
            l_AllDone = true;
            int rc = HandleFile::get_xbbServerGetCurrentJobIds(l_PathJobIds, ONLY_RETURN_REMOVED_JOBIDS);
            if ((!rc) && l_PathJobIds.size() > 0)
            {
                for (size_t i=0; i<l_PathJobIds.size() && (l_JobsToSchedulePerPass == 0 || l_JobsScheduled < l_JobsToSchedulePerPass); i++)
                {
                    bfs::path job = bfs::path(l_PathJobIds[i]);
                    bfs::path l_PathToRemove = job.parent_path().string() + "/." + job.filename().string();
                    LOG(bb,debug) << "asyncRemoveJobInfo(): " << job.string() << " being renamed to " << l_PathToRemove.string();
                    try
                    {
                        bfs::rename(job, l_PathToRemove);
                    }
                    catch (std::exception& e1)
                    {
                        rc = -1;
                    }

                    if (!rc)
                    {
                        BBPruneMetadata* l_Request = new BBPruneMetadata(l_PathToRemove.string());
                        g_LocalAsync.issueAsyncRequest(l_Request);
                        ++l_JobsScheduled;
                    }
                    rc = 0;
                }
                if (l_JobsToSchedulePerPass > 0 && l_JobsScheduled >= l_JobsToSchedulePerPass)
                {
                    // In an attempt to let other servers jump in and schedule some pruning of the metadata...
                    // NOTE: We may delay for 1 minute when there are no jobs left to schedule, but the
                    //       timing here isn't that critical...
                    usleep(60000000);    // Delay for 1 minute...
                    l_JobsScheduled = 0;
                    l_AllDone = false;
                }
            }
        }
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }

    doitEnd(&g_AsyncRemoveJobInfo_Controller);

    return;
}

void BBCheckCycleActivities::doit()
{
    doitStart(&g_CycleActivities_Controller);

    try
    {
        // See if it is time to dump local async manager
        g_Dump_Local_Async_Controller.checkTimeToPerform();

        // See if it is time to dump the work manager
        g_Dump_WrkQMgr_Controller.checkTimeToPerform();

        // See if it is time to swap out the existing async request file and create a new one
        g_SwapAsyncRequestFile_Controller.checkTimeToPerform();

        // See if it is time to dump IO Stats
        g_BBIO_Stats_Controller.checkTimeToPerform();

        // See if it is time to dump IB Stats
        g_BBIB_Stats_Controller.checkTimeToPerform();

        // See if it is time to dump counters
        g_Dump_Counters_Controller.checkTimeToPerform();

        // See if it is time to dump the heartbeat information
        g_Dump_Heartbeat_Data_Controller.checkTimeToPerform();

        // See if it is time to asynchronously remove job information from the cross-bbServer metadata
        g_AsyncRemoveJobInfo_Controller.checkTimeToPerform();

        // See if it is time to remove any expired async request files
        g_RemoveAsyncRequestFile_Controller.checkTimeToPerform();

        // See if it is time to have a heartbeat for this bbServer
        // NOTE: This controller executes the "heartbeat operation" directly inline
        g_Heartbeat_Controller.checkTimeToPerform();
    }
    catch(ExceptionBailout& e) { }
    catch(std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }

    doitEnd(&g_CycleActivities_Controller);

    return;
}

void BBCleanUpContribId::doit()
{
    WRKQE* l_WrkQE = 0;

    int l_LocalMetadataLocked = 0;
    int l_TransferQueueLocked = 0;
    int l_rmvWrkQ_Locked = 0;

    bool l_AllDone = false;
    try
    {
        while (!l_AllDone)
        {
            l_AllDone = true;
            uint64_t l_JobId = tagid.getJobId();

            // NOTE: This mutex serializes with the bbwrkqmgr::rmvWrkQ()
            wrkqmgr.lock_rmvWrkQ();
            l_rmvWrkQ_Locked = 1;

            int rc = wrkqmgr.getWrkQE(&lvkey, l_WrkQE);
            if (rc == 1 && l_WrkQE)
            {
                rc = 0;
                CurrentWrkQE = l_WrkQE;

                lockLocalMetadata(&lvkey, "BBCleanUpContribId::doit");
                l_LocalMetadataLocked = 1;
                lockTransferQueue(&lvkey, "BBCleanUpContribId::doit");
                l_TransferQueueLocked = 1;

                size_t l_CurrentNumberOfInFlightExtents = 1;
                {

                    BBLV_Info* l_LV_Info = metadata.getLV_Info(&lvkey);
                    // NOTE: If stageout end processing has started, we cannot continue.
                    //       That processing will clean up this transfer definition.
                    if (l_LV_Info && (!l_LV_Info->stageOutEnded()))
                    {
                        l_CurrentNumberOfInFlightExtents = l_LV_Info->moreExtentsToTransfer(handle, contribid, 0);
                    }
                    else
                    {
                        BAIL;
                    }
                }

                if (!l_CurrentNumberOfInFlightExtents)
                {
                    for (auto it = metadata.metaDataMap.begin(); it != metadata.metaDataMap.end(); ++it)
                    {
                        if ((it->first).first == connection_name && (it->second).getJobId() == l_JobId)
                        {
                            (it->second).getTagInfoMap()->cleanUpContribId(&lvkey, tagid, handle, contribid);
                        }
                    }
                }
                else
                {
                    l_AllDone = false;
                }
            }

            if (l_TransferQueueLocked)
            {
                l_TransferQueueLocked = 0;
                unlockTransferQueue(&lvkey, "BBCleanUpContribId::doit");
            }
            CurrentWrkQE = (WRKQE*)0;

            if (l_LocalMetadataLocked)
            {
                l_LocalMetadataLocked = 0;
                unlockLocalMetadata(&lvkey, "BBCleanUpContribId::doit");
            }

            if (l_rmvWrkQ_Locked)
            {
                l_rmvWrkQ_Locked = 0;
                wrkqmgr.unlock_rmvWrkQ();
            }

            if (!l_AllDone)
            {
                usleep((useconds_t)250000); // Still had extents not yet removed from
                                            // the in-flight queue.  Delay 250 milliseconds.
            }
        }
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }

    // Cleanup
    try
    {
        if (l_TransferQueueLocked)
        {
            l_TransferQueueLocked = 0;
            unlockTransferQueue(&lvkey, "BBCleanUpContribId::doit - On exit");
        }
        CurrentWrkQE = (WRKQE*)0;
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }

    try
    {
        if (l_LocalMetadataLocked)
        {
            l_LocalMetadataLocked = 0;
            unlockLocalMetadata(&lvkey, "BBCleanUpContribId::doit - On exit");
        }
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }

    try
    {

        if (l_rmvWrkQ_Locked)
        {
            l_rmvWrkQ_Locked = 0;
            wrkqmgr.unlock_rmvWrkQ();
        }
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }

    return;
}

void BBCleanUpTagInfo::doit()
{
    WRKQE* l_WrkQE = 0;

    int l_LocalMetadataLocked = 0;
    int l_TransferQueueLocked = 0;
    int l_rmvWrkQ_Locked = 0;

    bool l_AllDone = false;
    try
    {
        while (!l_AllDone)
        {
            l_AllDone = true;
            uint64_t l_JobId = tagid.getJobId();

            // NOTE: This mutex serializes with the bbwrkqmgr::rmvWrkQ()
            wrkqmgr.lock_rmvWrkQ();
            l_rmvWrkQ_Locked = 1;

            int rc = wrkqmgr.getWrkQE(&lvkey, l_WrkQE);
            if (rc == 1 && l_WrkQE)
            {
                rc = 0;
                CurrentWrkQE = l_WrkQE;

                lockLocalMetadata(&lvkey, "BBCleanUpTagInfo::doit");
                l_LocalMetadataLocked = 1;
                lockTransferQueue(&lvkey, "BBCleanUpTagInfo::doit");
                l_TransferQueueLocked = 1;

                size_t l_CurrentNumberOfInFlightExtents = 1;
                {
                    BBLV_Info* l_LV_Info = metadata.getLV_Info(&lvkey);
                    // NOTE: If stageout end processing has started, we cannot continue.
                    //       That processing will clean up this taginfo object.
                    if (l_LV_Info && (!l_LV_Info->stageOutEnded()))
                    {
                        BBTagInfo* l_TagInfo = l_LV_Info->getTagInfo(tagid);
                        if (l_TagInfo)
                        {
                            l_CurrentNumberOfInFlightExtents = l_LV_Info->moreExtentsToTransfer(l_TagInfo->getTransferHandle(), UNDEFINED_CONTRIBID, 0);
                        }
                        else
                        {
                            BAIL;
                        }
                    }
                    else
                    {
                        BAIL;
                    }
                }

                if (!l_CurrentNumberOfInFlightExtents)
                {
                    for (auto it = metadata.metaDataMap.begin(); it != metadata.metaDataMap.end(); ++it)
                    {
                        if ((it->first).first == connection_name && (it->second).getJobId() == l_JobId)
                        {
                            (it->second).getTagInfoMap()->cleanUpTagInfo(&lvkey, tagid);
                        }
                    }
                }
                else
                {
                    l_AllDone = false;
                }
            }

            if (l_TransferQueueLocked)
            {
                l_TransferQueueLocked = 0;
                unlockTransferQueue(&lvkey, "BBCleanUpTagInfo::doit");
            }
            CurrentWrkQE = (WRKQE*)0;

            if (l_LocalMetadataLocked)
            {
                l_LocalMetadataLocked = 0;
                unlockLocalMetadata(&lvkey, "BBCleanUpTagInfo::doit");
            }

            if (l_rmvWrkQ_Locked)
            {
                l_rmvWrkQ_Locked = 0;
                wrkqmgr.unlock_rmvWrkQ();
            }

            if (!l_AllDone)
            {
                usleep((useconds_t)250000); // Still had extents not yet removed from
                                            // the in-flight queue.  Delay 250 milliseconds.
            }
        }
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }

    // Cleanup
    try
    {
        if (l_TransferQueueLocked)
        {
            l_TransferQueueLocked = 0;
            unlockTransferQueue(&lvkey, "BBCleanUpTagInfo::doit - On exit");
        }
        CurrentWrkQE = (WRKQE*)0;
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }

    try
    {
        if (l_LocalMetadataLocked)
        {
            l_LocalMetadataLocked = 0;
            unlockLocalMetadata(&lvkey, "BBCleanUpTagInfo::doit - On exit");
        }
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }

    try
    {
        if (l_rmvWrkQ_Locked)
        {
            l_rmvWrkQ_Locked = 0;
            wrkqmgr.unlock_rmvWrkQ();
        }
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }

    return;
}

void BBCounters::doit()
{
    doitStart(&g_Dump_Counters_Controller);

    try
    {
        start_logging();
        #define MKBBCOUNTER(id) if(bbcounters[BB_COUNTERS_##id] != bbcounters_shadow[BB_COUNTERS_##id]) { LOG(bb,always) << "BB Counter '" #id "' = " << bbcounters[BB_COUNTERS_##id] << " (delta " << (bbcounters[BB_COUNTERS_##id] - bbcounters_shadow[BB_COUNTERS_##id]) << ")"; bbcounters_shadow[BB_COUNTERS_##id] = bbcounters[BB_COUNTERS_##id]; }
        #include "bbcounters.h"
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }
    end_logging();

    doitEnd(&g_Dump_Counters_Controller);

    return;
}

void BBDumpHeartbeatData::doit()
{
    doitStart(&g_Dump_Heartbeat_Data_Controller);

    try
    {
        start_logging();
        wrkqmgr.dumpHeartbeatData("info");
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }
    end_logging();

    doitEnd(&g_Dump_Heartbeat_Data_Controller);

    return;
}

void BBDumpLocalAsync::doit()
{
    doitStart(&g_Dump_Local_Async_Controller);

    try
    {
        start_logging();
        g_LocalAsync.dump(" Local Async Mgr (Not an error - Timer Interval)");
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }
    end_logging();

    doitEnd(&g_Dump_Local_Async_Controller);

    return;
}

void BBDumpWrkQMgr::doit()
{
    doitStart(&g_Dump_WrkQMgr_Controller);

    try
    {
        start_logging();
        wrkqmgr.dump("info", " Work Queue Mgr (Not an error - Timer Interval)", DUMP_ALWAYS);
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }
    end_logging();

    doitEnd(&g_Dump_WrkQMgr_Controller);

    return;
}

void BBIB_Stats::doit()
{
    doitStart(&g_BBIB_Stats_Controller);

    try
    {
        string l_Port_Rcv_Data = "port_rcv_data";
        string l_Port_Xmit_Data = "port_xmit_data";

        start_logging();
        for(const auto& line : runCommand("grep '' /sys/class/infiniband/*/ports/*/*counters/*"))
        {
            auto tokens = buildTokens(line, ":");
            if(tokens.size() == 2)
            {
                if(diskStatCache[tokens[0]] != tokens[1])
                {
                    if(!diskStatCache[tokens[0]].empty())
                    {
                        uint64_t l_Delta = stoll(tokens[1]) - stoll(diskStatCache[tokens[0]]);
                        LOG(bb,always) << "IBSTAT:  " << line << " (delta " << l_Delta << ")";
                        if (tokens[0].find(g_IB_NVMe_Adapter) != string::npos)
                        {
                            if (tokens[0].find(l_Port_Rcv_Data) != string::npos)
                            {
                                g_Last_Port_Rcv_Data_Delta = l_Delta;
                            }
                            else if (tokens[0].find(l_Port_Xmit_Data) != string::npos)
                            {
                                g_Last_Port_Xmit_Data_Delta = l_Delta;
                            }
                        }
                    }
                    else
                    {
                        LOG(bb,always) << "IBSTAT:  " << line;
                    }

                    diskStatCache[tokens[0]] = tokens[1];
                }
            }
        }
    }
    catch(ExceptionBailout& e) { }
    catch(std::exception& e)
    {
        // NOTE:  Set the two static xmit/rcv values to -1 so we DO NOT
        //        attempt any additional async job prunes/removals of branches/trees.
        g_Last_Port_Rcv_Data_Delta = -1;
        g_Last_Port_Xmit_Data_Delta = -1;
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }
    end_logging();

    doitEnd(&g_BBIB_Stats_Controller);

    return;
}

void BBIO_Stats::doit()
{
    doitStart(&g_BBIO_Stats_Controller);

    try
    {
        std::string cmd = string("/usr/bin/timeout ") + to_string(g_DiskStatsRate+2) + string(" /usr/bin/iostat -xym -p ALL ") + to_string(g_DiskStatsRate);

        std::vector<std::string> l_Lines = runCommand(cmd);
        start_logging();
        for(const auto& line : l_Lines)
        {
            auto tokens = buildTokens(line, " ");
            if(tokens.size() > 10)
            {
                if(diskStatCache[tokens[0]] != line)
                {
                    diskStatCache[tokens[0]] = line;
                    LOG(bb,always) << "IOSTAT:  " << line;
                }
            }
        }
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }
    end_logging();

    doitEnd(&g_BBIO_Stats_Controller);

    return;
}

void BBLogIt::doit()
{
    try
    {
        LOG(bb,info) << data.c_str();
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }

    return;
}

void BBPruneMetadata::doit()
{
    int64_t l_RequestNumber = 0;

    bfs::path l_PathToRemove = bfs::path(path);
    try
    {
        vector<std::string> l_HandlesProcessed = vector<std::string>();
        l_HandlesProcessed.reserve(4096);
        bool l_AllDone = false;

        LOG(bb,info) << "BBLocalAsync::BBPruneMetadata(): START: Removal of cross-bbServer metadata at " << path;
        while (!l_AllDone)
        {
            try
            {
                l_AllDone = true;
                if (!access(path.c_str(), F_OK))
                {
                    // Jobid directory path still exists
                    for (auto& jobstep : boost::make_iterator_range(bfs::directory_iterator(l_PathToRemove), {}))
                    {
                        if (!pathIsDirectory(jobstep)) continue;
                        for (auto& handlebucket : boost::make_iterator_range(bfs::directory_iterator(jobstep), {}))
                        {
                            if (!pathIsDirectory(handlebucket)) continue;
                            for (auto& handledir : boost::make_iterator_range(bfs::directory_iterator(handlebucket), {}))
                            {
                                if (!pathIsDirectory(handledir)) continue;
                                std::string l_Handle = handledir.path().string();
                                if (find(l_HandlesProcessed.begin(), l_HandlesProcessed.end(), l_Handle) == l_HandlesProcessed.end())
                                {
                                    BBPruneMetadataBranch* l_Request = new BBPruneMetadataBranch(l_Handle);
                                    l_RequestNumber = g_LocalAsync.issueAsyncRequest(l_Request);
                                    l_HandlesProcessed.push_back(l_Handle);
                                }
                            }
                        }
                    }
                }
            }
            catch (std::exception& e2)
            {
                // Tolerate any exceptions when looping through the jobid directory...
                l_AllDone = false;
            }
        }

        // NOTE: Create a dummy request of the same type as above to test for when that
        //       priority of the requests issued above are complete.  We cannot use the
        //       last request allocated on the heap because once issued, the BBLocalAsync
        //       object owns the existence of that request.  We cannot touch the request
        //       after we issue it.  BBLocalAsync processing will delete the allocated
        //       storage for the request object.
        BBPruneMetadataBranch l_Dummy = BBPruneMetadataBranch("");
        bool l_MsgSent = false;
        while (l_RequestNumber && l_RequestNumber > g_LocalAsync.getLastRequestNumberProcessed(&l_Dummy))
        {
            LOG(bb,debug) << "BBLocalAsync::BBPruneMetadata(): >>>>> DELAY <<<<< l_RequestNumber " << l_RequestNumber \
                          << ", g_LocalAsync.getLastRequestNumberProcessed() " << g_LocalAsync.getLastRequestNumberProcessed(&l_Dummy) \
                          << ", delay 10 seconds and retry.";
            l_MsgSent = true;
            usleep(10000000);   // Delay 10 seconds
        }
        if (l_MsgSent)
        {
            LOG(bb,debug) << "BBLocalAsync::BBPruneMetadata(): >>>>> RESUME <<<<< l_RequestNumber " << l_RequestNumber \
                          << ", g_LocalAsync.getLastRequestNumberProcessed() " << g_LocalAsync.getLastRequestNumberProcessed(&l_Dummy);
        }

        BBPruneMetadataBranch* l_Request = new BBPruneMetadataBranch(l_PathToRemove.string());
        l_RequestNumber = g_LocalAsync.issueAsyncRequest(l_Request);
        l_MsgSent = false;
        while (l_RequestNumber > g_LocalAsync.getLastRequestNumberProcessed(&l_Dummy))
        {
            LOG(bb,debug) << "BBLocalAsync::BBPruneMetadata(): >>>>> DELAY <<<<< l_RequestNumber " << l_RequestNumber \
                          << ", g_LocalAsync.getLastRequestNumberProcessed() " << g_LocalAsync.getLastRequestNumberProcessed(&l_Dummy) \
                          << ", delay 1 second and retry.";
            l_MsgSent = true;
            usleep(1000000);   // Delay 1 second
        }
        if (l_MsgSent)
        {
            LOG(bb,debug) << "BBLocalAsync::BBPruneMetadata(): >>>>> RESUME <<<<< l_RequestNumber " << l_RequestNumber \
                          << ", g_LocalAsync.getLastRequestNumberProcessed() " << g_LocalAsync.getLastRequestNumberProcessed(&l_Dummy);
        }
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
        LOG(bb,error) << "BBLocalAsync::BBPruneMetadata(): Unsuccessful removal of cross-bbServer metadata at " << path.c_str() \
                      << ". This cross-bbServer metadata must be manually removed.";
    }

    LOG(bb,info) << "BBLocalAsync::BBPruneMetadata():   END: Removal of cross-bbServer metadata at " << path;

    return;
}

void BBPruneMetadataBranch::doit()
{
    try
    {
        bool l_AllDone = false;
        while (!l_AllDone)
        {
            l_AllDone = true;
            if (!highIB_Activity())
            {
                try
                {
                    bfs::path l_PathToRemove = bfs::path(path);
                    bfs::remove_all(l_PathToRemove);
                    LOG(bb,debug) << "BBLocalAsync::BBPruneMetadataBranch(): Successful removal of cross-bbServer metadata at " << path.c_str();
                }
                catch (std::exception& e2)
                {
                    LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e2);
                    LOG(bb,error) << "BBLocalAsync::BBPruneMetadataBranch(): Unsuccessful removal of cross-bbServer metadata at " << path.c_str() \
                                  << ". This cross-bbServer metadata must be manually removed.";
                }
            }
            else
            {
                l_AllDone = false;
                LOG(bb,debug) << "BBLocalAsync::BBPruneMetadataBranch(): IB activity too high to do removal at " << path.c_str() \
                              << ". Current " << g_IB_NVMe_Adapter << " port_rcv_data delta " << g_Last_Port_Rcv_Data_Delta \
                              << ", current " << g_IB_NVMe_Adapter << " port_xmit_data delta " << g_Last_Port_Xmit_Data_Delta \
                              << ", current IB stats low activity clip value " << g_IBStatsLowActivityClipValue \
                              << ". Delaying " << g_DiskStatsRate << " seconds...";
                sleep(g_DiskStatsRate);
            }
        }
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }

    return;
}

void BBRemoveAsyncRequestFile::doit()
{
    int l_SeqNbr = 0;
    int l_CurrentSeqNbr = 0;
    char* l_AsyncRequestFileNamePtr = 0;
    bool l_WorkQueueMgrLocked = false;
    bool l_TransferQueueLocked = false;

    doitStart(&g_RemoveAsyncRequestFile_Controller);

    try
    {
        // Find the current async request file
        wrkqmgr.lockWorkQueueMgr((LVKey*)0, "BBRemoveAsyncRequestFile::doit");
        l_WorkQueueMgrLocked = true;
        HPWrkQE->lock((LVKey*)0, "BBRemoveAsyncRequestFile::doit");
        l_TransferQueueLocked = true;

        int rc = wrkqmgr.verifyAsyncRequestFile(l_AsyncRequestFileNamePtr, l_SeqNbr);

        l_TransferQueueLocked = false;
        HPWrkQE->unlock((LVKey*)0, "BBRemoveAsyncRequestFile::doit");
        l_WorkQueueMgrLocked = false;
        wrkqmgr.unlockWorkQueueMgr((LVKey*)0, "BBRemoveAsyncRequestFile::doit");

        if ((!rc) && l_AsyncRequestFileNamePtr)
        {
            bfs::path datastore(g_BBServer_Metadata_Path);
            for (auto& asyncfile : boost::make_iterator_range(bfs::directory_iterator(datastore), {}))
            {
                if(pathIsDirectory(asyncfile)) continue;

                if (asyncfile.path().filename().string() == XBBSERVER_ASYNC_REQUEST_BASE_FILENAME)
                {
                    try
                    {
                        // Old style....  Simply delete it...
                        int rc = remove(asyncfile.path().c_str());
                        if (!rc)
                        {
                            LOG(bb,info) << "BBLocalAsync::BBRemoveAsyncRequestFile(): Deprecated async request file " << asyncfile.path().c_str() << " removed";
                        }
                        else
                        {
                            LOG(bb,warning) << "BBLocalAsync::BBRemoveAsyncRequestFile(): Deprecated async request file " << asyncfile.path().c_str() \
                                            << " could not be removed, errno=" << errno << ", "<< strerror(errno) \
                                            << ". Continuing...";
                        }
                    }
                    catch(std::exception& e)
                    {
                        // NOTE:  It is possible for multiple bbServers to be performing maintenance at the same time.
                        //        Tolerate any exception and continue...
                    }
                    continue;
                }

                int l_Count = sscanf(asyncfile.path().filename().c_str(),"asyncRequests_%d", &l_CurrentSeqNbr);
                if (l_Count == 1 && l_CurrentSeqNbr < l_SeqNbr)
                {
                    try
                    {
                        // Old async file....  If old enough, delete it...
                        struct stat l_Statinfo;
                        threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::statsyscall, asyncfile.path().c_str(), __LINE__);
                        int rc = stat(asyncfile.path().c_str(), &l_Statinfo);
                        threadLocalTrackSyscallPtr->clearTrack();
                        FL_Write(FLAsyncRqst, Stat, "Get stats for async request file having seqnbr %ld for aging purposes, rc %ld.", l_CurrentSeqNbr, rc, 0, 0);
                        if (!rc)
                        {
                            time_t l_CurrentTime = time(0);
                            if (difftime(l_CurrentTime, l_Statinfo.st_atime) > ASYNC_REQUEST_FILE_PRUNE_TIME)
                            {
                                rc = remove(asyncfile.path().c_str());
                                if (!rc)
                                {
                                    LOG(bb,info) << "BBLocalAsync::BBRemoveAsyncRequestFile(): Async request file " << asyncfile.path() << " removed";
                                }
                                else
                                {
                                    LOG(bb,warning) << "BBLocalAsync::BBRemoveAsyncRequestFile(): Async request file " << asyncfile.path().c_str() \
                                                    << " could not be removed, errno=" << errno << ", "<< strerror(errno) \
                                                    << ". Continuing...";
                                }
                            }
                        }
                    }
                    catch(std::exception& e)
                    {
                        // NOTE:  It is possible for multiple bbServers to be performing maintenance at the same time.
                        //        Tolerate any exception and continue...
                    }
                }
            }
        }
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }

    // Cleanup
    try
    {
        if (l_TransferQueueLocked)
        {
            l_TransferQueueLocked = false;
            HPWrkQE->unlock((LVKey*)0, "BBRemoveAsyncRequestFile::doit - On exit");
        }
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }

    try
    {
        if (l_WorkQueueMgrLocked)
        {
            l_WorkQueueMgrLocked = false;
            wrkqmgr.unlockWorkQueueMgr((LVKey*)0, "BBRemoveAsyncRequestFile::doit - On exit");
        }
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }

    try
    {
        if (l_AsyncRequestFileNamePtr)
        {
            delete [] l_AsyncRequestFileNamePtr;
            l_AsyncRequestFileNamePtr = 0;
        }
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }

    doitEnd(&g_RemoveAsyncRequestFile_Controller);

    return;
}

void BBSwapAsyncRequestFile::doit()
{
    stringstream errorText;

    const char* l_OpenOption = "ab";
    int l_SeqNbr = 0;

    FILE* l_FilePtr = 0;
    char* l_AsyncRequestFileNamePtr = 0;
    bool l_WorkQueueMgrLocked = false;
    bool l_TransferQueueLocked = false;

    doitStart(&g_SwapAsyncRequestFile_Controller);

    try
    {
        wrkqmgr.lockWorkQueueMgr((LVKey*)0, "BBSwapAsyncRequestFile::doit");
        l_WorkQueueMgrLocked = true;
        HPWrkQE->lock((LVKey*)0, "BBSwapAsyncRequestFile::doit");
        l_TransferQueueLocked = true;

        // Find the current async request file
        int rc = wrkqmgr.verifyAsyncRequestFile(l_AsyncRequestFileNamePtr, l_SeqNbr);
        if ((!rc) && l_AsyncRequestFileNamePtr)
        {
            threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::fopensyscall, l_AsyncRequestFileNamePtr, __LINE__);
            l_FilePtr = ::fopen(l_AsyncRequestFileNamePtr, l_OpenOption);
            threadLocalTrackSyscallPtr->clearTrack();
            if (l_FilePtr != NULL)
            {
                setbuf(l_FilePtr, NULL);
                FL_Write(FLAsyncRqst, OpenForSwap, "Open async request file having seqnbr %ld using mode 'ab'. File pointer returned is %p.", l_SeqNbr, (uint64_t)(void*)l_FilePtr, 0, 0);

                // Check the file size
                threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::ftellsyscall, l_FilePtr, __LINE__);
                int64_t l_Offset = (int64_t)::ftell(l_FilePtr);
                threadLocalTrackSyscallPtr->clearTrack();
                if (l_Offset >= 0)
                {
                    FL_Write(FLAsyncRqst, RtvEndOffset, "Check if it is time to create a new async request file. Current file has seqnbr %ld, ending offset %ld.", l_SeqNbr, l_Offset, 0, 0);
                    if (l_Offset > (int64_t)ASYNC_REQUEST_FILE_SIZE_FOR_SWAP)
                    {
                        // Time for a new async request file...
                        delete [] l_AsyncRequestFileNamePtr;
                        l_AsyncRequestFileNamePtr = 0;
                        wrkqmgr.verifyAsyncRequestFile(l_AsyncRequestFileNamePtr, l_SeqNbr, CREATE_NEW_FILE);
                    }
                }
                else
                {
                    FL_Write(FLAsyncRqst, RtvEndOffsetFail, "Failed ftell() request, sequence number %ld, errno %ld.", l_SeqNbr, errno, 0, 0);
                    errorText << "BBSwapAsyncRequestFile(): Failed ftell() request, sequence number " << l_SeqNbr;
                    LOG_ERROR_TEXT_ERRNO(errorText, errno);
                }

                // Close the file...
                FL_Write(FLAsyncRqst, CloseForSwap, "Close async request file having seqnbr %ld using mode 'ab'. File pointer is %p.", l_SeqNbr, (uint64_t)(void*)l_FilePtr, 0, 0);
                ::fclose(l_FilePtr);
                l_FilePtr = NULL;
            }
            else
            {
                FL_Write(FLAsyncRqst, OpenForSwapFailed, "Open async request file having seqnbr %ld using mode 'ab'.", l_SeqNbr, 0, 0, 0);
                errorText << "BBSwapAsyncRequestFile(): Open async request file having seqnbr " << l_SeqNbr << " using mode 'ab' failed.";
                LOG_ERROR_TEXT_ERRNO(errorText, errno);
            }
        }

        l_TransferQueueLocked = false;
        HPWrkQE->unlock((LVKey*)0, "BBSwapAsyncRequestFile::doit");
        l_WorkQueueMgrLocked = false;
        wrkqmgr.unlockWorkQueueMgr((LVKey*)0, "BBSwapAsyncRequestFile::doit");
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }

    // Cleanup
    try
    {
        if (l_FilePtr)
        {
            FL_Write(FLAsyncRqst, CloseForSwapOnExit, "Close async request file having seqnbr %ld using mode 'ab'. File pointer is %p.", l_SeqNbr, (uint64_t)(void*)l_FilePtr, 0, 0);
            ::fclose(l_FilePtr);
            l_FilePtr = NULL;
        }
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }

    try
    {
        if (l_TransferQueueLocked)
        {
            l_TransferQueueLocked = false;
            HPWrkQE->unlock((LVKey*)0, "BBSwapAsyncRequestFile::doit - On exit");
        }
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }

    try
    {
        if (l_WorkQueueMgrLocked)
        {
            l_WorkQueueMgrLocked = false;
            wrkqmgr.unlockWorkQueueMgr((LVKey*)0, "BBSwapAsyncRequestFile::doit - On exit");
        }
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }

    try
    {
        if (l_AsyncRequestFileNamePtr)
        {
            delete [] l_AsyncRequestFileNamePtr;
            l_AsyncRequestFileNamePtr = 0;
        }
    }
    catch (ExceptionBailout& e) { }
    catch (std::exception& e)
    {
        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
    }

    doitEnd(&g_SwapAsyncRequestFile_Controller);

    return;
}


/*
 * Classes derived from BBLocalRequest dump() methods
 */
void BBLocalRequest::dump(const char* pPrefix)
{
    stringstream dumpData;
    if (strlen(pPrefix))
    {
        dumpData << pPrefix;
    }
    dumpRequest(dumpData);
    LOG(bb,info) << dumpData.str();

    return;
}

void BBCleanUpContribId::dump(const char* pPrefix)
{
    stringstream dumpData;

    if (strlen(pPrefix))
    {
        dumpData << pPrefix;
    }
    dumpRequest(dumpData);
    dumpData << ", jobid " << tagid.getJobId() \
             << ", jobstepid " << tagid.getJobStepId() \
             << ", tag "<< tagid.getTag() << ", handle "<< handle \
             << ", contribid "<< contribid;
    LOG(bb,info) << dumpData.str();

    return;
}

void BBCleanUpTagInfo::dump(const char* pPrefix)
{
    stringstream dumpData;

    if (strlen(pPrefix))
    {
        dumpData << pPrefix;
    }
    dumpRequest(dumpData);
    dumpData << "Connection " << connection_name << ", " << lvkey \
             << ", jobid " << tagid.getJobId() \
             << ", jobstepid " << tagid.getJobStepId() \
             << ", tag "<< tagid.getTag();
    LOG(bb,info) << dumpData.str();

    return;
}

void BBLogIt::dump(const char* pPrefix)
{
    stringstream dumpData;

    if (strlen(pPrefix))
    {
        dumpData << pPrefix;
    }
    dumpRequest(dumpData);
    dumpData << ", data " << data;
    LOG(bb,info) << dumpData.str();

    return;
}

void BBPruneMetadata::dump(const char* pPrefix)
{
    stringstream dumpData;

    if (strlen(pPrefix))
    {
        dumpData << pPrefix;
    }
    dumpRequest(dumpData);
    dumpData << ", trunk path " << path;
    LOG(bb,info) << dumpData.str();

    return;
}

void BBPruneMetadataBranch::dump(const char* pPrefix)
{
    stringstream dumpData;

    if (strlen(pPrefix))
    {
        dumpData << pPrefix;
    }
    dumpRequest(dumpData);
    dumpData << ", branch path " << path;
    LOG(bb,info) << dumpData.str();

    return;
}
