#!/usr/bin/env python                    
# -*- coding: utf-8 -*-                   # remove when Python 3
#=============================================================================
#   
#    hcdiag/src/framework/hcdiag_run.py
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

import sys
import os
import argparse
from datetime import datetime, date, time
import signal
import getpass
import logging

from config_parser import MasterProperties
from config_parser import TestProperties
from log_handler import LogHandler
import csmi_interface as csmi


HCDIAG_PROPERTIES='/opt/ibm/csm/hcdiag/etc/hcdiag.properties'
me='hcdiag_run'
thismodule='hcdiag'
thishost=os.uname()[1].split(".")[0]

#------------------------------------------------------------------------------ 
""" signal handler
"""
#------------------------------------------------------------------------------ 
def signal_handler(signal, frame):
    logger.debug('Signal {0} caught. Terminating...' .format(signal))
    ret=  csmii.delete_allocation()
    logger.debug('Call delete allocation ended with rc= {0}, during signal handling.' .format(ret))
    ret = csmii.stop_run(signal)   
    logger.debug('Call stop_run ended with rc= {0}, during signal handling.' .format(ret))
    tharness.cleanup()
    logger.ras('hcdiag.fwk.signal', 'signal=%d' %(signal), thishost )
    sys.exit(signal)


def subscribe_to_signal():
    signal.signal(signal.SIGINT, signal_handler)           # ctrl C
    signal.signal(signal.SIGUSR1, signal_handler)          
    signal.signal(signal.SIGUSR2, signal_handler)          
    signal.signal(signal.SIGTERM, signal_handler) 
                            
#------------------------------------------------------------------------------ 

def list_available_test_bucket(tconfig, alist) :
   diagtittle= 'Health Check Diagnostics version %s, running on %s %s, %s machine.' \
             %(__version__, os.uname()[0], os.uname()[2], thishost)
   print diagtittle
   tconfig.list(alist)
   print '\ndone...\n' 
   sys.exit(0)


""" Main
"""
#------------------------------------------------------------------------------ 

VERBOSE_LEVEL= {
   'info'       : logging.INFO, 
   'error'      : logging.ERROR,
   'critical'   : logging.CRITICAL,
   'debug'      : logging.DEBUG,
} 


if __name__ == "__main__":

  from version import __version__

  parser=argparse.ArgumentParser()
  group = parser.add_mutually_exclusive_group(required=True)
  group2 = parser.add_mutually_exclusive_group(required=False)
  group3 = parser.add_mutually_exclusive_group(required=False)

  group.add_argument('--test', 
                      type=str, nargs='+', metavar='t', help='test to run')
  group.add_argument('--bucket', '-b',
                      type=str, nargs=1, metavar='b', help='bucket to run')
  group.add_argument('--list', '-l', 
                      type=str, nargs='?', choices=['test','bucket','group', 'all'], const='all', metavar='item', help='list available: test, group, bucket. Default is all')

  parser.add_argument('--target', '-t',
                       type=str, nargs=1, metavar='n[,n ...]', help='target on which to run health check/diagnostic. When running in Management Mode, xcat noderange syntax is accepted.')
                       #type=str, nargs='+', metavar='n',help='target on which to run health check/diagnostic')

  group2.add_argument('--nocsm', 
                       action="store_true", help='do not use csm')
  group2.add_argument('--usecsm', 
                       action="store_true", help='use csm')
  group3.add_argument('--noallocation', 
                       action="store_true", help='do not allocate the target nodes')
  group3.add_argument('--allocation_id', 
                       type=int, metavar='allocation_id', help='allocation_id that reserves the taget')
  parser.add_argument('--fanout', '-f', 
                       type=int, metavar='fanout_value', help='maximum number of concurrent remote shell command processes')
  parser.add_argument('--diagproperties', 
                       type=str, metavar='filename', help='diag properties file')
  parser.add_argument('--testproperties', 
                       type=str, metavar='filename', help='test properties file')
  parser.add_argument('--thresholdproperties', 
                       type=str, metavar='filename', help='threshold properties file')
  parser.add_argument('--logdir', 
                       type=str, metavar='dir', help='root directory for the log files')
  parser.add_argument('--stoponerror',
                       type=str, choices=['no','node','system'], metavar='action', help='define action if test fail {no: continue, node: stop at node level, system: stop the run}')
  # this is console verbose
  parser.add_argument('--verbose', '-v', 
                       type=str, choices=['debug','info', 'warn', 'error', 'critical'], metavar='level', help='stdout verbosity {debug, info, warn, error, critical}')
  parser.add_argument('--version', action='version', version='%(prog)s {version}'.format(version=__version__))


  vlevel= {'debug': logging.DEBUG, 'info' : logging.INFO, 'warn': logging.WARNING, 'error' : logging.ERROR, 'critical' : logging.CRITICAL} 
  args = parser.parse_args()
  
  #-- parse command line arguments, tests|diag properties files
  mconfigt = MasterProperties()

  mprop =  HCDIAG_PROPERTIES if not args.diagproperties else args.diagproperties  
  if not mconfigt.read_file(mprop, args):
     print 'Error reading master properties file {0}. Exiting...' .format(mprop)
     sys.exit(1)

  mconfig = mconfigt.get_attributes()
  #print mconfig

  mgmt_mode=False if mconfig['mgmt_mode']=='no' else True
  tconfig = TestProperties(mconfig['timeout'], mgmt_mode )
  tconfig.read_file(mconfig['testproperties'])


  #-- if it --list option, all other arguments will be ignored, nothing to check
  if args.list:
     if args.target or args.fanout:
        print me, ': --list requested, all other arguments  will be ignored.'
     list_available_test_bucket(tconfig, args.list)
     sys.exit(0)

  #-- arguments validation
  if mgmt_mode: 
     if not args.target :
        print me, ': error:  --target is required.'
        sys.exit(1)

     if args.fanout != None and args.fanout < 1:
        print me, ': error: argument --fanout/-f: xcat fanout value must be greater then zero.'
        sys.exit(1)

  else: # Node mode
     if mconfig['allocation_id'] == '1':
        print me, ': error: when running in Node mode, allocation cannot be requested. Use --noallocation option, or pass an existent allocation_id.'
        sys.exit(1)
     if args.fanout != None:
        print me, ': running in Node mode, --fanout will be innored.'


  usecsm= False
  csmidir= None
  if mconfig['csm'] == 'yes' : 
     csmidir=mconfig['csmi_bindir'] 
     # check if csmi api directory exist
     if not os.path.isdir(csmidir) :
       print 'Can not find csmi directory {0}. Is csmi installed?' .format(csmidir)
       sys.exit(1)
     usecsm= True

  runid=datetime.now().strftime("%y%m%d%H%M%S%f")

  logd= '%s/%s' %(mconfig['logdir'], runid )
  logf= '%s/%s-%s.log' %(mconfig['logdir'], me, runid)

  console_level=VERBOSE_LEVEL[mconfig['console_verbose'].lower()]
  log_level=VERBOSE_LEVEL[mconfig['log_verbose'].lower()]
  bds= True if mconfig['bds'].lower() == 'yes' else False
  

  try:
     logger= LogHandler(thismodule, console_level, log_level, True, bds, logf)                                                       
  except IOError as e:
     print e
     print 'Error creating the log handlers. Exiting...'
     sys.exit(1)

  
  thisuser = getpass.getuser() 
  csmii = csmi.CsmiInterface(logger, csmidir, usecsm, mconfig['allocation_id'], runid, thisuser)

  logger.set_csmi(csmii)
  if mgmt_mode:
     import harness
     tharness= harness.Harness(mconfig, tconfig, logger, runid, logd, csmii, thisuser, args.target)
  else:
     import base_harness
     tharness= base_harness.BaseHarness(mconfig, tconfig, logger, runid, logd, csmii, thisuser, args.target)

  # now we can start logging!
  logger.ras('hcdiag.fwk.started', 'version=%s, os=%s, osversion=%s, location_name=%s' %(__version__, os.uname()[0], os.uname()[2], thishost))
  logger.info('Using configuration file {0}.' .format(mprop))
  if not mgmt_mode: logger.info('XCAT fanout ignored, allocation disabled.')
  logger.info('Using tests configuration file {0}.' .format(mconfig['testproperties']))
  logger.info('Health Check Diagnostics, run id {0}, initializing...' .format(runid))

  #-- validate the test(s)

  if args.test:
    logger.info('Validating command argument test.')
    ret = tharness.validate_test(args.test)
  else:    # it is bucket
    logger.info('Validating command argument bucket.')
    ret= tharness.validate_bucket(args.bucket[0])
                  
  if ret == False:
    logger.error('Invalid test/bucket argument. Exiting...')
    sys.exit(1)

  #-- check target syntax and check if target responds to sshd
  logger.info('Validating command argument target.')
  ret=tharness.validate_target()

  if ret == False:
     logger.error('Error validating target argument. Exiting...')
     sys.exit(1)
                               
  # subscribe for signal  (after csmi initialization)
  subscribe_to_signal()

  # todo: pass xcat noderange
  # we are doing delayed allocation. If all tests are targetType=management, we don't create allocation
  if tharness.create_allocation(runid) != 0 :
     logger.error('Error creating an allocation. Exiting ...')
     sys.exit(csmi.ALLOCATION_ERROR)
  
  # create directory for log files... we do it here to avoid empty directory
  try:
    os.mkdir(logd,0755)
  except OSError as e:
    print e
    logger.critical('Subdirectory {0} creation failed.' .format(logd))
    csmii.stop_run(1)   
    csmii.delete_allocation()
    sys.exit(1)

  # let's update the database, invoking csmi api
  ret = csmii.start_run(logd, ' '.join(sys.argv))   
  if ret != 0: 
     csmii.delete_allocation()
     sys.exit(ret)

  #-- let's run!
  ret = tharness.run()
  csmii.delete_allocation()

  # let's update the database, invoking csmi api
  rc = csmii.stop_run(ret, tharness.ras_inserted)
  if rc != 0:
     logger.error('Error invoking csmi csm_diag_run_end, rc= {0}.' .format(rc))

  logger.ras('hcdiag.fwk.ended', 'rc=%d' %(ret), thishost)

  #-- we return what the run return
  sys.exit(ret)

