#/bin/bash

# check input file argument present
if [ -z $1 ]; then
    echo "ERROR: missing filename! Usage:  $0 <filename>"
    exit 1
else
    echo "Processing file: $1"
fi


filename=$1

#check file exists and file type is file
if [ ! -f ${filename} ]; then
    echo "ERROR: file not found or not a regular file: ${filename}"
    exit 1
fi


#check file name extension to define commenting chars
echo ${filename} | grep -e "\.c$" -e "\.cc$" -e "\.h$" &> /dev/null && FILETYPE="C"
echo ${filename} | grep -e "\.cmake$" -e "CMakeLists\.txt$" &> /dev/null && FILETYPE="CMAKE"
echo ${filename} | grep -e "\.sh$" &> /dev/null && FILETYPE="SHELL"
echo ${filename} | grep -e "\.sql$" &> /dev/null && FILETYPE="SQL"
echo ${filename} | grep -e "\.pl$" &> /dev/null && FILETYPE="PERL"

case ${FILETYPE} in
    "C") 
        MLINE_COMMENT_BEG="/*"
        MLINE_COMMENT_END="*/"
        LINE_COMMENT=""
        ;;
    "CMAKE")
        MLINE_COMMENT_BEG="#"
        MLINE_COMMENT_END=" "
        LINE_COMMENT="#"
        ;;
    "SHELL")
        MLINE_COMMENT_BEG="#"
        MLINE_COMMENT_END=" "
        LINE_COMMENT="#"
        ;;
    *)
        echo "UNRECOGNIZED/UNIMPLEMENTED FILE EXTENSION."
        exit 1
        ;;
esac


read -d '' JUST_FILENAME <<-EOF
${MLINE_COMMENT_BEG}================================================================================
${LINE_COMMENT}
${LINE_COMMENT}    $filename
EOF

read -d '' HEADER_CONST  <<- EOF 
${LINE_COMMENT}\ \ © Copyright IBM Corporation 2015-2017. All Rights Reserved
${LINE_COMMENT}
${LINE_COMMENT}    This program is licensed under the terms of the Eclipse Public License
${LINE_COMMENT}    v1.0 as published by the Eclipse Foundation and available at
${LINE_COMMENT}    http://www.eclipse.org/legal/epl-v10.html
${LINE_COMMENT}
${LINE_COMMENT}    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
${LINE_COMMENT}    restricted by GSA ADP Schedule Contract with IBM Corp.
${LINE_COMMENT}
${LINE_COMMENT}================================================================================${MLINE_COMMENT_END}
EOF



check_header()
{
    HAS_HEADER=0
    head -n 20  $filename | grep "Copyright *IBM *Corporation.*All Rights Reserved" &> /dev/null && \
        echo "Found Copyright Header." && \
        HAS_HEADER=1
}

check_filename()
{
    FILENAME_MATCH=0
    FSTR=`grep ${filename} ${filename} | head -n 1 | sed s/"[ ]*"//g`
    if [ -z ${FSTR} ]; then
        echo "No Filename found"
    elif [ "${FSTR}" == "${filename}" ]; then
        echo "Filename matches."
        FILENAME_MATCH=1;
    else
        echo "Filename mismatch."
    fi
}

update_file()
{
    PERMISSIONS=`stat -c "%a" $2`
    echo "Updating file content of $2, permissions: $PERMISSIONS"
    mv $1 $2
    chmod ${PERMISSIONS} $2
}


update_filename()
{
    echo "${JUST_FILENAME}" > /tmp/intermediate_header_add.file
    echo "${LINE_COMMENT}" >> /tmp/intermediate_header_add.file
    sed -n '/Copyright *IBM *Corporation.*All Rights Reserved/,$ p' ${filename} >> /tmp/intermediate_header_add.file

    SED_RC=$?

    FL_NEW=`wc -l /tmp/intermediate_header_add.file | cut -d ' ' -f1`
    FL_OLD=`wc -l ${filename} | cut -d ' ' -f1`
    LINE_DIFF=$[ $FL_NEW - $FL_OLD ]
    if [[ $LINE_DIFF -lt -2 ]]; then
        echo "sed pattern match error. Skipping..."
        exit 1
    fi

    if [ $SED_RC -eq 0 ]; then
        update_file /tmp/intermediate_header_add.file ${filename}
    fi
}

add_header()
{
    echo "${JUST_FILENAME}" > /tmp/intermediate_header_add.file
    echo "${LINE_COMMENT}" >> /tmp/intermediate_header_add.file
    echo "${HEADER_CONST}" >> /tmp/intermediate_header_add.file
    cat ${filename} >> /tmp/intermediate_header_add.file

    if [ $? -eq 0 ]; then
        update_file /tmp/intermediate_header_add.file ${filename}
    fi
}


check_header

if [ $HAS_HEADER -eq 1 ]; then
    check_filename

    if [ $FILENAME_MATCH -eq 1 ]; then
        echo "Nothing to do."
        exit 0
    else
        echo "Filename doesn't match. Updating filename in header..."
        update_filename
        exit 1
    fi
fi

echo "Copyright header not found. Adding...."


add_header

exit 0
