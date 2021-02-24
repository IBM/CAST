#!/bin/sh
# encoding: utf-8
#================================================================================
#
#    findUserJobs.py
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

from datetime import datetime
from dateutil.parser import parse
from elasticsearch import Elasticsearch
from elasticsearch.serializer import JSONSerializer
from elasticsearch import exceptions

import cast_helper as cast

TARGET_ENV='CAST_ELASTIC'


def main(args):

    # Specify the arguments.
    parser = argparse.ArgumentParser(
        description='''A tool for finding a list of the supplied user's jobs.''')
    

    parser.add_argument( '-u', '--user', metavar='username', dest='user', default=None,
        help="The user name to perform the query on, either this or -U must be set.")
    parser.add_argument( '-U', '--userid', metavar='userid', dest='userid', default=None,
        help="The user id to perform the query on, either this or -u must be set.")
    parser.add_argument( '--size', metavar='size', dest='size', default=1000,
        help='The number of results to be returned. (default=1000)')
    parser.add_argument( '--state', metavar='state', dest='state', default=None, 
        help='Searches for jobs matching the supplied state.')

    parser.add_argument( '--starttime', metavar='YYYY-MM-DDTHH:MM:SS', dest='starttime', default=None,
        help='A timestamp representing the beginning of the absolute range to look for failed jobs, if not set no lower bound will be imposed on the search.')
    parser.add_argument( '--endtime', metavar='YYYY-MM-DDTHH:MM:SS', dest='endtime', default=None,
        help='A timestamp representing the ending of the absolute range to look for failed jobs, if not set no upper bound will be imposed on the search.')

    # TODO should this be a percentage?
    parser.add_argument( '--commonnodes', metavar='threshold', dest='commonnodes', default=-1,
        help='Displays a list of nodes that the user jobs had in common if set. Only nodes with collisions exceeding the threshold are shown. (Default: -1)')

    parser.add_argument( '-v', '--verbose', action='store_true',
        help='Displays all retrieved fields from the `cast-allocation` index.')

    parser.add_argument( '-t', '--target', metavar='hostname:port', dest='target', default=None, 
        help='An Elasticsearch server to be queried. This defaults to the contents of environment variable "CAST_ELASTIC".')


    args = parser.parse_args()

    # If the target wasn't specified check the environment for the target value, printing help on failure.
    if args.target is None:
        if TARGET_ENV in os.environ:
            args.target = os.environ[TARGET_ENV]
        else:
            parser.print_help()
            print("Missing target, '%s' was not set." % TARGET_ENV)
            return 2

    if args.user is None and args.userid is None:
        parser.print_help()
        print("Missing user, --user or --userid must be supplied.")
        return 2
    
    # Open a connection to the elastic cluster, if this fails is wrong on the server.
    es = Elasticsearch(
        args.target, 
        sniff_on_start=True,
        sniff_on_connection_fail=True,
        sniffer_timeout=60
    )

    # Ammend compute nodes for common node search.
    fields=cast.USER_JOB_FIELDS
    if args.commonnodes > 0:
       fields += ["data.compute_nodes"]



    try:
        resp = cast.search_user_jobs(es, 
            user_name  = args.user, 
            user_id    = args.userid,
            job_state  = args.state,
            start_time = args.starttime,
            end_time   = args.endtime,
            size       = args.size)
    except exceptions.RequestError as e:
        cast.print_request_error(e)
        return 4

    # Parse the response from elasticsearch.
    hits       = cast.deep_get(resp, "hits", "hits")
    total_hits = cast.deep_get(resp, "hits","total")
    node_collisions = {}

    print_fmt="{5: >10} | {0: >5} | {1: >8} | {2: <8} | {3: <26} | {4: <26}"
    print(print_fmt.format("AID", "P Job ID", "S Job ID", "Begin Time", "End Time", "State"))

    hits.sort(key=lambda x: cast.deep_get(x,"_source","data","allocation_id"), reverse=False)

    # Process hits.
    for hit in hits:
        data=cast.deep_get(hit,"_source","data")
        
        if data:
            condition = cast.deep_get(data, "history","end_time")
            print( print_fmt.format(
                data.get("allocation_id"), data.get("primary_job_id"), data.get("secondary_job_id"),
                data.get("begin_time"), cast.deep_get(data, "history","end_time") if (condition!=None) else " ",
                data.get("state")))
            
            # Generate a counter. 
            if args.commonnodes > 0:
                for node in data.get("compute_nodes"):
                    node_collisions[node] = 1 + node_collisions.get(node, 0)


    # Print out common nodes with collisions above threshold.
    if args.commonnodes > 0:
        max_width=4
        collision_found=False

        # get the max width to improve printing. 
        for key in node_collisions:
            max_width=max(len(key),max_width)

        print( "=============================" )
        print( "Nodes common between jobs:" )
        print( "=============================" )
        print("{0:>{1}} : {2}".format("node", max_width, "common count"))
        
        node_count=int(args.commonnodes)
        for key,value in sorted( node_collisions.iteritems(), key=lambda k_v: (k_v[1],k_v[0]), reverse=False):
            if int(value) > node_count:
                collision_found = True
                print("{0:>{1}} : {2}".format(key, max_width, value))
        
        if not collision_found:
            print("No nodes exceeded collision threshold: {0}".format(args.commonnodes))

if __name__ == "__main__":
    sys.exit(main(sys.argv))
