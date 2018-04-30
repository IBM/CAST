#!/bin/python
# encoding: utf-8
# ================================================================================
#
# ut_find_job_keys.py
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
.. module::ut_find_job_keys
:platform: Linux
:synopsis: Tests the find_job_keys use case.
.. moduleauthor:: John Dunham (jdunham@us.ibm.com)
'''

import unittest
import context
import usecases.find_job_keys as find_job_keys

# Predominantly used for debug loging.
import logging
logger=logging.getLogger(__name__)
DIV_START = "\n" + "="*80
DIV_END   = "\n\n"

INVALID_JOBID = 2147483647 
VALID_JOBID = 17



class TimeRangeTest(unittest.TestCase):

    tests = {
        "test_valid_job" : [ "", "-j", VALID_JOBID],
        "test_invalid_job" : [ "", "-j", INVALID_JOBID],
        "test_illegal_jobid" : ["","-j","a"],
        
    }
    
    def test_valid_job(self):
        logger.info("Enter test_valid_job%s", DIV_START)
        with self.assertRaises(SystemExit) as rv:
            find_job_keys.main(self.tests["test_valid_job"])

        logger.info("Exit test_valid_job%s", DIV_END)
        self.assertEqual(rv.exception.code, 0)
	
    def test_invalid_job(self):
        logger.info("Enter test_invalid_job%s", DIV_START)

        with self.assertRaises(SystemExit) as rv:
            find_job_keys.main(self.tests["test_invalid_job"])

        logger.info("Exit test_invalid_job%s", DIV_END)
        self.assertEqual(rv.exception.code, 1)

    def test_illegal_job(self):
        logger.info("Enter test_illegal_jobb%s", DIV_START)

        with self.assertRaises(SystemExit) as rv:
            find_job_keys.main(self.tests["test_illegal_jobid"])

        logger.info("Exit test_illegal_job%s", DIV_END)
        self.assertEqual(rv.exception.code, 1)


if __name__ == '__main__':
    unittest.main()
