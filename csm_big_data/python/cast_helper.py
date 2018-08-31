#!/bin/python
# encoding: utf-8
#================================================================================
#
#    cast_helper.py
#
#    © Copyright IBM Corporation 2015-2018. All Rights Reserved
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
.. module:: cast_helper
    :platform: Linux
    :synopsis: A collection of frequently used queries and patterns for Elasticsearch accesses.
.. moduleauthor:: John Dunham (jdunham@us.ibm.com)
'''


import os
import re

from elasticsearch import Elasticsearch
from elasticsearch.serializer import JSONSerializer
from datetime import datetime
   
TARGET_ENV='CAST_ELASTIC'

# Date constants

DATE_FORMAT        = '(\d{4})-(\d{1,2})-(\d{1,2})[ \.T]*(\d{0,2}):{0,1}(\d{0,2}):{0,1}(\d{0,2})'
DATE_FORMAT_PRINT  = '%Y-%m-%d %H:%M:%S'
TIME_SEARCH_FORMAT = 'yyyy-MM-dd HH:mm:ss'

USER_JOB_FIELDS=["data.primary_job_id","data.secondary_job_id", "data.allocation_id", 
    "data.user_name", "data.begin_time", "data.history.end_time", "data.state"]

SEARCH_JOB_FIELDS=["data.primary_job_id","data.secondary_job_id", "data.allocation_id", 
    "data.user_name", "data.user_id", "data.begin_time", "data.history.end_time", "data.state"]

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

    :returns string: A datetime formatted string using the @ref DATE_FORMAT_PRINT. The now string
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
            new_timestamp=datetime.strftime(date, DATE_FORMAT_PRINT)

        elif timestamp is "now":
            new_timestamp=datetime.strftime(datetime.now(), DATE_FORMAT_PRINT)
        
        else:
            raise ValueError('"%s" is not a legal timestamp.' % timestamp)
    
    return new_timestamp

def build_job_bounding_time_range(start_time, end_time,  
    start_field="data.begin_time", end_field="date.history.end_time"):

    # Build the time range
    start_time = convert_timestamp(start_time)
    end_time = convert_timestamp(end_time)

    # Build the time range.
    timestamp = { "format" : TIME_SEARCH_FORMAT } 
    timestamp = { "format" : TIME_SEARCH_FORMAT } 

    if start_time:
        timestamp["gte"]=start_time

    if end_time:
        timestamp["lte"]=end_time


    if start_time or end_time:
        target=[]
        match_min=1

        # The start field is always considered the "Baseline" 
        target.append( { "range" : { start_field : timestamp } } )

        # If the end_field differs, increment the min match counter and add a missing catch for fields that are optional.
        if end_field != start_field:
            target.append( { "range" : { end_field : timestamp } } )
        
    else:
        target=None
        match_min=0
    
    return(target, match_min)

    

def build_time_range( start_time, end_time, 
    start_field="@timestamp", end_field="@timestamp", 
    end_optional=False, bounding_test=False):
    ''' Builds a timerange for an elasticsearch query.

    WARNING: This is going to be broken down into multiple fuctions, it has too many moving parts 
    to be maintained correctly.

    :param string start_time: The start time for the range being queried. If @ref bounding_test is 
        specified this timestamp will be used as the "midpoint".
    :param string end_time: The end time for the range being queried. 
        
    :param string start_field: The field to be associated with the start time.
    :param string end_field: The field to be associated with the end time.

    :param bool end_optional: If the ending field is optional this must be specified so the query 
        can intelligently respond.
    :param bool bounding_test: Performs a bounding test (WARNING will be moved to a new function)
    
    :returns (list, min_match): A list of time range queries, the minimum number of those queries which must match.
    '''
    # TODO This function probably should be spearated for clarity.
    # TODO this function really needs better documentation/needs a rewrite.

    # Build the time range
    start_time = convert_timestamp(start_time)
    end_time = convert_timestamp(end_time)

    # Build the time range.
    timestamp_start = { "format" : TIME_SEARCH_FORMAT } 
    timestamp_end   = { "format" : TIME_SEARCH_FORMAT } 
    
    # Build the start time field.
    if start_time:
        timestamp_start["gte"]=start_time
        if end_field == start_field and end_time:
            timestamp_start["lte"]=end_time

    # If the end time was set specify an lte option.
    if end_time:
        timestamp_end["lte"]=end_time

    # For single point ranges build a bounding box and swap the start and end. 
    if bounding_test:
        timestamp_end["lte"] = start_time
        temp_time            = timestamp_start
        timestamp_start      = timestamp_end 
        timestamp_end        = temp_time

    # If the time range has real data process, otherwise return the empty set.
    if start_time or end_time:
        target=[]
        match_min=1

        # The start field is always considered the "Baseline" 
        target.append( { "range" : { start_field : timestamp_start } } )

        # If the end_field differs, increment the min match counter and add a missing catch for fields that are optional.
        if end_field != start_field:
            match_min += 1
            target.append( { "range" : { end_field : timestamp_end } } )

            if end_optional:
                target.append({"bool" : { "must_not" : { "exists" : { "field" : end_field } } } })
        
    else:
        target=None
        match_min=0
    
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
        end_field    = "data.history.end_time",
        end_optional = True)

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


