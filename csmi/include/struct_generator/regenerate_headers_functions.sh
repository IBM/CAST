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

# Generates the md5 sum file and changelog for files that were changed.
# Consult function body for parameter documentation. A string assumed past postion 7 containing 
# the changelog information.
finalize()
{
    target_dir=$1    # Target directory to write to.
    file_name=$2     # Name of the file that the md5sum was calculated for.
    release_count=$3 # Number of fields in the previous release, if -1, assumed to be new.
    count=$4         # Number of fields in this release.
    hash_code=$5     # The hash code of this release.
    type_name=$6     # Name of the type being finalized.
    difference=$(( count - release_count ))

    # Write the md5 sum to the type order file.
    echo ${file_name} ${count} ${hash_code} >> ${target_dir}/type_order.def_temp

    # If the difference was not zero, report the changes.
    if [[ ${difference} -gt 0 ]]
    then
        echo "### ${type_name}" >> ${target_dir}/change.log
        if [[  ${release_count} -ne -1 ]]
        then
            echo "**Added ${difference}**${@:7}" >> ${target_dir}/change.log
        else
            echo "**New Data Type**${@:7}" >> ${target_dir}/change.log
        fi
        echo "" >> ${target_dir}/change.log
    fi
}

# Clears the versioning details out of the files (generally for major version upgrades).
# $1 - Index of the API group to clear.
clear_files()
{
    index=$1 # The index to clear the files for.
    target_struct_dir=${BASE_DIR}/${dir}
    target_enum_dir=${BASE_DIR}/${ENUM_DIRS[${index}]}

    sed -i 's:def.*:def:g' $target_struct_dir/type_order.def
    sed -i 's:def.*:def:g' $target_enum_dir/type_order.def
}

# Generates the headers and c files for the CSM API structs.
# $1 - Index of the API group being generated.
build_files()
{
    index=$1

    # Doxygen grouping
    doxy_group=${API_GROUPS[${index}]}
    doxy_long=${API_GROUPS_LONG[${index}]}

    # Define Directories
    target_struct_dir=${BASE_DIR}/${dir}
    target_enum_dir=${BASE_DIR}/${ENUM_DIRS[${index}]}

    # File names
    o_file_types=${OUTPUT_FILE_TYPES[${index}]}
    o_file_types_int=${O_FILE_H_TYPE_INT[${index}]}
    o_file_functs=${OUTPUT_FILE_FUNCTS[${index}]}
    o_file_c=${OUTPUT_FILE_C[${index}]}
    o_file_h_int=${O_FILE_H_INT[${index}]}
    o_file_c_int=${O_FILE_C_INT[${index}]}
    api_file=${API_HEADERS[${index}]}
    py_file=${CSMI_DIR}/${O_FILE_C_PY[${index}]}
    sed -i "/STRUCTS_BEGIN/q" $py_file

    #python_module=${PYTHON_API_MODULES[${index}]}
    #python_file="${PYTHON_DIR}${python_module}.py"
    #sed "s/MODULE/${python_module}/g" csmi.py > ${python_file}
    
    # Erase Flags
    o_types_written=0
    o_funct_written=0
    o_c_written=0

    # Initialize the types header.
    build_copyright_header "include/${o_file_types}"> ${OUTPUT_DIR}/${o_file_types}
    build_struct_header_doc ${o_file_types} ${doxy_group} >> ${OUTPUT_DIR}/${o_file_types}
    build_struct_defines ${OUTPUT_DEFS_TYPES[${index}]} >> ${OUTPUT_DIR}/${o_file_types}
    
    # Add the special includes and preprocessor directives
    if [ -e ${target_struct_dir}/special_preprocess.def ]
    then
        echo "// Begin special_preprocess directives" >> ${OUTPUT_DIR}/$o_file_types
        cat ${target_struct_dir}/special_preprocess.def >>  ${OUTPUT_DIR}/$o_file_types
        echo "// End special_preprocess directives" >> ${OUTPUT_DIR}/$o_file_types
        echo "" >> ${OUTPUT_DIR}/$o_file_types
    fi

    # Initialize the functs header.
    build_copyright_header "include/${o_file_functs}" > ${OUTPUT_DIR}/${o_file_functs}
    build_function_header_doc ${o_file_functs} ${doxy_group} >> ${OUTPUT_DIR}/${o_file_functs}
    echo "#include \"${o_file_types}\"" >> ${OUTPUT_DIR}/${o_file_functs}
    build_struct_defines ${OUTPUT_DEFS_FUNCTS[${index}]} >> ${OUTPUT_DIR}/${o_file_functs}

    build_copyright_header "${o_file_c}" > ${CSMI_DIR}/${o_file_c}
    build_c_inc ${INCLUDE_DIR}${o_file_functs} >> ${CSMI_DIR}/${o_file_c}
    echo '#include "'${MACROS_DIR}${MACROS}'"' >> ${CSMI_DIR}/${o_file_c}
    echo '#include "'csmi/${o_file_types_int}'"' >>  ${CSMI_DIR}/${o_file_c}

    # Initialize the internal type header.
    build_copyright_header  ${o_file_types_int} > ${CSMI_DIR}/${o_file_types_int}
    build_struct_defines ${OUTPUT_DEFS_TYPE_INTERNAL[${index}]} >> ${CSMI_DIR}/${o_file_types_int}
    echo '#include "'${INCLUDE_DIR}${o_file_types}'"' >> ${CSMI_DIR}/${o_file_types_int} 

    # Initialize the internal header.
    build_copyright_header  ${o_file_h_int} > ${CSMI_DIR}/${o_file_h_int}
    build_struct_defines ${OUTPUT_DEFS_INTERNAL[${index}]} >> ${CSMI_DIR}/${o_file_h_int}
    echo '#include "'${INCLUDE_DIR}${o_file_types}'"' >> ${CSMI_DIR}/${o_file_h_int} 
    echo '#include "'${STRUCT_HASH}'"' >> ${CSMI_DIR}/${o_file_h_int}
    
    # Initialize the interal C file.
    build_copyright_header  ${o_file_c_int} > ${CSMI_DIR}/${o_file_c_int}
    echo '#include "'csmi/${o_file_h_int}'"' >>  ${CSMI_DIR}/${o_file_c_int}
    echo '#include "'csmi/${o_file_types_int}'"' >>  ${CSMI_DIR}/${o_file_c_int}

    # Build the enumerated directory. 
    if [ -e ${target_enum_dir}/type_order.def ]
    then
        if [ ${finalize} -eq 1 ]
        then
            >${target_enum_dir}/type_order.def_temp
            echo "" > ${target_enum_dir}/change.log
            echo "## ${doxy_long} " >> ${target_enum_dir}/change.log
        fi

        while read -r -u4 file enums hash_code
        do
            # Skip blank lines.
            [ -z ${file} ] && continue

            def_file=${target_enum_dir}/${file}
            if [ ! -f ${def_file} ] 
            then
                echo "File Not Found: ${def_file}" 
                ((missing_count+=1))
                continue
            fi

            ((o_types_written+=1))
            ((o_c_written+=1))

            # If the enum or hash code were not set, assume the data is invalid.
            if [[ $enums == "" || $hash_code == "" ]]
            then
                enums=-1
                hash_code=-1
            fi

            # Attempt to generate the enumerated types, caching the changes.
            hash_str=$(./generate_enum.pl ${def_file} ${OUTPUT_DIR}/${o_file_types} \
                ${CSMI_DIR}/${o_file_c} ${finalize} ${enums} ${hash_code} $py_file)         

            if [ $? -eq 0 ]
            then
                if [ ${finalize} -eq 1 ]
                then
                    finalize ${target_enum_dir} ${file} ${enums} ${hash_str}
                fi
            else
                echo "Failure Detected in ${def_file}!"
                echo ${hash_str};
                ((failure_count+=1))
            fi
        done 4<${target_enum_dir}/type_order.def
    fi

    # Define the actual structs.
    if [ -e ${target_struct_dir}/type_order.def ]
    then
        if [ ${finalize} -eq 1 ]
        then
            >${target_struct_dir}/type_order.def_temp
            echo "" > ${target_struct_dir}/change.log
            echo "## ${doxy_long} " >> ${target_struct_dir}/change.log
        fi

        while read -r -u4 file fields hash_code
        do
            # Skip blank lines.
            [ -z ${file} ] && continue

            def_file=${target_struct_dir}/${file}
            if [ ! -f ${def_file} ]
            then
                echo "File Not Found: ${def_file}" 
                ((missing_count+=1))
                continue
            fi

            ((o_types_written+=1))
            ((o_funct_written+=1))
            ((o_c_written+=1))

            # If the fields or hash code were not set, assume the data is invalid.
            if [[ $fields == "" || $hash_code == "" ]]
            then
                fields=-1
                hash_code=-1
            fi

            # Attempt to generate the struct types, caching the changes.
            hash_str=$(./generate_struct.pl ${def_file} ${OUTPUT_DIR}/${o_file_types} \
                ${OUTPUT_DIR}/${o_file_functs} ${CSMI_DIR}/${o_file_c} \
                ${INCLUDE_DIR}${def_file#*/} ${finalize} ${fields} ${hash_code}\
                ${CSMI_DIR}/${o_file_h_int} ${CSMI_DIR}/${o_file_c_int}\
                ${CSMI_DIR}/${o_file_types_int} $py_file)

            if [ $? -eq 0 ]
            then
                if [ ${finalize} -eq 1 ]
                then
                    finalize ${target_struct_dir} ${file} ${fields} ${hash_str}
                fi
            else
                echo "Failure Detected in ${def_file}!"
                echo ${hash_str};
                ((failure_count+=1))
            fi
        done 4<${target_struct_dir}/type_order.def
    fi
    
    #if [ -f ${BASE_DIR}/${api_file} ]
    #then
    #    echo "Pythonizing ${BASE_DIR}/${api_file}"
    #    echo "# Function bindings" >> scratch.py
    #    echo "######################################"\
    #        "##########################################" >> scratch.py
    #
    #    ./function_parse.pl  ${BASE_DIR}/${api_file} ${python_module} >> scratch.py
    #    #pythonize_functs ${BASE_DIR}/${api_file}
    #fi

    ## Write the python bindings
    #cat scratch.py >> ${python_file}
    #>scratch.py
    echo "};" >> $py_file
    
    # Complete the file header. 
    echo "/** @} */" >> ${OUTPUT_DIR}/${o_file_types}
    build_header_end >> ${OUTPUT_DIR}/${o_file_types}
    build_header_end >> ${OUTPUT_DIR}/${o_file_functs}
    build_header_end >> ${CSMI_DIR}/${o_file_h_int}
    build_header_end >> ${CSMI_DIR}/${o_file_types_int}

    # If a file hasn't been written to, remove it.
    if [ ${o_types_written} -eq 0 ]
    then
        rm -f ${OUTPUT_DIR}/${o_file_types}
    fi
    
    if [ ${o_funct_written} -eq 0 ]
    then
        rm -f ${OUTPUT_DIR}/${o_file_functs}
        sed -i "s:${INCLUDE_DIR}${o_file_functs}:${INCLUDE_DIR}${o_file_types}:g" ${CSMI_DIR}/${o_file_c}

    fi
    
    if [ ${o_c_written} -eq 0 ]
    then
        rm -f ${CSMI_DIR}/${o_file_c}
    fi
    
    echo "Completed header generation for ${doxy_long}; Failures: ${failure_count}; Missing: ${missing_count};"

    if [[  ${failure_count} -gt 0  || ${missing_count} -gt 0 ]]
    then
        touch $FAILURE_FILE
    fi 

    exit ${failure_count}
}


# Processes the versioning transition, verifying the deltas are legal. Then generates the id.
# TODO this is uses too much global space code?
process_verision_delta()
{
    # Compute the deltas.
    major_delta=$(( ${major_vers} - ${major} ))
    minor_delta=$(( ${minor_vers} - ${minor} ))
    ptf_delta=$((   ${ptf_vers}   - ${ptf}   ))
    total_delta=$(( ${major_delta} + ${minor_delta} + ${ptf_delta} ))
    
    # EARLY RETURN: If theres no delta just continue execution.
    if [[ ${total_delta} -eq 0 ]]
    then
        return
    fi

    # Cache the old version, then build the new active version.
    old_version=${active_version}
    active_version="${VERSION_PREFIX}${major_vers}_${minor_vers}_${ptf_vers}"
    
    # Scan the changes and verify the deltas are legal.
    if [[ $major_delta -eq 1 ]] 
    then 
        if ! [[ ${ptf_vers} -eq 0 && ${minor_vers} -eq 0 ]] 
        then
            echo "ERROR: Invalid version transition, "
            echo "        when changing major versions, minor and ptf versions must be zeroed."
            echo "        ( Current Version: ${old_version} )"
            echo "        ( Target  Version: ${major_vers}_${minor_vers}_${ptf_vers} )"
            exit 1
        fi 
          
        # Min version changes with major version headers.
        sed -i "s:.*define ${MIN_VERSION}.*:#define ${MIN_VERSION} ${active_version}:1" \
            ${VERSION_HEADER}
         
    elif [[ ${minor_delta} -eq 1 ]]
    then 
        if ! [[ ${major_delta} -eq 0 && ${ptf_vers} -eq 0 ]]
        then
            echo "ERROR: Invalid version transition, "
            echo "        when changing minor versions, major versions can't be changed and "
            echo "        ptf version must be zeroed."
            echo "        ( Current Version: ${old_version} )"
            echo "        ( Target  Version: ${major_vers}_${minor_vers}_${ptf_vers} )"
            exit 1
        fi
    elif [[ ${ptf_delta} -eq 1 ]]
    then
        if [[ ${ptf_delta} -ne ${total_delta} ]]
        then
            echo "ERROR: Invalid version transition, "
            echo "        when changing ptf versions no other versions may change."
            echo "        ( Current Version: ${old_version} )"
            echo "        ( Target  Version: ${major_vers}_${minor_vers}_${ptf_vers} )"
            exit 1
        fi
    else
        if [[ ${total_delta} -gt 0 ]]
        then
            echo "ERROR: Version supplied is not sequentially following the current version!"
        else
            echo "ERROR: Version supplied is back level!"
        fi
        echo "        ( Current Version: ${old_version} )"
        echo "        ( Target  Version: ${major_vers}_${minor_vers}_${ptf_vers} )"
        exit 1
    fi

    # Compose the version number.
    active_version_num=$(( (${ptf_vers} << ${PTF_VERSION_SHIFT}) \
        + (${minor_vers} << ${MINOR_VERSION_SHIFT})              \
        + (${major_vers} << ${MAJOR_VERSION_SHIFT}) ))

    # Output the versioning.
    sed -i "s:.*define ${VERSION_ID}.*:#define ${VERSION_ID} ${active_version}:1" \
        ${VERSION_HEADER}
    sed -i "/${VERSION_INSERT}/ a #define ${active_version} ${active_version_num}" ${VERSION_HEADER}
}

