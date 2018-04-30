/*================================================================================
   
    csmd/src/daemon/src/csmi_request_handler/InvGetNodeInventory.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#ifndef __INV_GET_NODE_INVENTORY_H__
#define __INV_GET_NODE_INVENTORY_H__

#include "csmi_base.h"
#include "csmi_db_base.h"

class InvGetNodeInventory : public CSMI_DB_BASE {

public:
  InvGetNodeInventory(csm::daemon::HandlerOptions& options)
  : CSMI_DB_BASE(CSM_CMD_INV_get_node_inventory,options)
  {
    _SystemEventContext = RegisterSystemEvent( this, csm::daemon::SystemContent::DISCONNECTED );
  };
  
private:

  bool CreateSqlStmt(const std::string& arguments, const uint32_t len,
                std::string &stmt, int &errcode, std::string &errmsg, bool compareDataForPrivateCheckRes=false);

  // return the error code defined in csmi/src/common/include/csmi_cmd_error.h
  virtual int CreateByteArray(std::vector<csm::db::DBTuple *>&tuples, 
                                  char **buf, uint32_t &bufLen, bool compareDataForPrivateCheckRes=false);
  
  virtual void Process( const csm::daemon::CoreEvent &aEvent,
                        std::vector<csm::daemon::CoreEvent*>& postEventList );
  
  void NodeConnectRasEvent(const std::string &nodeName, std::vector<csm::daemon::CoreEvent*>& postEventList );
  void NodeDisconnectRasEvent(csm::network::Address_sptr addr,std::vector<csm::daemon::CoreEvent*>& postEventList );

  csm::daemon::EventContext_sptr _SystemEventContext;
};

#endif

