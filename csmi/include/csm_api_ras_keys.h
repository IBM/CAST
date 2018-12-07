/*================================================================================

    csmi/include/csm_api_ras_keys.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSM_API_RAS_KEYS_H__
#define __CSM_API_RAS_KEYS_H__

#ifdef __cplusplus
extern "C" {    
#endif

// ras components:
#define CSM_RAS_SRC_BMC        "bmc"        // Events created by ibmpowerhwmon from BMC system event logs (SELs)
#define CSM_RAS_SRC_CSM        "csm"        // Events detected internally by CSM
#define CSM_RAS_SRC_HCDIAG     "hcdiag"     // Events created by hcdiag
#define CSM_RAS_SRC_GPU        "gpu"        // Events related to GPUs, primarily XID errors

// CSM RAS msg_id strings:
#define CSM_RAS_MSG_ID_STATUS_UP               "csm.status.up"
#define CSM_RAS_MSG_ID_STATUS_DOWN             "csm.status.down"
#define CSM_RAS_MSG_ID_STATUS_LOST_REDUNDANCY  "csm.status.lost_redundancy"
#define CSM_RAS_MSG_ID_STATUS_FULL_REDUNDANCY  "csm.status.full_redundancy"
#define CSM_RAS_MSG_ID_CGROUP_CREATE_FAILURE   "csm.api.cgroup_create_failure"
#define CSM_RAS_MSG_ID_CGROUP_DELETE_FAILURE   "csm.api.cgroup_delete_failure"
#define CSM_RAS_MSG_ID_ALLOCATION_TIMEOUT      "csm.api.allocation_timeout"
#define CSM_RAS_MSG_ID_RETRY_EXCEEDED          "csm.soft_failure_recovery.retries_exceeded"


#define CSM_RAS_PROLOG_PROLOG_COLLISION        "csm.api.prolog_prolog_collision"
#define CSM_RAS_PROLOG_EPILOG_COLLISION        "csm.api.prolog_epilog_collision"
#define CSM_RAS_EPILOG_PROLOG_COLLISION        "csm.api.epilog_prolog_collision"
#define CSM_RAS_EPILOG_EPILOG_COLLISION        "csm.api.epilog_epilog_collision"


// severity goes on the end of the pub/sub topic...
// proposed ras severites
#define CSM_RAS_SEV_INFO_S  "INFO"   
#define CSM_RAS_SEV_WARN_S  "WARN" 
#define CSM_RAS_SEV_ERROR_S "ERROR"  
#define CSM_RAS_SEV_FATAL_S "FATAL"  

// Reserved case insensitive keywords
#define CSM_RAS_NONE "NONE"
 
// proposed topic hierachy 
// /CSM/ras/<category>/<component>/<id>/<severity>

// creating a ras id, we supply
// severity, categories, component, (then what??)

// severity is added automatically when the ras event is created..


// field identifiers:
#define CSM_RAS_FKEY_REC_ID          "rec_id"
#define CSM_RAS_FKEY_MSG_ID          "msg_id"
#define CSM_RAS_FKEY_SEVERITY        "severity"
#define CSM_RAS_FKEY_TIME_STAMP      "time_stamp"
#define CSM_RAS_FKEY_MASTER_TIME_STAMP "master_time_stamp"
#define CSM_RAS_FKEY_START_TIME_STAMP      "start_time_stamp"
#define CSM_RAS_FKEY_END_TIME_STAMP      "end_time_stamp"
#define CSM_RAS_FKEY_LOCATION_NAME   "location_name"
#define CSM_RAS_FKEY_PROCESSOR       "processor"
#define CSM_RAS_FKEY_COUNT           "count"
#define CSM_RAS_FKEY_CONTROL_ACTION  "control_action"
#define CSM_RAS_FKEY_IMMEDIATE_ACTION "immediate_action"
#define CSM_RAS_FKEY_MESSAGE         "message"
#define CSM_RAS_FKEY_RAW_DATA        "raw_data"

#define CSM_RAS_FKEY_CATEGORY        "category"
#define CSM_RAS_FKEY_COMPONENT       "component"

#define CSM_RAS_FKEY_MIN_TIME_IN_POOL    "min_time_in_pool"
#define CSM_RAS_FKEY_SUPPRESS_IDS        "suppress_ids"
#define CSM_RAS_FKEY_SUPPRESSED     "suppressed"

#define CSM_RAS_FKEY_KVCSV           "kvcsv"

#define CSM_RAS_FKEY_CTXID           "ctxid"        // internal context identifier...


// fields unique to msgtype...
#define CSM_RAS_FKEY_DESCRIPTION         "description"
#define CSM_RAS_FKEY_DECODER             "decoder"
#define CSM_RAS_FKEY_THRESHOLD_COUNT     "threshold_count"
#define CSM_RAS_FKEY_THRESHOLD_PERIOD    "threshold_period"
#define CSM_RAS_FKEY_RELEVANT_DIAGS      "relevant_diags"
#define CSM_RAS_FKEY_ENABLED             "enabled"
#define CSM_RAS_FKEY_SET_STATE           "set_state"
#define CSM_RAS_FKEY_VISIBLE_TO_USERS    "visible_to_users"


#define CSM_RAS_FKEY_WHERE      "where"
#define CSM_RAS_FKEY_ORDERBY    "orderby"
#define CSM_RAS_FKEY_LIMIT      "limit"
#define CSM_RAS_FKEY_OFFSET     "offset"

#ifdef __cplusplus
}
#endif


#endif

