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
from datetime import datetime

TARGET_ENV='CAST_ELASTIC'

def deep_get( obj, *keys):
    return reduce(lambda o, key: o.get(key, None) if o else None, keys, obj)

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
        help='A list of fields to retrieve metrics for.')
    parser.add_argument( '-i', '--index', metavar='index', dest='index', default='_all', 
        help='The index to query for metrics records.')
    

    args = parser.parse_args()

    # If the target wasn't specified check the environment for the target value, printing help on failure.
    if args.target == None:
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

    # Build the query to get the time range.
    should_query='{{ "query": {{ "bool": {{ "should":[{0}] {1} }} }} }}'
    match_clause= '{{ "match":{{ "{0}": {1} }} }}'

    if args.allocation_id > 0 :
        tr_query = should_query.format(
            match_clause.format("data.allocation_id", args.allocation_id), "")
    else : 
        tr_query = should_query.format(
            "{0},{1}".format(
                match_clause.format("data.primary_job_id", args.job_id ),
                match_clause.format("data.secondary_job_id", args.job_id_secondary )),
            ',"minimum_should_match" : 2' )

    # Execute the query on the cast-allocation index.
    tr_res = es.search(
        index="cast-allocation",
        body=tr_query
    )

    total_hits = deep_get(tr_res, "hits","total")

    print("Got {0} Hit(s) for specified job:".format(total_hits))
    if total_hits != 1:
        print("This implementation only supports queries where the hit count is equal to 1.")
        return 3

    hits=deep_get(tr_res, "hits", "hits")
    allocation=deep_get(hits[0], "_source", "data")

    # ---------------------------------------------------------------------------------------------
    date_format= '%Y-%m-%d %H:%M:%S.%f'
    print_format='%Y-%m-%d %H:%M:%S:%f'
    search_format='"yyyy-MM-dd HH:mm:ss:SSS"'
    
    start_time=datetime.strptime(allocation["begin_time"], date_format)
    start_time='"{0}"'.format(start_time.strftime(print_format)[:-3])
    
    if "history" in allocation:
        end_time=datetime.strptime(allocation["history"]["end_time"], date_format)
        end_time_clause=',"lte": "{0}"'.format(end_time.strftime(print_format)[:-3])
    else:
        end_time="now"
        end_time_clause=""

    timerange='''"range": {{ "@timestamp": {{ "gte": {0} {1}, "format" : {2} }} }}'''\
        .format(start_time, end_time_clause, search_format)

    
    # ---------------------------------------------------------------------------------------------

    # Build the hostnames string:
    if args.hosts is None:
        args.hosts = allocation["compute_nodes"]
    
    hostnames='''"multi_match" : {{ "query" : "{0}", "type" : "best_fields", "fields" : [ "hostname", "source" ], "tie_breaker" : 0.3, "minimum_should_match" : 1 }}'''
    hostnames = hostnames.format(" ".join(args.hosts))

    # ---------------------------------------------------------------------------------------------
    # Matrix stats are very interesting..
    #aggregation='"aggs": {{ "statistics" : {{ "matrix_stats" :{{ "fields" :  {0}  }} }} }}'.format(
    #    args.fields).replace("'",'"')

    stats=[]
    for field in args.fields:
        stats.append('"{0}_stat" : {{ "extended_stats" : {{ "field": "{0}" }} }}'.format(field))
    aggregation='"aggs": {{ {0} }}'.format(",".join(stats))


    query_filter='"must": [ {{ {0} }}, {{ {1} }} ]'.format(timerange, hostnames)


    query='{{ "query" :{{ "bool": {{ {0} }} }}, {1}, "size": {2} }}'.format(query_filter, aggregation, 0)
    

    key_res = es.search(  
        index=args.index, # TODO This should be replaced.
        body=query
    )

    if args.allocation_id > 0 :
        print("\n\nMetric Analysis for Allocation ID {0} :\n".format(args.allocation_id))
    else : 
        print("\n\nMetric Analysis for Job ID {0} - {1} :\n".format(args.job_id, args.job_id_secondary))
    

    # Print the table.
    aggs=deep_get(key_res, "aggregations")
    
    max_width=len("Field")
    for agg in aggs:
        max_width=max(max_width, len(agg))
    
    print("{0:>{1}} | {2: >10} | {3: >10} | {4: >10} | {5: >10} | Count".format(
        "Field", max_width, "Min", "Max", "Average", "Std Dev"))

    print_fmt="{0: >{1}} | {2:10.3f} | {3:10.3f} | {4:10.3f} | {5:10.3f} | {6}"
    for agg in aggs:
        print(print_fmt.format(agg, max_width, aggs[agg]["min"], aggs[agg]["max"],
            aggs[agg]["avg"], aggs[agg]["std_deviation"], aggs[agg]["count"]))

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
