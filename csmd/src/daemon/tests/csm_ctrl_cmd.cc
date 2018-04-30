/*================================================================================

    csmd/src/daemon/tests/csm_ctrl_cmd.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

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


#include "csmi/include/csm_api_common.h"
#include "csmi/src/common/include/csmi_common_utils.h"
#include "csmi/src/common/include/csmi_api_internal.h"

#include "src/csmi_request_handler/csmi_base.h"
#include "src/csmi_request_handler/ctrl_cmd_option.h"

static std::map<std::string, utility::bluecoral_sevs> str2severity = {
#define SEVERITY(n) { #n, utility::bluecoral_sevs::n },
#include "utilities/include/severity.h"
#undef SEVERITY
};


int client_test(const CtrlCmdOption& i_option)
{

  int rc = 0;
  std::cout << "Started client " << std::endl;
  
  if (csm_init_lib() != 0)
  {
    std::cerr << "ERROR: csm_init_lib() failed." << std::endl;
    return -1;
  }
  
  csm_api_object* csm_obj = csm_api_object_new(CSM_CTRL_cmd, NULL);
  
  std::string payload = CSMI_BASE::ConvertToBytes<CtrlCmdOption>(i_option);
  char *recvData=nullptr;
  uint32_t recvDataLen;
  
  if (csmi_sendrecv_cmd(csm_obj, CSM_CTRL_cmd, payload.c_str(), payload.length(),
                        &recvData, &recvDataLen) != 0 ) {
    std::cerr << "ERROR: errcode = " << csm_api_object_errcode_get(csm_obj) <<
                 "errmsg = " << csm_api_object_errmsg_get(csm_obj) << std::endl;
    rc++;
  }
  else
  {
    std::cout << "\n\n### RESPONSE FROM THE LOCAL DAEMON ###\n";
    std::cout << std::string(recvData, recvDataLen);
    std::cout << "#######################################\n";
    if (recvData) free(recvData);

  }
  
  csm_api_object_destroy(csm_obj);
  
  if (csm_term_lib() != 0)
  {
    std::cerr << "ERROR: csm_term_lib() failed." << std::endl;
    rc++;
  }
  
  //std::cout << "client exit: rc=" << rc << std::endl;
  return rc;
 
}

int ParseCommandLineOptions( int argc, char **argv, CtrlCmdOption &o_cmd_option )
{
  std::string log_csmdb;
  std::string log_csmd;
  std::string log_csmnet;
  std::string log_csmras;
  std::string log_csmapi;
  
  po::options_description usage("Supported Command Line Options");
  usage.add_options()
        ("help,h", "Show this help")
        ("dump_perf_data",
            "Dump the performance data")
/*
        ("dump_mem_usage",
            "Dump current memory usage")
*/
        ("log.csmdb",
            po::value<std::string>(&log_csmdb),
            "Specify the severity level ")
        ("log.csmd",
            po::value<std::string>(&log_csmd),
            "Specify the severity level ")
        ("log.csmnet",
            po::value<std::string>(&log_csmnet),
            "Specify the severity level ")
        ("log.csmras",
            po::value<std::string>(&log_csmras),
            "Specify the severity level ")
        ("log.csmapi",
            po::value<std::string>(&log_csmapi),
            "Specify the severity level ")
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

  int count = 0;
  if( vm.count( "log.csmdb" ) )
  {
    if (str2severity.find(log_csmdb) != str2severity.end())
    {
      o_cmd_option.set_log_csmdb(str2severity[log_csmdb]);
      count++;
    }
    else
    {
      std::cout << "Invalid severity: " << log_csmdb << std::endl;
      return -1;
    }
  }
  
  if( vm.count( "log.csmd" ) )
  {
    if (str2severity.find(log_csmd) != str2severity.end())
    {
      o_cmd_option.set_log_csmd(str2severity[log_csmd]);
      count++;
    }
    else
    {
      std::cout << "Invalid severity: " << log_csmd << std::endl;
      return -1;
    }
  }
  
  if( vm.count( "log.csmnet" ) )
  {
    if (str2severity.find(log_csmnet) != str2severity.end())
    {
      o_cmd_option.set_log_csmnet(str2severity[log_csmnet]);
      count++;
    }
    else
    {
      std::cout << "Invalid severity: " << log_csmnet << std::endl;
      return -1;
    }
  }
  
  if( vm.count( "log.csmras" ) )
  {
    if (str2severity.find(log_csmras) != str2severity.end())
    {
      o_cmd_option.set_log_csmras(str2severity[log_csmras]);
      count++;
    }
    else
    {
      std::cout << "Invalid severity: " << log_csmras << std::endl;
      return -1;
    }
  }
  if( vm.count( "log.csmapi" ) )
  {
    if (str2severity.find(log_csmapi) != str2severity.end())
    {
      o_cmd_option.set_log_csmapi(str2severity[log_csmapi]);
      count++;
    }
    else
    {
      std::cout << "Invalid severity: " << log_csmapi << std::endl;
      return -1;
    }
  }
  
  if( vm.count( "dump_perf_data" ) )
  {
    o_cmd_option.set_dump_perf_data();
    count++;
  }

  if( vm.count( "dump_mem_usage" ) )
  {
    o_cmd_option.set_dump_mem_usage();
    count++;
  }
    
  return count;

}

int main(int argc, char **argv)
{
    int rc = 0;
    setLoggingLevel(csmd, error);
    setLoggingLevel(csmnet, error);
    
    CtrlCmdOption cmd_option;
    rc = ParseCommandLineOptions(argc, argv, cmd_option);
    if (rc > 0)
    {
      rc = client_test(cmd_option);
    }
    return rc;
}
