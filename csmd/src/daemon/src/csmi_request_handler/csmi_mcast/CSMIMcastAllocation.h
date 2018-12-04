/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_mcast/CSMIMcastAllocation.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

/**
 * @file CSMIMcastAllocation.h
 * @author John Dunham (jdunham@us.ibm)
 */

#ifndef CSMI_MCAST_ALLOCATION 
#define CSMI_MCAST_ALLOCATION

#include "CSMIMcast.h"
#include "csmi/include/csmi_type_wm_funct.h"
#include "csmi/include/csm_api_macros.h"
#include "csmi/src/wm/include/csmi_wm_type_internal.h" 
#define STRUCT_TYPE csmi_allocation_mcast_context_t
//#include <map>

// Build the error map table.
struct CSMIAllocErrorComparator
{
    int map(int val) const
    {
        int returnVal=0;
        switch (val)
        {
            case CSMERR_ALLOC_BAD_FLAGS :
                returnVal=5;
                break;
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
 * @tparam DataStruct The allocation create/delete/update multicast context.
 */
template<>
CSMIMcast<STRUCT_TYPE, CSMIAllocErrorComparator>::~CSMIMcast();

/** @brief Builds a specialized payload to handle allocation create/delete multicast payloads.
 *
 * @tparam DataStruct The allocation create/delete/update multicast context.
 *
 * @param[out] buffer The payload of the message to send to the compute agents.
 * @param[out] bufferLength The length of the @p bufferLength payload.
 */
template<>
void CSMIMcast<STRUCT_TYPE, CSMIAllocErrorComparator>::BuildMcastPayload(char** buffer, uint32_t* bufferLength);

/**
 * @brief Generates a unique iSMIAllocErrorComparator
 * Reports Allocation Id, Primary Job Id and Secondary Job Id.
 *
 * @return A string containing the unique identifier for the multicast.
 */
template<>
std::string CSMIMcast<STRUCT_TYPE, CSMIAllocErrorComparator>::GenerateIdentifierString();

/** @brief A typedef for @ref csmi_allocation_mcast_context_t specializations of @ref CSMIMcast.
 */
typedef CSMIMcast<STRUCT_TYPE, CSMIAllocErrorComparator> CSMIMcastAllocation;

namespace csm{
namespace mcast{
namespace allocation{

/** @brief Parses a message payload for a Create Operation.
 *
 * The Parse operation saves values to @ref CSMIMcastAllocation::_Data.
 *
 * @param[in,out] mcastProps The properties of the multicast.
 * @param[in] content The message payload being parsed.
 *  
 * @return True if the response is valid.
 */
bool ParseResponseCreate(CSMIMcastAllocation* mcastProps,
    const csm::network::MessageAndAddress content);

/** @brief Parses a message payload for a Delete Operation.
 *
 * The Parse operation saves values to @ref CSMIMcastAllocation::_Data.
 *
 * @param[in,out] mcastProps The properties of the multicast.
 * @param[in] content The message payload being parsed.
 *  
 * @return True if the response is valid.
 */
bool ParseResponseDelete(CSMIMcastAllocation* mcastProps,
    const csm::network::MessageAndAddress content);

/** @brief Parses a message payload for a Recovery Operation.
  *
  * @param[in,out] mcastProps The properties of the multicast.
  * @param[in] content The message payload being parsed.
  *
  * @return True if the response is valid.
  */
bool ParseResponseRecover(CSMIMcastAllocation* mcastProps,
    const csm::network::MessageAndAddress content);
}
}
}

#undef STRUCT_TYPE
#endif
