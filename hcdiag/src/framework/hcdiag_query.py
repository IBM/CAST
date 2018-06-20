#!/usr/bin/env python                    
# -*- coding: utf-8 -*-                   # remove when Python 3
#=============================================================================
#   
#    hcdiag/src/framework/hcdiag_query.py
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
import logging
import re

from config_parser import MasterProperties
from log_handler import LogHandler
import harness
import csmi_interface as csmi


HCDIAG_PROPERTIES='/opt/ibm/csm/hcdiag/etc/hcdiag.properties'
CSM_BINDIR='/opt/ibm/csm/bin'

#------------------------------------------------------------------------------ 
""" signal handler
"""
#------------------------------------------------------------------------------ 
def signal_handler(signal, frame):
    logger.info('\nSignal {0} caught. Exiting...' .format(signal))
    sys.exit(signal)

def subscribe_to_signal():
    signal.signal(signal.SIGINT, signal_handler)           # ctrl C
    signal.signal(signal.SIGUSR1, signal_handler)          
    signal.signal(signal.SIGUSR2, signal_handler)          
    signal.signal(signal.SIGTERM, signal_handler) 
                            
def check_positive(value):
    ivalue = int(value)
    if ivalue < 0:
         raise argparse.ArgumentTypeError("%s is an invalid positive int value" % value)
    return ivalue



#------------------------------------------------------------------------------ 
""" parse_input_timestamp: parse syntax and few validation
    let the rest to the database
"""
#------------------------------------------------------------------------------ 
def parse_input_timestamp(dt):
   year  = None
   month = 1
   day   = 1
   hour  = 0
   min   = 0
   sec   = 0

   try :
      m= re.search(r"(?P<year>\d\d\d\d)", dt)
      year=int(m.group('year'))
      if year < 2017: logger.error("Invalid date format, year our of range: {0}." .format(dt)); sys.exit(1)
      try :
         m= re.search(r"(?P<year>\d\d\d\d)\-(?P<month>\d\d)", dt)
         month=int(m.group('month'))
         if month > 12: logger.error("Invalid date format, month out of range: {0}." .format(dt)); sys.exit(1)
         try :
            m= re.search(r"(?P<year>\d\d\d\d)\-(?P<month>\d\d)\-(?P<day>\d\d)", dt)
            day=int(m.group('day'))
            if day > 31: logger.error("Invalid date format, day out of range: {0}." .format(dt)); sys.exit(1)
            try :
               m= re.search(r"(?P<year>\d\d\d\d)\-(?P<month>\d\d)\-(?P<day>\d\d)\-(?P<hour>\d\d)", dt)
               hour=int(m.group('hour'))
               if hour > 24: logger.error("Invalid time format, hour out of range: {0}." .format(dt)); sys.exit(1)
               try :
                  m= re.search(r"(?P<year>\d\d\d\d)\-(?P<month>\d\d)\-(?P<day>\d\d)\-(?P<hour>\d\d)\:(?P<min>\d\d)", dt)
                  min=int(m.group('min'))
                  if (min > 60) or ((hour==24) and (min>0)) : logger.error("Invalid time format, minutes out of range: {0}." .format(dt)); sys.exit(1)
                  try :
                     m= re.search(r"(?P<year>\d\d\d\d)\-(?P<month>\d\d)\-(?P<day>\d\d)\-(?P<hour>\d\d)\:(?P<min>\d\d)\:(?P<sec>\d\d)", dt)
                     sec=int(m.group('sec'))
                     if (sec > 60) or ((hour == 24) and (sec > 0)): logger.error("Invalid time format, seconds out of range: {0}." .format(dt)); sys.exit(1)
                  except AttributeError: pass
               except AttributeError: pass
            except AttributeError: pass 
         except AttributeError: pass 
      except AttributeError: pass 
   except AttributeError: 
      logger.error("Invalid date/time format: {0}." .format(dt)); sys.exit(1)

   return '%d-%02d-%02d %02d:%02d:%02d' %(year, month, day, hour, min, sec)

#------------------------------------------------------------------------------ 
""" Main
"""
#------------------------------------------------------------------------------ 
                         
if __name__ == "__main__":
  # subscribe for signal

  from version import __version__
  logger = LogHandler('hcdiag_query', logging.DEBUG)
  #logger.open_console(logging.DEBUG)

  # now we can start logging!
  logger.info('{0}, version {1}, running on {2} {3}, {4} machine.' \
         .format( __file__, __version__, os.uname()[0], os.uname()[2], os.uname()[1].split(".")[0]))

  
  parser=argparse.ArgumentParser()
  parser.add_argument('--runid', '-r',
                       type=str, nargs='+', metavar='id',help='unique udentifier of the diagnostic run. All other arguments will be ignored.')

  parser.add_argument('--allocation', '-a',
                       type=str, nargs='+', metavar='id',help='unique udentifier of the allocation')

  parser.add_argument('--date', '-d',  
                       type=str,  nargs='+', help='date of the diagnostic run, format: yyyy-mm-dd-hh:mm:ss. If a second date is passed, the interval will be used in the query')
  #                    type=str, metavar='DATE [DATE]', help='date of the diagnostic run, format: yyyy-mm-dd-hh:mm:ss. If a second date is passed, the interval will be used in the query')


  parser.add_argument('--status', '-s',  
                       type=str, nargs='+', help='status of the run {runnning, completed, canceled, failed}')

  parser.add_argument('--ras',
                       type=str, choices=['yes', 'no'], help='ras message inserted for this run {yes, no}}')

  parser.add_argument('--results', 
                       action="store_true", help='include results of the run, tests and status per node')
  
  parser.add_argument('-o', '--output', 
                       type=str, metavar='file', help='store the output into a file')
  parser.add_argument('--version', action='version', version='%(prog)s {version}'.format(version=__version__))

  args = parser.parse_args()



  query_string=''
  if args.runid:
     query_string = '-r ' + ','.join(x for x in args.runid)
     logger.info('run_id was specified, all other arguments will be ignored')
  else:
     if args.allocation:
        query_string = '-a ' + ','.join(x for x in args.allocation)
     
     if args.date:
        dt= parse_input_timestamp(args.date[0]) 
        if dt: query_string = '%s -b "%s"' %(query_string, dt)
     
        if len(args.date) > 1:
           dt = parse_input_timestamp(args.date[1]) 
           if dt: query_string = '%s -B "%s"' %(query_string, dt)
     
     if args.status:
        query_string += ' -s ' + ','.join( x.upper() for x in args.status)
     
     if args.ras:
        st='T'
        if args.ras == 'no': st='F'
        query_string += ' -i ' + ' '.join(st);
    
  csmii = csmi.CsmiInterface(logger, CSM_BINDIR)

  
  # we need to redirect the output at this point if -o option is passed
  ofile=None
  if args.output:
     try:
        ofile= open(args.output, 'w') 
     except IOError as e:
        logger.error(e)
        sys.exit(1)
  
  if not query_string:
     logger.info('No arguments passed to the query, it might take several minutes...')

  rc= csmii.run_query(query_string, args.runid, args.results, ofile)

  if rc:
     logger.error('Query failed with rc = {0} ' .format(rc))

  if ofile:
        logger.info('output saved, file:  {0} ' .format(args.output))

  logger.info('done')
  sys.exit(rc)


