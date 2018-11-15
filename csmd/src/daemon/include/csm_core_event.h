/*================================================================================
   
    csmd/src/daemon/include/csm_core_event.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
/* csm_core_event.h
 *
 ******************************************/

#ifndef CSM_DAEMON_SRC_CSM_CORE_EVENT_H_
#define CSM_DAEMON_SRC_CSM_CORE_EVENT_H_

#include <iostream>
#include <fstream>
#include <typeindex>
#include <inttypes.h>
#include <memory>
#include <mutex>

#include <logging.h>
#include "include/csm_event_type_definitions.h"

namespace csm {
namespace daemon {

class CoreEvent; // class fowarding declaration. Avoid circular dependency!!!

// basic event context type
// ptr to this type can be stored with an event for any handler to extend and retrieve context
class EventContext
{

public:
    // caller is responsible to allcoate and free memory for aReqEven.
    // HandlerContext will keep this pointer
    // and is responsible for disposing it when the context object goes out of scope..
    EventContext(void *aEventHandler = nullptr, 
        uint64_t aAuxiliaryId = 0, 
        CoreEvent *aReqEvent = nullptr ) : 
            _EventHandler(aEventHandler),
            _AuxiliaryId(aAuxiliaryId),
            _ReqEvent(aReqEvent),
            _UserData(nullptr)
    { }
    
    EventContext( const EventContext &in ) : 
        _EventHandler( in._EventHandler ),
        _AuxiliaryId( in._AuxiliaryId ),
        _ReqEvent( in._ReqEvent ),
        _UserData( in._UserData)
    { }

    /* example to use the user data in the context
       int *ary = (int *) malloc(10*sizeof(int));
       GetUserDataRef() = std::shared_ptr<void>(ary, std::ptr_fun(free));
        
       data *d = new data();
       GetUserDataRef() = std::shared_ptr<void>(d)
    */
    std::shared_ptr<void>& GetUserDataRef() { return _UserData; }
    void *GetUserData()
    {
        return (_UserData == nullptr)? nullptr : _UserData.get();
    }
    
    uint64_t GetAuxiliaryId()
    {
        std::lock_guard<std::mutex> lockAux(_AuxiliaryMutex);
        return (_AuxiliaryId);
    }

    void SetAuxiliaryId(const uint64_t aAuxiliaryId)
    {
        std::lock_guard<std::mutex> lockAux(_AuxiliaryMutex);
        _AuxiliaryId = aAuxiliaryId;
    }
    
    CoreEvent* GetReqEvent() 
    { 
        std::lock_guard<std::mutex> lockReq(_ReqMutex);
        return _ReqEvent.get(); 
    }
    
    void SetReqEvent(CoreEvent *aEvent)
    {
        std::lock_guard<std::mutex> lockReq(_ReqMutex);
        _ReqEvent = std::shared_ptr<CoreEvent>(aEvent);
    }
    
    void *GetEventHandler() { return _EventHandler; }
    
    virtual ~EventContext()
    {
        _EventHandler = nullptr;
        _AuxiliaryId = 0;
        _ReqEvent = nullptr;
    }
  
private:
    void *_EventHandler;
  
    std::mutex _AuxiliaryMutex;
    uint64_t   _AuxiliaryId;

    std::shared_ptr<CoreEvent> _ReqEvent;
    std::mutex                 _ReqMutex;

    std::shared_ptr<void> _UserData;

};

typedef std::shared_ptr<EventContext> EventContext_sptr;

class EventContentBase
{
protected:
  EventContext_sptr _Context;

public:
  EventContentBase( EventContext_sptr const aContext ) : _Context( aContext ) {}
  EventContentBase( const EventContentBase &in ) : _Context( in._Context ) {}
  virtual ~EventContentBase() { _Context.reset(); }
  virtual EventContentBase *copy() const = 0;
  std::string GetTypeName() const { return std::string( typeid(*this).name() ); }
  EventContext_sptr GetContext() const { return _Context; }
private:
  EventContentBase() : _Context(nullptr) { }
};

template<typename EventContentType>
class EventContentContainer : public csm::daemon::EventContentBase
{
protected:
  EventContentType mTypedContent;

public:
  EventContentContainer( EventContentType const &aContent,
                         EventContext_sptr const aContext )
                         : EventContentBase( aContext ),
                           mTypedContent( aContent )
  {}
  EventContentContainer( const EventContentContainer &in )
  : EventContentBase( in._Context ),
    mTypedContent( in.mTypedContent )
  {}
  virtual csm::daemon::EventContentBase* copy() const
  {
    return new EventContentContainer( mTypedContent, _Context );
  }

  virtual ~EventContentContainer() {}
  virtual EventContentType GetContent() const
  {
    return mTypedContent;
  }
};

class CoreEvent
{
protected:
  csm::daemon::EventType mEventType;
  csm::daemon::EventContentBase *mContent;

public:
  CoreEvent() : mEventType( csm::daemon::EVENT_TYPE_UNKOWN ),
                mContent( nullptr ) {}

  template<typename EventContentType>
  CoreEvent( EventContentType const & aEventContent,
             const csm::daemon::EventType aEventType,
             const csm::daemon::EventContext_sptr aEventContext)
             : mEventType( aEventType ),
               mContent( new csm::daemon::EventContentContainer<EventContentType>( (EventContentType)aEventContent, aEventContext ) )
  { }

  CoreEvent( CoreEvent const & aOther )
             : mEventType( aOther.mEventType ),
               mContent( aOther.mContent? aOther.mContent->copy() : 0 )
  { }

  virtual ~CoreEvent() { if (mContent) delete mContent; }

  inline bool IsValid() const
  {
    return ( (mEventType > csm::daemon::EVENT_TYPE_UNKOWN) && ( mEventType < csm::daemon::EVENT_TYPE_MAX ) );
  }

  inline csm::daemon::EventType GetEventType() const { return mEventType; }
  inline csm::daemon::EventContext_sptr GetEventContext() const { return mContent->GetContext(); }
  inline csm::daemon::EventContentBase* GetContentBasePtr() const { return dynamic_cast<csm::daemon::EventContentBase*>(mContent); }
  inline bool HasSameContentTypeAs( const std::type_index aType ) const {
    if (mContent == nullptr) return false;
    else return aType == typeid( *(dynamic_cast<csm::daemon::EventContentBase*>(mContent)) );
  }
};

template<typename EventContentType>
class CoreEventBase : public csm::daemon::CoreEvent
{
public:
  CoreEventBase( EventContentType const & aEventContent,
                 const csm::daemon::EventType aEventType,
                 const csm::daemon::EventContext_sptr aContext = nullptr )
                 : csm::daemon::CoreEvent( aEventContent, aEventType, aContext )
  { }

  CoreEventBase( CoreEventBase const & aOther )
                 : csm::daemon::CoreEvent( aOther )
  { }

  CoreEventBase( csm::daemon::CoreEvent const & aOther )
                 : csm::daemon::CoreEvent( aOther )
  { }

  inline EventContentType GetContent() const
  {
    return ((csm::daemon::EventContentContainer<EventContentType>*)mContent)->GetContent();
  }
};

}  // namespace daemon
} // namespace csm

// include all implemented event types
#include "src/csm_event_types/csm_event_cpuinfo.h"
#include "src/csm_event_types/csm_event_logdata.h"

#endif /* CSM_DAEMON_SRC_CSM_CORE_EVENT_H_ */
