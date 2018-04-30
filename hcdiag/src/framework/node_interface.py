# -*- coding: utf-8 -*-                   # remove when Python 3
#==============================================================================
#   
#    hcdiag/src/framework/node_interface.py
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
import re

from target_interface import TargetInterface

def validate_noderange(noderange):
   #-- for now, we expect a sequencde of nodes
   # node01,node02,node03,...node10
   nodelist=None
   if not re.search('[^a-zA-Z0-9,-]', noderange):
      try:
         nodelist=noderange.split(",")
      except:
         nodelist=noderange
   return nodelist

#------------------------------------------------------------------- 
""" NodeInterface class definition
    This class implements the interface with xcat
"""
#------------------------------------------------------------------- 

class NodeInterface(TargetInterface):

   def __init__(self, logger, timeout, bindir=None, fanout=None):                                                                             
      super(NodeInterface, self).__init__(logger, timeout)
      self.name= os.uname()[1].split(".")[0]


   #------------------------------------------------------------------- 
   """ In node mode we only build the data structure
       Retrieve its serial number and if we got to this point,
       node must be ok
   """
   #------------------------------------------------------------------- 
   def get_node_info(self, nodes, target):
      nodelist=validate_noderange(nodes)
      if nodelist == None:
         self.logger.error('Could not validate the target. When running in Node mode, only comma separated nodes syntax is accepted.')
         return False

      for node in nodelist :
         serial_number=None
         if node == self.name:
            with open('/proc/device-tree/system-id','r') as f:
                s=f.readline()
                serial_number=''.join([c for c in s if c.isalpha() or c.isalnum() ])
         else : #  it is mpi, more nodes
             cmd = "ssh " + node + " cat /proc/device-tree/system-id"
             rc, sn= self.execute(cmd)
             if rc == 0:
                serial_number=''.join([c for c in sn[0] if c.isalpha() or c.isalnum() ]) 
             else:
               self.logger.ras('hcdiag.fmk.sn_err', 'node=%s' %(node), self.name)
               return False 

         self.logger.debug('Node {0} has serial number {1}' .format(node, serial_number))
         target[node]= {'fd': None, 'rc' : int(0), 'sn' : serial_number }
      #print "target is", target
      return True


   #----------------------------------------------------------------- 
   # check if the executable is present
   #   
   #----------------------------------------------------------------- 
   def check_exec_target(self, test_exec, target=None):
      return os.path.isfile(test_exec)
   
   #----------------------------------------------------------------- 
   # execute_target
   #   
   #----------------------------------------------------------------- 
   
   def execute_target(self, noderange, cmd):
      return self.execute_discard_output(cmd)

   #----------------------------------------------------------------- 
   # copy_remote
   #   
   #----------------------------------------------------------------- 
   #def copy_to_target(self, target, src, flags = ''):
   #   return  0

##    end class
      
