Using csm_db_history_delete.py
==============================

This section describes the deletion process associated with the CSM Database 
:ref:`history table <history_tables>` records. If run alone it will delete all history tables 
including the :ref:`csm_event_action table <csm_ras_event_action>`,  which contain a non-null
archive history timestamp.

.. note:: This script is designed to run as a `root` user. If you try to run as a postgres user the script will prompt a message and exit.

.. code-block:: bash

 -bash-4.2$ ./csm_db_history_delete.py -h
 ---------------------------------------------------------------------------------------------------------
 [INFO] Only root can run this script
 ---------------------------------------------------------------------------------------------------------

Usage Overview
--------------

The ``csm_db_history_delete.py`` script will accept certain flags:
 #. Interval time (in minutes) - required (tunable time interval for managing table record deletions)
 #. Database name              - required
 #. DB user_name               - optional
 #. Thread Count               - optional

.. code-block:: bash

 /opt/ibm/csm/db/csm_db_history_delete.py –h
 /opt/ibm/csm/db/csm_db_history_delete.py --help

+------------------------------------+--------------------------------------------+-------------------------------------------+
|               Options              |                 Description                |                   Result                  |
+====================================+============================================+===========================================+
| running the script with no options | ./csm_db_history_delete.py                 | Will prompt a message explaing that the   |
|                                    |                                            | -n/--count and or -d/--database           |
|                                    |                                            | is required                               |
+------------------------------------+--------------------------------------------+-------------------------------------------+
| running the script with            | ./csm_db_history_delete.py –n, --count     | specifies the time (in mins.) of oldest   |
|                                    |                                            | records which to delete.                  |
| -n, --count                        |                                            | (required)                                |
+------------------------------------+--------------------------------------------+-------------------------------------------+
| running the script with            | ./csm_db_history_delete.py -d, --database  | specifies the database name               |
| –d, --database                     |                                            | (required)                                |
+------------------------------------+--------------------------------------------+-------------------------------------------+
| running the script with            | ./csm_db_history_delete.py –u, --user      | specifies the database user name.         |
| –u, --user                         |                                            | (optional)                                |
+------------------------------------+--------------------------------------------+-------------------------------------------+
| running the script with            | ./csm_db_history_delete.py –-threads       | specifies threads.                        |
| –-threads                          |                                            | (optional)                                |
+------------------------------------+--------------------------------------------+-------------------------------------------+
| running the script with            | ./csm_db_history_deletee.py –h, --help     | see details below                         |
| –h, --help                         |                                            |                                           |
+------------------------------------+--------------------------------------------+-------------------------------------------+

Example (usage)
^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ /opt/ibm/csm/db/csm_db_history_delete.py –h
 ---------------------------------------------------------------------------------------------------------
 usage: csm_db_history_delete.py [-h] -n count -d db [-u user]
                                 [--threads threads]
 
 A tool for deleting the CSM Database history table records.
 
 optional arguments:
   -h, --help            show this help message and exit
   -n count, --count count
                         The time (in mins.) of oldest records which to delete.
                         required argument
   -d db, --database db  Database name to delete history records from. required
                         argument
   -u user, --user user  The database user. Default: postgres
   --threads threads     The number of threads for the thread pool. Default: 10
 
 ------------------------------------------------------------------------------

.. note:: This is a general overview of the CSM DB deletion process using the ``csm_db_history_delete.py`` script.

Script out results
------------------

.. code-block:: bash

 [root@c650mnp02 db]# /opt/ibm/csm/db/csm_db_history_delete.py -d csmdb -n 2880
 ---------------------------------------------------------------------------------------------------------
 Welcome to the CSM DB deletion of history table records script
 ---------------------------------------------------------------------------------------------------------
 Start Script Time:                 | 2018-12-10 11:56:13.395135
 ---------------------------------------------------------------------------------------------------------
 [INFO] Processing Table csm_allocation_state_history  | User Ct (time(mins)): 2880       | Act DB Ct: 0
 [INFO] Processing Table csm_config_history            | User Ct (time(mins)): 2880       | Act DB Ct: 0
 [INFO] Processing Table csm_allocation_history        | User Ct (time(mins)): 2880       | Act DB Ct: 0
 [INFO] Processing Table csm_allocation_node_history   | User Ct (time(mins)): 2880       | Act DB Ct: 0
 [INFO] Processing Table csm_db_schema_version_history | User Ct (time(mins)): 2880       | Act DB Ct: 0
 [INFO] Processing Table csm_diag_result_history       | User Ct (time(mins)): 2880       | Act DB Ct: 0
 [INFO] Processing Table csm_diag_run_history          | User Ct (time(mins)): 2880       | Act DB Ct: 0
 [INFO] Processing Table csm_dimm_history              | User Ct (time(mins)): 2880       | Act DB Ct: 0
 [INFO] Processing Table csm_gpu_history               | User Ct (time(mins)): 2880       | Act DB Ct: 0
 [INFO] Processing Table csm_hca_history               | User Ct (time(mins)): 2880       | Act DB Ct: 0
 [INFO] Processing Table csm_ib_cable_history          | User Ct (time(mins)): 2880       | Act DB Ct: 0
 [INFO] Processing Table csm_lv_history                | User Ct (time(mins)): 2880       | Act DB Ct: 0
 [INFO] Processing Table csm_lv_update_history         | User Ct (time(mins)): 2880       | Act DB Ct: 0
 [INFO] Processing Table csm_node_history              | User Ct (time(mins)): 2880       | Act DB Ct: 0
 [INFO] Processing Table csm_node_state_history        | User Ct (time(mins)): 2880       | Act DB Ct: 0
 [INFO] Processing Table csm_processor_socket_history  | User Ct (time(mins)): 2880       | Act DB Ct: 0
 [INFO] Processing Table csm_ssd_history               | User Ct (time(mins)): 2880       | Act DB Ct: 0
 [INFO] Processing Table csm_ssd_wear_history          | User Ct (time(mins)): 2880       | Act DB Ct: 0
 [INFO] Processing Table csm_step_history              | User Ct (time(mins)): 2880       | Act DB Ct: 0
 [INFO] Processing Table csm_step_node_history         | User Ct (time(mins)): 2880       | Act DB Ct: 0
 [INFO] Processing Table csm_switch_history            | User Ct (time(mins)): 2880       | Act DB Ct: 0
 [INFO] Processing Table csm_switch_inventory_history  | User Ct (time(mins)): 2880       | Act DB Ct: 0
 [INFO] Processing Table csm_vg_history                | User Ct (time(mins)): 2880       | Act DB Ct: 0
 [INFO] Processing Table csm_vg_ssd_history            | User Ct (time(mins)): 2880       | Act DB Ct: 0
 [INFO] Processing Table csm_ras_event_action          | User Ct (time(mins)): 2880       | Act DB Ct: 0
 ---------------------------------------------------------------------------------------------------------
 DB Name:                           | csmdb
 DB User Name:                      | postgres
 Thread Count:                      | 10
 Deletion Log Directory:            | /var/log/ibm/csm/db/csm_db_history_delete.log
 End Script Time:                   | 2018-12-10 11:56:13.441324
 Total Process Time:                | 0:00:00.046189
 ---------------------------------------------------------------------------------------------------------
 Finish CSM DB deletion script process
 ---------------------------------------------------------------------------------------------------------

If a user specifies a non related DB in the system, unrelated user name, and or if there are issues connecting to the DB server a message will display.

.. code-block:: bash

 [root@c650mnp02 db]# /opt/ibm/csm/db/csm_db_history_delete.py -d csmdb123 -n 1 -u abcd
 ---------------------------------------------------------------------------------------------------------
 Welcome to the CSM DB deletion of history table records script
 ---------------------------------------------------------------------------------------------------------
 Start Script Time:                 | 2018-12-10 11:56:19.555008
 ---------------------------------------------------------------------------------------------------------
 [CRITICAL] Unable to connect to local database.
 [CRITICAL] Unable to connect to local database.
 [CRITICAL] Unable to connect to local database.
 [CRITICAL] Unable to connect to local database.
 [CRITICAL] Unable to connect to local database.
 [CRITICAL] Unable to connect to local database.
 [CRITICAL] Unable to connect to local database.
 [CRITICAL] Unable to connect to local database.
 [CRITICAL] Unable to connect to local database.
 [CRITICAL] Unable to connect to local database.
 [CRITICAL] Unable to connect to local database.
 [CRITICAL] Unable to connect to local database.
 [CRITICAL] Unable to connect to local database.
 [CRITICAL] Unable to connect to local database.
 [CRITICAL] Unable to connect to local database.
 [CRITICAL] Unable to connect to local database.
 [CRITICAL] Unable to connect to local database.
 [CRITICAL] Unable to connect to local database.
 [CRITICAL] Unable to connect to local database.
 [CRITICAL] Unable to connect to local database.
 [CRITICAL] Unable to connect to local database.
 [CRITICAL] Unable to connect to local database.
 [CRITICAL] Unable to connect to local database.
 [CRITICAL] Unable to connect to local database.
 [CRITICAL] Unable to connect to local database.
 ---------------------------------------------------------------------------------------------------------
 DB Name:                           | csmdb123
 DB User Name:                      | abcd
 Thread Count:                      | 10
 Deletion Log Directory:            | /var/log/ibm/csm/db/csm_db_history_delete.log
 End Script Time:                   | 2018-12-10 11:56:19.601613
 Total Process Time:                | 0:00:00.046605
 ---------------------------------------------------------------------------------------------------------
 Finish CSM DB deletion script process
 ---------------------------------------------------------------------------------------------------------

The ``csm_db_history_delete.py`` script (when called manually) will delete history records which have been
archived with a *archive_history_timestamp*. Records in the history table that do not have an archived_history_timestamp
will remain in the system until it has been archived.

.. note:: Directory: The scripts logging information will be in a specified directory.

The history table delete log file will be in a .log file format and in the default directory:
``/var/log/ibm/csm/db/csm_db_history_delete.log``
