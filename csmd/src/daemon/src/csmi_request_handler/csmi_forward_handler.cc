/*================================================================================
   
    csmd/src/daemon/src/csmi_request_handler/csmi_forward_handler.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
// implement the CSM api node attributes command...
//
//

#include "csmi_forward_handler.h"
#include "include/csm_event_type_definitions.h"

#include "logging.h"

void CSMI_FORWARD_HANDLER::Process( const csm::daemon::CoreEvent &aEvent, std::vector<csm::daemon::CoreEvent*>& postEventList )
{

  enum stage
  {
    receive_client_request = 0,
    forward_response,
    done
  };

  csm::daemon::EventContext_sptr ctx = aEvent.GetEventContext();
  // the context will have AuxiliaryId set to 0
  if (ctx == nullptr) ctx = CreateContext(aEvent, this, 0);

  csm::daemon::NetworkEvent *ev = nullptr;

  switch (ctx->GetAuxiliaryId())
  {
    // good case: any new request from client, master, compute
    // bad case: anything late that had their context removed/cleaned up
    //           distinguishing feature: the response bit (if set, then it's a late msg with deleted context
    //                                   late requests could still be forwarded
    case receive_client_request:
    {
      if ( !isNetworkEvent(aEvent) )
      {
        LOG(csmd, warning) << "CSMI_FORWARD_HANDLER: Expecting a NetworkEvent in receive_client_request state. Got: " << aEvent.GetEventType()
            << ". Ignoring.";
        break;
      }

      csm::network::Address_sptr addr = GetNetworkAddress( aEvent );
      if( addr == nullptr )
      {
        LOG( csmd, warning ) << "CSMI_FORWARD_HANDLER: Received msg from nullptr address in receive_client_request state. Ignoring.";
        break;
      }

      // inspect the Message in response or error event
      csm::network::Message msg = GetNetworkMessage(aEvent);
      if( msg.GetResp() )
      {
        LOG(csmd, warning) << "CSMI_FORWARD_HANDLER: receive_client_request triggered by a (late?) response msg. Ignoring.";
        break;
      }

      // only return and not forward msgs with error flag if they come from a client.
      // otherwise it could be an API/handler trying to forward an error signal towards the master
      if(( msg.GetErr() ) && ( addr->GetAddrType() == csm::network::AddressType::CSM_NETWORK_TYPE_LOCAL ))
      {
        LOG(csmd, warning) << "CSMI_FORWARD_HANDLER: receive_client_request triggered by ErrorMsg from client " << addr->Dump() << ". cmd:"
            << csmi_cmds_to_str( msg.GetCommandType() );
        unsigned buflen = 0;
        char *buf = csmi_err_pack(CSMERR_SENDRCV_ERROR, "Invalid message received", &buflen);

        // now get the message in the original request
        msg = GetNetworkMessage(*(ctx->GetReqEvent()));
        msg.SetErr();
        msg.SetResp();
        msg.SetData(std::string(buf, buflen));
        if (buf) free(buf);
        msg.CheckSumUpdate();

        ctx->SetAuxiliaryId(done);
        ev = CreateNetworkEvent(msg, GetNetworkAddress(*(ctx->GetReqEvent())));
        
      }
      else
      {
        LOG(csmd, debug) << "CSMI_FORWARD_HANDLER: Forwarding a new request towards Master...";
        ctx->SetAuxiliaryId(forward_response);
        ev = CreateNetworkEvent( GetNetworkMessage(aEvent), _AbstractMaster, ctx);
      }

      break;
    }
    // good case: a response coming back from master(u,a), compute(a), or aggr(c)
    // bad case: a late response or an erroneous msg from a client
    case forward_response:
    {
      if ( !isNetworkEvent(aEvent) )
      {
        LOG(csmd, error) << "CSMI_FORWARD_HANDLER: Expecting a NetworkEvent in forward_response state. Got: " << aEvent.GetEventType();
        break;
      }

      csm::network::Address_sptr addr = GetNetworkAddress( aEvent );
      if(( addr == nullptr ) || ( addr->GetAddrType() == csm::network::AddressType::CSM_NETWORK_TYPE_LOCAL ))
      {
        LOG( csmd, warning ) << "CSMI_FORWARD_HANDLER: Received msg from nullptr or client address in forward_response state.";
        break;
      }

      // extract destination address from orig event
      csm::network::Address_sptr dst_addr = GetNetworkAddress(*(ctx->GetReqEvent()));
      // inspect the Message in response or error event
      csm::network::Message msg = GetNetworkMessage(aEvent);
      std::string payload=msg.GetData();
      if ( msg.GetErr() || (! msg.Validate()) )
      {
        LOG(csmd, info) << "CSMI_FORWARD_HANDLER: Forwarding ErrorMsg to client "
            << ( dst_addr != nullptr ? dst_addr->Dump() : "N/A" )
            << " for cmd: " << csmi_cmds_to_str( msg.GetCommandType() );
        csmi_err_t *err = csmi_err_unpack(msg.GetData().c_str(), msg.GetDataLen());
        if (err) 
        {
          // convert ECOMM|ETIMEOUT|EPERM to the csmi defined err code.
          // if not defined in the _errnoTocmderr, we will keep whatever it is in the original payload
          csmi_cmd_err_t csmi_err = GetCmdErr(err->errcode);
          if (csmi_err != CSMERR_NOTDEF)
          {
            // now we can override the payload with appropriate csmi error code
            uint32_t bufLen;
            char *buf = csmi_err_pack(csmi_err, err->errmsg, &bufLen);
            if (buf)
            {
              payload = std::string(buf, bufLen);
              free(buf);
            }
          }
          csmi_err_free(err);
        }
        else 
        {
          LOG(csmd, error) << "CSMI_FORWARD_HANDLER: Fail to unpack the payload with error flag set to 1";
        }

        if( dst_addr == nullptr )
        {
          LOG( csmd, warning ) << "CSMI_FORWARD_HANDLER: nullptr destination on original request. Cannot forward to caller. cmd: "
              << csmi_cmds_to_str( msg.GetCommandType() );
          break;
        }

        // now get the message in the original request
        msg = GetNetworkMessage(*(ctx->GetReqEvent()));
        msg.SetErr();
        msg.SetResp();
        msg.SetData(payload);
        msg.CheckSumUpdate();
        ev = CreateNetworkEvent(msg, dst_addr);
      }
      else
      {
        LOG(csmd, debug) << "CSMI_FORWARD_HANDLER: Forwarding a reply back towards orig. sender...";
        ev = ForwardReplyNetworkEvent(aEvent, ctx);
      }
      
      ctx->SetAuxiliaryId(done);
      break;
    }
    default:
      LOG(csmd, error) << "CSMI_FORWARD_HANDLER: Unexpected state.";
      break;
  }

  if (ev) postEventList.push_back(ev);
  else LOG(csmd, warning) << "CSMI_FORWARD_HANDLER: No message forwarded due to previous errors/problems.";

}
