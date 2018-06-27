#!/usr/bin/python
####################################################
#    bb.py
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
    Common include to bbapi functions.
"""

import ctypes
import collections
import ctypes
import itertools
import os
import random
import pprint
import sys
import time
import threading

from copy import deepcopy

from bbapi import *
from bbapiAdminProcs import *
from bbapiTest import *
from datetime import datetime

# NOTE:  All of the following "NO_" values
#        correspond to the "UNDEFINED_" variables
#        in bbinternal.h
NO_HOSTNAME = "%%_NuLl_HoStNaMe_%%"
NO_JOBID = 0
NO_JOBSTEPID = 0
NO_HANDLE = 0
# NOTE:  The following is a value that Coral_G/SetVar can handle...
NO_CONTRIBID = 999999999


DO_NOT_PRINT_VALUE = False
DEFAULT_REMOVE_JOB_INFO_DELAY = 30
# DEFAULT_GET_TRANSFERINFO_DELAY = 5
DEFAULT_GET_TRANSFERINFO_DELAY = 1
# DEFAULT_WAIT_FOR_COMPLETION_ATTEMPTS = 360  # 30 minutes
DEFAULT_WAIT_FOR_COMPLETION_ATTEMPTS = 1440  # 2 hours

Vars = {"buffer":ctypes.create_string_buffer,
        "cancelscope":ctypes.c_int32,
        "contribid":ctypes.c_uint32,
        "devicenum":ctypes.c_uint32,
        "flags":ctypes.c_int64,
        "errformat":ctypes.c_int32,
        "group":ctypes.create_string_buffer,
        "handle":ctypes.c_uint64,
        "hostname":ctypes.create_string_buffer,
        "jobid":ctypes.c_uint64,
        "key":ctypes.create_string_buffer,
        "mode":ctypes.c_uint32,
        "mountpoint":ctypes.create_string_buffer,
        "name":ctypes.create_string_buffer,
        "newpathname":ctypes.create_string_buffer,
        "numavailhandles":ctypes.c_uint64,
        "numbytesavailable":ctypes.c_uint64,
        "numcontrib":ctypes.c_uint32,
        "numhandles":ctypes.c_uint64,
        "numtransferdefs":ctypes.c_uint32,
        "option":ctypes.create_string_buffer,
        "owner":ctypes.create_string_buffer,
        "pathname":ctypes.create_string_buffer,
        "rate":ctypes.c_uint64,
        "size":ctypes.c_uint64,
        "size_str":ctypes.create_string_buffer,
        "source":ctypes.create_string_buffer,
        "status":ctypes.c_int64,
        "tag":ctypes.c_ulong,
        "target":ctypes.create_string_buffer,
        "transferdefs":ctypes.create_string_buffer,
        "type":ctypes.create_string_buffer,
        "value":ctypes.create_string_buffer,
        "variable":ctypes.create_string_buffer,
       }

api = None


#
# Helper routines
#

# Override the format() method for pprint.PrettyPrinter
# so that any unicode data is converted to utf8
class bbpprint(pprint.PrettyPrinter):
    def format(self, pObject, pContext, pMaxLevels, pLevel):
        if isinstance(pObject, unicode):
            return (pObject.encode('utf8'), True, False)
        else:
            return pprint.PrettyPrinter.format(self, pObject, pContext, pMaxLevels, pLevel)

def cvar(pVar, pValue):
    try:
        if (type(pValue) == dict):
            return Vars[pVar](pValue[pVar])
        else:
            return Vars[pVar](pValue)
    except:
        print "Unexpected error when attempting to build a cvar: Type(pVar)=", type(pVar), ", type(pValue)=", type(pValue), ", pVar=", pVar, ", pValue=", pValue, "error=", sys.exc_info()[0]
        return None

def checkMD5SumFile(pMD5SumFile):
    l_RC = 0

    l_SourceCheckSum = ""
    l_Result = []
    for l_Line in open(pMD5SumFile, "r"):
        l_Result = l_Line.strip().split()
        if len(l_SourceCheckSum) == 0:
            l_SourceCheckSum, l_SourceFile = l_Result
        else:
            if l_SourceCheckSum == l_Result[0]:
                print "** SUCCESS ** File %s with checksum of %s matches the checksum for source file %s" % (l_Result[1], l_Result[0], l_SourceFile)
            else:
                l_RC = -1
                print "** ERROR ** File %s with checksum of %s mismatches source file %s with checksum of %s" % (l_Result[1], l_Result[0], l_SourceFile, l_SourceCheckSum)

    return l_RC

def checkFiles(pSourceFiles, pTargetFiles):
    l_RC = 0

    l_MD5SumFile = "/tmp/md5sum%s" % (random.choice(xrange(1000000)))
    if type(pSourceFiles) in (tuple, list,):
        for i in xrange(len(pSourceFiles)):
            if type(pSourceFiles[i]) in (tuple, list,):
                for j in xrange(len(pSourceFiles[i])):
                    if type(pSourceFiles[i][j]) not in (list, tuple):
                        runCmd("md5sum %s %s > %s" % (pSourceFiles[i][j], pTargetFiles[i][j], l_MD5SumFile))
                        l_RC = checkMD5SumFile(l_MD5SumFile)
                    else:
                        for k in xrange(len(pSourceFiles[i][j])):
                            runCmd("md5sum %s %s > %s" % (pSourceFiles[i][j][k], pTargetFiles[i][j][k], l_MD5SumFile))
                            l_RC = checkMD5SumFile(l_MD5SumFile)
            else:
                runCmd("md5sum %s %s > %s" % (pSourceFiles[i], pTargetFiles[i], l_MD5SumFile))
                l_RC = checkMD5SumFile(l_MD5SumFile)
    else:
        runCmd("md5sum %s %s > %s" % (pSourceFiles, pTargetFiles, l_MD5SumFile))
        l_RC = checkMD5SumFile(l_MD5SumFile)

    runCmd("rm %s" % (l_MD5SumFile))

    if l_RC:
        raise bberror.BBError(rc=l_RC, text="MD5Sum check failure")

    return

def createRandomFile(pEnv, pFile, pSize):
    l_RandFilePgm = os.path.abspath(os.path.join(os.path.dirname(pEnv["LIBPATH"]), ".", "tools/randfile"))
    l_RemoveFilePgm = "rm"
    l_CreateFilePgm = "touch"
    if pSize > 0:
        runCmd("%s --file %s --size %d" % (l_RandFilePgm, pFile, pSize))
    else:
        runCmd("%s %s" % (l_RemoveFilePgm, pFile))
        runCmd("%s %s" % (l_CreateFilePgm, pFile))

    return

def getContribId(pPrintValue=DO_NOT_PRINT_VALUE):
    return Coral_GetVar('contribid', pPrintValue)

def getHandles(pStatus=BBSTATUS["BBALL"]):
    l_NumAvailHandles = 0
    l_NumHandles = -1
    while (l_NumAvailHandles > l_NumHandles):
        l_NumHandles = l_NumAvailHandles
        (l_NumAvailHandles, l_Handles) = BB_GetTransferList(pStatus, l_NumHandles)

    print "getHandles: jobid=%d, jobstepid=%d, contribid=%d, l_NumAvailHandles=%d, l_Handles=%s" % (
           (getJobId(), getJobStepId(), getContribId(), l_NumAvailHandles, `l_Handles`))

    return l_Handles

def getJobId(pPrintValue=DO_NOT_PRINT_VALUE):
    return Coral_GetVar('jobid', pPrintValue)

def getJobStepId(pPrintValue=DO_NOT_PRINT_VALUE):
    return Coral_GetVar('jobstepid', pPrintValue)

def incrJobStepId(pValue=1):
    l_JobStepId = getJobStepId()
    l_JobStepId = l_JobStepId + pValue
    setJobStepId(l_JobStepId)

    return

def initEnv(pEnv, pMountpoints=None, pDirectories=None):
    # Cleanup from prior variations...

    l_RC = 0
    l_Continue = True;

#    print
#    print ">>>> Start: Cleanup..."

    l_Suffix = "Tolerated exception and processing continues..."
    l_SuffixEnd = "Non-tolerated exception and processing is ending..."

    # NOTE: The initialization must occur first so that if removejobinfo is
    #       issued below, the removejobinfo is issued for the correct jobid.
    if (l_Continue):
#        print ">>>> Start: Initialize environment..."

        l_Iteration = pEnv.get("iteration", 0)
        setJobId(pEnv['jobid'] + l_Iteration*10)
        setJobStepId(pEnv['jobstepid'])
        setContribId(pEnv['contribid'])

#        print ">>>>   End: Initialize environment..."
    else:
        l_Continue = False

    if ((pMountpoints is not None) or (pDirectories is not None)):
        if l_Continue:
            # If multiple contributors and contribid is not 0,
            # delay waiting for the removejobinfo to be processed
            # and possibly propagated amongst bbServers.
            # Otherwise, issue the removejobinfo operation.
#            if pEnv['contribid'] != 0:
            if len(pEnv['contrib']) > 1 and pEnv['contribid'] != 0:
                time.sleep(DEFAULT_REMOVE_JOB_INFO_DELAY)
            else:
                try:
                    sudo_RemoveJobInfo(pEnv)
                except BBError as error:
                    l_Continue = error.handleError()

            if l_Continue:
                if pDirectories is not None:
                    if type(pDirectories) not in (tuple, list):
                        try:
                            l_Directory = pDirectories
                            sudo_RemoveDirectory(pEnv, l_Directory)
                        except BBError as error:
                            l_Continue = error.handleError()
                    else:
                        for l_Directory in pDirectories:
                            if l_Continue:
                                try:
                                    sudo_RemoveDirectory(pEnv, l_Directory)
                                except BBError as error:
                                    l_Continue = error.handleError()
                if l_Continue:
                    if pMountpoints is not None:
                        if type(pMountpoints) not in (tuple, list):
                            try:
                                l_Mountpoint = pMountpoints
                                sudo_RemoveLogicalVolume(pEnv, l_Mountpoint)
                            except BBError as error:
                                l_Continue = error.handleError()
                            if l_Continue:
                                try:
                                    sudo_RemoveDirectory(pEnv, l_Mountpoint)
                                except BBError as error:
                                    l_Continue = error.handleError()
                        else:
                            for l_Mountpoint in pMountpoints:
                                if l_Continue:
                                    try:
                                        sudo_RemoveLogicalVolume(pEnv, l_Mountpoint)
                                    except BBError as error:
                                        l_Continue = error.handleError()
                                if l_Continue:
                                    try:
                                        sudo_RemoveDirectory(pEnv, l_Mountpoint)
                                    except BBError as error:
                                        l_Continue = error.handleError()
                        if not l_Continue:
                            l_RC = -1
                else:
                    l_RC = -1
            else:
                l_RC = -1
        else:
            l_RC = -1

#    print ">>>>   End: Cleanup..."

    return l_RC

def printLastErrorDetailsSummary():
    print datetime.now().strftime("Current date/time: %Y-%m-%d %H:%M:%S")
    dummy = BBError()
    print dummy.getLastErrorDetailsSummary()

    return

def printStruct(pStruct):
    print "Start: Print of struct %s" % (pStruct)
    for l_FieldName, l_FieldType in pStruct._fields_:
        print "%s%s = %s" % (len("Start: ")*" ", l_FieldName, getattr(pStruct, l_FieldName))
    print "  End: Print of struct %s" % (pStruct)

    return

def printEnv(pEnv):
    print "Start: Print of environment"
    bbpprint().pprint(pEnv)
    print "  End: Print of environment"

    return

def runCmd(pCmd):
    print pCmd
    os.system(pCmd)

    return

# Setting the jobid from a program is not necessary if bbapi_main is invoked
# to run a testcase.  Changing the jobid in the middle of a bbapi testcase
# is 'cheating', as the same connection is being used.  But, it works to test
# most scenarios...  @DLH
def setJobId(pValue):
    Coral_SetVar('jobid', `pValue`)

    return

def setJobStepId(pValue):
    Coral_SetVar('jobstepid', `pValue`)

    return

# Setting the contribid from a program is not necessary if bbapi_main is invoked
# to run a testcase.  Changing the contribid in the middle of a bbapi testcase
# is 'cheating', as the same connection is being used.  But, it works to test
# most scenarios...  @DLH
def setContribId(pValue):
    Coral_SetVar('contribid', `pValue`)

    return

def waitForCompletion(pEnv, pHandles, pAttempts=DEFAULT_WAIT_FOR_COMPLETION_ATTEMPTS):
    l_Complete = False
    l_AllFullSuccess = True;
    l_Attempts = 1
    l_LastTotalTransferSize = 0
    l_Status = []
    l_TransferSize = []

    l_CopyHandles = deepcopy(pHandles)
    while ((not l_Complete) and l_Attempts < pAttempts):
        l_Complete = True
        l_Handles = deepcopy(l_CopyHandles)
        for l_Handle in l_Handles:
            l_Info = BB_GetTransferInfo(l_Handle)
            # NOTE: BBSTOPPED is not included below because we want to 'spin' on a stopped transfer definition,
            #       waiting for it to progress to another status.
            # NOTE: BBPARTIALSUCCESS is not included below because we want to 'spin' on a it also, waiting for
            #       it to progress back to BBINPROGRESS.  It is possible in the restart scenarios if all of the
            #       extents are processed, but the restart processing hasn't finished yet.
            if BBSTATUS[l_Info.status] not in ("BBFULLSUCCESS", "BBCANCELED",):
                l_Complete = False
                if l_LastTotalTransferSize != l_Info.totalTransferSize:
                    # Making progress...  Reset the number of attempts...
                    l_LastTotalTransferSize = l_Info.totalTransferSize
                    l_Attempts = 0
                if len(pEnv["contrib"]) == 1 or bb.getContribId() == 0:
                    l_Delay = DEFAULT_GET_TRANSFERINFO_DELAY
                else:
                    l_Delay = DEFAULT_GET_TRANSFERINFO_DELAY*2
                time.sleep(l_Delay)
                break
            else:
                if BBSTATUS[l_Info.status] not in ("BBFULLSUCCESS",) or BBSTATUS[l_Info.localstatus] not in ("BBFULLSUCCESS",):
                    l_AllFullSuccess = False
                l_Status.append((BBSTATUS[l_Info.localstatus],BBSTATUS[l_Info.status]))
                l_TransferSize.append((l_Info.localTransferSize,l_Info.totalTransferSize))
                l_CopyHandles.remove(l_Handle)
                l_Attempts = 0
        l_Attempts += 1

    if (l_Complete):
        if (len(pHandles) > 1):
            print

        for i in xrange(len(pHandles)):
            print "    *FINAL* Handle: %12s -> Status (Local:Overall) (%13s:%13s)   Transfer Size in bytes (Local:Total) (%s : %s)" % (pHandles[i], l_Status[i][0], l_Status[i][1], '{:,}'.format(l_TransferSize[i][0]), '{:,}'.format(l_TransferSize[i][1]))
    else:
        l_AllFullSuccess = False
        print "Exceeded the maximum number of attempts to have all handles reach a status of BBFULLSUCCESS.  %d attempts were made." % (pAttempts)

    return l_AllFullSuccess
