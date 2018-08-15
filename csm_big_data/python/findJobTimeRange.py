#!/bin/python
# encoding: utf-8
#================================================================================
#
#    findJobTimeRange.py
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
import json

TARGET_ENV='CAST_ELASTIC'

def deep_get( obj, *keys):
    return reduce(lambda o, key: o.get(key, None) if o else None, keys, obj)

def main(args):

    # Specify the arguments.
    parser = argparse.ArgumentParser(
        description='''A tool for finding when a job was running through use of the big data store.''')
    
    parser.add_argument( '-a', '--allocationid', metavar='int', dest='allocation_id', default=-1,
        help='The allocation ID of the job.')
    parser.add_argument( '-j', '--jobid', metavar='int', dest='job_id', default=-1,
        help='The job ID of the job.')
    parser.add_argument( '-s', '--jobidsecondary', metavar='int', dest='job_id_secondary', default=0,
        help='The secondary job ID of the job (default : 0).')
    parser.add_argument( '-t', '--target', metavar='hostname:port', dest='target', default=None, 
        help='An Elasticsearch server to be queried. This defaults to the contents of environment variable "CAST_ELASTIC".')
    parser.add_argument( '-H', '--hostnames', metavar='host', dest='hosts', nargs='*', default=None,
        help='A list of hostnames to filter the results to ')

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
    should_query='{{"query":{{"bool":{{ "should":[{0}] {1} }} }} }}'
    match_clause= '{{"match":{{"{0}":{1} }} }}'

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
    total_hits = tr_res["hits"]["total"]

    print("Found {0} matches for specified the job.".format(total_hits))
    if total_hits != 1:
        print("This implementation only supports queries where the hit count is equal to 1.")
        return 3


    # TODO make this code more fault tolerant
    hits= deep_get(tr_res, "hits", "hits")
    if len(hits) > 0 :
        tr_data = deep_get( hits[0], "_source", "data")

        date_format= '%Y-%m-%d %H:%M:%S.%f'
        print_format='%Y-%m-%d %H:%M:%S:%f'
        search_format='"yyyy-MM-dd HH:mm:ss:SSS"'

        start_time=datetime.strptime(tr_data["begin_time"], '%Y-%m-%d %H:%M:%S.%f')
        start_time='{0}'.format(start_time.strftime(print_format)[:-3])

        # If a history is present end_time is end_time, otherwise it's now.
        if "history" in tr_data:
            end_time=datetime.strptime(tr_data["history"]["end_time"], date_format)
            end_time='{0}'.format(end_time.strftime(print_format)[:-3])
        else:
            end_time="Still Running"

        print( "Start Time: {0} \n  End Time: {1}".format(start_time, end_time))
    
    # ---------------------------------------------------------------------------------------------



if __name__ == "__main__":
    sys.exit(main(sys.argv))
