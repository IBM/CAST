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

    # Print the results to a file
    l_PathFileName = os.path.join(l_Ctx["ROOTDIR"], "Analysis", "TransferRates.txt")
    cmn.printFormattedFile(l_Ctx, l_PathFileName, l_Data)

    return

if __name__ == '__main__':
    main(sys.argv)