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

# NOTE:  DO NOT add new CORAL attributes to the end of this list.  This list is frozen, to maintain existing enum values.
#        All new attributes must be added to ATTRIBUTE_NAMES_TO_ADD.
ATTRIBUTE_NAMES =  ('arg',
                    'argsIn',
                    'argsOut',
                    'atime',
                    'atimensec',
                    'attr_valid',
                    'attr_valid_nsec',
                    'attributeData',
                    'available_spare',                      #
                    'base',
                    'bavail',
                    'bbjob_objectversion',                  # __u32
                    'bbjob_serializeversion',               # __u32
                    'bfree',
                    'blksize',
                    'block',
                    'blocks',
                    'blocksize',
                    'bsize',
                    'buffer',                               # char*
                    'buffersize',                           # __u64
                    'burstBytesRead',                       # __u64
                    'burstBytesWritten',                    # __u64
                    'busy_time',                            # __u64
                    'checksum',                             # __u64
                    'child',
                    'chunkSize',
                    'clearMetaData',                        # __u32
                    'cmd',
                    'configPath',
                    'congestion_threshold',
                    'connectionKey',                        # char*
                    'contrib',                              # __u32
                    'contribid',                            # __u32
                    'count',
                    'crc32',
                    'critical_warning',                     # __u64
                    'ctime',
                    'ctimensec',
                    'daemon_pid',
                    'data',
                    'dataObtainedLocally',                  # __u32
                    'data_read',                            # __u64
                    'data_written',                         # __u64
                    'dev_major',
                    'dev_minor',
                    'devicenum',                            # __u32
                    'end',
                    'entry_valid',
                    'entry_valid_nsec',
                    'epochtimeinseconds',                   # time_t
                    'error',
                    'events',
                    'extents',                              # CharArray*
                    'ffree',
                    'fh',
                    'files',                                # CharArray*
                    'flags',
                    'frsize',
                    'fsync_flags',
                    'fuse_version_major',
                    'fuse_version_minor',
                    'generation',
                    'getattr_flags',
                    'getattr_out',
                    'gid',
                    'groupid',                              # __s32
                    'handle',
                    'handles',                              # CharArray*
                    'hdr',
                    'hostname',                             # char*
                    'id',
                    'in_iovs',
                    'in_size',
                    'ino',
                    'instance',                             # char_array
                    'inval_entry_out',
                    'jobid',                                # __u64
                    'jobstepid',                            # __u64
                    'keys',                                 # CharArray*
                    'kh',
                    'knownSerials',                         # CharArray*
                    'len',
                    'length',
                    'link',
                    'lk_flags',
                    'lkey',
                    'localBytesRead',                       # __u64
                    'localBytesWritten',                    # __u64
                    'localReadCount',                       # __u64
                    'localTransferSize',                    # __u64
                    'localWriteCount',                      # __u64
                    'localstatus',                          # __u64
                    'lock_owner',
                    'lookupname',
                    'major',
                    'markfailed',                           # __u32
                    'mask',
                    'matchstatus',                          # __u64
                    'max_background',
                    'max_read',
                    'max_readahead',
                    'max_write',
                    'maxlba',                               # __u64
                    'media_errors',                         # __u64
                    'minor',
                    'mntptgid',                             # __u32
                    'mntptuid',                             # __u32
                    'mode',
                    'mountpoint',                           # char*
                    'mountsize',                            # char*
                    'mtime',
                    'mtimensec',
                    'nackSerials',                          # CharArray*
                    'name',
                    'nameOfAttribute',
                    'namelen',
                    'newdir',
                    'newdirInode',
                    'newgroup',                             # char*
                    'newmode',                              # __u32
                    'newname',
                    'newowner',                             # char*
                    'newpathname',                          # char*
                    'next',
                    'nlink',
                    'nlookup',
                    'nodeid',
                    'notify',
                    'notify_unique',
                    'numBytesAvailable',                    # __u64
                    'numTransferDefs',                      # __u32
                    'num_err_log_entries',                  # __u64
                    'num_read_commands',                    # __u64
                    'num_write_commands',                   # __u64
                    'numavailhandles',                      # __u64
                    'numcontrib',                           # __u64
                    'numhandles',                           # __u64
                    'numreportingcontribs',                 # __u64
                    'objectversion',                        # __u32
                    'off',
                    'offset',
                    'offset2newname',
                    'offset2oldname',
                    'oldname',
                    'oldnodeid',
                    'opcode',
                    'openOutFlags',
                    'open_flags',
                    'option',                               # __u32
                    'outHelloVersion',
                    'out_iovs',
                    'out_size',
                    'owner',
                    'parent',
                    'pathname',                             # char*
                    'percentage_used',
                    'performoperation',                     # __u32
                    'pid',
                    'power_cycles',                         # __u64
                    'power_on_hours',                       # __u64
                    'rate',                                 # __u64
                    'rdev',
                    'read_flags',
                    'readcount',                            # __u64
                    'readtime',                             # __u64
                    'release_flags',
                    'responseHandle',                       # __u64
                    'result',
                    'resultCode',                           # __s32
                    'returncode',                           # __s32
                    'revents',
                    'rkey',
                    'secondsCleanup',
                    'serializeversion',                     # __u32
                    'signal',
                    'signalSent',
                    'size',
                    'sizetransferred',                      # __u64
                    'sourcefile',                           # char*
                    'sourceindex',                          # __u32
                    'start',
                    'statfsRC',
                    'statinfo',                             # char*
                    'status',                               # __u64
                    'tag',                                  # __u64
                    'temperature',
                    'time_gran',
                    'totalBytesRead',                       # __u64
                    'totalBytesWritten',                    # __u64
                    'totalProcessingTime',                  # __u64
                    'totalTransferKeyLength',               # __u64
                    'totalTransferSize',                    # __u64
                    'transferHandle',                       # __u64
                    'transfercontribid',                    # __u32
                    'transferdefs',                         # char*
                    'transferhandle',                       # __u64
                    'transfertimeinterval',                 # __s32
                    'type',
                    'uid',
                    'umask',
                    'unique',
                    'unlinkatname',
                    'unlinkedInode',
                    'unsafe_shutdowns',                     # __u64
                    'userid',                               # __s32
                    'uuid',                                 # char*
                    'valid',
                    'value',                                # char*
                    'value64',                              # __u64
                    'values',                               # CharArray*
                    'variable',                             # char*
                    'version',                              # char*
                    'version_major',
                    'version_minor',
                    'whence',
                    'whoami',                               # char_array
                    'workitemspertransfertimeinterval',     # __s32
                    'write_flags',
                    'writecount',                           # __u64
                    'writetime',                            # __u64
                   )

# NOTE:  All new attributes to be added must be added to end of this dictionary
#        with the appropriate 'next' numeric key value that will be used as the enum value.
# NOTE:  The developer must ensure that the attribute name being added is NOT in the
#        ATTRIBUTE_NAMES_NOT_SUPPORTED list.
ATTRIBUTE_NAMES_TO_ADD =   {
                                'synccount':4096,
                                'synctime':4097,
                                'timeBaseScale':4098,
                           }

# NOTE:  Add new CORAL message ids to either the end of the CORAL_ section or the end of the BB_ section
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
             'BB_GETTRANSFERCOUNT':292,
             'BB_SETSERVERCFGVALUE':293,
             'BB_GETSERVERCFGVALUE':294,
             'BB_GETFILEINFO':295,
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
