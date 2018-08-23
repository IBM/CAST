/*================================================================================
   
    csmi/src/inv/include/csmi_inv_internal.h

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
#ifndef _CSMI_INV_INTERNAL_H_
#define _CSMI_INV_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "csmi/include/csmi_type_inv.h"
#include "csmi/src/common/include/csmi_struct_hash.h"
extern const csmi_struct_mapping_t map_csmi_dimm_record_t;

extern const csmi_struct_mapping_t map_csmi_gpu_record_t;

extern const csmi_struct_mapping_t map_csmi_hca_record_t;

extern const csmi_struct_mapping_t map_csmi_ib_cable_record_t;

extern const csmi_struct_mapping_t map_csmi_ib_cable_history_record_t;

extern const csmi_struct_mapping_t map_csmi_node_attributes_record_t;

extern const csmi_struct_mapping_t map_csmi_node_attributes_history_record_t;

extern const csmi_struct_mapping_t map_csmi_node_query_state_history_record_t;

extern const csmi_struct_mapping_t map_csmi_processor_record_t;

extern const csmi_struct_mapping_t map_csmi_ssd_record_t;

extern const csmi_struct_mapping_t map_csmi_switch_record_t;

extern const csmi_struct_mapping_t map_csmi_switch_inventory_record_t;

extern const csmi_struct_mapping_t map_csmi_switch_ports_record_t;

extern const csmi_struct_mapping_t map_csmi_switch_details_t;

extern const csmi_struct_mapping_t map_csmi_switch_history_record_t;

extern const csmi_struct_mapping_t map_csmi_node_env_data_t;

extern const csmi_struct_mapping_t map_csmi_switch_env_data_t;

extern const csmi_struct_mapping_t map_csmi_fabric_topology_t;

extern const csmi_struct_mapping_t map_csmi_node_details_t;

extern const csmi_struct_mapping_t map_csmi_cluster_query_state_record_t;

extern const csmi_struct_mapping_t map_csmi_node_find_job_record_t;

extern const csmi_struct_mapping_t map_csm_ib_cable_inventory_collection_input_t;

extern const csmi_struct_mapping_t map_csm_ib_cable_inventory_collection_output_t;

extern const csmi_struct_mapping_t map_csm_ib_cable_query_input_t;

extern const csmi_struct_mapping_t map_csm_ib_cable_query_output_t;

extern const csmi_struct_mapping_t map_csm_ib_cable_query_history_input_t;

extern const csmi_struct_mapping_t map_csm_ib_cable_query_history_output_t;

extern const csmi_struct_mapping_t map_csm_ib_cable_update_input_t;

extern const csmi_struct_mapping_t map_csm_ib_cable_update_output_t;

extern const csmi_struct_mapping_t map_csm_node_attributes_query_input_t;

extern const csmi_struct_mapping_t map_csm_node_attributes_query_output_t;

extern const csmi_struct_mapping_t map_csm_node_attributes_query_details_input_t;

extern const csmi_struct_mapping_t map_csm_node_attributes_query_details_output_t;

extern const csmi_struct_mapping_t map_csm_node_attributes_query_history_input_t;

extern const csmi_struct_mapping_t map_csm_node_attributes_query_history_output_t;

extern const csmi_struct_mapping_t map_csm_node_query_state_history_input_t;

extern const csmi_struct_mapping_t map_csm_node_query_state_history_output_t;

extern const csmi_struct_mapping_t map_csm_node_attributes_update_input_t;

extern const csmi_struct_mapping_t map_csm_node_attributes_update_output_t;

extern const csmi_struct_mapping_t map_csm_node_delete_input_t;

extern const csmi_struct_mapping_t map_csm_node_delete_output_t;

extern const csmi_struct_mapping_t map_csm_node_find_job_input_t;

extern const csmi_struct_mapping_t map_csm_node_find_job_output_t;

extern const csmi_struct_mapping_t map_csm_switch_attributes_query_input_t;

extern const csmi_struct_mapping_t map_csm_switch_attributes_query_output_t;

extern const csmi_struct_mapping_t map_csm_switch_attributes_query_details_input_t;

extern const csmi_struct_mapping_t map_csm_switch_attributes_query_details_output_t;

extern const csmi_struct_mapping_t map_csm_switch_attributes_query_history_input_t;

extern const csmi_struct_mapping_t map_csm_switch_attributes_query_history_output_t;

extern const csmi_struct_mapping_t map_csm_switch_attributes_update_input_t;

extern const csmi_struct_mapping_t map_csm_switch_attributes_update_output_t;

extern const csmi_struct_mapping_t map_csm_switch_inventory_collection_input_t;

extern const csmi_struct_mapping_t map_csm_switch_inventory_collection_output_t;

extern const csmi_struct_mapping_t map_csm_switch_children_inventory_collection_input_t;

extern const csmi_struct_mapping_t map_csm_switch_children_inventory_collection_output_t;

extern const csmi_struct_mapping_t map_csm_cluster_query_state_input_t;

extern const csmi_struct_mapping_t map_csm_cluster_query_state_output_t;


#ifdef __cplusplus
}
#endif
#endif
