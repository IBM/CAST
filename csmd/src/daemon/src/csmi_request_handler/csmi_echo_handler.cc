/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_echo_handler.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
// implement the CSM api node attributes command...
//

#include "csmi_echo_handler.h"

#include "logging.h"

void CSMI_ECHO_HANDLER::Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList )
{    
    // for now, just echo

    LOG(csmd,debug) << "CSMI_ECHO_HANDLER: Processing";

    if( aEvent.HasSameContentTypeAs( typeid( csm::daemon::EventContentContainer<csm::network::MessageAndAddress> ) ) )
    {
      csm::daemon::NetworkEvent *ev = (csm::daemon::NetworkEvent*)&aEvent;
      csm::network::MessageAndAddress content = ev->GetContent();
      csm::network::Address_sptr addr = content.GetAddr();
      if( addr != nullptr )
      {
        LOG(csmd,debug) << "CSMI_ECHO_HANDLER: Found expected network event type. RemoteAddr: " << addr->Dump();
      }
      else
      {
        LOG(csmd,debug) << "CSMI_ECHO_HANDLER: Found expected network event type. RemoteAddr: NULL";
      }
    }
    else
    {
      LOG(csmd,warning) << "CSMI_ECHO_HANDLER:: Expecting network Event type: " << aEvent.HasSameContentTypeAs( typeid( csm::daemon::EventContentContainer<csm::network::MessageAndAddress> ) );
    }

    //postEventList.push_back( (csm::daemon::CoreEvent *) &aEvent );
    postEventList.push_back( CopyEvent(aEvent) );
}
