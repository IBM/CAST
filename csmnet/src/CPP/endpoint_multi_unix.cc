/*================================================================================

    csmnet/src/CPP/endpoint_multi_unix.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "logging.h"
#include "csmutil/include/csm_version.h"
#include "endpoint_multi_unix.h"
#include "csmi/src/common/include/csmi_cmds.h"

csm::network::EndpointMultiUnix::EndpointMultiUnix( const Address_sptr aLocalAddr,
                                                    const EndpointOptions_sptr aOptions )
: csm::network::EndpointDualUnix( aLocalAddr, aOptions ),
  _Heartbeat( CSM_NETWORK_HEARTBEAT_INTERVAL, CSM_PRIORITY_NO_ACK ),
  _UnixOptions( std::dynamic_pointer_cast<EndpointOptionsUnix>( aOptions ) )
{
  if( IsConnected() )
    CheckAndAddRemoteAddress( _RemoteAddr, true );
  else if( !IsServerEndpoint() )
    CheckAndAddRemoteAddress( _LocalAddr );   // register with local address if not connected

  // if the endpoint is a server, then the remote addr is just copied
  if( IsServerEndpoint() )
    SetRemoteAddr( _LocalAddr );
}

csm::network::EndpointMultiUnix::EndpointMultiUnix( const std::string &aPath,
                                                    const EndpointOptions_sptr aOptions )
: csm::network::EndpointDualUnix( aPath, aOptions ),
  _Heartbeat( CSM_NETWORK_HEARTBEAT_INTERVAL, CSM_PRIORITY_NO_ACK )
{
  if( IsConnected() )
    CheckAndAddRemoteAddress( _RemoteAddr, true );
  else if( !IsServerEndpoint() )
    CheckAndAddRemoteAddress( _LocalAddr );   // register with local address if not connected

  // if the endpoint is a server, then the remote addr is just copied
  if( IsServerEndpoint() )
    SetRemoteAddr( _LocalAddr );
}

csm::network::EndpointMultiUnix::~EndpointMultiUnix()
{
  _ToAccept.clear();
  _Removed.clear();
  for( auto &it : _Known )
  {
    delete it.second;
  }
  _Known.clear();
}

/* Connects to a server and returns the address
 */
int
csm::network::EndpointMultiUnix::Connect( const csm::network::Address_sptr aSrvAddr )
{
  if( ! IsServerEndpoint() )
  {
    int rc = csm::network::EndpointDualUnix::Connect( aSrvAddr );
    if( !rc )
      CheckAndAddRemoteAddress( aSrvAddr, true );
    return rc;
  }
  throw csm::network::Exception("Attempted to connect passive socket.");
  return -1;
}

csm::network::Endpoint*
csm::network::EndpointMultiUnix::Accept( )
{
  if( IsServerEndpoint() && ! _ToAccept.empty() )
  {
    csm::network::EndpointVirtualUnix *ret =  _ToAccept.front();
    _ToAccept.pop_front();

    if( ret == nullptr )
      throw csm::network::ExceptionProtocol("ERROR local handshake event: Accept on non-empty list returned nullptr.", EPROTO );

    if( ret->GetRemoteAddr() == nullptr )
      throw csm::network::ExceptionProtocol("ERROR local handshake event: Accept on new endpoint with nullptr address.", EBADF );

    if( !CheckRemoteAddress( ret->GetRemoteAddr() ) )
    {
      LOG( csmnet, debug ) << "Late Accept: client is already disconnected.";
      return nullptr;
    }
    LOG(csmnet, debug) << "MultiUnix::Accepting newly detected client: " << ret->GetRemoteAddr()->Dump();

    // send Accept msg response to peer
    csm::network::Message AcceptMsg;
    AcceptMsg.Init( CSM_CMD_STATUS,
                    CSM_HEADER_INT_BIT | CSM_HEADER_RESP_BIT,
                    CSM_PRIORITY_NO_ACK,
                    1, 0x1234, 0x1234,
                    geteuid(), getegid(),
                    _UnixOptions->_APIConfig->GetSerializedTimeouts() );
    try
    {
      ret->SendTo( AcceptMsg, ret->GetRemoteAddr() );
      ret->Connect( ret->GetRemoteAddr() );
    }
    catch ( csm::network::Exception &e )
    {
      LOG( csmnet, debug ) << "Newly detected client endpoint already down: " << ret->GetRemoteAddr()->Dump();
      RemoveFromClientRegistry( ret->GetRemoteAddr() );
      ret = nullptr;  // if endpoint is down, we cannot return any new endpoint
    }

    return dynamic_cast<csm::network::Endpoint*>(ret);
  }
  return nullptr;
}

// message based send
ssize_t
csm::network::EndpointMultiUnix::SendTo( const csm::network::Message &aMsg,
                                         const csm::network::Address_sptr aRemoteAddr )
{
  try
  {
    if( ! CheckRemoteAddress( aRemoteAddr ) )
      throw csm::network::ExceptionEndpointDown("MultiUnix::Attempt to send to unknown client." );
    return csm::network::EndpointDualUnix::SendTo( aMsg, aRemoteAddr );
  }
  catch ( csm::network::ExceptionEndpointDown &e )
  {
    LOG(csmnet, warning) << "MultiUnix::Remote endpoint down." << aRemoteAddr->Dump();
    RemoveFromClientRegistry( aRemoteAddr );
    throw e;
  }
}
ssize_t
csm::network::EndpointMultiUnix::Send( const csm::network::Message &aMsg )
{
  return csm::network::EndpointDualUnix::Send( aMsg );
}

// message based receive
ssize_t
csm::network::EndpointMultiUnix::RecvFrom( csm::network::MessageAndAddress &aMsgAddr )
{
  try
  {
    ssize_t ret = csm::network::EndpointDualUnix::RecvFrom( aMsgAddr );

    if( ret == 0 )
      return ret;
    if( ret < 0 )
      throw csm::network::ExceptionRecv("Receive error in MultiUnix endpoint.", -ret);

    bool known = CheckAndAddRemoteAddress( aMsgAddr.GetAddr(), true );
    if(( ! known ) && ( aMsgAddr._Msg.GetCommandType() != CSM_CMD_STATUS ))
    {
      ret = 0;
      RemoveFromClientRegistry( aMsgAddr.GetAddr() );
    }


    if( aMsgAddr._Msg.GetCommandType() == CSM_CMD_STATUS )
    {
      if( aMsgAddr._Msg.GetData().compare( CSM_DISCONNECT_MSG ) == 0 )
      {
        LOG( csmnet, debug ) << "Creating disconnect ACK for " << aMsgAddr.GetAddr()->Dump();
        // send Accept msg response to peer
        csm::network::Message DisconnectResp;
        DisconnectResp.Init( CSM_CMD_STATUS,
                             CSM_HEADER_INT_BIT | CSM_HEADER_ACK_BIT | CSM_HEADER_RESP_BIT,
                             CSM_PRIORITY_NO_ACK,
                             aMsgAddr._Msg.GetMessageID(), 0x1234, 0x1234,
                             geteuid(), getegid(),
                             "" );
        try
        {
          SendTo( DisconnectResp, aMsgAddr.GetAddr() );
        }
        catch ( csm::network::Exception &e )
        {
          LOG( csmnet, debug ) << "Unable to send Disconnect-ACK while shutting down client link. Endpoint already down: "
             << (( aMsgAddr.GetAddr() != nullptr ) ? aMsgAddr.GetAddr()->Dump() : "unknown.");
          RemoveFromClientRegistry( aMsgAddr.GetAddr() );
        }
        throw csm::network::ExceptionEndpointDown("Clean Disconnect", 0 ); // throw exception to trigger the disconnect event creation
      }
    }
    if( aMsgAddr._Msg.GetCommandType() == CSM_CMD_HEARTBEAT )
    {
      if(( aMsgAddr._Msg.GetAck() ) && known )
      {
        LOG( csmnet, debug ) << "Received Heartbeat ACK from " <<  aMsgAddr.GetAddr()->Dump();
        ret = 0;  // make sure this one gets ignored
      }
      else
      {
        LOG( csmnet, debug ) << "Received Heartbeat signal from " << aMsgAddr.GetAddr()->Dump();

        // if we receive a heartbeat message from a known client
        //   - it's either a self-send where aMsgAddr.GetAddr == LocalAddr
        //   - or it's an address collision between arbitrary known clients
        // lets make a fuss about it....!!!
        if(( known ) && ( aMsgAddr.GetAddr()->MakeKey() != GetLocalAddr()->MakeKey() ) && IsServerEndpoint() )
          throw csm::network::ExceptionProtocol("FATAL: heartbeat msg from known client means: address hash collision.");
      }
    }
      // heartbeats are hidden from upper layers, so we return 0 here.
//      return 0;
    return ret;
  }
  catch (csm::network::ExceptionEndpointDown &e )
  {
    RemoveFromClientRegistry( aMsgAddr.GetAddr() );
    throw;
  }
  catch (csm::network::ExceptionRecv &e )
  {
    // permission denied is the only case that is considered non-fatal and keeps the link open
    if( e.GetErrno() != EPERM )
      RemoveFromClientRegistry( aMsgAddr.GetAddr() );
    throw;
  }
  catch (csm::network::Exception &e )
  {
    CheckAndAddRemoteAddress( aMsgAddr.GetAddr(), true );
    throw;
  }
}

ssize_t
csm::network::EndpointMultiUnix::Recv( csm::network::Message &aMsg )
{
  try
  {
    // this can only succeed if this is a client endpoint.
    ssize_t ret = csm::network::EndpointDualUnix::Recv( aMsg );
    return ret;
  }
  catch (csm::network::ExceptionEndpointDown &e )
  {
    RemoveFromClientRegistry( _RemoteAddr );
    throw;
  }
  catch (csm::network::Exception &e )
  {
    CheckAndAddRemoteAddress( _RemoteAddr, true );
    throw;
  }
}

#define CSM_NET_CTRL_APPEND_TAIL( tail, ep ) \
{ \
    if( (tail) != nullptr ) \
    { \
      (tail)->_Next = new csm::network::NetworkCtrlInfo( csm::network::NET_CTL_DISCONNECT, 0, (ep)->GetRemoteAddr(), nullptr ); \
      LOG( csmnet, debug ) << "Appending to tail: " << (tail)->_Type << " new: " << (tail)->_Next->_Type; \
      (tail) = (tail)->_Next; \
    } \
    else \
    { \
      (tail) = new csm::network::NetworkCtrlInfo( csm::network::NET_CTL_DISCONNECT, 0, (ep)->GetRemoteAddr(), nullptr ); \
      LOG( csmnet, debug ) << "Appending a new tail: " << (tail)->_Type; \
      ret = (tail); \
    } \
}

/* data synchronization, e.g. flush any buffers */
csm::network::NetworkCtrlInfo*
csm::network::EndpointMultiUnix::Sync( const SyncAction aSync )
{
  csm::network::NetworkCtrlInfo *ret = EndpointDualUnix::Sync( aSync );

  // only do the heartbeat if it's sync about maintenance
  switch( aSync )
  {
    case csm::network::SYNC_ACTION_MAINTENANCE:
      break;
    case csm::network::SYNC_ACTION_READ:
      return nullptr;
      break;
    case csm::network::SYNC_ACTION_WRITE:
      break;
    case csm::network::SYNC_ACTION_ALL:
      break;
    default:
      throw csm::network::Exception("Unrecognized sync action requested.");
  }

  EndpointHeartbeat::TimeType ref = std::chrono::steady_clock::now();
  if( _Heartbeat.dueSend( ref ) )
  {
    csm::network::NetworkCtrlInfo *tail = ret;
    while( tail && tail->_Next )
      tail = tail->_Next;
    LOG( csmnet, debug ) << "Heartbeat timer triggers ping of known clients.";
    for( auto & it : _Known )
    {
      csm::network::EndpointVirtualUnix *ep = it.second;
      if(( ! ep->IsConnected() ) || ( ep->IsServerEndpoint() ))
        continue;

      // first check if we received the ACKs for the previous heartbeat
      if( ep->GetHeartbeat()->dueSend( ref ) )  // dueSend false mean we haven't received
      {
        LOG( csmnet, warning ) << "Heartbeat timeout: No response from client." << ep->GetRemoteAddr()->Dump();
        CSM_NET_CTRL_APPEND_TAIL ( tail, ep );
        _Removed.push_back( ep );
        ep->Disconnect();
        continue;
      }

      LOG( csmnet, debug ) << "Pinging known client: " << ep->GetRemoteAddr()->Dump();

      try
      {
        int hbrc = ep->Send( _Heartbeat.getMsg() );
        if( hbrc < 0 )
        {
          LOG( csmnet, warning ) << "Heartbeat failed: Send failed." << ep->GetRemoteAddr()->Dump() << " rc=" << hbrc;
          CSM_NET_CTRL_APPEND_TAIL ( tail, ep );
          _Removed.push_back( ep );
          ep->Disconnect();
        }
      }
      catch ( csm::network::Exception &e )
      {
        LOG( csmnet, warning ) << "Heartbeat failed: " << e.what() << ": "<< ep->GetRemoteAddr()->Dump() << " rc=" << e.GetErrno();
        CSM_NET_CTRL_APPEND_TAIL ( tail, ep );
        _Removed.push_back( ep );
        ep->Disconnect();
      }
    }
    while( ! _Removed.empty() )
    {
      csm::network::EndpointVirtualUnix *ep = _Removed.front();
      RemoveFromClientRegistry( ep->GetRemoteAddr() );
      _Removed.pop_front();
    }
    _Heartbeat.updateSendSuccess();
  }
  return ret;
}


void
csm::network::EndpointMultiUnix::RemoveFromClientRegistry( const csm::network::Address_sptr aAddr )
{
  if( aAddr == nullptr )
    return;

  csm::network::AddressCode code = aAddr->MakeKey();
  csm::network::EndpointVirtualMap::const_iterator check = _Known.find( code );

  if( check != _Known.end() )
  {
    LOG(csmnet, info) << "Deleting known client address: " << aAddr->Dump();

    csm::network::AddressUnix_sptr in = std::dynamic_pointer_cast<csm::network::AddressUnix>( aAddr );
    csm::network::AddressUnix_sptr listed = std::dynamic_pointer_cast<csm::network::AddressUnix>( check->second->GetRemoteAddr() );

    if( *in != *listed )
    {
      LOG(csmnet, error ) << " Detected Address-code collision: " << in->Dump()
          << " and " << listed->Dump()
          << " have keys: " << in->MakeKey()
          << " and " << listed->MakeKey();
      throw csm::network::Exception("BUG: Address-code collision or comparison failure...");
    }

    // clean out any entries in the accept-list to prevent late accepts of disconnected clients
    for( csm::network::EndpointVirtualFifo::iterator acc= _ToAccept.begin();
        acc != _ToAccept.end();
        ++acc )
    {
      csm::network::AddressUnix_sptr accept = std::dynamic_pointer_cast<csm::network::AddressUnix>( (*acc)->GetRemoteAddr() );
      if( *accept == *listed )
      {
        LOG(csmnet, warning) << "Found disconnecting endpoint with pending Accept() of client " << aAddr->Dump();
        _ToAccept.erase( acc );
        break;
      }
    }
    if( check->second != nullptr )
      delete check->second;
    _Known.erase( check );
  }
  else
    LOG(csmnet, warning) << "Trying to remove unknown remote client address: " << aAddr->Dump();
}

bool csm::network::EndpointMultiUnix::CheckAndAddRemoteAddress( const csm::network::Address_sptr aAddr,
                                                                const bool UpdateHeartbeat )
{
  if( aAddr == nullptr )
    return false;

  bool found = true;

  csm::network::AddressCode code = aAddr->MakeKey();
  csm::network::EndpointVirtualMap::const_iterator check = _Known.find( code );
  csm::network::EndpointVirtualUnix *ep = nullptr;

  if( check == _Known.end() )
  {
    LOG(csmnet, debug) << "Adding unknown local-type address: " << aAddr->Dump();
    csm::network::AddressUnix_sptr ua = std::dynamic_pointer_cast<csm::network::AddressUnix>( aAddr );

    csm::network::EndpointVirtualUnix *newEP = new csm::network::EndpointVirtualUnix( this, ua );
    if( newEP == nullptr )
      throw csm::network::Exception("BUG: Failed to create new client endpoint for addr: "+ aAddr->Dump() );

    if( ! newEP->IsServerEndpoint() )
      _ToAccept.push_back( newEP );
    _Known.insert( std::pair<csm::network::AddressCode, csm::network::EndpointVirtualUnix*>( code, newEP ) );
    found = false;
    ep = newEP;
  }
  else
  {
    LOG(csmnet, debug) << "Found already known local-type client address: " << aAddr->Dump();
    ep = (*check).second;
  }

  if(( UpdateHeartbeat ) && ( ep != nullptr ))
  {
    ep->GetHeartbeat()->updateRecvSuccess();
    LOG(csmnet, debug) << "Updated Heartbeat timeout for " << aAddr->Dump();
  }
  return found;
}

csm::network::Endpoint*
csm::network::EndpointMultiUnix::GetEndpoint( const csm::network::Address_sptr aAddr ) const
{
  if(( aAddr == nullptr ) || ( aAddr->GetAddrType() != csm::network::AddressType::CSM_NETWORK_TYPE_LOCAL ))
    return nullptr;

  csm::network::AddressCode code = aAddr->MakeKey();
  return GetEndpoint( code );
}

csm::network::Endpoint*
csm::network::EndpointMultiUnix::GetEndpoint( const csm::network::AddressCode aKey ) const
{
  LOG(csmnet, trace) << "Unix::GetEndpoint(): searching for local addr key: " << aKey;

  csm::network::EndpointVirtualMap::const_iterator check = _Known.find( aKey );
  if( check != _Known.end() )
    return _Known.at( aKey );
  return nullptr;
}


bool csm::network::EndpointMultiUnix::CheckRemoteAddress( const csm::network::Address_sptr aAddr ) const
{
  if( aAddr == nullptr )
    return false;

  bool found = true;

  csm::network::AddressCode code = aAddr->MakeKey();
  csm::network::EndpointVirtualMap::const_iterator check = _Known.find( code );

  if( check == _Known.end() )
  {
    LOG(csmnet, debug) << "Unknown (out of " << _Known.size() << ") remote peer address: " << aAddr->Dump();
    found = false;
  }
  else
    LOG(csmnet, debug) << "Found (out of " << _Known.size() << ") remote peer address: " << aAddr->Dump();
  return found;
}

