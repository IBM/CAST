# -*- coding: utf-8 -*-                   # remove when Python 3      
#================================================================================
#   
#    hcdiag/src/framework/harness.py
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
#===============================================================================*/

from __future__ import print_function
import os
import subprocess
import time
from datetime import datetime, timedelta
                               
#from timeout_manager import TimeoutManager
import base_harness 
from xcat_interface import XcatInterface
from proxy import Proxy

                               

#------------------------------------------------------------------- 
""" Harness class definition
    This class implements the Diagnostics harness
"""
#------------------------------------------------------------------- 
class Harness(base_harness.BaseHarness):                 
                               
   #------------------------------------------------------------------- 
   """Class constructor"""     
   #------------------------------------------------------------------- 
   def __init__(self, mattr, tconfig, logger, runid, logdir, csmii, user, cmd_target):
      super(Harness,self).__init__( mattr, tconfig, logger, runid, logdir, csmii, user, cmd_target, True) 
      self.tinterface= XcatInterface(logger, mattr['timeout'], mattr['xcat_bindir'], mattr['xcat_fanout']) 
      #print mattr
     
   #------------------------------------------------------------------- 
   """ prepare_run (with xcat)
   """
   #------------------------------------------------------------------- 
   def prepare_run(self, test)   :
      tattr= self.tconfig.get_test_attributes(test)    #= returns dict

      # host.list will will be created localy as /tmp/runid-host.list
      # remotely /tmp/hcdiag/runid/host.list

      remote_tmp_dir = '%s/%s/' %(self.mattr['tempdir'], self.runid)

      # cluster test
      if tattr['clustertest'] == 'yes':
        if len(self.target) < 2:
          self.logger.error('Test: {0} is defined as cluster test, but {1} node was especified.' .format(test, len(self.target)))
          return None

        rc = self.generate_hostlist(test)             ## cluster proxy
        if rc : return None

      if tattr['xcat_cmd'] == 'no':
        if self.tinterface.downnode != 0 :
          self.logger.error('Test: {0} can not run on nodes that are not up.' .format(test))
          return None

      cmd = None
      xarg = ' -s -z -t %s ' %(tattr['timeout'])      ## xcat arguments: data stream, exit status, timeout  
      export_logdir=''   
      export_cmd=''
      if self.mattr['common_fs'] == 'yes':
          export_logdir='HCDIAG_LOGDIR=%s/%s' %(self.logdir, test)
          export_cmd='export '+export_logdir

      texec = tattr['executable']
      targs = tattr['args']                           ## test arguments 

      path,fn = os.path.split(texec)
      if tattr['targettype'] == 'Management':         ## management proxy
         self.proxy= Proxy()
         self.proxy.set_proxy_mgt()
         # only needs to test if executable exist on mgt node
         if not ( os.path.exists(texec) and os.access(texec, os.X_OK)):
            self.logger.critical('Executable: {0} not found on management node or it is not executable. Exiting...' .format(texec))
            return None      ### handle this case exit exit
         texec='cd %s; ./%s %s %s' %(path, fn, self.noderange, targs)
      else :
         self.logger.debug('Checking if executable: {0} exists on remote node(s).' .format(texec)) 
         if self.tinterface.check_exec_target(texec, self.noderange) :
            #-- file exist remotely
            self.logger.info('Executable: {0} exists on remote node(s).' .format(texec))
            texec='cd %s; ./%s %s' %(path, fn, targs)
         else:
            #-- even if is not common_fs, try to execute
            #-- check if executable exist on mgt node
            self.logger.debug('Executable: {0} does not exist on remote node(s).' .format(texec))
            if not ( os.path.exists(texec) and os.access(texec, os.X_OK)):
               self.logger.critical('Executable: {0} not found on management node or it is not executable. Exiting...' .format(texec))
               return None      ### handle this case exit exit
         
            #-- we need to copy, make sure /tmp/hcdiag/pid  exist
            # let's create the tmp remote directory first
            rc= self.tinterface.create_target_tmpdir(self.mattr['tempdir'], self.runid, self.noderange)
            if rc != 0: return None 
         
            #-- let's copy
            rc = self.tinterface.copy_to_target(self.noderange, path, remote_tmp_dir, '-R')
            if rc:
               self.logger.critical('Copy {0} to {1} on remote node(s) failed, rc {2}. Exiting...' .format(path, remote_tmp_dir, rc))
               return None
            #-- when we copy, we need to make sure where to cd (case where test differs by param)
            texec='cd %s%s; ./%s %s' %(remote_tmp_dir, os.path.basename(path),  fn, targs)

                  
      # add environment
      #export_cmd=''
      #if export_logdir: export_cmd='export '+export_logdir
      tgt=None
      if self.proxy:
         tgt=self.proxy.node
         if self.proxy.type == 'cluster':
            if export_cmd : 
               export_cmd+=' MP_HOSTFILE=%s' %(self.proxy.hostlist)
            else :
               export_cmd='export MP_HOSTFILE=%s' %(self.proxy.hostlist)
            #texec= '%s;%s' %(export_cmd, texec) 
         #cmd = '%s/xdsh %s %s "%s"' %(self.mattr['xcat_bindir'], self.proxy.node, xarg, texec)
      else:
         tgt=self.noderange

      if export_cmd: export_cmd+=";"
      cmd = '%s/xdsh %s %s "%s%s"' %(self.mattr['xcat_bindir'], tgt, xarg, export_cmd, texec)

      return cmd


   #------------------------------------------------------------------- 
   """ Validades the target (xCat noderange syntax) and check the target
      node status using xCat """
   #------------------------------------------------------------------- 

   def validate_target(self): 
      # check first if any of tests are in the group rvitals, rinv and rpower
      has_xcat_cmd=False
      for t in self.tests:
         if self.tconfig.get_test_attribute(t,'xcat_cmd')  == 'yes': 
            has_xcat_cmd=True
            break
      #
      rc= self.tinterface.get_node_info(self.noderange, self.target, has_xcat_cmd)
      #print "!! ", self.target
      return rc

   #------------------------------------------------------------------- 
   """ store_node_stdout
   """   
   #------------------------------------------------------------------- 
                  
   def store_node_stdout( self, stdout, time_manager ):   
      fdopen=True
      if self.mattr['common_fs'] == 'yes': fdopen=False
      
      for line in iter(stdout.readline,''):
         sline=line.rstrip('\n')
         if len(sline) == 0:  # empty line
            self.logger.debug('empty stdout message received. Ignoring...')
            continue

         self.logger.debug('stdout "{0}" message received.' .format(sline))
         try:
            node, msg= sline.split(':',1)
            found= False
            if self.proxy:
               if node == self.proxy.node: 
                  found=True
                  if fdopen: fd= self.proxy.fd
            else: 
               if node in self.target:
                  found= True
                  if fdopen: fd= self.target[node]['fd']

            # it is a well formed message: "hostname: msg"
            if found:
               if self.mattr['watch_output_progress'] != '0':
                  time_manager.activity(node)

               if fdopen:
                  print(msg, file=fd)
                  fd.flush()
               else:
                  # check if it is a line with the name of the log file (common_fs=yes)
                  i = sline.find(self.logdir)
                  if i >= 0:
                     if self.proxy:
                        self.proxy.ofile= msg
                     else:
                        self.target[node]['ofile']= msg
                     continue

               i = sline.find('Remote_command_rc =')
               if i < 0:
                  i = sline.find('Killed by signal')

               if i >= 0:
                  # remove the period at the end of the message
                  msg=msg.rstrip('.')
                  e=[int(s) for s in msg[i:].split() if s.isdigit()]
                  if len(e): 
                    if self.proxy: 
                       self.proxy.rc= e[0]
                    else: 
                       self.target[node]['rc']= e[0]

                    if self.mattr['watch_output_progress'] != '0':
                       time_manager.remove(node)
                    continue
                  else:
                     self.logger.debug('stdout message "{0}", received from node {1}, but no rc or error code. Ignoring...' .format(sline, node))
            else:
                # will not happen
                self.logger.debug('stdout message "{0}", received from an unknown source {1}. Ignoring...' .format(sline), node)

         except (ValueError, TypeError) as e:
             self.logger.error('stdout "{0}", invalid message. Ignoring... ' .format(e, sline))
      
### end class


