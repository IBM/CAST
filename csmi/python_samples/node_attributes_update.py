#!/bin/python
# encoding: utf-8
#================================================================================
#
#    node_attributes_query.py
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
sys.path.append('.')

#add the python library to the path 
sys.path.append('/u/nbuonar/repos/CAST/work/csm/lib')

# eventually append the path as part of an rpm install 

import lib_csm_py as csm
import lib_csm_inv_py as inv
from pprint import pprint

csm.init_lib()

input = inv.node_attributes_update_input_t()
#input.node_names = ["node_01", "n01"]
#input.node_names_count = len(input.node_names)
nodes=["allie","n01","bobby"]
input.set_node_names(nodes)
input.state = csm.csmi_node_state_t.CSM_NODE_IN_SERVICE
#input.set_node_names(["node_01","n01"])
input.limit=-1
input.offset=-1

rc,handler,output = inv.node_attributes_update(input)

print rc 

if rc == csm.csmi_cmd_err_t.CSMERR_UPDATE_MISMATCH:
    print output.failure_count
    for i in range(0, output.failure_count):
        print output.get_failure_node_names(i)


csm.api_object_destroy(handler)

csm.term_lib()
