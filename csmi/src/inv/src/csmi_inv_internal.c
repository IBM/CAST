/*================================================================================
   
    csmi/src/inv/src/csmi_inv_internal.c

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
#include "csmi/src/inv/include/csmi_inv_internal.h"
#include "csmi/src/inv/include/csmi_inv_type_internal.h"
const csmi_struct_node_t csmi_dimm_record_tree[6] = {{"node_name",offsetof(csmi_dimm_record_t,node_name),0,NULL,0x746e3e2b,4},
{"physical_location",offsetof(csmi_dimm_record_t,physical_location),0,NULL,0x63efcf7a,4},
{"serial_number",offsetof(csmi_dimm_record_t,serial_number),0,NULL,0xd931f68d,4},
{"status",offsetof(csmi_dimm_record_t,status),0,NULL,0x1c8a8d49,68},
{NULL,0,0,NULL,0,0},
{"size",offsetof(csmi_dimm_record_t,size),0,NULL,0x7c9dede0,36}}
;

void* cast_csmi_dimm_record_t(void* ptr,size_t index) { 
    csmi_dimm_record_t ** ptr_cast = *(csmi_dimm_record_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csmi_dimm_record_t= {
    6,
    csmi_dimm_record_tree,
    cast_csmi_dimm_record_t
};

const csmi_struct_node_t csmi_gpu_record_tree[13] = {{"node_name",offsetof(csmi_gpu_record_t,node_name),0,NULL,0x746e3e2b,4},
{"status",offsetof(csmi_gpu_record_t,status),0,NULL,0x1c8a8d49,4},
{"inforom_image_version",offsetof(csmi_gpu_record_t,inforom_image_version),0,NULL,0xda562dc6,4},
{"gpu_id",offsetof(csmi_gpu_record_t,gpu_id),0,NULL,0x4ee05d,36},
{"pci_bus_id",offsetof(csmi_gpu_record_t,pci_bus_id),0,NULL,0x6b695f36,4},
{"hbm_memory",offsetof(csmi_gpu_record_t,hbm_memory),0,NULL,0xa023ea94,40},
{"device_name",offsetof(csmi_gpu_record_t,device_name),0,NULL,0xeb7bd8d5,4},
{NULL,0,0,NULL,0,0},
{"vbios",offsetof(csmi_gpu_record_t,vbios),0,NULL,0x108dd628,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"uuid",offsetof(csmi_gpu_record_t,uuid),0,NULL,0x7c9f377c,4},
{"serial_number",offsetof(csmi_gpu_record_t,serial_number),0,NULL,0xd931f68d,4}}
;

void* cast_csmi_gpu_record_t(void* ptr,size_t index) { 
    csmi_gpu_record_t ** ptr_cast = *(csmi_gpu_record_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csmi_gpu_record_t= {
    13,
    csmi_gpu_record_tree,
    cast_csmi_gpu_record_t
};

const csmi_struct_node_t csmi_hca_record_tree[12] = {{"node_name",offsetof(csmi_hca_record_t,node_name),0,NULL,0x746e3e2b,4},
{"board_id",offsetof(csmi_hca_record_t,board_id),0,NULL,0x37f2e639,4},
{"device_name",offsetof(csmi_hca_record_t,device_name),0,NULL,0xeb7bd8d5,4},
{"hw_rev",offsetof(csmi_hca_record_t,hw_rev),0,NULL,0x316f490,4},
{"part_number",offsetof(csmi_hca_record_t,part_number),0,NULL,0x532f81a4,4},
{"serial_number",offsetof(csmi_hca_record_t,serial_number),0,NULL,0xd931f68d,4},
{"fw_ver",offsetof(csmi_hca_record_t,fw_ver),0,NULL,0xfe6cb44e,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"pci_bus_id",offsetof(csmi_hca_record_t,pci_bus_id),0,NULL,0x6b695f36,4},
{"guid",offsetof(csmi_hca_record_t,guid),0,NULL,0x7c978a2e,4}}
;

void* cast_csmi_hca_record_t(void* ptr,size_t index) { 
    csmi_hca_record_t ** ptr_cast = *(csmi_hca_record_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csmi_hca_record_t= {
    12,
    csmi_hca_record_tree,
    cast_csmi_hca_record_t
};

const csmi_struct_node_t csmi_ib_cable_record_tree[23] = {{"discovery_time",offsetof(csmi_ib_cable_record_t,discovery_time),0,NULL,0x603630cb,4},
{"guid_s1",offsetof(csmi_ib_cable_record_t,guid_s1),0,NULL,0x14fe2691,4},
{"identifier",offsetof(csmi_ib_cable_record_t,identifier),0,NULL,0xbe5ad288,4},
{"length",offsetof(csmi_ib_cable_record_t,length),0,NULL,0xb2deac7,4},
{"revision",offsetof(csmi_ib_cable_record_t,revision),0,NULL,0x2aabb274,4},
{"port_s1",offsetof(csmi_ib_cable_record_t,port_s1),0,NULL,0xbc76f82d,4},
{"collection_time",offsetof(csmi_ib_cable_record_t,collection_time),0,NULL,0xd67e7d1f,4},
{NULL,0,0,NULL,0,0},
{"width",offsetof(csmi_ib_cable_record_t,width),0,NULL,0x10a3b0a5,4},
{"guid_s2",offsetof(csmi_ib_cable_record_t,guid_s2),0,NULL,0x14fe2692,4},
{"part_number",offsetof(csmi_ib_cable_record_t,part_number),0,NULL,0x532f81a4,4},
{"name",offsetof(csmi_ib_cable_record_t,name),0,NULL,0x7c9b0c46,4},
{"port_s2",offsetof(csmi_ib_cable_record_t,port_s2),0,NULL,0xbc76f82e,4},
{"comment",offsetof(csmi_ib_cable_record_t,comment),0,NULL,0xd363aa58,4},
{"serial_number",offsetof(csmi_ib_cable_record_t,serial_number),0,NULL,0xd931f68d,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"severity",offsetof(csmi_ib_cable_record_t,severity),0,NULL,0x16a499a0,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"type",offsetof(csmi_ib_cable_record_t,type),0,NULL,0x7c9ebd07,4}}
;

void* cast_csmi_ib_cable_record_t(void* ptr,size_t index) { 
    csmi_ib_cable_record_t ** ptr_cast = *(csmi_ib_cable_record_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csmi_ib_cable_record_t= {
    23,
    csmi_ib_cable_record_tree,
    cast_csmi_ib_cable_record_t
};

const csmi_struct_node_t csmi_ib_cable_history_record_tree[25] = {{"history_time",offsetof(csmi_ib_cable_history_record_t,history_time),0,NULL,0x60dc8265,4},
{"guid_s2",offsetof(csmi_ib_cable_history_record_t,guid_s2),0,NULL,0x14fe2692,4},
{"identifier",offsetof(csmi_ib_cable_history_record_t,identifier),0,NULL,0xbe5ad288,4},
{"width",offsetof(csmi_ib_cable_history_record_t,width),0,NULL,0x10a3b0a5,4},
{"part_number",offsetof(csmi_ib_cable_history_record_t,part_number),0,NULL,0x532f81a4,4},
{"port_s1",offsetof(csmi_ib_cable_history_record_t,port_s1),0,NULL,0xbc76f82d,4},
{"collection_time",offsetof(csmi_ib_cable_history_record_t,collection_time),0,NULL,0xd67e7d1f,4},
{"length",offsetof(csmi_ib_cable_history_record_t,length),0,NULL,0xb2deac7,4},
{"guid_s1",offsetof(csmi_ib_cable_history_record_t,guid_s1),0,NULL,0x14fe2691,4},
{"revision",offsetof(csmi_ib_cable_history_record_t,revision),0,NULL,0x2aabb274,4},
{"discovery_time",offsetof(csmi_ib_cable_history_record_t,discovery_time),0,NULL,0x603630cb,4},
{"name",offsetof(csmi_ib_cable_history_record_t,name),0,NULL,0x7c9b0c46,4},
{"port_s2",offsetof(csmi_ib_cable_history_record_t,port_s2),0,NULL,0xbc76f82e,4},
{"comment",offsetof(csmi_ib_cable_history_record_t,comment),0,NULL,0xd363aa58,4},
{"serial_number",offsetof(csmi_ib_cable_history_record_t,serial_number),0,NULL,0xd931f68d,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"severity",offsetof(csmi_ib_cable_history_record_t,severity),0,NULL,0x16a499a0,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"type",offsetof(csmi_ib_cable_history_record_t,type),0,NULL,0x7c9ebd07,4}}
;

void* cast_csmi_ib_cable_history_record_t(void* ptr,size_t index) { 
    csmi_ib_cable_history_record_t ** ptr_cast = *(csmi_ib_cable_history_record_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csmi_ib_cable_history_record_t= {
    25,
    csmi_ib_cable_history_record_tree,
    cast_csmi_ib_cable_history_record_t
};

const csmi_struct_node_t csmi_node_attributes_record_tree[57] = {{"type",offsetof(csmi_node_attributes_record_t,type),csmi_node_type_t_MAX,&csmi_node_type_t_strs,0x7c9ebd07,8},
{"available_cores",offsetof(csmi_node_attributes_record_t,available_cores),0,NULL,0x3fbd1be1,36},
{"comment",offsetof(csmi_node_attributes_record_t,comment),0,NULL,0xd363aa58,4},
{"ready",offsetof(csmi_node_attributes_record_t,ready),0,NULL,0x1046f5da,16},
{"discovered_gpus",offsetof(csmi_node_attributes_record_t,discovered_gpus),0,NULL,0x6d9c4a2b,36},
{"hard_power_cap",offsetof(csmi_node_attributes_record_t,hard_power_cap),0,NULL,0xb92ef1a3,36},
{"feature_4",offsetof(csmi_node_attributes_record_t,feature_4),0,NULL,0xd3976c24,4},
{"os_image_uuid",offsetof(csmi_node_attributes_record_t,os_image_uuid),0,NULL,0x36eafff,4},
{"kernel_version",offsetof(csmi_node_attributes_record_t,kernel_version),0,NULL,0x134c03eb,4},
{"physical_u_location",offsetof(csmi_node_attributes_record_t,physical_u_location),0,NULL,0x5783af2e,4},
{"node_name",offsetof(csmi_node_attributes_record_t,node_name),0,NULL,0x746e3e2b,4},
{"machine_model",offsetof(csmi_node_attributes_record_t,machine_model),0,NULL,0x8b68b22a,4},
{"available_processors",offsetof(csmi_node_attributes_record_t,available_processors),0,NULL,0xc0fd2a18,36},
{"feature_2",offsetof(csmi_node_attributes_record_t,feature_2),0,NULL,0xd3976c22,4},
{"serial_number",offsetof(csmi_node_attributes_record_t,serial_number),0,NULL,0xd931f68d,4},
{"os_image_name",offsetof(csmi_node_attributes_record_t,os_image_name),0,NULL,0x36a84c9,4},
{"available_gpus",offsetof(csmi_node_attributes_record_t,available_gpus),0,NULL,0x9b29be4,36},
{"state",offsetof(csmi_node_attributes_record_t,state),csmi_node_state_t_MAX,&csmi_node_state_t_strs,0x10614a06,8},
{"discovered_cores",offsetof(csmi_node_attributes_record_t,discovered_cores),0,NULL,0x20dc9308,36},
{"secondary_agg",offsetof(csmi_node_attributes_record_t,secondary_agg),0,NULL,0x4ccfa13b,4},
{"discovery_time",offsetof(csmi_node_attributes_record_t,discovery_time),0,NULL,0x603630cb,4},
{"discovered_hcas",offsetof(csmi_node_attributes_record_t,discovered_hcas),0,NULL,0x6d9c9cab,36},
{"physical_frame_location",offsetof(csmi_node_attributes_record_t,physical_frame_location),0,NULL,0x7762c5a4,4},
{"installed_swap",offsetof(csmi_node_attributes_record_t,installed_swap),0,NULL,0x872544bf,40},
{"primary_agg",offsetof(csmi_node_attributes_record_t,primary_agg),0,NULL,0xad402657,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"feature_1",offsetof(csmi_node_attributes_record_t,feature_1),0,NULL,0xd3976c21,4},
{"feature_3",offsetof(csmi_node_attributes_record_t,feature_3),0,NULL,0xd3976c23,4},
{"installed_memory",offsetof(csmi_node_attributes_record_t,installed_memory),0,NULL,0xd64b631d,40},
{"kernel_release",offsetof(csmi_node_attributes_record_t,kernel_release),0,NULL,0xdeeeaf06,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"discovered_dimms",offsetof(csmi_node_attributes_record_t,discovered_dimms),0,NULL,0x20eb4d06,36},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"discovered_ssds",offsetof(csmi_node_attributes_record_t,discovered_ssds),0,NULL,0x6da2e949,36},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"update_time",offsetof(csmi_node_attributes_record_t,update_time),0,NULL,0x7ceafa96,4},
{NULL,0,0,NULL,0,0},
{"discovered_sockets",offsetof(csmi_node_attributes_record_t,discovered_sockets),0,NULL,0x98e4efa8,36},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"collection_time",offsetof(csmi_node_attributes_record_t,collection_time),0,NULL,0xd67e7d1f,4}}
;

void* cast_csmi_node_attributes_record_t(void* ptr,size_t index) { 
    csmi_node_attributes_record_t ** ptr_cast = *(csmi_node_attributes_record_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csmi_node_attributes_record_t= {
    57,
    csmi_node_attributes_record_tree,
    cast_csmi_node_attributes_record_t
};

const csmi_struct_node_t csmi_node_attributes_history_record_tree[57] = {{"type",offsetof(csmi_node_attributes_history_record_t,type),csmi_node_type_t_MAX,&csmi_node_type_t_strs,0x7c9ebd07,8},
{"available_cores",offsetof(csmi_node_attributes_history_record_t,available_cores),0,NULL,0x3fbd1be1,36},
{"comment",offsetof(csmi_node_attributes_history_record_t,comment),0,NULL,0xd363aa58,4},
{"ready",offsetof(csmi_node_attributes_history_record_t,ready),0,NULL,0x1046f5da,16},
{"history_time",offsetof(csmi_node_attributes_history_record_t,history_time),0,NULL,0x60dc8265,4},
{"hard_power_cap",offsetof(csmi_node_attributes_history_record_t,hard_power_cap),0,NULL,0xb92ef1a3,36},
{"feature_4",offsetof(csmi_node_attributes_history_record_t,feature_4),0,NULL,0xd3976c24,4},
{"os_image_uuid",offsetof(csmi_node_attributes_history_record_t,os_image_uuid),0,NULL,0x36eafff,4},
{"kernel_version",offsetof(csmi_node_attributes_history_record_t,kernel_version),0,NULL,0x134c03eb,4},
{"physical_u_location",offsetof(csmi_node_attributes_history_record_t,physical_u_location),0,NULL,0x5783af2e,4},
{"node_name",offsetof(csmi_node_attributes_history_record_t,node_name),0,NULL,0x746e3e2b,4},
{"machine_model",offsetof(csmi_node_attributes_history_record_t,machine_model),0,NULL,0x8b68b22a,4},
{"available_processors",offsetof(csmi_node_attributes_history_record_t,available_processors),0,NULL,0xc0fd2a18,36},
{"feature_2",offsetof(csmi_node_attributes_history_record_t,feature_2),0,NULL,0xd3976c22,4},
{"serial_number",offsetof(csmi_node_attributes_history_record_t,serial_number),0,NULL,0xd931f68d,4},
{"os_image_name",offsetof(csmi_node_attributes_history_record_t,os_image_name),0,NULL,0x36a84c9,4},
{"available_gpus",offsetof(csmi_node_attributes_history_record_t,available_gpus),0,NULL,0x9b29be4,36},
{"state",offsetof(csmi_node_attributes_history_record_t,state),csmi_node_state_t_MAX,&csmi_node_state_t_strs,0x10614a06,8},
{"discovered_cores",offsetof(csmi_node_attributes_history_record_t,discovered_cores),0,NULL,0x20dc9308,36},
{"secondary_agg",offsetof(csmi_node_attributes_history_record_t,secondary_agg),0,NULL,0x4ccfa13b,4},
{"discovery_time",offsetof(csmi_node_attributes_history_record_t,discovery_time),0,NULL,0x603630cb,4},
{"discovered_hcas",offsetof(csmi_node_attributes_history_record_t,discovered_hcas),0,NULL,0x6d9c9cab,36},
{"physical_frame_location",offsetof(csmi_node_attributes_history_record_t,physical_frame_location),0,NULL,0x7762c5a4,4},
{"installed_swap",offsetof(csmi_node_attributes_history_record_t,installed_swap),0,NULL,0x872544bf,40},
{"primary_agg",offsetof(csmi_node_attributes_history_record_t,primary_agg),0,NULL,0xad402657,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"feature_1",offsetof(csmi_node_attributes_history_record_t,feature_1),0,NULL,0xd3976c21,4},
{"feature_3",offsetof(csmi_node_attributes_history_record_t,feature_3),0,NULL,0xd3976c23,4},
{"installed_memory",offsetof(csmi_node_attributes_history_record_t,installed_memory),0,NULL,0xd64b631d,40},
{"kernel_release",offsetof(csmi_node_attributes_history_record_t,kernel_release),0,NULL,0xdeeeaf06,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"discovered_dimms",offsetof(csmi_node_attributes_history_record_t,discovered_dimms),0,NULL,0x20eb4d06,36},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"discovered_gpus",offsetof(csmi_node_attributes_history_record_t,discovered_gpus),0,NULL,0x6d9c4a2b,36},
{"discovered_ssds",offsetof(csmi_node_attributes_history_record_t,discovered_ssds),0,NULL,0x6da2e949,36},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"update_time",offsetof(csmi_node_attributes_history_record_t,update_time),0,NULL,0x7ceafa96,4},
{NULL,0,0,NULL,0,0},
{"discovered_sockets",offsetof(csmi_node_attributes_history_record_t,discovered_sockets),0,NULL,0x98e4efa8,36},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"collection_time",offsetof(csmi_node_attributes_history_record_t,collection_time),0,NULL,0xd67e7d1f,4}}
;

void* cast_csmi_node_attributes_history_record_t(void* ptr,size_t index) { 
    csmi_node_attributes_history_record_t ** ptr_cast = *(csmi_node_attributes_history_record_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csmi_node_attributes_history_record_t= {
    57,
    csmi_node_attributes_history_record_tree,
    cast_csmi_node_attributes_history_record_t
};

const csmi_struct_node_t csmi_node_query_state_history_record_tree[7] = {{"alteration",offsetof(csmi_node_query_state_history_record_t,alteration),csmi_node_alteration_t_MAX,&csmi_node_alteration_t_strs,0x5fc5c138,8},
{"state",offsetof(csmi_node_query_state_history_record_t,state),csmi_node_state_t_MAX,&csmi_node_state_t_strs,0x10614a06,8},
{"ras_msg_id",offsetof(csmi_node_query_state_history_record_t,ras_msg_id),0,NULL,0xde95e99d,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"history_time",offsetof(csmi_node_query_state_history_record_t,history_time),0,NULL,0x60dc8265,4},
{"ras_rec_id",offsetof(csmi_node_query_state_history_record_t,ras_rec_id),0,NULL,0xe9402c30,4}}
;

void* cast_csmi_node_query_state_history_record_t(void* ptr,size_t index) { 
    csmi_node_query_state_history_record_t ** ptr_cast = *(csmi_node_query_state_history_record_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csmi_node_query_state_history_record_t= {
    7,
    csmi_node_query_state_history_record_tree,
    cast_csmi_node_query_state_history_record_t
};

const csmi_struct_node_t csmi_processor_record_tree[9] = {{"status",offsetof(csmi_processor_record_t,status),0,NULL,0x1c8a8d49,68},
{"socket",offsetof(csmi_processor_record_t,socket),0,NULL,0x1c31032e,36},
{"node_name",offsetof(csmi_processor_record_t,node_name),0,NULL,0x746e3e2b,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"available_cores",offsetof(csmi_processor_record_t,available_cores),0,NULL,0x3fbd1be1,36},
{"serial_number",offsetof(csmi_processor_record_t,serial_number),0,NULL,0xd931f68d,4},
{"discovered_cores",offsetof(csmi_processor_record_t,discovered_cores),0,NULL,0x20dc9308,36},
{"physical_location",offsetof(csmi_processor_record_t,physical_location),0,NULL,0x63efcf7a,4}}
;

void* cast_csmi_processor_record_t(void* ptr,size_t index) { 
    csmi_processor_record_t ** ptr_cast = *(csmi_processor_record_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csmi_processor_record_t= {
    9,
    csmi_processor_record_tree,
    cast_csmi_processor_record_t
};

const csmi_struct_node_t csmi_ssd_record_tree[25] = {{"wear_total_bytes_read",offsetof(csmi_ssd_record_t,wear_total_bytes_read),0,NULL,0x9e5253b8,40},
{"wear_lifespan_used",offsetof(csmi_ssd_record_t,wear_lifespan_used),0,NULL,0x7bc95915,56},
{"total_size",offsetof(csmi_ssd_record_t,total_size),0,NULL,0xc7f736e3,40},
{"discovery_time",offsetof(csmi_ssd_record_t,discovery_time),0,NULL,0x603630cb,4},
{"size",offsetof(csmi_ssd_record_t,size),0,NULL,0x7c9dede0,40},
{"wear_total_bytes_written",offsetof(csmi_ssd_record_t,wear_total_bytes_written),0,NULL,0xa2cc1da9,40},
{"wear_percent_spares_remaining",offsetof(csmi_ssd_record_t,wear_percent_spares_remaining),0,NULL,0xe9d9e3aa,56},
{"status",offsetof(csmi_ssd_record_t,status),0,NULL,0x1c8a8d49,68},
{"node_name",offsetof(csmi_ssd_record_t,node_name),0,NULL,0x746e3e2b,4},
{NULL,0,0,NULL,0,0},
{"update_time",offsetof(csmi_ssd_record_t,update_time),0,NULL,0x7ceafa96,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"serial_number",offsetof(csmi_ssd_record_t,serial_number),0,NULL,0xd931f68d,4},
{"device_name",offsetof(csmi_ssd_record_t,device_name),0,NULL,0xeb7bd8d5,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"pci_bus_id",offsetof(csmi_ssd_record_t,pci_bus_id),0,NULL,0x6b695f36,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"fw_ver",offsetof(csmi_ssd_record_t,fw_ver),0,NULL,0xfe6cb44e,4}}
;

void* cast_csmi_ssd_record_t(void* ptr,size_t index) { 
    csmi_ssd_record_t ** ptr_cast = *(csmi_ssd_record_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csmi_ssd_record_t= {
    25,
    csmi_ssd_record_tree,
    cast_csmi_ssd_record_t
};

const csmi_struct_node_t csmi_switch_record_tree[44] = {{"discovery_time",offsetof(csmi_switch_record_t,discovery_time),0,NULL,0x603630cb,4},
{"fw_version",offsetof(csmi_switch_record_t,fw_version),0,NULL,0x136b0847,4},
{"has_ufm_agent",offsetof(csmi_switch_record_t,has_ufm_agent),0,NULL,0xaab4c3f6,16},
{"gu_id",offsetof(csmi_switch_record_t,gu_id),0,NULL,0xf88a66d,4},
{"num_modules",offsetof(csmi_switch_record_t,num_modules),0,NULL,0x284a2a6d,36},
{"system_name",offsetof(csmi_switch_record_t,system_name),0,NULL,0x82ba5f6a,4},
{"collection_time",offsetof(csmi_switch_record_t,collection_time),0,NULL,0xd67e7d1f,4},
{"ip",offsetof(csmi_switch_record_t,ip),0,NULL,0x59783e,4},
{"ps_id",offsetof(csmi_switch_record_t,ps_id),0,NULL,0x102a6a34,4},
{"hw_version",offsetof(csmi_switch_record_t,hw_version),0,NULL,0x1b802a89,4},
{"switch_name",offsetof(csmi_switch_record_t,switch_name),0,NULL,0x482a8e77,4},
{"role",offsetof(csmi_switch_record_t,role),0,NULL,0x7c9d7937,4},
{"description",offsetof(csmi_switch_record_t,description),0,NULL,0x91b0c789,4},
{"comment",offsetof(csmi_switch_record_t,comment),0,NULL,0xd363aa58,4},
{"serial_number",offsetof(csmi_switch_record_t,serial_number),0,NULL,0xd931f68d,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"model",offsetof(csmi_switch_record_t,model),0,NULL,0xff203f6,4},
{"state",offsetof(csmi_switch_record_t,state),0,NULL,0x10614a06,4},
{NULL,0,0,NULL,0,0},
{"vendor",offsetof(csmi_switch_record_t,vendor),0,NULL,0x228173b3,4},
{"sw_version",offsetof(csmi_switch_record_t,sw_version),0,NULL,0x47f466f4,4},
{"physical_u_location",offsetof(csmi_switch_record_t,physical_u_location),0,NULL,0x5783af2e,4},
{"physical_frame_location",offsetof(csmi_switch_record_t,physical_frame_location),0,NULL,0x7762c5a4,4},
{"system_guid",offsetof(csmi_switch_record_t,system_guid),0,NULL,0x82b6dd52,4},
{"server_operation_mode",offsetof(csmi_switch_record_t,server_operation_mode),0,NULL,0x87894af0,4},
{"sm_mode",offsetof(csmi_switch_record_t,sm_mode),0,NULL,0x9d5ff749,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"total_alarms",offsetof(csmi_switch_record_t,total_alarms),0,NULL,0x78cc7a28,36},
{"type",offsetof(csmi_switch_record_t,type),0,NULL,0x7c9ebd07,4}}
;

void* cast_csmi_switch_record_t(void* ptr,size_t index) { 
    csmi_switch_record_t ** ptr_cast = *(csmi_switch_record_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csmi_switch_record_t= {
    44,
    csmi_switch_record_tree,
    cast_csmi_switch_record_t
};

const csmi_struct_node_t csmi_switch_inventory_record_tree[27] = {{"name",offsetof(csmi_switch_inventory_record_t,name),0,NULL,0x7c9b0c46,4},
{"max_ib_ports",offsetof(csmi_switch_inventory_record_t,max_ib_ports),0,NULL,0x1c135c0c,36},
{"host_system_guid",offsetof(csmi_switch_inventory_record_t,host_system_guid),0,NULL,0xd5e2674f,4},
{"severity",offsetof(csmi_switch_inventory_record_t,severity),0,NULL,0x16a499a0,4},
{"discovery_time",offsetof(csmi_switch_inventory_record_t,discovery_time),0,NULL,0x603630cb,4},
{"module_index",offsetof(csmi_switch_inventory_record_t,module_index),0,NULL,0xc43864a2,36},
{"device_name",offsetof(csmi_switch_inventory_record_t,device_name),0,NULL,0xeb7bd8d5,4},
{"number_of_chips",offsetof(csmi_switch_inventory_record_t,number_of_chips),0,NULL,0x45b74b8,36},
{"hw_version",offsetof(csmi_switch_inventory_record_t,hw_version),0,NULL,0x1b802a89,4},
{"status",offsetof(csmi_switch_inventory_record_t,status),0,NULL,0x1c8a8d49,4},
{NULL,0,0,NULL,0,0},
{"description",offsetof(csmi_switch_inventory_record_t,description),0,NULL,0x91b0c789,4},
{"comment",offsetof(csmi_switch_inventory_record_t,comment),0,NULL,0xd363aa58,4},
{"collection_time",offsetof(csmi_switch_inventory_record_t,collection_time),0,NULL,0xd67e7d1f,4},
{"device_type",offsetof(csmi_switch_inventory_record_t,device_type),0,NULL,0xeb7f8996,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"path",offsetof(csmi_switch_inventory_record_t,path),0,NULL,0x7c9c25f2,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"serial_number",offsetof(csmi_switch_inventory_record_t,serial_number),0,NULL,0xd931f68d,4}}
;

void* cast_csmi_switch_inventory_record_t(void* ptr,size_t index) { 
    csmi_switch_inventory_record_t ** ptr_cast = *(csmi_switch_inventory_record_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csmi_switch_inventory_record_t= {
    27,
    csmi_switch_inventory_record_tree,
    cast_csmi_switch_inventory_record_t
};

const csmi_struct_node_t csmi_switch_ports_record_tree[42] = {{"description",offsetof(csmi_switch_ports_record_t,description),0,NULL,0x91b0c789,4},
{"enabled_speed",offsetof(csmi_switch_ports_record_t,enabled_speed),0,NULL,0x2c6b10c0,4},
{"comment",offsetof(csmi_switch_ports_record_t,comment),0,NULL,0xd363aa58,4},
{"mirror",offsetof(csmi_switch_ports_record_t,mirror),0,NULL,0xdcdd520,4},
{"guid",offsetof(csmi_switch_ports_record_t,guid),0,NULL,0x7c978a2e,4},
{"width_active",offsetof(csmi_switch_ports_record_t,width_active),0,NULL,0x9e6db9c0,4},
{"collection_time",offsetof(csmi_switch_ports_record_t,collection_time),0,NULL,0xd67e7d1f,4},
{"lid",offsetof(csmi_switch_ports_record_t,lid),0,NULL,0xb888c3e,4},
{"number",offsetof(csmi_switch_ports_record_t,number),0,NULL,0x10f9208e,4},
{"discovery_time",offsetof(csmi_switch_ports_record_t,discovery_time),0,NULL,0x603630cb,4},
{"peer",offsetof(csmi_switch_ports_record_t,peer),0,NULL,0x7c9c3511,4},
{"external_number",offsetof(csmi_switch_ports_record_t,external_number),0,NULL,0x9a390390,4},
{"width_enabled",offsetof(csmi_switch_ports_record_t,width_enabled),0,NULL,0xb85ad14f,4},
{NULL,0,0,NULL,0,0},
{"physical_state",offsetof(csmi_switch_ports_record_t,physical_state),0,NULL,0xdbbc7de2,4},
{NULL,0,0,NULL,0,0},
{"mtu",offsetof(csmi_switch_ports_record_t,mtu),0,NULL,0xb8891fb,4},
{"module",offsetof(csmi_switch_ports_record_t,module),0,NULL,0xe32c72b,4},
{"parent",offsetof(csmi_switch_ports_record_t,parent),0,NULL,0x143c538f,4},
{"active_speed",offsetof(csmi_switch_ports_record_t,active_speed),0,NULL,0x3e93add1,4},
{"logical_state",offsetof(csmi_switch_ports_record_t,logical_state),0,NULL,0x63daa400,4},
{"name",offsetof(csmi_switch_ports_record_t,name),0,NULL,0x7c9b0c46,4},
{"system_guid",offsetof(csmi_switch_ports_record_t,system_guid),0,NULL,0x82b6dd52,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"width_supported",offsetof(csmi_switch_ports_record_t,width_supported),0,NULL,0xb27ee54a,4},
{"supported_speed",offsetof(csmi_switch_ports_record_t,supported_speed),0,NULL,0xc0a0393b,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"severity",offsetof(csmi_switch_ports_record_t,severity),0,NULL,0x16a499a0,4},
{"max_supported_speed",offsetof(csmi_switch_ports_record_t,max_supported_speed),0,NULL,0x3c485ae0,4},
{"mirror_traffic",offsetof(csmi_switch_ports_record_t,mirror_traffic),0,NULL,0x43338b1e,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"tier",offsetof(csmi_switch_ports_record_t,tier),0,NULL,0x7c9e7799,4}}
;

void* cast_csmi_switch_ports_record_t(void* ptr,size_t index) { 
    csmi_switch_ports_record_t ** ptr_cast = *(csmi_switch_ports_record_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csmi_switch_ports_record_t= {
    42,
    csmi_switch_ports_record_tree,
    cast_csmi_switch_ports_record_t
};

const csmi_struct_node_t csmi_switch_details_tree[6] = {{"switch_data",offsetof(csmi_switch_details_t,switch_data),0,&map_csmi_switch_record_t,0x48251390,0},
{"inventory_count",offsetof(csmi_switch_details_t,inventory_count),0,NULL,0xde0c63b,24},
{"inventory",offsetof(csmi_switch_details_t,inventory),offsetof(csmi_switch_details_t, inventory_count),&map_csmi_switch_inventory_record_t,0xac696ff3,1},
{NULL,0,0,NULL,0,0},
{"ports",offsetof(csmi_switch_details_t,ports),offsetof(csmi_switch_details_t, ports_count),&map_csmi_switch_ports_record_t,0x10288afd,1},
{"ports_count",offsetof(csmi_switch_details_t,ports_count),0,NULL,0x6b0e80c5,24}}
;

void* cast_csmi_switch_details_t(void* ptr,size_t index) { 
    csmi_switch_details_t ** ptr_cast = *(csmi_switch_details_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csmi_switch_details_t= {
    6,
    csmi_switch_details_tree,
    cast_csmi_switch_details_t
};

const csmi_struct_node_t csmi_switch_history_record_tree[49] = {{"history_time",offsetof(csmi_switch_history_record_t,history_time),0,NULL,0x60dc8265,4},
{"fw_version",offsetof(csmi_switch_history_record_t,fw_version),0,NULL,0x136b0847,4},
{"has_ufm_agent",offsetof(csmi_switch_history_record_t,has_ufm_agent),0,NULL,0xaab4c3f6,16},
{"gu_id",offsetof(csmi_switch_history_record_t,gu_id),0,NULL,0xf88a66d,4},
{"switch_name",offsetof(csmi_switch_history_record_t,switch_name),0,NULL,0x482a8e77,4},
{"system_name",offsetof(csmi_switch_history_record_t,system_name),0,NULL,0x82ba5f6a,4},
{"collection_time",offsetof(csmi_switch_history_record_t,collection_time),0,NULL,0xd67e7d1f,4},
{"ip",offsetof(csmi_switch_history_record_t,ip),0,NULL,0x59783e,4},
{"ps_id",offsetof(csmi_switch_history_record_t,ps_id),0,NULL,0x102a6a34,4},
{"num_modules",offsetof(csmi_switch_history_record_t,num_modules),0,NULL,0x284a2a6d,36},
{"discovery_time",offsetof(csmi_switch_history_record_t,discovery_time),0,NULL,0x603630cb,4},
{"role",offsetof(csmi_switch_history_record_t,role),0,NULL,0x7c9d7937,4},
{"description",offsetof(csmi_switch_history_record_t,description),0,NULL,0x91b0c789,4},
{"comment",offsetof(csmi_switch_history_record_t,comment),0,NULL,0xd363aa58,4},
{"serial_number",offsetof(csmi_switch_history_record_t,serial_number),0,NULL,0xd931f68d,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"model",offsetof(csmi_switch_history_record_t,model),0,NULL,0xff203f6,4},
{"state",offsetof(csmi_switch_history_record_t,state),0,NULL,0x10614a06,4},
{"hw_version",offsetof(csmi_switch_history_record_t,hw_version),0,NULL,0x1b802a89,4},
{"sw_version",offsetof(csmi_switch_history_record_t,sw_version),0,NULL,0x47f466f4,4},
{"physical_u_location",offsetof(csmi_switch_history_record_t,physical_u_location),0,NULL,0x5783af2e,4},
{NULL,0,0,NULL,0,0},
{"physical_frame_location",offsetof(csmi_switch_history_record_t,physical_frame_location),0,NULL,0x7762c5a4,4},
{"system_guid",offsetof(csmi_switch_history_record_t,system_guid),0,NULL,0x82b6dd52,4},
{"server_operation_mode",offsetof(csmi_switch_history_record_t,server_operation_mode),0,NULL,0x87894af0,4},
{"sm_mode",offsetof(csmi_switch_history_record_t,sm_mode),0,NULL,0x9d5ff749,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"vendor",offsetof(csmi_switch_history_record_t,vendor),0,NULL,0x228173b3,4},
{"operation",offsetof(csmi_switch_history_record_t,operation),0,NULL,0x40a16e96,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"total_alarms",offsetof(csmi_switch_history_record_t,total_alarms),0,NULL,0x78cc7a28,36},
{"type",offsetof(csmi_switch_history_record_t,type),0,NULL,0x7c9ebd07,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"archive_history_time",offsetof(csmi_switch_history_record_t,archive_history_time),0,NULL,0x9e88b9e6,4}}
;

void* cast_csmi_switch_history_record_t(void* ptr,size_t index) { 
    csmi_switch_history_record_t ** ptr_cast = *(csmi_switch_history_record_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csmi_switch_history_record_t= {
    49,
    csmi_switch_history_record_tree,
    cast_csmi_switch_history_record_t
};

const csmi_struct_node_t csmi_node_env_data_tree[1] = {{"field_01",offsetof(csmi_node_env_data_t,field_01),0,NULL,0x2404d529,68}}
;

void* cast_csmi_node_env_data_t(void* ptr,size_t index) { 
    csmi_node_env_data_t ** ptr_cast = *(csmi_node_env_data_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csmi_node_env_data_t= {
    1,
    csmi_node_env_data_tree,
    cast_csmi_node_env_data_t
};

const csmi_struct_node_t csmi_switch_env_data_tree[1] = {{"field_01",offsetof(csmi_switch_env_data_t,field_01),0,NULL,0x2404d529,68}}
;

void* cast_csmi_switch_env_data_t(void* ptr,size_t index) { 
    csmi_switch_env_data_t ** ptr_cast = *(csmi_switch_env_data_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csmi_switch_env_data_t= {
    1,
    csmi_switch_env_data_tree,
    cast_csmi_switch_env_data_t
};

const csmi_struct_node_t csmi_fabric_topology_tree[1] = {{"field_01",offsetof(csmi_fabric_topology_t,field_01),0,NULL,0x2404d529,68}}
;

void* cast_csmi_fabric_topology_t(void* ptr,size_t index) { 
    csmi_fabric_topology_t ** ptr_cast = *(csmi_fabric_topology_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csmi_fabric_topology_t= {
    1,
    csmi_fabric_topology_tree,
    cast_csmi_fabric_topology_t
};

const csmi_struct_node_t csmi_node_details_tree[14] = {{"dimms_count",offsetof(csmi_node_details_t,dimms_count),0,NULL,0x563b93c7,24},
{"ssds_count",offsetof(csmi_node_details_t,ssds_count),0,NULL,0x1d4cb32a,24},
{"node",offsetof(csmi_node_details_t,node),0,&map_csmi_node_attributes_record_t,0x7c9b46ab,0},
{"dimms",offsetof(csmi_node_details_t,dimms),offsetof(csmi_node_details_t, dimms_count),&map_csmi_dimm_record_t,0xf4c047f,1},
{"gpus_count",offsetof(csmi_node_details_t,gpus_count),0,NULL,0x3fb7b58c,24},
{"gpus",offsetof(csmi_node_details_t,gpus),offsetof(csmi_node_details_t, gpus_count),&map_csmi_gpu_record_t,0x7c977684,1},
{"hcas_count",offsetof(csmi_node_details_t,hcas_count),0,NULL,0xe44be80c,24},
{"processors",offsetof(csmi_node_details_t,processors),offsetof(csmi_node_details_t, processors_count),&map_csmi_processor_record_t,0x3037cb8,1},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"processors_count",offsetof(csmi_node_details_t,processors_count),0,NULL,0x676492c0,24},
{"hcas",offsetof(csmi_node_details_t,hcas),offsetof(csmi_node_details_t, hcas_count),&map_csmi_hca_record_t,0x7c97c904,1},
{"ssds",offsetof(csmi_node_details_t,ssds),offsetof(csmi_node_details_t, ssds_count),&map_csmi_ssd_record_t,0x7c9e15a2,1}}
;

void* cast_csmi_node_details_t(void* ptr,size_t index) { 
    csmi_node_details_t ** ptr_cast = *(csmi_node_details_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csmi_node_details_t= {
    14,
    csmi_node_details_tree,
    cast_csmi_node_details_t
};

const csmi_struct_node_t csmi_cluster_query_state_record_tree[15] = {{"node_name",offsetof(csmi_cluster_query_state_record_t,node_name),0,NULL,0x746e3e2b,4},
{"states",offsetof(csmi_cluster_query_state_record_t,states),offsetof(csmi_cluster_query_state_record_t, num_allocs),NULL,0x1c8a8b39,5},
{"update_time",offsetof(csmi_cluster_query_state_record_t,update_time),0,NULL,0x7ceafa96,4},
{"state",offsetof(csmi_cluster_query_state_record_t,state),csmi_node_state_t_MAX,&csmi_node_state_t_strs,0x10614a06,8},
{"num_allocs",offsetof(csmi_cluster_query_state_record_t,num_allocs),0,NULL,0x421fea12,24},
{"type",offsetof(csmi_cluster_query_state_record_t,type),csmi_node_type_t_MAX,&csmi_node_type_t_strs,0x7c9ebd07,8},
{"collection_time",offsetof(csmi_cluster_query_state_record_t,collection_time),0,NULL,0xd67e7d1f,4},
{NULL,0,0,NULL,0,0},
{"shared",offsetof(csmi_cluster_query_state_record_t,shared),offsetof(csmi_cluster_query_state_record_t, num_allocs),NULL,0x1bb15c9c,5},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"allocs",offsetof(csmi_cluster_query_state_record_t,allocs),offsetof(csmi_cluster_query_state_record_t, num_allocs),NULL,0xf202de83,1}}
;

void* cast_csmi_cluster_query_state_record_t(void* ptr,size_t index) { 
    csmi_cluster_query_state_record_t ** ptr_cast = *(csmi_cluster_query_state_record_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csmi_cluster_query_state_record_t= {
    15,
    csmi_cluster_query_state_record_tree,
    cast_csmi_cluster_query_state_record_t
};

const csmi_struct_node_t csm_ib_cable_inventory_collection_input_tree[3] = {{"inventory_count",offsetof(csm_ib_cable_inventory_collection_input_t,inventory_count),0,NULL,0xde0c63b,24},
{NULL,0,0,NULL,0,0},
{"inventory",offsetof(csm_ib_cable_inventory_collection_input_t,inventory),offsetof(csm_ib_cable_inventory_collection_input_t, inventory_count),&map_csmi_ib_cable_record_t,0xac696ff3,1}}
;

void* cast_csm_ib_cable_inventory_collection_input_t(void* ptr,size_t index) { 
    csm_ib_cable_inventory_collection_input_t ** ptr_cast = *(csm_ib_cable_inventory_collection_input_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_ib_cable_inventory_collection_input_t= {
    3,
    csm_ib_cable_inventory_collection_input_tree,
    cast_csm_ib_cable_inventory_collection_input_t
};

const csmi_struct_node_t csm_ib_cable_inventory_collection_output_tree[2] = {{"insert_count",offsetof(csm_ib_cable_inventory_collection_output_t,insert_count),0,NULL,0x7bae7a22,36},
{"update_count",offsetof(csm_ib_cable_inventory_collection_output_t,update_count),0,NULL,0x191a1ab0,36}}
;

void* cast_csm_ib_cable_inventory_collection_output_t(void* ptr,size_t index) { 
    csm_ib_cable_inventory_collection_output_t ** ptr_cast = *(csm_ib_cable_inventory_collection_output_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_ib_cable_inventory_collection_output_t= {
    2,
    csm_ib_cable_inventory_collection_output_tree,
    cast_csm_ib_cable_inventory_collection_output_t
};

const csmi_struct_node_t csm_ib_cable_query_input_tree[7] = {{"offset",offsetof(csm_ib_cable_query_input_t,offset),0,NULL,0x123b4b4c,36},
{"limit",offsetof(csm_ib_cable_query_input_t,limit),0,NULL,0xfdcc804,36},
{"serial_numbers_count",offsetof(csm_ib_cable_query_input_t,serial_numbers_count),0,NULL,0x54252ca8,24},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"serial_numbers",offsetof(csm_ib_cable_query_input_t,serial_numbers),offsetof(csm_ib_cable_query_input_t, serial_numbers_count),NULL,0xff70c8a0,5}}
;

void* cast_csm_ib_cable_query_input_t(void* ptr,size_t index) { 
    csm_ib_cable_query_input_t ** ptr_cast = *(csm_ib_cable_query_input_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_ib_cable_query_input_t= {
    7,
    csm_ib_cable_query_input_tree,
    cast_csm_ib_cable_query_input_t
};

const csmi_struct_node_t csm_ib_cable_query_output_tree[2] = {{"results_count",offsetof(csm_ib_cable_query_output_t,results_count),0,NULL,0x8261013f,24},
{"results",offsetof(csm_ib_cable_query_output_t,results),offsetof(csm_ib_cable_query_output_t, results_count),&map_csmi_ib_cable_record_t,0x3f2ab7f7,1}}
;

void* cast_csm_ib_cable_query_output_t(void* ptr,size_t index) { 
    csm_ib_cable_query_output_t ** ptr_cast = *(csm_ib_cable_query_output_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_ib_cable_query_output_t= {
    2,
    csm_ib_cable_query_output_tree,
    cast_csm_ib_cable_query_output_t
};

const csmi_struct_node_t csm_ib_cable_query_history_input_tree[3] = {{"offset",offsetof(csm_ib_cable_query_history_input_t,offset),0,NULL,0x123b4b4c,36},
{"limit",offsetof(csm_ib_cable_query_history_input_t,limit),0,NULL,0xfdcc804,36},
{"serial_number",offsetof(csm_ib_cable_query_history_input_t,serial_number),0,NULL,0xd931f68d,4}}
;

void* cast_csm_ib_cable_query_history_input_t(void* ptr,size_t index) { 
    csm_ib_cable_query_history_input_t ** ptr_cast = *(csm_ib_cable_query_history_input_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_ib_cable_query_history_input_t= {
    3,
    csm_ib_cable_query_history_input_tree,
    cast_csm_ib_cable_query_history_input_t
};

const csmi_struct_node_t csm_ib_cable_query_history_output_tree[2] = {{"results_count",offsetof(csm_ib_cable_query_history_output_t,results_count),0,NULL,0x8261013f,24},
{"results",offsetof(csm_ib_cable_query_history_output_t,results),offsetof(csm_ib_cable_query_history_output_t, results_count),&map_csmi_ib_cable_history_record_t,0x3f2ab7f7,1}}
;

void* cast_csm_ib_cable_query_history_output_t(void* ptr,size_t index) { 
    csm_ib_cable_query_history_output_t ** ptr_cast = *(csm_ib_cable_query_history_output_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_ib_cable_query_history_output_t= {
    2,
    csm_ib_cable_query_history_output_tree,
    cast_csm_ib_cable_query_history_output_t
};

const csmi_struct_node_t csm_ib_cable_update_input_tree[11] = {{"comment",offsetof(csm_ib_cable_update_input_t,comment),0,NULL,0xd363aa58,4},
{"guid_s2",offsetof(csm_ib_cable_update_input_t,guid_s2),0,NULL,0x14fe2692,4},
{"serial_numbers",offsetof(csm_ib_cable_update_input_t,serial_numbers),offsetof(csm_ib_cable_update_input_t, serial_numbers_count),NULL,0xff70c8a0,5},
{"guid_s1",offsetof(csm_ib_cable_update_input_t,guid_s1),0,NULL,0x14fe2691,4},
{"port_s1",offsetof(csm_ib_cable_update_input_t,port_s1),0,NULL,0xbc76f82d,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"serial_numbers_count",offsetof(csm_ib_cable_update_input_t,serial_numbers_count),0,NULL,0x54252ca8,24},
{"port_s2",offsetof(csm_ib_cable_update_input_t,port_s2),0,NULL,0xbc76f82e,4}}
;

void* cast_csm_ib_cable_update_input_t(void* ptr,size_t index) { 
    csm_ib_cable_update_input_t ** ptr_cast = *(csm_ib_cable_update_input_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_ib_cable_update_input_t= {
    11,
    csm_ib_cable_update_input_tree,
    cast_csm_ib_cable_update_input_t
};

const csmi_struct_node_t csm_ib_cable_update_output_tree[2] = {{"failure_count",offsetof(csm_ib_cable_update_output_t,failure_count),0,NULL,0xb64de7b5,24},
{"failure_ib_cables",offsetof(csm_ib_cable_update_output_t,failure_ib_cables),offsetof(csm_ib_cable_update_output_t, failure_count),NULL,0x78fd5dc0,5}}
;

void* cast_csm_ib_cable_update_output_t(void* ptr,size_t index) { 
    csm_ib_cable_update_output_t ** ptr_cast = *(csm_ib_cable_update_output_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_ib_cable_update_output_t= {
    2,
    csm_ib_cable_update_output_tree,
    cast_csm_ib_cable_update_output_t
};

const csmi_struct_node_t csm_node_attributes_query_input_tree[11] = {{"offset",offsetof(csm_node_attributes_query_input_t,offset),0,NULL,0x123b4b4c,36},
{"limit",offsetof(csm_node_attributes_query_input_t,limit),0,NULL,0xfdcc804,36},
{"node_names_count",offsetof(csm_node_attributes_query_input_t,node_names_count),0,NULL,0x868cf686,24},
{"node_names",offsetof(csm_node_attributes_query_input_t,node_names),offsetof(csm_node_attributes_query_input_t, node_names_count),NULL,0x23603fe,5},
{"ready",offsetof(csm_node_attributes_query_input_t,ready),0,NULL,0x1046f5da,16},
{"type",offsetof(csm_node_attributes_query_input_t,type),csmi_node_type_t_MAX,&csmi_node_type_t_strs,0x7c9ebd07,8},
{"comment",offsetof(csm_node_attributes_query_input_t,comment),0,NULL,0xd363aa58,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"state",offsetof(csm_node_attributes_query_input_t,state),csmi_node_state_t_MAX,&csmi_node_state_t_strs,0x10614a06,8}}
;

void* cast_csm_node_attributes_query_input_t(void* ptr,size_t index) { 
    csm_node_attributes_query_input_t ** ptr_cast = *(csm_node_attributes_query_input_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_node_attributes_query_input_t= {
    11,
    csm_node_attributes_query_input_tree,
    cast_csm_node_attributes_query_input_t
};

const csmi_struct_node_t csm_node_attributes_query_output_tree[2] = {{"results_count",offsetof(csm_node_attributes_query_output_t,results_count),0,NULL,0x8261013f,24},
{"results",offsetof(csm_node_attributes_query_output_t,results),offsetof(csm_node_attributes_query_output_t, results_count),&map_csmi_node_attributes_record_t,0x3f2ab7f7,1}}
;

void* cast_csm_node_attributes_query_output_t(void* ptr,size_t index) { 
    csm_node_attributes_query_output_t ** ptr_cast = *(csm_node_attributes_query_output_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_node_attributes_query_output_t= {
    2,
    csm_node_attributes_query_output_tree,
    cast_csm_node_attributes_query_output_t
};

const csmi_struct_node_t csm_node_attributes_query_details_input_tree[1] = {{"node_name",offsetof(csm_node_attributes_query_details_input_t,node_name),0,NULL,0x746e3e2b,4}}
;

void* cast_csm_node_attributes_query_details_input_t(void* ptr,size_t index) { 
    csm_node_attributes_query_details_input_t ** ptr_cast = *(csm_node_attributes_query_details_input_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_node_attributes_query_details_input_t= {
    1,
    csm_node_attributes_query_details_input_tree,
    cast_csm_node_attributes_query_details_input_t
};

const csmi_struct_node_t csm_node_attributes_query_details_output_tree[2] = {{"result_count",offsetof(csm_node_attributes_query_details_output_t,result_count),0,NULL,0x4236760c,24},
{"result",offsetof(csm_node_attributes_query_details_output_t,result),offsetof(csm_node_attributes_query_details_output_t, result_count),&map_csmi_node_details_t,0x192fd704,1}}
;

void* cast_csm_node_attributes_query_details_output_t(void* ptr,size_t index) { 
    csm_node_attributes_query_details_output_t ** ptr_cast = *(csm_node_attributes_query_details_output_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_node_attributes_query_details_output_t= {
    2,
    csm_node_attributes_query_details_output_tree,
    cast_csm_node_attributes_query_details_output_t
};

const csmi_struct_node_t csm_node_attributes_query_history_input_tree[6] = {{"offset",offsetof(csm_node_attributes_query_history_input_t,offset),0,NULL,0x123b4b4c,36},
{"limit",offsetof(csm_node_attributes_query_history_input_t,limit),0,NULL,0xfdcc804,36},
{"node_name",offsetof(csm_node_attributes_query_history_input_t,node_name),0,NULL,0x746e3e2b,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"order_by",offsetof(csm_node_attributes_query_history_input_t,order_by),0,NULL,0x245553bb,68}}
;

void* cast_csm_node_attributes_query_history_input_t(void* ptr,size_t index) { 
    csm_node_attributes_query_history_input_t ** ptr_cast = *(csm_node_attributes_query_history_input_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_node_attributes_query_history_input_t= {
    6,
    csm_node_attributes_query_history_input_tree,
    cast_csm_node_attributes_query_history_input_t
};

const csmi_struct_node_t csm_node_attributes_query_history_output_tree[2] = {{"results_count",offsetof(csm_node_attributes_query_history_output_t,results_count),0,NULL,0x8261013f,24},
{"results",offsetof(csm_node_attributes_query_history_output_t,results),offsetof(csm_node_attributes_query_history_output_t, results_count),&map_csmi_node_attributes_history_record_t,0x3f2ab7f7,1}}
;

void* cast_csm_node_attributes_query_history_output_t(void* ptr,size_t index) { 
    csm_node_attributes_query_history_output_t ** ptr_cast = *(csm_node_attributes_query_history_output_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_node_attributes_query_history_output_t= {
    2,
    csm_node_attributes_query_history_output_tree,
    cast_csm_node_attributes_query_history_output_t
};

const csmi_struct_node_t csm_node_query_state_history_input_tree[6] = {{"offset",offsetof(csm_node_query_state_history_input_t,offset),0,NULL,0x123b4b4c,36},
{"limit",offsetof(csm_node_query_state_history_input_t,limit),0,NULL,0xfdcc804,36},
{"node_name",offsetof(csm_node_query_state_history_input_t,node_name),0,NULL,0x746e3e2b,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"order_by",offsetof(csm_node_query_state_history_input_t,order_by),0,NULL,0x245553bb,68}}
;

void* cast_csm_node_query_state_history_input_t(void* ptr,size_t index) { 
    csm_node_query_state_history_input_t ** ptr_cast = *(csm_node_query_state_history_input_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_node_query_state_history_input_t= {
    6,
    csm_node_query_state_history_input_tree,
    cast_csm_node_query_state_history_input_t
};

const csmi_struct_node_t csm_node_query_state_history_output_tree[3] = {{"node_name",offsetof(csm_node_query_state_history_output_t,node_name),0,NULL,0x746e3e2b,4},
{"results",offsetof(csm_node_query_state_history_output_t,results),offsetof(csm_node_query_state_history_output_t, results_count),&map_csmi_node_query_state_history_record_t,0x3f2ab7f7,1},
{"results_count",offsetof(csm_node_query_state_history_output_t,results_count),0,NULL,0x8261013f,24}}
;

void* cast_csm_node_query_state_history_output_t(void* ptr,size_t index) { 
    csm_node_query_state_history_output_t ** ptr_cast = *(csm_node_query_state_history_output_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_node_query_state_history_output_t= {
    3,
    csm_node_query_state_history_output_tree,
    cast_csm_node_query_state_history_output_t
};

const csmi_struct_node_t csm_node_attributes_update_input_tree[15] = {{"comment",offsetof(csm_node_attributes_update_input_t,comment),0,NULL,0xd363aa58,4},
{"state",offsetof(csm_node_attributes_update_input_t,state),csmi_node_state_t_MAX,&csmi_node_state_t_strs,0x10614a06,8},
{"feature_2",offsetof(csm_node_attributes_update_input_t,feature_2),0,NULL,0xd3976c22,4},
{"ready",offsetof(csm_node_attributes_update_input_t,ready),0,NULL,0x1046f5da,16},
{"physical_frame_location",offsetof(csm_node_attributes_update_input_t,physical_frame_location),0,NULL,0x7762c5a4,4},
{"feature_1",offsetof(csm_node_attributes_update_input_t,feature_1),0,NULL,0xd3976c21,4},
{"feature_3",offsetof(csm_node_attributes_update_input_t,feature_3),0,NULL,0xd3976c23,4},
{"node_names",offsetof(csm_node_attributes_update_input_t,node_names),offsetof(csm_node_attributes_update_input_t, node_names_count),NULL,0x23603fe,5},
{NULL,0,0,NULL,0,0},
{"physical_u_location",offsetof(csm_node_attributes_update_input_t,physical_u_location),0,NULL,0x5783af2e,4},
{"node_names_count",offsetof(csm_node_attributes_update_input_t,node_names_count),0,NULL,0x868cf686,24},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"feature_4",offsetof(csm_node_attributes_update_input_t,feature_4),0,NULL,0xd3976c24,4}}
;

void* cast_csm_node_attributes_update_input_t(void* ptr,size_t index) { 
    csm_node_attributes_update_input_t ** ptr_cast = *(csm_node_attributes_update_input_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_node_attributes_update_input_t= {
    15,
    csm_node_attributes_update_input_tree,
    cast_csm_node_attributes_update_input_t
};

const csmi_struct_node_t csm_node_attributes_update_output_tree[3] = {{"failure_count",offsetof(csm_node_attributes_update_output_t,failure_count),0,NULL,0xb64de7b5,24},
{NULL,0,0,NULL,0,0},
{"failure_node_names",offsetof(csm_node_attributes_update_output_t,failure_node_names),offsetof(csm_node_attributes_update_output_t, failure_count),NULL,0xca9637a5,5}}
;

void* cast_csm_node_attributes_update_output_t(void* ptr,size_t index) { 
    csm_node_attributes_update_output_t ** ptr_cast = *(csm_node_attributes_update_output_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_node_attributes_update_output_t= {
    3,
    csm_node_attributes_update_output_tree,
    cast_csm_node_attributes_update_output_t
};

const csmi_struct_node_t csm_node_delete_input_tree[2] = {{"node_names_count",offsetof(csm_node_delete_input_t,node_names_count),0,NULL,0x868cf686,24},
{"node_names",offsetof(csm_node_delete_input_t,node_names),offsetof(csm_node_delete_input_t, node_names_count),NULL,0x23603fe,5}}
;

void* cast_csm_node_delete_input_t(void* ptr,size_t index) { 
    csm_node_delete_input_t ** ptr_cast = *(csm_node_delete_input_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_node_delete_input_t= {
    2,
    csm_node_delete_input_tree,
    cast_csm_node_delete_input_t
};

const csmi_struct_node_t csm_node_delete_output_tree[3] = {{"failure_count",offsetof(csm_node_delete_output_t,failure_count),0,NULL,0xb64de7b5,24},
{NULL,0,0,NULL,0,0},
{"failure_node_names",offsetof(csm_node_delete_output_t,failure_node_names),offsetof(csm_node_delete_output_t, failure_count),NULL,0xca9637a5,5}}
;

void* cast_csm_node_delete_output_t(void* ptr,size_t index) { 
    csm_node_delete_output_t ** ptr_cast = *(csm_node_delete_output_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_node_delete_output_t= {
    3,
    csm_node_delete_output_tree,
    cast_csm_node_delete_output_t
};

const csmi_struct_node_t csm_switch_attributes_query_input_tree[11] = {{"limit",offsetof(csm_switch_attributes_query_input_t,limit),0,NULL,0xfdcc804,36},
{"switch_names_count",offsetof(csm_switch_attributes_query_input_t,switch_names_count),0,NULL,0x4da7952,24},
{"offset",offsetof(csm_switch_attributes_query_input_t,offset),0,NULL,0x123b4b4c,36},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"state",offsetof(csm_switch_attributes_query_input_t,state),0,NULL,0x10614a06,4},
{"switch_names",offsetof(csm_switch_attributes_query_input_t,switch_names),offsetof(csm_switch_attributes_query_input_t, switch_names_count),NULL,0x4d7c5dca,5},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"order_by",offsetof(csm_switch_attributes_query_input_t,order_by),0,NULL,0x245553bb,68},
{"serial_number",offsetof(csm_switch_attributes_query_input_t,serial_number),0,NULL,0xd931f68d,4}}
;

void* cast_csm_switch_attributes_query_input_t(void* ptr,size_t index) { 
    csm_switch_attributes_query_input_t ** ptr_cast = *(csm_switch_attributes_query_input_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_switch_attributes_query_input_t= {
    11,
    csm_switch_attributes_query_input_tree,
    cast_csm_switch_attributes_query_input_t
};

const csmi_struct_node_t csm_switch_attributes_query_output_tree[2] = {{"results_count",offsetof(csm_switch_attributes_query_output_t,results_count),0,NULL,0x8261013f,24},
{"results",offsetof(csm_switch_attributes_query_output_t,results),offsetof(csm_switch_attributes_query_output_t, results_count),&map_csmi_switch_record_t,0x3f2ab7f7,1}}
;

void* cast_csm_switch_attributes_query_output_t(void* ptr,size_t index) { 
    csm_switch_attributes_query_output_t ** ptr_cast = *(csm_switch_attributes_query_output_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_switch_attributes_query_output_t= {
    2,
    csm_switch_attributes_query_output_tree,
    cast_csm_switch_attributes_query_output_t
};

const csmi_struct_node_t csm_switch_attributes_query_details_input_tree[1] = {{"switch_name",offsetof(csm_switch_attributes_query_details_input_t,switch_name),0,NULL,0x482a8e77,4}}
;

void* cast_csm_switch_attributes_query_details_input_t(void* ptr,size_t index) { 
    csm_switch_attributes_query_details_input_t ** ptr_cast = *(csm_switch_attributes_query_details_input_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_switch_attributes_query_details_input_t= {
    1,
    csm_switch_attributes_query_details_input_tree,
    cast_csm_switch_attributes_query_details_input_t
};

const csmi_struct_node_t csm_switch_attributes_query_details_output_tree[2] = {{"result_count",offsetof(csm_switch_attributes_query_details_output_t,result_count),0,NULL,0x4236760c,44},
{"result",offsetof(csm_switch_attributes_query_details_output_t,result),offsetof(csm_switch_attributes_query_details_output_t, result_count),&map_csmi_switch_details_t,0x192fd704,1}}
;

void* cast_csm_switch_attributes_query_details_output_t(void* ptr,size_t index) { 
    csm_switch_attributes_query_details_output_t ** ptr_cast = *(csm_switch_attributes_query_details_output_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_switch_attributes_query_details_output_t= {
    2,
    csm_switch_attributes_query_details_output_tree,
    cast_csm_switch_attributes_query_details_output_t
};

const csmi_struct_node_t csm_switch_attributes_query_history_input_tree[3] = {{"offset",offsetof(csm_switch_attributes_query_history_input_t,offset),0,NULL,0x123b4b4c,36},
{"limit",offsetof(csm_switch_attributes_query_history_input_t,limit),0,NULL,0xfdcc804,36},
{"switch_name",offsetof(csm_switch_attributes_query_history_input_t,switch_name),0,NULL,0x482a8e77,4}}
;

void* cast_csm_switch_attributes_query_history_input_t(void* ptr,size_t index) { 
    csm_switch_attributes_query_history_input_t ** ptr_cast = *(csm_switch_attributes_query_history_input_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_switch_attributes_query_history_input_t= {
    3,
    csm_switch_attributes_query_history_input_tree,
    cast_csm_switch_attributes_query_history_input_t
};

const csmi_struct_node_t csm_switch_attributes_query_history_output_tree[2] = {{"results_count",offsetof(csm_switch_attributes_query_history_output_t,results_count),0,NULL,0x8261013f,24},
{"results",offsetof(csm_switch_attributes_query_history_output_t,results),offsetof(csm_switch_attributes_query_history_output_t, results_count),&map_csmi_switch_history_record_t,0x3f2ab7f7,1}}
;

void* cast_csm_switch_attributes_query_history_output_t(void* ptr,size_t index) { 
    csm_switch_attributes_query_history_output_t ** ptr_cast = *(csm_switch_attributes_query_history_output_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_switch_attributes_query_history_output_t= {
    2,
    csm_switch_attributes_query_history_output_tree,
    cast_csm_switch_attributes_query_history_output_t
};

const csmi_struct_node_t csm_switch_attributes_update_input_tree[10] = {{"physical_frame_location",offsetof(csm_switch_attributes_update_input_t,physical_frame_location),0,NULL,0x7762c5a4,4},
{"state",offsetof(csm_switch_attributes_update_input_t,state),0,NULL,0x10614a06,4},
{"comment",offsetof(csm_switch_attributes_update_input_t,comment),0,NULL,0xd363aa58,4},
{"switch_names_count",offsetof(csm_switch_attributes_update_input_t,switch_names_count),0,NULL,0x4da7952,24},
{"physical_u_location",offsetof(csm_switch_attributes_update_input_t,physical_u_location),0,NULL,0x5783af2e,4},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{NULL,0,0,NULL,0,0},
{"switch_names",offsetof(csm_switch_attributes_update_input_t,switch_names),offsetof(csm_switch_attributes_update_input_t, switch_names_count),NULL,0x4d7c5dca,5}}
;

void* cast_csm_switch_attributes_update_input_t(void* ptr,size_t index) { 
    csm_switch_attributes_update_input_t ** ptr_cast = *(csm_switch_attributes_update_input_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_switch_attributes_update_input_t= {
    10,
    csm_switch_attributes_update_input_tree,
    cast_csm_switch_attributes_update_input_t
};

const csmi_struct_node_t csm_switch_attributes_update_output_tree[3] = {{"failure_count",offsetof(csm_switch_attributes_update_output_t,failure_count),0,NULL,0xb64de7b5,24},
{NULL,0,0,NULL,0,0},
{"failure_switches",offsetof(csm_switch_attributes_update_output_t,failure_switches),offsetof(csm_switch_attributes_update_output_t, failure_count),NULL,0xbe921c96,5}}
;

void* cast_csm_switch_attributes_update_output_t(void* ptr,size_t index) { 
    csm_switch_attributes_update_output_t ** ptr_cast = *(csm_switch_attributes_update_output_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_switch_attributes_update_output_t= {
    3,
    csm_switch_attributes_update_output_tree,
    cast_csm_switch_attributes_update_output_t
};

const csmi_struct_node_t csm_switch_inventory_collection_input_tree[3] = {{"inventory_count",offsetof(csm_switch_inventory_collection_input_t,inventory_count),0,NULL,0xde0c63b,24},
{NULL,0,0,NULL,0,0},
{"inventory",offsetof(csm_switch_inventory_collection_input_t,inventory),offsetof(csm_switch_inventory_collection_input_t, inventory_count),&map_csmi_switch_details_t,0xac696ff3,1}}
;

void* cast_csm_switch_inventory_collection_input_t(void* ptr,size_t index) { 
    csm_switch_inventory_collection_input_t ** ptr_cast = *(csm_switch_inventory_collection_input_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_switch_inventory_collection_input_t= {
    3,
    csm_switch_inventory_collection_input_tree,
    cast_csm_switch_inventory_collection_input_t
};

const csmi_struct_node_t csm_switch_inventory_collection_output_tree[1] = {{"TBD",offsetof(csm_switch_inventory_collection_output_t,TBD),0,NULL,0xb8820ff,68}}
;

void* cast_csm_switch_inventory_collection_output_t(void* ptr,size_t index) { 
    csm_switch_inventory_collection_output_t ** ptr_cast = *(csm_switch_inventory_collection_output_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_switch_inventory_collection_output_t= {
    1,
    csm_switch_inventory_collection_output_tree,
    cast_csm_switch_inventory_collection_output_t
};

const csmi_struct_node_t csm_switch_children_inventory_collection_input_tree[3] = {{"inventory_count",offsetof(csm_switch_children_inventory_collection_input_t,inventory_count),0,NULL,0xde0c63b,44},
{NULL,0,0,NULL,0,0},
{"inventory",offsetof(csm_switch_children_inventory_collection_input_t,inventory),offsetof(csm_switch_children_inventory_collection_input_t, inventory_count),&map_csmi_switch_details_t,0xac696ff3,1}}
;

void* cast_csm_switch_children_inventory_collection_input_t(void* ptr,size_t index) { 
    csm_switch_children_inventory_collection_input_t ** ptr_cast = *(csm_switch_children_inventory_collection_input_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_switch_children_inventory_collection_input_t= {
    3,
    csm_switch_children_inventory_collection_input_tree,
    cast_csm_switch_children_inventory_collection_input_t
};

const csmi_struct_node_t csm_switch_children_inventory_collection_output_tree[1] = {{"TBD",offsetof(csm_switch_children_inventory_collection_output_t,TBD),0,NULL,0xb8820ff,68}}
;

void* cast_csm_switch_children_inventory_collection_output_t(void* ptr,size_t index) { 
    csm_switch_children_inventory_collection_output_t ** ptr_cast = *(csm_switch_children_inventory_collection_output_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_switch_children_inventory_collection_output_t= {
    1,
    csm_switch_children_inventory_collection_output_tree,
    cast_csm_switch_children_inventory_collection_output_t
};

const csmi_struct_node_t csm_cluster_query_state_input_tree[7] = {{"offset",offsetof(csm_cluster_query_state_input_t,offset),0,NULL,0x123b4b4c,36},
{"limit",offsetof(csm_cluster_query_state_input_t,limit),0,NULL,0xfdcc804,36},
{"num_allocs",offsetof(csm_cluster_query_state_input_t,num_allocs),0,NULL,0x421fea12,36},
{NULL,0,0,NULL,0,0},
{"state",offsetof(csm_cluster_query_state_input_t,state),csmi_node_state_t_MAX,&csmi_node_state_t_strs,0x10614a06,8},
{"order_by",offsetof(csm_cluster_query_state_input_t,order_by),0,NULL,0x245553bb,68},
{"type",offsetof(csm_cluster_query_state_input_t,type),csmi_node_type_t_MAX,&csmi_node_type_t_strs,0x7c9ebd07,8}}
;

void* cast_csm_cluster_query_state_input_t(void* ptr,size_t index) { 
    csm_cluster_query_state_input_t ** ptr_cast = *(csm_cluster_query_state_input_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_cluster_query_state_input_t= {
    7,
    csm_cluster_query_state_input_tree,
    cast_csm_cluster_query_state_input_t
};

const csmi_struct_node_t csm_cluster_query_state_output_tree[2] = {{"results_count",offsetof(csm_cluster_query_state_output_t,results_count),0,NULL,0x8261013f,24},
{"results",offsetof(csm_cluster_query_state_output_t,results),offsetof(csm_cluster_query_state_output_t, results_count),&map_csmi_cluster_query_state_record_t,0x3f2ab7f7,1}}
;

void* cast_csm_cluster_query_state_output_t(void* ptr,size_t index) { 
    csm_cluster_query_state_output_t ** ptr_cast = *(csm_cluster_query_state_output_t***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
const csmi_struct_mapping_t map_csm_cluster_query_state_output_t= {
    2,
    csm_cluster_query_state_output_tree,
    cast_csm_cluster_query_state_output_t
};

