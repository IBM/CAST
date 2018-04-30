/*================================================================================

    csmnet/src/CPP/network_ctrl_path.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_NETWORK_CTRL_PATH_H_
#define CSMD_SRC_DAEMON_INCLUDE_NETWORK_CTRL_PATH_H_

#include <queue>
#include <mutex>

#include "address.h"
#include "csm_version_msg.h"

namespace csm {
namespace network {

typedef enum
{
  NET_CTL_UNVERIFIED,
  NET_CTL_CONNECT,
  NET_CTL_TIMEOUT,
  NET_CTL_DISCONNECT,
  NET_CTL_FAILOVER,
  NET_CTL_RESTARTED,
  NET_CTL_FATAL,
  NET_CTL_OTHER,
  NET_CTL_MAX
} NetworkCtrlEventType;

template<class stream>
static stream&
operator<<( stream &out, const csm::network::NetworkCtrlEventType &aType )
{
  switch( aType )
  {
    case NET_CTL_CONNECT:    out << "NET_CTL_CONNECT";    break;
    case NET_CTL_UNVERIFIED: out << "NET_CTL_UNVERIFIED"; break;
    case NET_CTL_TIMEOUT:    out << "NET_CTL_TIMEOUT";    break;
    case NET_CTL_DISCONNECT: out << "NET_CTL_DISCONNECT"; break;
    case NET_CTL_FAILOVER:   out << "NET_CTL_FAILOVER";   break;
    case NET_CTL_RESTARTED:  out << "NET_CTL_RESTARTED";  break;
    case NET_CTL_FATAL:      out << "NET_CTL_FATAL";      break;
    case NET_CTL_OTHER:      out << "NET_CTL_OTHER";      break;
    default:                 out << "ERROR: !!!OUT-OF-RANGE!!!";
  }
  return (out);
}

class NetworkCtrlInfo
{
public:
  NetworkCtrlInfo *_Next;
  const csm::network::Address_sptr _Address;
  uint32_t _MsgId;
  NetworkCtrlEventType _Type;
  VersionStruct _VersionData;

  NetworkCtrlInfo( )
  : _Next( nullptr ),
    _Address( nullptr ),
    _MsgId( 0 ),
    _Type( NET_CTL_OTHER ),
    _VersionData()
  { }

  NetworkCtrlInfo( const NetworkCtrlEventType i_Type,
                   const uint32_t i_MsgId,
                   const csm::network::Address_sptr i_Address,
                   const csm::network::NetworkCtrlInfo *i_Next = nullptr,
                   const csm::network::VersionStruct *i_VersionData = nullptr )
  : _Next( (csm::network::NetworkCtrlInfo *)i_Next ),
    _Address( i_Address ),
    _MsgId( i_MsgId ),
    _Type( i_Type ),
    _VersionData( i_VersionData != nullptr ? *i_VersionData : VersionStruct() )
  { }

  NetworkCtrlInfo( const NetworkCtrlInfo &i_Info )
  : _Next( i_Info._Next ),
    _Address( i_Info._Address ),
    _MsgId( i_Info._MsgId ),
    _Type( i_Info._Type ),
    _VersionData( i_Info._VersionData )
  { }

  ~NetworkCtrlInfo()
  {
    _Next = nullptr;
  }
};

typedef std::shared_ptr<csm::network::NetworkCtrlInfo> NetworkCtrlInfo_sptr;
typedef std::queue<csm::network::NetworkCtrlInfo_sptr> NetworkCtrlQueue;

class NetworkCtrlPath
{
  NetworkCtrlQueue _CtrlEvents;
  std::mutex _Lock;

public:
  NetworkCtrlPath();
  ~NetworkCtrlPath();

  void AddCtrlEvent( const csm::network::NetworkCtrlInfo_sptr i_Info );
  void AddCtrlEvent( const NetworkCtrlEventType i_Type, const uint32_t i_MsgId );
  void AddCtrlEvent( const csm::network::NetworkCtrlEventType i_Type,
                     const csm::network::Address_sptr i_Address,
                     const csm::network::VersionStruct *i_VersionData = nullptr );

  const csm::network::NetworkCtrlInfo_sptr GetCtrlEvent();

  void PushBack( csm::network::NetworkCtrlInfo *i_List );

  uint32_t CtrlEventCount() const { return _CtrlEvents.size(); }
};

typedef std::shared_ptr<NetworkCtrlPath> NetworkCtrlPath_sptr;

}  // namespace daemon
} // namespace csm



#endif /* CSMD_SRC_DAEMON_INCLUDE_NETWORK_CTRL_PATH_H_ */
