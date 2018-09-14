Using csm_db_history_delete.sh
==============================

This section describes the deletion process associated with the CSM DB :ref:`history tables <history_tables>`.  If ran alone it will delete all history tables including the :ref:`csm_event_action table <csm_ras_event_action>`, which are apart of the CSM DB or specifies DB (in quiet mode: no screen results output) which contains an archive history time stamp.

Usage Overview
--------------

.. code-block:: bash

 -bash-4.2$ ./csm_db_history_delete.sh -h
 ----------------------------------------------------------------------------------------
  CSM Database History Delete Usage
 ===================================================================================
    -h                       	Display this message.
    -t <dir>                 	Target directory to write the delete log to.
                             	Default: "/var/log/ibm/csm/delete"
    -n <time-count mins.>	    The time (in mins.) of oldest records which to delete.
                             	Attention: requires users input value
    -d <db>                  	Database to delete tables from.
                             	Attention: requires users input value
 ===================================================================================
 [Example ] ./csm_db_history_delete.sh -d [dbname] -n [time_interval] -t [/data_dir/]
 ----------------------------------------------------------------------------------------

.. note:: This is a general overview of the CSM DB deletion process using the ``csm_history_wrapper_delete_script_template.sh`` script.

The ``csm_history_wrapper_delete_script_template.sh`` script (when called manually) will delete history records which have been
archived with a archive_history_timestamp. Records in the history table that do not have an archived_history_timestamp
will remain in the system until it has been archived.

The ``csm_history_wrapper_delete_script_template.sh`` script will take in take in certain flags:
 #. *Database name*
 #. *Interval time (in minutes)*
 #. *History table name*
 #. *Specified directory to be written to*

Example (wrapper script)
^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 [./csm_history_wrapper_delete_script_template.sh] [dbname] [time_mins] [history_table_name] [data_dir]

.. attention:: The deletion wrapper script is a per history table deletion process.

Example (Script out results)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. code-block:: bash

 -bash-4.2$ ./csm_history_wrapper_delete_script_template.sh csmdb 1 csm_node_history /tmp/csm_node_history_delete/
 ------------------------------------------------------------------------------
   Table                        | Time        |  Delete Count (DB Actual)
 -------------------------------|-------------|--------------------------------
  csm_node_history              |  0.005      |   0
 ------------------------------------------------------------------------------
  Date/Time:                    |  2018-04-05.09.57.42
  DB Name:                      |  csmdb
  DB User:                      |  postgres
  Interval time (cmd-line):     |  1 Min(s).
  Total time (Cleanup):         |  0.005
 ------------------------------------------------------------------------------


.. note:: Directory: Currently the scripts are setup to archive the results in a specified directory.

Example output file:
The history delete timing results will be logged in a csv file:
``csm_db_delete_script.log``
