#!/bin/python
# encoding: utf-8
#================================================================================
#   
#    csmTransactionRebuild.py
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

import psycopg2
import argparse
import sys
import os
import json
import traceback

DEFAULT_DATABASE="csmdb"
DEFAULT_USER="postgres"
DEFAULT_OUTPUT="csm-transaction.log"

def dump_allocations( db, user, output ):
    try:
        db_conn= psycopg2.connect("dbname='{0}' user='{1}' host='localhost'".format(db, user))
    except:
        print "[CRITICAL] Unable to connect to local database."
        logger.info("Unable to connect to local database.")
        return

    # Generic Allocation fields.
    allocQueryFields='''a.allocation_id, a.primary_job_id, a.secondary_job_id,
a.ssd_file_system_name, a.launch_node_name, a.user_flags,
a.system_flags, a.ssd_min, a.ssd_max, a.num_nodes,
a.num_processors, a.num_gpus, a.projected_memory,
a.state, a.type, a.job_type,
a.user_name, a.user_id, a.user_group_id, 
a.user_script, a.begin_time, a.account,
a.comment,  a.job_name,
a.job_submit_time,
a.queue, a.requeue, a.time_limit,
a.wc_key, a.isolated_cores, a.smt_mode '''

    # Generic step fields.
    stepQueryFields='''s.step_id, s.allocation_id, s.argument,
s.begin_time, s.num_gpus, s.projected_memory, s.num_nodes, 
s.num_processors, s.num_tasks, s.status, 
s.executable, s.working_directory, s.environment_variable,
s.projected_memory '''

    # fields to pull from step history. 
    stepQueryHistoryFields=''' s.end_time, s.exit_status, s.error_message, 
s.io_stats, s.max_memory, s.memory_stats,
s.total_s_time, s.total_u_time 
'''

    cursor=db_conn.cursor()
    try:
        with open(output, 'w') as file:

            # Get running states.
            cursor.execute('''SELECT history_time,allocation_id, state 
FROM csm_allocation_state_history 
WHERE state='running' OR state='to-running' ''')
            allocStates={}
            for row in cursor:
                if row[1] not in allocStates:
                    allocStates[row[1]] = {}

                if row[2] == "to-running":
                    allocStates[row[1]]["running-start-timestamp"]=row[0]
                else:
                    allocStates[row[1]]["running-end-timestamp"]=row[0] 
            
            for allocation in allocStates:
                data_wrapped={
                    "type"      : "allocation",
                    "uid"       : allocation,
                    "data"      : allocStates[allocation]
                }
                file.write('{0}\n'.format(json.dumps(data_wrapped, default=str)))

            # =================================================================================
            # Dump the active allocaitons.
            query='''SELECT {0},  array_agg(an.node_name) as compute_nodes 
FROM csm_allocation a
JOIN csm_allocation_node an 
ON  a.allocation_id = an.allocation_id 
GROUP BY {0}
'''.format(allocQueryFields)  

            cursor.execute(query)
            colnames = [desc[0] for desc in cursor.description]
            for row in cursor:
                data=dict(zip(colnames, row))
                allocId=data["allocation_id"]
                if allocId not in allocStates and data["state"] is "running":
                    data["running-start-timestamp"]=data["begin_time"]

                data_wrapped={
                    "timestamp" : data["job_submit_time"],
                    "type"      : "allocation",
                    "uid"       : data["allocation_id"],
                    "data"      : data
                }
                file.write('{0}\n'.format(json.dumps(data_wrapped, default=str)))

            #====================================================================================
            # Dump the history allocations.
            query='''SELECT {0}, a.end_time,  array_agg(an.node_name) as compute_nodes 
FROM csm_allocation_history a
JOIN csm_allocation_node_history an 
ON  a.allocation_id = an.allocation_id 
GROUP BY {0}, a.end_time
'''.format(allocQueryFields)  

            cursor.execute(query)
            newColnames = [desc[0] for desc in cursor.description]
            historyColnames = [ name for name in newColnames if name not in set(colnames) ]
            for row in cursor:
                data=dict(zip(newColnames, row))
                data["history"]={}
                # Shift history
                for col in historyColnames:
                    data["history"][col]=data.pop(col, None)

                # Verify meta state.
                allocId=data["allocation_id"]
                if allocId in allocStates and "running-start-timestamp" not in allocStates[allocId]:
                    data["running-start-timestamp"]=data["begin_time"]

                data_wrapped={
                    "timestamp" : data["job_submit_time"],
                    "type"      : "allocation",
                    "uid"       : data["allocation_id"],
                    "data"      : data
                }
                file.write('{0}\n'.format(json.dumps(data_wrapped, default=str)))

            #====================================================================================
            # Dump the step transaction.
            query='''SELECT {0}, array_agg(sn.node_name) as compute_nodes 
FROM csm_step s
JOIN csm_step_node sn
ON  s.allocation_id = sn.allocation_id AND s.step_id = sn.step_id
GROUP BY {0}
'''.format(stepQueryFields)

            cursor.execute(query)
            colnames = [desc[0] for desc in cursor.description]
            for row in cursor:
                data=dict(zip(colnames, row))
                data_wrapped={
                    "timestamp" : data["begin_time"],
                    "type"      : "allocation-step",
                    "uid"       : "{0}-{1}".format(data["allocation_id"],data["step_id"]),
                    "data"      : data
                }
                file.write('{0}\n'.format(json.dumps(data_wrapped, default=str)))

            #====================================================================================
            # Dump the step history  transaction.
            query='''SELECT {0}, {1}, array_agg(sn.node_name) as compute_nodes 
FROM csm_step_history s
JOIN csm_step_node_history sn
ON  s.allocation_id = sn.allocation_id AND s.step_id = sn.step_id
GROUP BY {0}, {1}
'''.format(stepQueryFields, stepQueryHistoryFields)

            cursor.execute(query)
            newColnames = [desc[0] for desc in cursor.description]
            historyColnames = [ name for name in newColnames if name not in set(colnames) ]
            for row in cursor:
                data=dict(zip(newColnames, row))
                # Shift history
                data["history"]={}
                for col in historyColnames:
                    data["history"][col]=data.pop(col, None)

                data_wrapped={
                    "timestamp" : data["begin_time"],
                    "type"      : "allocation-step",
                    "uid"       : "{0}-{1}".format(data["allocation_id"],data["step_id"]),
                    "data"      : data
                }
                file.write('{0}\n'.format(json.dumps(data_wrapped, default=str)))

    
    except Exception as e:
        print "[INFO] Exception caught: {0}".format(e)
        traceback.print_exc()
    
    db_conn.commit()
    db_conn.close()

def main(args):

    # Parse the args.
    parser = argparse.ArgumentParser(
        description='A tool for regenerating the csm transactional logs from the database.')
   
    parser.add_argument( '-d', '--database', metavar='db', dest="db", default=DEFAULT_DATABASE,
        help="Database to archive tables from. Default: {0}".format(DEFAULT_DATABASE))
    parser.add_argument( '-u', '--user', metavar='user', dest="user", default=DEFAULT_USER,
        help="The database user. Default: {0}".format(DEFAULT_USER))
    parser.add_argument( '-o', '--output', metavar='output', dest="output", default=DEFAULT_OUTPUT,
        help="The output file, overwrites existing file. Default: {0}".format(DEFAULT_OUTPUT) )

    # Input parsing
    args = parser.parse_args()

    # Verifies path exists.
    dump_allocations( args.db, args.user, args.output )
    
if __name__ == "__main__":
    sys.exit(main(sys.argv))
