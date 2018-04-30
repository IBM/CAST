#!/bin/python
# encoding: utf-8
#================================================================================
#   
#   stat_settings.py
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
.. module:: csmbigdata.config.stat_settings
    :platform: Linux
    :synopsis: A python module for loading and storing Statistical Settings.
.. moduleauthor:: John Dunham (jdunham@us.ibm.com)
'''
import configparser
import os
import re

from csmbigdata.config.default_settings import DefaultSettings
from csmbigdata.config.settings_option import SettingsOption 

import logging
logger = logging.getLogger(__name__)

class StatisticalSettings(DefaultSettings):
    ''' A Class for storing and loading statistical settings.
    Follows the DefaultSettings model for storing and retrieving the settings.

    | Uses the following sections:

    * log_sources
    * log_sources_all
    * log_tags
    * log_tag_all
    * stat_options
    
    | Implements the following options:
    |  **Short** : m
    |  **Long**  : metrics

    '''

    def get_sub_sections( target, sources, config ):
        ''' Retrieves the subsections for a collection of sources.
        Extracts the fields and hostnames from the subsection, then stores them in
        a log_source_details dictionary attached to the target.

        The `log_source_details` dictionary is guaranteed to have the following pattern:
            :field_keys: (list) A collection of field keys for the log stource.
            :hostname_key: (string) The name of the hostname field.
            :timestamp_key: (string) The name of the string field.
        
        :param obj target: The target to save the section contents to.
        :param list sources: A collection of log sources/tags to get the field and 
            hostname information from.
        :param configparser config: The configuration containing the sources.
        '''
        logging.debug("Enter StatisticalSettings get_sub_sections")
        # If the target was none, don't bother continuing.
        if target is None:
            return
        
        source_section = None

        # Create a log_source_details field if it's not present.
        if target.log_source_details is None:
            target.log_source_details = {}

        for base_source in sources:
            # Strip the leading /
            source = re.sub(r"^/", '', base_source)
            source_section = target.get_section(config, source)

            if source_section is None:
                continue
            
            fields   = DefaultSettings.strip_split(source_section.get("field_keys"))
            hostname = source_section.get("hostname_key")
            timestamp = source_section.get("timestamp_key")
            
            target.log_source_details[source] = \
                {
                    "field_keys"   : fields, 
                    "hostname_key" : hostname,
                    "timestamp_key": timestamp
                }

        # Append this dictoinary for printing.
        if "log_source_details" not in target.runtime_attributes:
            target.runtime_attributes.append("log_source_details")

        logging.debug("Exit StatisticalSettings get_sub_sections")


    def get_stat_options( option_string ):
        ''' Parses then verifies that the stat options in the configuration are valid.
        
        :param string option_string: A comma separated list of stat options.
        :returns list: The list of "legal" stat options in the config.
        '''
        logging.debug("Enter StatisticalSettings verify_stat_options")
        stat_options = DefaultSettings.strip_split(option_string)

        # Verify the stat options only once.
        bad_values  = []
        good_values = []
        for entry in stat_options:
            entry_lower = entry.lower()  

            if entry_lower in StatisticalSettings.ALLOWED_METRICS:
                good_values.append(entry_lower)
            else:
                bad_values.append(entry_lower)

        if len(bad_values) > 0:
            logger.warning("The following metrics options weren't allowed {%s}.", \
                ", ".join(bad_values))
            logger.warning("Using the following metrics for calculations {%s}.",\
                ", ".join(good_values))

        logging.debug("Exit StatisticalSettings verify_stat_options")

        return good_values

    # ===========================================================================
    # Class Constants
    # ===========================================================================

    #: The map of options for this `Setting Element`.
    CONFIG_MAP = {
        "log_sources" : SettingsOption( None, None, "log_sources", None, 
            '''A collection of log sources, these sources will be filtered in queries''',
            '''csv list''', DefaultSettings.strip_split, get_sub_sections ), 

        "log_sources_all" : SettingsOption( None, None, "log_sources_all", None,
            '''A collection of log sources, these sources will not be filtered in queries''',
            '''csv list''', DefaultSettings.strip_split, get_sub_sections ), 
 
        "log_tags" : SettingsOption( None, None, "log_tags", None,
            '''A collection of log tags, these tags will be filtered in queries''',
            '''string''', DefaultSettings.strip_split, get_sub_sections ), 
            
        "log_tags_all": SettingsOption( None, None, "log_tags_all", None,
            '''A collection of log tags, these tags will not be filtered in queries''',
            '''string''', DefaultSettings.strip_split, get_sub_sections ),

        "stat_options": SettingsOption( None, "metrics", "stat_options", 
            ["min","max","avg","std"],
            '''A collection of statistical options for the query, must be one of the allowed metrics''',
            '''list''', get_stat_options )
    }

    #: The name of the section in the config file.
    CONFIG_SECTION = 'stat.settings'

    #: The name of the settings module in the Settings class. 
    CONFIG_NAME = 'statistics'
    
    #: A Map of authorized metrics to compute.
    ALLOWED_METRICS = {
        "min":"Minimum Value",
        "max":"Maximum Value",
        "avg":"Metrics",
        "std":"Standard Deviation"}

