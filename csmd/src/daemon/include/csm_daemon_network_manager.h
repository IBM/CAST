/*================================================================================

    csmd/src/daemon/include/csm_daemon_network_manager.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSM_DAEMON_SRC_CSM_DAEMON_NETWORK_HANDLING_H_
#define CSM_DAEMON_SRC_CSM_DAEMON_NETWORK_HANDLING_H_

#include <boost/thread.hpp>

#include <string>
#include <typeindex>
#include <deque>
#include <map>
#include <mutex>
#include <condition_variable>

#include "csmnet/src/CPP/csm_network_exception.h"
#include "csmnet/src/CPP/csm_network_msg_cpp.h"
#include "csmnet/src/CPP/address.h"
#include "csmnet/src/CPP/csm_message_and_address.h"
#include "csmnet/src/CPP/endpoint.h"
#include "csmnet/src/CPP/reliable_msg.h"

#include "csmi/src/common/include/csmi_serialization.h"

#include "include/csm_checksum_exception.h"
#include "csm_daemon_config.h"
#include "include/csm_core_event.h"
#include "include/message_control.h"
#include "include/csm_system_event.h"
#include "include/csm_daemon_state.h"
#include "include/csm_event_sink.h"
#include "include/csm_event_source.h"
#include "include/csm_event_manager.h"
#include "include/virtual_network_channel.h"
#include "include/virtual_network_channel_pool.h"
#include "include/connection_handling.h"
#include "include/csm_retry_backoff.h"
#include "include/interval_trigger.h"

// note: not include event routing here. use forward declaration. avoid circular dependency.
//#include "include/csm_event_routing.h"

namespace csm {
namespace daemon {

//class EventRouting; // class fowarding declaration. Avoid circular dependency!!!

class EventManagerNetwork : public csm::daemon::EventManager
{
  csm::network::ReliableMsg *_EndpointList;
  boost::thread * _Thread;
  volatile std::atomic<bool> _KeepThreadRunning;
  std::mutex _ThreadGreenlightLock;
  std::condition_variable _ThreadGreenlightCondition;
  std::atomic_bool _ReadyToRun;

  //csm::daemon::EventRouting* _EventRouting;
  csm::daemon::ConnectionHandling *_ConnMgr;
  csm::daemon::IntervalTrigger _ReconnectInterval;
  csm::daemon::DaemonState *_DaemonState;
  csm::daemon::MessageControl _MessageControl;
  csm::daemon::VirtualNetworkChannelPool *_NetworkChannelPool;
  csm::daemon::VirtualNetworkChannel_sptr _NetMgrChannel;
  csm::daemon::RetryBackOff *_IdleLoopRetry;
  csm::daemon::Configuration *_Config;
  
public:
  EventManagerNetwork( csm::daemon::ConnectionHandling *aNetMgr,
                       csm::daemon::DaemonState *aDaemonState,
                       csm::daemon::RetryBackOff *i_MainIdleLoopRetry );

  virtual ~EventManagerNetwork()
  {
    // as we go down, try to send a last compute-set update to make sure the master has the most recent information
    try { CreateComputeSetUpdateMsg(); }
    catch(...) { LOG( csmd, error ) << "Failed to send final compute node update to master."; }

    srand( time(nullptr) );
    _KeepThreadRunning = false;
    Unfreeze();
    _IdleLoopRetry->JoinThread( _Thread );
    _NetMgrChannel = nullptr;
    delete _IdleLoopRetry;
    delete _Thread;
    if (_NetworkChannelPool) delete _NetworkChannelPool;
    LOG( csmd, info ) << "Terminating NetMgr complete.";
  }

  virtual int Freeze()
  {
    LOG( csmd, trace ) << "NetworkMgr: Freeze... state=" << _ReadyToRun;
    _ReadyToRun = false;
    _Thread->interrupt();
    return 0;
  }
  virtual int Unfreeze()
  {
    LOG( csmd, trace ) << "NetworkMgr: Unfreeze...? " << _ReadyToRun;
    std::unique_lock<std::mutex> glock( _ThreadGreenlightLock );
    _ReadyToRun = true;
    _ThreadGreenlightCondition.notify_all();
    return 0;
  }
  void GreenLightWait()
  {
    std::unique_lock<std::mutex> glock( _ThreadGreenlightLock );
    while( !_ReadyToRun )
    {
      _ThreadGreenlightCondition.wait( glock );
      LOG( csmd, trace ) << "NetworkMgr: GreenLightWait condition met..? " << _ReadyToRun;
    }
  }


  inline bool GetThreadKeepRunning() const { return _KeepThreadRunning; }
  inline csm::daemon::RUN_MODE::mode_t GetRunMode() const { return _DaemonState->GetRunMode(); }
  inline bool IsARunningMode() const { return _DaemonState->IsARunningMode(); }
  inline csm::network::ReliableMsg * GetEndpointList() const { return _EndpointList; }
  inline csm::daemon::MessageControl* GetMessageControl()
  {
    return &_MessageControl;
  }
  inline csm::daemon::VirtualNetworkChannel_sptr GetNetMgrChannel() const { return _NetMgrChannel; }
  inline csm::daemon::RetryBackOff *GetRetryBackoff() { return _IdleLoopRetry; }

  bool ProcessNetCtlEvents();
  bool EndpointMaintenance();
  bool EndpointRecvActivity();
  bool EndpointSendActivity();
  bool EndpointIdleMaintenance();

  void CreateNetworkConnectionPool();

  inline void QueueInboundEvent( csm::daemon::CoreEvent * const i_Event,
                                 const csm::daemon::VirtualNetworkChannel_sptr i_VChan = nullptr )
  {
    // check if this is for some existing vchannel, otherwise just queue for regular event queue
    bool queued = false;

    csm::daemon::EventContext_sptr ctx = i_Event->GetEventContext();
    LOG( csmd, debug ) << "QueueInboundEvent: type=" << i_Event->GetEventType()
        << " ctx=" << (ctx != nullptr? ctx.get(): 0);
      
    if( ( i_VChan != nullptr ) && ( i_VChan->GetId() != _NetMgrChannel->GetId() ) )
    {
      queued = i_VChan->QueueInboundEvent( i_Event );   // try to queue to vchannel
    }

    if( ! queued )
    {
      _Source->QueueEvent( i_Event );
    }

  }
  inline csm::daemon::NetworkEvent * DequeueOutboundEvent( )
  {
    return dynamic_cast<csm::daemon::NetworkEvent*>( _Sink->FetchEvent() );
  }

  inline void QueueRecvEvent( csm::network::MessageAndAddress_sptr aData,
                              csm::daemon::MessageContextContainer_sptr aMsgContext )
  {
    //if (_EventRouting == nullptr) {
    //  LOG(csmd, error) << "EventManagerNetwork::QueueEvent(): Fail to access _EventRouting";
    //  return;
    //}

    if(( aMsgContext == nullptr ) || ( aMsgContext->_MsgAddr == nullptr ))
      throw csm::daemon::Exception("BUG: QueueRecvEvent() called with nullptr MsgContext or Msg.");

    // check if this is for some existing vchannel, otherwise just queue for regular event queue
    bool queued = false;

    if(( aMsgContext->_Context == nullptr ) && ( aMsgContext->_MsgAddr->_Msg.GetResp() ))
    {
      csm::network::MessageAndAddress_sptr msgAddr = aMsgContext->_MsgAddr;
      LOG( csmd, warning ) << "timeout/late response to cmd " << csm::network::cmd_to_string( msgAddr->_Msg.GetCommandType() )
          << " without context from " << (msgAddr->GetAddr() != nullptr ? msgAddr->GetAddr()->Dump() : "n/a" );
      return;
    }

    LOG( csmd, debug ) << "QueueInboundMsg: msgId=" << aData->_Msg.GetMessageID()
        << " ctx=" << aMsgContext->_Context.get();

    if( ( aMsgContext->_Connection != nullptr ) && ( aMsgContext->_Connection->GetId() != _NetMgrChannel->GetId() ) )
    {
      queued = aMsgContext->_Connection->QueueInboundMsg( aData, aMsgContext->_Context );   // try to queue to vchannel
      if( ! queued )
      {
        aMsgContext->_Connection = _NetMgrChannel;
        queued = _NetMgrChannel->QueueInboundMsg( aData, aMsgContext->_Context );
      }
    }

    if( ! queued )
    {
      // to-do: need to figure out later where to destroy the network event below
      csm::daemon::NetworkEvent *nwe = new csm::daemon::NetworkEvent( *aData,
                                                                      csm::daemon::EVENT_TYPE_NETWORK,
                                                                      aMsgContext->_Context );
      _Source->QueueEvent( nwe );
    }
  }

  inline void QueueInboundErrorEvent( csm::network::MessageAndAddress_sptr i_Data,
                                      const csm::daemon::MessageContextContainer_sptr i_Ctx,
                                      const std::string &aMsg,
                                      int aRC )
  {
    if( i_Ctx == nullptr )
      throw csm::daemon::Exception("BUG: QueueInboundErrorEvent() called with nullptr context.");

    csm::daemon::NetworkEvent *nwe = nullptr;
    nwe = CreateErrorEvent( i_Data, i_Ctx->_Context, aMsg, aRC );
    QueueInboundEvent( nwe, i_Ctx->_Connection );
  }

  inline void QueueInboundSystemEvent( const csm::daemon::SystemContent::SIGNAL_TYPE i_Signal,
                                       const csm::network::Address_sptr i_Addr,
                                       const csm::daemon::EventContext_sptr i_Ctx,
                                       const csm::daemon::VirtualNetworkChannel_sptr i_VChan = nullptr )
  {
    csm::daemon::SystemEvent *se = CreateSystemEvent( i_Signal, i_Addr, i_Ctx );
    QueueInboundEvent( se, i_VChan );
  }
  
  // create send error event based on already existing message header
  inline void QueueSendErrorEvent( csm::network::MessageAndAddress &i_Data,
                                   const csm::daemon::MessageContextContainer_sptr i_Ctx,
                                   const std::string &i_Msg,
                                   int i_RC )
  {
    if( i_Ctx == nullptr )
      throw csm::daemon::Exception("BUG: QueueSendErrorEvent() called with nullptr context.");

    uint32_t len = i_Msg.length();
    // todo: check if that's really useful - maybe better to keep the cmd as is (if available)
    i_Data._Msg.SetCommandType( CSM_CMD_ERROR );
    i_Data._Msg.SetErr();
    char *buf = csmi_err_pack( i_RC, i_Msg.c_str(), &len );
    if (buf)
    {
      i_Data._Msg.SetData( std::string( buf, len) );
      free(buf);
    }

    csm::daemon::NetworkEvent *nwe = new csm::daemon::NetworkEvent( i_Data, csm::daemon::EVENT_TYPE_NETWORK, i_Ctx->_Context );
    QueueInboundEvent( nwe, i_Ctx->_Connection );
  }

  inline bool GetOutboundActivity( csm::daemon::VirtualNetworkChannel_sptr &o_VChannel,
                                   csm::daemon::EventContext_sptr &o_Context,
                                   csm::network::MessageAndAddress_sptr &o_MsgAddr )
  {
    csm::daemon::NetworkEvent *nwe = DequeueOutboundEvent();
    if( ! nwe )
    {
      bool ret = _NetworkChannelPool->GetOutboundActivity( o_VChannel, o_Context, o_MsgAddr );
      if( ret )
      {
        LOG( csmd, debug ) << "Found VChannel Outbound Activity VChan=" << o_VChannel->GetId()
            << " msgId=" << o_MsgAddr->_Msg.GetMessageID();
      }
      return ret;
    }

    o_VChannel = _NetMgrChannel;
    o_Context = nwe->GetEventContext();
    o_MsgAddr = std::make_shared<csm::network::MessageAndAddress>( nwe->GetContent() );

    // dispose the event after retrieving all content
    delete nwe;

    return true;
  }

  void NotifyRegisteredContext(const csm::daemon::SystemContent::SIGNAL_TYPE aSignal, const csm::network::Address_sptr addr)
  {
    // only do the handler notification if the runmode is indicating that we're running
    if( !IsARunningMode() )
    {
      LOG( csmd, debug ) << "NETMGR: skipping handler notification because we're not in a running mode: " << GetRunMode();
      return;
    }
        // pass the disconnected activity to registered event handlers
    const csm::daemon::ContextListType* list = _DaemonState->GetContextList(aSignal);
    if( list == nullptr )
      return;
    for ( auto it : *list )
    {
      LOG(csmd, debug) << "NetMgr: queue system event " << addr->Dump() << " for registered handler: " << it->GetEventHandler();
      QueueInboundSystemEvent(aSignal, addr, it);
    }
  }

  inline boost::thread* GetThread() const { return _Thread; }

private:
  AddressListType GetMasterAddress();
  AddressListType GetAggregatorAddress();
  bool FilterSecondary( csm::daemon::ConnectedNodeStatus *i_NodeInfo,
                        const csm::network::MessageAndAddress_sptr i_MsgAddr ) const;
  int CreateDestinationAddresses( csm::network::MessageAndAddress_sptr i_MsgAddr,
                                  AddressListType &destAddrList );
  int ExtractDestinationAddresses( csm::network::MessageAndAddress_sptr i_MsgAddr,
                                   csm::daemon::MessageContextContainer_sptr i_MsgContext,
                                   AddressListType &o_DestAddrList );
  bool HandleDisconnect( csm::network::NetworkCtrlInfo_sptr info_itr );

  csm::daemon::NetworkEvent* CreateErrorEvent( const csm::network::MessageAndAddress_sptr aData,
                                               const csm::daemon::EventContext_sptr aCtx,
                                               const std::string &aMsg,
                                               const int aRC )
  {
    if( aData == nullptr )
      throw csm::daemon::Exception("BUG: QueueInboundErrorEvent() called with nullptr msg.");

    uint32_t len = aMsg.length();
    char *buf = csmi_err_pack( aRC, aMsg.c_str(), &len );
    if (buf)
    {
      aData->_Msg.CreateError( false, CSM_PRIORITY_DEFAULT, std::string( buf, len) );
      free(buf);
    }
    else
      aData->_Msg.CreateError( false, CSM_PRIORITY_DEFAULT );

    LOG(csmd,info) << "Creating error event: " << aMsg;
    return new csm::daemon::NetworkEvent( *aData, csm::daemon::EVENT_TYPE_NETWORK, aCtx );
  }

  csm::daemon::SystemEvent* CreateSystemEvent( const csm::daemon::SystemContent::SIGNAL_TYPE aSignal,
                                               const csm::network::Address_sptr aAddr,
                                               const csm::daemon::EventContext_sptr aCtx)
  {
    csm::daemon::SystemContent content(aSignal, aAddr);
    LOG(csmd,debug) << "Creating system event: " << aSignal;
    return new csm::daemon::SystemEvent( content, csm::daemon::EVENT_TYPE_SYSTEM, aCtx );

  }

  void QueueFailoverMsg( csm::network::Address_sptr addr );
  bool CreateComputeSetUpdateMsg();
  bool CheckAndGenerateComputeActions( csm::daemon::DaemonStateMaster *masterState );

};

}  // namespace daemon
} // namespace csm

#endif /* CSM_DAEMON_SRC_CSM_DAEMON_NETWORK_HANDLING_H_ */
