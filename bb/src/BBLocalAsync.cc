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

#include "bbinternal.h"
#include "BBLocalAsync.h"
#include "usage.h"
#include "xfer.h"

#include <boost/filesystem.hpp>

using namespace std;
using namespace boost;
namespace bfs = boost::filesystem;


/*
 * Helper methods
 */
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

        case MEDIUM:
            l_Priority = "MEDIUM";
            break;

        case LOW:
            l_Priority = "LOW";
            break;

        default:
            l_Priority = "Unknown";
            break;
    }

    return l_Priority;
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
        pRequest->dump("BBLocalAsync::addRequest(): Enqueuing: ");
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

int64_t BBAsyncRequestData::getNumberOfInFlightRequests()
{
    int64_t l_NumberOfRequests = 0;

    lock();
    l_NumberOfRequests = lastRequestNumberDispatched - lastRequestNumberProcessed;
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
                                      << ", removed request number " << pRequestNumber << " from outOfSequenceRequests";
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
        requests.pop();
        rc = increment(lastRequestNumberDispatched);
        pRequest->dump("BBLocalAsync::removeNextRequest(): Dequeuing: ");
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


/*
 * BBLocalAsync class methods
 */
void* BBLocalAsync::asyncRequestWorker(void* ptr)
{
    BBLocalRequest* l_Request = 0;

    while (1)
    {
        try
        {
            int64_t l_RequestNumber = g_LocalAsync.getNextRequest(l_Request);
            if (l_RequestNumber > 0 && l_Request)
            {
                l_Request->doit();
                g_LocalAsync.recordRequestCompletion(l_RequestNumber, l_Request);
                delete l_Request;
                l_Request = 0;
            }
        }
        catch (ExceptionBailout& e) { }
        catch (std::exception& e)
        {
            LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
        }
        if (l_Request)
        {
            delete l_Request;
        }
        l_Request = 0;
    }

    return NULL;
}

int BBLocalAsync::dispatchFromThisQueue(LOCAL_ASYNC_REQUEST_PRIORITY pPriority)
{
    int rc = 0;

    int64_t l_NumberOfNonDispatchedRequests = requestData[pPriority]->numberOfNonDispatchedRequests();
    if (l_NumberOfNonDispatchedRequests)
    {
        int64_t l_NumberOfInFlightRequests = (int64_t)requestData[pPriority]->getNumberOfInFlightRequests();
        int64_t l_MaximumConcurrentRunning = (int64_t)requestData[pPriority]->getMaximumConcurrentRunning();
        if (l_NumberOfInFlightRequests < l_MaximumConcurrentRunning)
        {
            rc = 1;
            LOG(bb,debug) << "BBLocalAsync::dispatchFromThisQueue(): Scheduling async request having priority " << getLocalAsyncPriorityStr(pPriority) \
                          << ", NumberOfNonDispatchedRequests " << l_NumberOfNonDispatchedRequests \
                          << ", NumberOfInFlightRequests " << l_NumberOfInFlightRequests \
                          << ", MaximumConcurrentRunning " << l_MaximumConcurrentRunning;
        }
    }

    return rc;
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

    size_t l_NumberOfHighPriorityRequestsInFlight = 0;
    size_t l_NumberOfMediumPriorityRequestsInFlight = 0;
    size_t l_NumberOfLowPriorityRequestsInFlight = 0;

    sem_wait(&work);

    lock();
    try
    {
        l_NumberOfHighPriorityRequestsInFlight = requestData[HIGH]->getNumberOfInFlightRequests();
        l_NumberOfMediumPriorityRequestsInFlight = requestData[MEDIUM]->getNumberOfInFlightRequests();
        l_NumberOfLowPriorityRequestsInFlight = requestData[LOW]->getNumberOfInFlightRequests();
        LOG(bb,debug) << "BBLocalAsync::getNextRequest(): Inflight HIGH " << l_NumberOfHighPriorityRequestsInFlight \
                      << ", MEDIUM " << l_NumberOfMediumPriorityRequestsInFlight \
                      << ", LOW " << l_NumberOfLowPriorityRequestsInFlight;

        LOCAL_ASYNC_REQUEST_PRIORITY l_Priority = HIGH;
        if (!dispatchFromThisQueue(l_Priority))
        {
            l_Priority = MEDIUM;
            if (!dispatchFromThisQueue(l_Priority))
            {
                l_Priority = LOW;
                if (!dispatchFromThisQueue(l_Priority))
                {
                    l_Priority = NONE;
                }
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
        // NOTE: Would be nice to make this more self-defining...
        requestData[HIGH] = new BBAsyncRequestData(HIGH, 0);
        requestData[MEDIUM] = new BBAsyncRequestData(MEDIUM, g_NumberOfAsyncRequestsThreads/3);
        requestData[LOW] = new BBAsyncRequestData(LOW, g_NumberOfAsyncRequestsThreads/6);
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
    LOG(bb,debug) << "BBLocalAsync::recordRequestCompletion(): Completed request, scheduling async request having priority " \
                  << getLocalAsyncPriorityStr(l_Request->getPriority()) \
                  << ", NumberOfNonDispatchedRequests " << requestData[l_Request->getPriority()]->numberOfNonDispatchedRequests() \
                  << ", NumberOfInFlightRequests " << requestData[l_Request->getPriority()]->getNumberOfInFlightRequests() \
                  << ", MaximumConcurrentRunning " << requestData[l_Request->getPriority()]->getMaximumConcurrentRunning();

    return;
}


/*
 * doit() methods
 */
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
        try
        {
            LOG(bb,info) << "BBLocalAsync::BBPruneMetadata(): START: Removal of cross-bbServer metadata at " << path;
            for (auto& jobstep : boost::make_iterator_range(bfs::directory_iterator(l_PathToRemove), {}))
            {
                if (!pathIsDirectory(jobstep)) continue;
                for (auto& handlebucket : boost::make_iterator_range(bfs::directory_iterator(jobstep), {}))
                {
                    if (!pathIsDirectory(handlebucket)) continue;
                    for (auto& handledir : boost::make_iterator_range(bfs::directory_iterator(handlebucket), {}))
                    {
                        if (!pathIsDirectory(handledir)) continue;
                        BBPruneMetadataBranch* l_Request = new BBPruneMetadataBranch(handledir.path().string());
                        l_RequestNumber = g_LocalAsync.issueAsyncRequest(l_Request);
                    }
                }
            }
        }
        catch (std::exception& e2)
        {
            // Tolerate any exceptions when looping through the jobid directory...
            if (!l_RequestNumber)
            {
                l_RequestNumber = -1;
            }
        }

        if (l_RequestNumber >= 0)
        {
            // NOTE: Create a dummy request of the same type as above to test for when that
            //       priority of the requests issued above are complete.  We cannot use the
            //       last request allocated on the heap because once issued, the BBLocalAsync
            //       object owns the existence of that request.  We cannot touch the request
            //       after we issue it.  BBLocalAsync processing will delete the allocated
            //       storage for the request object.
            BBPruneMetadataBranch l_Dummy = BBPruneMetadataBranch("");
            bool l_MsgSent = false;
            if (l_RequestNumber)
            {
                while (l_RequestNumber > g_LocalAsync.getLastRequestNumberProcessed(&l_Dummy))
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
        }
    }
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
                LOG(bb,debug) << "BBLocalAsync::BBPruneMetadataBranch(): IB activity too high to do final removal at " << path.c_str() \
                              << ". Current " << g_IB_Adapter << " port_rcv_data delta " << g_Last_Port_Rcv_Data_Delta \
                              << ", current " << g_IB_Adapter << " port_xmit_data delta " << g_Last_Port_Xmit_Data_Delta \
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


/*
 * dump() methods
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
