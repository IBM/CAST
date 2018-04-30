/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/helpers/EventHelpers.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#include "EventHelpers.h"

namespace csm {
namespace daemon {
namespace helper {

const uint64_t BAD_STATE       = UINT64_MAX;
const uint64_t BAD_TIMEOUT_LEN = UINT64_MAX;
const std::type_info& NETWORK_EVENT_TYPE = 
    typeid( csm::daemon::EventContentContainer<csm::network::MessageAndAddress> ); 
const std::type_info& DB_RESP_EVENT_TYPE = 
    typeid( csm::daemon::EventContentContainer<csm::db::DBRespContent> );
const std::type_info& DB_REQ_EVENT_TYPE = 
    typeid( csm::daemon::EventContentContainer<csm::db::DBReqContent> );
const std::type_info& TIMER_EVENT_TYPE = 
    typeid( csm::daemon::EventContentContainer<csm::daemon::TimerContent> );
const std::type_info& SYSTEM_EVENT_TYPE = 
    typeid( csm::daemon::EventContentContainer<csm::daemon::SystemContent> );
const std::type_info& ENVIRONMENTAL_EVENT_TYPE = 
    typeid( csm::daemon::EventContentContainer<csm::daemon::BitMap> );

} // End namespace helpers
} // End namespace daemon
} // End namespace csm
