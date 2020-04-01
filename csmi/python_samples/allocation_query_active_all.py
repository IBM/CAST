#!/usr/bin/python2
# encoding: utf-8
#================================================================================
#
#    allocation_query_active_all.py
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

#import wm_structs
#import wm
import sys
sys.path.append('.')
#sys.path.append('/u/jdunham/bluecoral/bluecoral/work/csm/lib')

#add the python library to the path 
sys.path.append('/u/nbuonar/repos/CAST/work/csm/lib')

# eventually append the path as part of an rpm install 

import lib_csm_py as csm
import lib_csm_wm_py as wm
from pprint import pprint

csm.init_lib()

alloc_input=wm.allocation_query_active_all_input_t()
alloc_input.limit=-1
alloc_input.offset=-1

rc,handler,alloc_output=wm.allocation_query_active_all(alloc_input)


for i in range (0, alloc_output.num_allocations):
    alloc=alloc_output.get_allocations(i)
    pprint(alloc.allocation_id)
    pprint(alloc.user_name)
    for i in range(0, alloc.num_nodes):
        pprint(alloc.get_compute_nodes(i))


csm.api_object_destroy(handler)

csm.term_lib()
