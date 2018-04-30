/*================================================================================

    csmd/src/daemon/include/csm_daemon.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_CSM_DAEMON_H_
#define CSMD_SRC_DAEMON_INCLUDE_CSM_DAEMON_H_

#include <string>

#include "include/csm_retry_backoff.h"
#include "include/run_mode.h"
#include "include/window_timer.h"

namespace csm {
namespace daemon {

class Daemon
{
  csm::daemon::RunMode _RunMode;
  csm::daemon::WindowTimer *_WindowTimer;

public:
  Daemon()
  : _RunMode( RUN_MODE::STARTED ),
    _WindowTimer( nullptr )
  {}
  int Run( int argc, char **argv );
  void Stop() { _RunMode.StopRunning(); }
  inline bool KeepRunning() const { return _RunMode.KeepRunning(); }
  inline RUN_MODE::mode_t GetRunMode() const { return _RunMode.Get(); }

private:
  inline bool IsRunningMode() const {
    return (( GetRunMode() == csm::daemon::RUN_MODE::READY_RUNNING ) ||
        ( GetRunMode() == csm::daemon::RUN_MODE::PARTIAL_CONNECT ));
  }

};

}   // namespace daemon
}  // namespace csm

#endif /* CSMD_SRC_DAEMON_INCLUDE_CSM_DAEMON_H_ */
