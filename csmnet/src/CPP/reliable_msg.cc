/*================================================================================

    csmnet/src/CPP/reliable_msg.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <ifaddrs.h>

#include <chrono>
#include <ctime>
#include <queue>

#include "logging.h"
#include "csmutil/include/csm_version.h"
#include "csmi/include/csmi_type_common.h"
#include "csmi/include/csm_api_macros.h"
#include "csmi/src/common/include/csmi_serialization.h"

#include "address.h"
#include "endpoint.h"
#include "multi_endpoint.h"
#include "message_ack.h"
#include "reliable_msg.h"
#include "csm_version_msg.h"

// message based send
ssize_t
csm::network::ReliableMsg::SendTo( const csm::network::MessageAndAddress &aMsgAddr )
{
  ssize_t ret = csm::network::MultiEndpoint::SendTo( aMsgAddr );
  if( ret <= 0 )
    return ret;
  if( ! _AckMgr.RegisterAck( aMsgAddr ) )
  {
    AddCtrlEvent( csm::network::NET_CTL_TIMEOUT, aMsgAddr._Msg.GetMessageID() );
    return -ENOBUFS;
  }
  return ret;
}

// message based receive
ssize_t
csm::network::ReliableMsg::RecvFrom( csm::network::MessageAndAddress &aMsgAddr )
{
  ssize_t ret = 0;
  bool BufferedAckedAlready = false;
  int retry_after_ack = 100;
retry:

  // check whether there's a buffered message from a sync-triggered recv call
  if( _BufferedAvailable )
  {
    LOG( csmnet, debug ) << "Buffered Msg available. len=" << _BufferedMsg._Msg.GetDataLen();
    aMsgAddr = _BufferedMsg;
    if( ! aMsgAddr._Msg.Validate() )
    {
      LOG( csmnet, error ) <<  "BUG: Invalid Msg/Header. detected in ReliableMsg::RecvFrom()";
      AddCtrlEvent( csm::network::NET_CTL_FATAL, aMsgAddr.GetAddr() );
    }
    ret = aMsgAddr._Msg.GetDataLen() + sizeof( csm_network_header_t );
    _BufferedAvailable = false;
    BufferedAckedAlready = true;
  }
  else
  {
    try {
      ret = csm::network::MultiEndpoint::RecvFrom( aMsgAddr );
      if(( ret > 0 ) && ( aMsgAddr.GetAddr() == nullptr ))
        throw csm::network::Exception("BUG: Successful RecvFrom() is supposed to return the source address.");
    }
    catch ( csm::network::ExceptionEndpointDown &e )
    {
      // already handled by MultiEndpoint::RecvFrom
    }
    catch( csm::network::ExceptionRecv &e )
    {
      // only permission errors will make it here
      if( e.GetErrno() != EPERM )
      {
        LOG( csmnet, error ) <<  "BUG: ExceptionRecv with error != EPERM detected in ReliableMsg::RecvFrom()";
        AddCtrlEvent( csm::network::NET_CTL_FATAL, aMsgAddr.GetAddr() );
      }
      if( RespondWithImmediateError( e, aMsgAddr ) < 0 )
      {
        DeleteEndpoint( aMsgAddr.GetAddr().get(), "ReliableMsg:RespondWithImmediateError" );
        AddCtrlEvent( csm::network::NET_CTL_DISCONNECT, aMsgAddr.GetAddr() );
        return 0;
      }
      ret = 0;
    }
  }

  if( ret <= 0 )
    return ret;

  // normally the ERR flag will be cleared for the ACK
  bool KeepErrorFlag = false;

  // DANGER: when set to true, make sure there's no ctrl event created!!!!
  bool disconnect = false;
  csm::network::NetworkCtrlEventType event_type = csm::network::NET_CTL_OTHER;
  if( aMsgAddr._Msg.GetInt() )
  {
    switch( aMsgAddr._Msg.GetCommandType() )
    {
      case CSM_CMD_STATUS:
      {
        // compare incoming message with our version
        ret = 0;

        csm::network::VersionStruct versionMsg;
        if(( aMsgAddr.GetAddr() != nullptr ) &&  ( aMsgAddr._Msg.GetDataLen() > 0 ))
        {
          if( aMsgAddr.GetAddr()->GetAddrType() == CSM_NETWORK_TYPE_LOCAL )
          {
            versionMsg._Version = aMsgAddr._Msg.GetData();
            versionMsg._Hostname = "localhost";
            versionMsg._Sequence = 0;
          }
          else
            try
            {
              csm::network::VersionMsg::ConvertToClass( aMsgAddr._Msg.GetData(), versionMsg );
            }
            catch( ... )
            {
              versionMsg._Version = aMsgAddr._Msg.GetData();
              versionMsg._Hostname = "notavail";
              versionMsg._Sequence = 0;
            }
        }

        if( !aMsgAddr._Msg.GetAck() )
          LOG( csmnet, info ) << "ReliableMsg: Connection verified: " << versionMsg._Version.substr(0,7)
            << "; host: " << versionMsg._Hostname
            << "; seq#: " << versionMsg._Sequence;

        // if not ACK, this is the initial version message
        if(!(aMsgAddr._Msg.GetAck()) && (0 != versionMsg._Version.compare(CSM_VERSION)) )
        {
          LOG(csmnet,warning) << "Version mismatch. LOCAL: " << CSM_VERSION << "; REMOTE: " << aMsgAddr._Msg.GetData();
          aMsgAddr._Msg.SetErr();
          aMsgAddr._Msg.SetCommandType( aMsgAddr._Msg.GetReservedID() ); // restore the command type to match what the sender version had
          aMsgAddr._Msg.SetReservedID( 0 );
          KeepErrorFlag = true;
          aMsgAddr._Msg.SetData( std::string("VERSION MISMATCH. Required: ") + std::string(CSM_VERSION, 0, 7));
          event_type = csm::network::NET_CTL_DISCONNECT;
          disconnect = true;
        }

        // check for NACK and log results
        else if( aMsgAddr._Msg.GetAck() && aMsgAddr._Msg.GetErr() )
        {
          LOG(csmnet,critical) << "Version mismatch (NACK). LOCAL:  " << CSM_VERSION << "; REMOTE: " << aMsgAddr._Msg.GetData();
          event_type = csm::network::NET_CTL_FATAL;
          disconnect = true;
        }
        else
        {
          // if things are great, set the verification status
          // and create the CONNECT event for upper layers

          csm::network::Endpoint *ep = csm::network::MultiEndpoint::GetEndpoint( aMsgAddr.GetAddr() );
          if( ep != nullptr )
          {
            // active side is already verified, however, if we attempt to verify the passive side twice, we have a protocol problem
            // (the active side will receive an ACK
            if(( !aMsgAddr._Msg.GetAck()) && ( ep->IsVerified() ))
            {
              LOG( csmnet, error ) << "Network protocol problem: Verifying a connection that's already verified.";
              event_type = csm::network::NET_CTL_FATAL;
              disconnect = true;
            }
            else
            {
              ep->SetVerified();
              AddCtrlEvent( csm::network::NET_CTL_CONNECT, aMsgAddr.GetAddr(), &versionMsg );
            }
          }
          else
          {
            LOG( csmnet, error ) << "Received connection verification from untracked endpoint: " << aMsgAddr.GetAddr()->Dump();
            event_type = csm::network::NET_CTL_DISCONNECT;
            disconnect = true;
          }
        }
        break;
      }
      case CSM_CMD_CONNECTION_CTRL:
      {
        if( aMsgAddr._Msg.GetAck() )
          break;

        if( 0 == aMsgAddr._Msg.GetData().compare(CSM_FAILOVER_MSG) )
        {
          LOG( csmnet, info ) << "Received ControlMsg from " << aMsgAddr.GetAddr()->Dump() << " to FAILOVER";
          AddCtrlEvent( csm::network::NET_CTL_FAILOVER, aMsgAddr.GetAddr() );
        }
        if( 0 == aMsgAddr._Msg.GetData().compare(CSM_RESTART_MSG) )
        {
          LOG( csmnet, debug ) << "Received ControlMsg from " << aMsgAddr.GetAddr()->Dump() << " to RESTART";
          AddCtrlEvent( csm::network::NET_CTL_RESTARTED, aMsgAddr.GetAddr() );
        }
        ret = 0; // don't expose this message, the network event is enough
        break;
      }
      case CSM_CMD_HEARTBEAT:
        event_type = csm::network::NET_CTL_DISCONNECT;
        disconnect = true;
        break;
      default:
        break;
    }
  }

  // check whether we received an ACK or a regular msg
  if( aMsgAddr._Msg.GetAck() )
  {
    if( aMsgAddr._Msg.GetErr() )
    {
      std::string errorMsg = (aMsgAddr._Msg.GetDataLen() > 0) ? aMsgAddr._Msg.GetData() : "Unspecified reason.";
      LOG( csmnet, critical ) << "Received NACK: " << errorMsg;
      event_type = csm::network::NET_CTL_FATAL;
      disconnect = true;
    }
    else if( _AckMgr.AckReceived( aMsgAddr._Msg ) )
    {
      ret = 0;
      --retry_after_ack;
      if( retry_after_ack > 0 )
        goto retry;
    }
    else
    {
      LOG( csmnet, debug ) << "Received ACK that's timed out or unknown. msgId|resp: " << aMsgAddr._Msg.GetMessageID()
         << "|" << aMsgAddr._Msg.GetResp();
      ret = 0;
    }
  }
  else
  {
    if( ! BufferedAckedAlready )
    {
      try
      {
        GenerateAndSendACK( aMsgAddr, KeepErrorFlag );
      }
      catch( csm::network::Exception &e )
      {
        LOG( csmnet, debug ) << "ReliableMsg: Failed to send ACK: " << e.what();
        event_type = csm::network::NET_CTL_DISCONNECT;
        disconnect = true;
        ret = 0;
      }
    }
  }
  if( disconnect )
  {
    LOG(csmnet,debug) << "ReliableMsg: event " << event_type << " Shutting down connection to " << aMsgAddr.GetAddr()->Dump();
    DeleteEndpoint( aMsgAddr.GetAddr().get(), "ReliableMsg:ConnectionErrors" );
    AddCtrlEvent( event_type, aMsgAddr.GetAddr() );
  }

  return ret;
}

/* data synchronization, e.g. check for ACKs, flush any buffers, send/recv pending requests, ... */
int csm::network::ReliableMsg::Sync( const csm::network::SyncAction aSync )
{
  int TimeoutDetected = 0;

  // check for timed-out ACKs
  _AckMgr.UpdateClock();
  for( std::pair<AckKeyType, csm::network::Address_sptr> msg_itr = _AckMgr.CheckTimeout();
      (msg_itr.first != 0);
      msg_itr = _AckMgr.CheckTimeout() )
  {
    // skip TIMEOUT events for any local client that's already disconnected
    if(( msg_itr.second->GetAddrType() == csm::network::CSM_NETWORK_TYPE_LOCAL ) &&
        ( !_Unix->CheckRemoteAddress( msg_itr.second )) )
    {
      LOG(csmnet, debug ) << "Found timeout on ACK to local client that no longer exists";
      continue;
    }
    LOG( csmnet, debug ) << "Adding entry to timed-out ACKs msgId|resp: " << msg_itr.first;
    AddCtrlEvent( csm::network::NET_CTL_TIMEOUT, msg_itr.first.GetMsgID() );
    TimeoutDetected += 1;
  }
  csm::network::MultiEndpoint::Sync( aSync );

  // return the total of pending ACKs and Ctrl Events
  return _AckMgr.GetAckCount() + CtrlEventCount();
}

int
csm::network::ReliableMsg::RespondWithImmediateError( const csm::network::Exception &e, csm::network::MessageAndAddress &aMsgAddr )
{
  // create error message and return to sender...
  uint32_t bufLen;
  char *ebuf;

  if( e.GetErrno() == EPERM )
    ebuf = csmi_err_pack( e.GetErrno(), csm_get_string_from_enum(csmi_cmd_err_t, CSMERR_PERM), &bufLen );
  else
    ebuf = csmi_err_pack( e.GetErrno(), e.what(), &bufLen);

  GenerateAndSendACK( aMsgAddr, true );

  aMsgAddr._Msg.SetFlags(0);
  aMsgAddr._Msg.SetResp();
  aMsgAddr._Msg.CreateError( false, CSM_PRIORITY_DEFAULT, std::string( ebuf, bufLen ) );
  try {
    if( aMsgAddr.GetAddr() == nullptr )
    {
      LOG( csmnet, error ) << "Send to empty address.";
      free( ebuf );
      return -EDESTADDRREQ;
    }
    SendTo( aMsgAddr );
  }
  catch ( csm::network::Exception &se )
  {
    LOG( csmnet, warning ) << "Send error during error response. Can be ignored." << se.what();
    free( ebuf );
    return -se.GetErrno();
  }
  free( ebuf );
  return 0;
}

void
csm::network::ReliableMsg::GenerateAndSendACK( const csm::network::MessageAndAddress &aMsgAddr, const bool KeepErrorFlag )
{
  csm::network::Address_sptr dstAddr = aMsgAddr.GetAddr();

  csm::network::MessageAndAddress *AckMsg = _AckMgr.CheckCreateACKMsg( aMsgAddr, dstAddr, KeepErrorFlag );
  if( AckMsg != nullptr )
  {
    LOG(csmnet, debug) << "Sending ACK for msgID|resp: " << aMsgAddr._Msg.GetMessageID() << "|" << aMsgAddr._Msg.GetResp()
        << " cmd: " << csm::network::cmd_to_string( aMsgAddr._Msg.GetCommandType() );
    csm::network::MultiEndpoint::SendTo( *AckMsg );
    delete AckMsg;
  }
}
