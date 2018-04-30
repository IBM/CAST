#!/usr/bin/python
####################################################
#    bbapiTest.py
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
    This module provides wrappers to the following bbapi test functions.

int Coral_ChangeServer(const char* pMountpoint, const int pTransferTimeInterval, const int pWorkItemsPerTransferTimeInterval);
int Coral_GetVar(const char* pVariable);
int Coral_SetVar(const char* pVariable, const char* pValue);
int Coral_StageOutStart(const char* pMountpoint);
"""

from ctypes import *
# import json

import bb
from bberror import *

PRINT_VALUE = False


#
# Wrappers for test bbapi calls
#

def Coral_ChangeServer(pMountpoint):
    l_Mountpoint = bb.cvar("mountpoint", pMountpoint)

    print "%sCoral_ChangeServer issued for mountpoint %s" % (os.linesep, pMountpoint)
    rc = bb.api.Coral_ChangeServer(l_Mountpoint)
    if (rc):
        raise Coral_ChangeServerError(rc)

    return

# // NOTE: Coral_G/SetVar currently only supports writing ascii values to be read as positive integers
def Coral_GetVar(pVariable, pPrintValue=PRINT_VALUE):
    l_Variable = bb.cvar("variable", pVariable)
    l_Value = bb.api.Coral_GetVar(l_Variable)
    if pPrintValue:
        print "===> Variable %s = %d" % (pVariable, l_Value)

    return l_Value

# // NOTE: BB_G/SetVar currently only supports writing ascii values to be read as positive integers
def Coral_SetVar(pVariable, pValue):
    try:
        l_Value = int(pValue)
        if (l_Value < 0):
            raise ValueError

    except ValueError:
        l_Value = None

    if (l_Value != None):
        l_Variable = bb.cvar("variable", pVariable)
        l_Value = bb.cvar("value", pValue)
        l_PreviousValue = Coral_GetVar(pVariable)
#        print "===> The value for variable '%s' is being changed from %d to %s" % (pVariable, l_PreviousValue, pValue)
        l_Value = bb.api.Coral_SetVar(l_Variable, l_Value)
        l_CurrentValue = Coral_GetVar(pVariable, False)
        if (int(pValue) != l_CurrentValue):
            print "===> Coral_SetVar Error:  Variable %s could not be set to a value of %s" % (pVariable, pValue)
    else:
        print "===> Coral_SetVar Error:  Input value of %s not 0 or a positive value" % (pValue)

    return

def Coral_StageOutStart(pMountpoint):
    l_Mountpoint = bb.cvar("mountpoint", pMountpoint)

    print "%sCoral_StageOutStart issued for mountpoint %s" % (os.linesep, pMountpoint)
    rc = bb.api.Coral_StageOutStart(l_Mountpoint)
    if (rc):
        raise Coral_StageOutStartError(rc)

    return
