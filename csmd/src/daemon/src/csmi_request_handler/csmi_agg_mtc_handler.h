/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_agg_mtc_handler.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSM_DAEMON_SRC_CSMI_AGG_MTC_HANDLER_H__
#define __CSM_DAEMON_SRC_CSMI_AGG_MTC_HANDLER_H__

#include "csmi_base.h"
#include "csmnet/src/CPP/csm_network_msg_cpp.h"
#include "csmnet/src/CPP/address.h"

class CSMI_AGG_MTC_HANDLER : public CSMI_BASE {
 
public:
  // this handler not tied to any specific csmi cmd
  CSMI_AGG_MTC_HANDLER(csm::daemon::HandlerOptions& options)
  : CSMI_BASE(CSM_CMD_UNDEFINED, options)
  { setCmdName(std::string("CSMI_AGG_MTC_HANDLER")); }
  
public:

  virtual void Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList );
    
  virtual ~CSMI_AGG_MTC_HANDLER() { }
};

#endif

