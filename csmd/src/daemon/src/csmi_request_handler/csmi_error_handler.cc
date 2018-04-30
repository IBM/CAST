/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_error_handler.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
// implement the CSM api node attributes command...
//

#include "csmi_error_handler.h"

#include "logging.h"

#include "csmi/src/common/include/csmi_serialization.h"


void CSMI_ERROR_HANDLER::Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList )
{
    if( !isNetworkEvent(aEvent))
    {
      LOG(csmd,warning) << "CSMI_ERROR_HANDLER: Do nothing if the incoming event is not a Network Event Type";
      return;
    }
    LOG(csmd,debug) << "CSMI_ERROR_HANDLER: Processing...";
    
    csm::network::MessageAndAddress nwd = dynamic_cast<const csm::daemon::NetworkEvent *>( &aEvent )->GetContent();
    if( nwd._Msg.GetAck() )
    {
      LOG( csmd, warning ) << "CSMI_ERROR_HANDLER: Got an error event with ACK set: likely a send error without context. Ignoring...";
      return;
    }

    // get the Message and address
    csm::network::Message msg = nwd._Msg;
    csm::network::Address_sptr addr = nwd.GetAddr();

    if( addr == nullptr )
    {
      LOG( csmd, warning ) << "Processing error without peer address. Ignoring.";
      return;
    }
    
    // if the command is CSM_CMD_ERROR, this event is generated internally due to some error.
    // for now, just dump the errcode + errmsg and return. Do not attempt to send back for now
    // as it may cause endless-loop.
    if (msg.GetCommandType() == CSM_CMD_ERROR)
    {
      if (msg.GetDataLen() > 0)
      {
        csmi_err_t *err = csmi_err_unpack(msg.GetData().c_str(), msg.GetDataLen());
        if (err)
        {
          LOG(csmd, warning) << "CSMI_ERROR_HANDLER: Got a CSM_CMD_ERROR message with errcode=" << err->errcode
            << " errmsg=" << err->errmsg;
          csmi_err_free(err);
          return;
        }
      }

      LOG(csmd, error) << "CSMI_ERROR_HANDLER: Got a CSM_CMD_ERROR Message";
      return;
      
      /*
      outMsg = msg;
      outMsg.SetErr();
      outMsg.SetResp();
      csm::daemon::EventContext_sptr context = aEvent.GetEventContext();
      if ( context != nullptr && (context->GetReqEvent() != nullptr) )
      {
        if (isNetworkEvent( *(context->GetReqEvent()) ) )
        {
          csm::daemon::NetworkEvent *reqEvent = (csm::daemon::NetworkEvent *) context->GetReqEvent();
          addr = reqEvent->GetContent().GetAddr();
        }
      }
      */
    }
    
    // come here when the csmi cmd is not CSM_CMD_ERROR. the event has the message header from the client.
    // will try to send back a error response
    csm::network::Message outMsg;
    uint32_t bufLen;
    char *buf = csmi_err_pack(_errcode, _errmsg.c_str(), &bufLen);
    if( ! buf )
      throw csm::daemon::Exception("CSMI_CMD_ERROR handler failed to allocate memory for error-string.");

    uint8_t flags = CSM_HEADER_RESP_BIT | CSM_HEADER_ERR_BIT;
    CreateNetworkMessage(msg, buf, bufLen, flags, outMsg);
    free( buf );

    // figure out the address and then compose a MessageAndAddress content
    csm::network::MessageAndAddress outContent;
    
    // \todo: Not clear if the address is generated correctly here in various cases
    // NOTE: As the utility and aggregator have a default event handler, the undefined
    //       csmi cmd case won't come here except at Master.
    bool isError = false;
    if (addr == nullptr)
    {
      LOG(csmd, error) << "CSMI_ERROR_HANDLER:: address is nullptr";
      isError = true;
    }
    else
      switch( addr->GetAddrType() )
      {
        case csm::network::CSM_NETWORK_TYPE_UTILITY:
        case csm::network::CSM_NETWORK_TYPE_PTP:
        case csm::network::CSM_NETWORK_TYPE_LOCAL:
          outContent =  csm::network::MessageAndAddress ( outMsg, addr);
          break;

        case csm::network::CSM_NETWORK_TYPE_AGGREGATOR:
          if( GetRole() != CSM_DAEMON_ROLE_MASTER )
          {
            outContent = csm::network::MessageAndAddress( outMsg, _AbstractMaster );
            break;
          }
          // no break here to handle else-path as default:
        default:
        {
          csm::network::Address_sptr rspAddress = CreateReplyAddress( addr.get() );
          if (rspAddress) {
            outContent = csm::network::MessageAndAddress( outMsg, rspAddress );
          } else isError = true;
        }
      }
    if (isError)
    {
      // addr may be nullptr due to the down connection
      LOG(csmd, error) << "CSMI_ERROR_HANDLER:: Fail to forward an error event";
    }
    else
    {
      postEventList.push_back( new csm::daemon::NetworkEvent(outContent, csm::daemon::EVENT_TYPE_NETWORK, nullptr) );
    }

}
