#!/bin/python
# encoding: utf-8
#================================================================================
#   
# find_jobs_running.py    
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
.. module:: usecases.find_jobs_running
    :platform: Linux
    :synopsis: A tool for determining what jobs were running on a node given the supplied time.
.. moduleauthor:: John Dunham (jdunham@us.ibm.com)
'''

# Import python native modules.
import sys
import re
import math
import json

# Set the csm_bds_context for development
import csm_bds_context

# Include the configuration modules:
from csmbigdata.config.settings      import Settings
from csmbigdata.config.time_settings   import TimeSettings
from csmbigdata.config.job_settings    import JobSettings
from csmbigdata.config.server_settings import RemoteServerSettings

# Unity payload for sending json to the server.
from csmbigdata.utils.unity_payload   import UnityResponse, UnityPayload

from csmbigdata.utils import unity_helpers as unity_helper

# Includes some helper functions to ensure that output is consistent across tools.
import csmbigdata.utils.output_helpers as output_helpers

# Tool for interacting with CSM
import csmbigdata.utils.csmi_tool as csmi_tool

# Include the various date modules.
import dateutil.parser
from datetime import *
from dateutil.relativedelta import *
from dateutil import tz

# Predominantly used for debug logging.
import logging
logger = logging.getLogger(__name__)

DIV_0 = '=' * 80
DIV_1 = '-' * 80
DIV_2 = '=' * 40
DIV_3 = '@' * 90

class ToolJobsRunning (object):
    ''' Contains the functions necessary to check for jobs running at a specified time on a remote LogAnalysis server.
    '''

    def __init__(self, args):
        ''' Initializes the tool. '''
        
        logger.debug("Begin ToolJobsRunning init")
        
        #: The user configurable input to the usecase.
        self.settings = Settings( )

        self.settings.help_string = '''A tool for retrieving the jobs that were running during the specified time.
    The -v or --verbose flag will query the csm apis to aggregate more information
    about the jobs that were found to be running.'''

        # self.settings.time
        self.settings.append_class( TimeSettings )
        # self.settings.job_info
        self.settings.append_class( JobSettings, ["target_hostnames", "no_hosts"])
        # self.settings.remote_server
        self.settings.append_class( RemoteServerSettings )
        
        # Parse the arguments and config file.
        self.settings.parse_args(args)
         
        #: The Log Analysis payload for this tool (UnityPayload).
        self.payload          = UnityPayload()

        #: Exposes the CSM CLI to python without needing to use subprocess.
        self.csmi_tool        = csmi_tool.CSMITool()
        
        ''' Initialize the unity connection. '''
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

        #: Tracks the jobs that were running in the specified time range (dict)
        self.job_alloc_map       = None

    
        logger.debug("End ToolJobsRunning init")

    def find_jobs( self ):
        ''' Finds the jobs running at the specified time. If the verbose flag was
        specified a secondary query will be run. Please consult jobs_running_during
        for details regarding the query against the big data store.

        :returns int: Return Code
        '''
        rc = self.jobs_running_during( )

        if self.settings.default.verbose:
            self.verify_jobs_running()    
        
        self.print_jobs_running()

        return rc

    def jobs_running_during( self ):
        ''' Finds the jobs running at the specified time. Results are output to
        the console. Current iteration of the query works as follows:

        | Key:
        |   a - "--targettime"
        |   b - "--targettime" - "--days"
        |   - - "--days"
        |   | - start/end of scan
        |   ~ - Unexamined days/time
        |
        |    b        a
        | ~~~|<-------|~~~~~~~~~
        |
        | The query first establishes a time range to search for allocation creations/deletions.
        | Using the supplied "--targettime" and "--days" the start and end times are computed with
        | the end time being the target time. Using this time range and any nodes specified a filter
        | is generated to reduce the number of records returned.

        | A query is then sent to the Log Analysis server and the results are parsed.
        | IF create is found
        |     The status of the job (a Boolean) is combined with True on that hostname.
        | ELSE IF delete is found
        |    The status of the job (a Boolean) is combined with False on that hostname.
        | 
        | IF the status of the job cached locally is True
        |     The hostname had that job/allocation running at the target time.

        | Some notes:
        |     1. If the Job was created before the search window and ends after it will not be detected.
        |        a. "--days" should be viewed as a heuristic value of sorts (e.g. the maximum run time).
        |     2. If the Job was created, but not properly destroyed this will create a false positive!'''
        
        logger.debug("Enter ToolJobsRunning jobs_running_during")
        
        # Set the range of time to search in. If no time is specified, usenow for time.
        if self.settings.time.date:
            self.end_time = self.settings.time.date
        else:
            self.end_time = datetime.now()

        self.start_time = self.end_time - relativedelta(days=self.settings.time.days)
        
        # Build the REST POST payload.
        self.payload.initialize()
        self.payload.set_logsources("logSource","/syslog")
        self.payload.set_getAttributes(["syslogHostname","message","timestamp"])
        self.payload.set_range_timestamp_filter(self.start_time, self.end_time)
        self.payload.set_query(unity_helper.BDS_ALLOCATION_QUERY)
        
        # Build query string.
        if self.settings.job_info.target_hostnames is not None:
            self.payload.set_query( self.payload.scala_host_query( \
                "syslogHostname", self.settings.job_info.target_hostnames), "AND")
        
        # Reset the allocation mapping.
        self.job_alloc_map = dict()
        logger.debug("Enter ToolJobsRunning UnityConnection")

        while self.payload.get_start() >= 0:
            # Execute the actual query.
            json_response = json.loads( self.payload.post( ) )

            logger.info("Processing results: %d->%d out of %s", 
                self.payload.get_start(), 
                self.payload.get_start() + self.payload.get_results(),
                json_response.get("totalResults"))
        
            # Set up the start.        
            self.payload.set_start(
                self.payload.determine_start(
                    json_response,
                    self.payload.get_start(),
                    self.payload.get_results()))

            # If an error was found report it and exit.
            if "result" in json_response and\
                json_response["result"].get("status") == "failure":

                logger.error("Error occured in communication: %s",
                    json_response["result"].get("message"))
                continue

            # If the search results were found in the response payload, we can process the data.
            if UnityResponse.SEARCH_RESULTS in json_response:
                # Iterate over the search.
                for entry in json_response[UnityResponse.SEARCH_RESULTS]:
                    attributes = entry[UnityResponse.ATTRIBUTES]
                    # Cache the reused results.
                    search_result  = re.search( unity_helper.BDS_ALLOCATION_EXTRACTOR, 
                        attributes["message"] )
                    hostname       = attributes["syslogHostname"]
                    timestamp      = attributes["timestamp"]

                    if search_result is None:
                        logger.debug("Message didn't have allocation details.")
                        continue

                    alloc_id, al_type, job_id, sec_id = search_result.group(1,2,3,4)

                    if alloc_id not in self.job_alloc_map:
                        # If the allocation id hasn't be found yet create an object for it.
                        self.job_alloc_map[alloc_id] = {
                            "job_id"     : job_id,
                            "sec_id"     : sec_id,
                            "hostnames"  : {},
			                "active"     : 0
                        }
                 
                
                    if hostname not in self.job_alloc_map[alloc_id]["hostnames"]:
                        # If the hostname is not present add it and assume it is running.
                        self.job_alloc_map[alloc_id]["hostnames"][hostname] =  True  
                
                    if  al_type == unity_helper.BDS_ALLOCATION_BEGIN_KEY:
                        # If the begin was found.
                        self.job_alloc_map[alloc_id]["hostnames"][hostname] = True and  \
                            self.job_alloc_map[alloc_id]["hostnames"][hostname]
			            
                        self.job_alloc_map[alloc_id]["active"] += 1
			

                    elif al_type == unity_helper.BDS_ALLOCATION_END_KEY:
                        # end was found.
                        self.job_alloc_map[alloc_id]["hostnames"][hostname] = False and \
                            self.job_alloc_map[alloc_id]["hostnames"][hostname]
                        self.job_alloc_map[alloc_id]["active"] -= 1
       
        logger.debug("Exit ToolJobsRunning UnityConnection")
        
        logger.debug("Exit ToolJobsRunning jobs_running_during")

        # Clear out the inactive allocations. 
        inactive_allocations = []
        for alloc_id in self.job_alloc_map:
            if self.job_alloc_map[alloc_id]["active"] <= 0:
                inactive_allocations.append(alloc_id)

        for allocation in inactive_allocations:
            del self.job_alloc_map[allocation]
        
        return 0

    def verify_jobs_running( self ):
        ''' Verify that the job was running at the time stamp specified using csmi api
        queries. Determines any other nodes that participated in the job.'''
        logger.debug("Enter ToolJobsRunning verify_jobs_running")

        if self.job_alloc_map is None:
            logger.debug("Exit ToolJobsRunning verify_jobs_running. No jobs to verify")
            return 1

        tz_local = tz.tzlocal()

        for alloc_id in self.job_alloc_map:
            # Initialize the new metadata.
            allocation_map = self.job_alloc_map[alloc_id]
            allocation_map["in_db"]      = False
            allocation_map["verified"]   = False
            allocation_map["run_time"]   = "Not Found" 
            allocation_map["start_time"] = "Not Found"
            allocation_map["end_time"]   = "Not Found"
            allocation_map["other_nodes"]= [] 
            

            # Query the csm database, if it fails, continue to the next allocation. 
            try :
                allocation_dict =  self.csmi_tool.allocation_query(alloc_id)

                if allocation_dict is None:
                    logger.debug("Allocation %s was not found in the csm database", alloc_id)
                    continue
                
                allocation_map["in_db"] = True
            except Exception as e:
                logger.error("Allocation %s was not found in the csm database, error: %s",\
                    alloc_id, e )
                continue
            
            # Determine start time.
            job_start_time  = allocation_dict.get(csmi_tool.CSM_AL_BEGIN_TIME)

            # Set the end time, revert to now, if no end time is found. 
            history_dict    = allocation_dict.get(csmi_tool.CSM_AL_HISTORY)
            if history_dict is not None:
                job_end_time    = history_dict.get(csmi_tool.CSM_AL_END_TIME)
            else:
                job_end_time = None
            # Set the endtime to now andcheck if the tzinfo is none. 
            if job_end_time is None:
                job_end_time = datetime.now()
            
            # The timestamp is assumed to be local time if it's not set.
            if job_end_time.tzinfo is None:
                job_end_time = job_end_time.replace(tzinfo=tz_local)
            
            # Add time metadata to the allocation map.
            if job_start_time is not None:
                
                # The timestamp is assumed to be local time if it's not set.
                if self.end_time.tzinfo  is None:
                    self.end_time = self.end_time.replace(tzinfo=tz_local)

                # The timestamp is assumed to be local time if it's not set.
                if job_start_time.tzinfo is None:
                    job_start_time = job_start_time.replace(tzinfo=tz_local)
                
                allocation_map["verified"] = \
                    self.end_time >= job_start_time and \
                    self.end_time <= job_end_time
                
                allocation_map["run_time"]   = job_end_time - job_start_time 
                allocation_map["start_time"] = job_start_time
                allocation_map["end_time"]   = job_end_time
                    
            # Determine additional nodes that participated.
            found_hostnames = allocation_dict.get(csmi_tool.CSM_AL_COMPUTE_NODES)
            if found_hostnames is not None and \
                "hostnames" in self.job_alloc_map[alloc_id]:

                allocation_hostnames = self.job_alloc_map[alloc_id]["hostnames"]
                
                for hostname in found_hostnames:
                    if hostname not in allocation_hostnames:
                        allocation_map["other_nodes"].append(hostname)

        return 0
        
    def print_jobs_running( self ):
        ''' Print the jobs that were found to be running.
        Isolates the output from the business logic.'''

        logger.debug("Enter ToolJobsRunning print_jobs_running")

        print ("")
        print (DIV_0)
        print ("The following jobs were active on the following nodes at " 
            + output_helpers.format_timestamp(self.end_time))
        print ("AllocationID | JobID | SecondaryID | Active Hostnames")
        print (DIV_0)
        
        if self.job_alloc_map is None:
            logger.debug("Exit ToolJobsRunning print_jobs_running")
            return 

        line_limit = len(DIV_1)
        line_count = 0
        tab_count = 5
        tab = " " * tab_count
        # Print the results to the console.
        for alloc_id in self.job_alloc_map:
            active_hosts = 0
            output = alloc_id +  " | " + self.job_alloc_map[alloc_id]["job_id"] + " | " + \
                self.job_alloc_map[alloc_id]["sec_id"] + " | "
            line_count = len(output)     
 
            for host in self.job_alloc_map[alloc_id]["hostnames"]:
                # TODO simplify code.
                if self.job_alloc_map[alloc_id]["hostnames"][host] : 
                    temp_count =  (len(host) + 2)
                    line_count += temp_count
                    
                    if line_count > line_limit:
                        output += "\n" + tab
                        line_count = temp_count  + tab_count

                    output += host + ", "

            print (output[:output.rfind(', ')])

            if self.settings.default.verbose:
                print (DIV_2)
                print ("Found in Database: %s" % \
                        self.job_alloc_map[alloc_id].get("in_db"))
                print ("Time Verified    : %s" % \
                        self.job_alloc_map[alloc_id].get("verified"))
                print ("Running Time     : %s" % \
                        self.job_alloc_map[alloc_id].get("run_time"))
                print ("Start Time       : %s" % \
                    output_helpers.format_timestamp(
                        self.job_alloc_map[alloc_id].get("start_time")))
                print ("End Time         : %s" % \
                        output_helpers.format_timestamp(
                            self.job_alloc_map[alloc_id].get("end_time")))
                    
                others = self.job_alloc_map[alloc_id].get("other_nodes")
                if  others is not None:
                    print ("Additional Nodes : %s" % ", ".join(others))

            print (DIV_1)
            print ("")

        print (DIV_0)

        logger.debug("Exit ToolJobsRunning print_jobs_running")

def main(args):
    # Start out using the default log configuration from the csm_bds_context.
    csm_bds_context.load_default_log_conf()

    running_tool = ToolJobsRunning(args)

    print("\nUse Case #5 Finds jobs that were running at the specified time.")
    rc = running_tool.find_jobs()

    if rc != 0:
        logger.error("Error Code {0} \nSettings Used:\n{1}".format(rc, running_tool.settings.get_values_string()))

    sys.exit(rc)
     
if __name__ == "__main__":
    main(sys.argv)
