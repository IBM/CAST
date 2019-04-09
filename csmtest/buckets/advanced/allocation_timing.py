#!/bin/python
# encoding: utf-8
#================================================================================
#
#    allocation_timing.py 
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

import sys

#add the python library to the path
sys.path.append('/opt/ibm/csm/lib')

import lib_csm_py as csm
import lib_csm_inv_py as inv
import lib_csm_wm_py as wm
from pprint import pprint
from datetime import datetime

csm.init_lib()

def create_timed_allocation(alloc_input):
    start_time=datetime.now()
    rc,handler,aid=wm.allocation_create(alloc_input)
    end_time=datetime.now()
    time_to_create = end_time - start_time
    if (rc == 0):
        print("Created allocation: " + str(aid) + " (isolated_cores = " + str(alloc_input.isolated_cores) + ")")
#        print("Start time: " + str(start_time))
#        print("End time: " + str(end_time))
        print("Create time: " + str(time_to_create.total_seconds()))
        return aid
    else:
        print("Create Failed")
        csm.api_object_destroy(handler)
        csm.term_lib()
        sys.exit(rc)

def delete_allocation(alloc_delete_input):
    rc,handler=wm.allocation_delete(alloc_delete_input)
    if (rc == 0):
        print("Allocation " + str(alloc_delete_input.allocation_id) + " successfully deleted")
    else:
        print("Failed to delete allocation " + str(alloc_delete_input.allocation_id))
        csm.api_object_destroy(handler)
        csm.term_lib()
        sys.exit(rc)

input = inv.node_attributes_query_input_t()
nodes=[str(sys.argv[1])]
node_list=[]
input.set_node_names(nodes)
input.limit=-1
input.offset=-1
alloc_input=wm.allocation_t()
alloc_input.primary_job_id=1
alloc_input.job_submit_time="now"

rc,handler,output = inv.node_attributes_query(input)

if(rc == csm.csmi_cmd_err_t.CSMI_SUCCESS):
    for i in range (0, output.results_count):
        node = output.get_results(i)
        print("node: " + str(node.node_name))
        print("node_state_value: " + str(node.state))
        if(str(node.state) == "CSM_NODE_IN_SERVICE"):
            node_list.append(str(node.node_name))

else:
    print("No matching records found.")
    csm.api_object_destroy(handler)
    csm.term_lib()
    sys.exit(rc)


print("Ready nodes: " + str(node_list))

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
csm.term_lib()
