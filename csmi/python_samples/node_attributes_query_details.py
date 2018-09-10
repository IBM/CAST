#!/bin/python
# encoding: utf-8
#================================================================================
#
#    node_attributes_query_details.py
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

input = inv.node_attributes_query_details_input_t()
input.node_name = "n10"

rc,handler,output = inv.node_attributes_query_details(input)

print("---")
print("Total_Records: " + str(output.result_count))
for i in range (0, output.result_count):
    print("Record_" + str(i+1) + ":")
    result = output.get_result(i)
    print("  node_name:            " + str(result.node.node_name))
    print("  node_collection_time: " + str(result.node.collection_time))
    print("  node_update_time:     " + str(result.node.update_time))
    print("  dimms_count: " + str(result.dimms_count))
    if(result.dimms_count > 0):
        print("  dimms:")
        for j in range (0, result.dimms_count):
            dimm = result.get_dimms(j)
            print("    - serial_number:     " + str(dimm.serial_number))
            print("      physical_location: " + str(dimm.physical_location))
            print("      size:              " + str(dimm.size))
print("...")
    
    #pprint(node.node_name)
    #pprint(node.collection_time)
    #pprint(node.update_time)
    #print(node.state)
    #print("node_state_value: " + str(node.state))
    #pprint(node.state)
    #pprint("node_state_value: " + str(node.state))
    #print(node.state)
    #print(node.node_name)
    #print(csm.csm_get_string_from_enum(csm.csmi_node_state_t, node.state))
    
# csm_get_string_from_enum(csmi_node_state_t, output->results[i]->state)


csm.api_object_destroy(handler)

csm.term_lib()
