#!/usr/bin/python
###########################################################
#     performBSCFS_TranferRates.py
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


def performBSCFS_Analysis(pCtx):
    print("%sStart: BSCFS transfer rate calculations..." % (os.linesep))

    if "BSCFS" in pCtx:
        for l_JobId in pCtx["BSCFS"]:
            for l_JobStepId in pCtx["BSCFS"][l_JobId]:
                for l_Server in pCtx["BSCFS"][l_JobId][l_JobStepId]:
                    for l_Connection in pCtx["BSCFS"][l_JobId][l_JobStepId][l_Server]:
                        l_Order = 1
                        l_Handles = pCtx["BSCFS"][l_JobId][l_JobStepId][l_Server][l_Connection]
                        l_AllDone = False
                        while not l_AllDone:
                            l_StartTime = [None, None]
                            for l_Handle in l_Handles.keys():
                                if l_Handles[l_Handle][0] == 0:
                                    if l_StartTime[1] is not None:
                                        if (cmn.compareTimes(l_StartTime, (l_Handle, l_Handles[l_Handle][3])) == 1):
                                            l_StartTime = [l_Handle, l_Handles[l_Handle][3]]
                                    else:
                                        l_StartTime = [l_Handle, l_Handles[l_Handle][3]]
                            if l_StartTime[1] is not None:
                                l_Handles[l_StartTime[0]][0] = l_Order
                                l_Order += 1
                            else:
                                l_AllDone = True   
                
                pCtx["BSCFS_TransferRates"] = {}
                pCtx["BSCFS_TransferRates"][0] = ("BSCFS Iteration:", "(StartTime", "EndTime", "TotalTransferSize", "TransferRate (GB/sec))")

                l_Order = 1
    
                l_AllDone = False
                while not l_AllDone:
                    l_AllDone = True
                    pCtx["BSCFS_TransferRates"][l_Order] = [None, None, 0, 0.0]
                    for l_Server in pCtx["BSCFS"][l_JobId][l_JobStepId]:
                        for l_Connection in pCtx["BSCFS"][l_JobId][l_JobStepId][l_Server]:
                            l_Handles = pCtx["BSCFS"][l_JobId][l_JobStepId][l_Server][l_Connection]
                            for l_Handle in l_Handles.keys():
                                if l_Handles[l_Handle][0] == l_Order:
                                    if pCtx["BSCFS_TransferRates"][l_Order][0] is not None:
                                        if (cmn.compareTimes((None, pCtx["BSCFS_TransferRates"][l_Order][0]), (None, l_Handles[l_Handle][3])) == 1):
                                            pCtx["BSCFS_TransferRates"][l_Order][0] = l_Handles[l_Handle][3]
                                    else:
                                        pCtx["BSCFS_TransferRates"][l_Order][0] = l_Handles[l_Handle][3]
                                    if pCtx["BSCFS_TransferRates"][l_Order][1] is not None:
                                        if (cmn.compareTimes((None, pCtx["BSCFS_TransferRates"][l_Order][1]), (None, l_Handles[l_Handle][4])) == -1):
                                            pCtx["BSCFS_TransferRates"][l_Order][1] = l_Handles[l_Handle][4]
                                    else:
                                        pCtx["BSCFS_TransferRates"][l_Order][1] = l_Handles[l_Handle][4]
                                    pCtx["BSCFS_TransferRates"][l_Order][2] += l_Handles[l_Handle][5]
                                    l_AllDone = False
                                    break
                    if l_AllDone == False:
                        l_ElapsedTime = float(cmn.calculateTimeDifferenceInSeconds((None, pCtx["BSCFS_TransferRates"][l_Order][1]), (None, pCtx["BSCFS_TransferRates"][l_Order][0])))
                        pCtx["BSCFS_TransferRates"][l_Order][3] = round((float(pCtx["BSCFS_TransferRates"][l_Order][2]) / float(l_ElapsedTime)) / float(10**9),6)
                        l_Order += 1

    # Remove the last, as that has no data
    if "BSCFS_TransferRates" in pCtx:
        pCtx["BSCFS_TransferRates"].pop(max(pCtx["BSCFS_TransferRates"].keys()), None)

        if len(pCtx["BSCFS_TransferRates"].keys()) > 0:
            # Print the results to a file
            l_PathFileName = os.path.join(pCtx["ROOTDIR"], "Analysis", "BSCFS_TransferRates.txt")
            cmn.printFormattedFile(pCtx, l_PathFileName, pCtx["BSCFS_TransferRates"])
            l_PathFileName = os.path.join(pCtx["ROOTDIR"], "Analysis", "BSCFS_TransferRates.json")
            cmn.printFormattedFileAsJson(pCtx, l_PathFileName, pCtx["BSCFS_TransferRates"])
        else:
            print("       No BSCFS transfers found...")  
    else:
        print("       No BSCFS transfers found...")  

    print("%s  End: BSCFS transfer rate calculations..." % (os.linesep))

    return



# Main routine
def main(*pArgs):
    l_Ctx = {}     # Environmental context

    # Establish the context
    cmn.getOptions(l_Ctx, pArgs[0])

    # Load the data as a pickle file from the input root directory
    cmn.loadData(l_Ctx)
    l_Data = l_Ctx["BSCFS"]

    # Optionally, print the results
    if l_Ctx["PRINT_PICKLED_RESULTS"]:
        cmn.printFormattedData(l_Ctx, l_Data)

    # Perform basic analysis
    performBSCFS_Analysis(l_Ctx)

    return


if __name__ == '__main__':
    main(sys.argv)