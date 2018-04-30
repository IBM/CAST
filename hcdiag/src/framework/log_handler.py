# -*- coding: utf-8 -*-                   # remove when Python 3
#==============================================================================
#   
#    hcdiag/src/framework/log_handler.py
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

import logging
import logging.handlers
from datetime import datetime, date, time
import re

#------------------------------------------------------------------- 
""" LogHandler class definition
    This class implements the interface with log mechanisms
    and also to csm ras subsystem
"""
#------------------------------------------------------------------- 
# ras defines only: INFO, WARNING, FATAL
# we define: debug, info, warning, critical. Mapping is:
# info            : INFO
# error,warning   : WARNING
# critical        : FATAL


ras_msg= {
'hcdiag.fwk.signal'         : { 'sev': 'INFO',   'message' : 'Signal $(signal) caught. Exiting...'}, 
'hcdiag.fwk.started'        : { 'sev': 'INFO',   'message' : 'Health Check Diagnostics version $(version), running on $(os) $(osversion), $(location_name) machine.'}, 
'hcdiag.fwk.ended'          : { 'sev': 'INFO',   'message' : 'Health Check Diagnostics ended, exit code $(rc).'},
'hcdiag.fmk.sn_err'         : { 'sev': 'FATAL',  'message' : 'Could not retrieve the Serial Number of node $(node).'},
'hcdiag.fmk.node_err'       : { 'sev': 'FATAL',  'message' : 'Node $(node) has issue: $(status).'},
'hcdiag.csmi.err'           : { 'sev': 'FATAL',  'message' : 'Error invoking csmi $(api), rc= $(rc). Exiting...'},
'hcdiag.csmi.warn'          : { 'sev': 'WARNING','message' : 'Error invoking csmi $(api), rc= $(rc).'},
'hcdiag.csmi.alloc_err'     : { 'sev': 'FATAL',  'message' : 'Error invoking csmi_allocation_create, node: $(node), rc=$(rc). Check permission or if nodes are in use.'},
'hcdiag.csmi.allocdel_err'  : { 'sev': 'WARNING','message' : 'Error invoking csmi csm_allocation_delete, allocation_id= $(id), rc= $(rc).'},
'hcdiag.test.fail'          : { 'sev': 'ERROR',  'message' : '$(test) FAIL on node $(node), serial number: $(sn), rc= $(rc). (details in $(file))'},
'hcdiag.xcat.noderange_err' : { 'sev': 'FATAL',  'message' : 'Invalid nodes and/or groups in noderange: $(noderange), or user has persmission issues.'},
'hcdiag.xcat.sn_err'        : { 'sev': 'FATAL',  'message' : 'Could not retrieve the serial number of the nodes $(nodes), rc= $(rc). Check noderange syntax/node with problem, xdsh permission.'},
} 

#------------------------------------------------------------------- 
""" LogHandler class is the common place for the logging and ras
    messages handling. """
#------------------------------------------------------------------- 

class LogHandler:

   #------------------------------------------------------------------- 
   """ Class constructor """
   #------------------------------------------------------------------- 
   def __init__(self, name, console_level=logging.INFO, log_level=logging.ERROR, stdout=True, syslog=False, file=None):                                                      
      self.csmi = None
      self.ras_msg_db=[]

      # create logger
      self.logger = logging.getLogger(name)
      self.logger.setLevel(log_level)
      
      
      if file:
         # create file logger handler
         fh = logging.FileHandler(file, mode='w')
         fformatter = logging.Formatter('%(asctime)s %(levelname)s: %(message)s')
         fh.setFormatter(fformatter)
         self.logger.addHandler(fh)
      
      if syslog:
         # create syslog logger handler
         sh = logging.handlers.SysLogHandler(address='/dev/log',facility=logging.handlers.SysLogHandler.LOG_DAEMON)
         sformatter = logging.Formatter(name+": %(levelname)s: %(message)s")
         sh.setFormatter(sformatter)
         self.logger.addHandler(sh)
      
      if stdout:
         # create stream logger handler
         ch = logging.StreamHandler()
         cformatter = logging.Formatter('%(message)s')
         ch.setFormatter(cformatter)
         ch.setLevel(console_level)
         self.logger.addHandler(ch)
      

   #------------------------------------------------------------------- 
   """ Class set csmi instance """
   #------------------------------------------------------------------- 

   def set_csmi(self, csmii) :
      if csmii : 
         self.csmi = csmii
         if self.csmi.usecsm:
            tmp_msg = self.csmi.query_ras()
            if tmp_msg:
               for msg in tmp_msg:
                  # validate the array from db
                  if msg in ras_msg:
                     self.ras_msg_db.append(msg) 
                  else:
                     self.logger.debug('ras message type: {0} is not a valid message type for the Diagnostic application. Ignoring...' .format(msg)) 


   #------------------------------------------------------------------- 
   """ Log the messages based on the severity and create ras event
       If the id passed is not in the dictionary, it logs as debug
   """
   #------------------------------------------------------------------- 
   def ras(self, id, key_value=None, location=None, raw_data=None) :
     rc=0
     if id in ras_msg: 
        # it is one that we know
        msg = ras_msg[id]['message']
        if key_value:
           kv= dict(item.split('=') for item in key_value.split(', '))
           for key, value in kv.iteritems():
             f=r'\$\({0}\)' .format(key)
             r=r'{0}' .format(value)
             msg= re.sub(f, r, msg)
        
        sev=ras_msg[id]['sev']
        ## log it anyway
        if sev == 'FATAL'    : self.logger.critical(msg)
        elif sev == 'ERROR'  : self.logger.error(msg)
        elif sev == 'WARNING': self.logger.warning(msg)
        else                 : self.logger.info(msg)
        
        if id in self.ras_msg_db:
           # it is subscribed, create a ras event
           rc= self.csmi.log_ras(id, key_value, location, raw_data)
     else:
        # this case should not happen
        self.logger.debug('id: {0} is not in the ras_msg table.' .format(id)) 

     return rc
      
   #------------------------------------------------------------------- 
   """ Log a info message """
   #------------------------------------------------------------------- 
   def info(self, msg) :
     self.logger.info(msg)

   #------------------------------------------------------------------- 
   """ Log a error message """
   #------------------------------------------------------------------- 
   def error(self, msg) :
     self.logger.error(msg)

   #------------------------------------------------------------------- 
   """ Log a critital message """
   #------------------------------------------------------------------- 
   def critical(self, msg) :
     self.logger.critical(msg)
   
   #------------------------------------------------------------------- 
   """ Log a debug message """
   #------------------------------------------------------------------- 
   def debug(self, msg) :
     self.logger.debug(msg)

##    end class
         
   
   
