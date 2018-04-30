#!/bin/python
# encoding: utf-8
#================================================================================
#   
#    job_settings.py
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
#================================================================================ 
'''
.. module:: csmbigdata.config.job_settings
    :platform: Linux
    :synopsis: A python module for loading and storing Job Settings.
.. moduleauthor:: John Dunham (jdunham@us.ibm.com)
'''

# Found in config directory.
from csmbigdata.config.default_settings import DefaultSettings
from csmbigdata.config.settings_option import SettingsOption 

# python-configparser
import configparser

# python-dateutil
import dateutil.parser
from datetime import *

# Configure the logging object.
import logging
logger = logging.getLogger(__name__)

class JobSettings(DefaultSettings):
    ''' A Class for storing and loading job settings. 
    Follows the DefaultSettings model for storing and retrieving the settings.

    | Uses the following sections:

    * job_id
    * secondary_job_id
    * keywords
    * target_hostnames
    
    | Implements the following flags:
    
    * nohosts -- Ignore all hostnames.
    * nokeys  -- Ignore all keywords.
    
    | Implements the following options:
    |  **Short** : j, s, k, n 
    |  **Long**   : jobid, jobidsecondary, keywords, hosts
    '''

    # ===========================================================================
    # Class Constants
    # ===========================================================================

    CONFIG_MAP = {
        "job_id" : SettingsOption( "j", "jobid", "job_id", -1, 
            '''Specifies a Job ID for the query, mandatory''',
            '''int''', int),

        "secondary_job_id" : SettingsOption( "s", "jobidsecondary", "secondary_job_id", 0,
            '''Specifies a secondary Job ID for the query, defaults to 0''', 
            '''int''', int ),
 
        "keywords" : SettingsOption( "k", "keywords", "keywords", None,
            '''Specifies a keyword search string for a query, keywords are comma delimited''', 
            '''key[,..]''',  DefaultSettings.strip_split ),
            
        "target_hostnames": SettingsOption( None, "hosts", "target_hostnames", None,
            '''Specifies a list of hostnames to filter on, hostnames are comma delimited''',
            '''hostname[,..]''',  DefaultSettings.strip_split ),

        "no_hosts": SettingsOption( None, "nohosts", None, False,
            '''Don't limit to the hosts specified in the config file''',
            '''''', is_flag = True ),

        "no_keys": SettingsOption( None, "nokeys", None, False,
            '''Don't limit to the keys specified in the config file''',
            '''''', is_flag = True )
    }

    #: The config section name of the JobSettings module.
    CONFIG_SECTION = 'job.settings'

    #: The name of the settings module in the Settings class.
    CONFIG_NAME = 'job_info'

