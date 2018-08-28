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
}

void csm::daemon::INV_DCGM_ACCESS::Init()
{
    // Only allow one thread to call DCGM at a time
    std::lock_guard<std::mutex> lock(dcgm_mutex);

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
    dcgmWatchPidFields_ptr = nullptr;
    dcgmWatchJobFields_ptr = nullptr;
    dcgmJobStartStats_ptr = nullptr;
    dcgmJobStopStats_ptr = nullptr;      
    dcgmJobGetStats_ptr = nullptr;
    dcgmJobRemove_ptr = nullptr;


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

    if (( dcgm_gpu_count < 0 ) || ( dcgm_gpu_count > DCGM_MAX_NUM_DEVICES))
    {
	LOG(csmd, error) << "Error: dcgmGetAllDevices returned unexpected gpu_count=" << dcgm_gpu_count;
        dcgm_init_flag = false;
        return;
    } else {
	LOG(csmd, debug) << "dcgm_gpu_count: " << dcgm_gpu_count;
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
   
    // Workaround for DCGM behavior:
    // While DCGM allows for multiple internal and external field groups, 
    // it appears to only store a single update_frequency, max_keep_age, and max_keep_samples per field.
    // If any two field groups, external or internal, both watch a common field, the watch settings of
    // one field group interfere with the watch settings of the other.
    // This causes interference between CSM environmental data and job stats data, for example.
    // In order to work around this behavior, always set all fields watched by CSM to use common settings.
   
    // For job stats fields, the data is calculated based on the samples that DCGM collects for
    // the job stats watched by WatchJobFields().
    // In order to have the most accurate accounting, we would prefer to always have samples representing 
    // the whole duration of the job. In order to have the most samples possible, try not to throw any
    // samples away due to max_keep_age or max_keep_samples limits being hit.
    // 
    // Scale max_keep_samples and max_keep_age to be around the length of the maximum expected job, plus some margin of error
    
    // Example values for a maximum job length of 30 days are below, actual values read from CSM daemon config
    // The CSM daemon config values can be overridden with tweaks 
    const uint32_t MAX_JOB_IN_SECONDS(60*60*24*30);
    const uint64_t UPDATE_FREQUENCY_IN_SECONDS(30);
    uint64_t update_frequency(UPDATE_FREQUENCY_IN_SECONDS*1000000);                 // How often to update this field in usecs
    double max_keep_age(MAX_JOB_IN_SECONDS*3);                                      // How long to keep data for this field in seconds
    uint32_t max_keep_samples(MAX_JOB_IN_SECONDS/UPDATE_FREQUENCY_IN_SECONDS);      // Maximum number of samples to keep (0=no limit)

    // Read actual values from csm config tweaks
    update_frequency = csm::daemon::Configuration::Instance()->GetTweaks()._DCGM_update_interval_s * 1000000;
    max_keep_age = csm::daemon::Configuration::Instance()->GetTweaks()._DCGM_max_keep_age_s;
    max_keep_samples = csm::daemon::Configuration::Instance()->GetTweaks()._DCGM_max_keep_samples;

    LOG(csmenv, info) << "DCGM watch settings: update_frequency: " << update_frequency
                      << ", max_keep_age: " << max_keep_age << ", max_keep_samples: " << max_keep_samples;
  
    // Start watching environmental fields 
    rc = (*dcgmWatchFields_ptr)(dcgm_handle, (dcgmGpuGrp_t) DCGM_GROUP_ALL_GPUS, csm_environmental_field_group_handle, update_frequency, 
      max_keep_age, max_keep_samples);
    if (rc != DCGM_ST_OK)
    {
        LOG(csmenv, error) << "Error: dcgmWatchFields returned \"" << errorString(rc) << "(" << rc << ")\"";
        dcgm_init_flag = false;
        return;
    }
    else 
    {
        LOG(csmenv, debug) << "dcgmWatchFields was successful";
    }
    
    // Start watching Job Fields to allow allocation job accounting to work correctly 
    rc = (*dcgmWatchJobFields_ptr)(dcgm_handle, (dcgmGpuGrp_t) DCGM_GROUP_ALL_GPUS, update_frequency, max_keep_age, max_keep_samples);
    if (rc != DCGM_ST_OK)
    {
        LOG(csmenv, error) << "Error: dcgmWatchJobFields returned \"" << errorString(rc) << "(" << rc << ")\"";
        dcgm_init_flag = false;
        return;
    }
    else 
    {
        LOG(csmenv, debug) << "dcgmWatchJobFields was successful";
    }

    // Call UpdateAllFields to initialize all of the fields watched above with valid data.
    // Without this call, any watched fields will not be guaranteed to have valid data the first time they are read.
    // This call prevents that situation by forcing valid data to be populated immediately after watching.
    //
    // A note about what UpdateAllFields does when using watches and DCGM in daemon mode:
    // UpdateAllFields(1) will only update fields if DCGM sees that they are due for an update based on the
    // current watch update frequency.
    // For example, if the watch update frequency is 10 seconds, but the data is polled by CSM every 1 second using:
    // UpdateAllFields() 
    // GetLatestValuesForFields()
    // DCGM will not actually update any of the data until 10 seconds has elapsed.
    // CSM will read the same set of data 10 times in a row until the update frequency triggers an update to occur.
    // The DCGM watch frequency should always be less than or equal to the CSM bucket frequency for useful data to be read. 
    //
    // UpdateAllFields() should be thought of as UpdateAllFieldsIfTheyAreOverdueForUpdating()
    // When running in DCGM daemon mode, UpdateAllFields() will be redundant most of the time because
    // DCGM is already polling the data based on the watch frequency.
    //
    // The exception to this is freshly watched fields. Calling UpdateAllFields(1) every time there is a change to 
    // watched fields forces valid data into the fields that are being watched.
    // 
    // UpdateAllFields is more important when using DCGM in embedded mode because DCGM is not polling on its own, 
    // but it still uses the same logic.
    rc = (*dcgmUpdateAllFields_ptr)(dcgm_handle, 1);
    if (rc != DCGM_ST_OK)
    {
        LOG(csmenv, error) << "Error: dcgmUpdateAllFields returned \"" << errorString(rc) << "(" << rc << ")\"";
        dcgm_init_flag = false;
        return;
    }
    else 
    {
        LOG(csmenv, debug) << "dcgmUpdateAllFields was successful";
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
    
   dcgmWatchPidFields_ptr = (dcgmWatchPidFields_ptr_t) dlsym(libdcgm_ptr, "dcgmWatchPidFields");
   if ( dcgmWatchPidFields_ptr == nullptr )
   {
      LOG(csmd, error) << dlerror();
      return false;
   }
   else
   {
      LOG(csmd, debug) << "dcgmWatchPidFields_ptr was successful";
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
   
   dcgmFieldValue_t csm_environmental_field_values[CSM_ENVIRONMENTAL_FIELD_COUNT]; 

   // scan the gpus
   for (int i = 0; i < dcgm_gpu_count; i++)
   {
      // get the latest values for CSM_ENVIRONMENTAL_FIELD_GROUP 
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

#ifdef REMOVED        
         // Extra printing for debugging field update times 
         for (uint32_t j = 0; j < CSM_ENVIRONMENTAL_FIELD_COUNT; j++)
         {
            LOG(csmenv, debug) << "GPU " << i << " " << csm_environmental_field_names[j]
                               << " field_ts: " << csm_environmental_field_values[j].ts;
         }
#endif

         if (has_gpu_data)
         {
            gpu_data_pt_list.push_back(gpu_pt);
         }
      }
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

   dcgmReturn_t rc(DCGM_ST_OK);

   char csm_job_stats_name[] = "CSM_JOB_STATS";

   rc = (*dcgmJobStartStats_ptr)(dcgm_handle, (dcgmGpuGrp_t) DCGM_GROUP_ALL_GPUS, csm_job_stats_name);
   if (rc != DCGM_ST_OK)
   {
      LOG(csmenv, error) << "Error: dcgmJobStartStats returned \"" << errorString(rc) << "(" << rc << ")\"";
      return false;
   }
   else 
   {
      LOG(csmenv, debug) << "dcgmJobStartStats was successful";
   }

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
   
   dcgmReturn_t rc(DCGM_ST_OK);
   char csm_job_stats_name[] = "CSM_JOB_STATS";
   
   rc = (*dcgmJobStopStats_ptr)(dcgm_handle, csm_job_stats_name);
   if (rc != DCGM_ST_OK)
   {
      LOG(csmenv, error) << "Error: dcgmJobStopStats returned \"" << errorString(rc) << "(" << rc << ")\"";
      return false;
   }
   else 
   {
      LOG(csmenv, debug) << "dcgmJobStopStats was successful";
   }
   
   dcgmJobInfo_t dcgm_job_stats;
   dcgm_job_stats.version = dcgmJobInfo_version;

   rc = (*dcgmJobGetStats_ptr)(dcgm_handle, csm_job_stats_name, &dcgm_job_stats);
   if (rc != DCGM_ST_OK)
   {
      LOG(csmenv, error) << "Error: dcgmJobGetStats returned \"" << errorString(rc) << "(" << rc << ")\"";
      return false;
   }
   else 
   {
      LOG(csmenv, debug) << "dcgmJobGetStats was successful, numGpus: " << dcgm_job_stats.numGpus;
  
      o_gpu_usage = 0;
      uint64_t elapsed_usecs(0);
      uint64_t gpu_usecs(0); 

      for (int32_t i = 0; i < dcgm_job_stats.numGpus; i++)
      {
         LOG(csmenv, debug) << "dcgmJobGetStats: gpuId: " << dcgm_job_stats.gpus[i].gpuId          << ", "
                            << " energyConsumed: "        << dcgm_job_stats.gpus[i].energyConsumed << ", "
                            << " startTime: "             << dcgm_job_stats.gpus[i].startTime      << ", "
                            << " endTime: "               << dcgm_job_stats.gpus[i].endTime        << ", "
                            << " smUtilization: "         << dcgm_job_stats.gpus[i].smUtilization.average;
         
         elapsed_usecs = dcgm_job_stats.gpus[i].endTime - dcgm_job_stats.gpus[i].startTime;
         gpu_usecs = (elapsed_usecs * dcgm_job_stats.gpus[i].smUtilization.average / 100);
         
         LOG(csmenv, debug) << "gpuId: "           << dcgm_job_stats.gpus[i].gpuId << ", "
                            << " numComputePids: " << dcgm_job_stats.gpus[i].numComputePids << ", "
                            << " smUtilization.minValue: "  << dcgm_job_stats.gpus[i].smUtilization.minValue << ", "
                            << " smUtilization.maxValue: "  << dcgm_job_stats.gpus[i].smUtilization.maxValue << ", "
                            << " smUtilization.average: "   << dcgm_job_stats.gpus[i].smUtilization.average;
      
         for (int32_t j = 0; j < dcgm_job_stats.gpus[i].numComputePids; j++)
         {
            LOG(csmenv, debug) << "gpuId: "    << dcgm_job_stats.gpus[i].gpuId << ", "
                               << " pid: "     << dcgm_job_stats.gpus[i].computePidInfo[j].pid << ", "
                               << " smUtil: "  << dcgm_job_stats.gpus[i].computePidInfo[j].smUtil << ", "
                               << " memUtil: " << dcgm_job_stats.gpus[i].computePidInfo[j].memUtil;
         }
 
         LOG(csmenv, debug) << "gpuId: "           << dcgm_job_stats.gpus[i].gpuId << ", "
                            << " elapsed_usecs: "  << elapsed_usecs << ", "
                            << " smUtilization: "  << dcgm_job_stats.gpus[i].smUtilization.average << ", "
                            << " gpu_usecs: "      << gpu_usecs;

         o_gpu_usage += gpu_usecs;
      }

   }

   rc = (*dcgmJobRemove_ptr)(dcgm_handle, csm_job_stats_name);
   if (rc != DCGM_ST_OK)
   {
      LOG(csmenv, error) << "Error: dcgmJobRemove returned \"" << errorString(rc) << "(" << rc << ")\"";
      return false;
   }
   else 
   {
      LOG(csmenv, debug) << "dcgmJobRemove was successful";
   }

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

