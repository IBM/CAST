#CSM API Source Directory#



##CSM API Boilerplate##


```c
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
    // Construct the buffer.
    csm_serialize_struct(API_PARAMETER_INPUT_TYPE, input, &buffer, &buffer_length);
    test_serialization( csm_obj, buffer );
    
    // Send a Message to the Backend.
    error_code = csmi_sendrecv_cmd(*csm_obj, expected_cmd,
    buffer, buffer_length, &return_buffer, &return_buffer_length);
    
    // If a success was detected process it, otherwise log the failure.
    if ( error_code == CSMI_SUCCESS )
    {
        if ( return_buffer )
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
            csm_api_object_errcode_set(*csm_obj, CSMI_NO_RESULTS);
            error_code = CSMI_NO_RESULTS;
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
```
