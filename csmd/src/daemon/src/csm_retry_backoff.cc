/*================================================================================

    csmd/src/daemon/src/csm_retry_backoff.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <unistd.h>
#include <signal.h>

#include <algorithm>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include <boost/thread.hpp>

#include "logging.h"
#include "include/csm_daemon_exception.h"

#include "include/csm_retry_backoff.h"



csm::daemon::RetryBackOff::RetryBackOff( const std::string & i_Identifier,
                                         const SleepType i_SleepType,
                                         const SleepType i_JobSleepType,
                                         const uint32_t i_MaxBackoff,
                                         const uint32_t i_BaseSleep,
                                         const uint32_t i_PollLoops,
                                         csm::network::ReliableMsg *i_Network )
: _Identifier( i_Identifier ),
  _SlowMeDown( 0 ), _MaxBackoff( i_MaxBackoff + i_PollLoops ),
  _SlowDownBaseFactor( i_BaseSleep ), _SlowDownOffset( i_PollLoops ),
  _SleepType( i_SleepType ),
  _JobSleepType( i_JobSleepType ),
  _ActualSleep( i_SleepType ),
  _Network( i_Network ),
  _CreatorThread( nullptr )
{
  _ReadyToGo = false;

  // enable the daemon signal set to listen for SIGALRM
  // note that the signal handler setup is not part of this class
  if(( _SleepType == SIGNAL ) || ( _JobSleepType == SIGNAL ))
  {
    sigfillset( &_SignalSet );
    sigdelset( &_SignalSet, SIGALRM );
  }

  if( (( _SleepType == SOCKET ) || ( _JobSleepType == SOCKET )) && ( _Network == nullptr ) )
      throw csm::daemon::Exception("BUG: Socket-based RetryBackOff requires Endpoint != nullptr.");

  LOG( csmd, debug ) << "Creating RetryBackOff: " << _Identifier << " Type: " << _SleepType << ":" << _JobSleepType
    << " Timings: l" << _SlowDownOffset << ":t" << _SlowDownBaseFactor << ":i" << _MaxBackoff;
}

bool
csm::daemon::RetryBackOff::AgainOrWait( const bool Running_Job_Mode )
{
  bool DidItSleep = false;

  // never attempt to retry via _SlowMeDown counter when running job
  if( Running_Job_Mode )
    _ActualSleep = _JobSleepType;
  else
  {
    // check if we reached the end of the #iterations configured to poll
    ++_SlowMeDown;
    if( ( !Running_Job_Mode ) && ( _SlowMeDown < _SlowDownOffset ))
      return DidItSleep;

    _SlowMeDown = std::min( _MaxBackoff, _SlowMeDown );
    _ActualSleep = _SleepType;
  }

  switch( _ActualSleep )
  {
    case NOOP:
      DidItSleep = true;
      break;
    case MICRO_SLEEP:
      usleep( GetSleepTime() );
      DidItSleep = true;
      // no way to force it to wake up
      break;
    case INTERRUPTIBLE_SLEEP:
      try {
        boost::this_thread::sleep_for( boost::chrono::microseconds( GetSleepTime() ) );
        DidItSleep = true;
      }
      catch ( const boost::thread_interrupted &e )
      {
        LOG( csmd, trace) << "RetryBack-Off: "<< _Identifier << " interrupted in sleep.";
        DidItSleep = false;
      }
      break;
    case CONDITIONAL:
    {
      LOG( csmd, trace) << "RetryBack-Off: "<< _Identifier << " conditional WAIT.";
      DidItSleep = WaitConditional();
      _ReadyToGo = false;
      LOG( csmd, trace ) << "RetryBack-Off: "<< _Identifier << " continue after conditional WAIT.";
      break;
    }
    case SIGNAL:
      LOG( csmd, trace) << "RetryBack-Off: "<< _Identifier << " sigsuspend.";
      sigsuspend( &_SignalSet );
      LOG( csmd, trace) << "RetryBack-Off: "<< _Identifier << " sigsuspend continue.";
      DidItSleep = true;
      break;
    case SOCKET:
      LOG( csmd, trace ) << "RetryBack-Off: " << _Identifier << " socket WAIT.";
      _Network->WaitForActivity();
      LOG( csmd, trace ) << "RetryBack-Off: " << _Identifier << " socket continue.";
      DidItSleep = true;
      break;
    default:
      throw Exception("BUG: AgainOrWait(): " + _Identifier + " Unknown sleep type (" + std::to_string(_ActualSleep ) + ")" );
  }
  return DidItSleep;
}


void
csm::daemon::RetryBackOff::WakeUp( const bool i_Sighandler )
{
  switch( _ActualSleep )
  {
    case NOOP:
      break;
    case MICRO_SLEEP:
      // no way to force it to wake up
      break;
    case INTERRUPTIBLE_SLEEP:
      // if we know the creator-thread, we can interrupt it.
      LOG( csmd, trace ) << "Waking up "<< _Identifier << " with thread-interrupt.";
      if( _CreatorThread != nullptr )
        _CreatorThread->interrupt();
      else
        throw csm::daemon::Exception( "Undefined Thread Owner! Interruptable sleep can only work with a known thread to interrupt." );
      break;
    case CONDITIONAL:
    {
      std::unique_lock<std::mutex> glock( _GreenlightLock );
      _ReadyToGo = true;
      LOG( csmd, trace ) << "Waking up "<< _Identifier << " with conditional";
      // Sighandler should not be allowed to call notify_all because it might deadlock
      if( ! i_Sighandler )
        _GreenlightCondition.notify_all();
      break;
    }
    case SIGNAL:
      // todo: cause alarm signal to be sent or forget about this case?
      break;

    case SOCKET:
      _Network->WakeUp();
      break;

    default:
      throw Exception("BUG: WakeUp():  " + _Identifier + "Unknown sleep type (" + std::to_string(_ActualSleep ) + ")" );
  }
}

// allows a caller-specified usleep based on the configured sleep type
// returns true if sleep was interrupted
bool
csm::daemon::RetryBackOff::uSleep( const uint64_t i_MicroSecs )
{
  if( ! _CreatorThread )
    throw csm::daemon::Exception( "Undefined Thread Owner! Interruptable sleep can only work with a known thread to interrupt." );

  try {
    _ActualSleep = INTERRUPTIBLE_SLEEP;
    boost::this_thread::sleep_for( boost::chrono::microseconds( i_MicroSecs ) );
    _ActualSleep = _SleepType;
  }
  catch ( const boost::thread_interrupted &e )
  {
    _ActualSleep = _SleepType;
    return true;
  }
  return false;
}

bool
csm::daemon::RetryBackOff::WaitConditional()
{
  bool DidItSleep = false;
  std::unique_lock<std::mutex> glock( _GreenlightLock );
  //while( !_ReadyToGo )
  {
    DidItSleep = true;
    LOG( csmd, trace ) << "GreenLightWait "<< _Identifier << " wait...? " << _SlowDownBaseFactor;
//    _GreenlightCondition.wait( glock );
    _GreenlightCondition.wait_for( glock, std::chrono::microseconds( _SlowDownBaseFactor ) );
    LOG( csmd, trace ) << "GreenLightWait "<< _Identifier << " condition met..? " << _ReadyToGo;
  }
  return DidItSleep;
}

bool
csm::daemon::RetryBackOff::JoinThread( boost::thread *i_Thread,
                                       const int i_Index )
{
  boost::chrono::duration<long long, boost::milli> joinWait( GetSleepTime() / 1000 + 10 );
  int JoinRetries = 5;
  bool joined = true;
  std::string index_string = "";
  if( i_Index >= 0 )
    index_string = std::to_string( i_Index );

  for( int jointry = 0; jointry < JoinRetries; ++jointry )
  {
    try {
      WakeUp();  // in case the db mgr thread is sleeping...
    }
    catch (csm::daemon::Exception &e )
    {
      LOG(csmd, error ) << e.what();
    }
    LOG( csmd, debug ) << "Joining " << _Identifier << index_string
        << " retried: " << jointry
        << " ptr=" << (void*)i_Thread;
    if (i_Thread != nullptr )
    {
      // allow the thread to join - if it doesn't "want" to, then terminate
      if( ! i_Thread->try_join_for( joinWait ) )
      {
        LOG( csmd, warning ) << _Identifier << index_string << " did not join on exit. retrying again";
      }
      else
      {
        if( jointry == JoinRetries - 1 )
        {
          joined = false;
          LOG( csmd, warning ) << _Identifier << index_string
              << " did not join after "<< jointry << " retries. Needs forced termination.";
        }
        break;
      }
    }
  }
  return joined;
}
