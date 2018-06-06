#!/bin/python
# encoding: utf-8
# ================================================================================
#
#    zimon_collector.py
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

import configparser
import sys

#from elasticsearch.serializer import JSONSerializer



def main(args):
    
    # TODO optional config
    config = configparser.ConfigParser()
    config.read('zimon_collector.ini')

    # EARLY RETURN
    # If zimon wasn't configured exit 
    if not 'zimon' in config:
        return 1 

    zimon = config['zimon']
    collector_host = zimon['host']
    collector_port = zimon['port']
    sensor_list    = zimon['sensors'].strip().replace("\n",",")
    sensor_hosts   = zimon['hosts'].strip().split("\n")
    bucket_size    = zimon['bucket_size']
    record_count   = zimon['record_count']
    collector_node = zimon['collector_node']


    for host in sensor_hosts:
        print host
    
    



if __name__ == "__main__":
    sys.exit(main(sys.argv))
