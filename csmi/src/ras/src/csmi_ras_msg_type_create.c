/*================================================================================

    csmi/src/ras/src/csmi_ras_msg_type_create.c

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

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

/* ## INCLUDES ## */
/*Needed for CSM*/
#include "csm_api_ras.h"
/*Needed for infrastructure*/
#include "csmi/src/common/include/csmi_api_internal.h"
#include "csmi/src/common/include/csmi_common_utils.h"
/*Needed for CSM logging*/
#include "csmutil/include/csmutil_logging.h"
#include "csmutil/include/timing.h"
/* ## DEFINES ## */
/* Defines to make API easier */
#define API_PARAMETER_INPUT_TYPE csm_ras_msg_type_create_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_ras_msg_type_create_output_t

static const csmi_cmd_t expected_cmd = CSM_CMD_ras_msg_type_create;

void csm_ras_msg_type_create_destroy(csm_api_object *csm_obj);

int csm_ras_msg_type_create(csm_api_object **csm_obj, API_PARAMETER_INPUT_TYPE* input, API_PARAMETER_OUTPUT_TYPE** output)
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
    create_csm_api_object(csm_obj, expected_cmd, csm_ras_msg_type_create_destroy);
    
    // TODO we need to metaprogram this somehow. -John
    if ( !input || 
            (input->threshold_count < 0)             || 
            (input->threshold_period < 0)            ||
            (input->control_action == NULL 
                || input->control_action[0] == '\0') ||

            (input->description == NULL 
                || input->description[0] == '\0')    ||

            (input->msg_id == NULL 
                || input->msg_id[0] == '\0')         || 

            (input->message == NULL 
                || input->message[0] == '\0')        ||
            (input->set_state < CSM_NODE_NO_DEF
				|| (input->set_state != CSM_NODE_NO_DEF && input->set_state != CSM_NODE_SOFT_FAILURE)
				|| input->set_state >=csm_enum_max(csmi_node_state_t) )
        )
            
    {
        if( !input )
        {
            csmutil_logging(error, "Invalid parameter: input parameter was null." );
        }
        else if (input->threshold_count < 0)
        {
            csmutil_logging(error, "Invalid parameter: 'threshold_count' less than zero." );
        }
        else if (input->threshold_period < 0)
        {
            csmutil_logging(error, "Invalid parameter: 'threshold_period' less than zero." );
        }
        else if (input->control_action == NULL || input->control_action[0] == '\0')
        {
            csmutil_logging(error,  "Invalid parameter: 'control_action' may not be empty.");
        }
        else if (input->description == NULL || input->description[0] == '\0')
        {
            csmutil_logging(error,  "Invalid parameter: 'description' may not be empty.");
        }
        else if ( input->msg_id == NULL || input->msg_id[0] == '\0')                  
        {
            csmutil_logging(error,  "Invalid parameter: 'msg_id' may not be empty.");
        }
        else if ( input->message == NULL || input->message[0] == '\0')               
        {
            csmutil_logging(error,  "Invalid parameter: 'message' may not be empty.");
        }
        else if(input->severity < CSM_RAS_NO_SEV
				|| input->severity >=csm_enum_max(csmi_ras_severity_t)
		)
        {
            csmutil_logging(error,  "Invalid parameter: 'severity' must be 'INFO', 'WARNING' or 'FATAL', API received: %s", csm_get_string_from_enum(csmi_ras_severity_t, input->severity));
        } 
		else if(input->set_state < CSM_NODE_NO_DEF
				|| (input->set_state != CSM_NODE_NO_DEF && input->set_state != CSM_NODE_SOFT_FAILURE)
				|| input->set_state >=csm_enum_max(csmi_node_state_t)
		)
		{
			csmutil_logging(error,  "Invalid parameter: 'set_state' must be 'undefined', or 'SOFT_FAILURE', API received: %s", csm_get_string_from_enum(csmi_node_state_t, input->set_state));
		}
		else{
			csmutil_logging(error,  "Invalid parameter: default.");
		}
    
        csm_api_object_errcode_set(*csm_obj, CSMERR_INVALID_PARAM);
        csm_api_object_errmsg_set(*csm_obj,
            strdup(csm_get_string_from_enum(csmi_cmd_err_t,CSMERR_INVALID_PARAM)));
        return CSMERR_INVALID_PARAM;
    }
    // Clamp the boolean fields.
    input->enabled          = input->enabled          > CSM_FALSE;
    input->visible_to_users = input->visible_to_users > CSM_FALSE;
    
    // EARLY RETURN
    // Construct the buffer.
    csm_serialize_struct(API_PARAMETER_INPUT_TYPE, input, &buffer, &buffer_length);
    test_serialization( csm_obj, buffer );
    
    // Send a Message to the Backend.
    error_code = csmi_sendrecv_cmd(*csm_obj, expected_cmd,
    buffer, buffer_length, &return_buffer, &return_buffer_length);
    
    // If a success was detected process it, otherwise log the failure.
    if ( error_code == CSMI_SUCCESS )
    {
        if ( csm_deserialize_struct(API_PARAMETER_OUTPUT_TYPE, output,
                return_buffer, return_buffer_length ) == 0 )
        {
            csm_api_object_set_retdata(*csm_obj, 1, *output);
        }
        else
        {
            csmutil_logging(error, "Deserialization failed");
            csm_api_object_errcode_set(*csm_obj, CSMERR_MSG_UNPACK_ERROR);
            csm_api_object_errmsg_set(*csm_obj,
                strdup(csm_get_string_from_enum(csmi_cmd_err_t, CSMERR_MSG_UNPACK_ERROR)));
            error_code = CSMERR_MSG_UNPACK_ERROR;
        }
    }
    else
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

void csm_ras_msg_type_create_destroy(csm_api_object *csm_obj)
{
	/* Function variables. */
	csmi_api_internal *csmi_hdl;
    API_PARAMETER_OUTPUT_TYPE *output = NULL;
	/* Helper variables. */
    //int i = 0;
    
	/* Verify it exists */
    if (csm_obj == NULL || csm_obj->hdl == NULL)
    {
        csmutil_logging(warning, "%s-%d: csm_api_object not valid", __FILE__, __LINE__);
        return;
    }

	/* Verify its the correct obj */
    csmi_hdl = (csmi_api_internal *) csm_obj->hdl;
    if (csmi_hdl->cmd != expected_cmd)
    {
        csmutil_logging(error, "%s-%d: Unmatched CSMI cmd\n", __FILE__, __LINE__);
        return;
    }

	/* Do the free */
	/* Set the data. */
    output = (API_PARAMETER_OUTPUT_TYPE *) csmi_hdl->ret_cdata;
	/* Free the API output struct */
	csm_free_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);
}

