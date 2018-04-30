/*================================================================================

    csmd/src/daemon/include/csm_daemon_role.h

 * © Copyright IBM Corporation 2017. All Rights Reserved
 *
 *  author: Lars Schneidenbach (schneidenbach@us.ibm.com)
 * created: Oct 16, 2017
 *
 ******************************************/

#ifndef CSMD_SRC_DAEMON_INCLUDE_CSM_DAEMON_ROLE_H_
#define CSMD_SRC_DAEMON_INCLUDE_CSM_DAEMON_ROLE_H_

#include <string>

enum CSMDaemonRole {
  CSM_DAEMON_ROLE_UNKNOWN = 0,
  CSM_DAEMON_ROLE_MASTER = 1,
  CSM_DAEMON_ROLE_AGGREGATOR = 2,
  CSM_DAEMON_ROLE_AGENT = 3,
  CSM_DAEMON_ROLE_UTILITY = 4,
  CSM_DAEMON_ROLE_MAX = 5
};

// The string returned here for each role is the value we expect users to use
// in csm.role in Configuration. Therefore, if any change to MASTER/AGGREGATOR/AGENT/UTILITY,
// it will affect the expected string value in csm.role.
static inline
std::string CSMDaemonRole_to_string( const CSMDaemonRole &aRole )
{
  switch( aRole )
  {
    case CSM_DAEMON_ROLE_UNKNOWN:     return std::string( "UNKNOWN" );    break;
    case CSM_DAEMON_ROLE_MASTER:      return std::string( "MASTER" );     break;
    case CSM_DAEMON_ROLE_AGGREGATOR:  return std::string( "AGGREGATOR" ); break;
    case CSM_DAEMON_ROLE_AGENT:       return std::string( "COMPUTE" );    break;
    case CSM_DAEMON_ROLE_UTILITY:     return std::string( "UTILITY" );    break;
    default:                          return std::string( "ROLE OUT OF RANGE" );   break;
  }
}




#endif /* CSMD_SRC_DAEMON_INCLUDE_CSM_DAEMON_ROLE_H_ */
