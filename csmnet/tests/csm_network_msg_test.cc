/*================================================================================

    csmnet/tests/csm_network_msg_test.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "csmutil/include/csm_test_utils.h"
#include "CPP/csm_network_msg_cpp.h"
#include "C/csm_network_msg_c.h"

#define RANDOM_MESSAGE_COUNT 500000
#define CSM_NET_MSG_TEST_DATA_SIZE 1024

void dataptr_crash_tests( csm_net_msg_t *cmsg,
                          char *data,
                          ssize_t (*csm_test_fn)( csm_net_msg_t *, const char *, const size_t ) )
{
  int i;

  (*csm_test_fn)( nullptr, nullptr, 0 );
  (*csm_test_fn)( nullptr, data, 0 );
  (*csm_test_fn)( cmsg, nullptr, 0 );
  (*csm_test_fn)( cmsg, data, 0 );

  for( i=0; i < 128; ++i )
  {
    int len = random() % CSM_NET_MSG_TEST_DATA_SIZE;
    (*csm_test_fn)( nullptr, nullptr, len );
    (*csm_test_fn)( nullptr, data, len );
    (*csm_test_fn)( cmsg, nullptr, len );
    (*csm_test_fn)( cmsg, data, len );
  }
}

int main( int argc, char **argv )
{
  int rc = 0;

  csm::network::Message msg;

  rc += TEST( msg.Init( CSM_CMD_ECHO,
                        0,
                        CSM_PRIORITY_DEFAULT,
                        2563245,
                        0x07341204,
                        0x06341204,
                        0, 0,
                        "Hello World"), true );

  rc += TEST( msg.GetAck(), false );   // ack properly set?
  rc += TEST( msg.GetPriority(), CSM_PRIORITY_DEFAULT );  // priority retrieval working properly?
  rc += TEST( msg.GetData(), "Hello World" );

  uint32_t chksum = msg.CheckSumCalculate();
  msg.CheckSumUpdate();
  rc += TEST( msg.GetCheckSum(), chksum );

  msg.SetAck();   // clear the ACK
  rc += TEST( msg.GetAck(), true );  // check if cleared
  rc += TEST( msg.GetPriority(), CSM_PRIORITY_DEFAULT );  // check if priority still correct

  chksum = msg.CheckSumCalculate();
  rc += TESTFAIL( msg.GetCheckSum(), chksum );

  msg.CheckSumUpdate();
  rc += TEST( msg.GetCheckSum(), chksum );

  // c-level tests
  csm_net_msg_t *cmsg;
  rc += TEST( cmsg = csm_net_msg_Init( 0,0,0,0,0,0,0,0,0), nullptr );

  // create a regular valid message
  rc += TESTFAIL( cmsg = csm_net_msg_Init( CSM_CMD_ECHO, 0, CSM_PRIORITY_DEFAULT, 0x14512, 100, 500, 0, 0, 0), nullptr );

  // allocate and set data area
  char *data = (char*)malloc( CSM_NET_MSG_TEST_DATA_SIZE );
  if( ! data ) return -1;

  // test initHdr from data buffer
  rc += TESTFAIL( cmsg = csm_net_msg_Init( CSM_CMD_ECHO, 0, CSM_PRIORITY_DEFAULT,
                                           0x14512, 100, 500,
                                           data, sizeof( csm_net_msg_t ), 0), nullptr );

  // crash tests
  rc += TEST( csm_net_msg_InitHdr( nullptr, nullptr ), -1 );
  rc += TEST( csm_net_msg_InitHdr( nullptr, data ), -1 );
  rc += TEST( csm_net_msg_InitHdr( cmsg, nullptr ), -1 );

  // create valid header from copy
  rc += TEST( memcpy( data, cmsg, sizeof( csm_network_header_t ) ), data );
  rc += TEST( csm_net_msg_InitHdr( cmsg, data ), 0 );

  // crash tests
  rc += TEST( csm_net_msg_GetProtocolVersion( nullptr ), 0xFF );
  rc += TEST( csm_net_msg_GetAck( nullptr ), 0 );
  rc += TEST( csm_net_msg_GetResponseFlag( nullptr ), 0 );
  rc += TEST( csm_net_msg_GetErrorFlag( nullptr ), 0 );
  rc += TEST( csm_net_msg_GetFlags( nullptr ), 0xFF );
  rc += TEST( csm_net_msg_GetPriority( nullptr ), CSM_NETWORK_PRIORITY_INVALID );
  rc += TEST( csm_net_msg_GetCommandType( nullptr ), CSM_CMD_INVALID );
  rc += TEST( csm_net_msg_GetMessageID( nullptr ), 0 );
  rc += TEST( csm_net_msg_GetCheckSum( nullptr ), 0 );
  rc += TEST( csm_net_msg_GetDataLen( nullptr ), 0 );
  rc += TEST( csm_net_msg_GetReserved( nullptr ), 0 );
  rc += TEST( csm_net_msg_GetData( nullptr ), nullptr );
  rc += TEST( csm_net_msg_GetUserID( nullptr ), CSM_CREDENTIAL_ID_UNKNOWN );
  rc += TEST( csm_net_msg_GetGroupID( nullptr ), CSM_CREDENTIAL_ID_UNKNOWN );

  csm_net_msg_SetProtocolVersion( nullptr );
  csm_net_msg_SetAck( nullptr );
  csm_net_msg_ClrAck( nullptr );
  csm_net_msg_SetResponseFlag( nullptr );
  csm_net_msg_ClrResponseFlag( nullptr );
  csm_net_msg_SetErrorFlag( nullptr );
  csm_net_msg_ClrErrorFlag( nullptr );
  csm_net_msg_SetFlags( nullptr, 0 );
  csm_net_msg_SetPriority( nullptr, CSM_NETWORK_PRIORITY_INVALID );
  csm_net_msg_SetCommandType( nullptr, CSM_CMD_INVALID );
  csm_net_msg_SetMessageID( nullptr, 0 );
  csm_net_msg_SetUserID( nullptr, CSM_CREDENTIAL_ID_UNKNOWN );
  csm_net_msg_SetGroupID( nullptr, CSM_CREDENTIAL_ID_UNKNOWN );
  csm_net_msg_SetReserved( nullptr, 0 );

  rc += TEST( csm_net_msg_CheckSumCalculate( nullptr ), 0 );
  rc += TEST( csm_net_msg_CheckSumUpdate( nullptr ), 0 );

  dataptr_crash_tests( cmsg, data, csm_net_msg_CopyData );
  dataptr_crash_tests( cmsg, data, csm_net_msg_SetData );
  dataptr_crash_tests( cmsg, data, csm_net_msg_SetDataAndChksum );
  dataptr_crash_tests( cmsg, data, csm_net_msg_CopyDataAndChksum );

  rc += TEST( csm_net_msg_GetHeaderBuffer( nullptr ), nullptr );
  rc += TEST( csm_net_msg_InitHdr( nullptr, data ), -1 );

  free( cmsg );

  // some kind of fuzzing by repeating tests with random data
  memset( data, 0x5a, CSM_NET_MSG_TEST_DATA_SIZE );
  int i=RANDOM_MESSAGE_COUNT;
  int correct_msg = 0;
  for( ; (i>0); --i )
  {
    csm_net_msg_t *tmsg;
    csm_csmi_cmds_t cmd = (csm_csmi_cmds_t)(random() % 0xFF);
    int flags = random() & 0x0F;
    uint8_t prio = (uint8_t)random() % 0xFF;
    uint64_t msgid = random();
    uint32_t uid = random();
    uint32_t gid = random();
    uint32_t resv = random();

    char *tdata = (char*)( (uintptr_t)data * (random() % 2));
    size_t datalen = random() % CSM_NET_MSG_TEST_DATA_SIZE;
    if( ! tdata )
      datalen = 0;

    bool expbool = (cmd < CSM_CMD_UNDEFINED ) || ( cmd >= CSM_CMD_MAX ) ||
        ( prio > CSM_NETWORK_MAX_PRIORITY) ||
        ( uid == CSM_CREDENTIAL_ID_UNKNOWN ) || ( gid == CSM_CREDENTIAL_ID_UNKNOWN ) ||
        (flags != ( flags & CSM_HEADER_FLAGS_MASK ) );


    int expect = expbool ? 22 : 0;

    tmsg = csm_net_msg_Init( cmd,
                             flags,
                             prio,
                             msgid,
                             uid,
                             gid,
                             tdata,
                             datalen,
                             resv );
    rc += TEST( errno, expect);
    if( expect != errno )
      std::cout << expect << ":" << errno << " != msg("
          << tmsg << "," << (int)cmd << "," << (int)flags << "," << (int)prio << "," << msgid << "," << resv << ")" << std::endl;

    if( (uintptr_t)tmsg )
    {
      rc += TEST( csm_net_msg_GetProtocolVersion( tmsg ), CSM_NETWORK_PROTOCOL_VERSION );
      rc += TEST( csm_net_msg_GetAck( tmsg ), flags & CSM_HEADER_ACK_BIT );
      rc += TEST( csm_net_msg_GetResponseFlag( tmsg ), flags & CSM_HEADER_RESP_BIT );
      rc += TEST( csm_net_msg_GetErrorFlag( tmsg ), flags & CSM_HEADER_ERR_BIT );
      rc += TEST( csm_net_msg_GetFlags( tmsg ), flags & CSM_HEADER_FLAGS_MASK );
      rc += TEST( csm_net_msg_GetPriority( tmsg ), prio );
      rc += TEST( csm_net_msg_GetCommandType( tmsg ), cmd );
      rc += TEST( csm_net_msg_GetMessageID( tmsg ), msgid );
      rc += TESTFAIL( csm_net_msg_GetCheckSum( tmsg ), 0 );
      rc += TEST( csm_net_msg_GetDataLen( tmsg ), datalen );
      rc += TEST( csm_net_msg_GetReserved( tmsg ), resv );
      rc += TEST( csm_net_msg_GetUserID( tmsg ), uid );
      rc += TEST( csm_net_msg_GetGroupID( tmsg ), gid );

      if( tdata )
        rc += TESTFAIL( csm_net_msg_GetData( tmsg ), nullptr );
      else
        rc += TEST( csm_net_msg_GetData( tmsg ), nullptr );

      ++correct_msg;
      free( tmsg );
    }
  }
  std::cout << "total = " << RANDOM_MESSAGE_COUNT << ";  valid_msgs = " << correct_msg << ";   ";

  if( data ) free( data );

  std::cout << "rc = " << rc << std::endl;

  return rc;
}
