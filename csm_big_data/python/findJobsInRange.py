#!/bin/sh
# encoding: utf-8
#================================================================================
#
#    findJobsInRange.py
#
#    © Copyright IBM Corporation 2015-2021. All Rights Reserved
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

import argparse
import sys
import os
from elasticsearch import Elasticsearch
from elasticsearch.serializer import JSONSerializer
from elasticsearch import exceptions
from datetime import datetime
import re

import cast_helper as cast

TARGET_ENV='CAST_ELASTIC'

def main(args):

	# Specify the arguments.
    parser = argparse.ArgumentParser(
        description='''A tool for finding jobs running during the specified time range on a specified node.''')

    parser.add_argument( '-t', '--target', metavar='hostname:port', dest='target', default=None, 
        help='An Elasticsearch server to be queried. This defaults to the contents of environment variable "CAST_ELASTIC".')
    parser.add_argument( '--starttime', metavar='YYYY-MM-DDTHH:MM:SS', dest='starttime', default=None,
        help='A timestamp representing the beginning of the absolute range to look for failed jobs, if not set no lower bound will be imposed on the search.')
    parser.add_argument( '--endtime', metavar='YYYY-MM-DDTHH:MM:SS', dest='endtime', default=None,
        help='A timestamp representing the ending of the absolute range to look for failed jobs, if not set no upper bound will be imposed on the search.')
    parser.add_argument( '-H', '--hostnames', metavar='host', dest='hosts', nargs='*', default=None,
        help='A list of hostnames to filter the results to.')
    parser.add_argument( '-s', '--size', metavar='size', dest='size', default=1000,
        help='The number of results to be returned. (default=1000)')

    args = parser.parse_args()

    # If the target wasn't specified check the environment for the target value, printing help on failure.
    if args.target is None:
        if TARGET_ENV in os.environ:
            args.target = os.environ[TARGET_ENV]
        else:
            parser.print_help()
            print("Missing target, '%s' was not set." % TARGET_ENV)
            return 2

    (range, match_min) =  cast.build_time_range(args.starttime, args.endtime)

    print(range, match_min)
    bool_query={ "should" : range, "minimum_should_match" : match_min }

    if args.hosts:
        bool_query["must"] = { 
            "match" : { 
                "data.compute_nodes" : { "query" : " ".join(args.hosts) }
            }
        }

    body={
        "query" : {
            "bool" : bool_query
        },
        "_source" : [ "data.allocation_id", "data.primary_job_id", "data.user_id", "data.user_name",
            "data.secondary_job_id", "data.begin_time", "data.history.end_time"],
        "size": args.size
    }
    
    json = JSONSerializer()

    # Open a connection to the elastic cluster.
    es = Elasticsearch(
        args.target, 
        sniff_on_start=True,
        sniff_on_connection_fail=True,
        sniffer_timeout=60
    )

    # Execute the query on the cast-allocation index.
    try:
        tr_res = es.search(
            index="cast-allocation",
            body=body
        )
    except exceptions.RequestError as e:
        cast.print_request_error(e)
        return 4

    # Get Hit Data
    hits          = cast.deep_get(tr_res, "hits", "hits")
    total_hits    = cast.deep_get(tr_res, "hits","total")
    hits_displayed= len(hits)

    print("# Search found {0} jobs running, displaying {1} jobs:\n".format(total_hits['value'], len(hits)))

    # Display the results of the search.
    if hits_displayed > 0:
        print_fmt="{5: <10} | {0: >13} | {1: >12} | {2: <14} | {3: <26} | {4: <26}"
        print(print_fmt.format("Allocation ID", "Prim. Job ID", "Second. Job ID", "Begin Time", "End Time", "User Name"))
        hits.sort(key=lambda x: cast.deep_get(x,"_source","data","allocation_id"), reverse=False)
        for hit in hits:
            data=cast.deep_get(hit, "_source", "data")
            if data:
                condition = cast.deep_get(data, "history","end_time")
                print(print_fmt.format(
                    data.get("allocation_id"), data.get("primary_job_id"), data.get("secondary_job_id"),
                    data.get("begin_time"), cast.deep_get(data, "history","end_time") if (condition!=None) else " ", 
                    data.get("user_name")))
        

    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv))
