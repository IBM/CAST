/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_generic.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/* csmi_generic.h

*

******************************************/

#ifndef CSMD_SRC_DAEMON_SRC_CSMI_REQUEST_HANDLER_CSMI_GENERIC_H_
#define CSMD_SRC_DAEMON_SRC_CSMI_REQUEST_HANDLER_CSMI_GENERIC_H_

#include "csmi_base.h"

class CSMI_GENERIC : public CSMI_BASE {

public:

virtual csm::daemon::EventContext_sptr CreateEventContext( const csm::daemon::EventType i_Type,
  const csm::daemon::CoreEvent &aEvent) = 0;

virtual int OnSystemEvent( csm::daemon::SystemContent &i_Content,
  csm::daemon::EventContext_sptr i_Context,
  std::vector<csm::daemon::CoreEvent*>& postEventList ) = 0;

virtual int OnSystemError( csm::daemon::SystemContent &i_Content,
  csm::daemon::EventContext_sptr i_Context,
  std::vector<csm::daemon::CoreEvent*>& postEventList ) = 0;

virtual int OnNetworkEvent( csm::network::MessageAndAddress &i_Content,
  csm::daemon::EventContext_sptr i_Context,
  std::vector<csm::daemon::CoreEvent*>& postEventList ) = 0;

virtual int OnNetworkError( csm::network::MessageAndAddress &i_Content,
  csm::daemon::EventContext_sptr i_Context,
  std::vector<csm::daemon::CoreEvent*>& postEventList ) = 0;

virtual int OnTimerEvent( csm::daemon::TimerContent &i_Content,
  csm::daemon::EventContext_sptr i_Context,
  std::vector<csm::daemon::CoreEvent*>& postEventList ) = 0;

virtual int OnTimerError( csm::daemon::TimerContent &i_Content,
  csm::daemon::EventContext_sptr i_Context,
  std::vector<csm::daemon::CoreEvent*>& postEventList ) = 0;

virtual int OnDBResponse( csm::db::DBRespContent &i_Content,
  csm::daemon::EventContext_sptr i_Context,
  std::vector<csm::daemon::CoreEvent*>& postEventList ) = 0;

virtual int OnDBError( csm::db::DBRespContent &i_Content,
  csm::daemon::EventContext_sptr i_Context,
  std::vector<csm::daemon::CoreEvent*>& postEventList ) = 0;

// return timeout in milliseconds
virtual uint64_t TimeToWaitReply() = 0;

virtual void Process( const csm::daemon::CoreEvent &aEvent, std::vector<csm::daemon::CoreEvent*>& postEventList )

{
  //note: adding errcode & errmsg in Context. Is it a good idea?
  
  int rc = 0;

  csm::daemon::EventType type = aEvent.GetEventType();
  csm::daemon::EventContext_sptr ctx = aEvent.GetEventContext();

  if( aEvent.GetEventContext() == nullptr )
  {
    ctx = CreateEventContext( type, aEvent );
    //note: every API needs to set up a request/reply timeout
    postEventList.push_back( CreateTimerEvent(TimeToWaitReply(), ctx) );
  }

  if ( isSystemEvent( aEvent ) )
  {
    csm::daemon::SystemContent content = dynamic_cast<const csm::daemon::SystemEvent*>(&aEvent)->GetContent();
    rc = OnSystemEvent( content, ctx, postEventList );
    if( rc )
      OnSystemError( content, ctx, postEventList );
  }
  
  if( isNetworkEvent( aEvent ) )
  {
    csm::network::MessageAndAddress content = dynamic_cast<const csm::daemon::NetworkEvent*>(&aEvent)->GetContent();
    rc = OnNetworkEvent( content, ctx, postEventList );
    if( rc )
      OnNetworkError( content, ctx, postEventList );
  }

  if( isDBRespEvent( aEvent ) )
  {
    csm::db::DBRespContent content = dynamic_cast<const csm::daemon::DBRespEvent*>(&aEvent)->GetContent();
    rc = OnDBResponse( content, ctx, postEventList );
    if( rc )
      OnDBError( content, ctx, postEventList );
  }

  if( isTimerEvent( aEvent ) )
  {
    csm::daemon::TimerContent content = dynamic_cast<const csm::daemon::TimerEvent*>(&aEvent)->GetContent();
    rc = OnTimerEvent( content, ctx, postEventList );
    if( rc )
      OnTimerError( content, ctx, postEventList );
  }
}

CSMI_GENERIC(csmi_cmd_t cmd, csm::daemon::HandlerOptions& options)
: CSMI_BASE( cmd, options )
{}

virtual ~CSMI_GENERIC() { }

};

 

#endif /* CSMD_SRC_DAEMON_SRC_CSMI_REQUEST_HANDLER_CSMI_GENERIC_H_ */