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

# eventually append the path as part of an rpm install

import lib_csm_py as csm
import lib_csm_inv_py as inv
import lib_csm_wm_py as wm
from pprint import pprint

csm.init_lib()

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

alloc_input.set_compute_nodes(node_list)
alloc_input.isolated_cores=0
alloc_input.state=wm.csmi_state_t.CSM_RUNNING
rc,handler,aid=wm.allocation_create(alloc_input)

if (rc == 0):
    print("Created allocation: " + str(aid))
else:
    print("Create Failed")
    csm.api_object_destroy(handler)
    csm.term_lib()
    sys.exit(rc)

alloc_delete_input=wm.allocation_delete_input_t()
alloc_delete_input.allocation_id=aid
rc,handler=wm.allocation_delete(alloc_delete_input)
if (rc == 0):
    print("Allocation " + str(alloc_delete_input.allocation_id) + " successfully deleted")
else:
    print("Failed to delete allocation " + str(alloc_delete_input.allocation_id))
    csm.api_object_destroy(handler)
    csm.term_lib()
    sys.exit(rc)

alloc_input.isolated_cores=0
rc,handler,aid=wm.allocation_create(alloc_input)

if (rc == 0):
    print("Created allocation: " + str(aid))
else:
    print("Create Failed")
    csm.api_object_destroy(handler)
    csm.term_lib()
    sys.exit(rc)

alloc_delete_input=wm.allocation_delete_input_t()
alloc_delete_input.allocation_id=aid
rc,handler=wm.allocation_delete(alloc_delete_input)
if (rc == 0):
    print("Allocation " + str(alloc_delete_input.allocation_id) + " successfully deleted")
else:
    print("Failed to delete allocation " + str(alloc_delete_input.allocation_id))
    csm.api_object_destroy(handler)
    csm.term_lib()
    sys.exit(rc)

alloc_input.isolated_cores=1
rc,handler,aid=wm.allocation_create(alloc_input)

if (rc == 0):
    print("Created allocation: " + str(aid))
else:
    print("Create Failed")
    csm.api_object_destroy(handler)
    csm.term_lib()
    sys.exit(rc)

alloc_delete_input=wm.allocation_delete_input_t()
alloc_delete_input.allocation_id=aid
rc,handler=wm.allocation_delete(alloc_delete_input)
if (rc == 0):
    print("Allocation " + str(alloc_delete_input.allocation_id) + " successfully deleted")
else:
    print("Failed to delete allocation " + str(alloc_delete_input.allocation_id))
    csm.api_object_destroy(handler)
    csm.term_lib()
    sys.exit(rc)

alloc_input.isolated_cores=0
rc,handler,aid=wm.allocation_create(alloc_input)

if (rc == 0):
    print("Created allocation: " + str(aid))
else:
    print("Create Failed")
    csm.api_object_destroy(handler)
    csm.term_lib()
    sys.exit(rc)

alloc_delete_input=wm.allocation_delete_input_t()
alloc_delete_input.allocation_id=aid
rc,handler=wm.allocation_delete(alloc_delete_input)
if (rc == 0):
    print("Allocation " + str(alloc_delete_input.allocation_id) + " successfully deleted")
else:
    print("Failed to delete allocation " + str(alloc_delete_input.allocation_id))
    csm.api_object_destroy(handler)
    csm.term_lib()
    sys.exit(rc)

csm.api_object_destroy(handler)

csm.term_lib()

