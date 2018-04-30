#!/usr/bin/python
####################################################
#    ResizeTest.py
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
    Testcases for to bbapi functions.
"""

import os
import sys

import bb
from bberror import *
from bbapi import *


def Resize(pEnv):
    rc = 0
    l_FuncName = sys._getframe().f_code.co_name

    print "   >>>>> Start: %s.%s..." % (__name__, l_FuncName)

    l_Owner = pEnv.get("OWNER", None)
    l_Group = pEnv.get("GROUP", None)
    l_Mode = int(pEnv.get("MODE", 0755))
    l_LVSize = "16M"

    l_OrgSrc = "%s" % pEnv["ORGSRC"]
    l_Mountpoint = "%s" % pEnv["MOUNT"]
    l_Target = "%s" % pEnv["TARGET"]

    # NOTE: Any non-zero return code that is returned from any of the BB_* api calls
    #       will cause an exception to be thrown and caught in the handler below.
    #       The return code is available as exception data...
    try:
        # Print out the environment to setup...
        bb.printEnv(pEnv)

        # Cleanup any prior running of this variation and setup to run this time...
        rc = bb.initEnv(pEnv, l_Mountpoint, (l_Target,))

        if (rc == 0):
            # Create the necessary directories...
            BB_CreateDirectory(l_Mountpoint)
            BB_ChangeOwner(l_Mountpoint, l_Owner, l_Group)
            BB_ChangeMode(l_Mountpoint, l_Mode)

            # Create the logical volume for the mountpoint...
            BB_CreateLogicalVolume(l_Mountpoint, l_LVSize)
            bb.runCmd("lsblk")
            bb.runCmd("ls -lar %s" % (l_Mountpoint))


            # Testcase variations

            # Relative specifictions to increase size- all successful

            print "%sIncrease by 512 bytes" % (os.linesep)
            BB_ResizeMountPoint(l_Mountpoint, "+512B", "BB_NONE")
            bb.runCmd("lsblk")
            print "%sIncrease by 512 sectors" % (os.linesep)
            BB_ResizeMountPoint(l_Mountpoint, "+512S", "BB_NONE")
            bb.runCmd("lsblk")
            print "%sIncrease by 64K" % (os.linesep)
            BB_ResizeMountPoint(l_Mountpoint, "+64K", "BB_NONE")
            bb.runCmd("lsblk")
            print "%sIncrease by 128M" % (os.linesep)
            BB_ResizeMountPoint(l_Mountpoint, "+128M", "BB_NONE")
            bb.runCmd("lsblk")
            print "%sIncrease by 0.25G" % (os.linesep)
            BB_ResizeMountPoint(l_Mountpoint, "+0.25G", "BB_NONE")
            bb.runCmd("lsblk")


            # Absolute specifiction to increase size - successful

            print "%sIncrease to 2G" % (os.linesep)
            BB_ResizeMountPoint(l_Mountpoint, "2G", "BB_NONE")
            bb.runCmd("lsblk")


            # Invalid resize specifications

            print "%sERROR - Invalid mountpoint" % (os.linesep)
            try:
                BB_ResizeMountPoint("JUST/A/BUNCH/OF/JUNK", "2.1G", "BB_NONE")
            except BBError as error:
                print `error`
            bb.runCmd("lsblk")

            print "%sERROR - Invalid size" % (os.linesep)
            try:
                BB_ResizeMountPoint(l_Mountpoint, "ABC", "BB_NONE")
            except BBError as error:
                print `error`
            bb.runCmd("lsblk")

            print "%sERROR - Invalid flags" % (os.linesep)
            try:
                BB_ResizeMountPoint(l_Mountpoint, "2.1G", "MORE_JUNK")
            except BBError as error:
                print `error`
            bb.runCmd("lsblk")

            print "%sERROR - Invalid size suffix of 'X'" % (os.linesep)
            try:
                BB_ResizeMountPoint(l_Mountpoint, "2X", "BB_NONE")
            except BBError as error:
                print `error`
            bb.runCmd("lsblk")

            # Attempt to decrease size (relative size) and preserve file system
            print "%sERROR - Cannot decrease size (relative size) and preserve XFS file system" % (os.linesep)
            try:
                BB_ResizeMountPoint(l_Mountpoint, "-256M", "BB_NONE")
            except BBError as error:
                print `error`
            bb.runCmd("lsblk")

            # Attempt to decrease size (absolute size) and preserve file system
            print "%sERROR - Cannot decrease size (absolute size) and preserve XFS file system" % (os.linesep)
            try:
                BB_ResizeMountPoint(l_Mountpoint, "1G", "BB_NONE")
            except BBError as error:
                print `error`
            bb.runCmd("lsblk")


            # Successful resize and do not preserve file system

            # Decrease size and do not preserve file system
            print "%sDecrease size by 5M and do not preserve XFS file system" % (os.linesep)
            BB_ResizeMountPoint(l_Mountpoint, "-5M", "BB_DO_NOT_PRESERVE_FS")
            bb.runCmd("lsblk")
            try:
                bb.runCmd("ls -lar %s" % (l_Mountpoint))
            except BBError as error:
                print `error`
            bb.runCmd("lsblk")


            # Attempt to access the mountpoint
            print "%sERROR - Mountpoint no longer mounted" % (os.linesep)
            try:
                BB_ResizeMountPoint(l_Mountpoint, "1G", "BB_NONE")
            except BBError as error:
                print `error`
            bb.runCmd("lsblk")

            # Cleanup...
            BB_RemoveLogicalVolume(l_Mountpoint)
            BB_RemoveDirectory(l_Mountpoint)

    except BBError as error:
        rc = error.rc
        print `error`

    print "   >>>>>   End: %s.%s..." % (__name__, l_FuncName)

    return rc


def main(pEnv):
    rc = 0;

    l_TestCases = (
                   Resize,
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
