# -*- coding: utf-8 -*-                   # remove when Python 3
#==============================================================================
#   
#    hcdiag/src/framework/target_interface.py
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
#==============================================================================

import subprocess
import os

#------------------------------------------------------------------- 
""" TargetInterface class definition
    This class implements the interface with xcat
"""
#------------------------------------------------------------------- 

class TargetInterface(object):

   def __init__(self, logger, timeout=None):                                                                             
      self.logger    = logger
      self.timeout   = timeout
      self.tmpcreated= False


   #----------------------------------------------------
   """  This method does the temporary directory cleanup
   """   
   #----------------------------------------------------
   def cleanup(self, noderange):
      if self.tmpcreated:
         cmd= '"rm -rf %s"' %(self.tmpdir)
         self.logger.debug('Cleaning up remote node(s) {0}.' .format(self.tmpdir))
         if self.execute_target(noderange, cmd):
            self.logger.error('Error cleaning up {0}. Ignoring...' .format(self.tmpdir))
      
   #----------------------------------------------------------------- 
   # create_target_tmpdir
   #   
   #----------------------------------------------------------------- 
   def create_target_tmpdir(self, tempdir, runid, noderange) :   
      # let's create the remote directory first with the correct permission
      if not self.tmpcreated:
        rc = self.execute_target(noderange, 'mkdir -p -m 777 ' + tempdir)
        if rc: 
           self.logger.critical('Error creating {0} on remote nodes(s). Exiting...' .format(tempdir))
           return 1
        self.tmpdir =  '%s/%s/' %(tempdir, runid)
        rc = self.execute_target(noderange, 'mkdir -p ' + self.tmpdir)
        if rc: 
           self.logger.critical('Error creating {0} on remote nodes(s). Exiting...' .format(self.tmpdir))
           return 1
        self.tmpcreated = True

      return 0
        
   #------------------------------------------------------------------------------ 
   # execute_discard_output
   #------------------------------------------------------------------------------ 
   def execute_discard_output(self, cmd):
      self.logger.debug(cmd)
      proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True )
      proc.wait()
      return proc.returncode
   
   #------------------------------------------------------------------------------ 
   # execute
   #------------------------------------------------------------------------------ 
   def execute(self, cmd, eyecatcher= None, check_digit= False, logdebug=True):
      self.logger.debug(cmd)
      value=[]
      proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True )
      for line in iter(proc.stderr.readline,''):
         line.rstrip('\n')
         line.strip()
         if len(line) and logdebug :  self.logger.debug(line)
         #print '==> stderr', len(line), line
      
      for line in iter(proc.stdout.readline,''):
         line.rstrip('\n')
         line.strip()
         #print "==> stdout", len(line), line
         if len(line) and logdebug :
            self.logger.debug(line)
            
            if eyecatcher: 
               i=line.find(eyecatcher)
               if i>= 0:
                  i+=len(eyecatcher)
                  v=line[i:].strip()
                  if check_digit:
                     if v.isdigit(): value.append(v)
                  else:
                     value.append(v)
            else:                         
               if check_digit:
                  if line.isdigit(): value.append(line)
               else:
                  value.append(line)

      proc.wait()
      #print "==> returning ", proc.returncode, value
      return proc.returncode, value
   
