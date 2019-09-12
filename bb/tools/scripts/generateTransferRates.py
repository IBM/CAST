#!/usr/bin/python
###########################################################
#     generateTransferRates.py
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

# Calculate transfer rates for the jobids/servers/connections
def calculateTransferRates(pCtx):
    print "Start: Transfer rate calculations..."
    if "JobIds" in pCtx["ElapsedTimeData"]:
        for l_JobId in pCtx["ElapsedTimeData"]["JobIds"]:
            l_JobIdEntry = pCtx["ElapsedTimeData"]["JobIds"][l_JobId]
            l_ElapsedTime = float(cmn.calculateTimeDifferenceInSeconds(l_JobIdEntry["EndDateTime"], l_JobIdEntry["StartDateTime"]))
            l_JobIdEntry["ElapsedTime (secs)"] = l_ElapsedTime
            if l_ElapsedTime:
                l_JobIdEntry["JobId TransferRate (GB/sec)"] = round((float(l_JobIdEntry["SizeTransferred"]) / float(l_ElapsedTime)) / float(10**9),3)
            else:
                l_JobIdEntry["JobId TransferRate (GB/sec)"] = None
            l_JobIdEntry["SizeTransferred"] = cmn.numericFormat(l_JobIdEntry["SizeTransferred"])
            if "Servers" in pCtx["ElapsedTimeData"]["JobIds"][l_JobId]:
                for l_Server in pCtx["ElapsedTimeData"]["JobIds"][l_JobId]["Servers"]:
                    l_ServerEntry = pCtx["ElapsedTimeData"]["JobIds"][l_JobId]["Servers"][l_Server]
                    l_ElapsedTime = float(cmn.calculateTimeDifferenceInSeconds(l_ServerEntry["EndDateTime"], l_ServerEntry["StartDateTime"]))
                    l_ServerEntry["ElapsedTime (secs)"] = l_ElapsedTime
                    if l_ElapsedTime:
                        l_ServerEntry["Server TransferRate (GB/sec)"] = round((float(l_ServerEntry["SizeTransferred"]) / float(l_ElapsedTime)) / float(10 ** 9),3)
                    else:
                        l_ServerEntry["Server TransferRate (GB/sec)"] = None
                    l_ServerEntry["SizeTransferred"] = cmn.numericFormat(l_ServerEntry["SizeTransferred"])
                    if "Connections" in pCtx["ElapsedTimeData"]["JobIds"][l_JobId]["Servers"][l_Server]:
                        for l_Connection in pCtx["ElapsedTimeData"]["JobIds"][l_JobId]["Servers"][l_Server]["Connections"]:
                            l_ConnectionEntry = pCtx["ElapsedTimeData"]["JobIds"][l_JobId]["Servers"][l_Server]["Connections"][l_Connection]
                            l_ElapsedTime = float(cmn.calculateTimeDifferenceInSeconds(l_ConnectionEntry["EndDateTime"], l_ConnectionEntry["StartDateTime"]))
                            l_ConnectionEntry["ElapsedTime (secs)"] = l_ElapsedTime
                            if l_ElapsedTime:
                                l_ConnectionEntry["Connection TransferRate (GB/sec)"] = round((float(l_ConnectionEntry["SizeTransferred"]) / float(l_ElapsedTime)) / float(10 ** 9),3)
                            else:
                                l_ConnectionEntry["Connection TransferRate (GB/sec)"] = None
                            l_ConnectionEntry["SizeTransferred"] = cmn.numericFormat(l_ConnectionEntry["SizeTransferred"])
    print "  End: Transfer rate calculations..."
    print

    return

# Main routine
def main(*pArgs):
    l_Ctx = {}     # Environmental context

    # Establish the context
    cmn.getOptions(l_Ctx, pArgs[0])

    # Load the data as a pickle file from the input root directory
    cmn.loadData(l_Ctx)
    l_Data = l_Ctx["ElapsedTimeData"]

    if l_Ctx["PRINT_PICKLED_RESULTS"]:
        cmn.printFormattedData(l_Ctx, l_Data)

    # Calculate the transfer rates
    calculateTransferRates(l_Ctx)

    # Print the results to a file
    l_PathFileName = os.path.join(l_Ctx["ROOTDIR"], "Analysis", "TransferRates.txt")
    cmn.printFormattedFile(l_Ctx, l_PathFileName, l_Data)

    return

if __name__ == '__main__':
    main(sys.argv)