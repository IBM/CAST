#!/bin/python
# encoding: utf-8
#================================================================================
#
#    elasticTest.py
#
#  © Copyright IBM Corporation 2015-2018. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================
from datetime import datetime
from elasticsearch import Elasticsearch
from elasticsearch.serializer import JSONSerializer

# Options
import getopt

# Load the csm component:
import sys
sys.path.append('/u/jdunham/bluecoral/bluecoral/work/csm/lib')
import lib_csm_py as csm
import lib_csm_wm_py as wm

short_opts = "ha:j:J:H:k:"
long_opts  = ["help","allocation_id=","primary_job_id=", "secondary_job_id=", "hostnames=", "key="]
cluster=['10.7.4.15:9200']

# TODO HELP


def query( input_details ):
    ''' Queries CSM and Elastic to get a listing of records with a matching key. '''
    # Query CSM for allocation details
    csm.init_lib()
    alloc_input=wm.allocation_query_input_t()
    alloc_input.allocation_id    = input_details["allocation_id"]
    alloc_input.primary_job_id   = input_details["primary_job_id"]
    alloc_input.secondary_job_id = input_details["secondary_job_id"] 

    rc,handler,alloc_output=wm.allocation_query(alloc_input)
    
    if rc is 0:
        allocation = alloc_output.allocation

        # Build some of the searches.
        end_time = "*"
        if allocation.history is not None:
            end_time = allocation.history.end_time.replace(' ','T')  + "Z"

        timerange='''@timestamp:[{0}Z TO {1}]'''.format(allocation.begin_time.replace(' ','T'), end_time)

        # The hostname query.
        hostnames='syslogHostname:('
        host_count=0
        if input_details["hostnames"] is not None:
            for i in range (0, allocation.num_nodes):
                host=allocation.get_compute_nodes(i)
                if host in input_details["hostnames"]:
                    host_count += 1
                    hostnames += "{0} OR ".format(host)
        else:
            for i in range (0, allocation.num_nodes):
                host_count += 1
                hostnames += "{0} OR ".format(allocation.get_compute_nodes(i))
            
        hostnames = hostnames[:-4]

        if host_count is 0:
            print("No hosts found matching query.")
            return 1;
        
        # The message portion of the query, splat query needs special TLC.
        message="message:"
        keys = input_details["key"].split(',')
        if input_details["key"] is not "*":
            for key in keys:
                message += '"{0}",'.format(key)
            message=message[:-1]
        else:
            message += "*"

        aggregation='aggs:{ keys: { filter key_count : { value_count: { "field" : " "} }'

        # Open a connection to the elastic cluster.
        es = Elasticsearch(
            cluster,
            sniff_on_start=True,
            sniff_on_connection_fail=True,
            sniffer_timeout=60
        )
        query='{0} AND {1} AND {2})'.format(message, timerange, hostnames)
        #query='message:"{0}" AND {1}'.format(input_details["key"], timerange)
        print(query)
        res = es.search( 
            index="_all",
            q=query
        )
        
        print("Got %d Hits:" % res['hits']['total'])
        #if res['hits']['total'] > 0:
        #    print(res['hits']['hits'][0])
    
    csm.api_object_destroy(handler)
    csm.term_lib()
#end query(input_details)

def main(args):
    # The configuration variables
    input_details={
        "allocation_id"    : -1,
        "primary_job_id"   : -1,
        "secondary_job_id" : 0,
        "hostnames"        : None,
        "key"              : "*"
    }
    
    # Load the configuration options TODO may want to use the code we had.
    try:
        opts, optargs = getopt.getopt(args[1:], short_opts, long_opts)
    except getopt.GetoptError as err:
        print("Invalid option detected: %s", err)
        sys.exit(1)
    
    for o,a in opts:
        if o in ("-h", "--help"):
            print("help not implemented")
            sys.exit()
        
        elif o in ("-a", "--allocation_id"):
            input_details["allocation_id"]    = int(a)

        elif o in ("-j", "--primary_job_id"):
            input_details["primary_job_id"]   = int(a)
        
        elif o in ("-J", "--secondary_job_id"):
            input_details["secondary_job_id"] = int(a)
        
        elif o in ("-H", "--hostnames"):
            input_details["hostnames"]        = a.split(',')
        
        elif o in ("-k", "--key"):
            input_details["key"]              = a
        
        else :
            assert False, "Option %s unhandled" % o

    query(input_details)
# End main(args)

if __name__ == "__main__":
    sys.exit(main(sys.argv))


