/*================================================================================

    csmnet/tests/epoll_wrapper_test.cc

  © Copyright IBM Corporation 2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/


#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <iostream>
#include <chrono>
#include <atomic>
#include <thread>
#include <memory>

#include <logging.h>

#include <boost/property_tree/json_parser.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>
#include <csmd/src/daemon/include/csm_api_config.h>

#include "csmutil/include/csm_test_utils.h"
#include "csm_timing.h"

#include "csmutil_logging.h"
#include "csmi/include/csm_api_common.h"
#include "csmi/src/common/include/csmi_common_utils.h"
#include "csmi/src/common/include/csmi_api_internal.h"


//#define MULTI_ENDPOINT_TEST


namespace pt = boost::property_tree;

int test_without_daemon( const std::string conffile )
{
  int ret = 0;

  pt::ptree conftree;
  pt::read_json(conffile, conftree);
  initializeLogging( std::string( "csm.log" ), conftree );

  for( int i=0; i<CSM_CMD_INVALID; ++i )
  {
    LOG( csmnet, always ) << csmi_cmds_t_strs[ i ] << ": "<< csm_get_timeout( i );
  }

  std::string to_file = "csm_api_timeouts.conf";

  boost::optional<std::string> value = conftree.get_optional<std::string>("csm.api_configuration_file");
  if( value.is_initialized() )
    to_file = *value;

  csm::daemon::CSMAPIConfig timeouts( to_file );

  csm_update_timeouts( timeouts.GetSerializedTimeouts().c_str(),
                       timeouts.GetSerializedTimeouts().length() );
  for( int i=0; i<CSM_CMD_INVALID; ++i )
  {
    LOG( csmnet, always ) << csmi_cmds_t_strs[ i ] << ": "<< csm_get_timeout( i );
  }
  return ret;
}


int test_with_daemon()
{
  if (csm_init_lib() != 0)
  {
    LOG( csmnet, error ) << "CL:ERROR: csm_init_lib() failed.";
    return 1;
  }

  for( int i=0; i<CSM_CMD_INVALID; ++i )
  {
    LOG( csmnet, always ) << csmi_cmds_t_strs[ i ] << ": "<< csm_get_timeout( i );
  }

  if (csm_term_lib() != 0)
  {
    LOG( csmnet, error ) << "CL:ERROR: csm_term_lib() failed.";
    return 1;
  }

  return 0;
}



int main(int argc, char **argv)
{
    int ret = 0;

    std::string conffile("csm_master.conf");
    if( argc > 1 )
      conffile = argv[ 1 ];

    ret += test_without_daemon( conffile );

    LOG( csmnet, error ) << " ------------------------------------------------------------------ ";

    ret += test_with_daemon();

    std::cout << "Test exiting: rc=" << ret << std::endl;
    return ret;
}
