/*================================================================================

    csmi/src/wm/src/csmi_cgroup_login.c

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/*
 * Author: John Dunham
 * Email:  jdunham@us.ibm.com
 */

/*csm includes*/
#include "csmutil/include/csmutil_logging.h"
#include "csmutil/include/timing.h"
 
#include "csmi/src/common/include/csmi_api_internal.h"
#include "csmi/src/common/include/csmi_common_utils.h"
#include "csmi/include/csm_api_workload_manager.h" 
// The structs used in sending and recieving queries from the infrastructure.
#define API_PARAMETER_INPUT_TYPE    csm_cgroup_login_input_t
#define API_PARAMETER_OUTPUT_TYPE  

static const csmi_cmd_t expected_cmd = CSM_CMD_cgroup_login;

int csm_cgroup_login(
    csm_api_object **handle, 
    API_PARAMETER_INPUT_TYPE* input)
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

    // EARLY RETURN
    // Build the args struct then serialize it. 
    csm_serialize_struct( API_PARAMETER_INPUT_TYPE, input, &buffer, &buffer_length);
    test_serialization( handle, buffer );

    // Send a Message to the Backend.
    error_code = csmi_sendrecv_cmd( *handle, expected_cmd,
        buffer, buffer_length, &return_buffer, &return_buffer_len );

    // If a success was detected process it, otherwise log the failure.
    if ( error_code != CSMI_SUCCESS )
    {
        csmutil_logging(error, "csmi_sendrecv_cmd failed: %d - %s",
            error_code, csm_api_object_errmsg_get(*handle));
    }

    // Free the buffers.
    if( return_buffer ) free(return_buffer);
    free(buffer);

    END_TIMING( csmapi, trace, csm_api_object_traceid_get(*handle), expected_cmd, api )

    return error_code;
}

