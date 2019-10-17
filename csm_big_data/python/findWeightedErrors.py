#!/bin/python
# encoding: utf-8
#================================================================================
#
#    findWeightedErrors.py
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
import json
import sets
import sys
import os

from datetime import datetime
from dateutil.parser import parse
from elasticsearch import Elasticsearch
from elasticsearch.serializer import JSONSerializer
from elasticsearch import exceptions
import cast_helper as cast

TARGET_ENV='CAST_ELASTIC'

def build_mapping_query(es, body, ranges, error):

    (index, source, mapping, category) = (
        error.get("index", "_all"),
        error.get("source", None),
        error.get("mapping", []),
        error.get("category", None ))
    
    # TODO Error reporting. 
    if category is None:
        return (None, None)
    
    #  Adjust the source.
    if ranges and source:
        mm = ranges[1].get("multi_match")
        if mm :
            mm["fields"] = [source]
    
    # Set up some aggregations.
    fields=sets.Set([source])
    agg_filters={}
    should_mapping=[]

    # Iterate over the mapping
    for field_map  in mapping:

        # Parse the field mapping. 
        (field, value, boost, threshold) = (
            field_map.get("field", ""), 
            field_map.get("value", ""), 
            int(field_map.get("boost", 0)),
            field_map.get("threshold", None)
        )

        # Build the field query. 
        if threshold:
            should={
                "range" : { 
                    field : {
                        threshold : value,
                        "boost"   : boost
                    }
                }
            }
        elif value.find(" ") == -1:
            should={ 
                "regexp" : { 
                    field : {
                        "value" : value.lower(),
                        "boost" : boost
                    }
                } 
            }
        else:
            should={ 
                "match_phrase" : { 
                    field : {
                        "query" : value,
                        "boost" : boost
                    }
                } 
            }
        
        # Add to the aggregations.
        fields.add(field)

        agg_name="{0} | {1}".format(field,value)
        agg_filters[agg_name] = { "filter" : should }
        should_mapping.append(should)    
    
    # Build the body of the query. 
    body["query"]= {
        "bool" : {
            "must"   : ranges,
            "should" : should_mapping,
            "minimum_should_match" : 1
        }
    }
    
    body["_source"]+= list(fields)
    body["aggs"] = agg_filters

    # Perform the search operation.
    key_res = es.search(
        index=index,
        body=body
    )
    
    return (category, key_res)

def main(args):

    # Specify the arguments.
    parser = argparse.ArgumentParser(
        description='''A tool which takes a weighted listing of keyword searches and presents aggregations of this data to the user.''')
    
    parser.add_argument( '-a', '--allocationid', metavar='int', dest='allocation_id', default=-1,
        help='The allocation ID of the job.')
    parser.add_argument( '-j', '--jobid', metavar='int', dest='job_id', default=-1,
        help='The job ID of the job.')
    parser.add_argument( '-s', '--jobidsecondary', metavar='int', dest='job_id_secondary', default=0,
        help='The secondary job ID of the job (default : 0).')
    parser.add_argument( '-t', '--target', metavar='hostname:port', dest='target', default=None, 
        help='An Elasticsearch server to be queried. This defaults to the contents of environment variable "CAST_ELASTIC".')
    parser.add_argument( '-v', '--verbose', action='store_true',
        help='Displays the top --size logs matching the --errormap mappings.')
    parser.add_argument( '--size', metavar='size', dest='size', default=10,
        help='The number of results to be returned. (default=10)')
    parser.add_argument( '-H', '--hostnames', metavar='host', dest='hosts', nargs='*', default=None,
        help='A list of hostnames to filter the results to.')
    parser.add_argument( '--errormap', metavar="file", dest="err_map_file", default=None,
        help='A map of errors to scan the user jobs for, including weights.')

    args = parser.parse_args()

    # If the target wasn't specified check the environment for the target value, printing help on failure.
    if args.target is None:
        if TARGET_ENV in os.environ:
            args.target = os.environ[TARGET_ENV]
        else:
            parser.print_help()
            print("Missing target, '%s' was not set." % TARGET_ENV)
            return 2
    
    # Load the weighted error mapping.
    error_map=None
    if args.err_map_file:
        error_map = JSONSerializer().loads(open(args.err_map_file).read())

    if error_map is None:
        parser.print_help()
        print("Error map '%s', could not be loaded" % args.err_map_file)
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


    (ranges, should_match) = cast.build_timestamp_range(
        tr_data.get("begin_time"), 
        cast.deep_get(tr_data, "history", "end_time"))
    
    ranges.append(hostnames)

    # ---------------------------------------------------------------------------------------------
    # Build a body for the mapping query.
    body={
        "_source" : [ "@timestamp"],
        "size"    : args.size,
    }

    # Check the keywords supplied by the json.
    results={}
    for error in error_map:
        (category, result) = build_mapping_query(es, body.copy(), ranges, error)
        results[category]=result


    print(" ")
    # Print the results.
    for category, response in sorted(results.iteritems(), 
            key=lambda (k, v): cast.deep_get( v, "hits", "max_score" ), reverse=True ):

        # Get aggregations. 
        aggregations = response.get("aggregations", [])
        total= cast.deep_get(response, "hits", "total")
        
        print( "\"{0}\" Max Score : {1}".format( category, 
            cast.deep_get(response, "hits", "max_score") ) ) 
        print( "\"{0}\" Count : {1}".format( category, total ) ) 
        
        if aggregations is not None:
            # Sort aggregations by document count.
            for (aggregation,value) in sorted(aggregations.iteritems(), 
                    key=lambda (k, v): v.get("doc_count"), reverse=True):
                print("  \"{0}\" : {1}".format( aggregation, value.get("doc_count") ) ) 

        if args.verbose:
            hits=cast.deep_get(response, "hits", "hits")

            print("\nTop {0} \"{1}\" Results:".format(len(hits), category))
            print("-"  * 42 )
            for hit in hits: 
                print(json.dumps(hit["_source"]))
        print("="  * 42 )
        print(" ")
    # ---------------------------------------------------------------------------------------------

if __name__ == "__main__":
    sys.exit(main(sys.argv))
