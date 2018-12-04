/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_mcast/CSMIMcastAllocation.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#include "CSMIMcastSoftFailureRecovery.h"
#define STRUCT_TYPE csmi_soft_failure_recovery_context_t

template<>
CSMIMcast<STRUCT_TYPE,CSMISoftFailureComparator>::~CSMIMcast()
{
    if(_Data) delete _Data;
    _Data = nullptr;
}

template<>
void CSMIMcast<STRUCT_TYPE,CSMISoftFailureComparator>::BuildMcastPayload(char** buffer, uint32_t* bufferLength)
{
    // Generate the payload
    csmi_soft_failure_recovery_payload_t * payload = nullptr;
    csm_init_struct_ptr(csmi_soft_failure_recovery_payload_t, payload);
    
    // TODO Put stuff here.

    csm_serialize_struct( csmi_soft_failure_recovery_payload_t, payload,
                        buffer, bufferLength );

    csm_free_struct_ptr(csmi_soft_failure_recovery_payload_t, payload);
}

template<>
std::string CSMIMcast<STRUCT_TYPE,CSMISoftFailureComparator>::GenerateIdentifierString()
{
    std::string idString = "Soft Failure Recovery";
    //if ( _Data )
    //    idString.append(std::to_string(_Data->allocation_id)).append( "; Primary Job Id: ")
    //        .append(std::to_string(_Data->primary_job_id)).append("; Secondary Job Id: ")
    //        .append(std::to_string(_Data->secondary_job_id));
    //else 
    //    idString.append("NOT FOUND");

    return idString;
}

namespace csm{
namespace mcast{
namespace nodes{

bool ParseResponseSoftFailure( 
    CSMIMcastSoftFailureRecovery* mcastProps,
    const csm::network::MessageAndAddress content )
{
    LOG(csmapi,trace) << "Parsing Mcast Soft Failure Recovery";
    
    // Track whether or not the received payload is valid.
    bool success = false;

    // If this is not in recovery and the message length is greater than zero parse the content.
    if( mcastProps && content._Msg.GetDataLen() > 0 )
    {
        LOG(csmapi,trace) << "Message length is greater than 0";
        csmi_soft_failure_recovery_payload_t *payload = nullptr;

        // Attempt to deserialize the struct, if it deserializes process.
        if ( csm_deserialize_struct( csmi_soft_failure_recovery_payload_t, &payload,
                content._Msg.GetData().c_str(), content._Msg.GetDataLen()) == 0 && payload->hostname )
        {
            success=true;

            std::string hostname(payload->hostname);
            LOG(csmapi,trace) << hostname << " Found " << payload->error_code;
            mcastProps->SetHostError(hostname, 
                payload->error_code, 
                payload->error_message);
            

            csm_free_struct_ptr(csmi_soft_failure_recovery_payload_t, payload);
        }
    }

    return success;
}

}
}
}
