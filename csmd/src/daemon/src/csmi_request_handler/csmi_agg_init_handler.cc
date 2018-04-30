/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_agg_init_handler.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
// implement the CSM api node attributes command...
//

#include "csmi_agg_init_handler.h"

#include "logging.h"

void CSMI_AGG_INIT_HANDLER::Process( const csm::daemon::CoreEvent &aEvent, std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  LOG(csmd,info) << "CSMI_AGG_INIT_HANDLER: Initializing Aggregator..";
  
  CSMI_INIT_HANDLER_BASE::Process(aEvent, postEventList);
}
