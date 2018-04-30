/*================================================================================

    csmd/src/daemon/src/csm_event_types/csm_event_cpuinfo.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/* csm_event_cpuinfo.h
 *
 ******************************************/

#ifndef CSM_DAEMON_SRC_CSM_EVENT_TYPES_CSM_EVENT_CPUINFO_H_
#define CSM_DAEMON_SRC_CSM_EVENT_TYPES_CSM_EVENT_CPUINFO_H_

#include <iostream>

#include "include/csm_core_event.h"

namespace csm {
namespace daemon {

struct CPUInfo{
    uint64_t mCPUCount;
    uint64_t mFrequency;
};

template<class stream>
static stream&
operator<<( stream &out, const csm::daemon::CPUInfo &data )
{
  out << "CPUs: "<< data.mCPUCount << " Freq: "<< data.mFrequency;
  return (out);
}

typedef csm::daemon::CoreEventBase<csm::daemon::CPUInfo> CPUInfoEvent;

}  // namespace daemon
} // namespace csm

#endif /* CSM_DAEMON_SRC_CSM_EVENT_TYPES_CSM_EVENT_CPUINFO_H_ */
