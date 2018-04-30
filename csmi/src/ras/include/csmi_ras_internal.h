/*================================================================================
   
    csmi/src/ras/include/csmi_ras_internal.h

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
#ifndef _CSMI_RAS_INTERNAL_H_
#define _CSMI_RAS_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "csmi/include/csmi_type_ras.h"
#include "csmi/src/common/include/csmi_struct_hash.h"
extern const csmi_struct_mapping_t map_csmi_ras_type_record_t;

extern const csmi_struct_mapping_t map_csm_ras_event_create_input_t;

extern const csmi_struct_mapping_t map_csmi_ras_event_action_record_t;

extern const csmi_struct_mapping_t map_csmi_ras_event_action_t;

extern const csmi_struct_mapping_t map_csmi_ras_event_t;

extern const csmi_struct_mapping_t map_csmi_ras_event_vector_t;

extern const csmi_struct_mapping_t map_csm_ras_event_query_input_t;

extern const csmi_struct_mapping_t map_csm_ras_event_query_output_t;

extern const csmi_struct_mapping_t map_csm_ras_event_query_allocation_input_t;

extern const csmi_struct_mapping_t map_csm_ras_event_query_allocation_output_t;

extern const csmi_struct_mapping_t map_csm_ras_msg_type_create_input_t;

extern const csmi_struct_mapping_t map_csm_ras_msg_type_create_output_t;

extern const csmi_struct_mapping_t map_csm_ras_msg_type_delete_input_t;

extern const csmi_struct_mapping_t map_csm_ras_msg_type_delete_output_t;

extern const csmi_struct_mapping_t map_csm_ras_msg_type_update_input_t;

extern const csmi_struct_mapping_t map_csm_ras_msg_type_update_output_t;

extern const csmi_struct_mapping_t map_csm_ras_msg_type_query_input_t;

extern const csmi_struct_mapping_t map_csm_ras_msg_type_query_output_t;

extern const csmi_struct_mapping_t map_csm_ras_subscribe_input_t;

extern const csmi_struct_mapping_t map_csm_ras_unsubscribe_input_t;


#ifdef __cplusplus
}
#endif
#endif
