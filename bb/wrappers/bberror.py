#!/usr/bin/python
####################################################
#    bberror.py
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
from __future__ import print_function
from ctypes import byref
from datetime import datetime

import inspect
import json
import os
import pprint

import bb


#
# BB_GetLastErrorDetails BBERRORFORMAT
#
BBERRORFORMAT = {"BBERRORJSON" : 1,     #### Output string will be in JSON format
                 "BBERRORXML"  : 2,     #### Output string will be in XML format
                 "BBERRORFLAT" : 3,     #### Output string will be in a non-hierarchical name value format
                 1 : "BBERRORJSON",     #### Output string will be in JSON format
                 2 : "BBERRORXML" ,     #### Output string will be in XML format
                 3 : "BBERRORFLAT",     #### Output string will be in a non-hierarchical name value format
                }

def BB_GetLastErrorDetails(pBBError=None, pFormat=BBERRORFORMAT["BBERRORJSON"]):
    l_Format = bb.cvar("errformat", pFormat)
    l_NumBytesAvailable = bb.cvar("numbytesavailable", 4095)
    l_BufferSize = bb.cvar("size", 0)
    while (l_NumBytesAvailable.value > l_BufferSize.value):
        l_BufferSize = bb.cvar("size", l_NumBytesAvailable.value + 1)
        l_Buffer = bb.cvar("buffer", l_BufferSize.value)
        rc = bb.api.BB_GetLastErrorDetails(l_Format, byref(l_NumBytesAvailable), l_BufferSize, byref(l_Buffer));
    if (rc):
        raise BB_GetLastErrorDetailsError(rc)

    l_Temp = []
    for i in xrange(l_BufferSize.value):
        if l_Buffer[i] != '\x00':
            l_Temp.append(l_Buffer[i])
        else:
            break
    l_Output = "".join(l_Temp)

    if (pBBError):
        lastErrorDetails = l_Output
    else:
        print("BB_GetLastErrorDetails: %s" % (l_Output))

    return l_Output


"""
    User exceptions to bbapi functions.
"""

class BBError(Exception):
    DEFAULT_NORMAL_RCs = [0,]
    DEFAULT_TOLERATED_ERROR_RCs = [-2,]

    def __init__(self, rc=-1, text="", func=""):
        self.rc = rc
        if (func == ""):
            self.func = inspect.stack()[2][3]
        if (len(text) > 0):
            self.text = text
            self.lastErrorDetails = ""
        else:
            self.text = "Error raised"
            self.lastErrorDetails = self.getLastErrorDetails()
        self.normalRCs = BBError.DEFAULT_NORMAL_RCs
        self.toleratedErrorRCs = BBError.DEFAULT_TOLERATED_ERROR_RCs

    def __repr__(self):
        return "BBError(rc=%d, text=%s, func=%s, normalRCs=%s toleratedErrorRCs=%s, lastErrorDetails=%s)" % (self.rc, self.text, self.func, self.normalRCs, self.toleratedErrorRCs, self.lastErrorDetails)

    def __str__(self):
        return os.linesep + self.func + ": " + self.text + ", rc=" + repr(self.rc) + os.linesep + self.getLastErrorDetailsSummary()

    def getLastErrorDetails(self):
        return json.loads(BB_GetLastErrorDetails(self))

    def getLastErrorDetailsSummary(self):
        KEYS_IN_SUMMARY = ("rc","env","in","dft","out","error",)

        l_SummaryKeys = KEYS_IN_SUMMARY

        # For tolerated exceptions, return a true summary.
        # Otherwise, return everything in the error details.
        if int(self.lastErrorDetails.get("rc", "0")) not in self.getAllToleratedRCs():
            l_SummaryKeys = self.lastErrorDetails.keys()

        # Return only those keys with data...
        l_Output = []
        for l_Key in l_SummaryKeys:
            if l_Key in self.lastErrorDetails:
                l_Output.append("[%s]=%s" % (l_Key, bb.bbpprint().pformat(self.lastErrorDetails[l_Key])))

        return (os.linesep.join(l_Output))

    def getAllToleratedRCs(self):
        return self.normalRCs + self.toleratedErrorRCs

    def getNormalRCs(self):
        return self.normalRCs

    def getToleratedErrorRCs(self):
        return self.toleratedErrorRCs

    def handleError(self, moreToleratedRCs=None):
        l_Continue = True
        l_Prefix = "Tolerated"
        l_AllToleratedRCs = self.getAllToleratedRCs()

        if moreToleratedRCs:
            if type(moreToleratedRCs) in (tuple, list):
                for l_RC in moreToleratedRCs:
                    l_AllToleratedRCs.append(l_RC)
            else:
                l_AllToleratedRCs.append(moreToleratedRCs)

        if self.rc not in l_AllToleratedRCs:
            l_Prefix = "Non-tolerated"
            l_Continue = False

        print(datetime.now().strftime("Current date/time: %Y-%m-%d %H:%M:%S"))
        print("%s exception from %s, rc=%d" % (l_Prefix, self.func, self.rc))
        if (not l_Continue):
            print("%s" % (repr(self)))

        return l_Continue

class BB_AddFilesError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_AddKeysError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_CancelTransferError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_ChangeModeError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_ChangeOwnerError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_CloseServerError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)
        self.toleratedErrorRCs.append(22)

class BB_CreateDirectoryError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_CreateLogicalVolumeError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_CreateTransferDefError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_FreeTransferDefError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_GetDeviceUsageError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_GetServerError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_GetServerByNameError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_GetThrottleRateError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_GetTransferHandleError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_GetTransferKeysError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_GetTransferListError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_GetTransferInfoError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_GetUsageError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)
        # Add directory not found as a tolerated exception
        self.toleratedErrorRCs.append(2)

class BB_OpenServerError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)
        self.toleratedErrorRCs.append(114)

class BB_RemoveDirectoryError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_RemoveJobInfoError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_RemoveLogicalVolumeError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_ResizeMountPointError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_RestartTransfersError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_ResumeError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_RetrieveTransfersError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_SetServerError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_SetThrottleRateError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_SetUsageLimitError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_StartTransferError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_StopTransfersError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class BB_SuspendError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)

class Coral_StageOutStartError(BBError):
    def __init__(self, rc):
        BBError.__init__(self, rc)
