/*================================================================================

    csmi/src/bb/src/csmi_bb_vg_create.c

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

#define API_PARAMETER_INPUT_TYPE csm_bb_vg_create_input_t
#define API_PARAMETER_OUTPUT_TYPE 

const static csmi_cmd_t expected_cmd = CSM_CMD_bb_vg_create;

int csm_bb_vg_create(csm_api_object **csm_obj, API_PARAMETER_INPUT_TYPE *input)
{
    START_TIMING()

    // Declare variables that we will use below.
    char     *buffer               = NULL;
    uint32_t  buffer_length        = 0;
    char     *return_buffer        = NULL;
    uint32_t  return_buffer_length = 0;
    int       error_code           = CSMI_SUCCESS;
    int64_t   ssd_allocation_total = 0;
    int32_t   ssd_idx;
    
    // EARLY RETURN
    // Create a csm_api_object and sets its csmi cmd and the destroy function
    create_csm_api_object(csm_obj, expected_cmd, NULL);

    // ======================================================================
    // Error checking.
    // @TODO verify that all these checks are desired.
    // ----------------------------------------------------------------------
    // EARLY RETURN
    if ( !input ||
            input->total_size < 0   ||
            input->available_size < 0 ||
            input->available_size > input->total_size ||
            input->node_name == NULL ||
            input->vg_name == NULL ||
            ( input->ssd_info_count <= 0 
                || input->ssd_info == NULL ))
    {
        if ( !input )
        {
            csmutil_logging(error, "Invalid parameter: input parameter was null." );
        }
        else if ( input->total_size < 0 )
        {
            csmutil_logging(error, "Invalid parameter: 'total_size' must be positive.");
        }
        else if ( input->available_size < 0 )
        {
            csmutil_logging(error, "Invalid parameter: 'available_size' must be positive.");
        }
        else if ( input->available_size > input->total_size )
        {
            csmutil_logging(error, "Invalid parameter: 'available_size' must be less than or "
                "equal to 'total_size'.");
        }
        else if ( input->node_name == NULL )
        {
            csmutil_logging(error, "Invalid parameter: 'node_name' must not be null.");
        }
        else if ( input->vg_name == NULL )
        {
            csmutil_logging(error, "Invalid parameter: 'vg_name' must not be null.");
        }
        else
        {
            csmutil_logging(error, "Invalid parameter: 'ssd_info_count' and 'ssd_info' must "
                "specify one or more ssds.");
        }

        csm_api_object_errcode_set(*csm_obj, CSMERR_INVALID_PARAM);
        csm_api_object_errmsg_set(*csm_obj,
            strdup(csm_get_string_from_enum(csmi_cmd_err_t,CSMERR_INVALID_PARAM)));

        
        return CSMERR_INVALID_PARAM;
    }

    // Also check ssds.
    for ( ssd_idx = 0; ssd_idx < input->ssd_info_count; ssd_idx++ )
    {
        if ( input->ssd_info[ssd_idx] == NULL ||
                input->ssd_info[ssd_idx]->ssd_serial_number == NULL ||
                input->ssd_info[ssd_idx]->ssd_allocation < 0 )
        {
            if ( !input )
            {
                csmutil_logging(error, "Invalid parameter: 'ssd_info[%i]' was null.",
                    ssd_idx);
            }
            else if ( input->ssd_info[ssd_idx]->ssd_serial_number == NULL )
            {
                csmutil_logging(error, "Invalid parameter: "
                    "'ssd_info[%i]->ssd_serial_number' was null.",
                    ssd_idx);
            }
            else if (input->ssd_info[ssd_idx]->ssd_allocation < 0 )
            {
                csmutil_logging(error, "Invalid parameter: "
                    "'ssd_info[%i]->ssd_allocation' must be positive.",
                    ssd_idx);
            }

            csm_api_object_errcode_set(*csm_obj, CSMERR_INVALID_PARAM);
            csm_api_object_errmsg_set(*csm_obj,
                strdup(csm_get_string_from_enum(csmi_cmd_err_t,CSMERR_INVALID_PARAM)));
            
            return CSMERR_INVALID_PARAM;
        }
        ssd_allocation_total += input->ssd_info[ssd_idx]->ssd_allocation;
    }
	if( ssd_allocation_total != input->total_size )
    {
        csmutil_logging(error, "Invalid parameter: 'total_size' did not match the sum total of "
            "'ssd_allocation' fields in the specified ssds.");

        csm_api_object_errcode_set(*csm_obj, CSMERR_INVALID_PARAM);
        csm_api_object_errmsg_set(*csm_obj,
            strdup(csm_get_string_from_enum(csmi_cmd_err_t,CSMERR_INVALID_PARAM)));
        
        return CSMERR_INVALID_PARAM;
    }
    // ======================================================================

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
