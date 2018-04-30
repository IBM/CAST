/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_system_event_state.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSMI_SYSTEM_EVENT_STATE_H__
#define __CSMI_SYSTEM_EVENT_STATE_H__
#include "csmi_handler_state.h"

/** @brief The base class for states that expect System events. */
class SystemEventState: public CSMIHandlerState
{
    using CSMIHandlerState::CSMIHandlerState;
public:
    /** @brief Identical parameter list to the CSMI_BASE::process function.
     *
     * Note: This is the finalization of the Process function.
     *
     * @param ctx The context of the event. Cast in the invoking function to 
     *  prevent every implementation of Process from having perform error checking.
     * @param aEvent The event that triggered this call.
     * @param postEventList The list of events to push new events to.
     */
    virtual void Process( 
        csm::daemon::EventContextHandlerState_sptr ctx,
        const csm::daemon::CoreEvent &aEvent, 
        std::vector<csm::daemon::CoreEvent*>& postEventList ) final
    {
        //TODO 
    }

protected:
    /** @brief Handles the parsing and storage of the recieved SystemEvents.
     */
    virtual void ParseSystemEvent( csm::daemon::EventContextHandlerState_sptr ctx ) = 0;

};

#endif


