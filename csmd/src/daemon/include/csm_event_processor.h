/*================================================================================

    csmd/src/daemon/include/csm_event_processor.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/* csm_daemon_event_processor.h
 *
 ******************************************/

#ifndef CSM_DAEMON_SRC_CSM_EVENT_TYPE_H_
#define CSM_DAEMON_SRC_CSM_EVENT_TYPE_H_

#include <cstdlib>
#include <inttypes.h>
#include <deque>
#include <exception>

#include "include/csm_event_type_definitions.h"
#include "include/csm_core_event.h"
#include "include/csm_event_sink.h"
#include "include/csm_event_source.h"

namespace csm {
namespace daemon {

typedef std::exception EventProcessorException;

class EventProcessor {
  csm::daemon::EventType mType;
  csm::daemon::EventSource *mSource;
  csm::daemon::EventSink *mSink;

public:
  EventProcessor( const csm::daemon::EventType aType,
                  const csm::daemon::EventSource *aSource,
                  const csm::daemon::EventSink *aSink )
                  : mType( aType ),
                    mSource( (csm::daemon::EventSource*)aSource ),
                    mSink( (csm::daemon::EventSink*)aSink )
  {}
  EventProcessor( const csm::daemon::EventType aType)
                  : mType( aType )
  {}
  virtual ~EventProcessor() {}

  virtual void AddEventSource(csm::daemon::EventSource *aSource)
  {
    mSource = aSource;
  }
  
  virtual void AddEventSink(csm::daemon::EventSink *aSink)
  {
    mSink = aSink;
  }
  
  virtual int Process( const csm::daemon::CoreEvent &aEvent ) = 0;
  inline csm::daemon::EventType GetType() const { return mType; }

protected:
  inline csm::daemon::EventSource * GetSource() { return mSource; }
  inline csm::daemon::EventSink * GetSink() { return mSink; }
};

}  // namespace daemon
} // namespace csm

//#include "src/csm_event_processors/csm_processor_db_request.h"
//#include "src/csm_event_processors/csm_processor_rassub_request.h"
//#include "src/csm_event_processors/csm_processor_mqtt.h"
//#include "src/csm_event_processors/csm_processor_echo.h"
//#include "src/csm_event_processors/csm_processor_node_inv.h"
//#include "src/csm_event_processors/csm_processor_agent.h"

//class CSMEventProcessorGeneric {
//  // priority and ranking will be useful for sorting of event sources within a particular set
//  uint8_t mPriority;
//  uint32_t mRanking;
//
//public:
//  CSMEventProcessorGeneric( const uint8_t aPriority = 0,
//                         const uint32_t aRanking = 0 )
//                         : mPriority( aPriority ),
//                           mRanking( aRanking )
//  {}
//  virtual ~CSMEventProcessorGeneric() {}
//
//  // retrieve an event if this processor has anything that generates events
//  virtual CSMCoreEvent& GetEvent() = 0;
//
//  // push an event to the event manager of this processor
//  virtual void PostEvent( const CSMCoreEvent &aEvent ) = 0;
//
//  // process any events that specific to this processor
//  virtual CSMCoreEvent& Process( const CSMCoreEvent &aEvent ) = 0;
//
//  bool operator<( const CSMEventProcessorGeneric &aOther ) const
//  {
//    return ( (mPriority < aOther.mPriority ) ||
//        ( ( mPriority == aOther.mPriority ) && ( mRanking < aOther.mRanking ) ));
//  }
//};
//
//template<class EventManagerType>
//class CSMEventProcessorWithManager : public CSMEventProcessorGeneric
//{
//  // missing here: eventmanager could be different for each event source
//  // could be a simple queue or stack or something more complex like the network interface
//  EventManagerType *mEventManager;
//
//public:
//  CSMEventProcessorWithManager( EventManagerType *aEventManager,
//                                const uint8_t aPriority = 0,
//                                const uint32_t aRanking = 0 )
//                                : CSMEventProcessorGeneric( aPriority,
//                                                            aRanking )
//  {
//    mEventManager = aEventManager;
//  }
//
//  inline void SetEventManager( EventManagerType *aEventManager ) { mEventManager = aEventManager; }
//  inline EventManagerType* GetEventManager() { return mEventManager; }
//
//
//};
//
//class CSMEventProcessorTest : public CSMEventProcessorWithManager<CSMQueueEventManager>
//{
//
//
//public:
//  CSMEventProcessorTest( CSMQueueEventManager *aEventManager,
//                      const uint8_t aPriority = 0,
//                      const uint32_t aRanking = 0 )
//                      : CSMEventProcessorWithManager( aEventManager,
//                                                      aPriority,
//                                                      aRanking )
//  {
//  }
//  virtual ~CSMEventProcessorTest() {}
//  virtual CSMCoreEvent& GetEvent()
//  {
//    CSMCoreEvent *ev = new CSMCoreEvent( GetEventManager()->front(), (const CSMEventProcessorTest*)this );
//    return *ev;
//  }
//  virtual CSMCoreEvent& Process( const CSMCoreEvent &aEvent )
//  {
//    return *(new CSMCoreEvent( random(), this ));
//  }
//  virtual void PostEvent( const CSMCoreEvent &aEvent )
//  {
//    CSMEventProcessorWithManager *EVM = (CSMEventProcessorWithManager*)aEvent.GetEventProcessor();
//    EVM->GetEventManager()->push_back( aEvent.GetEventNumber() );
//  }
//};
//

#endif /* CSM_DAEMON_SRC_CSM_EVENT_TYPE_H_ */
