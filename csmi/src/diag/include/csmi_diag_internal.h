/*================================================================================
   
    csmi/src/diag/include/csmi_diag_internal.h

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
#ifndef _CSMI_DIAG_INTERNAL_H_
#define _CSMI_DIAG_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "csmi/include/csmi_type_diag.h"
#include "csmi/src/common/include/csmi_struct_hash.h"
extern const csmi_struct_mapping_t map_csmi_diag_run_t;

extern const csmi_struct_mapping_t map_csmi_diag_run_query_details_result_t;

extern const csmi_struct_mapping_t map_csm_diag_run_end_input_t;

extern const csmi_struct_mapping_t map_csm_diag_result_create_input_t;

extern const csmi_struct_mapping_t map_csm_diag_run_begin_input_t;

extern const csmi_struct_mapping_t map_csm_diag_run_query_input_t;

extern const csmi_struct_mapping_t map_csm_diag_run_query_output_t;

extern const csmi_struct_mapping_t map_csm_diag_run_query_details_input_t;

extern const csmi_struct_mapping_t map_csm_diag_run_query_details_output_t;


#ifdef __cplusplus
}
#endif
#endif
