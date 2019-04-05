/*================================================================================

    csmd/src/daemon/tests/csm_infrastructure_health_check.cc
    _health_check.cc

  © Copyright IBM Corporation 2015-2019. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

namespace po = boost::program_options;

#include <iostream>

#include "logging.h"
#include "csm_daemon_exception.h"
#include "csmi/include/csm_api_common.h"
#include "csmi/src/common/include/csmi_common_utils.h"
#include "csmi/src/common/include/csmi_api_internal.h"

#include "include/csm_healthcheck_data.h"

#define TESTLOOPS 1

template<typename T>
static std::string ConvertToBytes(const T &i_option)
{
  std::stringstream ss;
  boost::archive::text_oarchive oa(ss);
  oa << i_option;
  return ss.str();
}

template<typename T>
static void ConvertToClass(const std::string &i_payload, T &o_option)
{
  std::stringstream ss;
  ss << i_payload;
  boost::archive::text_iarchive ia(ss);
  ia >> o_option;
}

int client_test(const HealthCheckData& i_option)
{
  int rc = 0;
  std::cout << "Starting. Contacting local daemon..." << std::endl;

  if (csm_init_lib() != 0)
  {
    std::cerr << "ERROR: csm_init_lib() failed." << std::endl;
    return -1;
  }
  std::cout << "Connected. Checking infrastructure... (this may take a moment. Please be patient...)" << std::endl;

  for( int i=0; (i<TESTLOOPS) && (rc == 0); ++i)
  {
    csm_api_object* csm_obj = csm_api_object_new(CSM_infrastructure_test, NULL);

    std::string payload = ConvertToBytes<HealthCheckData>(i_option);
    char *recvData=nullptr;
    uint32_t recvDataLen;

    if (csmi_sendrecv_cmd(csm_obj, CSM_infrastructure_test, payload.c_str(), payload.length(),
                          &recvData, &recvDataLen) != 0 )
    {
      std::cerr << "ERROR: errcode = " << csm_api_object_errcode_get(csm_obj) <<
          " errmsg = " << csm_api_object_errmsg_get(csm_obj) << std::endl;
      rc++;
    }
    else if (recvData)
    {
      HealthCheckData res;
      ConvertToClass<HealthCheckData>(std::string(recvData, recvDataLen), res);
      free(recvData);
      std::cout << "\n###### RESPONSE FROM THE LOCAL DAEMON #######\n";
      std::cout << res.Dump();
      std::cout << "#############################################\n\n";
    }
    csm_api_object_destroy(csm_obj);
  }

  std::cout << "Finished. Cleaning up..." << std::endl;

  if (csm_term_lib() != 0)
  {
    std::cerr << "ERROR: csm_term_lib() failed." << std::endl;
    rc++;
  }

  std::cout << "Test complete: rc=" << rc << std::endl;
  return rc;

}

int ParseCommandLineOptions( int argc, char **argv, HealthCheckData &o_cmd_option )
{
  po::options_description usage("Supported Command Line Options");
  usage.add_options()
        ("help,h", "Show this help")
        ("verbose,v",
            "Verbose Mode")
    ;

  po::variables_map vm;
  po::store( po::parse_command_line(argc, argv, usage), vm);
  po::notify(vm);

  if( vm.count( "help" ) )
  {
      std::cerr << usage << std::endl;
      errno = 0;
      return 0;
  }

  if( vm.count( "verbose" ) )
  {
    o_cmd_option.set_verbose_option();
  }

  return 1;

}

int main(int argc, char **argv)
{
    int rc = 0;
    setLoggingLevel(csmd, error);
    setLoggingLevel(csmnet, error);

    HealthCheckData cmd_option;
    rc = ParseCommandLineOptions(argc, argv, cmd_option);
    if (rc > 0)
    {
      try { rc = client_test(cmd_option); }
      catch ( csm::daemon::Exception &e )
      {
        std::cout << "Error detected: " << e.what() << std::endl;
      }
    }
    return rc;
}
