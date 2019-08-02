#================================================================================
#
#    buckets/basic/python_libraries.sh
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

# Bucket for CSM python library testing
# run through CSM FVT python scripts that utilize CSM python libraries

# Try to source the configuration file to get global configuration variables
if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

# Local Variables
LOG=${LOG_PATH}/buckets/basic/python_libraries.log
TEMP_LOG=${LOG_PATH}/buckets/basic/python_libraries_temp.log
FLAG_LOG=${LOG_PATH}/buckets/basic/python_libraries_flags.log

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "             Starting Python Libraries Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Test Case 1: Inventory Library - fvt_node_attributes_query_and_update.py
${FVT_PATH}/buckets/basic/fvt_node_attributes_query_and_update.py ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 1: Inventory Library - fvt_node_attributes_query_and_update.py"

# Test Case 2: Workload Manager Library - fvt_allocation_create_and_delete.py
${FVT_PATH}/buckets/basic/fvt_allocation_create_and_delete.py ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 2: Workload Manager Library - fvt_allocation_create_and_delete.py"

rm -f ${TEMP_LOG}

echo "------------------------------------------------------------" >> ${LOG}
echo "             Python Libraries Bucket COMPLETED" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
exit 0
