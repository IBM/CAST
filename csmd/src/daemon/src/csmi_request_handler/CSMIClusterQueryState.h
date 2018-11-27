/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIClusterQueryState.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSMI_CLUSTER_QUERY_STATE_H__
#define __CSMI_CLUSTER_QUERY_STATE_H__

#include "csmi_stateful_db.h"

class CSMIClusterQueryState : public CSMIStatefulDB { 

public:
    CSMIClusterQueryState(csm::daemon::HandlerOptions& options) : 
        CSMIStatefulDB(CSM_CMD_cluster_query_state,options) { }
  
    virtual bool CreatePayload(
        const std::string& stringBuffer,
        const uint32_t bufferLength,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr& ctx ) final;

    virtual bool CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **stringBuffer,
		uint32_t &bufferLength,
        csm::daemon::EventContextHandlerState_sptr& ctx ) final;

    void CreateOutputStruct(
        csm::db::DBTuple * const & fields,
        csmi_cluster_query_state_record_t ** output );
};

#endif
