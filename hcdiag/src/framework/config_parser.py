# -*- coding: utf-8 -*-                   # remove when Python 3
#================================================================================
#   
#    hcdiag/src/framework/config_parser.py
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
import os

import ConfigParser

#----------------------------------------------------------------- 
"""DiagProperties base class """
#----------------------------------------------------------------- 

class DiagProperties(object):

   #----------------------------------------------------------------- 
   """Class constructor"""
   #----------------------------------------------------------------- 
   def __init__(self):
      self.cfgparser = ConfigParser.ConfigParser()
       

   #----------------------------------------------------------------- 
   """Returns all the attributes of a section """
   #----------------------------------------------------------------- 
   def get_attributes(self, section):                            
      try:
         return (dict(self.cfgparser.items(section)))
      except ConfigParser.NoSectionError:
         print 'ERROR: Attributes for ', section, ' not found in the properties file.'
         return None 

      return None 


   #----------------------------------------------------------------- 
   """Returns the value of an attribute"""
   #----------------------------------------------------------------- 
   def get_attribute(self, section, attribute):
      if self.cfgparser.has_section(section): 
         try:
           return self.cfgparser.get(section, attribute)
         except ConfigParser.NoOptionError:
           return None
           
      return None

#----------------------------------------------------------------- 
""" MasterProperties class"""
#----------------------------------------------------------------- 

class MasterProperties(DiagProperties):
   #----------------------------------------------------------------- 
   """Class constructor"""
   #----------------------------------------------------------------- 
   def __init__(self):
      super(MasterProperties, self).__init__()
      self.section = 'MasterConfig'

   #----------------------------------------------------------------- 
   """Returns a dictionary of all the attributes                 """
   #----------------------------------------------------------------- 
   def get_attributes(self):
       return super(MasterProperties,self).get_attributes(self.section)

   #----------------------------------------------------------------- 
   """Returns the value of an attribute"""
   #----------------------------------------------------------------- 
   def get_attribute(self, attribute):
      return super(MasterProperties, self).get_attribute(self.section, attribute)

   #----------------------------------------------------------------- 
   """Read and parse the diagnostic properties file
      [test properties file, threshold properties file, allocation, usecsm,
      StopOnError, logdir, verbose"""
      #-- [testpropfile, threshpropfile, alloc, usecsm, stoponerror, logdir
      #-- verbose, --fanout]
   #------------------------------------------------------------------ 
   def read_file(self, file, args):

      self.cfgparser.read(file)
      if not self.cfgparser.has_section(self.section):
        return False

      #-- parse xcat_bindir from the hcdiag.properties file
      #-- if xcat_bindir is present, assuming we are running Management mode
      xdir='/opt/xcat/bin'
      try:         
         xdir=self.cfgparser.get(self.section, 'xcat_bindir')
      except ConfigParser.NoOptionError:
         self.cfgparser.set(self.section, 'xcat_bindir',  xdir)

      mgmt_mode='no'
      if os.path.isdir(xdir):
         mgmt_mode='yes'
         print 'INFO: xcat seems to be installed in {0}. Running in Management mode' .format(xdir)
      else:
         print 'INFO: running in Node mode.'
      self.cfgparser.set(self.section, 'mgmt_mode', mgmt_mode)


      ##-- check first if csm environment is going to be used, ultimate default is "yes"
      try:                           
         usecsm=self.cfgparser.get(self.section, 'csm').lower()
      except ConfigParser.NoOptionError:
         usecsm='yes'

      allocation='yes'
      if usecsm == 'no': allocation = 'no'
      if args and args.usecsm       : usecsm='yes'; allocation='yes'
      if args and args.nocsm        : usecsm='no'; allocation='no'
      if args and args.noallocation : allocation='no'
      self.cfgparser.set(self.section, 'csm', usecsm)
      self.cfgparser.set(self.section, 'allocation', allocation)


      ddir='/opt/ibm/csm/hcdiag'
      try:                           
         ddir=self.cfgparser.get(self.section, 'installdir')
      except ConfigParser.NoOptionError:
         self.cfgparser.set(self.section,'installdir', ddir)

      if usecsm=='yes':
         cdir='/opt/ibm/csm/bin'
         try:                           
            cdir=self.cfgparser.get(self.section, 'csmi_bindir')
         except ConfigParser.NoOptionError:
            self.cfgparser.set(self.section,'csmi_bindir', cdir)

      # if properties is passed, has to be fully qualified
      if args and args.testproperties:
         self.cfgparser.set(self.section, 'testproperties', args.testproperties)
      else:
         try:                           
            fname=self.cfgparser.get(self.section, 'testproperties')
            if not fname.startswith(ddir):
               self.cfgparser.set(self.section, 'testproperties', ddir+'/etc/'+fname )
         except ConfigParser.NoOptionError:
            self.cfgparser.set(self.section, 'testproperties', ddir+'/etc/test.properties' )

      if args and args.thresholdproperties:
         self.cfgparser.set(self.section,'thresholdproperties', args.thresholdproperties)
      else:
         try: 
            fname=self.cfgparser.get(self.section, 'thresholdproperties')
            if not fname.startswith(ddir):
               self.cfgparser.set(self.section, 'thresholdproperties', ddir+'/etc/'+fname )
         except ConfigParser.NoOptionError:
            self.cfgparser.set(self.section, 'thresholdproperties', ddir+'/etc/threshold.properties')
              
      if args and args.logdir:
         self.cfgparser.set(self.section, 'logdir', args.logdir)
      try:         
         self.cfgparser.get(self.section, 'logdir')
      except ConfigParser.NoOptionError:
         self.cfgparser.set(self.section, 'logdir',  '/var/ibm/csm/hcdiag/log')

      if args and args.stoponerror:
         self.cfgparser.set(self.section, 'stoponerror', args.stoponerror)
      else:
         try: 
            self.cfgparser.get(self.section, 'stoponerror')
         except ConfigParser.NoOptionError:
            self.cfgparser.set(self.section, 'stoponerror', 'no')
            
      if args and args.verbose:
         self.cfgparser.set(self.section, 'console_verbose', args.verbose)
      else:
         try: 
            self.cfgparser.get(self.section, 'console_verbose')
         except ConfigParser.NoOptionError:
            self.cfgparser.set(self.section, 'console_verbose', 'info')

      try: 
         self.cfgparser.get(self.section, 'log_verbose')
      except ConfigParser.NoOptionError:
         self.cfgparser.set(self.section, 'log_verbose', 'debug')


      if args and args.fanout:
         self.cfgparser.set(self.section, 'xcat_fanout', str(args.fanout))
      else:
         try: 
            self.cfgparser.get(self.section, 'xcat_fanout')
         except ConfigParser.NoOptionError:
            self.cfgparser.set(self.section, 'xcat_fanout', '64')
      
      cfs='no'  # default is no
      try:         
         cfs=self.cfgparser.get(self.section, 'common_fs')
      except ConfigParser.NoOptionError:
         self.cfgparser.set(self.section, 'common_fs',  cfs)

      default_watch='0'    # default is do not watch the output
      if mgmt_mode =='no' or cfs == 'yes':
         self.cfgparser.set(self.section, 'watch_output_progress',  default_watch)
      else:
         try:         
            self.cfgparser.get(self.section, 'watch_output_progress')
         except ConfigParser.NoOptionError:
            self.cfgparser.set(self.section, 'watch_output_progress', default_watch)

      try:         
         self.cfgparser.get(self.section, 'xcat_cmd')
      except ConfigParser.NoOptionError:
         self.cfgparser.set(self.section, 'xcat_cmd',  'no')


      try:         
         self.cfgparser.get(self.section, 'timeout')
      except ConfigParser.NoOptionError:
         self.cfgparser.set(self.section, 'timeout',  '10')

      try:         
         self.cfgparser.get(self.section, '')
      except ConfigParser.NoOptionError:
         self.cfgparser.set(self.section, 'timeout',  '10')


      try:         
         self.cfgparser.get(self.section, 'tty_progress_interval')
      except ConfigParser.NoOptionError:
         self.cfgparser.set(self.section, 'tty_progress_interval',  2)


      return True

   #----------------------------------------------------------------- 
   """ TestProperties class"""
   #----------------------------------------------------------------- 

class TestProperties(DiagProperties):

   #----------------------------------------------------------------- 
   """Class constructor"""
   #----------------------------------------------------------------- 

   def __init__(self,timeout, mgmt_mode):
      super(TestProperties, self).__init__()
      self.test_prefix = 'tests.'
      self.bucket_prefix = 'bucket.'
      self.timeout= timeout
      self.mgmt_mode= mgmt_mode

   #----------------------------------------------------------------- 
   """Returns all the attributes of a test"""
   #----------------------------------------------------------------- 
   def get_test_attributes(self, test):
      return super(TestProperties,self).get_attributes(self.test_prefix+test)

   #----------------------------------------------------------------- 
   """Returns all tests """
   #----------------------------------------------------------------- 
   def get_sections(self, section):
      if section == 'test':
         prefix= self.test_prefix
      else:
         if section == 'bucket':
            prefix= self.bucket_prefix
         else: 
            return None
        
      rl= []
      s=len(prefix)
      l=self.cfgparser.sections()
      for i in l:
        if i.startswith(prefix):
          rl.append(i[s:])
      return rl

   #----------------------------------------------------------------- 
   """Returns all buckets """
   #----------------------------------------------------------------- 
   def get_buckets(self):
      bl= []
      b=self.cfgparser.sections()
      s=len(self.bucket_prefix)
      for i in b:
        if i.startswith(self.bucket_prefix):
          bl.append(i[s:])
      return bl

   #----------------------------------------------------------------- 
   """Returns all the attributes of a bucket"""
   #----------------------------------------------------------------- 
   def get_bucket_attributes(self, bucket):
      return super(TestProperties,self).get_attributes(self.bucket_prefix+bucket)

   #----------------------------------------------------------------- 
   """Returns the value of a test attribute"""
   #----------------------------------------------------------------- 
   def get_test_attribute(self, test, attribute):
      return super(TestProperties,self).get_attribute(self.test_prefix+test, attribute)

   #----------------------------------------------------------------- 
   """Returns the value of a bucket"""
   #----------------------------------------------------------------- 
   def get_bucket_attribute(self, bucket, attribute):
      return super(TestProperties,self).get_attribute(self.bucket_prefix+bucket, attribute)

   #----------------------------------------------------------------- 
   """Returns the bucket's tests list"""
   #----------------------------------------------------------------- 
   def get_bucket_tests(self, bucket):
      tests = self.get_bucket_attribute(bucket, 'tests')
      if tests:
         return tests.replace(' ','').split(',')
      return None

   #----------------------------------------------------------------- 
   """Check if a test exists """
   #----------------------------------------------------------------- 
   def has_test(self, test):
      if self.cfgparser.has_section(self.test_prefix+test): 
        return True
      return False

   #----------------------------------------------------------------- 
   """Check if a bucket exists """
   #----------------------------------------------------------------- 
   def has_bucket(self, bucket):
      if self.cfgparser.has_section(self.bucket_prefix+bucket): 
        return True
      return False

   #----------------------------------------------------------------
   """Read and parse the test properties file
      [name, description, group, timeout, targetType, executable, parser]"""
   #----------------------------------------------------------------- 
   def read_file(self, file):
      self.cfgparser.read(file)

      if (len(self.cfgparser.sections())) == 0:
         print  'ERROR: Test configuration file ', file, 'not found, or empty.'
         sys.exit(98)

      todel=[]   # collect sections that are not valid
      for section in self.cfgparser.sections():

         if section.startswith(self.test_prefix):
            # it is a test, make sure the test has an executable and timeout
            if not self.cfgparser.has_option(section, 'executable'):
               print 'ERROR: No executable for the test: ', section, 'in the ', file, '. Exiting...'
               sys.exit(98)

            try:
               tmp=self.cfgparser.get(section, 'targettype')
               if tmp == 'Management' and not self.mgmt_mode:
                  todel.append(section)
                  continue;
            except:
               self.cfgparser.set(section, 'targettype', 'Compute')


            if not self.cfgparser.has_option(section, 'xcat_cmd'):
               self.cfgparser.set(section, 'xcat_cmd', 'no')
            if not self.cfgparser.has_option(section, 'clustertest'):
               self.cfgparser.set(section, 'clustertest', 'no')
            if not self.cfgparser.has_option(section, 'timeout'):
               self.cfgparser.set(section, 'timeout', self.timeout)
            if not self.cfgparser.has_option(section, 'targetType'):
               self.cfgparser.set(section, 'targetType', 'Compute')
            if not self.cfgparser.has_option(section, 'args'):
               self.cfgparser.set(section, 'args', '')

         elif section.startswith(self.bucket_prefix):   
            # it is a bucket, make sure it has tests
            try:
               ts=self.get_bucket_tests(section[len(self.bucket_prefix):])
               if ts == None:
                  print 'ERROR: ', file, ' contains ', section, ' with no tests attribute or it is empty. Exiting...'
                  sys.exit(98)
               
               invalidtest=''
               sectionadded=False
               for t in ts:
                 s=self.test_prefix+t
                 if not self.cfgparser.has_section(s):
                    invalidtest+=t+' '
                 elif s in todel and not sectionadded:
                       todel.append(section)
                       sectionadded=True

               if invalidtest:
                  print 'ERROR: ', file, ' contains invalid test: ', invalidtest, ' in ', section, '. Exiting...'
                  sys.exit(98)


            except ConfigParser.NoOptionError:
               print 'ERROR: No tests defined for this bucket: ', section, ' in the ', file, '. Exiting...'
               sys.exit(98)

         else:
            print 'ERROR: Invalid section: ', section, ' in the ', file, '. Exiting...'
            sys.exit(98)


      for s in todel:
         print "Info:", s, "ignored, can not run in Node mode."
         self.cfgparser.remove_section(s)
       

   #----------------------------------------------------------------
   """ print the:
      test, description
      bucket, tests
      group, tests """
   #----------------------------------------------------------------- 

   def list(self, item):
      t=None
      if item == 'test' or item == 'all':
        t= self.get_sections('test')    #= returns list
        if not len(t):
           print 'No tests defined.' 
           # it is safe to return at this point, even if all is passed
           # if tests are not defined, buckets are not valid...
           return 
                
        print '\nAvailable tests: ', len(t)
        for test in t:
           print '%-22s: %s' %(test, self.get_test_attribute(test, 'description'))    

      if item == 'bucket' or item == 'all':
        b= self.get_sections('bucket')    #= returns list
        if not len(b):
           print 'No buckets defined.'                         
        else:
           print '\nAvailable buckets: ', len(b)
           for bucket in b:
              print '%-16s: %s' %(bucket, self.get_bucket_tests(bucket))

      if item == 'group' or item == 'all':
        group=dict()
        if not t:
           t= self.get_sections('test')    #= returns list
        for test in t:
           g=self.get_test_attribute(test, 'group')    
           if g :
              if not g in group:
                group[g]=[]
              group[g].append(test)

        if len(group):
           print '\nAvailable groups: ', len(group)
           for g, t in group.iteritems(): 
              print '%-16s: %s' %(g, t)
        else:
           print 'No groups defined.'                         

