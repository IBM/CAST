/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csm_handler_options.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef CSM_DAEMON_SRC_CSM_HANDLER_OPTIONS_H_
#define CSM_DAEMON_SRC_CSM_HANDLER_OPTIONS_H_

#include "include/csm_daemon_state.h"
#include "include/csm_api_acl.h"

#include "csm_daemon_config.h"

namespace csm {
namespace daemon {

class HandlerOptions
{
public:
  HandlerOptions()
  { }
  
  HandlerOptions(const HandlerOptions &aSrc)
  { }
  
  virtual ~HandlerOptions() { }
  
  static inline void Init()
  {
    csmConfig = csm::daemon::Configuration::Instance();
    if (csmConfig)
    {
      _DBConnectionPool = csmConfig->GetDBConnectionPool();
      _DaemonState = csmConfig->GetDaemonState();
      _DaemonRole = csmConfig->GetRole();
      _AuthList = csmConfig->GetPermissionClass();
    }
  }
  static inline bool HasValidDBConn()
  {
    UpdateDBConnPool();
    return (_DBConnectionPool && _DBConnectionPool->GetNumOfDBConnections() > 0) ;
  }
  
  // \note: callers need to check if the return value is nullptr.
  // the number of free connections may be running out
  static inline csm::db::DBConnection_sptr AcquireDBConnection()
  {
    UpdateDBConnPool();
    if (_DBConnectionPool)
      return _DBConnectionPool->AcquireDBConnection();
    else
      return nullptr;
  }
  
  static inline void ReleaseDBConnection(csm::db::DBConnection_sptr aConn)
  {
    UpdateDBConnPool();
    if (_DBConnectionPool && aConn)
      _DBConnectionPool->ReleaseDBConnection(aConn);
  }
  
  static inline unsigned GetNumOfFreeDBConnections()
  {
    UpdateDBConnPool();
    if (_DBConnectionPool)
      return _DBConnectionPool->GetNumOfFreeDBConnections();
    else
      return 0;
  }

  static inline unsigned GetNumOfLockedDBConnections()
  {
    UpdateDBConnPool();
    if (_DBConnectionPool)
      return _DBConnectionPool->GetNumOfLockedDBConnections();
    else
      return 0;
  }
  
  static inline csm::daemon::DaemonState *GetDaemonState() { return _DaemonState; }
  static inline CSMDaemonRole GetRole() { return _DaemonRole; }
  
  static inline API_SEC_LEVEL GetSecurityLevel(csmi_cmd_t i_cmd)
  {
    if (_AuthList) return _AuthList->GetSecurityLevel(i_cmd);
    else return PRIVILEGED;
  }
  
  static inline bool HasPrivilegedAccess(uid_t i_uid, gid_t i_gid)
  {
    if (_AuthList) return _AuthList->HasPrivilegedAccess(i_uid, i_gid);
    else return false;
  }
  
  HandlerOptions& operator=( const csm::daemon::HandlerOptions &aIn )
  {
    return *this;
  }

private:
  static std::string _mqReqTopic;
  static std::string _mqReplyTopic;
  static std::string _rasTopicPrefix;
  static csm::db::DBConnectionPool * _DBConnectionPool;
  static csm::daemon::DaemonState* _DaemonState;
  static CSMDaemonRole _DaemonRole;
  static csm::daemon::CSMIAuthList_sptr _AuthList;

  static csm::daemon::Configuration *csmConfig;

  // HandlerOptions is initialized before DBConnectionPool
  // update if still nullptr
  static inline void UpdateDBConnPool()
  {
    if(nullptr == _DBConnectionPool)
      _DBConnectionPool = csmConfig->GetDBConnectionPool();
  }
};


} // end daemon
} // end csm

#endif
