/*================================================================================
   
    csmd/src/inv/src/inv_aggregator_handler.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
// implement the CSM api node attributes command...
//
#include "inv_aggregator_handler.h"
#include "include/csm_event_type_definitions.h"

#include "logging.h"
#include "inv_full_inventory.h"
#include "inv_get_node_inventory_serialization.h"

#include "csm_api_ras_keys.h"

#include "csmi/include/csmi_type_wm.h"

void INV_AGGREGATOR_HANDLER::NodeConnectRasEvent(const std::string &nodeName,
                                                 std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  // Disabling before removal
  return;

  csm::daemon::NetworkEvent* ne = CreateRasEventMessage(CSM_RAS_MSG_ID_STATUS_UP,    // &msg_id, 
                                                        nodeName,                    // &location_name,
                                                        "",                          // &raw_data,
                                                        "");                         // &kvcsv)

  if (ne)
    postEventList.push_back(ne);
}

void INV_AGGREGATOR_HANDLER::NodeDisconnectRasEvent(csm::network::Address_sptr addr,
                                                    std::vector<csm::daemon::CoreEvent*>& postEventList )
{
    // Disabling before removal
    return;
    
    std::string nodeName = _DaemonState->GetCNUidFromAddr(addr);
    LOG(csmapi,info) << "_DaemonState->GetCNUidFromAddr() returned: " << nodeName;
    csm::daemon::NetworkEvent* ne = CreateRasEventMessage(CSM_RAS_MSG_ID_STATUS_DOWN,  // &msg_id, 
                                                          nodeName,                    // &location_name,
                                                          "",                          // &raw_data,
                                                          "");                         // &kvcsv)

    if (ne)
      postEventList.push_back(ne);

}

std::string INV_AGGREGATOR_HANDLER::GetUidForCN(const csm::network::Message &msg)
{
  csm_full_inventory_t inventory;
  uint32_t bytes_unpacked(0);

  bytes_unpacked = get_node_inventory_unpack(msg.GetData(), inventory);
  if (bytes_unpacked == 0)
  {
    LOG(csmapi,error) << "GetUidForCN: unpack failed...";
  }
  else
  {
    LOG(csmapi,info) << "GetUidForCN: unpacking node name: " << inventory.node.node_name;
  }

  return std::string(inventory.node.node_name);
}

bool INV_AGGREGATOR_HANDLER::AddInventory(const csm::network::MessageAndAddress &content)
{
  // construct the unique key (src_msgId);
  std::string uid = GetUidForCN(content._Msg);
    
  LOG(csmd, info) << "INV_AGGREGATOR_HANDLER::AddInventory(): uid = " << uid;

  _DaemonState->AddInventory( uid, content );
  return true;
}

void INV_AGGREGATOR_HANDLER::SendInventory( const csm::network::Message &invMsg,
                                            const csm::daemon::EventContext_sptr context,
                                            std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  csm::network::MessageAndAddress content(invMsg, _AbstractMaster);
  content._Msg.SetMessageID(0);
  content._Msg.CheckSumUpdate();
  postEventList.push_back( new csm::daemon::NetworkEvent(content, csm::daemon::EVENT_TYPE_NETWORK, context) );
}

void INV_AGGREGATOR_HANDLER::SendAllInventory( const csm::daemon::CoreEvent &aEvent, std::vector<csm::daemon::CoreEvent*>& postEventList )
{

  std::vector< const csm::network::Message * > known_cn_list;
  _DaemonState->GetAllActiveInventory(known_cn_list);

  // borrow the Uid in context to record number of CN nodes
  csm::daemon::EventContext_sptr context( new csm::daemon::EventContext( this, known_cn_list.size() ) );

  // Send the local inventory
  if (GetDaemonState()->HasLocalInventory() == false )
  {
    LOG(csmd, error) << "INV_AGGREGATOR_HANDLER::SendAllInventory(): no local inventory has been collected.";
  }
  else
  {
    LOG(csmd, info) << "INV_AGGREGATOR_HANDLER::SendAllInventory(): Sending local inventory to Master";
    SendInventory( GetDaemonState()->GetLocalInventory(), context, postEventList);
  }

  // Send inventory for the compute nodes we are aggregating
  for( auto & it : known_cn_list )
  {
    SendInventory( *it, context, postEventList);
  }
    
  LOG(csmd, info) << "INV_AGGREGATOR_HANDLER::SendAllInventory(): Sending " << known_cn_list.size()  << " node inventory to Master";
}

void INV_AGGREGATOR_HANDLER::CheckReplyFromMaster(const csm::daemon::CoreEvent &aEvent)
{
   csm::daemon::EventContext_sptr context = aEvent.GetEventContext();
    
   if (context == nullptr) 
   {
     LOG(csmd, warning) << "INV_AGGREGATOR_HANDLER::CheckReplyFromMaster(): unexpectedly processing reply with context=0";
     return;
   }    

  // now inspect the network packet
  csm::daemon::NetworkEvent *ev = (csm::daemon::NetworkEvent *)&aEvent;
  csm::network::Message msg = ev->GetContent()._Msg;
    
  if (msg.GetErr())
  {
    LOG(csmd, error) << "INV_AGGREGATOR_HANDLER::CheckReplyFromMaster(): Got an Error from Master";
    csmi_err_t *err = csmi_err_unpack(msg.GetData().c_str(), msg.GetDataLen());
    if (err)
    {
      LOG(csmd, error) << "      errcode=" << err->errcode << " errmsg=" << err->errmsg;
      csmi_err_free(err);
    }
  }
  else
  {
    LOG(csmd, debug) << "INV_AGGREGATOR_HANDLER::CheckReplyFromMaster(): Got a reply from Master";
  }

  return;
}

void INV_AGGREGATOR_HANDLER::Process( const csm::daemon::CoreEvent &aEvent, std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  std::unique_lock<std::mutex> guard(_handlerLock);

  // if the local inventory doesn't exist, go create it
  if( GetDaemonState()->HasLocalInventory() == false )
  {
    LOG(csmd, info) << "INV_AGGREGATOR_HANDLER: Creating initial aggregator inventory.";
    CreateLocalInventory( aEvent );
  }

  // we've never created the inventory, so lets get that done without sending it
  if(aEvent.GetEventType() == csm::daemon::EVENT_TYPE_INITIAL)
  {
    // nothing to do on initial if the inventory was created
  }
  else if ( isSystemEvent( aEvent ) )
  {
    switch( getSystemEventType( aEvent ) )
    {
      case csm::daemon::SystemContent::CONNECTED:
      {
        csm::network::Address_sptr peer =  GetAddrFromSystemEvent( aEvent );
        switch( peer->GetAddrType() )
        {
          case csm::network::CSM_NETWORK_TYPE_AGGREGATOR:
            SendAllInventory( aEvent, postEventList );
            break;

          case csm::network::CSM_NETWORK_TYPE_PTP:
            // connectEP is done by connection handling. Handlers shouldn't modify the daemonstate
            // GetDaemonState()->ConnectEP( peer );
            break;

            // don't do anything for the other connection types.
            // Any actions depend on actual network msgs from the peers
          case csm::network::CSM_NETWORK_TYPE_LOCAL:
          default:
            break;
        }
        break;
      }

      case csm::daemon::SystemContent::DISCONNECTED:
      {
        const csm::network::Address_sptr addr = GetAddrFromSystemEvent(aEvent);
        csm::daemon::ConnectedNodeStatus *nInfo = GetDaemonState()->GetNodeInfo( addr );

        LOG(csmd, info) << "INV_AGGREGATOR_HANDLER: detected "  << nInfo->_ConnectionType
            << " connection failure. Type: " << addr->GetAddrType()
            << " Addr: " << addr->Dump();
        if(( addr->GetAddrType() == csm::network::CSM_NETWORK_TYPE_PTP ) &&
            (nInfo != nullptr ) &&
            (nInfo->_ConnectionType != csm::daemon::ConnectionType::SECONDARY ))  // only send disconnect for primary/single connections
        {
          NodeDisconnectRasEvent(addr, postEventList);
        }
        break;
      }

      case csm::daemon::SystemContent::RESTARTED:  // only happens if a compute sends the "Fresh start" message
      {
        csm::network::Address_sptr peer =  GetAddrFromSystemEvent( aEvent );
        if( peer == nullptr )
        {
          LOG( csmd, warning ) << "INV_AGGREGATOR_HANDLER: RESTARTED system event without peer address.";
          break;
        }
        csm::daemon::ConnectedNodeStatus *nInfo = GetDaemonState()->GetNodeInfo( peer );

        if( nInfo == nullptr )
        {
          LOG( csmd, warning ) << "INV_AGGREGATOR_HANDLER: Received RESTARTED signal from untracked compute " << peer->Dump() << ". Ignoring...";
          break;
        }
        if( nInfo->NeedSendInventory() )
        {
          NodeConnectRasEvent( nInfo->_NodeID, postEventList);
          SendInventory( nInfo->_LastInventory,
                         CreateContext((void *)this, 0),
                         postEventList);
          LOG(csmd, info) << "INV_AGGREGATOR_HANDLER: Sending a new node inventory (" << nInfo->_NodeID << ") to Master";
        }
        else
        {
          nInfo->EnableSendInventory();
          LOG( csmd, debug ) << "INV_AGGREGATOR_HANDLER: hit RESTARTED event without available inventory.";
        }

        break;
      }

      case csm::daemon::SystemContent::FAILOVER:
      {
//        csm::network::Address_sptr peer =  GetAddrFromSystemEvent( aEvent );
//        GetDaemonState()->SetConnectionTypeEP( peer, csm::daemon::ConnectionType::PRIMARY );
        // never send the inventory just because of a failover, it has already been sent at restart
        break;
      }

      default:
        LOG( csmd, warning ) << "INV_AGGREGATOR_HANDLER: unexpected system event type.";
    }
  }
  else if( isNetworkEvent( aEvent ) )
  {
    // now inspect the network packet
    csm::daemon::NetworkEvent *ev = (csm::daemon::NetworkEvent *)&aEvent;
    csm::network::MessageAndAddress content = ev->GetContent();
    csm::network::Message msg = content._Msg;
    csm::network::Address_sptr addr = content.GetAddr();

    LOG(csmd, debug) << "INV_AGGREGATOR_HANDLER processing addr type: " << addr->GetAddrType()
                     << ", command type: " << csm::network::cmd_to_string( msg.GetCommandType() );

    switch( addr->GetAddrType() )
    {
      case csm::network::CSM_NETWORK_TYPE_AGGREGATOR:
        if( msg.GetResp() )
          CheckReplyFromMaster(aEvent);  // response from master
        else
          SendAllInventory(aEvent, postEventList); // request from master
        break;

      case csm::network::CSM_NETWORK_TYPE_PTP:
        if( msg.GetResp() )
        {
          LOG( csmd, error ) << "INV_AGGREGATOR_HANDLER: Response from compute. Should never happen.";
        }
        else
        {
          AddInventory(content);
          csm::daemon::ConnectedNodeStatus *nInfo = GetDaemonState()->GetNodeInfo( addr );
          if( nInfo == nullptr )
          {
            LOG( csmd, warning ) << "INV_AGGREGATOR_HANDLER: Received message from untracked compute " << addr->Dump() << ". Ignoring...";
            break;
          }

          // need to send the inventory if the restarted signal happened first
          if( nInfo->NeedSendInventory() )
          {
            LOG( csmd, debug ) << "INV_AGGREGATOR_HANDLER: RESTART signal happened before inventory msg. Need to send now.";
            NodeConnectRasEvent( GetUidForCN(msg), postEventList);
            SendInventory( content._Msg,
                           CreateContext((void *)this, 0),
                           postEventList);
            LOG(csmd, info) << "INV_AGGREGATOR_HANDLER: Sending a new node inventory (" << nInfo->_NodeID << ") to Master";
          }
          else
            nInfo->EnableSendInventory();
        }
        break;
      default:
        LOG(csmd, warning) << "INV_AGGREGATOR_HANDLER: Unknown Address Type";
    }
  }
  else
  {
    LOG(csmd,error) << "INV_AGGREGATOR_HANDLER: Received unhandled event type: " << EventTypeToString(aEvent.GetEventType());
  }
}

void INV_AGGREGATOR_HANDLER::CreateLocalInventory( const csm::daemon::CoreEvent &aEvent )
{
  LOG(csmd, trace) << "Enter " << __PRETTY_FUNCTION__; 

  string node_type("");
  
  // Set node type variables based on the daemon role 
  CSMDaemonRole daemon_role = _handlerOptions.GetRole();
  if (daemon_role == CSM_DAEMON_ROLE_AGGREGATOR)
  {
    node_type = csm_get_string_from_enum(csmi_node_type_t, CSM_NODE_AGGREGATOR);
  }
  else
  {
    LOG(csmd, error) << "INV_AGGREGATOR_HANDLER: Unexpected daemon role. Stopping inventory collection.";
    return;
  }
  
  LOG(csmd, info) << "INV_AGGREGATOR_HANDLER: Collecting " << node_type << " inventory.";
  csm_full_inventory_t inventory;
  
  bool rc(false);  
  rc = GetFullInventory(inventory);
  if (rc == false) 
  {
    LOG(csmd, error) << "INV_AGGREGATOR_HANDLER: GetFullInventory returned false.";
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
    LOG(csmd, error) << "INV_AGGREGATOR_HANDLER: Failed to pack inventory. Stopping inventory collection.";
    return;
  }

  LOG(csmd, info) << "INV_AGGREGATOR_HANDLER: Node Inventory payload_str.size() = " << payload_str.size();
  
  csm::network::Message msg;
  bool hdrvalid = msg.Init (CSM_CMD_INV_get_node_inventory,
                               0, //flag
                               CSM_PRIORITY_WITH_ACK, //priority
                               0, //msg id
                               LOCAL_DAEMON,
                               MASTER,
                               geteuid(), getegid(),
                               payload_str);
  if (!hdrvalid)
  {  
    LOG(csmd, error) << "INV_AGGREGATOR_HANDLER: Failed to initialize a Message";
  }

  // Store the local inventory for future use
  GetDaemonState()->SetLocalInventory( msg );
    
  LOG(csmd, trace) << "Exit " << __PRETTY_FUNCTION__; 
}
