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
from elasticsearch import Elasticsearch
from elasticsearch.serializer import JSONSerializer


TARGET_ENV='CAST_ELASTIC'

def main(args):

	# Specify the arguments.
    parser = argparse.ArgumentParser(
        description='''A tool for finding keywords during the run time of a job.''')
    
    # parser.add_argument( '-a', '--allocationid', metavar='int', dest='allocation_id', default=-1,
    #     help='The allocation ID of the job.')
    # parser.add_argument( '-j', '--jobid', metavar='int', dest='job_id', default=-1,
    #     help='The job ID of the job.')
    # parser.add_argument( '-s', '--jobidsecondary', metavar='int', dest='job_id_secondary', default=0,
    #     help='The secondary job ID of the job (default : 0).')
    parser.add_argument( '-t', '--target', metavar='hostname:port', dest='target', default=None, 
        help='An Elasticsearch server to be queried. This defaults to the contents of environment variable "CAST_ELASTIC".')
    # Need to check if this is valid
    parser.add_argument( '-T', '--time', metavar='timestamp', dest='timestamp', default=None,
        help='A list of keywords to search for in the Big Data Store (default : *).')
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

    # Build the query to get jobs with timestamp.
    should_query='{{"query":{{"bool":{{"should":[{0}]}}}}}}'
    match_clause= '{{"match":{{"{0}":{1}}}}}'
    tr_query = should_query.format(
    	match_clause.format("@timestamp", args.timestamp))

    # Execute the query on the cast-allocation index.
    tr_res = es.search(
        index="cast-allocation",
        body=tr_query
    )
    total_hits = tr_res["hits"]["total"]

    print("Got {0} Hit(s) for specified job, searching for keywords.".format(total_hits))

    tr_data = tr_res["hits"]["hits"][0]["_source"]["data"]
    print(tr_data["data.allocation_id"])



if __name__ == "__main__":
    sys.exit(main(sys.argv))















