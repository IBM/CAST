/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csm_envdata_handler.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/


#include "csm_envdata_handler.h"
#include "logging.h"

void
CSM_ENVDATA_HANDLER::Process( const csm::daemon::CoreEvent &aEvent,
                              std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  if ( !isNetworkEvent(aEvent) )
  {
    LOG(csmenv, error) << "CSM_ENVDATA_HANDLER: Expecting a NetworkEvent...";
    return;
  }

  csm::network::MessageAndAddress msgAddr = GetNetworkContent( aEvent );

  if ( msgAddr.GetAddr()->GetAddrType() != csm::network::CSM_NETWORK_TYPE_PTP)
  {
    LOG(csmenv, error) << "CSM_ENVDATA_HANDLER: Expecting a PTP connection from compute node..";
    return;
  }

  CSM_Environmental_Data envData;
  csm::network::Message msg = GetNetworkMessage(aEvent);
  CSMI_BASE::ConvertToClass<CSM_Environmental_Data>(msg.GetData(), envData);

  if( ! envData.HasData() )
  {
    LOG( csmenv, debug ) << "ENVDATA empty from " << msgAddr.GetAddr()->Dump();
    return;
  }

  LOG(csmenv, info) << "received environmental data from: " << msgAddr.GetAddr()->Dump();
  envData.Print();

  /*
  GPU_data.Print_Double_DCGM_Field_Values();
  GPU_data.Print_Int64_DCGM_Field_Values();
  GPU_data.Print_Double_DCGM_Field_String_Identifiers();
  GPU_data.Print_Int64_DCGM_Field_String_Identifiers();
  */

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // store the env data to the node status map:
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  csm::daemon::DaemonStateAgg *daemonState = dynamic_cast<csm::daemon::DaemonStateAgg*>(_handlerOptions.GetDaemonState() );
  if( daemonState->UpdateEnvironmentalData( msgAddr.GetAddr(), envData ) == false )
  {
    LOG( csmenv, error ) << "CSM_ENVDATA_HANDLER: Failed to store environmental data.";
  }
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // trigger BDS mgr to write data to BDS
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  postEventList.push_back( CreateBDSEvent( envData.Get_Json_String() ) );

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

}
