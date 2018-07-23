/*================================================================================
   
    csmd/src/inv/include/inv_agent_handler.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#ifndef __INV_AGENT_HANDLER_H__
#define __INV_AGENT_HANDLER_H__

#include "csmd/src/daemon/src/csmi_request_handler/csmi_base.h"
#include "csmnet/src/CPP/csm_network_msg_cpp.h"
#include "csmnet/src/CPP/address.h"

#include <map>

class INV_AGENT_HANDLER : public CSMI_BASE {
 
public:
  // this handler not tied to any specific csmi cmd
  INV_AGENT_HANDLER(csm::daemon::HandlerOptions& options)
  : CSMI_BASE(CSM_CMD_UNDEFINED, options), _MsgId(0)
  {
    setCmdName(std::string("INV_AGENT_HANDLER"));
    _SystemEventContext = RegisterSystemEvent( this, csm::daemon::SystemContent::CONNECTED );
  }
  
public:

  virtual void Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList );
    
  virtual ~INV_AGENT_HANDLER()
  {
    UnregisterSystemEvent( _SystemEventContext, csm::daemon::SystemContent::CONNECTED );
  }
  
private:
  inline int GetMsgId() { return ++_MsgId; }
  
  virtual void CreateInventoryMsg( const csm::daemon::CoreEvent &aEvent );

  int _MsgId;

  csm::daemon::EventContext_sptr _SystemEventContext;

};

#endif

