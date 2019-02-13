/*================================================================================
   
    csmd/src/daemon/src/csmi_request_handler/InvSsdWearUpdate.h

  © Copyright IBM Corporation 2015-2019. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#ifndef __INV_SSD_WEAR_UPDATE_H__
#define __INV_SSD_WEAR_UPDATE_H__

#include "csmi_db_base.h"

class InvSsdWearUpdate : public CSMI_DB_BASE {

public:
   InvSsdWearUpdate(csm::daemon::HandlerOptions& options) : CSMI_DB_BASE(CSM_CMD_ssd_wear_update,options)
   {
   };

   void Process(const csm::daemon::CoreEvent &aEvent, std::vector<csm::daemon::CoreEvent*>& postEventList);
 
private:

   /**
    * Parses the original network event received and adds a DB Request to the postEventList 
    * Returns true is successful; event will be added to the postEventList
    * Returns false if unsuccessful or no updates needed; no event added to the postEventList
    */
   bool CreateDbRequest(const EventContextDBBase_sptr& ctx, const csm::daemon::CoreEvent& aEvent, std::vector<csm::daemon::CoreEvent*>& postEventList);

   bool CreateSqlStmt(const std::string& arguments, const uint32_t len,
                std::string &stmt, int &errcode, std::string &errmsg, bool compareDataForPrivateCheckRes=false);

   // return the error code defined in csmi/src/common/include/csmi_cmd_error.h
   int CreateByteArray(std::vector<csm::db::DBTuple *>&tuples, 
                                  char **buf, uint32_t &bufLen, bool compareDataForPrivateCheckRes=false);
};

#endif

