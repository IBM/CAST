/*================================================================================

    csmnet/tests/csm_multicast_msg_test.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <iostream>
#include "csmutil/include/csm_test_utils.h"
#include "CPP/csm_multicast_message.h"

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
                        geteuid(), getegid(),
                        "Hello World"), true );

  std::vector< std::string > node_list;
  node_list.push_back( std::string("node1") );
  node_list.push_back( std::string("node2") );

  csm::network::Message outMsg;
  bool ret = CreateMulticastMessage(msg, node_list, outMsg);
  rc += TEST(ret, true);

  rc += TEST( outMsg.GetAck(), false );   // ack properly set?
  rc += TEST( outMsg.GetMulticast(), true );  // mulitcast set?

  node_list.clear();
  csm::network::Message orgMsg;
  ret = DecodeMulticastMessage(outMsg, node_list, orgMsg);

  for (size_t i=0; i<node_list.size(); i++)
  {
    std::cout << "node " << i << " = " << node_list[i] << std::endl;
  }

  rc += TEST( node_list.size(), 2 );
  rc += TEST( orgMsg.GetAck(), msg.GetAck() );
  rc += TEST( orgMsg.GetMulticast(), false );
  rc += TEST( orgMsg.GetMessageID(), msg.GetMessageID() );
  rc += TEST( orgMsg.GetPriority(), msg.GetPriority() );

  std::cout << "GetData after decoding: \"" << orgMsg.GetData() << "\"" << std::endl;

  std::cout << " rc = " << rc << std::endl;
  return rc;
}
