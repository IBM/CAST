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

import Common as cmn

# Establishes environmental values in the context, overriding defaults with command line options
def getOptions(pCtx, pArgs):
    # \todo Input/verification of args needs to be beefed up...

    # Set the default values
    pCtx["ROOTDIR"] = "."
    pCtx["PRINT_PICKLED_RESULTS"] = False

    # Process the input args
    if len(pArgs):
        if len(pArgs) >= 2:
            pCtx["ROOTDIR"] = pArgs[1]

    # Normalize the input root path name
    pCtx["ROOTDIR"] = os.path.abspath(os.path.normpath(pCtx["ROOTDIR"]))

    # Print out the environment
    print "Environmental data"
    pprint.pprint(pCtx)
    print

    return

def saveData(pCtx, pData):
    # Store the data as a pickle file in the input root directory
    l_PicklePathFileName = os.path.join(pCtx["ROOTDIR"], "ConsoleData.pickle")
    l_PickleFile = open(l_PicklePathFileName, "wb")
    pickle.dump(pData, l_PickleFile)
    l_PickleFile.close()
    print "Console data results saved to %s" % l_PicklePathFileName

    return

def loadData(pCtx):
    # Store the data as a pickle file in the input root directory
    l_PicklePathFileName = os.path.join(pCtx["ROOTDIR"], "ConsoleData.pickle")
    l_PickleFile = open(l_PicklePathFileName, "rb")
    l_Data = pickle.load(l_PickleFile)
    l_PickleFile.close()
    print "Console data results loaded from %s" % l_PicklePathFileName

    return l_Data

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

def ensure(pPathName):
    if not os.path.exists(pPathName):
        os.makedirs(pPathName)

    return

def writeOutput(pCtx, pFileName, pOutput):
    l_File = open(pFileName, "w")
    l_File.writelines(pOutput)
    l_File.close()
    print "Results saved to %s" % pFileName

    return

def getServers(pData):
    l_Servers = pData.keys()
    l_Servers.sort()

    return l_Servers

def getJobIds(pData):
    l_JobIds = set()

    l_Servers = getServers(pData)
    for l_Server in l_Servers:
        l_JobIdsToAdd = cmn.getJobIdsForServer(pData, l_Server)
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

    l_Handles = cmn.getHandlesForServer(pData, pServer)
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
