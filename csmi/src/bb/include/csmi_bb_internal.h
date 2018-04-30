/*================================================================================
   
    csmi/src/bb/include/csmi_bb_internal.h

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
#ifndef _CSMI_BB_INTERNAL_H_
#define _CSMI_BB_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "csmi/include/csmi_type_bb.h"
#include "csmi/src/common/include/csmi_struct_hash.h"
extern const csmi_struct_mapping_t map_csmi_vg_record_t;

extern const csmi_struct_mapping_t map_csmi_lv_record_t;

extern const csmi_struct_mapping_t map_csmi_bb_vg_ssd_info_t;

extern const csmi_struct_mapping_t map_csm_bb_cmd_input_t;

extern const csmi_struct_mapping_t map_csm_bb_cmd_output_t;

extern const csmi_struct_mapping_t map_csm_bb_lv_create_input_t;

extern const csmi_struct_mapping_t map_csm_bb_lv_delete_input_t;

extern const csmi_struct_mapping_t map_csm_bb_lv_query_input_t;

extern const csmi_struct_mapping_t map_csm_bb_lv_query_output_t;

extern const csmi_struct_mapping_t map_csm_bb_lv_update_input_t;

extern const csmi_struct_mapping_t map_csm_bb_vg_create_input_t;

extern const csmi_struct_mapping_t map_csm_bb_vg_delete_input_t;

extern const csmi_struct_mapping_t map_csm_bb_vg_delete_output_t;

extern const csmi_struct_mapping_t map_csm_bb_vg_query_input_t;

extern const csmi_struct_mapping_t map_csm_bb_vg_query_output_t;

extern const csmi_struct_mapping_t map_csmi_bb_cmd_payload_t;


#ifdef __cplusplus
}
#endif
#endif
