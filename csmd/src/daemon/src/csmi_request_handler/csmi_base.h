/*================================================================================
   
    csmd/src/daemon/src/csmi_request_handler/csmi_base.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

#ifndef _CSM_BASE
#define _CSM_BASE

#include <stdint.h>
#include <string>
#include <iostream>
#include <assert.h>
#include <cstdio>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include "logging.h"
#include "csmutil/include/csm_version.h"
#include "csmi/src/common/include/csmi_serialization.h"
#include "csmi/src/common/include/csmi_common_utils.h"
#include "csmi/include/csmi_type_common.h"




#include "csmd/src/db/include/DBConnectionPool.h"
#include "csmd/src/db/include/csm_db_event_content.h"

#include "include/csm_core_event.h"
#include "include/csm_timer_event.h"
#include "include/csm_daemon_state.h"
#include "include/csm_daemon_network_manager.h"
#include "include/csm_system_event.h"
#include "include/csm_bitmap.h"

#include "csmnet/src/CPP/csm_network_msg_cpp.h"
#include "csmnet/src/CPP/address.h"
#include "csmnet/src/CPP/csm_message_and_address.h"
#include "csmnet/src/CPP/csm_multicast_message.h"
#include "csmi/include/csmi_type_ras_funct.h"
#include "csmi/include/csm_api_macros.h"

#include "csm_handler_options.h"

class CSMI_BASE {

public:

  //every derived class has to implment this Process method. Will define Process as a pure virtual function later.
  virtual void Process( const csm::daemon::CoreEvent &aEvent, std::vector<csm::daemon::CoreEvent*>& postEventList ) = 0;

    inline csmi_cmd_err_t GetCmdErr(int aErrorno)
    {
        csmi_cmd_err_t newErrorNo = (csmi_cmd_err_t)aErrorno;
        switch(aErrorno)
        {
            case ECOMM:
                newErrorNo = CSMERR_SENDRCV_ERROR;
                break;
            case ETIMEDOUT:
                newErrorNo = CSMERR_TIMEOUT;
                break;
            case EPERM:
                newErrorNo = CSMERR_PERM;
                break;
            default:
            ;
        }
        
        return newErrorNo;
    }

protected:
  CSMI_BASE(csmi_cmd_t cmd, csm::daemon::HandlerOptions& options)
  :_cmdType(cmd), _handlerOptions(options), _newRequestCount(0)
  {    
    if (cmd >= CSM_CMD_MAX) {
      // bring down the daemon in this fatal error
      LOG(csmapi, error) << "CSMI_BASE(): not a valid csmi cmd";
      exit(-1);
    }
    const char *name;
    if ((name = csmi_cmdname_get(cmd)) != NULL)
        _cmdName = std::string(name);
    else
      _cmdName = std::string("cmd_unspecified");
    if ((name = csmi_classname_get(cmd)) != NULL)
      _className = std::string(name);
    else
      _className = std::string("class_unspecified");
    if ((name = csmi_dbtabname_get(cmd)) != NULL)
      _tabName = std::string(name);
    else
      _tabName = std::string("tab_unspecified");

    _packFunc = csmi_pack_get(cmd);
    _unpackFunc = csmi_unpack_get(cmd);
    _argPackFunc = csmi_argpack_get(cmd);
    _argUnpackFunc = csmi_argunpack_get(cmd);
    _AuxId = 1;

    _SecurityLevel = options.GetSecurityLevel(cmd);
    LOG( csmd, debug ) << csmi_cmdname_get(cmd) << ": Security Level is: " << _SecurityLevel;

    _AbstractAggregator = std::make_shared<csm::network::AddressAbstract>(csm::network::ABSTRACT_ADDRESS_AGGREGATOR);
    _AbstractMaster =  std::make_shared<csm::network::AddressAbstract>(csm::network::ABSTRACT_ADDRESS_MASTER);
    _AbstractBroadcast =  std::make_shared<csm::network::AddressAbstract>(csm::network::ABSTRACT_ADDRESS_BROADCAST);
    _AbstractSelf = std::make_shared<csm::network::AddressAbstract>(csm::network::ABSTRACT_ADDRESS_SELF);

  }
  
  /***********************************************************
   ***    General functions                                ***
   **********************************************************/

  
  /**
    \brief Check if the daemon has access to database
  */
  inline bool HasValidDBConn()
  {
    return csm::daemon::HandlerOptions::HasValidDBConn();;
  }
  
  inline csm::db::DBConnection_sptr AcquireDBConnection()
  {
    return csm::daemon::HandlerOptions::AcquireDBConnection();
  }
  
  void ReleaseDBConnection(csm::db::DBConnection_sptr aConn)
  {
    csm::daemon::HandlerOptions::ReleaseDBConnection(aConn);
  }

  inline unsigned GetNumOfFreeDBConnections() { return csm::daemon::HandlerOptions::GetNumOfFreeDBConnections(); }
  inline unsigned GetNumOfLockedDBConnections() const { return csm::daemon::HandlerOptions::GetNumOfLockedDBConnections(); }
   
  inline csm::daemon::API_SEC_LEVEL GetSecurityLevel(csmi_cmd_t i_cmd)
  {
    return csm::daemon::HandlerOptions::GetSecurityLevel(i_cmd);
  }
  
  inline bool HasPrivilegedAccess(uid_t i_uid, gid_t i_gid) const
  {
    return csm::daemon::HandlerOptions::HasPrivilegedAccess(i_uid, i_gid);
  }
   
  inline csm::daemon::DaemonState *GetDaemonState() const
  {
    return csm::daemon::HandlerOptions::GetDaemonState();
  }

  inline CSMDaemonRole GetRole() const { return csm::daemon::HandlerOptions::GetRole(); }
  
  /**
    \brief Make a copy of a CoreEvent
  */
  csm::daemon::CoreEvent* CopyEvent(const csm::daemon::CoreEvent &aEvent)
  {
    csm::daemon::CoreEvent *ret = nullptr;
    if (isNetworkEvent(aEvent))
      ret = new csm::daemon::NetworkEvent( ((csm::daemon::NetworkEvent *) &aEvent)->GetContent(),
                                            aEvent.GetEventType(), aEvent.GetEventContext() );
    else if (isDBReqEvent(aEvent))
      ret = new csm::daemon::DBReqEvent( ((csm::daemon::DBReqEvent *) &aEvent)->GetContent(),
                                            aEvent.GetEventType(), aEvent.GetEventContext() );
    else if (isDBRespEvent(aEvent))
      ret = new csm::daemon::DBRespEvent( ((csm::daemon::DBRespEvent *) &aEvent)->GetContent(),
                                            aEvent.GetEventType(), aEvent.GetEventContext() );
    else if (isTimerEvent(aEvent))
      ret = new csm::daemon::TimerEvent( ((csm::daemon::TimerEvent *) &aEvent)->GetContent(),
                                            aEvent.GetEventType(), aEvent.GetEventContext() );
    else if (isSystemEvent(aEvent))
      ret = new csm::daemon::SystemEvent( ((csm::daemon::SystemEvent *) &aEvent)->GetContent(),
                                            aEvent.GetEventType(), aEvent.GetEventContext() );
    return ret;
    
  }

  /** 
    \brief Automatically generate a AuxiliaryId for EventContext
  */
#ifdef __UINT64_MAX__
  inline uint64_t CreateCtxAuxId() { return ((++_AuxId) % __UINT64_MAX__); }
#else
  inline uint64_t CreateCtxAuxId() { return ((++_AuxId) % UINTMAX_MAX); }
#endif
  /**
    \brief Create a new context containing the CoreEvent and the event handler object
    
    \param[in] aEvent the CoreEvent stored in the EventContext
    \param[in] aClass the Event Handler Object
    \param[in] aAuxiliaryId The AuxiliaryId set in the context
    
    \return Return the created new context
  */
  inline csm::daemon::EventContext_sptr CreateContext(const csm::daemon::CoreEvent &aEvent, void *aClass, uint64_t aAuxiliaryId)
  {
    csm::daemon::EventContext_sptr context( new csm::daemon::EventContext( aClass, aAuxiliaryId, CopyEvent(aEvent) ) );
    return context;
  }
  

  /**
    \brief Create a new context containing the event handler object
   
    \param[in] aClass the Event Handler Object
    \param[in] aAuxiliaryId The AuxiliaryId set in the context
    
    \return Return the created new context
  */
  inline csm::daemon::EventContext_sptr CreateContext(void *aClass, uint64_t aAuxiliaryId)
  {
    csm::daemon::EventContext_sptr context( new csm::daemon::EventContext( aClass, aAuxiliaryId ) );
    return context;
  }

  /*
    \brief Create a returned Address based on the input Address
    \Note If the input Address is a MQTopic address, need to create the topic based on the input.
    If the input is a local unix or ptp address, simply make a copy.
    
    \return Return non-null ptr if the return address is created successfully. Return nullptr if
    failing to do so.
  */
  csm::network::Address_sptr CreateReplyAddress(const csm::network::Address* reqAddress)
  {
    csm::network::Address_sptr rspAddress;
    switch( reqAddress->GetAddrType() )
    {
      case csm::network::CSM_NETWORK_TYPE_AGGREGATOR:
        rspAddress = std::make_shared<csm::network::AddressAggregator>( *dynamic_cast<const csm::network::AddressAggregator*>( reqAddress ));
        break;

      case csm::network::CSM_NETWORK_TYPE_UTILITY:
        rspAddress = std::make_shared<csm::network::AddressUtility>( *dynamic_cast<const csm::network::AddressUtility*>( reqAddress ));
        break;

      case csm::network::CSM_NETWORK_TYPE_LOCAL:
        rspAddress = std::make_shared<csm::network::AddressUnix>( *dynamic_cast<const csm::network::AddressUnix*>( reqAddress ));
        break;

      case csm::network::CSM_NETWORK_TYPE_PTP:
        rspAddress = std::make_shared<csm::network::AddressPTP>( *dynamic_cast<const csm::network::AddressPTP*>( reqAddress ));
        break;

      case csm::network::CSM_NETWORK_TYPE_ABSTRACT:
        rspAddress = std::make_shared<csm::network::AddressAbstract>( *dynamic_cast<const csm::network::AddressAbstract*>( reqAddress ));
        break;

      default:
        LOG(csmapi, warning) << "CSMI_BASE::CreateReplyAddress(): Unexpected Address Type: " << reqAddress->GetAddrType();
        return nullptr;
        break;
    }

   return rspAddress;
  }
  
  /***********************************************************
   *** Helper functions for NetworkEvent  (For Master only)***
   **********************************************************/
  /**
    \brief Create a reply NetworkEvent at Master
    \note reqEvent has to be a NetworkEvent
    
    \param[in] payload The byte array in the Message payload
    \param[in] payloadLen The length of the byte array
    \param[in] reqEvent The request NetworkEvent. Will use its Message header to construct a reply
    \param[in] aContext The context will be associated with the reply
    \param[in] isError if true, the error flag in the reply will be set.
    
    \return Return the Reply NetworkEvent at Master. Nullptr if an error occurs
  */
  csm::daemon::NetworkEvent* CreateReplyNetworkEvent(const char *payload, const uint32_t payloadLen,
                    const csm::daemon::CoreEvent& reqEvent, const csm::daemon::EventContext_sptr aContext,
                    bool isError = false)
  {
    csm::daemon::NetworkEvent *ev = (csm::daemon::NetworkEvent *)&reqEvent;
    csm::network::MessageAndAddress reqContent = ev->GetContent();
 
    uint8_t flags = CSM_HEADER_RESP_BIT;
    if (isError) flags |= CSM_HEADER_ERR_BIT;
      
    csm::daemon::NetworkEvent *netEvent = nullptr;
    csm::network::Address_sptr rspAddress = CreateReplyAddress(reqContent.GetAddr().get());
    if (rspAddress) {
      netEvent = CreateNetworkEvent(payload, payloadLen, flags, reqContent._Msg, rspAddress, aContext, isError);
    }
    return netEvent;
    
  }

  /**
    \brief Create an error reply NetworkEvent at Master
    \note aEvent must be a NetworkEvent
    
    \param[in] errcode The errcode needs to be added in the Message payload
    \param[in] errmsg The errmsg needs to be added in the Message payload
    \param[in] aEvent The request NetworkEvent. Will use its header and address to construct a valid reply.
    
    \return Return the Error NetworkEvent if success. Nullptr if an error occurs
  */
  csm::daemon::NetworkEvent* CreateErrorEvent(const int errcode, const std::string& errmsg,
                    csm::network::MessageAndAddress &reqContent)
  {
    
    // create a NetworkEvent with error info
    char *buf=nullptr;
    uint32_t bufLen = 0;
    buf = csmi_err_pack(errcode, errmsg.c_str(), &bufLen);
    uint8_t flags = CSM_HEADER_RESP_BIT | CSM_HEADER_ERR_BIT;

    csm::network::Message rspMsg;
    bool ret = CreateNetworkMessage(reqContent._Msg, buf, bufLen, flags, rspMsg);
    if (buf) free(buf);
    
    csm::daemon::NetworkEvent *netEvent = nullptr;
    csm::network::Address_sptr rspAddress = CreateReplyAddress(reqContent.GetAddr().get());
    if ( ret && rspAddress) {
      csm::network::MessageAndAddress errContent ( rspMsg, rspAddress );
      
      netEvent = new csm::daemon::NetworkEvent(errContent,
                            csm::daemon::EVENT_TYPE_NETWORK, nullptr);
    }
    return netEvent;
  }
  
    /**
     *@brief Creates an error event for a PTP connection.
     */
    csm::daemon::NetworkEvent* CreateErrorEventAgg(
        const int errcode, 
        const std::string& errmsg,
        const csm::daemon::CoreEvent& aEvent)
    {
        csm::daemon::NetworkEvent *ev = (csm::daemon::NetworkEvent *)&aEvent;
        csm::network::MessageAndAddress reqContent = ev->GetContent();

        char *buf=nullptr;
        uint32_t bufLen = 0;
        buf = csmi_err_pack(errcode, errmsg.c_str(), &bufLen);
        uint8_t flags = CSM_HEADER_RESP_BIT | CSM_HEADER_ERR_BIT;

        csm::network::Message rspMsg;
        bool ret = CreateNetworkMessage(reqContent._Msg, buf, bufLen, flags, rspMsg);
        if (buf) free(buf);
        
        csm::daemon::NetworkEvent *netEvent = nullptr;
        //csm::network::Address_sptr rspAddress = CreateReplyAddress(reqContent.GetAddr().get());
        if ( ret ) {
          csm::network::MessageAndAddress errContent ( rspMsg, _AbstractAggregator );
          
          netEvent = new csm::daemon::NetworkEvent(errContent,
                                csm::daemon::EVENT_TYPE_NETWORK, nullptr);
        }
        return netEvent;
    }

  csm::daemon::NetworkEvent* CreateErrorEvent(const int errcode, const std::string& errmsg,
                    const csm::daemon::CoreEvent& aEvent)
  {
    const csm::daemon::NetworkEvent *ev = dynamic_cast<const csm::daemon::NetworkEvent *>( &aEvent );
    csm::network::MessageAndAddress reqContent = ev->GetContent();
    return CreateErrorEvent(errcode, errmsg, reqContent);
    
  }
  
  /**
    \brief Create a NetworkEvent with the original NetworkEvent and a list of string at Master
    \note The returned NetworkEvent has a multi-cast flag set and needs to interpret the data differently.
    
    \param[in] msg The input Message that needs to convert to a multi-cast message
    \param[in] list A list of strings need to be added in the Message's data
    
    \return Return a Multi-cast NetworkEvent
  */
  csm::daemon::NetworkEvent* CreateMulticastEvent(const csm::network::Message &msg,
                                    const std::vector<std::string> &list,
                                    const csm::daemon::EventContext_sptr aContext = nullptr)
  {
    csm::network::Message outMsg;
    if ( !csm::network::CreateMulticastMessage(msg, list, outMsg) ) return nullptr;

    // master should send this message to aggregators using CSM/MASTER/QUERY mqtt topic
    csm::network::MessageAndAddress content( outMsg, _AbstractAggregator );
      
    csm::daemon::NetworkEvent *netEvent = new csm::daemon::NetworkEvent(content,
                            csm::daemon::EVENT_TYPE_NETWORK, aContext);
      
    return netEvent;
  }
  
  /**
    \brief Create a NetworkEvent with the original NetworkEvent and a list of string at Master
    \note The returned NetworkEvent has a multi-cast flag set and needs to interpret the data differently.
    
    \param[in] aEvent The input CoreEvent that needs to convert to a multi-cast message
    \param[in] list A list of strings need to be added in the Message's data
    
    \return Return a Multi-cast NetworkEvent
  */
  csm::daemon::NetworkEvent* CreateMulticastEvent(const csm::daemon::CoreEvent& aEvent, const std::vector<std::string> &list)
  {
    csm::daemon::NetworkEvent *ev = (csm::daemon::NetworkEvent *)&aEvent;
    csm::network::MessageAndAddress reqContent = ev->GetContent();
    csm::network::Message msg = reqContent._Msg;
    return CreateMulticastEvent(msg, list);
  }
  
  /***********************************************************
   ***              Helper functions for NetworkEvent      ***
   **********************************************************/
  /**
    \brief Check if a CoreEvent is a NetworkEvent
  */
  inline bool isNetworkEvent(const csm::daemon::CoreEvent& aEvent)
  {
    return aEvent.HasSameContentTypeAs( typeid( csm::daemon::EventContentContainer<csm::network::MessageAndAddress> ) );
  }

  /**
    \brief Return the Message in the CoreEvent
    \note aEvent must be a NetworkEvent
  */
  inline csm::network::Message GetNetworkMessage(const csm::daemon::CoreEvent &aEvent)
  {
    csm::network::MessageAndAddress nwd = dynamic_cast<const csm::daemon::NetworkEvent *>( &aEvent )->GetContent();
    return nwd._Msg;
  }

  /**
    \brief Return the Address in the CoreEvent
    \note aEvent must be a NetworkEvent
  */
  inline csm::network::Address_sptr GetNetworkAddress(const csm::daemon::CoreEvent &aEvent)
  {
    csm::network::MessageAndAddress nwd = dynamic_cast<const csm::daemon::NetworkEvent *>( &aEvent )->GetContent();
    return nwd.GetAddr();
  }

  /**
    \brief Return the MessageAndAddress in the CoreEvent
    \note aEvent must be a NetworkEvent
  */
  inline csm::network::MessageAndAddress GetNetworkContent(const csm::daemon::CoreEvent &aEvent)
  {
    csm::network::MessageAndAddress nwd = dynamic_cast<const csm::daemon::NetworkEvent *>( &aEvent )->GetContent();
    return nwd;
  }
  
  /**
    \brief Get the payload and payloadLen in a CoreEvent
    \note The CoreEvent has to be a NetworkEvent
    
    \param[in] aEvent a NetworkEvent
    \param[out] payload The data in Message
    \param[out] payloadLen The data length in Message
  */
  inline void GetPayloadFromNetworkEvent(const csm::daemon::CoreEvent &aEvent,
                     std::string& payload, uint32_t& payloadLen)
  {
    csm::daemon::NetworkEvent *ev = (csm::daemon::NetworkEvent *)&aEvent;
    csm::network::MessageAndAddress content = ev->GetContent();
    payload = content._Msg.GetData();
    payloadLen = content._Msg.GetDataLen();
  }

  /**
    \brief Get the Error Flag in Message
    \note aEvent has to be a NetworkEvent
    
    \param[in] aEvent The NetworkEvent
    \param[out] Return the error flag in Message
  */
  inline bool GetErrFlagFromNetworkEvent(const csm::daemon::CoreEvent &aEvent)
  {
    csm::daemon::NetworkEvent *ev = (csm::daemon::NetworkEvent *)&aEvent;
    csm::network::MessageAndAddress content = ev->GetContent();
    return content._Msg.GetErr();
  }

  /**
    \brief Get the MessageId field in the CoreEvent in the EventContext
    \note The CoreEvent in EventContext has to be a NetworkEvent
    
    \param[in] aContext The EventContext which contains the CoreEvent.
    \param[out] msgId The MessageId in the NetworkEvent content
    
    \return True if the CoreEvent in the context is valid and is a NetworkEvent, false otherwise.
  */
  inline bool GetMessageIDFromRequestInContext(const csm::daemon::EventContext_sptr aContext, uint64_t& msgId)
  {
    csm::daemon::CoreEvent *req = aContext->GetReqEvent();
    if (req == nullptr) return false;
    if ( !isNetworkEvent(*req) ) return false;
    csm::daemon::NetworkEvent *event = (csm::daemon::NetworkEvent *) req;
    csm::network::MessageAndAddress content = event->GetContent();
    csm::network::Message msg = content._Msg;
    msgId = msg.GetMessageID();
    
    return true;    
  }
  
  /**
    \brief Create a NetworkEvent using the MessageAndAddress and context
  */
  inline csm::daemon::NetworkEvent* CreateNetworkEvent(const csm::network::MessageAndAddress &content,
                                                        const csm::daemon::EventContext_sptr aContext = nullptr)
  {
    return ( new csm::daemon::NetworkEvent(content, csm::daemon::EVENT_TYPE_NETWORK, aContext) );
  }
  /**
    \brief Create a NetworkEvent using the Message, Address and context
  */
  inline csm::daemon::NetworkEvent* CreateNetworkEvent(const csm::network::Message &msg, const csm::network::Address_sptr addr,
                                                      const csm::daemon::EventContext_sptr aContext = nullptr)
  {
    csm::network::MessageAndAddress content( msg, addr );
    return CreateNetworkEvent(content, aContext);
  }
 
  /**
    \brief Create a new NetworkEvent with the Address given in addr
    
    \param[in] payload The byte array in the returned Message payload
    \param[in] payloadLen The length of the byte array
    \param[in[ flags The flags to be set in the returned Message header
    \param[in] msg The Message's command/priority/src/dst will be copied to the returned Message
    \param[in] addr The address of the destination
    \param[in] aContext The context will be associated with the returned NetworkEvent
    \param[in] isError if true, the error flag in the returned Message will be set.
    
    \return Return the NetworkEvent. Nullptr if an error occurs
  */
  csm::daemon::NetworkEvent* CreateNetworkEvent(const char *payload, const uint32_t payloadLen, const uint32_t flags,
                    const csm::network::Message &msg, const csm::network::Address_sptr addr,
                    const csm::daemon::EventContext_sptr aContext,
                    bool isError)
  {
    csm::network::Message rspMsg;
    bool ret = CreateNetworkMessage(msg, payload, payloadLen, flags, rspMsg);
    
    csm::daemon::NetworkEvent *netEvent = nullptr;
    if (ret)
    {
      csm::network::MessageAndAddress content( rspMsg, addr );
      
      netEvent = new csm::daemon::NetworkEvent(content,
                            csm::daemon::EVENT_TYPE_NETWORK, aContext);
    }
    
    return netEvent;
  }
  
  // return true if a network message is successfully initialized
  // Note: Will use command/priority/srcAddr/dstAddr from the msg and use other parameters
  //       to create the message. For message id, will use the one in msg if msgId is a null-ptr
  /**
    \brief Create a new Message
    
    \param[in] msg The message's priority/src/dst will be used in the returned message
    \param[in] buf The data in the returned message
    \param[in] bufLen The data len in the returned message
    \param[in] flags The flags set in the returned message
    \param[in] msgId The pointer to a message id. If nullptr, will use msg's message id 
               in the returned message.
               
    \param[out] rspMsg The returned Message
    
    \return Return true if the returned message has a valid checksum
  */
  inline bool CreateNetworkMessage(const csm::network::Message& msg,
                            const char *buf, const uint32_t bufLen, const uint8_t flags,
                            csm::network::Message &rspMsg,
                            const uint64_t *msgId = nullptr)
  {
    uint64_t messageId = (msgId == nullptr)? msg.GetMessageID():(*msgId);
    bool hdrvalid = rspMsg.Init(msg.GetCommandType(),
                        flags,
                        msg.GetPriority(),
                        messageId,
                        msg.GetSrcAddr(),
                        msg.GetDstAddr(),
                        msg.GetUserID(),
                        msg.GetGroupID(),
                        std::string(buf, bufLen));
    if (!hdrvalid)
    {
      LOG(csmapi, error) << "Message.Init returned FALSE!";
    }
    return hdrvalid;
  }
  
  /**
    \brief Create a NetworkEvent with MessageId set to 0
    
    \param[in] msg The message's message id will be reset and then used in the returned NetworkEvent
    \param[in] addr The Address in the returned NetworkEvent
    \param[in] aContext The context associated with the returned NetworkEvent
    
    \return return the NetworkEvent. Nullptr if an error occurs
  */
  inline csm::daemon::NetworkEvent* ForwardNetworkEventWithMessageId0(csm::network::Message msg,
                                                                      const csm::network::Address_sptr addr,
                                                                      const csm::daemon::EventContext_sptr aContext = nullptr)
  {
    msg.SetMessageID(0);
    msg.CheckSumUpdate();
      
    return ( CreateNetworkEvent(msg, addr, aContext) );
  }
  
  /**
    \brief Create a NetworkEvent with MessageId set to 0
    \note reqEvent has to be a NetworkEvent
    
    \param[in] reqEvent Use the Message in reqEvent but change its message id to 0
    \param[in] addr The Address in the returned NetworkEvent
    \param[in] aContext The context associated with the returned NetworkEvent
    
    \return return the NetworkEvent. Nullptr if an error occurs
  */
  inline csm::daemon::NetworkEvent* ForwardNetworkEventWithMessageId0(const csm::daemon::CoreEvent& reqEvent,
                                                const csm::network::Address_sptr addr,
                                                const csm::daemon::EventContext_sptr aContext = nullptr)
  {
    csm::daemon::NetworkEvent *ev = (csm::daemon::NetworkEvent *)&reqEvent;
    csm::network::MessageAndAddress reqContent = ev->GetContent();
      
    return ( ForwardNetworkEventWithMessageId0(reqContent._Msg, addr, aContext) );
  }
  
  /**
    \brief Forward a reply NetworkEvent for an earlier request
    \note respEvent has to be a NetworkEvent.
    If the ReqEvent in the reqContext is valid, its address has to be UNIX socket or TCP
    
    \param[in] respEvent The response NetworkEvent
    \param[in] reqContext The context of the earlier request. Its address has to be a UNIX socket or TCP
    \param[in] aContext The context associated with the returned Networkevent
    
    \return Return the NetworkEvent. Nullptr if an error occurs.
  */
  csm::daemon::NetworkEvent* ForwardReplyNetworkEvent(const csm::daemon::CoreEvent& respEvent,
                                                const csm::daemon::EventContext_sptr reqContext,
                                                const csm::daemon::EventContext_sptr aContext = nullptr)
  {
    //LOG(csmapi, info) << "CSMI_BASE::ForwardReplyNetworkEvent(): forwarding reply to original requester...";
    csm::daemon::NetworkEvent *ev = (csm::daemon::NetworkEvent *)&respEvent;
    csm::network::MessageAndAddress respContent = ev->GetContent();
    
    csm::daemon::NetworkEvent *reqEvent = (csm::daemon::NetworkEvent *) reqContext->GetReqEvent();
    if (reqEvent == nullptr)
    {
      LOG(csmapi, error) << "CSMI_BASE::ForwardReplyNetworkEvent(): reqEvent is nullptr. Drop the response!";
      return nullptr;
    }
    
    csm::network::MessageAndAddress reqContent = reqEvent->GetContent();
    if (reqContent.GetAddr()->GetAddrType() != csm::network::CSM_NETWORK_TYPE_LOCAL &&
        reqContent.GetAddr()->GetAddrType() != csm::network::CSM_NETWORK_TYPE_PTP)
    {
      LOG(csmapi, error) << "CSMI_BASE::ForwardReplyNetworkEvent(): Address in reqEvent is not expected.";
      return nullptr;
    }

    return ( CreateNetworkEvent(respContent._Msg, reqContent.GetAddr(), aContext) );
  }
  
  /***********************************************************
   ***              Helper functions for DB Event          ***
   **********************************************************/
   
  /**
    \brief Check if aEvent is a DBRespEvent
  */
  inline bool isDBRespEvent(const csm::daemon::CoreEvent& aEvent)
  {
    return aEvent.HasSameContentTypeAs( typeid( csm::daemon::EventContentContainer<csm::db::DBRespContent> ) );
  }
  /**
    \brief Check is aEvent is a DBReqEvent
  */
  inline bool isDBReqEvent(const csm::daemon::CoreEvent& aEvent)
  {
    return aEvent.HasSameContentTypeAs( typeid( csm::daemon::EventContentContainer<csm::db::DBReqContent> ) );
  }

  /**
    \brief Return the DBRespContent in the CoreEvent
    \note aEvent must be a DBRespEvent
  */
  inline csm::db::DBRespContent GetDBRespContent(const csm::daemon::CoreEvent &aEvent)
  {
    csm::daemon::DBRespEvent *ev = (csm::daemon::DBRespEvent *)&aEvent;
    csm::db::DBRespContent reqContent = ev->GetContent();
    return reqContent;
  }
    
  /**
    \brief Create a DBReqEvent containing sqlStmt and the context
  */
  inline csm::daemon::DBReqEvent* CreateDBReqEvent(const std::string &sqlStmt,
                                                   const csm::daemon::EventContext_sptr context,
                                                   const csm::db::DBConnection_sptr dbConn=nullptr)
  {
    csm::db::DBReqContent dbcontent(sqlStmt, dbConn);
    csm::daemon::DBReqEvent *dbevent = new csm::daemon::DBReqEvent(dbcontent, csm::daemon::EVENT_TYPE_DB_Request, context);
    return dbevent;
  }

  /**
    \brief Convert the tuples in dbRes to a vector of tuple
    
    \param[in] dbRes A pointer to a DBResult
    \param[out] tuples A vector of csm::db::DBTuple pointers
    
    \return Return false if dbRes does not have success status
  */
  bool GetTuplesFromDBResult(csm::db::DBResult_sptr dbRes, std::vector<csm::db::DBTuple *>& tuples)
  {
    if (dbRes->GetResStatus() != csm::db::DB_SUCCESS) return false;
    
    int nrows = dbRes->GetNumOfTuples();

    tuples.resize(nrows);
    for (int i=0;i<nrows;i++) {
      tuples[i] = dbRes->GetTupleAtRow(i);
    }
    
    return true;
  }

  /**
    \brief Check if aEvent (DBRespEvent) has accessed the DB successfully.
    \note aEvent has to be a DBRespEvent
    
    \param[in] aEvent a DBRespEvent
    \param[out] errcode The errcode if dbRes is not valid
    \param[out] errmsg The errmsg if dbRes is not valid
    
    \return Return true if aEvent is a DBRespEvent and it contains a valid DB result.
  */
  bool InspectDBResult(csm::db::DBRespContent &dbResp, int &errcode, std::string& errmsg)
  {
    csm::db::DBResult_sptr dbRes = dbResp.GetDBResult();
    // dbRes == nullptr may no longer happen. The dbMgr will not forward nullptr to handlers
    // as it may be due to db connection. dbMgr will leave the request in the queue and try later.
    if (dbRes == nullptr || dbRes->GetResStatus() != csm::db::DB_SUCCESS)
    {
      errcode = CSMERR_DB_ERROR;
      if (dbRes == nullptr)
        errmsg.append( "No Database Connection in Local Daemon" );
      else
        errmsg.append( dbRes->GetErrMsg() );
      return false;
    }
    else
      return true;

  }

  bool InspectDBResult(const csm::daemon::CoreEvent &aEvent, int &errcode, std::string& errmsg)
  {
    csm::daemon::DBRespEvent *dbevent = (csm::daemon::DBRespEvent *) &aEvent;
    csm::db::DBRespContent dbResp = dbevent->GetContent();
    return InspectDBResult(dbResp, errcode, errmsg);
  }

  /***********************************************************
   ***              Helper functions for EnvironmentalEvent       ***
   **********************************************************/
  inline bool isEnvironmentalEvent(const csm::daemon::CoreEvent& aEvent)
  {
    return aEvent.HasSameContentTypeAs( typeid( csm::daemon::EventContentContainer<csm::daemon::BitMap> ) );
  }
  
  inline csm::daemon::BitMap GetBitMap(const csm::daemon::CoreEvent &aEvent)
  {
    return dynamic_cast<const csm::daemon::EnvironmentalEvent *>( &aEvent )->GetContent();
  }
  /***********************************************************
   ***              Helper functions for TimerEvent       ***
   **********************************************************/
  inline bool isTimerEvent(const csm::daemon::CoreEvent& aEvent)
  {
    return aEvent.HasSameContentTypeAs( typeid( csm::daemon::EventContentContainer<csm::daemon::TimerContent> ) );
  }
  
  csm::daemon::TimerEvent *CreateTimerEvent( const uint64_t aMilliSeconds, void *aClass )
  {
    uint64_t contextId = CreateCtxAuxId();
    csm::daemon::TimerContent content( aMilliSeconds );
    csm::daemon::EventContext_sptr context( new csm::daemon::EventContext( aClass, contextId ) );
    return new csm::daemon::TimerEvent( content, csm::daemon::EVENT_TYPE_TIMER, context );
  }

  csm::daemon::TimerEvent *CreateTimerEvent( const uint64_t aMilliSeconds,
                                             const csm::daemon::EventContext_sptr aContext,
                                             const uint64_t aTargetState = UINT64_MAX )
  {
    csm::daemon::TimerContent content( aMilliSeconds, aTargetState );
    return new csm::daemon::TimerEvent( content, csm::daemon::EVENT_TYPE_TIMER, aContext );
  }

  inline uint64_t GetTimerInterval(const csm::daemon::CoreEvent &aEvent)
  {
    csm::daemon::TimerContent nwd = dynamic_cast<const csm::daemon::TimerEvent *>( &aEvent )->GetContent();
    return nwd.GetTimerInterval();
  }

  inline csm::daemon::NetworkEvent* CreateTimeoutNetworkResponse(const csm::daemon::CoreEvent &aEvent,
                                                                 const int aErrorCode,
                                                                 const std::string &aErrorText )
  {
    csm::daemon::TimerContent timerData = dynamic_cast<const csm::daemon::TimerEvent *>( &aEvent )->GetContent();
    const csm::daemon::NetworkEvent *request = dynamic_cast<const csm::daemon::NetworkEvent *>( aEvent.GetEventContext()->GetReqEvent() );
    if( request == nullptr )
      return nullptr;

    csm::network::MessageAndAddress msgAddr = request->GetContent();

    return ( CreateErrorEvent( aErrorCode, aErrorText, msgAddr ) );
  }

  /***********************************************************
   ***              Helper functions for SystemEvent       ***
   **********************************************************/
  
  /**
    \brief Check if a CoreEvent is a SystemEvent
  */
  inline bool isSystemEvent(const csm::daemon::CoreEvent& aEvent) const
  {
    return aEvent.HasSameContentTypeAs( typeid( csm::daemon::EventContentContainer<csm::daemon::SystemContent> ) );
  }
  
  inline csm::daemon::SystemContent::SIGNAL_TYPE getSystemEventType( const csm::daemon::CoreEvent &aEvent ) const
  {
    if( isSystemEvent( aEvent ))
    {
      csm::daemon::SystemContent content = ((csm::daemon::SystemEvent *) &aEvent)->GetContent();
      return content.GetSignalType();
    }
    return csm::daemon::SystemContent::UNDEFINED;
  }
  /**
    \brief Check if a CoreEvent is a SystemEvent with CONNECTED signal
  */
  inline bool isSystemConnectedEvent(const csm::daemon::CoreEvent& aEvent)
  {
    if (isSystemEvent(aEvent))
    {
      csm::daemon::SystemContent content = ((csm::daemon::SystemEvent *) &aEvent)->GetContent();
      if (content.GetSignalType() == csm::daemon::SystemContent::CONNECTED) return true;
    }
    return false;
  }
  
  inline bool isSystemConnectedEvent(csm::daemon::SystemContent& content)
  {
    return (content.GetSignalType() == csm::daemon::SystemContent::CONNECTED);
  }
  /**
    \brief Check if a CoreEvent is a SystemEvent with DISCONNECTED signal
  */
  inline bool isSystemDisconnectedEvent(const csm::daemon::CoreEvent& aEvent)
  {
    if (isSystemEvent(aEvent))
    {
      csm::daemon::SystemContent content = ((csm::daemon::SystemEvent *) &aEvent)->GetContent();
      if (content.GetSignalType() == csm::daemon::SystemContent::DISCONNECTED) return true;
    }
    return false;
  }
  inline bool isSystemDisconnectedEvent(csm::daemon::SystemContent& content)
  {
      return (content.GetSignalType() == csm::daemon::SystemContent::DISCONNECTED);
  }
  
  /**
    \brief Return the Address content in the System Event
    \note  The CoreEvent has to be a SystemEvent
  */
  inline const csm::network::Address_sptr GetAddrFromSystemEvent(const csm::daemon::CoreEvent& aEvent)
  {

    csm::daemon::SystemContent content = ((csm::daemon::SystemEvent *) &aEvent)->GetContent();
    return content.GetAddr();

  }

  /**
    \brief Register an new context to track system events of a particular type

    \param[in] aClass A Event Handler Object
    \param[in] aSignal  the signal type

    \return The new created context
  */
  inline csm::daemon::EventContext_sptr RegisterSystemEvent(void *aClass,
                                                            const csm::daemon::SystemContent::SIGNAL_TYPE aSignal )
  {
    csm::daemon::EventContext_sptr context = CreateContext(aClass, CreateCtxAuxId());
    _handlerOptions.GetDaemonState()->RegisterContext(aSignal, context);
    return context;
  }
  /**
    \brief Register the context to monitor new connection activity

    \param[in] aContext A EventContextType
    \param[in] aSignal  the additional signal type
  */
  inline void RegisterSystemEvent( const csm::daemon::EventContext_sptr aContext,
                                   const csm::daemon::SystemContent::SIGNAL_TYPE aSignal )
  {
    _handlerOptions.GetDaemonState()->RegisterContext( aSignal, aContext );
  }
  /**
    \brief Remove the context from monitoring connection activity

    \param[in] aContext A EventContextType
    \param[in] aSignal  the signal type

    \return True if the context can be found in the registration list, false if not found
  */
  inline bool UnregisterSystemEvent( const csm::daemon::EventContext_sptr aContext,
                                     const csm::daemon::SystemContent::SIGNAL_TYPE aSignal )
  {
    return _handlerOptions.GetDaemonState()->UnregisterContext( aSignal, aContext);
  }


  inline csm::daemon::SystemEvent* CreateJobStartSystemEvent( void *aClass = nullptr)
  {
    csm::daemon::EventContext_sptr context = (aClass == nullptr)? nullptr : CreateContext(aClass, CreateCtxAuxId());
    csm::daemon::SystemContent content(csm::daemon::SystemContent::JOB_START);
    return new csm::daemon::SystemEvent( content, csm::daemon::EVENT_TYPE_SYSTEM, context );
  }
  
  inline csm::daemon::SystemEvent* CreateJobStartSystemEvent( const csm::daemon::EventContext_sptr aContext )
  {
    csm::daemon::SystemContent content(csm::daemon::SystemContent::JOB_START);
    return new csm::daemon::SystemEvent( content, csm::daemon::EVENT_TYPE_SYSTEM, aContext );
  }

  inline csm::daemon::SystemEvent* CreateJobEndSystemEvent( void *aClass = nullptr)
  {
    csm::daemon::EventContext_sptr context = (aClass == nullptr)? nullptr : CreateContext(aClass, CreateCtxAuxId());
    csm::daemon::SystemContent content(csm::daemon::SystemContent::JOB_END);
    return new csm::daemon::SystemEvent( content, csm::daemon::EVENT_TYPE_SYSTEM, context );
  }
  
  inline csm::daemon::SystemEvent* CreateJobEndSystemEvent( const csm::daemon::EventContext_sptr aContext )
  {
    csm::daemon::SystemContent content(csm::daemon::SystemContent::JOB_END);
    return new csm::daemon::SystemEvent( content, csm::daemon::EVENT_TYPE_SYSTEM, aContext );
  }
    
  // only handle the Connected system event for MQTT connections
  bool CheckMQTTConnection( const csm::daemon::CoreEvent &aEvent )
  {
    if ( !isSystemEvent(aEvent) ) return false;
    
    return( ( isSystemConnectedEvent(aEvent) )&&
            ( GetAddrFromSystemEvent(aEvent)->GetAddrType() == csm::network::CSM_NETWORK_TYPE_AGGREGATOR ) );
  }
  
  inline csm::daemon::NetworkEvent* CreateRasEventMessage(const std::string &msg_id, 
                                                          const std::string &location_name,
                                                          const std::string &raw_data,
                                                          const std::string &kvcsv)
  {
    char* buffer = nullptr;
    uint32_t bufferLength = 0;

    // Generate a timestamp for this event 
    // Example format:
    // 2016-05-12 15:12:11.799506
    char time_stamp[80];
    char time_stamp_with_usec[80];

    struct timeval now_tv;
    time_t rawtime;
    struct tm *info;

    gettimeofday(&now_tv, NULL);
    rawtime = now_tv.tv_sec;
    info = localtime( &rawtime );

    strftime(time_stamp, 80, "%Y-%m-%d %H:%M:%S", info);
    snprintf(time_stamp_with_usec, 80, "%s.%06lu", time_stamp, now_tv.tv_usec);

    csm_ras_event_create_input_t rargs;

    csm_init_struct_versioning(&rargs);
    rargs.msg_id        = strdup(msg_id.c_str());
    rargs.time_stamp    = strdup(time_stamp_with_usec);
    rargs.location_name = strdup(location_name.c_str());
    rargs.raw_data      = strdup(raw_data.c_str());
    rargs.kvcsv         = strdup(kvcsv.c_str());

    
    csm_serialize_struct(csm_ras_event_create_input_t, &rargs, &buffer, &bufferLength );
    csm_free_struct(csm_ras_event_create_input_t, rargs);

    if ( buffer )
    {
      std::string payload(buffer, bufferLength);

      LOG(csmd, debug) << __FILE__ << ":" << __LINE__ << " " << "CreateRasEventMessage bufLen = " << bufferLength
          << " payloadlen = " << payload.size();

      free(buffer);
  
      uint8_t flags = CSM_HEADER_INT_BIT;
      csm::network::Message rspMsg;
      bool hdrvalid = rspMsg.Init(CSM_CMD_ras_event_create,
                          flags,
                          CSM_PRIORITY_DEFAULT, // aPriority
                          0,                  // aMessageID
                          1,                  // aSrcAddr
                          1,                  // aDstAddr
                          geteuid(), getegid(),               // user, group ID
                          payload);
  
      if (!hdrvalid) 
      {
        LOG(csmd, error) << __FILE__ << ":" << __LINE__ << " " << "Message.Init returned FALSE!";
        return(NULL);
      } 
      else 
      {
          LOG(csmras, info) << "RAS EVENT CREATED    " << "ts:" << time_stamp_with_usec << " loc:" << location_name << " msg:" << msg_id; 
          csm::network::MessageAndAddress netcontent( rspMsg, _AbstractMaster );
          csm::daemon::NetworkEvent *netEvent = new csm::daemon::NetworkEvent(netcontent,csm::daemon::EVENT_TYPE_NETWORK, NULL);
          return(netEvent);
      }
    }
    return(NULL);
  }

public:

  template<typename T>
  static std::string ConvertToBytes(const T i_option)
  {
    std::stringstream ss;
    boost::archive::text_oarchive oa(ss);
    oa << i_option;
    return ss.str();
  }

  template<typename T>
  static void ConvertToClass(const std::string &i_payload, T &o_option)
  {
    std::stringstream ss;
    ss << i_payload;
    boost::archive::text_iarchive ia(ss);
    ia >> o_option;
  }
    
  inline virtual std::string getClassName() {
    return _className;
  }

  inline virtual csmi_cmd_t getCmdType() {
    return _cmdType;
  }
  
  inline virtual std::string getCmdName() const {
    return _cmdName;
  }

  inline virtual void setCmdName(std::string aString)
  { _cmdName = aString; }
  
  inline csm::daemon::API_SEC_LEVEL GetSecurityLevel() const { return _SecurityLevel; }

  virtual ~CSMI_BASE() { }

  inline void AddPublicAPIHandlers(std::unordered_set< std::shared_ptr<CSMI_BASE> >& list)
  { _PublicAPIHandlers = list; }
  
  inline void UpdateNewRequestCount() { _newRequestCount++; }
  inline int GetNewRequestCount() { return _newRequestCount; }
    /** @return The Abstract Aggregator associated with this handler. */
    inline csm::network::AddressAbstract_sptr GetAbstractAggregator() 
    { 
        return _AbstractAggregator; 
    }

    /** @return The Abstract Master associated with this handler. */
    inline csm::network::AddressAbstract_sptr GetAbstractMaster()     
    { 
        return _AbstractMaster; 
    }
  
protected:
  csmi_cmd_t _cmdType;
  csm::daemon::HandlerOptions _handlerOptions;
  std::string _cmdName;
  std::string _className;
  std::string _tabName;
  std::mutex _handlerLock;  // a global lock for the whole handler class
  packPrototype _packFunc;
  unpackPrototype _unpackFunc;
  packPrototype _argPackFunc;
  unpackPrototype _argUnpackFunc;
  uint64_t _AuxId;
  csm::network::AddressAbstract_sptr _AbstractAggregator;
  csm::network::AddressAbstract_sptr _AbstractMaster;
  csm::network::AddressAbstract_sptr _AbstractBroadcast;
  csm::network::AddressAbstract_sptr _AbstractSelf;
  std::map<int, csmi_cmd_err_t> _errnoTocmderr;
  std::unordered_set< std::shared_ptr<CSMI_BASE> > _PublicAPIHandlers;

    
private:
  csm::daemon::API_SEC_LEVEL _SecurityLevel;
  int _newRequestCount;
};

typedef std::shared_ptr<CSMI_BASE> CSMI_BASE_sptr;

#endif
