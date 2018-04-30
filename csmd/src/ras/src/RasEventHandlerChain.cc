/*================================================================================

    csmd/src/ras/src/RasEventHandlerChain.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <tuple>


#include "../include/RasEventHandlerChain.h"
#include "../include/RasEventHandlerAction.h"
#include "../include/RasEventHandlerVarSub.h"
#include "../include/RasSmartLock.h"

#include "logging.h"
#include "csm_api_ras.h"



//#include "MetadataAdder.h"
//#include "VarSubstitute.h"
//#include "RasDecoder.h"

using namespace std;


RasEventHandlerChain::RasEventHandlerChain() 
{
    _chainLock = PTHREAD_MUTEX_INITIALIZER;
    _env = "PROD";
    _initialized = false;
    _csmConfig = nullptr;

}

RasEventHandlerChain::~RasEventHandlerChain() 
{
    clear();
}

void RasEventHandlerChain::setEnvironment(string e) { 
    _env = e; 
    LOG(csmras,trace) << "RasEventHandlerChain environment=" << _env.c_str();
}
string RasEventHandlerChain::getEnvironment() { 
    return _env; 
}


void RasEventHandlerChain::setCsmConfig(boost::property_tree::ptree *csmConfig)
{
    _csmConfig = csmConfig;

    // todo, pass the configuration on to the handlers that are interested in the data...
}




RasRc RasEventHandlerChain::handle(RasEvent& event)
{  
    initChain();
    // only handle the event once 
    if (event.handled()) return(RasRc(CSMI_SUCCESS));

    RasSmartLock lock(&_chainLock);

    try {
        _handleVarSub->handle(event);
        _handleAction->handle(event);
        event.setHandled(true);
    } catch (exception& e) {
        LOG(csmras,error) << "RasEventHandlerChain handler " << " failed. " << e.what();
        return(RasRc(CSMERR_RAS_HANDLER_ERROR, e.what()) );
    }
    return(RasRc(CSMI_SUCCESS));
}

using namespace std;
void RasEventHandlerChain::initChain() {
    RasSmartLock lock(&_chainLock);

    if (_initialized) return;

    // Clear prior storage and handlers, in case this is not the first time it is being called.
    clear_l();

    _handleVarSub.reset(new RasEventHandlerVarSub());
    _handleAction.reset(new RasEventHandlerAction());

    if (_csmConfig) {
        std::string scriptDir = _csmConfig->get<string>("csm.ras.action.scriptdir", std::string( "./default_scriptdir" ) );
        std::string logDir = _csmConfig->get<string>("csm.ras.action.logdir", std::string( "./defaultlog_dir" ) );
        unsigned maxActions = _csmConfig->get<unsigned>("csm.ras.action.maxactions", 100);
        unsigned actionTimeout = _csmConfig->get<unsigned>("csm.ras.action.timeout", 20);
        #if 0
        LOG(csmras, info) << "RasEventHandlerChain scriptDir = " << scriptDir;
        LOG(csmras, info) << "RasEventHandlerChain logDir = " << logDir;
        LOG(csmras, info) << "RasEventHandlerChain maxActions = " << maxActions;
        LOG(csmras, info) << "RasEventHandlerChain actionTimeout = " << actionTimeout;
        #endif
        _handleAction->setScriptDir(scriptDir);
        _handleAction->setLogDir(logDir);
        _handleAction->setMaxActions(maxActions);
        _handleAction->setActionTimeout(actionTimeout);

    }
    else {
        LOG(csmras, error) << "RasEventHandlerChain missing csmConfig";
    }
    // The variable substitution handler is last
    //    _handlers.push_back(new RasEventHandlerVarSub());
    // last handler in the chain is the Action handler..
    //    _handlers.push_back(new RasEventHandlerAction());

    _initialized = true;
}


void RasEventHandlerChain::clear() 
{
    RasSmartLock lock(&_chainLock);
    //Clear all memory allocated during initChain()
    clear_l();
}
void RasEventHandlerChain::clear_l() 
{
    #if 0
    for (unsigned i=0; i<_handlers.size(); ++i) {
        delete _handlers[i];
    }
    _handlers.clear();
    #endif
    _initialized = false; 
}

void RasEventHandlerChain::reinitialized() { 
    RasSmartLock lock(&_chainLock);
    _initialized = false; 
} 

