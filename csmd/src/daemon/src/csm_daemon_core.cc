/*================================================================================

    csmd/src/daemon/src/csm_daemon_core.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <strings.h>
#include <vector>
#include <unordered_set>

#include "csm_daemon_config.h"
#include "include/jitter_window.h"

#include "include/csm_core_event.h"
#include "include/csm_timer_event.h"
#include "include/csm_event_source.h"
#include "include/csm_event_source_set.h"
#include "include/csm_event_sink.h"
#include "include/csm_event_sink_set.h"
#include "include/csm_event_routing.h"

#include "include/csm_event_manager.h"
#include "include/csm_daemon_network_manager.h"
#include "include/csm_db_manager.h"
#include "include/csm_timer_manager.h"

#include "src/csmi_request_handler/csmi_base.h"

#include "csmi/src/common/include/csmi_cmds.h"


#include "include/thread_pool.h"
#include "include/thread_manager.h"

#include "src/csm_event_sources/csm_source_environmental.h"
#include "include/csm_daemon_core.h"

csm::daemon::CoreGeneric::CoreGeneric()
: _netMgr(nullptr),
  _dbMgr(nullptr),
  _timerMgr(nullptr),
  _bdsMgr(nullptr),
  _envSource(nullptr),
  _intervalSrc(nullptr),
  _EventRouting(nullptr),
  _threadMgr(nullptr),
  _IdleLoopRetry( "MainLoop",
                  RetryBackOff::SleepType::CONDITIONAL,  // during regular operation
                  RetryBackOff::SleepType::CONDITIONAL, // during job-running mode
                  1, 1000000, 1000 )
{
  srand( time(nullptr) );

  // populate the csmi_cmd_t list which can only be done by the main loop thread
  // instead of the worker threads
  _cmdsByMainThread.insert( (int) CSM_CTRL_cmd );
  _Config = csm::daemon::Configuration::Instance();

  bzero( &_WindowInterval, sizeof( _WindowInterval ) );
  _WindowInterval.it_interval.tv_sec = _Config->GetTimerInterval() / 1000000;
  _WindowInterval.it_interval.tv_usec = _Config->GetTimerInterval() % 1000000;
  _WindowInterval.it_value.tv_sec =  _Config->GetTimerInterval()/ 1000000;
  _WindowInterval.it_value.tv_usec = _Config->GetTimerInterval()  % 1000000;
  _ActiveWindow = -1;
  _WindowMax = _Config->GetLCMForBuckets() + 1;
  LOG( csmd, debug ) << "Setting up DaemonCore: " << _Config->GetRole()
     << " WindowMax=" << _WindowMax
     << " Interval=" << _Config->GetTimerInterval() << "µs";
}

// call this only after _EventRouting is initialized by the daemons
void
csm::daemon::CoreGeneric::InitInfrastructure(const csm::daemon::EventManagerNetwork *netMgr,
                                             const csm::daemon::EventManagerDB *dbMgr,
                                             const csm::daemon::EventManagerTimer *timerMgr,
                                             const csm::daemon::EventManagerBDS *bdsMgr )
{
  // specify the command type of the handler to have access to all public api handlers
  AddPublicAPIHandlers(CSM_CTRL_cmd);

  if( netMgr )
  {
    _netMgr = const_cast<csm::daemon::EventManagerNetwork*>( netMgr );

    // create available event sources
    AddEventSource( netMgr->GetEventSource(), NETWORK_SRC_ID, 0, DEFAULT_NETWORK_SRC_INTERVAL );
    _EventSinks.Add( csm::daemon::EVENT_TYPE_NETWORK, netMgr->GetEventSink() );
  }
  else
    throw csm::daemon::Exception("BUG: no net-mgr defined. Cannot continue.");

  // to-do: will force the Master to have dbMgr to continue
  if ( dbMgr ) {
    _dbMgr = const_cast<csm::daemon::EventManagerDB*>( dbMgr );
    AddEventSource( dbMgr->GetEventSource(), DB_SRC_ID, 0, DEFAULT_DB_SRC_INTERVAL );
    _EventSinks.Add( csm::daemon::EVENT_TYPE_DB_Request, dbMgr->GetEventSink() );
  }

  if( timerMgr )
  {
    _timerMgr = const_cast<csm::daemon::EventManagerTimer*>( timerMgr );
    AddEventSource( timerMgr->GetEventSource(), TIMER_SRC_ID, 0, DEFAULT_TIMER_SRC_INTERVAL );
    _EventSinks.Add(csm::daemon::EVENT_TYPE_TIMER, timerMgr->GetEventSink() );
  }
  else
    throw csm::daemon::Exception("BUG: no timer-mgr defined. Cannot continue.");

  csm::daemon::RecurringTasks RT = _Config->GetRecurringTasks();
  if( RT.IsEnabled() )
  {
    _intervalSrc = new csm::daemon::EventSourceInterval( GetRetryBackOff() );
    AddEventSource( _intervalSrc, INTERVAL_SRC_ID, 0, RT.GetMinInterval() );
  }

  if( bdsMgr )
  {
    _bdsMgr = const_cast<csm::daemon::EventManagerBDS*>( bdsMgr );
    _EventSinks.Add( csm::daemon::EVENT_TYPE_BDS, _bdsMgr->GetEventSink() );
  }

  csm::daemon::EventSink* systemSink = new csm::daemon::EventSinkSystem( _netMgr->GetConnectionHandling() );
  _EventSinks.Add( csm::daemon::EVENT_TYPE_SYSTEM, systemSink );

  //csm::daemon::EventSourceEnvironmental* envSource = new csm::daemon::EventSourceEnvironmental();
  //AddEventSource(envSource, "ENVIRONMENTAL");
  // make sure to remove from source set and delete in destructor
}

void
csm::daemon::CoreGeneric::DestroyInfrastructure()
{
  if( _netMgr )
  {
    _EventSources.Remove( _netMgr->GetEventSource(), 0 );
    _EventSinks.Remove( csm::daemon::EVENT_TYPE_NETWORK );
    delete _netMgr;
    _netMgr = nullptr;
  }

  if( _timerMgr )
  {
    _EventSources.Remove( _timerMgr->GetEventSource(), 0 );
    _EventSinks.Remove( csm::daemon::EVENT_TYPE_TIMER );
    delete _timerMgr;
    _timerMgr = nullptr;
  }

  if( _bdsMgr )
  {
    _EventSinks.Remove( csm::daemon::EVENT_TYPE_BDS );
    delete _bdsMgr;
    _bdsMgr = nullptr;
  }

  if(_envSource)
  {
    _EventSources.Remove( _envSource, 0 );
    delete _envSource;
  }

  if( _intervalSrc )
  {
    _EventSources.Remove( _intervalSrc, 0 );
    delete _intervalSrc;
  }

  if( _dbMgr )
  {
    _EventSources.Remove( _dbMgr->GetEventSource(), 0 );
    _EventSinks.Remove( csm::daemon::EVENT_TYPE_DB_Request );
    delete _dbMgr;
    _dbMgr = nullptr;
  }

  _EventSinks.Clear();
  if (_EventRouting) delete _EventRouting;
}

void
csm::daemon::CoreGeneric::AddEventSource( const csm::daemon::EventSource *aNewSource,
                                          const uint64_t aIdentifier,
                                          const uint8_t aBucketID,
                                          const uint64_t aInterval )
{
  // todo: enable buckets of event sources
  // todo: calculate window cycle skip from number of windows and jitter window interval
  try {
    _EventSources.Add( aNewSource, aIdentifier, aBucketID, aInterval );
  }
  catch ( csm::daemon::EventSourceException &e ) {
    LOG(csmd,error) << "Error adding event source ID=" << aIdentifier << " to bucket=" << aBucketID;
  }
}


CSMI_BASE *
csm::daemon::CoreGeneric::GetEventHandler(const csm::daemon::CoreEvent &aEvent)
{
  // CSMI_BASE constructor is guarded by the CSM_CMD_MAX. The cmd has to be less than CSM_CMD_MAX
  uint8_t cmd = CSM_CMD_MAX;
  if (aEvent.HasSameContentTypeAs( typeid( csm::daemon::EventContentContainer<csm::network::MessageAndAddress> ) ) )
  {
    csm::daemon::NetworkEvent *ev = (csm::daemon::NetworkEvent *)&aEvent;
    csm::network::MessageAndAddress content = ev->GetContent();
    cmd = content._Msg.GetCommandType();

    LOG(csmd, debug) << "CSM Command Type: " << csmi_cmds_to_str( cmd );
  }
  // if cmd is not CSM_CMD_MAX at this point, it means aEvent is a NetworkEvent.

  CSMI_BASE *handler=nullptr;
  csm::daemon::EventContext_sptr ctx = aEvent.GetEventContext();
  if (ctx != nullptr)
  {
    if ( (handler = (CSMI_BASE *) ctx->GetEventHandler()) )
    {
      LOG(csmd, debug) << "GetEventHandler(): Get Event Handler from the stored Context:" << handler->getCmdName();
      uint8_t cmd_in_handler = handler->getCmdType();
      if (cmd > CSM_CMD_UNDEFINED && cmd_in_handler > CSM_CMD_UNDEFINED &&
          cmd < CSM_FIRST_INTERNAL && cmd_in_handler < CSM_FIRST_INTERNAL &&
          cmd != cmd_in_handler)
      {
        LOG(csmd, warning) << "GetEventHandler(): The handler in the context has different csmi command type. command_in_handler = " << (int) cmd_in_handler << " command_in_aEvent = " << (int) cmd << ".";
      }

    }
    else LOG(csmd, warning) << "GetEventHandler(): Event Handler is NULL in the Event Context";
  }

  // the environmental event should have the nullptr context
  if (aEvent.GetEventType() == EVENT_TYPE_ENVIRONMENTAL)
  {
    handler = GetEnvironmentalHandler();
  }

  // a timer event with a nullptr context is either an error or is the magic interval trigger
  if(( aEvent.GetEventType() == EVENT_TYPE_TIMER ) && ( aEvent.GetEventContext() == nullptr ))
  {
    const csm::daemon::TimerEvent *te = dynamic_cast<const csm::daemon::TimerEvent*>( &aEvent );
    if(( te != nullptr ) && ( te->GetContent().GetTimerInterval() == EVENT_CTX_INTERVAL_MAGIC ))
    {
      LOG( csmd, trace ) << "Found Magic-number in Timer. Triggering interval handler.";
      handler = GetIntervalHandler();
    }
  }

  // if no context or event pointer in context is null, try to get the event handler
  // based on the command type if aEvent is a NetworkEvent
  if( handler == nullptr && cmd != CSM_CMD_MAX )
  {

    // Get the event handler from the Routing class
    if (_EventRouting) handler = _EventRouting->GetEventHandler((csmi_cmd_t) cmd);

    if (handler)
    {
      handler->UpdateNewRequestCount();

      LOG(csmd, debug) << "GetEventHandler(): Get Event Handler from command type:" << handler->getCmdName();
    }
    else
    {
      if ((handler = GetDefaultEventHandler()))
      {
        LOG(csmd, debug) << "GetEventHandler(): Use the Default Event Handler:" << handler->getCmdName()
            << " for command: " << cmd << " evType: " << aEvent.GetEventType();
      }
    }

  }

  // NOTE: will only come here at Master as Aggregator and Utility have their own default event
  //       handler. Is this what we want?
  if (handler == nullptr) {
    handler = GetErrorEventHandler();

    LOG(csmd, error) << "GetEventHandler(): Cannot find a registered handler. Return the ErrorEventHandler!";

  }
  return handler;
}
