#!/bin/python
# encoding: utf-8
#================================================================================
#
#    ufmCollection.py
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

#------------------------------PROGRAM INFORMATION-------------------------------------#
#                                                                                      #
# ufmCollection.py -- a script to collect "counter" data from UFM and store           #
# it in the BDS.                                                                       #
#                                                                                      #
# Authors/Contact:                                                                     #
# - John Dunham - Email: jdunham@us.ibm.com                                            #
# - Nick Buonarota - Email: nbuonar@us.ibm.com                                         #
#                                                                                      #
# Purpose: Simple script that is packaged with BDS. Can be run individually and        #
# independantly when ever called upon.                                                 #
#                                                                                      #
# Usage:                                                                               #
# - Run the program.                                                                   #
#   - pass in parameters.                                                              #
#      - REQUIRED [--ufm] : This tells program where UFM is (an IP address)            #
#      - REQUIRED [--logstash] : This tells program where logstash is (an IP address)  #
#      - OPTIONAL [--logstash-port] : This specifies the port for logstash             #
#      - OPTIONAL [--ufm_restAPI_args-attributes] : attributes for ufm restAPI         #
#        - CSV                                                                         #
#          Example:                                                                    #
#            - Value1                                                                  #
#            - Value1,Value2                                                           #
#      - OPTIONAL [--ufm_restAPI_args-functions] : functions for ufm restAPI           #
#        - CSV                                                                         #
#      - OPTIONAL [--ufm_restAPI_args-scope_object] : scope_object for ufm restAPI     #
#        - single string                                                               #
#      - OPTIONAL [--ufm_restAPI_args-interval] : interval for ufm restAPI             #
#        - int                                                                         #
#      - OPTIONAL [--ufm_restAPI_args-monitor_object] : monitor_object for ufm restAPI #
#        - single string                                                               #
#      - OPTIONAL [--ufm_restAPI_args-objects] : objects for ufm restAPI               #
#        - CSV                                                                         #
#      FOR ALL ufm_restAPI related arguments:                                          #
#        - see ufm restAPI for documentation                                           #
#        - json format                                                                 #
#        - program provides default value if no user provides                          #
# - Data is collected from UFM restAPI, then copied into BDS.                          #
# - When program is finished, a record should appear in the BDS.                       #
#                                                                                      #
#--------------------------------------------------------------------------------------#

import socket
import json
import sys
import getopt

from urllib import urlencode
import urllib2

# Defaults for ufm restAPI - overridden by user options if provided.
''' The default payload for requests.'''
POST_PAYLOAD= {
    "attributes"    : [ 
        "Infiniband_MBOut", 
        "Infiniband_MBOutRate", 
        "Infiniband_MBIn",
        "Infiniband_MBInRate", 
        "Infiniband_PckOut",
        "Infiniband_PckOutRate",
        "Infiniband_PckIn",
        "Infiniband_PckInRate",
        "Infiniband_RcvErrors",
        "Infiniband_XmtDiscards",
        "Infiniband_SymbolErrors",
        "Infiniband_LinkRecovers",
        "Infiniband_LinkDowned",
        "Infiniband_LinkIntegrityErrors",
        "Infiniband_RcvRemotePhysErrors",
        "Infiniband_XmtConstraintErrors",
        "Infiniband_RcvConstraintErrors",
        "Infiniband_ExcBufOverrunErrors",
        "Infiniband_RcvSwRelayErrors",
        "Infiniband_VL15Dropped",
        "Infiniband_XmitWait",
        "Infiniband_CumulativeErrors",
        "Infiniband_CBW",
        "Infiniband_Normalized_MBOut",
        "Infiniband_Normalized_CBW",
        "Infiniband_NormalizedXW" ],
    "functions"     : [ "RAW" ],
    "scope_object"  : "Site",
    "interval"      : 2,
    "monitor_object": "Port",
    "objects"       : ["Grid.default"]
}

''' The snapshot uri.'''
SNAPSHOT_URL = 'http://%s/ufmRest/monitoring/snapshot'

SHORT_OPTS="h"
LONG_OPTS=[
"help",
"ufm=", 
"logstash=", 
"logstash-port=", 
"ufm_restAPI_args-attributes=",
"ufm_restAPI_args-functions=",
"ufm_restAPI_args-scope_object=",
"ufm_restAPI_args-interval=",
"ufm_restAPI_args-monitor_object=",
"ufm_restAPI_args-objects="
]

USAGE = '''
 Purpose: Simple script that is packaged with BDS. Can be run individually and        
 independantly when ever called upon.                                                 
                                                                                      
 Usage:                                                                               
 - Run the program.                                                                   
   - pass in parameters.                                                              
      - REQUIRED [--ufm] : This tells program where UFM is (an IP address)            
      - REQUIRED [--logstash] : This tells program where logstash is (an IP address)  
      - OPTIONAL [--logstash-port] : This specifies the port for logstash             
      - OPTIONAL [--ufm_restAPI_args-attributes] : attributes for ufm restAPI         
        - CSV                                                                         
          Example:                                                                    
            - Value1                                                                  
            - Value1,Value2                                                           
      - OPTIONAL [--ufm_restAPI_args-functions] : functions for ufm restAPI           
        - CSV                                                                         
      - OPTIONAL [--ufm_restAPI_args-scope_object] : scope_object for ufm restAPI     
        - single string                                                               
      - OPTIONAL [--ufm_restAPI_args-interval] : interval for ufm restAPI             
        - int                                                                         
      - OPTIONAL [--ufm_restAPI_args-monitor_object] : monitor_object for ufm restAPI 
        - single string                                                               
      - OPTIONAL [--ufm_restAPI_args-objects] : objects for ufm restAPI               
        - CSV                                                                         
      FOR ALL ufm_restAPI related arguments:                                          
        - see ufm restAPI for documentation                                           
        - json format                                                                 
        - program provides default value if no user provides                          
'''

def main(args):
    
    # Variables to hold the user options
    ufm_url=None
    logstash=None
    logstash_port=10522

    # Load the options.
    try:
        opts, optargs = getopt.getopt(args[1:], SHORT_OPTS, LONG_OPTS)
    except getopt.GetoptError as err:
        print("Invalid option detected: %s", err)
        print(USAGE)
        return 1
    
    for o,a in opts:
        if o in ("-h", "--help"):
            print(USAGE)
            return 1
        elif o in ("--ufm"):
            ufm_url= SNAPSHOT_URL % a
        elif o in ("--logstash"):
            logstash=a
        elif o in ("--logstash-port"):
            logstash_port=a
        elif o in ("--ufm_restAPI_args-attributes"):
            POST_PAYLOAD["attributes"]=a.split(',')
        elif o in ("--ufm_restAPI_args-functions"):
            POST_PAYLOAD["functions"]=a.split(',')
        elif o in ("--ufm_restAPI_args-scope_object"):
            POST_PAYLOAD["scope_object"]=a
        elif o in ("--ufm_restAPI_args-interval"):
            POST_PAYLOAD["interval"]=a
        elif o in ("--ufm_restAPI_args-monitor_object"):
            POST_PAYLOAD["monitor_object"]=a
        elif o in ("--ufm_restAPI_args-objects"):
            POST_PAYLOAD["objects"]=a.split(',')
    
    if ( ufm_url is None or logstash is None ):
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
    except socket.error as msg:
        sys.exit(1)
    
    payload=''
    # Iterate over the data and build a payload.
    for timestamp in data:
        sources = data[timestamp]["Port"]
    
        for source in sources:
            sources[source]['type']      = 'counters-ufm'
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
