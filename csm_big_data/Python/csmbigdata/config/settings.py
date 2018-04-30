#!/bin/python
# encoding: utf-8
#================================================================================
#   
#    settings.py
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
.. module:: csmbigdata.config.settings
    :platform: Linux
    :synopsis: A container for managing Settings that the user may change.
.. moduleauthor:: John Dunham (jdunham@us.ibm.com)
'''
# Used by the print_help function.
import csmbigdata.utils.output_helpers as output_helpers

from csmbigdata.config.default_settings import DefaultSettings
from csmbigdata.config.default_command_settings import DefaultCommandSettings

import os
import sys
import getopt
import configparser
import logging
logger = logging.getLogger(__name__)


class Settings(dict):
    ''' A Specialized dictionary that manages "Settings Elements" of type `DefaultSettings`. '''
    def __init__( self ): 
        ''' Initializes a Settings object using an options list. '''
        logging.debug("Enter Settings init")
        
        #: A collection of the settings elements that have been registered in the `Settings` dictionary.
        self.settings_elements = []
    
        #: The help string to be desplayed when :meth:`print_help` is called. This should not outline options.
        self.help_string = ""
        
        #: The default settings for the commandline (e.g. help, verbose, etc.)
        self.default = DefaultCommandSettings()

        logging.debug("Exit Settings init")

    def append_class(self, 
        section_class, 
        options = None, 
        config_section = None, 
        setting_name = None):
        ''' Initializes a class of type `DefaultSettings` using the options provided and then
            adds the resulting object to the Settings dictionary. 
            
            This function will modify both the Settings dictionary and the 
            `settings_elements` list.

            :param class<DefaultSettings> section_class: A reference to a class of type 
                `DefaultSettings`. This class will be instantiated and added to the 
                `Settings` dictionary.
            
            :param list options: A list of options to use from the supplied class. If None
                all class options will be used.
            
            :param string config_section: The name of the config section for this instantiation
                of the class. If None the default section name for the class will be used.
            
            :param string setting_name: The key that this instantiation of the class should 
                have in the `Settings` dictionary. If None the `CONFIG_NAME` of the class
                will be used as a key.
        '''
        logging.debug("Enter Settings append_class")

        if setting_name is None: 
            setting_name = section_class.CONFIG_NAME

        self[setting_name] = section_class( options, config_section )
        self.settings_elements.append(setting_name)

        logging.debug("Exit Settings append_class")
    
    def append_class_anon(self, setting_name, options_map, config_section = None):
        ''' Initializes a `DefaultSettings` class with the supplied `options_map`
            and `config_section`, then stores it in the `Settings` dictionary under
            key `setting_name`. 

            If no `config_section` is supplied, `setting_name` is used as the configuration
            section.

            :param string setting_name: The key that represents this setting element
                in the `Settings` dictionary.

            :param string options_map: A map with a { "Option Name" : `SettingsOption` } key:value
                pair. This defines the options and parsing behavior for the "anonymous" Settings
                options.

            :param string config_section: The config section for this settings element in 
                a configparser ini file.
        '''
        logging.debug("Enter Settings append_class_anon")

        if config_section is None:
            config_section = setting_name
        
        self[setting_name] = DefaultSettings( None, config_section, options_map )
        self.settings_elements.append(setting_name)

        logging.debug("Exit Settings append_class_anon")

    # TODO should this sys exit or give control to invoker.
    def parse_args(self, args):
        ''' Parses and processes the arguments from the user. If a valid config
            file is specified, this function will attempt to parse the contents
            before moving on to the user's command line input.

            The `default` Settings element is always parsed first.

            :param list args: The argument list containing user input.

        '''
        # Build the options objects for retrieving user input.
        logging.debug("Enter Settings parse_args")

        long_opts  = []
        short_opts = self.default.append_opts(long_opts)
        for element in self.settings_elements:
            short_opts += self[element].append_opts( long_opts )

        try:
            opts, args = getopt.getopt(args[1:], short_opts, long_opts)
        except getopt.GetoptError as err:
            logger.error("Invalid option specified: %s", err)
            self.print_help()
            sys.exit(1)

        args_map = dict()
    
        # Build an argument string.
        for opt, arg in opts:
            opt_start = 2 if opt[1] == '-' else 1
            args_map[opt[opt_start:]] = arg

        config_file = None
        try:
            # Process the default opts with a higher precedence
            self.default.parse_config(None, args_map)
            config_file = self.process_defaults()

            # Parse the individual Settings Elements.
            for element in self.settings_elements:
                self[element].parse_config(config_file, args_map)

        except ValueError as error:
            logger.error(error)
            self.print_help()
            sys.exit(1)
            
        logging.debug("Exit Settings parse_args")

    # TODO organization.
    def absolute_path( self, file_name ):
        ''' Gets the absolute path for a file if the path is relative.

        :param string file_name: The file name to retrieve the absolut path of.

        :returns string: The absolute path to `file_name`.
        '''
        if file_name[0] != "/":
            return os.getcwd() + "/" + file_name
        return file_name

    # TODO should this be in csmbigdata.config.default_settings?
    def process_defaults( self ):
        ''' Process the default commandline arguments. See `csmbigdata.config.default_settings`
            for specifics. This function handles --help and --helpconfig, in addition to
            loading the configparser object from an ini file.

            :returns configparser: Returns a configparser object if a config was specified.
        '''
        logging.debug("Enter Settings parse_args")

        # -h, --help 
        if self.default.help:
            self.print_help()
            sys.exit(0)
        
        # Resolve the absolute paths.
        self.default.output = self.absolute_path(self.default.output)
        self.default.config = self.absolute_path(self.default.config)
        
        # --helpconfig
        if self.default.help_config and self.default.output is not None:
            default_conf_contents = "; The sample config file for { }"
            for element in self.settings_elements:
                default_conf_contents += self[element].get_config_help()

            try:
                config_file = open(self.default.output, 'w')
                config_file.write(default_conf_contents)
                config_file.close()
            except:
                logger.error("Unable to write the config help to {0}. Exiting...".format(
                    self.default.output))
                sys.exit(1)

            output_helpers.eprint ( "Sample Configuration file written to: {0}".format(
                self.default.output ))

            # Exit if everything happened clean.
            sys.exit(0)

        
        # Try to load the config file, if it doesn't work don't fail, just return None.
        if self.default.config is not None:
            config = configparser.ConfigParser()

            try: 
                config.read(self.default.config)
            except: 
                logger.warning("Could not load %s", self.default.config)
                config = None
        else:
            config = None
        
        logging.debug("Exit Settings parse_args")

        return config
            

    def print_help(self):
        ''' Prints a help message built from the Settings elements 
            owned by the invoking object. Printed to stderr.
        '''
        logging.debug("Enter Settings print_help")

        # TODO precursor help output.
        output_helpers.eprint ( " " * 4 + self.help_string )
        output_helpers.eprint ( " " * 4 + "Options:" )
        output_helpers.eprint ( " " * 4 +  "=" * 84 )

        output_helpers.eprint(
            self.default.get_help_strings( 4, 44, 10,  30))

        # TODO Smarter sizing.       
        for element in self.settings_elements:
            output_helpers.eprint(
                self[element].get_help_strings( 4, 44, 10,  30) )

        logging.debug("Exit Settings print_help")

    def get_values_string( self ):
        ''' Dumps the values of the `Settings` dictionary in a human readable format.

        Used in debugging problems with an execution.

        :returns string: A human readable string of all of the key value pairs in
            the `Settings` dictionary.
        '''
        logging.debug("Enter Settings get_values_string")
        
        value_string = "-"*7 + "\ndefault\n" + "-"*7 + "\n"
        value_string += self.default.get_values_string("\n")

        for element in self.settings_elements:
            value_string += "\n" + "-" * len(element) +\
                "\n" + element + "\n" +  "-" * len(element) + "\n"
            value_string += self[element].get_values_string("\n")

        logging.debug("Exit Settings get_values_string")

        return value_string
        
    # Accessor Functions
    # TODO do these need docstrings?
    def __getattr__ ( self, attr ):
        return self.get( attr )
        
    def __setattr__ ( self, name, value ):
        self.__setitem__( name, value )

    def __setitem__( self, name, value ):
        super( Settings, self ).__setitem__( name, value )
        self.__dict__.update( { name: value } )


