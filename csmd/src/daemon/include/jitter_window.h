/*================================================================================

    csmd/src/daemon/include/jitter_window.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_JITTER_WINDOW_H_
#define CSMD_SRC_DAEMON_INCLUDE_JITTER_WINDOW_H_

#include <chrono>
#include <atomic>

namespace csm {
namespace daemon {

typedef enum {
  WINDOW_NO_ADVANCE,
  WINDOW_ADVANCE,
  WINDOW_RESET
} JitterWindowAction;

template< typename ClockResolution = std::chrono::microseconds, typename ClockResolutionShort = std::micro,
   typename ClockType = std::chrono::high_resolution_clock >
class JitterWindow {

  typedef std::chrono::time_point< ClockType > WindowTimeType;
  typedef std::chrono::duration<long, ClockResolutionShort> WindowDurationType;

  WindowTimeType _WindowStart;
  WindowTimeType _DaemonContinue;
  WindowDurationType _JitterInterval;
  WindowDurationType _WindowDuration;
  WindowDurationType _ExtendedWindow;
  WindowDurationType _MaxDaemonOpTime;
  std::string _UnitShort;
  std::atomic_int _WindowClosed;
  unsigned _DefaultExtensionFactor;

public:
  JitterWindow() {}
  JitterWindow( const std::chrono::duration<long, std::micro> i_Interval,
                const std::chrono::duration<long, std::micro> i_Duration,
                const std::string i_UnitShort = "µs",
                const unsigned i_DefaultExtensionFactor = 1 )
  {
    _WindowStart = ClockType::now();
    _JitterInterval = i_Interval;
    _WindowDuration = i_Duration;
    _ExtendedWindow = WindowDurationType(0);
    _DaemonContinue = _WindowStart;
    _MaxDaemonOpTime = WindowDurationType(0);
    _UnitShort = i_UnitShort;
    _WindowClosed = 0;
    _DefaultExtensionFactor = i_DefaultExtensionFactor;
  }
  ~JitterWindow() {}

  void Reset()
  {
    WindowTimeType DaemonOpsEnd = ClockType::now();
    _WindowStart = DaemonOpsEnd;
    _DaemonContinue = DaemonOpsEnd;
    _WindowClosed = 0;
  }

  // check window expiration and return useconds to sleep
  bool CheckClose( const int i_WindowIndex )
  {
    WindowTimeType DaemonOpsEnd = ClockType::now();
    WindowDurationType DaemonOpTime = std::chrono::duration_cast<ClockResolution>( DaemonOpsEnd - _DaemonContinue);

    // todo: look for other means to determine the expected amount of one daemon loop
    if(( DaemonOpTime < _WindowDuration + _ExtendedWindow ))
    {
      if( _ExtendedWindow == WindowDurationType(0) )
      {
        long long newMaxOp = (double)_MaxDaemonOpTime.count() * 0.5  + (double)DaemonOpTime.count() * 0.5;
        _MaxDaemonOpTime = WindowDurationType( newMaxOp );
        if( _MaxDaemonOpTime >= _WindowDuration )
          _MaxDaemonOpTime = _WindowDuration / 2;
      }
    }
    else
    {
      LOG( csmd, warning ) << "Jitter Window violation: exceeded allowed time by " << (DaemonOpTime - _WindowDuration).count() << "mus";
    }

    // check how much time is left inside the current window:
    WindowDurationType ElapsedWindow =
        std::chrono::duration_cast<ClockResolution>( DaemonOpsEnd - _WindowStart );

    // Return if window is still open
    if( ElapsedWindow < (_WindowDuration - _MaxDaemonOpTime + _ExtendedWindow ) )
    {
      _DaemonContinue = DaemonOpsEnd;  // neglecting the few operations between begin of function and here...
      return ( _WindowClosed == 1 );
    }

#if REPRODUCABILITY_REQUIRED
    // make sure the daemon stays active until the full window is consumed
    // otherwise that would reduce reproducability of results
    while( ElapsedWindow < _WindowDuration )
    {
      DaemonOpsEnd = ClockType::now();
      ElapsedWindow =
          std::chrono::duration_cast<ClockResolution>( DaemonOpsEnd - _WindowStart );
    }
#endif

    LOG(csmd, trace) << "SCHED: window idx=" << i_WindowIndex << " closing. MaxDaemonOpTime: "
        << _MaxDaemonOpTime.count() << _UnitShort;

    _WindowClosed = 1;

    return (_WindowClosed == 1);
  }

  void Open( const uint64_t i_ExtendFactor = 1 )
  {
    _WindowClosed = 0;
    _DaemonContinue = ClockType::now();
    _WindowStart = _DaemonContinue;
    _ExtendedWindow = _WindowDuration * ( i_ExtendFactor - 1 );
  }

  void ExtendWindow( const unsigned i_ExtendFactor = 0 )
  {
    unsigned ex = ( i_ExtendFactor == 0 ) ? _DefaultExtensionFactor : i_ExtendFactor;
    if(( ex < 1 ) || ( _JitterInterval / ex > _WindowDuration ))
    _ExtendedWindow = _WindowDuration * ( ex - 1 );
  }

  inline bool IsClosed() const { return _WindowClosed == 1; }
};

}   // namespace daemon
}  // namespace csm

#endif /* CSMD_SRC_DAEMON_INCLUDE_JITTER_WINDOW_H_ */
