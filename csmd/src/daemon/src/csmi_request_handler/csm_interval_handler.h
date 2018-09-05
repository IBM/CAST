/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csm_interval_handler.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSM_DAEMON_SRC_CSM_DAEMON_INTERVAL_HANDLER_H__
#define __CSM_DAEMON_SRC_CSM_DAEMON_INTERVAL_HANDLER_H__

#include "csmi_base.h"
#include "csm_daemon_config.h"

class CSM_INTERVAL_HANDLER: public CSMI_BASE {

public:
  CSM_INTERVAL_HANDLER(csm::daemon::HandlerOptions& options)
  : CSMI_BASE(CSM_CMD_UNDEFINED, options)
  {
    setCmdName(std::string("CSM_INTERVAL"));
  }

  virtual void Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList );

};

#endif /* __CSM_DAEMON_SRC_CSM_DAEMON_INTERVAL_HANDLER_H__ */
