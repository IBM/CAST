/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_mcast/CSMIBBCMD.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#include "CSMIMcastBB.h"
#include "csmi/src/bb/include/csmi_bb_type_internal.h"
#define STRUCT_TYPE csmi_bb_cmd_context_t

template<>
CSMIMcast<STRUCT_TYPE,CSMIBBCMDComparator>::~CSMIMcast()
{
    if(_Data) delete _Data;
    _Data = nullptr;
}

template<>
void CSMIMcast<STRUCT_TYPE,CSMIBBCMDComparator>::BuildMcastPayload(char** buffer, uint32_t* bufferLength)
{
    // Generate the leaner Burst Buffer payload.
    csmi_bb_cmd_payload_t *bbPayload = nullptr;
    csm_init_struct_ptr(csmi_bb_cmd_payload_t, bbPayload);

    bbPayload->bb_cmd_str  =_Data->command_arguments;
    bbPayload->bb_cmd_int = _Data->user_id;
    bbPayload->hostname = strdup("");

    csm_serialize_struct( csmi_bb_cmd_payload_t, bbPayload,
                        buffer, bufferLength );

    bbPayload->bb_cmd_str = nullptr;

    csm_free_struct_ptr(csmi_bb_cmd_payload_t, bbPayload);
}

template<>
std::string CSMIMcast<STRUCT_TYPE,CSMIBBCMDComparator>::GenerateIdentifierString()
{
    std::string idString = "User ID: ";
    if ( _Data )
        idString.append(std::to_string(_Data->user_id));
    else 
        idString.append("NOT FOUND");

    return idString;
}

namespace csm{
namespace mcast{
namespace bb{

bool ParseCMDResponse( 
    CSMIBBCMD* mcastProps,
    const csm::network::MessageAndAddress content )
{
    LOG(csmapi,trace) << "Parsing Multicast Command Response";
    
    // Track whether or not the received payload is valid.
    bool success = false;
    
    // If this is not in recovery and the message length is greater than zero parse the content.
    if( mcastProps && content._Msg.GetDataLen() > 0 )
    {
        csmi_bb_cmd_payload_t *bbPayload = nullptr;

        // Attempt to deserialize the struct, if it deserializes process.
        if ( csm_deserialize_struct( csmi_bb_cmd_payload_t, &bbPayload,
                content._Msg.GetData().c_str(), content._Msg.GetDataLen()) == 0 )
        {
            STRUCT_TYPE* bbCMD = mcastProps->GetData();

            if(bbCMD && bbPayload->bb_cmd_str && bbPayload->hostname)
            {
                success = true;
                std::string hostname(bbPayload->hostname);
                mcastProps->SetHostError(hostname, bbPayload->bb_cmd_int);
                bbCMD->cmd_output.append(bbPayload->hostname).append(" : ").
                    append(bbPayload->bb_cmd_str).append(";");
            }
            
            csm_free_struct_ptr(csmi_bb_cmd_payload_t, bbPayload);
        }
    }

    return success;
}

}
}
}
