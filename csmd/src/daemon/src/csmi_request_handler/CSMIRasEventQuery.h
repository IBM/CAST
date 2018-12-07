/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIRasEventQuery.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSMI_RAS_EVENT_QUERY_H__
#define __CSMI_RAS_EVENT_QUERY_H__

#include "csmi_stateful_db.h"
#include <csmd/src/ras/include/RasEventHandlerChain.h>

class CSMIRasEventQuery : public CSMIStatefulDB {

public:
	CSMIRasEventQuery(csm::daemon::HandlerOptions &options):CSMIStatefulDB(CSM_CMD_ras_event_query, options)
	{
	  
	}

	// COMPOSE SQL STATEMENT
	// if error occurs when composing a SQL stmt, this call will return false and errcode
	// and errmsg, bool compareDataForPrivateCheckRes=false shall be set with error info
	virtual bool CreatePayload(
		const std::string& stringBuffer,
		const uint32_t bufferLength,
		csm::db::DBReqContent **dbPayload,
		csm::daemon::EventContextHandlerState_sptr& ctx ) final;
		
	//DEAL WITH RETURNS FROM SQL QUERY
	virtual bool CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char** stringBuffer, 
		uint32_t &bufferLength,
        csm::daemon::EventContextHandlerState_sptr& ctx ) final;	
		
	
		
	//Ralph temp function
	//ToDo: remove
    void addWhere(std::ostringstream &where, const std::string &f, const char *v);

	/** @brief Creates an allocation structure using the supplied tuples.
     *  FIXME  THIS IS DUPLICATED CODE!
     *
     *  @param fields The fields containing the allocation query results.
     *  @param output The returned output.
     *  
     */
	void CreateOutputStruct(
        csm::db::DBTuple * const & fields, 
        csmi_ras_type_record_t** output );
		
	private:
		std::string trim(const std::string& str);
};

#endif
