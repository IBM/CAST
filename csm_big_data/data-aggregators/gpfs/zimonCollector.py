#!/bin/python
# encoding: utf-8
# ================================================================================
#
#    zimonCollector.py
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
# ================================================================================

import sys 
import json
import getopt
import socket
import logging.config
logger=logging.getLogger(__name__)

# Add zimon query handlers.
sys.path.insert(0, '/usr/lpp/mmfs/lib/mmsysmon')
sys.path.insert(0, '/usr/lpp/mmfs/lib/mmsysmon/threshold')
from Query import Query
from QueryHandler import QueryHandler2 as QueryHandler

# Help string.
HELP_STR='''
A tool for extracting zimon sensor data from a gpfs collector node and shipping it in a json format.
to logstash. Intended to be run from a cron job.

Options:
    Flag                              | Description < default >
    ==================================|============================================================
    -h, --help                        | Displays this message.
    --collector <host>                | The hostname of the gpfs collector. <127.0.0.1>
    --collector-port <port>           | The collector port for gpfs collector. <9084>
    --logstash <host>                 | The logstash instance to send the JSON to. <127.0.0.1>
    --logstash-port <port>            | The logstash port to send the JSON to. <10522>
    --bucket-size <int>               | The size of the bucket accumulation in seconds. <60>
    --num-buckets <int>               | The number of buckets to retrieve in the query. <10>
    --metrics <Metric1[,Metric2,...]> | A comma separated list of zimon sensors to get metrics from.
                                      |  <cpu_system,cpu_user,mem_active,gpfs_ns_bytes_read,
                                      |      gpfs_ns_bytes_written,gpfs_ns_tot_queue_wait_rd,
                                      |      gpfs_ns_tot_queue_wait_wr>
'''

# Commandline options.
SHORT_OPTS="h"
LONG_OPTS=["help","collector=","collector-port=", "logstash=", "logstash-port=", 
    "metrics=", "bucket-size=", "num-buckets="]

def main(args):
    ''' Main function, performs a query to zimon and ships the results to BDS.'''

    # Collector connection, 
    collector="127.0.0.1"
    collectorPort=9084
    
    # Logstash connection. 
    logstash="127.0.0.1"
    logstashPort=10522

    # Query options.
    bucketSize=60
    numBuckets=10
    metrics="cpu_system,cpu_user,mem_active,gpfs_ns_bytes_read,gpfs_ns_bytes_written,gpfs_ns_tot_queue_wait_rd    ,gpfs_ns_tot_queue_wait_wr"
    
    try:
        opts, optargs =  getopt.getopt(args[1:], SHORT_OPTS, LONG_OPTS)
    except getopt.GetoptError as err: 
        print("Invalid option detected: %s", err)
        print(HELP_STR)
        return 1

    for o,a in opts:
        if o in ("-h", "--help"):
            print(HELP_STR)
            return 1
        elif o in ("--collector="):
            collector = a
        elif o in ("--collector-port="):
            collectorPort = a
        elif o in ("--logstash="):
            logstash = a
        elif o in ("--logstash-port="):
            logstashPort = a
        elif o in ("--metrics="):
            metrics = a
        elif o in ("--bucket-size="):
            bucketSize = a
        elif o in ("--num-buckets="):
            numBuckets = a

    # Build the query handler.
    qh = QueryHandler(collector, collectorPort, logger)

    # Construct the query.
    query=Query()
    query.normalize_rates = False
    query.setBucketSize(bucketSize)
    query.setTime(num_buckets=numBuckets)
    query.addMetric(metrics)

    # Run the query, clean the data.
    qr = qh.runQuery(query) 

    if qr is None:
        print("QueryResult had no data.")
        return 3

    qr.remove_rows_with_no_data()
    sensorData={}
    rowNum=0
    
    # Pre construct the map.
    for columnInfo in qr.columnInfos:
        hostName=columnInfo.keys[0][0]
        sensorData[hostName]=[]
    
    # Iterate over the rows recieved.
    for row in qr.rows:
        colNum=0
    
        # Initialize the objects for this row. 
        for host in sensorData:
            sensorData[host].append({
                "source"   : host, 
                "type"     : "zimon",
                "timestamp": row.tstamp,
                "data": {}})
    
        # Parse the column info on this pass. 
        for columnInfo in qr.columnInfos:
            sensorName=columnInfo.name
            hostName=columnInfo.keys[0][0]
            sensorData[hostName][rowNum]["data"][sensorName] = row.values[colNum]
            
            colNum+=1
        rowNum+=1
    
    # Payload to be written to big data store,
    payload=""
    for host in sensorData:
        for row in sensorData[host]:
            payload+=json.dumps(row) + '\n'
    
    # Write the generated payload to the socket.
    try:
        logstash_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        logstash_socket.connect((logstash, logstashPort))
        logstash_socket.send(payload)
        logstash_socket.close()
    except socket.error, msg:
        print("Socket had error: %s", msg)
        return 2
    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv))
