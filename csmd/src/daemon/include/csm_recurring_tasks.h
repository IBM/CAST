/*================================================================================

    csmd/src/daemon/include/csm_recurring_tasks.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_CSM_RECURRING_TASKS_H_
#define CSMD_SRC_DAEMON_INCLUDE_CSM_RECURRING_TASKS_H_

#include <inttypes.h>
#include <algorithm>

#include <boost/math/common_factor.hpp>

namespace csm {
namespace daemon {

typedef struct
{
  unsigned _Interval;
  unsigned _Retry;
  bool _Enabled;
} SoftFailRecovery_t;

class RecurringTasks
{
  bool _Enabled;   ///< indicate whether this feature was enabled through config
  unsigned _LCDInterval;  ///< keep track of the LCD of configured intervals (in seconds)
  SoftFailRecovery_t _SFRecovery;  ///< coordinates for the soft-fail recovery task

public:
  RecurringTasks( )
  : _Enabled( false ),
    _LCDInterval( 1 ),
    _SFRecovery()
  {
      _SFRecovery._Interval = 0;
      _SFRecovery._Retry = 0;
  }
  ~RecurringTasks() {}

  inline void Enable() { _Enabled = true; }
  inline void Disable() { _Enabled = false; }
  inline bool IsEnabled() const { return _Enabled; }

  inline SoftFailRecovery_t GetSoftFailRecovery() const { return _SFRecovery; }
  inline void SetSoftFailRecovery( const unsigned i_Interval, const unsigned i_Retry, const bool i_Enabled )
  {
    _SFRecovery._Interval = i_Interval;
    _SFRecovery._Retry = i_Retry;
    _SFRecovery._Enabled = i_Enabled;
  }

  inline void UpdateLCM()
  {
    // since we have only one interval yet, this is just a trivial placeholder for LCD
    _LCDInterval = boost::math::lcm( _SFRecovery._Interval, _SFRecovery._Interval );
  }

  inline unsigned GetMinInterval() const { return _LCDInterval; }

};

template<class stream>
static stream&
operator<<( stream &out, const csm::daemon::RecurringTasks &data )
{
  out << " LCD-Interval=" << data.GetMinInterval() << "s; SoftFailRecovery=( " << data.GetSoftFailRecovery()._Interval << "s : " << data.GetSoftFailRecovery()._Retry << " )";
  return (out);
}



}  // daemon
} // csm



#endif /* CSMD_SRC_DAEMON_INCLUDE_CSM_RECURRING_TASKS_H_ */
