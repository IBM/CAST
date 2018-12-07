/*================================================================================

    csmd/src/daemon/src/connection_handling.cc

  © Copyright IBM Corporation 2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifdef logprefix
#undef logprefix
#endif
#define logprefix "CONN-HDLNG"

#include <algorithm>
#include "csm_pretty_log.h"

#include "csmnet/src/CPP/csm_version_msg.h"
#include "csm_daemon_config.h"
#include "include/connection_handling.h"

csm::daemon::ConnectionHandling::ConnectionHandling( const ConnectionDefinition *i_Prim,
                                                     const ConnectionDefinitionList &i_Dependent,
                                                     const csm::daemon::RunMode *i_RunMode ):
   _RunModeRef(nullptr), _DaemonState(nullptr),_PrimKey(), _ScndKey() 
{
  if( i_Dependent.empty() )
    throw csm::daemon::Exception("Connection handling creation with empty connection list.");

  Initialize( i_Prim, i_Dependent, i_RunMode );
}

csm::daemon::ConnectionHandling::ConnectionHandling( const ConnectionDefinitionList &aDependent,
                                                     const csm::daemon::RunMode *i_RunMode ):
    _RunModeRef(nullptr), _DaemonState(nullptr),_PrimKey(), _ScndKey()
{
  if( aDependent.empty() )
    throw csm::daemon::Exception("Connection handling creation with empty connection list.");

  Initialize( &aDependent[0], aDependent, i_RunMode );
}


csm::daemon::ConnectionHandling::~ConnectionHandling()
{
  CSMLOG( csmd, debug ) << "Cleaning up ";
  _Primary = nullptr;
  _Dependent.clear();
  _EndpointList.Clear();
  csm::network::VersionMsg::Exit();
}

void
csm::daemon::ConnectionHandling::Initialize( const ConnectionDefinition *i_Prim,
                                             const ConnectionDefinitionList &i_Dependent,
                                             const csm::daemon::RunMode *i_RunMode )
{
  _Dependent = i_Dependent;
  _RunModeRef = const_cast<csm::daemon::RunMode*>( i_RunMode );
  _Primary = const_cast<ConnectionDefinition*>(i_Prim);
  _DaemonState = csm::daemon::Configuration::Instance()->GetDaemonState();
  if( _DaemonState == nullptr )
    throw csm::daemon::Exception("Trying to initalize ConnectionHandling without initialized DaemonState." );

  csm::network::VersionMsg::Init( csm::daemon::Configuration::Instance()->GetHostname() );
}


int
csm::daemon::ConnectionHandling::ProcessNonCritical( const csm::daemon::SystemContent::SIGNAL_TYPE i_Signal,
                                                     const csm::network::Address_sptr i_Addr )
{
  int rc = 0;

  CSMLOG( csmd, debug ) << "Processing event: " << i_Signal
    << " for non-critical connection to: " << ( i_Addr != nullptr ? i_Addr->Dump() : "empty" );
  switch( i_Signal )
  {
    case csm::daemon::SystemContent::RETRY_CONNECT:
      break;
    case csm::daemon::SystemContent::CONNECTED:
      _DaemonState->ConnectEP( i_Addr );
      break;

    case csm::daemon::SystemContent::DISCONNECTED:
      _DaemonState->DisconnectEP( i_Addr );
      break;

    case csm::daemon::SystemContent::DAEMONS_START:
    case csm::daemon::SystemContent::RESTARTED:
    case csm::daemon::SystemContent::FAILOVER:
    case csm::daemon::SystemContent::FATAL:
      break;
    default:
      CSMLOG( csmd, debug ) << "Unhandled system event: " << i_Signal
        << " for addr: " << ( i_Addr != nullptr ? i_Addr->Dump() : "empty" );
      rc = -EINVAL;
      break;
  }
  return rc;
}


int
csm::daemon::ConnectionHandling::ProcessSystemEvent( const csm::daemon::SystemContent::SIGNAL_TYPE i_Signal,
                                                     const csm::network::Address_sptr i_Addr,
                                                     const uint64_t i_MsgIdCandidate )
{
  std::lock_guard<std::mutex> guard( _Lock );

  // first check if this address is part of the critical connections.
  // otherwise the event should be ignored
  if( ! IsCritical( i_Addr ) )
    return ProcessNonCritical( i_Signal, i_Addr );

  if( i_Signal != csm::daemon::SystemContent::DAEMONS_START )
  {
    CSMLOG( csmd, debug ) << "Processing system event: " << i_Signal << " for crit.connection: "
      <<  ( i_Addr != nullptr ? i_Addr->Dump() : "empty");
  }
  else
    CSMLOG( csmd, debug ) << "Processing system event: " << i_Signal;

  int rc;
  switch( i_Signal )
  {
    case csm::daemon::SystemContent::DAEMONS_START:
      rc = CreatePrimary(); // no further action here; let the netmgr trigger via CONNECT event
      if( rc != 0 )
        CSMLOG( csmd, debug ) << "Non-zero return from CreatePrimary: rc=" << rc;
      break;

    case csm::daemon::SystemContent::RETRY_CONNECT:
      break;
    case csm::daemon::SystemContent::CONNECTED:
      if(( i_Addr != nullptr ) &&( i_Addr->MakeKey() == _Primary->_Addr->MakeKey() ))
      {
        rc = PrimaryUp();
        if( rc != 0 )
          CSMLOG( csmd, debug ) << "Non-zero return from PrimaryUp: rc=" << rc;
      }
      break;

    case csm::daemon::SystemContent::DISCONNECTED:
      if(( i_Addr != nullptr ) &&( i_Addr->MakeKey() == _Primary->_Addr->MakeKey() ))
        rc = PrimaryDown();
      break;

    case csm::daemon::SystemContent::FATAL:
      PrimaryDown();
      _RunModeRef->Transition( REASON_EXIT, EPIPE );
      break;

    case csm::daemon::SystemContent::RESTARTED:
    case csm::daemon::SystemContent::FAILOVER:
    default:
      CSMLOG( csmd, debug ) << "Unhandled system event: " << i_Signal
        << " for addr: " << ( i_Addr != nullptr ? i_Addr->Dump() : "empty" );
      rc = -EINVAL;
      break;
  }
  return 1;
}

int
csm::daemon::ConnectionHandling::TakeDependentDown( const int i_First )
{
  for( unsigned n=i_First; n<_Dependent.size(); ++n )
  {
    csm::network::Address_sptr addr = _Dependent[ n ]._Addr;
    if( addr == nullptr )
      continue;

    CSMLOG( csmd, info ) << "Removing listening endpoint type:" << addr->GetAddrType()
        << " addr=" << addr->Dump();
    _EndpointList.DeleteEndpoint( addr.get(), "ConnectionHandling::TakeDependentDown");
  }
  return 0;
}


/*
 * Try to restore every dependent connection if it has a configured address but no existing endpoint yet
 * If it causes any exception, don't touch the runmode, just return EAGAIN
 * If it causes a fatal exception, transition to runmode: EXIT and return ENOTCONN
 * Returns EALREADY if all connections were already up (for callers to see whether new connections got created)
 */
int
csm::daemon::ConnectionHandling::TakeDependentUp( const int i_First )
{
  int rc = EALREADY;
  for( unsigned n=i_First; n<_Dependent.size(); ++n )
  {
    ConnectionDefinition *it = &_Dependent[ n ];
    if(( it == nullptr ) || ( it->_Addr == nullptr ))
      continue;

    if( _EndpointList.GetEndpoint( it->_Addr ) == nullptr )
    {
      CSMLOG( csmd, info ) << "Creating/Restoring listening endpoint type:" << it->_Addr->GetAddrType()
          << " addr=" << it->_Addr->Dump();
      try
      {
        csm::network::Endpoint *ep = _EndpointList.NewEndpoint( it->_Addr, it->_Opts );
        if( ep == nullptr )
          rc = ENOTCONN;
        else
          rc = 0;
      }
      catch( csm::network::ExceptionFatal &f )
      {
        CSMLOG( csmd, error ) << "Fatal error while creating endpoint type:" << it->_Addr->GetAddrType()
          << " addr=" << it->_Addr->Dump() << ": " << f.what();
        _RunModeRef->Transition( REASON_EXIT, f.GetErrno() );
        rc = ENOTCONN;
      }
      catch( csm::network::Exception &e )
      {
        CSMLOG( csmd, warning ) << "Unable to restore endpoint type:" << it->_Addr->GetAddrType()
          << " addr=" << it->_Addr->Dump() << ": " << e.what();
        rc = EAGAIN;
      }
    }
  }
  return rc;
}

int
csm::daemon::ConnectionHandling::CheckConnectivity()
{
  std::lock_guard<std::mutex> guard( _Lock );
  return CheckConnectivityNoLock();
}

int
csm::daemon::ConnectionHandling::CheckConnectivityNoLock()
{
  int rc = 0;
  for( auto it : _Dependent )
  {
    if( it._Addr == nullptr )
      continue;

    // check if there's an actual endpoint object allocated for this address
    csm::network::AddressCode addr_key = it._Addr->MakeKey();
    if( _EndpointList.GetEndpoint( addr_key ) == nullptr )
    {
      CSMLOG( csmd, debug ) << " Unavailable critical connection: "  << it._Addr->Dump();
      if( addr_key == _PrimKey)
        rc |= 1;
      else
        rc |= 4;
      continue;
    }

    // don't attempt to look up listening endpoints in DaemonState - they are not tracked there
    if(( it._Opts == nullptr ) || ( it._Opts->_IsServer ))
      continue;

    // look up the status of the connection in DaemonState and see if status is RUNNING
    csm::daemon::ConnectedNodeStatus *info =  _DaemonState->GetNodeInfo( it._Addr );
    if( info != nullptr )
    {
      if( info->_NodeMode != csm::daemon::RUN_MODE::READY_RUNNING )
      {
        CSMLOG( csmd, debug ) << " NodeStatus of "  << it._Addr->Dump() << ": " << info->_NodeMode;
        if( addr_key == _PrimKey )
          rc |= 1;
        else
          rc |= 4;
      }
    }
    else
    {
      CSMLOG( csmd, debug ) << " Unconfirmed connection in DaemonState!" << it._Addr->Dump();
      if( addr_key == _PrimKey )
        rc |= 1;
      else
        rc |= 4;
    }
  }
  CSMLOG( csmd, debug ) << "CheckConnectivity = " << rc;
  return rc;
}

int
csm::daemon::ConnectionHandling::CreateConnection( const ConnectionDefinition *i_Conn,
                                                   const std::string &i_Text )
{
  if(( i_Conn == nullptr ) || ( i_Conn->_Addr == nullptr ))
    return 0;
  try
  {
    if( _EndpointList.GetEndpoint( i_Conn->_Addr ) != nullptr )
    {
      CSMLOG( csmd, debug ) << i_Text << " connection already active. " << i_Conn->_Addr->Dump();
      return EALREADY;   // not an actual error, just for callers to distinguish between existing and new connection
    }

    if( i_Conn->_Opts == nullptr)
      throw csm::daemon::Exception("Trying to create connection without endpoint options. Config failure?");

    csm::daemon::RUN_MODE::mode_t initial_mode = csm::daemon::RUN_MODE::DISCONNECTED;
    if( i_Conn->_Opts->_IsServer )
      initial_mode = csm::daemon::RUN_MODE::READY_RUNNING;
    csm::network::Endpoint *ep = _EndpointList.NewEndpoint( i_Conn->_Addr, i_Conn->_Opts );
    if( ep != nullptr )
    {
      _DaemonState->AddEP( i_Conn->_Addr, i_Conn->_Type, initial_mode, nullptr, i_Conn->_HostName );
      return 0;
    }
    else
    {
      CSMLOG( csmd, warning ) << i_Text << " connection failed for unknown reason. Needs retry. ";
      return ENOTCONN;
    }
  }
  catch( csm::network::Exception &e )
  {
    CSMLOG( csmd, warning ) << i_Text << " connection failed. Needs retry. " << e.what();
    return ENOTCONN;
  }
  return 0;
}






/*
 *  ********************************************************
 *  ******************** M A S T E R ***********************
 *  ********************************************************
 */
csm::daemon::ConnectionHandling_master::ConnectionHandling_master( const ConnectionDefinitionList &i_Dependent,
                                                                   const csm::daemon::RunMode *i_RunMode )
: ConnectionHandling( nullptr, i_Dependent, i_RunMode )
{
}

int
csm::daemon::ConnectionHandling_master::ProcessSystemEvent( const csm::daemon::SystemContent::SIGNAL_TYPE i_Signal,
                                                            const csm::network::Address_sptr i_Addr,
                                                            const uint64_t i_MsgIdCandidate )
{
  std::lock_guard<std::mutex> guard( _Lock );

  // first check if this address is part of the critical connections.
  // otherwise the event should be ignored
  if( ! IsCritical( i_Addr ) )
    return ProcessNonCritical( i_Signal, i_Addr );

  CSMLOG( csmd, debug ) << "Processing system event: " << i_Signal << " for crit.connection: "
      <<  ( i_Addr != nullptr ? i_Addr->Dump() : "empty");

  int rc = 0;
  switch( i_Signal )
  {
    case csm::daemon::SystemContent::DAEMONS_START:
      rc = PrimaryUp();
      if(( rc == 0 ) && ( CheckConnectivityNoLock() == 0 ))
        _RunModeRef->Transition( REASON_CONNECT );
      break;

    case csm::daemon::SystemContent::CONNECTED:
      rc = PrimaryUp();
      if(( rc == 0 ) && ( CheckConnectivityNoLock() == 0 ))
        _RunModeRef->Transition( REASON_CONNECT );
      break;

    case csm::daemon::SystemContent::DISCONNECTED:
      if( CheckConnectivityNoLock() != 0 ) // just test if the disconnect affected any of the critical connections
        _RunModeRef->Transition( REASON_DISCONNECT, 0 );
      break;

    case csm::daemon::SystemContent::FATAL:
      rc = PrimaryDown();
      _RunModeRef->Transition( REASON_EXIT, EPIPE );
      break;

    case csm::daemon::SystemContent::RETRY_CONNECT:
    case csm::daemon::SystemContent::RESTARTED:
    case csm::daemon::SystemContent::FAILOVER:
    default:
      throw csm::daemon::Exception( "BUG: Unexpected system event type. Master should never see this event."+std::to_string( i_Signal ));
      break;
  }
  return 1;
}

int
csm::daemon::ConnectionHandling_master::PrimaryDown()
{
  return TakeDependentDown( 0 );
}
int
csm::daemon::ConnectionHandling_master::PrimaryUp()
{
  return TakeDependentUp();
}










/*
 *  ********************************************************
 *  ******************** U T I L I T Y *********************
 *  ********************************************************
 */
csm::daemon::ConnectionHandling_utility::ConnectionHandling_utility( const ConnectionDefinitionList &i_Dependent,
                                                                     const csm::daemon::RunMode *i_RunMode )
: ConnectionHandling( i_Dependent, i_RunMode )
{
  if(( _Primary == nullptr) || ( _Primary->_Addr == nullptr ) ||
      ( _Primary->_Addr->GetAddrType() != csm::network::AddressType::CSM_NETWORK_TYPE_UTILITY ))
    throw csm::daemon::Exception( "Invalid initialization data for master address." );
}

int
csm::daemon::ConnectionHandling_utility::ProcessSystemEvent( const csm::daemon::SystemContent::SIGNAL_TYPE i_Signal,
                                                             const csm::network::Address_sptr i_Addr,
                                                             const uint64_t i_MsgIdCandidate )
{
  return ConnectionHandling::ProcessSystemEvent( i_Signal, i_Addr );
}

int
csm::daemon::ConnectionHandling_utility::PrimaryDown()
{
  CSMLOG( csmd, debug ) << "PrimaryDown processing. " << _Primary->_Addr->Dump();
  _DaemonState->DisconnectEP( _Primary->_Addr );
  int rc = TakeDependentDown( 1 );
  _RunModeRef->Transition( csm::daemon::run_mode_reason_t::REASON_ERROR );
  return rc;
}

int
csm::daemon::ConnectionHandling_utility::PrimaryUp()
{
  CSMLOG( csmd, debug ) << "PrimaryUp processing. " << _Primary->_Addr->Dump();
  _DaemonState->ConnectEP( _Primary->_Addr );
  int rc = TakeDependentUp();
  if( rc == 0 )
    _RunModeRef->Transition( csm::daemon::run_mode_reason_t::REASON_CONNECT );
  return rc;
}

int
csm::daemon::ConnectionHandling_utility::CreatePrimary()
{
  int rc = CreateConnection( _Primary, "Master" );
  if( rc == EALREADY )
    rc = 0;

  return rc;
}










/*
 *  ********************************************************
 *  *************** A G G R E G A T O R ********************
 *  ********************************************************
 */
csm::daemon::ConnectionHandling_aggregator::ConnectionHandling_aggregator( const ConnectionDefinitionList &i_Dependent,
                                                                           const csm::daemon::RunMode *i_RunMode )
: ConnectionHandling( i_Dependent, i_RunMode )
{
  if(( _Primary == nullptr) || ( _Primary->_Addr == nullptr ) ||
    ( _Primary->_Addr->GetAddrType() != csm::network::AddressType::CSM_NETWORK_TYPE_AGGREGATOR ) )
    throw csm::daemon::Exception( "Invalid initialization data for master address." );

  if( _DaemonState != nullptr )
    dynamic_cast<csm::daemon::DaemonStateAgg*>( _DaemonState )->InitActiveAddresses();
}

int
csm::daemon::ConnectionHandling_aggregator::ProcessSystemEvent( const csm::daemon::SystemContent::SIGNAL_TYPE i_Signal,
                                                                const csm::network::Address_sptr i_Addr,
                                                                const uint64_t i_MsgIdCandidate )
{
  return ConnectionHandling::ProcessSystemEvent( i_Signal, i_Addr );
}

int
csm::daemon::ConnectionHandling_aggregator::PrimaryDown()
{
  CSMLOG( csmd, debug ) << "PrimaryDown processing. " << _Primary->_Addr->Dump();
  _DaemonState->DisconnectEP( _Primary->_Addr );
  // on a disconnect, the master will update the refcount for all known computes of this agg
  // so any updates that couldn't be sent already, will have to be committed/silenced
  dynamic_cast<csm::daemon::DaemonStateAgg*>(_DaemonState)->GetComputeSet()->Commit();
  TakeDependentDown( 1 );

  /*
   * kill all existing connections because this disconnected aggregator is no more functional
   * so all computes should get the opportunity to fail over
   */
  _EndpointList.Clear();

  _RunModeRef->Transition( csm::daemon::run_mode_reason_t::REASON_ERROR );
  return 0;
}


int
csm::daemon::ConnectionHandling_aggregator::PrimaryUp()
{
  CSMLOG( csmd, debug ) << "PrimaryUp processing. " << _Primary->_Addr->Dump();
  _DaemonState->ConnectEP( _Primary->_Addr );
  // on a reconnect to the master, we have had cleared all nodes on the disconnect
  // and we need to make sure that we have an empty set of computes
  dynamic_cast<csm::daemon::DaemonStateAgg*>(_DaemonState)->GetComputeSet()->Clear();
  int rc = TakeDependentUp();
  if( rc == 0 )
    _RunModeRef->Transition( csm::daemon::run_mode_reason_t::REASON_CONNECT );
  return rc;
}

int
csm::daemon::ConnectionHandling_aggregator::CreatePrimary()
{
  int rc = CreateConnection( _Primary, "Master" );
  if( rc == EALREADY )
    rc = 0;

  return rc;
}











/*
 *  ********************************************************
 *  ******************* C O M P U T E **********************
 *  ********************************************************
 */
csm::daemon::ConnectionHandling_compute::ConnectionHandling_compute( const ConnectionDefinitionList &i_Dependent,
                                                                     const csm::daemon::RunMode *i_RunMode )
: ConnectionHandling( i_Dependent, i_RunMode ),
  _Connected( 0 ),
  _MsgIdCandidate( 0 )
{
  if( _Dependent.size() < 2 )
    throw csm::daemon::Exception( "Insufficient number of connections defined for compute daemon operation." );

  _Secondary = &_Dependent[ 1 ];
  _SingleAggregator = ( _Secondary->_Addr == nullptr );

  CSMLOG( csmd, info ) << "Setting up compute-aggregator connections. SingleMode=" << _SingleAggregator;

  if(( _Primary == nullptr ) ||
      ( _Primary->_Addr == nullptr ) ||
      ( _Primary->_Addr->GetAddrType() != csm::network::AddressType::CSM_NETWORK_TYPE_PTP ) )
    throw csm::daemon::Exception( "Invalid initialization data for primary address." );

  if( (!_SingleAggregator) && (( _Secondary->_Addr == nullptr ) ||
      ( _Secondary->_Addr->GetAddrType() != csm::network::AddressType::CSM_NETWORK_TYPE_PTP )) )
    throw csm::daemon::Exception( "Invalid initialization data for secondary address." );

  _PrimKey = _Primary->_Addr->MakeKey();
  if( _SingleAggregator )
    _ScndKey = 0;
  else
    _ScndKey = _Secondary->_Addr->MakeKey();

  if( _DaemonState != nullptr )
  {
    _ComputeDaemonState = dynamic_cast<csm::daemon::DaemonStateAgent*>( _DaemonState );
    if( _ComputeDaemonState == nullptr )
      throw csm::daemon::Exception("BUG: Invalid DaemonState type.");
    _ComputeDaemonState->InitActiveAddresses();
  }
  else
    throw csm::daemon::Exception( "Connection handling impossible with DaemonState=nullptr." );
}

csm::daemon::ConnectionHandling_compute::~ConnectionHandling_compute()
{
  _Secondary = nullptr;
}

int
csm::daemon::ConnectionHandling_compute::ProcessSystemEvent( const csm::daemon::SystemContent::SIGNAL_TYPE i_Signal,
                                                             const csm::network::Address_sptr i_Addr,
                                                             const uint64_t i_MsgIdCandidate )
{
  std::lock_guard<std::mutex> guard( _Lock );

  // first check if this address is part of the critical connections.
  // otherwise the event should be ignored
  if( ! IsCritical( i_Addr ) )
    return ProcessNonCritical( i_Signal, i_Addr );

  CSMLOG( csmd, debug ) << "Processing system event: " << i_Signal << " for crit.connection: "
      <<  ( i_Addr != nullptr ? i_Addr->Dump() : "n/a");

  _MsgIdCandidate = i_MsgIdCandidate;
  int rc = 0;
  switch( i_Signal )
  {
    case csm::daemon::SystemContent::RETRY_CONNECT:
      // only try to create one of the missing connections
      CSMLOG( csmd, debug ) << "RetryConnect with connected=" << _Connected;
      if(( _Connected & csm::daemon::ConnectionHandling_compute::CONN_HDL_SECONDARY ) != 0 )
      {
        rc = CreatePrimary();
        if( rc != 0 )
          CSMLOG( csmd, debug ) << "Non-zero return from CreatePrimary: rc=" << rc;
        break;
      }
      if(( _Connected & csm::daemon::ConnectionHandling_compute::CONN_HDL_PRIMARY ) != 0 )
      {
        rc = CreateSecondary();
        if( rc != 0 )
          CSMLOG( csmd, debug ) << "Non-zero return from CreateSecondary: rc=" << rc;
        break;
      }

      break;
    case csm::daemon::SystemContent::DAEMONS_START:
      rc = CreatePrimary();
      if( rc != 0 )
        CSMLOG( csmd, debug ) << "Non-zero return from CreatePrimary: rc=" << rc;
      break;

    case csm::daemon::SystemContent::CONNECTED:
    {
      if( i_Addr == nullptr )
        break;
      csm::network::AddressCode key = i_Addr->MakeKey();
      if( key == _PrimKey )
      {
        rc = PrimaryUp();
        if( rc != 0 )
          CSMLOG( csmd, debug ) << "Non-zero return from PrimaryUp: rc=" << rc;
      }
      if( key == _ScndKey )
      {
        rc = SecondaryUp();
        if( rc != 0 )
          CSMLOG( csmd, debug ) << "Non-zero return from SecondaryUp: rc=" << rc;
      }
      break;
    }
    case csm::daemon::SystemContent::DISCONNECTED:
    {
      if( i_Addr == nullptr )
        break;
      csm::network::AddressCode key = i_Addr->MakeKey();
      if( key == _PrimKey )
        rc = PrimaryDown();
      if( key == _ScndKey )
        rc = SecondaryDown();
      break;
    }
    case csm::daemon::SystemContent::FATAL:
      rc = PrimaryDown();
      if( ! _SingleAggregator )
        rc += SecondaryDown();

      _RunModeRef->Transition( REASON_EXIT, EPIPE );
      break;

    case csm::daemon::SystemContent::RESTARTED:
    case csm::daemon::SystemContent::FAILOVER:
    default:
      throw csm::daemon::Exception( "BUG: Unexpected system event type. "+std::to_string( i_Signal ));
      break;
  }
  return 1;
}

int
csm::daemon::ConnectionHandling_compute::CreateSecondary()
{
  int rc2 = 0;

  // interrupt the process here, if we have only one aggregator configured
  if( _SingleAggregator )
    return rc2;

  rc2 = CreateConnection( _Secondary, "Secondary" );
  if( rc2 == EALREADY )
    rc2 = 0;
  return rc2;
}

int
csm::daemon::ConnectionHandling_compute::CreatePrimary()
{
  int rc1 = 0;

  rc1 = CreateConnection( _Primary, "Primary" );
  if( rc1 == EALREADY )
    rc1 = 0;

  // if the primary got created without error, don't attempt to create the secondary yet
  // the secondary should only be created when the primary is either failing or up already
  // to prevent race between the 2 connections: the winner would become the primary connection
  // which is especially not wanted during start of daemon

  if( rc1 != 0 )
    return CreateSecondary();

  return rc1;
}


int
csm::daemon::ConnectionHandling_compute::UpTransitionAndFailover( const csm::network::Address_sptr i_Addr,
                                                                const bool i_Failover )
{
  CSMLOG( csmd, debug ) << "change in connection state of " << i_Addr->Dump() << ". Transition and failover=" << i_Failover;
  // in case we detected a failover: set aggregator and send msg
  // queue that msg before changing runmode to make sure it's the first thing the netmgr does after resuming action
  if( i_Failover )
  {
    QueueFailoverMsg( i_Addr );
    _ComputeDaemonState->SetPrimaryAggregator( i_Addr );
  }

  // with single aggregators, do a full mode transition to running
  if( _SingleAggregator )
    _RunModeRef->Transition( REASON_TRANSITION );
  else if( _Connected != CONN_HDL_NONE )
    _RunModeRef->Transition( REASON_CONNECT );
  else
    throw csm::daemon::Exception("BUG: Called PrimaryUp() without any of primary/secondary alive." );

  return 0;
}


int
csm::daemon::ConnectionHandling_compute::PrimaryUp()
{
  int rc = 0;

  // store old value for comparison (this function has to be reentrant)
  int old_connect = _Connected;
  _Connected |= CONN_HDL_PRIMARY;

  _DaemonState->ConnectEP( _Primary->_Addr );

  bool failover = false;
  // only if the current mode is disconnected, we need to attempt to bring up the secondary connections
  if( _RunModeRef->Get() == csm::daemon::RUN_MODE::DISCONNECTED )
  {
    // if we're returning from disconnected mode, we need to send a failover msg
    failover = true;

    CSMLOG( csmd, info ) << "Configured Primary " << _Primary->_Addr->Dump()
      << " is up. Creating secondary and dependent listeners. Failover=" << failover;
    CreateSecondary();  // now try to bring up the secondary too
    rc = TakeDependentUp( 2 );
  }

  // see if we have to do any mode transitions or failovers at all (only do that if connections are new)
  if( old_connect == _Connected )
    return rc;

  return UpTransitionAndFailover( _Primary->_Addr, failover );
}

int
csm::daemon::ConnectionHandling_compute::SecondaryUp()
{
  if( _SingleAggregator )
    throw csm::daemon::Exception("BUG: Calling SecondaryUp in single-aggregator setup." );

  int rc = 0;

  // store old value for comparison (this function has to be reentrant)
  int old_connect = _Connected;
  _Connected |= CONN_HDL_SECONDARY;

  _DaemonState->ConnectEP( _Secondary->_Addr );

  bool failover = false;
  // only if the current mode is disconnected, we need to attempt to bring up the secondary connections
  if( _RunModeRef->Get() == csm::daemon::RUN_MODE::DISCONNECTED )
  {
    // if we're returning from disconnected mode, we need to send a failover msg
    failover = true;

    CSMLOG( csmd, info ) << "Configured Secondary " << _Secondary->_Addr->Dump()
      << " is up. Creating primary and dependent listeners. Failover=" << failover;
    CreatePrimary();
    rc = TakeDependentUp( 2 );
  }

  // see if we have to do any mode transitions or failovers at all (only do that if connections are new)
  if( old_connect == _Connected )
  {
    CSMLOG( csmd, debug ) << "No change in connection state, skipping further action.";
    return rc;
  }

  return UpTransitionAndFailover( _Secondary->_Addr, failover );
}


int
csm::daemon::ConnectionHandling_compute::PrimaryDown()
{
  int rc = 0;
  int old_connect = _Connected;
  CSMLOG( csmd, debug ) << "PrimaryDown(): _connected=" << _Connected;
  _Connected &= CONN_HDL_SECONDARY; // remove the first conn

  // do we need to send a failover msg?
  csm::network::Address_sptr addr = _DaemonState->GetActiveAddress();
  bool failover = ((!_SingleAggregator) && ((addr == nullptr ) || ( addr->MakeKey() == _PrimKey )) );
  if( failover )
    _ComputeDaemonState->SetPrimaryAggregator( _Secondary->_Addr );

  _DaemonState->DisconnectEP( _Primary->_Addr );

  // see if we have to do any mode transitions at all (only do that if connections have changed)
  if( old_connect == _Connected )
    return rc;

  _RunModeRef->Transition( REASON_DISCONNECT );
  // do a second transition if this is a single-aggregator setup
  if( _SingleAggregator )
    _RunModeRef->Transition( REASON_DISCONNECT );


  // if this was the last redundant connection, shut down the dependent listeners too
  if( _RunModeRef->Get() == csm::daemon::RUN_MODE::DISCONNECTED )
  {
    CSMLOG( csmd, info ) << "All aggregator connections down. Removing dependent listeners.";
    rc = TakeDependentDown( 2 ); // don't attempt to shut down secondary connection
  }
  else
  {
    if( failover ) QueueFailoverMsg( _Secondary->_Addr );
  }
  return rc;
}

int
csm::daemon::ConnectionHandling_compute::SecondaryDown()
{
  if( _SingleAggregator )
    throw csm::daemon::Exception("BUG: Calling SecondaryDown in single-aggregator setup." );

  CSMLOG( csmd, debug ) << "SecondaryDown(): _connected=" << _Connected;
  int rc = 0;
  int old_connect = _Connected;
  _Connected &= CONN_HDL_PRIMARY; // remove the second conn

  // do we need to send a failover msg?
  csm::network::Address_sptr addr = _DaemonState->GetActiveAddress();
  bool failover = ((!_SingleAggregator) && ((addr == nullptr ) || ( addr->MakeKey() == _ScndKey )) );
  if( failover )
    _ComputeDaemonState->SetPrimaryAggregator( _Primary->_Addr );

  _DaemonState->DisconnectEP( _Secondary->_Addr );

  // make the call reentrant: no transition if no connection change
  if( old_connect != _Connected )
    _RunModeRef->Transition( REASON_DISCONNECT );

  if( _RunModeRef->Get() == csm::daemon::RUN_MODE::DISCONNECTED )
  {
    CSMLOG( csmd, info ) << "Both aggregator connections down. Removing dependent listeners.";
    rc = TakeDependentDown( 2 ); // don't attempt to shut down secondary connection
  }
  else
  {
    if( failover ) QueueFailoverMsg( _Primary->_Addr );
  }
  return rc;
}

int
csm::daemon::ConnectionHandling_compute::CheckConnectivityNoLock()
{
  int rc = 0;
  CSMLOG( csmd, debug ) << "Compute: Checking connectivity";
  for( auto it : _Dependent )
  {
    if( it._Addr == nullptr )
      continue;

    // check if there's an actual endpoint object allocated for this address
    csm::network::AddressCode addr_key = it._Addr->MakeKey();
    if( _EndpointList.GetEndpoint( addr_key ) == nullptr )
    {
      CSMLOG( csmd, debug ) << " Unavailable critical connection: "  << it._Addr->Dump();
      if( addr_key == _PrimKey )
        rc |= 1;
      else if( addr_key == _ScndKey )
        rc |= 2;
      else
        rc |= 4;
      continue;
    }

    // don't attempt to look up listening endpoints in DaemonState - they are not tracked there
    if(( it._Opts == nullptr ) || ( it._Opts->_IsServer ))
      continue;

    // look up the status of the connection in DaemonState and see if status is RUNNING
    csm::daemon::ConnectedNodeStatus *info =  _DaemonState->GetNodeInfo( it._Addr );
    if( info != nullptr )
    {
      if( info->_NodeMode != csm::daemon::RUN_MODE::READY_RUNNING )
      {
        CSMLOG( csmd, debug ) << " NodeStatus of "  << it._Addr->Dump() << ": " << info->_NodeMode;
        if( addr_key == _PrimKey )
          rc |= 1;
        else if( addr_key == _ScndKey )
          rc |= 2;
        else
          rc |= 4;
      }
    }
    else
    {
      CSMLOG( csmd, debug ) << " Unconfirmed connection in DaemonState!" << it._Addr->Dump();
      if( addr_key == _PrimKey )
        rc |= 1;
      else if( addr_key == _ScndKey )
        rc |= 2;
      else
        rc |= 4;
    }
  }
  CSMLOG( csmd, debug ) << "CheckConnectivity = " << rc;
  return rc;
}

void csm::daemon::ConnectionHandling_compute::ResetPrimary()
{
  if( _SingleAggregator )
    return;

  if( _Connected == CONN_HDL_MASK_BOTH )
  {
    csm::network::Address_sptr addr = _DaemonState->GetActiveAddress();
    if( addr->MakeKey() == _ScndKey )
    {
      CSMLOG( csmd, debug ) << "ResetPrimary() Resetting Primary agg to configured: " << _Primary->_Addr->Dump();
      QueueResetMsg( _Secondary->_Addr );
      QueueFailoverMsg( _Primary->_Addr );
      _ComputeDaemonState->SetPrimaryAggregator( _Primary->_Addr );
    }
    else
      CSMLOG( csmd, debug ) << "ResetPrimary() no action necessary. Primary agg already matches configured: " << _Primary->_Addr->Dump();
  }
}


void csm::daemon::ConnectionHandling_compute::QueueFailoverMsg( csm::network::Address_sptr addr )
{
  if( addr == nullptr )
    throw csm::daemon::Exception( "Trying failover to nullptr address." );

  CSMLOG( csmd, debug ) << "Creating FAILOVER message for Aggregator: " << addr->Dump() << " to signal primary address.";
  csm::network::MessageAndAddress evData;
  evData.SetAddr( addr );
  evData._Msg.Init( CSM_CMD_CONNECTION_CTRL,
                    CSM_HEADER_INT_BIT,
                    CSM_PRIORITY_DEFAULT,
                    _MsgIdCandidate,
                    0x0, 0x0,
                    geteuid(), getegid(),
                    std::string( CSM_FAILOVER_MSG ));
  _EndpointList.SendTo( evData );
}


void csm::daemon::ConnectionHandling_compute::QueueResetMsg( csm::network::Address_sptr addr )
{
  if( addr == nullptr )
    throw csm::daemon::Exception( "Trying reset msg to nullptr address." );

  CSMLOG( csmd, debug ) << "Creating RESET message for Aggregator: " << addr->Dump() << " to signal secondary address.";
  csm::network::MessageAndAddress evData;
  evData.SetAddr( addr );
  evData._Msg.Init( CSM_CMD_CONNECTION_CTRL,
                    CSM_HEADER_INT_BIT,
                    CSM_PRIORITY_DEFAULT,
                    _MsgIdCandidate,
                    0x0, 0x0,
                    geteuid(), getegid(),
                    std::string( CSM_RESET_MSG ));
  _EndpointList.SendTo( evData );
}
