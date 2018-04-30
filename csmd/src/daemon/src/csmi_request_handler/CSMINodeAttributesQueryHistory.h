/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMINodeAttributesQueryHistory.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/*
* Author: Nick Buonarota
* Email: nbuonar@us.ibm.com
*/
#ifndef __CSMI_NODE_ATTRIBUTES_QUERY_HISTORY_H__
#define __CSMI_NODE_ATTRIBUTES_QUERY_HISTORY_H__

#include "csmi_stateful_db.h"

class CSMINodeAttributesQueryHistory : public CSMIStatefulDB {

public:
	CSMINodeAttributesQueryHistory(csm::daemon::HandlerOptions& options) : 
        CSMIStatefulDB(CSM_CMD_node_attributes_query_history, options) { }

    virtual bool CreatePayload(
        const std::string& stringBuffer,
        const uint32_t bufferLength,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr ctx ) final;

    virtual bool CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **stringBuffer,
		uint32_t &bufferLength,
        csm::daemon::EventContextHandlerState_sptr ctx ) final;

    void CreateOutputStruct(
        csm::db::DBTuple * const & fields, 
        csmi_node_attributes_history_record_t **output );
};

#endif
