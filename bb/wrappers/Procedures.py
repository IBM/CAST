#!/usr/bin/python
####################################################
#    Procedures.py
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
    Common Burst Buffer Procedures
"""

import time

import bb
from bberror import *
from bbapi import *
from bbapiAdmin import *
from bbapiTest import *

#
# Interfaces directly invoked from the command line
# These can be run under the user's profile
#

def CancelTransfers(pEnv):
    rc = 0

    try:
        bb.initEnv(pEnv)

        l_TargetHandle = pEnv["handle"]
        l_CancelScope = pEnv["cancelscope"]
        l_Handles = bb.getHandles()
        for l_Handle in l_Handles:
            try:
                if (l_TargetHandle == bb.NO_HANDLE or l_TargetHandle == l_Handle):
                    BB_CancelTransfer(l_Handle, l_CancelScope)
            except BBError as error:
                error.handleError()
    except BBError as error:
        error.handleError()
        rc = error.rc

    return rc

def GetHandle(pEnv):
    rc = 0

    try:
        bb.initEnv(pEnv)

        l_Contrib = pEnv["contrib"]
        l_Handle = BB_GetTransferHandle(pEnv["tag"], len(l_Contrib), l_Contrib)
    except BBError as error:
        error.handleError()
        rc = error.rc

    return rc

def GetTransferInfo(pEnv):
    rc = 0

    try:
        bb.initEnv(pEnv)

        l_Handle = pEnv["handle"]
        l_Info = BB_GetTransferInfo(l_Handle)
        bb.printStruct(l_Info)
    except BBError as error:
        error.handleError()
        rc = error.rc

    return rc

def GetUsage(pEnv):
    rc = 0

    try:
        bb.initEnv(pEnv)

        l_Mount = pEnv["MOUNT"]
        BB_GetUsage(l_Mount)
    except BBError as error:
        error.handleError()
        rc = error.rc

    return rc

def GetWaitForReplyCount(pEnv):
    rc = 0

    try:
        bb.initEnv(pEnv)

        l_Attempts = 0
        l_Continue = 1
        l_PrevServer = None
        l_PrevValue = None
        l_PrintOption = False;
        while (l_Continue > 0):
            l_ActiveServer = BB_GetServer("active", False)
            if (l_ActiveServer != ""):
                if (l_Attempts % 120 == 0):
                    l_PrintOption = True
                l_Value = int(BB_GetServerByName(l_ActiveServer, "waitforreplycount", l_PrintOption))
                time.sleep(0.25)

            l_PrintOption = False
            if (l_PrevServer != l_ActiveServer or l_PrevValue != l_Value):
                l_PrintOption = True
                l_PrevServer = l_ActiveServer
                l_PrevValue = l_Value
                l_Attempts = 0
            l_Attempts += 1
    except BBError as error:
        error.handleError()
        rc = error.rc

    return rc

def ListHandles(pEnv):
    rc = 0

    try:
        bb.initEnv(pEnv)

        bb.getHandles()
    except BBError as error:
        error.handleError()
        rc = error.rc

    return rc

def ListTransfers(pEnv):
    rc = 0

    try:
        bb.initEnv(pEnv)

        l_Handles = bb.getHandles()
        for l_Handle in l_Handles:
            try:
                BB_GetTransferInfo(l_Handle)
            except BBError as error:
                error.handleError()

    except BBError as error:
        error.handleError()
        rc = error.rc

    return rc


#
# Interfaces directly invoked from the command line
# These can be must be run uder the root profile
#

def CloseServer(pEnv):
    rc = 0

    try:
        bb.initEnv(pEnv)
        l_Name = pEnv["name"]

        BB_CloseServer(l_Name)
    except BBError as error:
        error.handleError()
        rc = error.rc

    return rc

def CreateLogicalVolume_Direct(pEnv):
    rc = 0

    try:
        bb.initEnv(pEnv)

        BB_CreateLogicalVolume(pEnv["MOUNT"], pEnv["SIZE"])
    except BBError as error:
        error.handleError()
        rc = error.rc

    return rc

def GetServer(pEnv):
    rc = 0

    try:
        bb.initEnv(pEnv)
        l_Type = pEnv["type"]

        BB_GetServer(l_Type)
    except BBError as error:
        error.handleError()
        rc = error.rc

    return rc

def OpenServer(pEnv):
    rc = 0

    try:
        bb.initEnv(pEnv)
        l_Name = pEnv["name"]

        BB_OpenServer(l_Name)
    except BBError as error:
        error.handleError()
        rc = error.rc

    return rc

def RetrieveTransfers(pEnv):
    rc = 0

    try:
        bb.initEnv(pEnv)
        l_HostName = pEnv["hostname"]
        l_Handle = pEnv["handle"]
        l_Flags = DEFAULT_BB_RTV_TRANSFERDEFS_FLAGS
        if pEnv.has_key("FLAGS"):
            l_Flags = BB_RTV_TRANSFERDEFS_FLAGS.get(pEnv["FLAGS"], DEFAULT_BB_RTV_TRANSFERDEFS_FLAGS)

        (l_NumberOfTranferDefs, l_TransferDefs, l_BytesForTransferDefs) = BB_RetrieveTransfers(l_HostName, l_Handle, l_Flags)
    except BBError as error:
        error.handleError()
        rc = error.rc

    return rc

def RestartTransfers(pEnv):
    rc = 0
    l_ResumeCN_Host = False
    l_NumStoppedTransferDefs = 0

    try:
        bb.initEnv(pEnv)
        l_HostName = pEnv["hostname"]
        l_Handle = pEnv["handle"]
        l_Flags = DEFAULT_BB_RTV_TRANSFERDEFS_FLAGS
        if pEnv.has_key("FLAGS"):
            l_Flags = BB_RTV_TRANSFERDEFS_FLAGS.get(pEnv["FLAGS"], DEFAULT_BB_RTV_TRANSFERDEFS_FLAGS)

        l_ActiveServer = BB_GetServer("active")
        if (pEnv["IO_FAILOVER"]):
            # New ESS, set up that environment
            l_PrimaryServer = BB_GetServer("primary")
            l_BackupServer = BB_GetServer("backup")

            if (l_ActiveServer == "" or l_ActiveServer == l_PrimaryServer):
                l_NewServer = l_BackupServer
            else:
                l_NewServer = l_PrimaryServer

            if (l_NewServer.upper() in ("NONE",)):
                raise BBError(rc=-1, text="There is no definied backup for this bbServer")

            '''
            # Don't do this as it causes a window...  'activate' makes the swap atomically between the two servers...
            try:
                BB_SetServer("offline", l_ActiveServer)
            except BBError as error:
                if not error.handleError():
                    raise
            '''

            try:
                BB_OpenServer(l_NewServer)
            except BBError as error:
                if not error.handleError():
                    raise

            bb.flushWaiters(l_ActiveServer)
            BB_SetServer("activate", l_NewServer)

        # NOTE: Try to let start transfers to complete their second volley
        #       before suspending the connection(s)
        bb.flushWaiters(l_ActiveServer)
        BB_Suspend(l_HostName)
        l_ResumeCN_Host = True

        # NOTE: We do not want to close the connection to the previouly active server until we have
        #       stopped any transfers that may still be running on that server
        (l_NumberOfTranferDefs, l_TransferDefs, l_BytesForTransferDefs) = BB_RetrieveTransfers(l_HostName, l_Handle, l_Flags)
        if (l_NumberOfTranferDefs > 0):
            if (not (l_Flags == BB_RTV_TRANSFERDEFS_FLAGS["ONLY_DEFINITIONS_WITH_STOPPED_FILES"])):
                l_NumStoppedTransferDefs = BB_StopTransfers(l_HostName, l_Handle, l_TransferDefs, l_BytesForTransferDefs)
                print "%d transfer definition(s) were found already stopped or were stopped by the previous operation" % (l_NumStoppedTransferDefs)
            else:
                l_NumStoppedTransferDefs = l_NumberOfTranferDefs
                print "Additional transfer definition(s) are not being stopped because the option passed on the retrieve transfer operation indicated %s" % (BB_RTV_TRANSFERDEFS_FLAGS[l_Flags])
        else:
            print "No transfer definition(s) were found given the provided input criteria.  An operation to stop transfers will not be attempted."

        if (l_ResumeCN_Host):
            l_ResumeCN_Host = False
            try:
                # NOTE:  This resume will ensure that the new
                #        bbServer is 'resumed' from any prior
                #        suspend activity.  If the new bbServer
                #        is not suspended, it is a tolerated exception.
                #        This resume will also, via the async request file,
                #        resume the old bbServer for this CN hostname.
                #        However, any transfers on the old bbServer
                #        have been stopped, and thus, have been removed
                #        from any transfer queues.
                BB_Resume(l_HostName)
            except BBError as error:
                if not error.handleError():
                    raise

        # NOTE: Even if no transfer definitions were stopped, we still need to issue the restart.
        #       The stop transfer(s) could have actually been performed on other bbServers via
        #       the async request file...
        if (l_NumberOfTranferDefs > 0):
            # Restart the transfers
            l_NumRestartedTransferDefs = BB_RestartTransfers(l_HostName, l_Handle, l_TransferDefs, l_BytesForTransferDefs)
        else:
            print "%sNo transfer definition(s) were found given the provided input criteria.  An operation to restart transfers will not be attempted." % (os.linesep)

        if (pEnv["IO_FAILOVER"]):
            # Now, close the connection to the previously active bbServer
            try:
                # First, make sure we have no waiters...
 #               bb.flushWaiters(l_ActiveServer)

                # Close the connection to the 'old' server
                if (l_ActiveServer != ""):
                    BB_CloseServer(l_ActiveServer)
            except BBError as error:
                if not error.handleError():
                    raise

        elif (0==1 and pEnv["CN_FAILOVER"]):
            # New CN, set up that environment
            # Not ready for prime time...

            l_Mountpoint = pEnv["MOUNT"]
            l_Owner = pEnv.get("OWNER", None)
            l_Group = pEnv.get("GROUP", None)
            l_Mode = int(pEnv.get("MODE", 0755))
            l_LVSize = pEnv.get("SIZE", "1G")
            sudo_CreateDirectory(pEnv, l_Mountpoint)
            sudo_ChangeOwner(pEnv, l_Mountpoint, l_Owner, l_Group)
            sudo_ChangeMode(pEnv, l_Mountpoint, l_Mode)
            sudo_CreateLogicalVolume(pEnv, l_Mountpoint, l_LVSize)

        l_ActiveServer = BB_GetServer("active")

    except BBError as error:
        error.handleError()
        rc = error.rc

    # If we need to resume the active bbServer, do so now...
    if (l_ResumeCN_Host):
        l_ResumeCN_Host = False
        BB_Resume(l_HostName)

    return rc

def Resume(pEnv):
    rc = 0

    try:
        bb.initEnv(pEnv)
        l_HostName = pEnv["hostname"]

        BB_Resume(l_HostName)
    except BBError as error:
        error.handleError()
        rc = error.rc

    return rc

def SetServer(pEnv):
    rc = 0

    try:
        bb.initEnv(pEnv)
        l_Type = pEnv["type"]
        l_Name = pEnv["name"]

        BB_SetServer(l_Type, l_Name)
    except BBError as error:
        error.handleError()
        rc = error.rc

    return rc

def StopTransfers(pEnv):
    rc = 0

    try:
        bb.initEnv(pEnv)
        l_HostName = pEnv["hostname"]
        l_Handle = pEnv["handle"]
        l_Flags = DEFAULT_BB_RTV_TRANSFERDEFS_FLAGS
        if pEnv.has_key("FLAGS"):
            l_Flags = BB_RTV_TRANSFERDEFS_FLAGS.get(pEnv["FLAGS"], DEFAULT_BB_RTV_TRANSFERDEFS_FLAGS)

        (l_NumberOfTranferDefs, l_TransferDefs, l_BytesForTransferDefs) = BB_RetrieveTransfers(l_HostName, l_Handle, l_Flags)
        if (l_NumberOfTranferDefs > 0):
            l_NumStoppedTransferdDefs = BB_StopTransfers(l_HostName, l_Handle, l_TransferDefs, l_BytesForTransferDefs)
    except BBError as error:
        error.handleError()
        rc = error.rc

    return rc

def Suspend(pEnv):
    rc = 0

    try:
        bb.initEnv(pEnv)
        l_HostName = pEnv["hostname"]

        BB_Suspend(l_HostName)
    except BBError as error:
        error.handleError()
        rc = error.rc

    return rc


#
# Interfaces invoked via bbapiAdminProcs
# (i.e., --procedure_args contains the necessary arguments...)
#
# These implementations invoke another instance of the python
# interpreter with sudo...
#

def ChangeMode(pEnv):
    rc = 0

    try:
        bb.initEnv(pEnv)

        l_Args = pEnv["procedure_args"].split(",")
        BB_ChangeMode(l_Args[0], int(l_Args[1]))
    except BBError as error:
        error.handleError()
        rc = error.rc

    return rc

def ChangeOwner(pEnv):
    rc = 0

    try:
        bb.initEnv(pEnv)

        l_Args = pEnv["procedure_args"].split(",")
        BB_ChangeOwner(l_Args[0], l_Args[1], l_Args[2])
    except BBError as error:
        error.handleError()
        rc = error.rc

    return rc

def CreateDirectory(pEnv):
    rc = 0

    try:
        bb.initEnv(pEnv)

        l_Args = pEnv["procedure_args"].split(",")
        BB_CreateDirectory(l_Args[0])
    except BBError as error:
        error.handleError()
        rc = error.rc

    return rc

def CreateLogicalVolume(pEnv):
    rc = 0

    try:
        bb.initEnv(pEnv)

        l_Args = pEnv["procedure_args"].split(",")
        BB_CreateLogicalVolume(l_Args[0], l_Args[1], int(l_Args[2]))
    except BBError as error:
        error.handleError()
        rc = error.rc

    return rc

def RemoveDirectory(pEnv):
    rc = 0

    try:
        bb.initEnv(pEnv)

        l_Args = pEnv["procedure_args"].split(",")
        BB_RemoveDirectory(l_Args[0])
    except BBError as error:
        error.handleError()
        rc = error.rc

    return rc

def ResizeMountPoint(pEnv):
    rc = 0

    try:
        bb.initEnv(pEnv)

        l_Args = pEnv["procedure_args"].split(",")
        BB_ResizeMountPoint(l_Args[0], l_Args[1], int(l_Args[2]))
    except BBError as error:
        error.handleError()
        rc = error.rc

    return rc

# NOTE:  Currently, a test command...
def StageOutStart(pEnv):
    rc = 0

    try:
        bb.initEnv(pEnv)

        l_Args = pEnv["procedure_args"].split(",")
        Coral_StageOutStart(l_Args[0])
    except BBError as error:
        error.handleError()
        rc = error.rc

    return rc


#
# Interfaces invoked via either the command line or bbapiAdminProcs.
# (When via the command line, must be invoked using root.  Otherwise,
#  sudo is used for the python interpreter running the procedure.)
#
def RemoveJobInfo(pEnv):
    rc = 0

    try:
        bb.initEnv(pEnv)

        BB_RemoveJobInfo()
    except BBError as error:
        error.handleError()
        rc = error.rc

    return rc

def RemoveLogicalVolume(pEnv):
    rc = 0

    try:
        bb.initEnv(pEnv)
        if (pEnv["procedure_args"] == ""):
            l_Mountpoint = pEnv["MOUNT"]
        else:
            l_Mountpoint = pEnv["procedure_args"]

        BB_RemoveLogicalVolume(l_Mountpoint)
    except BBError as error:
        error.handleError()
        rc = error.rc

    return rc
