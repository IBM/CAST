#!/bin/python
# encoding: utf-8
# ================================================================================
#
#  ut_jobs_running.py
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
.. module::ut_jobs_running
:platform: Linux
:synopsis: Tests the find_jobs_running use case.
.. moduleauthor:: John Dunham (jdunham@us.ibm.com)
'''

import unittest
import context
import usecases.find_jobs_running as find_jobs_running

# Predominantly used for debug loging.
import logging.config
logger=logging.getLogger(__name__)

DIV_START = "\n" + "="*80
DIV_END   = "\n\n"

INVALID_JOBID = 2147483647 
VALID_JOBID = 1000001


class JobsRunningTest(unittest.TestCase):

    # -t '09/21/16 22:27:10-0400' -d .5 -n "c931f04p32,c931f04p34,c931f04p36,c931f04p38"

    tests = {
        "test_bad_target"  : [ "", "--targettime", "09-22-2016 04:27:12:910"],
        "test_bad_days"    : [ "", "--days", "d"],
        "test_good_no_host": [ "", "--targettime", "09/21/16 22:27:10-0400", "--days", "1", "--nohost"],
        "test_good_target" : [ "", "--targettimet", "09/21/16 22:27:10-0400", "--days", "1",
             "--hosts", "c931f04p32,c931f04p34,c931f04p36,c931f04p38"],
        "test_good_d_days" : [ "", "--targettime", "09/21/16 22:27:10-0400", "--days", ".5",
            "--hosts", "c931f04p32,c931f04p34,c931f04p36,c931f04p38"]

    }
    

    def test_bad_target(self):
        logger.info("Enter test_bad_days%s", DIV_START)
        with self.assertRaises(SystemExit) as rv:
            find_jobs_running.main(self.tests["test_bad_target"])
        
        logger.info("Exit%s", DIV_END)
        self.assertEqual(rv.exception.code, 1)

    def test_bad_days(self):
        logger.info("Enter test_bad_days%s", DIV_START)
        with self.assertRaises(SystemExit) as rv:
            find_jobs_running.main(self.tests["test_bad_days"])
    
        logger.info("Exit%s", DIV_END)
        self.assertEqual(rv.exception.code, 1)

    def test_valid_time(self):
        logger.info("Enter test_valid_time%s", DIV_START)
        with self.assertRaises(SystemExit) as rv:
            find_jobs_running.main(self.tests["test_good_target"])

        logger.info("Exit%s", DIV_END)
        self.assertEqual(rv.exception.code, 0)
    
    def test_valid_time_nohost(self):
        logger.info("Enter test_valid_time_nohost%s", DIV_START)
        with self.assertRaises(SystemExit) as rv:
            find_jobs_running.main(self.tests["test_good_no_host"])

        logger.info("Exit%s", DIV_END)
        self.assertEqual(rv.exception.code, 0)

    def test_good_d_days(self):
        logger.info("Enter test_good_d_days%s", DIV_START)
        with self.assertRaises(SystemExit) as rv:
            find_jobs_running.main(self.tests["test_good_d_days"])
        
        logger.info("Exit%s", DIV_END)
        self.assertEqual(rv.exception.code, 0)


if __name__ == '__main__':
    unittest.main()
