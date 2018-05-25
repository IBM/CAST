/*================================================================================

    csmd/src/daemon/src/csm_bds_manager.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifdef logprefix
#undef logprefix
#endif
#define logprefix "BDSMGR"
#include "csm_pretty_log.h"

#include "csm_daemon_config.h"
#include "include/csm_bds_manager.h"

void BDSManagerMain( csm::daemon::EventManagerBDS *aMgr )
{
  aMgr->GreenLightWait();
  csm::daemon::EventSinkBDS *timers = dynamic_cast<csm::daemon::EventSinkBDS*>( aMgr->GetEventSink() );

  csm::daemon::RetryBackOff *retry = aMgr->GetRetryBackoff();

  bool idle = true;
  bool bds_enabled = aMgr->BDSActive();

  CSMLOG( csmd, debug ) << "Starting BDSMgr thread.";
  while( aMgr->GetThreadKeepRunning() )
  {
    aMgr->GreenLightWait();
    if( ! aMgr->GetThreadKeepRunning() )
      break;

    csm::daemon::BDSEvent *bds_ev = dynamic_cast<csm::daemon::BDSEvent*>( timers->FetchEvent() );
    idle = ( bds_ev == nullptr );

    // if nothing to do, just wait for regular wakeup
    if( idle )
      retry->AgainOrWait( false );
    else
    {
      if( ! bds_enabled )
        continue;

      /* if there's a timer-event, set up an interruptable sleep
       * (interruptable because a new timer event might require to wake up earlier and change ordering
       */
      std::string content = bds_ev->GetContent();

      // send string to bds connection
      CSMLOG( csmd, debug ) << "Sending data to BDS: " << content;

    }
  }
}

csm::daemon::EventManagerBDS::EventManagerBDS( const csm::daemon::BDS_Info &i_BDS_Info,
                                               csm::daemon::RetryBackOff *i_MainIdleLoopRetry )
: _BDS_Info( i_BDS_Info ),
  _IdleRetryBackOff( "BDSMgr", csm::daemon::RetryBackOff::SleepType::CONDITIONAL,
                     csm::daemon::RetryBackOff::SleepType::INTERRUPTIBLE_SLEEP,
                     0, 10000000, 1 )
{
  _Sink = new csm::daemon::EventSinkBDS( &_IdleRetryBackOff );

  _KeepThreadRunning = true;
  _ReadyToRun = false;
  _Thread = new boost::thread( BDSManagerMain, this );
  _IdleRetryBackOff.SetThread( _Thread );
  Unfreeze();
}

csm::daemon::EventManagerBDS::~EventManagerBDS()
{
  _KeepThreadRunning = false;  // ask the mgr loop to exit
  Freeze();   // make sure, there's no activity going on

  Unfreeze();   // allow the thread to run again...
  _IdleRetryBackOff.WakeUp();   // or/and wake it up
  CSMLOG( csmd, debug ) << "Exiting BDSMgr...";
  _Thread->join();
  CSMLOG( csmd, info ) << "Terminating BDSMgr complete";
  delete _Thread;
}
