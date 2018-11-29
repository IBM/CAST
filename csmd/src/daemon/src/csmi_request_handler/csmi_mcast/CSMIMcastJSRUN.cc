/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_mcast/CSMIJSRUNCMD.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#include "CSMIMcastJSRUN.h"
#include "csmi/src/wm/include/csmi_wm_type_internal.h"
#define STRUCT_TYPE csmi_jsrun_cmd_context_t

template<>
CSMIMcast<STRUCT_TYPE>::~CSMIMcast()
{
    if( _Data ) delete _Data;
    _Data = nullptr;
}

template<>
void CSMIMcast<STRUCT_TYPE>::BuildMcastPayload(char** buffer, uint32_t* bufferLength)
{
    // Generate the leaner JSRUN  payload.
    csmi_jsrun_cmd_payload_t *jsrunPayload = nullptr;
    csm_init_struct_ptr(csmi_jsrun_cmd_payload_t, jsrunPayload);

    jsrunPayload->user_id        = _Data->user_id;
    jsrunPayload->allocation_id  = _Data->allocation_id;
    jsrunPayload->kv_pairs       = strdup(_Data->kv_pairs);
    jsrunPayload->jsm_path       = strdup(_Data->jsm_path);
    jsrunPayload->type           = _Data->type;
    jsrunPayload->hostname       = strdup("");
    jsrunPayload->num_nodes      = _Data->num_nodes;
    jsrunPayload->compute_nodes  = _Data->compute_nodes;
    jsrunPayload->launch_node    = strdup(_Data->launch_node);

    csm_serialize_struct( csmi_jsrun_cmd_payload_t, jsrunPayload,
                        buffer, bufferLength );

    jsrunPayload->num_nodes     = 0;
    jsrunPayload->compute_nodes = nullptr;
    csm_free_struct_ptr(csmi_jsrun_cmd_payload_t, jsrunPayload);
}

template<>
std::string CSMIMcast<STRUCT_TYPE>::GenerateIdentifierString()
{
    std::string idString = "User ID: ";
    if ( _Data )
    {
        idString.append(std::to_string(_Data->user_id));
        idString.append(" Allocation ID: ").append(std::to_string(_Data->allocation_id));
    }
    else 
    {
        idString.append("NOT FOUND");
    }

    return idString;
}

namespace csm{
namespace mcast{
namespace wm{

bool ParseCMDResponse( 
    CSMIJSRUNCMD* mcastProps,
    const csm::network::MessageAndAddress content )
{
    LOG(csmapi,trace) << "Parsing Multicast Command Response";
    
    // Track whether or not the received payload is valid.
    bool success = false;
    
    // If this is not in recovery and the message length is greater than zero parse the content.
    if( mcastProps && content._Msg.GetDataLen() > 0 )
    {
        csmi_jsrun_cmd_payload_t *jsrunPayload = nullptr;

        // Attempt to deserialize the struct, if it deserializes process.
        if ( csm_deserialize_struct( csmi_jsrun_cmd_payload_t, &jsrunPayload,
                content._Msg.GetData().c_str(), content._Msg.GetDataLen()) == 0 )
        {
            STRUCT_TYPE* jsrunCMD = mcastProps->GetData();

            if(jsrunCMD && jsrunPayload->hostname)
            {
                success = true;
                std::string hostname(jsrunPayload->hostname);
                mcastProps->SetHostError(hostname, CSMI_SUCCESS);
            }
            
            csm_free_struct_ptr(csmi_jsrun_cmd_payload_t, jsrunPayload);
        }
    }

    return success;
}

}
}
}
