#!/bin/python
# encoding: utf-8
# ================================================================================
#   
# find_job_keys.py
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
.. module:: usecases.find_job_keys
    :platform: Linux
    :synopsis: A tool for querying the log analysis server, filters to the nodes and time range of the supplied jobid.
.. moduleauthor:: John Dunham (jdunham@us.ibm.com)
'''

# Import the system, regex and json modules.
import sys
import re
import json

# Set the csm_bds_context for development work.
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

# Convenience functions for use with Log Analysis queries.
import csmbigdata.utils.unity_helpers as unity_helper

# Includes some helper functions to ensure that output is consistent across tools.
import csmbigdata.utils.output_helpers as output_helpers

# Include the various date modules.
import dateutil.parser
from datetime import *
from dateutil.relativedelta import *

# Predominantly used for debug loging.
import logging.config
logger=logging.getLogger(__name__)

# Formatting constants. TODO Centralize.
DIV_0 = '=' * 80
DIV_1 = '-' * 80
DIV_2 = '=' * 40
DIV_3 = '@' * 90

class ToolJobKeys (object):
    ''' Facilitates keyword searching against a Log Analysis Server.'''

    def __init__(self, args):
        ''' Initializes the tool. '''

        logger.debug("Begin ToolJobKeys init")
        
        #: The user configurable input to the usecase.
        self.settings = Settings( )

        self.settings.help_string = '''A tool for determining the occurance rate of keywords during the run time of a job 
     in the syslog and mmfs logs. User supplied time ranges will be overriden by the 
     actual time range of the specified job.'''
    
        # self.settings.time
        self.settings.append_class( TimeSettings )
        # self.settings.job_info
        self.settings.append_class( JobSettings )
        # self.settings.remote_server   
        self.settings.append_class( RemoteServerSettings )
        
        # Parse the arguments and config file.
        self.settings.parse_args(args)

        #: The payload for the query (UnityPayload). 
        self.payload          = UnityPayload()

        #: Exposes the CSM CLI to python without needing to use subprocess.
        self.csmi_tool        = CSMITool()

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

        #: Holds the jobs that were running in the specified time range (dict). 
        self.job_id_map       = None

        #: Overall start time for the job (datetime).
        self.job_start_time   = None

        #: Overall end time for the job (datetime).
        self.job_end_time     = None

        #: The allocation id, the user will generally not know this id (int).
        self.allocation_id    = 0

        logger.debug("Exit ToolJobKeys init")
        
    def stat_keywords(self):
        ''' Determines the incidence rate of keywords and displays logs containing the
        keyword specified (if the verbose flag is set).'''

        logger.debug("Enter ToolJobKeys stat_keywords")
    
        # Get the time range
        try:
            self.job_id_map, self.job_start_time, self.job_end_time =  \
                unity_helper.find_job_time_range_csm(
                    self.payload,
                    self.settings.job_info.job_id,
                    self.settings.job_info.secondary_job_id,
                    self.settings.job_info.target_hostnames,
                    self.settings.time.date,
                    self.settings.time.days ) 
        except ValueError as e:
            logger.error("Unable find the job time range, error was: %s", e)
            return 1

        if self.job_id_map is None or len(self.job_id_map) == 0 : 
            if self.settings.job_info.target_hostnames is not None:
                logger.warning("No errors were detected, but jobid '{0}' was not found for targeted hosts: [{1}]. Please consult your settings and try again.".format( 
                    self.settings.job_info.job_id,
                    ",".join(self.settings.job_info.target_hostnames ) ) )

            else:
                logger.warning("No errors were detected, but jobid '{0}' was not found. Please consult your settings and try again.".format( 
                    self.settings.job_info.job_id) )

            logger.debug("Exit ToolJobKeys stat_keywords")
            return 1


        if self.settings.default.verbose:
            unity_helper.print_job_time_range( self.job_id_map, self.settings.job_info.job_id, self.job_start_time, self.job_end_time )

        # Cache the number of hosts.
        num_hosts = len(self.job_id_map)

        logger.debug("Building ToolJobKeys stat_keywords query") 

        self.payload.initialize()
        self.payload.set_logsources("logSource", ["/syslog","/mmfs"])
        self.payload.set_getAttributes( ["timestamp","syslogHostname","message"] )
        self.payload.set_range_timestamp_filter( self.job_start_time, self.job_end_time )
        self.payload.set_term_facet("syslogHostname", num_hosts) 

        # Build the repeated query string.
        query_string = " AND " +  self.payload.scala_host_query( 'syslogHostname', self.job_id_map )

        # Zero the keyword counts.
        for node in self.job_id_map:
            self.job_id_map[node]["keyword_counts"] = [0] * len(self.settings.job_info.keywords)
        
        logger.debug("stat_keywords query built: %s", self.payload.get_json_payload())

        # Cache the baseline variables for the server communication loop.
        default_start = self.payload.get_start()
        keyword_index = 0

        # Map to store the verbose results for formatting properly.
        if self.settings.default.verbose:
            verbose_map = dict()

        logger.debug("Begin BDS communication")
        # Execute the search for each keyword.
        for keyword in self.settings.job_info.keywords:
            logger.debug("Gathering statistics about \'%s\' keyword", keyword) 

            # Finalize the query.
            self.payload.set_query("\"" + keyword + "\"" + query_string)
                
            while self.payload.get_start() >= 0:

                logger.debug("Executing stat_keywords \'%s\' keyword query", keyword)
                # Execute the actual query.
                json_response = json.loads ( self.payload.post( ) )

                # If the total results count is found in the response and the results 
                # exceed the results returned increment the start point for the next iteration.
                # Else set the start to -1 so the execution knows not to iterate.
                self.payload.set_start( 
                    self.payload.determine_start(
                        json_response, 
                        self.payload.get_start(), 
                        self.payload.get_results()))


                # TODO Should facetResults et. all be moved to Constants?
                # get the statistics for each
                if 'facetResults' in json_response and \
                    'term_facet'  in json_response["facetResults"] and \
                    'counts'      in json_response["facetResults"]['term_facet']:

                    logger.debug("Counting for the \'%s\' keyword were found", keyword)

                    for count in json_response["facetResults"]['term_facet']['counts']:
                        self.job_id_map[count['term']]['keyword_counts'][keyword_index] = \
                            count['count']

                # XXX Maybe this should be output to a file?
                # If the verbose option is set cache the messages for output.
                if self.settings.default.verbose and "searchResults" in json_response:
                    logger.debug("Search results for the \'%s\' keyword were" +\
                        " found, gathering for verbose output", keyword)

                    verbose_map[keyword] = dict()
                    for entry in json_response["searchResults"]:
                        attributes=entry.get("attributes")

                        if attributes is None:
                            continue

                        hostname=attributes["syslogHostname"]
                        if hostname not in verbose_map[keyword]:
                            verbose_map[keyword][hostname]  = [] 

                        if 'mmfsEventDescription' in attributes:
                            message = attributes['mmfsEventDescription']
                        elif "message" in  attributes:
                            message = attributes["message"]

                        # TODO should this timestamp be formatted?
                        if "timestamp" in attributes:
                            message = attributes["timestamp"] + ": " + message

                        verbose_map[keyword][hostname].append(message)
                    ''' End for loop '''
            ''' End While Loop '''

            logger.debug("Done gathering statistics about \'%s\' keyword", keyword) 

            # Update the loop sentinels.
            keyword_index += 1
            self.payload.set_start( default_start )

        ''' End For Loop '''
        logger.debug("End BDS communication")


        logger.debug("Keyword statistics gathering complete, outputing results")

        # Pretty Print. 
        print("\nSearched from \n{0} to {1}".format( 
            output_helpers.format_timestamp(self.job_start_time),
            output_helpers.format_timestamp(self.job_end_time)))
        print("\n{0} \nKeyword Statistics\n{1}".format(DIV_0, DIV_1))
        # Print the results to the console.
        for node in self.job_id_map:
            key_index       = 0
            keyword_counts  = " "
            for keyword in self.job_id_map[node]['keyword_counts']:
                keyword_counts += self.settings.job_info.keywords[key_index] +\
                    "=" + unicode(keyword) + " "
                key_index +=1
            print(node + keyword_counts)
        print(DIV_0 + "\n")
        
        if self.settings.default.verbose:
            for keyword in verbose_map:
                print("\n{0}\nContains Keyword: {1}\n{2}\n".format(DIV_2,keyword,DIV_2))
                # TODO this might need to be tokenized.
                for host in verbose_map[keyword]:
                    print("\n" + DIV_2 + "\n" + host + ":\n"+ DIV_2 + "\n")
                    for message in verbose_map[keyword][host]:
                        print message
                print(DIV_1)

        logger.debug("Exit ToolJobKeys stat_keywords")

        return 0

def main(args):
    # Start out using the default log configuration from the csm_bds_context.
    csm_bds_context.load_default_log_conf()

    info_tool = ToolJobKeys(args)

    print("Use Case #2; keyword counts over a time range:")
    rc = info_tool.stat_keywords()

    if rc != 0:
        logger.error("Error Code {0} \nSettings Used:\n{1}".format(rc, info_tool.settings.get_values_string()))

    sys.exit(rc)

if __name__ == "__main__":
    sys.exit(main(sys.argv))
