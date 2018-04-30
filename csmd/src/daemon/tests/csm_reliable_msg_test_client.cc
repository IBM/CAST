/*================================================================================

    csmd/src/daemon/tests/csm_reliable_msg_test_client.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <atomic>
#include <thread>

#include <logging.h>
#include "csmutil/include/csm_test_utils.h"
#include <csm_network_header.h>
#include <csmnet/src/CPP/csm_network_msg_cpp.h>
#include <csmnet/src/CPP/csm_network_exception.h>
#include <csmnet/src/CPP/endpoint.h>
#include <csmnet/src/CPP/endpoint_ptp.h>
#include <csmnet/src/CPP/endpoint_dual_unix.h>
#include <csmnet/src/CPP/endpoint_multi_unix.h>
#include <csmnet/src/CPP/reliable_msg.h>

#include "csm_reliable_msg_test.h"


int SingleUnitTest()
{
  int rc = 0;

  csm::network::MultiEndpointTest MET;

  rc += MET.TestNullptrEPEvent();

  return rc;
}


int main(int argc, char **argv)
{
    int rc = 0;
    // set true if you want client to be parent process for debugging

    setLoggingLevel(csmd, DBG_LEVEL);
    setLoggingLevel(csmnet, DBG_LEVEL );

    rc = SingleUnitTest();
    if( rc > 0 )
      return rc;

    if( TEST_SEPARATE_CLIENT )
      rc = client_test();
    else
    {
      LOG( csmd, warning ) << "Compiled with integrated client, separate client program is useless. " << std::endl
          << "Please run csm_reliable_msg_test instead.";

      rc = 1;
    }

    std::cout << "Test exiting: rc=" << rc << std::endl;
    return rc;
}
