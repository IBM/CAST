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
from elasticsearch import exceptions
import json

import cast_helper as cast

TARGET_ENV='CAST_ELASTIC'

def mum_should_matchdeep_get( obj, *keys):
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
    parser.add_argument( '-v', '--verbose', action='store_true',
        help='Displays additional details about the job in the output.')

    args = parser.parse_args()

    # If allocation_id or job_id wasn't specified, printing help on failure.
    if args.allocation_id == -1 and args.job_id ==-1 :
        parser.print_help()
        print("Missing either allocationid or jobid. Require 1 of these fields to search.")
        return 2

    # If the target wasn't specified check the environment for the target value, printing help on failure.
    if args.target is None:
        if TARGET_ENV in os.environ:
            args.target = os.environ[TARGET_ENV]
        else:
            parser.print_help()
            print("Missing target, '%s' was not set." % TARGET_ENV)
            return 2

    # set up the fields for the search operation.
    fields=cast.SEARCH_JOB_FIELDS
    if args.verbose:
        fields.append("data.compute_nodes")
        
    
    # Open a connection to the elastic cluster, if this fails is wrong on the server.
    es = Elasticsearch(
        args.target, 
        sniff_on_start=True,
        sniff_on_connection_fail=True,
        sniffer_timeout=60
    )

    # Execute the query on the cast-allocation index.
    try:
        tr_res = cast.search_job(es, args.allocation_id, args.job_id, args.job_id_secondary, fields=fields)
    except exceptions.RequestError as e:
        cast.print_request_error(e)
        return 4


    total_hits = cast.deep_get(tr_res, "hits", "total")

    print("# Found {0} matches for specified the job.".format(total_hits))
    if total_hits == 0:
        print("# Sorry. Could not find any matching results.")
        return 0
    if total_hits != 1:
        print("# This implementation only supports queries where the hit count is equal to 1.")
        return 3

    # TODO make this code more fault tolerant
    hits= cast.deep_get(tr_res, "hits", "hits")
    if len(hits) > 0 :
        tr_data = cast.deep_get( hits[0], "_source", "data")

        date_format= '%Y-%m-%d %H:%M:%S.%f'
        print_format='%Y-%m-%d.%H:%M:%S:%f'
        search_format='"yyyy-MM-dd HH:mm:ss:SSS"'

        start_time=datetime.strptime(tr_data["begin_time"], '%Y-%m-%d %H:%M:%S.%f')
        start_time='{0}'.format(start_time.strftime(print_format)[:-3])

        # If a history is present end_time is end_time, otherwise it's now.
        if "history" in tr_data:
            end_time=datetime.strptime(tr_data["history"]["end_time"], date_format)
            end_time='{0}'.format(end_time.strftime(print_format)[:-3])
        else:
            end_time="now"
        
        print( "\nallocation-id: {0}".format(tr_data["allocation_id"]))
        print( "job-id: {0} - {1}".format(tr_data["primary_job_id"], tr_data["secondary_job_id"]))
        print( "user-name: {0} \nuser-id: {1}".format(tr_data["user_name"], tr_data["user_id"]))
        print( "begin-time: {0} \nend-time: {1}".format(start_time, end_time))
        
        if args.verbose:
            nodes=tr_data.get("compute_nodes", [])

            print('hostnames: ')
            for node in nodes:
                print("   - {0}".format(node))

    
    # ---------------------------------------------------------------------------------------------



if __name__ == "__main__":
    sys.exit(main(sys.argv))
