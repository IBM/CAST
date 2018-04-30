#!/usr/bin/python
#================================================================================
#
#    csmrestd/tests/py_thruput.py
#
#  © Copyright IBM Corporation 2015,2016. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================


#
# simple rest api, thruput test..
# First with http, then with https...

import json;
import httplib;
import socket;
import time

host='127.0.0.1';
port=5555
loopbackUrl='/csmi/V1.0/loopback/test'

#socket.getaddrinfo(host, port, 0, socket.SOCK_STREAM)
#exit


connection = httplib.HTTPConnection(host, port)
#connection.set_debuglevel(1)
connection.connect()


starttime=time.time()
timeout = starttime + 10   # 10 seconds from now

n = 0;
while True:
    if time.time() > timeout:
        break;
    connection.request('POST', loopbackUrl, json.dumps({
    	"msg_id": "test.testcat01.test01",
    	"time_stamp": "boogerstamp",
    	"location": "fsgb001",
    	"raw_data":"raw data"
# Removed due to symtax error:
#      'kvcsv' => 'k1=v1,k2=v2'
    	}))
    result = connection.getresponse().read();
    n += 1

endtime=time.time()
deltatime = endtime-starttime
print "num_posts = %d, %d/s\n" % (n,n/deltatime)






