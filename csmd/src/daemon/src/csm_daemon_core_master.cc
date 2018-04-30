/*================================================================================

    csmd/src/daemon/src/csm_daemon_core_master.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/** file csm_daemon_core_master.cc
 *
 ******************************************/
#include <iostream>
#include <vector>

#include <logging.h>

#include "include/csm_daemon_core.h"

namespace csm {
namespace daemon {

// Master daemon construction/initialization
CoreMaster::CoreMaster()
{
  LOG(csmd,debug) << "Master created ...";
  _EventRouting = new csm::daemon::EventRoutingMaster();
  
  // call after initializing EvnetRouting
  //InitInfrastructure();
}

CoreMaster::~CoreMaster()
{
  //  daemon destruction  
}

}  // namespace daemon
} // namespace csm
