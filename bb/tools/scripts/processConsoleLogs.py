#!/usr/bin/python
###########################################################
#     processConsoleLogs.py
#
#     Copyright IBM Corporation 2015,2016. All Rights Reserved
#
#     This program is licensed under the terms of the Eclipse Public License
#     v1.0 as published by the Eclipse Foundation and available at
#     http://www.eclipse.org/legal/epl-v10.html
#
#     U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#     restricted by GSA ADP Schedule Contract with IBM Corp.
###########################################################


import copy
import os
import re
import pprint
import sys

import Common as cmn

# NOTE: Handles aren't always requested 'everywhere', so we don't search for this entry anymore...
def GetHandle(pCtx, pData, *pArgs):
    pLogDate, pLogTime, pHostName, pConnection, pConnectionIP, pLVUuid, pJobId, pJobStepId, pTag, pNumContrib, pContribs, pHandle = pArgs[0]
    l_GetHandleEntry = pData["Handles"][pHandle]
    # l_GetHandleEntry["LogDate"] = pLogDate
    # l_GetHandleEntry["LogTime"] = pLogTime
    l_GetHandleEntry["JobId"] = pJobId
    l_GetHandleEntry["JobStepId"] = pJobStepId
    l_GetHandleEntry["Tag"] = pTag
    l_GetHandleEntry["NumContrib"] = pNumContrib
    l_GetHandleEntry["Contribs"] = pContribs

    return

def FileCompletion(pCtx, pData, *pArgs):
    pLogDate, pLogTime, pFileName, pConnection, pConnectionIP, pLVUuid, pHandle, pContribId, pSourceIndex, pStatus, pTransferType, pTransferSize, pReadCount, pReadTime, pWriteCount, pWriteTime = pArgs[0]
    if "Files" not in pData["Handles"][pHandle]["Connections"][pConnection]["LVUuids"][pLVUuid]["ContribIds"][pContribId]:
        pData["Handles"][pHandle]["Connections"][pConnection]["LVUuids"][pLVUuid]["ContribIds"][pContribId]["Files"] = {}
    pData["Handles"][pHandle]["Connections"][pConnection]["LVUuids"][pLVUuid]["ContribIds"][pContribId]["Files"][pFileName] = {}
    l_FileEntry = pData["Handles"][pHandle]["Connections"][pConnection]["LVUuids"][pLVUuid]["ContribIds"][pContribId]["Files"][pFileName]
    l_FileEntry["LogTime"] = pLogTime
    l_FileEntry["SourceIndex"] = int(pSourceIndex)
    l_FileEntry["Status"] = pStatus
    l_FileEntry["TransferType"] = pTransferType
    l_FileEntry["TransferSize"] = int(pTransferSize)
    l_FileEntry["ReadCount"] = int(pReadCount)
    l_FileEntry["ReadTime"] = float(pReadTime)
    l_FileEntry["WriteCount"] = int(pWriteCount)
    l_FileEntry["WriteTime"] = float(pWriteTime)

    return

def ContribIdCompletion(pCtx, pData, *pArgs):
    pLogDate, pLogTime, pContribId, pConnection, pConnectionIP, pLVUuid, pHandle, pStatus, pProcessingTime = pArgs[0]
    l_ContribIdEntry = pData["Handles"][pHandle]["Connections"][pConnection]["LVUuids"][pLVUuid]["ContribIds"][pContribId]
    l_ContribIdEntry["LogTime"] = pLogTime
    l_ContribIdEntry["Status"] = pStatus
    l_ContribIdEntry["ProcessingTime"] = pProcessingTime

    return

def HandleCompletion(pCtx, pData, *pArgs):
    pLogDate, pLogTime, pHandle, pConnection, pConnectionIP, pLVUuid, pStatus = pArgs[0]
    l_HandleEntry = pData["Handles"][pHandle]
    l_HandleEntry["LogTime"] = pLogTime
    l_HandleEntry["Status"] = pStatus

    return

def StartTransfer(pCtx, pData, *pArgs):
    pLogDate, pLogTime, pConnection, pConnectionIP, pLVUuid, pHostName, pJobId, pJobStepId, pHandle, pContribId, pRestart = pArgs[0]
    l_GetHandleEntry = pData["Handles"][pHandle]
    l_GetHandleEntry["JobId"] = pJobId
    l_GetHandleEntry["JobStepId"] = pJobStepId
    l_StartTransferEntry = pData["Handles"][pHandle]["Connections"][pConnection]["LVUuids"][pLVUuid]["ContribIds"][pContribId]
    l_StartTransferEntry["JobId"] = pJobId
    l_StartTransferEntry["JobStepId"] = pJobStepId
    l_StartTransferEntry["Restart"] = pRestart

    return

def StartDumpWorkQueueMgr(pCtx, pData, *pArgs):
    pLineData = pArgs[0][0]
    if pCtx["WORK_QUEUE_MGR_DATA"] == None:
        pCtx["WORK_QUEUE_MGR_DATA"] = []
    pCtx["WORK_QUEUE_MGR_DATA"].append(pLineData[:-1])

    return

def EndDumpWorkQueueMgr(pCtx, pData, *pArgs):
    pLineData, pLogDate, pLogTime = pArgs[0]
    StartDumpWorkQueueMgr(pCtx, pData, (pLineData,))
    pCtx["WORK_QUEUE_MGR_DATA"].append("")

    l_DumpWorkQueueMgrEntry = pData["WorkQueueMgr"]
    l_DumpWorkQueueMgrEntry[pLogTime] = copy.deepcopy(pCtx["WORK_QUEUE_MGR_DATA"])
    pCtx["WORK_QUEUE_MGR_DATA"] = None

    return

def Error(pCtx, pData, *pArgs):
    pLogDate, pLogTime, pErrorData = pArgs[0]
    if "Errors" not in pData:
        pData["Errors"] = {}
    l_ErrorEntry = pData["Errors"]
    l_ErrorEntry[pLogTime] = pErrorData

    return

Handle = None
Connection = None
ConnectionIP = None
LVUuid = None
ContribId = None

# KEYS - Tuple of tuples, (key, data type)
KEYS = (("Handle",int), ("Connection",str), ("ConnectionIP",str), ("LVUuid",str), ("ContribId",int))

# SEARCHES - Tuple of tuples, (RegEx expression, tuple of group(i) for each key value (0 indicates not available), data types for each group(i), procedure to process parsed data)
SEARCHES = (
#    (re.compile("(\d+-\d+-\d+)\s+(\d+:\d+:\d+.\d+).*getHandle:\s+hostname\s+([A-Za-z0-9]+),\s+LVKey\(([a-z0-9.]+)\s+\(([0-9.]+)\),([a-z0-9-]+)\),\s+job\((\d+),(\d+)\),\s+tag\s+(\d+),\s+numcontrib\s+(\d+),\s+contrib\s+(\([0-9,]\))\s+->\s+handle\s+(\d+)"),
#     (12, 4, 5, 6, 0),
#     (str, str, str, str, str, str, int, int, int, int, str, int,),
#     GetHandle),
    (re.compile("(\d+-\d+-\d+)\s+(\d+:\d+:\d+.\d+).*msgin\_starttransfer\s+\(1\):\s+Start\s+processing\s+LVKey\(([a-z0-9.]+)\s+\(([0-9.]+)\),([a-z0-9-]+)\),\s+hostname\s+([A-Za-z0-9]+),\s+jobid\s+(\d+),\s+jobstepid\s+(\d+),\s+handle\s+(\d+),\s+contribid\s+(\d+),\s+restart=(true|false)"),
     (9, 3, 4, 5, 10),
     (str, str, str, str, str, str, int, int, int, int, str,),
     StartTransfer),
    (re.compile("(\d+-\d+-\d+)\s+(\d+:\d+:\d+.\d+).*->bbproxy:\s+Transfer\s+completed\s+for\s+file\s+([A-Za-z0-9/._-]+),\s+LVKey\(([a-z0-9.]+)\s+\(([0-9.]+)\),([a-z0-9-]+)\),\s+handle\s+(\d+),\s+contribid\s+(\d+),\s+sourceindex\s+(\d+),\s+file\s+status\s+([A-Z_]+),\s+transfer\s+type\s+([A-Za-z_]+),\s+size\s+transferred\s+is\s+(\d+)\s+bytes,\s+read\s+count/cumulative\s+time\s+(\d+)/([0-9.]+)\s+seconds,\s+write\s+count/cumulative\s+time\s+(\d+)/([0-9.]+)\s+seconds"),
     (7, 4, 5, 6, 8),
     (str, str, str, str, str, str, int, int, int, str, str, int, int, float, int, float,),
     FileCompletion),
    (re.compile("(\d+-\d+-\d+)\s+(\d+:\d+:\d+.\d+).*->bbproxy:\s+Transfer\s+completed\s+for\s+contribid\s*(\d+),\s*LVKey\(([a-z0-9.]+)\s+\(([0-9.]+)\),([a-z0-9-]+)\),\s+handle\s+(\d+),\s*status\s*([A-Z_]+), total\s*processing\s*time\s*([0-9.]+)\s*seconds"),
     (7, 4, 5, 6, 3),
     (str, str, int, str, str, str, int, str, float,),
     ContribIdCompletion),
    (re.compile("(\d+-\d+-\d+)\s+(\d+:\d+:\d+.\d+).*->bbproxy:\s+Transfer\s+completed\s+for\s+handle\s*(\d+),\s*LVKey\(([a-z0-9.]+)\s+\(([0-9.]+)\),([a-z0-9-]+)\),\s+status\s+([A-Z_]+)"),
     (3, 4, 5, 6, 0),
     (str, str, int, str, str, str, str,),
     HandleCompletion),
    (re.compile("(\d+-\d+-\d+)\s+(\d+:\d+:\d+.\d+).*bb::error\s*\|\s*(.*)"),
     (0, 0, 0, 0, 0),
     (str, str, str,),
     Error),
    (re.compile("(.*>>>>>\s+Start:\s+WRKQMGR\s+Work\s+Queue\s+Mgr\s+\(Not\s+an\s+error\s+-\s+Timer\s+Interval\)\s+<<<<<)"),
     (0, 0, 0, 0, 0),
     (str,),
     StartDumpWorkQueueMgr),
    (re.compile("((\d+-\d+-\d+)\s+(\d+:\d+:\d+.\d+).*>>>>>\s+End:\s+WRKQMGR\s+Work\s+Queue\s+Mgr\s+\(Not\s+an\s+error\s+-\s+Timer\s+Interval\)\s+<<<<<)"),
     (0, 0, 0, 0, 0),
     (str, str, str,),
     EndDumpWorkQueueMgr),
)
WORK_QUEUE_MGR_DATA = re.compile("Throttle\s*Mode:|Last\s*Queue\s*Processed:|Async\s*Seq#:|Number\s*of\s*Workqueue\s*Entries:|LVKey\(None,[f-]+\),\s+Job\s+\d+|LVKey\([a-z0-9.]+\s+\([0-9.]+\),[a-z0-9-]+\),\s+Job\s+\d+")

# Establishes environmental values in the context, overriding defaults with command line options
def getOptions(pCtx, pArgs):
    # \todo Input/verification of args needs to be beefed up...

    # Set the default values
    pCtx["ROOTDIR"] = "."
    pCtx["NAME_PATTERN"] = ("console.*", re.compile("console.*"))
    pCtx["INPUT_SERVER_NAME"] = None
    pCtx["ALL_SERVER_NAMES_PRIMED"] = False
    pCtx["PRINT_PICKLED_RESULTS"] = False
    pCtx["SAVE_PICKLED_RESULTS"] = True

    # Process the input args
    if len(pArgs):
        if len(pArgs) >= 2:
            pCtx["ROOTDIR"] = pArgs[1]
            if len(pArgs) >= 3:
                pCtx["NAME_PATTERN"] = (pArgs[2], re.compile(pArgs[2]))
                if len(pArgs) >= 4:
                    pCtx["INPUT_SERVER_NAME"] = pArgs[3]

    # Normalize the input root path name
    pCtx["ROOTDIR"] = os.path.abspath(os.path.normpath(pCtx["ROOTDIR"]))
    pCtx["WORK_QUEUE_MGR_DATA"] = None

    # Print out the environment
    print "Environmental data"
    pprint.pprint(pCtx)
    print

    return

# Return the server name from the path
def getServerName(pCtx, pPath):
    l_ServerName = None
    if pCtx["INPUT_SERVER_NAME"]:
        l_ServerName = pCtx["INPUT_SERVER_NAME"]
    else:
        l_Path = [pPath,pPath]
        while (l_Path[0] != pCtx["ROOTDIR"]):
            l_Path = os.path.split(l_Path[0])
            l_ServerName = l_Path[1]

    return l_ServerName

# Parse/process a given line of output
def processLine(pCtx, pData, pLine):
    l_DoProcessLine = False
    l_ArgsIsEntireLine = False

    # Search for an interesting line...
    for l_Search in SEARCHES:
        l_Success = l_Search[0].search(pLine)
        if l_Success:
            break

     # Found an interesting line...
    if l_Success:
        l_DoProcessLine = True
        # Set the key values from the parsed data
        i = 0
        for (l_VariableName, l_Func) in KEYS:
            l_Index = l_Search[1][i]
            if l_Index:
                globals()[l_VariableName] = l_Func(l_Success.group(l_Index))
            else:
                globals()[l_VariableName] = None
            i += 1

        # Prime any necessary data based on the key values
        if Handle != None:
            if Handle not in pData["Handles"]:
                pData["Handles"][Handle] = {}
            if "Connections" not in pData["Handles"][Handle]:
                pData["Handles"][Handle]["Connections"] = {}
            if Connection != None:
                if Connection not in pData["Handles"][Handle]["Connections"]:
                    pData["Handles"][Handle]["Connections"][Connection] = {}
                if ConnectionIP != None:
                    pData["Handles"][Handle]["Connections"][Connection]["ConnectionIP"] = ConnectionIP
                if LVUuid != None:
                    if "LVUuids" not in pData["Handles"][Handle]["Connections"][Connection]:
                        pData["Handles"][Handle]["Connections"][Connection]["LVUuids"] = {}
                    if LVUuid not in pData["Handles"][Handle]["Connections"][Connection]["LVUuids"]:
                        pData["Handles"][Handle]["Connections"][Connection]["LVUuids"][LVUuid] = {}
                    if "ContribIds" not in pData["Handles"][Handle]["Connections"][Connection]["LVUuids"][LVUuid]:
                        pData["Handles"][Handle]["Connections"][Connection]["LVUuids"][LVUuid]["ContribIds"] = {}
                    if ContribId != None:
                        if ContribId not in pData["Handles"][Handle]["Connections"][Connection]["LVUuids"][LVUuid]["ContribIds"]:
                            pData["Handles"][Handle]["Connections"][Connection]["LVUuids"][LVUuid]["ContribIds"][ContribId] = {}
    elif pCtx["WORK_QUEUE_MGR_DATA"]:
        l_Success = WORK_QUEUE_MGR_DATA.search(pLine)
        if l_Success:
            # WorkQueueMgr dump in progress...  Treat as 'StartDumpWorkQueueMgr'
            l_Search = SEARCHES[5]
            l_DoProcessLine = True
            l_ArgsIsEntireLine = True

    if l_DoProcessLine:
        # Build the argument list
        if not l_ArgsIsEntireLine:
            l_Args = [l_Search[2][i](l_Success.group(i + 1)) for i in xrange(len(l_Search[2]))]
        else:
            l_Args = (pLine,)
        # Invoke the procedure to handle this parsed data
        l_Search[3](pCtx, pData, l_Args)

# Process files for a given path
def processFiles(pCtx, pData, pPath, pFiles):
    for l_FileName in pFiles:
        l_FullFileName = os.path.join(pPath,l_FileName)
        if os.path.isfile(l_FullFileName) and (not os.path.islink(l_FullFileName)):
            l_Success = pCtx["NAME_PATTERN"][1].search(l_FileName)
            if l_Success:
                print "%sStart: Processing %s as a console log..." % (4*" ", l_FullFileName)
                with open(l_FullFileName) as l_File:
                    while (True):
                        l_Lines = l_File.readlines(64*1024)
                        if (l_Lines):
                            for l_Line in l_Lines:
                               processLine(pCtx, pData, l_Line)
                        else:
                            break
                # NOTE: If we split in the middle of a work queue mgr dump, we lose that data...
                pCtx["WORK_QUEUE_MGR_DATA"] = None
                print "%s  End: Processing %s as a console log..." % (4*" ", l_FullFileName)
            else:
                print "%sSkipping %s, does not match file pattern..." % (4*" ", l_FullFileName)
        else:
            print "%sSkipping %s, not a file or is a link..." % (4*" ", l_FullFileName)

    return

# Walk the root path, processing subdirectories/files along the way
def walkPaths(pCtx, pData, pPath):
    for l_Path, l_SubDirs, l_Files in os.walk(pPath):
        if not pCtx["ALL_SERVER_NAMES_PRIMED"]:
            if pCtx["INPUT_SERVER_NAME"] == None:
                for l_SubDir in l_SubDirs:
                    pData[l_SubDir] = {}
                    pData[l_SubDir]["Handles"] = {}
                    pData[l_SubDir]["WorkQueueMgr"] = {}
            else:
                pData[pCtx["INPUT_SERVER_NAME"]] = {}
                pData[pCtx["INPUT_SERVER_NAME"]]["Handles"] = {}
                pData[pCtx["INPUT_SERVER_NAME"]]["WorkQueueMgr"] = {}
            pCtx["ALL_SERVER_NAMES_PRIMED"] = True

        if l_Files:
            l_ServerName = getServerName(pCtx, l_Path)
            if l_ServerName:
                print "Adding data for bbServer:", l_ServerName
                processFiles(pCtx, pData[l_ServerName], l_Path, l_Files)
                print

    return

# Main routine
def main(*pArgs):
    l_Ctx = {}     # Environmental context
    l_Data = {}    # All data is stored here

    # Establish the context
    getOptions(l_Ctx, pArgs[0])
    # Walk the paths processing data along the way, starting with the input root directory
    walkPaths(l_Ctx, l_Data, l_Ctx["ROOTDIR"])

    if l_Ctx["PRINT_PICKLED_RESULTS"]:
        cmn.printFormattedData(l_Ctx, l_Data)

    if l_Ctx["SAVE_PICKLED_RESULTS"]:
        cmn.saveData(l_Ctx, l_Data)
    else:
        print "Results not saved"

    return


if __name__ == '__main__':
    main(sys.argv)