/*================================================================================

    csmnet/src/CPP/csm_message_and_address.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMNET_SRC_CPP_CSM_MESSAGE_AND_ADDRESS_H_
#define CSMNET_SRC_CPP_CSM_MESSAGE_AND_ADDRESS_H_

#include <memory>
#include "csmnet/src/CPP/address.h"
#include "csmnet/src/CPP/csm_network_msg_cpp.h"


namespace csm {
namespace network {

class MessageAndAddress {
  csm::network::Address_sptr _Addr;
public:
  csm::network::Message _Msg;

  MessageAndAddress() : _Addr( nullptr ) {}
  ~MessageAndAddress()
  {
    _Addr = nullptr;
  }
  MessageAndAddress( const csm::network::Message &aMsg, const csm::network::Address_sptr aAddr )
  : _Addr( aAddr), _Msg( aMsg )
  { }
  MessageAndAddress( const MessageAndAddress &in )
  : _Addr( in._Addr), _Msg( in._Msg )
  { }
  MessageAndAddress& operator=( const MessageAndAddress &in )
  {
    _Msg = in._Msg;
    _Addr = in._Addr;
    return *this;
  }
  void Init( const csm::network::Message &aMsg, const csm::network::Address_sptr aAddr )
  {
    _Addr = aAddr;
    _Msg = aMsg;
  }
  void SetAddr( const csm::network::Address_sptr aAddr )
  {
    _Addr = aAddr;
  }
  csm::network::Address_sptr GetAddr() const { return _Addr; }
};

template<class stream>
static stream&
operator<<( stream &out, const MessageAndAddress &data )
{
//  csm::network::AddressUnix *addr = (csm::network::AddressUnix*)(data._Addr);
  out << " Msg: "<< data._Msg << std::endl;
      // << " Addr: "<< addr->Dump();
  return (out);
}

typedef std::shared_ptr< MessageAndAddress > MessageAndAddress_sptr;

} // namespace csm::network
}

#endif /* CSMNET_SRC_CPP_CSM_MESSAGE_AND_ADDRESS_H_ */
