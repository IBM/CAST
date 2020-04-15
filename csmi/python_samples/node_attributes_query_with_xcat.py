#!/usr/bin/python2
# encoding: utf-8
#================================================================================
#
#    node_attributes_query_with_xcat.py
#
#  © Copyright IBM Corporation 2015-2019. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================


# ABOUT: 
# -------
# This is an example python script of how to use xCAT python API to gather 
# node range information and pass that into CSM APIs. 
# -------
# author: Nick Buonarota
# email: nbuonar@us.ibm.com


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

# PART 1 ====  Get the node names from xcat

# set up xCAT info
# replace this with your xCAT info
XCATMN = "127.0.0.1"
username = "admin"
password = "password"

# set your xCAT restAPI url
REST_ENDPOINT = "https://" + XCATMN + "/xcatws"

# Here is your xcat search. 
# replace as needed for your search. 
# examples:
# nodes - c650f03p[07,09,11]
# node range - c650f03p[07-10]
# node group - all
node_string = "c650f03p[07,09,11]"

# use the xCAT restAPI url to construct a message that will return the information you want.
get_node_range = REST_ENDPOINT + "/nodes/" + node_string + "/nodels/"


# Here we:
# Send a request to get all nodes, using the url above and passing in user and password
#
response = requests.get(get_node_range + "?userName=" + username + "&userPW=" + password, verify=False)

# get the data
#
# save the json data as a python list
# nodes_j = response.json()
# 
# change the data from unicode to utf8
# This is needed for csm api "set_ARRAY" function
# nodes_j1 = [x.encode('utf8') for x in nodes_j]

# Once you understand the above you can combine as in below. 
# The above code in comments has been combined into one line.
nodes = [x.encode('utf8') for x in response.json()]

# Once the nodes have been gathered. You can call the CSM API. 


# PART 2 ==== CSM API


# CSM API
csm.init_lib()

input = inv.node_attributes_query_input_t()

# set nodes from gathered xCAT data above.
input.set_node_names(nodes)

# set other input data
input.limit=-1
input.offset=-1

# Call the CSM API
rc,handler,output = inv.node_attributes_query(input)

# Access csm api output how ever you want. 

if(rc == csm.csmi_cmd_err_t.CSMI_SUCCESS):
    for i in range (0, output.results_count):
        record = output.get_results(i)
        pprint(record.node_name)
        pprint(record.collection_time)
        pprint(record.update_time)
        print(record.state)
        print("node_state_value: " + str(record.state))
        pprint(record.state)
        pprint("node_state_value: " + str(record.state))
        print(record.state)
        print(record.node_name)
else:
    print("No matching records found.")


csm.api_object_destroy(handler)

csm.term_lib()
