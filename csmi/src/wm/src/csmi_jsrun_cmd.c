/*================================================================================

    csmi/src/wm/src/csmi_jsrun_cmd.c

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include "csmutil/include/csmutil_logging.h"
#include "csmutil/include/timing.h"

#include "csmi/src/common/include/csmi_api_internal.h"
#include "csmi/src/common/include/csmi_common_utils.h"
#include "csmi/include/csm_api_workload_manager.h"
#include <string.h>
#include <ctype.h>

// The structs used in sending and recieving queries from the infrastructure.
#define API_PARAMETER_INPUT_TYPE  csm_jsrun_cmd_input_t
#define API_PARAMETER_OUTPUT_TYPE 

// Performs a test to see if a string is alpha_numeric.
#define flag_test( str, debug_str ) \
    if ( str ) {                    \
        char* current = str;        \
        char  illegal = 0;          \
                                    \
        for ( ; !illegal && *current; current++ )  \
            illegal |= !( isalpha(*current) ||     \
            isdigit(*current) || *current == ' ' ||\
            *current == '=' || *current == ',' );  \
                                                   \
        if ( illegal ) {                                                \
            csmutil_logging(error,                                      \
            "%s-%d: Invalid "                                           \
            debug_str                                                   \
            " parameter: Character '%c' is not a legal character",      \
                __FILE__, __LINE__, *(current-1));                      \
            csm_api_object_errcode_set(*handle, CSMERR_INVALID_PARAM); \
            csm_api_object_errmsg_set(*handle,                         \
                strdup( csm_get_string_from_enum(csmi_cmd_err_t,CSMERR_INVALID_PARAM)));        \
            return CSMERR_INVALID_PARAM;                                \
        }                                                               \
    }           

static const csmi_cmd_t expected_cmd = CSM_CMD_jsrun_cmd;

int csm_jsrun_cmd(
    csm_api_object **handle, 
    API_PARAMETER_INPUT_TYPE *input)
{
    START_TIMING()

    // Declare variables that we will use below.
    char    *buffer             = NULL;
    uint32_t buffer_length      = 0;   
    char     *return_buffer     = NULL; 
    uint32_t  return_buffer_len = 0;   
    int      error_code         = CSMI_SUCCESS;

    // EARLY RETURN
    // Create a csm_api_object and sets its csmi cmd and the destroy function
    create_csm_api_object(handle, expected_cmd, NULL);
    
    // Verify that the allocation has nodes, the state is valid and the number of shared nodes is valid.
    if(  !input                                                        || 
         input->allocation_id <= 0)
    {
        if ( !input )
        {
            csmutil_logging(error, "Can't run a jsrun command without an input struct.");
        }
        else
        {
            csmutil_logging(error,"Invalid parameter: Allocation id must be greater than zero.");
        }
        csm_api_object_errcode_set(*handle, CSMERR_INVALID_PARAM);
        csm_api_object_errmsg_set(*handle, 
            strdup(csm_get_string_from_enum(csmi_cmd_err_t,CSMERR_INVALID_PARAM)));
        return CSMERR_INVALID_PARAM;
    }
    
    // Verify that both the user and system flags have only alpha-numeric characters.
    flag_test(input->kv_pairs, "kv_flags")
    
    // EARLY RETURN
    // Serialize the structure.
    csm_serialize_struct(API_PARAMETER_INPUT_TYPE, input, &buffer, &buffer_length );
    test_serialization( handle, buffer );

    // Send a Message to the Backend.
    error_code = csmi_sendrecv_cmd(*handle, expected_cmd, 
        buffer, buffer_length, &return_buffer, &return_buffer_len);

    // If a success was detected process it, otherwise log the failure.
    if ( error_code != CSMI_SUCCESS )
    {
        csmutil_logging(error, "csmi_sendrecv_cmd failed: %d - %s",
            error_code, csm_api_object_errmsg_get(*handle));
    }

    free(buffer);
 
    END_TIMING( csmapi, trace, csm_api_object_traceid_get(*handle), expected_cmd, api )

    return error_code;
}


