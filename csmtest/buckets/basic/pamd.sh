#!/bin/bash
#================================================================================
#
#    buckets/basic/pamd.sh
#
#    © Copyright IBM Corporation 2015-2019. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================

# Bucket for pam testing.
# csm_jsrun
# Used Variables:
# ==================================================
# - CSM_PATH
# - USER     - Needs sshkey auth to computenodes.
# - COMPUTE_NODES
# =================================================

clear_allocations()
{
    # Clean up the allocations.
    allocations=$(${CSM_PATH}/csm_allocation_query_active_all | grep allocation_id| tr -s " " | cut -d" " -f4)
    
    for allocation in ${allocations}
    do
        ${CSM_PATH}/csm_allocation_delete -a ${allocation} 
    done
    
    # Drop all of the nodes into in service
    ${CSM_PATH}/csm_node_attributes_update -n ${COMPUTE_NODES} -s IN_SERVICE
}


if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
    . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
    echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
    exit 1
fi

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
	exit 1
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "                 Starting pamd Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}


# Configure All of the nodes.
#----------------------------------------------------------------------------------------------
TEST_SCRIPT="/root/test.sh"
TEST_OUTPUT="/root/test.out"

# Setup the PAM modules
xdsh ${COMPUTE_NODES} "sed -i 's/^#\(.*libcsmpam.so\)/\1/g' /etc/pam.d/sshd;service sshd restart" > ${TEMP_LOG} 2>&1

# Distribute the test script.
xdsh ${COMPUTE_NODES} "echo '!/bin/bash' > ${TEST_SCRIPT}; echo 'printenv > ${TEST_OUTPUT} 2>&1; ulimit -a >> ${TEST_OUTPUT} 2>&1 >> ${TEST_SCRIPT}; chmod +x ${TEST_SCRIPT};" > ${TEMP_LOG} 2>&1

clear_allocations
#----------------------------------------------------------------------------------------------

# Test #1-3 - Create an allocation as a non root user ${USER}. 
alloc_id=$(${CSM_PATH}/csm_allocation_create -j 1 -n ${COMPUTE_NODES} -u ${USER} 2>/dev/null| grep allocation_id | cut -d " " -f 2)

ssh -l ${USER} ${SINGLE_COMPUTE} "echo test" > /dev/null
check_return_exit  $? 0 "Test Case 1: ${USER} allowed to login to node"

ssh -l ${SECOND_USER} ${SINGLE_COMPUTE} "echo test" > /dev/null
check_return_error $? 0 "Test Case 2: ${SECOND_USER} not allowed to login to node"

ssh -l root  ${SINGLE_COMPUTE} "echo test" > /dev/null
check_return_exit $? 0 "Test Case 3: ${SECOND_USER} allowed to login to node"

${CSM_PATH}/csm_allocation_delete -a ${alloc_id}

# Test 4-6 - Root user create after deleting allocation.
alloc_id=$(${CSM_PATH}/csm_allocation_create -j 1 -n ${COMPUTE_NODES} -u root 2>/dev/null| grep allocation_id | cut -d " " -f 2)

ssh -l ${USER} ${SINGLE_COMPUTE} "echo test" > /dev/null
check_return_error  $? 0 "Test Case 4: ${USER} not allowed to login to node"

ssh -l ${SECOND_USER} ${SINGLE_COMPUTE} "echo test" > /dev/null
check_return_error $? 0 "Test Case 5: ${SECOND_USER} not allowed to login to node"

ssh -l root  ${SINGLE_COMPUTE} "echo test" > /dev/null
check_return_exit $? 0 "Test Case 6: ${SECOND_USER} allowed to login to node"


# Test 7-9 - Add user to whitelist attempt to login.
xdsh ${COMPUTE_NODES} "echo ${USER} >> /etc/pam.d/csm/whitelist"

ssh -l ${USER} ${SINGLE_COMPUTE} "echo test" > /dev/null 
check_return_exit  $? 0 "Test Case 7: ${USER} allowed to login to node"

ssh -l ${SECOND_USER} ${SINGLE_COMPUTE} "echo test" > /dev/null
check_return_error $? 0 "Test Case 8: ${SECOND_USER} not allowed to login to node"

ssh -l root  ${SINGLE_COMPUTE} "echo test" > /dev/null
check_return_exit $? 0 "Test Case 9: ${SECOND_USER} allowed to login to node"

# Reset env.
${CSM_PATH}/csm_allocation_delete -a ${alloc_id}

# Test 10-12 - No allocation running, whitelist enabled.
ssh -l ${USER} ${SINGLE_COMPUTE} "echo test" > /dev/null
check_return_exit $? 0 "Test Case 10: ${USER} allowed to login to node"

ssh -l ${SECOND_USER} ${SINGLE_COMPUTE} "echo test" > /dev/null
check_return_error $? 0 "Test Case 11: ${SECOND_USER} not allowed to login to node"

ssh -l root  ${SINGLE_COMPUTE} "echo test" > /dev/null
check_return_exit $? 0 "Test Case 12: ${SECOND_USER} allowed to login to node"

# Test 12-14 No allocation running, whitelist disabled.
# Clear whitelist.
xdsh ${COMPUTE_NODES} "> /etc/pam.d/csm/whitelist"

ssh -l ${USER} ${SINGLE_COMPUTE} "echo test" > /dev/null
check_return_error  $? 0 "Test Case 13: ${USER} not allowed to login to node"

ssh -l ${SECOND_USER} ${SINGLE_COMPUTE} "echo test" > /dev/null
check_return_error $? 0 "Test Case 14: ${SECOND_USER} not allowed to login to node"

ssh -l root  ${SINGLE_COMPUTE} "echo test" > /dev/null
check_return_exit $? 0 "Test Case 15: ${SECOND_USER} allowed to login to node"


# TODO ULIMIT testing.

#----------------------------------------------------------------------------------------------

# Make sure we leave the allocations and nodes where we found them.
# TODO clean PAM modules.
clear_allocations


echo "------------------------------------------------------------" >> ${LOG}
echo "           Basic pamd Bucket Passed" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "Additional Flags:" >> ${LOG}
echo -e "${FLAGS}" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

rm -f ${TEMP_LOG}
exit 0
