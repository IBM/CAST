/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_stateful_forward/CSMIStatefulForwardRequest.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "CSMIStatefulForwardRequest.h"

bool StatefulForwardRequest::HandleNetworkMessage(
        const csm::network::MessageAndAddress content,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        csm::daemon::EventContextHandlerState_sptr ctx ) 
{
    LOG( csmapi, trace ) << "StatefulForwardRequest::HandleNetworkMessage: Enter";
    
    // Track whether the forwarding was successful.
    bool success = this->ForwardToMaster(content._Msg, ctx, postEventList);

    LOG( csmapi, trace ) << "StatefulForwardRequest::HandleNetworkMessage: Exit";
    return success;
}

