/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csm_conn_ctrl_handler.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSM_DAEMON_SRC_CSM_CONN_CTRL_HANDLER_H__
#define __CSM_DAEMON_SRC_CSM_CONN_CTRL_HANDLER_H__

#include "csmi_init_handler_base.h"
#include "csmnet/src/CPP/csm_network_msg_cpp.h"
#include "csmnet/src/CPP/address.h"


class CONN_CTRL_HANDLER : public CSMI_INIT_HANDLER_BASE {
 
public:
  // this handler not tied to any specific csmi cmd
  CONN_CTRL_HANDLER(csm::daemon::HandlerOptions& options)
  : CSMI_INIT_HANDLER_BASE(options)
  {
    setCmdName(std::string("CONN_CTRL_HANDLER"));
    _SystemEventContext = RegisterSystemEvent( this, csm::daemon::SystemContent::CONNECTED );
  }
  
public:

  virtual void Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList );
    
  virtual ~CONN_CTRL_HANDLER()
  {
    UnregisterSystemEvent( _SystemEventContext, csm::daemon::SystemContent::CONNECTED );
  }

private:
  csm::daemon::EventContext_sptr _SystemEventContext;

};

#endif

