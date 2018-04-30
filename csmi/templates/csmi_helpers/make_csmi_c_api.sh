#!/bin/bash
###########################################################
#     make_csmi_c_api.sh
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
# @Author John Dunham (jdunham@us.ibm.com)

usage()
{
    cat << EOF

    Usage: $0 options

    This script copies the hosts file to the group or nodelist supplied.

    OPTIONS
        -h                 Shows this message.
        -a                 The author's name.
        -e                 The email of the author.
        -n [function name] The function name, for use when the function has already been declared.
        -t [type]          The type of function. The following values are supported:
            bb - Burst Buffer
            di - Diagnostic
            iv - Inventory
            ra - RAS
            wm - Workload Manager
        -z                 The debug flag.
EOF
}

# Builds the actual API function this script generates.
build_function()
{
    trimmed_name=${1/csm_/}
    csmi_name=${1/csm_/csmi_}
    function_prototype=${2/;/}
    
    function_string="const static csmi_cmd_t ExpectedCmd = CSM_CMD_${trimmed_name};\n\n"
    function_string+="/** @brief Destroys the csm_api object, releasing the handle pointed "
    function_string+="to by the object.\n \*\n \*"
    function_string+=" @param csm_obj An object containing a pointer to a handle for the "
    function_string+="${trimmed_name}\n */\n"
    function_string+="void ${csmi_name}_destroy(csm_api_object *csm_obj);\n\n"
    function_string+="${function_prototype}\n{\n    return 0;\n}\n\n"
    function_string+="void ${csmi_name}_destroy(csm_api_object *csm_obj)\n{\n}\n"

    echo ${function_string} 
}

# Populates the files.
populate_template()
{
    temp_file="/tmp/template_builder"
    > ${temp_file}

    cp $1 ${temp_file}

    file_define="_${serial_header^^}_"
    file_define=${file_define/./_}

    file_target=${source_directory}

    file_includes=""

    case $3 in
        sh)
            file_name=${serial_header}
            file_target+="/include/${file_name}"
            includes=( ${INCLUDE_SERIAL_H[@]} )
            ;;
        sc)
            file_name=${2/csm/csmi}"_serialization.c"
            file_target+="/src/${file_name}"
            includes=( ${INCLUDE_SERIAL_C[@]} )
            file_includes=${INCLUDE_SERIAL_C_DEFAULT}
            ;;
        fc)
            file_name=${2/csm/csmi}".c"
            file_target+="/src/${file_name}"
            includes=( ${INCLUDE_FUNCT_C[@]}  "../include/${serial_header}")
            file_includes=${INCLUDE_FUNCT_C_DEFAULT}
            ;;
        *)
            includes=( )
            ;;
    esac 

    for include in ${includes[@]}
    do
        file_includes+="#include \"${include}\" \n"
    done

    sed -i -e "s!@FILE_NAME!${file_name}!g"  \
        -e "s!@FILE_DEFINE!${file_define}!g" \
        -e "s!@INCLUDE!${file_includes}!g"   \
        -e "s!@FUNCTIONS!""${4}""!g"         \
        -e "s!@AUTHOR!${author}!g"           \
        -e "s!@EMAIL!${author_email}!g"      \
        ${temp_file}
    
    if [ $debug == 0 ]
    then
        if [ -z ${file_target} ]
        then
            echo "FILE EXISTS!"
            mv ${file_target} ${file_target}"_$(date +%s)"
        fi

        mv ${temp_file} ${file_target}
        echo ${file_target} >> ${touched_files}
    else
        printf "This would move ${temp_file} \n${file_target}\n\n"
        cat ${temp_file}
    fi
}

# Csmi files:
csmi_home=`cd ../..; pwd`
csmi_include="${csmi_home}/include"
source_directory="${csmi_home}/src/"

# Transaction files:
touched_files="/tmp/touched_$(whoami)"

> ${touched_files}

# Function Details:
function_name=0
function_type=0
function_ini=""
function_signature=""
author="@AUTHOR"
author_email="@EMAIL"

debug=0

# Parse Arguments
# ===============

while getopts "hn:t:a:e:z" OPTION
do
    case ${OPTION} in 
        a)
            author=${OPTARG}
            ;;
        e)
            author_email=${OPTARG}
            ;;
        n)
            function_name=${OPTARG}
            ;;
        t)
            function_type=${OPTARG}
            ;;
        z) 
            debug=1
            ;;
        h) 
            usage;
            exit 1 ;;
    esac
done

# ===============


# Get the function type and load the variables.
# =============================================

case ${function_type} in
    bb) 
        function_ini="types/burst_buffer.ini";;
    di)
        function_ini="types/diag.ini";;
    iv)
        function_ini="types/inventory.ini";;
    ra)
        function_ini="types/ras.ini";;
    wm)
        function_ini="types/wm.ini";;
    *)
        echo "Invalid function type!"; usage; exit 1 ;;
esac

source ${function_ini}

# Find the source directory
source_directory+=${SRC_DIR}

echo "Building csmi ${LONG_NAME} function:"

# =============================================

# Extract the function signature from the header api.
# ===================================================
if [[ ${function_name} != 0 ]]
then    
    function_pattern=".*[ ]${function_name}\(.*"
    function_signature=$(awk "/${function_pattern}/,/;/ { print }"  ${csmi_include}/${HEADER_API})


    if [[ ${function_signature} != "" ]]
    then
        echo "Found the function signature in ${csmi_include}/${HEADER_API}:"
        echo "${function_signature}"
        echo
    else
        echo "Failed to find a signature matching ${function_name}."
        echo "Specify the function in ${csmi_include}/${HEADER_API}."
        exit 1
    fi


else
    echo "A function name must be specified!" 
    exit 1
fi

# Make the Templates
serial_template_h="templates/csmi_serialization_template.h"
serial_template_c="templates/csmi_serialization_template.c"
serial_fuction_template="templates/serialization_functions"
function_template="templates/csmi_function_template.c"
default_ini="types/defaults.ini"

serial_header=${function_name/csm/csmi}"_serialization.h"
cmd_name="CSM_CMD_${function_name/csm_/}"
struct_name="${function_name}_t"
source ${default_ini}

# Source the serial functions.
source ${serial_fuction_template}
populate_template ${serial_template_c} ${function_name} "sc" "${serial_functions}"

# Build the serial header
serial_functions=$(echo "${serial_functions}" | sed 's!\\n{[^}]*}!;!g')
populate_template ${serial_template_h} ${function_name} "sh" "${serial_functions}"
 
# Stub out the function
function_implementation=$(build_function ${function_name} "${function_signature}") 
populate_template ${function_template} ${function_name} "fc" "${function_implementation}"

# =======================================================================================

# Add the serial definitions to csmi_serialization.c
serial_file="${csmi_home}/src/common/src/csmi_serialization.c"
include_tag="\@Automated_Includes"
include_str="#include \\\"csmi/src/${SRC_DIR}/include/${serial_header}\\\""

if [ $( grep -c ${cmd_name} ${serial_file}) -eq 0 ]
then
    awk "/\@${LONG_NAME}/{print; print \"  ${serial_callback}\";next}1" ${serial_file} > temp

    if [ ${debug} -eq 0 ] 
    then
        mv temp ${serial_file}
        echo ${serial_file} >> ${touched_files}
    else
        echo ${serial_callback}
        echo "Debugging, this file is too long to output to console. ${serial_file}"
        echo ""
    fi
else
    echo "${serial_callback} has already been defined."
fi

if [ $( grep -c "${include_str}" ${serial_file}) -eq 0 ]
then
    awk "/${include_tag}/{print; print \"${include_str}\";next}1" ${serial_file} > temp

    if [ ${debug} -eq 0 ] 
    then
        mv temp ${serial_file}
        echo ${serial_file} >> ${touched_files}
    else
        echo ${include_str}
        echo "Debugging, this file is too long to output to console. ${serial_file}"
        echo ""
    fi
else
    echo "${include_str} has already been defined."
fi


# =======================================================================================

# csmi_cmd_t addition.
cmd_def_file="${csmi_home}/src/common/include/csmi_cmds_def.h"
cmd_def="cmd(${function_name/csm_/})"

if [ $(grep -c ${cmd_def} ${cmd_def_file}) -eq 0 ]
then
    awk "/${LONG_NAME}/{print; print \"${cmd_def}\";next}1" ${cmd_def_file} > temp 
    
    if [ ${debug} -eq 0 ] 
    then
        mv temp ${cmd_def_file}
        echo ${cmd_def_file} >> ${touched_files}
    else
        echo ${cmd_def}
        echo "Debugging, this file is too long to output to console. ${cmd_def_file}"
        echo ""
    fi
else
    echo "API Definition already found for ${function_name}."
fi

# =======================================================================================

# Generate the structs.

api_header="${csmi_include}/csm_api.h"
api_insert_tag="@Automatic_Structs"
struct_stub="\n \\** @brief FIXME Stub! */\ntypedef struct {\n/* FIXME AUTOMATED STUB! - ${author} */\n}${struct_name};\n\n"


if [ $(grep -c ${struct_name} ${api_header} ) -eq 0 ]
then
    awk "/${api_insert_tag}/{print; print \"${struct_stub}\";next}1" ${api_header} > temp
    
    if [ ${debug} -eq 0 ]
    then
        mv temp ${api_header}
        echo ${api_header} >> ${touched_files}
    else
        printf "${struct_stub}"
        echo "Debugging, this file is too long to output to console.${api_header}."
        echo ""
    fi
else
    echo "${struct_name} was already defined."
fi

# =======================================================================================

# Generate the test files.

# TODO
# =======================================================================================

# Update the CMAKE
#TODO
# =======================================================================================

cat << EOF

${function_name} has been stubbed out. The following files were modified:
$(cat ${touched_files})

Next steps are as follows:
1. Add structs to ${csmi_include}/csm_api.h
2. Add test cases.
3. Implement the back half of the CSM API for this API.
EOF

rm -f ${touched_files}
