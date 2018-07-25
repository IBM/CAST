/*================================================================================
   
    csmd/src/inv/src/inv_agent_handler.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

#include "inv_agent_handler.h"
#include "logging.h"
#include "inv_full_inventory.h"
#include "inv_get_node_inventory_serialization.h"
#include "csmi/include/csmi_type_wm.h"

#include <string>

using std::string;

void INV_AGENT_HANDLER::Process( const csm::daemon::CoreEvent &aEvent, std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  LOG(csmd, trace) << "Enter " << __PRETTY_FUNCTION__; 

  /* main behavior:
   if localInv is not initialized
      register for connects
      create inventory

   if init event:
      do nothing but the above init
   if connectEvent:
      send inventory
   if networkEvent
      process as a response
   */

  // protect/order the race between init and connect event
  // first entry does the init; second entry will block
  std::lock_guard<std::mutex> guard( _handlerLock );

  // if the local inventory doesn't exist, go create it
  if( ! GetDaemonState()->HasLocalInventory() )
  {
    LOG(csmd, info) << "INV_AGENT_HANDLER: Creating initial node inventory.";
    CreateInventoryMsg( aEvent );
  }

  // we've never created the inventory, so lets get that done without sending it
  if(aEvent.GetEventType() == csm::daemon::EVENT_TYPE_INITIAL)
  {
    // nothing to do on initial if the inventory was created and dcgm initialized
  }
  else if ( isSystemEvent( aEvent ) )
  {
    if( isSystemConnectedEvent( aEvent ) )
    {
      csm::daemon::SystemContent sysContent = dynamic_cast<const csm::daemon::SystemEvent *>( &aEvent)->GetContent();
      LOG( csmd, info ) << "INV_AGENT_HANDLER: CONNECT event. Re-/sending node inventory to: " << sysContent.GetAddr()->Dump();
      postEventList.push_back( CreateNetworkEvent( GetDaemonState()->GetLocalInventory(), sysContent.GetAddr(), nullptr ) );
    }
  }
  else if ( aEvent.GetEventType() == csm::daemon::EVENT_TYPE_NETWORK)
  {
    csm::network::Message msg = GetNetworkMessage(aEvent);
    if (msg.GetErr())
    {
      csmi_err_t *err = csmi_err_unpack(msg.GetData().c_str(), msg.GetDataLen());
      if (err)
      {
        if (err->errcode == ETIMEDOUT)
        {
          // the error is ack timeout. Will get here if the Master is not up, resend until the master comes up.
          LOG(csmd, warning) << "INV_AGENT_HANDLER: Got an ACK timeout. Resending the inventory.";
          postEventList.push_back( CreateNetworkEvent( GetDaemonState()->GetLocalInventory(),
                                                       GetNetworkAddress(aEvent),
                                                       nullptr ) );
        }
        else
        {
          LOG(csmd, error) << "INV_AGENT_HANDLER: Received error response, errcode=" << err->errcode << " errmsg=" << err->errmsg;
        }
        csmi_err_free(err);
      }
      else
      {
        LOG(csmd, error) << "INV_AGENT_HANDLER: Failed to unpack error response message";
      }
    }
    else
    {
      // the inventory handler at Master (reuse db_base Process) would send out a response without payload
      LOG(csmd, info) << "INV_AGENT_HANDLER: Got an inventory response from the Master.";
    }
  }

  LOG(csmd, trace) << "Exit " << __PRETTY_FUNCTION__; 
}

void INV_AGENT_HANDLER::CreateInventoryMsg( const csm::daemon::CoreEvent &aEvent )
{
  LOG(csmd, trace) << "Enter " << __PRETTY_FUNCTION__; 

  string node_type("");
  
  // Set node type variables based on the daemon role 
  CSMDaemonRole daemon_role = _handlerOptions.GetRole();
  if (daemon_role == CSM_DAEMON_ROLE_AGENT)
  {
    node_type = csm_get_string_from_enum(csmi_node_type_t, CSM_NODE_COMPUTE);
  }
  else if (daemon_role == CSM_DAEMON_ROLE_UTILITY)
  {
    node_type = csm_get_string_from_enum(csmi_node_type_t, CSM_NODE_UTILITY);
  }
  else
  {
    LOG(csmd, error) << "INV_AGENT_HANDLER: Unexpected daemon role. Stopping inventory collection.";
    return;
  }
  
  LOG(csmd, info) << "INV_AGENT_HANDLER: Collecting " << node_type << " inventory.";
  csm_full_inventory_t inventory;
  
  bool rc(false);  
  rc = GetFullInventory(inventory);
  if (rc == false) 
  {
    LOG(csmd, error) << "INV_AGENT_HANDLER: GetFullInventory returned false.";
    return;
  }

  // Set the type string in the inventory structure
  strncpy(inventory.node.type, node_type.c_str(), CSM_TYPE_MAX);
  inventory.node.type[CSM_TYPE_MAX - 1] = '\0';
  
  string payload_str("");
  uint32_t bytes_packed(0);
  bytes_packed = get_node_inventory_pack(inventory, payload_str); 

  if ( bytes_packed == 0 || payload_str.size() == 0 ) 
  {
    LOG(csmd, error) << "INV_AGENT_HANDLER: Failed to pack inventory. Stopping inventory collection.";
    return;
  }

  LOG(csmd, info) << "INV_AGENT_HANDLER: Node Inventory payload_str.size() = " << payload_str.size();
 
  csm::network::Message msg;
  bool hdrvalid = msg.Init (CSM_CMD_INV_get_node_inventory,
                               0, //flag
                               CSM_PRIORITY_WITH_ACK, //priority
                               0, //msg id
                               LOCAL_DAEMON,
                               //AGGREGATOR,
                               MASTER,
                               geteuid(), getegid(),
                               payload_str);
  if (!hdrvalid)
  {  
    LOG(csmd, error) << "INV_AGENT_HANDLER: Failed to initialize a Message";
  }

  // Save the context for this event 
  csm::network::MessageAndAddress content_response( msg, _AbstractBroadcast);

  // do not actually send this msg.
//  postEventList.push_back( CreateNetworkEvent(content_response, CreateContext((void *)this, 0)) );
 
  GetDaemonState()->SetLocalInventory( msg );

  LOG(csmd, trace) << "Exit " << __PRETTY_FUNCTION__; 
}
