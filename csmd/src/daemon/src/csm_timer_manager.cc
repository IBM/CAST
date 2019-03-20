/*================================================================================

    csmd/src/daemon/src/csm_timer_manager.cc

  © Copyright IBM Corporation 2015-2019. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "include/csm_timer_manager.h"

void TimerManagerMain( csm::daemon::EventManagerTimer *aMgr )
{
  aMgr->GreenLightWait();
  csm::daemon::EventSourceTimer *src = dynamic_cast<csm::daemon::EventSourceTimer*>( aMgr->GetEventSource() );
  csm::daemon::EventSinkTimer *timers = dynamic_cast<csm::daemon::EventSinkTimer*>( aMgr->GetEventSink() );

  // If the src and timers weren't set return.
  if (!(src && timers)) 
  {
      LOG( csmd, error ) << "Unable to retrieve EventSourceTimer or EventSinkTimer.";
      return;
  }

  csm::daemon::RetryBackOff *retry = aMgr->GetRetryBackoff();

  bool idle = true;

  LOG( csmd, debug ) << "Starting TimerMgr thread.";
  while( aMgr->GetThreadKeepRunning() )
  {
    aMgr->GreenLightWait();
    if( ! aMgr->GetThreadKeepRunning() )
      break;

    csm::daemon::TimerEvent *tev = dynamic_cast<csm::daemon::TimerEvent*>( timers->FetchEvent() );
    idle = ( tev == nullptr );

    // if nothing to do, just wait for regular wakeup
    if( idle )
    {
      try { retry->AgainOrWait( false ); }
      catch ( csm::daemon::Exception &e ) { LOG( csmd, error ) << e.what; break; }
    }
    else
    {
      /* if there's a timer-event, set up an interruptable sleep
       * (interruptable because a new timer event might require to wake up earlier and change ordering
       */
      csm::daemon::TimerContent content = tev->GetContent();

      // uSleep does an interruptable sleep and returns true if we got interrupted
      LOG( csmd, trace ) << "Setting timer to trigger in " << content.RemainingMicros() << "µs.";
      bool interrupted = false;
      try { retry->uSleep( content.RemainingMicros() ); }
      catch ( csm::daemon::Exception &e ) { LOG( csmd, error ) << e.what; }
      // after returning from sleep, we better check if we still need to keep running
      if( ! aMgr->GetThreadKeepRunning() )
        break;
      if( interrupted )
      {
        // we got interrupted - means, there's another timer-event potentially earlier timer
        // or the main thread closes the jitter window (need to prevent any timer from waking up while it's closed
        // so we put the current event back into the timer queue and pretent that we're just all new...
        LOG( csmd, trace ) << "Timer interrupted with " << content.RemainingMicros() << "µs remaining.";
        timers->RestoreEvent( tev );
        continue;
      }
      else
      {
        // all good, we finished the sleep and the timer event is to be queued
        src->QueueEvent( tev );
      }
    }
  }
}

csm::daemon::EventManagerTimer::EventManagerTimer( csm::daemon::RetryBackOff *i_MainIdleLoopRetry )
: _TimerQueue(),
  _IdleRetryBackOff( "TimerMgr", csm::daemon::RetryBackOff::SleepType::CONDITIONAL,
                     csm::daemon::RetryBackOff::SleepType::INTERRUPTIBLE_SLEEP,
                     0, 1000000, 1 )
{
  _Source = new csm::daemon::EventSourceTimer( i_MainIdleLoopRetry );
  _Sink = new csm::daemon::EventSinkTimer( &_IdleRetryBackOff );

  _KeepThreadRunning = true;
  _ReadyToRun = false;
  _Thread = new boost::thread( TimerManagerMain, this );
  _IdleRetryBackOff.SetThread( _Thread );
  Unfreeze();
}

csm::daemon::EventManagerTimer::~EventManagerTimer()
{
  _KeepThreadRunning = false;  // ask the mgr loop to exit
  Freeze();   // make sure, there's no timer in sleep

  Unfreeze();   // allow the thread to run again...
  try { _IdleRetryBackOff.WakeUp(); }  // or/and wake it up
  catch ( csm::daemon::Exception &e ) { LOG( csmd, error ) << e.what(); }
  LOG( csmd, debug ) << "Exiting TimerMgr...";
  try { _Thread->join(); }
  catch ( ... ) { LOG( csmd, error ) << "Failure while joining timer mgr thread."; }
  LOG( csmd, info ) << "Terminating TimerMgr complete";
  delete _Thread;
}
