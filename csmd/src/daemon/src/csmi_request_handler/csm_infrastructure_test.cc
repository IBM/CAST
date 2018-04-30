/*================================================================================
   
    csmd/src/daemon/src/csmi_request_handler/csm_infrastructure_test.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
// implement the CSM api node attributes update command...
//

#include <malloc.h>

#include "csm_infrastructure_test.h"
#include "include/csm_event_type_definitions.h"

#include "csmnet/src/CPP/csm_network_exception.h"

/*
 * This test case is triggered by a CSM API client who is sending a CSM_infrastructure_test command.
 * The request will be flowing through the utility, master, aggregator and the compute node
 * (can support multiple compute nodes now). Then the compute node will gather some node status and
 * compose a reply network event. The aggregator will be responsible for aggregating the node status
 * and compose a single reply. This reply event will then be flowing through master,
 * utility and the csm api client.
 * 
 * The testing script is set up here: csmd/src/daemon/tests/csm_infrastructure_test.sh
 */

/*
 * CSM_INFRASTRUCTURE_TEST
 */
void CSM_INFRASTRUCTURE_TEST::TestSystemEvent( const csm::daemon::CoreEvent &aEvent)
{
  const csm::network::Address_sptr addr = GetAddrFromSystemEvent(aEvent);
  if (isSystemConnectedEvent(aEvent))
  {
    LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST::TestSystemEvent: A new connection: " << addr->Dump();
  }
  else if (isSystemDisconnectedEvent(aEvent))
  {
    LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST::TestSystemEvent: A down connection: " << addr->Dump();
  }
}

// initialize a list of Utility node address codes that are expected to respond
// after init, this is contains the utility node address codes that haven't responded yet
void
EventContextTestMaster::InitUtilList( const std::set<csm::network::AddressCode> i_UtilInfo )
{
  std::lock_guard<std::mutex> guard(_Lock);
  _UtilExpect = i_UtilInfo;
}

int
EventContextTestMaster::ReceivedUtilInfo( const csm::network::AddressCode i_AddrCode )
{
  std::lock_guard<std::mutex> guard(_Lock);
  if( _UtilExpect.erase( i_AddrCode ) == 0 )
    throw std::out_of_range("Utility AddressCode out of range.");
  return _UtilExpect.size();
}

void
EventContextTestMaster::InitAggList( const std::set<csm::network::AddressCode> i_AggInfo )
{
  std::lock_guard<std::mutex> guard(_Lock);
  _AggExpect = i_AggInfo;
}

int
EventContextTestMaster::ReceivedAggInfo( const csm::network::AddressCode i_AddrCode )
{
  std::lock_guard<std::mutex> guard(_Lock);
  if( _AggExpect.erase( i_AddrCode ) == 0 )
    throw std::out_of_range("Aggregator AddressCode out of range.");
  return _AggExpect.size();
}

