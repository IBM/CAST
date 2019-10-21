#!/bin/python
# encoding: utf-8
#================================================================================
#
#    findJobMetrics.py
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
from elasticsearch import Elasticsearch
from elasticsearch.serializer import JSONSerializer
from elasticsearch import exceptions
from datetime import datetime
import operator

import cast_helper as cast

TARGET_ENV='CAST_ELASTIC'

def main(args):

    # Specify the arguments.
    parser = argparse.ArgumentParser(
        description='''A tool for finding metrics about the nodes participating in the supplied job id.''')
    
    parser.add_argument( '-a', '--allocationid', metavar='int', dest='allocation_id', default=-1,
        help='The allocation ID of the job.')
    parser.add_argument( '-j', '--jobid', metavar='int', dest='job_id', default=-1,
        help='The job ID of the job.')
    parser.add_argument( '-s', '--jobidsecondary', metavar='int', dest='job_id_secondary', default=0,
        help='The secondary job ID of the job (default : 0).')
    parser.add_argument( '-t', '--target', metavar='hostname:port', dest='target', default=None, 
        help='An Elasticsearch server to be queried. This defaults to the contents of environment variable "CAST_ELASTIC".')
    parser.add_argument( '-H', '--hostnames', metavar='host', dest='hosts', nargs='*', default=None,
        help='A list of hostnames to filter the results to.')
    parser.add_argument( '-f', '--fields', metavar='field', dest='fields', nargs='*', default=None,
        help='A list of fields to retrieve metrics for (REQUIRED).')
    parser.add_argument( '-i', '--index', metavar='index', dest='index', default='_all', 
        help='The index to query for metrics records.')
    parser.add_argument( '--correlation', action='store_true',
        help="Displays the correlation between the supplied fields over the job run.")

    args = parser.parse_args()

    # If the target wasn't specified check the environment for the target value, printing help on failure.
    if args.target is None:
        if TARGET_ENV in os.environ:
            args.target = os.environ[TARGET_ENV]
        else:
            parser.print_help()
            print("Missing target, '%s' was not set." % TARGET_ENV)
            return 2

    if args.fields is None:
        print("Fields weren't set for metrics analysis.")
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

    print("Got {0} Hit(s) for specified job:".format(total_hits))
    if total_hits == None:
        print("# Sorry. Could not find any matching results.")
        return 0
    if total_hits != 1:
        print("This implementation only supports queries where the hit count is equal to 1.")
        return 3

    hits       = cast.deep_get(tr_res, "hits", "hits")
    allocation = cast.deep_get(hits[0], "_source", "data")

    # ---------------------------------------------------------------------------------------------
    # Build the hostnames string:
    if args.hosts is None:
        args.hosts = allocation.get("compute_nodes")
    
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
    date_format= '%Y-%m-%d %H:%M:%S.%f'
    print_format='%Y-%m-%d %H:%M:%S:%f'
    search_format='epoch_millis'

    # Determine the timerange:
    start_time=datetime.strptime(allocation.get("begin_time"), date_format)

    timestamp_range= {
        "gte"    : "{0}000".format(start_time.strftime('%s')),
        "format" : search_format
    }

    # If a history is present end_time is end_time, otherwise it's now.
    if "history" in allocation:
        end_time=datetime.strptime(allocation.get("history").get("end_time"), date_format)
        timestamp_range["lte"] = "{0}999".format(end_time.strftime('%s'))
    
    timerange={
        "range" : {
            "@timestamp" : timestamp_range
        }
    }
    # ---------------------------------------------------------------------------------------------

    # Matrix stats are very interesting..
    stats={
        "statistics" : {
            "matrix_stats" : {
                "fields" : args.fields
            }
        }
    }

    for field in args.fields:
        stats[field] = { "extended_stats" : { "field" : field } } 

    body={ 
        "query" :{ 
            "bool": { 
                "must" : [
                    hostnames,
                    timerange
                ]
            }
        },
        "aggs": stats, 
        "size" : 0
    }

    try:
        key_res = es.search(  
            index=args.index, # TODO This should be replaced.
            body=body
        )
    except exceptions.RequestError as e:
        cast.print_request_error(e)
        return 4

    if args.allocation_id > 0 :
        print("\nMetric Analysis for Allocation ID {0} :\n".format(args.allocation_id))
    else : 
        print("\nMetric Analysis for Job ID {0} - {1} :\n".format(args.job_id, args.job_id_secondary))
    

    # Print the table.
    aggs=cast.deep_get(key_res, "aggregations")
    if aggs is not None:
        max_width=len("Field")
        for agg in aggs:
            max_width=max(max_width, len(agg))
        
        print("{0:>{1}} | {2: >14} | {3: >14} | {4: >14} | {5: >14} | Count".format(
            "Field", max_width, "Min", "Max", "Average", "Std Dev"))

        print_fmt="{0: >{1}} | {2:>14.3f} | {3:>14.3f} | {4:>14.3f} | {5:>14.3f} | {6}"
        print_str="{0: >{1}} | {2:>14.3} | {3:>14.3} | {4:>14.3} | {5:>14.3} | {6}"

        for agg in aggs:
            try:
                print(print_fmt.format(agg, max_width, aggs[agg]["min"], aggs[agg]["max"],
                    aggs[agg]["avg"], aggs[agg]["std_deviation"], aggs[agg]["count"]))
            except ValueError:
                continue
            except KeyError:
                continue

        #print matrix stats
        if args.correlation:
            print("\n{0}".format("="*80))
            print("Field Correlations:")
            stat_fields=aggs["statistics"].get("fields",[])
            for stat in stat_fields:
                name=stat["name"]
                print("\n{0}:".format( name))
                
                correlation=stat["correlation"]
                corr_d=sorted(correlation.items(), key=operator.itemgetter(1))
                
                for field in corr_d:
                    if field[0] != name:
                        print("  {0} : {1}".format(field[0], field[1]))

    else:
        print("No aggregations were found.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
