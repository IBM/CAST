#!/bin/python
# encoding: utf-8
#================================================================================
#
#    ufm_collection.py
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
import socket
import json
import sys
import getopt

from urllib import urlencode
import urllib2

''' The default payload for requests.'''
POST_PAYLOAD= {
    "attributes"    : [ "Infiniband_MBOut", "Infiniband_MBOutRate", "Infiniband_MBIn",
        "Infiniband_MBInRate", "Infiniband_PckOut","Infiniband_PckOutRate","Infiniband_PckIn",
        "Infiniband_PckInRate","Infiniband_RcvErrors","Infiniband_RcvErrors_Delta",
        "Infiniband_XmtDiscards","Infiniband_XmtDiscards_Delta","Infiniband_SymbolErrors",
        "Infiniband_SymbolErrors_Delta","Infiniband_LinkRecovers", "Infiniband_LinkRecovers_Delta",
        "Infiniband_LinkDowned","Infiniband_LinkDowned_Delta","Infiniband_LinkIntegrityErrors",

        "Infiniband_LinkIntegrityErrors_Delta","Infiniband_RcvRemotePhysErrors",
        "Infiniband_RcvRemotePhysErrors_Delta","Infiniband_XmtConstraintErrors",
        "Infiniband_XmtConstraintErrors_Delta","Infiniband_RcvConstraintErrors",
        "Infiniband_RcvConstraintErrors_Delta","Infiniband_ExcBufOverrunErrors",
        "Infiniband_ExcBufOverrunErrors_Delta","Infiniband_RcvSwRelayErrors",
        "Infiniband_RcvSwRelayErrors_Delta","Infiniband_VL15Dropped",

        "Infiniband_VL15Dropped_Delta","Infiniband_XmitWait","Infiniband_CumulativeErrors",
        "Infiniband_CBW","Infiniband_Normalized_MBOut","Infiniband_Normalized_CBW",
        "Infiniband_NormalizedXW" ],
    "functions"     : [ "RAW" ],
    "scope_object"  : "Site",
    "interval"      : 2,
    "monitor_object": "Port",
    "objects"       : ["Grid.default"]
}

''' The snapshot uri.'''
SNAPSHOT_URL = 'http://%s/ufmRest/monitoring/snapshot'

SHORT_OPTS=[]
LONG_OPTS=["ufm=", "logstash=", "logstash-port="]

def main(args):
    
    ufm_url=None
    logstash=None
    logstash_port=10522

    # Load the options.
    try:
        opts, optargs = getopt.getopt(args[1:], SHORT_OPTS, LONG_OPTS)
    except getopt.GetoptError as err:
        print("Invalid option detected: %s", err)
        return 1
    
    for o,a in opts:
        if o in ("--ufm"):
            ufm_url= SNAPSHOT_URL % a
        elif o in ("--logstash"):
            logstash=a
        elif o in ("--logstash-port"):
            logstash_port=a
    
    if ( ufm_url == None or logstash == None ):
        print( "Script requires both `ufm` and `logstash` to be set" )
        return 2

    #HOST= '10.7.4.41'
    #PORT= 10522
    #URL='http://10.7.0.41/ufmRest/monitoring/snapshot'
    username="admin"
    password="123456"
    
    # Build the payload
    json_payload=json.dumps(POST_PAYLOAD)
    json_len=len(json_payload)
    
    # Build the request
    request  = urllib2.Request(ufm_url,json_payload, 
        {
            'Content-Type'  : 'application/json', 
            'Content-Length': json_len,
            'Authorization' : ('Basic %s' % ("%s:%s" % (username,password)).encode('base64').strip())
        })
    response = urllib2.urlopen(request)
    
    data_str = response.read()
    data = json.loads(data_str)
    
    # Make sure the socket connection can be opened
    try:
        logstash_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        logstash_socket.connect((logstash, logstash_port))
    except socket.error, msg:
        sys.exit(1)
    
    payload=''
    # Iterate over the data and build a payload.
    for timestamp in data:
        sources = data[timestamp]["Port"]
    
        for source in sources:
            sources[source]['type']      = 'ufm-counters'
            sources[source]['timestamp'] = timestamp
            sources[source]['source']    = source
            payload += json.dumps(sources[source], indent=0, separators=(',', ':')).replace('\n','')+ '\n'
    
        logstash_socket.send(payload)
        #Uncomment to see what was sent.
        #print payload
        payload=''
    
    logstash_socket.close()
    sys.exit(0)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
