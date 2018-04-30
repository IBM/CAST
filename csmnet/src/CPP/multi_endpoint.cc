/*================================================================================

    csmnet/src/CPP/multi_endpoint.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <logging.h>
#include "csm_network_config.h"
#include "csm_network_exception.h"
#include "address.h"
#include "csm_network_msg_cpp.h"
#include "csm_message_and_address.h"

#include "endpoint_options.h"
#include "endpoint.h"
#include "endpoint_unix.h"
#include "endpoint_dual_unix.h"
#include "endpoint_ptp.h"
#include "endpoint_utility.h"
#include "endpoint_aggregator.h"
#include "endpoint_multi_unix.h"
#include "network_ctrl_path.h"
#include "multi_endpoint.h"


csm::network::MultiEndpoint::MultiEndpoint()
: _EPL(), _Epoll( EPOLLIN | EPOLLERR | EPOLLRDHUP | EPOLLPRI ),
  _PassiveEPL(), _PassiveEpoll( EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLRDHUP | EPOLLPRI ),
  _OutboundPipe( nullptr ),
  _ActivityEpoll( EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLRDHUP | EPOLLPRI ),
  _Unix( nullptr )
{
  _OutboundPipe = new csm::network::EndpointPipe(nullptr, nullptr);
  _ActivityEpoll.Add( _OutboundPipe->GetSocket(), _OutboundPipe );
  _ActivityEpoll.Add( _PassiveEpoll.GetEpollSocket(), &_PassiveEpoll );
  _ActivityEpoll.Add( _Epoll.GetEpollSocket(), &_Epoll );
}

csm::network::MultiEndpoint::MultiEndpoint( const csm::network::MultiEndpoint * aEndpoint )
: _EPL( aEndpoint->_EPL ), _Epoll( EPOLLIN | EPOLLERR | EPOLLRDHUP | EPOLLPRI | EPOLLET ),
  _PassiveEPL( aEndpoint->_PassiveEPL ), _PassiveEpoll( EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLRDHUP | EPOLLPRI ),
  _ActivityEpoll( EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLRDHUP | EPOLLPRI ),
  _Unix( aEndpoint->_Unix )
{
  for( auto & it: _PassiveEPL )
  {
    csm::network::Endpoint *ep = dynamic_cast<csm::network::Endpoint*>( it.second );
    if( ! ep )
      continue;

    _PassiveEpoll.Add( ep );
  }
  for( auto & it: _EPL )
  {
    csm::network::Endpoint *ep = dynamic_cast<csm::network::Endpoint*>( it.second );
    if( ! ep )
      continue;

    _Epoll.Add( ep );
  }
  _OutboundPipe = aEndpoint->_OutboundPipe;
  _ActivityEpoll.Add( _OutboundPipe->GetSocket(), _OutboundPipe );
  _ActivityEpoll.Add( _PassiveEpoll.GetEpollSocket(), &_PassiveEpoll );
  _ActivityEpoll.Add( _Epoll.GetEpollSocket(), &_Epoll );
}

csm::network::MultiEndpoint::~MultiEndpoint( ) noexcept(false)
{
  Clear();
  _ActivityEpoll.Del( _Epoll.GetEpollSocket() );
  _ActivityEpoll.Del( _PassiveEpoll.GetEpollSocket() );
  _ActivityEpoll.Del( _OutboundPipe->GetSocket() );
}

csm::network::Endpoint* csm::network::MultiEndpoint::NewEndpoint( const csm::network::Address_sptr aAddr,
                                                                  const csm::network::EndpointOptions_sptr aOptions )
{
  if(( ! aAddr )||( ! aOptions ))
    return nullptr;
  csm::network::Endpoint *nEP = nullptr;
  switch ( aAddr->GetAddrType() )
  {
    case CSM_NETWORK_TYPE_LOCAL:
      nEP = new csm::network::EndpointMultiUnix( aAddr, aOptions );
      break;
    case CSM_NETWORK_TYPE_PTP:
      if( std::dynamic_pointer_cast<csm::network::EndpointOptionsPTP<csm::network::AddressPTP>>(aOptions)->HasSSLInfo() )
        nEP = new csm::network::EndpointCompute_sec( aAddr, aOptions );
      else
        nEP = new csm::network::EndpointCompute_plain( aAddr, aOptions );
      break;
    case CSM_NETWORK_TYPE_UTILITY:
      if( std::dynamic_pointer_cast<csm::network::EndpointOptionsPTP<csm::network::AddressUtility>>(aOptions)->HasSSLInfo() )
        nEP = new csm::network::EndpointUtility_sec( aAddr, aOptions );
      else
        nEP = new csm::network::EndpointUtility_plain( aAddr, aOptions );
      break;
    case CSM_NETWORK_TYPE_AGGREGATOR:
      if( std::dynamic_pointer_cast<csm::network::EndpointOptionsPTP<csm::network::AddressAggregator>>(aOptions)->HasSSLInfo() )
        nEP = new csm::network::EndpointAggregator_sec( aAddr, aOptions );
      else
        nEP = new csm::network::EndpointAggregator_plain( aAddr, aOptions );
      break;
    case CSM_NETWORK_TYPE_UNKNOWN:
    case CSM_NETWORK_TYPE_ABSTRACT:
    case CSM_NETWORK_TYPE_MAX:
      throw csm::network::Exception("MultiEndpoint::NewEndpoint(): Unsupported address type: ");
  }

  std::lock_guard<std::mutex> guard( _EndpointLock );
  CheckAndAddEndpoint( nEP );

  return nEP;
}

csm::network::Endpoint* csm::network::MultiEndpoint::NewEndpoint( csm::network::Endpoint *aEndpoint )
{
  std::lock_guard<std::mutex> guard( _EndpointLock );
  if( aEndpoint )
  {
    if( ! aEndpoint->IsServerEndpoint() )
    {
      if( aEndpoint->GetRemoteAddr() == nullptr )
        throw csm::network::Exception("MultiEndpoint::NewEndpoint() requires remote address to insert new EP!");
    }

    CheckAndAddEndpoint( aEndpoint );

    return aEndpoint;
  }
  return nullptr;
}

// requires the lock to be held and aAddr to be non-null !!!
int csm::network::MultiEndpoint::DeleteEndpointNoLock( const Address *aAddr, const std::string &where )
{
  bool notAvail = true;
  csm::network::AddressCode code = aAddr->MakeKey();
  csm::network::Endpoint *ep = nullptr;

  csm::network::EndpointList::iterator it = _EPL.find( code );
  if( it != _EPL.end() )
  {
    notAvail = false;
    ep = it->second;
    _EPL.erase( code ); // delete the entry regardless of ep exists or not (the entry exists and needs to go)
    if( ep != nullptr )
    {
      LOG( csmnet, info ) << where << ":Endpoint removal: " << aAddr->Dump()
        << " " << ( ep->IsVerified() || ep->IsServerEndpoint() ? "" : "UNVERIFIED endpoint") << " remaining=" << _EPL.size();

      // non-server unix endpoints are not tracked via epoll
      if(( ep == _Unix ) || ( ep->GetAddrType() != csm::network::AddressType::CSM_NETWORK_TYPE_LOCAL ))
        _Epoll.Del( ep );
      LOG( csmnet, debug ) << "Removing endpoint address :" << ep->GetLocalAddr()->Dump()
            << " -- " << (ep->GetRemoteAddr() != nullptr ? ep->GetRemoteAddr()->Dump() : "n/a");
    }
    else
      throw csm::network::ExceptionFatal("BUG: found nullptr endpoint in endpoint list for addr: " + aAddr->Dump(), ENOENT );

    if( ep == _Unix )
      _Unix = nullptr;
    delete ep;
  }

  it = _PassiveEPL.find( code );
  if( it != _PassiveEPL.end() )
  {
    notAvail = false;
    ep = it->second;
    _PassiveEPL.erase( code );
    if( ep != nullptr )
    {
      LOG( csmnet, info ) << where << ":Passive Endpoint removal: " << aAddr->Dump()
        << " remaining=" << _PassiveEPL.size();

      _PassiveEpoll.Del( ep );
      LOG( csmnet, debug ) << "Removing passive endpoint address: " << ep->GetLocalAddr()->Dump();
    }
    else
      throw csm::network::ExceptionFatal(where+"BUG: found nullptr endpoint in passive endpoint list for addr: " + aAddr->Dump() );

    if( ep == _Unix )
      _Unix = nullptr;
    delete ep;
  }

  if( notAvail )
  {
    if( aAddr->GetAddrType() != csm::network::AddressType::CSM_NETWORK_TYPE_LOCAL )
    {
      LOG( csmnet, warning ) << "DELETE: Trying to delete a non-tracked endpoint." << aAddr->Dump();
    }
  }
  return 0;
}

int csm::network::MultiEndpoint::DeleteEndpoint( const Address *aAddr, const std::string &where )
{
  if( aAddr )
  {
    std::lock_guard<std::mutex> guard( _EndpointLock );
    return DeleteEndpointNoLock( aAddr, where );
  }
  else
  {
    throw csm::network::ExceptionFatal( "BUG: Trying to remove endpoint with nullptr address from:"+where, EINVAL );
  }
  return 0;
}

int csm::network::MultiEndpoint::DeleteEndpoint( const Endpoint *aEndpoint, const std::string &where )
{
  if( aEndpoint )
  {
    std::lock_guard<std::mutex> guard( _EndpointLock );
    return DeleteEndpointNoLock( aEndpoint->GetRemoteAddr().get(), where );
  }
  else
  {
    throw csm::network::ExceptionFatal( "BUG: Trying to remove nullptr endpoint from:"+where, EINVAL );
  }
  return 0;
}

int csm::network::MultiEndpoint::Clear()
{
  LOG(csmnet, debug) << "MultiEndpoint::Clear(): active EPs. size=" << _EPL.size();
  std::lock_guard<std::mutex> guard( _EndpointLock );
  std::deque<csm::network::Address*> deleteList;

  for( auto & it: _EPL )
  {
    const csm::network::Endpoint *ep = dynamic_cast<const csm::network::Endpoint*>( it.second );
    if( ! ep )
    {
      LOG( csmnet, warning ) << "MultiEndpoint::Clear(): potential problem with bookkeeping: found nullptr in active endpoint list.";
      continue;
    }

    AddCtrlEvent( csm::network::NET_CTL_DISCONNECT, ep->GetRemoteAddr() );
    deleteList.push_back( ep->GetRemoteAddr().get() );
  }

  while( ! deleteList.empty() )
  {
    csm::network::Address* delAddr = deleteList.front();
    LOG(csmnet, debug) << "MultiEndpoint::Clear(): cleaning ep type: " << delAddr->GetAddrType();
    try { DeleteEndpointNoLock( delAddr, "CLEAR" ); }
    catch( ... ) {}
    deleteList.pop_front();
  }

  _EPL.clear();

  LOG(csmnet, debug) << "MultiEndpoint::Clear(): passive EPs. size=" << _PassiveEPL.size();
  for( auto & it: _PassiveEPL)
  {
    const csm::network::Endpoint *ep = dynamic_cast<const csm::network::Endpoint*>( it.second );
    if( ! ep )
    {
      LOG( csmnet, warning ) << "MultiEndpoint::Clear(): potential problem with bookkeeping: found nullptr in active endpoint list.";
      continue;
    }

    AddCtrlEvent( csm::network::NET_CTL_DISCONNECT, ep->GetLocalAddr() );
    deleteList.push_back( ep->GetLocalAddr().get() );
  }

  while( ! deleteList.empty() )
  {
    csm::network::Address* delAddr = deleteList.front();
    LOG(csmnet, debug) << "MultiEndpoint::Clear(): cleaning ep type: " << delAddr->GetAddrType();
    try { DeleteEndpointNoLock( delAddr, "CLEAR" ); }
    catch( ... ) {}
    deleteList.pop_front();
  }

  _PassiveEPL.clear();
  return 0;
}

csm::network::Endpoint* csm::network::MultiEndpoint::ProcessPassiveUnix( )
{
  csm::network::Endpoint *ret = nullptr;

  ret = _Unix->Accept();
  if( ret != nullptr )
  {
    if( ret->GetLocalAddr() && ret->GetRemoteAddr() )
    {
      LOG(csmnet, debug) << "MultiEndpoint::Accept(): new client connection: local=" << ret->GetLocalAddr()->Dump()
        << " remote="  << ( ret->GetRemoteAddr() != nullptr ? ret->GetRemoteAddr()->Dump() : "n/a" )
        << " on srv.type: " << _Unix->GetAddrType();
      AddCtrlEvent( csm::network::NET_CTL_UNVERIFIED, ret->GetRemoteAddr() );
    }
    else
    {
      LOG( csmnet, warning ) << "MultiEndpoint::Accept(): found incompletely initialized client endpoint. Ignoring. Client will time out";
      ret = nullptr;
    }
  }
  return ret;
}

csm::network::Endpoint* csm::network::MultiEndpoint::ProcessPassive( csm::network::Endpoint *aEndpoint,
                                                                     const uint32_t aEvents )
{
  if( aEndpoint == nullptr )
    return nullptr;

  csm::network::Endpoint *ret = nullptr;

  if( aEvents & (EPOLLERR | EPOLLRDHUP | EPOLLHUP))
  {
    LOG(csmnet, debug ) << "MultiEndpoint::ProcessPassive(): disconnect: " << aEndpoint->GetAddrType();
    AddCtrlEvent( csm::network::NET_CTL_DISCONNECT, aEndpoint->GetRemoteAddr() );
    DeleteEndpoint( aEndpoint, "ProcessPassive" );
    ret = nullptr;
  }
  else if( aEvents & EPOLLIN )
  {
    ret = aEndpoint->Accept();
    if( ret != nullptr )
    {
      LOG(csmnet, info) << "MultiEndpoint::ProcessPassive(): new connection: " << ret->GetRemoteAddr()->Dump()
        << " on srv.type: " << aEndpoint->GetAddrType()
        << " connections=" << _EPL.size();
      NewEndpoint( ret );
      AddCtrlEvent( csm::network::NET_CTL_UNVERIFIED, ret->GetRemoteAddr() );
    }
  }
  return ret;
}

csm::network::Endpoint* csm::network::MultiEndpoint::Accept( const bool aBlocking )
{
  csm::network::Endpoint *ret = nullptr;
  uint32_t events;
  csm::network::Endpoint *ep = _PassiveEpoll.NextActiveEP( aBlocking, &events );

  // in case there was no other activity, check if we have any unix sockets to add
  if( ep == nullptr )
  {
    try
    {
      if(_Unix != nullptr )
        ret = ProcessPassiveUnix( );
      if( ret != nullptr )
        ep = _Unix;
    }
    catch( csm::network::ExceptionProtocol &e )
    {
      LOG( csmnet, warning ) << e.what();
    }
  }
  else // regular accept case/activity on a passive socket
  {
    try {
      ret = ProcessPassive( ep, events );
      _PassiveEpoll.AckEvent();
    }
    catch ( csm::network::Exception &e )
    {
      LOG( csmnet, warning ) << "MultiEndpoint::Accept(): Accept error: " << e.what();
      _PassiveEpoll.AckEvent();
      ret = nullptr;
    }
  }
  return ret;
}

  // message based send
ssize_t
csm::network::MultiEndpoint::SendTo( const csm::network::MessageAndAddress &aMsgAddr )
{
  return SendTo( aMsgAddr._Msg, aMsgAddr.GetAddr() );
}

ssize_t
csm::network::MultiEndpoint::SendTo( const csm::network::Message &aMsg,
                                     const csm::network::Address_sptr aRemoteAddr )
{
  size_t rc = 0;
  csm::network::Endpoint *ep = GetEndpoint( aRemoteAddr );
  if( ep != nullptr )
  {
    try
    {
      rc = ep->SendTo( aMsg, aRemoteAddr );
    }
    catch( csm::network::Exception &e )
    {
      LOG( csmnet, warning ) << "MultiEndpoint::SendTo: " << e.what();
      AddCtrlEvent( csm::network::NET_CTL_DISCONNECT, ep->GetRemoteAddr() );
      DeleteEndpoint( ep, "Recv:Sync" );
      rc = -e.GetErrno();
    }
  }
  else
    return -ENOTCONN;
  return rc;
}


// message based receive
ssize_t
csm::network::MultiEndpoint::RecvFrom( csm::network::MessageAndAddress &aMsgAddr )
{
  uint32_t events = 0;
  csm::network::Endpoint *ep = _Epoll.NextActiveEP( false, &events );

  if( ep == nullptr )
    return 0;

  int rc = 0;

  // process any error activity (before attempting to send/recv)
  if( events & (EPOLLRDHUP|EPOLLERR) )
  {
    LOG( csmnet, info ) << "MultiEndpoint::Recv: shutdown or disconnect of "
        << (( ep->GetRemoteAddr() != nullptr ) ? ep->GetRemoteAddr()->Dump() : "empty" );
    aMsgAddr.SetAddr( ep->GetRemoteAddr() );
    AddCtrlEvent( csm::network::NET_CTL_DISCONNECT, ep->GetRemoteAddr() );
    DeleteEndpoint( ep, "Recv:HUP" );
    rc = 0;
    // warum schmeiß ich hier eine exception????
    //throw csm::network::ExceptionEndpointDown( "Endpoint disconnected", ENOTCONN );
  }
  else // process any non-error activity
  {
    // if there's requested outbound activity, make sure it's processed
    if( events & (EPOLLOUT) )
      try
      {
        PushBack( ep->Sync(csm::network::SYNC_ACTION_WRITE ) );
      }
      catch( csm::network::Exception &e )
      {
        LOG( csmnet, warning ) << "MultiEndpoint::Recv/Sync: " << e.what();
        AddCtrlEvent( csm::network::NET_CTL_DISCONNECT, ep->GetRemoteAddr() );
        DeleteEndpoint( ep, "Recv:Sync" );
        _Epoll.AckEvent();
        throw;
      }

    if( events & EPOLLIN )
    {
      try
      {
        rc = ep->RecvFrom( aMsgAddr );
        LOG( csmnet, debug ) << "MultiEndpoint::Recv(): msgId: " << aMsgAddr._Msg.GetMessageID()
          << " cmd: " << csm::network::cmd_to_string( aMsgAddr._Msg.GetCommandType() );
        if( ep == _Unix )
          ProcessPassiveUnix();
      }
      catch ( csm::network::ExceptionFatal &e )
      {
        LOG( csmnet, error ) << "MultiEndpoint::RecvFrom(): " << e.what();
        AddCtrlEvent( csm::network::NET_CTL_FATAL, ep->GetRemoteAddr() );
        DeleteEndpoint( ep, "Recv:Fatal" );
        _Epoll.AckEvent();
        throw;
      }
      catch ( csm::network::Exception &e )
      {
        if( aMsgAddr.GetAddr() == nullptr )
          throw csm::network::Exception("BUG: RecvFrom is supposed to return the address of the down endpoint.");

        // permission errors will be handled by ReliableMsg and won't cause the endpoint to disconnect
        if( e.GetErrno() == EPERM )
          throw;

        if(( aMsgAddr.GetAddr()->GetAddrType() != csm::network::CSM_NETWORK_TYPE_LOCAL ) && ( e.GetErrno() != 0 ))
        { LOG( csmnet, info ) << "Disconnect addr=:" << aMsgAddr.GetAddr()->Dump() << " :" << e.what();  }
        else { LOG( csmnet, debug ) << "Disconnect addr=:" << aMsgAddr.GetAddr()->Dump() << " :" << e.what(); }

        AddCtrlEvent( csm::network::NET_CTL_DISCONNECT, aMsgAddr.GetAddr() );
        // do not delete the unix ep just because of a receive error
        if( ep->GetAddrType() != CSM_NETWORK_TYPE_LOCAL )
          DeleteEndpoint( ep, "Recv:Err" );
        else
          DeleteEndpoint( aMsgAddr.GetAddr().get(), "Recv:Err" );
        rc = 0;
      }
    }
  }
  _Epoll.AckEvent();
  return rc;
}

  /* data synchronization, e.g. flush any buffers */
int csm::network::MultiEndpoint::Sync( const csm::network::SyncAction aSync )
{
  int rc = 0;
  csm::network::Endpoint *ep = nullptr;
  std::deque<csm::network::Endpoint*> toDelEP;

  {
    std::lock_guard<std::mutex> guard( _EndpointLock );
    for( auto & it: _EPL )
    {
      ep = dynamic_cast<csm::network::Endpoint*>( it.second );
      if( ! ep )
        continue;

      try {
        csm::network::NetworkCtrlInfo *actions = ep->Sync( aSync );
        if( actions != nullptr )
          ++rc;
        PushBack( actions );
      }
      catch( csm::network::Exception &e )
      {
        if( ep->GetRemoteAddr() != nullptr )
        {
          AddCtrlEvent( csm::network::NET_CTL_DISCONNECT, ep->GetRemoteAddr() );
          ++rc;
        }
        toDelEP.push_back( ep ); // can't alter the list, while scanning it
      }
    }
  }

  // take care of broken endpoints
  while( ! toDelEP.empty() )
  {
    ep = toDelEP.front();
    toDelEP.pop_front();
    DeleteEndpoint( ep, "Sync:Active" );
  }

  {
    std::lock_guard<std::mutex> guard( _EndpointLock );
    for( auto & it: _PassiveEPL )
    {
      ep = dynamic_cast<csm::network::Endpoint*>( it.second );
      if( ! ep )
        continue;

      try
      {
        csm::network::NetworkCtrlInfo *actions = ep->Sync( aSync );
        if( actions != nullptr )
          ++rc;
        PushBack( actions );
      }
      catch( csm::network::Exception &e )
      {
        AddCtrlEvent( csm::network::NET_CTL_DISCONNECT, ep->GetLocalAddr() );
        ++rc;
        toDelEP.push_back( ep ); // can't alter the list, while scanning it
      }
    }
  }

  // take care of broken endpoints
  while( ! toDelEP.empty() )
  {
    ep = toDelEP.front();
    toDelEP.pop_front();
    DeleteEndpoint( ep, "SyncPassive" );
  }

  // Create any network events, e.g. events caused by disconnects, connects, OOB data, etc
  return rc;
}

csm::network::Endpoint* csm::network::MultiEndpoint::GetEndpoint( const csm::network::Address_sptr aAddress ) const
{
  if( aAddress == nullptr )
    return nullptr;

  // if it's an unknown local address, we shouldn't return the endpoint
  LOG( csmnet, trace ) << "GetEndpoint(): Searching for addr: " << aAddress->Dump();

  return GetEndpoint( aAddress->MakeKey() );
}

csm::network::Endpoint* csm::network::MultiEndpoint::GetEndpoint( const csm::network::AddressCode aKey ) const
{
  // if it's an unknown local address, we shouldn't return the endpoint
  LOG( csmnet, trace ) << "GetEndpoint(): Searching for addr key: " << aKey;
  csm::network::EndpointList::const_iterator epit = _EPL.find( aKey );
  if( epit != _EPL.end() )
  {
    return dynamic_cast<csm::network::Endpoint*>( epit->second );
  }
  if( _Unix != nullptr )
  {
    csm::network::Endpoint *ep = _Unix->GetEndpoint( aKey );
    if( ep != nullptr )
      return ep;
  }
  epit = _PassiveEPL.find( aKey );
  if( epit != _PassiveEPL.end() )
  {
    return dynamic_cast<csm::network::Endpoint*>( epit->second );
  }
  return nullptr;
}

void csm::network::MultiEndpoint::Log()
{
  csm::network::EndpointList::iterator it = _EPL.begin();
  LOG(csmnet, debug) << "MultiEndPoint List:";
  for( it = _EPL.begin(); it != _EPL.end(); ++it )
  {
    csm::network::Endpoint *ep = (csm::network::Endpoint*)( it->second );
    if( ! ep )
      LOG(csmnet, error) << "NULLPTR Endpoint - we're broken!!!!";

    if( ep && !ep->GetRemoteAddr() )
      LOG(csmnet, debug) << "MultiEP[ LOCAL ]: type="<< ep->GetAddrType()
        << " local_addr=" << ep->GetLocalAddr()->Dump();

    if( ep && ep->GetRemoteAddr() )
      LOG(csmnet, debug) << "MultiEP[" << ep->GetRemoteAddr()->MakeKey() << "]: type="<< ep->GetAddrType()
        << " addr=" << ep->GetRemoteAddr()->Dump();
  }
}

bool csm::network::MultiEndpoint::CheckAndAddEndpoint( csm::network::Endpoint *aEndpoint )
{
  if(( aEndpoint == nullptr ) || ( aEndpoint->GetLocalAddr() == nullptr ))
    throw csm::network::ExceptionEndpointDown("Failed to create endpoint.");

  // local EPs are going to be listed as non-passive regardless of being serverEP or not
  bool passive = (aEndpoint->IsServerEndpoint() && aEndpoint->GetAddrType() != csm::network::AddressType::CSM_NETWORK_TYPE_LOCAL );

  // check if we got the unix server endpoint here
  if(( aEndpoint->IsServerEndpoint() ) &&
      ( aEndpoint->GetAddrType() == csm::network::AddressType::CSM_NETWORK_TYPE_LOCAL ))
  {
    if( _Unix != nullptr )
      LOG( csmnet, warning ) << "Reassigning main unix endpoint. addr=" << _Unix->GetRemoteAddr()->Dump()
        << " to " << aEndpoint->GetLocalAddr()->Dump();
    _Unix = dynamic_cast<csm::network::EndpointMultiUnix*>( aEndpoint );
  }

  if(( ! passive )&& ( aEndpoint->GetRemoteAddr() == nullptr ))
  {
    delete aEndpoint;
    aEndpoint = nullptr;
    throw csm::network::ExceptionEndpointDown("Connected EP with empty remote address! deleting.");
  }

  csm::network::AddressCode code = 0;

  if( aEndpoint->GetRemoteAddr() != nullptr )
    code = aEndpoint->GetRemoteAddr()->MakeKey();
  else
    code = aEndpoint->GetLocalAddr()->MakeKey();

  if( code == 0 )
    throw csm::network::ExceptionEndpointDown("BUG: Trying to add endpoint with addressCode=0");

  LOG(csmnet, debug) << "MultiEndpoint::NewEndpoint(): addr=" << aEndpoint->GetLocalAddr()->Dump()
      << " socket=" << aEndpoint->GetSocket()
      << " key=" << code;
  if(( passive ) && ( aEndpoint != _Unix ))
  {
    csm::network::Address_sptr laddr = aEndpoint->GetLocalAddr();
    LOG(csmnet, info) << "New listening endpoint. Type=" << laddr->GetAddrType()
        << " addr=" << aEndpoint->GetLocalAddr()->Dump();
    csm::network::Endpoint *newep = (_PassiveEPL[ code ] = aEndpoint);
    if( newep != aEndpoint )
      throw csm::network::ExceptionFatal("BUG: AddressCode collision or bookkeeping problem. Endpoint with the same code already exists.");
    _PassiveEpoll.Add( aEndpoint );
  }
  else
  {
    LOG(csmnet, debug) << "New endpoint/connection. Type=" << aEndpoint->GetLocalAddr()->GetAddrType()
      << " addr=" << ( (aEndpoint->GetRemoteAddr() != nullptr) ? aEndpoint->GetRemoteAddr()->Dump() : "n/a" );
    csm::network::Endpoint *newep = ( _EPL[ code ] = aEndpoint );
    if( newep != aEndpoint )
      throw csm::network::ExceptionFatal("BUG: AddressCode collision or bookkeeping problem. Endpoint with the same code already exists.");

    if(( aEndpoint == _Unix ) || ( aEndpoint->GetAddrType() != csm::network::AddressType::CSM_NETWORK_TYPE_LOCAL ))
      _Epoll.Add( aEndpoint );
  }
  return true;
}

int
csm::network::MultiEndpoint::WaitForActivity()
{
  void *ep = _ActivityEpoll.WaitEvent( true );
  LOG( csmnet, trace ) << "WaitForActivity: woke up with ep=" << (void*)ep;
  errno = 0;
  if( ep == (void*)_OutboundPipe )
    while( _OutboundPipe->DataPending() );

  if( ep == nullptr )
    return 0;
  else
    return 1;
}

int
csm::network::MultiEndpoint::WakeUp()
{
  LOG( csmnet, trace ) << "WakeUp of netmgr.";
  return _OutboundPipe->Send( csm::network::Message() );
}


//////////////////////////////////////////////////////////////////////////////
// EPOLL Management wrappers

csm::network::EpollWrapper::EpollWrapper( const uint32_t aEventMask )
: _ActiveEvents( 0 ), _CurrentEvent( 0 ), _CompletedEvent( 0 ),
  _EventMask( aEventMask ), _ActiveEP( nullptr ), _ActiveEPevents( 0 )
{
  _CtrlSock = CreateEpollSocket();
  bzero( _EpollEvents, sizeof (struct epoll_event) * CSM_MULTENDPOINT_MAX_EVENTS );
}

csm::network::EpollWrapper::~EpollWrapper()
{
  close( _CtrlSock );
}

int csm::network::EpollWrapper::CreateEpollSocket()
{
  int ret = epoll_create( CSM_MULTENDPOINT_MAX_EVENTS );
  if( ret < 0 )
    throw csm::network::Exception("MultiEndpoint(): Failed to create epoll ctrl socket.");
  return ret;
}

int csm::network::EpollWrapper::Add ( const int aSocket, void *ptr )
{
  struct epoll_event ev;
  bzero( &ev, sizeof( struct epoll_event ) );
  ev.events = _EventMask;
  ev.data.ptr = ptr;
  int rc = epoll_ctl(_CtrlSock, EPOLL_CTL_ADD, aSocket, &ev);
  LOG( csmnet, debug ) << "MultiEndpoint::AddToEpoll(): epoll_ctl " << _CtrlSock << " added socket: " << aSocket;
  if ( rc == -1)
    throw csm::network::Exception("MultiEndpoint::AddToEpoll(): epoll_ctl failed to add new endpoint.");
  return rc;
}

int csm::network::EpollWrapper::Add( const csm::network::Endpoint *aEndpoint )
{
  std::lock_guard<std::mutex> guard( _Lock );
  if( !aEndpoint )
    throw csm::network::Exception("MultiEndpoint::AddToEpoll(): nullptr endpoint.");
  int socket = aEndpoint->GetSocket();
  if( socket <= 0 )  // don't add endpoints that return 0 or less
    return 0;

  return Add( socket, (void*)aEndpoint );
}

int csm::network::EpollWrapper::Del( const int aSocket )
{
  // remove from epoll ctrl socket
  struct epoll_event ev;
  ev.events = _EventMask;
  ev.data.ptr = nullptr;
  int rc = epoll_ctl(_CtrlSock, EPOLL_CTL_DEL, aSocket, &ev);
  if ( rc != 0 )
  {
    LOG( csmnet, debug ) << "MultiEndpoint::DelFromEpoll(): (ignore) epoll_ctl failed to remove endpoint with socket " << aSocket
    << " rc=" << errno;
  }
  else
  {
    LOG( csmnet, debug ) << "MultiEndpoint::DelFromEpoll(): epoll_ctl " << _CtrlSock << " removed socket: " << aSocket;
  }

  return rc;
}

int csm::network::EpollWrapper::DelNoLock( const csm::network::Endpoint *aEndpoint )
{
  if( !aEndpoint )
    throw csm::network::Exception("MultiEndpoint::DelFromEpoll(): nullptr endpoint.");

  int socket = aEndpoint->GetSocket();
  if( socket <= 0 )  // don't add endpoints that return 0 or less
    return 0;

  // remove/nullify any other events that might be in the current event list
  for( int evSlot = _ActiveEvents; evSlot > 0; --evSlot )
  {
    if( _EpollEvents[ evSlot ].data.ptr == aEndpoint )
    {
      _EpollEvents[ evSlot ].data.ptr = nullptr;
    }
  }

  // if we try to delete the current active EP, we need to wipe any pending events of that EP
  // and then reset the active EP to null
  if( _ActiveEP == aEndpoint )
  {
    LOG( csmnet, debug ) << "MultiEndpoint::DelFromEpoll(): requested endpoint is active";
//    FlushCurrentEvents();
    _ActiveEP = nullptr;
    _ActiveEPevents = 0;
  }

  return Del( socket );
}

void* csm::network::EpollWrapper::WaitEvent( const bool aBlock )
{
  void *ret = nullptr;
  int timeout = ( CSM_NETWORK_EVENT_TIMEOUT ) * (int)aBlock;
  _ActiveEvents = epoll_wait( _CtrlSock, _EpollEvents, 1, timeout );
  if( _ActiveEvents > 0 )
    ret = _EpollEvents[0].data.ptr;
  return ret;
}

static
std::string log_epoll_event( uint32_t events )
{
  std::string ret = std::to_string( events );
  if( events & EPOLLIN ) ret.append( "|IN" );
  if( events & EPOLLOUT ) ret.append( "|OUT" );
  if( events & EPOLLERR ) ret.append( "|ERR" );
  if( events & EPOLLHUP ) ret.append( "|HUP" );
  if( events & EPOLLPRI ) ret.append( "|PRI" );
  if( events & EPOLLRDHUP ) ret.append( "|RDHUP" );
  return ret;
}

// fetch the currently active event (or acquire a new array of events if empty)
csm::network::Endpoint * csm::network::EpollWrapper::NextActiveEP( const bool aBlock, uint32_t *aEvents )
{
  csm::network::Endpoint *ret = nullptr;
  int timeout = CSM_MILLISEC_TIMEOUT * (int)aBlock;
  std::lock_guard<std::mutex> guard( _Lock );

  if( _ActiveEP )
  {
    LOG(csmnet, debug) << "Endpoint still active: " << log_epoll_event( _ActiveEPevents );
    *aEvents = _ActiveEPevents;
    return _ActiveEP;
  }

  // don't return new events if the existing ones haven't been completed
  if( _CompletedEvent >= _ActiveEvents )
  {
    _ActiveEvents = epoll_wait( _CtrlSock, _EpollEvents, CSM_MULTENDPOINT_MAX_EVENTS, timeout );
    _CurrentEvent = 0;
    _CompletedEvent = 0;
    if( _ActiveEvents <= 0 )
      ret = nullptr;
    else
      LOG(csmnet, debug) << "sock=" << _CtrlSock << " New EPOLL events: " << _ActiveEvents;
  }

  if( _CurrentEvent < _ActiveEvents )
  {
    ret = (csm::network::Endpoint*)( _EpollEvents[ _CurrentEvent ].data.ptr );
    _ActiveEPevents = _EpollEvents[ _CurrentEvent ].events;

    ++_CurrentEvent;

    if( ret == nullptr )
    {
      LOG( csmnet, debug ) << "sock=" << _CtrlSock << "ActiveEvents list contains a nullptr. Endpoint has been deleted earlier.";
      _ActiveEP = nullptr;
      _ActiveEPevents = 0;
      AckEventNoLock();  // returning nullptr after an event was detected: need to ACK this event here
      ret = nullptr;
    }

    LOG(csmnet, debug) << "sock=" << _CtrlSock << " next EPOLL event: " << log_epoll_event( _ActiveEPevents )
        << " cnt=" << _CurrentEvent << "/" << _ActiveEvents << " @" << ((void*)ret == nullptr ? 0 : ret->GetSocket() );

    if( aEvents != nullptr )
      *aEvents = _ActiveEPevents;
  }
  _ActiveEP = ret;
  return ret;
}

int csm::network::EpollWrapper::FlushCurrentEvents()
{
  if( _ActiveEvents != _CompletedEvent )
  {
    _ActiveEvents = 0;
    _CompletedEvent = 0;
    _ActiveEP = nullptr;
    _ActiveEPevents = 0;
    return 1;
  }
  return 0;
}

// acknowledge the current event by forwarding indices to the next epoll event
int csm::network::EpollWrapper::AckEventNoLock( )
{
  if(( _ActiveEP ) && ( ! _ActiveEP->DataPending() ))
  {
    LOG(csmnet, debug) << "ActiveEP: no more data. Deactivating: " << _ActiveEP->GetAddrType();
    _ActiveEP = nullptr;
    _ActiveEPevents = 0;
  }

  // if the EP is still activated, we can't ACK this event - prevent underflow
  if( _ActiveEP )
    return 0;

  if( _CompletedEvent < _ActiveEvents )
    ++_CompletedEvent;
  else
    throw csm::network::Exception("AckEvent protocol error. Ack exceeds available epoll Events.");

  LOG(csmnet, debug) << "AckEvent() " << _CompletedEvent << "/" << _ActiveEvents;

  return 0;
}

