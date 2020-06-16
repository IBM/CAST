#!/bin/sh
# encoding: utf-8
#================================================================================
#
#    cast_helper.py
#
#    © Copyright IBM Corporation 2015-2020. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================

# The beginning of this script is both valid shell and valid python,
# such that the script starts with the shell and is reexecuted with
# the right python.
#
# The intent is to run as python3 on RHEL8 and python2 on RHEL7
#
'''which' python3 > /dev/null 2>&1 && exec python3 "$0" "$@" || exec python2 "$0" "$@"
'''

'''
.. module:: cast_helper
    :platform: Linux
    :synopsis: A collection of frequently used queries and patterns for Elasticsearch accesses.
.. moduleauthor:: John Dunham (jdunham@us.ibm.com)
'''


import os
import re

from elasticsearch import Elasticsearch
from elasticsearch.serializer import JSONSerializer
from elasticsearch import exceptions
from datetime import datetime
from functools import reduce
   
TARGET_ENV='CAST_ELASTIC'

# Date constants

DATE_FORMAT        = '(\d{4})-(\d{1,2})-(\d{1,2})[ \.T]*(\d{0,2}):{0,1}(\d{0,2}):{0,1}(\d{0,2})'
DATE_FORMAT_PRINT  = '%Y-%m-%d %H:%M:%S'
#TIME_SEARCH_FORMAT = 'yyyy-MM-dd HH:mm:ss'
TIME_SEARCH_FORMAT = "epoch_millis"

USER_JOB_FIELDS=["data.primary_job_id","data.secondary_job_id", "data.allocation_id", 
    "data.user_name", "data.begin_time", "data.history.end_time", "data.state"]

SEARCH_JOB_FIELDS=["data.primary_job_id","data.secondary_job_id", "data.allocation_id", 
    "data.user_name", "data.user_id", "data.begin_time", "data.history.end_time", "data.state"]

def print_request_error(exception):
    '''Performs a print of the request error.
        :param elasticsearch.exceptions.RequestError: A request error to print the contents of.
    '''
    error = exception.info.get("error")

    print("A RequestError was recieved:")
    if error is not None:
        print("  type: {0}\n  reason: {1}".format(error.get("type"), error.get("reason")))
        
        root_cause = error.get("root_cause")

        if root_cause is not None:
            i = 0
            for cause in root_cause:
                print("  root_{2}_type: {0}\n  root_{2}_reason: {1}\n\t".format(
                    cause.get("type"), cause.get("reason"), i))
    else:
        print(exception)
    

def get_env():
    '''Gets useful evironment variables.
    
    :return tuple: 
        (TARGET_ENV)
    ''' 
    return (os.environ[TARGET_ENV])

def deep_get( obj, *keys):
    '''
    Performs a deep get operation. This acts as a utility function for deeply nested elasticsearch
    responses.

    :param dict obj: The dictionary object perform a deep get on.
    :param list keys: A list of strings representing keys for deeply nested objects. If any of the
        keys are not present in the nesting the None type will be returned. This may be specified
        as a set of variadic arguments to the function.

    :returns obj: Returns the object found at the level of indirection defined by the list of keys.
        If the key list supplied is not a legal indirection the None type will be returned.
    '''
    return reduce(lambda o, key: o.get(key, None) if o else None, keys, obj)

def convert_timestamp( timestamp ):
    '''  Converts a timestamp string into a datetime formatted string. Primarily used to verify 
    user input.
    
    :param string timestamp: A timestamp in the @ref DATE_FORMAT format (yyyy-MM-dd[ .]HH:mm:ss).
        Everything from precision hour and higher is optional as an input.

    :returns string: A datetime formatted string in epoch milliseconds. The now string
        will be rendered as the output from datetime.now().

    :raises ValueError: If the timestamp is not formatted legally a value error will be raised.
    '''
    new_timestamp=None
    if timestamp:
        time_search=re.search( DATE_FORMAT, timestamp )

        # Split on search success
        if time_search : 
            (year,month,day,hour,minute,second) = time_search.groups()
            date = datetime( year=int(year), month=int(month), day=int(day),
                hour=int(hour if hour else  0),
                minute=int(minute if minute else 0),
                second=int(second if second else 0) )
            new_timestamp=datetime.strftime(date, "%s000")

        elif timestamp is "now":
            new_timestamp=datetime.strftime(datetime.now(), "%s000")

        elif timestamp.isdigit() :
            new_timestamp="{0}000".format(timestamp)
        
        else:
            raise ValueError('"%s" is not a legal timestamp.' % timestamp)
    else:
        new_timestamp=datetime.strftime(datetime.now(), "%s000")

    
    return new_timestamp

def build_time_range(start_time, end_time,  
    start_field="data.begin_time", end_field="data.history.end_time"):
    ''' Builds a set of queries to determine any records for which the record range (defined by
    start_field and end_field) collides with the range defined by start_time and end_time.

    The logic used in these time range queries is : 
        `start_field <= end_time && ( end_field >= start_time || end_field is None)`

    :param string start_time:  The start of the time range to find records in.
    :param string end_time:    The end of the time range to find records in.
    :param string start_field: The start field denoting the start time of a record.
    :param string end_field:   The end field denoting the end time of a record (may be empty in elasticsearch).
    
    :returns (list, int): Returns a list of range queries and the minimum number that must match
        for a record to be considered "in range".
    '''
    # Build the time range
    start_time = convert_timestamp(start_time)
    end_time = convert_timestamp(end_time)

    if start_time and end_time:
        # Build the time range.
        timestamp_start = { "format" : TIME_SEARCH_FORMAT } 
        timestamp_end   = { "format" : TIME_SEARCH_FORMAT } 
    
        timestamp_end["gte"]   = start_time
        timestamp_start["lte"] = end_time
        target=[]
        match_min=2

        # The start field is always considered the "Baseline" 
        target.append( { "range" : { start_field : timestamp_start } } )

        # Build the end range so it can capture running jobs.
        target.append( { "range" : { end_field  : timestamp_end } } )
        target.append( {  "bool" : { "must_not" : { "exists" : { "field" : end_field } } } } )
    else:
        target=None
        match_min=0
    
    return(target, match_min)

def build_target_time_search( target_time, start_field="data.begin_time", 
    end_field="data.history.end_time"):
    ''' Perform a search for all records which have a range containing the supplied target_time.

    Invokes build_time_range to generate a bounding test.

    :param string target_time: The target time to search for collisions with elasticsearch records.
    :param string start_field: The start field denoting the start time of a record.
    :param string end_field:   The end field denoting the end time of a record (may be empty in elasticsearch).
    
    :returns (list, int): Returns a list of range queries and the minimum number that must match
        for a record to be considered "in range".
    '''
    
    return build_time_range(target_time, target_time, start_field, end_field)
    
    

def build_timestamp_range( start_time, end_time, field="@timestamp"):
    ''' Builds a timestamp range filter.

    A query for all records for which the contents of the `field` in the record are between 
    `start_time` and `end_time`
    
    :param string start_time: The start time of the range to find records in.
    :param string end_time: The end time of the range to find records in.
    :param string field: The field to search on.
    
    :returns (list, int): Returns a list of range queries and the minimum number that must match 
        for a record to be considered "in range". This will be (None, 0) if the range can't be built.
    '''

    # Build the time range
    start_time = convert_timestamp(start_time)
    end_time   = convert_timestamp(end_time)

    # Build the time range.
    target=[]
    match_min=0
    timestamp = { "format" : TIME_SEARCH_FORMAT, "gte" : start_time  } 
    
    # Build the start time field.
    if start_time:
        match_min = 1
        timestamp["gte"] = start_time

    if end_time:
        match_min = 1
        timestamp["lte"] = end_time
    
    if  match_min is 1:
        target.append( { "range" : { field : timestamp } } )
    else:
        target=None
    
    return(target, match_min)


def search_user_jobs( es, user_name=None, user_id=None, job_state=None, 
    start_time=None, end_time=None, size=1000,
    fields=USER_JOB_FIELDS, index="cast-allocation" ):
    ''' Performs an elastic search query to retrieve a listing of jobs that a user participated in.

    :param Elasticsearch es: An initialized elasticsearch object, a search operation is performed with
        this object.
    :param string user_name: A user to search elasticsearch for, takes precedence over @ref user_id.
    :param string user_id: A user id to search elasticsearch for, if @ref user_name is specified, ignored.
    :param string job_state: Filters the results to the supplied job states.

    :param string start_time: A starting time to search for running jobs (yyyy-MM-dd[ .]HH:mm:ss).
    :param string end_time: An ending time to search for running jobs (yyyy-MM-dd[ .]HH:mm:ss).
    :param int size: The number of jobs to display in the results.

    :param list fields: A list of fields see in the output of the search.
    :param string index: The index to search for the job, "cast-allocation" is the default index name.

    :returns dict: A dictionary containing the results from the elasticsearch query.
    '''
    
    query={}

    # Build the bool query
    # ==============================================
    # Match the user name.   
    if user_name:
        must_query= [ { "match" : { "data.user_name" : user_name } } ]
    elif user_id:
        must_query= [ { "match" : { "data.user_id" : user_id } } ]
    else:
        return None

    # TODO Verify supplied state? 
    if job_state:
       must_query.append( { "match": { "data.state" : job_state } } )

    query["bool"]={ "must" : must_query}

    (should, min_count) = build_time_range(start_time, end_time, 
        start_field  = "data.begin_time", 
        end_field    = "data.history.end_time")

    if should: 
        query["bool"]["should"]               = should
        query["bool"]["minimum_should_match"] = min_count
    #=======================================

    body={
        "query"   : query,
        "_source" : fields,
        "size"    : size
    }

    return es.search(
        index=index,
        body=body
    )

def search_job( es, allocation_id=0, primary_job_id=0, secondary_job_id=0, size=1000,
    fields=None, index="cast-allocation" ):
    ''' Performs an elastic search query to retrieve any jobs that match the specified job.

    :param Elasticsearch es: An initialized elasticsearch object, a search operation is performed with
        this object.
    :param int allocation_id: The allocation id of the job to be searched for, takes precedence over job id.
    :param int primary_job_id: The primary job id of the job to be searched for, overriden by allocation id.
    :param int secondary_job_id: The secondary id of the job to be searched for, only used if primary is set.
    :param int size: The number of jobs to display in the results.

    :param list fields: A list of fields see in the output of the search.
    :param string index: The index to search for the job, "cast-allocation" is the default index name.

    :returns dict: A dictionary containing the results from the elasticsearch query.
    '''

    query={ "bool" : { "should" : [] } }
    
    if allocation_id > 0 : 
        query={ 
            "bool" : { 
                "should" : [ 
                    { "match" : { "data.allocation_id" : allocation_id } } 
                ] 
            } 
        }
    elif primary_job_id > 0 and secondary_job_id >= 0:
        query={ 
            "bool" : { 
                "should" : [ 
                    { "match" : { "data.primary_job_id"   : primary_job_id } },
                    { "match" : { "data.secondary_job_id" : secondary_job_id } } 
                ],
                "minimum_should_match" : 2
            } 
        }
    else:
        return None

    body={
        "query" : query,
        "size"  : size
    }

    if fields : 
        body["_source"] = fields
    

    return es.search(
        index=index,
        body=body
    )


