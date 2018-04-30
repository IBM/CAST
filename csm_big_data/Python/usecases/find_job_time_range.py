#!/bin/python
# encoding: utf-8
# ================================================================================
#   
# find_job_time_range.py    
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
# ================================================================================ 
'''
.. module:: usecases.find_job_time_range
    :platform: Linux
    :synopsis: A tool for determining when a job was running.
.. moduleauthor:: John Dunham (jdunham@us.ibm.com)
'''
# Import the system, regex and json modules.
import sys
import re
import json

import csm_bds_context

# Include the configuration modules:
from csmbigdata.config.settings      import Settings
from csmbigdata.config.time_settings   import TimeSettings
from csmbigdata.config.job_settings    import JobSettings
from csmbigdata.config.server_settings import RemoteServerSettings 

# Unity payload for sending json to the server.
from csmbigdata.utils.unity_payload   import UnityPayload

# Tool for interacting with CSM
from csmbigdata.utils.csmi_tool       import CSMITool

# Unity Helper does the heavy lifting.
from csmbigdata.utils import unity_helpers as unity_helper

import logging.config

logger=logging.getLogger(__name__)

# Formatting constants. TODO Centralize.
DIV_0 = '=' * 80
DIV_1 = '-' * 80
DIV_2 = '=' * 40
DIV_3 = '@' * 90

class ToolJobRange (object):
    ''' Class collecting Job Time Range utility. '''

    def __init__(self, args):
        ''' Initializes the tool. '''

        logger.debug("Begin ToolJobRange init")

        #: The user configurable input to the usecase.
        self.settings = Settings( )

        self.settings.help_string = '''A tool for retrieving the start and end time of a job in the LogAnalysis Big Data Store.'''

        # self.settings.time
        self.settings.append_class( TimeSettings )
        # self.settings.job_info
        self.settings.append_class( JobSettings, ["target_hostnames", "no_hosts", "job_id", "secondary_job_id" ])
        # self.settings.remote_server
        self.settings.append_class( RemoteServerSettings )
        
        # Parse the arguments and config file.
        self.settings.parse_args(args)
         
        #: The Log Analysis payload for this tool (UnityPayload).
        self.payload          = UnityPayload()

        #: Exposes the CSM CLI to python without needing to use subprocess.
        self.csmi_tool        = CSMITool()

        #: The payload for the query (UnityPayload). 
        self.payload          = UnityPayload()

        # Initialize the unity connection. 
        try: 
            self.payload.create_connection(
                self.settings.remote_server.construct_uri(),
                self.settings.remote_server.access["userid"], 
                self.settings.remote_server.access["userpass"]) 
        except Exception as e:
            # If the login fails, exit the system with a 1.
            logger.error(e)
            logger.error("Please verify that the remote_server section was properly configured.")

            logger.error("Error Code 1 \nSettings Used:\n{0}".format(
                self.settings.get_values_string()))

            if self.settings.remote_server.access is None:
                logger.error("The access setting MUST point to a separate configuration file.")

            sys.exit(1)

        logger.debug("Exit ToolJobRange init")

    def find_job_time_range(self):
        ''' Find the time range of the specified job id.'''
        
        logger.debug("Enter ToolJobRange find_job_time_range")

        # Get the time range and exit if a value error was thrown, 
        # logging the error to the error log.
        try:
            job_id_map, start_time, end_time =  \
                unity_helper.find_job_time_range_csm(
                    self.payload,
                    self.settings.job_info.job_id,
                    self.settings.job_info.secondary_job_id,
                    self.settings.job_info.target_hostnames,
                    self.settings.time.date,
                    self.settings.time.days )
        except ValueError as e:
            logger.error(e)
            logger.debug("Exit ToolJobRange find_job_time_range")
            return 1

        if job_id_map is None or len(job_id_map) == 0 : 
            if self.settings.job_info.target_hostnames is not None:
                logger.warning("No errors were detected, but jobid '{0}' was not found for targeted hosts: [{1}]. Please consult your settings and try again.".format( 
                    self.settings.job_info.job_id,
                    ",".join(self.settings.job_info.target_hostnames ) ) )

            else:
                logger.warning("No errors were detected, but jobid '{0}' was not found. Please consult your settings and try again.".format( 
                    self.settings.job_info.job_id) )

            logger.debug("Exit ToolJobRange find_job_time_range")
            return 1

        print("")
        rc = unity_helper.print_job_time_range(
            job_id_map, 
            self.settings.job_info.job_id, 
            start_time, 
            end_time)
        
        logger.debug("Exit ToolJobRange find_job_time_range")
        return rc

def main(args):
    # Start out using the default log configuration from the csm_bds_context.
    csm_bds_context.load_default_log_conf()

    info_tool = ToolJobRange(args)
   
    print("\nUse Case #1; time range of an allocation:")
    rc = info_tool.find_job_time_range()

    if rc != 0:
        logger.error("Error Code {0} \nSettings Used:\n{1}".format(rc, info_tool.settings.get_values_string()))

    sys.exit(rc)

if __name__ == "__main__":
    sys.exit(main(sys.argv))
