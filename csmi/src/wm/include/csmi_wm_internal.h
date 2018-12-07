/*================================================================================
   
    csmi/src/wm/include/csmi_wm_internal.h

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
#ifndef _CSMI_WM_INTERNAL_H_
#define _CSMI_WM_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "csmi/include/csmi_type_wm.h"
#include "csmi/src/common/include/csmi_struct_hash.h"
extern const csmi_struct_mapping_t map_csmi_allocation_history_t;

extern const csmi_struct_mapping_t map_csmi_allocation_t;

extern const csmi_struct_mapping_t map_csmi_allocation_accounting_t;

extern const csmi_struct_mapping_t map_csmi_allocation_step_list_t;

extern const csmi_struct_mapping_t map_csmi_allocation_state_history_t;

extern const csmi_struct_mapping_t map_csmi_allocation_details_t;

extern const csmi_struct_mapping_t map_csmi_allocation_step_history_t;

extern const csmi_struct_mapping_t map_csmi_allocation_step_t;

extern const csmi_struct_mapping_t map_csmi_ssd_resources_record_t;

extern const csmi_struct_mapping_t map_csmi_node_resources_record_t;

extern const csmi_struct_mapping_t map_csmi_cgroup_t;

extern const csmi_struct_mapping_t map_csmi_allocation_resources_record_t;

#define map_csm_allocation_create_input_t map_csmi_allocation_t
extern const csmi_struct_mapping_t map_csm_allocation_query_details_input_t;

extern const csmi_struct_mapping_t map_csm_allocation_query_details_output_t;

extern const csmi_struct_mapping_t map_csm_allocation_update_state_input_t;

#define map_csm_allocation_step_begin_input_t map_csmi_allocation_step_t
extern const csmi_struct_mapping_t map_csm_allocation_step_end_input_t;

extern const csmi_struct_mapping_t map_csm_allocation_step_query_input_t;

extern const csmi_struct_mapping_t map_csm_allocation_step_query_output_t;

extern const csmi_struct_mapping_t map_csm_allocation_step_query_details_input_t;

#define map_csm_allocation_step_query_details_output_t map_csm_allocation_step_query_output_t
extern const csmi_struct_mapping_t map_csm_allocation_step_query_active_all_input_t;

#define map_csm_allocation_step_query_active_all_output_t map_csm_allocation_step_query_output_t
extern const csmi_struct_mapping_t map_csm_node_resources_query_input_t;

extern const csmi_struct_mapping_t map_csm_node_resources_query_output_t;

extern const csmi_struct_mapping_t map_csm_node_resources_query_all_input_t;

extern const csmi_struct_mapping_t map_csm_node_resources_query_all_output_t;

extern const csmi_struct_mapping_t map_csm_allocation_step_cgroup_create_input_t;

extern const csmi_struct_mapping_t map_csm_allocation_step_cgroup_delete_input_t;

extern const csmi_struct_mapping_t map_csm_allocation_resources_query_input_t;

extern const csmi_struct_mapping_t map_csm_allocation_resources_query_output_t;

extern const csmi_struct_mapping_t map_csm_allocation_update_history_input_t;

extern const csmi_struct_mapping_t map_csm_allocation_query_input_t;

extern const csmi_struct_mapping_t map_csm_allocation_query_output_t;

extern const csmi_struct_mapping_t map_csm_allocation_query_active_all_input_t;

extern const csmi_struct_mapping_t map_csm_allocation_query_active_all_output_t;

extern const csmi_struct_mapping_t map_csm_allocation_delete_input_t;

extern const csmi_struct_mapping_t map_csm_cgroup_login_input_t;

extern const csmi_struct_mapping_t map_csmi_allocation_gpu_metrics_t;

extern const csmi_struct_mapping_t map_csmi_allocation_mcast_context_t;

extern const csmi_struct_mapping_t map_csmi_allocation_mcast_payload_request_t;

extern const csmi_struct_mapping_t map_csmi_allocation_mcast_payload_response_t;

extern const csmi_struct_mapping_t map_csmi_allocation_step_mcast_context_t;

extern const csmi_struct_mapping_t map_csmi_allocation_step_mcast_payload_t;

extern const csmi_struct_mapping_t map_csmi_jsrun_cmd_payload_t;

extern const csmi_struct_mapping_t map_csm_jsrun_cmd_input_t;

extern const csmi_struct_mapping_t map_csmi_soft_failure_recovery_payload_t;

extern const csmi_struct_mapping_t map_csm_soft_failure_recovery_node_t;

extern const csmi_struct_mapping_t map_csm_soft_failure_recovery_input_t;

extern const csmi_struct_mapping_t map_csm_soft_failure_recovery_output_t;


#ifdef __cplusplus
}
#endif
#endif
