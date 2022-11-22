#!/usr/bin/python
####################################################
#    bbapiAdmin.py
#
#    Copyright IBM Corporation 2017,2017. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
###################################################

"""
    This module provides wrappers to the following bbapi functions.

int BB_ChangeMode(const char* pathname, mode_t mode)
int BB_ChangeOwner(const char* pathname, const char* owner, const char* group)
int BB_CloseServer(const char* name);
int BB_CreateDirectory(const char* newpathname)
int BB_CreateLogicalVolume(const char* mountpoint, const char* size, BBCREATEFLAGS flags=BBXFS)
int BB_GetServer(const char* type, size_t bufsize, char* buffer);
int BB_GetServerByName(const char* bbserverName, const char* type, size_t bufsize, char* buffer);
int BB_OpenServer(const char* name);
int BB_RemoveDirectory(const char* pathname)
int BB_RemoveJobInfo()
int BB_RemoveLogicalVolume(const char* mountpoint)
int BB_ResizeMountPoint(const char* mountpoint, char** logicalvolume, const char* size, BBRESIZEFLAGS flags)
int BB_RestartTransfers(const char* pHostName, const uint64_t pHandle, const char* pTransferDefs, const size_t pTransferDefsSize);
int BB_Resume(const char* pHostName);
int BB_RetrieveTransfers(const char* pHostName, const uint64_t pHandle, const BB_RTV_TRANSFERDEFS_FLAGS pFlags, size_t* pNumBytesAvail, const size_t pBufferSize, char* pBuffer);
int BB_SetServer(const char* type, const char* name);
int BB_StopTransfers(const char* pHostName, const uint64_t pHandle, const char* pTransferDefs, const size_t pTransferDefsSize);
int BB_Suspend(const char* pHostName);

    Not implemented

int BB_QueryServer(enum BBServerQuery, size_t bufsize, char* buffer);
int BB_SwitchServer(const char* buffer);
"""

from ctypes import *
import subprocess

import bb
from bbapi import *
import bbapiAdminProcs as adminProcs
from bberror import *
import time

PRINT_VALUE = True


#
# Miscellaneous
#

ValueMap = {"%%_NuLl_HoStNaMe_%%":"*", "":'""'}

LIBPATH = "/u/dlherms/git/bluecoral/bb/wrappers"


#
# Wrappers for official bbapiAdmin calls
#

def BB_ChangeMode(pPathName, pMode):
    l_PathName = bb.cvar("pathname", pPathName)
    l_Mode = bb.cvar("mode", pMode)

    print("%sBB_ChangeMode issued to change the mode for path %s to %s" % (os.linesep, pPathName, oct(pMode)))
    rc = bb.api.BB_ChangeMode(l_PathName, l_Mode)
    if (rc):
        raise BB_ChangeModeError(rc)

    bb.printLastErrorDetailsSummary()
    print("Mode for path %s changed to %s" % (pPathName, oct(pMode)))

    return

def BB_ChangeOwner(pPathName, pOwner, pGroup):
    l_PathName = bb.cvar("pathname", pPathName)
    l_Owner = bb.cvar("owner", pOwner)
    l_Group = bb.cvar("group", pGroup)

    print('%sBB_ChangeOwner issued to change the ownership of path %s to user "%s", group "%s"' % (os.linesep, pPathName, pOwner, pGroup))
    rc = bb.api.BB_ChangeOwner(l_PathName, l_Owner, l_Group)
    if (rc):
        raise BB_ChangeOwnerError(rc)

    bb.printLastErrorDetailsSummary()
    print('Ownership of path %s changed to user "%s", group "%s"' % (pPathName, pOwner, pGroup))

    return

def BB_CloseServer(pName):
    l_Name = bb.cvar("name", pName)

    print("%sBB_CloseServer issued for name %s" % (os.linesep, pName))
    rc = bb.api.BB_CloseServer(l_Name)
    if (rc):
        raise BB_CloseServerError(rc)

    bb.printLastErrorDetailsSummary()

    print("%sBB_CloseServer completed for name %s" % (os.linesep, pName))

    return

def BB_CreateDirectory(pNewPathName):
    l_NewPathName = bb.cvar("newpathname", pNewPathName)

    print("%sBB_CreateDirectory issued to create directory %s" % (os.linesep, pNewPathName))
    rc = bb.api.BB_CreateDirectory(l_NewPathName)
    if (rc):
        raise BB_CreateDirectoryError(rc)

    bb.printLastErrorDetailsSummary()
    print("Directory %s created" % (pNewPathName))

    return

def BB_CreateLogicalVolume(pMountpoint, pSize, pFlags=DEFAULT_BBCREATEFLAGS):
    l_NormalRCs = BB_CreateLogicalVolumeError(BBError(Exception())).getNormalRCs()
    l_ToleratedErrorRCs = BB_CreateLogicalVolumeError(BBError(Exception())).getToleratedErrorRCs()
    rc = l_ToleratedErrorRCs[0]

    l_Mountpoint = bb.cvar("mountpoint", pMountpoint)
    l_Size = bb.cvar("size_str", pSize)
    l_Flags = bb.cvar("flags", pFlags)

    print("%sBB_CreateLogicalVolume issued to create logical volume with size %s, directory %s mounted, file system flag %s" % (os.linesep, pSize, pMountpoint, BBCREATEFLAGS[l_Flags.value]))

    while (True):
        rc = bb.api.BB_CreateLogicalVolume(l_Mountpoint, l_Size, l_Flags)
        if (rc in (l_NormalRCs + l_ToleratedErrorRCs)):
            if (rc in l_ToleratedErrorRCs):
                dummy = BBError()
                if ("Attempt to retry" in dummy.getLastErrorDetailsSummary()):
                    print("Logical volume cannot be created because of a suspended condition.  This create logical volume request will be attempted again in 15 seconds.")
                    time.sleep(15)
                else:
                    print("Logical volume cannot be created now, rc %d. See error details." % (rc))
                    break
            else:
                break
        else:
            raise BB_CreateLogicalVolumeError(rc)

    bb.printLastErrorDetailsSummary()
    if (rc in l_NormalRCs):
        print("Logical volume created with size %s, directory %s mounted, file system flag %s" % (pSize, pMountpoint, BBCREATEFLAGS[l_Flags.value]))

    return

def BB_GetServer(pType, pPrintOption=True):
    l_BufferSize = 512

    l_Type = bb.cvar("type", pType)
    l_Size = bb.cvar("size", l_BufferSize)
    l_Buffer = bb.cvar("buffer", l_BufferSize)

    if (pPrintOption):
        print("%sBB_GetServer issued for type %s" % (os.linesep, pType))
    rc = bb.api.BB_GetServer(l_Type, l_Size, byref(l_Buffer))
    if (rc):
        raise BB_GetServerError(rc)

    if (pPrintOption):
        bb.printLastErrorDetailsSummary()

    l_Temp = []
    for i in range(l_Size.value):
        if l_Buffer[i] != b'\0':
            l_Temp.append(l_Buffer[i])
        else:
            break
    l_Output = (b"".join(l_Temp)).decode()
    l_TypePrt = list(pType)
    l_TypePrt[0] = l_TypePrt[0].upper()
    if (pPrintOption):
        print("%s Server: %s" % ("".join(l_TypePrt), l_Output))

    return l_Output

def BB_GetServerByName(pName, pType, pPrintOption=True):
    l_BufferSize = 512

    l_Name = bb.cvar("type", pName)
    l_Type = bb.cvar("type", pType)
    l_Size = bb.cvar("size", l_BufferSize)
    l_Buffer = bb.cvar("buffer", l_BufferSize)

    if (pPrintOption):
        print("%sBB_GetServerByName issued for name %s, type %s" % (os.linesep, pName, pType))
    rc = bb.api.BB_GetServerByName(l_Name, l_Type, l_Size, byref(l_Buffer))
    if (rc):
        raise BB_GetServerByNameError(rc)

    if (pPrintOption):
        bb.printLastErrorDetailsSummary()

    l_Temp = []
    for i in range(l_Size.value):
        if l_Buffer[i] != b'\0':
            l_Temp.append(l_Buffer[i])
        else:
            break
    l_Output = (b"".join(l_Temp)).decode()
    if (pPrintOption):
        print("%s Server: %s  Option: %s  Result: %s" % (os.linesep, pName, pType, l_Output))

    return l_Output

def BB_OpenServer(pName):
    l_Name = bb.cvar("name", pName)

    print("%sBB_OpenServer issued for name %s" % (os.linesep, pName))
    rc = bb.api.BB_OpenServer(l_Name)
    if (rc):
        raise BB_OpenServerError(rc)

    bb.printLastErrorDetailsSummary()

    print("%sBB_OpenServer completed for name %s" % (os.linesep, pName))

    return

def BB_RemoveDirectory(pPathName):
    l_PathName = bb.cvar("pathname", pPathName)

    print("%sBB_RemoveDirectory issued to remove directory %s" % (os.linesep, pPathName))
    rc = bb.api.BB_RemoveDirectory(l_PathName)
    if (rc):
        raise BB_RemoveDirectoryError(rc)

    bb.printLastErrorDetailsSummary()
    print("Directory %s removed" % (pPathName))

    return

def BB_RemoveJobInfo():
    print("%sBB_RemoveJobInfo issued" % (os.linesep))

    rc = bb.api.BB_RemoveJobInfo()
    if (rc):
        raise BB_RemoveJobInfoError(rc)

    bb.printLastErrorDetailsSummary()
    print("BB_RemoveJobInfo completed")

    return

def BB_RemoveLogicalVolume(pMountpoint):
    l_NormalRCs = BB_RemoveLogicalVolumeError(BBError(Exception())).getNormalRCs()
    l_ToleratedErrorRCs = BB_RemoveLogicalVolumeError(BBError(Exception())).getToleratedErrorRCs()

    l_Mountpoint = bb.cvar("mountpoint", pMountpoint.encode())

    print("%sBB_RemoveLogicalVolume issued to remove the logical volume associated with mountpoint %s" % (os.linesep, pMountpoint))

    rc = bb.api.BB_RemoveLogicalVolume(l_Mountpoint)
    while ((rc not in l_NormalRCs) and (rc not in l_ToleratedErrorRCs)):
        dummy = BBError()
        if ("Device or resource busy" not in dummy.getLastErrorDetailsSummary()):
            raise BB_RemoveLogicalVolumeError(rc)
        else:
            # NOTE: This could be a 'normal' case where restart transfer definition has not completed before remove logical volume
            #       is attempted in the normal flow of operations from the original start transfer request.
            #       The restart transfer may take 'minutes' attempting to determine if the transfer definition is in a stopped state.
            print("Device or resource busy.  The remove logical volume for mountpoint %s request will be re-attempted in 30 seconds." % (pMountpoint))
            time.sleep(30)
            rc = bb.api.BB_RemoveLogicalVolume(l_Mountpoint)

    bb.printLastErrorDetailsSummary()
    if (rc in l_NormalRCs):
        print("Logical volume associated with mountpoint %s removed" % (pMountpoint))

    return

def BB_ResizeMountPoint(pMountpoint, pSize, pFlags=DEFAULT_BBRESIZEFLAGS):
    l_Mountpoint = bb.cvar("mountpoint", pMountpoint)
    l_Size = bb.cvar("size_str", pSize)
    l_Flags = bb.cvar("flags", pFlags)

    print("%sBB_ResizeMountPoint issued to have mountpoint %s resized with a size specification of %s and flags %s" % (os.linesep, pMountpoint, pSize, BBRESIZEFLAGS[l_Flags.value]))
    rc = bb.api.BB_ResizeMountPoint(l_Mountpoint, l_Size, l_Flags)
    if (rc):
        raise BB_ResizeMountPointError(rc)

    bb.printLastErrorDetailsSummary()
    print("Mountpoint %s resized with a size specification of %s and flags %s" % (pMountpoint, pSize, BBRESIZEFLAGS[l_Flags.value]))

    return

def BB_RestartTransfers(pHostName, pHandle, pTransferDefs, pTransferDefsSize):
    l_NormalRCs = BB_RestartTransfersError(BBError(Exception())).getNormalRCs()
    l_ToleratedErrorRCs = BB_RestartTransfersError(BBError(Exception())).getToleratedErrorRCs()
    rc = l_ToleratedErrorRCs[0]

    l_HostName = bb.cvar("hostname", pHostName)
    l_Handle = bb.cvar("handle", pHandle)
    l_NumberOfRestartedTransferDefs = bb.cvar("numtransferdefs", 0)

    print('%sBB_RestartTransfers issued to restart transfer definitions using this criteria:  hostname %s, handle %d, transferdefs size %d, transferdefs %s' % (os.linesep, ValueMap.get(l_HostName.value, l_HostName.value), l_Handle.value, pTransferDefsSize.value, pTransferDefs.value))

    while (True):
        rc = bb.api.BB_RestartTransfers(l_HostName, l_Handle, byref(l_NumberOfRestartedTransferDefs), byref(pTransferDefs), pTransferDefsSize)
        if (rc in (l_NormalRCs + l_ToleratedErrorRCs)):
            if (rc in l_ToleratedErrorRCs):
                dummy = BBError()
                if ("Attempt to retry" in dummy.getLastErrorDetailsSummary()):
                    print("Restart transfers cannot be performed for handle %s because of a suspended condition.  This restart transfers request will be attempted again in 15 seconds." % (l_Handle))
                    time.sleep(15)
                else:
                    print("Restart transfers cannot be performed for handle %s. See error details." % (l_Handle))
                    break
            else:
                break
        else:
            raise BB_RestartTransfersError(rc)


    bb.printLastErrorDetailsSummary()
    print('%sBB_RestartTransfers completed' % (os.linesep))

    if (rc in l_NormalRCs):
        return l_NumberOfRestartedTransferDefs.value

def BB_Resume(pHostHame):
    l_HostName = bb.cvar("hostname", pHostHame)

    print('%sBB_Resume issued for hostname %s' % (os.linesep, ValueMap.get(l_HostName.value, l_HostName.value)))

    rc = bb.api.BB_Resume(l_HostName)
    if (rc):
        raise BB_ResumeError(rc)

    bb.printLastErrorDetailsSummary()
    print('%sBB_Resume completed for hostname %s' % (os.linesep, ValueMap.get(l_HostName.value, l_HostName.value)))

    return

def BB_RetrieveTransfers(pHostHame, pHandle, pFlags=DEFAULT_BB_RTV_TRANSFERDEFS_FLAGS):
    l_HostName = bb.cvar("hostname", pHostHame)
    l_Handle = bb.cvar("handle", pHandle)
    l_Flags = bb.cvar("flags", pFlags)
    l_BufferSizeIncr = 16*1024

    l_NumTransferDefs = bb.cvar("numtransferdefs", 0)
    l_NumBytesAvailable = bb.cvar("numbytesavailable", l_BufferSizeIncr)
    l_BufferSize = bb.cvar("size", 0)
    while (l_NumBytesAvailable.value > l_BufferSize.value):
        l_BufferSize = bb.cvar("size", ((l_NumBytesAvailable.value + (l_BufferSizeIncr - 1)) / l_BufferSizeIncr) * l_BufferSizeIncr)
        l_Buffer = bb.cvar("buffer", l_BufferSize.value)
        print('%sBB_RetrieveTransfers issued to retrieve transfer definitions using this criteria:  hostname %s, handle %d, flags %s, bytesavailable %d, bytesprovided %d' % (os.linesep, ValueMap.get(l_HostName.value, l_HostName.value), l_Handle.value, BB_RTV_TRANSFERDEFS_FLAGS[l_Flags.value], l_NumBytesAvailable.value, l_BufferSize.value))
        rc = bb.api.BB_RetrieveTransfers(l_HostName, l_Handle, l_Flags, byref(l_NumTransferDefs), byref(l_NumBytesAvailable), l_BufferSize, byref(l_Buffer))
        if (rc):
            raise BB_RetrieveTransfersError(rc)

    bb.printLastErrorDetailsSummary()
#    print '%sBB_RetrieveTransfers completed the retrieval of transfer definitions using this criteria:  hostname %s, handle %d, flags %s, number of transferdefs %d, length of transferdefs %d, transferdefs |%s|' % (os.linesep, ValueMap.get(l_HostName.value, l_HostName.value), l_Handle.value, BB_RTV_TRANSFERDEFS_FLAGS[l_Flags.value], l_NumTransferDefs.value, l_NumBytesAvailable.value, l_Buffer.value)
    print('%sBB_RetrieveTransfers completed the retrieval of transfer definitions using this criteria:  hostname %s, handle %d, flags %s, number of transferdefs %d.' % (os.linesep, ValueMap.get(l_HostName.value, l_HostName.value), l_Handle.value, BB_RTV_TRANSFERDEFS_FLAGS[l_Flags.value], l_NumTransferDefs.value))

    return (l_NumTransferDefs.value, l_Buffer, l_NumBytesAvailable)

def BB_SetServer(pType, pName):
    l_Type = bb.cvar("type", pType)
    l_Name = bb.cvar("name", pName)

    print("%sBB_SetServer issued for type %s, name %s" % (os.linesep, pType, pName))
    rc = bb.api.BB_SetServer(l_Type, l_Name)
    if (rc):
        raise BB_SetServerError(rc)

    bb.printLastErrorDetailsSummary()

    print("%sBB_SetServer completed for type %s, name %s" % (os.linesep, pType, pName))

    return

def BB_StopTransfers(pHostName, pHandle, pTransferDefs, pTransferDefsSize):
    l_HostName = bb.cvar("hostname", pHostName)
    l_Handle = bb.cvar("handle", pHandle)
    l_NumStoppedTransferDefs = bb.cvar("numtransferdefs", 0)

    print('%sBB_StopTransfers issued to retrieve transfer definitions using this criteria:  hostname %s, handle %d, transferdefs size %d, transferdefs %s' % (os.linesep, ValueMap.get(l_HostName.value, l_HostName.value), l_Handle.value, pTransferDefsSize.value, pTransferDefs.value))

    rc = bb.api.BB_StopTransfers(l_HostName, l_Handle, byref(l_NumStoppedTransferDefs), byref(pTransferDefs), pTransferDefsSize)
    if (rc):
        raise BB_StopTransfersError(rc)

    bb.printLastErrorDetailsSummary()
    print('%sBB_StopTransfers completed' % (os.linesep))

    return l_NumStoppedTransferDefs.value

def BB_Suspend(pHostHame):
    l_HostName = bb.cvar("hostname", pHostHame)

    print('%sBB_Suspend issued for hostname %s' % (os.linesep, ValueMap.get(l_HostName.value, l_HostName.value)))

    rc = bb.api.BB_Suspend(l_HostName)
    if (rc):
        raise BB_SuspendError(rc)

    bb.printLastErrorDetailsSummary()
    print('%sBB_Suspend completed for hostname %s' % (os.linesep, ValueMap.get(l_HostName.value, l_HostName.value)))

    return
