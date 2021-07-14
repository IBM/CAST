#!/usr/bin/python
####################################################
#    bbapi.py
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

int BB_AddFiles(BBTransferDef_t* transfer, const char* source, const char* target, BBFILEFLAGS flags)
int BB_AddKeys(BBTransferDef_t* transfer, const char* key, const char* value)
int BB_CancelTransfer(BBTransferHandle_t pHandle, BBCANCELSCOPE pScope)
int BB_CreateTransferDef(BBTransferDef_t** transfer)
int BB_FreeTransferDef(BBTransferDef_t* transfer)
int BB_GetDeviceUsage(uint32_t devicenum, BBDeviceUsage_t* usage)
int BB_GetThrottleRate(const char* mountpoint, uint64_t* rate)
int BB_GetTransferHandle(BBTAG pTag, uint64_t pNumContrib, uint32_t pContrib[], BBTransferHandle_t* pHandle)
int BB_GetTransferInfo(BBTransferHandle_t pHandle, BBTransferInfo_t* pInfo)
int BB_GetTransferKeys(BBTransferHandle_t handle, size_t* buffersize, char* bufferForKeyData)
int BB_GetTransferList(BBSTATUS pMatchStatus, uint64_t* pNumHandles, BBTransferHandle_t pHandles[], uint64_t* pNumAvailHandles)
int BB_GetUsage(const char* mountpoint, BBUsage_t* usage)
int BB_SetThrottleRate(const char* mountpoint, uint64_t rate)
int BB_SetUsageLimit(const char* mountpoint, BBUsage_t* usage)
int BB_StartTransfer(BBTransferDef_t* pTransfer, BBTransferHandle_t pHandle)
"""

from ctypes import *
# import json

import bb
from bberror import *
import re
import time


#
# Flags/constants for bbapi interfaces
#

#
# BB_AddFiles BBFILEFLAGS
#
BBFILEFLAGS = { "None"                      : 0x0000,  #### No specification
                "BBRecursive"               : 0x0001,  #### Recursively transfer all files and subdirectories
                "BBTestStageIn"             : 0x0004,  #### Test value to force stagein for the file pair
                "BBTestStageOut"            : 0x0008,  #### Test value to force stageout for the file pair
                "BB_BSCFS_Transfer"         : 0x0010,  #### File transfer type -- BSCFS
                "INVALID_BBFILEFLAG"        : 0x270F,  #### Invalid test value for 'normal' file
                "INVALID_BSCFS_BBFILEFLAG"  : 0x2710,  #### Invalid test value for 'BSCFS' file
                0x0000 : "None"                     ,  #### No specification
                0x0001 : "BBRecursive"              ,  #### Recursively transfer all files and subdirectories
                0x0004 : "BBTestStageIn"            ,  #### Test value to force stagein for the file pair
                0x0008 : "BBTestStageOut"           ,  #### Test value to force stageout for the file pair
                0x0010 : "BB_BSCFS_Transfer"        ,  #### Test value to force stagein for the file pair
                0x270F : "INVALID_BBFILEFLAG"       ,  #### Invalid test value for 'normal' file
                0x2710 : "INVALID_BSCFS_BBFILEFLAG" ,  #### Invalid test value for 'BSCFS' file
               }
DEFAULT_BBFILEFLAG = BBFILEFLAGS["None"]
INVALID_BBFILEFLAG = BBFILEFLAGS["INVALID_BBFILEFLAG"]
INVALID_BSCFS_BBFILEFLAG = BBFILEFLAGS["INVALID_BSCFS_BBFILEFLAG"]

BBFileBSCFS           = 0x0010  #### Test value for file transfer order of 0 (highest)
BBFileTransferOrder_0 = 0x0000  #### Test value for file transfer order of 0 (highest)
BBFileTransferOrder_1 = 0x0040  #### Test value for file transfer order of 1
BBFileTransferOrder_2 = 0x0080  #### Test value for file transfer order of 2
BBFileTransferOrder_3 = 0x00C0  #### Test value for file transfer order of 3 (lowest)
BBFileBundleId_1      = 0x0100  #### Test value for file bundle id 1
BBFileBundleId_2      = 0x0200  #### Test value for file bundle id 2
BBFileBundleId_3      = 0x0300  #### Test value for file bundle id 3
BBFileBundleId_4      = 0x0400  #### Test value for file bundle id 4
BBFileBundleId_5      = 0x0500  #### Test value for file bundle id 5
BBFileBundleId_6      = 0x0600  #### Test value for file bundle id 6
BBFileBundleId_7      = 0x0700  #### Test value for file bundle id 7
BBFileBundleId_8      = 0x0800  #### Test value for file bundle id 8
BBFileBundleId_9      = 0x0900  #### Test value for file bundle id 9

#
# BB_CancelTransfer BBCANCELSCOPE
#
BBCANCELSCOPE = {"BBSCOPETRANSFER" : 0x0001,  #### Cancel only impacts the local contribution
                 "BBSCOPETAG"      : 0x0002,  #### Cancel impacts all transfers associated by the tag
                 "INVALID"         : 0x270F,  #### Invalid value
                 0x0001 : "BBSCOPETRANSFER",  #### Cancel only impacts the local contribution
                 0x0002 : "BBSCOPETAG"     ,  #### Cancel impacts all transfers associated by the tag
                 0x270F : "INVALID"        ,  #### Invalid value
                }
DEFAULT_BBCANCELSCOPE = BBCANCELSCOPE["BBSCOPETAG"]
INVALID_BBCANCELSCOPE = BBCANCELSCOPE["INVALID"]

#
# BB_GetTransferList BBSTATUS
#
BBSTATUS = {"BBNONE"           : 0x0000000000000000,  #### No status (used by BB_GetTransferInfo)
            "BBNOTSTARTED"     : 0x0000000000000001,  #### The tag has been defined to the system
                                                      #### but none of the contributors have yet
                                                      #### reported.
            "BBINPROGRESS"     : 0x0000000000000002,  #### The tag has been defined to the system
                                                      #### and data for the associated transfer
                                                      #### definitions is actively being transferred.
            "BBPARTIALSUCCESS" : 0x0000000000000004,  #### Less than the full number of contributors
                                                      #### reported for the tag, but all of the
                                                      #### data for those reported transfer definitions
                                                      #### has been transferred successfully.  However,
                                                      #### some contributors never reported, and
                                                      #### therefore, not all data was transferred.
            "BBFULLSUCCESS"    : 0x0000000000000008,  #### All contributors have reported for the tag
                                                      #### and all data for all of the associated transfer
                                                      #### definitions has been transferred successfully.
            "BBCANCELED"       : 0x0000000000000010,  #### All data transfers associated with the tag
                                                      #### have been cancelled by the user.  No or
                                                      #### some data may have been transferred.
            "BBFAILED"         : 0x0000000000000020,  #### One or more data transfers have failed for
                                                      #### one or more of the associated transfer
                                                      #### definitions for the tag.  No or some data
                                                      #### may have been transferred successfully.
            "BBSTOPPED"        : 0x0000000000000040,  #### One or more data transfers have been stopped for
                                                      #### one or more of the associated transfer
                                                      #### definitions for the tag.  No or some data
                                                      #### may have been transferred successfully.
            "BBNOTACONTRIB"    : 0x0000000000000100,  #### The contributor making the API request is
                                                      #### NOT in the array of contributors for this tag.
                                                      #### Therefore, localStatus and localTransferSize
                                                      #### cannot be returned.
            "BBNOTREPORTED"    : 0x0000000000000200,  #### The contributor making the API request is
                                                      #### in the array of contributors for this tag.
                                                      #### However, it has not yet reported in with it's
                                                      #### transfer definition.
            "BBALL"            : 0xFFFFFFFFFFFFFFFF,  #### All status values (used by BB_GetTransferList)
            0x0000000000000000 : "BBNONE"          ,  #### No status (used by BB_GetTransferInfo)
            0x0000000000000001 : "BBNOTSTARTED"    ,  #### The tag has been defined to the system
                                                      #### but none of the contributors have yet
                                                      #### reported.
            0x0000000000000002 : "BBINPROGRESS"    ,  #### The tag has been defined to the system
                                                      #### and data for the associated transfer
                                                      #### definitions is actively being transferred.
            0x0000000000000004 : "BBPARTIALSUCCESS",  #### Less than the full number of contributors
                                                      #### reported for the tag, but all of the
                                                      #### data for those reported transfer definitions
                                                      #### has been transferred successfully.  However,
                                                      #### some contributors never reported, and
                                                      #### therefore, not all data was transferred.
            0x0000000000000008 : "BBFULLSUCCESS"   ,  #### All contributors have reported for the tag
                                                      #### and all data for all of the associated transfer
                                                      #### definitions has been transferred successfully.
            0x0000000000000010 : "BBCANCELED"      ,  #### All data transfers associated with the tag
                                                      #### have been cancelled by the user.  No or
                                                      #### some data may have been transferred.
            0x0000000000000020 : "BBFAILED"        ,  #### One or more data transfers have failed for
                                                      #### one or more of the associated transfer
                                                      #### definitions for the tag.  No or some data
                                                      #### may have been transferred successfully.
            0x0000000000000040 : "BBSTOPPED"       ,  #### One or more data transfers have been stopped for
                                                      #### one or more of the associated transfer
                                                      #### definitions for the tag.  No or some data
                                                      #### may have been transferred successfully.
            0x0000000000000100 : "BBNOTACONTRIB"   ,  #### The contributor making the API request is
                                                      #### NOT in the array of contributors for this tag.
                                                      #### Therefore, localStatus and localTransferSize
                                                      #### cannot be returned.
            0x0000000000000200 : "BBNOTREPORTED"   ,  #### The contributor making the API request is
                                                      #### in the array of contributors for this tag.
                                                      #### However, it has not yet reported in with it's
                                                      #### transfer definition.
            0xFFFFFFFFFFFFFFFF : "BBALL"           ,  #### All status values (used by BB_GetTransferList)
           }


#
# Flags/constants for bbapiAdmin interfaces
#

#
# BB_CreateLogicalVolume BBCREATEFLAGS
#
BBCREATEFLAGS = {"BBXFS"       : 0x0001,  #### Create xfs logical volume
                 "BBEXT4"      : 0x0002,  #### Create ext4 logical volume
                 "BBFSCUSTOM1" : 0x0100,  #### Site custom logical value 1
                 "BBFSCUSTOM2" : 0x0200,  #### Site custom logical value 2
                 "BBFSCUSTOM3" : 0x0400,  #### Site custom logical value 3
                 "BBFSCUSTOM4" : 0x0800,  #### Site custom logical value 4
                 "INVALID"     : 0x270F,  #### Invalid value
                 0x0001 : "BBXFS"      ,  #### Create xfs logical volume
                 0x0002 : "BBEXT4"     ,  #### Create ext4 logical volume
                 0x0100 : "BBFSCUSTOM1",  #### Site custom logical value 1
                 0x0200 : "BBFSCUSTOM2",  #### Site custom logical value 2
                 0x0400 : "BBFSCUSTOM3",  #### Site custom logical value 3
                 0x0800 : "BBFSCUSTOM4",  #### Site custom logical value 4
                 0x270F : "INVALID"    ,  #### Invalid value
                }
DEFAULT_BBCREATEFLAGS = BBCREATEFLAGS["BBXFS"]
INVALID_BBCREATEFLAGS = BBCREATEFLAGS["INVALID"]

#
# BB_ResizeMountPoint BBRESIZEFLAGS
#
BBRESIZEFLAGS = {"BB_NONE"               : 0x0000,  #### No resize flag option
                 "BB_DO_NOT_PRESERVE_FS" : 0x0001,  #### Do not preserve file system
                 "INVALID"               : 0x270F,  #### Invalid value
                 0x0000 : "BB_NONE"              ,  #### No resize flag option
                 0x0001 : "BB_DO_NOT_PRESERVE_FS",  #### Do not preserve file system
                 0x270F : "INVALID"              ,  #### Invalid value
                }
DEFAULT_BBRESIZEFLAGS = BBRESIZEFLAGS["BB_NONE"]
INVALID_BBRESIZEFLAGS = BBRESIZEFLAGS["INVALID"]

#
# BB_RetrieveTransfers
#
BB_RTV_TRANSFERDEFS_FLAGS = {"ALL_DEFINITIONS"                         : 0x0001,  #### All definitions
                             "ONLY_DEFINITIONS_WITH_UNFINISHED_FILES"  : 0x0002,  #### Only definitions with unfinished files
                             "ONLY_DEFINITIONS_WITH_STOPPED_FILES"     : 0x0003,  #### Only definitions with stopped files
                             "INVALID"                                 : 0x270F,  #### Invalid value
                             0x0001 : "ALL_DEFINITIONS",                          #### All definitions
                             0x0002 : "ONLY_DEFINITIONS_WITH_UNFINISHED_FILES",   #### Only rebuilt definitions with unfinished files
                             0x0003 : "ONLY_DEFINITIONS_WITH_STOPPED_FILES",      #### Only rebuilt definitions with stopped files
                             0x270F : "INVALID"                           ,       #### Invalid value
                            }
DEFAULT_BB_RTV_TRANSFERDEFS_FLAGS = BB_RTV_TRANSFERDEFS_FLAGS["ONLY_DEFINITIONS_WITH_UNFINISHED_FILES"]
INVALID_BB_RTV_TRANSFERDEFS_FLAGS = BB_RTV_TRANSFERDEFS_FLAGS["INVALID"]


#
# Structs
#
class BBUsage_t(Structure):
    _fields_ = [("totalBytesRead", c_uint64),       #### Number of bytes written to the logical volume
                ("totalBytesWritten", c_uint64),    #### Number of bytes read from the logical volume
                ("localBytesRead", c_uint64),       #### Number of bytes written to the logical volume via compute node
                ("localBytesWritten", c_uint64),    #### Number of bytes read from the logical volume via compute node
                ("burstBytesRead", c_uint64),       #### Number of bytes written to the logical volume via burst buffer transfers
                ("burstBytesWritten", c_uint64),    #### Number of bytes read from the logical volume via burst buffer transfers
                ("localReadCount", c_uint64),       #### Number of read operations to the logical volume
                ("localWriteCount", c_uint64),]     #### Number of write operations to the logical volume

class BBDeviceUsage_t(Structure):
    _fields_ = [("critical_warning", c_uint64),     #### Number of bytes written to the logical volume
                ("temperature", c_double),          #### Temperature of the SSD in degrees C
                ("available_spare", c_double),      #### Amount of SSD capacity over-provisioning that remains
                ("percentage_used", c_double),      #### Estimate of the amount of SSD life consumed.
                ("data_read", c_uint64),            #### Number of bytes read from the SSD over the life of the device.
                ("data_written", c_uint64),         #### Number of bytes written to the SSD over the life of the device.
                ("num_read_commands", c_uint64),    #### Number of I/O read commands received from the compute node.
                ("num_write_commands", c_uint64),   #### Number of completed I/O write commands from the compute node.
                ("busy_time", c_uint64),            #### Amount of time that the I/O controller was handling I/O requests.
                ("power_cycles", c_uint64),         #### Number of power on events for the SSD.
                ("power_on_hours", c_uint64),       #### Number of hours that the SSD has power.
                ("unsafe_shutdowns", c_uint64),     #### Number of unexpected power OFF events.
                ("media_errors", c_uint64),         #### Count of all data unrecoverable events.
                ("num_err_log_entries", c_uint64),] #### Number of error log entries available.

class BBTransferInfo_t(Structure):
    _fields_ = [("handle", c_uint64),                   #### Transfer handle
                ("contribid", c_uint32),                #### Contributor Id
                ("jobid", c_uint64),                    #### Job Id
                ("jobstepid", c_uint64),                #### Jobstep Id
                ("tag", c_ulong),                       #### User specified tag
                ("numcontrib", c_uint64),               #### Number of contributors
                ("status", c_int64),                    #### Current status of the transfer
                ("numreportingcontribs", c_uint64),     #### Number of reporting contributors
                ("totalTransferKeyLength", c_uint64),   #### Total number of bytes required to retrieve all
                                                        ####   key:value pairs using BB_GetTransferKeys()
                ("totalTransferSize", c_uint64),        #### Total number of bytes for the files associated with
                                                        ####   all transfer definitions
                ("localstatus", c_int64),               #### Current status of the transfer for the requesting contributor
                ("localTransferSize", c_uint64),]       #### Total number of bytes for the files associated with
                                                        ####   the transfer definition for the requesting contributor


#
# Miscellaneous
#


#
# Wrappers for official bbapi calls
#

def BB_AddFiles(pTransferDef, pSource, pTarget, pFlags=DEFAULT_BBFILEFLAG):
    l_Source = bb.cvar("source", pSource)
    l_Target = bb.cvar("target", pTarget)
    l_Flags = bb.cvar("flags", pFlags)

    print("%sBB_AddFiles issued to add source file %s and target file %s with flag %s to transfer definition %s" % (os.linesep, pSource, pTarget, BBFILEFLAGS[l_Flags.value], repr(pTransferDef)))
    rc = bb.api.BB_AddFiles(pTransferDef, l_Source, l_Target, l_Flags)
    if (rc):
        raise BB_AddFilesError(rc)

    bb.printLastErrorDetailsSummary()
    print("Source file %s and target file %s with flag %s added to transfer definition %s" % (pSource, pTarget, BBFILEFLAGS[l_Flags.value], repr(pTransferDef)))

    return

def BB_AddKeys(pTransferDef, pKey, pValue):
    l_Key = bb.cvar("key", pKey)
    l_Value = bb.cvar("value", pValue)

    print("%sBB_AddKeys issued to add key %s, value %s to transfer definition %s" % (os.linesep, pKey, pValue, repr(pTransferDef)))
    rc = bb.api.BB_AddKeys(pTransferDef, l_Key, l_Value)
    if (rc):
        raise BB_AddKeysError(rc)

    bb.printLastErrorDetailsSummary()
    print("Key '%s' with keyvalue '%s' added to transfer definition %s" % (pKey, pValue, repr(pTransferDef)))

    return

def BB_CancelTransfer(pHandle, pCancelScope=DEFAULT_BBCANCELSCOPE):
    l_NormalRCs = BB_CancelTransferError(BBError(Exception())).getNormalRCs()
    l_ToleratedErrorRCs = BB_CancelTransferError(BBError(Exception())).getToleratedErrorRCs()

    l_Handle = bb.cvar("handle", pHandle)
    l_CancelScope = bb.cvar("cancelscope", pCancelScope)

    print("%sBB_CancelTransfer issued to initiate cancel for handle %s, with cancel scope of %s" % (os.linesep, pHandle, BBCANCELSCOPE[l_CancelScope.value]))
    rc = bb.api.BB_CancelTransfer(l_Handle, l_CancelScope)
    if ((rc not in l_NormalRCs) and (rc not in l_ToleratedErrorRCs)):
        dummy = BBError()
        FIND_INCORRECT_BBSERVER = re.compile(".*A cancel request for an individual transfer definition must be directed to the bbServer servicing that jobid and contribid")
        l_ErrorSummary = dummy.getLastErrorDetailsSummary()
        l_Success = FIND_INCORRECT_BBSERVER.search(l_ErrorSummary)
        if ((not l_Success) or (pCancelScope == DEFAULT_BBCANCELSCOPE)):
            raise BB_CancelTransferError(rc)
        else:
            # NOTE: This could be a 'normal' case where the cancel operation is running simultaneously with a failover operation
            #       to a new bbServer.  Retry the cancel operation...
            print("Cancel operation with a cancel scope of BBSCOPETRANSFER was issued to the incorrect bbServer due to concurrent failover processing.  Exception was tolerated and cancel operation will not be retried.")
            rc = -2

    bb.printLastErrorDetailsSummary()
    if (rc == 0):
        print("Cancel initiated for handle %s, with cancel scope of %s" % (pHandle, l_CancelScope.value))

    return

def BB_CreateTransferDef():
    l_TransferDef = POINTER(c_uint32)()

    print("%sBB_CreateTransferDef issued" % (os.linesep))
    rc = bb.api.BB_CreateTransferDef(byref(l_TransferDef))
    if (rc):
        raise BB_CreateTransferDefError(rc)

    bb.printLastErrorDetailsSummary()
    print("Transfer definition %s created" % (repr(l_TransferDef)))

    return l_TransferDef

def BB_FreeTransferDef(pTransferDef):
    print("%sBB_FreeTransferDef issued for %s" % (os.linesep, repr(pTransferDef)))
    rc = bb.api.BB_FreeTransferDef(pTransferDef)
    if (rc):
        raise BB_FreeTransferDefError(rc)

    bb.printLastErrorDetailsSummary()
    print("Transfer definition %s freed" % (repr(pTransferDef)))

    return

def BB_GetDeviceUsage(pDeviceNum):
    l_DeviceNum = bb.cvar("devicenum", pDeviceNum)
    l_DeviceUsage = create_string_buffer(sizeof(BBDeviceUsage_t))

    print("%sBB_GetDeviceUsage issued for device number %d" % (os.linesep, pDeviceNum))
    rc = bb.api.BB_GetDeviceUsage(l_DeviceNum, byref(l_DeviceUsage))
    if (rc):
        raise BB_GetDeviceUsageError(rc)
    l_ReturnStruct = BBDeviceUsage_t.from_buffer_copy(l_DeviceUsage)

    bb.printLastErrorDetailsSummary()
    print("Estimated percentage of life used for device number %d is %d%%" % (pDeviceNum, getattr(l_ReturnStruct, "percentage_used")))

    return l_ReturnStruct

def BB_GetThrottleRate(pMountpoint):
    l_Mountpoint = bb.cvar("mountpoint", pMountpoint)
    l_Rate = bb.cvar("rate", 0)

    l_NormalRCs = BB_GetThrottleRateError(BBError(Exception())).getNormalRCs()
    l_ToleratedErrorRCs = BB_GetThrottleRateError(BBError(Exception())).getToleratedErrorRCs()

    print("%sBB_GetThrottleRate issued for mountpoint %s" % (os.linesep, pMountpoint))
    while (True):
        rc = bb.api.BB_GetThrottleRate(l_Mountpoint, byref(l_Rate))
        if (rc in (l_NormalRCs + l_ToleratedErrorRCs)):
            if (rc in l_ToleratedErrorRCs):
                print("Retrieving the throttle rate for mountpoint %s cannot be completed at this time.  Operation will be retried again in 10 seconds." % (pMountpoint))
                time.sleep(10)
            else:
                break
        else:
            raise BB_GetThrottleRateError(rc)

    bb.printLastErrorDetailsSummary()
    print("Throttle rate for mountpoint %s is %d bytes/sec" % (pMountpoint, l_Rate.value))

    return l_Rate.value

def BB_GetTransferHandle(pTag, pNumContrib, pContrib):
    l_Tag = bb.cvar("tag", pTag)
    l_NumContrib = bb.cvar("numcontrib", pNumContrib)
    Contrib = c_uint32 * pNumContrib
    l_Contrib = Contrib(*pContrib)
    l_Handle = bb.cvar("handle", 0)

    print("%sBB_GetTransferHandle issued for job (%d,%d), tag %d, numcontrib %d, contrib %s" % (os.linesep, bb.getJobId(), bb.getJobStepId(), pTag, pNumContrib, repr(pContrib)))
    rc = bb.api.BB_GetTransferHandle(l_Tag, l_NumContrib, l_Contrib, byref(l_Handle))
    if (rc):
        raise BB_GetTransferHandleError(rc)

    bb.printLastErrorDetailsSummary()
    print("Transfer handle %s obtained, for job (%d,%d), tag %d, numcontrib %d, contrib %s" % (l_Handle.value, bb.getJobId(), bb.getJobStepId(), pTag, pNumContrib, repr(pContrib)))

    return l_Handle.value

def BB_GetTransferInfo(pHandle):
    l_Handle = bb.cvar("handle", pHandle)
    l_Info = create_string_buffer(sizeof(BBTransferInfo_t))

    print("%sBB_GetTransferInfo issued for handle %s" % (os.linesep, pHandle))
    rc = bb.api.BB_GetTransferInfo(l_Handle, byref(l_Info))
    if (rc):
        raise BB_GetTransferInfoError(rc)
    l_ReturnStruct = BBTransferInfo_t.from_buffer_copy(l_Info)

    bb.printLastErrorDetailsSummary()
    print("BB_GetTransferInfo completed%s  Handle: %12d -> Status (Local:Overall) (%13s:%13s)   Transfer Size in bytes (Local:Total) (%s : %s)" % (os.linesep, l_ReturnStruct.handle, BBSTATUS[l_ReturnStruct.localstatus], BBSTATUS[l_ReturnStruct.status], '{:,}'.format(l_ReturnStruct.localTransferSize), '{:,}'.format(l_ReturnStruct.totalTransferSize)))

    return l_ReturnStruct

def BB_GetTransferKeys(pHandle, pSize):
    l_Handle = bb.cvar("handle", pHandle)
    l_Size = bb.cvar("size", pSize)
    l_Buffer = bb.cvar("buffer", pSize)

    print("%sBB_GetTransferKeys issued for handle %s, buffer size %d" % (os.linesep, pHandle, pSize))
    rc = bb.api.BB_GetTransferKeys(l_Handle, l_Size, byref(l_Buffer))
    if (rc):
        raise BB_GetTransferKeysError(rc)

    bb.printLastErrorDetailsSummary()
#    print json.dumps(l_Buffer, sort_keys=True, indent=4, separators=(',', ': '))

    l_Temp = []
    for i in range(l_Size.value):
        if l_Buffer[i] != b'\0':
            l_Temp.append(l_Buffer[i])
        else:
            break
    l_Output = (b"".join(l_Temp)).decode()
    print("Transfer keys: %s" % (l_Output))

    return l_Output

def BB_GetTransferList(pMatchStatus, pNumHandles):
    l_MatchStatus = bb.cvar("status", pMatchStatus)
    l_NumHandles = bb.cvar("numhandles", pNumHandles)
#    l_Handles = create_string_buffer(sizeof(c_uint64)*pNumHandles)
    l_Handles = create_string_buffer(int(((max((sizeof(c_uint64)*pNumHandles),1))+63)/64)*64)
    l_NumAvailHandles = bb.cvar("numavailhandles", 0)
    l_MatchStatusStr = BBSTATUS.get(pMatchStatus, repr(pMatchStatus))

    print("%sBB_GetTransferList issued with match status of %s" % (os.linesep, l_MatchStatusStr))
    rc = bb.api.BB_GetTransferList(l_MatchStatus, byref(l_NumHandles), byref(l_Handles), byref(l_NumAvailHandles))
    if (rc):
        raise BB_GetTransferListError(rc)

#   NOTE: We are failing atttempting to get the handle values from the string buffer.
#         For now, we get the handle values from BBError...
    '''
    l_HandleCount = min(pNumHandles, l_NumAvailHandles.value)
    l_HandleArray = (c_ulonglong*l_HandleCount).from_address(addressof(l_Handles))
    l_Output = [l_HandleArray[i] for i in range(l_HandleCount)]
    '''
#    l_Output = list(map(int, l_HandleArray))
#    print(l_Output)
#    for i in range(l_HandleCount):
#        l_Output.append(l_HandleArray[i])

    bb.printLastErrorDetailsSummary()

    l_Output = []
    if pNumHandles > 0:
        l_BBError = BBError()
        l_BBErrorData = l_BBError.getLastErrorDetails()
        if "out" in l_BBErrorData.keys():
            for l_Key, l_Value in l_BBErrorData["out"].items():
                if l_Key[0:7] == "handle_":
                    if l_Value.isdigit():
                        l_Output.append(int(l_Value))
                    else:
                        raise BB_GetTransferListError(rc=-1, text="Handle value %s from BBError is invalid" % (l_Value))
        else:
            raise BB_GetTransferListError(rc=-1, text="Key 'out' not found in BBError")

        if l_NumAvailHandles.value != len(l_Output):
            raise BB_GetTransferListError(rc=-1, text="Wrong number of handle values returned in BBError")

    print("BB_GetTransferList completed, %d handle(s) available, %s" % (l_NumAvailHandles.value, l_Output))

    return (l_NumAvailHandles.value, l_Output)

def BB_GetUsage(pMountpoint):
    l_Mountpoint = bb.cvar("mountpoint", pMountpoint)
    l_Usage = create_string_buffer(sizeof(BBUsage_t))

    print("%sBB_GetUsage issued for mountpoint %s" % (os.linesep, pMountpoint))
    rc = bb.api.BB_GetUsage(l_Mountpoint, byref(l_Usage))
    if (rc):
        raise BB_GetUsageError(rc)

    bb.printLastErrorDetailsSummary()
    l_ReturnStruct = BBUsage_t.from_buffer_copy(l_Usage)
    print("For mount point %s, the following are the current usage values:" % (pMountpoint))
    print("    Total bytes read = %d, Total bytes written = %d" % (getattr(l_ReturnStruct, "totalBytesRead"), getattr(l_ReturnStruct, "totalBytesWritten")))
    print("    Local bytes read = %d, Local bytes written = %d" % (getattr(l_ReturnStruct, "localBytesRead"), getattr(l_ReturnStruct, "localBytesWritten")))
    print("    Burst bytes read = %d, Burst bytes written = %d" % (getattr(l_ReturnStruct, "burstBytesRead"), getattr(l_ReturnStruct, "burstBytesWritten")))

    return l_ReturnStruct

def BB_SetThrottleRate(pMountpoint, pRate):
    l_Mountpoint = bb.cvar("mountpoint", pMountpoint)
    l_Rate = bb.cvar("rate", pRate)

    print("%sBB_SetThrottleRate issued to set the throttle rate for mountpoint %s to %d bytes/sec" % (os.linesep, pMountpoint, pRate))
    rc = bb.api.BB_SetThrottleRate(l_Mountpoint, l_Rate)
    if (rc):
        raise BB_SetThrottleRateError(rc)

    bb.printLastErrorDetailsSummary()
    print("Throttle rate for mountpoint %s changed to %d bytes/sec" % (pMountpoint, pRate))

    return

def BB_SetUsageLimit(pMountpoint, pUsage):
    l_Mountpoint = bb.cvar("mountpoint", pMountpoint)

    print("%sBB_SetUsageLimit issued to set the usage limit for mountpoint %s to %d" % (os.linesep, pMountpoint, pUsage))
    rc = bb.api.BB_SetUsage(l_Mountpoint, byref(pUsage))
    if (rc):
        raise BB_SetUsageError(rc)

    bb.printLastErrorDetailsSummary()
    print("For mount point %s, the following usage limits were set:" % (pMountpoint))
    print("    Total bytes read = %d, Total bytes written = %d" % (getattr(pUsage, "totalBytesRead"), getattr(pUsage, "totalBytesWritten")))
    print("    Local bytes read = %d, Local bytes written = %d" % (getattr(pUsage, "localBytesRead"), getattr(pUsage, "localBytesWritten")))
    print("    Burst bytes read = %d, Burst bytes written = %d" % (getattr(pUsage, "burstBytesRead"), getattr(pUsage, "burstBytesWritten")))

    return

def BB_StartTransfer(pTransferDef, pHandle):
    l_NormalRCs = BB_StartTransferError(BBError(Exception())).getNormalRCs()
    l_ToleratedErrorRCs = BB_StartTransferError(BBError(Exception())).getToleratedErrorRCs()

    l_Handle = bb.cvar("handle", pHandle)

    print("%sBB_StartTransfer issued to start the transfer %s for handle %s" % (os.linesep, repr(pTransferDef), pHandle))

    while (True):
        rc = bb.api.BB_StartTransfer(pTransferDef, l_Handle)
        if (rc in (l_NormalRCs + l_ToleratedErrorRCs)):
            if (rc in l_ToleratedErrorRCs):
                dummy = BBError()
                if ("Attempt to retry" in dummy.getLastErrorDetailsSummary()):
                    print("Transfer %s cannot be started for handle %s because of a suspended condition.  This start transfer request will be attempted again in 15 seconds." % (repr(pTransferDef), pHandle))
                    time.sleep(15)
                else:
                    print("Transfer %s cannot be started for handle %s because of a suspended condition.  Restart logic will resubmit this start transfer operation." % (repr(pTransferDef), pHandle))
                    break
            else:
                break
        else:
            raise BB_StartTransferError(rc)

    bb.printLastErrorDetailsSummary()
    if (rc == 0):
        print("Transfer %s started for handle %s" % (repr(pTransferDef), pHandle))

    return
