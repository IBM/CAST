/*================================================================================

    csmd/src/daemon/include/csm_connection_definition.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_CSM_CONNECTION_DEFINITION_H_
#define CSMD_SRC_DAEMON_INCLUDE_CSM_CONNECTION_DEFINITION_H_

#include <utility>
#include <vector>
#include <set>

#include "csmnet/src/CPP/address.h"
#include "csmnet/src/CPP/endpoint_options.h"
#include "include/csm_connection_type.h"
#include "include/csm_bucket_item.h"

namespace csm {
namespace daemon {

/*
 *  any priority that's below this is not considered critical enough for the
 *  connection mgr to cause a disconnect
 */
#define CSM_CONNECTION_MGR_CRITICAL_PRIORITY (1)


/*
 * the highest priority available (required for the conn mgr to start connections with highest prio first)
 */
#define CSM_CONNECTION_MGR_MAX_PRIORITY ( CSM_CONNECTION_MGR_CRITICAL_PRIORITY + 4 )

class ConnectionDefinition
{
public:
  csm::network::Address_sptr _Addr;
  csm::network::EndpointOptions_sptr _Opts;
  csm::daemon::ConnectionType::CONN_TYPE _Type;
  std::string _HostName;
  int _Priority;
  ConnectionDefinition( csm::network::Address_sptr i_Addr,
                        csm::network::EndpointOptions_sptr i_Opts,
                        const int i_Prio,
                        const std::string i_HostName,
                        const ConnectionType::CONN_TYPE i_Type = ConnectionType::SINGLE )
  : _Addr( i_Addr ),
    _Opts( i_Opts ),
    _Type( i_Type ),
    _HostName( i_HostName ),
    _Priority( i_Prio )
  {
    if( _Priority > CSM_CONNECTION_MGR_MAX_PRIORITY )
      _Priority = CSM_CONNECTION_MGR_MAX_PRIORITY;
  }
};

typedef std::vector<csm::daemon::ConnectionDefinition> ConnectionDefinitionList;
typedef std::pair<int, std::set<BucketItemType>> FreqAndBucketItems;


}
}

#endif /* CSMD_SRC_DAEMON_INCLUDE_CSM_CONNECTION_DEFINITION_H_ */
