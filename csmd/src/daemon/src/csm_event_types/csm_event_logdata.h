/*================================================================================

    csmd/src/daemon/src/csm_event_types/csm_event_logdata.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/* csm_event_logdata.h
 *
 ******************************************/

#ifndef CSM_DAEMON_SRC_CSM_EVENT_TYPES_CSM_EVENT_LOGDATA_H_
#define CSM_DAEMON_SRC_CSM_EVENT_TYPES_CSM_EVENT_LOGDATA_H_

#include "include/csm_core_event.h"

typedef std::string CSMLogData;

namespace csm {
namespace daemon {

typedef csm::daemon::CoreEventBase<CSMLogData> LogEvent;

}  // namespace daemon
} // namespace csm


#endif /* CSM_DAEMON_SRC_CSM_EVENT_TYPES_CSM_EVENT_LOGDATA_H_ */
