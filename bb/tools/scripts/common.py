#!/usr/bin/python
###########################################################
#     common.py
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


import os
import pickle
import pprint
import re

import datetime as dt


# Establishes environmental values in the context, overriding defaults with command line options
def getOptions(pCtx, pArgs):
    # \todo Input/verification of args needs to be beefed up...

    # Set the default values
    pCtx["ROOTDIR"] = "."
    pCtx["OUTPUT_DIRECTORY_NAME"] = "Analysis"
    pCtx["PICKLE_FILENAME"] = "ConsoleData.pickle"
    pCtx["PRINT_PICKLED_RESULTS"] = False

    # Process the input args
    if len(pArgs):
        if len(pArgs) >= 2:
            pCtx["ROOTDIR"] = pArgs[1]

    # Normalize the input root path name
    pCtx["ROOTDIR"] = os.path.abspath(os.path.normpath(pCtx["ROOTDIR"]))
    pCtx["OUTPUT_DIRECTORY"] = os.path.join(pCtx["ROOTDIR"], pCtx["OUTPUT_DIRECTORY_NAME"])

    # Print out the environment
    print "Environmental data"
    pprint.pprint(pCtx)
    print

    return

def saveData(pCtx):
    # Store the data as a pickle file in the input root directory/Analysis
    l_PicklePathFileName = os.path.join(pCtx["OUTPUT_DIRECTORY"], pCtx["PICKLE_FILENAME"])
    l_PickleFile = open(l_PicklePathFileName, "wb")
    pickle.dump(pCtx["ServerData"], l_PickleFile)
    pickle.dump(pCtx["ElapsedTimeData"], l_PickleFile)
    l_PickleFile.close()
    print "Console data results saved to %s" % l_PicklePathFileName

    return

def loadData(pCtx):
    # Load the data as a pickle file from the input root directory/Analysis
    l_PicklePathFileName = os.path.join(pCtx["OUTPUT_DIRECTORY"], pCtx["PICKLE_FILENAME"])
    l_PickleFile = open(l_PicklePathFileName, "rb")
    pCtx["ServerData"] = pickle.load(l_PickleFile)
    pCtx["ElapsedTimeData"] = pickle.load(l_PickleFile)
    l_PickleFile.close()
    print "Console data results loaded from %s" % l_PicklePathFileName

    return

def printFormattedData(pCtx, pData):
    # Print out the data, formatted
    print "Collected bbServer data"
    pprint.pprint(pData)
    print

    return

def printFormattedFile(pCtx, pFileName, pData):
    # Print out the data, formatted
    l_File = open(pFileName, 'w')
    l_Printer = pprint.PrettyPrinter(indent=0, stream=l_File)
    l_Printer.pprint(pData)
    l_File.close()
    print "Results saved to %s" % pFileName

    return

def writeOutput(pCtx, pFileName, pOutput):
    l_File = open(pFileName, "w")
    l_File.writelines(pOutput)
    l_File.close()
    print "Results saved to %s" % pFileName

    return

def calculateTimeDifferenceInSeconds(pDateTime1, pDateTime2):
    # NOTE: Extremely simplistic.  Currently , we ignore the date value and only factor in the time values.
    #       Current code WILL NOT work if job spans midnight...
    SPLIT_TIME = re.compile("(\d+):(\d+):(\d+)\.(\d+)")

    l_Seconds = None
    if (pDateTime1[1] == None or pDateTime2[1] == None):
        print "DateTime values are not available"
    else:
        l_Success = SPLIT_TIME.search(pDateTime1[1])
        if l_Success:
            l_Args = map(int, l_Success.groups(0))
            l_Time1 = dt.timedelta(hours=l_Args[0], minutes=l_Args[1], seconds=l_Args[2], microseconds=l_Args[3])
            l_Success = SPLIT_TIME.search(pDateTime2[1])
            if l_Success:
                l_Args = map(int, l_Success.groups(0))
                l_Time2 = dt.timedelta(hours=l_Args[0], minutes=l_Args[1], seconds=l_Args[2], microseconds=l_Args[3])
                l_TimeDelta = l_Time1 - l_Time2
                l_Seconds = float(l_TimeDelta.total_seconds())
            else:
                print "pDataTime2 cannot be parsed"
                l_Seconds = None
        else:
            print "pDataTime1 cannot be parsed"
            l_Seconds = None

    return l_Seconds

def compareTimes(pDateTime1, pDateTime2):
    # NOTE: Extremely simplistic.  Currently , we do a string compare on the time values.
    #       Current code WILL NOT work if job spans midnight...
    if pDateTime1[1] < pDateTime2[1]:
        rc = -1
    elif pDateTime1[1] > pDateTime2[1]:
        rc = 1
    else:
        rc = 0

    return rc

def ensure(pPathName):
    if not os.path.exists(pPathName):
        os.makedirs(pPathName)

    return

def getServers(pData):
    l_Servers = pData.keys()
    l_Servers.sort()

    return l_Servers

def getJobIds(pData):
    l_JobIds = set()

    l_Servers = getServers(pData)
    for l_Server in l_Servers:
        l_JobIdsToAdd = getJobIdsForServer(pData, l_Server)
        if l_JobIdsToAdd:
            for l_JobId in l_JobIdsToAdd:
                l_JobIds.add(l_JobId)
    l_JobIds = [l_JobId for l_JobId in l_JobIds]
    l_JobIds.sort()

    return l_JobIds

def getHandlesForServer(pData, pServer):
    l_Handles = pData[pServer]["Handles"].keys()
    l_Handles.sort()

    return l_Handles

def getJobIdsForServer(pData, pServer):
    l_JobIds = set()

    l_Handles = getHandlesForServer(pData, pServer)
    for l_Handle in l_Handles:
        if "JobId" in pData[pServer]["Handles"][l_Handle]:
            l_JobIds.add(pData[pServer]["Handles"][l_Handle]["JobId"])
    l_JobIds = [l_JobId for l_JobId in l_JobIds]
    l_JobIds.sort()

    return l_JobIds

def getServersForJobid(pData, pJobId):
    l_ServersToReturn = []

    l_Servers = getServers(pData)
    for l_Server in l_Servers:
        l_JobIds = getJobIdsForServer(pData, l_Server)
        if pJobId in l_JobIds:
            l_ServersToReturn.append(l_Server)
    l_ServersToReturn.sort()

    return l_ServersToReturn

# pData is pData[pServerValue]["Handles"][pHandleValue]
def getConnections(pData):
    l_Connections = pData["Connections"].keys()
    l_Connections.sort()

    return l_Connections

# pData is pData[pServerValue]["Handles"][pHandleValue]["Connections"][pConnectionValue]["LVUuids"][pLVUuidValue]
def getContribs(pData):
    l_Contribs = pData["Contribs"].keys()
    l_Contribs.sort()

    return l_Contribs

# pData is pData[pServerValue]["Handles"][pHandleValue]["Connections"][pConnectionValue]["LVUuids"][pLVUuidValue]["Contribs"][pContribIdValue]
def getFiles(pData):
    l_Files = pData["Files"].keys()
    l_Files.sort()

    return l_Files

# pData is pData[pServerValue]["Handles"][pHandleValue]["Connections"][pConnectionValue]
def getLVUuids(pData):
    l_LVUuids = pData["LVUuids"].keys()
    l_LVUuids.sort()

    return l_LVUuids

def getHandlesForServer(pData, pServer):
    l_Handles = pData[pServer]["Handles"].keys()
    l_Handles.sort()

    return l_Handles

def getHandlesForServer(pData, pServer):
    l_Handles = pData[pServer]["Handles"].keys()
    l_Handles.sort()

    return l_Handles

def getErrorsForServer(pData, pServer):

    return pData[pServer]["Errors"]

def getWrkQTimeStampsForServer(pData, pServer):
    l_TimeStamps = pData[pServer]["WorkQueueMgr"].keys()
    l_TimeStamps.sort()

    return l_TimeStamps
