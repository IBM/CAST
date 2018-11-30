/*================================================================================
   
    csmi/src/wm/src/csmi_wm_internal.c

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
#include "csmi/src/wm/include/csmi_wm_internal.h"
#include "csmi/src/wm/include/csmi_wm_type_internal.h"
const csmi_struct_node_t csmi_allocation_history_tree[3] = {{"end_time",offsetof(csmi_allocation_history_t,end_time),0,NULL,0xb56ec18a,4},
{"archive_history_time",offsetof(csmi_allocation_history_t,archive_history_time),0,NULL,0x9e88b9e6,4},
{"exit_status",offsetof(csmi_allocation_history_t,exit_status),0,NULL,0xe8583582,36}}
;

void* cast_csmi_allocation_history_t(void* ptr,size_t index, char isArray) { 
    csmi_allocation_history_t ** ptr_cast = *(csmi_allocation_history_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_allocation_history_t= {
    3,
    csmi_allocation_history_tree,
    cast_csmi_allocation_history_t
};

const csmi_struct_node_t csmi_allocation_tree[44] = {{"ssd_min",offsetof(csmi_allocation_t,ssd_min),0,NULL,0xabb1b072,40},
{"user_id",offsetof(csmi_allocation_t,user_id),0,NULL,0x45c27210,36},
{"primary_job_id",offsetof(csmi_allocation_t,primary_job_id),0,NULL,0xcfd430cf,40},
{"shared",offsetof(csmi_allocation_t,shared),0,NULL,0x1bb15c9c,16},
{"allocation_id",offsetof(csmi_allocation_t,allocation_id),0,NULL,0x99d3da77,40},
{"secondary_job_id",offsetof(csmi_allocation_t,secondary_job_id),0,NULL,0xbc667133,36},
{"projected_memory",offsetof(csmi_allocation_t,projected_memory),0,NULL,0xe6057cfd,36},
{"launch_node_name",offsetof(csmi_allocation_t,launch_node_name),0,NULL,0xf7cc9c5,4},
{"num_gpus",offsetof(csmi_allocation_t,num_gpus),0,NULL,0x338e5253,36},
{"type",offsetof(csmi_allocation_t,type),csmi_allocation_type_t_MAX,&csmi_allocation_type_t_strs,0x7c9ebd07,8},
{"num_nodes",offsetof(csmi_allocation_t,num_nodes),0,NULL,0xa5d6722d,24},
{"time_limit",offsetof(csmi_allocation_t,time_limit),0,NULL,0xadbb7332,40},
{"user_flags",offsetof(csmi_allocation_t,user_flags),0,NULL,0xc4ddbbf0,4},
{"system_flags",offsetof(csmi_allocation_t,system_flags),0,NULL,0xd97b5e76,4},
{"num_processors",offsetof(csmi_allocation_t,num_processors),0,NULL,0xeac9b7c7,36},
{"job_submit_time",offsetof(csmi_allocation_t,job_submit_time),0,NULL,0xb996701,4},
{"state",offsetof(csmi_allocation_t,state),csmi_state_t_MAX,&csmi_state_t_strs,0x10614a06,8},
{"account",offsetof(csmi_allocation_t,account),0,NULL,0x1cbdb112,4},
{"ssd_file_system_name",offsetof(csmi_allocation_t,ssd_file_system_name),0,NULL,0x33b3dbb2,4},
{"begin_time",offsetof(csmi_allocation_t,begin_time),0,NULL,0x5f818b18,4},
{"user_script",offsetof(csmi_allocation_t,user_script),0,NULL,0x7e4ec898,4},
{"job_type",offsetof(csmi_allocation_t,job_type),csmi_job_type_t_MAX,&csmi_job_type_t_strs,0x9b0819e1,8},
{"ssd_max",offsetof(csmi_allocation_t,ssd_max),0,NULL,0xabb1af74,40},
{NULL,0,0,NULL,0,0},
{"user_group_id",offsetof(csmi_allocation_t,user_group_id),0,NULL,0xb690441c,36},
{"user_name",offsetof(csmi_allocation_t,user_name),0,NULL,0xc029f5a4,4},
{NULL,0,0,NULL,0,0},
{"comment",offsetof(csmi_allocation_t,comment),0,NULL,0xd363aa58,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"isolated_cores",offsetof(csmi_allocation_t,isolated_cores),0,NULL,0xfb061e75,36},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"queue",offsetof(csmi_allocation_t,queue),0,NULL,0x103db68a,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"wc_key",offsetof(csmi_allocation_t,wc_key),0,NULL,0x24aa4e27,4},
{NULL,0,0,NULL,0,0},
{"requeue",offsetof(csmi_allocation_t,requeue),0,NULL,0x3f066941,4},
{"history",offsetof(csmi_allocation_t,history),0,&map_csmi_allocation_history_t,0x46b87b17,0},
{"compute_nodes",offsetof(csmi_allocation_t,compute_nodes),offsetof(csmi_allocation_t, num_nodes),NULL,0x74676dda,5},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"job_name",offsetof(csmi_allocation_t,job_name),0,NULL,0x9b046920,4}}
;

void* cast_csmi_allocation_t(void* ptr,size_t index, char isArray) { 
    csmi_allocation_t ** ptr_cast = *(csmi_allocation_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_allocation_t= {
    44,
    csmi_allocation_tree,
    cast_csmi_allocation_t
};

const csmi_struct_node_t csmi_allocation_accounting_tree[16] = {{"gpu_usage",offsetof(csmi_allocation_accounting_t,gpu_usage),0,NULL,0x4178e945,40},
{"ib_tx",offsetof(csmi_allocation_accounting_t,ib_tx),0,NULL,0xfa26dbb,40},
{"gpfs_write",offsetof(csmi_allocation_accounting_t,gpfs_write),0,NULL,0x6947993f,40},
{"ib_rx",offsetof(csmi_allocation_accounting_t,ib_rx),0,NULL,0xfa26d79,40},
{"energy_consumed",offsetof(csmi_allocation_accounting_t,energy_consumed),0,NULL,0x2934342c,40},
{"power_shifting_ratio",offsetof(csmi_allocation_accounting_t,power_shifting_ratio),0,NULL,0x5af42f2b,36},
{"memory_usage_max",offsetof(csmi_allocation_accounting_t,memory_usage_max),0,NULL,0xd625f597,40},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"ssd_read",offsetof(csmi_allocation_accounting_t,ssd_read),0,NULL,0x21ea6a4a,40},
{"power_cap_hit",offsetof(csmi_allocation_accounting_t,power_cap_hit),0,NULL,0x315b4c49,40},
{"gpu_energy",offsetof(csmi_allocation_accounting_t,gpu_energy),0,NULL,0x4aeb6e5a,40},
{"ssd_write",offsetof(csmi_allocation_accounting_t,ssd_write),0,NULL,0x5f997379,40},
{"cpu_usage",offsetof(csmi_allocation_accounting_t,cpu_usage),0,NULL,0x6f872541,40},
{"gpfs_read",offsetof(csmi_allocation_accounting_t,gpfs_read),0,NULL,0xebe7ef50,40},
{"power_cap",offsetof(csmi_allocation_accounting_t,power_cap),0,NULL,0x15494165,36}}
;

void* cast_csmi_allocation_accounting_t(void* ptr,size_t index, char isArray) { 
    csmi_allocation_accounting_t ** ptr_cast = *(csmi_allocation_accounting_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_allocation_accounting_t= {
    16,
    csmi_allocation_accounting_tree,
    cast_csmi_allocation_accounting_t
};

const csmi_struct_node_t csmi_allocation_step_list_tree[4] = {{"step_id",offsetof(csmi_allocation_step_list_t,step_id),0,NULL,0xae22086d,40},
{"num_nodes",offsetof(csmi_allocation_step_list_t,num_nodes),0,NULL,0xa5d6722d,24},
{"end_time",offsetof(csmi_allocation_step_list_t,end_time),0,NULL,0xb56ec18a,4},
{"compute_nodes",offsetof(csmi_allocation_step_list_t,compute_nodes),0,NULL,0x74676dda,4}}
;

void* cast_csmi_allocation_step_list_t(void* ptr,size_t index, char isArray) { 
    csmi_allocation_step_list_t ** ptr_cast = *(csmi_allocation_step_list_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_allocation_step_list_t= {
    4,
    csmi_allocation_step_list_tree,
    cast_csmi_allocation_step_list_t
};

const csmi_struct_node_t csmi_allocation_state_history_tree[2] = {{"history_time",offsetof(csmi_allocation_state_history_t,history_time),0,NULL,0x60dc8265,4},
{"state",offsetof(csmi_allocation_state_history_t,state),csmi_state_t_MAX,&csmi_state_t_strs,0x10614a06,8}}
;

void* cast_csmi_allocation_state_history_t(void* ptr,size_t index, char isArray) { 
    csmi_allocation_state_history_t ** ptr_cast = *(csmi_allocation_state_history_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_allocation_state_history_t= {
    2,
    csmi_allocation_state_history_tree,
    cast_csmi_allocation_state_history_t
};

const csmi_struct_node_t csmi_allocation_details_tree[13] = {{"power_cap_hit",offsetof(csmi_allocation_details_t,power_cap_hit),0,NULL,0x315b4c49,28},
{"ssd_read",offsetof(csmi_allocation_details_t,ssd_read),0,NULL,0x21ea6a4a,28},
{"num_nodes",offsetof(csmi_allocation_details_t,num_nodes),0,NULL,0xa5d6722d,24},
{"steps",offsetof(csmi_allocation_details_t,steps),offsetof(csmi_allocation_details_t, num_steps),&map_csmi_allocation_step_list_t,0x10615a94,1},
{NULL,0,0,NULL,0,0},
{"ssd_write",offsetof(csmi_allocation_details_t,ssd_write),0,NULL,0x5f997379,28},
{"num_steps",offsetof(csmi_allocation_details_t,num_steps),0,NULL,0xa633b043,24},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"num_transitions",offsetof(csmi_allocation_details_t,num_transitions),0,NULL,0x3fe3fad2,24},
{"state_transitions",offsetof(csmi_allocation_details_t,state_transitions),offsetof(csmi_allocation_details_t, num_transitions),&map_csmi_allocation_state_history_t,0x9d16c003,1},
{NULL,0,0,NULL,0,0},
{"node_accounting",offsetof(csmi_allocation_details_t,node_accounting),offsetof(csmi_allocation_details_t, num_nodes),&map_csmi_allocation_accounting_t,0xac4406b5,1}}
;

void* cast_csmi_allocation_details_t(void* ptr,size_t index, char isArray) { 
    csmi_allocation_details_t ** ptr_cast = *(csmi_allocation_details_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_allocation_details_t= {
    13,
    csmi_allocation_details_tree,
    cast_csmi_allocation_details_t
};

const csmi_struct_node_t csmi_allocation_step_history_tree[21] = {{"omp_thread_limit",offsetof(csmi_allocation_step_history_t,omp_thread_limit),0,NULL,0x8884ccc6,4},
{"max_memory",offsetof(csmi_allocation_step_history_t,max_memory),0,NULL,0x66b074e3,40},
{"total_u_time",offsetof(csmi_allocation_step_history_t,total_u_time),0,NULL,0xa692ad0b,56},
{"gpu_stats",offsetof(csmi_allocation_step_history_t,gpu_stats),0,NULL,0x4155465f,4},
{"cpu_stats",offsetof(csmi_allocation_step_history_t,cpu_stats),0,NULL,0x6f63825b,4},
{"total_s_time",offsetof(csmi_allocation_step_history_t,total_s_time),0,NULL,0xa1e85bc9,56},
{"exit_status",offsetof(csmi_allocation_step_history_t,exit_status),0,NULL,0xe8583582,36},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"memory_stats",offsetof(csmi_allocation_step_history_t,memory_stats),0,NULL,0x85ddb50c,4},
{"archive_history_time",offsetof(csmi_allocation_step_history_t,archive_history_time),0,NULL,0x9e88b9e6,4},
{NULL,0,0,NULL,0,0},
{"io_stats",offsetof(csmi_allocation_step_history_t,io_stats),0,NULL,0xa9304fab,4},
{"error_message",offsetof(csmi_allocation_step_history_t,error_message),0,NULL,0xf41641f3,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"end_time",offsetof(csmi_allocation_step_history_t,end_time),0,NULL,0xb56ec18a,4}}
;

void* cast_csmi_allocation_step_history_t(void* ptr,size_t index, char isArray) { 
    csmi_allocation_step_history_t ** ptr_cast = *(csmi_allocation_step_history_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_allocation_step_history_t= {
    21,
    csmi_allocation_step_history_tree,
    cast_csmi_allocation_step_history_t
};

const csmi_struct_node_t csmi_allocation_step_tree[26] = {{"num_nodes",offsetof(csmi_allocation_step_t,num_nodes),0,NULL,0xa5d6722d,36},
{"begin_time",offsetof(csmi_allocation_step_t,begin_time),0,NULL,0x5f818b18,4},
{"projected_memory",offsetof(csmi_allocation_step_t,projected_memory),0,NULL,0xe6057cfd,36},
{"num_gpus",offsetof(csmi_allocation_step_t,num_gpus),0,NULL,0x338e5253,36},
{"executable",offsetof(csmi_allocation_step_t,executable),0,NULL,0x7c422127,4},
{"step_id",offsetof(csmi_allocation_step_t,step_id),0,NULL,0xae22086d,40},
{"num_processors",offsetof(csmi_allocation_step_t,num_processors),0,NULL,0xeac9b7c7,36},
{"status",offsetof(csmi_allocation_step_t,status),csmi_step_status_t_MAX,&csmi_step_status_t_strs,0x1c8a8d49,8},
{"argument",offsetof(csmi_allocation_step_t,argument),0,NULL,0x40da0e88,4},
{"environment_variable",offsetof(csmi_allocation_step_t,environment_variable),0,NULL,0x74e8fc3f,4},
{"allocation_id",offsetof(csmi_allocation_step_t,allocation_id),0,NULL,0x99d3da77,40},
{"num_tasks",offsetof(csmi_allocation_step_t,num_tasks),0,NULL,0xa63b987a,36},
{"working_directory",offsetof(csmi_allocation_step_t,working_directory),0,NULL,0xdfe1263a,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"history",offsetof(csmi_allocation_step_t,history),0,&map_csmi_allocation_step_history_t,0x46b87b17,0},
{"compute_nodes",offsetof(csmi_allocation_step_t,compute_nodes),offsetof(csmi_allocation_step_t, num_nodes),NULL,0x74676dda,5},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"user_flags",offsetof(csmi_allocation_step_t,user_flags),0,NULL,0xc4ddbbf0,4}}
;

void* cast_csmi_allocation_step_t(void* ptr,size_t index, char isArray) { 
    csmi_allocation_step_t ** ptr_cast = *(csmi_allocation_step_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_allocation_step_t= {
    26,
    csmi_allocation_step_tree,
    cast_csmi_allocation_step_t
};

const csmi_struct_node_t csmi_ssd_resources_record_tree[3] = {{"update_time",offsetof(csmi_ssd_resources_record_t,update_time),0,NULL,0x7ceafa96,4},
{"wear_lifespan_used",offsetof(csmi_ssd_resources_record_t,wear_lifespan_used),0,NULL,0x7bc95915,56},
{"serial_number",offsetof(csmi_ssd_resources_record_t,serial_number),0,NULL,0xd931f68d,4}}
;

void* cast_csmi_ssd_resources_record_t(void* ptr,size_t index, char isArray) { 
    csmi_ssd_resources_record_t ** ptr_cast = *(csmi_ssd_resources_record_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_ssd_resources_record_t= {
    3,
    csmi_ssd_resources_record_tree,
    cast_csmi_ssd_resources_record_t
};

const csmi_struct_node_t csmi_node_resources_record_tree[23] = {{"node_available_gpus",offsetof(csmi_node_resources_record_t,node_available_gpus),0,NULL,0x2fe53009,36},
{"ssds_count",offsetof(csmi_node_resources_record_t,ssds_count),0,NULL,0x1d4cb32a,24},
{"vg_available_size",offsetof(csmi_node_resources_record_t,vg_available_size),0,NULL,0xa5e09dbc,40},
{"node_state",offsetof(csmi_node_resources_record_t,node_state),csmi_node_state_t_MAX,&csmi_node_state_t_strs,0x29ab88b,8},
{"node_available_cores",offsetof(csmi_node_resources_record_t,node_available_cores),0,NULL,0x2c4234a6,36},
{"node_name",offsetof(csmi_node_resources_record_t,node_name),0,NULL,0x746e3e2b,4},
{"vg_total_size",offsetof(csmi_node_resources_record_t,vg_total_size),0,NULL,0xc464e35f,40},
{"node_ready",offsetof(csmi_node_resources_record_t,node_ready),0,NULL,0x280645f,16},
{"node_available_processors",offsetof(csmi_node_resources_record_t,node_available_processors),0,NULL,0xff985fd,36},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"node_installed_memory",offsetof(csmi_node_resources_record_t,node_installed_memory),0,NULL,0x53739482,40},
{"ssds",offsetof(csmi_node_resources_record_t,ssds),offsetof(csmi_node_resources_record_t, ssds_count),&map_csmi_ssd_resources_record_t,0x7c9e15a2,1},
{NULL,0,0,NULL,0,0},
{"node_update_time",offsetof(csmi_node_resources_record_t,node_update_time),0,NULL,0xfa0078db,4},
{"node_discovered_sockets",offsetof(csmi_node_resources_record_t,node_discovered_sockets),0,NULL,0xdf0e4d,36},
{NULL,0,0,NULL,0,0},
{"vg_update_time",offsetof(csmi_node_resources_record_t,vg_update_time),0,NULL,0x70e3692,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"node_discovered_gpus",offsetof(csmi_node_resources_record_t,node_discovered_gpus),0,NULL,0x5a2162f0,36},
{"node_type",offsetof(csmi_node_resources_record_t,node_type),csmi_node_type_t_MAX,&csmi_node_type_t_strs,0x7471eeec,8},
{"node_discovered_cores",offsetof(csmi_node_resources_record_t,node_discovered_cores),0,NULL,0x9e04c46d,36}}
;

void* cast_csmi_node_resources_record_t(void* ptr,size_t index, char isArray) { 
    csmi_node_resources_record_t ** ptr_cast = *(csmi_node_resources_record_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_node_resources_record_t= {
    23,
    csmi_node_resources_record_tree,
    cast_csmi_node_resources_record_t
};

const csmi_struct_node_t csmi_cgroup_tree[5] = {{"num_params",offsetof(csmi_cgroup_t,num_params),0,NULL,0x64594df8,24},
{"params",offsetof(csmi_cgroup_t,params),offsetof(csmi_cgroup_t, num_params),NULL,0x143c4269,5},
{"type",offsetof(csmi_cgroup_t,type),csmi_cgroup_controller_t_MAX,&csmi_cgroup_controller_t_strs,0x7c9ebd07,8},
{NULL,0,0,NULL,0,0},
{"values",offsetof(csmi_cgroup_t,values),offsetof(csmi_cgroup_t, num_params),NULL,0x22383ff5,5}}
;

void* cast_csmi_cgroup_t(void* ptr,size_t index, char isArray) { 
    csmi_cgroup_t ** ptr_cast = *(csmi_cgroup_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_cgroup_t= {
    5,
    csmi_cgroup_tree,
    cast_csmi_cgroup_t
};

const csmi_struct_node_t csmi_allocation_resources_record_tree[3] = {{"ready",offsetof(csmi_allocation_resources_record_t,ready),0,NULL,0x1046f5da,16},
{NULL,0,0,NULL,0,0},
{"node_name",offsetof(csmi_allocation_resources_record_t,node_name),0,NULL,0x746e3e2b,4}}
;

void* cast_csmi_allocation_resources_record_t(void* ptr,size_t index, char isArray) { 
    csmi_allocation_resources_record_t ** ptr_cast = *(csmi_allocation_resources_record_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_allocation_resources_record_t= {
    3,
    csmi_allocation_resources_record_tree,
    cast_csmi_allocation_resources_record_t
};


const csmi_struct_node_t csm_allocation_query_details_input_tree[1] = {{"allocation_id",offsetof(csm_allocation_query_details_input_t,allocation_id),0,NULL,0x99d3da77,40}}
;

void* cast_csm_allocation_query_details_input_t(void* ptr,size_t index, char isArray) { 
    csm_allocation_query_details_input_t ** ptr_cast = *(csm_allocation_query_details_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_allocation_query_details_input_t= {
    1,
    csm_allocation_query_details_input_tree,
    cast_csm_allocation_query_details_input_t
};

const csmi_struct_node_t csm_allocation_query_details_output_tree[2] = {{"allocation",offsetof(csm_allocation_query_details_output_t,allocation),0,&map_csmi_allocation_t,0xdc80184b,0},
{"allocation_details",offsetof(csm_allocation_query_details_output_t,allocation_details),0,&map_csmi_allocation_details_t,0xd9be0df0,0}}
;

void* cast_csm_allocation_query_details_output_t(void* ptr,size_t index, char isArray) { 
    csm_allocation_query_details_output_t ** ptr_cast = *(csm_allocation_query_details_output_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_allocation_query_details_output_t= {
    2,
    csm_allocation_query_details_output_tree,
    cast_csm_allocation_query_details_output_t
};

const csmi_struct_node_t csm_allocation_update_state_input_tree[3] = {{"new_state",offsetof(csm_allocation_update_state_input_t,new_state),csmi_state_t_MAX,&csmi_state_t_strs,0xe3dcb12f,8},
{"allocation_id",offsetof(csm_allocation_update_state_input_t,allocation_id),0,NULL,0x99d3da77,40},
{"exit_status",offsetof(csm_allocation_update_state_input_t,exit_status),0,NULL,0xe8583582,36}}
;

void* cast_csm_allocation_update_state_input_t(void* ptr,size_t index, char isArray) { 
    csm_allocation_update_state_input_t ** ptr_cast = *(csm_allocation_update_state_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_allocation_update_state_input_t= {
    3,
    csm_allocation_update_state_input_tree,
    cast_csm_allocation_update_state_input_t
};


const csmi_struct_node_t csm_allocation_step_end_input_tree[5] = {{"allocation_id",offsetof(csm_allocation_step_end_input_t,allocation_id),0,NULL,0x99d3da77,40},
{"status",offsetof(csm_allocation_step_end_input_t,status),csmi_step_status_t_MAX,&csmi_step_status_t_strs,0x1c8a8d49,8},
{"step_id",offsetof(csm_allocation_step_end_input_t,step_id),0,NULL,0xae22086d,40},
{NULL,0,0,NULL,0,0},
{"history",offsetof(csm_allocation_step_end_input_t,history),0,&map_csmi_allocation_step_history_t,0x46b87b17,0}}
;

void* cast_csm_allocation_step_end_input_t(void* ptr,size_t index, char isArray) { 
    csm_allocation_step_end_input_t ** ptr_cast = *(csm_allocation_step_end_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_allocation_step_end_input_t= {
    5,
    csm_allocation_step_end_input_tree,
    cast_csm_allocation_step_end_input_t
};

const csmi_struct_node_t csm_allocation_step_query_input_tree[5] = {{"allocation_id",offsetof(csm_allocation_step_query_input_t,allocation_id),0,NULL,0x99d3da77,40},
{"limit",offsetof(csm_allocation_step_query_input_t,limit),0,NULL,0xfdcc804,36},
{"step_id",offsetof(csm_allocation_step_query_input_t,step_id),0,NULL,0xae22086d,40},
{NULL,0,0,NULL,0,0},
{"offset",offsetof(csm_allocation_step_query_input_t,offset),0,NULL,0x123b4b4c,36}}
;

void* cast_csm_allocation_step_query_input_t(void* ptr,size_t index, char isArray) { 
    csm_allocation_step_query_input_t ** ptr_cast = *(csm_allocation_step_query_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_allocation_step_query_input_t= {
    5,
    csm_allocation_step_query_input_tree,
    cast_csm_allocation_step_query_input_t
};

const csmi_struct_node_t csm_allocation_step_query_output_tree[2] = {{"num_steps",offsetof(csm_allocation_step_query_output_t,num_steps),0,NULL,0xa633b043,36},
{"steps",offsetof(csm_allocation_step_query_output_t,steps),offsetof(csm_allocation_step_query_output_t, num_steps),&map_csmi_allocation_step_t,0x10615a94,1}}
;

void* cast_csm_allocation_step_query_output_t(void* ptr,size_t index, char isArray) { 
    csm_allocation_step_query_output_t ** ptr_cast = *(csm_allocation_step_query_output_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_allocation_step_query_output_t= {
    2,
    csm_allocation_step_query_output_tree,
    cast_csm_allocation_step_query_output_t
};

const csmi_struct_node_t csm_allocation_step_query_details_input_tree[5] = {{"allocation_id",offsetof(csm_allocation_step_query_details_input_t,allocation_id),0,NULL,0x99d3da77,40},
{"limit",offsetof(csm_allocation_step_query_details_input_t,limit),0,NULL,0xfdcc804,36},
{"step_id",offsetof(csm_allocation_step_query_details_input_t,step_id),0,NULL,0xae22086d,40},
{NULL,0,0,NULL,0,0},
{"offset",offsetof(csm_allocation_step_query_details_input_t,offset),0,NULL,0x123b4b4c,36}}
;

void* cast_csm_allocation_step_query_details_input_t(void* ptr,size_t index, char isArray) { 
    csm_allocation_step_query_details_input_t ** ptr_cast = *(csm_allocation_step_query_details_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_allocation_step_query_details_input_t= {
    5,
    csm_allocation_step_query_details_input_tree,
    cast_csm_allocation_step_query_details_input_t
};


const csmi_struct_node_t csm_allocation_step_query_active_all_input_tree[3] = {{"offset",offsetof(csm_allocation_step_query_active_all_input_t,offset),0,NULL,0x123b4b4c,36},
{"limit",offsetof(csm_allocation_step_query_active_all_input_t,limit),0,NULL,0xfdcc804,36},
{"allocation_id",offsetof(csm_allocation_step_query_active_all_input_t,allocation_id),0,NULL,0x99d3da77,40}}
;

void* cast_csm_allocation_step_query_active_all_input_t(void* ptr,size_t index, char isArray) { 
    csm_allocation_step_query_active_all_input_t ** ptr_cast = *(csm_allocation_step_query_active_all_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_allocation_step_query_active_all_input_t= {
    3,
    csm_allocation_step_query_active_all_input_tree,
    cast_csm_allocation_step_query_active_all_input_t
};


const csmi_struct_node_t csm_node_resources_query_input_tree[4] = {{"offset",offsetof(csm_node_resources_query_input_t,offset),0,NULL,0x123b4b4c,36},
{"limit",offsetof(csm_node_resources_query_input_t,limit),0,NULL,0xfdcc804,36},
{"node_names_count",offsetof(csm_node_resources_query_input_t,node_names_count),0,NULL,0x868cf686,24},
{"node_names",offsetof(csm_node_resources_query_input_t,node_names),offsetof(csm_node_resources_query_input_t, node_names_count),NULL,0x23603fe,5}}
;

void* cast_csm_node_resources_query_input_t(void* ptr,size_t index, char isArray) { 
    csm_node_resources_query_input_t ** ptr_cast = *(csm_node_resources_query_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_node_resources_query_input_t= {
    4,
    csm_node_resources_query_input_tree,
    cast_csm_node_resources_query_input_t
};

const csmi_struct_node_t csm_node_resources_query_output_tree[2] = {{"results_count",offsetof(csm_node_resources_query_output_t,results_count),0,NULL,0x8261013f,24},
{"results",offsetof(csm_node_resources_query_output_t,results),offsetof(csm_node_resources_query_output_t, results_count),&map_csmi_node_resources_record_t,0x3f2ab7f7,1}}
;

void* cast_csm_node_resources_query_output_t(void* ptr,size_t index, char isArray) { 
    csm_node_resources_query_output_t ** ptr_cast = *(csm_node_resources_query_output_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_node_resources_query_output_t= {
    2,
    csm_node_resources_query_output_tree,
    cast_csm_node_resources_query_output_t
};

const csmi_struct_node_t csm_node_resources_query_all_input_tree[3] = {{"limit",offsetof(csm_node_resources_query_all_input_t,limit),0,NULL,0xfdcc804,36},
{NULL,0,0,NULL,0,0},
{"offset",offsetof(csm_node_resources_query_all_input_t,offset),0,NULL,0x123b4b4c,36}}
;

void* cast_csm_node_resources_query_all_input_t(void* ptr,size_t index, char isArray) { 
    csm_node_resources_query_all_input_t ** ptr_cast = *(csm_node_resources_query_all_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_node_resources_query_all_input_t= {
    3,
    csm_node_resources_query_all_input_tree,
    cast_csm_node_resources_query_all_input_t
};

const csmi_struct_node_t csm_node_resources_query_all_output_tree[2] = {{"results_count",offsetof(csm_node_resources_query_all_output_t,results_count),0,NULL,0x8261013f,24},
{"results",offsetof(csm_node_resources_query_all_output_t,results),offsetof(csm_node_resources_query_all_output_t, results_count),&map_csmi_node_resources_record_t,0x3f2ab7f7,1}}
;

void* cast_csm_node_resources_query_all_output_t(void* ptr,size_t index, char isArray) { 
    csm_node_resources_query_all_output_t ** ptr_cast = *(csm_node_resources_query_all_output_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_node_resources_query_all_output_t= {
    2,
    csm_node_resources_query_all_output_tree,
    cast_csm_node_resources_query_all_output_t
};

const csmi_struct_node_t csm_allocation_step_cgroup_create_input_tree[7] = {{"num_components",offsetof(csm_allocation_step_cgroup_create_input_t,num_components),0,NULL,0x7feddc9a,24},
{"pid",offsetof(csm_allocation_step_cgroup_create_input_t,pid),0,NULL,0xb889d42,64},
{"allocation_id",offsetof(csm_allocation_step_cgroup_create_input_t,allocation_id),0,NULL,0x99d3da77,40},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"components",offsetof(csm_allocation_step_cgroup_create_input_t,components),offsetof(csm_allocation_step_cgroup_create_input_t, num_components),&map_csmi_cgroup_t,0x9827a18b,1},
{"cgroup_name",offsetof(csm_allocation_step_cgroup_create_input_t,cgroup_name),0,NULL,0xa90e8395,4}}
;

void* cast_csm_allocation_step_cgroup_create_input_t(void* ptr,size_t index, char isArray) { 
    csm_allocation_step_cgroup_create_input_t ** ptr_cast = *(csm_allocation_step_cgroup_create_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_allocation_step_cgroup_create_input_t= {
    7,
    csm_allocation_step_cgroup_create_input_tree,
    cast_csm_allocation_step_cgroup_create_input_t
};

const csmi_struct_node_t csm_allocation_step_cgroup_delete_input_tree[4] = {{"num_types",offsetof(csm_allocation_step_cgroup_delete_input_t,num_types),0,NULL,0xa648b409,24},
{"allocation_id",offsetof(csm_allocation_step_cgroup_delete_input_t,allocation_id),0,NULL,0x99d3da77,40},
{"cgroup_name",offsetof(csm_allocation_step_cgroup_delete_input_t,cgroup_name),0,NULL,0xa90e8395,4},
{"controller_types",offsetof(csm_allocation_step_cgroup_delete_input_t,controller_types),offsetof(csm_allocation_step_cgroup_delete_input_t, num_types),NULL,0x8afb5cbd,1}}
;

void* cast_csm_allocation_step_cgroup_delete_input_t(void* ptr,size_t index, char isArray) { 
    csm_allocation_step_cgroup_delete_input_t ** ptr_cast = *(csm_allocation_step_cgroup_delete_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_allocation_step_cgroup_delete_input_t= {
    4,
    csm_allocation_step_cgroup_delete_input_tree,
    cast_csm_allocation_step_cgroup_delete_input_t
};

const csmi_struct_node_t csm_allocation_resources_query_input_tree[3] = {{"offset",offsetof(csm_allocation_resources_query_input_t,offset),0,NULL,0x123b4b4c,36},
{"limit",offsetof(csm_allocation_resources_query_input_t,limit),0,NULL,0xfdcc804,36},
{"allocation_id",offsetof(csm_allocation_resources_query_input_t,allocation_id),0,NULL,0x99d3da77,40}}
;

void* cast_csm_allocation_resources_query_input_t(void* ptr,size_t index, char isArray) { 
    csm_allocation_resources_query_input_t ** ptr_cast = *(csm_allocation_resources_query_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_allocation_resources_query_input_t= {
    3,
    csm_allocation_resources_query_input_tree,
    cast_csm_allocation_resources_query_input_t
};

const csmi_struct_node_t csm_allocation_resources_query_output_tree[2] = {{"results_count",offsetof(csm_allocation_resources_query_output_t,results_count),0,NULL,0x8261013f,24},
{"results",offsetof(csm_allocation_resources_query_output_t,results),offsetof(csm_allocation_resources_query_output_t, results_count),&map_csmi_allocation_resources_record_t,0x3f2ab7f7,1}}
;

void* cast_csm_allocation_resources_query_output_t(void* ptr,size_t index, char isArray) { 
    csm_allocation_resources_query_output_t ** ptr_cast = *(csm_allocation_resources_query_output_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_allocation_resources_query_output_t= {
    2,
    csm_allocation_resources_query_output_tree,
    cast_csm_allocation_resources_query_output_t
};

const csmi_struct_node_t csm_allocation_update_history_input_tree[12] = {{"allocation_id",offsetof(csm_allocation_update_history_input_t,allocation_id),0,NULL,0x99d3da77,40},
{"user_id",offsetof(csm_allocation_update_history_input_t,user_id),0,NULL,0x45c27210,36},
{"user_name",offsetof(csm_allocation_update_history_input_t,user_name),0,NULL,0xc029f5a4,4},
{"account",offsetof(csm_allocation_update_history_input_t,account),0,NULL,0x1cbdb112,4},
{NULL,0,0,NULL,0,0},
{"user_group_id",offsetof(csm_allocation_update_history_input_t,user_group_id),0,NULL,0xb690441c,36},
{"comment",offsetof(csm_allocation_update_history_input_t,comment),0,NULL,0xd363aa58,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"job_name",offsetof(csm_allocation_update_history_input_t,job_name),0,NULL,0x9b046920,4},
{NULL,0,0,NULL,0,0},
{"reservation",offsetof(csm_allocation_update_history_input_t,reservation),0,NULL,0xd289fb77,4}}
;

void* cast_csm_allocation_update_history_input_t(void* ptr,size_t index, char isArray) { 
    csm_allocation_update_history_input_t ** ptr_cast = *(csm_allocation_update_history_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_allocation_update_history_input_t= {
    12,
    csm_allocation_update_history_input_tree,
    cast_csm_allocation_update_history_input_t
};

const csmi_struct_node_t csm_allocation_query_input_tree[3] = {{"secondary_job_id",offsetof(csm_allocation_query_input_t,secondary_job_id),0,NULL,0xbc667133,36},
{"allocation_id",offsetof(csm_allocation_query_input_t,allocation_id),0,NULL,0x99d3da77,40},
{"primary_job_id",offsetof(csm_allocation_query_input_t,primary_job_id),0,NULL,0xcfd430cf,40}}
;

void* cast_csm_allocation_query_input_t(void* ptr,size_t index, char isArray) { 
    csm_allocation_query_input_t ** ptr_cast = *(csm_allocation_query_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_allocation_query_input_t= {
    3,
    csm_allocation_query_input_tree,
    cast_csm_allocation_query_input_t
};

const csmi_struct_node_t csm_allocation_query_output_tree[1] = {{"allocation",offsetof(csm_allocation_query_output_t,allocation),0,&map_csmi_allocation_t,0xdc80184b,0}}
;

void* cast_csm_allocation_query_output_t(void* ptr,size_t index, char isArray) { 
    csm_allocation_query_output_t ** ptr_cast = *(csm_allocation_query_output_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_allocation_query_output_t= {
    1,
    csm_allocation_query_output_tree,
    cast_csm_allocation_query_output_t
};

const csmi_struct_node_t csm_allocation_query_active_all_input_tree[3] = {{"limit",offsetof(csm_allocation_query_active_all_input_t,limit),0,NULL,0xfdcc804,36},
{NULL,0,0,NULL,0,0},
{"offset",offsetof(csm_allocation_query_active_all_input_t,offset),0,NULL,0x123b4b4c,36}}
;

void* cast_csm_allocation_query_active_all_input_t(void* ptr,size_t index, char isArray) { 
    csm_allocation_query_active_all_input_t ** ptr_cast = *(csm_allocation_query_active_all_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_allocation_query_active_all_input_t= {
    3,
    csm_allocation_query_active_all_input_tree,
    cast_csm_allocation_query_active_all_input_t
};

const csmi_struct_node_t csm_allocation_query_active_all_output_tree[3] = {{"num_allocations",offsetof(csm_allocation_query_active_all_output_t,num_allocations),0,NULL,0x4d10bf0d,24},
{NULL,0,0,NULL,0,0},
{"allocations",offsetof(csm_allocation_query_active_all_output_t,allocations),offsetof(csm_allocation_query_active_all_output_t, num_allocations),&map_csmi_allocation_t,0x6c83221e,1}}
;

void* cast_csm_allocation_query_active_all_output_t(void* ptr,size_t index, char isArray) { 
    csm_allocation_query_active_all_output_t ** ptr_cast = *(csm_allocation_query_active_all_output_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_allocation_query_active_all_output_t= {
    3,
    csm_allocation_query_active_all_output_tree,
    cast_csm_allocation_query_active_all_output_t
};

const csmi_struct_node_t csm_allocation_delete_input_tree[5] = {{"primary_job_id",offsetof(csm_allocation_delete_input_t,primary_job_id),0,NULL,0xcfd430cf,40},
{"allocation_id",offsetof(csm_allocation_delete_input_t,allocation_id),0,NULL,0x99d3da77,40},
{"exit_status",offsetof(csm_allocation_delete_input_t,exit_status),0,NULL,0xe8583582,36},
{NULL,0,0,NULL,0,0},
{"secondary_job_id",offsetof(csm_allocation_delete_input_t,secondary_job_id),0,NULL,0xbc667133,36}}
;

void* cast_csm_allocation_delete_input_t(void* ptr,size_t index, char isArray) { 
    csm_allocation_delete_input_t ** ptr_cast = *(csm_allocation_delete_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_allocation_delete_input_t= {
    5,
    csm_allocation_delete_input_tree,
    cast_csm_allocation_delete_input_t
};

const csmi_struct_node_t csm_cgroup_login_input_tree[6] = {{"allocation_id",offsetof(csm_cgroup_login_input_t,allocation_id),0,NULL,0x99d3da77,40},
{"pid",offsetof(csm_cgroup_login_input_t,pid),0,NULL,0xb889d42,64},
{"user_name",offsetof(csm_cgroup_login_input_t,user_name),0,NULL,0xc029f5a4,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"migrate_pid",offsetof(csm_cgroup_login_input_t,migrate_pid),0,NULL,0xb05bda0a,68}}
;

void* cast_csm_cgroup_login_input_t(void* ptr,size_t index, char isArray) { 
    csm_cgroup_login_input_t ** ptr_cast = *(csm_cgroup_login_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_cgroup_login_input_t= {
    6,
    csm_cgroup_login_input_tree,
    cast_csm_cgroup_login_input_t
};

const csmi_struct_node_t csmi_allocation_gpu_metrics_tree[7] = {{"num_gpus",offsetof(csmi_allocation_gpu_metrics_t,num_gpus),0,NULL,0x338e5253,40},
{"gpu_id",offsetof(csmi_allocation_gpu_metrics_t,gpu_id),offsetof(csmi_allocation_gpu_metrics_t, num_gpus),NULL,0x4ee05d,1},
{"cpu_usage",offsetof(csmi_allocation_gpu_metrics_t,cpu_usage),offsetof(csmi_allocation_gpu_metrics_t, num_cpus),NULL,0x6f872541,1},
{NULL,0,0,NULL,0,0},
{"num_cpus",offsetof(csmi_allocation_gpu_metrics_t,num_cpus),0,NULL,0x338c20cf,40},
{"gpu_usage",offsetof(csmi_allocation_gpu_metrics_t,gpu_usage),offsetof(csmi_allocation_gpu_metrics_t, num_gpus),NULL,0x4178e945,1},
{"max_gpu_memory",offsetof(csmi_allocation_gpu_metrics_t,max_gpu_memory),offsetof(csmi_allocation_gpu_metrics_t, num_gpus),NULL,0xf084750e,1}}
;

void* cast_csmi_allocation_gpu_metrics_t(void* ptr,size_t index, char isArray) { 
    csmi_allocation_gpu_metrics_t ** ptr_cast = *(csmi_allocation_gpu_metrics_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_allocation_gpu_metrics_t= {
    7,
    csmi_allocation_gpu_metrics_tree,
    cast_csmi_allocation_gpu_metrics_t
};

const csmi_struct_node_t csmi_allocation_mcast_context_tree[47] = {{"allocation_id",offsetof(csmi_allocation_mcast_context_t,allocation_id),0,NULL,0x99d3da77,40},
{"num_gpus",offsetof(csmi_allocation_mcast_context_t,num_gpus),0,NULL,0x338e5253,36},
{"num_processors",offsetof(csmi_allocation_mcast_context_t,num_processors),0,NULL,0xeac9b7c7,36},
{"ib_tx",offsetof(csmi_allocation_mcast_context_t,ib_tx),offsetof(csmi_allocation_mcast_context_t, num_nodes),NULL,0xfa26dbb,1},
{"gpfs_write",offsetof(csmi_allocation_mcast_context_t,gpfs_write),offsetof(csmi_allocation_mcast_context_t, num_nodes),NULL,0x6947993f,1},
{"primary_job_id",offsetof(csmi_allocation_mcast_context_t,primary_job_id),0,NULL,0xcfd430cf,40},
{"isolated_cores",offsetof(csmi_allocation_mcast_context_t,isolated_cores),0,NULL,0xfb061e75,36},
{"ib_rx",offsetof(csmi_allocation_mcast_context_t,ib_rx),offsetof(csmi_allocation_mcast_context_t, num_nodes),NULL,0xfa26d79,1},
{"power_cap",offsetof(csmi_allocation_mcast_context_t,power_cap),offsetof(csmi_allocation_mcast_context_t, num_nodes),NULL,0x15494165,1},
{"gpu_energy",offsetof(csmi_allocation_mcast_context_t,gpu_energy),offsetof(csmi_allocation_mcast_context_t, num_nodes),NULL,0x4aeb6e5a,1},
{"save_allocation",offsetof(csmi_allocation_mcast_context_t,save_allocation),0,NULL,0x6f9d0af9,68},
{"secondary_job_id",offsetof(csmi_allocation_mcast_context_t,secondary_job_id),0,NULL,0xbc667133,36},
{"projected_memory",offsetof(csmi_allocation_mcast_context_t,projected_memory),0,NULL,0xe6057cfd,36},
{"gpfs_read",offsetof(csmi_allocation_mcast_context_t,gpfs_read),offsetof(csmi_allocation_mcast_context_t, num_nodes),NULL,0xebe7ef50,1},
{"energy",offsetof(csmi_allocation_mcast_context_t,energy),offsetof(csmi_allocation_mcast_context_t, num_nodes),NULL,0xfb77e8af,1},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"state",offsetof(csmi_allocation_mcast_context_t,state),csmi_state_t_MAX,&csmi_state_t_strs,0x10614a06,8},
{"power_cap_hit",offsetof(csmi_allocation_mcast_context_t,power_cap_hit),offsetof(csmi_allocation_mcast_context_t, num_nodes),NULL,0x315b4c49,1},
{"gpu_usage",offsetof(csmi_allocation_mcast_context_t,gpu_usage),offsetof(csmi_allocation_mcast_context_t, num_nodes),NULL,0x4178e945,1},
{"ps_ratio",offsetof(csmi_allocation_mcast_context_t,ps_ratio),offsetof(csmi_allocation_mcast_context_t, num_nodes),NULL,0x52c9e086,1},
{"cpu_usage",offsetof(csmi_allocation_mcast_context_t,cpu_usage),offsetof(csmi_allocation_mcast_context_t, num_nodes),NULL,0x6f872541,1},
{"type",offsetof(csmi_allocation_mcast_context_t,type),csmi_allocation_type_t_MAX,&csmi_allocation_type_t_strs,0x7c9ebd07,8},
{"num_nodes",offsetof(csmi_allocation_mcast_context_t,num_nodes),0,NULL,0xa5d6722d,24},
{"user_flags",offsetof(csmi_allocation_mcast_context_t,user_flags),0,NULL,0xc4ddbbf0,4},
{"system_flags",offsetof(csmi_allocation_mcast_context_t,system_flags),0,NULL,0xd97b5e76,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"memory_max",offsetof(csmi_allocation_mcast_context_t,memory_max),offsetof(csmi_allocation_mcast_context_t, num_nodes),NULL,0xee7ddc83,1},
{NULL,0,0,NULL,0,0},
{"gpu_metrics",offsetof(csmi_allocation_mcast_context_t,gpu_metrics),offsetof(csmi_allocation_mcast_context_t, num_nodes),&map_csmi_allocation_gpu_metrics_t,0xfc3c27a7,1},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"shared",offsetof(csmi_allocation_mcast_context_t,shared),0,NULL,0x1bb15c9c,16},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"timestamp",offsetof(csmi_allocation_mcast_context_t,timestamp),0,NULL,0x5c073e19,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"compute_nodes",offsetof(csmi_allocation_mcast_context_t,compute_nodes),offsetof(csmi_allocation_mcast_context_t, num_nodes),NULL,0x74676dda,5},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"user_name",offsetof(csmi_allocation_mcast_context_t,user_name),0,NULL,0xc029f5a4,4},
{"start_state",offsetof(csmi_allocation_mcast_context_t,start_state),csmi_state_t_MAX,&csmi_state_t_strs,0xc8b079b3,8}}
;

void* cast_csmi_allocation_mcast_context_t(void* ptr,size_t index, char isArray) { 
    csmi_allocation_mcast_context_t ** ptr_cast = *(csmi_allocation_mcast_context_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_allocation_mcast_context_t= {
    47,
    csmi_allocation_mcast_context_tree,
    cast_csmi_allocation_mcast_context_t
};

const csmi_struct_node_t csmi_allocation_mcast_payload_request_tree[14] = {{"primary_job_id",offsetof(csmi_allocation_mcast_payload_request_t,primary_job_id),0,NULL,0xcfd430cf,40},
{"secondary_job_id",offsetof(csmi_allocation_mcast_payload_request_t,secondary_job_id),0,NULL,0xbc667133,36},
{"num_processors",offsetof(csmi_allocation_mcast_payload_request_t,num_processors),0,NULL,0xeac9b7c7,36},
{"num_gpus",offsetof(csmi_allocation_mcast_payload_request_t,num_gpus),0,NULL,0x338e5253,36},
{"user_name",offsetof(csmi_allocation_mcast_payload_request_t,user_name),0,NULL,0xc029f5a4,4},
{"projected_memory",offsetof(csmi_allocation_mcast_payload_request_t,projected_memory),0,NULL,0xe6057cfd,36},
{"isolated_cores",offsetof(csmi_allocation_mcast_payload_request_t,isolated_cores),0,NULL,0xfb061e75,36},
{"shared",offsetof(csmi_allocation_mcast_payload_request_t,shared),0,NULL,0x1bb15c9c,16},
{"allocation_id",offsetof(csmi_allocation_mcast_payload_request_t,allocation_id),0,NULL,0x99d3da77,40},
{NULL,0,0,NULL,0,0},
{"user_flags",offsetof(csmi_allocation_mcast_payload_request_t,user_flags),0,NULL,0xc4ddbbf0,4},
{"system_flags",offsetof(csmi_allocation_mcast_payload_request_t,system_flags),0,NULL,0xd97b5e76,4},
{NULL,0,0,NULL,0,0},
{"create",offsetof(csmi_allocation_mcast_payload_request_t,create),0,NULL,0xf715b2b9,68}}
;

void* cast_csmi_allocation_mcast_payload_request_t(void* ptr,size_t index, char isArray) { 
    csmi_allocation_mcast_payload_request_t ** ptr_cast = *(csmi_allocation_mcast_payload_request_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_allocation_mcast_payload_request_t= {
    14,
    csmi_allocation_mcast_payload_request_tree,
    cast_csmi_allocation_mcast_payload_request_t
};

const csmi_struct_node_t csmi_allocation_mcast_payload_response_tree[25] = {{"gpu_usage",offsetof(csmi_allocation_mcast_payload_response_t,gpu_usage),0,NULL,0x4178e945,40},
{"ib_tx",offsetof(csmi_allocation_mcast_payload_response_t,ib_tx),0,NULL,0xfa26dbb,40},
{"gpfs_read",offsetof(csmi_allocation_mcast_payload_response_t,gpfs_read),0,NULL,0xebe7ef50,40},
{"ib_rx",offsetof(csmi_allocation_mcast_payload_response_t,ib_rx),0,NULL,0xfa26d79,40},
{"pc_hit",offsetof(csmi_allocation_mcast_payload_response_t,pc_hit),0,NULL,0x1456257c,40},
{"gpfs_write",offsetof(csmi_allocation_mcast_payload_response_t,gpfs_write),0,NULL,0x6947993f,40},
{"create",offsetof(csmi_allocation_mcast_payload_response_t,create),0,NULL,0xf715b2b9,68},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"power_cap",offsetof(csmi_allocation_mcast_payload_response_t,power_cap),0,NULL,0x15494165,36},
{"ps_ratio",offsetof(csmi_allocation_mcast_payload_response_t,ps_ratio),0,NULL,0x52c9e086,36},
{"cpu_usage",offsetof(csmi_allocation_mcast_payload_response_t,cpu_usage),0,NULL,0x6f872541,40},
{"memory_max",offsetof(csmi_allocation_mcast_payload_response_t,memory_max),0,NULL,0xee7ddc83,40},
{"energy",offsetof(csmi_allocation_mcast_payload_response_t,energy),0,NULL,0xfb77e8af,40},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"gpu_energy",offsetof(csmi_allocation_mcast_payload_response_t,gpu_energy),0,NULL,0x4aeb6e5a,40},
{"error_code",offsetof(csmi_allocation_mcast_payload_response_t,error_code),csmi_cmd_err_t_MAX,&csmi_cmd_err_t_strs,0x53eff629,8},
{NULL,0,0,NULL,0,0},
{"hostname",offsetof(csmi_allocation_mcast_payload_response_t,hostname),0,NULL,0xeba474a4,4},
{NULL,0,0,NULL,0,0},
{"error_message",offsetof(csmi_allocation_mcast_payload_response_t,error_message),0,NULL,0xf41641f3,4},
{NULL,0,0,NULL,0,0},
{"gpu_metrics",offsetof(csmi_allocation_mcast_payload_response_t,gpu_metrics),0,&map_csmi_allocation_gpu_metrics_t,0xfc3c27a7,0}}
;

void* cast_csmi_allocation_mcast_payload_response_t(void* ptr,size_t index, char isArray) { 
    csmi_allocation_mcast_payload_response_t ** ptr_cast = *(csmi_allocation_mcast_payload_response_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_allocation_mcast_payload_response_t= {
    25,
    csmi_allocation_mcast_payload_response_tree,
    cast_csmi_allocation_mcast_payload_response_t
};

const csmi_struct_node_t csmi_allocation_step_mcast_context_tree[7] = {{"num_nodes",offsetof(csmi_allocation_step_mcast_context_t,num_nodes),0,NULL,0xa5d6722d,24},
{"compute_nodes",offsetof(csmi_allocation_step_mcast_context_t,compute_nodes),offsetof(csmi_allocation_step_mcast_context_t, num_nodes),NULL,0x74676dda,5},
{"user_flags",offsetof(csmi_allocation_step_mcast_context_t,user_flags),0,NULL,0xc4ddbbf0,4},
{"begin",offsetof(csmi_allocation_step_mcast_context_t,begin),0,NULL,0xf2587ea,68},
{"allocation_id",offsetof(csmi_allocation_step_mcast_context_t,allocation_id),0,NULL,0x99d3da77,40},
{"step_id",offsetof(csmi_allocation_step_mcast_context_t,step_id),0,NULL,0xae22086d,40},
{"json_str",offsetof(csmi_allocation_step_mcast_context_t,json_str),0,NULL,0xee47efb7,4}}
;

void* cast_csmi_allocation_step_mcast_context_t(void* ptr,size_t index, char isArray) { 
    csmi_allocation_step_mcast_context_t ** ptr_cast = *(csmi_allocation_step_mcast_context_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_allocation_step_mcast_context_t= {
    7,
    csmi_allocation_step_mcast_context_tree,
    cast_csmi_allocation_step_mcast_context_t
};

const csmi_struct_node_t csmi_allocation_step_mcast_payload_tree[7] = {{"allocation_id",offsetof(csmi_allocation_step_mcast_payload_t,allocation_id),0,NULL,0x99d3da77,40},
{"begin",offsetof(csmi_allocation_step_mcast_payload_t,begin),0,NULL,0xf2587ea,68},
{"user_flags",offsetof(csmi_allocation_step_mcast_payload_t,user_flags),0,NULL,0xc4ddbbf0,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"step_id",offsetof(csmi_allocation_step_mcast_payload_t,step_id),0,NULL,0xae22086d,40},
{"hostname",offsetof(csmi_allocation_step_mcast_payload_t,hostname),0,NULL,0xeba474a4,4}}
;

void* cast_csmi_allocation_step_mcast_payload_t(void* ptr,size_t index, char isArray) { 
    csmi_allocation_step_mcast_payload_t ** ptr_cast = *(csmi_allocation_step_mcast_payload_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_allocation_step_mcast_payload_t= {
    7,
    csmi_allocation_step_mcast_payload_tree,
    cast_csmi_allocation_step_mcast_payload_t
};

const csmi_struct_node_t csmi_jsrun_cmd_payload_tree[7] = {{"allocation_id",offsetof(csmi_jsrun_cmd_payload_t,allocation_id),0,NULL,0x99d3da77,40},
{"user_id",offsetof(csmi_jsrun_cmd_payload_t,user_id),0,NULL,0x45c27210,24},
{"jsm_path",offsetof(csmi_jsrun_cmd_payload_t,jsm_path),0,NULL,0xe89734bb,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"kv_pairs",offsetof(csmi_jsrun_cmd_payload_t,kv_pairs),0,NULL,0x9c4b0fc4,4},
{"hostname",offsetof(csmi_jsrun_cmd_payload_t,hostname),0,NULL,0xeba474a4,4}}
;

void* cast_csmi_jsrun_cmd_payload_t(void* ptr,size_t index, char isArray) { 
    csmi_jsrun_cmd_payload_t ** ptr_cast = *(csmi_jsrun_cmd_payload_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_jsrun_cmd_payload_t= {
    7,
    csmi_jsrun_cmd_payload_tree,
    cast_csmi_jsrun_cmd_payload_t
};

const csmi_struct_node_t csm_jsrun_cmd_input_tree[3] = {{"kv_pairs",offsetof(csm_jsrun_cmd_input_t,kv_pairs),0,NULL,0x9c4b0fc4,4},
{"allocation_id",offsetof(csm_jsrun_cmd_input_t,allocation_id),0,NULL,0x99d3da77,40},
{"jsm_path",offsetof(csm_jsrun_cmd_input_t,jsm_path),0,NULL,0xe89734bb,4}}
;

void* cast_csm_jsrun_cmd_input_t(void* ptr,size_t index, char isArray) { 
    csm_jsrun_cmd_input_t ** ptr_cast = *(csm_jsrun_cmd_input_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_jsrun_cmd_input_t= {
    3,
    csm_jsrun_cmd_input_tree,
    cast_csm_jsrun_cmd_input_t
};

