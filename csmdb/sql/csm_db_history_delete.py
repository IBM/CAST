#!/bin/python
# encoding: utf-8
#================================================================================
#   
#    csm_db_history_delete.py
# 
#    © Copyright IBM Corporation 2015-2019. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
# 
#================================================================================

#================================================================================
# usage             ./csm_db_history_delete.py
# current_version   2.0
# date_created:     12-07-2018
# date_modified:    02-14-2019
#================================================================================

import psycopg2
import argparse
import json
import sys
import os
import commands
import logging.config
import logging.handlers as handlers
import time
from logging.handlers import RotatingFileHandler
import datetime
from datetime import date
import threading
from multiprocessing.dummy import Pool as ThreadPool

DEFAULT_LOG='''/var/log/ibm/csm/db/csm_db_history_delete.log'''
DEFAULT_USER="postgres"
DEFAULT_THREAD_POOL=10

# Additional Formatting style
line1 = "---------------------------------------------------------------------------------------------------------"

# username defined
username = commands.getoutput("whoami")

if not os.geteuid() == 0:
    print "{0}".format(line1)
    print "[INFO] Only root can run this script"
    sys.exit("{0}".format(line1))

logger = logging.getLogger()
logger.setLevel(logging.DEBUG)

# Add a rotating handler
handler = logging.handlers.RotatingFileHandler(DEFAULT_LOG,mode='w',maxBytes=1000000,backupCount=365)

# Create and configure logger
LOG_FORMAT = logging.Formatter('%(asctime)s (%(name)s) [%(levelname)s]:\t%(message)s')

# Add handler and formatter.
handler.setFormatter(LOG_FORMAT)
logger.addHandler(handler)

# Test the logger create start time and also check the arguments
a = datetime.datetime.now()
fullCmdArguments = sys.argv
argumentList = fullCmdArguments[1:]

if '-n' in argumentList and '-d' in argumentList or \
    '--count' in argumentList and '--database' in argumentList or \
    '-n' in argumentList and '--database' in argumentList or \
    '--count' in argumentList and '-d' in argumentList:
    print "{0}".format(line1)
    print "Welcome to the CSM DB deletion of history table records script"
    print "{0}".format(line1)
    print "Start Script Time:                                    | {0}".format(a)
    print "{0}".format(line1)
    print "Deletion Log Directory:                               | {0}".format(DEFAULT_LOG)
    logger.info("Welcome to the CSM DB deletion of history table records script")
    logger.info("{0}".format(line1))
    logging.info("Start Script Time:                 | {0}".format(a))
    logger.info("{0}".format(line1))
    logging.info("Deletion Log Directory:            | {0}".format(DEFAULT_LOG))
else:
    print "{0}".format(line1) 

TABLES=[ "csm_allocation_history", "csm_allocation_node_history", "csm_allocation_state_history", 
    "csm_config_history", "csm_db_schema_version_history", "csm_diag_result_history", 
    "csm_diag_run_history", "csm_dimm_history", "csm_gpu_history", "csm_hca_history", 
    "csm_ib_cable_history", "csm_lv_history", "csm_lv_update_history", "csm_node_history", 
    "csm_node_state_history", "csm_processor_socket_history", "csm_ssd_history", 
    "csm_ssd_wear_history", "csm_step_history", "csm_step_node_history", "csm_switch_history",
    "csm_switch_inventory_history", "csm_vg_history", "csm_vg_ssd_history"]

RAS_TABLES=["csm_ras_event_action"]

def dump_table( db, user, table_name, count, is_ras=False ):
    try:
        db_conn= psycopg2.connect("dbname='{0}' user='{1}' host='localhost'".format(db, user))
    except:
        print "[CRITICAL] Unable to connect to local database."
        logger.info("Unable to connect to local database.")
        return

    time= "master_time_stamp" if is_ras else "history_time"

    cursor=db_conn.cursor()
    
    ## Delete records from history table and total count.
    sql = "WITH delete_1 AS \
    ( DELETE FROM {0} WHERE {1} < (NOW() - INTERVAL '{2} MIN') AND archive_history_time IS NOT NULL RETURNING *) \
    select count(*) from delete_1".format(table_name, time, count)
    number_of_rows = cursor.execute(sql)
    (result,) = cursor.fetchall()

    # Gather the script time for calculation
    b = datetime.datetime.now()
    delta = b - a
    
    # Gather the results for printing to screen and logging
    print "[INFO] Processing Table {0:<29} | User Ct (time(mins)): {1:<10} | Act DB Ct: {2:<10}".format(table_name, count, result[0])
    logger.info("Tbl: {0:<29} | User Ct: {1:<10} | Act DB Ct: {2:<10}".format(table_name, count, result[0]))
    
    db_conn.commit()
    db_conn.close()

def main(args):

    # Parse the args.
    parser = argparse.ArgumentParser(
        description="------------------------------------------------------------------------------\n"
                    "A tool for deleting the CSM Database history table records.\n"
                    "------------------------------------------------------------------------------\n"
                    "LogDir:/var/log/ibm/csm/db/csm_db_history_delete.log\n"
                    "------------------------------------------------------------------------------\n",
        add_help=True,
        epilog="------------------------------------------------------------------------------")
   
    parser.add_argument( '-n', '--count', metavar='count', dest="count", required=True,
        help="The time (in mins.) of oldest records which to delete. required argument")
    parser.add_argument( '-d', '--database', metavar='db', dest="db", required=True,
        help="Database name to delete history records from. required argument")
    parser.add_argument( '-u', '--user', metavar='user', dest="user", default=DEFAULT_USER,
        help="The database user. Default: {0}".format(DEFAULT_USER))
    parser.add_argument( '--threads', metavar='threads', dest="threads", default=DEFAULT_THREAD_POOL,
        help="The number of threads for the thread pool. Default: {0}".format(DEFAULT_THREAD_POOL))

    # Input parsing
    args = parser.parse_args()

    # Process the script detail info. for screen and logging.
    
    logger.info("{0}".format(line1))
    logging.info("DB Name:                           | {0}".format(args.db))
    logging.info("DB User Name:                      | {0}".format(args.user))
    logging.info("Script User Name:                  | {0}".format(username))
    logging.info("Thread Count:                      | {0}".format(args.threads))
    logger.info("{0}".format(line1))
    print "{0}".format(line1)
    print "DB Name:                                              | {0}".format(args.db)
    print "DB User Name:                                         | {0}".format(args.user)
    print "Script User Name:                                     | {0}".format(username)
    print "Thread Count:                                         | {0}".format(args.threads)
    print "{0}".format(line1)
    
    pool = ThreadPool(int(args.threads))

    pool.map( lambda table: dump_table( args.db, args.user, table, args.count), TABLES)

    for table in RAS_TABLES:
        dump_table( args.db, args.user, table, args.count, True)

    # Process the finishing info. for screen and logging.
    ft = datetime.datetime.now()
    delta2 = ft - a
    
    logger.info("{0}".format(line1))
    logging.info("End Script Time:                   | {0}".format(ft))
    logging.info("Total Process Time:                | {0}".format(delta2))
    logger.info("{0}".format(line1))
    logging.info("Finish CSM DB deletion script process")
    logger.info("{0}".format(line1))
    print "{0}".format(line1)
    print "End Script Time:                                      | {0}".format(ft)
    print "Total Process Time:                                   | {0}".format(delta2)
    print "{0}".format(line1)
    print "Finish CSM DB deletion script process"
    print "{0}".format(line1)

if __name__ == "__main__":
    sys.exit(main(sys.argv))
