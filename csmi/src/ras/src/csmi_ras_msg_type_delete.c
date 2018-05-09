/*================================================================================

    csmi/src/ras/src/csmi_ras_msg_type_delete.c

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
#define API_PARAMETER_INPUT_TYPE csm_ras_msg_type_delete_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_ras_msg_type_delete_output_t

static const csmi_cmd_t expected_cmd = CSM_CMD_ras_msg_type_delete;

void csm_ras_msg_type_delete_destroy(csm_api_object *csm_obj);

int csm_ras_msg_type_delete(
    csm_api_object **csm_obj, 
    API_PARAMETER_INPUT_TYPE* input, 
    API_PARAMETER_OUTPUT_TYPE** output)
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
    create_csm_api_object(csm_obj, expected_cmd, csm_ras_msg_type_delete_destroy);
    
    if ( !input ||
            input->msg_ids_count <= 0 || input->msg_ids == NULL )
    {
        if( !input )
        {
            csmutil_logging(error, "Invalid parameter: input parameter was null." );
        }
        else
        {
            csmutil_logging(error, "Invalid parameter: 'msg_ids_count' and 'msg_ids' must define"
                " a list of message ids to be removed. " );
        }

    }
    
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
        
            // TODO do we really need this?
            /* Calculate the remaining output fields. */
            (*output)->expected_number_of_deleted_msg_ids = input->msg_ids_count;
            (*output)->not_deleted_msg_ids_count = (*output)->expected_number_of_deleted_msg_ids - (*output)->deleted_msg_ids_count;
            if ( (*output)->not_deleted_msg_ids_count > 0 )
            {
                (*output)->not_deleted_msg_ids = (char**)calloc((*output)->not_deleted_msg_ids_count, sizeof(char*));
                
                // FIXME This is not great can be nearly O(N^2)
                uint32_t i;
                int not_deleted_msg_ids_COUNTER = 0;
                for(i = 0; i < input->msg_ids_count; i ++){
                	uint32_t j = 0;
                	char foundMatch = 0;
                	for(j = 0; j < (*output)->deleted_msg_ids_count; j++){
                		if(strcmp(input->msg_ids[i], (*output)->deleted_msg_ids[j]) == 0){
                			foundMatch = 1;
                		}
                	}
                	if(foundMatch == 0){
                		(*output)->not_deleted_msg_ids[not_deleted_msg_ids_COUNTER] = strdup(input->msg_ids[i]);
                		not_deleted_msg_ids_COUNTER++;
                	}
                }
            }
            else
            {
                (*output)->not_deleted_msg_ids = NULL;
            }
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

void csm_ras_msg_type_delete_destroy(csm_api_object *csm_obj)
{
	/*For debug trace - this will print if the logging level is set to 'trace'.*/
	csmutil_logging(trace, "%s-%d: ", __FILE__, __LINE__);
	csmutil_logging(trace, "  Entered csm_ras_msg_type_delete_destroy.");
	
	/* Function variables. */
	csmi_api_internal *csmi_hdl;
    API_PARAMETER_OUTPUT_TYPE *output = NULL;
	/* Helper variables. */
    //uint32_t i = 0;
    
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
	
	csmutil_logging(trace, "%s-%d: ", __FILE__, __LINE__);
	csmutil_logging(trace, "  Exiting csm_ras_msg_type_delete_destroy.");
}

