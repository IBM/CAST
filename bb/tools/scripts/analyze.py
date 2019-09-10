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
import pprint
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

    # Print the results
    if l_Ctx["PRINT_PICKLED_RESULTS"]:
        cmn.printFormattedData(l_Ctx, l_Data)

    # Determine list of servers
    l_TestOutput.append("List of Servers%s" % (os.linesep))
    l_Servers = cmn.getServers(l_Data)
    l_TestOutput.append("%d servers, %s%s" % (len(l_Servers), `l_Servers`, os.linesep))
    l_TestOutput.append("%s" % (os.linesep))

    # Determine list of jobids
    l_TestOutput.append("List of JobIds%s" % (os.linesep))
    l_JobIds = cmn.getJobIds(l_Data)
    l_TestOutput.append("%d jobids, %s%s" % (len(l_JobIds), `l_JobIds`, os.linesep))
    l_TestOutput.append("%s" % (os.linesep))

    # Determine handles per server
    l_TestOutput.append("List of Handles per Server%s" % (os.linesep))
    for l_Server in l_Servers:
        l_Handles = cmn.getHandlesForServer(l_Data, l_Server)
        l_TestOutput.append("For server %s, %d handles, %s%s" % (l_Server, len(l_Handles), `l_Handles`, os.linesep))
    l_TestOutput.append("%s" % (os.linesep))

    # Determine jobids per server
    l_TestOutput.append("List of JobIds per Server%s" % (os.linesep))
    for l_Server in l_Servers:
        l_JobIds = cmn.getJobIdsForServer(l_Data, l_Server)
        l_TestOutput.append("For server %s, %d jobids, %s%s" % (l_Server, len(l_JobIds), `l_JobIds`, os.linesep))
    l_TestOutput.append("%s" % (os.linesep))

    # Determinne servers per jobid
    l_TestOutput.append("List of Servers per JobId%s" % (os.linesep))
    l_JobIds = cmn.getJobIds(l_Data)
    for l_JobId in l_JobIds:
        l_Servers = cmn.getServersForJobid(l_Data, l_JobId)
        l_TestOutput.append("For jobid %d, %d servers, %s%s" % (l_JobId, len(l_Servers), `l_Servers`, os.linesep))
    l_TestOutput.append("%s" % (os.linesep))

    # Write out the basic results
    l_PathFileName = os.path.join(l_Ctx["ROOTDIR"], "Analysis", "BasicData.txt")
    cmn.writeOutput(l_Ctx, l_PathFileName, l_TestOutput)
    print "Basic results written to %s" % l_PathFileName
    print

    # Start detailed analysis per jobid, per server
    #
    # For each jobid...
    l_JobIds = cmn.getJobIds(l_Data)
    for l_JobId in l_JobIds:
        l_Output = {}
        l_Output[l_JobId] = {}
        l_NumberOfHandlesForAllServers = 0

        # For each server...
        l_Servers = cmn.getServersForJobid(l_Data, l_JobId)
        for l_Server in l_Servers:
            l_Output[l_JobId][l_Server] = {}
            l_Output[l_JobId][l_Server]["Handles"] = {}
            l_Output[l_JobId][l_Server]["Handles"]["NumberOfConnections"] = 0
            l_Output[l_JobId][l_Server]["Handles"]["SizeTransferred"] = 0
            l_Output[l_JobId][l_Server]["NotSuccessfulHandles"] = []
            l_Output[l_JobId][l_Server]["Handles"]["Connections"] = {}

            # For each handle...
            l_Handles = cmn.getHandlesForServer(l_Data, l_Server)
            l_NumberOfHandles = 0
            l_HandleProcessingTimes = [None,None]
            l_NotSuccessfulHandles = []
            for l_Handle in l_Handles:
                if l_Data[l_Server]["Handles"][l_Handle]["JobId"] == l_JobId:

                    # JobId matches...
                    l_NumberOfHandles = l_NumberOfHandles + 1
                    l_NumberOfHandlesForAllServers = l_NumberOfHandlesForAllServers + 1
                    if l_Data[l_Server]["Handles"][l_Handle]["Status"] != "BBFULLSUCCESS":
                        l_NotSuccessfulHandles.append((l_Handle, l_Data[l_Server]["Handles"][l_Handle]["Status"]))
                    l_NumberOfConnections = l_Output[l_JobId][l_Server]["Handles"]["NumberOfConnections"]
                    l_SizeTransferred = l_Output[l_JobId][l_Server]["Handles"]["SizeTransferred"]

                    # For each connection...
                    l_Connections = l_Data[l_Server]["Handles"][l_Handle]["Connections"]
                    for l_Connection in l_Connections:
                        if l_Connection not in l_Output[l_JobId][l_Server]["Handles"]["Connections"]:
                            l_NumberOfConnections = l_NumberOfConnections + 1
                            l_Output[l_JobId][l_Server]["Handles"]["Connections"][l_Connection] = {}
                            l_Output[l_JobId][l_Server]["Handles"]["Connections"][l_Connection]["ContribIds"] = {}
                            l_Output[l_JobId][l_Server]["Handles"]["Connections"][l_Connection]["ContribIds"]["NumberOfContribIds"] = 0
                            l_Output[l_JobId][l_Server]["Handles"]["Connections"][l_Connection]["ContribIds"]["ProcessingTimes (File Min/Max)"] = [None,None]
                            l_Output[l_JobId][l_Server]["Handles"]["Connections"][l_Connection]["ContribIds"]["ReadTimes (File Min/Max)"] = [None,None]
                            l_Output[l_JobId][l_Server]["Handles"]["Connections"][l_Connection]["ContribIds"]["WriteTimes (File Min/Max)"] = [None,None]

                        # For each LVUuid...
                        l_LVUuids = l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"]
                        for l_LVUuid in l_LVUuids:

                            # For each ContribId...
                            l_ContribIds = l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"]
                            l_ProcessingTimes = l_Output[l_JobId][l_Server]["Handles"]["Connections"][l_Connection]["ContribIds"]["ProcessingTimes (File Min/Max)"]
                            l_NumberOfContribIds = l_Output[l_JobId][l_Server]["Handles"]["Connections"][l_Connection]["ContribIds"]["NumberOfContribIds"]
                            l_NotSuccessfulContribIds = []
                            for l_ContribId in l_ContribIds:
                                l_NumberOfContribIds = l_NumberOfContribIds + 1
                                if l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"][l_ContribId]["Status"] != "BBFULLSUCCESS":
                                    l_NotSuccessfulContribIds.append((l_ContribId, l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"][l_ContribId]["Status"]))
                                if "SizeTransferred" in l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"][l_ContribId]:
                                    l_SizeTransferred += l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"][l_ContribId]["SizeTransferred"]
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

                                l_ReadTimes = l_Output[l_JobId][l_Server]["Handles"]["Connections"][l_Connection]["ContribIds"]["ReadTimes (File Min/Max)"]
                                l_WriteTimes = l_Output[l_JobId][l_Server]["Handles"]["Connections"][l_Connection]["ContribIds"]["WriteTimes (File Min/Max)"]
                                if "Files" in l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"][l_ContribId]:
                                    l_Files = l_Data[l_Server]["Handles"][l_Handle]["Connections"][l_Connection]["LVUuids"][l_LVUuid]["ContribIds"][l_ContribId]["Files"]

                                    # For each file...
                                    for l_File in l_Files:
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
                                # End of files...
                                l_Output[l_JobId][l_Server]["Handles"]["Connections"][l_Connection]["ContribIds"]["ReadTimes (File Min/Max)"] = l_ReadTimes
                                l_Output[l_JobId][l_Server]["Handles"]["Connections"][l_Connection]["ContribIds"]["WriteTimes (File Min/Max)"] = l_WriteTimes
                            # End of contribids...
                            if l_NotSuccessfulContribIds:
                                if "NotSuccessfulContribIds" not in l_Output[l_JobId][l_Server]["Handles"]["Connections"][l_Connection]["ContribIds"]:
                                    l_Output[l_JobId][l_Server]["Handles"]["Connections"][l_Connection]["ContribIds"]["NotSuccessfulContribIds"]
                                l_Output[l_JobId][l_Server]["Handles"]["Connections"][l_Connection]["ContribIds"]["NotSuccessfulContribIds"] = l_Output[l_JobId][l_Server]["Handles"]["Connections"][l_Connection]["ContribIds"]["NotSuccessfulContribIds"] + l_NotSuccessfulContribIds
                            l_Output[l_JobId][l_Server]["Handles"]["Connections"][l_Connection]["ContribIds"]["NumberOfContribIds"] = l_NumberOfContribIds
                            l_Output[l_JobId][l_Server]["Handles"]["Connections"][l_Connection]["ContribIds"]["ProcessingTimes (File Min/Max)"] = l_ProcessingTimes
                    # End of connections...
                    l_Output[l_JobId][l_Server]["Handles"]["NumberOfConnections"] = l_NumberOfConnections
                    if l_SizeTransferred:
                        l_Output[l_JobId][l_Server]["Handles"]["SizeTransferred"] = l_SizeTransferred
            # End of handles...
            if not l_Output[l_JobId][l_Server]["Handles"]["SizeTransferred"]:
                del l_Output[l_JobId][l_Server]["Handles"]["SizeTransferred"]
            if l_NotSuccessfulHandles:
                l_Output[l_JobId][l_Server]["NotSuccessfulHandles"] = l_Output[l_JobId][l_Server]["NotSuccessfulHandles"] + l_NotSuccessfulHandles
            l_Output[l_JobId][l_Server]["Handles"]["NumberOfHandles"] = l_NumberOfHandles
            l_Output[l_JobId][l_Server]["Handles"]["ProcessingTimes (ContribId Min/Max)"] = l_HandleProcessingTimes
        # End of servers...
        l_Output[l_JobId]["NumberOfHandlesForAllServers"] = l_NumberOfHandlesForAllServers

        # Calculate the min/max processing times for all contribids, across all servers
        l_ServerProcessingTimes = [None, None]
        for l_Server in l_Output[l_JobId].keys():
            if type(l_Output[l_JobId][l_Server]) == dict:
                if l_ServerProcessingTimes == [None, None]:
                    l_ServerProcessingTimes = l_Output[l_JobId][l_Server]["Handles"]["ProcessingTimes (ContribId Min/Max)"]
                else:
                    if (l_Output[l_JobId][l_Server]["Handles"]["ProcessingTimes (ContribId Min/Max)"][0][1] < l_ServerProcessingTimes[0][1]):
                        l_ServerProcessingTimes[0] = l_Output[l_JobId][l_Server]["Handles"]["ProcessingTimes (ContribId Min/Max)"][0]
                    if (l_Output[l_JobId][l_Server]["Handles"]["ProcessingTimes (ContribId Min/Max)"][1][1] > l_ServerProcessingTimes[1][1]):
                        l_ServerProcessingTimes[1] = l_Output[l_JobId][l_Server]["Handles"]["ProcessingTimes (ContribId Min/Max)"][1]
        l_Output[l_JobId]["ProcessingTimes (All Servers ContribId Min/Max)"] = l_ServerProcessingTimes

        # Output the results
#        pprint.pprint(l_Output)

        cmn.ensure(os.path.join(l_Ctx["ROOTDIR"], "Analysis", `l_JobId`))
        l_PathFileName = os.path.join(l_Ctx["ROOTDIR"], "Analysis", `l_JobId`, "Details.txt")
        cmn.printFormattedFile(l_Ctx, l_PathFileName, l_Output)

    return

if __name__ == '__main__':
    main(sys.argv)