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
    should_query='{{"query":{{"bool":{{"must":[{0}]}}}}}}'
    match_clause= '{{"match":{{"{0}":{1}}}}}'

    if args.allocation_id > 0 :
        tr_query = should_query.format(
            match_clause.format("data.allocation_id", args.allocation_id))
    else : 
        tr_query = should_query.format(
            "{0},{1}".format(
                match_clause.format("data.primary_job_id", args.job_id ),
                match_clause.format("data.secondary_job_id", args.job_id_secondary )))

    # Execute the query on the cast-allocation index.
    tr_res = es.search(
        index="cast-allocation",
        body=tr_query
    )
    total_hits = tr_res["hits"]["total"]

    print("Got {0} Hit(s) for specified job, searching for keywords.".format(total_hits))
    if total_hits != 1:
        print("This implementation only supports queries where the hit count is equal to 1.")
        return 3

    print(tr_res["hits"]["hits"][0]["_source"]["data"]["primary_job_id"])

    # TODO make this code more fault tolerant
    tr_data = tr_res["hits"]["hits"][0]["_source"]["data"]
    # ---------------------------------------------------------------------------------------------
    
    # tr_data["compute_nodes"].append('c650f08p21')

    # Print out Nodes Used
    for nodes in tr_data["compute_nodes"]:
        print(nodes)



    # ed_res = es.search(
    #     index='cast-zimon-*',
    #     body={
    #         'query':{
    #             'range' : {
    #                 'timestamp': {
    #                     'gte' : tr_data["begin_time"], 
    #                     'lte' : tr_data["history"]["end_time"], 
    #                     'relation' : "within"
                #     }
                # }
                # 'bool':{
                #     'should': {
                #         'match':{
                #             'terms':{
                #                 'source': tr_data["compute_nodes"],
                #                 'minimum_should_match': 1
                #             }
                #         }
                #     }
                # }
                # 'terms':{
                #     'source': tr_data["compute_nodes"],
                #     'minimum_should_match': 1
                # }
                
                # "match_all": {}
            # },
            # 'filter':{
            #     'terms':{
            #         'source': 'c650f08p21'
            #     }
            # }
    #     }
        
    # )

    # print(ed_res["hits"]["hits"][0]["_source"]["source"])
    # print(ed_res["hits"]["total"])
    # print(ed_res["hits"]["hits"][0])




if __name__ == "__main__":
    sys.exit(main(sys.argv))
