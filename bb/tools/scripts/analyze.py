#!/usr/bin/python
###########################################################
#     analyze.py
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
import sys

import common as cmn

# Main routine
def main(*pArgs):
    l_Ctx = {}     # Environmental context
    l_TestOutput = []

    # Establish the context
    cmn.getOptions(l_Ctx, pArgs[0])

    # Load the data as a pickle file from the input root directory
    cmn.loadData(l_Ctx)
    l_Data = l_Ctx["ServerData"]
    l_StageInData = l_Ctx["StageInData"]["jobIds"]
    l_StageOutData = l_Ctx["StageOutData"]["jobIds"]

    # Print the results
    if l_Ctx["PRINT_PICKLED_RESULTS"]:
        cmn.printFormattedData(l_Ctx, l_Data)

    # Determine list of servers
    l_TestOutput.append("List of Servers%s" % (os.linesep))
    l_Servers = cmn.getServers(l_Data)
    l_TestOutput.append("%d server(s), %s%s" % (len(l_Servers), `l_Servers`, os.linesep))
    l_TestOutput.append("%s" % (os.linesep))

    # Determine list of jobids
    l_TestOutput.append("List of JobIds%s" % (os.linesep))
    l_JobIds = cmn.getJobIds(l_Data)
    l_TestOutput.append("%d jobid(s), %s%s" % (len(l_JobIds), `l_JobIds`, os.linesep))
    l_TestOutput.append("%s" % (os.linesep))

    # Determine jobids per server
    l_TestOutput.append("List of JobIds per Server%s" % (os.linesep))
    for l_Server in l_Servers:
        l_JobIds = cmn.getJobIdsForServer(l_Data, l_Server)
        l_TestOutput.append("For server %s, %d jobid(s), %s%s" % (l_Server, len(l_JobIds), `l_JobIds`, os.linesep))
    l_TestOutput.append("%s" % (os.linesep))

    # Determinne servers per jobid
    l_TestOutput.append("List of Servers per JobId%s" % (os.linesep))
    l_JobIds = cmn.getJobIds(l_Data)
    for l_JobId in l_JobIds:
        l_Servers = cmn.getServersForJobid(l_Data, l_JobId)
        l_TestOutput.append("For jobid %d, %d server(s), %s%s" % (l_JobId, len(l_Servers), `l_Servers`, os.linesep))
    l_TestOutput.append("%s" % (os.linesep))

    # Determine handles per jobid, per server, per jobstepid, per connection
    l_TestOutput.append("List of Handles per JobId, per Server, per JobStepId, per Connection%s" % (os.linesep))
    for l_JobId in l_JobIds:
        l_Servers = cmn.getServersForJobid(l_Data, l_JobId)
        l_TestOutput.append("%sJobId %d, %d server(s)%s" % (2*" ", l_JobId, len(l_Servers), os.linesep))
        for l_Server in l_Servers:
            l_JobStepIds = cmn.getJobStepIdsForServerJobId(l_Data, l_Server, l_JobId)
            l_TestOutput.append("%sJobId %d, server %s, %d jobstep(s)%s" % (4*" ", l_JobId, l_Server, len(l_JobStepIds), os.linesep))
            for l_JobStepId in l_JobStepIds:
                l_ConnectionHandleData = cmn.getHandlesPerConnectionForJobIdJobStepId(l_Data, l_Server, l_JobId, l_JobStepId)
                l_ConnectionHandles = l_ConnectionHandleData.keys()
                l_ConnectionHandles.sort()
                l_TestOutput.append("%sJobStepId %d, %d connection(s)%s" % (6*" ", l_JobStepId, len(l_ConnectionHandles), os.linesep))
                for l_Connection in l_ConnectionHandles:
                    l_Handles = l_ConnectionHandleData[l_Connection]
                    l_Handles.sort()
                    l_TestOutput.append("%sConnection %s. %d handle(s), %s%s" % (8*" ", l_Connection, len(l_Handles), `l_Handles`, os.linesep))
            l_TestOutput.append("%s" % (os.linesep))

    # Print out stagein and stageout logs for the found jobids
    l_TestOutput.append("StageIn and StageOut Log Data for Found Jobs%s" % (os.linesep))
    for l_JobId in l_JobIds:
        l_TestOutput.append("%sJobId %d%s" % (2*" ", l_JobId, os.linesep))
        l_TestOutput.append("%sStageIn Log Data%s" % (4*" ", os.linesep))
        l_OutputGenerated = False
        if l_JobId in l_StageInData:
            for l_Line in l_StageInData[l_JobId]:
                l_TestOutput.append("%s%s%s" % (6*" ", l_Line, os.linesep))
                l_OutputGenerated = True
        if not l_OutputGenerated:
            l_TestOutput.append("%sNo stagein data found%s" % (6*" ", os.linesep))
        l_TestOutput.append("%s" % (os.linesep))

        l_TestOutput.append("%sStageOut Log Data%s" % (4*" ", os.linesep))
        l_OutputGenerated = False
        if l_JobId in l_StageOutData:
            for l_Line in l_StageOutData[l_JobId]:
                l_TestOutput.append("%s%s%s" % (6*" ", l_Line, os.linesep))
                l_OutputGenerated = True
        if not l_OutputGenerated:
            l_TestOutput.append("%sNo stageout data found%s" % (6*" ", os.linesep))
        l_TestOutput.append("%s" % (os.linesep))

    # Write out the basic results
    l_PathFileName = os.path.join(l_Ctx["ROOTDIR"], "Analysis", "BasicData.txt")
    cmn.writeOutput(l_Ctx, l_PathFileName, l_TestOutput)
    print "Basic results written to %s" % l_PathFileName
    print

    # Start detailed analysis per jobid/jobstepid, per server
    #
    # For each jobid...
    l_JobIds = cmn.getJobIds(l_Data)
    for l_JobId in l_JobIds:
        l_Output = {}
        l_Output[l_JobId] = {}
        l_NumberOfConnectionsForAllServers = 0
        l_NumberOfHandlesForAllServers = 0

        # For each server...
        l_Servers = cmn.getServersForJobid(l_Data, l_JobId)
        for l_Server in l_Servers:
            l_Output[l_JobId][l_Server] = {}
            l_NumberOfConnectionsForServer = 0
            l_NumberOfHandlesForServer = 0

            # For each jobstepid...
            l_JobStepIds = cmn.getJobStepIdsForServerJobId(l_Data, l_Server, l_JobId)
            for l_JobStepId in l_JobStepIds:
                l_Output[l_JobId][l_Server][l_JobStepId] = {}
                l_Output[l_JobId][l_Server][l_JobStepId]["handles"] = {}
                l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["NumberOfConnections"] = 0
                l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["SizeTransferred"] = 0
                l_Output[l_JobId][l_Server][l_JobStepId]["NotSuccessfulHandles"] = []
                l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["connections"] = {}

                # For each handle...
                l_Handles = cmn.getHandlesForServer(l_Data, l_Server)
                l_NumberOfHandles = 0
                l_HandleProcessingTimes = [None,None]
                l_NotSuccessfulHandles = []
                for l_Handle in l_Handles:
                    if l_Data[l_Server]["Handles"][l_Handle]["JobId"] == l_JobId and \
                       l_Data[l_Server]["Handles"][l_Handle]["JobStepId"] == l_JobStepId:

                        # JobId and JobStepId matches...
                        l_NumberOfHandles = l_NumberOfHandles + 1
                        l_NumberOfHandlesForServer = l_NumberOfHandlesForServer + 1
                        l_NumberOfHandlesForAllServers = l_NumberOfHandlesForAllServers + 1
                        if "Status" in l_Data[l_Server]["Handles"][l_Handle]:
                            if l_Data[l_Server]["Handles"][l_Handle]["Status"] != "BBFULLSUCCESS":
                                l_NotSuccessfulHandles.append((l_Handle, l_Data[l_Server]["Handles"][l_Handle]["Status"]))
                        else:
                            l_NotSuccessfulHandles.append((l_Handle, "NOT_COMPLETED"))
                        l_NumberOfConnections = l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["NumberOfConnections"]
                        l_SizeTransferred = l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["SizeTransferred"]

                        # For each connection...
                        l_Connections = l_Data[l_Server]["Handles"][l_Handle]["Connections"]
                        for l_Connection in l_Connections:
                            if l_Connection not in l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["connections"]:
                                l_NumberOfConnections = l_NumberOfConnections + 1
                                l_NumberOfConnectionsForServer = l_NumberOfConnectionsForServer + 1
                                l_NumberOfConnectionsForAllServers = l_NumberOfConnectionsForAllServers + 1
                                l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["connections"][l_Connection] = {}
                                l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["connections"][l_Connection]["contribIds"] = {}
                                l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["connections"][l_Connection]["contribIds"]["NumberOfContribIds"] = 0
                                l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["connections"][l_Connection]["contribIds"]["processingTimes (File Min/Max)"] = [None,None]
                                l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["connections"][l_Connection]["contribIds"]["readTimes (File Min/Max)"] = [None,None]
                                l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["connections"][l_Connection]["contribIds"]["writeTimes (File Min/Max)"] = [None,None]
                                l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["connections"][l_Connection]["contribIds"]["syncTimes (File Min/Max)"] = [None,None]

                            # For each LVUuid...
                            l_LVUuids = l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"]
                            for l_LVUuid in l_LVUuids:

                                # For each ContribId...
                                l_ContribIds = l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"]
                                l_ProcessingTimes = l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["connections"][l_Connection]["contribIds"]["processingTimes (File Min/Max)"]
                                l_NumberOfContribIds = l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["connections"][l_Connection]["contribIds"]["NumberOfContribIds"]
                                l_NotSuccessfulContribIds = []
                                for l_ContribId in l_ContribIds:
                                    l_NumberOfContribIds = l_NumberOfContribIds + 1
                                    if "Status" in l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"][l_ContribId]:
                                        if l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"][l_ContribId]["Status"] != "BBFULLSUCCESS":
                                            l_NotSuccessfulContribIds.append((l_ContribId, l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"][l_ContribId]["Status"]))
                                    else:
                                        l_NotSuccessfulContribIds.append((l_ContribId,"NOT_COMPLETED"))
                                    if "SizeTransferred" in l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"][l_ContribId]:
                                        l_SizeTransferred += l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"][l_ContribId]["SizeTransferred"]
                                    if "ProcessingTime" in l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"][l_ContribId]:
                                        l_ProcessingTime = (l_ContribId, l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"][l_ContribId]["ProcessingTime"])
                                        if l_ProcessingTimes == [None,None]:
                                            l_ProcessingTimes = [l_ProcessingTime,l_ProcessingTime]
                                        else:
                                            if (l_ProcessingTime[1] < l_ProcessingTimes[0][1]):
                                                l_ProcessingTimes[0] = l_ProcessingTime
                                            if (l_ProcessingTime[1] > l_ProcessingTimes[1][1]):
                                                l_ProcessingTimes[1] = l_ProcessingTime

                                        if l_HandleProcessingTimes == [None,None]:
                                            l_HandleProcessingTimes = l_ProcessingTimes
                                        else:
                                            if (l_ProcessingTimes[0][1] < l_HandleProcessingTimes[0][1]):
                                                l_HandleProcessingTimes[0] = l_ProcessingTimes[0]
                                            if (l_ProcessingTimes[1][1] > l_HandleProcessingTimes[1][1]):
                                                l_HandleProcessingTimes[1] = l_ProcessingTimes[1]

                                    l_ReadTimes = l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["connections"][l_Connection]["contribIds"]["readTimes (File Min/Max)"]
                                    l_WriteTimes = l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["connections"][l_Connection]["contribIds"]["writeTimes (File Min/Max)"]
                                    l_SyncTimes = l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["connections"][l_Connection]["contribIds"]["syncTimes (File Min/Max)"]

                                    l_TransferTypes = set()
                                    if "Files" in l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"][l_ContribId]:
                                        l_Files = l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"][l_ContribId]["Files"]

                                        # For each file...
                                        for l_File in l_Files:
                                            if "ReadCount" in l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"][l_ContribId]["Files"][l_File]:
                                                l_ReadTime = (l_ContribId, l_File,
                                                              (l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"][l_ContribId]["Files"][l_File]["ReadCount"],
                                                               l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"][l_ContribId]["Files"][l_File]["ReadTime"]))
                                                if l_ReadTimes == [None,None]:
                                                    l_ReadTimes = [l_ReadTime,l_ReadTime]
                                                else:
                                                    if (l_ReadTime[2][1] < l_ReadTimes[0][2][1]):
                                                        l_ReadTimes[0] = l_ReadTime
                                                    if (l_ReadTime[2][1] > l_ReadTimes[1][2][1]):
                                                        l_ReadTimes[1] = l_ReadTime
                                                l_WriteTime = (l_ContribId, l_File,
                                                               (l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"][l_ContribId]["Files"][l_File]["WriteCount"],
                                                                l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"][l_ContribId]["Files"][l_File]["WriteTime"]))
                                                if l_WriteTimes == [None,None]:
                                                    l_WriteTimes = [l_WriteTime,l_WriteTime]
                                                else:
                                                    if (l_WriteTime[2][1] < l_WriteTimes[0][2][1]):
                                                        l_WriteTimes[0] = l_WriteTime
                                                    if (l_WriteTime[2][1] > l_WriteTimes[1][2][1]):
                                                        l_WriteTimes[1] = l_WriteTime
                                                l_TransferTypes.add(l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"][l_ContribId]["Files"][l_File]["TransferType"])

                                                # NOTE:  The output will associate the sync count/time with the source file, when in fact that data is for the corresponding target files.
                                                if l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"][l_ContribId]["Files"][l_File]["SyncCount"] != None:
                                                    l_SyncTime = (l_ContribId, l_File,
                                                                  (l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"][l_ContribId]["Files"][l_File]["SyncCount"],
                                                                   l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"][l_ContribId]["Files"][l_File]["SyncTime"]))
                                                    if l_SyncTimes == [None, None]:
                                                        l_SyncTimes = [l_SyncTime, l_SyncTime]
                                                    else:
                                                        if (l_SyncTime[2][1] < l_SyncTimes[0][2][1]):
                                                            l_SyncTimes[0] = l_SyncTime
                                                        if (l_SyncTime[2][1] > l_SyncTimes[1][2][1]):
                                                            l_SyncTimes[1] = l_SyncTime
                                    # End of files...
                                    l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["connections"][l_Connection]["contribIds"]["readTimes (File Min/Max)"] = l_ReadTimes
                                    l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["connections"][l_Connection]["contribIds"]["writeTimes (File Min/Max)"] = l_WriteTimes
                                    l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["connections"][l_Connection]["contribIds"]["syncTimes (File Min/Max)"] = l_SyncTimes
                                    l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["connections"][l_Connection]["contribIds"]["transferTypes"] = [l_TransferType for l_TransferType in l_TransferTypes]
                                # End of contribids...
                                if l_NotSuccessfulContribIds:
                                    if "NotSuccessfulContribIds" not in l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["connections"][l_Connection]["contribIds"]:
                                        l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["connections"][l_Connection]["contribIds"]["NotSuccessfulContribIds"] = []
                                    l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["connections"][l_Connection]["contribIds"]["NotSuccessfulContribIds"] = l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["connections"][l_Connection]["contribIds"]["NotSuccessfulContribIds"] + l_NotSuccessfulContribIds
                                l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["connections"][l_Connection]["contribIds"]["NumberOfContribIds"] = l_NumberOfContribIds
                                l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["connections"][l_Connection]["contribIds"]["processingTimes (File Min/Max)"] = l_ProcessingTimes
                        # End of connections...
                        l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["NumberOfConnections"] = l_NumberOfConnections
                        if l_SizeTransferred:
                            l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["SizeTransferred"] = l_SizeTransferred
                # End of handles...
                if l_NotSuccessfulHandles:
                    l_Output[l_JobId][l_Server][l_JobStepId]["NotSuccessfulHandles"] = l_Output[l_JobId][l_Server][l_JobStepId]["NotSuccessfulHandles"] + l_NotSuccessfulHandles
                l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["NumberOfHandles"] = l_NumberOfHandles
                l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["ProcessingTimes (ContribId Min/Max)"] = l_HandleProcessingTimes
            l_Output[l_JobId][l_Server]["NumberOfConnectionsForServer"] = l_NumberOfConnectionsForServer
            l_Output[l_JobId][l_Server]["NumberOfHandlesForServer"] = l_NumberOfHandlesForServer
        # End of servers...
        l_Output[l_JobId]["NumberOfConnectionsForAllServers"] = l_NumberOfConnectionsForAllServers
        l_Output[l_JobId]["NumberOfHandlesForAllServers"] = l_NumberOfHandlesForAllServers

        # Calculate the min/max processing times for all contribids, across all servers
        l_ServerProcessingTimes = [None, None]
        for l_Server in l_Output[l_JobId].keys():
            if type(l_Output[l_JobId][l_Server]) == dict:
                for l_JobStepId in l_Output[l_JobId][l_Server].keys():
                    if type(l_Output[l_JobId][l_Server][l_JobStepId]) == dict:
                        if l_ServerProcessingTimes == [None, None]:
                            l_ServerProcessingTimes = l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["ProcessingTimes (ContribId Min/Max)"]
                        else:
                            if (l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["ProcessingTimes (ContribId Min/Max)"][0][1] < l_ServerProcessingTimes[0][1]):
                                l_ServerProcessingTimes[0] = l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["ProcessingTimes (ContribId Min/Max)"][0]
                            if (l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["ProcessingTimes (ContribId Min/Max)"][1][1] > l_ServerProcessingTimes[1][1]):
                                l_ServerProcessingTimes[1] = l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["ProcessingTimes (ContribId Min/Max)"][1]
        l_Output[l_JobId]["ProcessingTimes (All Servers ContribId Min/Max)"] = l_ServerProcessingTimes

        # Format data/add average size transferred per handle data
        for l_JobId in l_Output:
            for l_Server in l_Output[l_JobId]:
                if type(l_Output[l_JobId][l_Server]) == dict:
                    for l_JobStepId in l_Output[l_JobId][l_Server]:
                        # NOTE: Not every l_Server element is a server name.
                        #       We only want to process those with a "Handles" key.
                        try:
                            if "SizeTransferred" in l_Output[l_JobId][l_Server][l_JobStepId]["handles"]:
                                if l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["SizeTransferred"]:
                                    l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["Avg SizeTransferred/Handle"] = cmn.numericFormat(round(float(l_Output[l_JobId][l_Server]["handles"]["SizeTransferred"])/float(l_Output[l_JobId][l_Server]["handles"]["NumberOfHandles"]),3))
                                    l_Output[l_JobId][l_Server][l_JobStepId]["handles"]["SizeTransferred"] = cmn.numericFormat(l_Output[l_JobId][l_Server]["handles"]["SizeTransferred"])
                        except Exception:
                            pass

        # Output the results
#        pprint.pprint(l_Output)

        cmn.ensure(os.path.join(l_Ctx["ROOTDIR"], "Analysis", `l_JobId`))
        l_PathFileName = os.path.join(l_Ctx["ROOTDIR"], "Analysis", `l_JobId`, "Details.txt")
        cmn.printFormattedFile(l_Ctx, l_PathFileName, l_Output)
        l_PathFileName = os.path.join(l_Ctx["ROOTDIR"], "Analysis", `l_JobId`, "Details.json")
        cmn.printFormattedFileAsJson(l_Ctx, l_PathFileName, l_Output)

    return

if __name__ == '__main__':
    main(sys.argv)