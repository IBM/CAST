#!/usr/bin/python2
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

#add the python library to the path
sys.path.append('/opt/ibm/csm/lib')

# eventually append the path as part of an rpm install

import lib_csm_py as csm
import lib_csm_inv_py as inv
from pprint import pprint

csm.init_lib()

input = inv.node_attributes_query_input_t()
nodes=[str(sys.argv[1])]
input.set_node_names(nodes)
input.limit=-1
input.offset=-1

rc,handler,output = inv.node_attributes_query(input)

if(rc == csm.csmi_cmd_err_t.CSMI_SUCCESS):
    for i in range (0, output.results_count):
        node = output.get_results(i)
        print("node: " + str(node.node_name))
        print("node_state_value: " + str(node.state))
else:
    print("No matching records found.")
    csm.api_object_destroy(handler)
    csm.term_lib()
    sys.exit(rc)

if(str(node.state) == "CSM_NODE_DISCOVERED"):
    print("node state is DISCOVERED. Updating to IN_SERVICE")
    input = inv.node_attributes_update_input_t()
    input.set_node_names(nodes)
    input.state = csm.csmi_node_state_t.CSM_NODE_IN_SERVICE
    rc,handler,output = inv.node_attributes_update(input)
    if (rc == csm.csmi_cmd_err_t.CSMI_SUCCESS):
        input = inv.node_attributes_query_input_t()
        input.set_node_names(nodes)
        rc,handler,output = inv.node_attributes_query(input)
        node = output.get_results(0)
        print("node: " + str(node.node_name))
        print("node_state_value: " + str(node.state))
        if (str(node.state) != str(csm.csmi_node_state_t.CSM_NODE_IN_SERVICE)):
            print("Error: Node not IN_SERVICE after update")
            csm.api_object_destroy(handler)
            csm.term_lib()
            sys.exit(rc)
    else:
        print("Error updating node.")
        csm.api_object_destroy(handler)
        csm.term_lib()
        sys.exit(rc)

csm.api_object_destroy(handler)

csm.term_lib()
