#!/bin/python
# encoding: utf-8
#================================================================================
#   
#    time_settings.py
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
"""
.. module:: csmbigdata.config.time_settings
    :platform: Linux
    :synopsis: A python module for loading and storing TimeSettings.
.. moduleauthor:: John Dunham (jdunham@us.ibm.com)
"""

from csmbigdata.config.default_settings import DefaultSettings
from csmbigdata.config.settings_option import SettingsOption

import configparser
import dateutil.parser

from datetime import *
import logging
logger = logging.getLogger(__name__)

class TimeSettings(DefaultSettings):
    """ A Class for storing and loading job settings. 
    Follows the DefaultSettings model for storing and retrieving the settings.
    
    | Uses the following sections:
    
    * date
    * days
    
    | Implements the following options:
    |  **Long**   : date, days

    """

    def compute_time_range(self):
        """ Computes the time range. If no time was supplied the end time is set as now,
        otherwise the supplied time is set as the endtime.
        
        :returns (Datetime, Datetime): 2-tuple containing the start and end date information. 
            ( start_time, end_time )
        """
        end_time = self.target_datetime
        if end_time is None:
            end_time = datetime.now()
        start_time = end_time - relativedelta(days=self.num_days)
        return (start_time, end_time)

    def compute_time_range_timestamps(self):
        """ Computes the time range and then converts it to unix timestamps.
        
        :returns (int, int): 2-tuple containing the start and end date information as a unix 
            timestamp ( start_time, end_time )
        """
        time_range = self.compute_time_range()
        return (int(time_range[0].strftime('%s')) * 1000, int([1].strftime('%s')) * 1000)

    CONFIG_MAP = {
        'date': SettingsOption( None, 'targettime', 'target_datetime', datetime.now(), 
            'The target date time to focus the operation on, overriden if the use case finds a better time for the query in csm. An empty string indicates now', 
            'MM-DD-YYYY HH:MM:SS.sX', dateutil.parser.parse ),

        'days': SettingsOption( None, 'days', 'num_days', 7, 
            'The number of days to perform the operation on, starting from the target time going backwards. Overriden if the use case finds a better range for the query in csm', 
            'float', float)}

    CONFIG_SECTION = 'time.settings'

    CONFIG_NAME = 'time'
