#!/bin/python
# encoding: utf-8
# ================================================================================
#
#    ut_csmi_tool.py
#
#  © Copyright IBM Corporation 2015,2016. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
# ================================================================================
'''
.. module::ut_csmi_tool
:platform: Linux
:synopsis: Tests the csmi_tool module.
.. moduleauthor:: John Dunham (jdunham@us.ibm.com)
'''

import unittest
import context
import csmbigdata.utils.csmi_tool as csmi_tool_m

# Predominantly used for debug loging.
import logging
logger=logging.getLogger(__name__)
DIV_START = "\n" + "="*80
DIV_END   = "\n\n"

INVALID_JOBID = 2147483647 
VALID_JOBID = 17

# TODO this needs to be standardized?
CSM_HOME = "../../../../../work/csm/bin/"

class CSMIToolTest(unittest.TestCase):

    def test_legal_params_allocation(self):
        logger.info("Enter test_legal_params_allocation %s", DIV_START)

        csmi_tool = csmi_tool_m.CSMITool()
        csmi_tool.set_csm_home(CSM_HOME)
        
        try:
            logger.info("Testing Allocation Query: No Parameters: ")
            ret = csmi_tool.allocation_query()
            if ret is not None:
                raise ValueError("allocation_query did not return None, %s" % ret)
        
   #         logger.info("Testing Allocation Query: Allocation ID: ")
   #         print csmi_tool.allocation_query(1)
            

        except ValueError, e:
            self.fail(e)

        



if __name__ == '__main__':
    unittest.main()
