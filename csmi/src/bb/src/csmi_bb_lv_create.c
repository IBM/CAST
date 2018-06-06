/*================================================================================

    csmi/src/bb/src/csmi_bb_lv_create.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/*
 * Author: Nick Buonarota
 * Email:  nbuonar@us.ibm.com
 */
 
 /*C includes*/
 #include <string.h>
/*CSM includes*/
/*Needed for structs and functions*/
#include "csmi/include/csm_api_burst_buffer.h" 
/*Needed for infrastructure*/
#include "csmi/src/common/include/csmi_api_internal.h" 
 
#include "csmi/src/common/include/csmi_common_utils.h" 
/*Needed for infrastructure logging*/
#include "csmutil/include/csmutil_logging.h"
#include "csmutil/include/timing.h"

#define API_PARAMETER_INPUT_TYPE csm_bb_lv_create_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_BbLvCreate_output_t

static const csmi_cmd_t expected_cmd = CSM_CMD_bb_lv_create;

int csm_bb_lv_create(csm_api_object **csm_obj, API_PARAMETER_INPUT_TYPE* input)
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
	
    // EARLY RETURN
    if ( !input ||
			input->allocation_id < 0 ||
			input->current_size < 0 ||
			input->file_system_mount == NULL || input->file_system_mount[0] == '\0' ||
			input->file_system_type == NULL || input->file_system_type[0] == '\0' ||
            input->logical_volume_name == NULL || input->logical_volume_name[0] == '\0' ||
			input->node_name == NULL || input->node_name[0] == '\0' ||			
			!(input->state == 'C' 
                || input->state == 'M' ) ||
			input->vg_name == NULL || input->vg_name[0] == '\0' )
    {
		/* Checks to see if we have input. */
		if ( !input )
        {
            csmutil_logging(error, "Invalid parameter: input parameter was null." );
        }
		/* Checks to see if we have a negative allocation_id */
		else if(input->allocation_id < 0)
		{
			csmutil_logging(error, "Invalid parameter: 'allocation_id' can not be less than zero."); 
		}
		/* Checks to see if we have a negative current size */
		else if(input->current_size < 0)
		{
			csmutil_logging(error, "Invalid parameter: 'current_size' can not be less than zero."); 
		}
		/* Checks to see if we have a file_system_mount. */
		else if(input->file_system_mount == NULL || input->file_system_mount[0] == '\0')
		{
			csmutil_logging(error, "Invalid parameter: 'file_system_mount' must be specified and not be null."); 
		}
		/* Checks to see if we have a file_system_type. */
		else if(input->file_system_type == NULL || input->file_system_type[0] == '\0')
		{
			csmutil_logging(error, "Invalid parameter: 'file_system_type' must be specified and not be null."); 
		}
		/* Checks to see if we have a lv name. */
		else if(input->logical_volume_name == NULL || input->logical_volume_name[0] == '\0')
		{
			csmutil_logging(error, "Invalid parameter: 'logical_volume_name' must be specified and not be null."); 
		}
		/* Checks to see if we have a node_name. */
		else if(input->node_name == NULL || input->node_name[0] == '\0')
		{
			csmutil_logging(error, "Invalid parameter: 'node_name' must be specified and not be null."); 
		}
		/* Checks to see if we are creating a valid state. */
		else if (!(input->state == 'C' 
                || input->state == 'M'))
        {
		    csmutil_logging(error, "Invalid parameter: 'state' must be 'C', or 'M'.");
        }
		/* Checks to see if we have a vg_name. */
		else if(input->vg_name == NULL || input->vg_name[0] == '\0')
		{
			csmutil_logging(error, "Invalid parameter: 'vg_name' must be specified and not be null."); 
		}
		else
		{
			csmutil_logging(error, "Invalid parameter: default case");
		}

		/* Set API object error code and message. */
        csm_api_object_errcode_set(*csm_obj, CSMERR_INVALID_PARAM);
        csm_api_object_errmsg_set(*csm_obj,
            strdup(csm_get_string_from_enum(csmi_cmd_err_t,CSMERR_INVALID_PARAM)));
        
        return CSMERR_INVALID_PARAM;
    }
	
	/*Parameters should be good now. So we should be safe to debug print. */
	/* [ToDo: INSERT DEBUG PRINT HERE] */

    // EARLY RETURN
    // Construct the buffer.
    csm_serialize_struct(API_PARAMETER_INPUT_TYPE, input, &buffer, &buffer_length);
    test_serialization( csm_obj, buffer );
    
    // Send a Message to the Backend.
    error_code = csmi_sendrecv_cmd(*csm_obj, expected_cmd,
        buffer, buffer_length, &return_buffer, &return_buffer_length);
    
    // If a success was detected process it, otherwise log the failure.
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
