/*================================================================================
   
    csmi/src/bb/src/csmi_bb_internal.c

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
#include "csmi/src/bb/include/csmi_bb_internal.h"
#include "csmi/src/bb/include/csmi_bb_type_internal.h"
const csmi_struct_node_t csmi_vg_record_tree[5] = {{"total_size",offsetof(csmi_vg_record_t,total_size),0,NULL,0xc7f736e3,40},
{"node_name",offsetof(csmi_vg_record_t,node_name),0,NULL,0x746e3e2b,4},
{"scheduler",offsetof(csmi_vg_record_t,scheduler),0,NULL,0xdc0deaa4,16},
{"available_size",offsetof(csmi_vg_record_t,available_size),0,NULL,0x9b91340,40},
{"vg_name",offsetof(csmi_vg_record_t,vg_name),0,NULL,0x76500bc2,4}}
;

void* cast_csmi_vg_record_t(void* ptr,size_t index, char isArray) { 
    csmi_vg_record_t ** ptr_cast = *(csmi_vg_record_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_vg_record_t= {
    5,
    csmi_vg_record_tree,
    cast_csmi_vg_record_t
};

const csmi_struct_node_t csmi_lv_record_tree[15] = {{"max_size",offsetof(csmi_lv_record_t,max_size),0,NULL,0x5d6ef4a5,40},
{"file_system_mount",offsetof(csmi_lv_record_t,file_system_mount),0,NULL,0x33eec6bb,4},
{"vg_name",offsetof(csmi_lv_record_t,vg_name),0,NULL,0x76500bc2,4},
{"state",offsetof(csmi_lv_record_t,state),0,NULL,0x10614a06,68},
{"current_size",offsetof(csmi_lv_record_t,current_size),0,NULL,0x454b21c2,40},
{"logical_volume_name",offsetof(csmi_lv_record_t,logical_volume_name),0,NULL,0x7221a037,4},
{"allocation_id",offsetof(csmi_lv_record_t,allocation_id),0,NULL,0x99d3da77,40},
{NULL,0,0,NULL,0,0},
{"updated_time",offsetof(csmi_lv_record_t,updated_time),0,NULL,0x247e0e7a,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"begin_time",offsetof(csmi_lv_record_t,begin_time),0,NULL,0x5f818b18,4},
{"node_name",offsetof(csmi_lv_record_t,node_name),0,NULL,0x746e3e2b,4},
{NULL,0,0,NULL,0,0},
{"file_system_type",offsetof(csmi_lv_record_t,file_system_type),0,NULL,0xa47f99ea,4}}
;

void* cast_csmi_lv_record_t(void* ptr,size_t index, char isArray) { 
    csmi_lv_record_t ** ptr_cast = *(csmi_lv_record_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_lv_record_t= {
    15,
    csmi_lv_record_tree,
    cast_csmi_lv_record_t
};

const csmi_struct_node_t csmi_bb_vg_ssd_info_tree[2] = {{"ssd_allocation",offsetof(csmi_bb_vg_ssd_info_t,ssd_allocation),0,NULL,0x953b1814,40},
{"ssd_serial_number",offsetof(csmi_bb_vg_ssd_info_t,ssd_serial_number),0,NULL,0x1beecdb6,4}}
;

void* cast_csmi_bb_vg_ssd_info_t(void* ptr,size_t index, char isArray) { 
    csmi_bb_vg_ssd_info_t ** ptr_cast = *(csmi_bb_vg_ssd_info_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_bb_vg_ssd_info_t= {
    2,
    csmi_bb_vg_ssd_info_tree,
    cast_csmi_bb_vg_ssd_info_t
};

const csmi_struct_node_t csm_bb_cmd_input_tree[3] = {{"command_arguments",offsetof(csm_bb_cmd_input_t,command_arguments),0,NULL,0x6a022ed9,4},
{"node_names",offsetof(csm_bb_cmd_input_t,node_names),offsetof(csm_bb_cmd_input_t, node_names_count),NULL,0x23603fe,5},
{"node_names_count",offsetof(csm_bb_cmd_input_t,node_names_count),0,NULL,0x868cf686,24}}
;

void* cast_csm_bb_cmd_input_t(void* ptr,size_t index, char isArray) { 
    csm_bb_cmd_input_t ** ptr_cast = *(csm_bb_cmd_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_bb_cmd_input_t= {
    3,
    csm_bb_cmd_input_tree,
    cast_csm_bb_cmd_input_t
};

const csmi_struct_node_t csm_bb_cmd_output_tree[1] = {{"command_output",offsetof(csm_bb_cmd_output_t,command_output),0,NULL,0x7c09e914,4}}
;

void* cast_csm_bb_cmd_output_t(void* ptr,size_t index, char isArray) { 
    csm_bb_cmd_output_t ** ptr_cast = *(csm_bb_cmd_output_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_bb_cmd_output_t= {
    1,
    csm_bb_cmd_output_tree,
    cast_csm_bb_cmd_output_t
};

const csmi_struct_node_t csm_bb_lv_create_input_tree[13] = {{"current_size",offsetof(csm_bb_lv_create_input_t,current_size),0,NULL,0x454b21c2,40},
{"state",offsetof(csm_bb_lv_create_input_t,state),0,NULL,0x10614a06,68},
{"allocation_id",offsetof(csm_bb_lv_create_input_t,allocation_id),0,NULL,0x99d3da77,40},
{NULL,0,0,NULL,0,0},
{"file_system_mount",offsetof(csm_bb_lv_create_input_t,file_system_mount),0,NULL,0x33eec6bb,4},
{"node_name",offsetof(csm_bb_lv_create_input_t,node_name),0,NULL,0x746e3e2b,4},
{"file_system_type",offsetof(csm_bb_lv_create_input_t,file_system_type),0,NULL,0xa47f99ea,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"logical_volume_name",offsetof(csm_bb_lv_create_input_t,logical_volume_name),0,NULL,0x7221a037,4},
{"vg_name",offsetof(csm_bb_lv_create_input_t,vg_name),0,NULL,0x76500bc2,4}}
;

void* cast_csm_bb_lv_create_input_t(void* ptr,size_t index, char isArray) { 
    csm_bb_lv_create_input_t ** ptr_cast = *(csm_bb_lv_create_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_bb_lv_create_input_t= {
    13,
    csm_bb_lv_create_input_tree,
    cast_csm_bb_lv_create_input_t
};

const csmi_struct_node_t csm_bb_lv_delete_input_tree[15] = {{"num_bytes_read",offsetof(csm_bb_lv_delete_input_t,num_bytes_read),0,NULL,0x38181676,40},
{"num_bytes_written",offsetof(csm_bb_lv_delete_input_t,num_bytes_written),0,NULL,0xd3acfa7,40},
{"node_name",offsetof(csm_bb_lv_delete_input_t,node_name),0,NULL,0x746e3e2b,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"logical_volume_name",offsetof(csm_bb_lv_delete_input_t,logical_volume_name),0,NULL,0x7221a037,4},
{"allocation_id",offsetof(csm_bb_lv_delete_input_t,allocation_id),0,NULL,0x99d3da77,40},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"num_writes",offsetof(csm_bb_lv_delete_input_t,num_writes),0,NULL,0x75dc6b52,40},
{"num_reads",offsetof(csm_bb_lv_delete_input_t,num_reads),0,NULL,0xa6194b83,40}}
;

void* cast_csm_bb_lv_delete_input_t(void* ptr,size_t index, char isArray) { 
    csm_bb_lv_delete_input_t ** ptr_cast = *(csm_bb_lv_delete_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_bb_lv_delete_input_t= {
    15,
    csm_bb_lv_delete_input_tree,
    cast_csm_bb_lv_delete_input_t
};

const csmi_struct_node_t csm_bb_lv_query_input_tree[14] = {{"offset",offsetof(csm_bb_lv_query_input_t,offset),0,NULL,0x123b4b4c,36},
{"logical_volume_names_count",offsetof(csm_bb_lv_query_input_t,logical_volume_names_count),0,NULL,0x3741312,24},
{"node_names_count",offsetof(csm_bb_lv_query_input_t,node_names_count),0,NULL,0x868cf686,24},
{"node_names",offsetof(csm_bb_lv_query_input_t,node_names),offsetof(csm_bb_lv_query_input_t, node_names_count),NULL,0x23603fe,5},
{"limit",offsetof(csm_bb_lv_query_input_t,limit),0,NULL,0xfdcc804,36},
{"allocation_ids_count",offsetof(csm_bb_lv_query_input_t,allocation_ids_count),0,NULL,0x49964552,24},
{"allocation_ids",offsetof(csm_bb_lv_query_input_t,allocation_ids),offsetof(csm_bb_lv_query_input_t, allocation_ids_count),NULL,0xd44f29ca,1},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"logical_volume_names",offsetof(csm_bb_lv_query_input_t,logical_volume_names),offsetof(csm_bb_lv_query_input_t, logical_volume_names_count),NULL,0xb655a78a,5}}
;

void* cast_csm_bb_lv_query_input_t(void* ptr,size_t index, char isArray) { 
    csm_bb_lv_query_input_t ** ptr_cast = *(csm_bb_lv_query_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_bb_lv_query_input_t= {
    14,
    csm_bb_lv_query_input_tree,
    cast_csm_bb_lv_query_input_t
};

const csmi_struct_node_t csm_bb_lv_query_output_tree[2] = {{"results_count",offsetof(csm_bb_lv_query_output_t,results_count),0,NULL,0x8261013f,24},
{"results",offsetof(csm_bb_lv_query_output_t,results),offsetof(csm_bb_lv_query_output_t, results_count),&map_csmi_lv_record_t,0x3f2ab7f7,1}}
;

void* cast_csm_bb_lv_query_output_t(void* ptr,size_t index, char isArray) { 
    csm_bb_lv_query_output_t ** ptr_cast = *(csm_bb_lv_query_output_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_bb_lv_query_output_t= {
    2,
    csm_bb_lv_query_output_tree,
    cast_csm_bb_lv_query_output_t
};

const csmi_struct_node_t csm_bb_lv_update_input_tree[7] = {{"current_size",offsetof(csm_bb_lv_update_input_t,current_size),0,NULL,0x454b21c2,40},
{"state",offsetof(csm_bb_lv_update_input_t,state),0,NULL,0x10614a06,68},
{"node_name",offsetof(csm_bb_lv_update_input_t,node_name),0,NULL,0x746e3e2b,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"logical_volume_name",offsetof(csm_bb_lv_update_input_t,logical_volume_name),0,NULL,0x7221a037,4},
{"allocation_id",offsetof(csm_bb_lv_update_input_t,allocation_id),0,NULL,0x99d3da77,40}}
;

void* cast_csm_bb_lv_update_input_t(void* ptr,size_t index, char isArray) { 
    csm_bb_lv_update_input_t ** ptr_cast = *(csm_bb_lv_update_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_bb_lv_update_input_t= {
    7,
    csm_bb_lv_update_input_tree,
    cast_csm_bb_lv_update_input_t
};

const csmi_struct_node_t csm_bb_vg_create_input_tree[13] = {{"ssd_info_count",offsetof(csm_bb_vg_create_input_t,ssd_info_count),0,NULL,0xbca8962,24},
{"available_size",offsetof(csm_bb_vg_create_input_t,available_size),0,NULL,0x9b91340,40},
{"total_size",offsetof(csm_bb_vg_create_input_t,total_size),0,NULL,0xc7f736e3,40},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"node_name",offsetof(csm_bb_vg_create_input_t,node_name),0,NULL,0x746e3e2b,4},
{"scheduler",offsetof(csm_bb_vg_create_input_t,scheduler),0,NULL,0xdc0deaa4,16},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"ssd_info",offsetof(csm_bb_vg_create_input_t,ssd_info),offsetof(csm_bb_vg_create_input_t, ssd_info_count),&map_csmi_bb_vg_ssd_info_t,0x21e5a1da,1},
{"vg_name",offsetof(csm_bb_vg_create_input_t,vg_name),0,NULL,0x76500bc2,4}}
;

void* cast_csm_bb_vg_create_input_t(void* ptr,size_t index, char isArray) { 
    csm_bb_vg_create_input_t ** ptr_cast = *(csm_bb_vg_create_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_bb_vg_create_input_t= {
    13,
    csm_bb_vg_create_input_tree,
    cast_csm_bb_vg_create_input_t
};

const csmi_struct_node_t csm_bb_vg_delete_input_tree[3] = {{"node_name",offsetof(csm_bb_vg_delete_input_t,node_name),0,NULL,0x746e3e2b,4},
{NULL,0,0,NULL,0,0},
{"vg_name",offsetof(csm_bb_vg_delete_input_t,vg_name),0,NULL,0x76500bc2,4}}
;

void* cast_csm_bb_vg_delete_input_t(void* ptr,size_t index, char isArray) { 
    csm_bb_vg_delete_input_t ** ptr_cast = *(csm_bb_vg_delete_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_bb_vg_delete_input_t= {
    3,
    csm_bb_vg_delete_input_tree,
    cast_csm_bb_vg_delete_input_t
};

const csmi_struct_node_t csm_bb_vg_delete_output_tree[2] = {{"failure_count",offsetof(csm_bb_vg_delete_output_t,failure_count),0,NULL,0xb64de7b5,24},
{"failure_vg_names",offsetof(csm_bb_vg_delete_output_t,failure_vg_names),offsetof(csm_bb_vg_delete_output_t, failure_count),NULL,0x9bf2a25c,5}}
;

void* cast_csm_bb_vg_delete_output_t(void* ptr,size_t index, char isArray) { 
    csm_bb_vg_delete_output_t ** ptr_cast = *(csm_bb_vg_delete_output_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_bb_vg_delete_output_t= {
    2,
    csm_bb_vg_delete_output_tree,
    cast_csm_bb_vg_delete_output_t
};

const csmi_struct_node_t csm_bb_vg_query_input_tree[7] = {{"offset",offsetof(csm_bb_vg_query_input_t,offset),0,NULL,0x123b4b4c,36},
{"limit",offsetof(csm_bb_vg_query_input_t,limit),0,NULL,0xfdcc804,36},
{"node_names_count",offsetof(csm_bb_vg_query_input_t,node_names_count),0,NULL,0x868cf686,24},
{"node_names",offsetof(csm_bb_vg_query_input_t,node_names),offsetof(csm_bb_vg_query_input_t, node_names_count),NULL,0x23603fe,5},
{NULL,0,0,NULL,0,0},
{"vg_names",offsetof(csm_bb_vg_query_input_t,vg_names),offsetof(csm_bb_vg_query_input_t, vg_names_count),NULL,0x40518475,5},
{"vg_names_count",offsetof(csm_bb_vg_query_input_t,vg_names_count),0,NULL,0x939ab43d,24}}
;

void* cast_csm_bb_vg_query_input_t(void* ptr,size_t index, char isArray) { 
    csm_bb_vg_query_input_t ** ptr_cast = *(csm_bb_vg_query_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_bb_vg_query_input_t= {
    7,
    csm_bb_vg_query_input_tree,
    cast_csm_bb_vg_query_input_t
};

const csmi_struct_node_t csm_bb_vg_query_output_tree[2] = {{"results_count",offsetof(csm_bb_vg_query_output_t,results_count),0,NULL,0x8261013f,24},
{"results",offsetof(csm_bb_vg_query_output_t,results),offsetof(csm_bb_vg_query_output_t, results_count),&map_csmi_vg_record_t,0x3f2ab7f7,1}}
;

void* cast_csm_bb_vg_query_output_t(void* ptr,size_t index, char isArray) { 
    csm_bb_vg_query_output_t ** ptr_cast = *(csm_bb_vg_query_output_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_bb_vg_query_output_t= {
    2,
    csm_bb_vg_query_output_tree,
    cast_csm_bb_vg_query_output_t
};

const csmi_struct_node_t csmi_bb_cmd_payload_tree[3] = {{"bb_cmd_str",offsetof(csmi_bb_cmd_payload_t,bb_cmd_str),0,NULL,0xd01a1df4,4},
{"bb_cmd_int",offsetof(csmi_bb_cmd_payload_t,bb_cmd_int),0,NULL,0xd019f2a6,24},
{"hostname",offsetof(csmi_bb_cmd_payload_t,hostname),0,NULL,0xeba474a4,4}}
;

void* cast_csmi_bb_cmd_payload_t(void* ptr,size_t index, char isArray) { 
    csmi_bb_cmd_payload_t ** ptr_cast = *(csmi_bb_cmd_payload_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_bb_cmd_payload_t= {
    3,
    csmi_bb_cmd_payload_tree,
    cast_csmi_bb_cmd_payload_t
};

