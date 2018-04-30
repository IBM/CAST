/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_error_handler.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSM_DAEMON_SRC_CSMI_ERROR_HANDLER_H__
#define __CSM_DAEMON_SRC_CSMI_ERROR_HANDLER_H__

#include "csmi_base.h"
#include "csmnet/src/CPP/csm_network_msg_cpp.h"
#include "csmnet/src/CPP/address.h"



#include <map>

class CSMI_ERROR_HANDLER : public CSMI_BASE {
 
public:
  // this handler not tied to any specific csmi cmd
  CSMI_ERROR_HANDLER(csm::daemon::HandlerOptions& options)
  : CSMI_BASE(CSM_CMD_ERROR, options)
  {
    _errcode = CSMERR_CMD_UNKNOWN;
    _errmsg = std::string("CSM_ERROR -- CSMI CMD Unknown To Daemon");
  }
  
public:

  virtual void SetErrInfo(csmi_cmd_err_t errcode, std::string errmsg)
  {
    _errcode = errcode;
    _errmsg = errmsg;
  }
  
  virtual void SetErrCode(csmi_cmd_err_t errcode)
  {
    _errcode = errcode;
  }
  
  virtual void SetErrMsg(std::string errmsg)
  {
    _errmsg = errmsg;
  }
  
  virtual void Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList );
    
  virtual ~CSMI_ERROR_HANDLER() { }
  
private:
  csmi_cmd_err_t _errcode;
  std::string _errmsg;
};

#endif

