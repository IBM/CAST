/*================================================================================
   
    csmd/src/daemon/src/csmi_request_handler/csm_infrastructure_test_agent.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

#include "csm_infrastructure_test.h"

void CSM_INFRASTRUCTURE_TEST_AGENT::Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  if ( isSystemEvent(aEvent) )
  {
    TestSystemEvent(aEvent);
    return;
  }
  
  if ( !isNetworkEvent(aEvent) )
  {
    LOG(csmd, warning) << "CSM_INFRASTRUCTURE_TEST_AGENT: Expecting a NetworkEvent...";
    return;
  }
  
  csm::network::Address_sptr addr = GetNetworkAddress(aEvent);
  if(( addr == nullptr ) || ( addr->GetAddrType() != csm::network::CSM_NETWORK_TYPE_PTP ))
  {
    LOG(csmd, warning) << "CSM_INFRASTRUCTURE_TEST_AGENT: Only accepting messages from Aggregators.";
    return;
  }
  
  csm::daemon::EventContext_sptr ctx = aEvent.GetEventContext();
  if (ctx == nullptr)
    ctx = std::make_shared<csm::daemon::EventContext>(this, INITIAL_STATE, CopyEvent(aEvent));

  LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_AGENT: TEST_SETUP = " << ctx->GetAuxiliaryId();
  
  if (ctx->GetAuxiliaryId() == INITIAL_STATE)
  {
    csm::daemon::Configuration *config = csm::daemon::Configuration::Instance();

    // determine which (configured) aggregator has sent the message
    int aggr = 0;
    LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_AGENT: Looking up msgAddr: " << addr->Dump();
    csm::daemon::ConnectionType::CONN_TYPE conn_type;
    for( auto it : config->GetCriticalConnectionList() )
    {
      if(( it._Addr != nullptr ) && ( it._Addr->GetAddrType() == csm::network::AddressType::CSM_NETWORK_TYPE_PTP ))
      {
        LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_AGENT: Comparing to AGG" << aggr << ": " << it._Addr->Dump();
        if( addr->Dump().compare( it._Addr->Dump() ) == 0 )
        { break; }
        ++aggr;
      }
    }
    switch( aggr )
    {
      case 0:
        conn_type = csm::daemon::ConnectionType::PRIMARY;
        break;
      case 1:
        conn_type = csm::daemon::ConnectionType::SECONDARY;
        break;
      default:
      {
        LOG( csmd, warning ) << "CSM_INFRASTRUCTURE_TEST_AGENT: Failed to determine msg comes from primary or secondary aggregator. Assuming PRIMARY";
        conn_type = csm::daemon::ConnectionType::PRIMARY;
      }
    }

    ComputeInfo data = ComputeInfo( config->GetHostname(),
                                    std::string(CSM_VERSION, 7) );
    data.SetConnectionType( conn_type );
    data.SetDaemonID( GetDaemonState()->GetDaemonID() );
    csm::network::Message msg = GetNetworkMessage(aEvent);
    
    msg.SetData( CSMI_BASE::ConvertToBytes<ComputeInfo>(data) );
    msg.SetResp();
    msg.CheckSumUpdate();

    csm::network::MessageAndAddress content_response( msg, addr );
    
    LOG(csmd, info) << "CSM_INFRASTRUCTURE_TEST_AGENT: Created node info and sent to aggregator: " << addr->Dump();

    ctx->SetAuxiliaryId(DONE_STATE);
    postEventList.push_back( CreateNetworkEvent(content_response, ctx) );
  }
  
}
