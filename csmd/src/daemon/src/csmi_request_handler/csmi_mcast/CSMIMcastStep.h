/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_mcast/CSMIMcastStep.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
/**
 * @file CSMIMcastStep.h
 * @author John Dunham (jdunham@us.ibm)
 */

#ifndef CSMI_MCAST_STEP 
#define CSMI_MCAST_STEP

#include "CSMIMcast.h"
#include "csmi/include/csmi_type_wm_funct.h"
#include "csmi/src/wm/include/csmi_wm_type_internal.h" 

#define STRUCT_TYPE csmi_allocation_step_mcast_context_t

// Build the error map table.
struct CSMIAllocStepComparator
{
    int map(int val) const
    {
        int returnVal=0;
        switch (val)
        {
            case CSMERR_MULTI_GEN_ERROR:
                returnVal=-1;
                break;
            default:
                break;
        }
        return returnVal;
    }

    bool operator() (const int& a, const int& b) const
    {
        return map(a) < map(b);
    }
};

/** @brief Frees the @ref _Data structure if not null.
 * @tparam DataStruct The step begin/end  multicast context.
 */
template<>
CSMIMcast<STRUCT_TYPE,CSMIAllocStepComparator>::~CSMIMcast();

/** @brief Builds a specialized payload to handle step begin/end multicast payloads.
 * @todo finish this document.
 */
/** @brief Builds a specialized payload to handle step begin/end multicast payloads.
 *
 * @tparam DataStruct The step begin/end multicast context.
 *
 * @param[out] buffer The payload of the message to send to the compute agents.
 * @param[out] bufferLength The length of the @p bufferLength payload.
 */
template<>
void CSMIMcast<STRUCT_TYPE,CSMIAllocStepComparator>::BuildMcastPayload(char** buffer, uint32_t* bufferLength);

/**
 * @brief Generates a unique identifier for the multicast.
 * Reports Allocation Id and Step Id.
 *
 * @return A string containing the unique identifier for the multicast.
 */
template<>
std::string CSMIMcast<STRUCT_TYPE,CSMIAllocStepComparator>::GenerateIdentifierString();

/** @brief A typedef for @ref csmi_allocation_step_mcast_context_t specializations of @ref CSMIMcast.
 */
typedef CSMIMcast<STRUCT_TYPE,CSMIAllocStepComparator> CSMIStepMcast;

namespace csm{
namespace mcast{
namespace step{
// Static helper functions, for template level bindings for a state
/** @brief Parses a message payload for step
 *
 * The Parse operation saves values to @ref CSMIAllocationMcast::_Data.
 *
 * @param[in,out] mcastProps The properties of the multicast.
 * @param[in] content The message Payload being parsed.
 *  
 * @return True if the response is valid.
 */
bool ParseResponse(CSMIStepMcast* mcastProps,
    const csm::network::MessageAndAddress content);
}
}
}
#undef STRUCT_TYPE
#endif

