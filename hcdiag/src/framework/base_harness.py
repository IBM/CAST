# -*- coding: utf-8 -*-                   # remove when Python 3      
#================================================================================
#   
#    hcdiag/src/framework/base_harness.py
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

import os
import subprocess
import time
from threading import Thread
from datetime import datetime, timedelta
                               
from config_parser import TestProperties
from timeout_manager import TimeoutManager
from node_interface import NodeInterface
from proxy import Proxy
import csmi_interface

                               
#------------------------------------------------------------------- 
""" Starts a thread function
"""
#------------------------------------------------------------------- 
def start_thread(tgt, *ags):
    """ This function starts a thread """

    t = Thread( target=tgt, args=ags)
    t.daemon = True
    t.start()
    return t



#------------------------------------------------------------------- 
""" BaseHarness class definition
    This class implements the Diagnostics harness
"""
#------------------------------------------------------------------- 
class BaseHarness(object):                 
                               
   #------------------------------------------------------------------- 
   """Class constructor"""     
   #------------------------------------------------------------------- 
   def __init__(self, mattr, tconfig, logger, runid, logdir, csmii, user, cmd_target, mgmt_mode=False):
      self.tconfig = tconfig
      self.csmi = csmii
      self.tests = []          
      self.noderange=''
      self.tinterface=None

      # xcat noderange syntax is comma separated
      if not mgmt_mode: 
         self.noderange=os.uname()[1].split(".")[0]
         self.tinterface=NodeInterface(logger, mattr['timeout'])

      if cmd_target: 
         if not mgmt_mode: self.noderange+=','
         self.noderange += ','.join(cmd_target)  

      self.proxy = None
      self.target = dict()     
      self.logger = logger     
      self.logdir = logdir     
      self.runid = runid       
      self.mattr = mattr
      self.user = user
      self.exit_code=0       # if one test fail, set to 100
      self.ras_inserted=0    # False
      self.mgmt_mode=mgmt_mode
      #print mattr

                           
   #------------------------------------------------------------------- 
   """ prepare_run   (no xcat)
   """
   #------------------------------------------------------------------- 
   def prepare_run(self, test)   :
      tattr= self.tconfig.get_test_attributes(test)    #= returns dict

      export_cmd='export HCDIAG_LOGDIR=%s/%s' %(self.logdir, test)
      ## cluster test 
      if tattr['clustertest'] == 'yes':
        if len(self.target) < 2:
          self.logger.error('Test: {0} is defined as cluster test, but {1} node was especified.' .format(test, len(self.target)))
          return None

        rc = self.generate_hostlist(test)   ## cluster proxy
        if rc : return None
        export_cmd+= ' MP_HOSTFILE=%s' %(self.proxy.hostlist)   ## /tmp/hcdiag/9999999/host.list or
                                                                ## /home/diagadmin/log/999999/host.list

      elif len(self.target) != 1:
          self.logger.error('Test: {0} is not a cluster test, but {1} node(s) was especified.' .format(test, len(self.target)))
          return None
      else:                                ## local proxy
         self.proxy= Proxy()
         self.proxy.set_proxy_local()
        

      cmd = None
      texec = tattr['executable']
      targs = tattr['args']
      self.logger.debug('Checking if executable: {0} exists on target node(s).' .format(texec)) 
      if self.tinterface.check_exec_target(texec, self.proxy.node) :
         #-- file exist remotely
         self.logger.info('Executable: {0} exists on target node(s).' .format(texec))
         path,fn = os.path.split(texec)
         cmd= '%s; cd %s; ./%s %s' %(export_cmd, path, fn, targs)

      return cmd

   #------------------------------------------------------------------- 
   """ Validades the target 
   """
   #------------------------------------------------------------------- 

   def validate_target(self): 
      rc= self.tinterface.get_node_info(self.noderange, self.target)
      return rc

   #------------------------------------------------------------------- 
   """ store_node_stdout
   """   
   #------------------------------------------------------------------- 
                  
   def store_node_stdout( self, stdout, time_manager ):   
      for line in iter(stdout.readline,''):
         #print "### line stdout", line
         sline=line.rstrip('\n')
         if len(sline) == 0:     #empty line
            self.logger.debug('Exception: {0}. Empty stdout line received from the remote node. Ignoring...' .format(sline))
            continue

         i = sline.find(self.logdir)
         if i >= 0:
            try:
              # it contains the log file
              ofile=sline[i:]
              node=sline.rsplit('/',1)[1].split('-',1)[0]

              found= False
              if self.proxy:
                 if node == self.proxy.node: 
                    self.proxy.ofile=ofile
                    found=True
              else:           
                 if node in self.target:
                    self.target[node]['ofile']=ofile
                    found= True
           
              if found:
                 if self.mattr['watch_output_progress'] != '0':
                    time_manager.activity(node)
            except (ValueError, TypeError) as e:
               self.logger.debug('Exception: {0}. Invalid stdout line received. {1}. Ignoring... ' .format(e, sline))
         else:
             i = line.find('Remote_command_rc =')
             if i >= 0:
                e=[int(s) for s in line[i:].split() if s.isdigit()]
                if len(e): 
                   if self.proxy: 
                      self.proxy.rc= e[0]
                   else: 
                      self.target[node]['rc']= e[0]
                else:
                   self.logger.debug('Message: "{0}", received from an unknown node. Ignoring...' .format(sline))
             else:
                self.logger.debug('Message: "{0}", received from an unknown node. Ignoring...' .format(sline))

   #------------------------------------------------------------------- 
   """ run
   """   
   #------------------------------------------------------------------- 
   def run(self):
      ##-- at this point target and tests are valid

      ntest= len(self.tests)
      to_remove=[]

      for t in self.tests:
        self.logger.info('Preparing to run {0}.' .format(t))
        rc, t_starttime = self.run_test(t)
       
        if rc != 0: return rc

        rc= self.process_test_result(t, t_starttime)
        if rc != 0: return rc 

        if self.proxy:
           if self.proxy.rc:
             if not self.exit_code: self.exit_code= csmi_interface.COMPLETED_FAIL
             if ntest > 1:
                if self.mattr['stoponerror'] == 'system' or self.mattr['stoponerror'] == 'node' :
                    self.logger.debug('Stopping test(s) on all nodes. ')
                    break     # this is the case that a cluster test fails and stoponerror is node or system
           self.proxy = None

        else:   
           for node, rec in self.target.iteritems():
              if  rec['rc'] :   # not zero return code
                 if not self.exit_code: self.exit_code= csmi_interface.COMPLETED_FAIL
                 if self.mattr['stoponerror'] == 'system':  return 0    # we already process the result... we can exit
                 if ntest > 1:
                    if self.mattr['stoponerror'] == 'node':
                       to_remove.append(node)         # remove from the dict
                       self.noderange+=',-'+node      # remove from xcat noderange
                       self.logger.debug('Stopping test(s) on node {0}.' .format(node) )
                    else:
                       self.target[node]['rc']=int(0)
           
           if len(to_remove):
              dict((k, self.target.pop(k, None)) for k in to_remove)
              if len(self.target) == 0:
                 # don't have any node to run 
                 self.logger.info('stoponerror set and there is not more node to continue. Exiting...')
                 break
              to_remote[:] = []
           ntest-=1

      self.cleanup()
      return self.exit_code

   #------------------------------------------------------------------- 
   """ run_test  (no xcat)
   """   
   #------------------------------------------------------------------- 
   def run_test(self, test):
      stime= datetime.now()
      cmd = self.prepare_run(test)
      if cmd == None: return 89, stime.strftime("%Y-%m-%d-%H:%M:%S.%f")   
      # open log files
      self.open_logfile(test)   
                                     
      tattr= self.tconfig.get_test_attributes(test)    #= returns dict
      #-- let the user know test is about to start
      self.logger.debug('Executing {0} on remote node(s)' .format(cmd))
      self.logger.info('{0} started on {1} node(s) at {2}. It might take up to {3}s.' .format(test, len(self.target), stime, tattr['timeout']))
      tm = TimeoutManager(self.logger, timedelta(seconds=int(self.mattr['tty_progress_interval'])), timedelta(seconds=int(self.mattr['watch_output_progress'])))
      if self.mattr['watch_output_progress'] == '0':
         tm_thread = start_thread(tm.tick_thread)
      else:
         tm_thread = start_thread(tm.alert_thread)

      #-- initiate the timeout manager with known nodes
      if self.proxy:            
         tm.activity(self.proxy.node)
      else:
         for node in self.target:
            tm.activity(node)

      #-- let's execute!!
      proc = subprocess.Popen(cmd, stdout=subprocess.PIPE,stderr=subprocess.PIPE, shell=True )
      threads =  [start_thread(self.store_node_stdout, proc.stdout, tm)]
      threads += [start_thread(self.print_log_stderr, proc.stderr, tm)]
      proc.wait()
      for t in threads: t.join()
      tm.done()
     
      while tm_thread.isAlive():
         self.logger.debug('Waiting for the timeout manager thread to end.')
         time.sleep(0.05)
     
      etime= datetime.now()
      self.logger.info('{0} ended on {1} node(s) at {2}, rc= {3}, elapsed time: {4}' .format(test, len(self.target), etime, proc.returncode, etime-stime))
      self.close_logfile()
      
      if not self.mgmt_mode or (self.proxy and self.proxy.type=='mgt'):
         self.proxy.rc=proc.returncode
   
      del tm      # delete the TimeoutMamager
      return 0, stime.strftime("%Y-%m-%d %H:%M:%S.%f")  
     
   #------------------------------------------------------------------- 
   """ process_test_result
   """   
   #------------------------------------------------------------------- 
   def process_test_result(self, test, test_starttime):
      if self.proxy:
         if not self.proxy.ofile: self.logger.error("Invalid log file or directory: " +self.logdir + ". Exiting."); return 1

         ## log the proxy first
         if self.proxy.rc == 0:   
            self.logger.info('{0} PASS on node {1}, serial number: {2}.' .format(test, self.proxy.node, self.proxy.sn))
         else:
            ret=self.logger.ras('hcdiag.test.fail', 'test=%s, node=%s, sn=%s, rc=%d, file=%s' %(test, self.proxy.node, self.proxy.sn, self.proxy.rc, self.proxy.ofile), os.uname()[1].split(".")[0])
            if self.csmi.usecsm and (ret==0):  self.ras_inserted=1

         ret = self.csmi.add_result(test, self.proxy.node, self.proxy.sn, test_starttime, self.proxy.ofile, self.proxy.rc)

         ## log all the nodes for cluster test
         if not ret:
            for node, rec in self.target.iteritems():
               if node != self.proxy.node:
                  ret = self.csmi.add_result(test, node, rec['sn'], test_starttime, self.proxy.ofile, self.proxy.rc)
                  if ret !=0: break
      else:   
         for node, rec in self.target.iteritems():
            if not rec['ofile'] : self.logger.error("Invalid log file or directory: " +self.logdir + ". Exiting."); return 1
            if rec['rc'] == 0: 
               self.logger.info('{0} PASS on node {1}, serial number: {2}.' .format(test, node, rec['sn']))
            else:
               ret=self.logger.ras('hcdiag.test.fail', 'test=%s, node=%s, sn=%s, rc=%d, file=%s' %(test, node, rec['sn'], rec['rc'], rec['ofile']), os.uname()[1].split(".")[0])
               if self.csmi.usecsm and (ret==0) : self.ras_inserted=1

            ret = self.csmi.add_result(test, node, rec['sn'], test_starttime, rec['ofile'], rec['rc'] )
            if ret != 0: break     

      if not self.csmi.usecsm :
         ret= self.csmi.save_result(test, test_starttime)

      return ret

   #------------------------------------------------------------------- 
   """ generate_hostlist (general)
   """   
   #------------------------------------------------------------------- 
   def generate_hostlist(self, test):
     node_tmpdir = '%s/%s/' %(self.mattr['tempdir'], self.runid)    ## /tmp/hcdiag/9999999
     target_file = '%s/host.list' %(self.logdir)                    ## /home/diagadmin/log/9999999/host.list

     proxy_host  = None
     proxy_sn    = None
     host_file   = target_file                                     ## assume the final host.list

     if self.mgmt_mode:                                        ## fist in the target 
        proxy_host= self.target.keys()[0]
        proxy_sn  = self.target[proxy_host]['sn']
        if self.mattr['common_fs'] == 'no' :
           host_file = '/tmp/%s-host.list' %(self.runid)              ##  /tmp/99999999-host.list on mgt node
           target_file='%s/host.list' %(node_tmpdir)                   ## /tmp/hcdiag/9999999/host.list
           # create /tmp/hcdiag/999999999
           rc = self.tinterface.create_target_tmpdir(self.mattr['tempdir'], self.runid, proxy_host)
           if rc:
              self.logger.critical('Error creating {0} on node {1}. Exiting...' .format(node_tmpdir, proxy_host))
              return rc

     if not self.proxy:
        self.proxy= Proxy()
        self.proxy.set_proxy_cluster(target_file,proxy_host, proxy_sn)


     try:
        fd = open(host_file, 'w') 
     except IOError as e:
        self.logger.debug(e)
        self.logger.error('{0} file creation failed. Ending...' .format(host_file))
        return 2

     self.logger.debug('Creating {0} file, with {1} lines.' .format(host_file, len(self.target)))

     for node in self.target: print >> fd, node
     fd.close()                 

     ## we need to copy the host file to the remote nodes
     if self.mgmt_mode and self.mattr['common_fs'] == 'no' :
        #self.logger.info('Copying {0} to {1}:{2}.' .format(host_file, self.proxy.node, target_file))
        rc = self.tinterface.copy_to_target(self.proxy.node, host_file, target_file)
        if rc:
           self.logger.error('Error copying {0} to {1}:{2}.' .format(host_file, proxy_host, target_file))
           return rc

     return 0 

   #------------------------------------------------------------------- 
   """ validate tests
   """   
   #------------------------------------------------------------------- 
   def validate_test(self, tests):
      invalidtest=''
      for t in tests:
         if not self.tconfig.has_test(t):
            invalidtest+=' '+t

      if invalidtest:
         self.logger.error('Invalid test: '+invalidtest)
         return False

      self.tests=tests
      return True

   #------------------------------------------------------------------- 
   """ validate bucket
   """   
   #------------------------------------------------------------------- 
   def validate_bucket(self, b):
      if not self.tconfig.has_bucket(b):
         self.logger.error("Invalid bucket: {0}." .format(b) )
         return False   
      # if bucket exists, test(s) exist and is valid
      self.tests = self.tconfig.get_bucket_tests(b)
      if self.tests == None:
         return False
      return True



   #------------------------------------------------------------------- 
   """ open_logfile       (common)
   """   
   #------------------------------------------------------------------- 
   def open_logfile(self, test):
      odir= '%s/%s' %(self.logdir, test)
      if not os.path.exists(odir):
         self.logger.debug('Creating output directory for {0}.' .format(test))
         try:
            os.mkdir(odir, 0o755)
            self.logger.debug('Directory {0} created.' .format(odir))
         except OSError as e:
            self.logger.critical('Exception: {0}. Subdirectory {0} creation failed.' .format(e, odir))
            return False
   
      
      # if common filesystem we do not need to open the files
      if self.mattr['common_fs'] == 'yes' : return True
      ## checkme
      if self.proxy : return self.proxy.open_logfile(odir, self.mattr['common_fs'])

      # we append the timestamp in case the same test is invoked more then once in the same run
      now= datetime.now().strftime("%Y-%m-%d-%H_%M_%S")
      for t in self.target:
         ofile = '%s/%s-%s.output' %(odir, t, now)
         self.logger.debug('Output file: {0} created.' .format(ofile))
         try:
            self.target[t]['ofile']= ofile    # this is redundant
            self.target[t]['fd']= open(ofile, 'w') 
         except IOError as e:
            self.logger.error(e)
            self.close_logfile()
            return False
   
      return True  

   #------------------------------------------------------------------- 
   """ close_logfile
   """   
   #------------------------------------------------------------------- 
   def close_logfile(self):
      if self.proxy:
         self.proxy.close_logfile()
      else:
         for t in self.target:
           if self.target[t]['fd'] != None:
                 self.target[t]['fd'].close()
        
        
   #-------------------------------------------------------------------- 
   """ This function prints stderr to display and as well as to log file
       stderr is not well formed like stdout  
       Examples of message:
         "Error: Caught SIGINT - terminating the child processes."
         "Error: c460c032 remote shell had error code: 255"
   """ 
   #------------------------------------------------------------------- 
   def print_log_stderr( self, stderr, time_manager):
      fdopen=True
      if self.mattr['common_fs'] == 'yes': fdopen=False

      for line in iter(stderr.readline,''):
         sline=line.rstrip('\n')
         if len(sline) == 0:  # empty line
            self.logger.debug('empty stderr message received.  Ignoring...')
            continue
                   
         self.logger.debug('stderr "{0}" message received.' .format(sline))
         # check to see if the line contains a node
         node= self.string_has_target(sline)
         err=0
         if node:
            i = line.find('error code: ')
            if i < 0: i = line.find(node)
            if i>=0 :   
               e=[int(s) for s in line[i:].split() if s.isdigit()]
               if len(e): 
                  err= e[0]
               else:
                  self.logger.debug('stderr message "{0}", received from node {1}, but no error code' .format(sline, node))

            # make sure we don't overwrite the err from stdout, it is more meaningful 
            if self.proxy:
               if fdopen: fd= self.proxy.fd
               if not self.proxy.rc and err : self.proxy.rc=err
            else:
               if fdopen: fd=self.target[node]['fd'] 
               if not self.target[node]['rc'] and err: self.target[node]['rc']=err
      
            time_manager.remove(node)
            if fdopen:
             print >> fd, sline
             fd.flush()
         else:
            self.logger.debug('stderr message "{0}" received from unknown source.' .format(sline))
               

   #---------------------------------------------------
   """  This method invokes create_allocation if needed
   """   
   #---------------------------------------------------
   def create_allocation(self, runid) :
      # needs to iterate the tests to see if one of them is targetType=compute
      # if targetType is management, we don't do allocation

      # it is noallocation already
      if self.csmi.allocation_id != "1": 
         #print "returning 0"
         return 0
      #else:
      #   print "continue"
      #
      #count=0
      #for test in self.tests:
      #   if self.tconfig.get_test_attribute(test,'targettype')  == 'Compute': 
      #      count+=1 
      #if count == 0 :
      #   self.csmi.allocation_id=0
      #   return 0

      return self.csmi.create_allocation(runid, self.target.keys())


   #-------------------------------------------------------------------- 
   """  This method checks if a string (sterr) has the node name
   """   
   #------------------------------------------------------------------- 
   def string_has_target(self, str):
      for node in str.split():
         if node in self.target :
            return node
      return None

   #----------------------------------------------------
   """  This method does the temporary directory cleanup
   """   
   #----------------------------------------------------
   def cleanup(self):
      self.tinterface.cleanup(self.noderange)

        
### end class


