/*================================================================================

    csmd/src/ras/src/RasEventHandlerAction.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "csm_api_ras.h"
#include "../include/RasEventHandlerAction.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdio.h>
#include <map>
#include <exception>

#include "logging.h"
#include "../include/RasRunActionScript.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>

using namespace std;

RasRunActionScriptThread::~RasRunActionScriptThread()
{

    // make sure we get to the destructor for these..
    LOG(csmras, info) << "RasRunActionScriptThread::~RasRunActionScriptThread";

}

void RasRunActionScriptThread::actionThread()
{
LOG(csmras, info) << "RasRunActionScriptThread::actionThread";
    _rc = Run(_script, _arg);      // run the script in the background...
    if (_rc != 0) {
        LOG(csmras, error) << "RasEventHandlerAction script " << _script << " failed " << getErrStr();
    }

    // action ctrl knows about shared ptr's, not direct ptrs.....  
    // how do we get the shared ptr to this, without a circular reference...
    _threadDone = true;
    _actionCtl.threadDone(this);
};

void RasRunActionScriptThread::runThread(const std::string &script, 
                                         const std::string &arg)
{
    _script = script;           // hold on to parameters.
    _arg = arg;

LOG(csmras, info) << "RasRunActionScriptThread::runThread ";

    // start a background thread...
    _thread = boost::thread(&RasRunActionScriptThread::actionThread, this);

}

RasEventActionCtl::RasEventActionCtl(unsigned maxActions, unsigned maxThreads) :
    _maxActions(maxActions),
    _maxThreads(maxThreads),
    _actionQueueMutex(),
    _workSem(),
    _actionQueue(),
    _runningThreads(),
    _finishedThreads(),
    _theadPtrMap()
{
    LOG(csmras, info) << "RasEventActionCtl::RasEventActionCtl()";
}

unsigned RasEventActionCtl::getRunningThreads()
{
    boost::unique_lock<boost::mutex>  guard(_actionQueueMutex);
    return(_runningThreads.size());
}

void RasEventActionCtl::cleanAllDoneThreads()
{
LOG(csmras, info) << "RasEventActionCtl::cleanAllDoneThreads + " << _finishedThreads.size();
    boost::unique_lock<boost::mutex>  guard(_actionQueueMutex);
    while (_finishedThreads.size() > 0)  {
        RasRunActionScriptThread *thread = *(_finishedThreads.begin());
        //std::map<RasRunActionScriptThread *, std::shared_ptr<RasRunActionScriptThread> >::iterator it = 
        //  _theadPtrMap.find(thread);

        _finishedThreads.erase(thread);
        _theadPtrMap.erase(thread);   // toss the ptr...

    }
LOG(csmras, info) << "RasEventActionCtl::cleanAllDoneThreads - " << _finishedThreads.size();
}




void RasEventActionCtl::addRunningThread(std::shared_ptr<RasRunActionScriptThread> thread)
{
    boost::unique_lock<boost::mutex>  guard(_actionQueueMutex);
    _runningThreads.insert(thread.get());
    _theadPtrMap[thread.get()] = thread;
}
void RasEventActionCtl::threadDone(RasRunActionScriptThread *thread)
{
LOG(csmras, info) << "RasEventActionCtl::threadDone";
    boost::unique_lock<boost::mutex>  guard(_actionQueueMutex);
    _runningThreads.erase(thread);           // transfer from running to finished.
    _finishedThreads.insert(thread);
    wakeup();                       // wakeup the semaphore to cleanout the thread base and accept new work...

}
void RasEventActionCtl::removeThread(RasRunActionScriptThread *thread)
{
LOG(csmras, info) << "RasEventActionCtl::removeThread";
    boost::unique_lock<boost::mutex>  guard(_actionQueueMutex);
    _finishedThreads.erase(thread);   // garbage collection...
    _theadPtrMap.erase(thread);   // toss the ptr...

}

void RasEventActionCtl::wakeup()
{
    _workSem.notify_one();
}

void RasEventActionCtl::addRasActionQueue(const std::string &rasJsonData)
{
    try {
        boost::unique_lock<boost::mutex>  guard(_actionQueueMutex);
        if (_actionQueue.size() < _maxActions) {
            _actionQueue.push(rasJsonData);     // encode the event in json format
            wakeup();     // kick the action queue to notice the kill...
        }
        else {
            LOG(csmras, warning) << "Ras Action queueue overflow";
        }
    }
    catch (exception& e) {
        LOG(csmras, error) << __FUNCTION__ << ":" 
                           << __FILE__ << ":" 
                           << __LINE__ << " exception: " 
                           << e.what();
    }
}

void RasEventActionCtl::setMaxActions(unsigned maxActions) 
{
    _maxActions = maxActions; 
};
void RasEventActionCtl::setMaxThreads(unsigned maxThreads) 
{
    _maxThreads = maxThreads; 
};
unsigned RasEventActionCtl::getMaxActions() 
{
    return(_maxActions);
};

/**
 * Get the next action in the action queue, and optional wait 
 * for it via the workSemaphore... 
 * 
 * @param rasJsonData 
 * @param wait 
 * 
 * @return bool 
 */
bool RasEventActionCtl::getNextAction(std::string &rasJsonData, bool wait)
{
    boost::unique_lock<boost::mutex>  guard(_actionQueueMutex);
    if (wait) {
        if (_actionQueue.empty()) {
            LOG(csmras, info) << __FUNCTION__ << ":" 
                               << "_workSem.wait" ;
            _workSem.wait(guard);     // wait forever for work to arrive...
            LOG(csmras, info) << __FUNCTION__ << ":" 
                               << "wakeup" ;
        }
        if (_actionQueue.empty())       // even with the wait we may turn up empty...
            return(false);
        if (_runningThreads.size() > _maxThreads)
            return(false);
        rasJsonData = _actionQueue.front();
        _actionQueue.pop();
        guard.unlock();
        return(true);
    }
    else {
        if (_actionQueue.empty()) 
            return(false);
        if (_runningThreads.size() > _maxThreads)
            return(false);
        rasJsonData = _actionQueue.front();
        _actionQueue.pop();
        guard.unlock();
        return(true);
    }
}


/**
 * RunFatalAction -- run the fatal action script.
 * 
 * 
 * @param rasJsonData 
 */
void RasEventHandlerAction::runFatalAction(const RasEvent& event)
{
    if (_fatalScr.size() == 0)  //nothing to do here??
        return;                 

    LOG(csmras, info) << "FatalAction= " << _fatalScr << endl;

    RasEvent fevent = event;        // is there a proper copy constructor for this??

    // replace the control action with an immediate action...
    fevent.setValue(CSM_RAS_FKEY_IMMEDIATE_ACTION, _fatalScr);

    // and put it on the queue...
    _actionCtl.addRasActionQueue(fevent.getJsonData());



}

//#ifdef __linux
//LOG_DECLARE_FILE("ras");
//#endif

RasEvent& RasEventHandlerAction::handle(RasEvent& event)
{
    string szAction = event.getValue(CSM_RAS_FKEY_CONTROL_ACTION);
    string szImmedAction = event.getValue(CSM_RAS_FKEY_IMMEDIATE_ACTION);

    if (szImmedAction.size())
        szAction = szImmedAction;     // override with the immediate action if we have it...

    // nothing to do, just exit...
    if (szAction.size() == 0)
        return event;

    LOG(csmras, info) << "Action = " << szAction << endl;

    // todo, check and take any action script here...
    // where do we track down the configuration stuff??

    // we need a number of pieces of info here.
    // 1. base location of the action files to execute.
    // 2. where to send the script output (should it go to the main logging file, or, 
    //    somewhere else...
    // 3. Action timeout...  (time to wait before aborting the action script.., 
    //    in case of a hang...)..
    // 
    // action scripts, what should supervise the output?
    //                 a dedicated thread in this module, or something else..

    // what should the action model be here...??
    //    what to do with a ras storm.....???

    // we will limit the number of actions queued at any time and if we get more, we drop them on the floor
    // after logging the fact that we did that...
    _actionCtl.addRasActionQueue(event.getJsonData());

    return event;
}

RasEventHandlerAction::RasEventHandlerAction() :
    _name("RasEventHandlerAction"),
    _scriptDir("csmRasActionScripts"),   // temporary place holder for ras action scrpits...
    _fatalScr(),
    _logDir(),
    _maxActions(1000),                   // maximum actions to allow at any one time...
    _maxThreads(20),                     // maximum number of work threads to float...
    _actionTimeout(30),                  // 30 second action timeout value...
    _killActionThread(false),
    _actionCtl(_maxActions, _maxThreads),
    _actionThread()        // thread to execute the actions in...
{
    LOG(csmras, info) << "RasEventHandlerAction::RasEventHandlerAction()";
    // todo, get scriptDir and maxActions from config info area...

    // start the action thread....

    try {
        _actionThread = boost::thread(&RasEventHandlerAction::actionThread, this);
    }
    catch (exception& e) {
        LOG(csmras, error) << __FUNCTION__ << ":" 
                           << __FILE__ << ":" 
                           << __LINE__ << " exception: " 
                           << e.what();
    }

}

RasEventHandlerAction::~RasEventHandlerAction()
{
    // kill off the thread as we tear things down...
    killActionThread();
}

/**
 * actionThread -- worker thread to take the actions
 *                 for the given ras subsystem.
 * 
 */
void RasEventHandlerAction::actionThread() 
{
    while ( ! _killActionThread) {
        try {
            string rasJson;
            _actionCtl.cleanAllDoneThreads();      // do garbage collection...
            if (_actionCtl.getNextAction(rasJson, true)) {

                RasEvent event;
                event.setFromJsonData(rasJson);
    
                string scriptName = event.getValue(CSM_RAS_FKEY_CONTROL_ACTION);
                if ((scriptName.size() == 0) || boost::iequals(scriptName, CSM_RAS_NONE))    // toss it, nothing here...
                {
                    continue;
                }
    
                // where should this go from here?
                // alternative:
                //     -- invoke popen, capture output and leave here.
                //     -- send to a subscriber topic csmras/actions
                //     -- just rely on a master action subscriber to pay attention to the
                //     -- action stream....??
                // 
                // To keep the options open, the action dispatcher should probably be its own class.
                //    then we could wrap it with something that receives data from the mosquito 
                //    queue...
                shared_ptr<RasRunActionScriptThread> runScript(new RasRunActionScriptThread(_actionCtl));
                runScript->setTimeout(_actionTimeout);
                namespace fs = boost::filesystem;
                string bname = fs::basename(scriptName);
                LOG(csmras, info) << "RasEventHandlerAction::actionThread " << bname;
                string logfile;
                if (_logDir.size()) 
                    logfile = _logDir + "/";
                logfile += bname + ".log";
                runScript->setLogFile(logfile);
                if (scriptName[0] != '/')
                    runScript->setScriptDir(_scriptDir);
    
                // todo, getthis information into the syslog, the fact that we ran this...
                _actionCtl.addRunningThread(runScript);
                runScript->runThread(scriptName, rasJson);      // pass the json data into the script
    
            }
        }
        catch (exception& e) {
            LOG(csmras, error) << __FUNCTION__ << ":" 
                               << __FILE__ << ":" 
                               << __LINE__ << " exception: " 
                               << e.what();
        }

    }

}


/**
 * Kill off the action thread...
 * @param drain -- true wait for the thread to drain. 
 * @param timeout -- timeout to wait for the thread to drain 
 *                before killing it all...
 *                Any action not taken gets logged... 
 */
void RasEventHandlerAction::killActionThread(bool drain, unsigned timeout)
{
    try {
        // TODO, figure out how to drain the queue with a timeout...
        _killActionThread = true;
        _actionCtl.wakeup();        // how to do this under lock...

        _actionThread.join();
    }
    catch (exception& e) {
        LOG(csmras, error) << __FUNCTION__ << ":" 
                           << __FILE__ << ":" 
                           << __LINE__ << " exception: " 
                           << e.what();
    }
}

