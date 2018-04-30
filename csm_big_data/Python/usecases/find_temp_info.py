#!/bin/python
# encoding: utf-8
# ================================================================================
#   
# find_job_keys_hadoop.py
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
.. module:: usecases.find_temp_info
    :platform: Linux
    :synopsis: Queries Hive for temperature data above a specified value (in celsius) over a time range.
.. moduleauthor:: John Dunham (jdunham@us.ibm.com)
'''

# Import the os, system, regex and json modules.
import os
import sys
import re
import json

# Set the csm_bds_context for development work.
import csm_bds_context

from csmbigdata.utils.hive_payload import HiveConnection

# Include the configuration modules:
from csmbigdata.config.settings      import Settings
from csmbigdata.config.time_settings   import TimeSettings
from csmbigdata.config.settings_option import SettingsOption

# Includes some helper functions to ensure that output is consistent across tools.
import csmbigdata.utils.output_helpers as output_helpers


# Include the various date modules.
import dateutil.parser
from datetime import *
from dateutil.relativedelta import *

# Predominantly used for debug loging.
import logging.config
logger=logging.getLogger(__name__)

def get_sample_time( sample_string ):
    ''' Converts the supplied sample_string to an integer sample time in milliseconds.

    :param string sample_string: A string that specifies the length of time between 
        samples in minutes.

    :returns int: An integer representing the sample_string in milliseconds.        
    '''
    return int( float(sample_string) * 60000)

class ToolTempInfo (object):
    ''' Facilitates keyword searching against a Log Analysis Server.'''
    
    # TODO use output helper
    #: The date format for printing out dates.
    DATE_FORMAT =  '''%Y-%m-%d %H:%M:%S'''
    

    def __init__(self, args):
        ''' Initializes the tool. 

        :param args: The argument list.
        '''

        logger.debug("Begin ToolTempInfo init")

        #: The user configurable input to the usecase.
        self.settings = Settings( )
        
        self.settings.help_string = '''A Tool for performing Analytics on CPU temperature sensors.
     There are two `use cases` represented in this script:

     1) Find all temperatures over a user specified limit (mintemp) then dispay the results 
         as a frequency histogram:

           Flag: --plot

           Supported Options: --mintemp, --buckets, --days, --targettime

     2) Find any Allocations that were running when a temperature over the limit (mintemp) 
         was detected. Generates a csv file, outputting the first 30 lines to stdout:
        
           Flag: --allocids
           
           Supported Options: --mintemp, --sampletime, --days, targettime, --output (for csv file)

           *NOTE* `--sampletime` defines the number of minutes AFTER the end of an allocation that
                bmc temperature samples are still accepted. This should match the window used 
                by your data aggregator.
'''

        # self.settings.time
        self.settings.append_class( TimeSettings )

        # Example of how to use the settings class without making a new class.
        # self.settings.hive
        self.settings.append_class_anon( "hive", 
            {
                "query_user" : SettingsOption( None, "hiveuser", "user", "hdfs",
                    '''The user to execute Hive queries''', '''string''' ),
                "host_name"  : SettingsOption( None, "hivehost", "host", "localhost",
                    '''The hostname of the Hive Server''', '''string''' ),
                "port"       : SettingsOption( None, "hiveport", "port", "10000",
                    '''The port Hive is available on the Hive Server''', '''string'''
                    , str )
            }, "hive.server" )

        # self.settings.temp
        self.settings.append_class_anon( "temp" ,
            {
                "min_temp" : SettingsOption( None, "mintemp", "min_temperature", 0.0,
                    'The minimum temperature to get info for (Celsius)', 'float', float ),

                "plot" : SettingsOption( None, "plot", None, False,
                    'Displays a frequency histogram for the temperatures greater than or equal to `mintemp`', 
                    is_flag = True),
                
                "buckets" : SettingsOption( None, "buckets", "buckets", 10,
                    'The number of buckets for Histogram data', 'int', int ),
                
                "alloc_ids" : SettingsOption( None, "allocids", None, False,
                    'Generate a csv file collecting Allocations that were running when the temperature was greater than or equal to `mintemp`', is_flag = True),
                
                "sample_time" : SettingsOption( None, "sampletime", "sample_time", 30,
                    'The window of time in minutes between samples of the temperature (should match the bmc temp sensor aggregator rate)', 'int', 
                    get_sample_time ),
            }, "temp.params" )

        # Parse the arguments and config file.
        self.settings.parse_args(args)

        if self.settings.temp.plot and self.settings.temp.alloc_ids:
            logger.error("--plot and --alloc_ids may not be set together!")
            self.settings.print_help()
            logger.error("Error Code {0} \nSettings Used:\n{1}".format(
                rc, self.settings.get_values_string()))
            sys.exit(1)

        #: The connection to the hive resource.
        self.hive_connection = None

        logger.debug("Exit ToolTempInfo init")
        
    def compute_time_range(self): 
        ''' Computes the time range. If no time was supplied the end time is set as now,
        otherwise the supplied time is set as the endtime.

        :returns (Datetime, Datetime): 2-tuple containing the start and end date information. 
            ( start_time, end_time )
        '''
        
        # If the target time was not specified, set it to now. 
        end_time = self.settings.time.target_datetime

        if end_time is None:
            end_time = datetime.now()

        # Compute the start time.O
        start_time = end_time - relativedelta( days=self.settings.time.days )
    
        return ( start_time, end_time )

    def compute_time_range_timestamps(self):
        ''' Computes the time range and then converts it to unix timestamps.

        :returns (int, int): 2-tuple containing the start and end date information as a unix 
            timestamp ( start_time, end_time )
        '''
        time_range = self.compute_time_range()
        
        return ( int( time_range[0].strftime( "%s" ) ) * 1000,
            int( [1].strftime( "%s" ) )*1000 )

    def temp_analytics(self):
        ''' Performs temperature analytics on the Hive Server. Currently only 
        perfoms analytics on bmc_temp_cpu_core_max.
        '''
        # Print the use case identifier for this execution.
        if self.settings.temp.alloc_ids:
            output_helpers.eprint("Use Case #7; Find any Allocations that were running when a temperature over the limit was detected.")

        elif self.settings.temp.plot:
            output_helpers.eprint("Use Case #6; Finds all temperatures over the user limit, then displays the results as a frequency histogram.")
        else:
            logger.error("No valid execution flag set, --allocids or --plot must be set")
            return 1

        # Build parameters.
        time_range_tup = self.compute_time_range()
        params = {
            "min_temp"    : self.settings.temp.min_temp,
            "min_time"    : int(time_range_tup[0].strftime("%s"))*1000, 
            "max_time"    : int(time_range_tup[1].strftime("%s"))*1000,
            "sample_time" : self.settings.temp.sample_time,
            "buckets"     : self.settings.temp.buckets
        }

        # Initialize a hive connection.
        try:
            self.hive_connection = HiveConnection()
            self.hive_connection.connect( 
                self.settings.hive.host_name, 
                self.settings.hive.port, 
                self.settings.hive.query_user )
        except Exception, e:
            logger.error(e)
            logger.error("Unable to open a connection to the Hive Server")
            logger.error("Verify that the hive section of the configuration points to a valid Hive Server")
            return 1

        # Return Code.
        rc=0

        # Perform the business logic.
        if self.settings.temp.alloc_ids:
            rc=self.retrieve_allocation_ids(time_range_tup, params)

        elif self.settings.temp.plot: 
            rc=self.retrieve_temp_histogram(time_range_tup, params)

        # Clean up.
        self.hive_connection.close()

        # TODO
        return rc

    def retrieve_temp_histogram( self, time_range_tup, params ):
        ''' Uses Hive to generate a histogram of maximum cpu temperatures in
        the specified time range. This histogram output can be bounded
        by a minimum temperature.

        :param tuple time_range_tup: A 2-tuple holding a start and end datetime objects.
        :param dict params: 
            :min_temp: The minimum temperature inclusive temperature for searching for allocations.
            :min_time: The start time to search for allocations.
            :max_time: The end time to search for allocations.
            :buckets: The number of buckets to place the results of the histogram into.

        '''
        query = '''SELECT histogram_numeric( bmc_temp_cpu_max, %(buckets)d) 
            FROM unity.bmc_temp_sensor
            WHERE bmc_temp_cpu_max >= %(min_temp)d 
            AND `timestamp` BETWEEN %(min_time)d AND %(max_time)d'''

        data= self.hive_connection.query(query, params, self.settings.default.verbose)

        if data is None:
            return 1     

        self.graph_histogram(
            data[0], 
            "Max CPU Temperatures Detected \n\tfrom {} to {}".format(
                time_range_tup[0].strftime(ToolTempInfo.DATE_FORMAT),
                time_range_tup[1].strftime(ToolTempInfo.DATE_FORMAT)),
            "Max CPU Temperature", 
            "Frequency of Max CPU Temperature")
        
        return 0

    def retrieve_allocation_ids(self, time_range_tup, params ):
        ''' Retrieves the allocations that were running when a node was
        at a temperature greater than or equal to the temperature supplied in params.
        Outputs the results to the csv file specified in `self.settings.default.output`.

        :param tuple time_range_tup: A 2-tuple holding a start and end datetime objects.
        :param dict params: 
            :min_temp: The minimum temperature inclusive temperature for searching for allocations.
            :min_time: The start time to search for allocations.
            :max_time: The end time to search for allocations.
            :sample_time: The sample time for the bmc temperature sensor.
        '''
        
        # The query to execute against the Hive server.
        query = '''
        SELECT 
            allocation_id, 
            max_temp, 
            temperature_host AS hostnames,
            ts_collect AS ts_collect
        FROM
        ( 
            SELECT 
                bmc_hostname as temperature_host,
                bmc_temp_cpu_max as max_temp,
                `timestamp` as ts_collect
            FROM 
                unity.bmc_temp_sensor 
            WHERE bmc_temp_cpu_max >= %(min_temp)d 
            AND `timestamp` BETWEEN %(min_time)d AND %(max_time)d 
        ) bmc_query
        JOIN 
        (
            SELECT 
                sysloghostname,
                regexp_extract(message[0],"Allocation ([0-9]*) ",1) allocation_id,
                COLLECT_LIST(message[0] RLIKE  "Allocation.*create:") types,
                COLLECT_LIST(`timestamp`) timestamps
            FROM unity.syslog
            WHERE 
                `timestamp` BETWEEN %(min_time)d AND %(max_time)d
                AND message[0] RLIKE  '.*Allocation.*[cd][re][el][ae]te:.*'
            GROUP BY 
                sysloghostname, 
                regexp_extract(message[0],"Allocation ([0-9]*) ",1)  
        ) syslog_query
        WHERE 
            syslog_query.sysloghostname = bmc_query.temperature_host
            AND 
            ( 
                syslog_query.types[0] AND bmc_query.ts_collect >= syslog_query.timestamps[0]
                OR NOT syslog_query.types[0] AND bmc_query.ts_collect <= syslog_query.timestamps[0]
            )
            AND
            (
                size(syslog_query.timestamps) = 1
                OR syslog_query.timestamps[1] >= bmc_query.ts_collect -  %(sample_time)d
            )
        '''

        # Query hive.
        data = self.hive_connection.query( query, params, self.settings.default.verbose )
        if data is None:
            return 1     

        headers = self.hive_connection.headers()
        
        # Notify the user about the number of allocations found in the range, and write the 
        # results to the console and output file.
        print("\nAllocations found with temperatures greater than {}C \n\tfrom {} to {}".format(
            params["min_temp"],
            time_range_tup[0].strftime(ToolTempInfo.DATE_FORMAT),
            time_range_tup[1].strftime(ToolTempInfo.DATE_FORMAT)))
        output_helpers.make_csv_file(headers, data, self.settings.default.output, True)

        return 0
        

    def graph_histogram(self, data, graph_name, data_axis, freq_axis, y_freq=True):
        ''' Prints an ASCII histogram to the console using the histogram data in the data tuple.

        :param tuple data: A tuple containing a single list containing histogram data points.
            These data point objects are in the following format: {'x':0,'y':1}
        :param string graph_name: The name of the graph to produce. 
        :param string data_axis: The type of the data presented. (y label in graph)
        :param string freq_axis: The type of frequency presented in the data. (x label in graph)
        :param bool y_freq: 
            :True: The y field in the histogram is the frequency value.
            :False: The x field in the histogram is the frequency value.
        '''
        # EARLY RETURN if histogram is None.
        if data[0] is None:
            print("\nHistogram was empty, graph could not be made for: ")
            print(graph_name + "\n")
            return 

        # Set the sorting to use the x field if x_sorted is set.
        data_key     = 'x' if y_freq else 'y'
        frquency_key = 'y' if y_freq else 'x' 

        # Load and sort the histogram on the data axis.
        histogram = json.loads(data[0]) 
        histogram.sort(key=lambda dv: dv[data_key])
    
        # Cache the values for normalization.
        min_value = min(histogram, key=lambda fv: fv[frquency_key])[frquency_key]
        max_value = max(histogram, key=lambda fv: fv[frquency_key])[frquency_key]
        n_value   = int(sum(value[frquency_key] for value in histogram))
        scale     = max(max_value - min_value, 1) # Make sure that it's at least 1

        # Setup the strings for output.
        max_width  = 60
        fixed_width= 14
        fixed_str  = "{:3.1f} |{} {:<5d}"
        bar_char   = "="
        
        # Print it out.
        print("\n\n")
        print("-"*(max_width+fixed_width))

        print ( graph_name + " (n={:d})".format(n_value)) 
        print ('\ty : ' + data_axis)
        print ('\tx : ' + freq_axis)
        print ("-"*(max_width+fixed_width))

        for value in histogram:
            freq = int(round(((value[frquency_key] - min_value)/scale) * max_width))
            print(fixed_str.format(
                float(value[data_key]),
                bar_char*freq + bar_char, 
                int(value[frquency_key])))

        print ("-"*(max_width+fixed_width))
        

def main(args):
    # Start out using the default log configuration from the csm_bds_context.
    csm_bds_context.load_default_log_conf()

    info_tool = ToolTempInfo(args)

    rc = info_tool.temp_analytics()

    if rc != 0:
        logger.error("Error Code {0} \nSettings Used:\n{1}".format(
            rc, info_tool.settings.get_values_string()))

    sys.exit(rc)

if __name__ == "__main__":
    sys.exit(main(sys.argv))
