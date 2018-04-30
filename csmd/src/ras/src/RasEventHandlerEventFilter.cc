/*================================================================================

    csmd/src/ras/src/RasEventHandlerEventFilter.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "../include/RasEventHandlerEventFilter.h"

#include "logging.h"


using namespace std;


RasEvent& RasEventHandlerEventFilter::handle(RasEvent& event)
{
    //LOG_TRACE_MSG("EventFilter handling event id=" <<  event.getDetail(RasEvent::MSG_ID).c_str() << " msg=" <<  event.getDetail(RasEvent::MESSAGE).c_str());
    // TRACE(("EventFilter handling event id=%s msg=%s \n", event.getDetail(RasEvent::MSG_ID).c_str(),  event.getDetail(RasEvent::MESSAGE).c_str()));


    LOG(csmras,info) << "RasEventHandlerEventFilter event id " << event.msg_id();

    string msg_id = event.msg_id();
    for (RasEventChangeSpecList::RasEventSpecVect::iterator iter = _specs.begin(); 
         iter != _specs.end(); iter++) {
        LOG(csmras,info) << "iter->msg_id() = " << iter->msg_id() ;
        if (msg_id != iter->msg_id())
            continue;
        LOG(csmras,info) << "RasEventHandlerEventFilter event id " 
                  << event.msg_id() << " setValue(" 
                  << iter->key() << ", " << iter->value() 
                  << ")";
        event.setValue(iter->key(), iter->value());
    }

    return(event);

}
RasEventHandlerEventFilter::RasEventHandlerEventFilter(const RasEventChangeSpecList& specs) :
    _name("RasEventHandlerEventFilter"),
    _specs(specs)
    
{
    
}
RasEventHandlerEventFilter::~RasEventHandlerEventFilter() {

}
