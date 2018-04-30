/*================================================================================
   
    csmd/src/inv/include/inv_aggregator_handler.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#ifndef __INV_AGGREGATOR_HANDLER_H__
#define __INV_AGGREGATOR_HANDLER_H__

#include "csmd/src/daemon/src/csmi_request_handler/csmi_base.h"
#include "csmnet/src/CPP/csm_network_msg_cpp.h"
#include "csmnet/src/CPP/address.h"

#include <map>

#include <boost/thread.hpp>

class INV_AGGREGATOR_HANDLER : public CSMI_BASE {

public:
  INV_AGGREGATOR_HANDLER(csm::daemon::HandlerOptions& options)
  : CSMI_BASE(CSM_CMD_UNDEFINED, options)
  {
    setCmdName(std::string("INV_AGGREGATOR_HANDLER"));
    // here, we must have a DaemonStateAgg object as this handler can only be called from Agg.
    _DaemonState = (csm::daemon::DaemonStateAgg *) options.GetDaemonState();
    _SystemEventContext = RegisterSystemEvent( this, csm::daemon::SystemContent::DISCONNECTED );
    RegisterSystemEvent( _SystemEventContext, csm::daemon::SystemContent::CONNECTED );
    RegisterSystemEvent( _SystemEventContext, csm::daemon::SystemContent::RESTARTED );
    RegisterSystemEvent( _SystemEventContext, csm::daemon::SystemContent::FAILOVER );
  }
  
public:

  virtual void Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList );
    
  virtual ~INV_AGGREGATOR_HANDLER()
  {
    UnregisterSystemEvent( _SystemEventContext, csm::daemon::SystemContent::CONNECTED );
    UnregisterSystemEvent( _SystemEventContext, csm::daemon::SystemContent::DISCONNECTED );
    UnregisterSystemEvent( _SystemEventContext, csm::daemon::SystemContent::RESTARTED );
    UnregisterSystemEvent( _SystemEventContext, csm::daemon::SystemContent::FAILOVER );
  }
  
private:
  void NodeConnectRasEvent(const std::string &nodeName, std::vector<csm::daemon::CoreEvent*>& postEventList );
  void NodeDisconnectRasEvent(csm::network::Address_sptr addr,std::vector<csm::daemon::CoreEvent*>& postEventList );
  inline std::string GetUidForCN(const csm::network::Message &msg);
  bool AddInventory(const csm::network::MessageAndAddress &content);

  void SendInventory( const csm::network::Message &invMsg,
                      const csm::daemon::EventContext_sptr context,
                      std::vector<csm::daemon::CoreEvent*>& postEventList );

  // Used to send the local inventory from this daemon
  void CreateLocalInventory( const csm::daemon::CoreEvent &aEvent );
 
  void SendAllInventory( const csm::daemon::CoreEvent &aEvent, std::vector<csm::daemon::CoreEvent*>& postEventList );
  void CheckReplyFromMaster(const csm::daemon::CoreEvent &aEvent);

  std::string _mqttReqTopic, _mqttReplyTopic;
  csm::daemon::DaemonStateAgg* _DaemonState;

  csm::daemon::EventContext_sptr _SystemEventContext;
};

#endif

