# -*- coding: utf-8 -*-                   # remove when Python 3
#================================================================================
#   
#    hcdiag/src/framework/timeout_manager.py
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


import sys
import heapq
from datetime import datetime,timedelta
import time
import threading

class TimeoutManager:
   #------------------------------------------------------------------- 
   """Class constructor"""
   #------------------------------------------------------------------- 
   def __init__(self, logger, dotperiod = timedelta(seconds=2), timeout = timedelta(seconds=10)):
       self.heap = []
       self.map = dict()
       self.REMOVED = '<removed>'
       self.last_dot_time = datetime.now()
       self.dotperiod = dotperiod
       self.timeout = timeout
       self.cv = threading.Condition()
       self.all_done = False
       self.logger = logger
       self.dotprinted = dotperiod

   #------------------------------------------------------------------- 
   """Each time some outpu occurs, this method is call to register activity
   """
   #------------------------------------------------------------------- 
   def activity(self, node, ts = False):
       if ts is False:
           ts = datetime.now()
       # print a dot if the last dot was a while ago
       if ts - self.last_dot_time >=  self.dotperiod:
           sys.stdout.write('.')
           sys.stdout.flush()
           self.dotprinted = 1
           self.last_dot_time = ts
       # do we have this node in the list already? remove it
       self.cv.acquire()
       if node in self.map:
           self.map[node][1]=self.REMOVED
           # if head of queue is flagged as removed, pop it away
           while self.heap and self.heap[0][1] is self.REMOVED:
               heapq.heappop(self.heap)
       # insert node into queue
       entry=[ts,node]
       self.map[node]=entry
       heapq.heappush(self.heap,entry)
       self.cv.release()
       #print 'head (oldest) is now', self.heap[0]

   #------------------------------------------------------------------- 
   """Remove the monitored node, we know we won't here from it
   """
   #------------------------------------------------------------------- 
   def remove(self, node):
       self.cv.acquire()
       if node in self.map:
           self.map[node][1]=self.REMOVED
           self.map.pop(node)
       self.cv.release()

   #------------------------------------------------------------------- 
   """ after all tests are done or terminated, this method is called to 
       realease the timeout print thread"""
   #------------------------------------------------------------------- 
   def done(self):
       self.cv.acquire()
       self.all_done = True
       self.cv.notify()
       self.cv.release()
       if self.dotprinted:
          sys.stdout.write('\n')
       sys.stdout.flush()

   #------------------------------------------------------------------- 
   """ this method is called by the timeout print thread, it sleeps most
       of the time unless checking if head of heap has timed out """
   #------------------------------------------------------------------- 
   def alert_thread(self):
       self.cv.acquire()
       while True:
           wait_time = self.timeout
           ts = datetime.now()
           # if head of queue is flagged as removed, pop it away
           while self.heap and self.heap[0][1] is self.REMOVED:
               heapq.heappop(self.heap)
           if self.heap:
               td = (ts - self.heap[0][0])
               if td > self.timeout:
                   entry = self.heap[0]
                   node = entry[1]
                   heapq.heappop(self.heap)
                   entry[0] = ts
                   heapq.heappush(self.heap,entry)
                   self.cv.release()
                   #print '*',node, 
                   sys.stdout.write("*%s " % node)
                   sys.stdout.flush()
                   self.logger.debug('{0} waiting for output' .format(node))  
                   self.cv.acquire()
                   continue
               else:
                   wait_time = self.timeout - td
           if self.all_done:
                self.cv.release()
                return
           self.cv.wait(wait_time.total_seconds()+1)
           if self.all_done:
               self.cv.release()
               return
           

   #------------------------------------------------------------------- 
   """ This method shows dots based on the dotperiod only, 
       no activity is monitored.  """
   #------------------------------------------------------------------- 
   def tick_thread(self):
      while True:
         ts = datetime.now()
         # print a dot if the last dot was a while ago
         if ts - self.last_dot_time >=  self.dotperiod:
            sys.stdout.write('.')
            self.dotprinted = 1
            sys.stdout.flush()
            self.last_dot_time = ts

         if self.all_done:
            return
         
         self.cv.acquire()
         self.cv.wait(self.dotperiod.total_seconds())
         self.cv.release()
         if self.all_done:
            return
