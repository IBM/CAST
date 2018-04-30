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

#import wm_structs
#import wm
import sys
sys.path.append('.')
#sys.path.append('/u/jdunham/bluecoral/bluecoral/work/csm/lib')

import lib_csm_py as csm
import lib_csm_wm_py as wm
from pprint import pprint

csm.init_lib()

alloc_input=wm.allocation_t()
alloc_input.primary_job_id=1
alloc_input.job_submit_time="now"

nodes=["c650f03p37-vm10","c650f03p37-vm11"]
alloc_input.set_compute_nodes(nodes)


rc,handler,aid=wm.allocation_create(alloc_input)

pprint(aid)
csm.api_object_destroy(handler)

csm.term_lib()
