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
from datetime import datetime

TARGET_ENV='CAST_ELASTIC'

def main(args):

	# Specify the arguments.
    parser = argparse.ArgumentParser(
        description='''A tool for finding keywords during the run time of a job.''')
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
    match_clause= '{{"match":{{"{0}":"{1}"}}}}'
    filter_clause = '{{"filter":{{ "range": {{  "data.begin_time": {{ "gte" : "{0}","lte" : "{1}"}}}}}}}}'
    tr_query = should_query.format(
    	match_clause.format("@timestamp", args.timestamp))
    
    print(tr_query)
    
    # Time from milliseconds to date format
    tm_stmp = datetime.fromtimestamp(int(args.timestamp)/1000.0).strftime('%Y-%m-%d %H:%M:%S.%f')
    day_before = datetime.fromtimestamp((int(args.timestamp)-86400000)/1000.0).strftime('%Y-%m-%d %H:%M:%S.%f')
    day_after = datetime.fromtimestamp((int(args.timestamp)+86400000)/1000.0).strftime('%Y-%m-%d %H:%M:%S.%f')
    print("Timestamp: " + tm_stmp)   
    print("Day Before: " + day_before)
    print("Day After: " + day_after)

    db = (int(args.timestamp)-86400000)
    da = (int(args.timestamp)+86400000)

    #time_query = '{{"query": {{"range" : {{"data": {{ "begin_time" : {{ "gte" : "{0}","lte" : "{1}","relation" : "within" }}}}}}}}}}'.format(db,da)

    time_query = should_query.format(filter_clause.format(day_before, day_after))
    print(time_query)
#    Execute the query on the cast-allocation index.
    tr_res = es.search(
        index="cast-allocation",
        #body=tr_query
        #body=time_query
		body={
			'query':{
				'match_all' :{}
			}
		}
        #body={
        #  'query': { 
            #'range' : {
			#	'data.begin_time': {
			#		'gte' : "2018-07-17 15:28:36",
			#		'lte' : "2018-07-17 15:28:43",
			#		'relation' : "within"
			#	}
			#}
			#'range' : {
				#'@timestamp' : {
					#'gte' : 1531855716000,
					#'lte' : 1531855723000,
					#'gte' : db,
					#'lte' : da,
					#'relation' : "within"
				#}
			#}
			#'match_all': {},
			#'wildcard' : {'data.begin_time': '*'}
			#'bool': { 
            #  'should': [
            #   {'match':{ 'data.begin_time': "*"}}
            #  ],
               
			#  'filter': [
            #   {'range':{ 'data.begin_time': {'gte': "2018-07-17 15:28:41.315178" }}}
            # ]
            #}
          #}
        #}
        
    )
    total_hits = tr_res["hits"]["total"]

    print("Got {0} Hit(s) for specified job, searching for keywords.".format(total_hits))
    #print(tr_res["hits"]["hits"])
    # tr_data = tr_res["hits"]["hits"][0]["_source"]["data"]

    for data in tr_res["hits"]["hits"]:
        tr_data = data["_source"]["data"]
        

        start_time='"{0}"'.format(tr_data["begin_time"])
        # If a history is present end_time is end_time, otherwise it's now.
        if "history" in tr_data:
            end_time='"{0}"'.format(tr_data["history"]["end_time"])
        else:
            end_time="*"
        print("begin_time: "  + start_time)
        print("end_time:   " + end_time)
        
        start = datetime.strptime(start_time, '%Y-%m-%d %H:%M:%S.%f')
        if start > day_before:
            print("Within time range")



if __name__ == "__main__":
    sys.exit(main(sys.argv))















