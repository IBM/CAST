/*================================================================================

    csmd/src/inv/include/inv_master_handler.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __INV_MASTER_HANDLER_H
#define __INV_MASTER_HANDLER_H

#include "csmd/src/daemon/src/csmi_request_handler/csmi_base.h"
#include "csmnet/src/CPP/csm_network_msg_cpp.h"
#include "csmnet/src/CPP/address.h"


class INV_MASTER_HANDLER : public CSMI_BASE {
 
public:
  // this handler not tied to any specific csmi cmd
  INV_MASTER_HANDLER(csm::daemon::HandlerOptions& options)
  : CSMI_BASE(CSM_CMD_UNDEFINED, options)
  { setCmdName(std::string("INV_MASTER_HANDLER")); }
  
public:

  virtual void Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList );
    
  virtual ~INV_MASTER_HANDLER() { }
  
};

#endif

