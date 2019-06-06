#    bbApiTools.py
#
#    Copyright IBM Corporation 2015,2016. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.https://stackoverflow.com/users/signup?ssrc=head&returnurl=%2fusers%2fstory%2fcurrent&utm_source=stackoverflow.com&utm_medium=dev-story&utm_campaign=signup-redirect


import os
import subprocess
import sys
import json
import time
from robot.libraries.BuiltIn import BuiltIn

TIMESTAMP = time.ctime

class bbApiTools(object):

    def __init__(self, jsonData={}):
        self._jsonData = jsonData
		
    def setJsonData(self, jsonData):
        self._jsonData = jsonData

    def getJsonData(self):
        print("non-JSON result: '%s'" % (self._jsonData))
        try:
           m = json.dumps(self._jsonData)
           self.jout = json.loads(m)
           print("non-JSON result: '%s'" % (self.jout['version']['minor']))
           return self.jout
        except ValueError as e:
 #          print("non-JSON result: '%s'" % (self._output))
           self.jout = ""
           raise AssertionError("Invalid Jason Element")
		
    def getClientVerMinor(self):
        try:
           m = json.dumps(self._jsonData)
           self.jout = json.loads(m)
           print("non-JSON result: '%s'" % (self.jout['version']['minor']))
           return self.jout['version']['minor']
        except ValueError as e:
 #          print("non-JSON result: '%s'" % (self._output))
           self.jout = ""
           raise AssertionError("Invalid Jason Element")
		   
		   
    def setClientVerMinor(self, value):
 #       print("non-JSON result: '%s'" % (self.jout[element]))
        try:
           m = json.dumps(self._jsonData)
           self.jout = json.loads(m)
           self.jout['version']['minor'] = value
           print("non-JSON result: '%s'" % (self.jout['version']['minor']))
           self._jsonData = self.jout
        except ValueError as e:
 #          print("non-JSON result: '%s'" % (self._output))
           self.jout = ""
           raise AssertionError("Invalid JSON Element")
		   
    def setClientGitCommit(self, value):
 #       print("non-JSON result: '%s'" % (self.jout[element]))
        try:
           m = json.dumps(self._jsonData)
           self.jout = json.loads(m)
           self.jout['gitcommit'] = value
           print("non-JSON result: '%s'" % (self.jout['gitcommit']))
           self._jsonData = self.jout
        except ValueError as e:
 #          print("non-JSON result: '%s'" % (self._output))
           self.jout = ""
           raise AssertionError("Invalid JSON Element")
		 
		   
