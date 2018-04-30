/*================================================================================

    csmd/src/daemon/src/csm_daemon_state.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "include/csm_daemon_state.h"

#ifdef logprefix
#undef logprefix
#endif
#define logprefix "DAEMONSTATE"
#include "csm_pretty_log.h"

#include "csm_daemon_config.h"

csm::daemon::DaemonState::~DaemonState()
{
  std::lock_guard<std::mutex> guard( _map_lock );
  _NodeStateMap.clear();

  for( auto &it : _ContextMap )
  {
    it.second->clear();
    delete it.second;
  }
  _ContextMap.clear();
}

csm::network::AddressCode
csm::daemon::DaemonState::GenerateNodeID( const csm::network::Address_sptr i_Addr ) const
{
  csm::network::AddressCode code;
  switch( i_Addr->GetAddrType() )
  {
    case csm::network::CSM_NETWORK_TYPE_PTP:
      // only consider the IP address. ignore the port
      code = (csm::network::AddressCode( std::dynamic_pointer_cast<csm::network::AddressPTP>( i_Addr )->_IP() ) << 2) + (csm::network::AddressCode)i_Addr->GetAddrType();
#if (defined CSM_MULTI_COMPUTE_PER_NODE) || (defined CSM_MULTI_AGGREGATOR_PER_NODE)
      code = (code << 16) + (csm::network::AddressCode)( std::dynamic_pointer_cast<csm::network::AddressPTP>( i_Addr )->_Port() );
#endif
      break;
    case csm::network::CSM_NETWORK_TYPE_UTILITY:
      // only consider the IP address. ignore the port
      code = (csm::network::AddressCode( std::dynamic_pointer_cast<csm::network::AddressUtility>( i_Addr )->_IP() ) << 2) + (csm::network::AddressCode)i_Addr->GetAddrType();
      break;
    case csm::network::CSM_NETWORK_TYPE_AGGREGATOR:
      // only consider the IP address. ignore the port
      code = (csm::network::AddressCode( std::dynamic_pointer_cast<csm::network::AddressAggregator>( i_Addr )->_IP() ) << 2) + (csm::network::AddressCode)i_Addr->GetAddrType();
#ifdef CSM_MULTI_AGGREGATOR_PER_NODE
      code = (code << 16) + (csm::network::AddressCode)( std::dynamic_pointer_cast<csm::network::AddressAggregator>( i_Addr )->_Port() );
#endif
      break;
    default:
      code = i_Addr->MakeKey();
      break;
  }
  CSMLOG( csmd, trace ) << "GenerateNodeID from " << i_Addr->Dump() << " = " << code;
  return code;
}

// note that the default impl of this ignores the connection type
void
csm::daemon::DaemonState::AddEP(const csm::network::Address_sptr addr,
                                const ConnectionType::CONN_TYPE type,
                                const RUN_MODE::mode_t runmode,
                                const csm::network::Message *msg,
                                const std::string nodeID )
{
  //todo: need a strategy here. Now opt not to keep the history of the client connections
  if (addr->GetAddrType() == csm::network::CSM_NETWORK_TYPE_LOCAL) return;
  csm::network::AddressCode key = GenerateNodeID( addr );

  // check if node is already listed - if not, only 're-add' if mode is disconnected

  // create a new CNState with initial values - we check the insert to learn about already existing entries with the same address
  // note: insert does not overwrite any existing value in a map
  csm::daemon::ConnectedNodeStatus CNState;
  CNState._Bounced = 0;
  CNState._ConnectionType = type;
  CNState._LastInventory = ( msg != nullptr ? *msg : csm::network::Message());
  CNState._NeedSendInventory = false;
  CNState._NodeAddr = addr;
  CNState._NodeID = nodeID;
  CNState._NodeMode = RUN_MODE::CONFIGURED;  // set an unused state when adding. the mode will be updated by calling UpdateEPStatus()

  std::lock_guard<std::mutex> guard( _map_lock );
  if( _NodeStateMap.find( key ) != _NodeStateMap.end() )
  {
    CSMLOG( csmd, debug ) << "Already known " << addr->Dump() << " node/daemon " << csm::daemon::RUN_MODE::to_string( _NodeStateMap[ key ]._NodeMode );

    if( _NodeStateMap[ key ]._NodeMode != RUN_MODE::DISCONNECTED )
    {
      CSMLOG( csmd, error ) << "ERROR: Duplicate AddEP to node in READY state. Could be Address Key collision or message drop/duplicate.";
    }
  }
  else
  {
    _NodeStateMap.insert( std::pair<csm::network::AddressCode,
                          csm::daemon::ConnectedNodeStatus>( key,
                                                             CNState ));
  }

  // update all info that might be outdated compared to a previous connection from that node
  _NodeStateMap[ key ]._NodeAddr = addr;
  _NodeStateMap[ key ]._ConnectionType = type;

  if( ! UpdateEPStatus( key, addr, runmode) )
  {
    CSMLOG( csmd, error ) << "Failed to update node status to RUNNING for addr: " << addr->Dump();
  }

  CSMLOG(csmd, debug) << "Adding a node entry " << key
      << " address: " << _NodeStateMap[ key ]._NodeAddr->Dump()
      << " bounced: " << _NodeStateMap[ key ]._Bounced
      << " id: "      << _NodeStateMap[ key ]._NodeID
      << " runmode: " << csm::daemon::RUN_MODE::to_string( _NodeStateMap[ key ]._NodeMode )
      << " state map-size = " << _NodeStateMap.size();
}


void
csm::daemon::DaemonState::SetConnectionTypeEP( const csm::network::Address_sptr addr,
                                               const csm::daemon::ConnectionType::CONN_TYPE type )
{
  if( addr->GetAddrType() == csm::network::CSM_NETWORK_TYPE_PTP )
  {
    csm::network::AddressCode nodeID = GenerateNodeID( addr );

    std::lock_guard<std::mutex> guard( _map_lock );
    auto it = _NodeStateMap.find( nodeID );
    if (it != _NodeStateMap.end())
    {
      CSMLOG( csmd, info ) << "Compute node " << addr->Dump() << " switched to " << type << " connection.";
      it->second._ConnectionType = type;
    }
    else
    {
      CSMLOG(csmd, warning) << "SetConnectionTypeEP(): Cannot find addr:" << addr->Dump();
    }
  }
}

void
csm::daemon::DaemonState::DisconnectEP(const csm::network::Address_sptr addr)
{
  if( addr == nullptr )
  {
    CSMLOG( csmd, warning ) << "Potential bug: DisconnectEP called with nullptr addr.";
    return;
  }
  //todo: need a strategy here. Now opt not to keep the history of the client connections
  if (addr->GetAddrType() == csm::network::CSM_NETWORK_TYPE_LOCAL) return;
  CSMLOG( csmd, info ) << "DISCONNECTEP: " << addr->Dump();

  csm::network::AddressCode nodeID = GenerateNodeID( addr );

  std::lock_guard<std::mutex> guard( _map_lock );
  if( ! UpdateEPStatus(nodeID, addr, RUN_MODE::DISCONNECTED) )
  {
    CSMLOG(csmd, warning) << "UpdateEPStatus(): Cannot find addr:" << addr->Dump();
  }
}

void
csm::daemon::DaemonState::ConnectEP(const csm::network::Address_sptr addr)
{
  if( addr == nullptr )
  {
    CSMLOG( csmd, warning ) << "Potential bug: DisconnectEP called with nullptr addr.";
    return;
  }
  //todo: need a strategy here. Now opt not to keep the history of the client connections
  if( addr->GetAddrType() == csm::network::CSM_NETWORK_TYPE_LOCAL ) return;

  csm::network::AddressCode nodeID = GenerateNodeID( addr );

  std::lock_guard<std::mutex> guard( _map_lock );
  if( ! UpdateEPStatus(nodeID, addr, RUN_MODE::READY_RUNNING ) )
  {
    CSMLOG( csmd, warning ) << "UpdateEPStatus(): Cannot find addr:" << addr->Dump();
  }
}


bool
csm::daemon::DaemonState::UpdateEPStatus(const csm::network::AddressCode i_NodeID,
                                         const csm::network::Address_sptr i_Addr,
                                         const RUN_MODE::mode_t i_Status)
{
  auto it = _NodeStateMap.find( i_NodeID );
  if (it != _NodeStateMap.end())
  {
    if( i_Addr->MakeKey() != it->second._NodeAddr->MakeKey() )
    {
      CSMLOG( csmd, warning ) << "UpdateEPStatus(): Remote src address " << i_Addr->Dump()
        << "does not match, ignoring status update. Counts as a bounce.";
      it->second._Bounced++;
      return false;
    }

    switch( i_Status )
    {
      case RUN_MODE::READY_RUNNING:
        if( it->second._NodeMode == RUN_MODE::DISCONNECTED )
        {
          it->second._NodeMode = i_Status;
          CSMLOG(csmd, debug) << "UpdateEPStatus(): Status of " << i_Addr->Dump()
              << " change to " << RUN_MODE::to_string( i_Status );
          break;
        }

        if( it->second._NodeMode == RUN_MODE::READY_RUNNING )
        {
          CSMLOG( csmd, info ) << "UpdateEPStatus(): Node " << i_Addr->Dump()
              << " already in RUNNING mode. Duplicate up-event? Suggesting to check RAS and actual node status.";
          break;
        }

        CSMLOG( csmd, warning ) << "UpdateEPStatus(): Node " << i_Addr->Dump()
          << " unexpected node state: " << RUN_MODE::to_string( it->second._NodeMode )
          << " when trying to change to RUNNING";
        break;

      case RUN_MODE::DISCONNECTED:
        if( it->second._NodeMode != RUN_MODE::DISCONNECTED )
        {
          if( it->second._NodeMode != RUN_MODE::CONFIGURED )
            it->second._Bounced++;
          it->second._NodeMode = i_Status;
          it->second._NeedSendInventory = false;
          // do not count as a bounce if we're just coming out of configured mode
          CSMLOG(csmd, info) << "UpdateEPStatus(): Status of connection to: " << i_Addr->Dump()
              << " change to " << RUN_MODE::to_string( i_Status );
        }
        else
        {
          CSMLOG( csmd, debug ) << "UpdateEPStatus(): " << i_Addr->Dump()
             << " already in DISCONNECTED mode.";
        }

        break;
      default:
        CSMLOG( csmd, warning ) << "UpdateEPStatus(): unhandled runmode change to " << RUN_MODE::to_string( i_Status )
          << " requested for addr " << i_Addr->Dump();
    }
    return true;
  }
  else
    return false;
}

csm::daemon::ConnectedNodeStatus*
csm::daemon::DaemonState::GetNodeInfo( const csm::network::AddressCode i_AddrCode )
{
  std::lock_guard<std::mutex> guard( _map_lock );
  try
  {
    csm::daemon::ConnectedNodeStatus *info = & _NodeStateMap.at( i_AddrCode );
    if( info != nullptr )
    {
      CSMLOG( csmd, debug ) << "GetNodeInfo(): Retrieved node info: addr=" << info->_NodeAddr->Dump()
          << "; type=" << info->_ConnectionType << "; status=" << csm::daemon::RUN_MODE::to_string( info->_NodeMode );
    }
    else
    {
      CSMLOG( csmd, debug ) << "GetNodeInfo(): Nodeinfo == nullptr";
    }
    return & _NodeStateMap.at( i_AddrCode );
  }
  catch( std::out_of_range &e )
  {
    CSMLOG( csmd, debug ) << "GetNodeInfo(): Nodeinfo not found";
    // do nothing...
  }
  return nullptr;
}


csm::daemon::ConnectedNodeStatus*
csm::daemon::DaemonState::GetNodeInfo( const csm::network::Address_sptr addr )
{
  if( addr->GetAddrType() == csm::network::CSM_NETWORK_TYPE_LOCAL ) return nullptr;

  csm::network::AddressCode key = GenerateNodeID( addr );

  csm::daemon::ConnectedNodeStatus *node = GetNodeInfo( key );

  if( node == nullptr )
    CSMLOG( csmd, debug ) << "NodeStateMap does not contain info about " << addr->Dump();

  return node;
}

void csm::daemon::DaemonState::SetNodeInfo( const csm::network::Address_sptr addr,
                                            const std::string &id,
                                            const csm::network::Message *msg)
{
  csm::network::AddressCode key = GenerateNodeID( addr );

  std::lock_guard<std::mutex> guard( _map_lock );

  auto it = _NodeStateMap.find( key );
  if (it != _NodeStateMap.end())
  {
    _NodeStateMap[ key ]._NodeID = id;
    if( msg != nullptr )
    {
      _NodeStateMap[ key ]._LastInventory = *msg;
    }
    CSMLOG( csmd, debug ) << "SetNodeInfo(): Updating " << addr->Dump() << ". adding Inventory msg and ID=" << id;
  }
}

int
csm::daemon::DaemonState::GetAllEPs(csm::daemon::AddressListType &list,
                                    const ConnectionType::CONN_TYPE type,
                                    const bool i_Connected)
{
  list.clear();
  std::lock_guard<std::mutex> guard( _map_lock );

  for( auto it : _NodeStateMap )
  {
    if( i_Connected )
      if( it.second._NodeMode != RUN_MODE::DISCONNECTED  )
      {
        CSMLOG( csmd, debug ) << "GetAllEPs(): found active: "
            << it.second._ConnectionType << ": " << it.second._NodeAddr->Dump();
        if(( type == it.second._ConnectionType ) || ( type == ConnectionType::ANY ))
          list.push_back( it.second._NodeAddr );
      }
    if( ! i_Connected )
      if( it.second._NodeMode == RUN_MODE::DISCONNECTED  )
      {
        CSMLOG( csmd, debug ) << "GetAllEPs(): found disconnected: "
            << it.second._ConnectionType << ": " << it.second._NodeAddr->Dump();
        if(( type == it.second._ConnectionType ) || ( type == ConnectionType::ANY ))
          list.push_back( it.second._NodeAddr );
      }
  }
  CSMLOG(csmd, debug) << "GetAllEPs(): # of selected EPs = " << list.size();
  return list.size();
}

csm::network::Address_sptr csm::daemon::DaemonState::GetNextHopAddress( const std::string i_NodeName )
{
  // check if it's a directly connected node
  // todo: this is O(n) complexity and should be improved....
  for( auto it : _NodeStateMap )
  {
    if(( it.second._NodeMode == RUN_MODE::READY_RUNNING ) &&
       ( it.second._NodeID == i_NodeName ) &&
       (it.second._ConnectionType != csm::daemon::ConnectionType::SECONDARY )) // temporary filter the secondary address
      return it.second._NodeAddr;
  }

  // todo: check if it needs to go through an aggregator
  return nullptr;
}

void
csm::daemon::DaemonState::RegisterContext(const csm::daemon::SystemContent::SIGNAL_TYPE aSignal,
                                          const csm::daemon::EventContext_sptr aContext)
{
  csm::daemon::ContextListType* list;
  if (_ContextMap.find(aSignal) == _ContextMap.end())
  {
    list = new std::vector<csm::daemon::EventContext_sptr>();
    _ContextMap[aSignal] = list;
  }
  else
    list = _ContextMap[aSignal];

  list->push_back(aContext);

  CSMLOG(csmd, debug) << "RegisterContext: Signal=" << aSignal << " contextList=" << _ContextMap[aSignal]->size();
}

csm::daemon::ContextListType*
csm::daemon::DaemonState::GetContextList(const csm::daemon::SystemContent::SIGNAL_TYPE aSignal) const
{
  if (_ContextMap.find(aSignal) == _ContextMap.end()) return nullptr;
  else return _ContextMap.at( aSignal );
}

bool
csm::daemon::DaemonState::UnregisterContext(const csm::daemon::SystemContent::SIGNAL_TYPE aSignal,
                                            const csm::daemon::EventContext_sptr aContext)
{
  if (_ContextMap.find(aSignal) != _ContextMap.end())
  {
    ContextListType *list = _ContextMap[aSignal];
    for (size_t i=0; list && list->size(); i++)
    {
      if (list->at(i) == aContext)
      {
        list->erase(list->begin()+i);
        CSMLOG(csmd, debug) << "UnregisterContext: Signal=" << aSignal << " contextList=" << _ContextMap[aSignal]->size();
        return true;
      }
    }
  }
  return false;
}

void
csm::daemon::DaemonState::RecordPerfData(std::string i_csmi_api, double i_ms)
{
  std::lock_guard<std::mutex> guard( _map_lock );
  auto it = _PerfDataMap.find(i_csmi_api);
  if (it == _PerfDataMap.end()) _PerfDataMap[i_csmi_api] = i_ms;
  else it->second += i_ms;
}

std::string
csm::daemon::DaemonState::DumpPerfData()
{
  std::stringstream ss;
  _map_lock.lock();
  ss << "Performace Data:\n";
  for (auto elem: _PerfDataMap)
  {
    ss << "CSMAPI=" << elem.first << " TIME=" << elem.second << " ms\n";
  }
  _PerfDataMap.clear();
  _map_lock.unlock();

  return ss.str();
}


std::string
csm::daemon::DaemonState::DumpMapSize()
{
  std::stringstream ss;
  ss << "DaemonState Total Map Entries: ";
  int total_size = _NodeStateMap.size()
      + _PerfDataMap.size();
  ss << total_size << "\n";
  return ss.str();
}

bool
csm::daemon::DaemonState::UpdateEnvironmentalData( const csm::network::Address_sptr addr,
                                                   const CSM_Environmental_Data& data )
{
  csm::network::AddressCode key = GenerateNodeID( addr );

  std::lock_guard<std::mutex> guard( _map_lock );

  if( _NodeStateMap.find( key ) == _NodeStateMap.end() )
  {
    CSMLOG( csmd, warning ) << "UpdateEnvironmentalData: Cannot insert env data for unknown node: " << addr->Dump();
    return false;
  }

  _NodeStateMap[ key ]._EnvData |= data;

  CSMLOG( csmd, debug ) << "UpdateEnvironmentalData: completed update " << addr->Dump();
  //_NodeStateMap[ key ]._EnvData.Print();
  return true;
}

std::string
csm::daemon::DaemonState::GetCNUidFromAddr(const csm::network::Address_sptr addr) const
{
  const csm::network::AddressCode key = GenerateNodeID( addr );

  try
  {
    std::lock_guard<std::mutex> guard( _map_lock );
    auto nodeInfo = _NodeStateMap.at( key );
    CSMLOG( csmd, debug ) << "GetCNUidFromAddr(): addr=" << addr->Dump() << " nodeID=" << nodeInfo._NodeID;
    return nodeInfo._NodeID;
  }
  catch( std::out_of_range &e )
  {
    CSMLOG( csmd, warning ) << "GetCNUidFromAddr(): addr=" << addr->Dump() << " lookup failed. Unknown node?";
    return std::string("");
  }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// DaemonStateMaster

int
csm::daemon::DaemonStateMaster::GetAggregators( csm::daemon::AddressListType &list,
                                                const ConnectionType::CONN_TYPE type,
                                                const bool i_Connected )
{
  list.clear();
  std::lock_guard<std::mutex> guard( _map_lock );

  for( auto it : _NodeStateMap )
  {
    if( it.second._NodeAddr->GetAddrType() != csm::network::AddressType::CSM_NETWORK_TYPE_AGGREGATOR )
      continue;

    if( i_Connected )
      if( it.second._NodeMode != RUN_MODE::DISCONNECTED  )
      {
        CSMLOG( csmd, debug ) << "GetAggregators(): found active: " << it.second._NodeAddr->Dump();
        list.push_back( it.second._NodeAddr );
      }
    if( ! i_Connected )
      if( it.second._NodeMode == RUN_MODE::DISCONNECTED  )
      {
        CSMLOG( csmd, debug ) << "GetAggregators(): found disconnected: " << it.second._NodeAddr->Dump();
        list.push_back( it.second._NodeAddr );
      }
  }
  LOG(csmd, debug) << "GetAggregators(): # of active EPs = " << list.size();
  return list.size();
}

// need to add additional parameter to indicate whether the conn is PRIMARY or SECONDARY
void
csm::daemon::DaemonStateMaster::AddEP(const csm::network::Address_sptr addr,
                                      const ConnectionType::CONN_TYPE type,
                                      const RUN_MODE::mode_t runmode,
                                      const csm::network::Message *msg,
                                      const std::string nodeID )
{
  csm::daemon::DaemonState::AddEP( addr, type, runmode, msg, nodeID );
  if(( addr != nullptr ) && ( addr->GetAddrType() == csm::network::CSM_NETWORK_TYPE_AGGREGATOR ))
    _aggregators.Add( addr, csm::daemon::ComputeSet() );
}

static inline
std::string nodelist_to_string( const csm::daemon::ComputeNodeList_t &list )
{
  std::string ret;
  for( auto it : list )
  {
    ret.append( it );
    ret.append(";");
  }
  return ret;
}

void
csm::daemon::DaemonStateMaster::DisconnectEP(const csm::network::Address_sptr addr)
{
  if(( addr != nullptr ) && ( addr->GetAddrType() == csm::network::CSM_NETWORK_TYPE_AGGREGATOR ))
  {
    _aggregators.Disconnect( addr );
    ComputeNodeList_t disconnected = _aggregators.GetAggrDisconnectedNodes( addr );
    CSMLOG( csmd, info ) << "Aggregator: " << addr->Dump() << " down. No path to: " << disconnected.size()
        << " compute node(s)";
    LOG( csmd, trace ) << "Aggregator: " << addr->Dump() << " down. Disconnected Nodelist: " << nodelist_to_string( disconnected );

  }
  return csm::daemon::DaemonState::DisconnectEP( addr );
}
void
csm::daemon::DaemonStateMaster::ConnectEP(const csm::network::Address_sptr addr)
{
  if(( addr != nullptr ) && ( addr->GetAddrType() == csm::network::CSM_NETWORK_TYPE_AGGREGATOR ))
    _aggregators.Connect( addr );
  return csm::daemon::DaemonState::ConnectEP( addr );
}

bool
csm::daemon::DaemonStateMaster::IsNodeConnected( const std::string node )
{
  return true;
}

std::vector<std::string>
csm::daemon::DaemonStateMaster::GetAggrDisconnectedNodes( const csm::network::Address_sptr downAggrAddr ) const
{
  return _aggregators.GetAggrDisconnectedNodes( downAggrAddr );
}

void
csm::daemon::DaemonStateMaster::UpdateAggregator( const csm::network::Address_sptr aggr,
                                                  csm::network::Message msg )
{
  std::string data = msg.GetData();

  ComputeSet cs;
  csm::daemon::ComputeSet::ConvertDiffToClass( data, cs );

  csm::daemon::ComputeNodeList_t newnodes = cs.GetInsertList();
  csm::daemon::ComputeNodeList_t deadnodes = cs.GetDeleteList();

  _aggregators.Update( aggr, newnodes, deadnodes );
}

csm::daemon::ComputeActionEntry_t
csm::daemon::DaemonStateMaster::GetNextComputeAction()
{
  return _aggregators.GetNextEvent();
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////
// DaemonStateAggregator

void
csm::daemon::DaemonStateAgg::AddInventory(const std::string& aNodeUid,
                                          const csm::network::MessageAndAddress& content)
{
  const csm::network::Message msg = content._Msg;
  const csm::network::Address_sptr addr = content.GetAddr();

  csm::network::AddressCode key = GenerateNodeID( addr );

  std::lock_guard<std::mutex> guard( _map_lock );
  // AddInventory is called from event handlers. With thread pool enabled, we may have multiple
  // threads call this function simultaneously. Need to lock here when we update the map.
  _NodeStateMap[ key ]._LastInventory = msg;
  _NodeStateMap[ key ]._NodeID = aNodeUid;
  _NodeKeywordMap[ aNodeUid ] = key;
  RUN_MODE::mode_t oldmode = _NodeStateMap[ key ]._NodeMode;
//  UpdateEPStatus( key, addr, oldmode );

  CSMLOG( csmd, debug ) << "AddInventory: " << addr->Dump()
      << " nodename=" << aNodeUid << " old runmode=" << RUN_MODE::to_string( oldmode )
      << " setting to " << RUN_MODE::to_string( _NodeStateMap[ key ]._NodeMode );
}

  // will return nullptr if the addr cannot be found.
  // if address is found, return the message and its current status
const csm::network::Message*
csm::daemon::DaemonStateAgg::GetInventory(const csm::network::Address_sptr addr,
                                          RUN_MODE::mode_t& status) const
{
  csm::network::AddressCode key = GenerateNodeID( addr );
  std::lock_guard<std::mutex> guard( _map_lock );
  auto sit = _NodeStateMap.find(key);
  if( sit != _NodeStateMap.end() )
  {
    status = sit->second._NodeMode;
    return( &(sit->second._LastInventory ) );
  }
  else return nullptr;
}

// to-do: Agg only return the inventory information for the primary set of connections
// and status is not equal to disconnected
void
csm::daemon::DaemonStateAgg::GetAllActiveInventory(std::vector<const csm::network::Message *>& invList)
{
  csm::daemon::AddressListType list;
  // the returned EP would not be in DISCONNECTED state
  invList.clear();
  std::lock_guard<std::mutex> guard( _map_lock );
  for( auto & it : _NodeStateMap )
  {
    if(( it.second._NodeMode != RUN_MODE::DISCONNECTED  ) &&
        ( it.second._ConnectionType == ConnectionType::PRIMARY))
    {
      const csm::network::Message *msg = &it.second._LastInventory;
      CSMLOG( csmd, trace ) << "GetAllActiveInventory(): found active primary: " << it.second._NodeAddr->Dump()
          << " InvLen=" << msg->GetDataLen();
      if( msg->GetDataLen() > 0 )
        invList.push_back( msg );
    }
  }
  CSMLOG(csmd, debug) << "GetAllActiveInventory(): # of connected primary nodes= " << invList.size();
}

csm::network::Address_sptr
csm::daemon::DaemonStateAgg::GetAddrForCN(const std::string &aNodeUid) const
{
  std::lock_guard<std::mutex> guard( _map_lock );
  auto it = _NodeKeywordMap.find(aNodeUid);
  if ( it != _NodeKeywordMap.end() )
  {
    auto elem = _NodeStateMap.find(it->second);
    if (elem != _NodeStateMap.end())
      return elem->second._NodeAddr;
  }
  return nullptr;
}

// return the node name list that are in disconnected state
void
csm::daemon::DaemonStateAgg::GetAllDisconnectedEPs(std::vector<csm::daemon::ConnectedNodeStatus_sptr> &list,
                                                   const ConnectionType::CONN_TYPE type )
{
  list.clear();
  std::lock_guard<std::mutex> guard( _map_lock );
  for( auto & it : _NodeStateMap )
  {
    // any node info that matches the type and is either disconnected
    if( ( it.second._NodeMode == RUN_MODE::DISCONNECTED) &&
        (( it.second._ConnectionType == type ) || ( type == ConnectionType::ANY ) ) )
    {
      list.push_back( std::make_shared<csm::daemon::ConnectedNodeStatus>( it.second ) );
    }
  }

  CSMLOG(csmd, debug) << "GetAllDisconnectedEP(): # of " << type << " disconnected EPs = " << list.size();
}

csm::daemon::DaemonStateAgg::~DaemonStateAgg()
{
  _NodeKeywordMap.clear();
  _PerfDataMap.clear();
}

void csm::daemon::DaemonStateAgg::InitActiveAddresses( )
{
  for( auto it : csm::daemon::Configuration::Instance()->GetCriticalConnectionList() )
  {
    if(( it._Addr != nullptr ) && ( it._Addr->GetAddrType() == csm::network::AddressType::CSM_NETWORK_TYPE_PTP ))
    {
      CSMLOG( csmd, info ) << "DaemonState: Setting up " << it._Addr->Dump() << " as primary listener";
      _PrimaryListener = it._Addr;
      break;
    }
  }
}

void
csm::daemon::DaemonStateAgg::DisconnectEP(const csm::network::Address_sptr addr)
{
  if(( addr != nullptr ) && ( addr->GetAddrType() == csm::network::CSM_NETWORK_TYPE_PTP ))
    _ComputeList.DelNode( GetCNUidFromAddr( addr ) );
  csm::daemon::DaemonState::DisconnectEP( addr );
}
void
csm::daemon::DaemonStateAgg::ConnectEP(const csm::network::Address_sptr addr)
{
  csm::daemon::DaemonState::ConnectEP( addr );

  if( addr->GetAddrType() == csm::network::CSM_NETWORK_TYPE_PTP )
    _ComputeList.AddNode( GetCNUidFromAddr( addr ) );
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////
// DaemonStateAgent/Compute

void csm::daemon::DaemonStateAgent::InitActiveAddresses( )
{
  int aggr = 0;
  for( auto it : csm::daemon::Configuration::Instance()->GetCriticalConnectionList() )
  {
    if(( it._Addr != nullptr ) && ( it._Addr->GetAddrType() == csm::network::AddressType::CSM_NETWORK_TYPE_PTP ))
    {
      CSMLOG( csmd, info ) << "Setting up " << it._Addr->Dump() << " as aggregator[" << aggr << "]";
      _Aggregators[ aggr++ ] = it._Addr;
    }
  }
}

void csm::daemon::DaemonStateAgent::ResetPrimary()
{
  csm::daemon::Configuration *config = csm::daemon::Configuration::Instance();
  SetPrimaryAggregator( config->GetConfiguredAggregatorAddress() );
}

void csm::daemon::DaemonStateAgent::SetPrimaryAggregator( const csm::network::Address_sptr primary )
{
  std::lock_guard<std::mutex> guard( _map_lock );

  if( primary == nullptr )
    throw csm::daemon::Exception("BUG: SetPrimaryAggregator attemped with nullptr.");

  CSMLOG( csmd, debug ) << "SetPrimaryAggregator(): current status: primary=" << ( _Aggregators[ 0 ] != nullptr ? _Aggregators[ 0 ]->Dump() : "NULL" )
      << " secondary=" << ( _Aggregators[ 1 ] != nullptr ? _Aggregators[ 1 ]->Dump() : "NULL" );
  if(( _Aggregators[ 1 ] != nullptr )&&( _Aggregators[ 0 ]->MakeKey() != primary->MakeKey() ))
  {
    std::swap( _Aggregators[ 0 ], _Aggregators[ 1 ] );
    CSMLOG( csmd, info ) << "Updated aggregator priority: primary=" << _Aggregators[ 0 ]->Dump() << " secondary=" << _Aggregators[ 1 ]->Dump();
  }
  if( _Aggregators[ 0 ] != nullptr )
    _NodeStateMap[ _Aggregators[ 0 ]->MakeKey() ]._ConnectionType = csm::daemon::ConnectionType::PRIMARY;
  if( _Aggregators[ 1 ] != nullptr )
    _NodeStateMap[ _Aggregators[ 1 ]->MakeKey() ]._ConnectionType = csm::daemon::ConnectionType::SECONDARY;
}

// compute AddEP will force initial state to be secondary + disconnected
void
csm::daemon::DaemonStateAgent::AddEP( const csm::network::Address_sptr addr,
                                      const ConnectionType::CONN_TYPE type,
                                      const RUN_MODE::mode_t runmode,
                                      const csm::network::Message *msg,
                                      const std::string nodeID )
{
  if( addr->GetAddrType() == csm::network::CSM_NETWORK_TYPE_PTP )
  {
    ConnectionType::CONN_TYPE connType = ConnectionType::SECONDARY;
    if( addr->MakeKey() == _Aggregators[ 0 ]->MakeKey() )
      connType = ConnectionType::PRIMARY;
    else
      connType = ConnectionType::SECONDARY;
    CSMLOG( csmd, debug ) << "Initial ADD " << addr->Dump()
        << " as " << connType << " + " << csm::daemon::RUN_MODE::to_string( runmode );
    DaemonState::AddEP( addr, connType, runmode, msg, nodeID );
  }
  else
    DaemonState::AddEP( addr, ConnectionType::SINGLE, runmode, msg, nodeID );
}
