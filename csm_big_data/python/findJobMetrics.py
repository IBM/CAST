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
    # nodes_to_query
    # Print out Nodes Used
    node_list = list()
    node_list.append('c650f08p21')
    for nodes in tr_data["compute_nodes"]:
        node_list.append(str(nodes))

    print(node_list)

    begin_time = tr_data["begin_time"][:10]+'T'+tr_data["begin_time"][11:]+'Z'
    end_time = tr_data["history"]["end_time"][:10]+'T'+tr_data["history"]["end_time"][11:]+'Z'

    print(begin_time)
    print(end_time)
    timerange='''@timestamp:[{0} TO {1}]'''.format(begin_time, end_time)
    nodes = 'source: {0}'.format(" ".join(node_list))
    env_query="{0} AND {1}".format(nodes, timerange)
    print env_query
    ed_res =es.search(
        index='cast-zimon-*',
        q=env_query
    )

    # TODO make sure which index to query
    # Check if querying the terms is OR or AND currently
    # ed_res = es.search(
    #     index='*',#'cast-zimon-*',
    #     body=
    #     {
    #         'size': 10000,
    #         'from' : 0,
    #         'query':{
    #             'bool' :{
    #                 'must':{
    #                     'match_all': {}
    #                 },
    #                 'should':{
    #                     'range' : {
    #                         '_source.@timestamp': {
    #                             'gte' : '2018-08-03T10:21:41.08686Z',#begin_time, 
    #                             'lte' : '2018-08-03T14:33:17.249406Z',#end_time, 
    #                             'relation' : "within"
    #                         }
    #                     }
    #                 },
    #                 'filter':{
    #                     'terms':{
    #                         'source':tr_data["compute_nodes"]
    #                     }
    #                 }
    #             },

    #         }
    #     }
    # )

    # print(ed_res["hits"]["hits"][0]["_source"]["@timestamp"])
    print("Hits: " + str(ed_res["hits"]["total"]) +"\n")
    # print(ed_res["hits"])

    for tmstmp in ed_res["hits"]["hits"]:
        print(tmstmp["_source"]["@timestamp"])
        print('\tMem_Active: '+ str(tmstmp["_source"]["data"]["mem_active"]))
        print('\tCPU_User: '+ str(tmstmp["_source"]["data"]["cpu_user"])+"\n")




if __name__ == "__main__":
    sys.exit(main(sys.argv))
