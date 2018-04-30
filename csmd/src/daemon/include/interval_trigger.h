/*================================================================================

    csmd/src/daemon/include/interval_trigger.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_INTERVAL_TRIGGER_H_
#define CSMD_SRC_DAEMON_INCLUDE_INTERVAL_TRIGGER_H_

#include <chrono>

#include "logging.h"

namespace csm {
namespace daemon {

class IntervalTrigger
{
  std::chrono::time_point<std::chrono::system_clock> _NextTrigger;
  std::chrono::milliseconds _Interval;
public:
  IntervalTrigger( const uint64_t i_IntervalMilli )
  {
    _Interval = std::chrono::milliseconds( i_IntervalMilli );
    _NextTrigger = std::chrono::system_clock::now() + _Interval;
  }
  ~IntervalTrigger() {}

  bool IsReady()
  {
    bool ready = false;
    std::chrono::time_point<std::chrono::system_clock> current = std::chrono::system_clock::now();
    if( current > _NextTrigger )
    {
      ready = true;
      _NextTrigger = current + _Interval;
    }
    return ready;
  }
};

}   // daemon
}  // csm

#endif /* CSMD_SRC_DAEMON_INCLUDE_INTERVAL_TRIGGER_H_ */
