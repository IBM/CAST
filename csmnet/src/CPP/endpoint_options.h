/*================================================================================

    csmnet/src/CPP/endpoint_options.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMNET_SRC_CPP_ENDPOINT_OPTIONS_H_
#define CSMNET_SRC_CPP_ENDPOINT_OPTIONS_H_

#include <sys/types.h>
#include <grp.h>

#include "csmd/src/daemon/include/csm_api_acl.h"
#include "csmd/src/daemon/include/csm_api_config.h"

#include "csm_message_and_address.h"

namespace csm {
namespace network {

class EndpointOptions {
public:
  bool _IsServer;
  EndpointOptions( const bool aIsServer = false ) : _IsServer( aIsServer ) {}
  virtual ~EndpointOptions( ) {};
};
typedef std::shared_ptr<csm::network::EndpointOptions> EndpointOptions_sptr;


class EndpointOptionsUnix : public EndpointOptions {
public:
  std::string _ServerPath;   // optional entry for clients to find server
  gid_t _Permissions;     // access permissions of socket file
  std::string _Group;         // groupID of socket file (defaults to current user if 0
  csm::daemon::CSMIAuthList_sptr _AuthList; // optional object storing the api permission info
  csm::daemon::CSMAPIConfig_sptr _APIConfig; // stores the API timeouts and potential other settings per API

  EndpointOptionsUnix( const bool aIsServer,
                       const mode_t aPermissions = 0777,
                       const std::string aGroup = std::string( getgrgid( getegid() )->gr_name ))
  : EndpointOptions( aIsServer ),
    _ServerPath( std::string("") ),
    _Permissions( aPermissions ),
    _Group( aGroup ),
    _AuthList(),
    _APIConfig()
  {}

  EndpointOptionsUnix( const bool aIsServer,
                       const csm::daemon::CSMIAuthList_sptr aAuthList,
                       const csm::daemon::CSMAPIConfig_sptr aAPIConfig,
                       const mode_t aPermissions = 0777,
                       const std::string aGroup = std::string( getgrgid( getegid() )->gr_name ))
  : EndpointOptions( aIsServer ),
    _ServerPath( std::string("") ),
    _Permissions( aPermissions ),
    _Group( aGroup ),
    _AuthList(aAuthList),
    _APIConfig(aAPIConfig)
  {}
  
  EndpointOptionsUnix( const char *aServerPath)
  : EndpointOptions( false ),
    _ServerPath( std::string( aServerPath ) ),
    _Permissions( 0700 ),
    _Group( std::string( getgrgid( getegid() )->gr_name ) ),
    _AuthList(),
    _APIConfig()
  {}
  EndpointOptionsUnix( const std::string &aServerPath )
  : EndpointOptions( false ),
    _ServerPath( aServerPath ),
    _Permissions( 0700 ),
    _Group( std::string( getgrgid( getegid() )->gr_name ) ),
    _AuthList(),
    _APIConfig()
  {}
  EndpointOptionsUnix( const csm::network::AddressUnix *aSrvAddr )
  : EndpointOptions( false ),
    _ServerPath( std::string( aSrvAddr->_SockAddr.sun_path ) ),
    _Permissions( 0700 ),
    _Group( std::string( getgrgid( getegid() )->gr_name ) ),
    _AuthList(),
    _APIConfig()
  {}
  
  virtual ~EndpointOptionsUnix() {}
};
typedef std::shared_ptr<csm::network::EndpointOptionsUnix> EndpointOptionsUnix_sptr;

typedef EndpointOptionsUnix EndpointOptionsDual;
typedef std::shared_ptr<csm::network::EndpointOptionsDual> EndpointOptionsDual_sptr;

typedef struct
{
  std::string _CAFile;
  std::string _CredPem;
} SSLFilesCollection;

class EndpointOptionsMQ : public EndpointOptions {
public:
  std::string _MyId;
  int _Keepalive;
  csm::network::SSLFilesCollection _SSLFiles;

  EndpointOptionsMQ( const std::string aMyId,
                     const int aKeepalive,
                     const SSLFilesCollection &aSSLFiles )
  : EndpointOptions( false ), // our MQTT endpoints are always clients
    _MyId( aMyId ),
    _Keepalive( aKeepalive ),
    _SSLFiles( aSSLFiles )
  {}
  virtual ~EndpointOptionsMQ() {}
  bool HasSSLInfo() const { return !( _SSLFiles._CAFile.empty() || ( _SSLFiles._CredPem.empty() ) ); }
};
typedef std::shared_ptr<csm::network::EndpointOptionsMQ> EndpointOptionsMQ_sptr;

class EndpointOptionsPTP_base : public EndpointOptions {
public:
  uint32_t _HeartbeatInterval;
  bool _SecondaryConnection;
  csm::network::SSLFilesCollection _SSLFiles;

  EndpointOptionsPTP_base( const bool aIsServer,
                           const csm::network::SSLFilesCollection &aSSLFiles,
                           const bool aSecondary,
                           const uint32_t aHeartbeatInterval )
  : EndpointOptions( aIsServer ),
    _HeartbeatInterval( aHeartbeatInterval ),
    _SecondaryConnection( aSecondary ),
    _SSLFiles( aSSLFiles )
  {}

  bool IsSecondary() const { return _SecondaryConnection; }
  uint32_t getHeartbeatInterval() const { return _HeartbeatInterval; }
  bool HasSSLInfo() const { return !( _SSLFiles._CAFile.empty() || ( _SSLFiles._CredPem.empty() ) ); }
};

template<typename AddressClass>
class EndpointOptionsPTP : public EndpointOptionsPTP_base {
public:
  AddressClass _ServerAddr;

  EndpointOptionsPTP( const bool aIsServer, const csm::network::SSLFilesCollection &aSSLFiles,
                      const bool aSecondary = false,
                      const uint32_t aHeartbeatInterval = CSM_NETWORK_HEARTBEAT_INTERVAL )
  : EndpointOptionsPTP_base( aIsServer, aSSLFiles, aSecondary, aHeartbeatInterval ),
    _ServerAddr(0,0)
  {}
  EndpointOptionsPTP( const bool aIsServer, const bool aSecondary = false,
                      const uint32_t aHeartbeatInterval = CSM_NETWORK_HEARTBEAT_INTERVAL )
  : EndpointOptionsPTP_base( aIsServer, csm::network::SSLFilesCollection(), aSecondary, aHeartbeatInterval ),
    _ServerAddr(0,0)
  {}
  EndpointOptionsPTP( const uint32_t aIP,
                      const in_port_t aPort,
                      const csm::network::SSLFilesCollection &aSSLFiles,
                      const bool aSecondary = false,
                      const uint32_t aHeartbeatInterval = CSM_NETWORK_HEARTBEAT_INTERVAL )
  : EndpointOptionsPTP_base( false, aSSLFiles, aSecondary, aHeartbeatInterval ),
    _ServerAddr( aIP, aPort )
  {}
  EndpointOptionsPTP( const AddressClass &aServerAddr,
                      const bool aSecondary = false,
                      const uint32_t aHeartbeatInterval = CSM_NETWORK_HEARTBEAT_INTERVAL )
  : EndpointOptionsPTP_base( false, csm::network::SSLFilesCollection(), aSecondary, aHeartbeatInterval ),
    _ServerAddr( aServerAddr )
  {}
  EndpointOptionsPTP( const AddressClass *aServerAddr,
                      const bool aSecondary = false,
                      const uint32_t aHeartbeatInterval = CSM_NETWORK_HEARTBEAT_INTERVAL )
  : EndpointOptionsPTP_base( false, csm::network::SSLFilesCollection(), aSecondary, aHeartbeatInterval ),
    _ServerAddr( *aServerAddr )
  {}
  virtual ~EndpointOptionsPTP() {}
};
using EndpointOptionsPTP_base_sptr = std::shared_ptr<EndpointOptionsPTP_base>;
using EndpointOptionsPTP_sptr = std::shared_ptr<EndpointOptionsPTP<AddressPTP>>;
using EndpointOptionsUtility_sptr = std::shared_ptr<EndpointOptionsPTP<AddressUtility>>;
using EndpointOptionsAggregator_sptr = std::shared_ptr<EndpointOptionsPTP<AddressAggregator>>;

} // namespace csm::network
}

#endif /* CSMNET_SRC_CPP_ENDPOINT_OPTIONS_H_ */
