/*================================================================================
   
    csmi/src/common/src/csmi_common_internal.c

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
#include "csmi/src/common/include/csmi_common_internal.h"
#include "csmi/src/common/include/csmi_common_type_internal.h"
const csmi_struct_node_t csm_node_error_tree[3] = {{"errcode",offsetof(csm_node_error_t,errcode),0,NULL,0x74acc5a9,44},
{"source",offsetof(csm_node_error_t,source),0,NULL,0x1c3aff76,4},
{"errmsg",offsetof(csm_node_error_t,errmsg),0,NULL,0xfbc757d5,4}}
;

void* cast_csm_node_error_t(void* ptr,size_t index, char isArray) { 
    csm_node_error_t ** ptr_cast = *(csm_node_error_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csm_node_error_t= {
    3,
    csm_node_error_tree,
    cast_csm_node_error_t
};

const csmi_struct_node_t csmi_err_tree[4] = {{"error_count",offsetof(csmi_err_t,error_count),0,NULL,0xd1ef0537,24},
{"errcode",offsetof(csmi_err_t,errcode),0,NULL,0x74acc5a9,44},
{"errmsg",offsetof(csmi_err_t,errmsg),0,NULL,0xfbc757d5,4},
{"node_errors",offsetof(csmi_err_t,node_errors),offsetof(csmi_err_t, error_count),&map_csm_node_error_t,0x352e9f67,1}}
;

void* cast_csmi_err_t(void* ptr,size_t index, char isArray) { 
    csmi_err_t ** ptr_cast = *(csmi_err_t***)ptr;
    return ptr_cast && isArray ? ptr_cast[index] : (void*)ptr_cast;
};
const csmi_struct_mapping_t map_csmi_err_t= {
    4,
    csmi_err_tree,
    cast_csmi_err_t
};

const int csm_min_printable_type = 5;
const int csm_type_formatter_len = 19;
