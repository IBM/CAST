/*================================================================================

    csmnet/tests/buffer_state_test.cc

  © Copyright IBM Corporation 2015-2019. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <string>
#include <iostream>

#include "logging.h"
#include "CPP/csm_network_msg_cpp.h"
#include "CPP/endpoint_buffer.h"
#include "csmutil/include/csm_test_utils.h"
#include <sys/un.h>

char* RandomStringBuffer()
{
  size_t len = DGRAM_PAYLOAD_MAX * 2;
  char *buf = new char[len];
  for( size_t n=0; n < len; ++n )
    buf[ n ] = (char)(random() % 26 + 97);
  return buf;
}

ssize_t GenerateMsgBuffer( char * o_Buffer, const char *i_Content, ssize_t i_MaxLen )
{
  csm::network::Message msg;

  ssize_t stlen = random() % i_MaxLen;
  char *stpos = (char*)i_Content + (random() % DGRAM_PAYLOAD_MAX);

  std::string data = std::string( stpos, stlen );

  msg.Init( (random()+1) % CSM_CMD_MAX,
            random() & CSM_HEADER_FLAGS_MASK,
            random() % CSM_NETWORK_MAX_PRIORITY,
            random(),
            random(),
            random(),
            random(),
            random(),
            data );
  if( ! msg.Validate() )
    return -1;

  char * cpos = o_Buffer;
  memcpy( cpos, msg.GetHeaderBuffer(), sizeof( csm_network_header ) );
  cpos += sizeof( csm_network_header );
  memcpy( cpos, msg.GetDataPtr(), msg.GetDataLen() );

  return sizeof( csm_network_header ) + msg.GetDataLen();
}



int main( int argc, char **argv )
{
  int rc = 0;

  csm::network::EndpointBuffer epbuf;
//  csm::network::Message *msg;
  csm::network::Message testMsg;
  ssize_t len;

  char *content = RandomStringBuffer();

  // test initialization
  rc += TEST( epbuf.GetRecvBufferPtr() != nullptr, true );
  rc += TEST( epbuf.GetRecvSpace(), DGRAM_PAYLOAD_MAX );

  // insert a single message and try retrieve
  rc += TESTFAIL( len = GenerateMsgBuffer( epbuf.GetRecvBufferPtr(), content, epbuf.GetRecvSpace() ), -1 );
  try
  {
    epbuf.Update( len );
  }
  catch (csm::network::ExceptionProtocol &e )
  {
    rc += 1;
  }
  rc += TESTFAIL( epbuf.Recv( testMsg ), 0 );  // Recv/consume msg
  rc += TEST( testMsg.Validate(), true );  // message needs to be valid

  rc += TEST( epbuf.Recv( testMsg ), 0 );  // make sure msg is consumed

  // force a half-msg
  len = GenerateMsgBuffer( epbuf.GetRecvBufferPtr(), content, epbuf.GetRecvSpace() );
  epbuf.Update( len - 1 );
  rc += TEST( epbuf.Recv( testMsg ), 0 );  // nothing to recv
  rc += TEST( testMsg.Validate(), true );  // validation should still find the previous valid msg

  // adjust to fit message end
  epbuf.Update( 1 );
  // add another message before doing Recv()
  len = GenerateMsgBuffer( epbuf.GetRecvBufferPtr(), content, epbuf.GetRecvSpace() );
  epbuf.Update( len );

  rc += TESTFAIL( epbuf.Recv( testMsg ), 0 );  // there should be a msg now ...
  rc += TEST( testMsg.Validate(), true );      // ... a valid message
  rc += TESTFAIL( epbuf.Recv( testMsg ), 0 );  // there should be another msg too ...
  rc += TEST( testMsg.Validate(), true );      // ... a valid message

  // invalid messages
  epbuf.Update(200);
  rc += TEST( epbuf.Recv(testMsg), 200 );
  rc += TEST( testMsg.GetErr(), true );
  rc += TEST( testMsg.Validate(), false );
  rc += TEST( epbuf.IsEmpty(), true );


  len = GenerateMsgBuffer(epbuf.GetRecvBufferPtr(), RandomStringBuffer(), DGRAM_PAYLOAD_MAX-100);
  epbuf.Update(len);
  epbuf.Update(100);

  epbuf.Recv(testMsg);  // valid message
  rc += TEST( testMsg.GetDataLen(), len-sizeof(csm_network_header) );
  rc += TEST( testMsg.GetErr(), false );
  rc += TEST( testMsg.Validate(), true );

  epbuf.Recv(testMsg); // invalid message
  rc += TEST( testMsg.GetDataLen(), 100-sizeof(csm_network_header) );
  rc += TEST( testMsg.GetErr(), true );
  rc += TEST( testMsg.Validate(), false );

  // too long message
  csm::network::Message msg;
  csm_network_header header;

  header._ProtocolVersion = CSM_NETWORK_PROTOCOL_VERSION;
  header._CommandType = (random()+1) % CSM_CMD_MAX;
  header._Flags = random() & CSM_HEADER_FLAGS_MASK;
  header._Priority = random() % CSM_NETWORK_MAX_PRIORITY;

  std::string long_msg = std::string(RandomStringBuffer()).substr(0, DGRAM_PAYLOAD_MAX+1);
  header._DataLen = long_msg.length();
  header._CheckSum = csm_header_check_sum( &header, long_msg.c_str() );

  rc += [&](){
    msg.InitHdr((char*)&header);

    //memcpy( epbuf.GetRecvBufferPtr(), msg.GetHeaderBuffer(), sizeof(csm_network_header) );
    //memcpy( epbuf.GetRecvBufferPtr()+sizeof(csm_network_header), long_msg.c_str(), long_msg.length() );
    try {
      epbuf.Update(sizeof(csm_network_header)+long_msg.length());
    } catch (csm::network::ExceptionProtocol& e) {
      return 0;
    }
    return 1;
  }();



  // EndpointStateUnix
  csm::network::EndpointStateUnix epbuf_unix;
  csm::network::MessageAndAddress MsgAddr;

  rc += TEST( epbuf_unix.GetAddr(), nullptr );

  sockaddr_un s;
  bzero(&s, sizeof(sockaddr_un));
  memcpy( s.sun_path, CSM_NETWORK_LOCAL_SSOCKET, strnlen( CSM_NETWORK_LOCAL_SSOCKET, UNIX_PATH_MAX ) );
  epbuf_unix.UpdateAddress(&s);

  rc += TEST( epbuf_unix.GetAddr().get()->Dump(), CSM_NETWORK_LOCAL_SSOCKET);

  // Update, Recv
  len = GenerateMsgBuffer(epbuf_unix.GetRecvBufferPtr(), RandomStringBuffer(), DGRAM_PAYLOAD_MAX);
  epbuf_unix.Update(len, (struct sockaddr*)&s);
  rc += TEST( epbuf_unix.GetAddr().get()->Dump(), CSM_NETWORK_LOCAL_SSOCKET );
  rc += TEST( epbuf_unix.IsEmpty(), false );


  // Recv
  epbuf_unix.Recv(MsgAddr);
  rc += TEST( MsgAddr._Msg.GetDataLen(), len-sizeof(csm_network_header) );
  rc += TEST( MsgAddr.GetAddr().get()->Dump(), CSM_NETWORK_LOCAL_SSOCKET );
  rc += TEST( epbuf_unix.GetAddr().get()->Dump(), CSM_NETWORK_LOCAL_SSOCKET );
  rc += TEST( epbuf_unix.IsEmpty(), true );

  std::cout << "Exiting with rc=" << rc << std::endl;
  return rc;
}

