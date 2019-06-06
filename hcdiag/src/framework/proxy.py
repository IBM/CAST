# -*- coding: utf-8 -*-                   # remove when Python 3      
#================================================================================
#   
#    hcdiag/src/framework/proxy.py
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
from datetime import datetime, timedelta
import os



#------------------------------------------------------------------- 
""" Proxy class definition
    This class implements the Proxy for the harness, when the test
    is not executed in all target tests.
    - cluster test: executed in one node of the target
    - 
"""
#------------------------------------------------------------------- 
class Proxy:
                                    
   #-------------------------
   """ Class constructor """
   #-------------------------
   def __init__(self):
      self.node = None
      self.rc   = 0
      self.fd   = None
      self.sn   = None
      self.type = None
      self.ofile = None
      self.hostlist = None

   #-------------------------------------------
   """ Build the proxy as management proxy """
   #-------------------------------------------
   def set_proxy_mgt(self):
      self.type = 'mgt'
      self.node= os.uname()[1].split(".")[0]
      with open('/proc/device-tree/system-id','r') as f:
         s=f.readline()
         self.sn=''.join([c for c in s if c.isalpha() or c.isalnum() ])
      #self.ofile='%s/%s/%s-%s.output' %(logdir, test, self.node, datetime.now().strftime("%Y-%m-%d-%H_%M_%S"))

   #-------------------------------------------
   """ Build the proxy as local proxy """
   #-------------------------------------------
   def set_proxy_local(self):
      self.type = 'local'
      self.node= os.uname()[1].split(".")[0]
      with open('/proc/device-tree/system-id','r') as f:
         s=f.readline()
         self.sn=''.join([c for c in s if c.isalpha() or c.isalnum() ])
      #self.ofile='%s/%s/%s-%s.output' %(logdir, test, self.node, datetime.now().strftime("%Y-%m-%d-%H_%M_%S"))

   #---------------------------------------
   """ Build the proxy as cluster proxy """
   #---------------------------------------
   def set_proxy_cluster(self, hostlist, node=None, sn=None) :
      self.hostlist= hostlist
      if node:
         self.node = node
      else :
         self.node= os.uname()[1].split(".")[0]
      self.type = 'cluster'

      if sn :
         self.sn   = sn
      else :
         with open('/proc/device-tree/system-id','r') as f:
            s=f.readline()
            self.sn=''.join([c for c in s if c.isalpha() or c.isalnum() ])

      #self.ofile='%s/%s/%s-%s.output' %(logdir, test, self.node, datetime.now().strftime("%Y-%m-%d-%H_%M_%S"))

   #----------------------
   """ open log file """
   #----------------------
   def open_logfile(self, odir, common_fs) :
      self.ofile = '%s/%s-%s.output' %(odir, self.node, datetime.now().strftime("%Y-%m-%d-%H_%M_%S"))
      if common_fs == 'no':
        try:
           self.fd= open(self.ofile, 'w') 
        except IOError as e:
           return False
       
        return True


   #----------------------
   """ close log file """
   #----------------------
   def close_logfile(self):
      if self.fd != None : self.fd.close()

   #--------------------------------
   """ return the type of proxy """
   #--------------------------------
   def get_type(self):
      return self.type

   def dump(self):
      print("Proxy::node", self.node)
      print("Proxy::rc", self.rc)       
      print("Proxy::fd", self.fd)       
      print("Proxy::sn", self.sn)       
      print("Proxy::type", self.type)     
      print("Proxy::ofile", self.ofile)    
      print("Proxy::hostlist", self.hostlist) 

# end class
