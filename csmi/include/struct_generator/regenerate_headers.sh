#!/bin/bash
#================================================================================
#
#    csmi/include/regenerate_headers.sh
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
cd "$(dirname "$0")"

# Include the text generation functions and constants.
. ./regenerate_headers_constants.sh --source-only
. ./regenerate_headers_text_functs.sh --source-only
. ./regenerate_headers_functions.sh --source-only


# ==================================================================
# Parse the user's input.
optstring="hf:c"

finalize=0           # Flag indicating that the script should set the version.
version_code="0.0.0" # The version code for this run of the script.
major_vers=0         # The major version of the CSM APIs. 
minor_vers=0         # The minor version of the CSM APIs.
ptf_vers=0           # The ptf version of the CSM APIs.

active_version=$(grep -oP "CSM_VERSION_ID[ ]*\K.*" ${VERSION_HEADER})
active_version_num=$(grep -oP "${active_version}[ ]*\K.*" ${VERSION_HEADER})

# Parse the user input.
while getopts $optstring OPTION
do
    case $OPTION in
        h)
            usage; exit 1;;
        f)
            if [[ ${OPTARG} =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]
            then
                finalize=1
                version_code=${OPTARG}
                IFS=. read -r major_vers minor_vers ptf_vers <<< ${OPTARG}
                IFS=_ read -r j1 j2 major minor ptf <<< ${active_version}

                process_verision_delta
            else
                echo "ERROR: -f requires a version code #.#.#"
                usage
                exit 1
            fi

            ;;
        c)
            index=0
            for dir in ${STRUCT_DIRS[@]}
            do
                clear_files $index &
            
                ((index+=1))
            done
            wait 
            exit 0;;
    esac
done

# ==================================================================
# Build the headers for the serailization functions for use later.

# Compile the header functions.
cpp header_funct_def.txt -o .header_funct.txt.swp -I ${CSM_DIR}

# Build an array of the compiled functions.
S_IFS=$IFS
IFS=$'\n'
functs=($(grep STRUCT_NAME .header_funct.txt.swp ))
IFS=$S_IFS

rm -f .header_funct.txt.swp

# Clear the header_funct.txt field.
> header_funct.txt

# Iterate over the comments and build them with the function.
function_index=0
while read -r -u3 line  
do
    # If the line is empty insert the function.
    if [ "${line}" == "" ]
    then
        echo ${functs[$function_index]} >> header_funct.txt

        ((function_index+=1))
    fi
    echo "$line" >> header_funct.txt
done 3< header_funct_comments.txt
# ==================================================================

echo "Regenerating headers (should take about 2 seconds)"

failure_count=0
missing_count=0
index=0
for dir in ${STRUCT_DIRS[@]}
do
    build_files $index &

    ((index+=1))
done

wait

# TODO Clean this up.
./generate_print_formatter.pl c >> "${CSMI_DIR}/src/common/src/csmi_common_internal.c"

# TODO is there a way to better integrate this?
build_copyright_header "/src/common/include/csmi_common_generated.h" > "${CSMI_DIR}/src/common/include/csmi_common_generated.h"
build_struct_defines "_CSMI_COMMON_GENERATED_H_" >> "${CSMI_DIR}/src/common/include/csmi_common_generated.h"
./generate_print_formatter.pl h >> "${CSMI_DIR}/src/common/include/csmi_common_generated.h"
build_header_end >> "${CSMI_DIR}/src/common/include/csmi_common_generated.h"

# Determine Failure Count.
if [ -e $FAILURE_FILE ]
then
    echo "Completed with errrors."
    failure_count=1
    rm -f $FAILURE_FILE
fi

# Finalize the change log and save the new type orders.
if [[ ${finalize} -eq 1 && ${failure_count} -eq 0  ]]
then
    log_dir=${CHANGE_LOG_DIR}/${major_vers}
    change_log="${log_dir}/change_${major_vers}_${minor_vers}_${ptf_vers}.log"
    mkdir -p ${log_dir}

    # Generates the enum type change log.
    build_change_log ${version_code} > ${change_log}
    for dir in ${ENUM_DIRS[@]}
    do
        target_dir=${BASE_DIR}/${dir}
        mv ${target_dir}/type_order.def_temp ${target_dir}/type_order.def

        if [[ -e ${target_dir}/change.log  && $(wc -l < ${target_dir}/change.log ) -gt 2 ]]
        then
            cat ${target_dir}/change.log >> ${change_log}
        fi
        rm -f ${target_dir}/change.log
    done

    # Generates the struct type change log.
    echo ""               >> ${change_log}
    echo "# Struct Types" >> ${change_log}
    for sdir in ${STRUCT_DIRS[@]}
    do
        target_dir=${BASE_DIR}/${sdir}
        mv ${target_dir}/type_order.def_temp ${target_dir}/type_order.def

        if [[ -e ${target_dir}/change.log  && $(wc -l < ${target_dir}/change.log ) -gt 2 ]]
        then
            cat ${target_dir}/change.log >> ${change_log}
        fi
        rm -f ${target_dir}/change.log
    done

    sed -i "s:;\*:\n  *:g" ${change_log}

    sed -i "s:${DEV_VERSION}:${active_version}:g" ${BASE_DIR}/${TYPES_DIR}*/*/*.def
elif [[ ${finalize} -eq 1 ]]
then
    echo "Unable to finalize version, please consult error messages!"
    for dir in ${ENUM_DIRS[@]}
    do
        target_dir=${BASE_DIR}/${dir}
        rm -f ${target_dir}/type_order.def_temp
        rm -f ${target_dir}/change.log 
    done
    for dir in ${STRUCT_DIRS[@]}
    do
        target_dir=${BASE_DIR}/${dir}
        rm -f ${target_dir}/type_order.def_temp
        rm -f ${target_dir}/change.log 
    done
fi

# Remove header function file.
rm -f header_funct.txt

exit ${failure_count}
