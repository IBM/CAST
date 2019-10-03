#if 0
#
#    txpParser.py
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


#   Script:   txpParser
#   Purpose:  Perform the necessary pre-build processing for transport.
#             Parse fuse.h and other specific header files as identified
#             by this script in determineFilesToParse().  Constant values
#             are harvested and various header files are generated into
#             the ${CMAKE_CURRENT_BINARY_DIR}/include directory.
#             All generated header files are named using "gened_*.h" as a
#             template.
#
#   NOTE:  This script requires no additional python packages to run.

# Imports
import os
import pickle
import pprint
import re
import subprocess
import sys
import platform

# DEBUG values
# 0 - No print statements
# 1 - Basic print statements
# 2 - Verbose
DEBUG = 0

# The following variables will be determined early in the processing
# and do not have to be filled in manually
TRANSPORT_BUILD_PATH = None
TRANSPORT_BUILD_INCLUDE_PATH = None
TRANSPORT_BUILD_METADATA_PATH = None
GitTopLevel = None

#
# >>>>>  Start - variables to modify for individual runs...
#
import txpConfig as cfg
#
# >>>>>    End - variables to modify for individual runs...
#

CommentStarted = False

SCRIPT_NAME = None

FUSE_METAFILE_PREFIX = 'Fuse'
FUSE_METAFILE_EXTENSION = 'p'

STARTING_SLASH_SLASH_COMMENT = re.compile("^\s*//")
STARTING_SLASH_STAR_COMMENT = re.compile("^\s*/\*")

FUSE_OPCODE_BEGIN = [False, re.compile("^\s*enum\s+fuse_opcode\s+{")]
FUSE_OPCODE_VALUE = [None, re.compile("^\s*(\S+)\s+=\s+(\d+)\s*,")]
FUSE_OPCODE_END = [False, re.compile("^\s*};")]
ID_BEGIN = [[None, 0], re.compile("^\s*(typedef)?\s*struct\s+(\S+)\s*(\S+)?\s*([{|;])")]
ID_STRUCT_ELEMENT = [None, re.compile("^\s*(struct)\s*(\S+)\s+(\S+)\s*;")]
ID_ELEMENT = [None, re.compile("^\s*(\S+)\s+(\S+)\s*;")]
FUSE_KERNEL_VERSION = [False, re.compile("^#define\s+FUSE_KERNEL_VERSION\s+(\d+)")]
FUSE_KERNEL_MINOR_VERSION = [False, re.compile("^#define\s+FUSE_KERNEL_MINOR_VERSION\s+(\d+)")]

STATIC_STRUCTURE_DATA_1 = (' ',
                           'static const size_t MAXIMUM_NUMBER_OF_ATTRIBUTES = 1*1024;',
                           'static const size_t DEFAULT_NUMBER_OF_CHAR_ARRAYS = 1*1024;',
                           '//static const size_t MAXIMUM_NUMBER_OF_ATTRIBUTES = 8*1024;',
                           '//static const size_t DEFAULT_NUMBER_OF_CHAR_ARRAYS = 8*1024;',
                           ' ',
                           'static const size_t DEFAULT_BUFFER_SIZE = 1024*49;',
                           'static const size_t MAXIMUM_ATTRIBUTE_DUMP_SIZE = 256;',
                           'static const size_t MAXIMUM_DUMP_SIZE = 2048;',
                           ' ',
                           'static const uint32_t EYECATCHER = 0xDEADBEEF;',
                           ' ',
                           '/**',
                           ' * \\brief Constants for the length of data items in the serialized into a buffer.',
                           ' *',
                           ' * \\note  These must be kept in sync with any changes made to the way Msg and Attribute',
                           ' *         objects are serialized into a buffer.',
                           ' *',
                           ' */',
                           'static const size_t LENGTH_OF_MSG_EYECATCHER = sizeof(uint32_t);',
                           'static const size_t LENGTH_OF_MSG_RESERVED_1 = sizeof(uint32_t);',
                           'static const size_t LENGTH_OF_MSG_LENGTH = sizeof(size_t);',
                           'static const size_t LENGTH_OF_MSG_NUMBER = sizeof(int32_t);',
                           'static const size_t LENGTH_OF_PREV_MSG_NUMBER = sizeof(int32_t);',
                           'static const size_t LENGTH_OF_NEXT_MSG_NUMBER = sizeof(int32_t);',
                           'static const size_t LENGTH_OF_REQUEST_MSG_NUMBER = sizeof(int32_t);',
                           'static const pid_t  LENGTH_OF_MSG_CREATOR_PID = sizeof(pid_t);',
                           'static const pid_t  LENGTH_OF_MSG_REQUESTOR_PID = sizeof(pid_t);',
                           'static const pthread_t LENGTH_OF_MSG_CREATOR_TID = sizeof(pthread_t);',
                           'static const pthread_t LENGTH_OF_MSG_REQUESTOR_TID = sizeof(pthread_t);',
                           'static const size_t LENGTH_OF_MSG_ID = sizeof(int32_t);',
                           'static const size_t LENGTH_OF_MSG_VERSION = sizeof(uint16_t);',
                           'static const size_t LENGTH_OF_MSG_STATE = sizeof(uint16_t);',
                           'static const size_t LENGTH_OF_MSG_HEADER_CRC_VALUE = sizeof(unsigned long);',
                           'static const size_t LENGTH_OF_MSG_CRC_VALUE = sizeof(unsigned long);',
                           'static const size_t LENGTH_OF_PAD_VALUE = sizeof(uint8_t);',
                           ' ',
                           'static const size_t LENGTH_OF_HEADER_NO_RESERVED = ',
                           '    LENGTH_OF_MSG_EYECATCHER +',
                           '    LENGTH_OF_MSG_RESERVED_1 +',
                           '    LENGTH_OF_MSG_LENGTH +',
                           '    LENGTH_OF_MSG_NUMBER +',
                           '    LENGTH_OF_PREV_MSG_NUMBER +',
                           '    LENGTH_OF_NEXT_MSG_NUMBER +',
                           '    LENGTH_OF_REQUEST_MSG_NUMBER +',
                           '    LENGTH_OF_MSG_CREATOR_PID +',
                           '    LENGTH_OF_MSG_REQUESTOR_PID +',
                           '    LENGTH_OF_MSG_CREATOR_TID +',
                           '    LENGTH_OF_MSG_REQUESTOR_TID +',
                           '    LENGTH_OF_MSG_ID +',
                           '    LENGTH_OF_MSG_VERSION +',
                           '    LENGTH_OF_MSG_STATE +',
                           '    LENGTH_OF_MSG_HEADER_CRC_VALUE +',
                           '    LENGTH_OF_MSG_CRC_VALUE +',
                           '    LENGTH_OF_PAD_VALUE;',
                           ' ',
                           'static const size_t LENGTH_OF_MSG_RESERVED_2 = ((LENGTH_OF_HEADER_NO_RESERVED%16) ? 16-(LENGTH_OF_HEADER_NO_RESERVED%16) : 16);',
                           ' ',
                           'static const size_t LENGTH_OF_ATTR_TYPE = sizeof(TXP_ATTR_TYPE_TYPE);',
                           'static const size_t LENGTH_OF_ATTR_NAME = sizeof(TXP_ATTR_NAME_TYPE);',
                           'static const size_t LENGTH_OF_ATTR_CHAR_ARRAY_LENGTH = sizeof(TXP_CHAR_ARRAY_LENGTH_TYPE);',
                           'static const size_t LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_NUMBER_OF_ELEMENTS = sizeof(TXP_ARRAY_OF_CHAR_ARRAYS_NUMBER_OF_ELEMENTS_TYPE);',
                           'static const size_t LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_LENGTH = sizeof(TXP_ARRAY_OF_CHAR_ARRAYS_LENGTH_TYPE);',
                           'static const size_t LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE_LENGTH = sizeof(TXP_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE_TYPE);',
                           'static const size_t LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_ELEMENT_DATA_LENGTH = sizeof(TXP_ARRAY_OF_CHAR_ARRAYS_ELEMENT_DATA_TYPE);',
                           ' ',
                           '/**',
                           ' * \\brief Constants for offsets to data items related to a Msg object in a serialized buffer.',
                           ' *',
                           ' * \\note  These offsets are from the beginning of the serialized stream for the Msg.',
                           ' */',
                           'static const size_t OFFSET_TO_MSG_EYECATCHER = 0;',
                           'static const size_t OFFSET_TO_MSG_RESERVED_1 = OFFSET_TO_MSG_EYECATCHER + LENGTH_OF_MSG_EYECATCHER;',
                           'static const size_t OFFSET_TO_MSG_LENGTH = OFFSET_TO_MSG_RESERVED_1 + LENGTH_OF_MSG_RESERVED_1;',
                           'static const size_t OFFSET_TO_MSG_NUMBER = OFFSET_TO_MSG_LENGTH + LENGTH_OF_MSG_LENGTH;',
                           'static const size_t OFFSET_TO_PREV_MSG_NUMBER = OFFSET_TO_MSG_NUMBER + LENGTH_OF_MSG_NUMBER;',
                           'static const size_t OFFSET_TO_NEXT_MSG_NUMBER = OFFSET_TO_PREV_MSG_NUMBER + LENGTH_OF_PREV_MSG_NUMBER;',
                           'static const size_t OFFSET_TO_REQUEST_MSG_NUMBER = OFFSET_TO_NEXT_MSG_NUMBER + LENGTH_OF_NEXT_MSG_NUMBER;',
                           'static const size_t OFFSET_TO_MSG_CREATOR_PID = OFFSET_TO_REQUEST_MSG_NUMBER + LENGTH_OF_REQUEST_MSG_NUMBER;',
                           'static const size_t OFFSET_TO_MSG_REQUESTOR_PID = OFFSET_TO_MSG_CREATOR_PID + LENGTH_OF_MSG_CREATOR_PID;',
                           'static const size_t OFFSET_TO_MSG_CREATOR_TID = OFFSET_TO_MSG_REQUESTOR_PID + LENGTH_OF_MSG_REQUESTOR_PID;',
                           'static const size_t OFFSET_TO_MSG_REQUESTOR_TID = OFFSET_TO_MSG_CREATOR_TID + LENGTH_OF_MSG_CREATOR_TID;',
                           'static const size_t OFFSET_TO_MSG_ID = OFFSET_TO_MSG_REQUESTOR_TID + LENGTH_OF_MSG_REQUESTOR_TID;',
                           'static const size_t OFFSET_TO_MSG_VERSION = OFFSET_TO_MSG_ID + LENGTH_OF_MSG_ID;',
                           'static const size_t OFFSET_TO_MSG_STATE = OFFSET_TO_MSG_VERSION + LENGTH_OF_MSG_VERSION;',
                           'static const size_t OFFSET_TO_MSG_HEADER_CRC_VALUE = OFFSET_TO_MSG_STATE + LENGTH_OF_MSG_STATE;',
                           'static const size_t OFFSET_TO_MSG_CRC_VALUE = OFFSET_TO_MSG_HEADER_CRC_VALUE + LENGTH_OF_MSG_HEADER_CRC_VALUE;',
                           'static const size_t OFFSET_TO_PAD_VALUE = OFFSET_TO_MSG_CRC_VALUE + LENGTH_OF_MSG_CRC_VALUE;',
                           'static const size_t OFFSET_TO_FIRST_ATTRIBUTE = LENGTH_OF_MSG_EYECATCHER +',
                           '                                                LENGTH_OF_MSG_RESERVED_1 +',
                           '                                                LENGTH_OF_MSG_LENGTH +',
                           '                                                LENGTH_OF_MSG_NUMBER +',
                           '                                                LENGTH_OF_PREV_MSG_NUMBER +',
                           '                                                LENGTH_OF_NEXT_MSG_NUMBER +',
                           '                                                LENGTH_OF_REQUEST_MSG_NUMBER +',
                           '                                                LENGTH_OF_MSG_CREATOR_PID +',
                           '                                                LENGTH_OF_MSG_REQUESTOR_PID +',
                           '                                                LENGTH_OF_MSG_CREATOR_TID +',
                           '                                                LENGTH_OF_MSG_REQUESTOR_TID +',
                           '                                                LENGTH_OF_MSG_ID +',
                           '                                                LENGTH_OF_MSG_VERSION +',
                           '                                                LENGTH_OF_MSG_STATE +',
                           '                                                LENGTH_OF_MSG_HEADER_CRC_VALUE +',
                           '                                                LENGTH_OF_MSG_CRC_VALUE +',
                           '                                                LENGTH_OF_PAD_VALUE +',
                           '                                                LENGTH_OF_MSG_RESERVED_2;',
                           ' ',
		                   '// NOTE:  Keep the serialized common header a multiple of 4 bytes for CRC calculation purposes.',
                           'static const size_t LENGTH_OF_MSG_HEADER = OFFSET_TO_FIRST_ATTRIBUTE;',
                           ' ',
                           '/**',
                           ' * \\brief Constants for offsets to data items related to an Attribute object in a serialized buffer.',
                           ' *',
                           ' * \\note  These offsets are from the beginning of the stream of bytes for a given Attribute object.',
                           ' */',
                           'static const size_t OFFSET_TO_ATTR_TYPE = 0;',
                           'static const size_t OFFSET_TO_ATTR_NAME = OFFSET_TO_ATTR_TYPE + LENGTH_OF_ATTR_TYPE;',
                           'static const size_t OFFSET_TO_ATTR_DATA = OFFSET_TO_ATTR_NAME + LENGTH_OF_ATTR_NAME;',
                           'static const size_t OFFSET_TO_ATTR_CHAR_ARRAY_LENGTH = OFFSET_TO_ATTR_DATA;',
                           'static const size_t OFFSET_TO_ATTR_CHAR_ARRAY_DATA = OFFSET_TO_ATTR_CHAR_ARRAY_LENGTH + LENGTH_OF_ATTR_CHAR_ARRAY_LENGTH;',
                           'static const size_t OFFSET_TO_ATTR_ARRAY_OF_CHAR_ARRAYS_NUMBER_OF_ELEMENTS = OFFSET_TO_ATTR_DATA;',
                           'static const size_t OFFSET_TO_ATTR_ARRAY_OF_CHAR_ARRAYS_LENGTH = OFFSET_TO_ATTR_ARRAY_OF_CHAR_ARRAYS_NUMBER_OF_ELEMENTS + LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_NUMBER_OF_ELEMENTS;',
                           'static const size_t OFFSET_TO_ATTR_ARRAY_OF_CHAR_ARRAYS_DATA = OFFSET_TO_ATTR_ARRAY_OF_CHAR_ARRAYS_LENGTH + LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_LENGTH;',
                           'static const size_t OFFSET_TO_ATTR_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE = OFFSET_TO_ATTR_ARRAY_OF_CHAR_ARRAYS_DATA;',
                           'static const size_t OFFSET_TO_ATTR_ARRAY_OF_CHAR_ARRAYS_ELEMENT_DATA = OFFSET_TO_ATTR_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE + LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE_LENGTH;',
                           ' ',
                           '/**',
                           ' * \\brief Constants used to segment the range of Id values used to identify messages.',
                           ' *',
                           ' *     Ranges:',
                           ' *         fuse_opcode - 1 through 65,535',
                           ' *         CORAL range - 65536 through (2**32)-1',
                           ' *',
                           ' * \\note  The CORAL range may be further divided into more CORAL specific ranges.',
                           ' */',
                           'static const uint32_t BEGINNING_FUSE_RANGE = 0;',
                           'static const uint32_t BEGINNING_CORAL_RANGE = 1<<16;',
                           ' ',
                           '/**',
                           ' * \\brief isDigits',
                           ' *',
                           ' *     Method to determine if all input characters are digits.',
                           ' */',
                           'inline bool isDigits(char *str) {',
                           '    while (*str) {',
                           '        if (!isdigit(*str++)) {',
                           '            return false;',
                           '        }',
                           '    }',
                           '    return true;',
                           '}',
                           ' ',
                           '/**',
                           ' * \\enum Id',
                           ' *',
                           ' * \\brief A unique identifier for a given Msg.',
                           ' *',
                           ' *     All Msg objects are constructed with a Id.',
                           ' *',
                           ' * \\note  For the fuse_opcode values, all are listed here even if the CORAL code does not',
                           ' *         specifically have support for that opcode for all Msg() versions.',
                           ' *         See the Msg::isSupported() method to determine if a given fuse_opcode is supported',
                           ' *         by CORAL for a given version.',
                           ' */',
                           'enum Id {',
                           '    ID_INVALID           = txp::BEGINNING_FUSE_RANGE + 0,',
                           ' ',
                           '    //  Keep this portion of FUSE message ids in sync with fuse_opcode in fuse.h...',
                          )

STATIC_STRUCTURE_DATA_2 = (' ',
                           '    //  The CORAL message id range is 65536 to ???, inclusive.',
                          )

STATIC_STRUCTURE_DATA_3 = (' ',
                           '    //  Additional Coral specific message ids go here...',
                           ' ',
                           '};',
                           ' ',
                           '/**',
                           ' * \\enum AttributeType',
                           ' *',
                           ' * \\brief Identifies the data type for a given message attribute.',
                           ' *',
                           ' */',
                           'enum AttributeType {',
                           '    ATTRIBUTE_TYPE_INVALID      = 0x0000,',
                           ' ',
                           '    CHAR                        = 0x0001,',
                           '    UINT8                       = 0x0002,',
                           '    UINT16                      = 0x0003,',
                           '    UINT32                      = 0x0004,',
                           '    UINT64                      = 0x0005,',
                           '    INT8                        = 0x0006,',
                           '    INT16                       = 0x0007,',
                           '    INT32                       = 0x0008,',
                           '    INT64                       = 0x0009,',
                           '    CHAR_ARRAY                  = 0x000A,   // No derived class for this dataType.',
                           '                                            // It can only be a dataType within a',
                           '                                            // buffer.',
                           '    ARRAY_OF_CHAR_ARRAYS        = 0x000B,   // No derived class for this dataType.',
                           '                                            // It can only be a dataType within a',
                           '                                            // buffer.',
                           ' ',
                           '    // NOTE:  The following PTR data types must have',
                           '    //        the same enum values as above except that',
                           '    //        the 16th bit is ON...',
                           '    PTR_CHAR                    = 0x0101,',
                           '    PTR_UINT8                   = 0x0102,',
                           '    PTR_UINT16                  = 0x0103,',
                           '    PTR_UINT32                  = 0x0104,',
                           '    PTR_UINT64                  = 0x0105,',
                           '    PTR_INT8                    = 0x0106,',
                           '    PTR_INT16                   = 0x0107,',
                           '    PTR_INT32                   = 0x0108,',
                           '    PTR_INT64                   = 0x0109,',
                           '    PTR_CHAR_ARRAY              = 0x010A,',
                           '    PTR_ARRAY_OF_CHAR_ARRAYS    = 0x010B,',
                           '};',
                           ' ',
                           '/**',
                           ' * \\enum AttributeName',
                           ' *',
                           ' * \\brief Gives the name of the attribute.',
                           ' *',
                           ' *     For each message Id(), there is typically a set of message attributes (parameters) related to',
                           ' *     that specific message Id().  The set of attributes may change for different message versions',
                           ' *     and for the different supported variations of the messsage.',
                           ' *',
                           ' */',
                           'enum AttributeName {',
                           '    ATTRIBUTE_NAME_INVALID =  0,',
                          )

STATIC_STRUCTURE_DATA_4 = ('};',
                           ' ',
                           '/**',
                           ' * \\enum DeserializeOption',
                           ' *',
                           ' * \\brief Gives the option to use when deserializing a message from a buffer into a',
                           ' *         Msg() object and its associated Attribute() objects.',
                           ' *',
                           ' *     Values are:',
                           ' *         DO_NOT_COPY_DATA    - Every attempt is made to not copy the data.  The attributes',
                           ' *                               associated with the Msg() point to the data in the',
                           ' *                               serialization buffer.',
                           ' *         COPY_DATA_TO_OBJECT - For all attribute types, except PTR_TO_CHAR_ARRAY and',
                           ' *                               PTR_TO_ARRAY_OF_CHAR_ARRAYS, the data is copied to the',
                           ' *                               Attribute() object associated with the Msg() object.',
                           ' *                               For PTR_TO_CHAR_ARRAY and PTR_TO_ARRAY_OF_CHAR_ARRAYS,',
                           ' *                               the data is copied to heap and a pointer to that',
                           ' *                               array is stored in the attribute associated with the Msg() object.',
                           ' *         COPY_DATA_TO_HEAP   - For all attribute types, the data is copied to heap and a pointer',
                           ' *                               to that data is stored in the attribute associated with the',
                           ' *                               Msg() object.',
                           ' *         COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS -',
                           ' *                               For all attribute types, except PTR_TO_CHAR_ARRAY and',
                           ' *                               PTR_TO_ARRAY_OF_CHAR_ARRAYS, the data is copied to the',
                           ' *                               Attribute() object associated with the Msg() object.',
                           ' *                               For PTR_TO_CHAR_ARRAY and PTR_TO_ARRAY_OF_CHAR_ARRAYS,',
                           ' *                               the data is NOT copied and the attribute associated with the',
                           ' *                               Msg() points to the data in the serialization buffer.',
                           ' *',
                           ' */',
                           'enum DeserializeOption {',
                           '    DESERIALIZE_OPTION_INVALID = 0,',
                           '    DO_NOT_COPY_DATA = 1,',
                           '    COPY_DATA_TO_OBJECT = 2,',
                           '    COPY_DATA_TO_HEAP = 3,',
                           '    COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS = 4,',
                           '};',
                           ' ',
                           'static const DeserializeOption DEFAULT_DESERIALIZE_OPTION = txp::COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS;',
                           ' ',
                           '/**',
                           ' * \\enum DataLocation',
                           ' *',
                           ' * \\brief Gives the location of the data during deserialization.',
                           ' *',
                           ' *     Values are:',
                           ' *         BUFFER_HAS_DATA        - Buffer has the data.',
                           ' *         BUFFER_HAS_PTR_TO_DATA - Buffer has a pointer to the data.',
                           ' *',
                           ' */',
                           'enum DataLocation {',
                           '    DATA_LOCATION_INVALID = 0,',
                           '    BUFFER_HAS_DATA = 1,',
                           '    BUFFER_HAS_PTR_TO_DATA = 2,',
                           '};',
                          )

STATIC_STRUCTURE_DATA_5 = ('/**\n',
                           ' * \\note  DO NOT EDIT THIS FILE.  THIS FILE IS GENERATED AT BUILD TIME.\n',
                           ' *         See txpParser.py in the transport scripts directory for more details.\n',
                           ' *\n',
                           ' */\n',
                          )


def addItems(pResults, pKey, pData):
	# NOTE:  l_Value is not currently used...
    pResults[pKey] = {}
    if type(pData) == dict:
        for l_Value, l_Key in pData.items():
            pResults[pKey][l_Key] = l_Value
    else:
        for l_Key in pData:
            pResults[pKey][l_Key] = set()

    return


def addItems2(pResults, pKey, pData):
    pResults[pKey] = {}
    l_Index = 1;
    for l_Key, l_Value in pData.items():
        pResults[pKey][(l_Index, l_Key)] = l_Value
        l_Index = l_Index + 1

    return


def clean(pName):
    return pName.strip("[0]")


def determineFilesToParse(pFilesToParse):
    l_RC = 0

    #
    #  Name of the input path/file(s) to parse
    #
    #  Three files are current parsed for the 'build' environment
    #    fuse.h
    #    fshipcld/include/fshipcld.h
    #    bb/src/messages.h
    #
    #  'Windows' is just for testing this parser script and the
    #  final else leg is for any other special test environments.
    #
    #  NOTE:  fuse.h should be the first header file parsed so that
    #         FUSE_KERNEL_VERSION and FUSE_KERNEL_MINOR_VERSION are
    #         properly set when building the rest of the metadata
    #         dictionaries.
    #

    # NOTE: Add new attributes to txpConfig.py, at the end of ATTRIBUTE_NAMES_TO_ADD
    if platform.system() == 'Linux':    # e.g. bgqsn3, bgqsn5,
        # NOTE:  On Linux, CMake sets the environment variables in sub-shells, so they are not set predictable...
        pFilesToParse.append(os.path.normcase(os.path.join('/usr', 'include', 'linux', 'fuse.h')))
        pFilesToParse.append(os.path.normcase(os.path.join(GitTopLevel, 'fshipcld', 'include', 'fshipcld.h')))
#        pFilesToParse.append(os.path.normcase(os.path.join(GitTopLevel, 'bb', 'src', 'messages.h')))
    elif platform.system() == 'Darwin': # OSX
        pFilesToParse.append(os.environ.get('FUSE_HEADER'))
        pFilesToParse.append(os.environ.get('FSHIPCLD_HEADER'))
    elif platform.system() == 'Windows':
        pFilesToParse.append('C:/Users/IBM_ADMIN/workspace/kernel-2.6.32-504.8.1.el6.src.rpm/linux-2.6.32-504.8.1.el6/include/linux/fuse.h')
    else:
        pFilesToParse.append(os.path.normcase(os.path.join('/usr', 'include', 'linux', 'fuse.h')))

    print "%s - The following files will be parsed for data:" % (SCRIPT_NAME)
    for l_File in pFilesToParse:
        print "%s%s" % (" "*16, l_File)

    return l_RC


def genAttributeInclude1(pOutfile, pResults):
    if pResults.has_key("NAMES"):
        l_Names = pResults["NAMES"].keys()
        l_Names.sort()
        pOutfile.append('%s// txp::Attribute::isAttrNameValid()\n' % (' '*8))
        for l_Name in l_Names:
            l_AttrSupported = True
            for l_Attr in cfg.ATTRIBUTE_NAMES_NOT_SUPPORTED:
                if l_Name.startswith(l_Attr):
                    l_AttrSupported = False
                    break
            if l_AttrSupported:
                pOutfile.append('%scase txp::%s:\n' % (' '*8, clean(l_Name)))
    if pResults.has_key("NAMES_TO_ADD"):
        l_Keys = pResults["NAMES_TO_ADD"].keys()
        l_Keys.sort()
        for l_Key in l_Keys:
            pOutfile.append('%scase txp::%s:\n' % (' '*8, l_Key[1]))
    if pResults.has_key("STRUCTS"):
        l_Structs = pResults["STRUCTS"].keys()
        l_Structs.sort()
        for l_Struct in l_Structs:
            pOutfile.append('%scase txp::%s:\n' % (' '*8, clean(l_Struct)))
    pOutfile.append('%sbreak;\n' % (' '*12))

    return


def genAttributeInclude2(pOutfile, pResults):
    if pResults.has_key("NAMES"):
        l_Names = pResults["NAMES"].keys()
        l_Names.sort()
        pOutfile.append('%s// txp::Attribute::attrNameToChar()\n' % (' '*12))
        for l_Name in l_Names:
            l_AttrSupported = True
            for l_Attr in cfg.ATTRIBUTE_NAMES_NOT_SUPPORTED:
                if l_Name.startswith(l_Attr):
                    l_AttrSupported = False
                    break
            if l_AttrSupported:
                pOutfile.append('%scase txp::%s:%s' % (' '*12, clean(l_Name), os.linesep))
                pOutfile.append('%sstrCpy(pBuffer, "%s", pSize);\n' % (' '*16, clean(l_Name)))
                pOutfile.append('%sbreak;\n' % (' '*16))
    if pResults.has_key("NAMES_TO_ADD"):
        l_Keys = pResults["NAMES_TO_ADD"].keys()
        l_Keys.sort()
        for l_Key in l_Keys:
            pOutfile.append('%scase txp::%s:%s' % (' '*12, l_Key[1], os.linesep))
            pOutfile.append('%sstrCpy(pBuffer, "%s", pSize);\n' % (' '*16, l_Key[1]))
            pOutfile.append('%sbreak;\n' % (' '*16))
    if pResults.has_key("STRUCTS"):
        l_Structs = pResults["STRUCTS"].keys()
        l_Structs.sort()
        for l_Struct in l_Structs:
            pOutfile.append('%scase txp::%s:%s' % (' '*12, clean(l_Struct), os.linesep))
            pOutfile.append('%sstrCpy(pBuffer, "%s", pSize);\n' % (' '*16, clean(l_Struct)))
            pOutfile.append('%sbreak;\n' % (' '*16))

    return


def genMsgInclude1(pOutfile, pResults):
    # Used by Id::isMsgIdSupported()
    if pResults.has_key("FUSE_OPCODE"):
        l_Ids = pResults["FUSE_OPCODE"].keys()
        l_Ids.sort()
        pOutfile.append('%s// txp::Msg::isSupported(pId)\n' % (' '*8))
        for l_Id in l_Ids:
            pOutfile.append('%scase txp::%s:\n' % (' '*8, pResults["FUSE_OPCODE"][l_Id]))
    if pResults.has_key("CORAL_IDS"):
        l_Ids = pResults["CORAL_IDS"].keys()
        l_Ids.sort()
        for l_Id in l_Ids:
            pOutfile.append('%scase txp::%s:\n' % (' '*8, pResults["CORAL_IDS"][l_Id]))
    pOutfile.append('%sbreak;\n' % (' '*12))

    return


def genMsgInclude2(pOutfile, pResults):
    if pResults.has_key("FUSE_OPCODE"):
        l_Ids = pResults["FUSE_OPCODE"].keys()
        l_Ids.sort()
        pOutfile.append('%s// txp::Msg::isValid(pId)\n' % (' '*8))
        for l_Id in l_Ids:
            pOutfile.append('%scase txp::%s:\n' % (' '*8, pResults["FUSE_OPCODE"][l_Id]))
    if pResults.has_key("CORAL_IDS"):
        l_Ids = pResults["CORAL_IDS"].keys()
        l_Ids.sort()
        for l_Id in l_Ids:
            pOutfile.append('%scase txp::%s:\n' % (' '*8, pResults["CORAL_IDS"][l_Id]))
    pOutfile.append('%sbreak;\n' % (' '*12))

    return


def genMsgInclude3(pOutfile, pResults):
    if pResults.has_key("IDS_NOT_VALID"):
        l_Ids = pResults["IDS_NOT_VALID"].keys()
        l_Ids.sort()
        pOutfile.append('%s// txp::Msg::msgIdToChar()\n' % (' '*12))
        for l_Id in l_Ids:
            pOutfile.append('%scase txp::%s:\n' % (' '*12, pResults["IDS_NOT_VALID"][l_Id]))
            pOutfile.append('%sstrCpy(pBuffer, "%s", pSize);\n' % (' '*16, pResults["IDS_NOT_VALID"][l_Id]))
            pOutfile.append('%sbreak;\n' % (' '*16))
    if pResults.has_key("FUSE_OPCODE"):
        l_Ids = pResults["FUSE_OPCODE"].keys()
        l_Ids.sort()
        for l_Id in l_Ids:
            pOutfile.append('%scase txp::%s:\n' % (' '*12, pResults["FUSE_OPCODE"][l_Id]))
            pOutfile.append('%sstrCpy(pBuffer, "%s", pSize);\n' % (' '*16, pResults["FUSE_OPCODE"][l_Id]))
            pOutfile.append('%sbreak;\n' % (' '*16))
    if pResults.has_key("CORAL_IDS"):
        l_Ids = pResults["CORAL_IDS"].keys()
        l_Ids.sort()
        for l_Id in l_Ids:
            pOutfile.append('%scase txp::%s:\n' % (' '*12, pResults["CORAL_IDS"][l_Id]))
            pOutfile.append('%sstrCpy(pBuffer, "%s", pSize);\n' % (' '*16, pResults["CORAL_IDS"][l_Id]))
            pOutfile.append('%sbreak;\n' % (' '*16))

    return


def genMsgHandlerInclude1(pOutfile, pResults):
    if pResults.has_key("FUSE_OPCODE"):
        l_Ids = pResults["FUSE_OPCODE"].keys()
        l_Ids.sort()
        pOutfile.append('%s// MessageHandler::processMessage()\n' % (' '*8))
        for l_Id in l_Ids:
            pOutfile.append('%scase txp::Id::%s:\n' % (' '*8, pResults["FUSE_OPCODE"][l_Id]))
    if pResults.has_key("CORAL_IDS"):
        l_Ids = pResults["CORAL_IDS"].keys()
        l_Ids.sort()
        for l_Id in l_Ids:
            pOutfile.append('%scase txp::Id::%s:\n' % (' '*8, pResults["CORAL_IDS"][l_Id]))
    pOutfile.append('%sbreak;\n' % (' '*12))

    return


def preProcessLine(pLine):
    global CommentStarted

    l_Start = 0
    l_End = len(pLine)-1

    if DEBUG == 2:
        print '%s' % ('='*80)

    if pLine[l_Start:l_End]:
        if not CommentStarted:
            l_Success = STARTING_SLASH_SLASH_COMMENT.search(pLine)
            if l_Success:
                if DEBUG == 2:
                    print 'Starting // found'
                l_End = 0
            else:
                l_Success = STARTING_SLASH_STAR_COMMENT.search(pLine)
                if l_Success:
                    if DEBUG == 2:
                        print 'Starting /* found'
                    l_Index = pLine.rfind('*/')
                    if l_Index != -1:
                        # Process anything after the comment
                        l_Start = l_Index+2
                    else:
                        l_End = 0
                        CommentStarted = True
                else:
                    l_Index = pLine.find('/*')
                    if l_Index != -1:
                        # Process anything before the comment
                        if DEBUG == 2:
                            print '/* found at position %d' % (l_Index)
                        l_End = l_Index-1
                        l_Index = pLine.rfind('*/')
                        if l_Index == -1:
                            CommentStarted = True
                        else:
                            if DEBUG == 2:
                                print '*/ found at position %d' % (l_Index)
                            # NOTE:  If we find '/*' in the line and
                            #        it does not start the line, we
                            #        assume that there is nothing
                            #        meaningful after the ending '*/'.
                            #        Therefore, if there was a valid
                            #        statement before the '/*' and a
                            #        valid statement after the ending
                            #        '*/', we would not process the
                            #        second statement.  @@DLH
                            pass
        else:
            l_Index = pLine.rfind('*/')
            if l_Index != -1:
                if DEBUG == 2:
                    print '*/ found at position %d' % (l_Index)
                l_Start = l_Index+2
                CommentStarted = False
            else:
                l_End = 0

    if DEBUG == 2:
        print "l_Start = %d, l_End = %d, CommentStarted = %s, |%s|" % (l_Start, l_End, CommentStarted, pLine[l_Start:l_End])

    return pLine[l_Start:l_End]


def parseFiles(pFilesToParse, pResults):
    global ID_BEGIN, FUSE_KERNEL_VERSION, FUSE_KERNEL_MINOR_VERSION, FUSE_OPCODE_BEGIN, FUSE_OPCODE_END
    l_RC = 0

    addItems(pResults, "NAMES", cfg.ATTRIBUTE_NAMES)
    pResults["STRUCTS"] = {}
    for l_File in pFilesToParse:
        if os.path.isfile(l_File):
            try:
                l_LineNo = 0
                for l_OrgLine in open(l_File, "r").readlines():
                    l_LineNo += 1
                    l_Line = preProcessLine(l_OrgLine)
                    if l_Line:
                        # First check for id/struct processing...
                        if ID_BEGIN[0][0] != None:
                            ID_BEGIN[0][1] += l_Line.count('{')
                            ID_BEGIN[0][1] -= l_Line.count('}')
                            if ID_BEGIN[0][1] >= 0:
                                if ID_BEGIN[0][1] == 0:
                                    if DEBUG:
                                        print "%36s -> %5d: %s" % ("End id/struct", l_LineNo, l_OrgLine[:-1])
                                    # Last closing right paren found...  End of this id/struct...
                                    ID_BEGIN[0] = [None, 0]
                                elif ID_BEGIN[0][1] == 1:
                                    if ((l_Line.count('{') + l_Line.count('}')) < 2):
                                        # Search for another element (data member) for the current Id (which is represented by a struct...)
                                        l_Success = ID_STRUCT_ELEMENT[1].search(l_Line)
                                        if l_Success:
                                            # Found a struct element for the current Id being processed...
                                            if DEBUG:
                                                print "%36s -> %5d: %s" % ("Continue struct element", l_LineNo, l_OrgLine[:-1])
                                                # pprint.pprint(l_Success.groups())
                                            pResults["STRUCTS"][ID_BEGIN[0][0]]['struct'].append((l_Success.groups()[1], l_Success.groups()[0]))
                                        else:
                                            l_Success = ID_ELEMENT[1].search(l_Line)
                                            if l_Success:
                                                # Found an element for the current Id being processed...
                                                if DEBUG:
                                                    print "%36s -> %5d: %s" % ("Continue element", l_LineNo, l_OrgLine[:-1])
                                                    # pprint.pprint(l_Success.groups())
                                                l_AttrType = l_Success.groups()[0]
                                                l_AttrName = clean(l_Success.groups()[1])
                                                pResults["STRUCTS"][ID_BEGIN[0][0]]['struct'].append((l_AttrName, l_AttrType))
                                                if l_AttrName not in cfg.INVALID_FUSE_NAMES:
                                                    if not pResults["NAMES"].has_key(l_AttrName):
                                                        pResults["NAMES"][l_AttrName] = set()
                                                    pResults["NAMES"][l_AttrName].add(l_AttrType)
                                            else:
                                                if DEBUG:
                                                    print "%36s -> %5d: %s" % ("Skip, not an element", l_LineNo, l_OrgLine[:-1])
                                    else:
                                        if DEBUG:
                                            print "%36s -> %5d: %s" % ("Skip, 2 or more parens on line", l_LineNo, l_OrgLine[:-1])
                                elif ID_BEGIN[0][1] > 1:
                                    if DEBUG:
                                        print "%36s -> %5d: %s" % ("Skip, greater than 1 open paren", l_LineNo, l_OrgLine[:-1])
                                else:
                                    print "%s - Error: More right parens found than left when parsing." % (SCRIPT_NAME)
                                    l_RC = -6
                                    break
                        else:
                            # Not in the middle of processing an Id, so search for a new one...
                            l_Success = ID_BEGIN[1].search(l_Line)
                            if l_Success:
                                if (l_Success.groups()[1] != l_Success.groups()[2]) :
                                    # Found the beginning of a new Id...
                                    l_StructName = l_Success.groups()[1]
                                    pResults["STRUCTS"][l_StructName] = {}
                                    pResults["STRUCTS"][l_StructName]['Version'] = (pResults["FUSE_KERNEL_VERSION"], pResults["FUSE_KERNEL_MINOR_VERSION"])
                                    pResults["STRUCTS"][l_StructName]['struct'] = []
                                    if (l_Success.groups()[3]) == '{':
                                        # Ends with a right paren...
                                        if DEBUG:
                                            print "%36s -> %5d: %s" % ("Begin id/struct", l_LineNo, l_OrgLine[:-1])
                                            # pprint.pprint(l_Success.groups())
                                        ID_BEGIN[0][0] = l_StructName
                                        ID_BEGIN[0][1] = l_Line.count('{')
                                    else:
                                        # Ends with a semicolon...
                                        if DEBUG:
                                            print "%36s -> %5d: %s" % ("Begin/end id/struct", l_LineNo, l_OrgLine[:-1])
                                            # pprint.pprint(l_Success.groups())
                                        pResults["STRUCTS"][l_StructName]['struct'].append((l_Success.groups()[2], l_Success.groups()[1]))

                                else:
                                    if DEBUG:
                                        print "%36s -> %5d: %s" % ("Skip, simple typedef", l_LineNo, l_OrgLine[:-1])

                            elif os.path.basename(l_File) == 'fuse.h':
                                if not FUSE_OPCODE_END[0]:
                                    # Have not reached the end of the fuse_opcodes...
                                    if not FUSE_OPCODE_BEGIN[0]:
                                        # Have not found the beginning of the fuse_opcodes...
                                        if not FUSE_KERNEL_VERSION[0]:
                                            # Have not found FUSE_KERNEL_VERSION... (First item to find...)
                                            l_Success = FUSE_KERNEL_VERSION[1].search(l_Line)
                                            if l_Success:
                                                # Record FUSE_KERNEL_VERSION...
                                                FUSE_KERNEL_VERSION[0] = True
                                                if (l_Success.groups()[0]).isdigit():
                                                    pResults["FUSE_KERNEL_VERSION"] = int(l_Success.groups()[0])
                                                else:
                                                    l_RC = -1
                                                    break
                                            else:
                                                if DEBUG:
                                                    print "%36s -> %5d: %s" % ("Skip, no hit for all parsing (1)", l_LineNo, l_OrgLine[:-1])
                                            continue

                                        elif not FUSE_KERNEL_MINOR_VERSION[0]:
                                            # Have not found FUSE_KERNEL_MINOR_VERSION... (Second item to find...)
                                            l_Success = FUSE_KERNEL_MINOR_VERSION[1].search(l_Line)
                                            if l_Success:
                                                # Record FUSE_KERNEL_MINOR_VERSION...
                                                FUSE_KERNEL_MINOR_VERSION[0] = True
                                                if (l_Success.groups()[0]).isdigit():
                                                    pResults["FUSE_KERNEL_MINOR_VERSION"] = int(l_Success.groups()[0])
                                                else:
                                                    l_RC = -2
                                                    break
                                            else:
                                                if DEBUG:
                                                    print "%36s -> %5d: %s" % ("Skip, no hit for all parsing (2)", l_LineNo, l_OrgLine[:-1])
                                            continue

                                        l_Success = FUSE_OPCODE_BEGIN[1].search(l_Line)
                                        if l_Success:
                                            # Found the beginning of the fuse_opcodes... (Third 'group' of items to find...)
                                            if DEBUG:
                                                print "%36s -> %5d: %s" % ("Beginning of the fuse opcodes...", l_LineNo, l_OrgLine[:-1])
                                            FUSE_OPCODE_BEGIN[0] = True
                                            pResults["FUSE_OPCODE"] = {}
                                        else:
                                            if DEBUG:
                                                print "%36s -> %5d: %s" % ("Skip, no hit for all parsing (3)", l_LineNo, l_OrgLine[:-1])
                                    else:
                                        # Currently processing the fuse_opcodes...  Search for another value...
                                        l_Success = FUSE_OPCODE_VALUE[1].search(l_Line)
                                        if l_Success:
                                            # Record this fuse_opcode value...
                                            if DEBUG:
                                                print "%36s -> %5d: %s" % ("Fuse opcode...", l_LineNo, l_OrgLine[:-1])
                                            pResults["FUSE_OPCODE"][int(l_Success.groups()[1])] = l_Success.groups()[0]
                                        else:
                                            # Failed in finding a new opcode value...  Check for the end of the fuse_opcodes...
                                            l_Success = FUSE_OPCODE_END[1].search(l_Line)
                                            if l_Success:
                                                # Reached the end of the fuse_opcodes...
                                                # Start looking for message Ids... (Fourth group of items to find...)
                                                if DEBUG:
                                                    print "%36s -> %5d: %s" % ("End of the fuse opcodes...", l_LineNo, l_OrgLine[:-1])
                                                FUSE_OPCODE_END[0] = True
                                            else:
                                                if DEBUG:
                                                    print "%36s -> %5d: %s" % ("Skip, no hit for all parsing (4)", l_LineNo, l_OrgLine[:-1])
                                else:
                                    if DEBUG:
                                        print "%36s -> %5d: %s" % ("Skip, no hit for all parsing (5)", l_LineNo, l_OrgLine[:-1])
                            else:
                                if DEBUG:
                                    print "%36s -> %5d: %s" % ("Skip, no hit for all parsing (6)", l_LineNo, l_OrgLine[:-1])
                    else:
                        if DEBUG:
                            print "%36s -> %5d: %s" % ("Skip, preProcessLine()", l_LineNo, l_OrgLine[:-1])

                # Finished processing file....  Sanity check...
                if DEBUG == 2:
                    print '%s' % ('='*80)

                if ID_BEGIN[0][0] != None:
                    print "%s - Error: Parsing of structs for AttributeNames did not end normally." % (SCRIPT_NAME)
                    l_RC = -3
                    break

            except Exception as l_Exception:
                print "ERROR - Script %s, line number %s, Exception: %s" % (os.path.split(sys.exc_info()[2].tb_frame.f_code.co_filename)[1], sys.exc_info()[2].tb_lineno, l_Exception)
                l_RC = -4
                break

        else:
            print "%s - Error: Input fuse header file not found at %s" % (SCRIPT_NAME, l_File)
            l_RC = -5
            break

    return l_RC


def generateHeaderFile(pResults):
    l_RC = 0

    fOut = None

    try:
        l_File = os.path.normcase(os.path.join(GitTopLevel, TRANSPORT_BUILD_INCLUDE_PATH)) + os.sep + cfg.COMMON_HEADER_FILE
        fOut = open(l_File, "w")

        fOut.write("static const uint8_t TXP_FUSE_KERNEL_VERSION = %s;\n" % (pResults.get("FUSE_KERNEL_VERSION", "")))
        fOut.write("static const uint8_t TXP_FUSE_KERNEL_MINOR_VERSION = %s;\n" % (pResults.get("FUSE_KERNEL_MINOR_VERSION", "")))

        for l_Line in STATIC_STRUCTURE_DATA_1:
            fOut.write("%s\n" % (l_Line))

        if pResults.has_key("FUSE_OPCODE"):
            l_OpCodes = pResults["FUSE_OPCODE"].keys()
            l_OpCodes.sort()
            for l_OpCode in l_OpCodes:
                fOut.write("%s%-20s = txp::BEGINNING_FUSE_RANGE + %4d,\n" % (" "*4, pResults["FUSE_OPCODE"][l_OpCode], l_OpCode))

        for l_Line in STATIC_STRUCTURE_DATA_2:
            fOut.write("%s\n" % (l_Line))

        if pResults.has_key("CORAL_IDS"):
            l_OpCodes = pResults["CORAL_IDS"].keys()
            l_OpCodes.sort()
            for l_OpCode in l_OpCodes:
                fOut.write("%s%-20s = txp::BEGINNING_CORAL_RANGE + %4d,\n" % (" "*4, pResults["CORAL_IDS"][l_OpCode], l_OpCode))

        for l_Line in STATIC_STRUCTURE_DATA_3:
            fOut.write("%s\n" % (l_Line))

        l_Names = pResults["NAMES"].keys()
        l_Names.sort()
        l_Index = 1
        for l_Name in l_Names:
            l_AttrSupported = True
            for l_Attr in cfg.ATTRIBUTE_NAMES_NOT_SUPPORTED:
                if l_Name.startswith(l_Attr):
                    l_AttrSupported = False
                    break
            if l_AttrSupported:
                fOut.write("%s%-22s = %2d,\n" % (" "*4, l_Name, l_Index))
                l_Index += 1

        if pResults.has_key("NAMES_TO_ADD"):
            l_Keys = pResults["NAMES_TO_ADD"].keys()
            l_Keys.sort()
            for l_Key in l_Keys:
                fOut.write("%s%-22s = %2d,\n" % (" "*4, l_Key[1], pResults["NAMES_TO_ADD"][l_Key]))

        l_Structs = pResults["STRUCTS"].keys()
        l_Structs.sort()
        l_Index = 16385
        for l_Struct in l_Structs:
            fOut.write("%s%-30s = %5d,\n" % (" "*4, clean(l_Struct), l_Index))
            l_Index += 1

        for l_Line in STATIC_STRUCTURE_DATA_4:
            fOut.write("%s\n" % (l_Line))

        fOut.close()

    except Exception as l_Exception:
        print "ERROR - Script %s, line number %s, Exception: %s" % (os.path.split(sys.exc_info()[2].tb_frame.f_code.co_filename)[1], sys.exc_info()[2].tb_lineno, l_Exception)
        if fOut:
            fOut.close()
        l_RC = -1

    return l_RC


def generateExecutableIncludes(pResults):
    l_RC = 0

    fOut = None
    l_Outfile = []

    try:
        for l_Include in cfg.GENED_EXECUTABLE_INCLUDES:
            if l_RC == 0:
                del l_Outfile[:]
                if l_Include == 'gened_Msg_CC_Include_1.h':
                    genMsgInclude1(l_Outfile, pResults)

                elif l_Include == 'gened_Msg_CC_Include_2.h':
                    # Used by Id::isMsgIdValid()
                    genMsgInclude2(l_Outfile, pResults)

                elif l_Include == 'gened_Msg_CC_Include_3.h':
                    # Used by Id::msgIdToChar()
                    genMsgInclude3(l_Outfile, pResults)

                elif l_Include == 'gened_MsgHandler_CC_Include_1.h':
                    # Used by MessageHandler::processMessage()
                    genMsgHandlerInclude1(l_Outfile, pResults)

                elif l_Include == 'gened_Attribute_CC_Include_1.h':
                    # Used by Attribute::isAttrNameValid()
                    genAttributeInclude1(l_Outfile, pResults)

                elif l_Include == 'gened_Attribute_CC_Include_2.h':
                    # Used by Attribute::attrNameToChar()
                    genAttributeInclude2(l_Outfile, pResults)

                else:
                    l_RC = -1

            if l_Outfile:
                l_FileName = os.path.normcase(os.path.join(GitTopLevel, TRANSPORT_BUILD_INCLUDE_PATH)) + os.sep + l_Include
                fOut = open(l_FileName, "w")
                fOut.writelines(STATIC_STRUCTURE_DATA_5)
                fOut.writelines(l_Outfile)
                fOut.close()
                fOut = None

                print "%s - File generated -> %s" % (SCRIPT_NAME, l_FileName)

    except Exception as l_Exception:
        print "ERROR - Script %s, line number %s, Exception: %s" % (os.path.split(sys.exc_info()[2].tb_frame.f_code.co_filename)[1], sys.exc_info()[2].tb_lineno, l_Exception)
        if fOut:
            fOut.close()
        l_RC = -1

    return l_RC


def processFuseVersions(pResults, pReplaceCurrentMetadataFile):
    l_RC = 0

    fOut = None

    try:
        if (pResults["FUSE_KERNEL_VERSION"] != None and pResults["FUSE_KERNEL_MINOR_VERSION"] != None):
            l_Current_Fuse_Metadata_Path = os.path.normcase(os.path.join(GitTopLevel, cfg.TRANSPORT_METADATA_PATH))
            l_Current_Fuse_Metadata_Name = FUSE_METAFILE_PREFIX + '_' + `pResults["FUSE_KERNEL_VERSION"]` + '_' + `pResults["FUSE_KERNEL_MINOR_VERSION"]`
            l_Current_Fuse_Metadata_File = os.path.normcase(os.path.join(l_Current_Fuse_Metadata_Path, l_Current_Fuse_Metadata_Name + '.' + FUSE_METAFILE_EXTENSION))

            l_ReplaceCurrentMetadataFile = pReplaceCurrentMetadataFile

            # NOTE: Do not produce pickle metadata until versioning is implemented...  @@DLH
            if 1==1 and ((not os.path.exists(l_Current_Fuse_Metadata_File)) or l_ReplaceCurrentMetadataFile):
                #  Dump out the current fuse metadata file
                fOut = open(l_Current_Fuse_Metadata_File, 'w')
                pickle.dump(pResults, fOut)
                fOut.close()
                fOut = None
                print "%s - SUCCESS: Fuse metadata file %s successfully created or refreshed" % ((SCRIPT_NAME), l_Current_Fuse_Metadata_Name + '.' + FUSE_METAFILE_EXTENSION)

            # Load the current fuse metadata file
            fOut = open(l_Current_Fuse_Metadata_File, "r")
            l_Results = pickle.load(fOut)
            fOut.close()
            fOut = None

            # Print out a text version of the current fuse metadata file to the transport build path
            fOut = open(os.path.normcase(os.path.join(GitTopLevel, TRANSPORT_BUILD_METADATA_PATH, l_Current_Fuse_Metadata_Name + '.' + 'txt')), "w+")
            pprint.pprint(l_Results, fOut)
            fOut.close()
            fOut = None

    except Exception as l_Exception:
        print "ERROR - Script %s, line number %s, Exception: %s" % (os.path.split(sys.exc_info()[2].tb_frame.f_code.co_filename)[1], sys.exc_info()[2].tb_lineno, l_Exception)
        if fOut:
            fOut.close()
        l_RC = -1

    return l_RC


def main():
    global SCRIPT_NAME, TRANSPORT_BUILD_PATH, TRANSPORT_BUILD_INCLUDE_PATH, TRANSPORT_BUILD_METADATA_PATH
    global GitTopLevel

    print sys.argv[0], sys.argv[1], sys.argv[2]

    SCRIPT_NAME = os.path.basename(os.path.normpath(sys.argv[0]))

    # sys.argv[1] is CMAKE_CURRENT_BINARY_DIR
    TRANSPORT_BUILD_PATH = os.path.normcase(sys.argv[1])
    TRANSPORT_BUILD_INCLUDE_PATH = os.path.normcase(os.path.join(TRANSPORT_BUILD_PATH, 'include'))
    TRANSPORT_BUILD_METADATA_PATH = os.path.normcase(os.path.join(TRANSPORT_BUILD_PATH, 'metadata'))

    GitTopLevel = os.path.normcase(sys.argv[2])

    l_FilesToParse = []

    l_Results = {}
    l_Results["FUSE_KERNEL_VERSION"] = None
    l_Results["FUSE_KERNEL_MINOR_VERSION"] = None

    for l_Key, l_Ids in (("CORAL_IDS",cfg.CORAL_IDS), ("IDS_NOT_VALID",cfg.IDS_NOT_VALID)):
        addItems(l_Results, l_Key, l_Ids)

    for l_Key, l_Ids in (("NAMES_TO_ADD",cfg.ATTRIBUTE_NAMES_TO_ADD),):
        addItems2(l_Results, l_Key, l_Ids)

    l_RC = determineFilesToParse(l_FilesToParse)
    if (l_RC == 0):
        l_RC = parseFiles(l_FilesToParse, l_Results)
        if (l_RC == 0):
            l_RC = generateHeaderFile(l_Results)
            if (l_RC == 0):
                l_RC = generateExecutableIncludes(l_Results)
                if (l_RC == 0):
                    l_RC = processFuseVersions(l_Results, False)
                    if (l_RC == 0):
                        pass
                    else:
                        print "%s - Error returned from processFuseVersions().  See prior messages." % (SCRIPT_NAME)
                else:
                    print "%s - Error returned from generateExecutableIncludes().  See prior messages." % (SCRIPT_NAME)
            else:
                print "%s - Error returned from generateHeaderFile().  See prior messages." % (SCRIPT_NAME)
        else:
            print "%s - Error returned from parseFiles().  See prior messages." % (SCRIPT_NAME)
    else:
        print "%s - Error returned from determineFilesToParse().  See prior messages." % (SCRIPT_NAME)

    if l_RC == 0:
        print "%s - Successfully ended." % (SCRIPT_NAME)
    else:
        print "%s - ERROR: transport related includes/code not all successfully generated.  See previous messages." % (SCRIPT_NAME)

    return l_RC


if __name__ == '__main__':
    sys.exit( main() )

#endif
