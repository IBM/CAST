#!/bin/python
# encoding: utf-8
#================================================================================
#
#   ufm_config.py
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

import sys
import os
from shutil import copyfile
import configparser

CONFIG_FILE='''/opt/ufm/files/conf/gv.cfg'''

def main(args):
    gv_conf = configparser.ConfigParser(allow_no_value = True)

    config_file = args[1] if len(args) > 1 else CONFIG_FILE
    
    if config_file[0] != "/":
        config_file = os.getcwd() + "/" + config_file

    try:
        gv_conf.read(config_file)
        copyfile( config_file, config_file + ".orig" )
        cfg_handle = open(config_file,'w')
    except:
        print ( "Exiting, unable to read {0}".format(config_file) )
        return 1

    # Set the defaults up.
    gv_conf.set("Server","suppress_events_level]","enable_all_events")

    gv_conf.set("Logging","level","INFO")
    gv_conf.set("Logging","syslog","true")
    gv_conf.set("Logging","event_syslog","true")
    gv_conf.set("Logging","syslog_level","INFO")

    gv_conf.set("CSV","write_interval","30")
    gv_conf.set("CSV","ext_ports_only","yes")
    gv_conf.set("CSV","max_files","10")

    gv_conf.set("MonitoringHistory","history_configured","true")
    
    gv_conf.write(cfg_handle)
    cfg_handle.close()

if __name__ == "__main__":
    sys.exit(main(sys.argv))
