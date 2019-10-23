#!/usr/bin/python
###########################################################
#     generateWorkQueueMgrDumps.py
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

import re

import common as cmn


JOB = re.compile(",\s+Job\s+(\d+),")
INFLIGHT = re.compile("#InFlt\s+(\d+)")
ACT_QUEUE = re.compile("LVKey\([a-z0-9.]+\s+\([0-9.]+\),[a-z0-9-]+\),\s+Job\s+\d+")
CURRENT_SIZE = re.compile("CurSize\s+(\d+)")

def generateWorkQueueMgrDumps(pCtx):
    print("%sStart: Generate work queue manager dumps..." % (os.linesep))

    l_Data = pCtx["ServerData"]
    l_JobIds = cmn.getJobIds(l_Data)
    for l_JobId in l_JobIds:
        l_Output = []
        l_Output.append("JobId %d%s" % (l_JobId, os.linesep))
        l_Servers = cmn.getServers(l_Data)
        for l_Server in l_Servers:
            l_ServerNameWasOutput = False
            l_JobIds = cmn.getJobIdsForServer(l_Data, l_Server)
            if (l_JobId in l_JobIds):
                l_TimeStamps = cmn.getWrkQTimeStampsForServer(l_Data, l_Server)
                for l_TimeStamp in l_TimeStamps:
                    l_Continue = False
                    for l_Item in l_Data[l_Server]["WorkQueueMgr"][l_TimeStamp]:
                        l_Success = JOB.search(l_Item)
                        if l_Success:
                            if int(l_Success.group(1)) == l_JobId:
                                l_Continue = True
                                break
                    if l_Continue:
                        if not l_ServerNameWasOutput:
                            l_ServerNameWasOutput = True
                            l_Output.append("Server %s%s" % (l_Server, os.linesep))
                        l_NumberInFlight = 0
                        l_NumberActiveQueues = 0
                        for l_Item in l_Data[l_Server]["WorkQueueMgr"][l_TimeStamp]:
                            l_Success = INFLIGHT.search(l_Item)
                            if l_Success:
                                l_NumberInFlight += int(l_Success.group(1))
                            l_Success = ACT_QUEUE.search(l_Item)
                            if l_Success:
                                l_Success = CURRENT_SIZE.search(l_Item)
                                if l_Success:
                                    if int(l_Success.group(1)) != 0:
                                       l_NumberActiveQueues += 1
                            l_Output.append("%s%s" % (l_Item, os.linesep))
                        l_Output.append("%sNumber of Active Work Queues = %d, Total Number in Flight = %d%s%s" % (" "*4, l_NumberActiveQueues, l_NumberInFlight, os.linesep, os.linesep))
            if (not l_ServerNameWasOutput):
                l_Output.append("Server %s%s" % (l_Server, os.linesep))
                l_Output.append("No data%s%s" % (os.linesep, os.linesep))
        l_Output.append(os.linesep)

        # Output the results
        cmn.ensure(os.path.join(pCtx["OUTPUT_DIRECTORY"], `l_JobId`))
        l_PathFileName = os.path.join(pCtx["OUTPUT_DIRECTORY"], `l_JobId`, "WorkQueueMgrDumps.txt")
        cmn.writeOutput(pCtx, l_PathFileName, l_Output)

    print("%s  End: Generate work queue manager dumps..." % (os.linesep))

    return


# Main routine
def main(*pArgs):
    l_Ctx = {}     # Environmental context

    # Establish the context
    cmn.getOptions(l_Ctx, pArgs[0])

    # Load the data as a pickle file from the input root directory
    cmn.loadData(l_Ctx)
    l_Data = l_Ctx["ServerData"]

    # Optionally, print the results
    if l_Ctx["PRINT_PICKLED_RESULTS"]:
        cmn.printFormattedData(l_Ctx, l_Data)

    # Generate the work queue manager dumps listing
    generateWorkQueueMgrDumps(l_Ctx)

    return


if __name__ == '__main__':
    main(sys.argv)
