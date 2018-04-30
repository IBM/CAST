# -*- coding: utf-8 -*-                  # remove when Python 3
#==============================================================================
#   
#    hcdiag/src/framework/xcat_interface.py
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
from target_interface import TargetInterface

#------------------------------------------------------------------- 
""" XcatInterface class definition
    This class implements the interface with xcat
"""
#------------------------------------------------------------------- 
DEFAULT_XCAT_FANOUT=64

class XcatInterface(TargetInterface):

   def __init__(self, logger, timeout, bindir, fanout):                                                                             
      super(XcatInterface, self).__init__(logger, timeout)
      self.bindir  = bindir
      self.fanout  = fanout
      self.downnode = 0
      

   #------------------------------------------------------------------- 
   """ Check the status of the target nodes 
       Retrieve its serial number
       Validades the target (xCat noderange syntax)
   """
   #------------------------------------------------------------------- 
   def get_node_info(self, noderange, target, has_xcat_cmd=False ):
      cmd='%s/nodestat %s' %(self.bindir, noderange)
      status= dict()
      thishost=os.uname()[1].split(".")[0]

      rc, status = self.execute(cmd)

      # node(s) is not recognized by xcat
      if (rc != 0) or (len(status) == 0): 
         self.logger.ras('hcdiag.xcat.noderange_err', 'noderange=%s' %(noderange), thishost)
         return False
         
      ret= True
      downlist=[]
      for node, st in status.iteritems():
         if 'sshd' in st:
            self.logger.debug('Node {0:} is OK' .format(node))
         else:       
            if has_xcat_cmd:
               self.logger.info('Node {0:} is "{1}"' .format(node,st))
               downlist.append(node)
               self.downnode+=1
            else:
               self.logger.ras('hcdiag.fmk.node_err', 'node=%s, status=%s' %(node, st), thishost)
               ret= False
      if not ret : return ret 

      sn= dict()
      # ask for the serial number only for the nodes that are up
      if self.downnode == len(status):
         self.logger.debug('All nodes in noderange {0} are down' .format(noderange))
      else:
         if self.downnode :
            cmd= '%s/xdsh %s,-%s cat /proc/device-tree/system-id' %(self.bindir,noderange,",-".join(downlist) )  
         else:
            cmd= '%s/xdsh %s cat /proc/device-tree/system-id' %(self.bindir, noderange)   

         rc, sn = self.execute(cmd)
         if (rc != 0) or (len(sn) == 0) : 
            self.logger.ras('hcdiag.xcat.sn_err', 'nodes=%s, rc=%d' %(noderange, rc), thishost)
            return False
      
      for node in status :
        if node in sn:
           s=''.join([c for c in sn[node] if c.isalpha() or c.isalnum() ])
           target[node]= {'fd': None, 'rc' : int(0), 'sn' : s, 'ofile' : None, 'state' : 'up' }
           self.logger.debug('Node {0} has serial number {1}' .format(node, s))
        else:
           if node in downlist:
              target[node]= {'fd': None, 'rc' : int(0), 'sn' : None, 'ofile' : None, 'state' : 'down' }
           else:
              self.logger.ras('hcdiag.fmk.sn_err', 'node=%s' %(node), thisthost)
              ret= False

      return ret


      
   #----------------------------------------------------------------- 
   # check if the executable is present
   #   
   #----------------------------------------------------------------- 
   def check_exec_target(self, test_exec, target):
      cmd='test -x %s'  %(test_exec)
      rc = self.execute_target(target, cmd)
      if rc : return False
      return True
   
   #----------------------------------------------------------------- 
   # execute_target
   #   
   #----------------------------------------------------------------- 
   
   def execute_target(self, target, command):
      xflags= ''
      if int(self.fanout) > DEFAULT_XCAT_FANOUT: xflags+= ' -f '+ self.fanout
      cmd= '%s/xdsh %s -t %s %s "%s"' %(self.bindir, target, self.timeout, xflags, command)
      self.logger.debug(cmd)
      return self.execute_discard_output(cmd)

   #----------------------------------------------------------------- 
   # copy_to_target
   #  destination is always tmpdir 
   #----------------------------------------------------------------- 
   def copy_to_target(self, target, src, dest=None, flags = ''):
      if not dest : dest=self.tmpdir
      self.logger.debug('Copying {0} to {1}.' .format(src, dest ))                                
   
      xflags= ''

      ## xdcp does not propagate the user that is running, uses root instead (fixed)
      #if user != 'root': xflags= '-l ' + user
      if int(self.fanout) > DEFAULT_XCAT_FANOUT: xflags= ' -f '+ self.fanout

      cmd='%s/xdcp %s %s %s %s %s' %(self.bindir, target, xflags, flags, src, dest) 
      #print  "!! ", cmd
      return self.execute_discard_output(cmd)

      
   #------------------------------------------------------------------- 
   """  execute
        xcat command output is always prefixed by <NODE: ...>
   """
   #------------------------------------------------------------------- 
   def execute(self, cmd):
      self.logger.debug(cmd)
      proc = subprocess.Popen(cmd, stdout=subprocess.PIPE,stderr=subprocess.PIPE, shell=True )
      
      value=None
      ret= True
      value=dict()          
      for line in iter(proc.stderr.readline,''):
         line.rstrip('\n')
         line.strip()
         if len(line) :  self.logger.debug(line)
         ret= False
         #print '==> stderr', len(line), line
      
      for line in iter(proc.stdout.readline,''):
         if ret:
            line.rstrip('\n')
            line.strip()
            #print "==> stdout", len(line), line
            if len(line) :
               self.logger.debug(line)
               (node,sep,last)= line.partition(':')
               value[node]= last.split()[0]
            else:                            
               self.logger.debug('Empty line reveived from strout')
            
      proc.wait()
      return proc.returncode, value

### end class

      
