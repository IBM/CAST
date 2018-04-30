#!/usr/bin/python
#================================================================================
#
#    csmrestd/tests/py_one.py
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



connection = httplib.HTTPConnection(host, port)
#connection.set_debuglevel(1)
connection.connect()


connection.request('POST', loopbackUrl, json.dumps({
    "msg_id": "test.testcat01.test01",
    "time_stamp": "boogerstamp",
    "location": "fsgb001",
    "raw_data":"raw data",
    "kvcsv": "k1=v1,k2=v2"
    }))
result = connection.getresponse().read();








