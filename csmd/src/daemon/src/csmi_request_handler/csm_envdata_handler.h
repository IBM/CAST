/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csm_envdata_handler.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef CSMD_SRC_DAEMON_SRC_CSMI_REQUEST_HANDLER_CSM_ENVDATA_HANDLER_H_
#define CSMD_SRC_DAEMON_SRC_CSMI_REQUEST_HANDLER_CSM_ENVDATA_HANDLER_H_

#include "csmd/src/daemon/include/csm_environmental_data.h"
#include "csmi_base.h"
#include "logging.h"

class CSM_ENVDATA_HANDLER: public CSMI_BASE {

public:
  CSM_ENVDATA_HANDLER(csm::daemon::HandlerOptions& options)
  : CSMI_BASE(CSM_environmental_data, options)
  {
    setCmdName(std::string("CSM_ENVDATA_HANDLER"));
    //_context = RegisterSystemWindowOpenEvent(this);
    //_startItem = csm::daemon::CPU;

    //InitUserData();
  }

  virtual void Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList );

};



#endif /* CSMD_SRC_DAEMON_SRC_CSMI_REQUEST_HANDLER_CSM_ENVDATA_HANDLER_H_ */
