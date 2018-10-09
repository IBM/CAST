#!/bin/python
# encoding: utf-8
#================================================================================
#
#    findJobsRunning.py
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
import re

import cast_helper as cast

TARGET_ENV='CAST_ELASTIC'

def main(args):

	# Specify the arguments.
    parser = argparse.ArgumentParser(
        description='''A tool for finding jobs running at the specified time.''')
    parser.add_argument( '-t', '--target', metavar='hostname:port', dest='target', default=None, 
        help='An Elasticsearch server to be queried. This defaults to the contents of environment variable "CAST_ELASTIC".')
    parser.add_argument( '-T', '--time', metavar='YYYY-MM-DD HH:MM:SS', dest='timestamp', default="now",
        help='A timestamp representing a point in time to search for all running CSM Jobs. HH, MM, SS are optional, if not set they will be initialized to 0. (default=now)')
    parser.add_argument( '-s', '--size', metavar='size', dest='size', default=1000,
        help='The number of results to be returned. (default=1000)')
    parser.add_argument( '-H', '--hostnames', metavar='host', dest='hosts', nargs='*', default=None,
        help='A list of hostnames to filter the results to.')

    args = parser.parse_args()

    # If the target wasn't specified check the environment for the target value, printing help on failure.
    if args.target == None:
        if TARGET_ENV in os.environ:
            args.target = os.environ[TARGET_ENV]
        else:
            parser.print_help()
            print("Missing target, '%s' was not set." % TARGET_ENV)
            return 2

    # Parse the user's date.
    date_format='(\d{4})-(\d{1,2})-(\d{1,2})[ \.T]*(\d{0,2}):{0,1}(\d{0,2}):{0,1}(\d{0,2})'
    #date_print_format='%Y-%m-%d %H:%M:%S'
    date_print_format='%s'

    target_date=args.timestamp
    time_search=re.search(date_format, target_date)

    # Build the target timestamp and verify validity.
    if time_search : 
        (year,month,day,hour,minute,second) = time_search.groups()
        date = datetime( year=int(year), month=int(month), day=int(day), 
            hour=int(hour if hour else  0), 
            minute=int(minute if minute else 0), 
            second=int(second if second else 0) )

        target_date=datetime.strftime(date, date_print_format)

    elif target_date == "now":
        target_date=datetime.strftime(datetime.now(), date_print_format)
    else:
        parser.print_help()
        print("Invalid timestamp: {0}".format(target_date))
        return 2

    (range, match_min) =  cast.build_target_time_search(target_date)

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
        "_source" : [ "data.allocation_id", "data.primary_job_id", 
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
    tr_res = es.search(
        index="cast-allocation",
        body=body
    )

    # Get Hit Data
    hits          = cast.deep_get(tr_res, "hits", "hits")
    total_hits    = cast.deep_get(tr_res, "hits","total")
    hits_displayed= len(hits)

    print("Search found {0} jobs running at '{2}', displaying {1} jobs:\n".format(total_hits, len(hits), target_date)) 

    # Display the results of the search.
    if hits_displayed > 0:
        print_fmt="{0: >13} | {1: >12} | {2: <14} | {3: <26} | {4: <26}"
        print(print_fmt.format("Allocation ID", "Prim. Job ID", "Second. Job ID", "Begin Time", "End Time"))
        for hit in hits:
            data=cast.deep_get(hit, "_source", "data")
            if data:
                print(print_fmt.format(
                    data.get("allocation_id"), data.get("primary_job_id"), data.get("secondary_job_id"),
                    data.get("begin_time"), cast.deep_get(data, "history","end_time")))
        

    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv))
