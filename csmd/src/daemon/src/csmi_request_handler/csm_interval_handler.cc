/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csm_interval_handler.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef logprefix
#define logprefix "INTERVAL_HDLR"
#endif

#include "csm_pretty_log.h"
#include "csm_interval_handler.h"

void
CSM_INTERVAL_HANDLER::Process( const csm::daemon::CoreEvent &aEvent,
                               std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  CSMLOG( csmd, info ) << "INTERVAL: triggered handler to process";
}
