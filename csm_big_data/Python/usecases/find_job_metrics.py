#!/bin/python
# encoding: utf-8
#================================================================================
#  find_job_metrics.py
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
.. module:: usecases.find_job_metrics
    :platform: Linux
    :synopsis: A tool for computing metrics based on the execution time range of a job.
.. moduleauthor:: John Dunham (jdunham@us.ibm.com)
'''

# Import python native/installed packages.
import sys
import numpy
import json
import dateutil

# Import the csm_bds_context for Development.
import csm_bds_context

# Include the configuration modules:
from csmbigdata.config.settings      import Settings
from csmbigdata.config.time_settings   import TimeSettings
from csmbigdata.config.job_settings    import JobSettings
from csmbigdata.config.server_settings import RemoteServerSettings
from csmbigdata.config.stat_settings   import StatisticalSettings

# Unity payload for sending json to the server.
from csmbigdata.utils.unity_payload   import UnityPayload, UnityResponse

# Convenience functions for use with Log Analysis queries.
import csmbigdata.utils.unity_helpers as unity_helper

# Includes some helper functions to ensure that output is consistent across tools.
import csmbigdata.utils.output_helpers as output_helpers

# Tool for interacting with CSM
from csmbigdata.utils.csmi_tool       import CSMITool

import socket
import logging
import re
logger = logging.getLogger(__name__)

class MetricsMetadata(object):
    '''Defines the metadata used to compute metrics. An object of this class should represent 
    either a log source or log tag in the Log Analysis Big Data Store.'''

    #: A regular expresion for detecting the hostname field.
    HOSTNAME_RE = r'''(?i)hostname'''

    #: A regular expresion for detecting the timestamp field.
    TIMESTAMP_RE = r'''(?i)^timestamp'''

    def __init__( self, attributes= None, hostname_key= None, field_keys= None, timestamp_key= None ):
        ''' Initializes the metadata object. Builds a metadata object if the attributes field is specified by the user.

            :param json attributes: The attributes of an entry in the response of a Log Analysis POST query.
            :param string hostname_key: The hostname field name in the data source.
            :param list field_keys: A list of strings containing the field names to perform metrics operations on.
            :param string timestamp_key: The timestamp field name in the data source.
        '''
        logger.debug("Entering  MetricsMetadata __init__")
        
        #: The key of the hostname in the log source (String). 
        self.hostname    = None

        #: The key of the timestamp in the log source (String).
        self.timestamp   = None

        #: A mapping of the metrics names to header location (dict).
        self.metrics     = dict()
        
        #: The number of of fields tracked by the metrics dictionary (int).
        self.num_metrics = 0
        
        #: The fields that the metrics will be calculated upon.
        #: This is typically used for output (list).
        self.headers     = []

        if attributes is not None:
            self.build_metadata( attributes, hostname_key, field_keys, timestamp_key )

        logger.debug("Exit  MetricsMetadata __init__")
    
    def build_metadata( self, attributes, hostname_key= None, field_keys= None, timestamp_key= None ):
        ''' Generates a metadata object for storing metric data.

            :param json attributes: The attributes of an entry in the response of a Log Analysis POST query.
            :param string hostname_key: The hostname field name in the data source.
            :param list field_keys: A list of strings containing the field names to perform metrics operations on.
            :param string timestamp_key: The timestamp field name in the data source.
        '''
        logger.debug("Entering MetricsMetadata build_metadata")
        
        self.hostname  = hostname_key
        self.timestamp = timestamp_key
        self.metrics   = dict()
        self.headers   = []
        self.metrics[unicode("timestamp")] = 0
        
        field_counter = 1
        
        # Determine the metadata for the numeric fields, hostname and timestamp.
        for attribute in attributes:
            if timestamp_key is None and \
                re.search( MetricsMetadata.TIMESTAMP_RE, attribute, re.IGNORECASE ):
                self.timestamp = attribute

            elif hostname_key is None and \
                re.search( MetricsMetadata.HOSTNAME_RE, attribute, re.IGNORECASE ):
                self.hostname = attribute

            elif field_keys is None:
                try:
                    value = float( attributes[attribute])
                except ValueError:
                    pass
                else:
                    self.metrics[attribute] = field_counter
                    self.headers.append(attribute)
                    field_counter += 1

        # Set configured field metadata
        if field_keys is not None:
            for attribute in field_keys:
                self.metrics[attribute] = field_counter
                self.headers.append(attribute)
                field_counter += 1
                
        self.num_metrics = field_counter

        logger.debug("Exiting MetricsMetadata build_metadata")

    def build_metric_dict(self):
        ''' Creates the dictionary used to store metrics data.
            
            | Dictionary uses the following k:v structure:
            | 
            | {
            |     "count" : int,
            |     "sum"   : list<float>,
            |     "min"   : list<float>,
            |     "max"   : list<float>,
            |     "avg"   : list<float>,
            |     "std"   : list<float>,
            |     "raw"   : list<list<float>>
            | }
        '''

        temp_dict = dict()

        temp_dict["count"] = 0
        temp_dict["sum"]   = [0.0] * self.num_metrics
        temp_dict["min"]   = [sys.float_info.max] * self.num_metrics
        temp_dict["max"]   = [-1 * sys.float_info.max] * self.num_metrics
        temp_dict["avg"]   = [0.0] * self.num_metrics
        temp_dict["std"]   = [0.0] * self.num_metrics
        
        temp_dict["raw"]   = []

        for i in range(self.num_metrics):
            temp_dict["raw"].append([])

        return temp_dict

    def get_raw_index(self, field):
        ''' Gets the index for the supplied field.

            :param string field: The field to get the index of.
            :return: The index of a field.
            :rtype: int
        '''
        return self.metrics.get(field)
    
    def field_exists(self, field):
        ''' Determines if a field exists for metric analysis in the metrics dict.
            
            :param string field: The field to check in the metrics map.
            :return: Whether or not the field exists.
            :rtype: Boolean
        '''
        return field in self.metrics


class ToolJobMetrics(object):
    ''' A tool for aggregating metrics from nodes that participated in the specified job id over the job execution time.'''
    

    def __init__(self, args):
        ''' Initializes the metrics tool.'''
        logger.debug("Enter ToolJobMetrics init")

        #: The user configurable input to the usecase.
        self.settings = Settings( )

        self.settings.help_string = '''A tool for retrieving metrics from nodes that participated in the supplied job id.
   Uses the JobSettings, RemoteServerSettings and StatisticalSettings configuration modules.'''

        # self.settings.time
        self.settings.append_class( TimeSettings )
        # self.settings.job_info
        self.settings.append_class( JobSettings, ["no_hosts", "target_hostnames", "job_id", "secondary_job_id"] )
        # self.settings.remote_server
        self.settings.append_class( RemoteServerSettings )
        # self.settings.statistics
        self.settings.append_class( StatisticalSettings )
        
        # Parse the arguments and config file.
        self.settings.parse_args(args)

        #: The payload object for connecting to the Log Analysis server (UnityPayload).
        self.payload = UnityPayload()
        
        #: Exposes the CSM CLI to python without needing to use subprocess.
        self.csmi_tool        = CSMITool()

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
        
        logger.debug("Exit ToolJobMetrics init")
    
    def get_job_metrics(self):
        ''' Gets the metrics from the Log Analysis server during the job execution on the specified nodes.'''
        logger.debug("Enter ToolJobMetrics get_job_metrics")

        # Get the time range and exit if a value error was thrown, logging the error to the error log.
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
            return 1

        if job_id_map is None or len(job_id_map) == 0 : 
            if self.settings.job_info.target_hostnames is not None:
                logger.warning("No errors were detected, but jobid '{0}' was not found for targeted hosts: [{1}]. Please consult your settings and try again.".format( 
                    self.settings.job_info.job_id,
                    ",".join(self.settings.job_info.target_hostnames ) ) )

            else:
                logger.warning("No errors were detected, but jobid '{0}' was not found. Please consult your settings and try again.".format( 
                    self.settings.job_info.job_id) )

            logger.debug("Exit ToolJobMetrics get_job_metrics")
            return 1

        if self.settings.default.verbose:
            unity_helper.print_job_time_range(job_id_map, self.settings.job_info.job_id, start_time, end_time)

        if job_id_map is not None:
            keys = job_id_map.keys()
        else:
            keys = []

        # Get the "filtered" metrics
        try:
            ip_addrs = self.modify_unity_payload(
                self.payload,
                start_time,
                end_time,
                keys,
                self.settings.statistics.log_sources,
                self.settings.statistics.log_tags)
        except ValueError as e:
            logger.error(e)
            return 1

        metrics, metadata = self.get_metrics_unity(
            self.payload, 
            self.settings.statistics.log_source_details,
            keys,
            ip_addrs)

        self.print_metrics( 
            metrics, 
            metadata, 
            start_time, 
            end_time, 
            self.settings.job_info.job_id,
            self.settings.statistics.stat_options)
        
        # Get the "unfiltered" metrics
        ip_addrs = self.modify_unity_payload(
            self.payload,
            start_time,
            end_time,
            log_sources = self.settings.statistics.log_sources_all,
            tags        = self.settings.statistics.log_tags_all)
        
        metrics, metadata = self.get_metrics_unity(
            self.payload,
            self.settings.statistics.log_source_details )

        self.print_metrics( 
            metrics, 
            metadata, 
            start_time, 
            end_time, 
            self.settings.job_info.job_id,
            self.settings.statistics.stat_options)

        return 0
            
    @staticmethod
    def modify_unity_payload(
        payload,
        start_time, 
        end_time, 
        nodes=None, 
        log_sources=None, 
        tags=None
    ):
        ''' Modifies the supplied payload for a metrics query. Executes initialize_payload() before populating the payload for the metrics query.

            :param UnityPayload payload: A payload object with an existing connection to a Log Analysis Server
            :param datetime start_time: The start of the range to aggregate metrics for.
            :param datetime end_time: The end of the range to aggregate metrics for.
            :param list nodes: The nodes to perform the statistical analysis on.
            :param list log_sources: A collection of Log Analysis logSource values to search in the query.
            :param list tags: A collection of Log Analysis tags to search in the query.
            :returns: A maping of ip to ip address.
            :rtype: dict'''
        payload.initialize()
        
        # Set the log sources.
        append = False
        if log_sources is not None:
            payload.set_logsources( "logSource", log_sources )
            append = True
        
        if tags is not None:
            payload.set_logsources( "tags", tags, append )

        ip_addrs = None
        if nodes is not None:
            # Get the ip address mapping.
            ip_addrs = ToolJobMetrics.map_ip( nodes )
            if ip_addrs is not None:
                payload.set_query(payload.scala_host_query(None,(nodes + ip_addrs.keys())))

        # Construct the payload.
        payload.set_range_timestamp_filter(start_time, end_time)

        return ip_addrs

    @staticmethod 
    def get_metrics_unity(
        payload,
        sources,
        nodes=None,
        ip_addrs=None
    ):
        ''' Queries a LogAnalysis server and performs metrics analysis on the results.

            :param UnityPayload payload: A payload object that has been configured through modify_unity_payload with an active connection to a LogAnalysis server.
            :param dict sources: A dictionary containing the hostname and fields for log sources.
            :param list nodes: A collection of hostnames/ip addresses
            :return: A dictionary of metric data and a MetricsMetadata object.
            :rtype: dict, MetricsMetadata'''

        # Initialize the metadata and metrics objects.
        metadata_objects = dict()
        metrics          = { "global" : {} }
        filter_on_nodes  = nodes is not None 

        if filter_on_nodes:
            for node in nodes:
                metrics[node] = {}
       
        # Query the server
        while payload.get_start() >= 0:
            json_response = json.loads(payload.post())
            
            payload.set_start( payload.determine_start( json_response ) )

            if UnityResponse.SEARCH_RESULTS not in json_response:
                continue
            
            for entry in json_response[UnityResponse.SEARCH_RESULTS]:
                # Get and cache the data source for the entry.
                attributes = entry.get(UnityResponse.ATTRIBUTES)
                data_source = attributes.get(UnityResponse.DATA_SOURCE)

                if data_source not in metadata_objects:
                    keys = dict()

                    if sources is not None and \
                        data_source in sources:
                        keys = sources[data_source]
                    
                    # Parameter expansion is great - John Dunham
                    metadata_objects[data_source] = \
                        MetricsMetadata( attributes, **keys )
                        
                    for metric in metrics:
                        metrics[metric][data_source] = \
                            metadata_objects[data_source].build_metric_dict()
                    
                # Resolve the hostname this entry belongs to.
                hostname = attributes.get(metadata_objects[data_source].hostname)
                if filter_on_nodes and (ip_addrs is not None and hostname in ip_addrs):
                    hostname = ip_addrs[hostname]

                # Build metrics object for the hostname specified in the entry.
                if hostname not in metrics:
                    metrics[hostname] = {}
                if data_source not in metrics[hostname] :
                    metrics[hostname][data_source] = \
                        metadata_objects[data_source].build_metric_dict()    
                
                try:
                    if metadata_objects[data_source].timestamp is None:
                        logger.debug("%s data source timestamp was not set")
                        continue
                        # FIXME give the user a choice?
                    # Resolve the timestamp.
                    timestamp  = attributes.get(metadata_objects[data_source].timestamp)
                    last_index = timestamp.rfind(':')
                    timestamp  = timestamp[:last_index] + "." +  timestamp[last_index+1:]
                    metric_time= dateutil.parser.parse(timestamp)

                    # Update the count.
                    metric_index = metrics[hostname][data_source]["count"]
                    metrics[hostname][data_source]["count"] += 1
                    metrics["global"][data_source]["count"] += 1

                    # Cache the timestamp.
                    raw_index =  metadata_objects[data_source].get_raw_index("timestamp") 
                    metrics[hostname][data_source]["raw"][raw_index].insert(metric_index, metric_time)

                except Exception as e:
                    logger.warning( "Error detected when caching the timestamp for this entry: %s",e)
                    continue
                
                for attribute in attributes:
                    if not metadata_objects[data_source].field_exists(attribute):
                        continue

                    raw_index = metadata_objects[data_source].get_raw_index(attribute)
                    if raw_index == 0:
                        continue

                    try: 
                        value = float( attributes[attribute] )
                    except ValueError:
                        value = 0.0
                    
                    metrics[hostname][data_source]["raw"][raw_index].insert( metric_index, value )
                    
                    metrics[hostname][data_source]["sum"][raw_index] += value
                    metrics["global"][data_source]["sum"][raw_index] += value
                    
                    metrics[hostname][data_source]["max"][raw_index] = \
                        max( value, metrics[hostname][data_source]["max"][raw_index] )

                    metrics[hostname][data_source]["min"][raw_index] = \
                        min( value, metrics[hostname][data_source]["min"][raw_index] )
        #==================================================================================

        # Compute total metrics
        for data_source in metrics["global"]:
            global_count = metrics["global"][data_source]["count"]

            for raw_index in range(1, metadata_objects[data_source].num_metrics):

                metrics["global"][data_source]["avg"][raw_index] = \
                    metrics["global"][data_source]["sum"][raw_index] / max(global_count, 1)
                
                variance_sum = 0
                #num_records  = 0 
                for hostname in metrics:
                    if hostname == "global":
                        # TODO Is global std useful?
                        metrics[hostname][data_source]["std"][raw_index] = -1.0
                        continue

                    # If the datasource didn't aggregate any data for that metric, continue.
                    if data_source not in metrics[hostname]:
                        continue

                    metrics["global"][data_source]["max"][raw_index] = max( 
                        metrics["global"][data_source]["max"][raw_index], 
                        metrics[hostname][data_source]["max"][raw_index])

                    metrics["global"][data_source]["min"][raw_index] = min( 
                        metrics["global"][data_source]["min"][raw_index], 
                        metrics[hostname][data_source]["min"][raw_index])


                    local_avg = metrics[hostname][data_source]["sum"][raw_index]\
                        / max( metrics[hostname][data_source]["count"], 1)

                    metrics[hostname][data_source]["avg"][raw_index] = local_avg
                    metrics[hostname][data_source]["std"][raw_index] = \
                        ToolJobMetrics.std(
                            metrics[hostname][data_source]["raw"][raw_index], 
                            local_avg )
                    
                    # For computing the average standard deviation of a field.
                    #num_records  += len(metrics[hostname][data_source]["raw"][raw_index])
                    #variance_sum += pow( metrics[hostname][data_source]["std"][raw_index], 2)

                # XXX I'm not sure if this is "correct" math - John Dunham (jdunham@us.ibm.com(
                #metrics["global"][data_source]["std"][raw_index] =\
                #   numpy.sqrt( variance_sum / variance_sum )

        return metrics, metadata_objects

    @staticmethod
    def print_metrics(
        metrics, 
        metadata, 
        start_time   = None, 
        end_time     = None,
        job_id       = None, 
        stat_options = ["avg", "min", "max", "std"] ):
        ''' Prints out the aggrgated metrics.
                
            :param dict metrics: The aggregated metrics.
            :param MetricsMetadata metadata: The metadata object for the metrics map.
            :param datetime start_time: The start time of the supplied job.
            :param datetime end_time: The end time of the supplied job.
            :param int job_id: The job id.
            :param list stat_options: Collection of metrics to diplay in the output.
        '''
        logger.debug( "Enter ToolJobMetrics.print_metrics")
        # Output formatters
        DIV_HEADER = "=" * 5 + " {0} " + "=" * 5
        DIV_0      = "-" * 50
        DIV_1      = "=" * 50
        DIV_2      = "@" * 75

        # Header
        # @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        print ( "\n" + DIV_2 )
        print ( "\n{0}\nDisplaying the following metrics:\n\t    {1}".format(
            DIV_1, ", ".join(stat_options) ) )
        
        if job_id is not None:
            print ( "Job ID:     {0}".format( job_id ) )

        if start_time is not None:
            print ( "Start Time: {0}".format( output_helpers.format_timestamp( start_time ) ) )

        if end_time is not None:
            print ( "End Time:   {0}".format( output_helpers.format_timestamp( end_time ) ) )

        print ( DIV_1 )
        # @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

        # Generator expression
        headers = ",".join('{0: >12}'.format( stat ) for stat in stat_options ) + ",   key"

        # Move the global metric to the end of the list.
        hostnames = [hostname for hostname in metrics.keys() if hostname != "global"]
        hostnames.append("global")

        # Print out the metrics
        for hostname in hostnames:
            print ("\n{0}\n{1}:\n".format( DIV_0, hostname ) )
            
            for data_source in metrics[hostname]:
                
                print ( DIV_HEADER.format( data_source ) )
                
                body = [""] * len(metadata[data_source].headers)

                # Build the body
                for header_index, metric in enumerate(stat_options):
                    for body_index, field in enumerate(metadata[data_source].headers):
                        stat_index = metadata[data_source].metrics.get(field)
                        try:
                            value = round( metrics[hostname][data_source][metric][stat_index], 4 )
                            if abs(value) == sys.float_info.max:
		                        value = "NaN"	
                        except:
                            value = "NaN"
                        
                        
                        body[body_index] += "{0: >12},".format(value)

                # Append the field name to the row.
                for body_index, field_name in enumerate( metadata[data_source].headers ):
                    body[body_index] += "   " + field_name

                print ( headers )
                for stat in body :
                    print ( stat )
                print ("")

    logger.debug( "Exit ToolJobMetrics.print_metrics")

    @staticmethod
    def std(values, average=None):
        ''' A passthrough to numpy.std.  Performs the standard deviation on the supplied values.

            :param list values: A list of float values to perform the standard deviation on.
            :param float average: The average for the standard deviation, currently unused.
            :returns: The Standard Deviation for the supplied values.
            :rtype: float'''
        logger.debug("Enter unity_metrics.std")
        count = len(values)

        if count < 1 or not (isinstance(values[0], float) or isinstance(values[0], int)):
             logger.debug("Exit unity_metrics.std, standard deviation not computed")
             return
        
        logger.debug("Exit unity_metrics.std")
        return numpy.std(values)

    @staticmethod
    def map_ip(hostnames):
        ''' Translate a list of hostnames to a maping of ip addresses to hostnames.

            :param list hostnames: A list of hostnames to map to ip address.
            :returns: A mapping of ip addresses to hostnames.
            :rtype: dict'''
        logger.debug("Entering unity_metrics.map_ip")

        ip_addrs = dict()
        for hostname in hostnames:
            # If the hostname is not in the /etc/hosts, don't crash the execution,
            # just don't add it to the list.
            try:
                ip = socket.gethostbyname(hostname)
                ip_addrs[ip] = hostname
            except socket.gaierror as e:
                logger.warning("Hostname \'%s\' was not found in /etc/hosts: %s",hostname,e)
                pass

            logger.debug("Exiting unity_metrics.map_ip")
            return ip_addrs

def main(args):
    # Start out using the default log configuration from the csm_bds_context.
    csm_bds_context.load_default_log_conf()

    print("Use Case #4: Job Metrics")
    tool = ToolJobMetrics(args)

    rc =  tool.get_job_metrics()
    if rc != 0:
        logger.error("Error Code {0} \nSettings Used:\n{1}".format(rc, tool.settings.get_values_string()))

    sys.exit(rc)
    
if __name__ == "__main__":
    sys.exit(main(sys.argv))
