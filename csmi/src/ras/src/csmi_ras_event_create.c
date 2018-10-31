/*================================================================================

    csmi/src/ras/src/csmi_ras_event_create.c

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/*Needed for CSM*/
#include "csm_api_ras.h"

#include "csmutil/include/csmutil_logging.h"
#include "csmutil/include/timing.h"

#include "csmi/src/common/include/csmi_api_internal.h"
#include "csmi/src/common/include/csmi_common_utils.h"

#define API_PARAMETER_INPUT_TYPE csm_ras_event_create_input_t
#define API_PARAMETER_OUTPUT_TYPE

static const csmi_cmd_t expected_cmd = CSM_CMD_ras_event_create;

int csm_ras_event_create(
    csm_api_object **csm_obj, 
    const char *msg_id, 
    const char *time_stamp,
    const char *location_name,
    const char *raw_data,
    const char *kvcsv)
{
    START_TIMING()
    // Declare variables that we will use below.
    char     *buffer               = NULL;
    uint32_t  buffer_length        = 0;
    char     *return_buffer        = NULL;
    uint32_t  return_buffer_length = 0;
    int       error_code           = CSMI_SUCCESS;
    API_PARAMETER_INPUT_TYPE input;
    
    // EARLY RETURN
    // Create a csm_api_object and sets its csmi cmd and the destroy function
    create_csm_api_object(csm_obj, expected_cmd, NULL);
   
    // CSM API initialize struct 
    csm_init_struct(API_PARAMETER_INPUT_TYPE, input);
 
    // Populate the struct.
    input.msg_id        = msg_id        ? strdup(msg_id)        : NULL;
    input.time_stamp    = time_stamp    ? strdup(time_stamp)    : NULL;
    input.location_name = location_name ? strdup(location_name) : NULL;
    input.raw_data      = raw_data      ? strdup(raw_data)      : NULL;
    input.kvcsv         = kvcsv         ? strdup(kvcsv)         : NULL;
    
    // EARLY RETURN
    // Construct the buffer.
    csm_serialize_struct(API_PARAMETER_INPUT_TYPE, &input, &buffer, &buffer_length);
    csm_free_struct(API_PARAMETER_INPUT_TYPE, input);
    test_serialization( csm_obj, buffer );
    
    // Send a Message to the Backend.
    error_code = csmi_sendrecv_cmd(*csm_obj, expected_cmd,
        buffer, buffer_length, &return_buffer, &return_buffer_length);
    
    if ( error_code != CSMI_SUCCESS )
    {
        csmutil_logging(error, "csmi_sendrecv_cmd failed: %d - %s",
            error_code, csm_api_object_errmsg_get(*csm_obj));
    }
    
    // Free the buffers.
    if ( return_buffer ) free(return_buffer);
    free(buffer);

    END_TIMING( csmapi, trace, csm_api_object_traceid_get(*csm_obj), expected_cmd, api )

    return error_code;
}

