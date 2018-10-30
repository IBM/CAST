#!/bin/python
# encoding: utf-8
#================================================================================
#
#    allocation_create.py   
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
sys.path.append('/opt/ibm/csm/lib')

import lib_csm_py as csm
import lib_csm_wm_py as wm
from pprint import pprint

csm.init_lib()

alloc_input=wm.allocation_t()
alloc_input.primary_job_id=1
alloc_input.job_submit_time="now"

nodes=[str(sys.argv[1])]

# Allocation Create
alloc_input.set_compute_nodes(nodes)
rc,handler,aid=wm.allocation_create(alloc_input)

if (rc == 0):
    print("Created allocation: " + str(aid))
else: 
    print("Create Failed")
    csm.api_object_destroy(handler)
    csm.term_lib()
    sys.exit(rc)

# Allocation Query
alloc_query_input=wm.allocation_query_input_t()
alloc_query_input.allocation_id=aid
rc,handler,alloc_output=wm.allocation_query(alloc_query_input)

if (rc == 0):
    print("Query Successful:")
    print("\tAllocation ID: " + str(alloc_output.allocation.allocation_id))
    print("\tPrimary Job ID: " +  str(alloc_output.allocation.primary_job_id))
    print("\tNumber of Nodes: " + str(alloc_output.allocation.num_nodes))
else:
    print("Query Failed")
    csm.api_object_destroy(handler)
    csm.term_lib()
    sys.exit(rc)

# Allocation Delete
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
