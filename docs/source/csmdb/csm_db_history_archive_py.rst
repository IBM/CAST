Using csm_db_history_archive.py
===============================

This section describes the archiving process associated with the CSM DB :ref:`history tables <history_tables>`.
If run alone it will archive all history tables in the CSM Database, including the 
:ref:`csm_ras_event_action table <csm_ras_event_action>`.

.. note:: This script is designed to run as a `root` user. If you try to run as a postgres user the script will prompt a message and exit.

.. code-block:: bash

 -bash-4.2$ ./csm_db_history_archive.py -h
 ---------------------------------------------------------------------------------------------------------
 [INFO] Only root can run this script
 ---------------------------------------------------------------------------------------------------------

Usage Overview
--------------

.. code-block:: bash

 /opt/ibm/csm/db/csm_db_history_archive.py –h
 /opt/ibm/csm/db/csm_db_history_archive.py --help

The help command (-h, --help) will specify each of the options available to use.

+------------------------------------+--------------------------------------------+-------------------------------------------+
|               Options              |                 Description                |                   Result                  |
+====================================+============================================+===========================================+
| running the script with no options | ./csm_db_history_archive.py                | Will execute with default configured      |
|                                    |                                            | settings                                  |
+------------------------------------+--------------------------------------------+-------------------------------------------+
| running the script with            | ./csm_db_history_archive.py –t, --target   | Specifies the target directory where      |
| –t, --target                       |                                            | json files will be written to.            |
+------------------------------------+--------------------------------------------+-------------------------------------------+
| running the script with            | ./csm_db_history_archive.py –n, --count    | specifies the number of records to be     |
| -n, --count                        |                                            | archived.                                 |
+------------------------------------+--------------------------------------------+-------------------------------------------+
| running the script with            | ./csm_db_history_archive.py -d, --database | specifies the database name               |
| –d, --database                     |                                            |                                           |
+------------------------------------+--------------------------------------------+-------------------------------------------+
| running the script with            | ./csm_db_history_archive.py –u, --user     | specifies the database user name.         |
| –u, --user                         |                                            |                                           |
+------------------------------------+--------------------------------------------+-------------------------------------------+
| running the script with            | ./csm_db_history_archive.py –-threads      | specifies threads.                        |
| –-threads                          |                                            |                                           |
+------------------------------------+--------------------------------------------+-------------------------------------------+
| running the script with            | ./csm_db_history_archive.py –h, --help     | see details below                         |
| –h, --help                         |                                            |                                           |
+------------------------------------+--------------------------------------------+-------------------------------------------+

Example (usage)
^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_history_archive.py -h
 ---------------------------------------------------------------------------------------------------------
 usage: csm_db_history_archive.py [-h] [-t dir] [-n count] [-d db] [-u user]
                                  [--threads threads]
 
 A tool for archiving the CSM Database history tables.
 
 optional arguments:
   -h, --help            show this help message and exit
   -t dir, --target dir  Target directory to write archive to. Default:
                         /var/log/ibm/csm/archive
   -n count, --count count
                         Number of records to archive in the run. Default: 1000
   -d db, --database db  Database to archive tables from. Default: csmdb
   -u user, --user user  The database user. Default: postgres
   --threads threads     The number of threads for the thread pool. Default: 10
 
 ------------------------------------------------------------------------------

.. note:: This is a general overview of the CSM DB archive history process using the ``csm_db_history_archive.py`` script.

Script overview
^^^^^^^^^^^^^^^
The script may largely be broken into 

1. Create a temporary table to archive history data based on a condition.

  a. Connect to the Database with the postgres user.
  #. Drops and creates the temp table used in the archival process.
  #. The first query selects all the fields in the table.
  #. The second and third query is a nested query that defines a particular row count that a user can pass in or can be set as a default value. The data is filter by using the history_time)..
  #. The where clause defines whether the *archive_history_time* field is *NULL*.
  #. The user will have the option to pass in a row count value (ex. 10,000 records).
  #. The data will be ordered by ``history_time ASC``.

2. Copies all satisfied history data to a json file.
  
  a. Copies all the results from the temp table and appends to a json file

3. Then updates the *archive_history_timestamp* field, which can be later deleted during the purging process).

  a. Updates the csm_[table_name]_history table
  #. Sets the archive_history_time = current timestamp
  #. From clause on the temp table
  #. WHERE (compares history_time, from history table to temp table) AND history.archive_history_time IS NULL.


.. attention:: If this script below is run manually it will display the results to the screen.  
    This script only handles per table archiving.

Script out results
------------------

.. code-block:: bash

 [root@c650mnp02 db]# /opt/ibm/csm/db/csm_db_history_archive.py -d csmdb -t /tmp/test_archive_dir/ -n 100
 ---------------------------------------------------------------------------------------------------------
 Welcome to the CSM DB archiving script
 ---------------------------------------------------------------------------------------------------------
 Start Script Time:                 | 2018-11-23 11:25:02.027564
 ---------------------------------------------------------------------------------------------------------
 [INFO] Processing Table csm_config_history            | User Ct: 100        | Act DB Ct: 0
 [INFO] Processing Table csm_allocation_history        | User Ct: 100        | Act DB Ct: 0
 [INFO] Processing Table csm_allocation_node_history   | User Ct: 100        | Act DB Ct: 0
 [INFO] Processing Table csm_db_schema_version_history | User Ct: 100        | Act DB Ct: 100
 [INFO] Processing Table csm_allocation_state_history  | User Ct: 100        | Act DB Ct: 0
 [INFO] Processing Table csm_diag_result_history       | User Ct: 100        | Act DB Ct: 0
 [INFO] Processing Table csm_diag_run_history          | User Ct: 100        | Act DB Ct: 0
 [INFO] Processing Table csm_hca_history               | User Ct: 100        | Act DB Ct: 0
 [INFO] Processing Table csm_dimm_history              | User Ct: 100        | Act DB Ct: 0
 [INFO] Processing Table csm_ib_cable_history          | User Ct: 100        | Act DB Ct: 0
 [INFO] Processing Table csm_gpu_history               | User Ct: 100        | Act DB Ct: 0
 [INFO] Processing Table csm_lv_history                | User Ct: 100        | Act DB Ct: 0
 [INFO] Processing Table csm_lv_update_history         | User Ct: 100        | Act DB Ct: 0
 [INFO] Processing Table csm_processor_socket_history  | User Ct: 100        | Act DB Ct: 0
 [INFO] Processing Table csm_node_history              | User Ct: 100        | Act DB Ct: 0
 [INFO] Processing Table csm_ssd_history               | User Ct: 100        | Act DB Ct: 0
 [INFO] Processing Table csm_node_state_history        | User Ct: 100        | Act DB Ct: 0
 [INFO] Processing Table csm_ssd_wear_history          | User Ct: 100        | Act DB Ct: 0
 [INFO] Processing Table csm_step_history              | User Ct: 100        | Act DB Ct: 0
 [INFO] Processing Table csm_switch_inventory_history  | User Ct: 100        | Act DB Ct: 0
 [INFO] Processing Table csm_step_node_history         | User Ct: 100        | Act DB Ct: 0
 [INFO] Processing Table csm_vg_history                | User Ct: 100        | Act DB Ct: 0
 [INFO] Processing Table csm_switch_history            | User Ct: 100        | Act DB Ct: 0
 [INFO] Processing Table csm_vg_ssd_history            | User Ct: 100        | Act DB Ct: 0
 [INFO] Processing Table csm_ras_event_action          | User Ct: 100        | Act DB Ct: 0
 ---------------------------------------------------------------------------------------------------------
 DB Name:                           | csmdb
 Archiving Log Directory:           | /var/log/ibm/csm/db/csm_db_archive_script.log
 Archiving Data Directory:          | /tmp/test_archive_dir/
 End Script Time:                   | 2018-11-23 11:25:02.130501
 Total Process Time:                | 0:00:00.102937
 ---------------------------------------------------------------------------------------------------------
 Finish CSM DB archive script process
 ---------------------------------------------------------------------------------------------------------

.. attention:: While using the csm_stats_script (in another session) the user can monitor the results

.. code-block:: bash

 /opt/ibm/csm/db/csm_db_stats.sh –t <db_name>
 /opt/ibm/csm/db/csm_db_stats.sh –-tableinfo <db_name>

If a user specifies a non related DB in the system or if there are issues connecting to the DB server a message will display.

.. code-block:: bash

 [root@c650mnp02 db]# /opt/ibm/csm/db/csm_db_history_archive.py -d csmd -t /tmp/test_archive_dir/ -n 100
 ---------------------------------------------------------------------------------------------------------
 Welcome to the CSM DB archiving script
 ---------------------------------------------------------------------------------------------------------
 Start Script Time:                 | 2018-11-23 11:44:17.535131
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
 DB Name:                           | csmd
 Archiving Log Directory:           | /var/log/ibm/csm/db/csm_db_archive_script.log
 Archiving Data Directory:          | /tmp/test_archive_dir/
 End Script Time:                   | 2018-11-23 11:44:17.574674
 Total Process Time:                | 0:00:00.039543
 ---------------------------------------------------------------------------------------------------------
 Finish CSM DB archive script process
 ---------------------------------------------------------------------------------------------------------

.. note:: Directory: Currently the scripts are setup to archive the results in a specified directory.

The history table data will be archived in a `.json` file format and in the specified or default directory:
``csm_allocation_history.archive.2018-11-23.json``

The history table log file will be in a `.log` file format and in the default directory:
``/var/log/ibm/csm/db/csm_db_archive_script.log``
