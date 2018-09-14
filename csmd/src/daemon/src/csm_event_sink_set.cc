/*================================================================================

    csmd/src/daemon/src/csm_event_sink_set.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/* csm_event_sources.cc
 *
 ******************************************/

#include <stdlib.h>

#include <logging.h>
#include "csm_network_config.h"
#include "csm_daemon_config.h"
#include "include/csm_event_sink_set.h"
#include "include/csm_system_event.h"

csm::daemon::EventSinkSet::EventSinkSet()
{
  _Sinks.clear();
  _CurrentSink= _Sinks.begin();
}

csm::daemon::EventSinkSet::~EventSinkSet()
{
  _Sinks.clear();
}

// selects sink based on event type
csm::daemon::run_mode_reason_t csm::daemon::EventSinkSet::PostEvent( const csm::daemon::CoreEvent &aEvent )
{
  if (aEvent.HasSameContentTypeAs( typeid( csm::daemon::EventContentContainer<csm::daemon::SystemContent> ) ))
  {
      csm::daemon::SystemContent content = ((csm::daemon::SystemEvent *) &aEvent)->GetContent();
      if (content.GetSignalType() == csm::daemon::SystemContent::JOB_START ||
          content.GetSignalType() == csm::daemon::SystemContent::JOB_END)
      {
        return csm::daemon::REASON_JOB;
      }
  }
  
  try
  {
    csm::daemon::EventSink *sink = (*this)[ aEvent ];
    if( sink != nullptr )
    {
      if ( sink->PostEvent( aEvent ) == 0 )
        return csm::daemon::REASON_UNSPEC;
      else
        return csm::daemon::REASON_ERROR;
    }
  }
  catch (...)
  {
    LOG(csmd, error) << "EventSinkSet::PostEvent() Error for event type: " << aEvent.GetEventType();
    return csm::daemon::REASON_ERROR;
  }
  LOG(csmd, error) << "EventSinkSet::PostEvent(): Unknown sink requested for event type:" << aEvent.GetEventType();
  return csm::daemon::REASON_ERROR;
}

// adds a new event sink
int csm::daemon::EventSinkSet::Add( const EventType aType,
                                    const csm::daemon::EventSink *aSink )
{
  try
  {
    _Sinks[ aType ] = (csm::daemon::EventSink*)aSink;

    _CurrentSink = _Sinks.begin();
  }
  catch (...)
  {
    LOG( csmd, error ) << "EventSinkSet::Add(): Failed to add event sink for type: " << aType;
  }
  return 0;
}


// removes an event sink
int csm::daemon::EventSinkSet::Remove( const EventType aType )
{
  // priority will be mostly ignored for now
  try
  {
    _Sinks.erase( aType );
    _CurrentSink = _Sinks.begin();
  }
  catch (...)
  {
    LOG( csmd, error ) << "EventSinkSet::Remove(): Failed to remove event sink for type: " << aType;
  }
  return 0;
}


csm::daemon::EventSink* csm::daemon::EventSinkSet::operator[]( const csm::daemon::CoreEvent& aEvent )
{
  std::lock_guard<std::mutex> guard( _Lock );
  if( _CurrentSink->first != aEvent.GetEventType() )
  {
    _CurrentSink = _Sinks.find( aEvent.GetEventType() );
  }

  if( _CurrentSink != _Sinks.end() )
    return _CurrentSink->second;
  else
    return nullptr;
}

void csm::daemon::EventSinkSet::Clear()
{
  std::lock_guard<std::mutex> guard( _Lock );
  _Sinks.clear();
  _CurrentSink = _Sinks.begin();
}
