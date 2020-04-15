#!/usr/bin/python2
# encoding: utf-8
#================================================================================
#
#    findJobKeys.py
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

# TODO Implement a configuration file to better facilitate simple changes.

import argparse
import sys
import os
import re

ELASTIC_ENV='CAST_ELASTIC'

# File Constants

# Elastic list for logstash configuration file (logstash.conf)
ELASTIC_CLUSTER_LIST="_ELASTIC_IP_PORT_LIST_"

# filebeats LS:10523
LOGSTASH_CLUSTER_LIST = '_LOGSTASH_IP_PORT_LIST_'

#<kibana_host>:5601
KIBANA_HOST='_KIBANA_HOST_PORT_'

def fileReplace(source_file, target_file, key_dict):
    '''Replaces any matches in the key dictionary in the source file and writes it to the target file.'''
    try: 
        with open(source_file, 'r') as file :
          filedata = file.read()
        
        pattern = re.compile(r'\b(' + '|'.join(key_dict.keys()) + r')\b')
        result = pattern.sub(lambda x: key_dict[x.group()], filedata)
        
        with open(target_file, 'w') as file :
            file.write(result)
    except:
        return


def buildHostList(hosts, port, default=None):
    '''Builds a listing combing the hosts and port. If no host name is provided the default string is returned'''
    if hosts is not None:
        return ",".join(map( lambda h : '"{0}:{1}"'.format(h, port), hosts))
    else:
        return default

def main(args):
    
    parser = argparse.ArgumentParser(
        description='''A tool for quickly configuring the default CAST configuration files for your
            environment.''')

    
    parser.add_argument( '-e', '--elasticsearch', metavar='HOST', dest='elastic_hosts', nargs='*', 
        help='A list of elastic search hostnames, the list is concatenated with the port and stored in "CAST_ELASTIC"')
    parser.add_argument( '-p', '--elasticport', metavar='PORT', dest='elastic_port', default=9200,
        help='The elastic search port for API interactions (Default: 9200)')
    parser.add_argument( '-k', '--kibana', metavar='HOST', dest='kibana_host', 
        help='A kibana hostname, stored in "CAST_KIBANA"')
    parser.add_argument( '-P', '--kibanaport', metavar='PORT', dest='kibana_port', default=5601, 
        help='The port to access kibana on (Default: 5601)')
    parser.add_argument( '-l', '--logstash', metavar='HOST', dest='logstash_hosts', nargs='*', 
        help='A list of logstash hostnames in the cluster to be used in configuration')
    parser.add_argument( '--rsyslog', action='store_true', 
        help='Configures rsyslog with the first entry in the logstash_hosts list.')

    args = parser.parse_args()

    key_dict={}

    # The elastic list and kibana list are set in stone.
    key_dict[ELASTIC_CLUSTER_LIST] = buildHostList(args.elastic_hosts, args.elastic_port, ELASTIC_CLUSTER_LIST)
    key_dict[KIBANA_HOST] = buildHostList([args.kibana_host], args.kibana_port, KIBANA_HOST)
    
    # Set environment variable. 
    os.environ[ELASTIC_ENV] = key_dict[ELASTIC_CLUSTER_LIST]

    # Kibana
    # TODO Kibana configs haven't been written.

    # Logstash
    fileReplace( 
        "/opt/ibm/csm/bigdata/logstash/config/logstash.conf",
        "/etc/logstash/conf.d/logstash.conf",
        key_dict )


    # Filebeats
    if args.logstash_hosts is not None:
        key_dict[LOGSTASH_CLUSTER_LIST] = buildHostList(args.logstash_hosts[0:1], "10523", LOGSTASH_CLUSTER_LIST)
        fileReplace( 
            "/opt/ibm/csm/bigdata/beats/config/filebeat.yml",
            "/etc/filebeat/filebeat.yml",
            key_dict )
    
    # Elastic

    #fileReplace(
    #    "/opt/ibm/csm/bigdata/elasticsearch/elasticsearch.yml",
    #    "/etc/elasticsearch/elasticsearch.yml",
    #    key_dict )

    # Rsyslog
    if args.rsyslog and args.logstash_hosts is not None:
        with open('/etc/rsyslog.conf','a') as file:
            syslog_str='''$template logFormat, "%TIMESTAMP:::date-rfc3339% %HOSTNAME% %APP-NAME% %PROCID% %syslogseverity-text% %msg%\n"'
*.*;cron.none @@{0}:10515;logFormat'''.format(args.logstash_hosts[0])
            file.write(yslog_str)


if __name__ == '__main__':
    sys.exit(main(sys.argv))
