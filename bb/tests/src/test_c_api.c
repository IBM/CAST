/*******************************************************************************
 |    test_c_api.c
 |
 |  The purpose of the progam is to verify that the burst buffer API interfaces,
 |  as defined in bbapi.h, match the actual implementations for those APIs.
 |  The intent is that this program should successfully compile and link.
 |  It is not intended for this program to be run.
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

#include "bb/include/bbapi.h"
#include "bb/include/bbapiAdmin.h"

int main(int argc, char** argv)
{
    int rc;
    uint32_t uint32 = 0;
    uint32_t* uint32ptr = 0;
    uint64_t uint64 = 0;
    mode_t l_Mode = (mode_t)0;
    BBERRORFORMAT l_ErrorFormat = (BBERRORFORMAT)0;
    BBFILEFLAGS l_FileFlags = (BBFILEFLAGS)0;
    BBCREATEFLAGS l_CreateFlags = (BBCREATEFLAGS)0;
    BBRESIZEFLAGS l_ResizeFlags = (BBRESIZEFLAGS)0;
    BB_RTV_TRANSFERDEFS_FLAGS l_RetrieveTransferFlags = (BB_RTV_TRANSFERDEFS_FLAGS)0;
    BBTAG l_Tag = (BBTAG)0;
    BBCANCELSCOPE l_CancelScope = (BBCANCELSCOPE)0;
    BBSTATUS l_Status = (BBSTATUS)0;
    size_t l_Size = (size_t)0;
    size_t* l_SizePtr = 0;
    BBTransferDef_t l_TransferDef;
    BBTransferDef_t* l_TransferDefPtr;
    BBTransferHandle_t l_Handle;
    BBTransferInfo_t l_TransferInfo;
    BBUsage_t l_Usage;
    BBDeviceUsage_t l_DeviceUsage;
    char l_CharArray[16] = {'\0'};
    char* l_CharArrayPtr = 0;
    uint32_t uint32Array[2] = {0, 0};
    BBTransferHandle_t l_HandleArray[2] = {0, 0};

/*
extern int BB_InitLibrary(uint32_t contribId, const char* clientVersion);
extern int BB_TerminateLibrary();
extern int BB_GetLastErrorDetails(BBERRORFORMAT format, size_t* numAvailBytes, size_t buffersize, char* bufferForErrorDetails);
extern int BB_GetVersion(size_t size, char* APIVersion);
extern int BB_CreateTransferDef(BBTransferDef_t** transfer);
extern int BB_AddFiles(BBTransferDef_t* transfer, const char* source, const char* target, BBFILEFLAGS flags);
extern int BB_AddKeys(BBTransferDef_t* transfer, const char* key, const char* value);
extern int BB_FreeTransferDef(BBTransferDef_t* transfer);
extern int BB_GetTransferHandle(BBTAG tag, uint64_t numcontrib, uint32_t contrib[], BBTransferHandle_t* handle);
extern int BB_StartTransfer(BBTransferDef_t* transfer, BBTransferHandle_t handle);
extern int BB_CancelTransfer(BBTransferHandle_t handle, BBCANCELSCOPE scope);
extern int BB_GetTransferKeys(BBTransferHandle_t handle, size_t buffersize, char* bufferForKeyData);
extern int BB_GetTransferList(BBSTATUS matchstatus, uint64_t* numHandles, BBTransferHandle_t array_of_handles[], uint64_t* numAvailHandles);
extern int BB_GetTransferInfo(BBTransferHandle_t handle, BBTransferInfo_t* info);
extern int BB_GetThrottleRate(const char* mountpoint, uint64_t* rate);
extern int BB_SetThrottleRate(const char* mountpoint, uint64_t rate);
extern int BB_GetUsage(const char* mountpoint, BBUsage_t* usage);
extern int BB_SetUsageLimit(const char* mountpoint, BBUsage_t* usage);
extern int BB_GetDeviceUsage(uint32_t devicenum, BBDeviceUsage_t* usage);
extern int BB_CreateLogicalVolume(const char* mountpoint, const char* size, BBCREATEFLAGS flags);
extern int BB_ResizeMountPoint(const char* mountpoint, char** logicalvolume, const char* newsize, BBRESIZEFLAGS flags);
extern int BB_RemoveLogicalVolume(const char* mountpoint);
extern int BB_CreateDirectory(const char* newpathname);
extern int BB_ChangeOwner(const char* pathname, const char* newuser, const char* newgroup);
extern int BB_ChangeMode(const char* pathname, mode_t mode);
extern int BB_RemoveDirectory(const char* pathname);
extern int BB_RemoveJobInfo();
extern int BB_Suspend(const char* pHostName);
extern int BB_Resume(const char* pHostName);
extern int BB_RetrieveTransfers(const char* pHostName, const uint64_t pHandle, const BB_RTV_TRANSFERDEFS_FLAGS pFlags, uint32_t* pNumTransferDefs, size_t* pTransferDefsSize, const size_t pBufferSize, char* pBuffer);
extern int BB_StopTransfers(const char* pHostName, const uint64_t pHandle, uint32_t* pNumStoppedTransferDefs, const char* pTransferDefs, const size_t pTransferDefsSize);
extern int BB_RestartTransfers(const char* pHostName, const uint64_t pHandle, uint32_t* pNumRestartedTransferDefs, const char* pTransferDefs, const size_t pTransferDefsSize);

*/
    rc=0;

    rc=BB_InitLibrary(uint32, l_CharArray);
    rc=BB_TerminateLibrary();
    rc=BB_GetLastErrorDetails(l_ErrorFormat, &l_Size, l_Size, l_CharArrayPtr);
    rc=BB_GetVersion(l_Size, l_CharArray);
    rc=BB_CreateTransferDef(&l_TransferDefPtr);
    rc=BB_AddFiles(&l_TransferDef, l_CharArray, l_CharArray, l_FileFlags);
    rc=BB_AddKeys(&l_TransferDef, l_CharArray, l_CharArray);
    rc=BB_FreeTransferDef(&l_TransferDef);
    rc=BB_GetTransferHandle(l_Tag, uint64, uint32Array, &l_Handle);
    rc=BB_GetTransferKeys(l_Handle, l_Size, l_CharArrayPtr);
    rc=BB_StartTransfer(&l_TransferDef, l_Handle);
    rc=BB_CancelTransfer(l_Handle, l_CancelScope);
    rc=BB_GetTransferKeys(l_Handle, l_Size, l_CharArrayPtr);
    rc=BB_GetTransferList(l_Status, &uint64, l_HandleArray, &uint64);
    rc=BB_GetTransferInfo(l_Handle, &l_TransferInfo);
    rc=BB_GetThrottleRate(l_CharArray, &uint64);
    rc=BB_SetThrottleRate(l_CharArray, uint64);
    rc=BB_GetUsage(l_CharArray, &l_Usage);
    rc=BB_SetUsageLimit(l_CharArray, &l_Usage);
    rc=BB_GetDeviceUsage(uint32, &l_DeviceUsage);
    rc=BB_CreateLogicalVolume(l_CharArray, l_CharArray, l_CreateFlags);
    rc=BB_ResizeMountPoint(l_CharArray, l_CharArray, l_ResizeFlags);
    rc=BB_RemoveLogicalVolume(l_CharArray);
    rc=BB_CreateDirectory(l_CharArray);
    rc=BB_ChangeOwner(l_CharArray, l_CharArray, l_CharArray);
    rc=BB_ChangeMode(l_CharArray, l_Mode);
    rc=BB_RemoveDirectory(l_CharArray);
    rc=BB_RemoveJobInfo();
    rc=BB_Suspend(l_CharArrayPtr);
    rc=BB_Resume(l_CharArrayPtr);
    rc=BB_RetrieveTransfers(l_CharArrayPtr, l_Handle, l_RetrieveTransferFlags, uint32ptr, l_SizePtr, l_Size, l_CharArrayPtr);
    rc=BB_StopTransfers(l_CharArrayPtr, l_Handle, uint32ptr, l_CharArrayPtr, l_Size);
    rc=BB_RestartTransfers(l_CharArrayPtr, l_Handle, uint32ptr, l_CharArrayPtr, l_Size);

    return rc;
}
