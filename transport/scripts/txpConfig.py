#if 0
#
#    txpConfig.py
#
#    Copyright IBM Corporation 2015,2016. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#

#   Script:   txpConfig
#   Purpose:  Variables are defined here that can be changed to influence
#             the running of txpParser.py.  txpParser.py is run at build
#             time to perform the necessary pre-build processing for transport.
#
#   NOTE:  This script requires no additional python packages to run.

# Imports
import os

#
# >>>>>  Start - variables to modify for individual runs...
#

#  Change to the name of the path/header file to generate
TRANSPORT_INCLUDE_PATH = os.path.normcase(os.path.join('transport', 'include'))
TRANSPORT_METADATA_PATH = os.path.normcase(os.path.join('transport', 'metadata'))
TRANSPORT_SCRIPTS_PATH = os.path.normcase(os.path.join('transport', 'scripts'))
TRANSPORT_SRC_PATH = os.path.normcase(os.path.join('transport', 'src'))
ATTRIBUTE_NAMES_NOT_SUPPORTED = ('dummy',
                                 'padding',
                                 'spare',
                                 'unused',
                                 )
ATTRIBUTE_NAMES_TO_ADD = ('bbjob_objectversion',                # __u32
                          'bbjob_serializeversion',             # __u32
                          'buffer',                             # char*
                          'buffersize',                         # __u64
                          'checksum',                           # __u64
                          'clearMetaData',                      # __u32
                          'contribid',                          # __u32
                          'dataObtainedLocally',                # __u32
                          'epochtimeinseconds',                 # time_t
                          'handles',                            # CharArray*
                          'hostname',                           # char*
                          'jobid',                              # __u64
                          'jobstepid',                          # __u64
                          'localstatus',                        # __u64
                          'localTransferSize',                  # __u64
                          'markfailed',                         # __u32
                          'matchstatus',                        # __u64
                          'maxlba',                             # __u64
                          'mntptgid',                           # __u32
                          'mntptuid',                           # __u32
                          'numavailhandles' ,                   # __u64
                          'numhandles' ,                        # __u64
                          'numreportingcontribs',               # __u64
                          'numBytesAvailable',                  # __u64
                          'numTransferDefs',                    # __u32
                          'objectversion',                      # __u32
                          'option',                             # __u32
                          'performoperation',                   # __u32
                          'rate',                               # __u64
                          'returncode',                         # __s32
                          'serializeversion',                   # __u32
                          'sizetransferred',                    # __u64
                          'sourcefile',                         # char*
                          'sourceindex',                        # __u32
                          'statinfo',                           # char*
                          'status',                             # __u64
                          'totalTransferKeyLength',             # __u64
                          'totalTransferSize',                  # __u64
                          'transfercontribid',                  # __u32
                          'transferdefs',                       # char*
                          'transferhandle',                     # __u64
                          'transfertimeinterval',               # __s32
                          'value',                              # char*
                          'value64',                            # __u64
                          'variable',                           # char*
                          'uuid',                               # char*
                          'workitemspertransfertimeinterval',   # __s32
                          'connectionKey',                      # char*
                         )

CORAL_IDS = {'CORAL_AUTHENTICATE':1,
#             'CORAL_CHANGESERVER':2,
             'CORAL_ERROR':3,
             'CORAL_GETVAR':4,
             'CORAL_GOODBYE':5,
             'CORAL_HELLO':6,
             'CORAL_HELLO_MONITOR':7,
             'CORAL_NO_OP':8,
             'CORAL_READY':9,
             'CORAL_SETVAR':10,
             'CORAL_SIGNAL':11,
             'CORAL_STAGEOUT_START':12,
             'BB_ALL_FILE_TRANSFERS_COMPLETE':256,
             'BB_CANCELTRANSFER':257,
             'BB_CHMOD':258,
             'BB_CHOWN':259,
             'BB_CREATEDIR':260,
             'BB_CREATELOGICALVOLUME':261,
             'BB_GETDEVICEUSAGE':262,
             'BB_GETUSAGE':263,
             'BB_GETLVKEY':264,
             'BB_GETTHROTTLERATE':265,
             'BB_GETTRANSFERHANDLE':266,
             'BB_GETTRANSFERINFO':267,
             'BB_GETTRANSFERKEYS':268,
             'BB_GETTRANSFERLIST':269,
             'BB_REMOVEDIR':270,
             'BB_REMOVELOGICALVOLUME':271,
             'BB_RESIZEMOUNTPOINT':272,
             'BB_REMOVEJOBINFO':273,
             'BB_SETTHROTTLERATE':274,
	         'BB_SETUSAGELIMIT':275,
             'BB_STARTTRANSFER':276,
             'BB_TRANSFER_COMPLETE_FOR_CONTRIBID':277,
             'BB_TRANSFER_COMPLETE_FOR_FILE':278,
             'BB_TRANSFER_COMPLETE_FOR_HANDLE':279,
             'BB_TRANSFER_PROGRESS':280,
             'BB_SETSERVER':281,
             'BB_GETSERVER':282,
             'BB_SWITCHTRANSFER':283,
             'BB_RESTART_TRANSFERS':284,
             'BB_RETRIEVE_TRANSFERS':285,
             'BB_STOP_TRANSFERS':286,
             'BB_RESUME':287,
             'BB_SUSPEND':288,
             'BB_OPENSERVER':289,
             'BB_CLOSESERVER':290,
             'BB_GETSERVERBYNAME':291,
            }

IDS_NOT_SUPPORTED = (
                    )

IDS_NOT_VALID = {"ID_INVALID":0,
                }

INVALID_FUSE_NAMES = ('name[]',)

COMMON_HEADER_FILE = 'gened_Common.h'
GENED_EXECUTABLE_INCLUDES = ('gened_Msg_CC_Include_1.h',
                             'gened_Msg_CC_Include_2.h',
                             'gened_Msg_CC_Include_3.h',
#                             'gened_MsgHandler_CC_Include_1.h',
                             'gened_Attribute_CC_Include_1.h',
                             'gened_Attribute_CC_Include_2.h',
                            )
#
# >>>>>    End - variables to modify for individual runs...
#
#endif
