#!/bin/python
# encoding: utf-8
#================================================================================
#
#    findJobKeys.py
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


import argparse
import sys
import os
from datetime import datetime
from dateutil.parser import parse
from elasticsearch import Elasticsearch
from elasticsearch.serializer import JSONSerializer
from elasticsearch import exceptions

import json

import cast_helper as cast

TARGET_ENV='CAST_ELASTIC'

def main(args):

    # Specify the arguments.
    parser = argparse.ArgumentParser(
        description='''A tool for finding keywords in the "message" field during the run time of a job.''')
    
    parser.add_argument( '-a', '--allocationid', metavar='int', dest='allocation_id', default=-1,
        help='The allocation ID of the job.')
    parser.add_argument( '-j', '--jobid', metavar='int', dest='job_id', default=-1,
        help='The job ID of the job.')
    parser.add_argument( '-s', '--jobidsecondary', metavar='int', dest='job_id_secondary', default=0,
        help='The secondary job ID of the job (default : 0).')
    parser.add_argument( '-t', '--target', metavar='hostname:port', dest='target', default=None, 
        help='An Elasticsearch server to be queried. This defaults to the contents of environment variable "CAST_ELASTIC".')
    parser.add_argument( '-k', '--keywords', metavar='key', dest='keywords', nargs='*', default=['.*'],
        help='A list of keywords to search for in the Big Data Store. Case insensitive regular expressions (default : .*). If your keyword is a phrase (e.g. "xid 13") regular expressions are not supported at this time.')
    parser.add_argument( '-v', '--verbose', action='store_true',
        help='Displays any logs that matched the keyword search.' )
    parser.add_argument( '--size', metavar='size', dest='size', default=30,
        help='The number of results to be returned. (default=30)')
    parser.add_argument( '-H', '--hostnames', metavar='host', dest='hosts', nargs='*', default=None,
        help='A list of hostnames to filter the results to (filters on the "hostname" field, job independent).')

    args = parser.parse_args()

    # If the target wasn't specified check the environment for the target value, printing help on failure.
    if args.target is None:
        if TARGET_ENV in os.environ:
            args.target = os.environ[TARGET_ENV]
        else:
            parser.print_help()
            print("Missing target, '%s' was not set." % TARGET_ENV)
            return 2

    
    # Open a connection to the elastic cluster, if this fails is wrong on the server.
    es = Elasticsearch(
        args.target, 
        sniff_on_start=True,
        sniff_on_connection_fail=True,
        sniffer_timeout=60
    )

    # Execute the query on the cast-allocation index.
    try:
        tr_res =  cast.search_job(es, args.allocation_id, args.job_id, args.job_id_secondary)
    except exceptions.RequestError as e:
        cast.print_request_error(e)
        return 4

    total_hits = cast.deep_get(tr_res, "hits","total")

    print("Got {0} Hit(s) for specified job, searching for keywords.".format(total_hits))
    # Finding no matches with valid search criteria is a legit case. 
    # return 0, not 3
    if total_hits == None:
        print("# Sorry. Could not find any matching results.")
        return 0
    if total_hits != 1:
        print("This implementation only supports queries where the hit count is equal to 1.")
        return 3

    # TODO make this code more fault tolerant
    hits = cast.deep_get(tr_res, "hits", "hits")
    tr_data = cast.deep_get(hits[0], "_source", "data")

    # ---------------------------------------------------------------------------------------------
    # TODO Add utility script to do this.
    
    # Build the hostnames string:
    if args.hosts is None: 
        args.hosts = tr_data.get("compute_nodes")

    hostnames = {
        "multi_match" : {
            "query"                 : " ".join(args.hosts),
            "type"                  : "best_fields",
            "fields"                :  [ "hostname", "source"],
            "tie_breaker"           : 0.3,
            "minimum_should_match"  : 1
        }
    }

    # ---------------------------------------------------------------------------------------------
    # TODO Add a utility script to manage this.

    date_format= '%Y-%m-%d %H:%M:%S.%f'
    print_format='%Y-%m-%d %H:%M:%S:%f'
    search_format='epoch_millis'

    # Determine the timerange:
    start_time=datetime.strptime(tr_data.get("begin_time"), date_format)

    timestamp_range={
        "gte"    : start_time.strftime('%s000'), 
        "format" : search_format
    }    

    # If a history is present end_time is end_time, otherwise it's now.
    if "history" in tr_data:
        end_time=datetime.strptime(tr_data.get("history").get("end_time"), date_format)
        timestamp_range["lte"] = end_time.strftime('%s999')
    
    timerange={
        "range" : {
            "@timestamp" : timestamp_range
        }
    }    
    # ---------------------------------------------------------------------------------------------

    # Build the message query.
    keywords={}
    should_keywords=[]
    for key in args.keywords:
        if key.find(" ") == -1:
            should={ 
                "regexp" : { 
                    "message" : key.lower()  
                } 
            }
        else:
            should={ 
                "match_phrase" : { 
                    "message" : key
                } 
            }
            
        should_keywords.append(should)

        keywords[key] = { "filter" : should }

    # ---------------------------------------------------------------------------------------------

    # Submit the query
    body={
        "query" : {
            "bool" : {
                "must": [
                    timerange, 
                    hostnames,
                    { "exists" : { "field" : "message" } }
                ],
                "should" : should_keywords,
                "minimum_should_match": 1
            }
        },
	"sort" : [ "timestamp"],
        "_source" : [ "timestamp", "message", "hostname"],
        "size"    : args.size,
        "aggs"    : keywords 
    }

    try:
        key_res = es.search(
            index="_all",
            body=body
        )
    except exceptions.RequestError as e:
        cast.print_request_error(e)
        return 4

    # Print the count table.
    total=cast.deep_get(key_res,'hits','total')
    print("Got {0} keyword hits:\n".format( total ))
    
    aggregations= key_res.get("aggregations")

    max_width=7
    for key in args.keywords:
        max_width=max( max_width, len(key))

    print('{0: >{1}} | Count'.format("Keyword", max_width))
    for agg in aggregations:
        print('{0: >{1}} | {2}'.format(agg,max_width, cast.deep_get(aggregations,agg,"doc_count")))

    print(" ")

    # Verbosely print the hits
    if args.verbose:
        hits=key_res.get('hits', {"hits":[]})["hits"]
        print("Displaying {0} of {1} logs:".format(len(hits), total))
        for hit in hits:
            source = hit["_source"]
            print("{0} {1} | {2}".format(source.get("timestamp"), 
                source.get("hostname"), source.get("message")))

if __name__ == "__main__":
    sys.exit(main(sys.argv))
