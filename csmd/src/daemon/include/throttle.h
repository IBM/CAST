/*================================================================================

    csmd/src/daemon/include/throttle.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_THROTTLE_H_
#define CSMD_SRC_DAEMON_INCLUDE_THROTTLE_H_

#include <chrono>

namespace csm {
namespace daemon {

typedef std::chrono::time_point< std::chrono::steady_clock > TimeType;

template<uint32_t THROTTLE_GRANULARITY>
class Throttle
{
  std::chrono::seconds _RemainingSteps;   ///< number of remaining throttling steps
  std::chrono::seconds _EpochDuration;    ///< duration of epoch
  TimeType _EndOfEpoch;                   ///< marks the end of a check epoch
  uint64_t _StepLimit;                    ///< limit of steps allowed within epoch
  TimeType _LastThrottleTime;             ///< last time stamp where throttling happened
  uint64_t _StepCount;                    ///< number of steps since last throttling timestamp
  uint64_t _DownThrottleRate;             ///< decrement of remaining throttle steps
  std::string _ThrottleName;              ///< informative name to show in warning log messages

public:
  // the maximum allowed rate is calculated from maxSteps/Duration
  Throttle( const std::string i_Name,
            const uint64_t i_MaxSteps,
            const uint64_t i_Duration )
  : _RemainingSteps( 0 ),
    _EpochDuration( std::chrono::seconds( i_Duration )),
    _EndOfEpoch( std::chrono::steady_clock::now() + _EpochDuration ),
    _StepLimit( i_MaxSteps ),
    _LastThrottleTime( std::chrono::steady_clock::now() ),
    _StepCount( 0 ),
    _DownThrottleRate( 1 ),
    _ThrottleName( i_Name )
  {
    LOG( csmd, debug ) << "Setting up throttling of " << _ThrottleName
        << " to " << _StepLimit << " per " << _EpochDuration.count() << " sec."
        << " or " << _StepLimit / _EpochDuration.count() << " steps/s";
    if( THROTTLE_GRANULARITY > 1000000 )
      throw csm::daemon::Exception("Cannot create throttling with granularity > 1000000!");
  }

  inline bool NeedsThrottle() const
  {
    return (_RemainingSteps.count() > 0);
  }

  // adjust the throttle stepping rate after a certain time interval
  inline void AdjustToRate()
  {
    if( NeedsThrottle() )
    {
      // todo: replace with daemon clock to avoid syscalls
      TimeType newThrottleTime = std::chrono::steady_clock::now();
      std::chrono::seconds interval = std::chrono::duration_cast<std::chrono::seconds>( newThrottleTime - _LastThrottleTime );

      if( interval.count() > 0 )
      {
        uint64_t currentRate = _StepCount / interval.count();

        _LastThrottleTime = newThrottleTime;
        _StepCount = 0;

        if( currentRate <  10 )
        {
          _DownThrottleRate *= 2;
          LOG( csmd, debug ) << "Rate of " << _ThrottleName << " is lower than limit " << currentRate
              << " shortening time to end throttling by factor " << _DownThrottleRate;
        }
        else
          _DownThrottleRate = 1;
      }
    }
  }

  inline void CheckAndDo()
  {
    // count the number of calls to this...
    ++_StepCount;

    AdjustToRate();

    // do the actual throttling if needed
    if( NeedsThrottle() )
    {
      usleep( 1000000 / THROTTLE_GRANULARITY );
      _RemainingSteps -= std::chrono::seconds( _DownThrottleRate );
      if( ! NeedsThrottle() )
        LOG( csmd, warning ) << "Throttling of " << _ThrottleName << " finished. Continuing full speed.";
    }
  }

  inline void Update()
  {
    // todo: replace with daemon clock to avoid syscalls
    TimeType currentTime = std::chrono::steady_clock::now();
    if( currentTime < _EndOfEpoch )
    {
      std::chrono::seconds pause = std::chrono::duration_cast<std::chrono::seconds>( _EndOfEpoch - currentTime );
      _RemainingSteps += ( pause * THROTTLE_GRANULARITY );
      LOG( csmd, warning ) << "Daemon cannot sustain this rate of " << _ThrottleName
          << ". Throttling for " << pause.count() << " seconds.";
      _EndOfEpoch = currentTime + _EpochDuration;
      _DownThrottleRate = 1;
    }
  }
};

}  // namespace daemon
} // namespace csm

#endif /* CSMD_SRC_DAEMON_INCLUDE_THROTTLE_H_ */
