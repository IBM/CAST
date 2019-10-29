#!/usr/bin/python
###########################################################
#     performAllAnalysis.py
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

import sys

import common as cmn
from basicAnalysis import performBasicAnalysis
from generateDiskStatsListing import generateDiskStatsListing
from generateErrorsListing import generateErrorsListing
from generateTransferRates import calculateTransferRates
from generateWorkQueueMgrDumps import generateWorkQueueMgrDumps


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

    # Perform all analysis
    performBasicAnalysis(l_Ctx)
    generateDiskStatsListing(l_Ctx)
    generateErrorsListing(l_Ctx)
    calculateTransferRates(l_Ctx)
    generateWorkQueueMgrDumps(l_Ctx)

    return


if __name__ == '__main__':
    main(sys.argv)
