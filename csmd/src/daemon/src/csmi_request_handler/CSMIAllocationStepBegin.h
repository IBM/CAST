/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIAllocationStepBegin.h

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
#ifndef __CSMI_ALLOCATION_STEP_BEGIN_H__
#define __CSMI_ALLOCATION_STEP_BEGIN_H__

#include "csmi_stateful_db.h"
#include "csmi_mcast/CSMIMcastStep.h"


class CSMIAllocationStepBegin : public CSMIStatefulDB {

public:
	CSMIAllocationStepBegin(csm::daemon::HandlerOptions& options);

    virtual bool CreatePayload(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr ctx ) final;

    static csm::db::DBReqContent* UndoStepDB(
        csm::daemon::EventContextHandlerState_sptr ctx,
        CSMIStepMcast * mcastProps);

    static inline csm::db::DBReqContent* BadQuery(
        csm::daemon::EventContextHandlerState_sptr ctx,
        CSMIStepMcast * mcastProps)
    {
        return nullptr;
    }
    
    static bool UndoTerminal( const std::vector<csm::db::DBTuple *>&tuples, 
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr ctx );

    virtual bool CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr ctx ) final;

    static bool CreateByteArray(
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr ctx );


    static bool ParseInfoQuery(
        csm::daemon::EventContextHandlerState_sptr ctx,
        const std::vector<csm::db::DBTuple *>& tuples,
        CSMIStepMcast* mcastProps);
};

class CSMIAllocationStepBegin_Agent :  public CSMIStateful
{
    public:
        CSMIAllocationStepBegin_Agent( csm::daemon::HandlerOptions& options );

};
#endif
