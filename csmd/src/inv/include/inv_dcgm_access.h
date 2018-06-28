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

/////////////////////////////////////////////////////////////////////////////////
////// DCGM support is enabled
/////////////////////////////////////////////////////////////////////////////////

#ifdef DCGM
#include "dcgm_agent.h"
#include "dcgm_fields.h"
#include "dcgm_structs.h"
#include <dlfcn.h>
#include <string>
#include <vector>

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

  // functions

  static INV_DCGM_ACCESS* GetInstance(){ if( _Instance == nullptr ){ _Instance = new INV_DCGM_ACCESS(); } return _Instance; } // get the istance of the class object
  void Init(); // initialization for dlopen and  DCGM
  bool GetDCGMInstalledFlag(){ return dcgm_installed_flag; }; // dcgm installed flag, equal to true if dcgm is installed
  bool GetDlopenFlag(){ return dlopen_flag; } // dlopen flag, true if libdcgm_ptr is == nullptr
  bool GetDCGMInitFlag(){ return dcgm_init_flag; } // dcgm init flag, false if some modules were not loaded correcty or if some of the DCGM initialization functions failed
  bool GetDCGMUsedFlag(){ return dcgm_used_flag; } // dcgm used flag, true if DCGM is not installed, if Dlopen did not work correctly or if DCGM was not initialized correctly 
  unsigned int * GetGPUsIdsPointer(){ return &(gpu_ids[0]); } // pointer to the array of the gpu_ids
  dcgmDeviceAttributes_t * GetGPUsAttributesPointer(){ return &(gpu_attributes[0]); } // pointer to the arrray of the dcgmDeviceAttributes_ts
  int DCGMGPUCount(){ return dcgm_gpu_count; } // number of gpu of the node as counted by dcgm
  bool GetGPUsAttributes(); // populate the gpu_ids array and the gpu_attributes array, return false if something went wrong
  bool LogGPUsEnviromentalData(); // log GPU enviromental data
  void Get_Double_DCGM_Field_Values( vector<double> &vector_double_dcgm_field_values ); // transfer the double dcgm field values to the input vector
  void Get_Long_DCGM_Field_Values( vector<long> &vector_int64_dcgm_field_values ); // transfer the i64 dcgm field values to the input vector
  void Get_Double_DCGM_Field_String_Identifiers( vector<string> &vector_labels_for_double_dcgm_fields ); // transfer the double dcgm field string identifiers to the input vector
  void Get_Long_DCGM_Field_String_Identifiers( vector<string> &vector_labels_for_int64_dcgm_fields ); // transfer the i64 dcgm field string identifiers to the input vector
  ~INV_DCGM_ACCESS();

  int list_field_values_since(unsigned int gpuId, dcgmFieldValue_t *values, int numValues, void *userData);
  int list_field_values(unsigned int gpuId, dcgmFieldValue_t *values, int numValues, void *userData); 

  //static int objectCount = 0;

  /**
   * Read the GPU metrics that are needed at the start and end of an allocation and return them to the caller.
   *
   * @return true if successful, false if unsuccessful. 
   */
  bool ReadAllocationFields();

private:
  INV_DCGM_ACCESS();
  //static int objectCount;

  
  /**
   * Attempt to initialize the function pointers to DCGM functions using dlsym().
   *
   * @return true if successful, false if unsuccessful. 
   */
  bool InitializeFunctionPointers();

  /**
   * Attempt to create a field group for CSM to use in DCGM.
   *
   * @return true if successful, false if unsuccessful. 
   */
  bool CreateCsmFieldGroup(const uint32_t field_count, uint16_t fields[], char* field_group_name, dcgmFieldGrp_t* field_group_handle);

  bool DeleteCsmFieldGroup();


private:

  // pointer to the class object
  static INV_DCGM_ACCESS *_Instance;

  // flags
  bool dcgm_installed_flag;
  bool dlopen_flag;
  bool dcgm_init_flag;
  bool dcgm_used_flag;

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

  // other variables necessary for DCGM
  dcgmHandle_t dcgm_handle; // DCGM handle
  unsigned int gpu_ids[DCGM_MAX_NUM_DEVICES]; // GPU identifiers
  dcgmDeviceAttributes_t gpu_attributes[DCGM_MAX_NUM_DEVICES]; // GPU attributes
  int dcgm_gpu_count; // number of GPUs discovered by DCGM
  
  // other variables necessary for the fields
  unsigned int number_of_double_fields; // number of double fields to query
  unsigned int number_of_int64_fields; // number of int64 fields to query

  vector<unsigned short> vector_of_ids_of_double_fields; // vector of ids of double fields to query
  vector<unsigned short> vector_of_ids_of_int64_fields; // vector of ids of int64 fields to query

  vector<dcgmFieldValue_t> vector_of_double_values; // vector of double values for the fields
  vector<dcgmFieldValue_t> vector_of_int64_values; // vector of int64 values for the fields
  
  // DCGM GPU Group Names
  static char CSM_GPU_GROUP[];

  // DCGM GPU Field Group Names
  static char CSM_DOUBLE_FIELDS[];
  static char CSM_INT64_FIELDS[];
  static char CSM_ENVIRONMENTAL_FIELD_GROUP[];
  static char CSM_ALLOCATION_FIELD_GROUP[];

  // DCGM GPU Field Group Fields
  static uint16_t CSM_ALLOCATION_FIELDS[];
  static const uint32_t CSM_ALLOCATION_FIELD_COUNT; 

  // DCGM Handles, persisted across multiple calls to DCGM
  dcgmGpuGrp_t gpugrp; // handle to the group of gpus

  dcgmFieldGrp_t double_fields_grp;                    // handle to the double group of fields
  dcgmFieldGrp_t int64_fields_grp;                     // handle to the int64 group of fields
  dcgmFieldGrp_t csm_allocation_field_group_handle;    // handle to the CSM_ALLOCATION_FIELD_GROUP

  // other variables
  long long updateFreq; // update frequency in us for the DCGM fields
  double maxKeepAge;    // number of secs that DCGM has to store the samples
  int maxKeepSamples;   // number of samples per field that DCGM will store

  vector<vector<double>> vector_old_double_fields_value;
  vector<vector<long>> vector_old_int64_fields_value;

  vector<unsigned int> vector_flag_old_double_data;
  vector<unsigned int> vector_flag_old_int64_data;

  // additional variables for the fields

  vector<string> dcgm_meta_double_vector;
  vector<string> dcgm_meta_int64_vector;

  vector<vector<double>> vector_for_double_values_storage; // vector for the storage of double values for the fields
  vector<vector<long>> vector_for_int64_values_storage; // vector for the storage of int64 values for the fields

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

#include <vector>
#include <string>

namespace csm {
namespace daemon {

class INV_DCGM_ACCESS
{
public:

  // functions

  static INV_DCGM_ACCESS* GetInstance(){ if( _Instance == nullptr ){ _Instance = new INV_DCGM_ACCESS(); } return _Instance; } // get the istance of the class object
  bool GetDCGMInstalledFlag(){ return dcgm_installed_flag; } // dcgm installed flag, equal to true if dcgm is installed
  bool GetDlopenFlag(){ return dlopen_flag; } // dlopen flag, true if libdcgm_ptr is == nullptr
  bool GetDCGMInitFlag(){ return dcgm_init_flag; } // dcgm init flag, false if some modules were not loaded correcty or if some of the DCGM initialization functions failed
  bool GetDCGMUsedFlag(){ return dcgm_used_flag; } // dcgm used flag, true if DCGM is not installed, if Dlopen did not work correctly or if DCGM was not initialized correctly
  bool LogGPUsEnviromentalData(); // log GPU enviromental data
  void Get_Double_DCGM_Field_Values( std::vector<double> &vector_double_dcgm_field_values ); // transfer the double dcgm field values to the input vector
  void Get_Long_DCGM_Field_Values( std::vector<long> &vector_int64_dcgm_field_values ); // transfer the i64 dcgm field values to the input vector
  void Get_Double_DCGM_Field_String_Identifiers( std::vector<std::string> &vector_labels_for_double_dcgm_field_strings ); // transfer the double dcgm field string identifiers to the input vector
  void Get_Long_DCGM_Field_String_Identifiers( std::vector<std::string> &vector_labels_for_int64_dcgm_fields ); // transfer the i64 dcgm field string identifiers to the input vector
  ~INV_DCGM_ACCESS();

private:
  INV_DCGM_ACCESS();

private:

  // pointer to the class object
  static INV_DCGM_ACCESS *_Instance;

  // flags
  bool dcgm_installed_flag;
  bool dlopen_flag;
  bool dcgm_init_flag;
  bool dcgm_used_flag;


};

}   // namespace daemon
}  // namespace csm

#endif // DCGM support is disabled

#endif /* CSMD_SRC_INV_INCLUDE_INV_DCGM_ACCESS_H_ */
