/*================================================================================
    csmd/src/daemon/src/csmi_request_handler/CSMIAllocationUpdateState.h
    
    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
    
#ifndef __CSMI_ALLOCATION_UPDATE_STATE_H__
#define __CSMI_ALLOCATION_UPDATE_STATE_H__

#include "csmi_stateful_db.h"
#include "csmi_mcast/CSMIMcastAllocation.h"

/**
 *@brief todo
 */
class CSMIAllocationUpdateState : public CSMIStatefulDB
{
public:
    CSMIAllocationUpdateState(csm::daemon::HandlerOptions& options) ;

	virtual bool CompareDataForPrivateCheck(
        const std::vector<csm::db::DBTuple *>& tuples,
        const csm::network::Message &msg,
        csm::daemon::EventContextHandlerState_sptr& ctx) final;
	
	virtual bool RetrieveDataForPrivateCheck(
        const std::string& arguments, 
        const uint32_t len, 
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr& ctx ) final;

    virtual bool CreatePayload(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr& ctx ) final;

    static csm::db::DBReqContent* InsertStatsStatement(
        csm::daemon::EventContextHandlerState_sptr& ctx,
        CSMIMcastAllocation* mcastProps);

    static bool ParseInfoQuery(
        csm::daemon::EventContextHandlerState_sptr& ctx,
        const std::vector<csm::db::DBTuple *>& tuples,
        CSMIMcastAllocation* mcastProps);

    static csm::db::DBReqContent* UpdateAllocation(
        csm::daemon::EventContextHandlerState_sptr& ctx,
        CSMIMcastAllocation* mcastProps);

    virtual bool CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr& ctx ) final;

    static bool CreateByteArray(
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr& ctx );
    
    // A terminal function for the update state API.
    static bool  UpdateTerminal(
        const std::vector<csm::db::DBTuple *>& tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr& ctx );

    static inline csm::db::DBReqContent* NULLDBResp(
        csm::daemon::EventContextHandlerState_sptr& ctx,
        CSMIMcastAllocation* mcastProps){ return nullptr; }

    static csm::db::DBReqContent* MCASTDBReqSpawn(
        csm::daemon::EventContextHandlerState_sptr& ctx,
        CSMIMcastAllocation* mcastProps);

    static csm::db::DBReqContent* MCASTDBReqDeleteResponse(
        csm::daemon::EventContextHandlerState_sptr& ctx,
        CSMIMcastAllocation* mcastProps);
};

class CSMIAllocationUpdateState_Agent : public CSMIStateful
{
public:
    CSMIAllocationUpdateState_Agent( csm::daemon::HandlerOptions& options ); 
};


#endif 
