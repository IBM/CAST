/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_mcast/CSMIJSRUNCMD.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

/**
 * @file CSMIJSRUNCMD.h
 * @author John Dunham (jdunham@us.ibm)
 */

#ifndef CSMI_JSRUN_CMD 
#define CSMI_JSRUN_CMD

#include "CSMIMcast.h"
#include "csmi/include/csmi_type_wm_funct.h"
#define STRUCT_TYPE csmi_jsrun_cmd_context_t

// Build the error map table.
struct CSMIJSRUNCMDComparator
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
 * @brief Defines a context object for a JSRUN command multicast context.
 */
struct csmi_jsrun_cmd_context_t {
    uint32_t user_id;
    uint32_t num_nodes;
    int64_t  allocation_id;
    char*    kv_pairs;
    char*    jsm_path;
    char*    launch_node;
    char**   compute_nodes;
    csmi_allocation_type_t  type;
    
    csmi_jsrun_cmd_context_t() :
        user_id(0), num_nodes(0), allocation_id(0), kv_pairs(nullptr), jsm_path(nullptr),
        launch_node(nullptr),compute_nodes(nullptr), type(CSM_USER_MANAGED) {}

    ~csmi_jsrun_cmd_context_t()
    {
        if ( kv_pairs )    { free(kv_pairs);    kv_pairs    = nullptr; }
        if ( jsm_path )    { free(jsm_path);    jsm_path    = nullptr; }
        if ( launch_node ) { free(launch_node); launch_node = nullptr; }

        if ( compute_nodes != nullptr )
        {
            if ( num_nodes > 0 ) 
            {
                for (uint32_t i = 0; i < num_nodes; ++i)
                {
                    if (compute_nodes[i])
                    {
                        free(compute_nodes[i]);
                        compute_nodes[i] = nullptr;
                    }
                }
            }
            free(compute_nodes);
        }
        compute_nodes = nullptr;
    }
};

/** @brief Frees the @ref _Data structure if not null.
 * @tparam DataStruct The allocation create/delete/update multicast context.
 */
template<>
CSMIMcast<STRUCT_TYPE,CSMIJSRUNCMDComparator>::~CSMIMcast();

/** @brief Builds a specialized payload to handle burst buffer command multicast payloads.
 *
 * @tparam DataStruct The burst buffer command multicast context.
 *
 * @param[out] buffer The payload of the message to send to the compute agents.
 * @param[out] bufferLength The length of the @p bufferLength payload.
 */
template<>
void CSMIMcast<STRUCT_TYPE,CSMIJSRUNCMDComparator>::BuildMcastPayload(char** buffer, uint32_t* bufferLength);

/**
 * @brief Generates a unique identifier for the multicast.
 * Reports user id.
 *
 * @return A string containing the unique identifier for the multicast.
 */
template<>
std::string CSMIMcast<STRUCT_TYPE,CSMIJSRUNCMDComparator>::GenerateIdentifierString();

/** @brief A typedef for @ref csmi_bb_context_t specializations of @ref CSMIMcast.
 */
typedef CSMIMcast<STRUCT_TYPE,CSMIJSRUNCMDComparator> CSMIJSRUNCMD;

namespace csm{
namespace mcast{
namespace wm{

/** @brief Parses a message payload for a Create Operation.
 *
 * The Parse operation saves values to @ref CSMIJSRUNCMD::_Data.
 *
 * @param[in,out] mcastProps The properties of the multicast.
 * @param[in] content The message payload being parsed.
 *  
 * @return True if the response is valid.
 */
bool ParseCMDResponse(CSMIJSRUNCMD* mcastProps,
    const csm::network::MessageAndAddress content);
}
}
}

#undef STRUCT_TYPE
#endif
