/*================================================================================

    csmd/src/daemon/src/csm_daemon.cc

  © Copyright IBM Corporation 2015-2019. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <vector>      // postEventList
#include <errno.h>
#include <signal.h>

#ifdef logprefix
#undef logprefix
#endif
#define logprefix "CSMD_MAIN"
#include "csm_pretty_log.h"

#include "csmd/include/csm_daemon_config.h"

#include "csmi/src/common/include/csmi_serialization.h"
#include "csmi/src/common/include/csmi_cmds.h"

#include "include/csm_daemon_core.h"
#include "src/csmi_request_handler/csmi_base.h"
#include "include/connection_handling.h"
#include "include/thread_pool.h"
#include "include/thread_manager.h"

#include "include/csm_daemon.h"

#include "include/invoke_handler.h"

// event_count is used to track the number of processing events in the main loop
// and to provide statistics about the load of a daemon
volatile uint64_t csm::daemon::DaemonState::event_count = 0;

// defined outside of main because Timer handler needs access to DaemonCore
static csm::daemon::CoreGeneric *DaemonCore = nullptr;

//void TimerTick(int sig, siginfo_t *si, void *arg);
void TimerTick( void );

/////////////////////////////////////////////////////////////////////////////////

int csm::daemon::Daemon::Run( int argc, char **argv )
{
  csm::daemon::Configuration *CSMDaemonConfig = nullptr;
  csm::daemon::ConnectionHandling *connHdl = nullptr;
  csm::daemon::ThreadPool *thread_pool = nullptr;

  csm::daemon::EventManagerDB* dbMgr = nullptr;
  csm::daemon::EventManagerNetwork *netMgr = nullptr;
  csm::daemon::EventManagerTimer *timerMgr = nullptr;
  csm::daemon::EventManagerBDS *bdsMgr = nullptr;

  csm::daemon::EventManagerPool evMgrPool;
  csm::daemon::ThreadManager *threadMgr = nullptr;

  setLoggingLevel(csmd, info); // start with info level logging until it's changed by config

  csm::daemon::RetryBackOff ConnectRetry( "ConnectMain",
                                          RetryBackOff::SleepType::MICRO_SLEEP,
                                          RetryBackOff::SleepType::MICRO_SLEEP,
                                          5, 50000 );
  // main loop
  do
  {
    switch( GetRunMode() )
    {
      case csm::daemon::RUN_MODE::STARTED:
      {
        // read configuration
        try
        {
          CSMDaemonConfig = csm::daemon::Configuration::Instance( argc, argv, &_RunMode );
          _RunMode.SetRole( CSMDaemonConfig->GetRole() );
        }

        catch (csm::daemon::Exception &e)
        {
          std::cerr << "Unable to continue without proper configuration: " << e.what() << std::endl;
          _RunMode.Transition( csm::daemon::REASON_ERROR, EBADF );
          break;
        }

        _RunMode.Transition();
        break;
      }
///////////////////////////////////////////////////////////////////////////////////////////////////
      case csm::daemon::RUN_MODE::CONFIGURED:
      {
        // Break if the configuraation is null.
        if ( CSMDaemonConfig == nullptr )
        {
            errno = EINVAL;
            CSMLOG( csmd, error ) << "Daemon Role Setup: config missing; errno=" << errno;
            perror("Daemon Role Setup:");
            return errno;
        }

        // create the daemon cores, etc...
        // have to set up the pack/unpack functions in CSMI_MAPPING before creating DaemonCore
        csmi_cmd_hdl_init();
        // intitialize the static variables in HandlerOptions before creating DaemonCore
        csm::daemon::HandlerOptions::Init();

        // create daemon core based on role
        switch( CSMDaemonConfig->GetRole() )
        {
          case CSM_DAEMON_ROLE_MASTER:
              DaemonCore = new csm::daemon::CoreMaster();
              connHdl = new csm::daemon::ConnectionHandling_master( CSMDaemonConfig->GetCriticalConnectionList(), &_RunMode );
              try {
                dbMgr = new csm::daemon::EventManagerDB( CSMDaemonConfig->GetDBDefinitionInfo(), connHdl, DaemonCore->GetRetryBackOff() );
              }
              catch( csm::daemon::Exception &e ) {
                CSMLOG( csmd, debug ) << e.what();
                _RunMode.Transition( csm::daemon::REASON_ERROR, EPROTONOSUPPORT );
              }
              evMgrPool.push_back( dbMgr );

              break;

          case CSM_DAEMON_ROLE_UTILITY:
              DaemonCore = new csm::daemon::CoreUtility();
              connHdl = new csm::daemon::ConnectionHandling_utility( CSMDaemonConfig->GetCriticalConnectionList(), &_RunMode );
              break;

          case CSM_DAEMON_ROLE_AGGREGATOR:
              DaemonCore = new csm::daemon::CoreAggregator();
              connHdl = new csm::daemon::ConnectionHandling_aggregator( CSMDaemonConfig->GetCriticalConnectionList(), &_RunMode );
              bdsMgr = new csm::daemon::EventManagerBDS( CSMDaemonConfig->GetBDS_Info(), DaemonCore->GetRetryBackOff() );
              break;

          case CSM_DAEMON_ROLE_AGENT:
              DaemonCore = new csm::daemon::CoreAgent();
              connHdl = new csm::daemon::ConnectionHandling_compute( CSMDaemonConfig->GetCriticalConnectionList(), &_RunMode );
              break;

          default:
            errno = EINVAL;
            CSMLOG( csmd, error ) << "Daemon Role Setup: errno=" << errno;
            perror("Daemon Role Setup:");
            throw csm::daemon::Exception("Unrecognized Daemon Role", errno );
        }

        // make sure there's a valid DaemonState in the config, then set the runmode ptr
        if( CSMDaemonConfig->GetDaemonState() == nullptr )
          throw csm::daemon::Exception("FATAL: DaemonState is null!");

        if( DaemonCore == nullptr )
          throw csm::daemon::Exception("FATAL: DaemonCore could not be established/initialized." );
        if( connHdl == nullptr )
          throw csm::daemon::Exception("FATAL: Connection-Handling could not be established." );

        CSMDaemonConfig->GetDaemonState()->SetRunModePtr( &_RunMode );

        netMgr = new csm::daemon::EventManagerNetwork(connHdl, CSMDaemonConfig->GetDaemonState(), DaemonCore->GetRetryBackOff() );
        if( !netMgr )
          throw csm::daemon::Exception( "BUG: failed to create network manager." );
        evMgrPool.push_back( netMgr );

        timerMgr = new csm::daemon::EventManagerTimer( DaemonCore->GetRetryBackOff() );
        if( !timerMgr )
          throw csm::daemon::Exception( "BUG: failed to create timer manager." );
        evMgrPool.push_back( timerMgr );

        DaemonCore->InitInfrastructure( netMgr, dbMgr, timerMgr, bdsMgr );

        thread_pool = CSMDaemonConfig->GetThreadPool();
        if ( thread_pool != nullptr)
        {
          // set up the segfault handler
          struct sigaction act;
          sigset_t set;

          memset(&act, 0, sizeof(act));
          sigemptyset(&set);
          act.sa_mask = set;
          act.sa_sigaction = csm::daemon::ThreadPool::segfault_sigaction_handler;
          act.sa_flags = SA_SIGINFO;

          if( sigaction(SIGSEGV, &act, nullptr) != 0 )
          {
            LOG( csmd, error ) << "Failed to set up segfault signal handler. Continuing without being able to catch segfaults in thread pool.";
          }

          // though dbMgr/netMgr threads are not csmi handler, we record their ids in the handler map for debugging purpose
          if ( dbMgr ) dbMgr->RegisterThreads( thread_pool );
          if ( netMgr && netMgr->GetThread() ) thread_pool->MarkHandler( netMgr->GetThread()->get_id(), std::string("NetworkManager") );
          // record the main thread id too
          thread_pool->SetMainLoopThreadId( boost::this_thread::get_id() );
        }

        threadMgr = new csm::daemon::ThreadManager( thread_pool, &evMgrPool );

        // set up the window timer tick handler
        _WindowTimer = new csm::daemon::WindowTimer( CSMDaemonConfig->GetTimerInterval(),
                                                     CSMDaemonConfig->GetWindowDuration(),
                                                     TimerTick );
        _RunMode.Transition();
        break;
      }
///////////////////////////////////////////////////////////////////////////////////////////////////
      case csm::daemon::RUN_MODE::DISCONNECTED:
      {
        // for this state, we're breaking the state-machine mechanism to avoid skipping of post-connect processing
        int conn_rc = 10;
        do
        {
          connHdl->ProcessSystemEvent( csm::daemon::SystemContent::DAEMONS_START, nullptr );
          conn_rc = connHdl->CheckConnectivity();
          if(conn_rc != 0 )
            ConnectRetry.AgainOrWait();
        } while(( KeepRunning() ) && ( _RunMode.Get() == csm::daemon::RUN_MODE::DISCONNECTED ));

        // break out of this if we're asked to exit
        if( ! KeepRunning() )
        {
          _RunMode.Transition( csm::daemon::run_mode_reason_t::REASON_EXIT );
          continue;
        }

        // check if connectivity is established
        // it may take a few iterations before the netmgr triggers the processing of the CONNECT events
        if(( _RunMode.Get() == csm::daemon::RUN_MODE::READY_RUNNING ) ||
            ( _RunMode.Get() == csm::daemon::RUN_MODE::PARTIAL_CONNECT ))
        {
          // Initial Processing Step before the main loop
          std::vector<csm::daemon::CoreEvent*> initEventList;
          DaemonCore->InitProcess(initEventList);
          for (size_t e=0; e<initEventList.size(); e++)
          {
            DaemonCore->GetEventSinks().PostEvent( *initEventList[e] );
          }
          ConnectRetry.Reset();
          CSMLOG( csmd, debug ) << "CSM Daemon connected " << csm::daemon::RUN_MODE::to_string( GetRunMode() );

          // now it's ready to register the call back
          DaemonCore->manage_thread_RegisterThreadManager( threadMgr );
        }
        break;
      }
///////////////////////////////////////////////////////////////////////////////////////////////////
      case csm::daemon::RUN_MODE::READY_RUNNING_JOB:
      {
        // enter daemonCore Jitter mitigation routine, if we return, the window is open
        DaemonCore->JitterWindow( csm::daemon::WINDOW_NO_ADVANCE );

        // make sure we don't start sleeping caused by idle loops during RUNNING_JOB mode
        DaemonCore->IdleLoopReset();

        // no break here...
      }
///////////////////////////////////////////////////////////////////////////////////////////////////
      case csm::daemon::RUN_MODE::PARTIAL_CONNECT:
        // if we lost one aggregator, we should try to reconnect from time to time

        // no break here, the rest of the mode is the same as READY_RUNNING
      case csm::daemon::RUN_MODE::READY_RUNNING:
      {
        // fetch events configured event queues
        // this fetches one (or more) events from a set of EventSources according to their priority within the set
        // if multiple jitterwindows require different EventSources/Priorities, just create multiple sets and fetch from
        // e.g. EventSources[ jitterwindowindex ]
        try {
          csm::daemon::CoreEvent *ce = nullptr;
          do {
            do {
              ce = DaemonCore->Fetch();

              // during Jitter Window mitigation, we reset on each main loop:
              //    won't hit sleep() in Again() because offset is non-zero
              if( ce != nullptr )
              {
                DaemonCore->IdleLoopReset();
                ++csm::daemon::DaemonState::event_count;
              }
              else
                DaemonCore->IdleLoopAgainWait( GetRunMode() == csm::daemon::RUN_MODE::READY_RUNNING_JOB );

              // process events (including message sending
            } while( ( ( ce == nullptr ) || ( ! ce->IsValid() ) ) &&
                     ( KeepRunning() ) && IsRunningMode() );
          } while( ( ( ce == nullptr ) || ( ! ce->IsValid() ) ) &&
                   ( KeepRunning() ) && IsRunningMode() );

          // stop this processing immediately if we're no longer in running mode!
          if( ( ! IsRunningMode() ) &&
              ( GetRunMode() != csm::daemon::RUN_MODE::READY_RUNNING_JOB ) )
            continue;

          // break out of this if we're asked to exit
          if( ! KeepRunning() )
          {
            _RunMode.Transition( csm::daemon::run_mode_reason_t::REASON_EXIT );
            continue;
          }

          // need this checking when in READY_RUNNING_JOB. The earlier loop may exit with ce = nullptr
          if (ce == nullptr) continue;

          CSMI_BASE *handler = DaemonCore->GetEventHandler( *ce );

          CSMLOG(csmd, debug) << "==== [" << csm::daemon::DaemonState::event_count
              << "] event_type=" << csm::daemon::EventTypeToString( ce->GetEventType() )
              << " handler=" << handler->getCmdName()
              << " ====";

          if ( DaemonCore->AssignToWorkerThread(handler->getCmdType()) && thread_pool )
          {
            // check if there is any crashed thread. If yes, try to recreate.
            if ( thread_pool->HasThreadCrashed() )
              thread_pool->Recover();

            thread_pool->enqueue([=]()
            {
              CSMLOG(csmd, trace) << "Assigned thread(tid=" << boost::this_thread::get_id() <<
                    ") to the handler(" << handler->getCmdName() << ")";
              //MarkHandler will handle the locking
              thread_pool->MarkHandler( boost::this_thread::get_id(), handler->getCmdName() );
              // here, handlerProcess & reason have to be a local variable to the thread
              InvokeHandler handlerProcess( handler, DaemonCore);
              csm::daemon::run_mode_reason_t reason = handlerProcess(ce);
              //RemoveHandler will handle the locking
              thread_pool->RemoveHandler( boost::this_thread::get_id() );
              // have to switch the runmode here inside the worker thread
              if (reason == REASON_JOB)
              {
                DaemonCore->JitterWindow( csm::daemon::WINDOW_RESET );
                _RunMode.Transition(reason);
              }
            }
            );
          }
          else
          {
            // use the same main loop thread for the event handler
            csm::daemon::run_mode_reason_t reason = InvokeHandler( handler, DaemonCore)(ce);
            if (reason == REASON_JOB)
            {
              DaemonCore->JitterWindow( csm::daemon::WINDOW_RESET );
              _RunMode.Transition(reason);
            }
          }
        }
        catch ( csm::network::Exception &e )
        {
          CSMLOG(csmd,error) << "Network Access Error: " << e.what();
        }
        catch ( csm::daemon::Exception &e )
        {
          CSMLOG(csmd, error) << "CSM Daemon Exception: " << e.what();
        }
        catch (boost::archive::archive_exception &e)
        {
          CSMLOG(csmd, error) << "Boost archive_exception: " << e.what();
        }
        catch (std::exception &e)
        {
          CSMLOG(csmd,error) << "FATAL: Hit error in daemon main loop (" << e.what() << ")";
          _RunMode.Transition( csm::daemon::REASON_ERROR, EINVAL );
        }

        break;
      }
///////////////////////////////////////////////////////////////////////////////////////////////////
      case csm::daemon::RUN_MODE::CLEANUP:
      {
        CSMDaemonRole role = ( CSMDaemonConfig != nullptr )? CSMDaemonConfig->GetRole() : CSM_DAEMON_ROLE_UNKNOWN;
        CSMLOG( csmd, info ) << "Cleanup " << role << " Daemon with rc = " << _RunMode.GetErrorCode();
        //cleanup
        //delete eventRouting;

        // stop the window tick
        if( _WindowTimer )
        {
          delete _WindowTimer;
          _WindowTimer = nullptr;
        }

        if( threadMgr )
        {
          delete threadMgr;
          threadMgr = nullptr;
        }
        if( DaemonCore )
        {
          delete DaemonCore;
          DaemonCore = nullptr;
        }

        // done by ~DaemonCore  if( netMgr ) delete netMgr;
        // done by ~DaemonCore  if( dbMgr ) delete dbMgr;
        // done by ~DaemonCore  if( timerMgr ) delete timerMgr;
        if( connHdl )
        {
          delete connHdl;
          connHdl = nullptr;
        }

        // if there is any crashed thread, try to remove it from thread_group before exiting.
        if ( thread_pool && thread_pool->HasThreadCrashed() )
          thread_pool->Recover(false);

        if( CSMDaemonConfig )
          CSMDaemonConfig->Cleanup();
        _RunMode.Transition();
        break;
      }
///////////////////////////////////////////////////////////////////////////////////////////////////
      case csm::daemon::RUN_MODE::EXIT:
      {
        // can't use config here any more since we already cleaned up in CLEANUP state
        // reusing config would cause recreation of the configuration instance or use-after-free issues
        CSMLOG( csmd, info ) << "Exiting Daemon... rc = " << _RunMode.GetErrorCode();
        Stop();
        break;
      }
///////////////////////////////////////////////////////////////////////////////////////////////////
      default:
        throw csm::daemon::Exception("BUG: UNRECOGNIZED RUNMODE DETECTED. CAN'T CONTINUE.");
    }

  } while((GetRunMode() != csm::daemon::RUN_MODE::EXIT));

  return _RunMode.GetErrorCode();
}

/* ticks at the frequency of the window interval to advance the jitter window index */
//void TimerTick(int sig, siginfo_t *si, void *arg)
void TimerTick()
{
//  LOG(csmd, trace) << "SCHED: Entering Window Tick Handler.";
  DaemonCore->JitterWindow( csm::daemon::WINDOW_ADVANCE );
}

#undef logprefix

