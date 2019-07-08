/*================================================================================

    csmd/src/daemon/include/csm_healthcheck_data.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_CSM_HEALTHCHECK_DATA_H_
#define CSMD_SRC_DAEMON_INCLUDE_CSM_HEALTHCHECK_DATA_H_

#include <string>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include "csm_daemon_role.h"
#include "csm_connection_type.h"

class HealthNodeInfo
{
protected:
  std::string _type;
  std::string _id;
  std::string _version;
  uint64_t _daemonID;
  unsigned _bounced;
  bool _responding;

public:
  HealthNodeInfo( const std::string i_Type = "NOTYPE",
                  const std::string i_ID = "NOID",
                  const std::string i_Version = "UNKNOWN",
                  const unsigned i_Bounced = 0,
                  const bool i_Responding = false )
  : _type( i_Type ),
    _id( i_ID ),
    _version( i_Version ),
    _daemonID( 0 ),
    _bounced( i_Bounced ),
    _responding( i_Responding )
  {
  }

  HealthNodeInfo( const HealthNodeInfo & i_In )
  : _type( i_In._type ),
    _id( i_In._id ),
    _version( i_In._version ),
    _daemonID( i_In._daemonID ),
    _bounced( i_In._bounced ),
    _responding( i_In._responding )
  {
  }

private:
  friend class boost::serialization::access;

  template <class Archive>
  void serialize(Archive &ar, const unsigned int version)
  {
    ar & _type;
    ar & _id;
    ar & _version;
    ar & _daemonID;
    ar & _bounced;
    ar & _responding;
  }

public:
  std::string Dump(bool verbose_option) const
  {
    std::stringstream ss;
    ss << _type << ": " << _id;
    if( verbose_option )
      ss << " (bounced=" << _bounced << "; version=" << _version << ")";

    return ss.str();
  }
  void SetID( const std::string i_ID )
  {
    _id = i_ID;
  }
  inline std::string GetNodeType() const { return _type; }
  inline std::string GetID() const { return _id; }
  inline std::string GetVersion() const { return _version; }
  inline uint64_t GetDaemonID() const { return _daemonID; }
  inline unsigned GetBounced() const { return _bounced; }
  inline bool IsResponding() const { return _responding; }
  inline void SetBounced( const unsigned i_Bounced ) { _bounced = i_Bounced; }
  inline void SetDaemonID( const uint64_t i_DaemonID ) { _daemonID = i_DaemonID; }
};


class ComputeInfo : public HealthNodeInfo
{
public:
  ComputeInfo( const std::string i_ID = "NOID",
               const std::string i_Version = "UNKNOWN",
               const unsigned i_Bounced = 0,
               const bool i_Responding = true,
               const csm::daemon::ConnectionType::CONN_TYPE i_CType = csm::daemon::ConnectionType::PRIMARY )
  : HealthNodeInfo( CSMDaemonRole_to_string( CSM_DAEMON_ROLE_AGENT ),
                    i_ID,
                    i_Version,
                    i_Bounced,
                    i_Responding ),
    _ConnectionType( (int)i_CType )
  {}
  ComputeInfo( const ComputeInfo &i_In )
  : HealthNodeInfo( dynamic_cast<const HealthNodeInfo&>( i_In )),
    _ConnectionType( i_In._ConnectionType )
  {}
  ComputeInfo( const HealthNodeInfo &i_LocalData )
  : HealthNodeInfo( i_LocalData ),
    _ConnectionType( csm::daemon::ConnectionType::CONN_TYPE::PRIMARY )
  {
  }

  csm::daemon::ConnectionType::CONN_TYPE GetConnectionType() const
  {
    return (csm::daemon::ConnectionType::CONN_TYPE)_ConnectionType;
  }
  void SetConnectionType( const csm::daemon::ConnectionType::CONN_TYPE i_CType )
  {
    _ConnectionType = (int)i_CType;
  }

  std::string Dump(bool verbose_option) const
  {
    std::stringstream ss;
    ss << _type << ": " << _id;
    if( verbose_option )
      ss << " (bounced=" << _bounced
        << "; version=" << _version
        << "; link=" << (csm::daemon::ConnectionType::CONN_TYPE)_ConnectionType << ")";

    return ss.str();
  }
private:
  friend class boost::serialization::access;
  int _ConnectionType; // forced to int to allow serialization because CONN_TYPE operator<< generates a string for logging

  template <class Archive>
  void serialize(Archive &ar, const unsigned int version)
  {
    ar & (*dynamic_cast<HealthNodeInfo*>(this));
    ar & _ConnectionType;
  }
};
// note that we use boost::shared:ptr because of boost serialization...
typedef boost::shared_ptr<ComputeInfo> ComputeInfo_sptr;


class AggInfo : public HealthNodeInfo
{
  friend class HealthCheckData;
  friend class CSM_INFRASTRUCTURE_TEST_MASTER;
  friend class CSM_INFRASTRUCTURE_TEST_AGG;
  friend class CSM_INFRASTRUCTURE_TEST_AGENT;

public:
  AggInfo( const std::string i_ID = "NOID",
           const std::string i_Version = "UNKNOWN",
           const unsigned i_Bounced = 0,
           const bool i_Responding = true )
  : HealthNodeInfo(CSMDaemonRole_to_string( CSM_DAEMON_ROLE_AGGREGATOR ),
                   i_ID,
                   i_Version,
                   i_Bounced,
                   i_Responding )
  {
  }

  AggInfo( const HealthNodeInfo &i_LocalData )
  : HealthNodeInfo( i_LocalData )
  {
  }
private:
  friend class boost::serialization::access;

  template <class Archive>
  void serialize(Archive &ar, const unsigned int version)
  {
    ar & (*dynamic_cast<HealthNodeInfo*>(this));

    ar & _PrimaryComputes;
    ar & _SecondaryComputes;
  }

public:
  std::string Dump(bool verbose_option) const
  {
    // Do some preprocessing before dumping the output

    std::vector<std::string> active_list;
    std::vector<std::string> disconnected_list;
    std::vector<std::string> active_list2;
    std::vector<std::string> disconnected_list2;
    for ( auto it : _PrimaryComputes )
    {
      std::string name = it->Dump( verbose_option );
      if( it->IsResponding() )
        active_list.push_back( name );
      else
        disconnected_list.push_back( name );
    }
    for ( auto it : _SecondaryComputes )
    {
      std::string name = it->Dump( verbose_option );
      if( it->IsResponding() )
        active_list2.push_back( name );
      else
        disconnected_list2.push_back( name );
    }

    std::stringstream ss;
    const HealthNodeInfo *hni = dynamic_cast<const HealthNodeInfo*>(this);
    if( hni != NULL )
      ss << hni->Dump( verbose_option );

    ss << "\n\tActive_primary: " << active_list.size() << "\n";
    ss << "\tUnresponsive_primary: " << disconnected_list.size() << "\n";
    ss << "\tActive_secondary: " << active_list2.size() << "\n";
    ss << "\tUnresponsive_secondary: " << disconnected_list2.size() << "\n";

    if (verbose_option )
    {
      ss << "\n\tPrimary Nodes:" << "\n";
      ss << "\t\tActive: " << active_list.size() << "\n";

      for( auto node : active_list )
        ss << "\t\t\t" << node << "\n";

      ss << "\t\tUnresponsive: " << disconnected_list.size() << "\n";
      for( auto node : disconnected_list )
        ss << "\t\t\t" << node << "\n";

      ss << "\n\tSecondary Nodes:" << "\n";
      ss << "\t\tActive: " << active_list2.size() << "\n";

      for( auto node : active_list2 )
        ss << "\t\t\t" << node << "\n";

      ss << "\t\tUnresponsive: " << disconnected_list2.size() << "\n";
      for( auto node : disconnected_list2 )
        ss << "\t\t\t" << node << "\n";

    }

    return ss.str();
  }

private:
  std::vector<ComputeInfo_sptr> _PrimaryComputes;
  std::vector<ComputeInfo_sptr> _SecondaryComputes;
};

typedef boost::shared_ptr<AggInfo> AggInfo_sptr;

class UtilInfo : public HealthNodeInfo
{
public:
  UtilInfo( const std::string i_ID = "NOID",
            const std::string i_Version = "UNKNOWN",
            const unsigned i_Bounced = 0,
            const bool i_Responding = true )
  : HealthNodeInfo( CSMDaemonRole_to_string( CSM_DAEMON_ROLE_UTILITY ),
                    i_ID,
                    i_Version,
                    i_Bounced,
                    i_Responding )
  {
  }
  UtilInfo( const HealthNodeInfo &i_LocalData )
  : HealthNodeInfo( i_LocalData )
  {
  }
private:
  friend class boost::serialization::access;

  template <class Archive>
  void serialize(Archive &ar, const unsigned int version)
  {
    const HealthNodeInfo *hni = dynamic_cast<const HealthNodeInfo*>(this);
    if( hni != NULL )
      ar & (*dynamic_cast<HealthNodeInfo*>(this));
  }

public:
  std::string Dump(bool verbose_option) const
  {
    std::stringstream ss;
    const HealthNodeInfo *hni = dynamic_cast<const HealthNodeInfo*>(this);
    if( hni == NULL )
      return ss.str();
    ss << hni->Dump( verbose_option );
    return ss.str();
  }
};
typedef boost::shared_ptr<UtilInfo> UtilInfo_sptr;

class HealthCheckData
{
  friend class EventContextTest;
  friend class CSM_INFRASTRUCTURE_TEST;
  friend class CSM_INFRASTRUCTURE_TEST_MASTER;
  friend class CSM_INFRASTRUCTURE_TEST_AGG;
  friend class CSM_INFRASTRUCTURE_TEST_AGENT;
  friend class CSM_INFRASTRUCTURE_TEST_UTILITY;

public:
  HealthCheckData()
  {
    _verbose_option = false;

    _db_pool_size = 0;
    _db_free_pool_size = 0;
    _db_locked_pool_size = 0;
    _timer_test = false;
    _db_query_test = false;
    _mtc_test = false;
    _net_vchannel_test = false;
    _user_access_test = false;
    _unique_daemon_id_test = true;
    _errmsg = "";

#ifdef WITH_MASTER_LOAD_STATS
    _event_load = 0.0;
#endif
  }

  int GetActiveAgents(std::vector<std::string>& in_out_list) const
  {
    in_out_list.clear();
    for (unsigned int i=0; i< _agg_info.size(); i++)
    {
      for( auto node : _agg_info[i]->_PrimaryComputes )
      in_out_list.push_back( node->GetID() );
    }

    return in_out_list.size();
  }

  int GetActiveAgents() const
  {
    int numCNs = 0;
    for (unsigned int i=0; i< _agg_info.size(); i++)
    {
      numCNs += _agg_info[i]->_PrimaryComputes.size();
    }
    return numCNs;
  }

private:
  friend class boost::serialization::access;

  template <class Archive>
  void serialize(Archive &ar, const unsigned int version)
  {
    ar & _verbose_option;

    ar & _master;

    ar & _db_free_pool_size;
    ar & _db_locked_pool_size;
    ar & _timer_test;
    ar & _db_query_test;
    ar & _mtc_test;
    ar & _net_vchannel_test;
    ar & _user_access_test;
    ar & _unique_daemon_id_test;
#ifdef WITH_MASTER_LOAD_STATS
    ar & _event_load;
#endif
    ar & _agg_info;
    ar & _util_info;

    ar & _local;
    ar & _errmsg;
  }

public:

  void set_verbose_option() { _verbose_option = true; }

  std::string Dump() const
  {
    std::stringstream ss;

    ss << _master.Dump( _verbose_option );
    ss << "\n\tDB_free_conn_size: " << _db_free_pool_size << "\n";
    ss << "\tDB_locked_conn_pool_size: " << _db_locked_pool_size << "\n";
    if (_timer_test)
      ss << "\tTimer_test: success\n";
    else
      ss << "\tTimer_test: fail\n";

    if (_db_query_test)
      ss << "\tDB_sql_query_test: success\n";
    else
      ss << "\tDB_sql_query_test: fail\n";

    if( GetActiveAgents() > 0 )
    {
      if (_mtc_test)
        ss << "\tMulticast_test: success\n";
      else
        ss << "\tMulticast_test: fail\n";
    }
    else
      ss << "\tMulticast_test: skipped\n";

    if (_net_vchannel_test)
      ss << "\tNetwork_vchannel_test: success\n";
    else
      ss << "\tNetwork_vchannel_test: fail\n";

    if(_user_access_test)
      ss << "\tUser_permission_test: success\n";
    else
      ss << "\tUser_permission_test: fail\n";

    if(_unique_daemon_id_test)
      ss << "\tUniqueID_test: success\n";
    else
      ss << "\tUniqueID_test: fail\n";

#ifdef WITH_MASTER_LOAD_STATS
    ss << "\tevent_loop_load: " << std::fixed << std::setprecision(1) << _event_load << " events/s\n";
#endif

    ss << "\nAggregators:" << _agg_info.size() << "\n";
    int responded = 0;
    for( auto agg : _agg_info )
      if( agg->IsResponding() )
      {
        ss << "    " << agg->Dump( _verbose_option ) << "\n";
        ++responded;
      }

    ss << "\n  Unresponsive Aggregators: " << _agg_info.size() - responded << "\n";
    for( auto agg : _agg_info )
      if( ! agg->IsResponding() )
        ss << "    " << agg->Dump( _verbose_option ) << "\n";

    ss << "\nUtility Nodes:" << _util_info.size() << "\n";
//    ss << "  Active Utility Nodes:\n";
    responded = 0;
    for( auto util : _util_info )
      if( util->IsResponding() )
      {
        ss << "    " << util->Dump( _verbose_option ) << "\n";
        ++responded;
      }
    ss << "\n  Unresponsive Utility Nodes: " << _util_info.size() - responded << "\n";
    for( auto util : _util_info )
    {
      if( ! util->IsResponding() )
        ss << "       " << util->Dump( _verbose_option ) << "\n";
    }

    ss << "\nLocal_daemon: " << _local.Dump( _verbose_option ) << "\n";
    ss << "\tStatus: " << _errmsg << "\n";
    return ss.str();
  }

private:
  // options may be set by the clients
  bool _verbose_option;

  // system status gathered from daemons
  HealthNodeInfo _master;

  unsigned _db_pool_size;
  unsigned _db_free_pool_size;
  unsigned _db_locked_pool_size;
  bool _timer_test;
  bool _db_query_test;
  bool _mtc_test;
  bool _net_vchannel_test;
  bool _user_access_test;
  bool _unique_daemon_id_test;
  std::vector<AggInfo_sptr> _agg_info;
  std::vector<UtilInfo_sptr> _util_info;

  HealthNodeInfo _local;
  std::string _errmsg;

#ifdef WITH_MASTER_LOAD_STATS
  double _event_load;
#endif
};





#endif /* CSMD_SRC_DAEMON_INCLUDE_CSM_HEALTHCHECK_DATA_H_ */
