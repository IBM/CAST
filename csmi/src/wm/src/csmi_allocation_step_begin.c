/*================================================================================

    csmi/src/wm/src/csmi_allocation_step_begin.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/*
* Author: Nick Buonarota
* Email: nbuonar@us.ibm.com
*/

#include <string.h>

/*Needed for infrastructure logging*/
#include "csmutil/include/csmutil_logging.h"
#include "csmutil/include/timing.h"

/*Needed for structs and functions*/
#include "csmi/include/csm_api_workload_manager.h"

/*Needed for infrastructure*/
#include "csmi/src/common/include/csmi_api_internal.h"
#include "csmi/src/common/include/csmi_common_utils.h"

#define API_PARAMETER_INPUT_TYPE csm_allocation_step_begin_input_t
#define API_PARAMETER_OUTPUT_TYPE 

const static csmi_cmd_t expected_cmd = CSM_CMD_allocation_step_begin;

int csm_allocation_step_begin(
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

    // If the step was not supplied or the number of nodes was zero, return an error.	
    if ( !input || input->num_nodes == 0 )
    {
        csmutil_logging(error, "An invalid step was provided (steps need nodes)");

        csm_api_object_errcode_set(*handle, CSMERR_INVALID_PARAM);
        csm_api_object_errmsg_set(*handle, 
            strdup(csm_get_string_from_enum(csmi_cmd_err_t,CSMERR_INVALID_PARAM)));
        return CSMERR_INVALID_PARAM;
    }

    // Set the status of the step to running.
    input->status=CSM_STEP_RUNNING;

    // EARLY RETURN
	// Pack up the data to be sent to the back end
    csm_serialize_struct( API_PARAMETER_INPUT_TYPE, input, &buffer, &buffer_length );
    test_serialization( handle, buffer );
	
    // Send a Message to the Backend.
    error_code = csmi_sendrecv_cmd( *handle, expected_cmd,
        buffer, buffer_length, &return_buffer, &return_buffer_len );
    
    if ( error_code != CSMI_SUCCESS )
    {
        csmutil_logging(error, "csmi_sendrecv_cmd failed: %d - %s",
            error_code, csm_api_object_errmsg_get(*handle));
    }

	// Free memory
    if( return_buffer ) free( return_buffer );
    free(buffer);
	
    END_TIMING( csmapi, trace, csm_api_object_traceid_get(*handle), expected_cmd, api )

	return error_code;
}
