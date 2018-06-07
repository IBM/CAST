/*================================================================================

    csmi/src/wm/src/csmi_allocation_delete.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

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

#define API_PARAMETER_INPUT_TYPE  csm_allocation_delete_input_t
#define API_PARAMETER_OUTPUT_TYPE 

static const csmi_cmd_t expected_cmd = CSM_CMD_allocation_delete;

int csm_allocation_delete(
    csm_api_object **handle, 
    API_PARAMETER_INPUT_TYPE *input)
{
    START_TIMING()

    // Declare variables that we will use below.
    char     *buffer            = NULL;
    uint32_t  buffer_length     = 0;
    char     *return_buffer     = NULL; 
    uint32_t  return_buffer_len = 0;   
    int       error_code        = CSMI_SUCCESS;

    // EARLY RETURN
    // Create a csm_api_object and sets its csmi cmd and the destroy function
    create_csm_api_object(handle, expected_cmd, NULL);

    // Verify that the has an allocation id xor primary job id.
    if(  !input                                                          || 
         (input->allocation_id <= 0  && input->primary_job_id <= 0)      ||
         (input->allocation_id > 0 && input->primary_job_id > 0) )
    {
        csmutil_logging(error, "Please supply only an allocation id greater than zero or a primary job id greater than zero.");
        csm_api_object_errcode_set(*handle, CSMERR_INVALID_PARAM);
        csm_api_object_errmsg_set(*handle, 
            strdup(csm_get_string_from_enum(csmi_cmd_err_t,CSMERR_INVALID_PARAM)));
        return CSMERR_INVALID_PARAM;
    }

    // EARLY RETURN
    // Serialize the structure.
    csm_serialize_struct(API_PARAMETER_INPUT_TYPE, input, &buffer, &buffer_length );
    test_serialization( handle, buffer );

    // Send a Message to the Backend.
    error_code = csmi_sendrecv_cmd(*handle, expected_cmd,
        buffer, buffer_length, &return_buffer, &return_buffer_len );

    // If a success was detected process it, otherwise log the failure.
    if ( error_code == CSMI_SUCCESS )
    {
        csm_api_object_set_retdata(*handle, sizeof(input->allocation_id), &input->allocation_id);
    }
    else
    {
        csmutil_logging(error, "csmi_sendrecv_cmd failed: %d - %s",
            error_code, csm_api_object_errmsg_get(*handle));
    }

    if (return_buffer) free(return_buffer);
    free(buffer);
        
    END_TIMING( csmapi, trace, csm_api_object_traceid_get(*handle), expected_cmd, api )

    return error_code;
}

