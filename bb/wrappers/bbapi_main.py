#!/usr/bin/python
####################################################
#    bbapi_main.py
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
    This module is the main driver of the bbapi functions using Python wrappers.
    The following operations are performed:
      1) Invokes BB_InitLibrary()
      2) Invokes the identified Python testcase
      3) Invokes BB_TerminateLibrary()

    Options:
    -h --help           Display usage information
    -a --cn_failover    CN failover value.  Default is 0, which is False.  Any other integer value is True.
    -b --io_failover    ESS failover value.  Default is 0, which is False.  Any other integer value is True.
    -c --contrib        Contrib value to use.  Default is (0,).
    -d --name           Name for Set/Open/CloseServer.  No default.
    -e --floor          Floor path.  Default is /opt/ibm
                        NOTE:  This value is ignored if libpath is provided.
    -f --flags          Flags.  No default.
    -g --group          Group name to use.  Default is "users".
    -h --handle         Handle value.  Default is 0.
    -i --hostname       Hostname to use.  Default is "".
    -j --jobid          Jobid value to use.  Default is 1.
    -k --jobstepid      Jobstepid value to use.  Default is 1.
    -l --libpath        Library path to libbbAPI.so.  Default is "$FLOOR/bb/lib".
    -m --mount          Path for mountpoint(s).  Default is "/tmp/$USER/mnt".
    -n --mode           Mode value to use.  No default.  If not specified and needed,
                        the testcase must provide it.
    -o --owner          Owner value to use.  Default is $USER.
    -p --testpath       Path to testcase to run.  Default is ".", the current working directory.
    -q --orgsrc         Path to original source files.  Default is "/gpfs/gpfs0/$USER/source".
    -r --contribid      Contribid value to use. Default is 0.
    -s --size           Size of the logical volume to create.  Default is "1G".
                        NOTE:  If specifying a value like one-half gigabyte, the leading
                               zero MUST be specified.  (e.g., 0.5G and not .5G)
    -t --tag            Tag value to use.  Default is 1.
    -u --unixpath       Unixpath to use.  No default.  If not specified, the value is
                        obtained from the configuration file.
    -v --type           Type for GetServer.  No default.
    -w --procedure      Procedure to invoke.
    -x --target         Path prefix for target(s)  Default is "/gpfs/gpfs0/$USER/target".
    -y --testcase       Testcase to run.  Default is "bbtests".
                        NOTE:  This must be a python script file, but do not include the .py
    -z --config         Configuration file to use.  Default is "/etc/ibm/bb.cfg".
    -1 --process_args   Process arguments (bbapiAdminProcs interface only)
    -2 --iteration      Iteration number (NOTE: Test convention is to add 10 times this value
                                                to the jobid value in initEnv() to prevent jobid
                                                collision.  Used to generate unique jobid values
                                                by iteration.)
    -3 --jobid_bump     JobId bump value (NOTE: Test convention is to add this value
                                                to the jobid value in initEnv() to prevent jobid
                                                collision.  Used to generate unique jobid values
                                                within a testcase.)
    -4 --cancelscope    Cancel scope.  Default is BBSCOPETAG.

     All paths can be given as either absolute or relative to the current working directory.

     This main program establishes a burst buffer environment for an already running
     instance of bbServer/bbProxy.  It basically will:
        - Invoke BB_InitLibrary()
        - Invoke the specified testcase.
            This testcase may run one or more 'variations'.
            This testcase will typically create any necessary
              libraries/files and any necessary logical volume(s).
        - Invoke BB_TerminateLibrary()

     The options available on this main program allow tests to:
        - Stage-in files from an original source location (--orgsrc) to
            the mountpoint location (--mount), which is intended to be mounted
            on a logical volume that will allocate space from the bb volume group.
        - Stage-out files from the mountpoint location (--mount) to
            the target location (--target).

     Typical example using a personal sandbox:
        ~/git/bluecoral/bb/wrappers -> python bbapi_main.py --libpath ~/git/bluecoral/work/bb/lib

        The current working directory is the source side bb/wrappers directory.  This is so you can make changes directly
        to the python source code and have it immediately take effect for the next run.  Otherwise, you have to install the
        python changes to the /work side before they would take effect.

        The configuration file that is specified is the same as was used to start the bbServer/bbProxy that is to be
        tested.

        The libpath specified is the library where the libbbAPI.so can be found.  It must be the same libbbAPI.so that is
        associated with the bbServer/bbProxy that is to be tested.

        If the configuration file to be used is the normal configuration file for a sandbox and libpath is specified,
        the default configuration file used is one directory up from the libpath specification appended with "scripts/bb.cfg".
        Therefore, if libpath is specified and the normal sandbox configuration file is to be used, the config specification
        can be omitted.
"""

import ctypes
import getopt
import importlib
import os
import pprint
import sys

import bb
from bbapi import BBCANCELSCOPE, DEFAULT_BBCANCELSCOPE, INVALID_BBCANCELSCOPE

#
# Default values
#

DEFAULT_TESTPATH = "."
DEFAULT_TESTCASE = "bbtests"

DEFAULT_CONTRIB = (0,)
DEFAULT_CONTRIBID = 0
DEFAULT_JOBID = 1
DEFAULT_JOBSTEPID = 1
DEFAULT_TAG = 1

DEFAULT_HANDLE = 0
DEFAULT_HOSTNAME = ""

l_Temp = os.environ['USER']
#DEFAULT_CONFIG = (os.path.join(*("%s,etc,ibm,bb.cfg" % (os.path.sep)).split(",")))
DEFAULT_CONFIG = (os.path.join(*("%s,u,dlherms,CAST,work,bb,scripts,bb.cfg" % (os.path.sep)).split(",")))
DEFAULT_ORGSRC = (os.path.join(*("%s,gpfs,gpfs0,%s,source" % (os.path.sep, l_Temp)).split(",")))
DEFAULT_MOUNT = (os.path.join(*("%s,tmp,%s,mnt" % (os.path.sep, l_Temp)).split(",")))
DEFAULT_TARGET = (os.path.join(*("%s,gpfs,gpfs0,%s,target" % (os.path.sep, l_Temp)).split(",")))
DEFAULT_OWNER = l_Temp
DEFAULT_GROUP = "users"
DEFAULT_SIZE = "1.5G"

DEFAULT_CN_FAILOVER = False
DEFAULT_IO_FAILOVER = False

# NOTE:  When --procedure_args is a zero length, the shared library infomation is sent to the console.
#        When invoked via the command line, it will be the DEFAULT and the length will be zero.
#        When invoked via bbapiAdminProcs, it is always passed as a value, even if a particular
#        API doesn't have any arguments to pass.
DEFAULT_PROCEDURE_ARGS = ""
DEFAULT_ITERATION = 0
DEFAULT_JOBID_BUMP = 0

NOCONFIG = ""
NOUNIXPATH = ""
ALL = "*"

#
# Wrappers for bbapi calls needed for testcase setup
#
def BB_GetVersion(pSize, pAPI_Version):
    return bb.api.BB_GetVersion(pSize.value, pAPI_Version)

def BB_InitLibrary(pContribId, pClientVersion):
    return bb.api.BB_InitLibrary(pContribId.value, pClientVersion)

def BB_TerminateLibrary():
    return bb.api.BB_TerminateLibrary()

def Coral_InitLibrary(pContribId, pClientVersion, pConfig, pUnixPath):
    return bb.api.Coral_InitLibrary(pContribId.value, pClientVersion, pConfig, pUnixPath)


#
# Helper routines
#

def usage(code, msg=''):
    print(__doc__, file=sys.stderr)
    if msg:
        print(msg, file=sys.stderr)
    sys.exit(code)


def setDefaults(pEnv):
    setDefaultFloor(pEnv)
    setLibPath(pEnv)
    pEnv["cancelscope"] = DEFAULT_BBCANCELSCOPE
    pEnv["contrib"] = DEFAULT_CONTRIB
    pEnv["contribid"] = DEFAULT_CONTRIBID
    pEnv["handle"] = DEFAULT_HANDLE
    pEnv["hostname"] = DEFAULT_HOSTNAME
    pEnv["jobid"] = DEFAULT_JOBID
    pEnv["jobstepid"] = DEFAULT_JOBSTEPID
    pEnv["tag"] = DEFAULT_TAG

    pEnv['CONFIG'] = DEFAULT_CONFIG

    pEnv["ORGSRC"] = DEFAULT_ORGSRC
    pEnv["MOUNT"] = DEFAULT_MOUNT
    pEnv["TARGET"] = DEFAULT_TARGET
    pEnv["SIZE"] = DEFAULT_SIZE

    pEnv["OWNER"] = DEFAULT_OWNER
    pEnv["GROUP"] = DEFAULT_GROUP

    pEnv["CN_FAILOVER"] = DEFAULT_CN_FAILOVER
    pEnv["IO_FAILOVER"] = DEFAULT_IO_FAILOVER

    setDefaultTestPath(pEnv)
    pEnv["TESTCASE"] = DEFAULT_TESTCASE
    pEnv["procedure_args"] = DEFAULT_PROCEDURE_ARGS
    # NOTE: The iteration value can be passed into the main()
    #       routine of this module
    if ("iteration" not in pEnv):
        pEnv["iteration"] = DEFAULT_ITERATION
    # NOTE: The jobid_bump value can be passed into the main()
    #       routine of this module
    if ("jobid_bump" not in pEnv):
        pEnv["jobid_bump"] = DEFAULT_JOBID_BUMP

    return

def setDefaultFloor(pEnv):
    pEnv["FLOOR"] = os.path.join(*(",%s,opt,ibm" % (os.path.sep)).split(","))

    return

def setDefaultTestCase(pEnv):
    pEnv["TESTCASE"] = DEFAULT_TESTCASE

    return

def setDefaultTestPath(pEnv):
    pEnv["TESTPATH"] = os.path.abspath(DEFAULT_TESTPATH)

    return

def setLibPath(pEnv):
    pEnv["LIBPATH"] = pEnv["FLOOR"] + os.path.join(*("%s,bb,lib" % (os.path.sep)).split(","))

    return

def setLib(pEnv):
    pEnv["LIB"] = pEnv["LIBPATH"] + os.path.join(*("%s,libbbAPI.so" % (os.path.sep)).split(","))

    return

def setSysPath(pEnv):
    if pEnv["TESTPATH"] not in sys.path:
        sys.path.append(pEnv["TESTPATH"])
#    print "sys.path="
#    pprint.pprint(sys.path)

    return

def setWrapperPath(pEnv):
    l_LibPath = pEnv["LIBPATH"].split(os.path.sep)
    pEnv["WRAPPER_PATH"] = os.path.sep.join(l_LibPath[:-1]) + os.path.sep + 'wrappers'

    return

def processArgs(pEnv, pArgs):
    setDefaults(pEnv)

    try:
        l_Opts, l_Args = getopt.getopt(pArgs,"ha:b:c:d:e:f:g:h:i:j:k:l:m:n:o:p:q:r:s:t:u:v:w:x:y:z:1:2:3:4:",["cn_failover=","io_failover=","contrib=","name=","floor=","flags=","group=","handle=","hostname=","jobid=","jobstepid=","libpath=","mount=","mode=","owner=","testpath=","orgsrc=","contribid=","size=","tag=","unixpath=","type=","procedure=","target=","testcase=","config=","procedure_args=","iteration=","jobid_bump=","cancelscope="])
    except getopt.GetoptError as e:
        print(e, file=sys.stderr)
        usage(1, "Invalid arguments passed")

    l_ConfigSpecified = False
    l_LibPathSpecified = False
    for l_Opt, l_Arg in l_Opts:
#        print l_Opt, l_Arg
        if l_Opt == '-h':
            print(__doc__)
            sys.exit()
        elif l_Opt in ("-a", "--cn_failover"):
            if (l_Arg == "0"):
                pEnv["CN_FAILOVER"] = False
            else:
                pEnv["CN_FAILOVER"] = True
        elif l_Opt in ("-b", "--io_failover"):
            if (l_Arg == "0"):
                pEnv["IO_FAILOVER"] = False
            else:
                pEnv["IO_FAILOVER"] = True
        elif l_Opt in ("-c", "--contrib"):
            l_Temp = l_Arg.split(",")
            pEnv["contrib"] = tuple(map(int, l_Temp))
        elif l_Opt in ("-d", "--name"):
            pEnv["name"] = l_Arg
        elif l_Opt in ("-e", "--floor"):
            pEnv["FLOOR"] = os.path.abspath(l_Arg)
            setLibPath(pEnv["FLOOR"])
        elif l_Opt in ("-f", "--flags"):
            pEnv["FLAGS"] = l_Arg
        elif l_Opt in ("-g", "--group"):
            pEnv["GROUP"] = l_Arg
        elif l_Opt in ("-h", "--handle"):
            if (str(l_Arg).upper() != ALL):
                pEnv["handle"] = int(l_Arg)
            else:
                pEnv["handle"] = bb.NO_HANDLE
        elif l_Opt in ("-i", "--hostname"):
            if (str(l_Arg).upper() != ALL):
                pEnv["hostname"] = l_Arg
            else:
                pEnv["hostname"] = bb.NO_HOSTNAME
        elif l_Opt in ("-j", "--jobid"):
            if (str(l_Arg).upper() != ALL):
                pEnv["jobid"] = int(l_Arg)
            else:
                pEnv["jobid"] = bb.NO_JOBID
        elif l_Opt in ("-k", "--jobstepid"):
            if (str(l_Arg).upper() != ALL):
                pEnv["jobstepid"] = int(l_Arg)
            else:
                pEnv["jobstepid"] = bb.NO_JOBSTEPID
        elif l_Opt in ("-l", "--libpath"):
            pEnv["LIBPATH"] = os.path.abspath(l_Arg)
            l_LibPathSpecified = True
        elif l_Opt in ("-m", "--mount"):
            pEnv["MOUNT"] = os.path.abspath(l_Arg)
        elif l_Opt in ("-n", "--mode"):
            pEnv["MODE"] = int(l_Arg)
        elif l_Opt in ("-o", "--owner"):
            pEnv["OWNER"] = l_Arg
        elif l_Opt in ("-p", "--testpath"):
            pEnv["TESTPATH"] = os.path.abspath(l_Arg)
        elif l_Opt in ("-q", "--orgsrc"):
            pEnv["ORGSRC"] = os.path.abspath(l_Arg)
        elif l_Opt in ("-r", "--contribid"):
            if (str(l_Arg).upper() != ALL):
                pEnv["contribid"] = int(l_Arg)
            else:
                pEnv["contribid"] = bb.NO_CONTRIBID
        elif l_Opt in ("-s", "--size"):
            pEnv["SIZE"] = l_Arg
        elif l_Opt in ("-t", "--tag"):
            pEnv["tag"] = int(l_Arg)
        elif l_Opt in ("-u", "--unixpath"):
            pEnv["UNIXPATH"] = os.path.abspath(l_Arg)
        elif l_Opt in ("-v", "--type"):
            pEnv["type"] = l_Arg
        elif l_Opt in ("-w", "--procedure"):
            pEnv["procedure"] = l_Arg
        elif l_Opt in ("-x", "--target"):
            pEnv["TARGET"] = os.path.abspath(l_Arg)
        elif l_Opt in ("-y", "--testcase"):
            pEnv["TESTCASE"] = l_Arg
        elif l_Opt in ("-z", "--config"):
            pEnv["CONFIG"] = os.path.abspath(l_Arg)
            l_ConfigSpecified = True
        elif l_Opt in ("-1", "--procedure_args"):
            pEnv["procedure_args"] = os.path.abspath(l_Arg)
        elif l_Opt in ("-2", "--iteration"):
            pEnv["iteration"] = int(l_Arg)
        elif l_Opt in ("-3", "--jobid_bump"):
            pEnv["jobid_bump"] = int(l_Arg)
        elif l_Opt in ("-4", "--cancelscope"):
            pEnv["cancelscope"] = BBCANCELSCOPE[l_Arg]

    pEnv["COMMAND"] = os.path.abspath(__file__)
    pEnv["COMMAND_LINE_ARGS"] = " ".join(pArgs)
    setWrapperPath(pEnv)

    setLib(pEnv)
#    pprint.pprint(pEnv)

    return


def buildLibraryWrapper(pEnv):

    return ctypes.CDLL(pEnv["LIB"])


def main(pArgs):
    l_SavedPath = os.environ['PATH']
    l_NewPath = '.:' + l_SavedPath
    os.environ['PATH'] = l_NewPath
#    print("Environment variable $PATH set to %s" % (l_NewPath))

    l_Env = {}
    try:
        processArgs(l_Env, pArgs)

        # Build the ctypes interfaces to the BB APIs...
        bb.api = buildLibraryWrapper(l_Env)

        # We only print out the shared library information for the main routine...
        if (len(l_Env["procedure_args"]) == 0):
            print("Shared library:  %s" % (l_Env["LIB"]))

        l_Contribid = bb.cvar("contribid", l_Env)

        l_Size = bb.cvar("size", 256)
        l_API_Version = ctypes.create_string_buffer(256)
        rc = BB_GetVersion(l_Size, l_API_Version)

        if (rc == 0):
            if (l_Env["TESTCASE"] != "None"):
                if "CONFIG" in l_Env or "UNIXPATH" in l_Env:
                    l_Config = ctypes.c_char_p(l_Env.get("CONFIG", NOCONFIG).encode())
                    l_UnixPath = ctypes.c_char_p(l_Env.get("UNIXPATH", NOUNIXPATH).encode())
                    rc = Coral_InitLibrary(l_Contribid, l_API_Version, l_Config, l_UnixPath)
                else:
                    rc = BB_InitLibrary(l_Contribid, l_API_Version)

                if (rc == 0):
                    setSysPath(l_Env)
                    l_TestCase = importlib.import_module(l_Env["TESTCASE"])
                    rc = l_TestCase.main(l_Env)
                    print("bbapi_main.main(): rc = %d" % (rc))
                else:
                    print("BB_InitLibrary():  rc = %d" % (rc))

                BB_TerminateLibrary()
        else:
            print("BB_GetVersion():  rc = %d" % (rc))
    except Exception as e:
        rc = -1
        print('Exception raised from bbapi_main::main(), %s' % (e))
    finally:
        os.environ['PATH'] = l_SavedPath
#        print("Environment variable $PATH set to %s" % (l_SavedPath))

    return rc


#
# Invoke main routine
#

if __name__ == '__main__':
    rc = main(sys.argv[1:])

    if (rc in (0, -2)):
        rc = 0
    else:
        rc = -1
    print("bbapi_main: rc = %d" % (rc))

    sys.exit(rc)
