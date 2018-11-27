/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIAllocationStepEnd.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/*
* Author: John Dunham, Nick Buonarota
* Email: jdunham@us.ibm.com, nbuonar@us.ibm.com
*/
#ifndef __CSMI_ALLOCATION_STEP_END_H__
#define __CSMI_ALLOCATION_STEP_END_H__

#include "csmi_stateful_db.h"
#include "csmi_mcast/CSMIMcastStep.h"

class CSMIAllocationStepEnd : public CSMIStatefulDB {

public:
	CSMIAllocationStepEnd(csm::daemon::HandlerOptions& options);

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

    static bool ParseInfoQuery(
        csm::daemon::EventContextHandlerState_sptr& ctx,
        const std::vector<csm::db::DBTuple *>& tuples,
        CSMIStepMcast* mcastProps);

    virtual bool CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr& ctx ) final;

    static bool CreateByteArray(
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr& ctx );

    static inline csm::db::DBReqContent* BadQuery(
        csm::daemon::EventContextHandlerState_sptr& ctx,
        CSMIStepMcast* mcastProps)
    {
        return nullptr;
    }
};


class CSMIAllocationStepEnd_Agent :  public CSMIStateful
{
    public:
        CSMIAllocationStepEnd_Agent( csm::daemon::HandlerOptions& options );
};
#endif
