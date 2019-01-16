/*================================================================================

    csmd/src/daemon/src/csm_daemon_config.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include <sys/types.h>
#include <ifaddrs.h>

#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/math/common_factor.hpp>
#include <boost/crc.hpp>

#ifdef logprefix
#undef logprefix
#endif
#define logprefix "Configuration"

#include "csm_pretty_log.h"

#include "csmutil/include/csm_version.h"
#include "csmnet/src/CPP/endpoint_unix.h"
#include "csmnet/src/CPP/endpoint_dual_unix.h"
#include "csmnet/src/CPP/multi_endpoint.h"

#include "csmd/include/csm_daemon_config.h"
#include "csmd/src/daemon/include/message_control.h"

#include "csmi/include/csm_cmd_options.h"

#include "csmi/include/csm_api_consts.h"

#define XCAT_FILE_NAME "/opt/xcat/xcatinfo"

#ifdef DAEMONID_USE_CRC_HOSTNAME
#undef DAEMONID_USE_CRC_HOSTNAME
#endif

// todo: remove later: defined default fallback options while command options are discussed and might be changed
#ifndef CSM_OPT_HELP_LONG
#define CSM_OPT_HELP_LONG "help"
#define CSM_OPT_HELP_SHORT 'h'
#endif

#ifndef CSM_OPT_FILE_LONG
#define CSM_OPT_FILE_LONG "file"
#define CSM_OPT_FILE_SHORT "f"
#endif

#ifndef CSM_OPT_ROLE_LONG
#define CSM_OPT_ROLE_LONG "role"
#define CSM_OPT_ROLE_SHORT "r"
#endif

namespace pt = boost::property_tree;


namespace csm {
namespace daemon {

Configuration* Configuration::_instance = NULL;

Configuration::Configuration( int argc, char **argv, const RunMode *runmode )
: _isConfigInit(false),
  _Role( CSM_DAEMON_ROLE_UNKNOWN ),
  _Hostname(""),
  _DBConnectionPool(nullptr),
  _DaemonState(nullptr),
  _NetConnPool(nullptr),
  _ThreadPool(nullptr),
  _CSMIAuthList(nullptr),
  _CSMAPIConfigs(nullptr),
  _Tweaks()
{
  _CfgFile = "/etc/ibm/csm/csm_master.cfg";

  bool RoleOptionInCommand = false;
  try {
    // parse any command line options and set/update configuration status
    RoleOptionInCommand = ParseCommandLineOptions( argc, argv );
  }
  catch ( csm::daemon::Exception &e ) {
    throw;
  }

  try {
    // will do initializeLogging in LoadFromFile()
    LoadFromFile( _CfgFile, RoleOptionInCommand );
  }
  catch ( csm::daemon::Exception &e ) {
    throw;
    // shall not continue. The code for now will come here only when csm.role is not valid
  }

  CSMLOGp( csmd, info, "CSMD" ) << _Role << "; Version: " << std::string( CSM_VERSION, strnlen( CSM_VERSION, 10 ) )
    << "; Build: " << std::string( CSM_COMMIT, strnlen( CSM_COMMIT, 10 ))
    << "; Date: " << std::string( CSM_DATE) << "; Config file: " << _CfgFile;

  #define RETRIES 5
  #define WAIT 30
  for(int retry = 1; _Hostname.empty() && retry <= RETRIES; ++retry)
  {
    try
    {
      SetHostname();
    }
    catch (csm::daemon::Exception& e)
    {
      CSMLOG( csmd, debug ) << "hostname. Retrying " << retry << " of " << RETRIES << " after " << WAIT << "s...";
      sleep(WAIT);
    }
  }

  if(_Hostname.empty())
  {
    CSMLOG( csmd, error ) << "Could not determine hostname (NODE) from xCAT file: " << XCAT_FILE_NAME;
    throw csm::daemon::Exception("Failed to retrieve hostname from xCAT file!");
  }

  // next, set up the daemon state here
  // create a daemonID from ip and PID
  // first try to retrieve IPs from network interfaces
  uint32_t idaddr = 0;
#ifndef DAEMONID_USE_CRC_HOSTNAME

  struct ifaddrs *ifs, *cif;
  if( getifaddrs( &ifs ) != 0 )
  {
    throw csm::daemon::Exception("Failed to obtain local IP address for daemonID generation", errno );
  }
  cif = ifs;
  uint32_t max_idaddr = 0;
  uint32_t min_idaddr = 0xFFFFFFFF;
  for( cif = ifs; cif != nullptr; cif = cif->ifa_next )
  {
    if( ( cif->ifa_addr == nullptr ) || ( cif->ifa_addr->sa_family != AF_INET ))
      continue;

    struct sockaddr_in *saddr = (struct sockaddr_in*)(cif->ifa_addr);
    uint32_t tmp_idaddr = saddr->sin_addr.s_addr;
    // skip localhost address
    if( (tmp_idaddr & 0xFF) == 127 )
      continue;

    tmp_idaddr = ntohl( tmp_idaddr );
    if( tmp_idaddr > max_idaddr ) max_idaddr = tmp_idaddr;
    if( tmp_idaddr < min_idaddr ) min_idaddr = tmp_idaddr;
  }
  freeifaddrs( ifs );
  idaddr = max_idaddr;
#endif

  uint64_t daemonID = (uint64_t)idaddr;

  // fallback to hostname CRC in case no interface address could be found
  // this should only happen if there's just a loopback enabled
  if( daemonID == 0 )
  {
    boost::crc_optimal<32, 0x04C11DB7, 0, 0, false, false> crc;
    std::string daemonID_name = _Hostname;

    uint32_t *ptr = (uint32_t*)( daemonID_name ).c_str();
    crc.process_bytes( ptr, _Hostname.length() );

    daemonID = crc.checksum() & (( 1ull << MSGID_BITS_PER_DAEMON_ID ) - 1ull);
  }

  daemonID = (daemonID << MSGID_BITS_PER_PID) + ( getpid() & ((1 << MSGID_BITS_PER_PID)-1) );
  SetDaemonState( daemonID & ((1ull << MSGID_BITS_PER_DAEMON_ID)-1ull) );

  if  (_DaemonState)
  {
    CSMLOG( csmd, info ) << "DaemonID: " <<  _DaemonState->GetDaemonID()  
        << " using hostname: " << _Hostname << " PID: " << getpid();
  }
  else
  {
    CSMLOG( csmd, info ) << "DaemonID: MISSING  using hostname: " << _Hostname << " PID: " << getpid();
  }
  
  // after parsing the command line and the configuration file,
  // can try to set up the DBConnectionPool here. Now only Master has the premission.
  SetDBConnectionPool();

  CreateThreadPool();
  
  std::string authFile = GetValueInConfig( std::string("csm.api_permission_file") );
  if ( authFile.empty() )
    CSMLOG( csmd, warning ) << "Couldn't find/access api permission file. Any local client request will be considered PRIVILEGED!";
  
  // create AuthList even if authFile config doesn't exist to prevent nullptr
  _CSMIAuthList = std::make_shared<CSMIAuthList>( authFile );
  if( _CSMIAuthList == nullptr )
    throw csm::daemon::Exception("FATAL: Unable to create API permission list.");
  
  std::string apiConfFile = GetValueInConfig( std::string("csm.api_configuration_file") );
  _CSMAPIConfigs = std::make_shared<CSMAPIConfig>( apiConfFile );
  if( _CSMIAuthList == nullptr )
    throw csm::daemon::Exception("FATAL: Unable to create API permission list.");

  // set up endpoint <Address,Option> list for each daemon role
  SetCriticalConnectionList();

  // set up the configured tweaks
  SetTweaks();

  // set up potential BDS access
  if( _Role == CSM_DAEMON_ROLE_AGGREGATOR )
    SetBDS_Info();

  // set up several intervals and the jitter window configuration
  ConfigureDaemonTimers();

  SetRecurringTasks();

  LoadJitterMitigation();
}

void Configuration::SetHostname()
{
  // Collect the xCAT nodename
  std::ifstream xcat_in( XCAT_FILE_NAME );
  std::string line("");
  const std::string NODE_TOKEN("NODE=");

  if(!xcat_in.fail())
  {
    while (getline(xcat_in, line)) 
    {
      if (line.substr(0, NODE_TOKEN.size()) == NODE_TOKEN)
      {
        _Hostname = line.substr(NODE_TOKEN.size());
        break;
      }
    }
    xcat_in.close();
  }

  if (!_Hostname.empty())
  {
    if(_Hostname.size() >= CSM_NODE_NAME_MAX)
    {
      _Hostname.resize(CSM_NODE_NAME_MAX - 1);
    }
  }
  else
  {
    // for some roles it's fatal to not get the xCAT hostname/nodename
    if(( _Role == CSM_DAEMON_ROLE_AGENT ) || ( _Role == CSM_DAEMON_ROLE_UTILITY ))
    {
      throw csm::daemon::Exception( std::string("Configuration: Could not determine hostname (NODE) from xCAT file: ") + XCAT_FILE_NAME );
    }

    // for all other roles, we're falling back to the default hostname
    char host[256];
    memset( host, 0, 256 );
    gethostname( host, 256 );
    _Hostname = std::string( host );

    CSMLOG(csmd, warning ) << "Could not determine xcatname, falling back to gethostname = " << _Hostname;
  }

}

void Configuration::ConfigureDaemonTimers()
{
  _LCMOfBuckets = 1;
  
  // All daemons will use these intervals to schedule the environmental collection and other timed tasks
  // they will use the default interval setting ENVIRONMENTAL_GRANULARITY_DEFAULT
  _timer_interval = boost::posix_time::seconds(0);
  _window_duration = boost::posix_time::seconds(0);
  _window_extension_factor = 1;
  
  boost::posix_time::time_duration td;
  std::string interval = ENVIRONMENTAL_GRANULARITY_DEFAULT;
  std::string duration = ENVIRONMENTAL_GRANULARITY_MINIMUM;
  if (_isConfigInit)
  {
    interval = _config.get<std::string>("csm.data_collection.granularity", interval);
  }
  
  _timer_interval = boost::posix_time::duration_from_string(interval);
  _window_duration = boost::posix_time::duration_from_string(duration);
  CSMLOG(csmd, debug ) << "Data collection timer granularity = " << _timer_interval;
  
  if( _window_duration >= _timer_interval )
  {
    CSMLOG( csmd, error ) << "invalid setting: csm.data_collection.granularity < 0.5s. Will continue with defaults!!";
    _timer_interval = boost::posix_time::duration_from_string(ENVIRONMENTAL_GRANULARITY_DEFAULT);
    _window_duration = boost::posix_time::duration_from_string(ENVIRONMENTAL_GRANULARITY_MINIMUM);
  }

  _window_extension_factor = 1;
  if( _window_duration * _window_extension_factor > _timer_interval )
  {
    _window_extension_factor = 1;
  }

  if (!_isConfigInit) return;
  
  try
  {
    pt::ptree buckets = _config.get_child("csm.data_collection.buckets");
    CSMLOG( csmd, info ) << "Found " << buckets.size() << " buckets.";
  }
  catch( pt::ptree_error& f )
  {
    CSMLOG( csmd, info ) << "no buckets for data collection defined";
    return;
  }

  try
  {
    for (pt::ptree::value_type &bucket : _config.get_child("csm.data_collection.buckets"))
    {
      std::string execution_interval = bucket.second.get<std::string>("execution_interval");
      std::stringstream ss_noexcept(execution_interval);
      if (!(ss_noexcept >> td))
      {
        CSMLOG(csmd, error ) << "csm.data_collection.buckets.execution_interval (" << execution_interval << ") is not valid. Ignoring this bucket!";
        continue;
      }
    
      std::vector<std::string> items;
      for (pt::ptree::value_type &item : bucket.second.get_child("item_list"))
      {
        items.push_back(item.second.data());
      }
      
      boost::posix_time::time_duration execution = boost::posix_time::duration_from_string(execution_interval);
      if( execution < _timer_interval )
      {
        CSMLOG(csmd, warning ) << "csm.data_collection.buckets.execution_interval < granularity! Adjusting execution to granularity.";
        execution = _timer_interval;
      }
      int frequency = 1;
      if (execution.total_microseconds() > _timer_interval.total_microseconds())
         frequency = execution.total_microseconds()/_timer_interval.total_microseconds();
      
      std::set<BucketItemType> bset;
      for (size_t i=0; i<items.size(); i++)
      {
        boost::algorithm::to_upper(items[i]);
        auto it = str2BucketItemType.find(items[i]);
        if ( it != str2BucketItemType.end() )
        {
          bset.insert(it->second);
          //_BucketItemFrequency[ it->second ] = frequency;
          _LCMOfBuckets = boost::math::lcm(_LCMOfBuckets, frequency);
          CSMLOG(csmd, debug ) << "Add bucket item (" << it->second << ") with frequency " << frequency;
        }
        else
        {
          CSMLOG(csmd, warning ) << "Bucket item (" << items[i] << ") is not defined.";
        }
      }
      // store it only when there is at least one valid item in the bucket
      if (bset.size() > 0) _BucketList.push_back( FreqAndBucketItems(frequency, bset) );
    }
  }
  catch (pt::ptree_error& f)
  {
    CSMLOG(csmd, error ) << "DataCollection Error: " << f.what()
        << ". The rest of the data_collection config will be ignored!";
  }
  
  CSMLOG(csmd, debug ) << "Least Common Multiple of all buckets = " << _LCMOfBuckets;
  
}

#if 0
int Configuration::GetBucketItems(int windowId, std::vector<BucketItemType>& items)
{
  items.clear();
  
  if (windowId < 0)
  {
    LOG(csmd, error) << "GetBucketItems(): window id has to be a non-negative integer";
    return -1;
  }
  
  std::stringstream ss;
  for (auto it = _BucketItemFrequency.begin(); it != _BucketItemFrequency.end(); it++)
  {
    if ( windowId % it->second == 0 )
    {
      items.push_back(it->first);
      ss << " " << it->first;
    }
  }

  CSMLOG(csmd, info) << "windowId(" << windowId << "): " << ss.str();
  
  return items.size();
}
#endif

CSMDaemonRole Configuration::DaemonRoleFromString( const std::string &aRoleStr )
{
  switch ((aRoleStr.c_str())[0]) {
    case 'A':  // aggregator
    case 'a': return CSM_DAEMON_ROLE_AGGREGATOR; break;
    case 'C':  // compute node agent
    case 'c': return CSM_DAEMON_ROLE_AGENT; break;
    case 'M':  // master
    case 'm': return CSM_DAEMON_ROLE_MASTER; break;
    case 'U':  // utility node agent
    case 'u': return CSM_DAEMON_ROLE_UTILITY; break;
    default:
    {
      CSMLOG(csmd, error ) << "Unknown Daemon Role...";
      throw csm::daemon::Exception("Unknown Daemon Role.");
    }
  }
}

Configuration::~Configuration()
{
  // TODO Auto-generated destructor stub
  if( _DaemonState ) delete _DaemonState;
  if (_ThreadPool) delete _ThreadPool;
}

void Configuration::SetRole( const CSMDaemonRole aRole )
{
  if( aRole < CSM_DAEMON_ROLE_MAX )
    _Role = aRole;
  else
    throw csm::daemon::Exception("Invalid Daemon Role.");
}

void Configuration::LoadFromFile( const std::string &aFileName, bool roleOptionInCommand )
{
  try
  {
    pt::read_json(aFileName, _config);
    _isConfigInit = true;
  }
  catch (pt::json_parser_error& f)
  {
    CSMLOG(csmd, error ) << "in config file: " << aFileName << " ERROR: " << f.what();
    throw csm::daemon::Exception(std::string("Error while reading configuration file: ")+f.what(), EBADF);
  }

  try
  {
    // set up the logging component in csm
    std::string component("csm.log");
    initializeLogging(component);
  }
  catch (csm::daemon::Exception& e)
  {
    CSMLOG(csmd, error ) << "Error loading logging configuration: " << e.what()
        << " (continuing with default settings...)";
  }
  
  try
  {
    // checking if csm.role is specified
    std::string role = GetValueInConfig(std::string("csm.role"));
    if ( !roleOptionInCommand && !role.empty() )
    {
      boost::algorithm::to_upper(role);
      std::stringstream ss_m, ss_a, ss_c, ss_u;
      if (role == (ss_m << CSM_DAEMON_ROLE_MASTER).str() ) _Role = CSM_DAEMON_ROLE_MASTER;
      else if (role == (ss_a << CSM_DAEMON_ROLE_AGGREGATOR).str() ) _Role = CSM_DAEMON_ROLE_AGGREGATOR;
      else if (role == (ss_u << CSM_DAEMON_ROLE_UTILITY).str() ) _Role = CSM_DAEMON_ROLE_UTILITY;
      else if (role == (ss_c << CSM_DAEMON_ROLE_AGENT).str() ) _Role = CSM_DAEMON_ROLE_AGENT;
      else
      {
        CSMLOG(csmd, error ) << "csm.role (=" << role << ") is not valid";
        throw csm::daemon::Exception("Invalid daemon role in config file.", EINVAL);
      }
    }
  }
  catch (csm::daemon::Exception& e)
  {
    CSMLOG(csmd,error) << "The value in csm.role is not recognized: " << e.what();
    throw;
  }
}

void Configuration::initializeLogging(std::string &component)
{
  if (!_isConfigInit) return;

  // so far, initializeLogging() always returns 0
  if (::initializeLogging(component, _config) != 0)
  {
    throw csm::daemon::Exception("Unable to initialize logging.", EINVAL);
  }
}

std::string Configuration::GetValueInConfig(const std::string key) const
{
  // in the case when the config is not initialized, return the empty string
  // for this key.
  if (!_isConfigInit) 
  {
    return std::string();
  }

  boost::optional<std::string> value = _config.get_optional<std::string>(key);
  if (value.is_initialized()) return (*value);
  else return std::string();
}

void Configuration::GetValuesInConfig(const std::vector<std::string> &keys,
                                            std::vector<std::string> &values) const
{
  // in the case when the config is not initialized, return the empty string
  // for all keys.
  if (!_isConfigInit) 
  {
    values = std::vector<std::string> ( keys.size(), std::string() );
    return;
  }

  for (unsigned int i=0; i<keys.size(); i++) 
  {
    boost::optional<std::string> value = _config.get_optional<std::string>(keys[i]);
    if (value.is_initialized()) values.push_back(*value);
    else values.push_back(std::string());
  } 
}

bool Configuration::ParseCommandLineOptions( int argc, char **argv )
{
  bool RoleOptionInCommand = false;
  
  std::string roleOpt;
  std::string cfgFileOpt;

  po::options_description usage("Supported Command Line Options");
  std::string opt_help = std::string(CSM_OPT_HELP_LONG)+','+CSM_OPT_HELP_SHORT;
  std::string opt_file = std::string(CSM_OPT_FILE_LONG)+','+CSM_OPT_FILE_SHORT;
  std::string opt_role = std::string(CSM_OPT_ROLE_LONG)+','+CSM_OPT_ROLE_SHORT;
  usage.add_options()
        (opt_help.c_str(), "Show this help")
        (opt_file.c_str(),
            po::value<std::string>(&cfgFileOpt)->default_value(_CfgFile),
            "Specify configuration file (default: /etc/ibm/csm/csm_master.cfg)")
        (opt_role.c_str(),
            po::value<std::string>(&roleOpt),
            "Set the role of the daemon (M|m)[aster] | (A|a)[ggregator] | (C|c)[ompute] | (U|u)[tility]")
    ;

  po::variables_map vm;
  po::store( po::parse_command_line(argc, argv, usage), vm);
  po::notify(vm);

  if( vm.count( CSM_OPT_HELP_LONG ) )
  {
      std::cerr << usage << std::endl;
      errno = 0;
      exit(0);
  }

  // add cmd-line options to property_tree
  _config.add( "cmdline.configfile", cfgFileOpt );

  if( vm.count( CSM_OPT_ROLE_LONG ) )
  {
    _Role = DaemonRoleFromString( vm[CSM_OPT_ROLE_LONG].as<std::string>() );
    RoleOptionInCommand = true;
  }
  else _Role = CSM_DAEMON_ROLE_MASTER;
  
  if( vm.count( CSM_OPT_FILE_LONG ) )
    _CfgFile = vm[CSM_OPT_FILE_LONG].as<std::string>();
  CSMLOG(csmd, debug) << "Using command line provided config: " << _CfgFile;
  
  return RoleOptionInCommand;
}

void Configuration::SetDBConnectionPool()
{
  if (!_isConfigInit) return;
  
  // now just allow Master to access the DB
  if (_Role != CSM_DAEMON_ROLE_MASTER) return;
  
  // get the db connection setup
  std::vector<std::string> keys;
  keys.push_back(std::string("csm.db.connection_pool_size"));
  keys.push_back(std::string("csm.db.host"));
  keys.push_back(std::string("csm.db.user"));
  keys.push_back(std::string("csm.db.password"));
  keys.push_back(std::string("csm.db.name"));
  // not sure if we need it.
  //keys.push_back(std::string("csm.db.schema_name"));
  
  std::vector<std::string> values;
  GetValuesInConfig(keys, values);
  
  if (values[0].empty()) CSMLOG(csmd, warning ) << "csm.db.connection_pool_size not specified or empty";
  if (values[1].empty()) CSMLOG(csmd, warning ) << "csm.db.host not specified or empty";
  if (values[2].empty()) CSMLOG(csmd, warning ) << "csm.db.user not specified or empty";
  if (values[3].empty()) CSMLOG(csmd, warning ) << "csm.db.passwprd not specified or empty";
  if (values[4].empty()) CSMLOG(csmd, warning ) << "csm.db.name not specified or empty";

  // create connection
  csm::db::DBConnInfo dbInfo( values[1], values[2], values[3], values[4] );

  unsigned pool_size = DEFAULT_CONNECTION_POOL_SIZE;
  if (!values[0].empty())
    pool_size = std::stoi(values[0]);
  else
  {
    CSMLOG(csmd, warning ) << "db pool_size has invalid value" << values[0];
  }

  _dbInfo = DBDefinitionInfo(pool_size, dbInfo);
}

void Configuration::SetDaemonState( const uint64_t aDaemonId )
{
  if (_Role == CSM_DAEMON_ROLE_MASTER)          _DaemonState = new csm::daemon::DaemonStateMaster( aDaemonId );
  else if (_Role == CSM_DAEMON_ROLE_AGGREGATOR) _DaemonState = new csm::daemon::DaemonStateAgg( aDaemonId );
  else if (_Role == CSM_DAEMON_ROLE_AGENT)      _DaemonState = new csm::daemon::DaemonStateAgent( aDaemonId );
  else if (_Role == CSM_DAEMON_ROLE_UTILITY)    _DaemonState = new csm::daemon::DaemonStateUtility( aDaemonId );
}

void Configuration::CreateThreadPool()
{
  if (!_isConfigInit) return;
  
  // every daemon can possibly create its own thread pool for event handlers
  // for now, only the master config has included this option
  
  //\todo: Need to enforce the thread_pool_size >= 1. The main loop should be running as a separate thread
  std::string value = GetValueInConfig("csm.thread_pool_size");
  int thread_pool_size = 0;
  if (!value.empty())
  {
    thread_pool_size = std::stoi(value);
    // now the max is set to half of the available hardware threads on a node
    int max_threads = boost::thread::hardware_concurrency()/2;
    thread_pool_size = (thread_pool_size > max_threads)? max_threads:thread_pool_size;
    
    if (thread_pool_size > 0) _ThreadPool = new ThreadPool( thread_pool_size );
  }
  
  _ThreadPoolSize = thread_pool_size;
  CSMLOG(csmd, info ) << "thread_pool_size = " << thread_pool_size;
  
}

  std::string Configuration::GetUnixServerSocket() const
  {
    std::string localSocketName = std::string( CSM_NETWORK_LOCAL_SSOCKET );
    if (!_isConfigInit) {
      CSMLOG(csmd, error )
          << "csm.net.local_client_listen.socket failed to read from config. Using default: "
          << localSocketName;
    } else {
      localSocketName = GetValueInConfig( "csm.net.local_client_listen.socket" );
      if( localSocketName.empty() )
      {
        localSocketName = std::string( CSM_NETWORK_LOCAL_SSOCKET );
        CSMLOG(csmd, warning ) << "csm.net.local_client_listen.socket is missing or empty. Using default: "
            << localSocketName;
      }
    }
    return localSocketName;
  }
  
  mode_t Configuration::GetLocalSocketPermissions() const
  {
    std::string permStr = GetValueInConfig( std::string("csm.net.local_client_listen.permissions") );
    mode_t perms = 0777;
    if( ! permStr.empty() )
    {
      perms = std::stol( permStr, NULL, 8 );
      if( perms == 0 )
        perms = 0777;
    }
    return perms;
  }

  std::string Configuration::GetLocalSocketGroup() const
  {
    std::string groupID = GetValueInConfig( std::string("csm.net.local_client_listen.group") );
    if( groupID.empty() )
      return std::string( getgrgid( getegid() )->gr_name );  // get effective gid of user by default
    else
      return groupID;

  }

  uint32_t Configuration::GetHeartbeatInterval() const
  {
    uint32_t interval = CSM_NETWORK_HEARTBEAT_INTERVAL;
    std::string value = GetValueInConfig( std::string( "csm.net.heartbeat_interval" ) );
    if( ! value.empty() )
      interval = (uint32_t)strtol( value.c_str(), nullptr, 10 );

    if( interval < 3 )
    {
      interval = CSM_NETWORK_HEARTBEAT_INTERVAL;
      CSMLOG( csmd, warning ) << "Heartbeat interval is shorter than 3s. That's too short. Using default: " << interval;
    }
    if( interval > 3600 )
    {
      interval = CSM_NETWORK_HEARTBEAT_INTERVAL;
      CSMLOG( csmd, warning ) << "Heartbeat interval is longer than 3600s. That's too long. Using default: " << interval;
    }
    return interval;
  }

  // this will always create a server-side socket since the clients are not daemons
  void Configuration::AddConnectionDefinitionLocal( const int i_Prio )
  {
    std::string localSocketName = GetUnixServerSocket();
    csm::network::EndpointOptionsUnix_sptr opts = std::make_shared<csm::network::EndpointOptionsUnix>( true,
                                                                                                       GetPermissionClass(),
                                                                                                       GetAPIConfigClass(),
                                                                                                       GetLocalSocketPermissions(),
                                                                                                       GetLocalSocketGroup() );
    csm::network::Address_sptr addr = std::make_shared<csm::network::AddressUnix>( localSocketName.c_str() );
    
    _EndpointDefinitionList.push_back( csm::daemon::ConnectionDefinition(addr, opts, i_Prio, "localhost") );
  }
  
  uint32_t Configuration::GetConfigured_IP( const std::string &i_Description,
                                            const std::string &i_Key ) const
  {
    uint32_t ip = 0;
    std::string value = GetValueInConfig( i_Key.c_str() );
    if ( !value.empty() )
    {
      struct hostent *record;
      if ( (record = gethostbyname(value.c_str())) )
      {
        struct in_addr **addr_list = (struct in_addr **) record->h_addr_list;
        ip = ntohl(addr_list[0]->s_addr);
      }
      else
      {
        CSMLOG( csmd, error )  << i_Description << ": Unable to resolve hostname: " << value.c_str();
        throw csm::daemon::Exception( i_Description + ": Unable to resolve hostname" );
      }
    }
    else
    {
      CSMLOG( csmd, error ) << i_Key << " not defined in configuration.";
      throw csm::daemon::Exception( i_Key + " not defined in configuration." );
    }
    return ip;
  }

  in_port_t Configuration::GetConfigured_Port( const std::string &i_Description,
                                               const std::string &i_Key ) const
  {
    in_port_t port = 0;
    std::string value = GetValueInConfig(i_Key);
    if ( !value.empty() )
      port = std::stoi(value);
    else
    {
      CSMLOG( csmd, error ) << i_Key << " not defined in configuration.";
      throw csm::daemon::Exception( i_Key + " not defined in configuration." );
    }
    return port;
  }

  uint32_t Configuration::GetAggregatorA_IP() const
  {
    return GetConfigured_IP( "AggregatorA", "csm.net.aggregatorA.host" );
  }
  uint32_t Configuration::GetAggregatorB_IP() const
  {
    return GetConfigured_IP( "AggregatorB", "csm.net.aggregatorB.host" );
  }
  in_port_t Configuration::GetAggregatorA_Port() const
  {
    return GetConfigured_Port( "AggregatorA", "csm.net.aggregatorA.port" );
  }
  in_port_t Configuration::GetAggregatorB_Port() const
  {
    return GetConfigured_Port( "AggregatorB", "csm.net.aggregatorB.port" );
  }

  csm::network::AddressPTP_sptr Configuration::CreateAggregatorAddr( const bool i_Primary ) const
  {
    if( i_Primary )
      return std::make_shared<csm::network::AddressPTP>( GetAggregatorA_IP(),
                                                         GetAggregatorA_Port() );
    else
      return std::make_shared<csm::network::AddressPTP>( GetAggregatorB_IP(),
                                                         GetAggregatorB_Port() );
  }

  csm::network::AddressPTP_sptr Configuration::CreateComputeListenAddr() const
  {
    return std::make_shared<csm::network::AddressPTP>( GetConfigured_IP("ComputeListener", "csm.net.compute_listen.host"),
                                                       GetConfigured_Port("ComputeListener", "csm.net.compute_listen.port") );
  }
  csm::network::AddressUtility_sptr Configuration::GetUtilityServerAddr() const
  {
      return std::make_shared<csm::network::AddressUtility>( GetConfigured_IP("UtilityListener", "csm.net.utility_listen.host"),
                                                             GetConfigured_Port("UtilityListener", "csm.net.utility_listen.port") );
  }

  csm::network::AddressAggregator_sptr Configuration::GetAggregatorServerAddr() const
  {
      return std::make_shared<csm::network::AddressAggregator>( GetConfigured_IP("AggregatorListener", "csm.net.aggregator_listen.host"),
                                                                GetConfigured_Port("AggregatorListener", "csm.net.aggregator_listen.port") );
  }

  csm::network::AddressAggregator_sptr Configuration::GetMasterAggregatorAddr() const
  {
      return std::make_shared<csm::network::AddressAggregator>( GetConfigured_IP("AggregatorClient", "csm.net.master.host"),
                                                                GetConfigured_Port("AggregatorClient", "csm.net.master.port") );
  }

  csm::network::AddressUtility_sptr Configuration::GetMasterUtilityAddr() const
  {
      return std::make_shared<csm::network::AddressUtility>( GetConfigured_IP("UtilityClient", "csm.net.master.host"),
                                                             GetConfigured_Port("UtilityClient", "csm.net.master.port") );
  }

  std::pair<csm::network::Address_sptr,csm::network::Address_sptr> Configuration::AddConnectionDefinitionComputeListener( const int i_PrioA, const int i_PrioB )
  {
    if( GetValueInConfig("csm.net.compute_listen.host").empty() )
    {
      CSMLOG( csmd, error ) << "Config file has no compute_listen entry or host is empty.";
      throw csm::daemon::Exception("Config file has no compute_listen entry or host is empty.");
    }

    csm::network::SSLFilesCollection files;
    files._CAFile = GetValueInConfig("csm.net.ssl.ca_file");
    files._CredPem = GetValueInConfig("csm.net.ssl.cred_pem");
    csm::network::EndpointOptionsPTP_sptr options =
        std::make_shared<csm::network::EndpointOptionsPTP<csm::network::AddressPTP>>(
            true,
            files,
            false,
            GetHeartbeatInterval() );

    ConnectionDefinition prim_conn = ConnectionDefinition( CreateComputeListenAddr(),
                                                           options,
                                                           i_PrioA,
                                                           GetValueInConfig( "csm.net.compute_listen.host" ),
                                                           csm::daemon::ConnectionType::SINGLE );
    _EndpointDefinitionList.push_back( prim_conn );

    return std::make_pair( prim_conn._Addr, nullptr );
  }
  
  csm::network::Address_sptr Configuration::AddConnectionDefinitionComputeClient( const int i_PrioA, const int i_PrioB )
  {
    std::string aggA_host = GetValueInConfig("csm.net.aggregatorA.host");
    if( HostNameValidate( aggA_host ) != HOST_CONFIG_VALID )
    {
      CSMLOG( csmd, error ) << "Config file has incomplete aggregatorA configuration (" << aggA_host << ")";
      throw csm::daemon::Exception("Incomplete aggregator configuration. csm.net.aggregatorA.host missing or invalid.");
    }

    bool singleAggMode = false;
    std::string aggB_host = GetValueInConfig("csm.net.aggregatorB.host");
    switch( HostNameValidate( aggB_host ) )
    {
      case HOST_CONFIG_NONE:
        CSMLOG( csmd, info ) << "No secondary aggregator configured. Running in NON-REDUNDANT mode.";
        singleAggMode = true;
        break;
      case HOST_CONFIG_INVALID:
        CSMLOG( csmd, warning ) << "No secondary aggregator configured. Running in NON-REDUNDANT mode.";
        singleAggMode = true;
        break;
      case HOST_CONFIG_VALID:
      default:
        break;

    }

    csm::network::SSLFilesCollection files;
    files._CAFile = GetValueInConfig("csm.net.ssl.ca_file");
    files._CredPem = GetValueInConfig("csm.net.ssl.cred_pem");

    csm::network::EndpointOptionsPTP_sptr optionsA =
        std::make_shared<csm::network::EndpointOptionsPTP<csm::network::AddressPTP>>(
            GetAggregatorA_IP(),
            GetAggregatorA_Port(),
            files,
            false,
            GetHeartbeatInterval() );
    ConnectionDefinition prim_conn = ConnectionDefinition( CreateAggregatorAddr( true ),
                                                           optionsA,
                                                           i_PrioA,
                                                           GetValueInConfig( "csm.net.aggregatorA.host" ),
                                                           singleAggMode ?
                                                               csm::daemon::ConnectionType::PRIMARY :
                                                               csm::daemon::ConnectionType::SECONDARY );
    _EndpointDefinitionList.push_back( prim_conn );

    // if in single aggregator mode, we define an empty entry in the def list
    if( singleAggMode )
    {
      _EndpointDefinitionList.push_back( ConnectionDefinition( nullptr,
                                                               nullptr,
                                                               0,
                                                               "",
                                                               csm::daemon::ConnectionType::ANY ));
      return prim_conn._Addr;
    }

#ifndef CSM_MULTI_AGGREGATOR_PER_NODE
    // check for same host definition of aggregators in case we're not configured as CSM_MULTI_AGGREGATOR_PER_NODE
    if( GetValueInConfig( "csm.net.aggregatorB.host" ) == GetValueInConfig( "csm.net.aggregatorA.host" ) )
    {
      LOG( csmd, error ) << "Both configured aggregators are on the same host. csmd build is configured to run one aggregator per node only.";
      throw csm::daemon::Exception("Both configured aggregators are on the same host. csmd build is configured to run one aggregator per node only.");
    }
    else
#endif
    {
      csm::network::EndpointOptionsPTP_sptr optionsB =
          std::make_shared<csm::network::EndpointOptionsPTP<csm::network::AddressPTP>>(
              GetAggregatorB_IP(),
              GetAggregatorB_Port(),
              files,
              true,
              GetHeartbeatInterval() );
      _EndpointDefinitionList.push_back( ConnectionDefinition( CreateAggregatorAddr( false ),
                                                               optionsB,
                                                               i_PrioB,
                                                               GetValueInConfig( "csm.net.aggregatorB.host" ),
                                                               csm::daemon::ConnectionType::SECONDARY ) );
    }

    return prim_conn._Addr;
  }


  csm::network::Address_sptr Configuration::AddConnectionDefinitionUtilityListener( const int i_Prio )
  {
    csm::network::SSLFilesCollection files;
    files._CAFile = GetValueInConfig("csm.net.ssl.ca_file");
    files._CredPem = GetValueInConfig("csm.net.ssl.cred_pem");
    csm::network::EndpointOptionsUtility_sptr options =
        std::make_shared<csm::network::EndpointOptionsPTP<csm::network::AddressUtility>>(
            true,
            files,
            false,
            GetHeartbeatInterval() );

    ConnectionDefinition conn = ConnectionDefinition( GetUtilityServerAddr(),
                                                      options,
                                                      i_Prio,
                                                      GetValueInConfig( "csm.net.utility_listen.host" ) );
    _EndpointDefinitionList.push_back( conn );
    return conn._Addr;
  }

  csm::network::Address_sptr Configuration::AddConnectionDefinitionUtilityClient( const int i_Prio )
  {
    csm::network::SSLFilesCollection files;
    files._CAFile = GetValueInConfig("csm.net.ssl.ca_file");
    files._CredPem = GetValueInConfig("csm.net.ssl.cred_pem");
    csm::network::EndpointOptionsUtility_sptr options =
        std::make_shared<csm::network::EndpointOptionsPTP<csm::network::AddressUtility>>(
            GetConfigured_IP("Master",
                             "csm.net.master.host"),
            GetConfigured_Port("Master",
                               "csm.net.master.port"),
            files,
            false,
            GetHeartbeatInterval() );
    ConnectionDefinition conn = ConnectionDefinition( GetMasterUtilityAddr(),
                                                      options,
                                                      i_Prio,
                                                      GetValueInConfig( "csm.net.master.host" ) );
    _EndpointDefinitionList.push_back( conn );
    return conn._Addr;
  }

  csm::network::Address_sptr Configuration::AddConnectionDefinitionAggregatorListener( const int i_Prio )
  {
    csm::network::SSLFilesCollection files;
    files._CAFile = GetValueInConfig("csm.net.ssl.ca_file");
    files._CredPem = GetValueInConfig("csm.net.ssl.cred_pem");
    csm::network::EndpointOptionsAggregator_sptr options =
        std::make_shared<csm::network::EndpointOptionsPTP<csm::network::AddressAggregator>>(
            true,
            files,
            false,
            GetHeartbeatInterval() );
    ConnectionDefinition conn = ConnectionDefinition( GetAggregatorServerAddr(),
                                                      options,
                                                      i_Prio,
                                                      GetValueInConfig( "csm.net.aggregator_listen.host" ) );
    _EndpointDefinitionList.push_back( conn );
    return conn._Addr;
  }

  csm::network::Address_sptr Configuration::AddConnectionDefinitionAggregatorClient( const int i_Prio )
  {
    csm::network::SSLFilesCollection files;
    files._CAFile = GetValueInConfig("csm.net.ssl.ca_file");
    files._CredPem = GetValueInConfig("csm.net.ssl.cred_pem");
    csm::network::EndpointOptionsAggregator_sptr options =
        std::make_shared<csm::network::EndpointOptionsPTP<csm::network::AddressAggregator>>(
            GetConfigured_IP("Master",
                             "csm.net.master.host"),
            GetConfigured_Port("Master",
                               "csm.net.master.port"),
            files,
            false,
            GetHeartbeatInterval() );
    ConnectionDefinition conn = ConnectionDefinition( GetMasterAggregatorAddr(),
                                                      options,
                                                      i_Prio,
                                                      GetValueInConfig( "csm.net.master.host" ) );
    _EndpointDefinitionList.push_back( conn );
    return conn._Addr;
  }

  // the critical primary connection needs to be added first !
  void Configuration::SetCriticalConnectionList()
  {
    _Master = nullptr;
    _Aggregator = nullptr;
    
    switch( _Role )
    {
      case CSM_DAEMON_ROLE_MASTER:
      {
        AddConnectionDefinitionUtilityListener( CSM_CONNECTION_MGR_CRITICAL_PRIORITY + 1 );
        AddConnectionDefinitionAggregatorListener( CSM_CONNECTION_MGR_CRITICAL_PRIORITY + 2 );
        AddConnectionDefinitionLocal( CSM_CONNECTION_MGR_CRITICAL_PRIORITY );
        break;
      }
      
      case CSM_DAEMON_ROLE_AGGREGATOR:
      {
        _Master = AddConnectionDefinitionAggregatorClient( CSM_CONNECTION_MGR_CRITICAL_PRIORITY + 2 );
        AddConnectionDefinitionComputeListener( CSM_CONNECTION_MGR_CRITICAL_PRIORITY + 1,
                                                CSM_CONNECTION_MGR_CRITICAL_PRIORITY + 1 );
        AddConnectionDefinitionLocal( CSM_CONNECTION_MGR_CRITICAL_PRIORITY );
        break;
      }

      case CSM_DAEMON_ROLE_AGENT:
      {
        _Aggregator = AddConnectionDefinitionComputeClient( CSM_CONNECTION_MGR_CRITICAL_PRIORITY + 1,
                                                            CSM_CONNECTION_MGR_CRITICAL_PRIORITY + 1 );
        _Master = _Aggregator;
        AddConnectionDefinitionLocal( CSM_CONNECTION_MGR_CRITICAL_PRIORITY );

        break;
      }

      case CSM_DAEMON_ROLE_UTILITY:
      {
        _Master = AddConnectionDefinitionUtilityClient( CSM_CONNECTION_MGR_CRITICAL_PRIORITY + 1 );
        _Aggregator = _Master;
        AddConnectionDefinitionLocal( CSM_CONNECTION_MGR_CRITICAL_PRIORITY );
        
        break;
      }

      default:
        throw csm::daemon::Exception("Unknown/invalid Role found during configuration.");
    }
    if( _EndpointDefinitionList.empty() )
      throw csm::daemon::Exception( "Initialization completed with empty list of critical connections/listeners.", ENOTCONN );
  }
  
  void
  Configuration::SetTweaks()
  {
    bool enabled = false;
    std::string uint_val = GetValueInConfig( std::string("csm.tuning.netmgr_poll_loops") );
    if( ! uint_val.empty() )
    {
      _Tweaks._NetMgr_polling_loops = std::stoi( uint_val );
      enabled = true;
    }
    else
    {
      if( _Role == CSM_DAEMON_ROLE_AGENT )
        _Tweaks._NetMgr_polling_loops = 10;
      else
        _Tweaks._NetMgr_polling_loops = 1000;
    }

    // Defaults for a maximum job length of 30 days are below
    // Attempt to limit samples using max_keep_samples, but set max_keep_age to be a multiple of this time span just in case 
    // Note: if a job runs for longer than MAX_JOB_IN_SECONDS, DCGM job stats collected will truncate the data collected
    // to the last MAX_JOB_IN_SECONDS seconds worth of data
    const uint64_t DEFAULT_UPDATE_INTERVAL_S(30);      // set in seconds here, it will be scaled later
    const uint32_t MAX_JOB_IN_SECONDS(60*60*24*15);

    uint_val = GetValueInConfig( std::string("csm.tuning.dcgm_update_interval_s") );
    if( ! uint_val.empty() )
    {
      _Tweaks._DCGM_update_interval_s = (uint64_t)std::stoi( uint_val );
      enabled = true;
    }
    else
      _Tweaks._DCGM_update_interval_s = DEFAULT_UPDATE_INTERVAL_S;
   
    // Using max_keep_samples to control sample purging in DCGM, max_keep_age should be set to a larger value 
    uint_val = GetValueInConfig( std::string("csm.tuning.dcgm_max_keep_age_s") );
    if( ! uint_val.empty() )
    {
      _Tweaks._DCGM_max_keep_age_s = (double)std::stoi( uint_val );
      enabled = true;
    }
    else
      _Tweaks._DCGM_max_keep_age_s = (MAX_JOB_IN_SECONDS*2);  // Include some margin of error based on max expected job length
    
    // Use max_keep_samples to control sample purging in DCGM
    // Set based on the configured update interval and maximum supported job length
    uint_val = GetValueInConfig( std::string("csm.tuning.dcgm_max_keep_samples") );
    if( ! uint_val.empty() )
    {
      _Tweaks._DCGM_max_keep_samples = (uint32_t)std::stoi( uint_val );
      enabled = true;
    }
    else
      _Tweaks._DCGM_max_keep_samples = (MAX_JOB_IN_SECONDS/_Tweaks._DCGM_update_interval_s); 

    if( enabled )
      CSMLOG( csmd, info ) << "CSMD Tuning enabled: " << _Tweaks;
  }

  HostNameConfigState_t
  Configuration::HostNameValidate( std::string host_val )
  {
    if(( host_val.empty() ) || ( host_val.compare( CONFIGURATION_HOSTNAME_NONE ) == 0 ))
      return HOST_CONFIG_NONE;

    // todo: could be improved by excluding more invalid characters
    if(( host_val.find(' ') != std::string::npos ) ||
        ( host_val.find('_') != std::string::npos ))
      return HOST_CONFIG_INVALID;

    return HOST_CONFIG_VALID;
  }

  void
  Configuration::SetBDS_Info()
  {
    bool enabled = true;
    bool inactive = false;
    if( _Role != CSM_DAEMON_ROLE_AGGREGATOR )
    {
      CSMLOG( csmd, warning ) << "BDS Info/Connection from " << _Role << " is not supported.";
      return;
    }

    std::string host_val = GetValueInConfig( std::string("csm.bds.host") );
    switch( HostNameValidate( host_val ) )
    {
      case HOST_CONFIG_VALID:
        break;
      case HOST_CONFIG_NONE:
        inactive = true;
        // no break on purpose: NONE -> inactive and not enabled
      case HOST_CONFIG_INVALID:
        enabled = false;
      default:
        break;
    }

    std::string port_val = GetValueInConfig( std::string("csm.bds.port") );
    if( port_val.empty() )
      enabled = false;

    std::string rci_max_val = GetValueInConfig( std::string("csm.bds.reconnect_interval_max") );
    if( rci_max_val.empty() )
      rci_max_val = "5";

    std::string dce_val = GetValueInConfig( std::string("csm.bds.data_cache_expiration") );

    if( enabled )
    {
      char *strend;
      errno = 0;
      unsigned rci_max = (unsigned)std::strtoul( rci_max_val.c_str(), &strend, 10 );
      if(( errno != 0 ) || (*strend != '\0' ) || ( rci_max_val.c_str()[0] == '-' ) )
      {
        CSMLOG( csmd, warning ) << "Invalid BDS configuration entry reconnect_interval_max. No attempts to access BDS will be made. ("
          << host_val << ":" << port_val << ")";
        return;
      }

      unsigned dce = 0;
      if( dce_val.empty() )
        dce = 600;
      else
      {
        errno = 0;
        dce = (unsigned)std::strtoul( dce_val.c_str(), &strend, 10 );
        if(( errno != 0 ) || (*strend != '\0' ) || ( dce_val.c_str()[0] == '-' ))
        {
          CSMLOG( csmd, warning ) << "Invalid BDS configuration entry data_cache_expiration. No attempts to access BDS will be made. ("
            << host_val << ":" << port_val << ")";
          return;
        }
      }

      if(( dce > 0 ) && ( dce < rci_max ))
        CSMLOG( csmd, warning ) << "BDS data cache expires faster than the maximum reconnection interval. This might cause data loss when BDS is restarted.";

      if( dce == 0 )
        CSMLOG( csmd, warning ) << "BDS data caching is disabled. (expiration set to 0)";

      _BDS_Info.Init( host_val, port_val, rci_max, dce );
      CSMLOG( csmd, info ) << "Configuring BDS access with: " << _BDS_Info.GetHostname() << ":" << _BDS_Info.GetPort()
          << " intervals: " << _BDS_Info.GetReconnectIntervalMax() << ":" << _BDS_Info.GetDataCacheExpiration();
    }
    else
    {
      if( inactive )
      {
        CSMLOG( csmd, info ) << "BDS access disabled by configuration.";
      }
      else
      {
        CSMLOG( csmd, warning ) << "Invalid or missing BDS configuration. No attempts to access BDS will be made. ("
          << host_val << ":" << port_val << ")";
      }
    }
  }

  void
  Configuration::SetRecurringTasks()
  {
    std::string ckey_str = GetValueInConfig( "csm.recurring_tasks.enabled" );
    boost::algorithm::to_lower( ckey_str );
    if(( ckey_str.empty() ) || ( ckey_str.compare( "true") != 0 ))
    {
      _Cron.Disable();
      CSMLOG( csmd, info ) << "Recurring tasks disabled or not defined in config. Disabling feature.";
    }
    else
    {
      _Cron.Enable();
      ckey_str = GetValueInConfig( "csm.recurring_tasks.soft_fail_recovery.enabled" );
      boost::algorithm::to_lower( ckey_str );
      bool sfenabled = (( ! ckey_str.empty() ) && ( ckey_str.compare( "true") == 0 ));
      if( ! sfenabled )
      {
        _Cron.Disable();
        CSMLOG( csmd, info ) << "Recurring task: Soft-failure recovery is disabled. Disabling recurring tasks entirely.";
        return;
      }

      ckey_str = GetValueInConfig( "csm.recurring_tasks.soft_fail_recovery.interval" );
      boost::posix_time::time_duration interval_time = boost::posix_time::duration_from_string(ckey_str);
      unsigned interval = interval_time.total_seconds();

      ckey_str = GetValueInConfig( "csm.recurring_tasks.soft_fail_recovery.retry" );
      unsigned retry = strtol( ckey_str.c_str(), nullptr, 10 );

      if(( interval == 0 ) || ( retry == 0 ))
      {
        _Cron.Disable();
        throw csm::daemon::Exception("Recurring Tasks configuration error. Interval/Retry invalid." );
      }

      _Cron.SetSoftFailRecovery( interval, retry, sfenabled );

      _Cron.UpdateLCM();
    }

    if( _Cron.IsEnabled() )
      CSMLOG( csmd, info ) << "Recurring tasks config: " << _Cron;
  }
    
    std::string Configuration::ParseHexString(std::string hexStr)
    {
        std::string bitMapStr = "";
        std::string hexStrLower = hexStr;
        boost::algorithm::to_lower(hexStrLower);

        bool success       = true; 
        size_t strLen = hexStrLower.size();

        for ( size_t i = 0; success && i < strLen; ++i )
        {
            switch ( hexStrLower[i] ) 
            {
                case '0': { bitMapStr.append("0000"); break; } 
                case '1': { bitMapStr.append("0001"); break; } 
                case '2': { bitMapStr.append("0010"); break; }
                case '3': { bitMapStr.append("0011"); break; }
                case '4': { bitMapStr.append("0100"); break; }
                case '5': { bitMapStr.append("0101"); break; }
                case '6': { bitMapStr.append("0110"); break; }
                case '7': { bitMapStr.append("0111"); break; }
                case '8': { bitMapStr.append("1000"); break; }
                case '9': { bitMapStr.append("1001"); break; }
                case 'a': { bitMapStr.append("1010"); break; }
                case 'b': { bitMapStr.append("1011"); break; }
                case 'c': { bitMapStr.append("1100"); break; }
                case 'd': { bitMapStr.append("1101"); break; }
                case 'e': { bitMapStr.append("1110"); break; }
                case 'f': { bitMapStr.append("1111"); break; }
                default: success = false; break;
            }
        }

        if ( !success ) bitMapStr ="";
            
        return bitMapStr;
    }

    void Configuration::LoadJitterMitigation ()
    {
        const std::string SECTION    = "csm.jitter_mitigation.";
        const std::string SYSTEM_MAP = SECTION + "system_map";
        const std::string SYSTEM_SMT = SECTION + "system_smt";
        const std::string IRQ_MAP    = SECTION + "irq_affinity";
        const std::string CORE_ISO   = SECTION + "core_isolation";
        const std::string CORE_BLINK = SECTION + "core_blink";  
        std::string keyStr = "";

        // Parse core mappings.
        std::string systemMap      = ParseHexString(GetValueInConfig(SYSTEM_MAP));

        // Parse SMT for system.
        keyStr = GetValueInConfig(SYSTEM_SMT);
        int32_t systemSMT      = std::strtol(keyStr.c_str(), nullptr, 10);

        keyStr     = GetValueInConfig(IRQ_MAP);
        boost::algorithm::to_lower(keyStr);
        bool irqAffinity=  ( keyStr.empty()  || (keyStr.compare("true") == 0) );

        /*
        // If the core isolation flag is unset default to true.
        keyStr     = GetValueInConfig(CORE_ISO);
        boost::algorithm::to_lower(keyStr);
        bool coreIsolation  =  ( keyStr.empty()  || (keyStr.compare("true") == 0) );

        // If the Core Blink flag is unset default to true.
        keyStr     = GetValueInConfig( CORE_BLINK ) ;
        boost::algorithm::to_lower(keyStr);
        bool coreBlink =  ( keyStr.empty()  || (keyStr.compare("true") == 0) );
        */

        // Build the object.
        _JitterInfo.Init(systemMap, systemSMT, irqAffinity);
    } 

}  // namespace daemon
} // namespace csm

#undef logprefix

