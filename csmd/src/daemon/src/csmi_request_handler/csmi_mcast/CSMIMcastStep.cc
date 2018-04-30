/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_mcast/CSMIMcastStep.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#include "CSMIMcastStep.h"
#define STRUCT_TYPE csmi_allocation_step_mcast_context_t

template<>
CSMIMcast<STRUCT_TYPE>::~CSMIMcast()
{
    if( _Data )
    {
        csm_free_struct_ptr( STRUCT_TYPE, _Data );
        _Data = nullptr;
    }
}

template<>
void CSMIMcast<STRUCT_TYPE>::BuildMcastPayload(char** buffer, uint32_t* bufferLength)
{
    // TODO improve this function for memory usage.
    if ( _Data )
    {
        csmi_allocation_step_mcast_payload_t* mcastPayload = nullptr;
        csm_init_struct_ptr(csmi_allocation_step_mcast_payload_t, mcastPayload);

        mcastPayload->allocation_id    = _Data->allocation_id;
        mcastPayload->step_id          = _Data->step_id;
        mcastPayload->user_flags       = _Data->user_flags ? strdup(_Data->user_flags) : nullptr;
        mcastPayload->begin            = _Create;
        
        csm_serialize_struct( csmi_allocation_step_mcast_payload_t, mcastPayload,
                            buffer, bufferLength );
        csm_free_struct_ptr(csmi_allocation_step_mcast_payload_t, mcastPayload);
    }
}

template<>
std::string CSMIMcast<STRUCT_TYPE>::GenerateIdentifierString()
{
    std::string idString = "Allocation ID: ";
    if ( _Data )
        idString.append(std::to_string(_Data->allocation_id)).append( "; Step Id: ")
            .append(std::to_string(_Data->step_id));
    else 
        idString.append("NOT FOUND");

    return idString;
}

namespace csm{
namespace mcast{
namespace step{

bool ParseResponse( 
    CSMIStepMcast* mcastProps,
    const csm::network::MessageAndAddress content )
{
    LOG(csmapi,debug) << "Parsing Mcast Response";

    // Track whether or not the received payload is valid.
    bool success = false;

    // If this is not in recovery and the message length is greater than zero parse the content.
    if( mcastProps && content._Msg.GetDataLen() > 0 )
    {
        csmi_allocation_step_mcast_payload_t *allocPayload = nullptr;

        // Attempt to deserialize the struct, if it deserializes process.
        if ( csm_deserialize_struct( csmi_allocation_step_mcast_payload_t, &allocPayload,
            content._Msg.GetData().c_str(), content._Msg.GetDataLen()) == 0 )
        {
            // Always update for error recovery.
            if( allocPayload->hostname && allocPayload->begin == mcastProps->IsCreate() )
            {
                success = true;
                std::string hostname(allocPayload->hostname);
                mcastProps->SetHostError(hostname);
            }

            csm_free_struct_ptr(csmi_allocation_step_mcast_payload_t, allocPayload);
        }
    }
    
    return success;
}

}
}
}
