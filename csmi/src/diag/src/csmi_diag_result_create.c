/*================================================================================

    csmi/src/diag/src/csmi_diag_result_create.c

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
#include "csmi/include/csm_api_diagnostics.h"

/*Needed for infrastructure*/
#include "csmi/src/common/include/csmi_api_internal.h"

#include "csmi/src/common/include/csmi_common_utils.h"

#define API_PARAMETER_INPUT_TYPE csm_diag_result_create_input_t

const static csmi_cmd_t expected_cmd = CSM_CMD_diag_result_create;

int csm_diag_result_create(csm_api_object **csm_obj, API_PARAMETER_INPUT_TYPE *input)
{
    START_TIMING()

    // Declare variables that we will use below.
    char     *buffer               = NULL;
    uint32_t  buffer_length        = 0;
    char     *return_buffer        = NULL;
    uint32_t  return_buffer_length = 0;
    int       error_code           = CSMI_SUCCESS;
    
    // EARLY RETURN
    // Create a csm_api_object and sets its csmi cmd and the destroy function
    create_csm_api_object(csm_obj, expected_cmd, NULL);

    if( !input ||
        input->run_id <= 0 ||
        !(strncmp(input->status,"PASS", 4) == 0 
            || strncmp(input->status,"FAIL", 4) == 0))
    {
        if( !input )
        {
            csmutil_logging(error, "Invalid parameter: input parameter was null." );
        }
        else if ( input->run_id <= 0 )
        {
            csmutil_logging(error, "Invalid parameter: 'run_id' is less than or equal to zero");
        }
		else if ( !(strncmp(input->status,"PASS", 4) == 0 || strncmp(input->status,"FAIL", 4) == 0))
		{
			csmutil_logging(error, "Invalid parameter: 'status' must be 'PASS' or 'FAIL' ");
		}
        else
        {
            csmutil_logging(error,  "Invalid parameter: default");
        }
    
        csm_api_object_errcode_set(*csm_obj, CSMERR_INVALID_PARAM);
        csm_api_object_errmsg_set(*csm_obj,
            strdup(csm_get_string_from_enum(csmi_cmd_err_t,CSMERR_INVALID_PARAM)));
        return CSMERR_INVALID_PARAM;
    }
    
    // EARLY RETURN
    // Construct the buffer.
    csm_serialize_struct(API_PARAMETER_INPUT_TYPE, input, &buffer, &buffer_length);
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

