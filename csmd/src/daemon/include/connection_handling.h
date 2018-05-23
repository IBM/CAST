/*================================================================================

    csmd/src/daemon/include/connection_handling.h

  © Copyright IBM Corporation 2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_CONNECTION_HANDLING_H_
#define CSMD_SRC_DAEMON_INCLUDE_CONNECTION_HANDLING_H_

#include <mutex>

#include "csmnet/src/CPP/csm_network_exception.h"
#include "csmnet/src/CPP/address.h"
#include "csmnet/src/CPP/reliable_msg.h"

#include "include/csm_connection_definition.h"
#include "include/run_mode.h"
#include "include/csm_daemon_state.h"


namespace csm {
namespace daemon {


class ConnectionHandling
{
protected:
  ConnectionDefinition *_Primary;       // main connection that determines connect/disconnect mode
  ConnectionDefinitionList _Dependent; // dependent connections that don't require mode changes
  csm::network::ReliableMsg _EndpointList;
  csm::daemon::RunMode *_RunModeRef;
  csm::daemon::DaemonState *_DaemonState;
  csm::network::AddressCode _PrimKey;
  csm::network::AddressCode _ScndKey;
  std::mutex _Lock;

public:
  ConnectionHandling( const ConnectionDefinitionList &i_Dependent,
                      const csm::daemon::RunMode *i_RunMode );
  ConnectionHandling( const ConnectionDefinition *i_Prim,
                      const ConnectionDefinitionList &i_Dependent,
                      const csm::daemon::RunMode *i_RunMode );

  virtual ~ConnectionHandling();

  // processing of system events that affect the critical connections
  // will return < 0 on error
  // will return 0 if no action was taken
  // will return >0 if action was taken
  virtual int ProcessSystemEvent( const csm::daemon::SystemContent::SIGNAL_TYPE i_Signal,
                                  const csm::network::Address_sptr i_Addr,
                                  const uint64_t i_MsgIdCandidate = 0 ) = 0;

  // process system events for non-critital connections
  // mostly boils down to connect/disconnect in the tracking
  int ProcessNonCritical( const csm::daemon::SystemContent::SIGNAL_TYPE i_Signal,
                          const csm::network::Address_sptr i_Addr );

  csm::network::ReliableMsg* GetEndpointList() { return &_EndpointList; }

  // checks the status of the configured endpoints
  // returns a bitmap indicating with types of connections are missing
  //  1 - primary
  //  2 - secondary (if applicable)
  //  4 - any dependent
  // meant to get called externally - it locks the structures
  virtual int CheckConnectivity();

protected:
  // meant to get called internally only since it's not locking structures
  virtual int CheckConnectivityNoLock();

  // attempt to create the primary connection(s) before the dependent ones
  // needs to be re-entrant
  virtual int CreatePrimary() = 0;

  // actions to take if the primary connection goes down
  virtual int PrimaryDown() = 0;

  // actions to take if the primary connection comes back up
  virtual int PrimaryUp() = 0;

  // iterate through dependent list and take them down
  // start at the [first]
  // needs to be re-entrant
  int TakeDependentDown( const int i_First = 0 );

  // iterate through dependent list and bring them back up
  //  (all that are down now)
  // needs to be re-entrant
  int TakeDependentUp( const int i_First = 0 );

  // check whether the input address is an entry in the critical list
  // a nullptr addr is considered a critical connection (e.g. no address given during DAEMON_START event)
  inline bool IsCritical( const csm::network::Address_sptr i_Addr )
  {
    if( i_Addr == nullptr )
      return true;
    for( auto it : _Dependent )
    {
      if( it._Addr == nullptr )
        continue;
      if( it._Addr->MakeKey() == i_Addr->MakeKey() )
        return true;
    }
    return false;
  }

  // central function to create a connection
  // will create a connection based on the connection definition
  // if successful it adds it to daemonstate
  // returns 0 on success,  ENOTCONN otherwise
  int CreateConnection( const ConnectionDefinition *i_Conn,
                        const std::string &i_Text );
private:
  void Initialize( const ConnectionDefinition *i_Prim,
                   const ConnectionDefinitionList &i_Dependent,
                   const csm::daemon::RunMode *i_RunMode );

};


/*
 * Master has no primary connection that if it fails would require all others to take down
 * all endpoints are passive listeners with equal priority, independent of each other
 */
class ConnectionHandling_master : public ConnectionHandling
{
public:
  ConnectionHandling_master( const ConnectionDefinitionList &i_Dependent,
                             const csm::daemon::RunMode *i_RunMode );
  virtual ~ConnectionHandling_master() {}

  virtual int ProcessSystemEvent( const csm::daemon::SystemContent::SIGNAL_TYPE i_Signal,
                                  const csm::network::Address_sptr i_Addr,
                                  const uint64_t i_MsgIdCandidate = 0 );

protected:
  virtual int PrimaryDown();
  virtual int PrimaryUp();
  virtual int CreatePrimary() { return 0; } // nothing to do on master (it has no primary connection)
};




/*
 * Utility has primary connection to master
 *  + 1 local listener as dependent connections
 * If the master connection goes down:
 *  - the dependent connections will be shut down too and the daemon changes to disconnected state
 */
class ConnectionHandling_utility: public ConnectionHandling
{
public:
  ConnectionHandling_utility( const ConnectionDefinitionList &i_Dependent,
                                 const csm::daemon::RunMode *i_RunMode );
  virtual ~ConnectionHandling_utility() {}

  virtual int ProcessSystemEvent( const csm::daemon::SystemContent::SIGNAL_TYPE i_Signal,
                                  const csm::network::Address_sptr i_Addr,
                                  const uint64_t i_MsgIdCandidate = 0 );
protected:
  virtual int PrimaryDown();
  virtual int PrimaryUp();
  virtual int CreatePrimary();
};





/*
 * Aggregator has primary connection to master
 *  + 1 compute listener + 1 local listener as dependent connections
 * If the master connection goes down:
 *  - the dependent connections will be shut down too and the daemon changes to disconnected state
 */
class ConnectionHandling_aggregator : public ConnectionHandling
{
public:
  ConnectionHandling_aggregator( const ConnectionDefinitionList &i_Dependent,
                                 const csm::daemon::RunMode *i_RunMode );
  virtual ~ConnectionHandling_aggregator() {}

  virtual int ProcessSystemEvent( const csm::daemon::SystemContent::SIGNAL_TYPE i_Signal,
                                  const csm::network::Address_sptr i_Addr,
                                  const uint64_t i_MsgIdCandidate = 0 );
protected:
  virtual int PrimaryDown();
  virtual int PrimaryUp();
  virtual int CreatePrimary();
};


/*
 * Compute has redundant primary connection to aggregator
 *  + 1 dependent local listener
 * If one aggregator connection goes down, it attempts to failover to the other (depending on current state)
 */
class ConnectionHandling_compute : public ConnectionHandling
{
  enum {
    CONN_HDL_NONE = 0x0,
    CONN_HDL_PRIMARY = 0x1,
    CONN_HDL_SECONDARY = 0x2,
    CONN_HDL_MASK_BOTH = 0x3
  };

  DaemonStateAgent *_ComputeDaemonState;
  ConnectionDefinition *_Secondary;
  bool _SingleAggregator;
  int _Connected; // bit 0 and 1 indicate which (prim/scnd) connections are online
  volatile uint64_t _MsgIdCandidate;

public:
  ConnectionHandling_compute( const ConnectionDefinitionList &i_Dependent,
                              const csm::daemon::RunMode *i_RunMode );
  virtual ~ConnectionHandling_compute();

  virtual int ProcessSystemEvent( const csm::daemon::SystemContent::SIGNAL_TYPE i_Signal,
                                  const csm::network::Address_sptr i_Addr,
                                  const uint64_t i_MsgIdCandidate = 0 );

  void ResetPrimary();

protected:
  virtual int CheckConnectivityNoLock();
  virtual int PrimaryDown();
  virtual int PrimaryUp();
  int SecondaryDown();
  int SecondaryUp();
  virtual int CreatePrimary();
  void QueueFailoverMsg( csm::network::Address_sptr addr );
  void QueueResetMsg( csm::network::Address_sptr addr );
  int UpTransitionAndFailover( const csm::network::Address_sptr i_Addr,
                               const bool i_Failover );
private:
  int CreateSecondary();

};


}  // daemon
} // csm

#endif /* CSMD_SRC_DAEMON_INCLUDE_CONNECTION_HANDLING_H_ */
