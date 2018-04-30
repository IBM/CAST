/*================================================================================

    csmi/src/ras/src/csmi_ras_subscribe.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

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

#define API_PARAMETER_INPUT_TYPE csm_ras_subscribe_input_t

const static csmi_cmd_t expected_cmd = CSM_CMD_ras_subscribe;

void csm_ras_subscribe_destroy(csm_api_object *csm_obj);

int csm_ras_subscribe(csm_api_object **csm_obj,
                      const char *topic)
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
    create_csm_api_object(csm_obj, expected_cmd, csm_ras_subscribe_destroy);
    
    // FIXME strdup really isn't needed.
    input.topic = topic ? strdup(topic) : NULL;
    
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

void csm_ras_subscribe_destroy(csm_api_object *csm_obj)
{
    csmi_api_internal *csmi_hdl;
    csmi_hdl = (csmi_api_internal *) csm_obj->hdl;
    if (csmi_hdl->ret_cdata)
        free(csmi_hdl->ret_cdata);

}

