/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csm_handler_options.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include "csm_handler_options.h"


std::string csm::daemon::HandlerOptions::_mqReqTopic;
std::string csm::daemon::HandlerOptions::_mqReplyTopic;
std::string csm::daemon::HandlerOptions::_rasTopicPrefix;
csm::db::DBConnectionPool * csm::daemon::HandlerOptions::_DBConnectionPool=nullptr;
csm::daemon::DaemonState* csm::daemon::HandlerOptions::_DaemonState=nullptr;
CSMDaemonRole csm::daemon::HandlerOptions::_DaemonRole=CSM_DAEMON_ROLE_UNKNOWN;
csm::daemon::CSMIAuthList_sptr csm::daemon::HandlerOptions::_AuthList=nullptr;
csm::daemon::Configuration* csm::daemon::HandlerOptions::csmConfig=nullptr;
