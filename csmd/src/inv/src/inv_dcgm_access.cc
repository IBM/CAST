/*================================================================================

    csmd/src/inv/src/inv_dcgm_access.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "inv_dcgm_access.h"
#include "logging.h"
#include "csm_bds_keys.h"

#include <iostream>
#include "string.h"
#include "unistd.h"
#include <time.h>
#include <vector>
#include <map>

#include "csm_daemon_config.h"

using namespace std;

csm::daemon::INV_DCGM_ACCESS *csm::daemon::INV_DCGM_ACCESS::_Instance = nullptr;

/////////////////////////////////////////////////////////////////////////////////
//// DCGM support is enabled
/////////////////////////////////////////////////////////////////////////////////

#ifdef DCGM

// GPU Group Names
char csm::daemon::INV_DCGM_ACCESS::CSM_GPU_GROUP[] = "CSM_GPU_GROUP";

// GPU Field Group Names
char csm::daemon::INV_DCGM_ACCESS::CSM_DOUBLE_FIELDS[] = "CSM_DOUBLE_FIELDS";
char csm::daemon::INV_DCGM_ACCESS::CSM_INT64_FIELDS[] = "CSM_INT64_FIELDS";
char csm::daemon::INV_DCGM_ACCESS::CSM_ENVIRONMENTAL_FIELD_GROUP[] = "CSM_ENVIRONMENTAL_FIELD_GROUP";
char csm::daemon::INV_DCGM_ACCESS::CSM_ALLOCATION_FIELD_GROUP[] = "CSM_ALLOCATION_FIELD_GROUP";
      
// GPU Fields for CSM_ALLOCATION_FIELD_GROUP
uint16_t csm::daemon::INV_DCGM_ACCESS::CSM_ALLOCATION_FIELDS[] =
{
   DCGM_FI_DEV_TOTAL_ENERGY_CONSUMPTION,   // Total energy consumption for the GPU in mJ since the driver was last reloaded
   DCGM_FI_DEV_ECC_DBE_VOL_TOTAL,          // Total DBEs that have occurred since the driver was last reloaded. 
                                           // Action: Drain node and reboot it if this value is > 0.
   DCGM_FI_DEV_ECC_DBE_AGG_TOTAL,          // Total (aggregate) DBEs that have ever occurred on this GPU. 
                                           // Action: Track this value against the RMA thresholds in your documentation. 
   DCGM_FI_DEV_RETIRED_SBE,                // Number of pages ever retired due to multiple SBEs.
                                           // Action: Track this value against the RMA thresholds in your documentation.
   DCGM_FI_DEV_RETIRED_DBE                 // Number of pages ever retired due to DBEs.
                                           // Action: Track this value against the RMA thresholds in your documentation.
};

const uint32_t csm::daemon::INV_DCGM_ACCESS::CSM_ALLOCATION_FIELD_COUNT = 
   (sizeof(csm::daemon::INV_DCGM_ACCESS::CSM_ALLOCATION_FIELDS) / sizeof(uint16_t));
    
uint16_t csm::daemon::INV_DCGM_ACCESS::CSM_ENVIRONMENTAL_FIELDS[] =
{
   DCGM_FI_DEV_SERIAL,                            // Device Serial Number
   DCGM_FI_DEV_POWER_USAGE,                       // GPU power consumption, in Watts
   DCGM_FI_DEV_MEM_COPY_UTIL_SAMPLES,             // memory utilization samples
   DCGM_FI_DEV_GPU_UTIL_SAMPLES,                  // gpu utilization samples
   DCGM_FI_DEV_GPU_TEMP,                          // GPU temperature, in degrees C
   DCGM_FI_DEV_GPU_UTIL,                          // GPU utilization
   DCGM_FI_DEV_MEM_COPY_UTIL,                     // GPU memory utilization
   DCGM_FI_DEV_ENC_UTIL,                          // GPU encoder utilization
   DCGM_FI_DEV_DEC_UTIL,                          // GPU decoder utilizatioin
   DCGM_FI_DEV_NVLINK_BANDWIDTH_L0,               // GPU NVLINK bandwidth counter for line 0
   DCGM_FI_DEV_NVLINK_BANDWIDTH_L1,               // GPU NVLINK bandwidth counter for line 1
   DCGM_FI_DEV_NVLINK_BANDWIDTH_L2,               // GPU NVLINK bandwidth counter for line 2
   DCGM_FI_DEV_NVLINK_BANDWIDTH_L3,               // GPU NVLINK bandwidth counter for line 3
   DCGM_FI_DEV_NVLINK_CRC_FLIT_ERROR_COUNT_L0,    // GPU NV link flow control CRC error for lane 0
   DCGM_FI_DEV_NVLINK_CRC_FLIT_ERROR_COUNT_L1,    // GPU NV link flow control CRC error for lane 1
   DCGM_FI_DEV_NVLINK_CRC_FLIT_ERROR_COUNT_L2,    // GPU NV link flow control CRC error for lane 2
   DCGM_FI_DEV_NVLINK_CRC_FLIT_ERROR_COUNT_L3,    // GPU NV link flow control CRC error for lane 3
   DCGM_FI_DEV_NVLINK_CRC_DATA_ERROR_COUNT_L0,    // GPU NV link data CRC error for lane 0
   DCGM_FI_DEV_NVLINK_CRC_DATA_ERROR_COUNT_L1,    // GPU NV link data CRC error for lane 1
   DCGM_FI_DEV_NVLINK_CRC_DATA_ERROR_COUNT_L2,    // GPU NV link data CRC error for lane 2
   DCGM_FI_DEV_NVLINK_CRC_DATA_ERROR_COUNT_L3,    // GPU NV link data CRC error for lane 3
   DCGM_FI_DEV_NVLINK_REPLAY_ERROR_COUNT_L0,      // GPU NV link replay error counter for lane 0
   DCGM_FI_DEV_NVLINK_REPLAY_ERROR_COUNT_L1,      // GPU NV link replay error counter for lane 1
   DCGM_FI_DEV_NVLINK_REPLAY_ERROR_COUNT_L2,      // GPU NV link replay error counter for lane 2
   DCGM_FI_DEV_NVLINK_REPLAY_ERROR_COUNT_L3,      // GPU NV link replay error counter for lane 3
   DCGM_FI_DEV_NVLINK_RECOVERY_ERROR_COUNT_L0,    // GPU NV link recovery error for lane 0
   DCGM_FI_DEV_NVLINK_RECOVERY_ERROR_COUNT_L1,    // GPU NV link recovery error for lane 1
   DCGM_FI_DEV_NVLINK_RECOVERY_ERROR_COUNT_L2,    // GPU NV link recovery error for lane 2
   DCGM_FI_DEV_NVLINK_RECOVERY_ERROR_COUNT_L3,    // GPU NV link recovery error for lane 3
   DCGM_FI_DEV_POWER_VIOLATION,                   // GPU power violation in usecs
   DCGM_FI_DEV_THERMAL_VIOLATION,                 // GPU thermal power violation in usecs
   DCGM_FI_DEV_SYNC_BOOST_VIOLATION               // GPU boost sync violation in usecs
};

const uint32_t csm::daemon::INV_DCGM_ACCESS::CSM_ENVIRONMENTAL_FIELD_COUNT = 
   (sizeof(csm::daemon::INV_DCGM_ACCESS::CSM_ENVIRONMENTAL_FIELDS) / sizeof(uint16_t));

csm::daemon::INV_DCGM_ACCESS::INV_DCGM_ACCESS()
{

    // initialization
    Init();
    
    // get attributes for the GPUs of the node
    if ( !dlopen_flag && dcgm_init_flag ){
	this->GetGPUsAttributes();
    }

}

csm::daemon::INV_DCGM_ACCESS::~INV_DCGM_ACCESS()
{
    dcgmReturn_t rc(DCGM_ST_OK);

    // checking if dlopen and dcgm init functions were successfull
    if ( !dlopen_flag && dcgm_init_flag )
    {

     // finalize DCGM environment
     rc = (*dcgmFieldGroupDestroy_ptr)(dcgm_handle, double_fields_grp);
     if (rc != DCGM_ST_OK)
     {
	LOG(csmd, error) << "Error: dcgmFieldGroupDestroy returned \"" << errorString(rc) << "(" << rc << ")\"";
     } else {
        LOG(csmd, debug) << "dcgmFieldGroupDestroy was successful";
     }

     rc = (*dcgmFieldGroupDestroy_ptr)(dcgm_handle, int64_fields_grp);
     if (rc != DCGM_ST_OK)
     {
        LOG(csmd, error) << "Error: dcgmFieldGroupDestroy returned \"" << errorString(rc) << "(" << rc << ")\"";
     } else {
        LOG(csmd, debug) << "dcgmFieldGroupDestroy was successful";
     }

     rc = (*dcgmGroupDestroy_ptr)(dcgm_handle, gpugrp);
     if (rc != DCGM_ST_OK)
     {
	LOG(csmd, error) << "Error: dcgmGroupDestroy returned \"" << errorString(rc) << "(" << rc << ")\"";
     } else {
        LOG(csmd, debug) << "dcgmGroupDestroy was successful";
     }

     rc = (*dcgmDisconnect_ptr)(dcgm_handle);
     if (rc != DCGM_ST_OK)
     {
        LOG(csmd, error) << "Error: dcgmDisconnect returned \"" << errorString(rc) << "(" << rc << ")\"";
     } else {
        LOG(csmd, debug) << "dcgmDisconnect was successful";
     }

     rc = (*dcgmShutdown_ptr)();
     if (rc != DCGM_ST_OK)
     {
	LOG(csmd, error) << "Error: dcgmShutdown returned \"" << errorString(rc) << "(" << rc << ")\"";
     } else {
        LOG(csmd, debug) << "dcgmShutdown was successful";
     }

    }

    vector_of_ids_of_double_fields.clear();
    vector_of_ids_of_int64_fields.clear();

    vector_of_double_values.clear();
    vector_of_int64_values.clear();

    vector_old_double_fields_value.clear();
    vector_old_int64_fields_value.clear();

    dcgm_meta_double_vector.clear();
    dcgm_meta_int64_vector.clear();

    vector_of_ids_of_double_fields.clear();
    vector_of_ids_of_int64_fields.clear();

    vector_for_double_values_storage.clear();
    vector_for_int64_values_storage.clear();

}

void csm::daemon::INV_DCGM_ACCESS::Init()
{
    // Only allow one thread to call DCGM at a time
    std::lock_guard<std::mutex> lock(dcgm_mutex);

    // alloc and init of some vectors and arrays
    number_of_double_fields = 3;
    number_of_int64_fields = 29;

    vector_of_double_values.resize( number_of_double_fields );
    vector_of_int64_values.resize( number_of_int64_fields );
    
    // flags
    dlopen_flag = false;
    dcgm_init_flag = true;
    
    // initialize function pointers
    libdcgm_ptr = nullptr;
    dcgmInit_ptr = nullptr;
    dcgmConnect_ptr = nullptr;
    dcgmGetAllDevices_ptr = nullptr;
    dcgmGroupGetAllIds_ptr = nullptr;
    dcgmGroupGetInfo_ptr = nullptr;
    dcgmFieldGroupGetAll_ptr = nullptr;
    dcgmFieldGroupGetInfo_ptr = nullptr;
    dcgmGroupCreate_ptr = nullptr;
    dcgmGroupAddDevice_ptr = nullptr;
    dcgmFieldGroupCreate_ptr = nullptr;
    dcgmWatchFields_ptr = nullptr;
    dcgmUnwatchFields_ptr = nullptr;
    dcgmUpdateAllFields_ptr = nullptr;
    dcgmGetDeviceAttributes_ptr = nullptr;
    dcgmGetLatestValuesForFields_ptr = nullptr;
    dcgmFieldGroupDestroy_ptr = nullptr;
    dcgmGroupDestroy_ptr = nullptr;
    dcgmDisconnect_ptr = nullptr;
    dcgmShutdown_ptr = nullptr;
    DcgmFieldGetById_ptr = nullptr;
    dcgmWatchJobFields_ptr = nullptr;
    dcgmJobStartStats_ptr = nullptr;
    dcgmJobStopStats_ptr = nullptr;      
    dcgmJobGetStats_ptr = nullptr;
    dcgmJobRemove_ptr = nullptr;

    // other variables
    // updateFreq = 1000000;
    // maxKeepAge = 0.5;
    // updateFreq = 1;
    // maxKeepAge = 1000;
    // starting by setting in seconds then scale later
    updateFreq = csm::daemon::Configuration::Instance()->GetTweaks()._DCGM_update_interval_s;
    maxKeepAge = updateFreq + 5;  // add 5s to make sure we keep data longer than the update interval
    updateFreq *= 1000000; // scale to the right unit
    maxKeepSamples = 1;

    // other variables
    dcgm_gpu_count = 0; 

    // load symbols into function pointers
    libdcgm_ptr = dlopen("/usr/lib64/libdcgm.so", RTLD_LAZY);
    if (libdcgm_ptr == nullptr )
    {
        LOG(csmd, info) << "dlopen() returned: " << dlerror();
        LOG(csmd, info) << "Couldn't load libdcgm.so, no GPU inventory will be returned.";
        dlopen_flag = true;
        dcgm_init_flag = false;
        return;
    } else {
        LOG(csmd, debug) << "libdcgm_ptr was successful";
    }

    // Attempt to dynamically load the symbols needed for the DCGM functions CSM uses
    if ( InitializeFunctionPointers() == false )
    {
        dcgm_init_flag = false;
        return;
    }
    else
    {
        LOG(csmd, info) << "InitializeFunctionPointers() successfully loaded symbols for DCGM functions";
    }

    // initialize DCGM environment
    dcgmReturn_t rc(DCGM_ST_OK);
    rc = (*dcgmInit_ptr)();
    if (rc != DCGM_ST_OK)
    {
	LOG(csmd, error) << "Error: dcgmInit returned \"" << errorString(rc) << "(" << rc << ")\"";
        dcgm_init_flag = false;
        return;
    } else {
        LOG(csmd, debug) << "dcgmInit was successful";
    }

    char * ipAddress = strdup("127.0.0.1");
    rc = (*dcgmConnect_ptr)(ipAddress,&dcgm_handle);
    free( ipAddress) ;
    if (rc != DCGM_ST_OK)
    {
        LOG(csmd, error) << "Error: dcgmConnect returned \"" << errorString(rc) << "(" << rc << ")\"";
        dcgm_init_flag = false;
        return;
    } else {
        LOG(csmd, debug) << "dcgmConnect was successful";
    }

    rc = (*dcgmGetAllDevices_ptr)(dcgm_handle, gpu_ids, &dcgm_gpu_count);
    if (rc != DCGM_ST_OK)
    {
	LOG(csmd, error) << "Error: dcgmGetAllDevices returned \"" << errorString(rc) << "(" << rc << ")\"";
        dcgm_init_flag = false;
        return;
    } else {
        LOG(csmd, debug) << "dcgmGetAllDevices was successful";
    }

    if (( dcgm_gpu_count < 0 ) || ( dcgm_gpu_count >= UINT32_MAX))
    {
	LOG(csmd, error) << "Error: dcgmGetAllDevices returned unexpected gpu_count=" << dcgm_gpu_count;
        dcgm_init_flag = false;
        return;
    } else {
	LOG(csmd, debug) << "dcgm_gpu_count: " << dcgm_gpu_count;
    }

    // setting vectors of the fields
    vector_of_ids_of_double_fields.push_back( DCGM_FI_DEV_POWER_USAGE );                       // GPU power consumption, in Watts
    vector_of_ids_of_double_fields.push_back( DCGM_FI_DEV_MEM_COPY_UTIL_SAMPLES );             // memory utilizaiton samples
    vector_of_ids_of_double_fields.push_back( DCGM_FI_DEV_GPU_UTIL_SAMPLES );                  // gpu utilization samples

    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_GPU_TEMP );                           // GPU temperature, in degrees C
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_GPU_UTIL );                           // GPU utilization
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_MEM_COPY_UTIL );                      // GPU memory utilization
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_ENC_UTIL );                           // GPU encoder utilization
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_DEC_UTIL );                           // GPU decoder utilizatioin
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_NVLINK_BANDWIDTH_L0 );                // GPU NVLINK bandwidth counter for line 0
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_NVLINK_BANDWIDTH_L1 );                // GPU NVLINK bandwidth counter for line 1
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_NVLINK_BANDWIDTH_L2 );                // GPU NVLINK bandwidth counter for line 2
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_NVLINK_BANDWIDTH_L3 );                // GPU NVLINK bandwidth counter for line 3
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_NVLINK_CRC_FLIT_ERROR_COUNT_L0 );     // GPU NV link flow control CRC error for lane 0
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_NVLINK_CRC_FLIT_ERROR_COUNT_L1 );     // GPU NV link flow control CRC error for lane 1
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_NVLINK_CRC_FLIT_ERROR_COUNT_L2 );     // GPU NV link flow control CRC error for lane 2
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_NVLINK_CRC_FLIT_ERROR_COUNT_L3 );     // GPU NV link flow control CRC error for lane 3
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_NVLINK_CRC_DATA_ERROR_COUNT_L0 );     // GPU NV link data CRC error for lane 0
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_NVLINK_CRC_DATA_ERROR_COUNT_L1 );     // GPU NV link data CRC error for lane 1
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_NVLINK_CRC_DATA_ERROR_COUNT_L2 );     // GPU NV link data CRC error for lane 2
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_NVLINK_CRC_DATA_ERROR_COUNT_L3 );     // GPU NV link data CRC error for lane 3
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_NVLINK_REPLAY_ERROR_COUNT_L0 );       // GPU NV link replay error counter for lane 0
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_NVLINK_REPLAY_ERROR_COUNT_L1 );       // GPU NV link replay error counter for lane 1
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_NVLINK_REPLAY_ERROR_COUNT_L2 );       // GPU NV link replay error counter for lane 2
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_NVLINK_REPLAY_ERROR_COUNT_L3 );       // GPU NV link replay error counter for lane 3
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_NVLINK_RECOVERY_ERROR_COUNT_L0 );     // GPU NV link recovery error for lane 0
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_NVLINK_RECOVERY_ERROR_COUNT_L1 );     // GPU NV link recovery error for lane 1
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_NVLINK_RECOVERY_ERROR_COUNT_L2 );     // GPU NV link recovery error for lane 2
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_NVLINK_RECOVERY_ERROR_COUNT_L3 );     // GPU NV link recovery error for lane 3
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_POWER_VIOLATION );                    // GPU power violation in usecs
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_THERMAL_VIOLATION );                  // GPU thermal power violation in usecs
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_SYNC_BOOST_VIOLATION );               // GPU boost sync violation in usecs
    vector_of_ids_of_int64_fields.push_back( DCGM_FI_DEV_XID_ERRORS );                         // gpu utilization samples

    // create group of gpus
    rc = (*dcgmGroupCreate_ptr)(dcgm_handle, DCGM_GROUP_EMPTY, CSM_GPU_GROUP, &gpugrp);
    if (rc != DCGM_ST_OK)
    {
	LOG(csmd, error) << "Error: dcgmGroupCreate returned \"" << errorString(rc) << "(" << rc << ")\"";
        dcgm_init_flag = false;
        return;
    } else {
        LOG(csmd, debug) << "dcgmGroupCreate was successful";
    }

    // add gpus to the group
    for (int i = 0; i < dcgm_gpu_count; i++){
     rc = (*dcgmGroupAddDevice_ptr)(dcgm_handle, gpugrp, gpu_ids[i]);
     if (rc != DCGM_ST_OK)
     {
	LOG(csmd, error) << "Error: dcgmGroupAddDevice returned \"" << errorString(rc) << "(" << rc << ")\"";
	dcgm_init_flag = false;
	return;
     } else {
        LOG(csmd, debug) << "dcgmGroupAddDevice was successful";
     }
    }

    // create group of double fields
    rc = (*dcgmFieldGroupCreate_ptr)(dcgm_handle, number_of_double_fields, &vector_of_ids_of_double_fields.front(), CSM_DOUBLE_FIELDS, &double_fields_grp);
    if (rc != DCGM_ST_OK)
    {
	LOG(csmd, error) << "Error: dcgmFieldGroupCreate returned \"" << errorString(rc) << "(" << rc << ")\"";
        dcgm_init_flag = false;
        return;
    } else {
        LOG(csmd, debug) << "dcgmFieldGroupCreate was successful";
    }

    // create group of int64 fields
    rc = (*dcgmFieldGroupCreate_ptr)(dcgm_handle, number_of_int64_fields, &vector_of_ids_of_int64_fields.front(), CSM_INT64_FIELDS, &int64_fields_grp);
    if (rc != DCGM_ST_OK)
    {
        LOG(csmd, error) << "Error: dcgmFieldGroupCreate returned \"" << errorString(rc) << "(" << rc << ")\"";
        dcgm_init_flag = false;
        return;
    } else {
        LOG(csmd, debug) << "dcgmFieldGroupCreate was successful";
    }

    // Create DCGM Field Groups used internally by CSM
    // Create the DCGM Field Group used for collecting data during allocation create and delete
    bool success(false);
    success = CreateCsmFieldGroup(CSM_ALLOCATION_FIELD_COUNT, CSM_ALLOCATION_FIELDS, CSM_ALLOCATION_FIELD_GROUP, &csm_allocation_field_group_handle);
    if (success == false)
    {
        dcgm_init_flag = false;
        return;
    }
    
    success = CreateCsmFieldGroup(CSM_ENVIRONMENTAL_FIELD_COUNT, CSM_ENVIRONMENTAL_FIELDS, CSM_ENVIRONMENTAL_FIELD_GROUP, 
       &csm_environmental_field_group_handle);
    if (success == false)
    {
        dcgm_init_flag = false;
        return;
    }

    // Read the field names associated with the DCGM Field Groups used internally by CSM
    success = ReadFieldNames(CSM_ALLOCATION_FIELD_COUNT, CSM_ALLOCATION_FIELDS, csm_allocation_field_names);
    if (success == false)
    {
        LOG(csmenv, warning) << "ReadFieldNames() returned false for CSM_ALLOCATION_FIELDS";
    }

    success = ReadFieldNames(CSM_ENVIRONMENTAL_FIELD_COUNT, CSM_ENVIRONMENTAL_FIELDS, csm_environmental_field_names);
    if (success == false)
    {
        LOG(csmenv, warning) << "ReadFieldNames() returned false for CSM_ENVIRONMENTAL_FIELDS";
    }

    // watcher, necessary for the manual update, at this moment update fields every 1000 secs (1000000000 us), keep them for 10 secs, and only 1 sample per field
    // watcher, necessary for the manual update, at this moment update fields every   60 secs (  60000000 us), keep them for 45 secs, and only 1 sample per field
    rc = (*dcgmWatchFields_ptr)(dcgm_handle, gpugrp, double_fields_grp, updateFreq, maxKeepAge, maxKeepSamples);
    if (rc != DCGM_ST_OK)
    {
	LOG(csmd, error) << "Error: dcgmWatchFields returned \"" << errorString(rc) << "(" << rc << ")\"";
        dcgm_init_flag = false;
        return;
    } else {
        LOG(csmd, debug) << "dcgmWatchFields was successful";
    }

    // watcher, necessary for the manual update, at this moment update fields every 1000 secs (1000000000 us), keep them for 10 secs, and only 1 sample per field
    rc = (*dcgmWatchFields_ptr)(dcgm_handle, gpugrp, int64_fields_grp, updateFreq, maxKeepAge, maxKeepSamples);
    if (rc != DCGM_ST_OK)
    {
        LOG(csmd, error) << "Error: dcgmWatchFields returned \"" << errorString(rc) << "(" << rc << ")\"";
        dcgm_init_flag = false;
        return;
    } else {
        LOG(csmd, debug) << "dcgmWatchFields was successful";
    }

    // setting vectors of the flags for the old data
    vector_flag_old_double_data.push_back( 0 ); //  0
    vector_flag_old_double_data.push_back( 1 ); //  1
    vector_flag_old_double_data.push_back( 1 ); //  2

    vector_flag_old_int64_data.push_back( 0 ); //  0
    vector_flag_old_int64_data.push_back( 0 ); //  1
    vector_flag_old_int64_data.push_back( 0 ); //  2
    vector_flag_old_int64_data.push_back( 0 ); //  3
    vector_flag_old_int64_data.push_back( 0 ); //  4
    vector_flag_old_int64_data.push_back( 0 ); //  5
    vector_flag_old_int64_data.push_back( 1 ); //  6
    vector_flag_old_int64_data.push_back( 1 ); //  7
    vector_flag_old_int64_data.push_back( 1 ); //  8
    vector_flag_old_int64_data.push_back( 1 ); //  9
    vector_flag_old_int64_data.push_back( 1 ); // 10
    vector_flag_old_int64_data.push_back( 1 ); // 11
    vector_flag_old_int64_data.push_back( 1 ); // 12
    vector_flag_old_int64_data.push_back( 1 ); // 13 
    vector_flag_old_int64_data.push_back( 1 ); // 14
    vector_flag_old_int64_data.push_back( 1 ); // 15
    vector_flag_old_int64_data.push_back( 1 ); // 16
    vector_flag_old_int64_data.push_back( 1 ); // 17
    vector_flag_old_int64_data.push_back( 1 ); // 18
    vector_flag_old_int64_data.push_back( 1 ); // 19
    vector_flag_old_int64_data.push_back( 1 ); // 20
    vector_flag_old_int64_data.push_back( 1 ); // 21
    vector_flag_old_int64_data.push_back( 1 ); // 22
    vector_flag_old_int64_data.push_back( 1 ); // 23
    vector_flag_old_int64_data.push_back( 1 ); // 24
    vector_flag_old_int64_data.push_back( 1 ); // 24
    vector_flag_old_int64_data.push_back( 1 ); // 25
    vector_flag_old_int64_data.push_back( 1 ); // 26
    vector_flag_old_int64_data.push_back( 1 ); // 27
    vector_flag_old_int64_data.push_back( 0 ); // 28

    // setting vectors of the old data values
    for (int i = 0; i < dcgm_gpu_count; i++) {
     vector_old_double_fields_value.push_back( vector<double>() );
     for (unsigned int j = 0; j < number_of_double_fields; j++) {
      vector_old_double_fields_value[i].push_back( 0.0 );
     }
    }

    for (int i = 0; i < dcgm_gpu_count; i++) {
     vector_old_int64_fields_value.push_back( vector<long>() );
     for (unsigned int j = 0; j < number_of_int64_fields; j++) {
      vector_old_int64_fields_value[i].push_back( 0 );
     }
    }

    // about the meta fields

    LOG(csmd, info) << "Printing the unique identifiers of the watched DCGM double fields";
    for (unsigned int i = 0; i < number_of_double_fields; i++){
     dcgm_field_meta_t * dcgm_meta_double_field = (*DcgmFieldGetById_ptr)( vector_of_ids_of_double_fields[i] );
     if ( dcgm_meta_double_field == 0 )
     {
        LOG(csmd, error) << "Error: Field " << std::setw(2) << i << " - DcgmFieldGetById returned 0";
        dcgm_init_flag = false;
        return;
     } else {
        dcgm_meta_double_vector.push_back( dcgm_meta_double_field->tag );
        LOG(csmd, info) << "        Field " << std::setw(2) << i << " - The tag for the field is: " << dcgm_meta_double_field->tag;
     }
    }

    LOG(csmd, info) << "Printing the unique identifiers of the watched DCGM int64 fields";
    for (unsigned int i = 0; i < number_of_int64_fields; i++){
     dcgm_field_meta_t * dcgm_meta_int64_field = (*DcgmFieldGetById_ptr)( vector_of_ids_of_int64_fields[i] );
     if ( dcgm_meta_int64_field == 0 )
     {
        LOG(csmd, error) << "Error: Field " << std::setw(2) << i << " - DcgmFieldGetById returned 0";
        dcgm_init_flag = false;
        return;
     } else {
        dcgm_meta_int64_vector.push_back( dcgm_meta_int64_field->tag );
        LOG(csmd, info) << "        Field " << std::setw(2) << i << " - The tag for the field is: " << dcgm_meta_int64_field->tag;
     }
    }

    // about the storage vectors

    for (int i = 0; i < dcgm_gpu_count; i++) {
     vector_for_double_values_storage.push_back( vector<double>() );
     for (unsigned int j = 0; j < number_of_double_fields; j++) {
      vector_for_double_values_storage[i].push_back( 0.0 );
     }
    }

    for (int i = 0; i < dcgm_gpu_count; i++) {
     vector_for_int64_values_storage.push_back( vector<long>() );
     for (unsigned int j = 0; j < number_of_int64_fields; j++) {
      vector_for_int64_values_storage[i].push_back( 0 );
     }
    }

}

bool csm::daemon::INV_DCGM_ACCESS::InitializeFunctionPointers()
{
   dcgmInit_ptr = (dcgmInit_ptr_t) dlsym(libdcgm_ptr, "dcgmInit");
   if ( dcgmInit_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmInit_ptr was successful";
   }

   dcgmConnect_ptr = (dcgmConnect_ptr_t) dlsym(libdcgm_ptr, "dcgmConnect");
   if ( dcgmConnect_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmConnect_ptr was successful";
   }

   dcgmGetAllDevices_ptr = (dcgmGetAllDevices_ptr_t) dlsym(libdcgm_ptr, "dcgmGetAllDevices");
   if ( dcgmGetAllDevices_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmGetAllDevices_ptr was successful";
   }

   dcgmGroupGetAllIds_ptr = (dcgmGroupGetAllIds_ptr_t) dlsym(libdcgm_ptr, "dcgmGroupGetAllIds");
   if ( dcgmGetAllDevices_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmGroupGetAllIds_ptr was successful";
   }

   dcgmGroupGetInfo_ptr = (dcgmGroupGetInfo_ptr_t) dlsym(libdcgm_ptr, "dcgmGroupGetInfo");
   if ( dcgmGroupGetInfo_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmGroupGetInfo_ptr was successful";
   }

   dcgmFieldGroupGetAll_ptr = (dcgmFieldGroupGetAll_ptr_t) dlsym(libdcgm_ptr, "dcgmFieldGroupGetAll");
   if ( dcgmFieldGroupGetAll_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmFieldGroupGetAll_ptr was successful";
   }

   dcgmFieldGroupGetInfo_ptr = (dcgmFieldGroupGetInfo_ptr_t) dlsym(libdcgm_ptr, "dcgmFieldGroupGetInfo");
   if ( dcgmFieldGroupGetInfo_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmFieldGroupGetInfo_ptr was successful";
   }

   dcgmGroupCreate_ptr = (dcgmGroupCreate_ptr_t) dlsym(libdcgm_ptr, "dcgmGroupCreate");
   if ( dcgmGroupCreate_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmGroupCreate_ptr was successful";
   }

   dcgmGroupAddDevice_ptr = (dcgmGroupAddDevice_ptr_t) dlsym(libdcgm_ptr, "dcgmGroupAddDevice");
   if ( dcgmGroupAddDevice_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmGroupAddDevice_ptr was successful";
   }

   dcgmFieldGroupCreate_ptr = (dcgmFieldGroupCreate_ptr_t) dlsym(libdcgm_ptr, "dcgmFieldGroupCreate");
   if ( dcgmFieldGroupCreate_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmFieldGroupCreate_ptr was successful";
   }

   dcgmWatchFields_ptr = (dcgmWatchFields_ptr_t) dlsym(libdcgm_ptr, "dcgmWatchFields");
   if ( dcgmWatchFields_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmWatchFields_ptr was successful";
   }
   
   dcgmUnwatchFields_ptr = (dcgmUnwatchFields_ptr_t) dlsym(libdcgm_ptr, "dcgmUnwatchFields");
   if ( dcgmUnwatchFields_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmUnwatchFields_ptr was successful";
   }

   dcgmUpdateAllFields_ptr = (dcgmUpdateAllFields_ptr_t) dlsym(libdcgm_ptr, "dcgmUpdateAllFields");
   if ( dcgmUpdateAllFields_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmUpdateAllFields_ptr was successful";
   }

   dcgmGetAllDevices_ptr = (dcgmGetAllDevices_ptr_t) dlsym(libdcgm_ptr, "dcgmGetAllDevices");
   if ( dcgmGetAllDevices_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmGetAllDevices_ptr was successful";
   }

   dcgmGetDeviceAttributes_ptr = (dcgmGetDeviceAttributes_ptr_t) dlsym(libdcgm_ptr, "dcgmGetDeviceAttributes");
   if ( dcgmGetDeviceAttributes_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmGetDeviceAttributes_ptr was successful";
   }

   dcgmGetLatestValuesForFields_ptr = (dcgmGetLatestValuesForFields_ptr_t) dlsym(libdcgm_ptr, "dcgmGetLatestValuesForFields");
   if ( dcgmGetLatestValuesForFields_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmGetLatestValuesForFields_ptr was successful";
   }

   dcgmFieldGroupDestroy_ptr = (dcgmFieldGroupDestroy_ptr_t) dlsym(libdcgm_ptr, "dcgmFieldGroupDestroy");
   if ( dcgmFieldGroupDestroy_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmFieldGroupDestroy_ptr was successful";
   }

   dcgmGroupDestroy_ptr = (dcgmGroupDestroy_ptr_t) dlsym(libdcgm_ptr, "dcgmGroupDestroy");
   if ( dcgmGroupDestroy_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmGroupDestroy_ptr was successful";
   }

   dcgmDisconnect_ptr = (dcgmDisconnect_ptr_t) dlsym(libdcgm_ptr, "dcgmDisconnect");
   if ( dcgmDisconnect_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmDisconnect_ptr was successful";
   }

   dcgmShutdown_ptr = (dcgmShutdown_ptr_t) dlsym(libdcgm_ptr, "dcgmShutdown");
   if ( dcgmShutdown_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmShutdown_ptr was successful";
   }

   DcgmFieldGetById_ptr = (DcgmFieldGetById_ptr_t) dlsym(libdcgm_ptr, "DcgmFieldGetById");
   if ( DcgmFieldGetById_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "DcgmFieldGetById_ptr was successful";
   }
    
   dcgmWatchJobFields_ptr = (dcgmWatchJobFields_ptr_t) dlsym(libdcgm_ptr, "dcgmWatchJobFields");
   if ( dcgmWatchJobFields_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmWatchJobFields_ptr was successful";
   }

   dcgmJobStartStats_ptr = (dcgmJobStartStats_ptr_t) dlsym(libdcgm_ptr, "dcgmJobStartStats");
   if ( dcgmJobStartStats_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmJobStartStats_ptr was successful";
   }

   dcgmJobStopStats_ptr = (dcgmJobStopStats_ptr_t) dlsym(libdcgm_ptr, "dcgmJobStopStats");
   if ( dcgmJobStopStats_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmJobStopStats_ptr was successful";
   }

   dcgmJobGetStats_ptr = (dcgmJobGetStats_ptr_t) dlsym(libdcgm_ptr, "dcgmJobGetStats");
   if ( dcgmJobGetStats_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmJobGetStats_ptr was successful";
   }

   dcgmJobRemove_ptr = (dcgmJobRemove_ptr_t) dlsym(libdcgm_ptr, "dcgmJobRemove");
   if ( dcgmJobRemove_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmJobRemove_ptr was successful";
   }

   return true;
}

bool csm::daemon::INV_DCGM_ACCESS::GetGPUsAttributes(){

  // checking if dlopen and dcgm init functions were successfull
  if ( !dlopen_flag && dcgm_init_flag )
  {

   // cycling on the gpus of the group
   //for (int32_t i = 0; i < dcgm_gpu_count && i < DCGM_MAX_NUM_DEVICES; i++)
   for (int i = 0; i < dcgm_gpu_count && i < DCGM_MAX_NUM_DEVICES; i++)
   {

    // updating the version field for the gpu
    gpu_attributes[i].version = dcgmDeviceAttributes_version;

    // collecting the fields for the gpu
    dcgmReturn_t rc(DCGM_ST_OK);
    rc = (*dcgmGetDeviceAttributes_ptr)(dcgm_handle, gpu_ids[i], &gpu_attributes[i]);
    if (rc != DCGM_ST_OK)
    {
	// the get gpu attributes for this gpu failed
        LOG(csmd, error) << "Error: dcgmGetDeviceAttributes for gpu_id=" << gpu_ids[i] << " returned \"" << errorString(rc) << "(" << rc << ")\"";
        return false;

    }
    else
    {

	// updating the gpu identifier
        gpu_ids[i]=i;

    }

   }

  } else {

   // dlopen or dcgm init functions failed, no "get gpu attributes" will be executed
   LOG(csmd, error) << "Error: Dlopen or DCGM failed when the class was contsructed, no \"get gpu attributes\" will be executed";
   return false;

  }
  
  // dlopen and dcgm were successfull, gpu device attributed for all the gpus of the node have been collected
  return true;

}

bool csm::daemon::INV_DCGM_ACCESS::LogGPUsEnviromentalData()
{

  // checking if dlopen and dcgm init functions were successfull
  if ( !dlopen_flag && dcgm_init_flag )
  {

   // update all the fields
   dcgmReturn_t rc(DCGM_ST_OK);
   rc = (*dcgmUpdateAllFields_ptr)(dcgm_handle, 1);
   if (rc != DCGM_ST_OK)
   {
	LOG(csmenv, error) << "Error: dcgmUpdateAllFields returned \"" << errorString(rc) << "(" << rc << ")\"";
        return false;
   }

   // scanning the gpus
   for (int i = 0; i < dcgm_gpu_count; i++){

    // get the latests values of the double fields
    rc = (*dcgmGetLatestValuesForFields_ptr)(dcgm_handle, gpu_ids[i], &vector_of_ids_of_double_fields.front(), number_of_double_fields, &vector_of_double_values.front());
    if (rc != DCGM_ST_OK)
    {
	  LOG(csmenv, error) << "Error: dcgmGetLatestValuesForFields for gpu_id=" << gpu_ids[i] << " returned \"" << errorString(rc) << "(" << rc << ")\"";
          return false;
    }

    // updating the storage vector for the doubles
    for (unsigned int j = 0; j < number_of_double_fields; j++){
     vector_for_double_values_storage[i][j] = vector_of_double_values[j].value.dbl;
    }

    // log the double results
    for (unsigned int j = 0; j < number_of_double_fields; j++){

     int status = vector_of_double_values[j].status;
     string error_string = errorString( (dcgmReturn_t) vector_of_double_values[j].status );
     double actual_double_value;
     if ( vector_flag_old_double_data[j] == (unsigned int) 0 ){
      actual_double_value = vector_of_double_values[j].value.dbl;
      vector_old_double_fields_value[i][j] = 0.0;
     } else {
      actual_double_value = vector_of_double_values[j].value.dbl - vector_old_double_fields_value[i][j];
      vector_old_double_fields_value[i][j] = vector_of_double_values[j].value.dbl;
     }
     
     if ( status == DCGM_ST_OK ){
      if ( DCGM_FP64_IS_BLANK( actual_double_value ) ){
       LOG(csmenv, debug) << "GPU " << i << " - Field  " << j << " - Identifier " << std::setw(40) << dcgm_meta_double_vector[j] << " - Field not supported";
      } else {
       LOG(csmenv, debug) << "GPU " << i << " - Field  " << j << " - Identifier " << std::setw(40) << dcgm_meta_double_vector[j] << " - " << actual_double_value;
      }
     } else {
       LOG(csmenv, debug) << "GPU " << i << " - Field  " << j << " - Identifier " << std::setw(40) << dcgm_meta_double_vector[j] << " - " << error_string;
     }

    }

    // get the latests values of the int64 fields
    rc = (*dcgmGetLatestValuesForFields_ptr)(dcgm_handle, gpu_ids[i], &vector_of_ids_of_int64_fields.front(), number_of_int64_fields, &vector_of_int64_values.front());
    if (rc != DCGM_ST_OK)
    {
          LOG(csmenv, error) << "Error: dcgmGetLatestValuesForFields for gpu_id=" << gpu_ids[i] << " returned \"" << errorString(rc) << "(" << rc << ")\"";
          return false;
    }

    // updating the storage vector for the int64
    for (unsigned int j = 0; j < number_of_double_fields; j++){
     vector_for_int64_values_storage[i][j] = vector_of_int64_values[j].value.i64;
    }

    // log the int64 results
    for (unsigned int j = 0; j < number_of_int64_fields; j++){

     int status = vector_of_int64_values[j].status;
     string error_string = errorString( (dcgmReturn_t) vector_of_int64_values[j].status );
     long actual_int64_value;
     if ( vector_flag_old_int64_data[j] == (unsigned int) 0 ){
      actual_int64_value = vector_of_int64_values[j].value.i64;
      vector_old_int64_fields_value[i][j] = 0.0;
     } else {
      actual_int64_value = vector_of_int64_values[j].value.i64 - vector_old_int64_fields_value[i][j];
      vector_old_int64_fields_value[i][j] = vector_of_int64_values[j].value.i64;
     }

     if ( status == DCGM_ST_OK ){
      if ( DCGM_INT64_IS_BLANK( actual_int64_value ) ){
       LOG(csmenv, debug) << "GPU " << i << " - Field  " << j << " - Identifier " << std::setw(40) << dcgm_meta_int64_vector[j] << " - Field not supported";
      } else {
       LOG(csmenv, debug) << "GPU " << i << " - Field  " << j << " - Identifier " << std::setw(40) << dcgm_meta_int64_vector[j] << " - " << actual_int64_value;
      }
     } else {
       LOG(csmenv, debug) << "GPU " << i << " - Field  " << j << " - Identifier " << std::setw(40) << dcgm_meta_int64_vector[j] << " - " << error_string;
     }

    }

   }

  } else {

   // dlopen or dcgm init functions failed, no "gpu env data collection" will be executed
   LOG(csmenv, error) << "Error: Dlopen or DCGM failed when the class was constructed, no \"gpu env data collection\" will be executed";
   return false;

  }

  // dlopen and dcgm were successfull, gpu env data collection for all the gpus of the node have been executed
  return true;

}

void csm::daemon::INV_DCGM_ACCESS::Get_Double_DCGM_Field_Values( vector<double> &vector_double_dcgm_field_values )
{

 // resize input vector
 vector_double_dcgm_field_values.resize( dcgm_gpu_count * number_of_double_fields );

 // scan of gpus and double fields
 int index; 
 for (int i = 0; i < dcgm_gpu_count; i++ ){
  for (unsigned int j = 0; j < number_of_double_fields; j++ ){
   index = i * number_of_double_fields + j;
   vector_double_dcgm_field_values[ index ] = vector_for_double_values_storage[i][j];
  }
 }

}

void csm::daemon::INV_DCGM_ACCESS::Get_Long_DCGM_Field_Values( vector<long> &vector_int64_dcgm_field_values )
{

 // resize input vector
 vector_int64_dcgm_field_values.resize( dcgm_gpu_count * number_of_int64_fields );

 // scan of gpus and double fields
 int index;
 for (int i = 0; i < dcgm_gpu_count; i++ ){
  for (unsigned int j = 0; j < number_of_int64_fields; j++ ){
   index = i * number_of_int64_fields + j;
   vector_int64_dcgm_field_values[ index ] = vector_for_int64_values_storage[i][j];
  }
 }

}

void csm::daemon::INV_DCGM_ACCESS::Get_Double_DCGM_Field_String_Identifiers( vector<string> &vector_labels_for_double_dcgm_fields )
{

 // resize input vector
 vector_labels_for_double_dcgm_fields.resize( number_of_double_fields );

 // scan of double fields
 for (unsigned int i = 0; i < number_of_double_fields; i++ ){
  vector_labels_for_double_dcgm_fields[ i ] = dcgm_meta_double_vector[ i ];
 }

}

void csm::daemon::INV_DCGM_ACCESS::Get_Long_DCGM_Field_String_Identifiers( vector<string> &vector_labels_for_int64_dcgm_fields )
{

 // resize input vector
 vector_labels_for_int64_dcgm_fields.resize( number_of_int64_fields );

 // scan of double fields
 for (unsigned int i = 0; i < number_of_int64_fields; i++ ){
  vector_labels_for_int64_dcgm_fields[ i ] = dcgm_meta_int64_vector[ i ];
 }

}
 
bool csm::daemon::INV_DCGM_ACCESS::ReadAllocationFields()
{
   // Check for successful initialization
   if ( !dlopen_flag && dcgm_init_flag )
   {
      // We only need these fields specifically when an allocation is created and deleted, 
      // so we only watch these fields on demand and then immediately unwatch after reading

      dcgmReturn_t rc(DCGM_ST_OK);

      uint64_t update_frequency(1000);   // How often to update this field in usec
      double max_keep_age(1);            // How long to keep data for this field in seconds
      uint32_t max_keep_samples(1);      // Maximum number of samples to keep
      rc = (*dcgmWatchFields_ptr)(dcgm_handle, (dcgmGpuGrp_t) DCGM_GROUP_ALL_GPUS, csm_allocation_field_group_handle, update_frequency, max_keep_age, max_keep_samples);
      if (rc != DCGM_ST_OK)
      {
         LOG(csmenv, error) << "Error: dcgmWatchFields returned \"" << errorString(rc) << "(" << rc << ")\"";
         return false;
      }
      else 
      {
         LOG(csmenv, debug) << "dcgmWatchFields was successful";
      }

      // Sleep for update_frequency to make sure the fields have been updated before reading
      usleep(update_frequency);

      rc = (*dcgmUpdateAllFields_ptr)(dcgm_handle, 1);
      if (rc != DCGM_ST_OK)
      {
         LOG(csmenv, error) << "Error: dcgmUpdateAllFields returned \"" << errorString(rc) << "(" << rc << ")\"";
         return false;
      }

      dcgmFieldValue_t csm_allocation_field_values[CSM_ALLOCATION_FIELD_COUNT]; 

      // scanning the gpus
      for (int i = 0; i < dcgm_gpu_count; i++)
      {
         // get the latests values for CSM_ALLOCATION_FIELD_GROUP 
         rc = (*dcgmGetLatestValuesForFields_ptr)(dcgm_handle, gpu_ids[i], CSM_ALLOCATION_FIELDS, CSM_ALLOCATION_FIELD_COUNT, csm_allocation_field_values);
         if (rc != DCGM_ST_OK)
         {
            LOG(csmenv, error) << "Error: dcgmGetLatestValuesForFields for gpu_id=" << gpu_ids[i] << " returned \"" << errorString(rc) << "(" << rc << ")\"";
            return false;
         }
         else
         {
            for (uint32_t j = 0; j < CSM_ALLOCATION_FIELD_COUNT; j++)
            {
               if ( (csm_allocation_field_values[j].status == DCGM_ST_OK) &&
                    (csm_allocation_field_values[j].fieldType == DCGM_FT_INT64) &&
                    (! DCGM_INT64_IS_BLANK(csm_allocation_field_values[j].value.i64) ) ) 
               {
                  LOG(csmenv, info) << "GPU " << i << " Field: " << csm_allocation_field_values[j].fieldId
                                    << " (INT64), value: " << csm_allocation_field_values[j].value.i64;
               }
               else if ( (csm_allocation_field_values[j].status == DCGM_ST_OK) &&
                         (csm_allocation_field_values[j].fieldType == DCGM_FT_DOUBLE) &&
                         (! DCGM_FP64_IS_BLANK(csm_allocation_field_values[j].value.dbl) ) ) 
               {
                  LOG(csmenv, info) << "GPU " << i << " Field: " << csm_allocation_field_values[j].fieldId
                                    << " (FP64), value: " << csm_allocation_field_values[j].value.dbl;
               }
               else
               {
                  LOG(csmenv, info) << "GPU " << i << " Field: " << csm_allocation_field_values[j].fieldId
                                    << "unexpected case!";
               }
            }
         }
      }

      rc = (*dcgmUnwatchFields_ptr)(dcgm_handle, (dcgmGpuGrp_t) DCGM_GROUP_ALL_GPUS, csm_allocation_field_group_handle);
      if (rc != DCGM_ST_OK)
      {
         LOG(csmenv, error) << "Error: dcgmUnwatchFields returned \"" << errorString(rc) << "(" << rc << ")\"";
      }
      else 
      {
         LOG(csmenv, debug) << "dcgmUnwatchFields was successful";
      }

   }
   else
   {
      // dlopen or dcgm init functions failed, no "gpu env data collection" will be executed
      LOG(csmenv, error) << "Error: Dlopen or DCGM failed when the class was constructed, no \"gpu env data collection\" will be executed";
      return false;
   }

   // dlopen and dcgm were successfull, gpu env data collection for all the gpus of the node have been executed
   return true;
}

bool csm::daemon::INV_DCGM_ACCESS::CollectGpuData(std::list<boost::property_tree::ptree> &gpu_data_pt_list)
{
   LOG(csmenv, debug) << "Enter " << __FUNCTION__;

   if (Uninitialized())
   {
      LogInitializationWarning(__FUNCTION__);
      LOG(csmenv, debug) << "Exit " << __FUNCTION__;   
      return false;
   }
   
   // Only allow one thread to call DCGM at a time
   std::lock_guard<std::mutex> lock(dcgm_mutex);
   
   dcgmReturn_t rc(DCGM_ST_OK);

   uint64_t update_frequency(100);    // How often to update this field in usec
   double max_keep_age(1);            // How long to keep data for this field in seconds
   uint32_t max_keep_samples(1);      // Maximum number of samples to keep
   
   rc = (*dcgmWatchFields_ptr)(dcgm_handle, (dcgmGpuGrp_t) DCGM_GROUP_ALL_GPUS, csm_environmental_field_group_handle, update_frequency, 
      max_keep_age, max_keep_samples);
   if (rc != DCGM_ST_OK)
   {
      LOG(csmenv, error) << "Error: dcgmWatchFields returned \"" << errorString(rc) << "(" << rc << ")\"";
      return false;
   }
   else 
   {
      LOG(csmenv, debug) << "dcgmWatchFields was successful";
   }

   // Sleep for update_frequency to make sure the fields have been updated before reading
   usleep(update_frequency);
      
   rc = (*dcgmUpdateAllFields_ptr)(dcgm_handle, 1);
   if (rc != DCGM_ST_OK)
   {
      LOG(csmenv, error) << "Error: dcgmUpdateAllFields returned \"" << errorString(rc) << "(" << rc << ")\"";
      return false;
   }

   dcgmFieldValue_t csm_environmental_field_values[CSM_ENVIRONMENTAL_FIELD_COUNT]; 

   // scan the gpus
   for (int i = 0; i < dcgm_gpu_count; i++)
   {
      // get the latests values for CSM_ENVIRONMENTAL_FIELD_GROUP 
      rc = (*dcgmGetLatestValuesForFields_ptr)(dcgm_handle, gpu_ids[i], CSM_ENVIRONMENTAL_FIELDS, CSM_ENVIRONMENTAL_FIELD_COUNT, 
         csm_environmental_field_values);
      if (rc != DCGM_ST_OK)
      {
         LOG(csmenv, error) << "Error: dcgmGetLatestValuesForFields for gpu_id=" << gpu_ids[i] << " returned \"" << errorString(rc) << "(" << rc << ")\"";
         return false;
      }
      else
      {
         boost::property_tree::ptree gpu_pt;
         gpu_pt.put(CSM_BDS_KEY_TYPE, CSM_BDS_TYPE_GPU_COUNTERS);
         bool has_gpu_data(false);
         
         // lambda used to insert gpu id data as the first elements in data when a sensor match occurs 
         auto insert_gpu_field = [&](const std::string &key, const std::string &value)
         {
            // If this is the first valid field found, insert the identifier information first
            if (!has_gpu_data)
            {
               has_gpu_data = true;
               gpu_pt.put( "data.gpu_id", std::to_string(i) );
            }

            gpu_pt.put("data." + key, value);
         };
         
         for (uint32_t j = 0; j < CSM_ENVIRONMENTAL_FIELD_COUNT; j++)
         {
            if ( (csm_environmental_field_values[j].status == DCGM_ST_OK) &&
                 (csm_environmental_field_values[j].fieldType == DCGM_FT_INT64) &&
                 (! DCGM_INT64_IS_BLANK(csm_environmental_field_values[j].value.i64) ) ) 
            {
               LOG(csmenv, debug) << "GPU " << i << " " << csm_environmental_field_names[j]
                                  << " (INT64), value: " << csm_environmental_field_values[j].value.i64;
               insert_gpu_field(csm_environmental_field_names[j], std::to_string(csm_environmental_field_values[j].value.i64));
            }
            else if ( (csm_environmental_field_values[j].status == DCGM_ST_OK) &&
                      (csm_environmental_field_values[j].fieldType == DCGM_FT_DOUBLE) &&
                      (! DCGM_FP64_IS_BLANK(csm_environmental_field_values[j].value.dbl) ) ) 
            {
               LOG(csmenv, debug) << "GPU " << i << " " << csm_environmental_field_names[j] 
                                  << " (FP64), value: " << csm_environmental_field_values[j].value.dbl;
               insert_gpu_field(csm_environmental_field_names[j], std::to_string(csm_environmental_field_values[j].value.dbl));
            }
            else if ( (csm_environmental_field_values[j].status == DCGM_ST_OK) &&
                      (csm_environmental_field_values[j].fieldType == DCGM_FT_STRING) &&
                      (! DCGM_STR_IS_BLANK(csm_environmental_field_values[j].value.str) ) )
            {
               LOG(csmenv, debug) << "GPU " << i << " " << csm_environmental_field_names[j]
                                  << " (STR), value: " << csm_environmental_field_values[j].value.str;
               insert_gpu_field(csm_environmental_field_names[j], csm_environmental_field_values[j].value.str);
            }
            else
            {
               LOG(csmenv, debug) << "GPU " << i << " " << csm_environmental_field_names[j]
                                  << " unexpected case!";
            }
         }

         if (has_gpu_data)
         {
            gpu_data_pt_list.push_back(gpu_pt);
         }
      }
   }

   rc = (*dcgmUnwatchFields_ptr)(dcgm_handle, (dcgmGpuGrp_t) DCGM_GROUP_ALL_GPUS, csm_environmental_field_group_handle);
   if (rc != DCGM_ST_OK)
   {
      LOG(csmenv, error) << "Error: dcgmUnwatchFields returned \"" << errorString(rc) << "(" << rc << ")\"";
   }
   else 
   {
      LOG(csmenv, debug) << "dcgmUnwatchFields was successful";
   }
   
   LOG(csmenv, debug) << "Exit " << __FUNCTION__;   
   return true;
}

bool csm::daemon::INV_DCGM_ACCESS::StartAllocationStats(const int64_t &i_allocation_id)
{
   LOG(csmenv, debug) << "Enter " << __FUNCTION__ << ", i_allocation_id=" << i_allocation_id;

   if (Uninitialized())
   {
      LogInitializationWarning(__FUNCTION__);
      LOG(csmenv, debug) << "Exit " << __FUNCTION__ << ", i_allocation_id=" << i_allocation_id;   
      return false;
   }

   // Only allow one thread to call DCGM at a time
   std::lock_guard<std::mutex> lock(dcgm_mutex);

   LOG(csmenv, debug) << "Exit " << __FUNCTION__ << ", i_allocation_id=" << i_allocation_id;   
   return true;
}

bool csm::daemon::INV_DCGM_ACCESS::StopAllocationStats(const int64_t &i_allocation_id, int64_t &o_gpu_usage)
{
   LOG(csmenv, debug) << "Enter " << __FUNCTION__ << ", i_allocation_id=" << i_allocation_id;

   if (Uninitialized())
   {
      LogInitializationWarning(__FUNCTION__);
      LOG(csmenv, debug) << "Exit " << __FUNCTION__ << ", i_allocation_id=" << i_allocation_id << " o_gpu_usage=" << o_gpu_usage;   
      return false;
   }

   // Only allow one thread to call DCGM at a time
   std::lock_guard<std::mutex> lock(dcgm_mutex);
   
   o_gpu_usage = -1;
   LOG(csmenv, debug) << "Exit " << __FUNCTION__ << ", i_allocation_id=" << i_allocation_id << " o_gpu_usage=" << o_gpu_usage;   
   return true;
}

bool csm::daemon::INV_DCGM_ACCESS::Uninitialized()
{
   return ( (dlopen_flag == true) || (dcgm_init_flag == false) );
}

void csm::daemon::INV_DCGM_ACCESS::LogInitializationWarning(const std::string &function_name)
{
   if (dlopen_flag == true)
   {
      LOG(csmenv, warning) << "Failed to load libdcgm.so at start up, skipping " << function_name;
   }
   else if (dcgm_init_flag == false)
   {
      LOG(csmenv, warning) << "DCGM initialization failed at start up, skipping " << function_name;
   }
}

bool csm::daemon::INV_DCGM_ACCESS::CreateCsmFieldGroup(const uint32_t field_count, uint16_t fields[], char* field_group_name,
   dcgmFieldGrp_t* field_group_handle)
{
   // Check for successful initialization

   LOG(csmenv, debug) << field_group_name << " field_count=" << field_count;

   dcgmReturn_t rc(DCGM_ST_OK);
   rc = (*dcgmFieldGroupCreate_ptr)(dcgm_handle, field_count, fields, field_group_name, field_group_handle);
   if (rc != DCGM_ST_OK)
   {
      LOG(csmenv, error) << "Error: dcgmFieldGroupCreate returned \"" << errorString(rc) << "(" << rc << ")\"";
      return false;
   }
   else 
   {
      LOG(csmenv, debug) << "dcgmFieldGroupCreate was successful, " << field_group_name << " field_group_handle=" << field_group_handle;
      return true;
   }
}

bool csm::daemon::INV_DCGM_ACCESS::DeleteCsmFieldGroup()
{
   return true;
}

bool csm::daemon::INV_DCGM_ACCESS::ReadFieldNames(const uint32_t field_count, uint16_t fields[], std::vector<std::string> &field_names)
{
   bool status(true);

   field_names.resize(field_count, "unknown_field");

   for (uint32_t i = 0; i < field_count; i++)
   {
      dcgm_field_meta_t* dcgm_field_meta_ptr(NULL);
      dcgm_field_meta_ptr = (*DcgmFieldGetById_ptr)( fields[i] );
      if (dcgm_field_meta_ptr == NULL)
      {
         LOG(csmenv, error) << "Error: DcgmFieldGetById returned NULL for field id:" << fields[i];
         status = false;
      } 
      else 
      {
         field_names[i] = dcgm_field_meta_ptr->tag;
         LOG(csmenv, debug) << "Read field id: " << fields[i] << " field_name: " << field_names[i];
      }
   }

   return status;
}

int list_field_values(unsigned int gpuId, dcgmFieldValue_t *values, int numValues, void *userdata)
{

    // The void pointer at the end allows a pointer to be passed to this
    // function. Here we know that we are passing in a null terminated C
    // string, so I can cast it as such. This pointer can be useful if you
    // need a reference to something inside your function.
    std::cout << std::endl;
    std::map <unsigned int,dcgmFieldValue_t > field_val_map;
    //note this is a pointer to a map.
    field_val_map = *static_cast<std::map<unsigned int,
                                            dcgmFieldValue_t > *> (userdata);

    // Storing the values in the map where key is field Id and the value is
    // the corresponding data for the field.
    for (int i = 0; i < numValues; i++) {
        field_val_map[values[i].fieldId] = values[i];
    }


    // Output the information to screen.
    for (map<unsigned int, dcgmFieldValue_t>::iterator it = field_val_map.begin();
         it != field_val_map.end(); ++it) {
        std::cout << "Field ID => " << it->first << endl;
        std::cout << "Value => ";
	/*
        switch (DcgmFieldGetById((it->second).fieldId)->fieldType) {
            case DCGM_FT_BINARY:
                // Handle binary data
                break;
            case DCGM_FT_DOUBLE:
                std::cout << (it->second).value.dbl;
                break;
            case DCGM_FT_INT64:
                std::cout << (it->second).value.i64;
                break;
            case DCGM_FT_STRING:
                std::cout << (it->second).value.str;
                break;
            case DCGM_FT_TIMESTAMP:
                std::cout << (it->second).value.i64;
                break;
            default:
                std::cout << "Error in field types. " << (it->second).fieldType
                          << " Exiting.\n";
                // Error, return > 0 error code.
                return 1;
                break;
        }
	*/

        std::cout << std::endl;
        // Shutdown DCGM fields. This takes care of the memory initialized
        // when we called DcgmFieldsInit.

    }

    // Program executed correctly. Return 0 to notify DCGM (callee) that it
    // was successful.
    return 0;

}

int list_field_values_since(unsigned int gpuId, dcgmFieldValue_t *values, int numValues, void *userdata)
{

    int i =0;
    vector<dcgmFieldValue_t> dcgm_field_vals_vec;

    map <unsigned int,vector<dcgmFieldValue_t> > field_val_vec_map;
    //note this is a pointer to a map.
    field_val_vec_map = *static_cast<std::map<unsigned int, vector<dcgmFieldValue_t> > *> (userdata);

    for(i = 0;i<numValues;i++)
    {
        //Check if element exists in the map
        if(field_val_vec_map.count((values[i].fieldId)) > 0)
        {
            dcgm_field_vals_vec = field_val_vec_map[values[i].fieldId];

            dcgm_field_vals_vec.push_back(values[i]);

            field_val_vec_map[values[i].fieldId] = dcgm_field_vals_vec;
            std::cout<<"Pushed value for the existing field id => "<<
                     (values[i].fieldId)<<endl;
        }
            // Element is not present in map. Create a new vector and push values
            // into it.
        else
        {
            dcgm_field_vals_vec.push_back(values[i]);
            field_val_vec_map[values[i].fieldId] = dcgm_field_vals_vec;
            std::cout<<"\n\n\n\nPushed value for the non - existing field id => "
                     << values[i].fieldId<<endl;
        }
    }

    return 0;
}



#endif  // DCGM support is enabled

/////////////////////////////////////////////////////////////////////////////////
//// DCGM support is disabled
/////////////////////////////////////////////////////////////////////////////////

#ifndef DCGM

csm::daemon::INV_DCGM_ACCESS::INV_DCGM_ACCESS()
{
   // flags
   dlopen_flag = true;
   dcgm_init_flag = false;

   // logging
   LOG(csmenv, info) << "Built without DCGM support, GPU related functions are disabled.";
}

csm::daemon::INV_DCGM_ACCESS::~INV_DCGM_ACCESS()
{
}

bool csm::daemon::INV_DCGM_ACCESS::LogGPUsEnviromentalData()
{
   return true;
}

void
csm::daemon::INV_DCGM_ACCESS::Get_Double_DCGM_Field_Values( std::vector<double> &vector_double_dcgm_field_values )
{}
void
csm::daemon::INV_DCGM_ACCESS::Get_Long_DCGM_Field_Values( std::vector<long> &vector_int64_dcgm_field_values )
{}
void
csm::daemon::INV_DCGM_ACCESS::Get_Double_DCGM_Field_String_Identifiers( std::vector<std::string> &vector_labels_for_double_dcgm_field_strings )
{}
void
csm::daemon::INV_DCGM_ACCESS::Get_Long_DCGM_Field_String_Identifiers( std::vector<std::string> &vector_labels_for_int64_dcgm_fields )
{}

bool csm::daemon::INV_DCGM_ACCESS::ReadAllocationFields()
{
   LOG(csmenv, warning) << "Built without DCGM support, skipping ReadAllocationFields()";
   return false;
}

bool csm::daemon::INV_DCGM_ACCESS::CollectGpuData(std::list<boost::property_tree::ptree> &gpu_data_pt_list)
{
   LOG(csmenv, warning) << "Built without DCGM support, skipping CollectGpuData()";
   return false;
}

bool csm::daemon::INV_DCGM_ACCESS::StartAllocationStats(const int64_t &i_allocation_id)
{
   LOG(csmenv, warning) << "Built without DCGM support, skipping StartAllocationStats()";
   return false;
}

bool csm::daemon::INV_DCGM_ACCESS::StopAllocationStats(const int64_t &i_allocation_id, int64_t &o_gpu_usage)
{
   LOG(csmenv, warning) << "Built without DCGM support, skipping StopAllocationStats()";
   return false;
}

#endif  // DCGM support is disabled

