/*================================================================================

    csmi/src/common/include/csmi_internal_macros.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSMI_INTERNAL_MACROS_H__
#define __CSMI_INTERNAL_MACROS_H__

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Replaces null characters with a ' '.
 *
 * @param[in] character The character to replace if null.
 */
#define clean_char( character ) character ? character : 32


/**
 * @brief Formats a boolean value into one of the two supplied parameters.
 *
 * @param[in] val The boolean value to format.
 * @param[in] t The formatted value of a true field.
 * @param[in] f The formatted value of a false field.
 */
#define bool_format( val, t, f ) val == CSM_TRUE ? t : f

#define csm_parameter_struct_test(struct_var) !struct_var
#define csm_parameter_start_test(struct_var )                                           \
    char* param_error = NULL;                                                           \
    if( csm_parameter_struct_test(struct_var) )                                         \
        param_error = strdup("Invalid parameter: containing parameter was null.");      \

#define csm_parameter_test( condition, error_message  )                 \
    else if ( condition )                                              \
        param_error = strdup("Invalid parameter: " error_message);\
    

#define csm_parameter_end() \
    else param_error = strdup("Invalid parameter: critical error, detection failed!");\
    csmutil_logging(error, "%s", param_error);                                        \
    csm_api_object_errcode_set(*handle, CSMERR_INVALID_PARAM);                       \
    csm_api_object_errmsg_set(*handle, param_error );                                \
    return CSMERR_INVALID_PARAM;                                                      \

#define csm_parameter_end_test()\
    if ( param_error )                                             \
    {                                                              \
        csmutil_logging(error, "%s", param_error);                 \
        csm_api_object_errcode_set(*handle, CSMERR_INVALID_PARAM);\
        csm_api_object_errmsg_set(*handle, param_error );         \
        return CSMERR_INVALID_PARAM;                               \
    }

/** @brief Prints a list of node errors in @p csmobj.
 *
 * @param[in] csmobj The csm api object.
 */
#define csm_print_node_errors(csmobj)                                   \
    uint32_t node_error_count = csm_api_object_node_error_count_get(csmobj); \
    if ( node_error_count ) {                                           \
        printf("bad_node_count: %d\nbad_nodes:\n", node_error_count);   \
        int i = 0;                                                      \
        for(;i< node_error_count; i++){                                 \
            printf(" - %s[%d]: %s\n",                                   \
                csm_api_object_node_error_source_get(csmobj, i),        \
                csm_api_object_node_error_code_get(csmobj, i),          \
                csm_api_object_node_error_msg_get(csmobj, i));          \
        }                                                               \
    }

#ifdef __cplusplus
}
#endif

#endif
