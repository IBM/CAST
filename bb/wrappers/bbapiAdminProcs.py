#!/usr/bin/python
####################################################
#    bbapiAdminProcs.py
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

import subprocess
import time

from bbapi import *
import bberror


#
# Helper routines
#

def IssueCmd(pCmd):
    l_RC = -1

    l_Attempts = 1
    while (l_RC in (-1, )):
        l_RC = 0
        try:
            subprocess.check_call(pCmd, stderr=subprocess.STDOUT, shell=True)
        except subprocess.CalledProcessError as error:
            # NOTE: When invoking APIs via the subprocess module,
            #       we simply treat 0 and -2 as the only tolerated
            #       return codes.  We do not tie into the extra
            #       capabilities of the return code processing
            #       of BBError for each of the APIs.
            #       Maybe someday, but not needed now...
            # NOTE: If we receive a return code of -2, we still
            #       want to pass that back to our invoker.
            #       They may or may not tolerate the rc, but
            #       we will not signal the BBError exception.
            l_RC = -1
            print "Failure when attempting to invoke subprocess.check_call() with command of |", pCmd, "|"

            l_Attempts += 1
            if (l_Attempts > 25):
                l_RC = error.returncode
                print "IssueCmd(): Throwing BBError, l_RC=", l_RC
                raise bberror.BBError(rc=l_RC, text=pCmd)
            else:
                print "Attempting to re-submit the command, attempt number ", l_Attempts
            time.sleep(12)

    return l_RC


# NOTE: Convention below is to ALWAYS pass --process_args even if there are no arguments to pass.
#       In that case, pass "None".  The prevents the shared library information from being sent
#       to the console for the BB environments setup via the functions in this module...   @DLH

def sudo_ChangeMode(pEnv, pPathName, pMode):
    l_Cmd = "sudo python %s %s --libpath %s --testpath %s --iteration %d --testcase RunProcedure --procedure %s --procedure_args %s,%s" % \
            (pEnv["COMMAND"], pEnv["COMMAND_LINE_ARGS"], pEnv["LIBPATH"], pEnv["WRAPPER_PATH"], pEnv["iteration"], "ChangeMode", pPathName, pMode)
    return IssueCmd(l_Cmd)

def sudo_ChangeOwner(pEnv, pPathName, pOwner, pGroup):
    l_Cmd = "sudo python %s %s --libpath %s --testpath %s --iteration %d --testcase RunProcedure --procedure %s --procedure_args %s,%s,%s" % \
            (pEnv["COMMAND"], pEnv["COMMAND_LINE_ARGS"], pEnv["LIBPATH"], pEnv["WRAPPER_PATH"], pEnv["iteration"], "ChangeOwner", pPathName, pOwner, pGroup)
    return IssueCmd(l_Cmd)

# def sudo_CloseServer(pEnv, pName):
#     l_Cmd = "sudo python %s %s --libpath %s --testpath %s --iteration %d --testcase RunProcedure --procedure %s --procedure_args %s" % \
#             (pEnv["COMMAND"], pEnv["COMMAND_LINE_ARGS"], pEnv["LIBPATH"], pEnv["WRAPPER_PATH"], pEnv["iteration"], "CloseServer", pName)
#     return IssueCmd(l_Cmd)

def sudo_CreateDirectory(pEnv, pNewPathName):
    l_Cmd = "sudo python %s %s --libpath %s --testpath %s --iteration %d --testcase RunProcedure --procedure %s --procedure_args %s" % \
            (pEnv["COMMAND"], pEnv["COMMAND_LINE_ARGS"], pEnv["LIBPATH"], pEnv["WRAPPER_PATH"], pEnv["iteration"], "CreateDirectory", pNewPathName)
    return IssueCmd(l_Cmd)

def sudo_CreateLogicalVolume(pEnv, pMountpoint, pSize, pFlags=DEFAULT_BBCREATEFLAGS):
    l_Cmd = "sudo python %s %s --libpath %s --testpath %s --iteration %d --testcase RunProcedure --procedure %s --procedure_args %s,%s,%s" % \
            (pEnv["COMMAND"], pEnv["COMMAND_LINE_ARGS"], pEnv["LIBPATH"], pEnv["WRAPPER_PATH"], pEnv["iteration"], "CreateLogicalVolume", pMountpoint, pSize, pFlags)
    return IssueCmd(l_Cmd)

# def sudo_GetServer(pEnv, pType):
#     l_Cmd = "sudo python %s %s --libpath %s --testpath %s --iteration %d --testcase RunProcedure --procedure %s --procedure_args %s" % \
#             (pEnv["COMMAND"], pEnv["COMMAND_LINE_ARGS"], pEnv["LIBPATH"], pEnv["WRAPPER_PATH"], pEnv["iteration"], "GetServer", pType)
#     return IssueCmd(l_Cmd)

# def sudo_OpenServer(pEnv, pName):
#     l_Cmd = "sudo python %s %s --libpath %s --testpath %s --iteration %d --testcase RunProcedure --procedure %s --procedure_args %s" % \
#             (pEnv["COMMAND"], pEnv["COMMAND_LINE_ARGS"], pEnv["LIBPATH"], pEnv["WRAPPER_PATH"], pEnv["iteration"], "OpenServer", pName)
#     return IssueCmd(l_Cmd)

def sudo_RemoveDirectory(pEnv, pPathName):
    l_Cmd = "sudo python %s %s --libpath %s --testpath %s --iteration %d --testcase RunProcedure --procedure %s --procedure_args %s" % \
            (pEnv["COMMAND"], pEnv["COMMAND_LINE_ARGS"], pEnv["LIBPATH"], pEnv["WRAPPER_PATH"], pEnv["iteration"], "RemoveDirectory", pPathName)
    return IssueCmd(l_Cmd)

def sudo_RemoveJobInfo(pEnv):
    l_Cmd = "sudo python %s %s --libpath %s --testpath %s --iteration %d --testcase RunProcedure --procedure %s --procedure_args %s" % \
            (pEnv["COMMAND"], pEnv["COMMAND_LINE_ARGS"], pEnv["LIBPATH"], pEnv["WRAPPER_PATH"], pEnv["iteration"], "RemoveJobInfo", "None")
    return IssueCmd(l_Cmd)

def sudo_RemoveLogicalVolume(pEnv, pMountpoint):
    l_Cmd = "sudo python %s %s --libpath %s --testpath %s --iteration %d --testcase RunProcedure --procedure %s --procedure_args %s" % \
            (pEnv["COMMAND"], pEnv["COMMAND_LINE_ARGS"], pEnv["LIBPATH"], pEnv["WRAPPER_PATH"], pEnv["iteration"], "RemoveLogicalVolume", pMountpoint)
    return IssueCmd(l_Cmd)

def sudo_ResizeMountPoint(pEnv, pMountpoint, pSize, pFlags=DEFAULT_BBRESIZEFLAGS):
    l_Cmd = "sudo python %s %s --libpath %s --testpath %s --iteration %d --testcase RunProcedure --procedure %s --procedure_args %s,%s,%s" % \
            (pEnv["COMMAND"], pEnv["COMMAND_LINE_ARGS"], pEnv["LIBPATH"], pEnv["WRAPPER_PATH"], pEnv["iteration"], "ResizeMountPoint", pMountpoint, pSize, pFlags)
    return IssueCmd(l_Cmd)

def sudo_RestartTransfers(pEnv, pHostName, pHandle, pTransferDefs, pTransferDefsSize):
    l_Cmd = "sudo python %s %s --libpath %s --testpath %s --iteration %d --testcase RunProcedure --procedure %s --procedure_args %s,%s,%s,%s" % \
            (pEnv["COMMAND"], pEnv["COMMAND_LINE_ARGS"], pEnv["LIBPATH"], pEnv["WRAPPER_PATH"], pEnv["iteration"], "RestartTransfers", pEnv["procedure_args"], pHostName, pHandle, pTransferDefs, pTransferDefsSize)
    return IssueCmd(l_Cmd)

# def sudo_Resume(pEnv, pHostHame):
#     l_Cmd = "sudo python %s %s --libpath %s --testpath %s --iteration %d --testcase RunProcedure --procedure %s --procedure_args %s" % \
#             (pEnv["COMMAND"], pEnv["COMMAND_LINE_ARGS"], pEnv["LIBPATH"], pEnv["WRAPPER_PATH"], pEnv["iteration"], "Resume", pHostName)
#     return IssueCmd(l_Cmd)

# def sudo_RetrieveTransfers(pEnv, pHostHame, pHandle, pFlags=DEFAULT_sudo_RTV_TRANSFERDEFS_FLAGS):
#     l_Cmd = "sudo python %s %s --libpath %s --testpath %s --iteration %d --testcase RunProcedure --procedure %s --procedure_args %s,%s,%s" % \
#             (pEnv["COMMAND"], pEnv["COMMAND_LINE_ARGS"], pEnv["LIBPATH"], pEnv["WRAPPER_PATH"], pEnv["iteration"], "RetrieveTransfers", pHostHame, pHandle, pFlags)
#     return IssueCmd(l_Cmd)%s" % (pEnv["COMMAND"], pEnv["COMMAND_LINE_ARGS"], pEnv["LIBPATH"], pEnv["WRAPPER

# def sudo_SetServer(pEnv, pType, pName):
#     l_Cmd = "sudo python %s %s --libpath %s --testpath %s --iteration %d --testcase RunProcedure --procedure %s --procedure_args %s,%s" % \
#             (pEnv["COMMAND"], pEnv["COMMAND_LINE_ARGS"], pEnv["LIBPATH"], pEnv["WRAPPER_PATH"], pEnv["iteration"], "SetServer", pType, pName)
#     return IssueCmd(l_Cmd)

def sudo_StageOutStart(pEnv, pMountpoint):
    l_Cmd = "sudo python %s %s --libpath %s --testpath %s --iteration %d --testcase RunProcedure --procedure %s --procedure_args %s" % \
            (pEnv["COMMAND"], pEnv["COMMAND_LINE_ARGS"], pEnv["LIBPATH"], pEnv["WRAPPER_PATH"], pEnv["iteration"], "StageOutStart", pMountpoint)
    return IssueCmd(l_Cmd)

# def sudo_StopTransfers(pEnv, pHostName, pHandle, pTransferDefs, pTransferDefsSize):
#     l_Cmd = "sudo python %s %s --libpath %s --testpath %s --iteration %d --testcase RunProcedure --procedure %s --procedure_args %s,%s,%s,%s" % \
#             (pEnv["COMMAND"], pEnv["COMMAND_LINE_ARGS"], pEnv["LIBPATH"], pEnv["WRAPPER_PATH"], pEnv["iteration"], "StopServer", pHostName, pHandle, pTransferDefs, pTransferDefsSize)
#     return IssueCmd(l_Cmd)

# def sudo_Suspend(pEnv, pHostHame):
#     l_Cmd = "sudo python %s %s --libpath %s --testpath %s --iteration %d --testcase RunProcedure --procedure %s --procedure_args %s" % \
#             (pEnv["COMMAND"], pEnv["COMMAND_LINE_ARGS"], pEnv["LIBPATH"], pEnv["WRAPPER_PATH"], pEnv["iteration"], "Suspend", pHostHame)
#     return IssueCmd(l_Cmd)
