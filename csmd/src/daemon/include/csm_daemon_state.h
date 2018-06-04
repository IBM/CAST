/*================================================================================
   
    csmd/src/daemon/include/csm_daemon_state.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#ifndef CSM_DAEMON_SRC_CSM_DAEMON_STATE_H_
#define CSM_DAEMON_SRC_CSM_DAEMON_STATE_H_
#include <map>
#include <mutex>
#include <vector>
#include <memory>

#include "csmnet/include/csm_network_config.h"
#include "csmnet/src/CPP/address.h"
#include "csmnet/src/CPP/csm_network_msg_cpp.h"
#include "csmnet/src/CPP/csm_message_and_address.h"

#include "include/run_mode.h"
#include "include/csm_system_event.h"
#include "include/csm_core_event.h"
#include "include/csm_daemon_exception.h"
#include "include/csm_timer_event.h"
#include "include/csm_node_set.h"

#include "csmd/src/inv/include/inv_dcgm_access.h"
#include "include/csm_environmental_data.h"

#include "include/csm_connection_type.h"

namespace csm {
namespace daemon {

// mainly used by aggregator to track compute nodes, but likely useful later for other purposes too
class ConnectedNodeStatus {
public:
  csm::network::Address_sptr _NodeAddr;       ///< address of the node
  RUN_MODE::mode_t _NodeMode;                 ///< current status of the node
  ConnectionType::CONN_TYPE _ConnectionType;  ///< primary or secondary connection
  csm::network::Message _LastInventory;       ///< stores the latest inventory message from node
  bool _NeedSendInventory;                    ///< stores whether the inventory is recent
  std::string _NodeID;                        ///< record the UID related to the inventory actions.
  unsigned _Bounced;                          ///< how often did the node bounce since first registration
  CSM_Environmental_Data _EnvData;            ///< hold environmental data of the node

  ConnectedNodeStatus()
  : _NodeAddr(),
    _NodeMode(),
    _ConnectionType( ConnectionType::PRIMARY ),
    _LastInventory(),
    _NeedSendInventory( false ),
    _NodeID(""),
    _Bounced(0),
    _EnvData()
  {}

  ConnectedNodeStatus( const ConnectedNodeStatus& in )
  : _NodeAddr( in._NodeAddr ),
    _NodeMode( in._NodeMode ),
    _ConnectionType( in._ConnectionType ),
    _LastInventory( in._LastInventory ),
    _NeedSendInventory( in._NeedSendInventory ),
    _NodeID( in._NodeID ),
    _Bounced( in._Bounced ),
    _EnvData( in._EnvData )
  {}

  ConnectedNodeStatus& operator=( const ConnectedNodeStatus& in )
  {
    _NodeAddr = in._NodeAddr;
    _NodeMode = in._NodeMode;
    _ConnectionType = in._ConnectionType;
    _LastInventory = in._LastInventory;
    _NeedSendInventory = in._NeedSendInventory;
    _NodeID = in._NodeID;
    _Bounced = in._Bounced;
    _EnvData = in._EnvData;
    return *this;
  }
  // allows for only a single check, each call will reset to false
  inline bool NeedSendInventory()
  {
    bool state = _NeedSendInventory;
    _NeedSendInventory = false;
    return state;
  }
  inline void EnableSendInventory() { _NeedSendInventory = true; }
};
typedef std::shared_ptr<csm::daemon::ConnectedNodeStatus> ConnectedNodeStatus_sptr;


typedef std::map<csm::network::AddressCode, csm::daemon::ConnectedNodeStatus> CNStateMapType;

// map from the node UID (created from the CSM/XCAT node name) to an address key
typedef std::map<std::string, csm::network::AddressCode> NodeKeywordMapType;

typedef std::vector<csm::daemon::EventContext_sptr> ContextListType;
typedef std::map< csm::daemon::SystemContent::SIGNAL_TYPE, ContextListType* > ContextMapType;

typedef std::vector<csm::network::Address_sptr> AddressListType;


class DaemonState
{
public:
  DaemonState( const uint64_t aDaemonID,
               const bool aRequiresDCGM = false )
  :_daemon_id(aDaemonID),
   _event_load(0.0),
   _run_mode( nullptr ),
   _RequiresDCGM( aRequiresDCGM ),
   _DcgmAccess( nullptr )
  { }
  
  virtual ~DaemonState();

  csm::network::AddressCode GenerateNodeID( const csm::network::Address_sptr i_Addr ) const;

  // need to add additional parameter to indicate whether the conn is PRIMARY or SECONDARY
  virtual void AddEP(const csm::network::Address_sptr addr,
                     const ConnectionType::CONN_TYPE type = ConnectionType::SINGLE,
                     const RUN_MODE::mode_t runmode = RUN_MODE::READY_RUNNING,
                     const csm::network::Message *msg = nullptr,
                     const std::string nodeID = "" );
  
  virtual void SetConnectionTypeEP( const csm::network::Address_sptr addr,
                                    const csm::daemon::ConnectionType::CONN_TYPE );
  virtual void DisconnectEP(const csm::network::Address_sptr addr);
  virtual void ConnectEP(const csm::network::Address_sptr addr);
  
  virtual bool UpdateEnvironmentalData( const csm::network::Address_sptr addr, const CSM_Environmental_Data& data );
  std::string GetCNUidFromAddr(const csm::network::Address_sptr addr) const;
  
  virtual int GetAllEPs(csm::daemon::AddressListType &list,
                        const ConnectionType::CONN_TYPE type = ConnectionType::ANY,
                        const bool i_Connected = true );

  inline uint64_t GetDaemonID() const { return _daemon_id; }
  inline void SetDaemonID( const uint64_t aDaemonId ) { _daemon_id = aDaemonId; }
  
  inline csm::network::Message GetLocalInventory() const { return _LocalInventory; }
  inline bool HasLocalInventory() const { return _LocalInventory.GetDataLen() > 0; }
  inline void SetLocalInventory( const csm::network::Message &msg )
  {
    if( _LocalInventory.GetDataLen() > 0 )
      LOG( csmd, warning ) << "Warning: overwriting existing inventory! This might cause inconsistencies down the line.";
    _LocalInventory = msg;
  }

  void RegisterContext(const csm::daemon::SystemContent::SIGNAL_TYPE aSignal,
                       const csm::daemon::EventContext_sptr aContext);
  
  ContextListType* GetContextList(const csm::daemon::SystemContent::SIGNAL_TYPE aSignal) const;
  
  bool UnregisterContext(const csm::daemon::SystemContent::SIGNAL_TYPE aSignal,
                         const csm::daemon::EventContext_sptr aContext);
  void RecordPerfData(std::string i_csmi_api, double i_ms);
  std::string DumpPerfData();
  virtual std::string DumpMapSize();
  
  inline void UpdateEventLoadAndCount(uint64_t& prevEventCount, double timerInverse)
  {
    _event_load = 0.1*_event_load + 0.9*(event_count-prevEventCount)*timerInverse;
    prevEventCount= event_count;
  }
  inline double GetEventLoad() { return _event_load; }
  
  inline void UpdateTimestamp(TimerContent::TimeType aTime) { _timestamp = aTime; }
  inline TimerContent::TimeType GetTimestamp() { return _timestamp; }

  inline void SetRunModePtr( RunMode *i_RunMode_ptr )
  {
    _run_mode = i_RunMode_ptr;
  }
  inline RUN_MODE::mode_t GetRunMode() const
  {
    return _run_mode != nullptr ? _run_mode->Get() : csm::daemon::RUN_MODE::UNSPECIFIED;
  }
  inline bool IsARunningMode() const { return _run_mode->IsARunningMode(); }
  
  /*
   * acquire instance of dcgm
   *
   * from handler, one has to
   *      GetDaemonState()->GetDcgmInstance()-> .... ;
   * the first caller will trigger the DCGM initialization
   *
   */
  inline csm::daemon::INV_DCGM_ACCESS* GetDcgmInstance()
  {
    if( (_RequiresDCGM ) && ( _DcgmAccess == nullptr ) )
      _DcgmAccess = csm::daemon::INV_DCGM_ACCESS::GetInstance();
    return _DcgmAccess;
  }

  csm::daemon::ConnectedNodeStatus* GetNodeInfo( const csm::network::AddressCode i_AddrCode );
  csm::daemon::ConnectedNodeStatus* GetNodeInfo( const csm::network::Address_sptr addr );
  void SetNodeInfo( const csm::network::Address_sptr addr,
                    const std::string &id,
                    const csm::network::Message *msg = nullptr );

  csm::network::Address_sptr GetNextHopAddress( const std::string i_NodeName );
  virtual void InitActiveAddresses( ) {};
  virtual csm::network::Address_sptr GetActiveAddress() const { return nullptr; }
  virtual csm::network::Address_sptr GetSecondaryAddress() const { return nullptr; }

protected:
  // protected function cannot be called from outside because it expects the _map_lock to be hold by the caller
  bool UpdateEPStatus(const csm::network::AddressCode i_NodeID,
                      const csm::network::Address_sptr i_Addr,
                      RUN_MODE::mode_t i_Status);


private:
  uint64_t _daemon_id;
  csm::network::Message _LocalInventory;
  
  ContextMapType _ContextMap;

  double _event_load;
  RunMode *_run_mode;

  TimerContent::TimeType _timestamp;

  bool _RequiresDCGM;
public:
  static volatile uint64_t event_count;
  
protected:
  
  CNStateMapType _NodeStateMap;
  
  mutable std::mutex _map_lock;
  std::map<std::string, double> _PerfDataMap;

  csm::daemon::INV_DCGM_ACCESS *_DcgmAccess;
};

class DaemonStateMaster : public DaemonState
{
  AggregatorSet _aggregators;
public:
  DaemonStateMaster( const uint64_t aDaemonId )
  :DaemonState( aDaemonId ),
   _aggregators()
  { }

  int GetAggregators( csm::daemon::AddressListType &list,
                      const ConnectionType::CONN_TYPE type,
                      const bool i_Connected );

  bool IsNodeConnected( const std::string node );
  std::vector<std::string> GetAggrDisconnectedNodes( const csm::network::Address_sptr downAggrAddr ) const;

  // need to add additional parameter to indicate whether the conn is PRIMARY or SECONDARY
  virtual void AddEP(const csm::network::Address_sptr addr,
                     const ConnectionType::CONN_TYPE type = ConnectionType::SINGLE,
                     const RUN_MODE::mode_t runmode = RUN_MODE::READY_RUNNING,
                     const csm::network::Message *msg = nullptr,
                     const std::string nodeID = "" );
  virtual void DisconnectEP(const csm::network::Address_sptr addr);
  virtual void ConnectEP(const csm::network::Address_sptr addr);

  void UpdateAggregator( const csm::network::Address_sptr aggr,
                         csm::network::Message msg );

  csm::daemon::ComputeActionEntry_t GetNextComputeAction();
  std::vector<csm::network::Address_sptr> GetMulticastAggregators( const std::vector<std::string> computes );
};

class DaemonStateAgg : public DaemonState
{
public:
  DaemonStateAgg( const uint64_t aDaemonId )
  :DaemonState( aDaemonId )
  { }
  
  void AddInventory(const std::string& aNodeUid, const csm::network::MessageAndAddress& content);
  
  // will return nullptr if the addr cannot be found.
  // if address is found, return the message and its current status
  const csm::network::Message* GetInventory(const csm::network::Address_sptr addr,
                                            RUN_MODE::mode_t& status) const;
  
  // to-do: Agg only return the inventory information for the primary set of connections
  // and status is not equal to disconnected
  void GetAllActiveInventory(std::vector<const csm::network::Message *>& invList);
  
  csm::network::Address_sptr GetAddrForCN(const std::string &aNodeUid) const;
  
  // return the node name list that are in disconnected state
  void GetAllDisconnectedEPs(std::vector<csm::daemon::ConnectedNodeStatus_sptr> &list,
                             const ConnectionType::CONN_TYPE type = ConnectionType::ANY);
  

  virtual void InitActiveAddresses( );

  virtual csm::network::Address_sptr GetActiveAddress() const
  {
    return _PrimaryListener;
  }

  csm::daemon::ComputeSet* GetComputeSet() { return &_ComputeList; }
  virtual void DisconnectEP(const csm::network::Address_sptr addr);
  virtual void ConnectEP(const csm::network::Address_sptr addr);

  virtual ~DaemonStateAgg();
private:
  NodeKeywordMapType _NodeKeywordMap;
  csm::network::Address_sptr _PrimaryListener;
  csm::daemon::ComputeSet _ComputeList;
};

class DaemonStateAgent : public DaemonState
{
public:
  DaemonStateAgent( const uint64_t aDaemonId )
  : DaemonState( aDaemonId, true )
  {
    _Aggregators[ 0 ] = nullptr;
    _Aggregators[ 1 ] = nullptr;
  }

  virtual void InitActiveAddresses( );
  virtual csm::network::Address_sptr GetActiveAddress() const
  {
    return _Aggregators[ 0 ];
  }
  virtual csm::network::Address_sptr GetSecondaryAddress() const
  {
    return _Aggregators[ 1 ];
  }
  void SetPrimaryAggregator( const csm::network::Address_sptr primary );

  virtual void AddEP( const csm::network::Address_sptr addr,
                      const ConnectionType::CONN_TYPE type = ConnectionType::SECONDARY,
                      const RUN_MODE::mode_t runmode = RUN_MODE::DISCONNECTED,
                      const csm::network::Message *msg = nullptr,
                      const std::string nodeID = "" );


private:
  csm::network::Address_sptr _Aggregators[2];
};

class DaemonStateUtility : public DaemonState
{
public:
  DaemonStateUtility( const uint64_t aDaemonId )
  :DaemonState( aDaemonId, true )
  {}
};

} //namespace daemon
} //namespace csm

#endif  // CSM_DAEMON_SRC_CSM_DAEMON_STATE_H_
