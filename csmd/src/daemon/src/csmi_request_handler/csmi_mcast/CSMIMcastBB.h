/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_mcast/CSMIBBCMD.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

/**
 * @file CSMIBBCMD.h
 * @author John Dunham (jdunham@us.ibm)
 */

#ifndef CSMI_BB_CMD 
#define CSMI_BB_CMD

#include "CSMIMcast.h"
#include "csmi/include/csmi_type_bb_funct.h"
#define STRUCT_TYPE csmi_bb_cmd_context_t

// Build the error map table.
struct CSMIBBCMDComparator
{
    int map(int val) const
    {
        int returnVal=0;
        switch (val)
        {
            case CSMERR_GENERIC:
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

/**
 * @brief Defines a context object for a Burst Buffer command multicast context.
 */
struct csmi_bb_cmd_context_t {
    uint32_t user_id;
    uint32_t num_nodes;
    char*    command_arguments;
    char**   compute_nodes;
    std::string cmd_output;

    csmi_bb_cmd_context_t() :
        user_id(0), num_nodes(0), command_arguments(nullptr), 
        compute_nodes(nullptr), cmd_output("") { }

    ~csmi_bb_cmd_context_t()
    {
        if (command_arguments) free(command_arguments);

        if (compute_nodes != nullptr &&  num_nodes > 0 )
        {
            for (uint32_t i = 0; i < num_nodes; ++i) free(compute_nodes[i]);

            free(compute_nodes);
        }

        compute_nodes = nullptr;
        command_arguments = nullptr;
    }
};

/** @brief Frees the @ref _Data structure if not null.
 * @tparam DataStruct The allocation create/delete/update multicast context.
 */
template<>
CSMIMcast<STRUCT_TYPE,CSMIBBCMDComparator>::~CSMIMcast();

/** @brief Builds a specialized payload to handle burst buffer command multicast payloads.
 *
 * @tparam DataStruct The burst buffer command multicast context.
 *
 * @param[out] buffer The payload of the message to send to the compute agents.
 * @param[out] bufferLength The length of the @p bufferLength payload.
 */
template<>
void CSMIMcast<STRUCT_TYPE,CSMIBBCMDComparator>::BuildMcastPayload(char** buffer, uint32_t* bufferLength);

/**
 * @brief Generates a unique identifier for the multicast.
 * Reports user id.
 *
 * @return A string containing the unique identifier for the multicast.
 */
template<>
std::string CSMIMcast<STRUCT_TYPE,CSMIBBCMDComparator>::GenerateIdentifierString();

/** @brief A typedef for @ref csmi_bb_context_t specializations of @ref CSMIMcast.
 */
typedef CSMIMcast<STRUCT_TYPE,CSMIBBCMDComparator> CSMIBBCMD;

namespace csm{
namespace mcast{
namespace bb{

/** @brief Parses a message payload for a Create Operation.
 *
 * The Parse operation saves values to @ref CSMIBBCMD::_Data.
 *
 * @param[in,out] mcastProps The properties of the multicast.
 * @param[in] content The message payload being parsed.
 *  
 * @return True if the response is valid.
 */
bool ParseCMDResponse(CSMIBBCMD* mcastProps,
    const csm::network::MessageAndAddress content);
}
}
}

#undef STRUCT_TYPE
#endif
