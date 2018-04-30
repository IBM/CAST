#!/bin/python
# encoding: utf-8
#================================================================================
#
#    allocation_query.py
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
#sys.path.append('.')
sys.path.append('/u/jdunham/bluecoral/bluecoral/work/csm/lib')

import lib_csm_py as csm
import lib_csm_wm_py as wm
from pprint import pprint

csm.init_lib()

alloc_input=wm.allocation_query_input_t()
alloc_input.allocation_id=30

rc,handler,alloc_output=wm.allocation_query(alloc_input)

pprint(alloc_output.allocation.history)
pprint(alloc_output.allocation.user_name)
csm.api_object_destroy(handler)

csm.term_lib()
