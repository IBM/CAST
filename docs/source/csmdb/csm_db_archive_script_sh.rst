Using csm_db_archive_script.sh
==============================

This section describes the archiving process associated with the CSM DB :ref:`history tables <history_tables>`.
If ran alone it will archive all history tables including the :ref:`csm_ras_event_action table <csm_ras_event_action>`,
which are apart of the CSM DB or specifies DB (in quiet mode: no screen results output).

Usage Overview
--------------

.. code-block:: bash

 -bash-4.2$ ./csm_db_history_archive.sh -h
 ----------------------------------------------------------------------------------------
 CSM Database History Archive Usage -
 =======================================================================================
     -h		        Display this message.
     -t <dir>     	Target directory to write the archive to.
                      	Default: "/var/log/ibm/csm/archive"
     -n <count>		Number of records to archive in the run.
                      	Default: 100
     -d <db>      	Database to archive tables from.
                      	Default: "csmdb"
 ======================================================================================
 

 [Example ] ./csm_db_history_delete.sh -d [dbname] -n [time_interval] -t [/data_dir/]
 --------------------------------------------------------------------------------------

.. note:: This is a general overview of the CSM DB archive history process using the ``csm_history_wrapper_archive_script_template.sh`` script.

Script overview
^^^^^^^^^^^^^^^

**The wrapper script is broken down into three sections which include:**
 #. *Creating a temporary table to archive history data based on a condition.*
 #. *Copies all satisfied history data to a json file*
 #. *Then updates the archive_history_timestamp field, which can be later deleted during the purging process)*

**Section 1:**
 #. *Connects to the DB with the postgres user*
 #. *Drops (if exists) and creates the temp table used in the archiving process*
 #. *The first query selects all the specific fields in the table*
 #. *The second and third query is a nested query that defines a particular row count that a user can pass in or can be set as a default value. The data is filter by using the history_time).*
 #. *The where clause defines whether the archive_history_time field is NULL*
 #. *The user will have the option to pass in a row count value (ex. 10,000 records)*
 #. *The data will be ordered by history_time ASC.*

**Section 2:**
 #. *Copies all the results from the temp table and appends to a json file*

**Section 3:**
 #. *Updates the csm_[table_name]_history table*
 #. *Sets the archive_history_time = current timestamp*
 #. *From clause on the temp table*
 #. *WHERE (compares history_time, from history table to temp table) AND history.archive_history_time IS NULL*

The script argument options:
""""""""""""""""""""""""""""
 #. *Database name (Default: "csmdb")*
 #. *Archive counter (how many records to be archived: Default: 100)*
 #. *Specified directory to be written to* (Default: ``/var/log/ibm/csm/archive``)

Sample wrapper script:
""""""""""""""""""""""

.. attention:: If this script below is ran manually it will display the results to the screen.  This script only handles per table archiving.

.. code-block:: bash

	[./csm_history_wrapper_archive_script_template.sh] [dbname] [archive_counter] [history_table_name] [/data_dir/]

Script out results:
"""""""""""""""""""

.. code-block:: bash

 -bash-4.2$ ./csm_history_wrapper_archive_script_template.sh csmdb 10000 csm_node_history /tmp/
 ------------------------------------------------------------------------------
   Table                        |  Time       |  Archive Count
 -------------------------------|-------------|--------------------------------
  csm_node_history              |  0.157      |   10000
 ------------------------------------------------------------------------------
  Date/Time:                    |  2018-04-05.09.26.36.411615684
  DB Name:                      |  csmdb
  DB User:                      |  postgres
  archive_counter:              |  10000
  Total time:                   |  0.157
  Average time:                 |  0.157
 ------------------------------------------------------------------------------

.. attention:: While using the csm_stats_script (in another session) the user can monitor the results

.. code-block:: bash

 ./csm_db_stats.sh –t <db_name>

.. note:: Directory: Currently the scripts are setup to archive the results in a specified directory.

| **Example output file:**
| The history table data will be archived in a csv file along with the log file:
| (example:)
| ``csm_db_archive_script.log``
| ``csm_node_history.archive.2018-07-30.json``
