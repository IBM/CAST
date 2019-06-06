# -*- coding: utf-8 -*-                   # remove when Python 3
#==============================================================================
#   
#    hcdiag/src/framework/cmi_interface.py
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

from __future__ import print_function
import subprocess
import os
from datetime import datetime
from target_interface import TargetInterface

DB_ERROR             = 99
ALLOCATION_ERROR     = 89
COMPLETED_FAIL       = 100

#  diag_status values
#  RUNNING            The diagnostics software is currently running.
#  COMPLETED          The diagnostics run completed without a diagnostics software issue. 
#  COMPLETED_FAIL     The diagnostics run completed, but at least one test fails. 
#  FAILED             The diagnostics run encountered a software failure.
#  CANCELED           The diagnostics run was canceled. 

canceled_rc       = [2, 10, 12, 15]
completed_rc      = [0]
failed_rc         = [32, 1, -999, 89, 94]

#----------------------------------------------------------------
""" This class holds the results when running without csm 
    Instead of adding the results to the DB, it will print
    a summary in the console, and save in a file  """
#----------------------------------------------------------------
class Result :

  def __init__(self, ofile) :
    self.fd= open(ofile,'w')
    self.node_pass=[]
    self.node_fail=[]
    self.has_header   = False

  #----------------------------------------------------------------
  """ Add a new result """
  #----------------------------------------------------------------
  def add(self, node, st) :
    if st == 'PASS':
      self.node_pass.append(node)
    else:
      self.node_fail.append(node)

  #----------------------------------------------------------------
  """ Save the results in a disk file """
  #----------------------------------------------------------------
  def save(self, test, start_time) :
    if not self.has_header:
       print('\n', '='*31, 'Results summary ', '='*31, file=self.fd)
       self.has_header= True
    
    np= len(self.node_pass)              
    nf= len(self.node_fail)
    if np :
      print('\n', start_time[11:19], '='*71, '\n', file=self.fd)
      print(test, 'PASS on', np, 'node(s):\n', file=self.fd)  
      print(','.join(self.node_pass), '\n', file=self.fd)
      

    if nf :
      if not np: 
         print('\n', start_time[11:19], '='*71, '\n', file=self.fd)

      print(test, 'FAIL on', nf, 'node(s):\n', file=self.fd)  
      print(','.join(self.node_fail), file=self.fd)

    self.fd.flush()
    return 0
    
  #----------------------------------------------------------------
  """ reset the attributes to store another test """
  #----------------------------------------------------------------
  def reset(self) :
    self.node_pass=[]
    self.node_fail=[]

  #----------------------------------------------------------------
  """ print the summary in the console """
  #----------------------------------------------------------------
  def print_result(self) :
    print('\n', '='*80, file=self.fd)
    ofile= self.fd.name
    self.fd.close()
    self.fd= open(ofile, 'r')
    text= self.fd.read()
    print(text)


#------------------------------------------------------------------- 
""" CsmiInterface class definition
    This class handles all calls to the csm apis
"""
#------------------------------------------------------------------- 
class CsmiInterface(TargetInterface):

   def __init__(self, logger, installdir, usecsm=True, allocation_id=0, runid=0, user=None) :
      super(CsmiInterface,self).__init__(logger)
      self.runid         = runid
      self.usecsm        = usecsm
      self.csmidir       = installdir
      self.user          = user
      self.allocation_id = allocation_id
      self.result        = None
      self.alloc_mine    = False
     
   
   #------------------------------------------------------------------------------ 
   """ Invokes csm api to include a row in the csm_diag_run table:
         csm_diag_run_begin(csm_api_object_t **handle, csm_diag__run_t *newRunData)
       Table contains:
         int self.runid, begin_time, end_time, diag_status, inserted_ras, log_dir
   """
   #------------------------------------------------------------------------------ 
   def start_run(self, log_dir, cmdline):
      if not self.usecsm :
         #print '!!! %s/csm_diag_run_begin -r %s -a %s -l %s -c "%s"' %(self.csmidir, self.runid, self.allocation_id, log_dir, cmdline)
         ofile= '%s/result_summary' %(log_dir)
         self.result= Result(ofile)
         return 0
      
      rc= self.execute_discard_output('%s/csm_diag_run_begin -r %s -a %s -l %s -c "%s"' %(self.csmidir, self.runid, self.allocation_id, log_dir, cmdline))
      self.logger.debug('csmi.start_run ended with rc= {0}.' .format(rc))
      if rc != 0: 
         self.logger.ras('hcdiag.csmi.err', 'api=%s, rc=%d' %('csm_diag_run_begin', rc), os.uname()[1].split(".")[0])
         return DB_ERROR

      return 0
   
   
   #------------------------------------------------------------------------------ 
   """ Invokes csm api to update a row in the csm_diag_run table:
         int csm_diag_run_end(csm_api_object_t **handle, csm_id_t runId, time_t endTimestamp, char status)
       Table contains:
         self.runid, begin_time, end_time, diag_status, inserted_ras, log_dir
   """
   #------------------------------------------------------------------------------ 
   def stop_run(self, ret, ras_inserted=0):
      if not self.usecsm:    
         #print '!!! %s/csm_diag_run_end -r %s -s %s -i %s' %(self.csmidir, self.runid, self.map_run_rc(ret), ras_inserted)
         if self.result: 
            self.result.print_result()
            self.result= None
      else: 
         cmd='%s/csm_diag_run_end -r %s -s %s -i %s' %(self.csmidir, self.runid, self.map_run_rc(ret), ras_inserted)
         rc= self.execute_discard_output(cmd)
         self.logger.debug('csmi.stop_run ended with rc= {0}.' .format(rc) )
         if rc != 0: return DB_ERROR
      return 0
   
   #------------------------------------------------------------------------------ 
   """ Invokes the csm api to create an allocation, i.e., to reserve the nodes 
       in order to run diaganostics
   """
   #------------------------------------------------------------------------------ 
   def create_allocation(self, runid, nodes) :
      if self.allocation_id == "1":  

         # todo: does command line will accept xcat noderange format?
         # for now, nodes is a list and need to convert to the api format: 
         # space separated string

         snode=','.join(nodes)
         
         cmd= '%s/csm_allocation_create -J 0 -j %s -u %s -t %s -s %s -n "%s"' %(self.csmidir, runid, self.user,  'diagnostics', 'running', snode)
         rc, res = self.execute(cmd, 'allocation_id: ', True)                          
         if rc:              
            self.logger.ras('hcdiag.csmi.alloc_err', 'node=%s, rc=%d' %(snode, rc), os.uname()[1].split(".")[0])
            return DB_ERROR
         else:
            self.logger.info('Allocation request successful, id= {0}' .format(res[0]) )
            self.allocation_id= res[0]
            self.alloc_mine=True
         
      return 0
   
   
   #------------------------------------------------------------------------------ 
   """ Invokes csm api to delete the allocation
   """
   #------------------------------------------------------------------------------ 
   def delete_allocation(self) :
      #if not self.usecsm or self.allocation_id == 0: return 0
      if not self.alloc_mine: return 0

      self.logger.info('Request csmd to release the allocation {0}.' .format(self.allocation_id))
      cmd= '%s/csm_allocation_delete -a %s' %(self.csmidir, self.allocation_id)
      rc= self.execute_discard_output(cmd)
      if rc:
         self.logger.error('Allocation delete request for allocation {0} failed, rc= {0}' .format(self.allocation_id, rc))
         self.logger.ras('hcdiag.csmi.allocdel_err', 'id=%s, rc=%d' %(self.allocation_id, rc), os.uname()[1].split(".")[0])
         return DB_ERROR
      else:
         self.logger.info('Allocation {0} delete request successful.' .format(self.allocation_id))

      return 0
   
   #------------------------------------------------------------------------------ 
   # runid, allocationid,test_case, iteration, node_name, serial_number, end_time,
   # hw_status, hw_replace, logf ile, analysis
   #------------------------------------------------------------------------------ 
   def add_result(self, test_case, node_name, serial_number, start_time, logfile, status = 0) :

      if status == 0:                             
        st= 'PASS'
      else: 
        st= 'FAIL'

      if self.usecsm :
         cmd= '%s/csm_diag_result_create -r %s -t %s -n %s -S %s -b \"%s\" -s %s -l %s' %(self.csmidir, 
                 self.runid, test_case, node_name, 
                 serial_number, start_time, st, logfile)
         
         rc= self.execute_discard_output(cmd)
         
         self.logger.debug('csmi.add_result ended with rc= {0}.' .format(rc) )
         
         if rc != 0: return DB_ERROR
      else:
         #print '!!! %s/csm_diag_result_create -r %s -t %s -n %s -S %s -b \"%s\" -s %s -l %s' %(self.csmidir, 
         #        self.runid, test_case, node_name, 
         #        serial_number, start_time, st, logfile)
         self.result.add(node_name, st)

      return 0   
   
                       
   #------------------------------------------------------------------------------ 
   #  ./csm_ras_event_create 
   #        <msg_id>:          message id of record to create.
   #        [-t|--timestamp]   time stamp to use if not specified, then timestamp is now
   #        [-l|--location]    location name/host name for this event\ 
   #        [-r|--rawdata]     raw_data raw data for evetn
   #        [-k|--kvcsv]       key/value csv string, for variable substutition in ras meesage
   #
   #------------------------------------------------------------------------------ 
   def log_ras(self, msgid, kv=None, location=None, raw_data=None):
      if  self.usecsm :
         cmd= '%s/csm_ras_event_create -m %s' %(self.csmidir,  msgid)
         if kv : cmd='%s -k "%s"' %(cmd, kv.replace(", ",","))
         if location : cmd= '%s  -l %s' %(cmd, location)
         if raw_data : cmd= '%s  -r "%s"' %(cmd, raw_data)

         rc= self.execute_discard_output(cmd)
         
         if rc == 0:
            self.logger.info('A RAS event was created in the database for this message')
         else :
            self.logger.debug('Failed to create a RAS event, rc= {0}' .format(rc) )
            return DB_ERROR

      return 0
   

   def query_ras(self) :
      if  self.usecsm :
         cmd= self.csmidir+'/csm_ras_msg_type_query -m "hcdiag.%%"' 
         rc, msg = self.execute(cmd, 'msg_id:', False)
         if rc == 0: return msg
             
      return None


   #------------------------------------------------------------------------------ 
   def map_run_rc(self, ret):
     if ret in canceled_rc:
        return 'CANCELED'
     if ret in failed_rc:
        return 'FAILED'
     if ret == COMPLETED_FAIL:
        return 'COMPLETED_FAIL'
   
     return 'COMPLETED'
   
   #------------------------------------------------------------------------------ 
   
   def map_hw_status(self, status):
     if status in success_hw_status:
        return 'success'
     if status in failed_hw_status:
        return 'failed'
     if status in marginal_hw_status:
        return 'marginal'
     return 'unknown'



   #------------------------------------------------------------------------------ 
   #  run_query
   #------------------------------------------------------------------------------ 
   #
   #def run_query(self, qstring, results=False, ofile=None, ttydisplay=True) :
   #   api= 'csm_diag_run_query'
   #   if results:
   #      api= 'csm_diag_run_query_details'
   #
   #   cmd= '%s/%s %s' %(self.csmidir, api, qstring)
   #   proc = subprocess.Popen(cmd, stdout=subprocess.PIPE,stderr=subprocess.PIPE, shell=True )
   #   
   #   for line in iter(proc.stderr.readline,''):
   #      line.rstrip('\n')
   #      if ofile:
   #         print >> ofile, line,
   #      self.logger.error(line)
   #   
   #   for line in iter(proc.stdout.readline,''):
   #      line.rstrip('\n')
   #      if ofile:
   #         print >> ofile, line,
   #      if ttydisplay:
   #         print line,
   #                               
   #   proc.wait()
   #   if proc.returncode != 0:
   #      return DB_ERROR
   #
   #   return 0
   
   
   #------------------------------------------------------------------------------ 
   #  run_query
   #------------------------------------------------------------------------------ 

   def run_query(self, qstring, runids, results=False, ofile=None, ttydisplay=True) :
      print_ready=True
      api= 'csm_diag_run_query'
      if results:
         if runids and len(runids)==1 :
            api= 'csm_diag_run_query_details'
         else :
            print_ready=False

      cmd= '%s/%s %s' %(self.csmidir, api, qstring)
      if print_ready:
         rc = self.execute_print(cmd, ofile, ttydisplay)
         return rc

      # not ready to print yet
      # if runid were not provided, get them based on the filters
      ids=runids  
      if ids==None:
         rc, ids = self.execute(cmd, "run_id:", True)
         if rc !=0: return rc
      
      if ids != None and len(ids) == 0 :
         self.logger.info('No entries found.')
         return 0

      for id in ids:
         cmd= '%s/csm_diag_run_query_details -r %s' %(self.csmidir, id)
         rc = self.execute_print(cmd, ofile, ttydisplay)
         
      return 0
   
   
   #------------------------------------------------------------------------------ 
   #  execute_print
   #------------------------------------------------------------------------------ 

   def execute_print(self, cmd, ofile=None, ttydisplay=True):
      self.logger.debug(cmd)
      proc = subprocess.Popen(cmd, stdout=subprocess.PIPE,stderr=subprocess.PIPE, shell=True )
      
      for line in iter(proc.stderr.readline,''):
         line.rstrip('\n')
         if ofile:
            print(line, end=' ', file=ofile)
         self.logger.error(line)
      
      for line in iter(proc.stdout.readline,''):
         line.rstrip('\n')
         if ofile:
            print(line, end=' ', file=ofile)
         if ttydisplay:
            print(line, end=' ')
                                  
      proc.wait()
      return proc.returncode

   #------------------------------------------------------------------------------ 
   #  save_result
   #------------------------------------------------------------------------------ 

   def save_result(self, test, start_time):
      if self.result: 
         self.result.save(test, start_time)
         self.result.reset()
      return 0

   #------------------------------------------------------------------------------ 
   #  print_results
   #------------------------------------------------------------------------------ 
   def print_results(self):
      if self.result: 
        text= self.result.fd.read()
        print(text)

