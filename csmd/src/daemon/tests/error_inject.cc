/*================================================================================

    csmd/src/daemon/tests/error_inject.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/


#include <iostream>

#include "logging.h"
#include "csmutil_logging.h"

#include "csmi/include/csm_api_common.h"
#include "csmi/src/common/include/csmi_common_utils.h"
#include "csmi/src/common/include/csmi_api_internal.h"
#include "csmi/include/csmi_type_common.h"
#include "src/csmi_request_handler/csm_error_case_handler.h"

int generate_random_message( std::string &data, size_t size )
{
  for( size_t n = 6; n < size; ++n )
    data[n] = (char)(random() % 26) + 97;

  data[size] = 0;
  return size;
}

int general_request_response_test( ErrorInjectionData &data, const int expected,
                                   const std::string &payload,
                                   char **recvData, uint32_t *recvDataLen,
                                   const csmi_cmd_err_t csmi_expect = CSMI_SUCCESS )
{
  int rc = 0;

  csm_api_object* csm_obj = csm_api_object_new(CSM_error_inject, NULL);

  *recvData = nullptr;
  *recvDataLen = 0;

  int err = csmi_sendrecv_cmd(csm_obj, CSM_error_inject,  payload.c_str(), payload.length(),
                              recvData, recvDataLen);

  if(err != expected)
  {
    std::cerr << "ERROR: errcode = " << csm_api_object_errcode_get(csm_obj) <<
        " errmsg = " << csm_api_object_errmsg_get(csm_obj) << std::endl;
    if( csmi_expect != csm_api_object_errcode_get(csm_obj) )
      rc++;
  }

  csm_api_object_destroy(csm_obj);
  return rc;
}

/** @brief Sends a command through the Infrastructure and waits for a response
 * this is the extended API of csmi_sendrecv_cmd() and allows to set priority
 * and flags explicitly.
 */
#ifdef __cplusplus
extern "C"
{
#endif
extern int csmi_sendrecv_cmd_ext(
    csm_api_object *csm_obj,
    csmi_cmd_t cmd,
    uint8_t flags,
    uint8_t priority,
    const char *sendPayload,
    uint32_t sendPayloadLen,
    char **recvPayload,
    uint32_t *recvPayloadLen );
#ifdef __cplusplus
}
#endif


int error_request_response_test( ErrorInjectionData &data, const int expected,
                                 uint8_t flags,
                                 uint8_t priority,
                                 const std::string &payload,
                                 char **recvData, uint32_t *recvDataLen )
{
  int rc = 0;

  csm_api_object* csm_obj = csm_api_object_new(CSM_error_inject, NULL);

  *recvData = nullptr;
  *recvDataLen = 0;

  int err = csmi_sendrecv_cmd_ext(csm_obj, CSM_error_inject, flags, priority, payload.c_str(), payload.length(),
                                  recvData, recvDataLen);

  if(err != expected)
  {
    std::cerr << "ERROR: errcode = " << csm_api_object_errcode_get(csm_obj) <<
        " errmsg = " << csm_api_object_errmsg_get(csm_obj) << std::endl;
    rc++;
  }

  csm_api_object_destroy(csm_obj);
  return rc;
}

int echo_test( const int expected )
{
  int rc = 0;

  csm_api_object* csm_obj = csm_api_object_new(CSM_CMD_ECHO, NULL);

  char *sendData=strdup( "This is an echo test" );
  char *recvData = nullptr;
  uint32_t recvDataLen = 0;

  int err = csmi_sendrecv_cmd(csm_obj, CSM_CMD_ECHO, sendData, strnlen( sendData, 64 ),
                              &recvData, &recvDataLen);

  if(err != expected)
  {
    std::cerr << "ERROR: errcode = " << csm_api_object_errcode_get(csm_obj) <<
        " errmsg = " << csm_api_object_errmsg_get(csm_obj) << std::endl;
    rc++;
  }

  csm_api_object_destroy(csm_obj);
  return rc;
}


int send_large_msg_test( ErrorInjectionData &data )
{
  int rc = 0;

  char *recvData=nullptr;
  uint32_t recvDataLen;
  int expected = 0;

  std::string longrequest = "NODATA";
  longrequest.resize( data.GetIntArg() );
  generate_random_message( longrequest, data.GetIntArg() );

  rc = general_request_response_test( data, expected, longrequest, &recvData, &recvDataLen );

  if( recvDataLen != 2 )
  {
    std::cerr << "ERROR: expected 2byte response." << std::endl;
    rc++;
  }

  return rc;
}

int bad_msg_test( ErrorInjectionData &data, const int testcase )
{
  int rc = 0;

  char *recvData=nullptr;
  uint32_t recvDataLen;

  uint8_t flags = 0;
  uint8_t priority = CSM_PRIORITY_DEFAULT;

  switch( testcase )
  {
    case 1:
      flags = CSM_HEADER_MTC_BIT;
      std::cout << "Multicast message requested: " << data.GetIntArg() << std::endl;
      rc = error_request_response_test( data, CSMERR_SENDRCV_ERROR, flags, priority, "", &recvData, &recvDataLen );
      break;
    case 2:
      flags = CSM_HEADER_INT_BIT;
      std::cout << "Internal message requested: " << data.GetIntArg() << std::endl;
      rc = error_request_response_test( data, CSMERR_SENDRCV_ERROR, flags, priority, "", &recvData, &recvDataLen );
      break;
    default:
      std::cout << "Unknown badmsg test." << data.GetIntArg() << std::endl;
      rc++;
      break;
  }

  return rc;
}


int client_test(ErrorInjectionData& data)
{
  int rc = 0;
  std::cout << "Starting. Contacting local daemon..." << std::endl;
  
  if (csm_init_lib() != 0)
  {
    std::cerr << "ERROR: csm_init_lib() failed." << std::endl;
    return -1;
  }
  // todo: handle case where it never returns?

  std::string payload = CSMI_BASE::ConvertToBytes<ErrorInjectionData>(data);
 
  char *recvData=nullptr;
  uint32_t recvDataLen;
  
  int expected = 0;
  switch(data.GetMode())
  {
    case ErrorInjectionData::TIMEOUT:
      expected = CSMERR_SENDRCV_ERROR;
      rc += general_request_response_test( data, expected, payload, &recvData, &recvDataLen, CSMERR_TIMEOUT );
      if(recvData)
      {
        std::cout << "Unexpected response: " << recvData << std::endl;
      }
      else
      {
        std::cout << "Timeout test complete." << std::endl;
      }
      break;

    case ErrorInjectionData::DBLOCK:
      rc += general_request_response_test( data, expected, payload, &recvData, &recvDataLen );
      LOG(csmd,debug) << "Completed: " << 
        csm::daemon::Configuration::Instance()->GetDBConnectionPool()->GetNumOfFreeDBConnections()
        << " connections released.";
      break;

    case ErrorInjectionData::LARGEMSG:
    {
      rc += general_request_response_test( data, expected, payload, &recvData, &recvDataLen );
      LOG( csmd, debug ) << "Large message requested: " << data.GetIntArg()
        << "received: " << recvDataLen;

      rc += send_large_msg_test( data );

      break;
    }

    case ErrorInjectionData::LOOP:
    {
      for( int n=0; n < data.GetIntArg(); ++n )
      {
        rc += general_request_response_test( data, expected, payload, &recvData, &recvDataLen );
        if( recvData != nullptr )
          free( recvData );
        recvData = nullptr;
      }
      break;
    }
    case ErrorInjectionData::BADMSG:
      rc += bad_msg_test( data, 1 );
      break;
    default:
      break;
  }

  if( data.GetEcho() )
  {
    std::cout << "FINISHED MAIN TEST. Sleeping for " << data.GetEcho() << "s before doing an echo test." << std::endl;
    sleep( data.GetEcho() );
    rc += echo_test( 0 );
  }
  else
    std::cout << "Finished. Cleaning up..." << std::endl;




  if (csm_term_lib() != 0)
  {
    std::cerr << "ERROR: csm_term_lib() failed." << std::endl;
    rc++;
  }
  
  std::cout << "Test complete: rc=" << rc << std::endl;
  return rc;

}

int ParseCommandLineOptions( int argc, char **argv, ErrorInjectionData &data )
{
  po::options_description usage("Supported Command Line Options");
  usage.add_options()
        ("help,h", "Show this help")
        ("timeout,t", po::value<int64_t>(), "Cause client to timeout after at least Ns")
        ("DBlock,n", po::value<int64_t>(), "Lock N database connections")
        ("drain,d", "Release held db connections over time" )
        ("largemsg,m", po::value<int64_t>(), "Respond with a large message")
        ("badmsg,b", po::value<int64_t>(), "Inject bad messages (1-MTC, 2-INT)")
        ("echo,e", po::value<int>(), "Add an echo request after N seconds once the test is finished")
        ("loop,l", po::value<int>(), "Run N iterations of echo requests" )
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
  if( vm.count( "echo" ) )
  {
    data.SetEcho( vm["echo"].as<int>() );
  }
  if( vm.count( "timeout") )
  {
    int64_t to = vm["timeout"].as<int64_t>();
    std::cout << " Client-timeout = " << to << std::endl;
    data.SetIntArg( to );
    data.SetMode(ErrorInjectionData::TIMEOUT);
  }
  if( vm.count( "DBlock") )
  {
    data.SetMode(ErrorInjectionData::DBLOCK);
    data.SetIntArg(vm["DBlock"].as<int64_t>());
    if(vm.count("drain"))
    {
      data.SetDrain(true);
    }
  }
  if( vm.count("largemsg") )
  {
    data.SetMode(ErrorInjectionData::LARGEMSG );
    data.SetIntArg( vm["largemsg"].as<int64_t>() );
    std::cout << "Requesting large message response. Size=" << data.GetIntArg() << std::endl;
  }
  if( vm.count("loop") )
  {
    data.SetMode(ErrorInjectionData::LOOP );
    data.SetIntArg( vm["loop"].as<int>() );
    std::cout << "Requesting looped echo cmds. Iter=" << data.GetIntArg() << std::endl;
  }
  if( vm.count("badmsg") )
  {
    data.SetMode( ErrorInjectionData::BADMSG );
    data.SetIntArg( vm["badmsg"].as<int64_t>() );
    std::cout << "Requesting bad message injection" << std::endl;
  }
  return 1;
}

int main(int argc, char **argv)
{
    int rc = 0;
    setLoggingLevel(csmd, error);
    setLoggingLevel(csmnet, error);
    csmutil_logging_level_set( strdup( "debug" ));
    ErrorInjectionData data;

    if(ParseCommandLineOptions(argc, argv, data))
    {
      rc = client_test(data);
    }
    return rc;
}
