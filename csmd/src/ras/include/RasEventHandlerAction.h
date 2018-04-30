/*================================================================================

    csmd/src/ras/include/RasEventHandlerAction.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __RASEVENT_HANDLER_ACTION_H__
#define __RASEVENT_HANDLER_ACTION_H__

#include <boost/thread.hpp>
#include <set>
#include <string>
#include <queue>


#include "RasEventHandler.h"
#include "RasRunActionScript.h"

class RasRunActionScriptThread;

class RasEventActionCtl
{
public:
    RasEventActionCtl(unsigned maxActions, unsigned maxThreads);
    void addRasActionQueue(const std::string &rasJsonData);
    bool getNextAction(std::string &rasJsonData, bool wait = false);
    void setMaxActions(unsigned maxActions);
    void setMaxThreads(unsigned maxThreads);

    unsigned getMaxActions();
    void wakeup();

    void addRunningThread(std::shared_ptr<RasRunActionScriptThread> thread);
    void threadDone(RasRunActionScriptThread  *thread);
    void removeThread(RasRunActionScriptThread *thread);

    void cleanAllDoneThreads();

    unsigned getRunningThreads();

    void joinAllThreads();


protected:
    unsigned _maxActions;
    unsigned _maxThreads;
    boost::mutex _actionQueueMutex;
    boost::condition_variable _workSem;
    std::queue<std::string> _actionQueue;
    std::set<RasRunActionScriptThread *> _runningThreads;
    std::set<RasRunActionScriptThread *> _finishedThreads;

    std::map<RasRunActionScriptThread *, std::shared_ptr<RasRunActionScriptThread> > _theadPtrMap;

private:

};


class RasRunActionScriptThread : public RasRunActionScript
{
public:
    RasRunActionScriptThread(RasEventActionCtl &actionCtl) :
        _actionCtl(actionCtl),
        _threadDone(false),
        _rc(0) {};
    virtual ~RasRunActionScriptThread();
    bool isThreadDone() {
        return(_threadDone); };
    int getReturnCode() {
        return(_rc); };


    void runThread(const std::string &script, const std::string &arg);

    void actionThread();
protected:
    RasEventActionCtl &_actionCtl;
    bool _threadDone;
    std::string _script;
    std::string _arg;
    boost::thread _thread;        // this thread to execute..
    int _rc;


};


class RasEventHandlerAction : public RasEventHandler
{
public:
    virtual RasEvent& handle(RasEvent& event);
    virtual const std::string& name() { return _name; }
    RasEventHandlerAction();
    virtual ~RasEventHandlerAction();
    
    void setScriptDir(const std::string &scriptDir) {_scriptDir = scriptDir;};
    void setFatalScript(const std::string &fatalScr) {_fatalScr = fatalScr;};
    void setLogDir(const std::string &logDir) {_logDir = logDir; };
    void setMaxActions(unsigned maxActions) {_maxActions = maxActions; _actionCtl.setMaxActions(maxActions); };
    void setActionTimeout(unsigned actionTimeout) {_actionTimeout = actionTimeout;};
    void setMaxThreads(unsigned maxThreads) {_maxThreads = maxThreads; _actionCtl.setMaxThreads(maxThreads); };

    const std::string &getScriptDir() {return(_scriptDir);};
    const std::string &getFatalScript() { return(_fatalScr);};
    unsigned getMaxActions() {return(_maxActions);};

    void actionThread();

    void runFatalAction(const RasEvent& event);

protected:

    std::string _name;
    std::string _scriptDir;
    std::string _fatalScr;
    std::string _logDir;
    unsigned _maxActions;
    unsigned _maxThreads;
    unsigned _actionTimeout;
    bool _killActionThread;
    RasEventActionCtl _actionCtl;
    boost::thread    _actionThread;        // thread to execute the actions in...


    void killActionThread(bool drain = 0, unsigned timeout = 0);

    std::string getScriptLogfileName(const std::string &scriptName);

private:
};

#endif /*VARSUBSTITUTE_H_*/

