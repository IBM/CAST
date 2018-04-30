#!/bin/bash
# encoding: utf-8
#================================================================================
#
#    data_collection.sh
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

BIG_DATA_STORE_IP="CHANGE THIS IP"
BIG_DATA_STORE_PORT="CHANGE THIS PORT"
HOSTS_FILE='zimon_hosts'

LOG_WRITE_LOCATION="/dev/tcp/${BIG_DATA_STORE_IP}/${BIG_DATA_STORE_PORT}"
METRIC_CSV="cpu_system,cpu_user,mem_active,gpfs_ns_bytes_read,gpfs_ns_bytes_written,gpfs_ns_tot_queue_wait_rd,gpfs_ns_tot_queue_wait_wr"

>/tmp/zimon_perf

for zimon_host in `cat ${HOSTS_FILE}`
do
    echo "get -cr metrics ${METRIC_CSV} from ${zimon_host} last 10 bucket_size 1" | /opt/IBM/zimon/zc 127.0.0.1 | grep -v "null" | grep -v "^Timestamp" | grep -v "^\." | sed s/^/${zimon_host},/  >> /tmp/zimon_perf
done

cat /tmp/zimon_perf > $LOG_WRITE_LOCATION
rm /tmp/zimon_perf

