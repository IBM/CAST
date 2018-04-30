#!/bin/python
# encoding: utf-8
#================================================================================
#   
#    default_settings.py
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
.. module:: csmbigdata.config.default_settings
    :platform: Linux
    :synopsis: Defines A Default Settings object that maps user input into a python dictionary.
.. moduleauthor:: John Dunham (jdunham@us.ibm.com)
'''

import sys
import configparser
import logging
logger = logging.getLogger(__name__)


class DefaultSettings(dict):
    ''' Defines the default behaviors for parsing user commandline and configparser input.'''

    #: A mapping of settings attribute to its details.
    CONFIG_MAP = {}

    #: The name of the section in the config file. 
    CONFIG_SECTION = ''
    
    #: The name of the settings module in the Settings class. 
    CONFIG_NAME = 'default'

    def __init__( self, options_list = None, config_section = None, config_map = None):
        ''' Initializes the attributes maintained DefaultSettings. objects.'''
        logging.debug("Enter DefaultSettings init")
        
        #: The config map to use for this settings object, allows for "anonymous" settings.
        self.config_map = config_map if config_map is not None else self.CONFIG_MAP

        if options_list is None:
            options_list = self.config_map.keys()

        #: The config section to use when parsing a configuration file for the setting module.
        self.config_section = config_section if config_section is not None else self.CONFIG_SECTION
        
        #: A list of options enabled for this settings object.
        self.enabled_options = [ option for option in options_list \
            if option in self.config_map ]

        #: A list of attributes generated during runtime by the options.
        self.runtime_attributes = []
        
        # Initialize the options to the values supplied. 
        for option in self.enabled_options:
            self[option] = self.config_map[option].default
        
        logging.debug("Exit DefaultSettings init")

    #===============================================================================
    # Static Methods.
    #===============================================================================
    @staticmethod
    def check_numeric_string( num_str, field_name="" ):
        ''' Determine if the supplied string contains only digits.
        If it doesn't raise a Value Error.

        :param string num_str: The numeric string to check.
        :param string field_name: The field name for the string, for error reporting.

        :raises ValueError: A value error with the context of why the string is not numeric.
        '''
        
        if not num_str.isdigit():
            raise ValueError("Field {0} should be a numeric type. " +\
                "Please verify that \"{1}\" only has digits.".format(num_str, field_name))


    @staticmethod
    def strip_split( dsv_string, data_separator = ',' ):
        ''' Split a data separated string and strip the elements into a list.
        
        :param string dsv_string: The string to split and strip.
        :param string data_separator:  The character(s) to split the string on (',' is default).

        :returns list: A list of strings matching the elements in the dsv string. 
        '''
        return [x.strip() for x in dsv_string.split(data_separator)]
    
    @staticmethod
    def get_section( config, section  ):
        ''' Retrieve the section from the supplied string.

        :param configparser config: The object referencing the configuration 
            holding the section.
        :param string section: The section to extract from the configuration. 

        :return configparser: A section of the supplied configparser `config`
        :return None: If there was no config supplied, let the caller handle it.
        '''
        logger.debug("Enter DefaultSettings get_section")
        
        # EARLY RETURN if no confi was defined, return None.
        if config is None or section is None or section == "":
            return None

        '''Check to see if the section is present.'''
        if config.has_section(section):
            return config[section]
        else:
            # Should this be a print statement?
            logger.warning('Config file was missing section: ' + section )
            return None

        logger.debug("Exit DefaultSettings get_section")
    #===============================================================================

    def parse_config(self, config, args_map=None ):
        ''' Retrive the general confiquration settings for job queries. 
            Store the results in the instance fields.

        :param configparser config: The config parser object pointing to the configuration.
        :param dict args_map: A dictionary containing a mapping of {option => argument}.

        :raises ValueError: If a parse fails catastrophically a value error with 
            the details of the failure is raised.
        '''
        logging.debug("Enter DefaultSettings parse_config")

        section = self.get_section(config, self.config_section)
        
        if section is None:
            self.parse_args( args_map )
            logging.debug("Exit DefaultSettings parse_config") 

            return
        
        # Iterate over the enabled options of this Settings Object and only check for those
        # Options.
        for option in self.enabled_options:
            option_details = self.config_map[option]

            if option_details.config_name is not None and\
                option_details.config_name in section:
                try:
                    value = section[option_details.config_name]
                    # If a parse function was supplied, parse it.
                    # Otherwise determine if it's a flag.
                    if option_details.parse_funct is not None:
                        self[option] = \
                            option_details.parse_funct(value) 
                    else:
                        self[option] = not (self[option]) if option_details.is_flag else value

                    # If this option fuels subsection parsing let that section take over.
                    if option_details.subsection_funct is not None:
                        option_details.subsection_funct(self, self[option], config)
                except Exception, e:
                    logger.error(e)
                    raise ValueError(("'{0}' could not be parsed, the expected pattern for this value is {1}. Received Value: '{2}'").format(option_details.config_name,  
                        option_details.arg_pattern, value ))

        self.parse_args( args_map )

        logging.debug("Exit DefaultSettings parse_config")
    
    def parse_args( self, args_map ):
        ''' Parse the arguments from the commandline. This function should only be 
            invoked from parse_config.

        :param dict args_map: A dictionary containing a mapping of {option => argument}.

        :raises ValueError: If a parse fails catastrophically a value error with 
            the details of the failure is raised.
        '''
        logging.debug("Enter JobSettings parse_args")

        # EARLY RETURN
        if args_map is None:
            return
        
        # For each option supplied search this object's config_map for a match.
        # If it was found, set it.
        for opt in args_map:
            self.parse_opt(opt, args_map[opt])
            #TODO remove args from the map? 


    def parse_opt(self, opt, opt_val):
        ''' Parses the value of an option using the `config_map`
        
        :param string opt: The option being parsed.
        :param string opt_value: The value of the supplied option.

        :return True: The option was found and successfully parsed.
        :return False: The option was not found and unable to be parsed.
        :raises ValueError: If a parse fails catastrophically a value error with 
            the details of the failure is raised.
        '''
        for option in self.enabled_options:
            #  Get the details for the option.
            option_details = self.config_map[option]

            # If the option is in the names.
            if opt in (option_details.short_name, option_details.long_name):
                try:
                    # If a parsing function was specified use it.
                    if option_details.parse_funct is not None:
                        self[option] = option_details.parse_funct(opt_val)
                    else:
                        self[option] = not(self[option]) if option_details.is_flag else opt_val

                    return True
                except Exception, e:
                    logger.error(e)
                    raise ValueError(("{0} could not be parsed, the expected pattern for this value is {1}. Received Value: '{2}'").format(option_details.config_name, 
                        option_details.arg_pattern, opt_val))
                
        return False

    def get_help_strings(
            self, 
            description_offset, 
            description_width, 
            option_offset, 
            option_width ):
        ''' Gets the help strings from the enabled options and returns them 
            as a single constrained string for output to the screen.

        :param int description_offset: The offset of the description paragraph, in characters.
        :param int description_width: The width of the description, in characters.
        :param int option_offset: The offset of the arguments/options in the help string 
            from the description paragraph, in characters.
        :param int option_width: The width of the arguments/options paragraph, in characters.

        :returns string: A help string for all the elements of this config object.
        '''
        help_str = ""
        
        # Iterate over the enabled options and generate their strings.
        for option in self.enabled_options:
            help_str += self.config_map[option].get_help_string( 
                description_offset, 
                description_width, 
                option_offset, 
                option_width ) 

        return help_str

    def get_config_help( self ):
        ''' Builds the config section for this config object.
        
        :returns string: A string containing all of the options in configparser format.
        ''' 

        help_str = "\n[{0}]\n".format(self.config_section)

        for option in self.enabled_options:
            help_str += self.config_map[option].get_config_string()

        return help_str

    def get_values_string( self, string_joiner = "" ):
        '''Creates a value string that contains the key value pair
        of all of the configured values. 

        :param string string_joiner: The string to join different k:v pairs.

        :returns string: A string containing all of the values in the settings
            object.
        '''
        value_string = ""

        for option in self.enabled_options:
            value_string += option + ": " + str(self[option]) + string_joiner
        
        for attribute in self.runtime_attributes:
            value_string += attribute + ": " + str(self[attribute]) + string_joiner

        return value_string

    def append_opts(self, long_opts):
        ''' Appends options to the supplied parameters for the enabled options.
        Uses the getopts format.

        :param list long_opts: A list of the long option strings, 
            options with arguments will be preceded by a '='.
        :returns string: A string containing all of the short options, 
            options with arguments will be preceded by a ':'.
        '''
        short_opts =""
        for option in self.enabled_options:
            if self.config_map[option].short_name is not None:
                short_opts += self.config_map[option].short_name +\
                     ":" * (not self.config_map[option].is_flag)

            if self.config_map[option].long_name is not None:
                long_opts.append(self.config_map[option].long_name  +\
                    "=" * (not self.config_map[option].is_flag))

        return short_opts

    # Accessor Functions
    # TODO do these need docstrings?
    def __getattr__ ( self, attr ):
        return self.get( attr )
        
    def __setattr__ ( self, name, value ):
        self.__setitem__( name, value )

    def __setitem__( self, name, value ):
        super( DefaultSettings, self ).__setitem__( name, value )
        self.__dict__.update( { name: value } )

