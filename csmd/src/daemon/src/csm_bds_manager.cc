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
    {
      retry->AgainOrWait( false );
    }
    else
    {
      // is BDS connection enabled at all?
      if( ! bds_enabled )
        continue;

      if( ! aMgr->CheckConnectivity() )
        aMgr->Connect();

      std::string content = bds_ev->GetContent();

      // anything to send in the event data?
      if( content.length() == 0 )
        continue;

      // send string to bds connection
      if( aMgr->SendData( content ) )
      {
        CSMLOG( csmd, trace ) << "Sent data to BDS: " << content;
      }
      else
      {
        CSMLOG( csmd, warning ) << "Failed sending to BDS: " << content.substr(0, 50 ) << ( content.length() > 49 ? "..." : "" );
      }
    }
  }
}

csm::daemon::EventManagerBDS::EventManagerBDS( const csm::daemon::BDS_Info &i_BDS_Info,
                                               csm::daemon::RetryBackOff *i_MainIdleLoopRetry )
: _BDS_Info( i_BDS_Info ),
  _IdleRetryBackOff( "BDSMgr", csm::daemon::RetryBackOff::SleepType::CONDITIONAL,
                     csm::daemon::RetryBackOff::SleepType::INTERRUPTIBLE_SLEEP,
                     0, 10000000, 1 ),
  _Socket( 0 )
{
  _Sink = new csm::daemon::EventSinkBDS( &_IdleRetryBackOff );

  _KeepThreadRunning = true;
  _ReadyToRun = false;
  _Thread = new boost::thread( BDSManagerMain, this );
  _IdleRetryBackOff.SetThread( _Thread );

  if( BDSActive() )
    Connect();
  Unfreeze();

}

bool
csm::daemon::EventManagerBDS::Connect()
{
  struct addrinfo hints, *clist;
  memset( &hints, 0, sizeof( struct addrinfo ) );
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  int tmp_errno = 0;

  if( (tmp_errno = getaddrinfo( _BDS_Info.GetHostname().c_str(), _BDS_Info.GetPort().c_str(), &hints, &clist )) != 0 )
  {
    CSMLOG( csmd, warning ) << "Unable to collect address info for " << _BDS_Info.GetHostname() << ":" << _BDS_Info.GetPort()
        << " error: " << gai_strerror( tmp_errno );
    return false;
  }

  _Socket = 0;
  for( struct addrinfo *c = clist; c != nullptr; c = c->ai_next )
  {
    _Socket = socket( c->ai_family, c->ai_socktype, c->ai_protocol );
    if( _Socket <= 0 )
    {
      CSMLOG( csmd, warning ) << "Unable to create socket: " << strerror( errno );
      freeaddrinfo( clist );
      return false;
    }

    if( connect( _Socket, c->ai_addr, c->ai_addrlen ) == 0 )
    {
      CSMLOG( csmd, info ) << "Connected to BDS at " << _BDS_Info.GetHostname() << ":" << _BDS_Info.GetPort();
      break;
    }

    close( _Socket );
    _Socket = 0;
  }

  freeaddrinfo( clist );

  if( _Socket == 0 )
  {
    CSMLOG( csmd, warning ) << "Unable to connect to BDS at " << _BDS_Info.GetHostname() << ":" << _BDS_Info.GetPort()
        << " No functional connection path found.";
    return false;
  }

  return true;
}

bool
csm::daemon::EventManagerBDS::CheckConnectivity()
{
  return (_Socket > 0 );
}

bool
csm::daemon::EventManagerBDS::SendData( const std::string data )
{
  ssize_t rc = 0;
  ssize_t done = 0;
  ssize_t remain = data.length();
  while(( rc >= 0 ) && ( rc < remain ))
  {
    rc = write( _Socket, data.c_str() + done, remain );
    if( rc > 0 )
    {
      remain -= rc;
      done += rc;
    }
  }

  return ((rc >= 0) && ( remain == 0 ));
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
