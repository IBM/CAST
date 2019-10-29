#!/usr/bin/python
###########################################################
#     generateDiskStatsListing.py
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


AVG_CPU = re.compile("avg-cpu")

def generateDiskStatsListing(pCtx):
    print("%sStart: Generate disk stats listing..." % (os.linesep))

    l_Output = []
    l_Data = pCtx["ServerData"]
    l_Servers = cmn.getServers(l_Data)
    for l_Server in l_Servers:
        l_Output.append("Disk stats for Server %s%s" % (l_Server, os.linesep))
        l_DiskStats = cmn.getDiskStatsForServer(l_Data, l_Server)
        l_TimeStamps = l_DiskStats.keys()
        l_TimeStamps.sort()
        l_NumberOfLeadingLineFeeds = 0
        for l_TimeStamp in l_TimeStamps:
            l_OptionalLineSep = ""
            l_Success = AVG_CPU.search(l_DiskStats[l_TimeStamp])
            if l_Success:
                if l_NumberOfLeadingLineFeeds != 0:
                    l_OptionalLineSep = os.linesep
                l_NumberOfLeadingLineFeeds += 1
            l_Output.append("%s%s:  %s%s" % (l_OptionalLineSep, l_TimeStamp, l_DiskStats[l_TimeStamp], os.linesep))
        l_Output.append(os.linesep)
    print

    # Output the results
    l_PathFileName = os.path.join(pCtx["ROOTDIR"], "Analysis", "DiskStats.txt")
    cmn.writeOutput(pCtx, l_PathFileName, l_Output)
    print "Results written to %s" % l_PathFileName

    print("%s  End: Generate disk stats listing..." % (os.linesep))

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

    # Generate disk stats listing
    generateDiskStatsListing(l_Ctx)

    return


if __name__ == '__main__':
    main(sys.argv)
