/*================================================================================
   
    csmi/src/diag/src/csmi_diag_internal.c

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
#include "csmi/src/diag/include/csmi_diag_internal.h"
#include "csmi/src/diag/include/csmi_diag_type_internal.h"
const csmi_struct_node_t csmi_diag_run_tree[10] = {{"cmd_line",offsetof(csmi_diag_run_t,cmd_line),0,NULL,0x8fec7820,4},
{"diag_status",offsetof(csmi_diag_run_t,diag_status),16,NULL,0x8321a7dd,70},
{"allocation_id",offsetof(csmi_diag_run_t,allocation_id),0,NULL,0x99d3da77,40},
{"begin_time",offsetof(csmi_diag_run_t,begin_time),0,NULL,0x5f818b18,4},
{"inserted_ras",offsetof(csmi_diag_run_t,inserted_ras),0,NULL,0x89b6b1e8,16},
{NULL,0,0,NULL,0,0},
{"end_time",offsetof(csmi_diag_run_t,end_time),0,NULL,0xb56ec18a,4},
{"run_id",offsetof(csmi_diag_run_t,run_id),0,NULL,0x1a4e4326,40},
{"history_time",offsetof(csmi_diag_run_t,history_time),0,NULL,0x60dc8265,4},
{"log_dir",offsetof(csmi_diag_run_t,log_dir),0,NULL,0x87bb87e5,4}}
;

void* cast_csmi_diag_run_t(void* ptr,size_t index, char isArray) { 
    csmi_diag_run_t ** ptr_cast = *(csmi_diag_run_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_diag_run_t= {
    10,
    csmi_diag_run_tree,
    cast_csmi_diag_run_t
};

const csmi_struct_node_t csmi_diag_run_query_details_result_tree[12] = {{"node_name",offsetof(csmi_diag_run_query_details_result_t,node_name),0,NULL,0x746e3e2b,4},
{"status",offsetof(csmi_diag_run_query_details_result_t,status),16,NULL,0x1c8a8d49,70},
{"end_time",offsetof(csmi_diag_run_query_details_result_t,end_time),0,NULL,0xb56ec18a,4},
{"run_id",offsetof(csmi_diag_run_query_details_result_t,run_id),0,NULL,0x1a4e4326,40},
{"history_time",offsetof(csmi_diag_run_query_details_result_t,history_time),0,NULL,0x60dc8265,4},
{"test_name",offsetof(csmi_diag_run_query_details_result_t,test_name),0,NULL,0x9a6d8425,4},
{"serial_number",offsetof(csmi_diag_run_query_details_result_t,serial_number),0,NULL,0xd931f68d,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"begin_time",offsetof(csmi_diag_run_query_details_result_t,begin_time),0,NULL,0x5f818b18,4},
{NULL,0,0,NULL,0,0},
{"log_file",offsetof(csmi_diag_run_query_details_result_t,log_file),0,NULL,0x7f2d9ce6,4}}
;

void* cast_csmi_diag_run_query_details_result_t(void* ptr,size_t index, char isArray) { 
    csmi_diag_run_query_details_result_t ** ptr_cast = *(csmi_diag_run_query_details_result_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_diag_run_query_details_result_t= {
    12,
    csmi_diag_run_query_details_result_tree,
    cast_csmi_diag_run_query_details_result_t
};

const csmi_struct_node_t csm_diag_run_end_input_tree[3] = {{"status",offsetof(csm_diag_run_end_input_t,status),16,NULL,0x1c8a8d49,70},
{"run_id",offsetof(csm_diag_run_end_input_t,run_id),0,NULL,0x1a4e4326,40},
{"inserted_ras",offsetof(csm_diag_run_end_input_t,inserted_ras),0,NULL,0x89b6b1e8,16}}
;

void* cast_csm_diag_run_end_input_t(void* ptr,size_t index, char isArray) { 
    csm_diag_run_end_input_t ** ptr_cast = *(csm_diag_run_end_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_diag_run_end_input_t= {
    3,
    csm_diag_run_end_input_tree,
    cast_csm_diag_run_end_input_t
};

const csmi_struct_node_t csm_diag_result_create_input_tree[9] = {{"status",offsetof(csm_diag_result_create_input_t,status),16,NULL,0x1c8a8d49,70},
{"run_id",offsetof(csm_diag_result_create_input_t,run_id),0,NULL,0x1a4e4326,40},
{"test_name",offsetof(csm_diag_result_create_input_t,test_name),0,NULL,0x9a6d8425,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"node_name",offsetof(csm_diag_result_create_input_t,node_name),0,NULL,0x746e3e2b,4},
{"serial_number",offsetof(csm_diag_result_create_input_t,serial_number),0,NULL,0xd931f68d,4},
{"begin_time",offsetof(csm_diag_result_create_input_t,begin_time),0,NULL,0x5f818b18,4},
{"log_file",offsetof(csm_diag_result_create_input_t,log_file),0,NULL,0x7f2d9ce6,4}}
;

void* cast_csm_diag_result_create_input_t(void* ptr,size_t index, char isArray) { 
    csm_diag_result_create_input_t ** ptr_cast = *(csm_diag_result_create_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_diag_result_create_input_t= {
    9,
    csm_diag_result_create_input_tree,
    cast_csm_diag_result_create_input_t
};

const csmi_struct_node_t csm_diag_run_begin_input_tree[5] = {{"cmd_line",offsetof(csm_diag_run_begin_input_t,cmd_line),0,NULL,0x8fec7820,4},
{"run_id",offsetof(csm_diag_run_begin_input_t,run_id),0,NULL,0x1a4e4326,40},
{"allocation_id",offsetof(csm_diag_run_begin_input_t,allocation_id),0,NULL,0x99d3da77,40},
{NULL,0,0,NULL,0,0},
{"log_dir",offsetof(csm_diag_run_begin_input_t,log_dir),0,NULL,0x87bb87e5,4}}
;

void* cast_csm_diag_run_begin_input_t(void* ptr,size_t index, char isArray) { 
    csm_diag_run_begin_input_t ** ptr_cast = *(csm_diag_run_begin_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_diag_run_begin_input_t= {
    5,
    csm_diag_run_begin_input_tree,
    cast_csm_diag_run_begin_input_t
};

const csmi_struct_node_t csm_diag_run_query_input_tree[22] = {{"allocation_ids_count",offsetof(csm_diag_run_query_input_t,allocation_ids_count),0,NULL,0x49964552,24},
{"offset",offsetof(csm_diag_run_query_input_t,offset),0,NULL,0x123b4b4c,36},
{"inserted_ras",offsetof(csm_diag_run_query_input_t,inserted_ras),0,NULL,0x89b6b1e8,16},
{"limit",offsetof(csm_diag_run_query_input_t,limit),0,NULL,0xfdcc804,36},
{"status",offsetof(csm_diag_run_query_input_t,status),0,NULL,0x1c8a8d49,20},
{"run_ids_count",offsetof(csm_diag_run_query_input_t,run_ids_count),0,NULL,0x7c4c3321,24},
{"begin_time_search_end",offsetof(csm_diag_run_query_input_t,begin_time_search_end),0,NULL,0xc583ac83,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"begin_time_search_begin",offsetof(csm_diag_run_query_input_t,begin_time_search_begin),0,NULL,0x34e5bb11,4},
{"run_ids",offsetof(csm_diag_run_query_input_t,run_ids),offsetof(csm_diag_run_query_input_t, run_ids_count),NULL,0x6416a859,1},
{NULL,0,0,NULL,0,0},
{"end_time_search_end",offsetof(csm_diag_run_query_input_t,end_time_search_end),0,NULL,0x8f62b7b5,4},
{"end_time_search_begin",offsetof(csm_diag_run_query_input_t,end_time_search_begin),0,NULL,0xf2b45ac3,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"allocation_ids",offsetof(csm_diag_run_query_input_t,allocation_ids),offsetof(csm_diag_run_query_input_t, allocation_ids_count),NULL,0xd44f29ca,1}}
;

void* cast_csm_diag_run_query_input_t(void* ptr,size_t index, char isArray) { 
    csm_diag_run_query_input_t ** ptr_cast = *(csm_diag_run_query_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_diag_run_query_input_t= {
    22,
    csm_diag_run_query_input_tree,
    cast_csm_diag_run_query_input_t
};

const csmi_struct_node_t csm_diag_run_query_output_tree[3] = {{"num_runs",offsetof(csm_diag_run_query_output_t,num_runs),0,NULL,0x33946edc,24},
{NULL,0,0,NULL,0,0},
{"runs",offsetof(csm_diag_run_query_output_t,runs),offsetof(csm_diag_run_query_output_t, num_runs),&map_csmi_diag_run_t,0x7c9d930d,1}}
;

void* cast_csm_diag_run_query_output_t(void* ptr,size_t index, char isArray) { 
    csm_diag_run_query_output_t ** ptr_cast = *(csm_diag_run_query_output_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_diag_run_query_output_t= {
    3,
    csm_diag_run_query_output_tree,
    cast_csm_diag_run_query_output_t
};

const csmi_struct_node_t csm_diag_run_query_details_input_tree[1] = {{"run_id",offsetof(csm_diag_run_query_details_input_t,run_id),0,NULL,0x1a4e4326,40}}
;

void* cast_csm_diag_run_query_details_input_t(void* ptr,size_t index, char isArray) { 
    csm_diag_run_query_details_input_t ** ptr_cast = *(csm_diag_run_query_details_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_diag_run_query_details_input_t= {
    1,
    csm_diag_run_query_details_input_tree,
    cast_csm_diag_run_query_details_input_t
};

const csmi_struct_node_t csm_diag_run_query_details_output_tree[3] = {{"num_details",offsetof(csm_diag_run_query_details_output_t,num_details),0,NULL,0x5d40f5fa,28},
{"details",offsetof(csm_diag_run_query_details_output_t,details),offsetof(csm_diag_run_query_details_output_t, num_details),&map_csmi_diag_run_query_details_result_t,0x982788b,1},
{"run_data",offsetof(csm_diag_run_query_details_output_t,run_data),0,&map_csmi_diag_run_t,0xe6e8e953,0}}
;

void* cast_csm_diag_run_query_details_output_t(void* ptr,size_t index, char isArray) { 
    csm_diag_run_query_details_output_t ** ptr_cast = *(csm_diag_run_query_details_output_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_diag_run_query_details_output_t= {
    3,
    csm_diag_run_query_details_output_tree,
    cast_csm_diag_run_query_details_output_t
};

