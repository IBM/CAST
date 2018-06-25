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

# Add zimon query handlers.
sys.path.insert(0, '/usr/lpp/mmfs/lib/mmsysmon')

import logging.config
logger=logging.getLogger(__name__)

import json
from queryHandler.Query import Query
from queryHandler.QueryHandler import QueryHandler2 as QueryHandler


TOP_ATTR    = [ 'id', 'frequency', 'tags', 'user_action_warn', 'user_action_error']
QUERY_ATTR  = ['type', 'name', 'metricOp', 'bucket_size',
              'computation', 'duration', 'filterBy', 'groupBy']
FILTER_ATTR = ['error', 'warn', 'direction', 'hysteresis']
RULES_ATR   = TOP_ATTR + QUERY_ATTR + FILTER_ATTR

server="127.0.0.1"
#server="0.0.0.0"
port=9084
logstash='10.7.4.41'
logstash_port=10522

qh = QueryHandler(server, port, logger)


#topo=qh.getTopology(True)
#for record in topo:
#    print(record)

query=Query()
query.normalize_rates = False
bucketSize=1
query.setBucketSize(bucketSize)
query.setTime(num_buckets=1)
query.addMetric("cpu_system,cpu_user,mem_active,gpfs_ns_bytes_read,gpfs_ns_bytes_written,gpfs_ns_tot_queue_wait_rd,gpfs_ns_tot_queue_wait_wr");


qr=qh.runQuery(query)
qr.remove_rows_with_no_data()
js=qr.json
#print(json.dumps(js))

# Implementation  #1

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

try:
    logstash_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    logstash_socket.connect((logstash, logstash_port))
    logstash_socket.send(payload)
    logstash_socket.close()
except socket.error, msg:
    sys.exit(1)




