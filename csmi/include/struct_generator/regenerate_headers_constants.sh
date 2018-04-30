#!/bin/bash
#================================================================================
#
#    csmi/include/regenerate_headers_constants.sh
#
#  © Copyright IBM Corporation 2015-2018. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================

# Numeric constants for versioning.
VERSION_WIDTH=8       # The width of a version code (major, minor and ptf have the same width).
VERSION_MASK=$(( (1<<${VERSION_WIDTH}) - 1))

# The verision shifts
PTF_VERSION_SHIFT=0
MINOR_VERSION_SHIFT=${VERSION_WIDTH}
MAJOR_VERSION_SHIFT=$(( ${VERSION_WIDTH} + ${MINOR_VERSION_SHIFT} ))

# Versioning constants
VERSION_PREFIX="CSM_VERSION_"
VERSION_ID="CSM_VERSION_ID"
VERSION_INSERT="VERSION START"
MIN_VERSION="CSM_MIN_VERSION"
DEV_VERSION="CSM_DEVELOPMENT"

# Directories for use in the generation functions.
BASE_DIR=".."
CSMI_DIR="../../"
CSM_DIR="../../../"
TYPES_DIR="csm_types/"
INCLUDE_DIR="csmi/include/"
MACROS_DIR="csmi/src/common/include/"
CHANGE_LOG_DIR="${CSMI_DIR}change_logs"

MACROS="csm_serialization_x_macros.h"
STRUCT_HASH="csmi/src/common/include/csmi_struct_hash.h"
API_HEADER="${BASE_DIR}/csm_api.h"
VERSION_HEADER="${BASE_DIR}/csm_api_version.h"

FAILURE_FILE=".csm_regenerate_failure" # Regenerate failure, if any of the attempts fail alert.

# Directory to output the results.
OUTPUT_DIR=${BASE_DIR}/

# The groups of the structs and functions, based on the folder they were located.
API_GROUPS=(  "wm_apis"     \
              "bb_apis"     \
              "inv_apis"    \
              "diag_apis"   \
              "launch_apis" \
              "common_apis" \
              "ras_apis" )

# Long names of the groups.
API_GROUPS_LONG=(  "Workload Management" \
                   "Burst Buffer"        \
                   "Inventory"           \
                   "Diagnostic"          \
                   "Launch"              \
                   "Common"              \
                   "RAS" )

# The directories to pull structs from.
STRUCT_DIRS=( "csm_types/struct_defs/wm" \
              "csm_types/struct_defs/bb" \
              "csm_types/struct_defs/inv" \
              "csm_types/struct_defs/diag" \
              "csm_types/struct_defs/launch" \
              "csm_types/struct_defs/common" \
              "csm_types/struct_defs/ras" )

# The directories to pull enums from.
ENUM_DIRS=( "csm_types/enum_defs/wm" \
            "csm_types/enum_defs/bb" \
            "csm_types/enum_defs/inv" \
            "csm_types/enum_defs/diag" \
            "csm_types/enum_defs/launch" \
            "csm_types/enum_defs/common" \
            "csm_types/enum_defs/ras" )

# The files to output data types to.
OUTPUT_FILE_TYPES=( "csmi_type_wm.h" \
                    "csmi_type_bb.h" \
                    "csmi_type_inv.h" \
                    "csmi_type_diag.h" \
                    "csmi_type_launch.h" \
                    "csmi_type_common.h" \
                    "csmi_type_ras.h" )
                    
# The files to output function definitions to.
OUTPUT_FILE_FUNCTS=( "csmi_type_wm_funct.h" \
                     "csmi_type_bb_funct.h" \
                     "csmi_type_inv_funct.h" \
                     "csmi_type_diag_funct.h" \
                     "csmi_type_launch_funct.h" \
                     "csmi_type_common_funct.h" \
                     "csmi_type_ras_funct.h" )

OUTPUT_FILE_C=("src/wm/src/csmi_wm_serialization.c" \
                "src/bb/src/csmi_bb_serialization.c" \
                "src/inv/src/csmi_inv_serialization.c" \
                "src/diag/src/csmi_diag_serialization.c" \
                "src/launch/src/csmi_launch_serialization.c"\
                "src/common/src/csmi_common_serial.c"\
                "src/ras/src/csmi_ras_serialization.c")

# The define guards for files.
OUTPUT_DEFS_TYPES=( "_CSMI_WM_TYPES_H_"\
                    "_CSMI_BB_TYPES_H_"\
                    "_CSMI_INV_TYPES_H_"\
                    "_CSMI_DIAG_TYPES_H_"\
                    "_CSMI_LAUNCH_TYPES_H_"\
                    "_CSMI_COMMON_TYPES_H_"\
                    "_CSMI_RAS_TYPES_H_")
                    
# The define guards for the functions.
OUTPUT_DEFS_FUNCTS=( "_CSMI_WM_TYPE_FUNCTS_H_" \
                     "_CSMI_BB_TYPE_FUNCTS_H_" \
                     "_CSMI_INV_TYPE_FUNCTS_H_" \
                     "_CSMI_DIAG_TYPE_FUNCTS_H_" \
                     "_CSMI_LAUNCH_TYPE_FUNCTS_H_" \
                     "_CSMI_COMMON_TYPE_FUNCTS_H_" \
                     "_CSMI_RAS_TYPE_FUNCTS_H_" )

# The define guards for internal files.
OUTPUT_DEFS_INTERNAL=( "_CSMI_WM_INTERNAL_H_"\
                    "_CSMI_BB_INTERNAL_H_"\
                    "_CSMI_INV_INTERNAL_H_"\
                    "_CSMI_DIAG_INTERNAL_H_"\
                    "_CSMI_LAUNCH_INTERNAL_H_"\
                    "_CSMI_COMMON_INTERNAL_H_"\
                    "_CSMI_RAS_INTERNAL_H_")


# The API header files.
API_HEADERS=( "csm_api_workload_manager.h" \
              "csm_api_burst_buffer.h" \
              "csm_api_inventory.h" \
              "csm_api_diagnostics.h" \
              "" \
              "" \
              "csm_api_ras.h" )

# The internal C files.
O_FILE_C_INT=("src/wm/src/csmi_wm_internal.c" \
                "src/bb/src/csmi_bb_internal.c" \
                "src/inv/src/csmi_inv_internal.c" \
                "src/diag/src/csmi_diag_internal.c" \
                "src/launch/src/csmi_launch_internal.c"\
                "src/common/src/csmi_common_internal.c"\
                "src/ras/src/csmi_ras_internal.c")

# The internal C files.
O_FILE_C_PY=("src/wm/src/csmi_wm_python.cc" \
                "src/bb/src/csmi_bb_python.cc" \
                "src/inv/src/csmi_inv_python.cc" \
                "src/diag/src/csmi_diag_python.cc" \
                "src/launch/src/csmi_launch_python.cc"\
                "src/common/src/csmi_python.cc"\
                "src/ras/src/csmi_ras_python.cc")

# The internal header files.
O_FILE_H_INT=("src/wm/include/csmi_wm_internal.h" \
                "src/bb/include/csmi_bb_internal.h" \
                "src/inv/include/csmi_inv_internal.h" \
                "src/diag/include/csmi_diag_internal.h" \
                "src/launch/include/csmi_launch_internal.h"\
                "src/common/include/csmi_common_internal.h"\
                "src/ras/include/csmi_ras_internal.h")

# The internal type header files.
O_FILE_H_TYPE_INT=("src/wm/include/csmi_wm_type_internal.h" \
                "src/bb/include/csmi_bb_type_internal.h" \
                "src/inv/include/csmi_inv_type_internal.h" \
                "src/diag/include/csmi_diag_type_internal.h" \
                "src/launch/include/csmi_launch_type_internal.h"\
                "src/common/include/csmi_common_type_internal.h"\
                "src/ras/include/csmi_ras_type_internal.h")

# The define guards for internal files.
OUTPUT_DEFS_TYPE_INTERNAL=( "_CSMI_WM_TYPE_INTERNAL_H_"\
                    "_CSMI_BB_TYPE_INTERNAL_H_"\
                    "_CSMI_INV_TYPE_INTERNAL_H_"\
                    "_CSMI_DIAG_TYPE_INTERNAL_H_"\
                    "_CSMI_LAUNCH_TYPE_INTERNAL_H_"\
                    "_CSMI_COMMON_TYPE_INTERNAL_H_"\
                    "_CSMI_RAS_TYPE_INTERNAL_H_")

# The python API modules
PYTHON_API_MODULES=( "csmi_wm"     \
                     "csmi_bb"     \
                     "csmi_inv"    \
                     "csmi_diag"   \
                     "csmi_launch" \
                     "csmi_common" \
                     "csmi_ras" )

# The name of the python binding file.
PYTHON_DIR="${BASE_DIR}/python_bindings/"

