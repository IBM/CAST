/*================================================================================

    csmd/include/csm_daemon_config.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSM_NETWORK_SRC_CSM_DAEMON_CONFIG_H_
#define CSM_NETWORK_SRC_CSM_DAEMON_CONFIG_H_

#include <iostream>
#include <string>
#include <exception>
#include <vector>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

#include "csmnet/src/CPP/endpoint_options.h"
#include "csmnet/src/CPP/endpoint.h"

#include "include/csm_db_manager.h"
#include "include/csm_daemon_state.h"
#include "include/thread_pool.h"
#include "include/csm_api_acl.h"
#include "include/csm_api_config.h"
#include "include/csm_connection_type.h"

#include "csmd/src/db/include/DBConnectionPool.h"
#include "csmd/src/daemon/include/virtual_network_channel_pool.h"

#include "csmd/src/daemon/include/csm_bucket_item.h"
#include "csmd/src/daemon/include/csm_connection_definition.h"
#include "csmd/src/daemon/include/csm_daemon_role.h"
#include "csmd/src/daemon/include/csm_tweaks.h"
#include "csmd/src/daemon/include/bds_info.h"

#ifndef DB_SCHEMA_VERSION
#define DB_SCHEMA_VERSION "unknown"
#endif

#define ENVIRONMENTAL_GRANULARITY_DEFAULT "00:00:01"
#define ENVIRONMENTAL_GRANULARITY_MINIMUM "00:00:00.50"

#define CONFIGURATION_HOSTNAME_NONE ("NONE")

namespace pt = boost::property_tree;
namespace po = boost::program_options;

template<class stream>
static stream&
operator<<( stream &out, const CSMDaemonRole &aRole )
{
  out << CSMDaemonRole_to_string( aRole );
  return( out );
}
namespace csm {
namespace daemon {

typedef std::pair<unsigned, csm::db::DBConnInfo> DBDefinitionInfo;

enum HostNameConfigState_t
{
  HOST_CONFIG_NONE,
  HOST_CONFIG_VALID,
  HOST_CONFIG_INVALID
};

class Configuration
{
public:
  virtual ~Configuration();

  static Configuration* Instance()
  {
    if( _instance == nullptr )
      throw csm::daemon::Exception("Can't initialize Configuration without parameters.");
    return _instance;
  }
  static Configuration* Instance( int argc, char **argv, const RunMode *runmode )
  {
    if (_instance == nullptr)
    {
      if(( argc == 0 )||( argv == nullptr )||(runmode == nullptr))
        throw csm::daemon::Exception("Insufficient parameters to initialize configuration.", EINVAL);
      else
        _instance = new Configuration( argc, argv, runmode );
    }
    return _instance;
  }
  static void Cleanup()
  {
    if( _instance != nullptr )
      delete _instance;
    _instance = nullptr;
  }
  
  void SetRole( const CSMDaemonRole aRole );

  // throw CSMConfigurationException if initializeLogging fails
  void initializeLogging(std::string &component);

  // if the key is not specified, add an empty string in the values string vector
  // values.size() equal to keys.size()
  void GetValuesInConfig(const std::vector<std::string> &keys, std::vector<std::string> &values) const;
  std::string GetValueInConfig(const std::string key) const;
  
  inline pt::ptree* GetProperties() const { return const_cast<pt::ptree*>( &_config ); }

  inline CSMDaemonRole GetRole() const { return _Role; }

  inline csm::daemon::DaemonState* GetDaemonState() const { return _DaemonState; }
  
  inline void SetDBConnectionPool( csm::db::DBConnectionPool *pool )
  { _DBConnectionPool = pool; }

  inline csm::db::DBConnectionPool *GetDBConnectionPool() const { return _DBConnectionPool; }
  
  inline void SetNetConnectionPool( csm::daemon::VirtualNetworkChannelPool *i_NetPool )
  { _NetConnPool = i_NetPool; }

  inline csm::daemon::VirtualNetworkChannelPool *GetNetConnectionPool() const { return _NetConnPool; }


  csm::daemon::ThreadPool *GetThreadPool() const { return _ThreadPool; }
  int GetThreadPoolSize() const { return _ThreadPoolSize; }
  
  csm::daemon::CSMIAuthList_sptr GetPermissionClass() const { return _CSMIAuthList; }
  csm::daemon::CSMAPIConfig_sptr GetAPIConfigClass() const { return _CSMAPIConfigs; }
  
  std::string GetClientId() const { return _clientId; }
  const ConnectionDefinitionList& GetCriticalConnectionList() const { return _EndpointDefinitionList; }
  
  // non-dynamic specs of master and aggregator (addresses as spec'd in config file)
  csm::network::Address_sptr GetConfiguredMasterAddress() const { return _Master; }
  csm::network::Address_sptr GetConfiguredAggregatorAddress() const { return _Aggregator; }

  std::string GetHostname() const { return _Hostname; }
 
  /**
     \brief return the active bucket item list for a given windowId
     \param[in] windowId A window id for the jitter window. The minimum value is 0.
     \return -1 if the windowId is not valid. Otherwise, return the size of bucket items
  */
  //int GetBucketItems(int windowId, std::vector<BucketItemType>& items);
  int GetLCMForBuckets() const { return _LCMOfBuckets; }
  uint64_t GetTimerInterval() const { return (uint64_t)_timer_interval.total_microseconds(); }
  uint64_t GetWindowDuration() const { return (uint64_t)_window_duration.total_microseconds(); }
  unsigned GetWindowExtensionFactor() const { return _window_extension_factor; }

  /* get the interval for each bucket */
  void GetBucketIntervals(std::vector<int> &i_o_list)
  {
    i_o_list.resize(_BucketList.size());
    for (size_t i=0; i<_BucketList.size(); i++) i_o_list[i] = _BucketList[i].first;
  }
  /* get the bucket item for a bucket */
  void GetBucketItems(size_t i_bucket_id, std::set<BucketItemType> &i_o_list) const
  {
    i_o_list.clear();
    if (i_bucket_id < _BucketList.size()) i_o_list = _BucketList[i_bucket_id].second;
  }
  
  size_t GetNumOfBuckets() const { return _BucketList.size(); }
  csm::daemon::Tweaks GetTweaks() const { return _Tweaks; }

  DBDefinitionInfo GetDBDefinitionInfo() const { return _dbInfo; }

  csm::daemon::BDS_Info GetBDS_Info() const { return _BDS_Info; }
  
private:
  Configuration( int argc, char **argv, const RunMode *runmode );

  // parse the command line and retrieve basic settings/defaults
  // the return value is to indicate if the --role(-r) option is present in the command line option
  bool ParseCommandLineOptions( int argc, char **argv );

  // open config file and fill data
  // throw file system exception if access fails...
  void LoadFromFile( const std::string &aFileName, bool roleOptionInCommand );

  // set the daemon role; will throw CSMConfigurationException if aRoleStr is not recognized
  CSMDaemonRole DaemonRoleFromString(const std::string &aRoleStr );
  
  // set up the DBConnectionPool
  void SetDBConnectionPool();

  // initialize the daemon state
  void SetDaemonState( const uint64_t aDaemonId );
  
  void CreateThreadPool();
  
  void SetCriticalConnectionList();

  void SetTweaks();

  void SetBDS_Info();
  
  void ConfigureDaemonTimers();

  // retrieve hostname from xCAT
  void SetHostname();
  HostNameConfigState_t HostNameValidate( std::string host_val );
  
  std::string GetUnixServerSocket() const ;
  mode_t GetLocalSocketPermissions() const;
  std::string GetLocalSocketGroup() const;
  uint32_t GetHeartbeatInterval() const;
  void AddConnectionDefinitionLocal( const int i_Prio );

  uint32_t GetConfigured_IP( const std::string &i_Description,
                             const std::string &i_Key ) const;
  in_port_t GetConfigured_Port( const std::string &i_Description,
                                const std::string &i_Key ) const;
  csm::network::AddressUtility_sptr GetUtilityServerAddr() const;
  csm::network::AddressAggregator_sptr GetAggregatorServerAddr() const;
  std::pair<csm::network::Address_sptr,csm::network::Address_sptr> AddConnectionDefinitionComputeListener( const int i_PrioA, const int i_PrioB );
  csm::network::Address_sptr AddConnectionDefinitionUtilityListener( const int i_Prio );
  csm::network::Address_sptr AddConnectionDefinitionAggregatorListener( const int i_Prio );
  csm::network::Address_sptr AddConnectionDefinitionComputeClient( const int i_PrioA, const int i_PrioB );
  csm::network::Address_sptr AddConnectionDefinitionUtilityClient( const int i_Prio );
  csm::network::Address_sptr AddConnectionDefinitionAggregatorClient( const int i_Prio );
  uint32_t GetAggregatorA_IP() const;
  in_port_t GetAggregatorA_Port() const;
  uint32_t GetAggregatorB_IP() const;
  in_port_t GetAggregatorB_Port() const;
  csm::network::AddressAggregator_sptr GetMasterAggregatorAddr() const;
  csm::network::AddressUtility_sptr GetMasterUtilityAddr() const;
  csm::network::AddressPTP_sptr CreateComputeListenAddr() const;
  csm::network::AddressPTP_sptr CreateAggregatorAddr( const bool i_Primary ) const;

  static Configuration* _instance;
  bool _isConfigInit;
  pt::ptree    _config;

  CSMDaemonRole _Role;
  std::string _CfgFile;
  std::string _Hostname;
  csm::db::DBConnectionPool *_DBConnectionPool;
  csm::daemon::DaemonState *_DaemonState;
  csm::daemon::VirtualNetworkChannelPool *_NetConnPool;
  csm::daemon::ThreadPool *_ThreadPool;
  int _ThreadPoolSize;
  csm::daemon::CSMIAuthList_sptr _CSMIAuthList;
  csm::daemon::CSMAPIConfig_sptr _CSMAPIConfigs;
  std::string _clientId;
  ConnectionDefinitionList _EndpointDefinitionList;
  csm::network::Address_sptr _Master;
  csm::network::Address_sptr _Aggregator;
  
  boost::posix_time::time_duration _timer_interval;
  boost::posix_time::time_duration _window_duration;  // kind of obsolete, window is not in use any more
  unsigned _window_extension_factor;

  //std::map<BucketItemType, int> _BucketItemFrequency;
  // pair.first: frequency pair.second: a list of bucket items
  std::vector< FreqAndBucketItems > _BucketList;
  int _LCMOfBuckets; // least common multiple of frequency of all buckets

  DBDefinitionInfo _dbInfo;

  csm::daemon::Tweaks _Tweaks;
  csm::daemon::BDS_Info _BDS_Info;
  
};

}  // namespace daemon
} // namespace csm


#endif /* CSM_NETWORK_SRC_CSM_DAEMON_CONFIG_H_ */
