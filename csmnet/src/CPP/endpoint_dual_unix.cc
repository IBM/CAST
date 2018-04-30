/*================================================================================

    csmnet/src/CPP/endpoint_dual_unix.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <cstdio>
#include <exception>
#include <iostream>
#include <algorithm>

#include <sys/types.h>   // stat()
#include <sys/stat.h>    // stat()

#include <unistd.h>      // close()
#include <sys/socket.h>  // socket calls
#include <sys/un.h>      // struct sockaddr_un

#include <errno.h>

#include <logging.h>
#include "csm_network_config.h"
#include "csm_network_exception.h"
#include "csm_network_msg_cpp.h"
#include "endpoint.h"
#include "endpoint_unix.h"
#include "endpoint_dual_unix.h"

csm::network::EndpointDualUnix::EndpointDualUnix( const csm::network::Address_sptr aLocalAddr,
                                                  const csm::network::EndpointOptions_sptr aOptions )
: csm::network::Endpoint( aLocalAddr, aOptions ),
  _DualOptions( std::dynamic_pointer_cast<csm::network::EndpointOptionsDual>( aOptions ) ),
  _CallBackRefCount( 0 )
{
  std::string base_path = std::string( ((csm::network::AddressUnix*)aLocalAddr.get())->_SockAddr.sun_path );

  _Default = new EndpointUnix( aLocalAddr, aOptions );

  // create a secondary channel for callback traffic
  _Callback = new EndpointUnix( GenerateSecondaryAddress( base_path ), aOptions );

  _Connected = _Default->IsConnected() & _Callback->IsConnected();
  if( _Default->IsConnected() != _Callback->IsConnected() )
    throw csm::network::Exception("EndpointDualUnix: Conflicting connection state after socket creation.");

  SetRemoteAddr( _Default->GetRemoteAddr() );
}

csm::network::EndpointDualUnix::EndpointDualUnix( const std::string &aPath,
                                                  const csm::network::EndpointOptions_sptr aOptions )
: csm::network::Endpoint( nullptr, aOptions ),
  _DualOptions( std::dynamic_pointer_cast<csm::network::EndpointOptionsDual>( aOptions ) ),
  _CallBackRefCount( 0 )
{
  _Default = new EndpointUnix( aPath, aOptions );
  SetLocalAddr( _Default->GetLocalAddr() );

  // create a secondary channel for callback traffic
  _Callback = new EndpointUnix( GenerateSecondaryAddress( aPath ), aOptions );

  _Connected = _Default->IsConnected() & _Callback->IsConnected();
  if( _Default->IsConnected() != _Callback->IsConnected() )
    throw csm::network::Exception("EndpointDualUnix: Conflicting connection state after socket creation.");

  SetRemoteAddr( _Default->GetRemoteAddr() );
}

csm::network::EndpointDualUnix::EndpointDualUnix( const Endpoint *aEP )
: csm::network::Endpoint( aEP ),
  _DualOptions( std::dynamic_pointer_cast<csm::network::EndpointOptionsDual>( aEP->GetOptions() ) ),
  _CallBackRefCount( dynamic_cast<const csm::network::EndpointDualUnix*>(aEP)->_CallBackRefCount )
{
  _Default = new csm::network::EndpointUnix( dynamic_cast<const csm::network::EndpointDualUnix*>(aEP)->_Default );
  _Callback = new csm::network::EndpointUnix( dynamic_cast<const csm::network::EndpointDualUnix*>(aEP)->_Callback );
}

csm::network::EndpointDualUnix::~EndpointDualUnix()
{
  csm::network::Message DisconnectMsg;
  // before sending disconnect msg, make sure we're connected at all but not "self-connected"
  if(( IsConnected() ) && ( GetRemoteAddr()->MakeKey() != GetLocalAddr()->MakeKey() ))
  {
    DisconnectMsg.Init( CSM_CMD_STATUS,
                        CSM_HEADER_INT_BIT,
                        CSM_PRIORITY_WITH_ACK,
                        1, 0x1234, 0x1234,
                        geteuid(), getegid(),
                        std::string( CSM_DISCONNECT_MSG ));
    _Default->Send( DisconnectMsg );
    _Callback->Send( DisconnectMsg );
    _Default->Recv( DisconnectMsg );
    _Callback->Recv( DisconnectMsg );
  }

  if( _Default ) delete _Default;
  if( _Callback ) delete _Callback;
}

int
csm::network::EndpointDualUnix::Connect( const Address_sptr aSrvAddr )
{
  int rc = _Default->Connect( aSrvAddr );
  rc += _Callback->Connect( GenerateSecondaryAddress( aSrvAddr ) );
  if( ! rc )
  {
    SetRemoteAddr( aSrvAddr );
    _Connected = _Default->IsConnected() & _Callback->IsConnected();
    if( _Default->IsConnected() != _Callback->IsConnected() )
      throw csm::network::Exception("EndpointDualUnix: Conflicting connection state after socket creation.");
  }
  return rc;
}

ssize_t
csm::network::EndpointDualUnix::RecvFrom( csm::network::MessageAndAddress &aMsgAddr )
{
  int rc = _Callback->RecvFrom( aMsgAddr );
  if( !rc )
    rc = _Default->RecvFrom( aMsgAddr );
  else
    aMsgAddr.SetAddr( _Default->GetRemoteAddr() ); // never expose the CB address to upper layers
  return rc;
}
ssize_t
csm::network::EndpointDualUnix::Recv( csm::network::Message &aMsg )
{
  int rc = _Callback->Recv( aMsg );
  if( !rc )
    rc = _Default->Recv( aMsg );

  return rc;
}
ssize_t
csm::network::EndpointDualUnix::SendTo( const csm::network::Message &aMsg,
                                        const csm::network::Address_sptr aRemoteAddr )
{
  if( aMsg.GetCbk() )
  {
    csm::network::AddressUnix_sptr cb_addr = AddressToCBChannel( aRemoteAddr );
    LOG(csmnet, debug) << "Sending through CB channel.: " << cb_addr->Dump();
    size_t rc = _Callback->SendTo( aMsg, cb_addr );
    return rc;
  }
  else
    return _Default->SendTo( aMsg, aRemoteAddr );
}
ssize_t
csm::network::EndpointDualUnix::Send( const csm::network::Message &aMsg )
{
  if( aMsg.GetCbk() )
    return _Callback->Send( aMsg );
  else
    return _Default->Send( aMsg );
}
csm::network::NetworkCtrlInfo*
csm::network::EndpointDualUnix::Sync( const csm::network::SyncAction aSync )
{
  csm::network::NetworkCtrlInfo *ret = _Callback->Sync();
  if( ! ret )
    ret = _Default->Sync();
  else
  {
    csm::network::NetworkCtrlInfo *tail = ret;
    while( tail->_Next != nullptr )
      tail = tail->_Next;
    tail->_Next = _Default->Sync();
  }
  return ret;
}
