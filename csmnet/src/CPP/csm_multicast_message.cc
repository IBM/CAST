/*================================================================================

    csmnet/src/CPP/csm_multicast_message.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include "csm_multicast_message.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/classification.hpp>

namespace csm {
namespace network {

//Create a multi-cast message with a node list
bool CreateMulticastMessage(const csm::network::Message &msg, const std::vector<std::string> &node_list,
            csm::network::Message& outMsg)
{

    // first, combine a list of nodes to a long string separated by ;
    std::ostringstream oss;
    std::copy( node_list.begin(), node_list.end(), std::ostream_iterator<std::string>(oss, ";") );

    std::stringstream payload;
    // write a 4 byte integer number for the node string length
    uint32_t len = (uint32_t) (oss.str().length());
    payload.write((const char *)&len, sizeof(uint32_t));
    // write node list
    payload << oss.str();
    // write original payload
    payload << msg.GetData();

    // enable multicast flag
    uint8_t flags = msg.GetFlags() | CSM_HEADER_MTC_BIT;

    bool hdrvalid = outMsg.Init(msg.GetCommandType(),
                        flags,
                        msg.GetPriority(),
                        msg.GetMessageID(),
                        msg.GetSrcAddr(),
                        msg.GetDstAddr(),
                        msg.GetUserID(),
                        msg.GetGroupID(),
                        payload.str(),
                        msg.GetReservedID());

    if (!hdrvalid)
    {
      LOG(csmapi, error) << "CreateMultiCastMessage(): fail in Message.Init()...";
      return false;
    }
    else return true;
}

int ExtractMulticastNodelist( const csm::network::Message &msg,
                              std::vector< std::string > &node_list,
                              uint32_t *node_string_len )
{
  if (!msg.GetMulticast())
  {
    LOG(csmapi, error) << "DecodeMultiCastMessage: MTC is not set";
    return -1;
  }

  std::string payload = msg.GetData();
  const char *byte_payload = payload.c_str();

  // get the length of the node string list
  memcpy(node_string_len, byte_payload, sizeof(uint32_t));

  // tokenize the node string list
  std::string node_list_str = payload.substr(sizeof(uint32_t), *node_string_len);
  boost::trim_if(node_list_str, boost::is_any_of(";"));
  boost::split(node_list, node_list_str, boost::is_any_of(";"), boost::token_compress_on);

  return node_list.size();
}


bool DecodeMulticastMessage(const csm::network::Message &msg,
                            std::vector< std::string >& node_list, csm::network::Message &outMsg)
{
  uint32_t node_string_len;
  int node_count = ExtractMulticastNodelist( msg, node_list, &node_string_len );

  if( node_count < 0 )
    return false;

  // get the original payload after node string list
  std::string real_payload = msg.GetData().substr( sizeof(uint32_t)+node_string_len );

  // clear out the multicast bit
  uint8_t flags = msg.GetFlags();
  flags &= ~CSM_HEADER_MTC_BIT;

  // restore the original message
  bool hdrvalid = outMsg.Init(msg.GetCommandType(),
                              flags,
                              msg.GetPriority(),
                              msg.GetMessageID(),
                              msg.GetSrcAddr(),
                              msg.GetDstAddr(),
                              msg.GetUserID(),
                              msg.GetGroupID(),
                              real_payload,
                              msg.GetReservedID());

  if (!hdrvalid)
  {
    LOG(csmapi, error) << "CreateMultiCastMessage(): fail in Message.Init()...";
    return false;
  }
  else return true;
}

} // namespace network
} // namespace csm
