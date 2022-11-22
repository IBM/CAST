#!/usr/bin/python
####################################################
#    RunProcedure.py
#
#    Copyright IBM Corporation 2017,2017. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
###################################################

"""
    Common Burst Buffer Procedures

    Example invocations with bluecoral as your current working directory:
        python ./bb/wrappers/bbapi_main.py --libpath ./work/bb/lib --testpath ./bb/wrappers
                                           --testcase RunProcedure --procedure CancelTransfers --jobid 20 --jobstepid 0

        python ./bb/wrappers/bbapi_main.py --libpath ./work/bb/lib --testpath ./bb/wrappers
                                           --testcase RunProcedure --procedure ListTransfers --jobid 20 --jobstepid 0

        python ./bb/wrappers/bbapi_main.py --libpath ./work/bb/lib --testpath ./bb/wrappers
                                           --testcase RunProcedure --procedure ListHandles --jobid 20 --jobstepid 0

        python ./bb/wrappers/bbapi_main.py --libpath ./work/bb/lib --testpath ./bb/wrappers
                                           --testcase RunProcedure --procedure ListTransfers --jobid 20 --jobstepid 0

        python ./bb/wrappers/bbapi_main.py --libpath ./work/bb/lib --testpath ./bb/wrappers
                                           --testcase RunProcedure --procedure Resume

        NOTE:  The following does a retrieve, followed by a stop, and then a restart
        python ./bb/wrappers/bbapi_main.py --libpath ./work/bb/lib --testpath ./bb/wrappers
                                           --testcase RunProcedure --procedure RestartTransfers --jobid 20 --jobstepid 0

        python ./bb/wrappers/bbapi_main.py --libpath ./work/bb/lib --testpath ./bb/wrappers
                                           --testcase RunProcedure --procedure RetrieveTransfers --jobid 20 --jobstepid 0

        NOTE:  The following does a retrieve, followed by a stop
        python ./bb/wrappers/bbapi_main.py --libpath ./work/bb/lib --testpath ./bb/wrappers
                                           --testcase RunProcedure --procedure StopTransfers --jobid 20 --jobstepid 0

        python ./bb/wrappers/bbapi_main.py --libpath ./work/bb/lib --testpath ./bb/wrappers
                                           --testcase RunProcedure --procedure Suspend
"""

import sys

import bb
import Procedures as procs

from bberror import *
from bbapi import *
from bbapiAdmin import *

def main(pEnv):
    rc = 0;

    if "procedure" in pEnv:
        try:
            rc = getattr(procs, pEnv["procedure"])(pEnv)
            print("%sProcedure -> %s, rc = %d" % (os.linesep, os.path.splitext(os.path.basename(pEnv["procedure"]))[0], rc))
        except:
            rc = -1
            print("When attempting to invoke procedure %s, unexpected error: %s" % (pEnv["procedure"], sys.exc_info()[0]))
    else:
        rc = -1
        print("Option -w or --procedure must be specified")

    return rc


#
# Invoke main routine
#

if __name__ == '__main__':
    main(sys.argv[1])
