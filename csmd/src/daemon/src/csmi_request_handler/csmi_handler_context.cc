/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_handler_context.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "csmi_handler_context.h"

#include "helpers/EventHelpers.h"

namespace csm {
namespace daemon{

EventContextHandlerState::EventContextHandlerState(
    void *aEventHandler, uint64_t aUid, 
    csm::daemon::CoreEvent *aReqEvent ) : 
        csm::daemon::EventContext(aEventHandler, aUid, aReqEvent),
        _RunID(UINT32_MAX),
        _HasPrivateAccess(true),
        _UserID(-1),
        _PrivateCheck(false),
        _ExpectedNumResponses(0),
        _ReceivedNumResponses(0),
        _ErrorCode(CSMI_SUCCESS),
        _DBErrorCode(0),
        _NodeErrors({}),
        _ErrorMessage(""),
        _UserData(nullptr),
        _DataDestructor(nullptr)
    {
        if ( aReqEvent != nullptr &&
                aReqEvent->HasSameContentTypeAs(csm::daemon::helper::NETWORK_EVENT_TYPE) )
        {
            csm::daemon::NetworkEvent *network_event = (csm::daemon::NetworkEvent *)aReqEvent;
            csm::network::MessageAndAddress content = network_event->GetContent();

            // Get the runid from the message.
            _RunID = content._Msg.GetReservedID();
        }
    }

// TODO Runid.
EventContextHandlerState::EventContextHandlerState(const csm::daemon::EventContext_sptr aCtx) : 
        csm::daemon::EventContext( aCtx->GetEventHandler(), 
                                    aCtx->GetAuxiliaryId(), 
                                    aCtx->GetReqEvent()),
        _RunID(UINT32_MAX),
        _HasPrivateAccess(true),
        _ExpectedNumResponses(0),
        _ReceivedNumResponses(0),
        _ErrorCode(CSMI_SUCCESS),
        _ErrorMessage(""),
        _UserData(nullptr),
        _DataDestructor(nullptr)
    {}
   
EventContextHandlerState::~EventContextHandlerState()
{
    if ( _UserData && _DataDestructor )
    {
        // Lock the Data and Destructor.
        std::lock_guard<std::mutex> lockData(_UserDataMutex);
        std::lock_guard<std::mutex> lockDest(_DestructorMutex);

        _DataDestructor(_UserData);
        _UserData = nullptr;
    }
}

uint32_t EventContextHandlerState::GetRunID()
{ 
    return _RunID; 
}

void EventContextHandlerState::SetHasPrivateAccess( bool check )
{
    std::lock_guard<std::mutex> lock(_PrivateMutex);
    _HasPrivateAccess=check; 
}

bool EventContextHandlerState::GetHasPrivateAccess ( ) 
{ 
    std::lock_guard<std::mutex> lock(_PrivateMutex);
    return _HasPrivateAccess; 
}

void EventContextHandlerState::SetPrivateCheckConfig( int64_t userID, bool performPrivateCheck )
{
    std::lock_guard<std::mutex> lock(_UserIDMutex);
    std::lock_guard<std::mutex> lockP(_PrivateCheckMutex);
    if ( _UserID < 0) 
    {
        _UserID = userID;
        _PrivateCheck = performPrivateCheck;
    }
}

bool EventContextHandlerState::CanUserAccess(int64_t userID) 
{ 
    std::lock_guard<std::mutex> lock(_UserIDMutex);
    std::lock_guard<std::mutex> lockP(_PrivateCheckMutex);

    if ( _PrivateCheck )
        return _UserID == userID;
    else
        return true;
}

void EventContextHandlerState::SetExpectedNumResponses( uint32_t responses ) 
{ 
    std::lock_guard<std::mutex> lock(_ExpectedMutex);
    _ExpectedNumResponses = responses; 
}

uint32_t EventContextHandlerState::GetExpectedNumResponses() 
{ 
    std::lock_guard<std::mutex> lock(_ExpectedMutex);
    return _ExpectedNumResponses; 
}

void EventContextHandlerState::SetReceivedNumResponses( uint32_t responses ) 
{ 
    std::lock_guard<std::mutex> lock(_ReceivedMutex);
    _ReceivedNumResponses = responses; 
}

uint32_t EventContextHandlerState::GetReceivedNumResponses() 
{ 
    std::lock_guard<std::mutex> lock(_ReceivedMutex);
    return _ReceivedNumResponses; 
}

bool EventContextHandlerState::ResponseCheck()
{
    std::lock_guard<std::mutex> lockRecv(_ReceivedMutex);
    std::lock_guard<std::mutex> lockExpected(_ExpectedMutex);
    return _ReceivedNumResponses >= _ExpectedNumResponses;
}


void EventContextHandlerState::SetErrorCode( int errorCode ) 
{ 
    std::lock_guard<std::mutex> lock(_ErrorCodeMutex);
    _ErrorCode = errorCode; 
}

int EventContextHandlerState::GetErrorCode() 
{ 
    std::lock_guard<std::mutex> lock(_ErrorCodeMutex);
    return _ErrorCode; 
}

void EventContextHandlerState::SetDBErrorCode( int errorCode ) 
{ 
    std::lock_guard<std::mutex> lock(_DBErrorCodeMutex);
    _DBErrorCode = errorCode; 
}

int EventContextHandlerState::GetDBErrorCode() 
{ 
    std::lock_guard<std::mutex> lock(_DBErrorCodeMutex);
    return _DBErrorCode; 
}

void EventContextHandlerState::SetErrorMessage( const std::string & message ) 
{ 
    std::lock_guard<std::mutex> lock(_ErrorMessageMutex);
    _ErrorMessage = message; 
}

void EventContextHandlerState::AppendErrorMessage( const std::string & message, char joiner )
{
    std::lock_guard<std::mutex> lock(_ErrorMessageMutex);
    _ErrorMessage.append( joiner + message );
}

void EventContextHandlerState::PrependErrorMessage( const std::string & message, char joiner )
{
    std::lock_guard<std::mutex> lock(_ErrorMessageMutex);
    _ErrorMessage = message + joiner + _ErrorMessage;
}

std::string EventContextHandlerState::GenerateUniqueID()
{
    return GetCommandName() + "[" + std::to_string(_RunID) + "]";
}


const std::string EventContextHandlerState::GetErrorMessage() 
{ 
    std::lock_guard<std::mutex> lock(_ErrorMessageMutex);
    return _ErrorMessage; 
}

std::string EventContextHandlerState::GetCommandName()
{
    CSMI_BASE* handler = static_cast<CSMI_BASE*>(GetEventHandler());
    std::string name = "";
    if ( handler )
    {
        name = handler->getCmdName();
    }
    return name;
}

char* EventContextHandlerState::GetErrorSerialized(uint32_t* bufLen) 
{
    std::lock_guard<std::mutex> lock(_ErrorMessageMutex);
    std::lock_guard<std::mutex> lockEc(_ErrorCodeMutex);
    std::lock_guard<std::mutex> lockNe(_NodeErrorsMutex);
    
    // Build the error container.
    csmi_err_t error;
    csm_init_struct_versioning(&error);
    error.errcode     = _ErrorCode;
    error.errmsg      = strdup(_ErrorMessage.c_str());
    error.error_count = _NodeErrors.size();
    error.node_errors = (csm_node_error_t**)_NodeErrors.data();

    // Serialize the message.
    char* buffer = nullptr;
    csm_serialize_struct(csmi_err_t, &error, &buffer, bufLen);

    // Free the error message.
    if (error.errmsg) free(error.errmsg);

    return buffer;

}


void EventContextHandlerState::SetUserData( void* userData ) 
{ 
    // Lock the user data before performing operations.
    std::lock_guard<std::mutex> lockData(_UserDataMutex);

    if ( _UserData && _DataDestructor )
    {
        std::lock_guard<std::mutex> lockDest(_DestructorMutex);
        _DataDestructor(_UserData);
    }
    
    _UserData = userData; 
}

void EventContextHandlerState::SetDataDestructor( std::function<void(void*)>  dest ) 
{
    std::lock_guard<std::mutex> lock(_DestructorMutex);
    _DataDestructor = dest; 
}

void EventContextHandlerState::SetNodeErrors(std::vector<csm_node_error_t*> nodeErrors) 
{
    std::lock_guard<std::mutex> lock(_NodeErrorsMutex);

    // Destroy the old list (should never happen.
    while(_NodeErrors.size() > 0)
    {
        csm_free_struct_ptr(csm_node_error_t, _NodeErrors.back());
        _NodeErrors.pop_back();
    }
    
    _NodeErrors = nodeErrors;

}

std::ostream& operator<<(std::ostream& os, const EventContextHandlerState* ctx)
{
    if ( ctx ) 
        return os << "[" << std::to_string(ctx->_RunID) << "]; ";
    else
        return os << "[nil]; ";
}

}
}

