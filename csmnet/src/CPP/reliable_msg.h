/*================================================================================

    csmnet/src/CPP/reliable_msg.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMNET_SRC_CPP_RELIABLE_MSG_H_
#define CSMNET_SRC_CPP_RELIABLE_MSG_H_

#include <chrono>
#include <ctime>
#include <queue>

#include "csm_network_config.h"
#include "csmnet/src/CPP/address.h"
#include "csmnet/src/CPP/endpoint.h"
#include "csmnet/src/CPP/multi_endpoint.h"
#include "csmnet/src/CPP/message_ack.h"

namespace csm {
namespace network {

#define CSM_NETWORK_ADDRESS_ERROR ((uint64_t)0xFFFFFFFFFFFFFFFFull)

class ReliableMsg : public csm::network::MultiEndpoint
{
  csm::network::MessageACK _AckMgr;
  csm::network::MessageAndAddress _BufferedMsg;
  bool _BufferedAvailable;

public:
  ReliableMsg( )
  : csm::network::MultiEndpoint( ),
    _AckMgr(  ),
    _BufferedAvailable( false )
  {
  }


  ReliableMsg( const ReliableMsg * aEndpoint )
  : csm::network::MultiEndpoint( dynamic_cast<const csm::network::MultiEndpoint*>(aEndpoint) ),
    _AckMgr( aEndpoint->_AckMgr ),
    _BufferedMsg( aEndpoint->_BufferedMsg ),
    _BufferedAvailable( aEndpoint->_BufferedAvailable )
  { }
  ~ReliableMsg( ) { }

  //////////////////////////////////////////
  // Communication

  // message based send
  ssize_t SendTo( const csm::network::MessageAndAddress &aMsgAddr );

  // message based receive
  ssize_t RecvFrom( csm::network::MessageAndAddress &aMsgAddr );

  /* data synchronization, e.g. check for ACKs, flush any buffers, send/recv pending requests, ... */
  int Sync( const csm::network::SyncAction aSync );

  void SetDefaultTimeout( const uint32_t aTimeout )
  {
    _AckMgr.SetDefaultTimeout( aTimeout );
  }
  uint64_t GetPendingACKCount() const
  {
    return _AckMgr.GetAckCount();
  }

private:
  int RespondWithImmediateError( const csm::network::Exception &e, csm::network::MessageAndAddress &aMsgAddr );
  void GenerateAndSendACK( const csm::network::MessageAndAddress &aMsgAddr, const bool KeepErrorFlag = false );
};

}  // network
} // csm


#endif /* CSMNET_SRC_CPP_RELIABLE_MSG_H_ */
