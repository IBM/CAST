/*================================================================================

    csmd/src/inv/src/inv_master_handler.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "inv_master_handler.h"

#include "logging.h"

#include "inv_get_node_inventory.h"

void INV_MASTER_HANDLER::Process( const csm::daemon::CoreEvent &aEvent, std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  LOG(csmd, info) << "Enter " << __PRETTY_FUNCTION__;
    
  // FOR PUBLISH
//  csm::network::Message msg;
  // Send a request to aggregators asking for compute node inventory data
//  msg.Init (GetCommandNodeInventory(),
//                               0, //flag
//                               CSM_PRIORITY_RETAIN, //priority
//                               0, //msg id
//                               MASTER,
//                               AGGREGATOR,
//                               geteuid(), getegid(),
//                               std::string(""));
//
//  postEventList.push_back( CreateNetworkEvent(msg, _AbstractAggregator) );

}
