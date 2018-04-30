/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/helpers/NetworkHelpers.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

#ifndef _NETWORK_HELPERS_H_
#define _NETWORK_HELPERS_H_
#include "EventHelpers.h"

namespace csm {
namespace daemon {
namespace helper {

/** @brief Creates a NetworkEvent with MessageId set to 0.
 *  @ingroup Messages_And_Address
 * 
 *  @param[in] msg      A message to use in the NetworkEvent, its MessageId will be set to zero before creating the event. 
 *  @param[in] addr     The Address to send the message to.
 *  @param[in] aContext The context associated with the returned NetworkEvent.
 *  
 *  @return The constructed NetworkEvent. If this event is unable to be constructed a nullptr will be returned.
 */
inline csm::daemon::NetworkEvent* ForwardNetworkEventWithMessageId0(
                                            csm::network::Message msg,
                                            const csm::network::Address_sptr addr,
                                            const csm::daemon::EventContext_sptr aContext = nullptr)
{
    msg.SetMessageID(0);
    msg.CheckSumUpdate();
    return ( csm::daemon::helper::CreateNetworkEvent(msg, addr, aContext) );
}

/** @brief Create a NetworkEvent with MessageId set to 0.
 *  @ingroup Messages_And_Address
 *
 *  @param[in] reqEvent A NetworkEvent containing a Message. This message is reused, but its id is set to zero.
 *  @param[in] addr     The Address to send the message to.
 *  @param[in] aContext The context associated with the returned NetworkEvent.
 * 
 *  @return The constructed NetworkEvent. If this event is unable to be constructed a nullptr will be returned.
 */
inline csm::daemon::NetworkEvent* ForwardNetworkEventWithMessageId0(
                                            const csm::daemon::CoreEvent& reqEvent,
                                            const csm::network::Address_sptr addr,
                                            const csm::daemon::EventContext_sptr aContext = nullptr)
{
    csm::daemon::NetworkEvent *ev = (csm::daemon::NetworkEvent *)&reqEvent;
    csm::network::MessageAndAddress reqContent = ev->GetContent();
    return ( ForwardNetworkEventWithMessageId0(reqContent._Msg, addr, aContext) );
}
    
// TODO What is a ReqEvent? -John Dunham (jdunham@us.ibm.com)
/** @brief Forward a reply NetworkEvent for an earlier request.
 *  @ingroup Messages_And_Address
 * 
 *  @param[in] respEvent  The NetworkEvent to be forwarded.
 *  @param[in] reqContext The context of the earlier request. Its address has to be a UNIX socket or TCP.
 *  @param[in] aContext   The context to associate with the forwarded NetworkEvent.
 *  
 *  @return A forwarded NetworkEvent, if the event cannot be forwarded a nullptr is returned.
 */
inline csm::daemon::NetworkEvent* ForwardReplyNetworkEvent(
                                        const csm::daemon::CoreEvent& respEvent,
                                        const csm::daemon::EventContext_sptr reqContext,
                                        const csm::daemon::EventContext_sptr aContext = nullptr)
{
    // Cast the event to a Network Event and retrieve its message.
    csm::daemon::NetworkEvent *ev = (csm::daemon::NetworkEvent *)&respEvent;
    csm::network::MessageAndAddress respContent = ev->GetContent();
  
    // Get the Request Event from the NetworkEvent
    csm::daemon::NetworkEvent *reqEvent = (csm::daemon::NetworkEvent *) reqContext->GetReqEvent();
    if (reqEvent == nullptr)
    {
        LOG(csmapi, error) << "CSMI_BASE::ForwardReplyNetworkEvent(): reqEvent is nullptr. Drop the response!";
        return nullptr;
    }
  
    // Get the Content from the Request Event.
    csm::network::MessageAndAddress reqContent = reqEvent->GetContent();
    if (reqContent.GetAddr()->GetAddrType() != csm::network::CSM_NETWORK_TYPE_LOCAL &&
            reqContent.GetAddr()->GetAddrType() != csm::network::CSM_NETWORK_TYPE_PTP)
    {
        LOG(csmapi, error) << "CSMI_BASE::ForwardReplyNetworkEvent(): Address in reqEvent is not expected.";
        return nullptr;
    }

    return ( csm::daemon::helper::CreateNetworkEvent(respContent._Msg, reqContent.GetAddr(), aContext) );              
}

/** @brief Get the MessageId field on the CoreEvent in the EventContext.
 *  @ingroup Messages_And_Address
 *  @note The CoreEvent in EventContext has to be a NetworkEvent.
 * 
 *  @param[in]  aContext The EventContext which contains the Event to retrieve the MessageId from.
 *  @param[out] msgId    The MessageId in the NetworkEvent content.
 * 
 *  @return True if the CoreEvent in the context is valid and is a NetworkEvent, false if otherwise.
 */
inline bool GetMessageIDFromRequestInContext(
                            const csm::daemon::EventContext_sptr aContext, 
                            uint64_t& msgId )
{
    csm::daemon::CoreEvent *req = aContext->GetReqEvent();

    // EARLY RETURN!
    // If the Event was null or not a Network Event return false.
    if ( req == nullptr || !(req->HasSameContentTypeAs(csm::daemon::helper::NETWORK_EVENT_TYPE) ) )
        return false;
    

    // Cast the event and retrieve the MessageId.
    csm::daemon::NetworkEvent *event = (csm::daemon::NetworkEvent *) req;
    csm::network::MessageAndAddress content = event->GetContent();
    csm::network::Message msg = content._Msg;
    msgId = msg.GetMessageID();
    
    return true;    
}

} // End namespace helpers
} // End namespace daemon
} // End namespace csm


#endif
