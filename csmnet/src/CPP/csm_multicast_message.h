/*================================================================================

    csmnet/src/CPP/csm_multicast_message.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMNET_SRC_CPP_CSM_MULTICAST_MESSAGE_H_
#define CSMNET_SRC_CPP_CSM_MULTICAST_MESSAGE_H_

#include "csmnet/src/CPP/csm_network_msg_cpp.h"
#include "logging.h"
#include <vector>

namespace csm {
namespace network {

bool CreateMulticastMessage(const csm::network::Message &msg, const std::vector<std::string> &node_list,
            csm::network::Message& outMsg);

int ExtractMulticastNodelist( const csm::network::Message &msg,
                              std::vector< std::string > &node_list,
                              uint32_t *node_string_len );

bool DecodeMulticastMessage(const csm::network::Message &msg,
                            std::vector< std::string >& node_list, csm::network::Message &outMsg);

} // namespace network
} //namespace csm
#endif
