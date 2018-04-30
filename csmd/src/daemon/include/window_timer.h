/*================================================================================

    csmd/src/daemon/include/window_timer.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_WINDOW_TIMER_H_
#define CSMD_SRC_DAEMON_INCLUDE_WINDOW_TIMER_H_

#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <thread>

namespace csm {
namespace daemon {

class WindowTimer
{
  uint64_t _Interval;
  volatile std::atomic_bool _KeepRunning;
  std::thread _TimerThread;

public:
  WindowTimer( const uint64_t i_Interval,
               const uint64_t i_Duration,
               void (AlarmCallback)( void ) )
  : _Interval( i_Interval )
  {
    _KeepRunning = true;

    LOG( csmd, debug ) << "Starting window timer thread...";

    // run the timer in a separate thread to prevent the alarm signal to interfere with
    _TimerThread = std::thread([ &, AlarmCallback ] {

      while( _KeepRunning )
      {
        // get current clock
        struct timespec time_val;
        clock_gettime( CLOCK_REALTIME, &time_val );
        uint64_t clock_us = (time_val.tv_sec*1000000) + (time_val.tv_nsec / 1000);

        // calc diff to next window open
        uint64_t window_remain = _Interval - (clock_us % _Interval);

        // set up a sleep until next window open
        LOG( csmd, trace ) << " setting up sleep for " << window_remain << "mys";
        usleep( window_remain );

        // call callback
        AlarmCallback();
      }

      // allow the timer to trigger before destroying the daemon core
      usleep( _Interval );
    } );

  }

  ~WindowTimer()
  {
    _KeepRunning = false;
    LOG( csmd, debug ) << "Stopping window timer thread...";
    _TimerThread.join();
  }

};

}  // namespace daemon
} // namespace csm

#endif /* CSMD_SRC_DAEMON_INCLUDE_WINDOW_TIMER_H_ */
