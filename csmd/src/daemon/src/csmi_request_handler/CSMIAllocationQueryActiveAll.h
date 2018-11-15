/*================================================================================
    csmd/src/daemon/src/csmi_request_handler/CSMIAllocationQueryActiveAll.h
    
    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
    
#ifndef  __CSMI_ALLOCATION_QUERY_ACTIVE_ALL_H__
#define  __CSMI_ALLOCATION_QUERY_ACTIVE_ALL_H__

#include "csmi_stateful_db.h"

class CSMIAllocationQueryActiveAll : public CSMIStatefulDB
{
public:
    CSMIAllocationQueryActiveAll(csm::daemon::HandlerOptions& options) : 
        CSMIStatefulDB(CSM_CMD_allocation_query_active_all, options) {}

    virtual bool CreatePayload(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr& ctx ) final;
   
    virtual bool CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr& ctx ) final;
   
private:

    /** @brief Creates an allocation structure using the supplied tuples.
     *  FIXME  THIS IS DUPLICATED CODE!
     *
     *  @param fields The fields containing the allocation query results.
     *  @param allocation The returned allocation.
     *  @param history_flag Flags whether or not to add history data.
     *  
     *  @return error code.
     */
    void CreateOutputStruct(csm::db::DBTuple * const & fields,
                                csmi_allocation_t **allocation);
};

#endif 
