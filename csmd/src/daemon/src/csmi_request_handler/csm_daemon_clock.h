/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csm_daemon_clock.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef __CSM_DAEMON_SRC_CSM_DAEMON_CLOCK_H__
#define __CSM_DAEMON_SRC_CSM_DAEMON_CLOCK_H__

#include "csmi_base.h"

class CSM_DAEMON_CLOCK: public CSMI_BASE {
private:
  uint64_t _timer; // in milliseconds
  double _timer_inverse; // relative to seconds
  uint64_t _prev_event_count;
  
  int _active_timer_count;
  
public:
  CSM_DAEMON_CLOCK(csm::daemon::HandlerOptions& options)
  : CSMI_BASE(CSM_CMD_UNDEFINED, options)
  {
    setCmdName(std::string("CSM_DAEMON_CLOCK"));
    _timer = 60000;
    _timer_inverse = 1000.0/_timer;
    _prev_event_count = 0;
    _active_timer_count = 0;
  }
  
public:

  virtual void Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList );
    

};

#endif

