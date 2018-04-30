/*================================================================================

    csmd/src/daemon/tests/csm_endpoint_pubsub_test.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/** file csm_daemon.cc
 *
 ******************************************/

#include <errno.h>
#include <iostream>

#include <logging.h>
#include "csm_daemon_config.h"

#include "csmnet/src/CPP/endpoint.h"
#include "csmnet/src/CPP/endpoint_pubsub.h"

#include "include/csm_core_event.h"
#include "include/csm_event_manager.h"
#include "include/csm_event_sink.h"
#include "include/csm_event_source.h"
#include "include/csm_event_source_set.h"

#include "include/csm_daemon_network_handling.h"
#include "include/csm_daemon_core.h"

#include "include/csm_event_routing.h"

int main( int argc, char **argv )
{
  int rc = 0;

  // need to initialize the config instnce before initializing EventRouting...
  try {
    csm::daemon::Configuration::Instance( argc, argv );
  }

  catch (csm::daemon::ConfigurationException &e) {
    if( errno )
    {
      LOG(csmd,error) << e.what();
      std::cerr << e.what() << std::endl;
    }
    return errno;
  }

  
  // create daemon role
  csm::daemon::EventRoutingUtility eventRouting;
  csm::daemon::CoreGeneric *DaemonCore;
  DaemonCore = new csm::daemon::CoreUtility((csm::daemon::EventRouting *) &eventRouting);
  DaemonCore->Fetch();
  return rc;
}
