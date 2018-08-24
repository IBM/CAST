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
    start_field="@timestamp", end_field="@timestamp", end_optional=False):
    # Build the time range
    start_time = convert_timestamp(start_time)
    end_time = convert_timestamp(end_time)

    # Build the time range.
    timestamp={ "format" : TIME_SEARCH_FORMAT }
    if start_time:
        timestamp["gte"]=start_time
    if end_time:
        timestamp["lte"]=end_time

    # If the time range has real data process, otherwise return the empty set.
    if start_time or end_time:
        target=[]
        match_min=1

        # The start field is always considered the "Baseline" 
        target.append( { "range" : { start_field : timestamp } } )

        # If the end_field differs, increment the min match counter and add a missing catch for fields that are optional.
        if end_field != start_field:
            match_min += 1
            target.append( { "range" : { end_field : timestamp } } )

            if end_optional:
                target.append({"bool" : { "must_not" : { "exists" : { "field" : end_field } } } })
        
    else:
        target=None
        match_min=0
    
    return(target, match_min)


def search_user_jobs( es, user_name=None, user_id=None,  job_state=None, start_time=None, end_time=None, size=1000,
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



