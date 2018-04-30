/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_init_handler_base.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include "csmi_base.h"
#ifndef _INIT_HANDLER_BASE
#define _INIT_HANDLER_BASE

class CSMI_INIT_HANDLER_BASE : public CSMI_BASE
{
public:
  CSMI_INIT_HANDLER_BASE(csm::daemon::HandlerOptions& options):
    CSMI_BASE(CSM_CMD_UNDEFINED, options) {}

  virtual ~CSMI_INIT_HANDLER_BASE() {}

  virtual void Process( const csm::daemon::CoreEvent &aEvent, std::vector<csm::daemon::CoreEvent*>& postEventList )
  {
    LOG(csmd,trace) << "CSMI_INIT_HANDLER_BASE: Processing...";
  }

};

#endif
