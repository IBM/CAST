/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csm_error_case_handler.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "csmi_base.h"
#include <vector>
#ifndef _CSM_ERR_INJECTION
#define _CSM_ERR_INJECTION

#define WAIT 1000

class ErrorInjectionData
{
public:
  enum Mode
  {
    TIMEOUT,
    DBLOCK,
    LARGEMSG,
    LOOP,
    BADMSG
  };
  friend class CSM_ERROR_CASE_HANDLER;
  ErrorInjectionData() : _mode(), _intarg(), drain( false ) {}

  void SetMode(Mode m) { _mode = m; }
  Mode GetMode() const { return _mode; }

  void SetIntArg(int64_t n) { _intarg = n; }
  int64_t GetIntArg() const { return _intarg; }

  void SetDrain(bool d) { drain = d; }
  bool GetDrain() const { return drain; }

  void SetEcho( int e ) { echo = e; }
  int GetEcho() const { return echo; }

private:
  Mode _mode;
  int64_t _intarg;
  bool drain = false;
  int echo = 0;

  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive &ar, const unsigned int version)
  {
    ar & _mode;
    ar & _intarg;
    ar & drain;
  }
};

class EventContextErrorInjection : public csm::daemon::EventContext
{
  public:
  EventContextErrorInjection(void *aEventHandler, uint64_t aUid, csm::daemon::CoreEvent *aReqEvent = nullptr)
  : EventContext(aEventHandler, aUid, aReqEvent){}
  
  ~EventContextErrorInjection()
  {
    for(auto& conn : conns)
    {
      csm::daemon::HandlerOptions::ReleaseDBConnection(conn);
    }
  }
private:
  friend class CSM_ERROR_CASE_HANDLER;
  std::vector<csm::db::DBConnection_sptr> conns;
  csm::db::DBConnection_sptr dbconn;
  ErrorInjectionData cached_data;
};


class CSM_ERROR_CASE_HANDLER : public CSMI_BASE
{
  public:
    CSM_ERROR_CASE_HANDLER(csm::daemon::HandlerOptions& options)
    : CSMI_BASE(CSM_CMD_UNDEFINED, options)
    { setCmdName(std::string("CSM_ERROR_CASE_HANDLER")); }

    virtual void Process(const csm::daemon::CoreEvent &aEvent, std::vector<csm::daemon::CoreEvent*>& postEventList);

  private:
    enum STAGE
    {
        INIT,
        TIMEOUT_RESPONSE,
        RELEASECONNS,
        DBREQ,
        DBRESP
    };
};

#endif
