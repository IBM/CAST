/*================================================================================

    csmd/src/daemon/include/invoke_handler.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_INVOKE_HANDLER_H_
#define CSMD_SRC_DAEMON_INCLUDE_INVOKE_HANDLER_H_

class InvokeHandler
{
private:
  CSMI_BASE *_handler;
  csm::daemon::CoreGeneric *_daemonCore;

public:
  InvokeHandler(CSMI_BASE *aHandler, csm::daemon::CoreGeneric *aDaemonCore)
  :_handler(aHandler), _daemonCore(aDaemonCore)
  { }

  /* note: this only catches the REASON_JOB for now. Ignore the other reasons returned from PostEventList. */
  csm::daemon::run_mode_reason_t operator()(csm::daemon::CoreEvent *i_o_event)
  {
    std::vector<csm::daemon::CoreEvent*> postEventList;
    /*
    auto t0 = std::chrono::steady_clock::now();
    */
    _handler->Process( *i_o_event, postEventList);
    /*
    auto t1 = std::chrono::steady_clock::now();
    */
    int debugging = 0;
    csm::daemon::run_mode_reason_t reason = csm::daemon::REASON_UNSPEC;
    for (size_t e=0; e<postEventList.size(); e++)
    {
      csm::daemon::run_mode_reason_t reason_for_event = _daemonCore->GetEventSinks().PostEvent( *postEventList[e] );
      /* Only update the "reason" if the returned value of PostEvent() is a REASON_JOB */ \
      if (reason_for_event == csm::daemon::REASON_JOB) 
      { 
        delete postEventList[e];
        reason = reason_for_event; 
        debugging++; 
      }
    }
    if (debugging > 1)
    {
      LOG(csmd, error) << "A handler returns more than one event for REASON_JOB";
    }

    delete i_o_event;
    /*
    double ms = std::chrono::duration<double, std::milli>(t1-t0).count();
    DaemonState->RecordPerfData(handler->getCmdName(), ms);
    */
    return reason;
  }
};


#endif /* CSMD_SRC_DAEMON_INCLUDE_INVOKE_HANDLER_H_ */
