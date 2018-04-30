/*================================================================================

    csmd/src/daemon/src/csm_daemon_core_aggregator.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/* csm_daemon_core_aggregator.cc
 *
 ******************************************/

#include <iostream>
#include <vector>

#include <logging.h>

#include "include/csm_daemon_core.h"

namespace csm {
namespace daemon {

// Aggregator daemon construction/initialization
CoreAggregator::CoreAggregator()
{
    LOG(csmd,debug) << "Aggregator Daemon created ...";
     _EventRouting = new csm::daemon::EventRoutingAgg();
    
    // call after initializing EvnetRouting
    //InitInfrastructure();
}

CoreAggregator::~CoreAggregator()
{
  //  daemon destruction
}
  

}  // namespace daemon
} // namespace csm
