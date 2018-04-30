/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csm_conn_ctrl_handler.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
// implement the CSM connection control command responder
//

#include "csm_conn_ctrl_handler.h"

#include "logging.h"

void CONN_CTRL_HANDLER::Process( const csm::daemon::CoreEvent &aEvent, std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  LOG(csmd,info) << "CONN_CTRL_HANDLER: Processing: " << EventTypeToString( aEvent.GetEventType() );

  if (aEvent.GetEventType() == csm::daemon::EVENT_TYPE_INITIAL)
  {
    LOG( csmd, debug ) << "Creating initial RESTART message to the primary aggregator: " << _AbstractAggregator->Dump();
    csm::network::MessageAndAddress restartMsg;
    restartMsg.SetAddr( _AbstractAggregator );
    restartMsg._Msg.Init( CSM_CMD_CONNECTION_CTRL,
                      CSM_HEADER_INT_BIT,
                      CSM_PRIORITY_DEFAULT,
                      0,
                      0x0, 0x0,
                      geteuid(), getegid(),
                      std::string( CSM_RESTART_MSG ));

    postEventList.push_back( CreateNetworkEvent(restartMsg, CreateContext((void *)this, 0)) );
  }
  else if( isSystemConnectedEvent( aEvent ) )
  {
  }
  else
    LOG(csmd,warning) << "CONN_CTRL_HANDLER: unexpected event for connection_ctrl";
}
