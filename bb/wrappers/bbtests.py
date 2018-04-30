#!/usr/bin/python
####################################################
#    bbtests.py
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
import time

"""
    Testcases for to bbapi functions.
"""

import collections
import ctypes
import os
import pprint
import sys
import time

import bb
from bberror import *
from bbapi import *
from bbapiAdminProcs import *
from bbapiTest import *


def BasicTransfers(pEnv):
    rc = 0
    l_FuncName = sys._getframe().f_code.co_name

    print "   >>>>> Start: %s.%s..." % (__name__, l_FuncName)

    l_Owner = pEnv.get("OWNER", None)
    l_Group = pEnv.get("GROUP", None)
    l_Mode = int(pEnv.get("MODE", 0755))
    l_LVSize = pEnv.get("SIZE", "1G")

    l_OrgSrc = pEnv["ORGSRC"]
    l_Mountpoint = pEnv["MOUNT"]
    l_Target = pEnv["TARGET"]

    l_FileAttrs = (("file1", 256*1024),
                   ("file2", 0),
                   ("file3", 1024*1024),
                   ("file4", 4*1024*1024),
                   ("file5", 16*1024*1024),
                  )
    l_SourceFiles = ((),
                     (os.path.join(l_OrgSrc, "file1"),),
                     (os.path.join(l_Mountpoint, "file1"),),
                     (os.path.join(l_Mountpoint, "file1b"),),
                     (os.path.join(l_OrgSrc, "file2"), os.path.join(l_OrgSrc, "file3"), os.path.join(l_OrgSrc, "file4"), os.path.join(l_OrgSrc, "file5"),),
                     (os.path.join(l_Mountpoint, "file1"), os.path.join(l_Mountpoint, "file2"), os.path.join(l_Mountpoint, "file3"), os.path.join(l_Mountpoint, "file4"), os.path.join(l_Mountpoint, "file5"),),
                    )
    l_TargetFiles = ((),
                     (os.path.join(l_Mountpoint, "file1"),),
                     (os.path.join(l_Mountpoint, "file1b"),),
                     (os.path.join(l_Target, "file1b"),),
                     (os.path.join(l_Mountpoint, "file2"), os.path.join(l_Mountpoint, "file3"), os.path.join(l_Mountpoint, "file4"), os.path.join(l_Mountpoint, "file5"),),
                     (os.path.join(l_Target, "file1"), os.path.join(l_Target, "file2"), os.path.join(l_Target, "file3"), os.path.join(l_Target, "file4"), os.path.join(l_Target, "file5"),),
                    )
    l_FileFlags = (("None",),
                   ("None",),
                   ("None",),
                   ("None",),
                   ("BBTestStageIn","BBTestStageIn","BBTestStageIn","BBTestStageIn",),
                   ("BBTestStageOut","BBTestStageOut","BBTestStageOut","BBTestStageOut","BBTestStageOut",),
                  )
    l_Tag = 0
    l_Contrib = pEnv["contrib"]
    l_NumHandles = 16

    # NOTE: Any non-zero return code that is returned from any of the BB_* api calls
    #       will cause an exception to be thrown and caught in the handler below.
    #       The return code is available as exception data...
    try:
        # Print out the environment to setup...
        bb.printEnv(pEnv)

        # Cleanup any prior running of this variation and setup to run this time...
        rc = bb.initEnv(pEnv, l_Mountpoint, l_Target)

        if (rc == 0):
            # Create the files to be transferred
            print "%sGenerating files with randfile..." % (os.linesep)
            for l_File, l_FileSize in (l_FileAttrs):
                bb.createRandomFile(pEnv, (os.path.join(l_OrgSrc, l_File)), l_FileSize)

            # Create the necessary directories...
            sudo_CreateDirectory(pEnv, l_Target)
            sudo_ChangeOwner(pEnv, l_Target, l_Owner, l_Group)
            sudo_ChangeMode(pEnv, l_Target, l_Mode)
            sudo_CreateDirectory(pEnv, l_Mountpoint)
            sudo_ChangeOwner(pEnv, l_Mountpoint, l_Owner, l_Group)
            sudo_ChangeMode(pEnv, l_Mountpoint, l_Mode)

            # This should fail because the logical volume has not yet been created.
            #
            # This error should be handled, as handleError() will tolerate all
            # exceptions where the rc is -2.  If rc is not -2, then the exception
            # is raised again to the outermost exception handler.
            #
            # The error is always printed out, with pertinent information taken
            # from BB_GetLastErrorDetails().
            #
            # NOTE: This is included here to show how to easily handle/tolerate
            #       bb exceptions...
            try:
                print "%sThe following BB_GetTransferHandle() is expected to fail with a rc=-2" % (os.linesep)
                l_Handle = BB_GetTransferHandle(l_Tag, len(l_Contrib), l_Contrib)
            except BBError as error:
                if not error.handleError():
                    raise

            # Create the logical volume for the mountpoint...
            sudo_CreateLogicalVolume(pEnv, l_Mountpoint, l_LVSize)

            # Run the testcase...
            for l_VarNum in xrange(0, len(l_SourceFiles)):
                print "      >>>>> Start: Variation %d..." % (l_VarNum)

                l_TransferDef = BB_CreateTransferDef()
                for l_FileNum in xrange(len(l_SourceFiles[l_VarNum])):
                    BB_AddFiles(l_TransferDef, l_SourceFiles[l_VarNum][l_FileNum], l_TargetFiles[l_VarNum][l_FileNum], BBFILEFLAGS[l_FileFlags[l_VarNum][l_FileNum]])

                l_Tag += 1
                l_Handle = BB_GetTransferHandle(l_Tag, len(l_Contrib), l_Contrib)
                BB_StartTransfer(l_TransferDef, l_Handle)

                if l_VarNum+1 == len(l_SourceFiles):
                    # Last variation, so we are doing the stageout processing.
                    # NOTE: This isn't required, but included for test purposes.
                    sudo_StageOutStart(pEnv, l_Mountpoint)

                l_Handles = bb.getHandles(BBSTATUS["BBALL"])
                l_AllFullSuccess = bb.waitForCompletion(pEnv, l_Handles)

                BB_FreeTransferDef(l_TransferDef)

                print "      >>>>>   End: Variation %d..." % (l_VarNum)

                if not l_AllFullSuccess:
                    raise BBError(rc=-1, text="Not all transfers had a status of BBFULLSUCCESS")

            bb.checkFiles(l_SourceFiles, l_TargetFiles)

            # Cleanup...

            sudo_RemoveLogicalVolume(pEnv, l_Mountpoint)
            sudo_RemoveDirectory(pEnv, l_Mountpoint)
            if (len(l_Contrib) == 1):
                sudo_RemoveJobInfo(pEnv)
            elif (pEnv["contribid"] == 0):
                time.sleep(5)
                sudo_RemoveJobInfo(pEnv)

    except BBError as error:
        rc = error.rc
        # NOTE: BB_GetLastErrorDetails() output is contained within the error object
        #       and pertinant information is printed out from that data...
        print `error`

    print "   >>>>>   End: %s.%s..." % (__name__, l_FuncName)

    return rc


def BasicTransfers2(pEnv):
    rc = 0
    l_FuncName = sys._getframe().f_code.co_name

    print "   >>>>> Start: %s.%s..." % (__name__, l_FuncName)

    l_Owner = pEnv.get("OWNER", None)
    l_Group = pEnv.get("GROUP", None)
    l_Mode = int(pEnv.get("MODE", 0755))
    l_LVSize = pEnv.get("SIZE", "1G")

    l_OrgSrc = pEnv["ORGSRC"]
    l_Mountpoint = pEnv["MOUNT"]
    l_Target = pEnv["TARGET"]

    l_FileAttrs = (("file1", 256*1024),
                   ("file2", 0),
                   ("file3", 1024*1024),
                   ("file4", 4*1024*1024),
                   ("file5", 16*1024*1024),
                  )
    l_SourceFiles = ((os.path.join(l_OrgSrc, "file1"),),
                     (os.path.join(l_Mountpoint, "file1"),),
                    )
    l_TargetFiles = ((os.path.join(l_Mountpoint, "file1"),),
                     (os.path.join(l_Target, "file1"),),
                    )
    l_FileFlags = (("None",),
                   ("None",),
                  )

    l_Tag = 0   # The first tag to use will be 1...
    l_Contrib = pEnv["contrib"]
    l_NumHandles = 16

    # NOTE: Any non-zero return code that is returned from any of the BB_* api calls
    #       will cause an exception to be thrown and caught in the handler below.
    #       The return code is available as exception data...
    try:
        # Print out the environment to setup...
        bb.printEnv(pEnv)

        # Cleanup any prior running of this variation and setup to run this time...
        rc = bb.initEnv(pEnv, None, None)

        if (rc == 0):
            # Create the files to be transferred
            print "%sGenerating files with randfile..." % (os.linesep)
            for l_File, l_FileSize in (l_FileAttrs):
                bb.createRandomFile(pEnv, (os.path.join(l_OrgSrc, l_File)), l_FileSize)

            # Remove/Create the necessary directories...
            sudo_RemoveDirectory(pEnv, l_Target)
            sudo_CreateDirectory(pEnv, l_Target)
            sudo_ChangeOwner(pEnv, l_Target, l_Owner, l_Group)
            sudo_ChangeMode(pEnv, l_Target, l_Mode)

            # Run the testcase...
            for l_VarNum in xrange(0, len(l_SourceFiles)):
                print "      >>>>> Start: Variation %d..." % (l_VarNum)

                l_TransferDef = BB_CreateTransferDef()
                for l_FileNum in xrange(len(l_SourceFiles[l_VarNum])):
                    BB_AddFiles(l_TransferDef, l_SourceFiles[l_VarNum][l_FileNum], l_TargetFiles[l_VarNum][l_FileNum], BBFILEFLAGS[l_FileFlags[l_VarNum][l_FileNum]])

                l_Tag += 1
                l_Handle = BB_GetTransferHandle(l_Tag, len(l_Contrib), l_Contrib)
                BB_StartTransfer(l_TransferDef, l_Handle)

                if l_VarNum+1 == len(l_SourceFiles):
                    # Last variation, so we are doing the stageout processing.
                    # NOTE: This isn't required, but included for test purposes.
                    sudo_StageOutStart(pEnv, l_Mountpoint)

                l_Handles = bb.getHandles(BBSTATUS["BBALL"])
                l_AllFullSuccess = bb.waitForCompletion(pEnv, l_Handles)

                BB_FreeTransferDef(l_TransferDef)

                print "      >>>>>   End: Variation %d..." % (l_VarNum)

                if not l_AllFullSuccess:
                    raise BBError(rc=-1, text="Not all transfers had a status of BBFULLSUCCESS")

            bb.checkFiles(l_SourceFiles, l_TargetFiles)

    except BBError as error:
        rc = error.rc
        # NOTE: BB_GetLastErrorDetails() output is contained within the error object
        #       and pertinant information is printed out from that data...
        print `error`

    print "   >>>>>   End: %s.%s..." % (__name__, l_FuncName)

    return rc


def BasicTransfers3(pEnv):
    rc = 0
    l_FuncName = sys._getframe().f_code.co_name

    print "   >>>>> Start: %s.%s..." % (__name__, l_FuncName)

    l_Owner = pEnv.get("OWNER", None)
    l_Group = pEnv.get("GROUP", None)
    l_Mode = int(pEnv.get("MODE", 0755))
    l_LVSize = "8G"

    l_OrgSrc = pEnv["ORGSRC"]
    l_Mountpoint = pEnv["MOUNT"]
    l_Target = pEnv["TARGET"]

    l_FileAttrs = (("file1", 256*1024),
                   ("file2", 0),
                   ("file3", 512*1024*1024),
                   ("file4", 768*1024*1024),
                   ("file5", 1024*1024*1024),
                  )
    l_SourceFiles = ((),
                     (os.path.join(l_OrgSrc, "file1"),),
                     (os.path.join(l_Mountpoint, "file1"),),
                     (os.path.join(l_Mountpoint, "file1b"),),
                     (os.path.join(l_OrgSrc, "file2"), os.path.join(l_OrgSrc, "file3"), os.path.join(l_OrgSrc, "file4"), os.path.join(l_OrgSrc, "file5"),),
                     (os.path.join(l_Mountpoint, "file1"), os.path.join(l_Mountpoint, "file2"), os.path.join(l_Mountpoint, "file3"), os.path.join(l_Mountpoint, "file4"), os.path.join(l_Mountpoint, "file5"),),
                    )
    l_TargetFiles = ((),
                     (os.path.join(l_Mountpoint, "file1"),),
                     (os.path.join(l_Mountpoint, "file1b"),),
                     (os.path.join(l_Target, "file1b"),),
                     (os.path.join(l_Mountpoint, "file2"), os.path.join(l_Mountpoint, "file3"), os.path.join(l_Mountpoint, "file4"), os.path.join(l_Mountpoint, "file5"),),
                     (os.path.join(l_Target, "file1"), os.path.join(l_Target, "file2"), os.path.join(l_Target, "file3"), os.path.join(l_Target, "file4"), os.path.join(l_Target, "file5"),),
                    )
    l_FileFlags = (("None",),
                   ("None",),
                   ("None",),
                   ("None",),
                   ("BBTestStageIn","BBTestStageIn","BBTestStageIn","BBTestStageIn",),
                   ("BBTestStageOut","BBTestStageOut","BBTestStageOut","BBTestStageOut","BBTestStageOut",),
                  )
    l_Tag = 0
    l_Contrib = pEnv["contrib"]
    l_NumHandles = 16

    # NOTE: Any non-zero return code that is returned from any of the BB_* api calls
    #       will cause an exception to be thrown and caught in the handler below.
    #       The return code is available as exception data...
    try:
        # Print out the environment to setup...
        bb.printEnv(pEnv)

        # Cleanup any prior running of this variation and setup to run this time...
        rc = bb.initEnv(pEnv, l_Mountpoint, l_Target)

        if (rc == 0):
            # Create the files to be transferred
            print "%sGenerating files with randfile..." % (os.linesep)
            for l_File, l_FileSize in (l_FileAttrs):
                bb.createRandomFile(pEnv, (os.path.join(l_OrgSrc, l_File)), l_FileSize)

            # Create the necessary directories...
            sudo_CreateDirectory(pEnv, l_Target)
            sudo_ChangeOwner(pEnv, l_Target, l_Owner, l_Group)
            sudo_ChangeMode(pEnv, l_Target, l_Mode)
            sudo_CreateDirectory(pEnv, l_Mountpoint)
            sudo_ChangeOwner(pEnv, l_Mountpoint, l_Owner, l_Group)
            sudo_ChangeMode(pEnv, l_Mountpoint, l_Mode)

            # This should fail because the logical volume has not yet been created.
            #
            # This error should be handled, as handleError() will tolerate all
            # exceptions where the rc is -2.  If rc is not -2, then the exception
            # is raised again to the outermost exception handler.
            #
            # The error is always printed out, with pertinent information taken
            # from BB_GetLastErrorDetails().
            #
            # NOTE: This is included here to show how to easily handle/tolerate
            #       bb exceptions...
            """
            try:
                print "%sThe following BB_GetTransferHandle() is expected to fail with a rc=-2" % (os.linesep)
                l_Handle = BB_GetTransferHandle(l_Tag, len(l_Contrib), l_Contrib)
            except BBError as error:
                if not error.handleError():
                    raise
            """

            for i in xrange(1,100):

                # Create the logical volume for the mountpoint...
                sudo_CreateLogicalVolume(pEnv, l_Mountpoint, l_LVSize)

                # Run the testcase...
                for l_VarNum in xrange(0, len(l_SourceFiles)):
                    print "      >>>>> Start: Variation %d..." % (l_VarNum)

                    l_TransferDef = BB_CreateTransferDef()
                    for l_FileNum in xrange(len(l_SourceFiles[l_VarNum])):
                        BB_AddFiles(l_TransferDef, l_SourceFiles[l_VarNum][l_FileNum], l_TargetFiles[l_VarNum][l_FileNum], BBFILEFLAGS[l_FileFlags[l_VarNum][l_FileNum]])

                    l_Tag += 1
                    l_Handle = BB_GetTransferHandle(l_Tag, len(l_Contrib), l_Contrib)
                    BB_StartTransfer(l_TransferDef, l_Handle)

                    """
                    if l_VarNum+1 == len(l_SourceFiles):
                        # Last variation, so we are doing the stageout processing.
                        # NOTE: This isn't required, but included for test purposes.
                        sudo_StageOutStart(pEnv, l_Mountpoint)
                    """

                    l_Handles = bb.getHandles(BBSTATUS["BBALL"])
                    l_AllFullSuccess = bb.waitForCompletion(pEnv, l_Handles)

                    BB_FreeTransferDef(l_TransferDef)

                    print "      >>>>>   End: Variation %d..." % (l_VarNum)

                    if not l_AllFullSuccess:
                        raise BBError(rc=-1, text="Not all transfers had a status of BBFULLSUCCESS")

                bb.checkFiles(l_SourceFiles, l_TargetFiles)

                # Cleanup...

                sudo_RemoveLogicalVolume(pEnv, l_Mountpoint)
                if (len(l_Contrib) == 1):
                    sudo_RemoveJobInfo(pEnv)
                elif (pEnv["contribid"] == 0):
                    time.sleep(5)
                    sudo_RemoveJobInfo(pEnv)

            sudo_RemoveDirectory(pEnv, l_Mountpoint)

    except BBError as error:
        rc = error.rc
        # NOTE: BB_GetLastErrorDetails() output is contained within the error object
        #       and pertinant information is printed out from that data...
        print `error`

    print "   >>>>>   End: %s.%s..." % (__name__, l_FuncName)

    return rc


def main(pEnv):
    rc = 0;

    l_TestCases = (
                   BasicTransfers,
#                   BasicTransfers2,
#                   BasicTransfers3,
                  )

    for i in xrange(1):
        for l_TestCase in l_TestCases:
            rc = l_TestCase(pEnv)
            print "Testcase -> %s, rc = %d" % (os.path.splitext(os.path.basename(l_TestCase.__name__))[0], rc)
            if (rc):
                break
        if (rc):
            break

    return rc


#
# Invoke main routine
#

if __name__ == '__main__':
    main(sys.argv[1])
