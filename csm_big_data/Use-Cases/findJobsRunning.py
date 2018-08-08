#!/bin/python
# encoding: utf-8
#================================================================================
#
#    findJobsrunning.py
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

def main(args):

	# Specify the arguments.
    parser = argparse.ArgumentParser(
        description='''A tool for finding keywords during the run time of a job.''')
    parser.add_argument( '-t', '--target', metavar='hostname:port', dest='target', default=None, 
        help='An Elasticsearch server to be queried. This defaults to the contents of environment variable "CAST_ELASTIC".')
    parser.add_argument( '-T', '--time', metavar='timestamp', dest='timestamp', default=None,
        help='The timestamp to search for jobs in YYYY-MM-DD HH:MM:SS.f format')
    parser.add_argument( '-d', '--days', metavar='days', dest='days', default=1,
        help='The days before and after the timestamp to include in the range')
    parser.add_argument( '-hr', '--hours', metavar='hours', dest='hours', default=0,
        help='The hours before and after the timestamp to include in the range')
    parser.add_argument( '-s', '--size', metavar='size', dest='size', default=500,
        help='The number of results to be returned')
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
    
    # Convert user input into milliseconds
    timestamp = int(datetime.strptime(args.timestamp, '%Y-%m-%d %H:%M:%S.%f').strftime('%s'))*1000

    # Time from milliseconds to date format to get the range
    tm_stmp = datetime.fromtimestamp(int(timestamp)/1000.0).strftime('%Y-%m-%d %H:%M:%S.%f')
    day_before = datetime.fromtimestamp((int(timestamp)-((int(args.days)*86400000) + (int(args.hours)*3600000)))/1000.0).strftime('%Y-%m-%d %H:%M:%S.%f')
    day_after  = datetime.fromtimestamp((int(timestamp)+((int(args.days)*86400000) + (int(args.hours)*3600000)))/1000.0).strftime('%Y-%m-%d %H:%M:%S.%f')

    # Execute the query on the cast-allocation index.
    tr_res = es.search(
        index="cast-allocation",
        body={
            'size': args.size,
            'from' : 0,
            'query': { 
                'range' : {
    				'data.begin_time': {
    					'gte' : day_before, 
    					'lte' : day_after, 
    					'relation' : "within"
    				}
    			},
                # Uncomment below to include a specific range
                # 'range' : {
                #     'data.history.end_time':{
                #         'gte' : day_before,
                #         'lte' : day_after, 
                #         'relation' : "within"
                #     }
                # }
            }
        }
        
    )
    total_hits = tr_res["hits"]["total"]

    print("----------Got {0} Hit(s) for jobs beginning and ending between {1} and {2}----------".format(total_hits,day_before,day_after))

    # If DB format is incorrect, uncomment to use this method to find all jobs running
    # query_results_extraction( es, day_before, day_after)

    for data in tr_res["hits"]["hits"]:
        tr_data = data["_source"]["data"]
        print("allocation_id: {0}".format(tr_data["allocation_id"]) )
        print("primary_job_id: {0}".format(tr_data["primary_job_id"]))
        print("secondary_job_id: {0}".format(tr_data["secondary_job_id"]))
        print("\tbegin_time: " + tr_data["begin_time"])
        print("\tend_time:   " + tr_data["history"]["end_time"] +"\n")

    
def query_results_extraction(es, day_before, day_after):

    tr_res = es.search(
        index="cast-allocation",
        body={
            'size' : args.size,
            'from' : 0,
            'query':{
                'match_all' :{}
            }
        }
    )

    print ("\n----------Checking for Jobs from {0} to {1}----------".format(day_before, day_after))
    for data in tr_res["hits"]["hits"]:
        tr_data = data["_source"]["data"]
        
        day_before   = datetime.strptime(day_before, '%Y-%m-%d %H:%M:%S.%f')
        day_after   = datetime.strptime(day_after, '%Y-%m-%d %H:%M:%S.%f')
        # If a history is present end_time is end_time, otherwise it's now.
        start_time = datetime.strptime(tr_data["begin_time"], '%Y-%m-%d %H:%M:%S.%f')
        if start_time > day_before and start_time < day_after and "history" in tr_data:
            end_time   = datetime.strptime(tr_data["history"]["end_time"], '%Y-%m-%d %H:%M:%S.%f')
            if end_time < day_before and end_time < day_after:
                print ("allocation_id: {0}".format(tr_data["allocation_id"]) )
                print("primary_job_id: {0}".format(tr_data["primary_job_id"]))
                print("secondary_job_id: {0}".format(tr_data["secondary_job_id"]))
                print("\tbegin_time: "  + str(start_time))
                print("\tend_time:   " + str(end_time) + '\n')
        
if __name__ == "__main__":
    sys.exit(main(sys.argv))
