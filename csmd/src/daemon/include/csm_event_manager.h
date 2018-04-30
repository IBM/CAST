/*================================================================================

    csmd/src/daemon/include/csm_event_manager.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSM_DAEMON_SRC_CSM_EVENT_MANAGER_H_
#define CSM_DAEMON_SRC_CSM_EVENT_MANAGER_H_

#include <deque>

#include "include/csm_core_event.h"
#include "include/csm_event_source.h"
#include "include/csm_event_sink.h"

namespace csm {
namespace daemon {

class EventManager
{
protected:
  EventSource *_Source;
  EventSink *_Sink;

public:
  EventManager( EventSource *i_Source = nullptr,
                EventSink *i_Sink = nullptr )
  : _Source( i_Source ),
    _Sink( i_Sink )
  {}
  virtual ~EventManager()
  {
    if( _Source ) delete _Source;
    if( _Sink ) delete _Sink;
    _Source = nullptr;
    _Sink = nullptr;
  }
  virtual int Freeze() { return 0; }
  virtual int Unfreeze() { return 0; }
  inline EventSource* GetEventSource() const { return _Source; }
  inline EventSink* GetEventSink() const { return _Sink; }
};

}  // namespace daemon
} // namespace csm

#endif /* CSM_DAEMON_SRC_CSM_EVENT_MANAGER_H_ */
