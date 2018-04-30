#!/bin/python
# encoding: utf-8
#================================================================================
#   
#   server_settings.py
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
.. module:: csmbigdata.config.server_settings
    :platform: Linux
    :synopsis: A python module for loading and storing Remote Server Settings. 
.. moduleauthor:: John Dunham (jdunham@us.ibm.com)
'''
from csmbigdata.config.default_settings import DefaultSettings
from csmbigdata.config.settings_option import SettingsOption 

import os
import configparser

import logging
logger = logging.getLogger(__name__)

def retrieve_access( access_file ):
    ''' A parsing function for RemoteServerSettings. This function is split from
    the class itself to make it possible for it to be used in the automation.

    :param string access_file: The file containing access details.

    :returns dict: A dictionary containing a **userid** and **userpass** field, if 
        the authentication wasn't found the values will be blank strings.
    '''
    if access_file[0] != '/':
        access_file = os.getcwd() + "/" + access_file

    # Load the access config file.
    access_config = configparser.ConfigParser()
    access_config.read(access_file)
    access = DefaultSettings.get_section(access_config, 'access.details')

    # Build the access obj.
    access_ret = { 
        "userid" : access['userid'] if 'userid' in access else '', 
        "userpass" : access['userpass'] if 'userpass' in access else '' 
        }
    
    return access_ret

class RemoteServerSettings(DefaultSettings):
    ''' A Class for storing and loading remote server settings.
    Follows the DefaultSettings model for storing and retrieving the settings.

    | Uses the following sections:

    * dir
    * protocol
    * port
    * host
    * access
        * userid
        * userpass

    '''

        
    def construct_uri( self ):
        ''' Construct the uri from the attributes.
        :returns string: The completed uri compiled from members of this class.
        '''
        logging.debug("Enter RemoteServerSettings construct_uri")

        uri = self.protocol + "://" + self.host
        
        if self.port != -1:
            uri += ":" + str( self.port )
        
        logging.debug("Exit RemoteServerSettings construct_uri")
        return uri + "/" + self.directory + "/"
    
    # ===========================================================================
    # Class Constants
    # ===========================================================================
    #: The map of options for this `Setting Element`.
    CONFIG_MAP = {
        "protocol" : SettingsOption( None, None, "protocol","https", 
            '''The protocol used in connecting to the remote server''',
            '''string''' ),

        "port" : SettingsOption( None, None, "port", -1,
            '''The port to connect to on the remote server''',
            '''int''', int ),
 
        "host" : SettingsOption( None, None, "host", "localhost",
            '''The hostname of the remote server''',
            '''string''' ),
            
        "directory": SettingsOption( None, None, "dir", 'Unity',
            '''Specifies a list of hostnames to filter on, hostnames are comma delimited''',
            '''string''' ),

        "access": SettingsOption( None, None, "access", None,
            '''Specifies a separate configuration file (access level 600 recommended) with an 'access.details' section. 
; This section has a 'userpass' and 'userid' field, which are the access details for the server.
;
; Sample access file contents ( this should be in a separate file, e.g. access.ini ):            
;  [access.details]
;  userid  : user
;  userpass: pass  
; 
; The access field in this file would look like this:
;  access: access.ini
;
; DO NOT add the access.details section to this file''',
            '''string''', retrieve_access )
    }

    #: The name of the section in the config file.
    CONFIG_SECTION = 'remote.server'  

    #: The name of the settings module in the Settings class. 
    CONFIG_NAME = 'remote_server'



