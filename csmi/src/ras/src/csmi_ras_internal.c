/*================================================================================
   
    csmi/src/ras/src/csmi_ras_internal.c

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
#include "csmi/src/ras/include/csmi_ras_internal.h"
#include "csmi/src/ras/include/csmi_ras_type_internal.h"
const csmi_struct_node_t csmi_ras_type_record_tree[15] = {{"threshold_period",offsetof(csmi_ras_type_record_t,threshold_period),0,NULL,0x45484d14,36},
{"msg_id",offsetof(csmi_ras_type_record_t,msg_id),0,NULL,0xe7c7058,4},
{"set_not_ready",offsetof(csmi_ras_type_record_t,set_not_ready),0,NULL,0x97e7daf5,16},
{"threshold_count",offsetof(csmi_ras_type_record_t,threshold_count),0,NULL,0x133c15a,36},
{"set_ready",offsetof(csmi_ras_type_record_t,set_ready),0,NULL,0x4341dbc5,16},
{"enabled",offsetof(csmi_ras_type_record_t,enabled),0,NULL,0x6a23e990,16},
{"visible_to_users",offsetof(csmi_ras_type_record_t,visible_to_users),0,NULL,0xa9800ac6,16},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"severity",offsetof(csmi_ras_type_record_t,severity),csmi_ras_severity_t_MAX,&csmi_ras_severity_t_strs,0x16a499a0,8},
{"set_state",offsetof(csmi_ras_type_record_t,set_state),csmi_node_state_t_MAX,&csmi_node_state_t_strs,0x435c2ff1,8},
{"control_action",offsetof(csmi_ras_type_record_t,control_action),0,NULL,0x4bd6e603,4},
{"description",offsetof(csmi_ras_type_record_t,description),0,NULL,0x91b0c789,4},
{NULL,0,0,NULL,0,0},
{"message",offsetof(csmi_ras_type_record_t,message),0,NULL,0xbe463eea,4}}
;

void* cast_csmi_ras_type_record_t(void* ptr,size_t index, char isArray) { 
    csmi_ras_type_record_t ** ptr_cast = *(csmi_ras_type_record_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_ras_type_record_t= {
    15,
    csmi_ras_type_record_tree,
    cast_csmi_ras_type_record_t
};

const csmi_struct_node_t csm_ras_event_create_input_tree[7] = {{"location_name",offsetof(csm_ras_event_create_input_t,location_name),0,NULL,0x54e4507e,4},
{"msg_id",offsetof(csm_ras_event_create_input_t,msg_id),0,NULL,0xe7c7058,4},
{"time_stamp",offsetof(csm_ras_event_create_input_t,time_stamp),0,NULL,0xae3ff458,4},
{NULL,0,0,NULL,0,0},
{"kvcsv",offsetof(csm_ras_event_create_input_t,kvcsv),0,NULL,0xfd1a732,4},
{NULL,0,0,NULL,0,0},
{"raw_data",offsetof(csm_ras_event_create_input_t,raw_data),0,NULL,0xf85a97e8,4}}
;

void* cast_csm_ras_event_create_input_t(void* ptr,size_t index, char isArray) { 
    csm_ras_event_create_input_t ** ptr_cast = *(csm_ras_event_create_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_ras_event_create_input_t= {
    7,
    csm_ras_event_create_input_tree,
    cast_csm_ras_event_create_input_t
};

const csmi_struct_node_t csmi_ras_event_action_record_tree[12] = {{"archive_history_time",offsetof(csmi_ras_event_action_record_t,archive_history_time),0,NULL,0x9e88b9e6,4},
{"rec_id",offsetof(csmi_ras_event_action_record_t,rec_id),0,NULL,0x1926b2eb,40},
{"msg_id_seq",offsetof(csmi_ras_event_action_record_t,msg_id_seq),0,NULL,0xdda2eb00,36},
{"count",offsetof(csmi_ras_event_action_record_t,count),0,NULL,0xf3d586e,36},
{"location_name",offsetof(csmi_ras_event_action_record_t,location_name),0,NULL,0x54e4507e,4},
{"message",offsetof(csmi_ras_event_action_record_t,message),0,NULL,0xbe463eea,4},
{"raw_data",offsetof(csmi_ras_event_action_record_t,raw_data),0,NULL,0xf85a97e8,4},
{"msg_id",offsetof(csmi_ras_event_action_record_t,msg_id),0,NULL,0xe7c7058,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"time_stamp",offsetof(csmi_ras_event_action_record_t,time_stamp),0,NULL,0xae3ff458,4}}
;

void* cast_csmi_ras_event_action_record_t(void* ptr,size_t index, char isArray) { 
    csmi_ras_event_action_record_t ** ptr_cast = *(csmi_ras_event_action_record_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_ras_event_action_record_t= {
    12,
    csmi_ras_event_action_record_tree,
    cast_csmi_ras_event_action_record_t
};

const csmi_struct_node_t csmi_ras_event_action_tree[13] = {{"rec_id",offsetof(csmi_ras_event_action_t,rec_id),0,NULL,0x1926b2eb,40},
{"count",offsetof(csmi_ras_event_action_t,count),0,NULL,0xf3d586e,36},
{"time_stamp",offsetof(csmi_ras_event_action_t,time_stamp),0,NULL,0xae3ff458,4},
{"msg_id",offsetof(csmi_ras_event_action_t,msg_id),0,NULL,0xe7c7058,4},
{NULL,0,0,NULL,0,0},
{"location_name",offsetof(csmi_ras_event_action_t,location_name),0,NULL,0x54e4507e,4},
{"msg_id_seq",offsetof(csmi_ras_event_action_t,msg_id_seq),0,NULL,0xdda2eb00,36},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"archive_history_time",offsetof(csmi_ras_event_action_t,archive_history_time),0,NULL,0x9e88b9e6,4},
{"message",offsetof(csmi_ras_event_action_t,message),0,NULL,0xbe463eea,4},
{"raw_data",offsetof(csmi_ras_event_action_t,raw_data),0,NULL,0xf85a97e8,4}}
;

void* cast_csmi_ras_event_action_t(void* ptr,size_t index, char isArray) { 
    csmi_ras_event_action_t ** ptr_cast = *(csmi_ras_event_action_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_ras_event_action_t= {
    13,
    csmi_ras_event_action_tree,
    cast_csmi_ras_event_action_t
};

const csmi_struct_node_t csmi_ras_event_tree[22] = {{"processor",offsetof(csmi_ras_event_t,processor),0,NULL,0x6cb287a5,36},
{"count",offsetof(csmi_ras_event_t,count),0,NULL,0xf3d586e,36},
{"suppress_ids",offsetof(csmi_ras_event_t,suppress_ids),0,NULL,0xad086749,4},
{"msg_id",offsetof(csmi_ras_event_t,msg_id),0,NULL,0xe7c7058,4},
{"control_action",offsetof(csmi_ras_event_t,control_action),0,NULL,0x4bd6e603,4},
{"min_time_in_pool",offsetof(csmi_ras_event_t,min_time_in_pool),0,NULL,0xa819bf26,36},
{"message",offsetof(csmi_ras_event_t,message),0,NULL,0xbe463eea,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"severity",offsetof(csmi_ras_event_t,severity),0,NULL,0x16a499a0,4},
{"location_name",offsetof(csmi_ras_event_t,location_name),0,NULL,0x54e4507e,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"time_stamp",offsetof(csmi_ras_event_t,time_stamp),0,NULL,0xae3ff458,4},
{"raw_data",offsetof(csmi_ras_event_t,raw_data),0,NULL,0xf85a97e8,4},
{"kvcsv",offsetof(csmi_ras_event_t,kvcsv),0,NULL,0xfd1a732,4},
{"rec_id",offsetof(csmi_ras_event_t,rec_id),0,NULL,0x1926b2eb,40},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"master_time_stamp",offsetof(csmi_ras_event_t,master_time_stamp),0,NULL,0xe32c0c43,4}}
;

void* cast_csmi_ras_event_t(void* ptr,size_t index, char isArray) { 
    csmi_ras_event_t ** ptr_cast = *(csmi_ras_event_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_ras_event_t= {
    22,
    csmi_ras_event_tree,
    cast_csmi_ras_event_t
};

const csmi_struct_node_t csmi_ras_event_vector_tree[3] = {{"num_ras_events",offsetof(csmi_ras_event_vector_t,num_ras_events),0,NULL,0xb3e851ae,24},
{NULL,0,0,NULL,0,0},
{"events",offsetof(csmi_ras_event_vector_t,events),offsetof(csmi_ras_event_vector_t, num_ras_events),&map_csmi_ras_event_t,0xfc089d5a,1}}
;

void* cast_csmi_ras_event_vector_t(void* ptr,size_t index, char isArray) { 
    csmi_ras_event_vector_t ** ptr_cast = *(csmi_ras_event_vector_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_ras_event_vector_t= {
    3,
    csmi_ras_event_vector_tree,
    cast_csmi_ras_event_vector_t
};

const csmi_struct_node_t csm_ras_event_query_input_tree[22] = {{"order_by",offsetof(csm_ras_event_query_input_t,order_by),0,NULL,0x245553bb,68},
{"offset",offsetof(csm_ras_event_query_input_t,offset),0,NULL,0x123b4b4c,36},
{"location_name",offsetof(csm_ras_event_query_input_t,location_name),0,NULL,0x54e4507e,4},
{"limit",offsetof(csm_ras_event_query_input_t,limit),0,NULL,0xfdcc804,36},
{"severity",offsetof(csm_ras_event_query_input_t,severity),csmi_ras_severity_t_MAX,&csmi_ras_severity_t_strs,0x16a499a0,8},
{"master_time_stamp_search_begin",offsetof(csm_ras_event_query_input_t,master_time_stamp_search_begin),0,NULL,0x2da38a1c,4},
{"end_time_stamp",offsetof(csm_ras_event_query_input_t,end_time_stamp),0,NULL,0x9c9c9dce,4},
{"msg_id",offsetof(csm_ras_event_query_input_t,msg_id),0,NULL,0xe7c7058,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"rec_id",offsetof(csm_ras_event_query_input_t,rec_id),0,NULL,0x1926b2eb,40},
{"start_time_stamp",offsetof(csm_ras_event_query_input_t,start_time_stamp),0,NULL,0x25f0f825,4},
{"control_action",offsetof(csm_ras_event_query_input_t,control_action),0,NULL,0x4bd6e603,4},
{NULL,0,0,NULL,0,0},
{"message",offsetof(csm_ras_event_query_input_t,message),0,NULL,0xbe463eea,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"master_time_stamp_search_end",offsetof(csm_ras_event_query_input_t,master_time_stamp_search_end),0,NULL,0x45dc3cce,4}}
;

void* cast_csm_ras_event_query_input_t(void* ptr,size_t index, char isArray) { 
    csm_ras_event_query_input_t ** ptr_cast = *(csm_ras_event_query_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_ras_event_query_input_t= {
    22,
    csm_ras_event_query_input_tree,
    cast_csm_ras_event_query_input_t
};

const csmi_struct_node_t csm_ras_event_query_output_tree[2] = {{"results_count",offsetof(csm_ras_event_query_output_t,results_count),0,NULL,0x8261013f,24},
{"results",offsetof(csm_ras_event_query_output_t,results),offsetof(csm_ras_event_query_output_t, results_count),&map_csmi_ras_event_t,0x3f2ab7f7,1}}
;

void* cast_csm_ras_event_query_output_t(void* ptr,size_t index, char isArray) { 
    csm_ras_event_query_output_t ** ptr_cast = *(csm_ras_event_query_output_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_ras_event_query_output_t= {
    2,
    csm_ras_event_query_output_tree,
    cast_csm_ras_event_query_output_t
};

const csmi_struct_node_t csm_ras_event_query_allocation_input_tree[3] = {{"offset",offsetof(csm_ras_event_query_allocation_input_t,offset),0,NULL,0x123b4b4c,36},
{"limit",offsetof(csm_ras_event_query_allocation_input_t,limit),0,NULL,0xfdcc804,36},
{"allocation_id",offsetof(csm_ras_event_query_allocation_input_t,allocation_id),0,NULL,0x99d3da77,40}}
;

void* cast_csm_ras_event_query_allocation_input_t(void* ptr,size_t index, char isArray) { 
    csm_ras_event_query_allocation_input_t ** ptr_cast = *(csm_ras_event_query_allocation_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_ras_event_query_allocation_input_t= {
    3,
    csm_ras_event_query_allocation_input_tree,
    cast_csm_ras_event_query_allocation_input_t
};

const csmi_struct_node_t csm_ras_event_query_allocation_output_tree[3] = {{"num_events",offsetof(csm_ras_event_query_allocation_output_t,num_events),0,NULL,0x4c25a8e9,24},
{NULL,0,0,NULL,0,0},
{"events",offsetof(csm_ras_event_query_allocation_output_t,events),offsetof(csm_ras_event_query_allocation_output_t, num_events),&map_csmi_ras_event_action_t,0xfc089d5a,1}}
;

void* cast_csm_ras_event_query_allocation_output_t(void* ptr,size_t index, char isArray) { 
    csm_ras_event_query_allocation_output_t ** ptr_cast = *(csm_ras_event_query_allocation_output_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_ras_event_query_allocation_output_t= {
    3,
    csm_ras_event_query_allocation_output_tree,
    cast_csm_ras_event_query_allocation_output_t
};

const csmi_struct_node_t csm_ras_msg_type_create_input_tree[15] = {{"threshold_period",offsetof(csm_ras_msg_type_create_input_t,threshold_period),0,NULL,0x45484d14,36},
{"msg_id",offsetof(csm_ras_msg_type_create_input_t,msg_id),0,NULL,0xe7c7058,4},
{"set_not_ready",offsetof(csm_ras_msg_type_create_input_t,set_not_ready),0,NULL,0x97e7daf5,16},
{"threshold_count",offsetof(csm_ras_msg_type_create_input_t,threshold_count),0,NULL,0x133c15a,36},
{"set_ready",offsetof(csm_ras_msg_type_create_input_t,set_ready),0,NULL,0x4341dbc5,16},
{"enabled",offsetof(csm_ras_msg_type_create_input_t,enabled),0,NULL,0x6a23e990,16},
{"visible_to_users",offsetof(csm_ras_msg_type_create_input_t,visible_to_users),0,NULL,0xa9800ac6,16},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"severity",offsetof(csm_ras_msg_type_create_input_t,severity),csmi_ras_severity_t_MAX,&csmi_ras_severity_t_strs,0x16a499a0,8},
{"set_state",offsetof(csm_ras_msg_type_create_input_t,set_state),csmi_node_state_t_MAX,&csmi_node_state_t_strs,0x435c2ff1,8},
{"control_action",offsetof(csm_ras_msg_type_create_input_t,control_action),0,NULL,0x4bd6e603,4},
{"description",offsetof(csm_ras_msg_type_create_input_t,description),0,NULL,0x91b0c789,4},
{NULL,0,0,NULL,0,0},
{"message",offsetof(csm_ras_msg_type_create_input_t,message),0,NULL,0xbe463eea,4}}
;

void* cast_csm_ras_msg_type_create_input_t(void* ptr,size_t index, char isArray) { 
    csm_ras_msg_type_create_input_t ** ptr_cast = *(csm_ras_msg_type_create_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_ras_msg_type_create_input_t= {
    15,
    csm_ras_msg_type_create_input_tree,
    cast_csm_ras_msg_type_create_input_t
};

const csmi_struct_node_t csm_ras_msg_type_create_output_tree[2] = {{"insert_successful",offsetof(csm_ras_msg_type_create_output_t,insert_successful),0,NULL,0xd2b0b4d9,16},
{"msg_id",offsetof(csm_ras_msg_type_create_output_t,msg_id),0,NULL,0xe7c7058,4}}
;

void* cast_csm_ras_msg_type_create_output_t(void* ptr,size_t index, char isArray) { 
    csm_ras_msg_type_create_output_t ** ptr_cast = *(csm_ras_msg_type_create_output_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_ras_msg_type_create_output_t= {
    2,
    csm_ras_msg_type_create_output_tree,
    cast_csm_ras_msg_type_create_output_t
};

const csmi_struct_node_t csm_ras_msg_type_delete_input_tree[2] = {{"msg_ids_count",offsetof(csm_ras_msg_type_delete_input_t,msg_ids_count),0,NULL,0xe8015413,24},
{"msg_ids",offsetof(csm_ras_msg_type_delete_input_t,msg_ids),offsetof(csm_ras_msg_type_delete_input_t, msg_ids_count),NULL,0xde0a7bcb,5}}
;

void* cast_csm_ras_msg_type_delete_input_t(void* ptr,size_t index, char isArray) { 
    csm_ras_msg_type_delete_input_t ** ptr_cast = *(csm_ras_msg_type_delete_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_ras_msg_type_delete_input_t= {
    2,
    csm_ras_msg_type_delete_input_tree,
    cast_csm_ras_msg_type_delete_input_t
};

const csmi_struct_node_t csm_ras_msg_type_delete_output_tree[5] = {{"deleted_msg_ids_count",offsetof(csm_ras_msg_type_delete_output_t,deleted_msg_ids_count),0,NULL,0xba570689,24},
{"not_deleted_msg_ids",offsetof(csm_ras_msg_type_delete_output_t,not_deleted_msg_ids),offsetof(csm_ras_msg_type_delete_output_t, not_deleted_msg_ids_count),NULL,0x8151eb71,5},
{"expected_number_of_deleted_msg_ids",offsetof(csm_ras_msg_type_delete_output_t,expected_number_of_deleted_msg_ids),0,NULL,0xc00f38ce,24},
{"not_deleted_msg_ids_count",offsetof(csm_ras_msg_type_delete_output_t,not_deleted_msg_ids_count),0,NULL,0xf236839,24},
{"deleted_msg_ids",offsetof(csm_ras_msg_type_delete_output_t,deleted_msg_ids),offsetof(csm_ras_msg_type_delete_output_t, deleted_msg_ids_count),NULL,0x9b010dc1,5}}
;

void* cast_csm_ras_msg_type_delete_output_t(void* ptr,size_t index, char isArray) { 
    csm_ras_msg_type_delete_output_t ** ptr_cast = *(csm_ras_msg_type_delete_output_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_ras_msg_type_delete_output_t= {
    5,
    csm_ras_msg_type_delete_output_tree,
    cast_csm_ras_msg_type_delete_output_t
};

const csmi_struct_node_t csm_ras_msg_type_update_input_tree[15] = {{"threshold_period",offsetof(csm_ras_msg_type_update_input_t,threshold_period),0,NULL,0x45484d14,36},
{"msg_id",offsetof(csm_ras_msg_type_update_input_t,msg_id),0,NULL,0xe7c7058,4},
{"set_not_ready",offsetof(csm_ras_msg_type_update_input_t,set_not_ready),0,NULL,0x97e7daf5,16},
{"threshold_count",offsetof(csm_ras_msg_type_update_input_t,threshold_count),0,NULL,0x133c15a,36},
{"set_ready",offsetof(csm_ras_msg_type_update_input_t,set_ready),0,NULL,0x4341dbc5,16},
{"enabled",offsetof(csm_ras_msg_type_update_input_t,enabled),0,NULL,0x6a23e990,16},
{"visible_to_users",offsetof(csm_ras_msg_type_update_input_t,visible_to_users),0,NULL,0xa9800ac6,16},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"severity",offsetof(csm_ras_msg_type_update_input_t,severity),csmi_ras_severity_t_MAX,&csmi_ras_severity_t_strs,0x16a499a0,8},
{"set_state",offsetof(csm_ras_msg_type_update_input_t,set_state),csmi_node_state_t_MAX,&csmi_node_state_t_strs,0x435c2ff1,8},
{"control_action",offsetof(csm_ras_msg_type_update_input_t,control_action),0,NULL,0x4bd6e603,4},
{"description",offsetof(csm_ras_msg_type_update_input_t,description),0,NULL,0x91b0c789,4},
{NULL,0,0,NULL,0,0},
{"message",offsetof(csm_ras_msg_type_update_input_t,message),0,NULL,0xbe463eea,4}}
;

void* cast_csm_ras_msg_type_update_input_t(void* ptr,size_t index, char isArray) { 
    csm_ras_msg_type_update_input_t ** ptr_cast = *(csm_ras_msg_type_update_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_ras_msg_type_update_input_t= {
    15,
    csm_ras_msg_type_update_input_tree,
    cast_csm_ras_msg_type_update_input_t
};

const csmi_struct_node_t csm_ras_msg_type_update_output_tree[2] = {{"update_successful",offsetof(csm_ras_msg_type_update_output_t,update_successful),0,NULL,0xf9cde27,16},
{"msg_id",offsetof(csm_ras_msg_type_update_output_t,msg_id),0,NULL,0xe7c7058,4}}
;

void* cast_csm_ras_msg_type_update_output_t(void* ptr,size_t index, char isArray) { 
    csm_ras_msg_type_update_output_t ** ptr_cast = *(csm_ras_msg_type_update_output_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_ras_msg_type_update_output_t= {
    2,
    csm_ras_msg_type_update_output_tree,
    cast_csm_ras_msg_type_update_output_t
};

const csmi_struct_node_t csm_ras_msg_type_query_input_tree[13] = {{"offset",offsetof(csm_ras_msg_type_query_input_t,offset),0,NULL,0x123b4b4c,36},
{"limit",offsetof(csm_ras_msg_type_query_input_t,limit),0,NULL,0xfdcc804,36},
{"control_action",offsetof(csm_ras_msg_type_query_input_t,control_action),0,NULL,0x4bd6e603,4},
{"msg_id",offsetof(csm_ras_msg_type_query_input_t,msg_id),0,NULL,0xe7c7058,4},
{NULL,0,0,NULL,0,0},
{"severity",offsetof(csm_ras_msg_type_query_input_t,severity),csmi_ras_severity_t_MAX,&csmi_ras_severity_t_strs,0x16a499a0,8},
{"set_states",offsetof(csm_ras_msg_type_query_input_t,set_states),offsetof(csm_ras_msg_type_query_input_t, set_states_count),NULL,0xaee22e84,5},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"set_states_count",offsetof(csm_ras_msg_type_query_input_t,set_states_count),0,NULL,0x5d2c6d8c,24},
{"message",offsetof(csm_ras_msg_type_query_input_t,message),0,NULL,0xbe463eea,4}}
;

void* cast_csm_ras_msg_type_query_input_t(void* ptr,size_t index, char isArray) { 
    csm_ras_msg_type_query_input_t ** ptr_cast = *(csm_ras_msg_type_query_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_ras_msg_type_query_input_t= {
    13,
    csm_ras_msg_type_query_input_tree,
    cast_csm_ras_msg_type_query_input_t
};

const csmi_struct_node_t csm_ras_msg_type_query_output_tree[2] = {{"results_count",offsetof(csm_ras_msg_type_query_output_t,results_count),0,NULL,0x8261013f,24},
{"results",offsetof(csm_ras_msg_type_query_output_t,results),offsetof(csm_ras_msg_type_query_output_t, results_count),&map_csmi_ras_type_record_t,0x3f2ab7f7,1}}
;

void* cast_csm_ras_msg_type_query_output_t(void* ptr,size_t index, char isArray) { 
    csm_ras_msg_type_query_output_t ** ptr_cast = *(csm_ras_msg_type_query_output_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_ras_msg_type_query_output_t= {
    2,
    csm_ras_msg_type_query_output_tree,
    cast_csm_ras_msg_type_query_output_t
};

const csmi_struct_node_t csm_ras_subscribe_input_tree[1] = {{"topic",offsetof(csm_ras_subscribe_input_t,topic),0,NULL,0x1070e304,4}}
;

void* cast_csm_ras_subscribe_input_t(void* ptr,size_t index, char isArray) { 
    csm_ras_subscribe_input_t ** ptr_cast = *(csm_ras_subscribe_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_ras_subscribe_input_t= {
    1,
    csm_ras_subscribe_input_tree,
    cast_csm_ras_subscribe_input_t
};

const csmi_struct_node_t csm_ras_unsubscribe_input_tree[1] = {{"topic",offsetof(csm_ras_unsubscribe_input_t,topic),0,NULL,0x1070e304,4}}
;

void* cast_csm_ras_unsubscribe_input_t(void* ptr,size_t index, char isArray) { 
    csm_ras_unsubscribe_input_t ** ptr_cast = *(csm_ras_unsubscribe_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_ras_unsubscribe_input_t= {
    1,
    csm_ras_unsubscribe_input_tree,
    cast_csm_ras_unsubscribe_input_t
};

