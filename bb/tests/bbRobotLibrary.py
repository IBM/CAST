from __future__ import print_function
#    bbRobotLibrary.py
#
#    Copyright IBM Corporation 2015,2016. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.


import os
import subprocess
import sys
import json
import time
from robot.libraries.BuiltIn import BuiltIn

TIMESTAMP = time.ctime

class bbRobotLibrary(object):

    def __init__(self, workdir, configfile):
        self._work_path= workdir
        self._config_file= configfile
        self._sut_path = os.path.join(workdir,
                                      'bb', 'bin', 'bbcmd')
        self._rawoutput = ''
        process = subprocess.Popen(["sudo","-v"], universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        self.sudooutput = process.communicate()

    def get_error_root(self):
        hostlist  = BuiltIn().replace_variables('${HOSTLIST}')
        if hostlist == '':
            return self.jout
        if self.jout['rc'] == 0:
            return self.jout
        rank = self.jout['error']['firstFailRank']
        return self.jout[rank]

    def failing_rank_is(self):
        hostlist  = BuiltIn().replace_variables('${HOSTLIST}')
        if hostlist == '':
            if self.jout['rc'] != 0:
                return 0
        try:
            if self.jout['rc'] != 0:
                return self.jout['error']['firstFailRank']
            return "none"
        except ValueError as e:
            return "none"

    def failing_details(self):
        try:
            err  = self.get_error_root()
            rank = self.failing_rank_is()
            return "Rank %s with status %s" % (rank, err['error']['text'])
        except ValueError as e:
            return "(failing_details_exception)"

    def status_should_be(self, expected_status):
        if expected_status != self.jout['rc']:
            if self.jout['rc'] != '0':
                raise AssertionError("Expected status to be '%s' but was '%s'.  Details '%s'"
                                     % (expected_status, self.jout['rc'], self.failing_details()))
            else:
                raise AssertionError("Expected status to be '%s' but was '%s'." % (expected_status, self.jout['rc']))

    def status_should_not_be(self, expected_status):
        if expected_status == self.jout['rc']:
            if self.jout['rc'] != '0':
                raise AssertionError("Expected status to not be '%s' but was '%s'.  Details '%s'"
                                     % (expected_status, self.jout['rc'], self.failing_details()))
            else:
                raise AssertionError("Expected status to not SUCCESSFULL")


    def read_device_data(self, field, rank):
        hostlist  = BuiltIn().replace_variables('${HOSTLIST}')
        if hostlist == '':
            return self.jout['out'][field]
        return self.jout[rank]['out'][field]


    def read_transfer_handle(self, rank):
        hostlist  = BuiltIn().replace_variables('${HOSTLIST}')
        if hostlist == '':
            return self.jout['out']['transferHandle']
        return self.jout[rank]['out']['transferHandle']

    def read_transfer_handle_list(self, rank):
        hostlist  = BuiltIn().replace_variables('${HOSTLIST}')
        if hostlist == '':
            return self.jout['out']['handles']
        return self.jout[rank]['out']['handles']

    def read_transfer_status(self, rank):
        hostlist  = BuiltIn().replace_variables('${HOSTLIST}')
        if hostlist == '':
            return self.jout['out']['status']
        return self.jout[rank]['out']['status']

    def bbcmd(self, command, *args):
        iamroot    = BuiltIn().replace_variables('${IAMROOT}')
        suiteisroot= BuiltIn().replace_variables('${SUITEISROOT}')
        ranklist   = BuiltIn().replace_variables('${RANKLIST}')
        hostlist   = BuiltIn().replace_variables('${HOSTLIST}')
        jobid      = BuiltIn().replace_variables('${JOBID}')
        jobstepid  = BuiltIn().replace_variables('${JOBSTEPID}')

        command = [self._sut_path, command, '--config', self._config_file] + list(args)

        if hostlist != '':
            command.insert(2, "--hostlist=" + hostlist)
            command.insert(2, "--target=" + ranklist)
        else:
            command.insert(2, "--contribid=0")

        command.insert(2, "--jobid=" + str(jobid))
        command.insert(2, "--jobstepid=" + str(jobstepid))

        if iamroot == '1' or suiteisroot == '1':
            command.insert(0, 'sudo')

        print("Command: '%s'" % ' '.join(command))

        process = subprocess.Popen(command, universal_newlines=True, stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT)
        self._output = process.communicate()[0].strip()
        try:
            self.jout = json.loads(self._output)
            print(json.dumps(self.jout, indent=4, sort_keys=True))
            print("non-JSON result: '%s'" % (self._output))
        except ValueError as e:
            print("non-JSON result: '%s'" % (self._output))
            self.jout = ""
            raise AssertionError("bbcmd returned non-JSON data")
