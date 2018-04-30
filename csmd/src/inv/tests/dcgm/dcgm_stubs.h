/*================================================================================

    csmd/src/inv/tests/dcgm/dcgm_stubs.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __DCGM_STUBS_H_
#define __DCGM_STUBS_H_

#define DCGM_MAX_NUM_DEVICES 16
#define DCGM_MAX_STR_LENGTH 256

enum dcgmReturn_t
{
  DCGM_ST_OK,
  DCGM_ST_UNINITIALIZED
};

enum dcgmOperationMode_t
{
  DCGM_OPERATION_MODE_AUTO = 1,
  DCGM_OPERATION_MODE_MANUAL = 2
};

typedef int dcgmHandle_t;

typedef struct dcgmDeviceSupportedClockSets_t
{
} dcgmDeviceSupportedClockSets_t;

typedef struct dcgmDeviceThermals_t
{
} dcgmDeviceThermals_t;

typedef struct dcgmDevicePowerLimits_t
{
} dcgmDevicePowerLimits_t;

typedef struct dcgmDeviceIdentifiers_t
{
  unsigned int version;
  char brandName[DCGM_MAX_STR_LENGTH];
  char deviceName[DCGM_MAX_STR_LENGTH];
  char pciBusId[DCGM_MAX_STR_LENGTH];
  char serial[DCGM_MAX_STR_LENGTH];
  char uuid[DCGM_MAX_STR_LENGTH];
  char vbios[DCGM_MAX_STR_LENGTH];
  char inforomImageVersion[DCGM_MAX_STR_LENGTH];
} dcgmDeviceIdentifiers_t;

typedef struct dcgmDeviceAttributes_t
{
  unsigned int version;
  struct dcgmDeviceSupportedClockSets_t clockSets;
  struct dcgmDeviceThermals_t thermalSettings;
  struct dcgmDevicePowerLimits_t powerLimits;
  struct dcgmDeviceIdentifiers_t identifiers;
} dcgmDeviceAttributes_t;

dcgmReturn_t dcgmInit (char *ipAddress, dcgmOperationMode_t opMode, dcgmHandle_t *pDcgmHandle)
{
  return DCGM_ST_OK;
}

// This is the current documented API, but I think this is incorrect:
// dcgmReturn_t dcgmGetAllDevices (dcgmHandle_t pDcgmHandle, unsigned int gpuIdList, int *count)
// I believe this is the correct API:
dcgmReturn_t dcgmGetAllDevices (dcgmHandle_t pDcgmHandle, unsigned int *gpuIdList, int *count)
{
  return DCGM_ST_OK;
}

dcgmReturn_t dcgmGetDeviceAttributes (dcgmHandle_t pDcgmHandle, unsigned int gpuId, dcgmDeviceAttributes_t *pDcgmAttr)
{
  return DCGM_ST_OK;
}

dcgmReturn_t dcgmShutdown (dcgmHandle_t pDcgmHandle)
{
  return DCGM_ST_OK;
}

#endif
