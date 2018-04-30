/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csm_ctrl_cmd_handler.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef __CSM_DAEMON_SRC_CSM_CTRL_CMD_HANDLER_H__
#define __CSM_DAEMON_SRC_CSM_CTRL_CMD_HANDLER_H__

#include "csmi_base.h"
#include "csmnet/src/CPP/csm_network_msg_cpp.h"
#include "csmnet/src/CPP/address.h"



#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/split_member.hpp>
#include <iostream>
#include <sstream>

#include "src/csmi_request_handler/ctrl_cmd_option.h"

class CSM_CTRL_CMD_HANDLER: public CSMI_BASE {
 
friend class CtrlCmdOption;

public:
  CSM_CTRL_CMD_HANDLER(csm::daemon::HandlerOptions& options)
  : CSMI_BASE(CSM_CTRL_cmd, options)
  { }
  
public:

  virtual void Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList );
    
  virtual ~CSM_CTRL_CMD_HANDLER() { }
  
  std::string GetMemUsage();
  std::string GetPerfData();
};

#endif

