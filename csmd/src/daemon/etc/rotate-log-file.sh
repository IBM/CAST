#! /bin/bash

#================================================================================
#
#    csmd/src/daemon/etc/rotate-log-file.sh
#
#  © Copyright IBM Corporation 2015-2017. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================

FILES_TO_SAVE=5
PERCENTAGE_USED_LIMIT=90

# Script requires exactly one parameter, the path to the configuration file that contains the log file name
if [ "$#" -ne 1 ]; then
    echo "Invalid number of parameters"
    echo "Usage: rotate-log-file.sh PATH_TO_CSM_CONFIG_FILE"
    exit 1
fi

CONFIG_FILE=$1

# Check to see if the CONFIG_FILE exists
if [ -e ${CONFIG_FILE} ]
then
    :
    #echo "${CONFIG_FILE}"
else
    echo "Could not find ${CONFIG_FILE}"
    exit 2
fi

# Retrieve log file name from configuration file
# Look for a pattern like this:
#                  ... "fileLog" : "/var/log/ibm/csm/csm_master.log" ...
#                  ^---------------^^-----------------------------^^---^
#            Match and remove this  |         Keep this           | Match and remove this
# -e 's/^.*"fileLog"[ ]*:[ ]*"//'                   -e 's/".*//'
LOG_FILE=`cat ${CONFIG_FILE} | tr -d "\n" | sed -e 's/^.*"fileLog"[ ]*:[ ]*"//' -e 's/".*//'`

# Check to see if the LOG_FILE exists, if not, silently exit
if [ -e ${LOG_FILE} ]
then
    :
    #echo "${LOG_FILE}"
else
    # This may or may not be an error, so just assume there is no rotation needed and exit
    exit 0 
fi

# Get the percent used of the partition where the log file resides
if DF_OUTPUT=$(df --direct --output=pcent -k ${LOG_FILE})
then
   #echo "${DF_OUTPUT}"
   PERCENT_USED_STRING=$(echo "${DF_OUTPUT}" | tail -n 1)
   PERCENT_USED=${PERCENT_USED_STRING%\%}
else
   echo "df failure, exit status $?"
   exit 3
fi

if (( ${PERCENT_USED} > ${PERCENTAGE_USED_LIMIT} )) ; then
    echo "Suppressing rotation due to lack of available space."
    exit 4
fi

# Rotate the files in a loop, starting with the oldest by name 
i=$FILES_TO_SAVE
j=$(($i-1))
while [ $i -ge 1 ]; do
    if [ $i -ne 1 ] ; then
        if [ -e "${LOG_FILE}.old.$j" ] ; then
            mv "${LOG_FILE}.old.$j" "${LOG_FILE}.old.$i"
        fi
    else
        if [ -e "${LOG_FILE}" ] ; then
            mv "${LOG_FILE}" "${LOG_FILE}.old.$i"
        fi
    fi
 
    i=$(($i-1))
    j=$(($i-1))
done
