#!/bin/sh
# encoding: utf-8
#================================================================================
#
#    allocation_timing.py 
#
#  © Copyright IBM Corporation 2015-2020. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================

# The beginning of this script is both valid shell and valid python,
# such that the script starts with the shell and is reexecuted with
# the right python.
#
# On RHEL 7, CSM uses a version of boost-python dependent on python2
# On RHEL 8, CSM uses a version of boost-python dependent on python3
# The intent is to run as python3 on RHEL8 and python2 on RHEL7
#
'''which' python3 > /dev/null 2>&1 && exec python3 "$0" "$@" || exec python2 "$0" "$@"
'''

import sys
import os

# Add the python library to the path
sys.path.append('/opt/ibm/csm/lib')

# Import libraries
import lib_csm_py as csm
import lib_csm_inv_py as inv
import lib_csm_wm_py as wm
from pprint import pprint
from datetime import datetime

# Init CSM LIB object
csm.init_lib()

# Read log path and compute nodes from config file
configf = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir, "csm_test.cfg"))
config = open(str(configf),"r")
for line in config:
    if "LOG_PATH" in line: 
        log_path = line[16:len(line)-1]
    if "COMPUTE_NODES" in line:
        compute_nodes = line[22:len(line)-2]
config.close()

# Open Log file
log = open(str(log_path) + "/buckets/advanced/allocation_timing.log","w")

# Create an allocation and capture timing information in log based on allocation_create_input_t object
def create_timed_allocation(alloc_input):
    start_time=datetime.now()
    rc,handler,aid=wm.allocation_create(alloc_input)
    end_time=datetime.now()
    time_to_create = end_time - start_time
    if (rc == 0):
        print("Created allocation: " + str(aid) + " (isolated_cores = " + str(alloc_input.isolated_cores) + ")")
        log.write("Allocation: " + str(aid) + " (isolated_cores = " + str(alloc_input.isolated_cores) + ")\n")
        print("Create time: " + str(time_to_create.total_seconds()))
        log.write("Create time: " + str(time_to_create.total_seconds()) + "\n")
        return aid
    else:
        print("Create Failed")
        csm.api_object_destroy(handler)
        csm.term_lib()
        sys.exit(rc)

# Delete allocation based on allocation_delete_input_t object
def delete_allocation(alloc_delete_input):
    rc,handler=wm.allocation_delete(alloc_delete_input)
    if (rc == 0):
        print("Allocation " + str(alloc_delete_input.allocation_id) + " successfully deleted")
    else:
        print("Failed to delete allocation " + str(alloc_delete_input.allocation_id))
        csm.api_object_destroy(handler)
        csm.term_lib()
        sys.exit(rc)

# Create node attributes query to identify nodes to use for allocation
input = inv.node_attributes_query_input_t()
nodes=[str(compute_nodes)]
input.set_node_names(nodes)
input.limit=-1
input.offset=-1
alloc_input=wm.allocation_t()
alloc_input.primary_job_id=1
alloc_input.job_submit_time="now"

# initialize node_list: List of nodes in service
node_list=[]

# Node Attributes Query
rc,handler,output = inv.node_attributes_query(input)

# Append names of nodes in service to node_list
if(rc == csm.csmi_cmd_err_t.CSMI_SUCCESS):
    for i in range (0, output.results_count):
        node = output.get_results(i)
        print("node: " + str(node.node_name))
        print("node state: " + str(node.state))
        if(str(node.state) == "CSM_NODE_IN_SERVICE"):
            node_list.append(str(node.node_name))
else:
    print("No matching records found.")
    csm.api_object_destroy(handler)
    csm.term_lib()
    sys.exit(rc)

# Initialize cores: Sequential list of isolated_cores input values
cores = [0, 0, 1, 0, 1, 2, 1, 0]

# Initialize allocation create input struct
alloc_input.set_compute_nodes(node_list)
alloc_input.state=wm.csmi_state_t.CSM_RUNNING

# Initialize allocation delete input struct
alloc_delete_input=wm.allocation_delete_input_t()

for i in cores:
    # Create Allocation with isolated_cores = cores[i]
    alloc_input.isolated_cores=i
    id=create_timed_allocation(alloc_input)
    # Delete Allocation
    alloc_delete_input.allocation_id=id
    delete_allocation(alloc_delete_input)


# Clean up handler and term lib
csm.api_object_destroy(handler)
log.close()
csm.term_lib()
