#!/bin/python
# encoding: utf-8
#================================================================================
#   
#    default_command_settings.py
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
.. module:: csmbigdata.config.default_command_settings
    :platform: Linux
    :synopsis: A python module for default commandline options.
.. moduleauthor:: John Dunham (jdunham@us.ibm.com)
'''

# Found in config directory.
from csmbigdata.config.default_settings import DefaultSettings
from csmbigdata.config.settings_option import SettingsOption 

# Configure the logging object.
import logging
logger = logging.getLogger(__name__)

class DefaultCommandSettings(DefaultSettings):
    ''' A Class for storing and loading default commandline settings.
    Do not implement this class, allow **Settings** to manage.
    Follows the DefaultSettings model for storing and retrieving the settings.

    | Implements the following flags:
    |  **Short**  : h, v, f
    |  **Long**   : help, helpconfig, verbose, force


    | Implements the following options:
    |  **Short**  : c, o
    |  **Long**   : config, output
    '''
    
    # ===========================================================================
    # Class Constants
    # ===========================================================================
    #: This module does NOT support the config file.
    CONFIG_SECTION = None

    #: The name of the settings module in the Settings class.
    CONFIG_NAME = 'cmd'

    #: The default options for the settings commandline.
    CONFIG_MAP = {
        "help"   : SettingsOption( "h", "help", 
            description = '''Displays this message''', is_flag = True ),

        "help_config"   : SettingsOption( None, "helpconfig", 
            description = '''Generates a sample config file at `output`''', 
            is_flag = True ),
        
        "verbose"  : SettingsOption( "v", "verbose", 
            description = '''Display additional information''', is_flag = True ),

        "force"  : SettingsOption( "f", "force", 
            description = '''Display additional information''', is_flag = True ),

        "output" : SettingsOption( None, "output", None, "tool.output",
            '''Specifies an output file, usecase dependent''', '''file''' ),
        "config" : SettingsOption( "c", "config", None, "default.ini",
            '''Specifies a configuration file''', '''file''' )

    }


