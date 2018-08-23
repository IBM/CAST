/*================================================================================

    csmd/src/inv/include/inv_dcgm_access.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/


#ifndef CSMD_SRC_INV_INCLUDE_INV_DCGM_ACCESS_H_
#define CSMD_SRC_INV_INCLUDE_INV_DCGM_ACCESS_H_

#include <string>
#include <vector>
#include <list>
#include <mutex>
#include <boost/property_tree/ptree.hpp>

/////////////////////////////////////////////////////////////////////////////////
////// DCGM support is enabled
/////////////////////////////////////////////////////////////////////////////////

#ifdef DCGM
#include "dcgm_agent.h"
#include "dcgm_fields.h"
#include "dcgm_structs.h"

#include <dlfcn.h>

using std::string;
using std::vector;

namespace csm {
namespace daemon {

class INV_DCGM_ACCESS
{
public:

  // define function pointer types for each symbol we are going to get via dlsym()  
  
  typedef dcgmReturn_t (*dcgmInit_ptr_t)();
  // dcgmInit_ptr_t - dcgmReturn_t DECLDIR dcgmInit();
  
  typedef dcgmReturn_t (*dcgmConnect_ptr_t)(char*, dcgmHandle_t*);
  // dcgmConnect_ptr_t - dcgmReturn_t DECLDIR dcgmConnect(char*, dcgmHandle_t*);

  typedef dcgmReturn_t (*dcgmGetAllDevices_ptr_t)(dcgmHandle_t, unsigned int [], int*);
  // dcgmGetAllDevices_ptr_t - dcgmReturn_t DECLDIR dcgmGetAllDevices(dcgmHandle_t pDcgmHandle, unsigned int gpuIdList[DCGM_MAX_NUM_DEVICES], int *count);

  typedef dcgmReturn_t (*dcgmGroupGetAllIds_ptr_t)(dcgmHandle_t, dcgmGpuGrp_t [], unsigned int*);
  // dcgmGroupGetAllIds_ptr_t - dcgmReturn_t DECLDIR dcgmGroupGetAllIds(dcgmHandle_t pDcgmHandle, dcgmGpuGrp_t groupIdList[], unsigned int *count);

  typedef dcgmReturn_t (*dcgmGroupGetInfo_ptr_t)(dcgmHandle_t, dcgmGpuGrp_t, dcgmGroupInfo_t*);
  // dcgmGroupGetInfo_ptr_t - dcgmReturn_t DECLDIR dcgmGroupGetInfo(dcgmHandle_t pDcgmHandle, dcgmGpuGrp_t groupId, dcgmGroupInfo_t *pDcgmGroupInfo);

  typedef dcgmReturn_t (*dcgmFieldGroupGetAll_ptr_t)(dcgmHandle_t, dcgmAllFieldGroup_t*);
  // dcgmFieldGroupGetAll_ptr_t - dcgmReturn_t dcgmFieldGroupGetAll(dcgmHandle_t dcgmHandle, dcgmAllFieldGroup_t *allGroupInfo)

  typedef dcgmReturn_t (*dcgmFieldGroupGetInfo_ptr_t)(dcgmHandle_t, dcgmFieldGroupInfo_t*);
  // dcgmFieldGroupGetInfo_ptr_t - dcgmReturn_t dcgmFieldGroupGetInfo(dcgmHandle_t dcgmHandle, dcgmFieldGroupInfo_t *fieldGroupInfo)
  
  typedef dcgmReturn_t (*dcgmGroupCreate_ptr_t)(dcgmHandle_t, dcgmGroupType_t, char*, dcgmGpuGrp_t*);
  // dcgmGroupCreate_ptr_t - dcgmReturn_t DECLDIR dcgmGroupCreate(dcgmHandle_t pDcgmHandle, dcgmGroupType_t type, char *groupName, dcgmGpuGrp_t *pDcgmGrpId);
  
  typedef dcgmReturn_t (*dcgmGroupAddDevice_ptr_t)(dcgmHandle_t, dcgmGpuGrp_t, unsigned int);
  // dcgmGroupAddDevice_ptr_t - dcgmReturn_t dcgmGroupAddDevice(dcgmHandle_t pDcgmHandle, dcgmGpuGrp_t groupId, unsigned int gpuId);
  
  typedef dcgmReturn_t (*dcgmFieldGroupCreate_ptr_t)(dcgmHandle_t, int, unsigned short*, char*, dcgmFieldGrp_t*);
  // dcgmFieldGroupCreate_ptr_t - dcgmReturn_t dcgmFieldGroupCreate(dcgmHandle_t dcgmHandle, int numFieldIds, unsigned short *fieldIds, char *fieldGroupName, dcgmFieldGrp_t *dcgmFieldGroupId);
  
  typedef dcgmReturn_t (*dcgmWatchFields_ptr_t)(dcgmHandle_t, dcgmGpuGrp_t, dcgmFieldGrp_t, long long, double, int);
  // dcgmWatchFields - dcgmReturn_t dcgmWatchFields(dcgmHandle_t pDcgmHandle, dcgmGpuGrp_t groupId, dcgmFieldGrp_t fieldGroupId, long long updateFreq, double maxKeepAge, int maxKeepSamples);
 
  typedef dcgmReturn_t (*dcgmUnwatchFields_ptr_t)(dcgmHandle_t, dcgmGpuGrp_t, dcgmFieldGrp_t);
  // dcgmUnwatchFields - dcgmReturn_t dcgmUnwatchFields(dcgmHandle_t pDcgmHandle, dcgmGpuGrp_t groupId, dcgmFieldGrp_t fieldGroupId);
 
  typedef dcgmReturn_t (*dcgmUpdateAllFields_ptr_t)(dcgmHandle_t, int);
  // dcgmUpdateAllFields_ptr_t - dcgmReturn_t dcgmUpdateAllFields(dcgmHandle_t pDcgmHandle, int waitForUpdate);
  
  typedef dcgmReturn_t (*dcgmGetDeviceAttributes_ptr_t)(dcgmHandle_t, unsigned int, dcgmDeviceAttributes_t*);
  // dcgmGetDeviceAttributes_ptr_t - dcgmReturn_t DECLDIR dcgmGetDeviceAttributes(dcgmHandle_t pDcgmHandle, unsigned int gpuId, dcgmDeviceAttributes_t *pDcgmAttr);
  
  typedef dcgmReturn_t (*dcgmGetLatestValuesForFields_ptr_t)(dcgmHandle_t, int, unsigned short [], unsigned int, dcgmFieldValue_t []);
  // dcgmGetLatestValuesForFields - dcgmReturn_t dcgmGetLatestValuesForFields(dcgmHandle_t pDcgmHandle, int gpuId, unsigned short fields[], unsigned int count, dcgmFieldValue_t values[]); 
  
  typedef dcgmReturn_t (*dcgmFieldGroupDestroy_ptr_t)(dcgmHandle_t, dcgmFieldGrp_t);
  // dcgmFieldGroupDestroy - dcgmReturn_t dcgmFieldGroupDestroy(dcgmHandle_t dcgmHandle, dcgmFieldGrp_t dcgmFieldGroupId);
  
  typedef dcgmReturn_t (*dcgmGroupDestroy_ptr_t)(dcgmHandle_t, dcgmGpuGrp_t);
  // dcgmGroupDestroy_ptr_t - dcgmReturn_t DECLDIR dcgmGroupDestroy(dcgmHandle_t pDcgmHandle, dcgmGpuGrp_t groupId);
  
  typedef dcgmReturn_t (*dcgmDisconnect_ptr_t)(dcgmHandle_t);
  // dcgmDisconnect_ptr_t - dcgmReturn_t DECLDIR dcgmDisconnect(dcgmHandle_t pDcgmHandle);

  typedef dcgmReturn_t (*dcgmShutdown_ptr_t)();
  // dcgmShutdown_ptr_t - dcgmReturn_t DECLDIR dcgmShutdown();

  typedef dcgm_field_meta_p (*DcgmFieldGetById_ptr_t)(unsigned short fieldId);
  // DcgmFieldGetById_ptr_t - dcgm_field_meta_p DcgmFieldGetById(unsigned short fieldId);

  typedef dcgmReturn_t (*dcgmWatchJobFields_ptr_t)(dcgmHandle_t, dcgmGpuGrp_t, long long, double, int);
  // dcgmWatchJobFields_ptr_t - dcgmReturn_t dcgmWatchJobFields(dcgmHandle_t, dcgmGpuGrp_t, long long, double, int);

  typedef dcgmReturn_t (*dcgmJobStartStats_ptr_t)(dcgmHandle_t, dcgmGpuGrp_t, char []);
  // dcgmJobStartStats_ptr_t - dcgmReturn_t dcgmJobStartStats(dcgmHandle_t, dcgmGpuGrp_t, char []);

  typedef dcgmReturn_t (*dcgmJobStopStats_ptr_t)(dcgmHandle_t, char []);
  // dcgmJobStopStats_ptr_t - dcgmReturn_t dcgmJobStopStats(dcgmHandle_t, char []);  

  typedef dcgmReturn_t (*dcgmJobGetStats_ptr_t)(dcgmHandle_t, char [], dcgmJobInfo_t*);
  // dcgmJobGetStats_ptr_t - dcgmReturn_t dcgmJobGetStats(dcgmHandle_t, char [], dcgmJobInfo_t*);

  typedef dcgmReturn_t (*dcgmJobRemove_ptr_t)(dcgmHandle_t, char []);
  // dcgmJobRemove_ptr_t - dcgmReturn_t dcgmJobRemove(dcgmHandle_t, char []);

  // functions

  static INV_DCGM_ACCESS* GetInstance(){ if( _Instance == nullptr ){ _Instance = new INV_DCGM_ACCESS(); } return _Instance; } // get the istance of the class object
  void Init(); // initialization for dlopen and  DCGM
  bool GetDlopenFlag(){ return dlopen_flag; } // dlopen flag, true if libdcgm_ptr is == nullptr
  bool GetDCGMInitFlag(){ return dcgm_init_flag; } // dcgm init flag, false if some modules were not loaded correcty or if some of the DCGM initialization functions failed
  unsigned int * GetGPUsIdsPointer(){ return &(gpu_ids[0]); } // pointer to the array of the gpu_ids
  dcgmDeviceAttributes_t * GetGPUsAttributesPointer(){ return &(gpu_attributes[0]); } // pointer to the arrray of the dcgmDeviceAttributes_ts
  int DCGMGPUCount(){ return dcgm_gpu_count; } // number of gpu of the node as counted by dcgm
  bool GetGPUsAttributes(); // populate the gpu_ids array and the gpu_attributes array, return false if something went wrong
  
  ~INV_DCGM_ACCESS();

  /**
   * Read the GPU metrics that are needed at the start and end of an allocation and return them to the caller.
   *
   * @return true if successful, false if unsuccessful. 
   */
  bool ReadAllocationFields();

  /**
   * Read the GPU metrics that are included in the GPU data collection bucket and add them to the gpu_data_pt_list object.
   *
   * @return true if successful, false if unsuccessful. 
   */
  bool CollectGpuData(std::list<boost::property_tree::ptree> &gpu_data_pt_list);

  /**
   * Start collecting GPU statistics for the specificed allocation.  
   *
   * @param i_allocation_id The CSM allocation id to associate with this set of statistics.
   *
   * @return true if successful, false if unsuccessful. 
   */
  bool StartAllocationStats(const int64_t &i_allocation_id);
  
  /**
   * Stop collecting GPU statistics for the specificed allocation.  
   *
   * @param i_allocation_id The CSM allocation id to stop collecting statistics for.
   * @param o_gpu_usage The cumulative GPU usage for all GPUs, measured in gpu*microseconds. Will not be modified if return value is false.
   *
   * @return true if successful, false if unsuccessful. 
   */
  bool StopAllocationStats(const int64_t &i_allocation_id, int64_t &o_gpu_usage);

private:
  INV_DCGM_ACCESS();

  /**
   * Attempt to initialize the function pointers to DCGM functions using dlsym().
   *
   * @return true if successful, false if unsuccessful. 
   */
  bool InitializeFunctionPointers();
  
  /**
   * Returns true if the class has failed to initialize correctly.
   */
  bool Uninitialized();

  /**
   * Used to log an appropriate warning message with the reason for failed initialization.
   */
  void LogInitializationWarning(const std::string &function_name);

  /**
   * Attempt to create a field group for CSM to use in DCGM.
   *
   * @return true if successful, false if unsuccessful. 
   */
  bool CreateCsmFieldGroup(const uint32_t field_count, uint16_t fields[], char* field_group_name, dcgmFieldGrp_t* field_group_handle);

  bool DeleteCsmFieldGroup();
  
  /**
   * Attempt to read the field names for all fields in a field group. 
   *
   * @return true if successful, false if unsuccessful. 
   */
  bool ReadFieldNames(const uint32_t field_count, uint16_t fields[], std::vector<std::string> &field_names);

private:

  // pointer to the class object
  static INV_DCGM_ACCESS *_Instance;

  std::mutex dcgm_mutex;

  // flags
  bool dlopen_flag;
  bool dcgm_init_flag;

  // function pointers
  void* libdcgm_ptr;
  dcgmInit_ptr_t dcgmInit_ptr;
  dcgmConnect_ptr_t dcgmConnect_ptr;
  dcgmGetAllDevices_ptr_t dcgmGetAllDevices_ptr;
  dcgmGroupGetAllIds_ptr_t dcgmGroupGetAllIds_ptr;
  dcgmGroupGetInfo_ptr_t dcgmGroupGetInfo_ptr;
  dcgmFieldGroupGetAll_ptr_t dcgmFieldGroupGetAll_ptr;
  dcgmFieldGroupGetInfo_ptr_t dcgmFieldGroupGetInfo_ptr; 
  dcgmGroupCreate_ptr_t dcgmGroupCreate_ptr;
  dcgmGroupAddDevice_ptr_t dcgmGroupAddDevice_ptr;
  dcgmFieldGroupCreate_ptr_t dcgmFieldGroupCreate_ptr;
  dcgmWatchFields_ptr_t dcgmWatchFields_ptr;
  dcgmUnwatchFields_ptr_t dcgmUnwatchFields_ptr;
  dcgmUpdateAllFields_ptr_t dcgmUpdateAllFields_ptr;
  dcgmGetDeviceAttributes_ptr_t dcgmGetDeviceAttributes_ptr;
  dcgmGetLatestValuesForFields_ptr_t dcgmGetLatestValuesForFields_ptr;
  dcgmFieldGroupDestroy_ptr_t dcgmFieldGroupDestroy_ptr;
  dcgmGroupDestroy_ptr_t dcgmGroupDestroy_ptr;
  dcgmDisconnect_ptr_t dcgmDisconnect_ptr;
  dcgmShutdown_ptr_t dcgmShutdown_ptr;
  DcgmFieldGetById_ptr_t DcgmFieldGetById_ptr;
  dcgmWatchJobFields_ptr_t dcgmWatchJobFields_ptr;
  dcgmJobStartStats_ptr_t dcgmJobStartStats_ptr;
  dcgmJobStopStats_ptr_t dcgmJobStopStats_ptr;
  dcgmJobGetStats_ptr_t dcgmJobGetStats_ptr;
  dcgmJobRemove_ptr_t dcgmJobRemove_ptr;
  
  // other variables necessary for DCGM
  dcgmHandle_t dcgm_handle; // DCGM handle
  unsigned int gpu_ids[DCGM_MAX_NUM_DEVICES]; // GPU identifiers
  dcgmDeviceAttributes_t gpu_attributes[DCGM_MAX_NUM_DEVICES]; // GPU attributes
  int dcgm_gpu_count; // number of GPUs discovered by DCGM
  
  static char CSM_ENVIRONMENTAL_FIELD_GROUP[];
  static char CSM_ALLOCATION_FIELD_GROUP[];

  // DCGM GPU Field Group Fields
  static uint16_t CSM_ALLOCATION_FIELDS[];
  static const uint32_t CSM_ALLOCATION_FIELD_COUNT; 
 
  static uint16_t CSM_ENVIRONMENTAL_FIELDS[];
  static const uint32_t CSM_ENVIRONMENTAL_FIELD_COUNT; 

  // DCGM Handles, persisted across multiple calls to DCGM
  dcgmGpuGrp_t gpugrp; // handle to the group of gpus

  dcgmFieldGrp_t double_fields_grp;                    // handle to the double group of fields
  dcgmFieldGrp_t int64_fields_grp;                     // handle to the int64 group of fields
  dcgmFieldGrp_t csm_allocation_field_group_handle;    // handle to the CSM_ALLOCATION_FIELD_GROUP
  dcgmFieldGrp_t csm_environmental_field_group_handle; // handle to the CSM_ENVIRONMENTAL_FIELD_GROUP
 
  // Names of the fields associated with the field groups 
  std::vector<std::string> csm_allocation_field_names; 
  std::vector<std::string> csm_environmental_field_names; 

  // other variables
  long long updateFreq; // update frequency in us for the DCGM fields
  double maxKeepAge;    // number of secs that DCGM has to store the samples
  int maxKeepSamples;   // number of samples per field that DCGM will store
};

// Initialize static member of class INV_DCGM_ACCESS
// int INV_DCGM_ACCESS::objectCount = 0;

}   // namespace daemon
}  // namespace csm

#endif // DCGM support is enabled

/////////////////////////////////////////////////////////////////////////////////
//////// DCGM support is disabled
/////////////////////////////////////////////////////////////////////////////////

#ifndef DCGM

namespace csm {
namespace daemon {

class INV_DCGM_ACCESS
{
public:

  // functions

  static INV_DCGM_ACCESS* GetInstance(){ if( _Instance == nullptr ){ _Instance = new INV_DCGM_ACCESS(); } return _Instance; } // get the istance of the class object
  bool GetDlopenFlag(){ return dlopen_flag; } // dlopen flag, true if libdcgm_ptr is == nullptr
  bool GetDCGMInitFlag(){ return dcgm_init_flag; } // dcgm init flag, false if some modules were not loaded correcty or if some of the DCGM initialization functions failed
  ~INV_DCGM_ACCESS();
  
  bool ReadAllocationFields();
  bool CollectGpuData(std::list<boost::property_tree::ptree> &gpu_data_pt_list);
  bool StartAllocationStats(const int64_t &i_allocation_id);
  bool StopAllocationStats(const int64_t &i_allocation_id, int64_t &o_gpu_usage);

private:
  INV_DCGM_ACCESS();

private:

  // pointer to the class object
  static INV_DCGM_ACCESS *_Instance;

  // flags
  bool dlopen_flag;
  bool dcgm_init_flag;

};

}   // namespace daemon
}  // namespace csm

#endif // DCGM support is disabled

#endif /* CSMD_SRC_INV_INCLUDE_INV_DCGM_ACCESS_H_ */
