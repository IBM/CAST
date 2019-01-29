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

# import generic
import sys
import requests
import json

#add the python library to the path 
sys.path.append('/opt/ibm/csm/lib')

# import csm
# eventually append the path as part of an rpm install 
import lib_csm_py as csm
import lib_csm_inv_py as inv
from pprint import pprint

# Get the node names from xcat

XCATMN        = "127.0.0.1"
username      = "wsuser"
password = "cluster_rest"



REST_ENDPOINT    = "https://" + XCATMN + "/xcatws"
create_node      = REST_ENDPOINT + "/nodes/"
get_all_nodes    = REST_ENDPOINT + "/nodes/"
get_token = REST_ENDPOINT + "/tokens"

get_node_range    = REST_ENDPOINT + "/nodes/n[1-3]/nodels"


#
# Send a request to get all nodes, passing in user and password
#
all_nodes = requests.get(get_node_range + "?userName=" + username + "&userPW=" + password, verify=False)

# Display returned data
print "List of all nodes extracted with userid and password:"
print all_nodes.content


# CSM API

csm.init_lib()

input = inv.node_attributes_query_input_t()
#input.node_names = ["node_01", "n01"]
#input.node_names_count = len(input.node_names)
nodes=["node_01","n01"]
input.set_node_names(nodes)
#input.set_node_names(["node_01","n01"])
input.limit=-1
input.offset=-1

rc,handler,output = inv.node_attributes_query(input)

if(rc == csm.csmi_cmd_err_t.CSMI_SUCCESS):
    for i in range (0, output.results_count):
        node = output.get_results(i)
        pprint(node.node_name)
        pprint(node.collection_time)
        pprint(node.update_time)
        print(node.state)
        print("node_state_value: " + str(node.state))
        pprint(node.state)
        pprint("node_state_value: " + str(node.state))
        print(node.state)
        print(node.node_name)
        #print(csm.csm_get_string_from_enum(csm.csmi_node_state_t, node.state))
else:
    print("No matching records found.")
	
# csm_get_string_from_enum(csmi_node_state_t, output->results[i]->state)


csm.api_object_destroy(handler)

csm.term_lib()
