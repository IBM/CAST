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

from urllib import urlencode
import urllib2

''' The default payload for requests.'''
POST_PAYLOAD= {
    "attributes"    : [ "Infiniband_PckIn","Infiniband_PckOut", 
                        "Infiniband_MBIn","Infiniband_MBOut" ],
    "functions"     : [ "RAW" ],
    "scope_object"  : "Site",
    "interval"      : 2,
    "monitor_object": "Device",
    "objects"       : ["Grid.default"]
}

''' The snapshot uri.'''
SNAPSHOT_URL = 'http://%s/ufmRest/monitoring/snapshot'



def main(args):

    # TODO This section should be converted to Commandline input
    HOST= '10.7.4.41'
    PORT= 10522
    URL='http://10.7.0.41/ufmRest/monitoring/snapshot'
    username="admin"
    password="123456"
    
    # Build the payload
    #post_payload={}
    #post_payload["attributes"]      = ["Infiniband_PckIn","Infiniband_PckOut","Infiniband_MBIn","Infiniband_MBOut"]
    #post_payload["functions"]       = [ "RAW" ]
    #post_payload["scope_object"]    = "Site"
    #post_payload["interval"]        = 2
    #post_payload["monitor_object"] = "Device"
    #post_payload["objects"]         = ["Grid.default"]
    json_payload=json.dumps(POST_PAYLOAD)
    json_len=len(json_payload)
    
    # Build the request
    request  = urllib2.Request(URL,json_payload, 
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
        logstash = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        logstash.connect((HOST,PORT))
    except socket.error, msg:
        sys.exit(1)
    
    payload=''
    # Iterate over the data and build a payload.
    for timestamp in data:
        devices = data[timestamp]["Device"]
    
        for device in devices:
            devices[device]['type']      = 'ufm-counters'
            devices[device]['timestamp'] = timestamp
            devices[device]['source']    = device
            payload += json.dumps(devices[device], indent=0, separators=(',', ':')).replace('\n','')+ '\n'
    
        logstash.send(payload)
        
        #Uncomment to see what was sent.
        #print payload
        payload=''
    
    logstash.close()
    sys.exit(0)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
