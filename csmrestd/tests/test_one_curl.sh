#!/bin/bash
#================================================================================
#
#    csmrestd/tests/test_one_curl.sh
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



jd="{\"alertName\":\"messages\/messages.log\/0\",\"alertType\":\"KEYWORD_MATCH\",\"description\":\"messages\",\"severity\":3,\"recordId\":\"1456390296330_1456390296329_44_4\",\"datasources\":[\"messages.log\"],\"alertLogRecordAnnotations\":{\"severity\":[\"O\"],\"threadID\":[\"000002cf\"],\"_writetime\":[1456390296329],\"_datasource\":[\"messages.log\"],\"datasourceHostname\":[\"scaserver12.in.ibm.com\"],\"logRecord\":[\"[2\/25\/16 3:51:18:733 EST] 000002cf SystemOut                                                    O 02\/25\/16 03:51:18:733 EST [Thread-140] ERROR com.ibm.tivoli.unity.common.logging.UnityLogger - IndexStatusChecker :\"],\"shortname\":[\"SystemOut\"],\"timestamp\":[1456390278733]},\"alertLogRecord\":\"[2\/25\/16 3:51:18:733 EST] 000002cf SystemOut                                                    O 02\/25\/16 03:51:18:733 EST [Thread-140] ERROR com.ibm.tivoli.unity.common.logging.UnityLogger - IndexStatusChecker :\",\"timestamp\":1456390278733,\"alertDetails\":{\"searchQuery\":\"error\"}}"

#echo $jd

#re="http://([^/]+)/"
#if [[ $jd =~ $re ]]; then echo ${BASH_REMATCH[1]}; fi



# take curl data from inline stdin
curl --data-binary  @-   http://127.0.0.1:5555/csmi/V1.0/loopback/test  <<EOF
msg_id: my.message.id
timestamp: $d
location_name: my-location-name
kvcsv=k1=v1,k2=v2
EOF








