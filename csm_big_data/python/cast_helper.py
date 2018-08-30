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
.. module:: cast.util
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
    "data.user_name", "data.begin_time", "data.history.end_time", "data.state"]

def get_env():
    '''Gets useful evironment variables.''' 
    return (os.environ[TARGET_ENV])

def deep_get( obj, *keys):
    return reduce(lambda o, key: o.get(key, None) if o else None, keys, obj)

def convert_timestamp( timestamp ):
    '''  
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

def build_time_range( start_time, end_time, 
    start_field="@timestamp", end_field="@timestamp", 
    end_optional=False, bounding_test=False):
    
    if start_time is None:
        return (None, None)

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


