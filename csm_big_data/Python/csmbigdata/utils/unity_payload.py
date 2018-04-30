# encoding: utf-8
#================================================================================
#   
#   unity_payload.py 
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
.. module:: csmbigdata.utils.unity_payload
    :platform: Linux
    :synopsis: A module for defining Log Analysis json payloads.
.. moduleauthor:: John Dunham (jdunham@us.ibm.com)
'''
import os
import sys
import getopt
import json
import configparser
from datetime import * 
from dateutil import tz
# Include the connection module, written by the scala team.
from csmbigdata.utils.unity import UnityConnection, get_response_content,UnityException

import logging.config
logger=logging.getLogger(__name__)

#: Maps the daylight savings time timezone names to the tz database timezone name.
DST_MAP = \
{
    "EDT" : "EST5EDT",
    "CST" : "CST6CDT",
    "MDT" : "MST7MDT",
    "PDT" : "PST8PDT"
}


class UnityResponse( object ) :
    '''A collection of constant strings used to access the results from the unity queries. Constants are used to prevent changes in the SCALA API from scuttling code accessing them.

    SEARCH_RESULTS
        ATTRIBUTES
            DATA_SOURCE'''
    
    #:The key that represents the actual results of a search; represented by a list of attributes.
    SEARCH_RESULTS = "searchResults"

    #: The object containing the information for an entry in the searchResults array.
    ATTRIBUTES = "attributes"

    #: The data source stored in the attributes of a search result.
    DATA_SOURCE = "_datasource"


class UnityPayload( object ):
    ''' A class for storing the various data used in POSTs to the Log Analysis server and managing the connection to the Log Analysis server.
    '''
    
    #: The Default starting id for the LogAnalysis query. 
    DEFAULT_START = 0

    #: Maximum number of results supported by a LogAnalysis query. 
    MAX_RESULTS = 1000
    
    #: The default LogAnalysis query. 
    DEFAULT_QUERY = '*'

    def __init__(self):
        '''The timezone, used in offsetting objects with no time zone. '''
        self.timezone = tz.tzlocal()
        self.__default_timezone = self.timezone.tzname(datetime.now(self.timezone))

        if self.__default_timezone in DST_MAP:
            self.__default_timezone = DST_MAP[self.__default_timezone]

        self.__connection       = None

        self.initialize()

    def initialize(self):
        ''' Initializie the payload to its default values. '''

        self.headers       = {
            'Accept'       : 'application/json',
            'Content-Type' : 'application/json; charset=utf-8'
        }
        self.resource      = "Search"

        self.timezone      = tz.gettz(self.__default_timezone)

        self.__query_payload = {}
        self.__query_payload["start"]          = UnityPayload.DEFAULT_START
        self.__query_payload["results"]        = UnityPayload.MAX_RESULTS
        self.__query_payload["outputTimeZone"] = self.__default_timezone
        self.__query_payload["sortKey"]        = ["-timestamp"]
        self.__query_payload["logsources"]     = []
        self.__query_payload["getAttributes"]  = []
        self.__query_payload["query"]          = UnityPayload.DEFAULT_QUERY
        self.__query_payload["filter"]         = {}
    
    # FIXME Fix the password hiding!!!!!!!
    def create_connection(self, base_uri, userid, userpass):
        ''' Creates a UnityConnection object for use by the payload.


        :param string base_uri: The base uri of the Log Analysis service.
        :param string userid: The user name.
        :param string userpass: The user password. 
        :raises ValueError: If the Unity connection is unable to log in to the server, raise a value :
        '''
        self.__connection = UnityConnection(
            base_uri,
            userid,
            userpass )

        try:
            self.__connection.login()
        except UnityException as e:
            logger.error(e)
            raise ValueError("Unable to login to %s, please verify the url and user password!"% base_uri)

    def set_default_timezone(self, timezone):
        ''' Sets the timezone string to be used in initialize calls. Sets the timezone as a side effect.

            :param string timezone: A time zone string (etc. "America/New_York")
        '''
        self.__default_timezone = timezone

        self.set_timezone(self.__default_timezone)
    
    def set_connection(self, unity_connection):
        ''' Sets the __connection attribute of the object.
        
        :param UnityConnection unity_connection: The connection object, assumed to be set up.
        '''
        self.__connection = unity_connection

    def post(self):
        ''' Performs and HTTPS POST of the payload to the supplied UnityConnection.
        
        :returns: An object that may be converted to an object type as specified in the Content-Type in the POST header. Defaults to json. If the post fails, the object is {}.
        :rtype: obj
        '''

        try:
            response = self.__connection.post(self.resource, \
                self.get_json_payload(), headers=self.headers)
            
            response = get_response_content( response )
        except:
            response = {}

        return response

    def get_json_payload(self):
        ''' Convert the payload to a json string.'''
        return json.dumps(self.__query_payload)
     
    def get_start(self):
        ''' Get the integer start value from the payload.'''
        return self.__query_payload["start"]

    def get_results(self):
        ''' Get the results integer value from the payload.'''
        return self.__query_payload["results"]

    def get_last_result(self):
        ''' Get the last result for grabbing additional data for requests.'''
        return self.__query_payload["start"] + self.__query_payload["results"]

    def scala_host_query ( self, field_name, hostnames ):
        ''' Builds a Log Analysis query for restricting another to hostnames.

        :param string field_name:  The field name in the log for the hostnames.
        :param  list hostnames: A list of hostnames for the query, this will be or delimited.
        :returns: A properly formated query string for the hostnames.
        :rtype: string
        '''
        if hostnames is None:
            return 

        query_string = " ("

        hosts_remaining = len( hostnames )
        for hostname in hostnames:
            if field_name is not None :
                query_string += field_name + ':"' + hostname + '"'
            else :
                query_string += hostname

            hosts_remaining -= 1
            if hosts_remaining > 0:
                query_string += ' OR '
        
        query_string += ')'

        return query_string
    
        
    def set_start(self, start_index):
        '''Set the start index for the query. Supports -1 as an exit condition.
        
        :param int start_index: The starting index, defaults to 0, -1 indicates an exit. [0..)
        '''
        if start_index > 0:
            self.__query_payload["start"] = start_index
        elif start_index == -1:
            self.__query_payload["start"] = -1
        else:
            self.__query_payload["start"] = 0
            

    def increment_start(self, start_increase = 1):
        ''' Increments the starting index by a specified amount.

        :param int start_increase: The amount to increase the start index. (0..)
        '''
        if start_index > 0:
            self.__query_payload["start"] += start_increase

    def determine_start(self, json_response, start=None, results=None):
        ''' Determine where the next start point for the query should be.

        :param json json_response: The response object to find the start point in.
        :param int start: The old start point, if not set uses payload start.
        :param int results: The old upper bound relative to the old start, if not set uses payload results.
        :returns: New start point, returns -1 if the total number of results is less than the new start point.
        :rtype: int
        '''
        new_start = -1
        
        if start is None:
            start = self.get_start()
        if results is None:
            results = self.get_results()

        if "totalResults" in json_response and \
            json_response["totalResults"] > start + results:
            new_start = start + results

        return new_start
        
    def set_results(self, num_results):
        ''' Set the number of results to get from the query.

        :param int num_results: The number of results to retrieve, maximum of 1000. [0..1000]
        '''
        if num_results > UnityPayload.DEFAULT_START :
            if num_results < UnityPayload.MAX_RESULTS : 
                self.__query_payload["results"] = num_results
            else:
                self.__query_payload["results"] = UnityPayload.MAX_RESULTS
        else:
            self.__query_payload["results"] = UnityPayload.DEFAULT_START
    
    def set_timezone(self, time_zone):
        '''Set the time zone string and the timezone object for the payload.. 

        :param string time_zone: The time zone string.
        '''
        self.__query_payload["outputTimeZone"] = time_zone
        self.timezone                          = tz.gettz(time_zone)
    
    def set_sort_key(self, sort_key, append_key = False):
        ''' Sets the keys to sort on for the returned data. Default sort key is '-timestamp

        :param (string|list) sort_key: The key(s) sorted on.
        :param Boolean append_key: If set True the sort_key will be appended to the list of keys.
        '''
        if not isinstance( sort_key, list ):
            if not append_key:
                self.__query_payload["sortKey"] = [ sort_key ]
            else:
                self.__query_payload["sortKey"].extend(sort_key)
        else:
            if not append_key:
                self.__query_payload["sortKey"] = sort_key
            else:
                self.__query_payload["sortKey"].extend(sort_key)
    
    # FIXME review this documentation?
    def set_logsources(self, source_type, source_names, append = False):
        '''Sets the log sources accessed. Matches Log Analysis source documentation.

        :param stringsource_type: The type of source the source names are associated with.
        :param list source_names: The source(s) to search on, associated with the type.
        :param Boolean append: Append the log sources to the existing ones.
        '''
        if not append:
            self.__query_payload["logsources"] = []
            
        if isinstance( source_names, list ):
            for name in source_names:
            
                self.__query_payload["logsources"].append({
                    "type" : source_type,
                    "name" : name
                })
        else:
            self.__query_payload["logsources"].append({
                "type" : source_type,
                "name" : source_names    
            })

    def set_getAttributes(self, attributes, append = False):      
        ''' Sets the attributes to retrieve from the query.

        :param (string|list) attributes: The attribute(s) to retrieve from the query.
        :param Boolean append: Append the new attributes to the existing ones.
        '''
        if attributes is None:
            del self.__query_payload["getAttributes"] 
            return
        elif "getAttributes"  not in self.__query_payload:
            self.__query_payload["getAttributes"] = []

        if not isinstance( attributes, list ):
            if not append:
                self.__query_payload["getAttributes"] = [ attributes ]
            else:
                self.__query_payload["getAttributes"].extend(attributes)
        else:
            if not append:
                self.__query_payload["getAttributes"] = attributes
            else:
                self.__query_payload["getAttributes"].extend(attributes)
       

    def set_query(self, query, bool_operator = None):
        ''' Sets the query for the payload, defaults to '\*'.

        :param string query: The query to use in the search operation.
        :param string bool_operator: The boolean operation (AND, OR, ...) to 
            combine with the existing query. If set to None, just overwrites previous query.
        '''
        if bool_operator is None:
            self.__query_payload["query"] = "(" + query + ")"
        else:
            self.__query_payload["query"] = self.__query_payload["query"] +\
                bool_operator + "(" + query +")"
    
    def set_range_timestamp_filter(self, from_date, to_date, clear_filters = False):
        ''' Setup the range filter for timestamps.

        :param datetime from_date: Starting inclusive date time for filter. Appends time zone info if not set.
        :param datetime to_date: Ending inclusive date time for filter. Appends time zone info if not set.
        :param Boolean clear_filters: Clear out existing filters if set to true.
        :raises ValueError: If the from_date and/or to_date is None.
        '''
        if clear_filters : 
            self.__query_payload["filter"] = { "range": { } }

        # Raise a meaningful error if either date is wrong, this takes no sides.
        if from_date is None or to_date is None:
            raise ValueError("A date specified was None, please resolve! from_date:%s to_date:%s"\
                % (from_date,to_date))

        # Cache for readablity.
        tz_str = self.__query_payload["outputTimeZone"]
        if from_date.tzinfo is None:
            from_date = from_date.replace(tzinfo=self.timezone)
            from_date = from_date.astimezone(tz=tz.gettz(tz_str))

        if to_date.tzinfo is None:
            to_date = to_date.replace(tzinfo=self.timezone)
            to_date = to_date.astimezone(tz=tz.gettz(tz_str))

        if from_date.tzinfo is not None:

            self.__query_payload["filter"]["range"] = { "timestamp": { 
                "dateFormat" : "yyyy-MM-dd HH:mm:ss Z SSS",
                "from"       : from_date.strftime("%Y-%m-%d %H:%M:%S %z %f")[:-3],
                "to"         : to_date.strftime("%Y-%m-%d %H:%M:%S %z %f")[:-3]
            } }
        else:
            self.__query_payload["filter"]["range"] = { "timestamp": { 
                "dateFormat" : "yyyy-MM-dd HH:mm:ss SSS",
                "from"       : from_date.strftime("%Y-%m-%d %H:%M:%S %f")[:-3],
                "to"         : to_date.strftime("%Y-%m-%d %H:%M:%S %f")[:-3]
            } }

    # TODO Add more filters.
    
    def clear_facets(self):
        ''' Clear out the existing facets. '''
        if "facets" in self.__query_payload:
            del self.__query_payload["facets"] 

    def set_term_facet(self, field, size, facet_name="term_facet"):
        ''' Add the term facet query to the payload. Gets the top N terms for the 
            supplied field, where N is size.

        :param string field: The field to execute the facet query on.
        :param (string|int) size: The number of terms to display.
        :param string facet_name: The name of the facet in the payload, defaults to "term_facet"
        '''
        if "facets" not in self.__query_payload:
            self.__query_payload["facets"] = {}

        self.__query_payload["facets"][facet_name] = {
            "terms" : {
                "field" : field,
                "size"  : size
            }
        }
    
    def set_histogram_facet(self, field, interval, facet_name="histogram_facet"):
        ''' Generates a histogram for the specified field at the specified interval.
        
        :param (string) field: Field to create the histogram.
        :param (string|int) interval: "Bucket Size" of the counts.
        :param (string) facet_name: The name of the facet in the payload, defaults to "histogram_facet"
        '''
        if "facets" not in self.__query_payload:
            self.__query_payload["facets"] = {}

        self.__query_payload["facets"][facet_name] = {
            "histogram" : {
                "field"    : field,
                "interval" : interval
            }
        }

    def set_stat_facet(self, field, stats, facet_name="stat_facet"):
        ''' Performs statistical analysis on a field. Works best on numeric fields.

        :param string field: Field to perform the statistical analysis on. 
        :param list stats: List of statistics to perform. May consist of any of the following: (max, min, sum, avg, count, missing, sumOfSquares, stddev)
        :param string facet_name: The name of the facet in the payload, defaults to "stat_facet"
        '''
        if "facets" not in self.__query_payload:
            self.__query_payload["facets"] = {}

        self.__query_payload["facets"][facet_name] = {
            "statistical" : {
                "field" : field,
                "stats" : stats
            }
        }
