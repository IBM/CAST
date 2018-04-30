/*================================================================================

    csmd/src/ras/include/RasEventHandlerChain.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __RAS_EVENT_HANDLER_CHAIN_H__
#define __RAS_EVENT_HANDLER_CHAIN_H__


#include <vector>
#include <string>
#include <pthread.h>
#include <boost/property_tree/ptree.hpp>

#include "RasRc.h"
#include "RasEvent.h"
#include "RasEventHandler.h"
#include "RasEventHandlerAction.h"
#include "RasEventHandlerVarSub.h"


/** 
 * \class RasEventHandlerChain
 *
 * RasEventHandlerChain is responsible for passing a RasEvent 
 * to a series of RasEventHandlers.  Singleton class, everything 
 * is static... 
 */

class RasEventHandlerChain
{
public:
    RasEventHandlerChain();
    ~RasEventHandlerChain();
    /** \brief Pass the RasEvent event to a series of handlers.
     */
    RasRc handle(RasEvent& event);
    void setEnvironment(std::string env);
    std::string getEnvironment();
    void setCsmConfig(boost::property_tree::ptree *config);
    void reinitialized();
    void clear();
protected:
    void initChain();
#ifdef __do_this_later__
    void setProperties(const bgq::utility::Properties::ConstPtr properties);
    bgq::utility::Properties::ConstPtr getProperties();
#endif
    void clear_l();
private:
    pthread_mutex_t _chainLock; // Sync access to chain initialization
    bool _initialized;
    std::shared_ptr<RasEventHandlerVarSub> _handleVarSub;
    std::shared_ptr<RasEventHandlerAction> _handleAction;
    std::string _env;

    boost::property_tree::ptree *_csmConfig;       // pointer to csm configuration.
};





#endif



