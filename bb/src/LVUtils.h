/*******************************************************************************
 |    LVUtils.h
 |
 |  � Copyright IBM Corporation 2015,2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/


#ifndef BB_LVUTILS_H_
#define BB_LVUTILS_H_

#include <vector>

#include <uuid/uuid.h>

#include "bbinternal.h"


/*******************************************************************************
 | Forward declarations
 *******************************************************************************/
class BBTransferDef;


/*******************************************************************************
 | Enumerators
 *******************************************************************************/
enum UMOUNT_ERROR_OPTION
{
    NO_ERROR_REPORTING       = 0,
    NORMAL_ERROR_REPORTING   = 1
};
typedef enum UMOUNT_ERROR_OPTION UMOUNT_ERROR_OPTION;


/*******************************************************************************
 | External functions
 *******************************************************************************/
extern void findBB_DevNames(vector<string>& pDevNames, const FIND_BB_DEVNAMES_OPTION pOption);
extern int getFileSystemType(const char* pDevName, char* pFileSystemType, size_t pFileSystemTypeLength);
extern int isMounted(const char* pVolumeGroupName, const char* pLogicalVolume, char* pMountPoint, size_t pMountPointLength);
extern int isMounted(const char* pMountPoint, char* pDevName, const size_t pDevNameLength, char* pFileSysType, const size_t pFileSysTypeLength);
extern int removeLogicalVolume(const char* pMountPoint, Uuid pLVUuid, ON_ERROR_FILL_IN_ERRSTATE pOnErrorFillInErrState);
extern int resizeLogicalVolume(const char* pMountPoint, char* &pLogicalVolume, const char* pMountSize, const uint64_t pFlags);
extern int processContrib(const uint64_t pNumContrib, uint32_t pContrib[]);
extern int setupTransfer(BBTransferDef* transfer, Uuid &lvuuid, const uint64_t pJobId, const uint64_t pHandle, const uint32_t pContribId, vector<struct stat*>& pStats, const uint32_t pPerformOperation);
extern int startTransfer(BBTransferDef* transfer, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId);
extern int timeToResizeLogicalVolume(const uint64_t pLastByteTransferred, const uint64_t pLVStartByte, const size_t pLVTotalSize, char* pNewSize, size_t pNewSizeLength);
extern int lsofRunCmd( const char* pDirectory);

#endif /* BB_LVUTILS_H_ */
