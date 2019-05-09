Using csm_db_stats.sh script
============================

This script will gather statistical information related to the CSM DB which includes, table data activity, index related information, and table lock monitoring, CSM DB schema version, DB connections stats query, DB user stats query, and PostgreSQL version installed .


.. note::
 For a quick overview of the script functionality, 


.. code-block:: bash

 /opt/ibm/csm/db/csm_db_stats.sh –h
 /opt/ibm/csm/db/csm_db_stats.sh --help
 
 This help command <-h, --help> will specify each of the options available to use.

The ``csm_db_stats.sh`` script creates a log file for each query executed. (Please see the log file for details): ``/var/log/ibm/csm/csm_db_stats.log``

Usage Overview
--------------

+------------------------------+------------------------------------------------+-------------------+
|            Options           |                     Command                    |       Result      |
+==============================+================================================+===================+
| Table data activity          | ./csm_db_stats.sh –t <my_db_name>              | see details below |
|                              | ./csm_db_stats.sh --tableinfo <my_db_name>     |                   |
+------------------------------+------------------------------------------------+-------------------+
| Index related information    | ./csm_db_stats.sh –i <my_db_name>              | see details below |
|                              | ./csm_db_stats.sh --indexinfo <my_db_name>     |                   |
+------------------------------+------------------------------------------------+-------------------+
| Index analysis information   | ./csm_db_stats.sh –x <my_db_name>              | see details below |
|                              | ./csm_db_stats.sh --indexanalysis <my_db_name> |                   |
+------------------------------+------------------------------------------------+-------------------+
| Table Locking Monitoring     | ./csm_db_stats.sh –l <my_db_name>              | see details below |
|                              | ./csm_db_stats.sh --lockinfo <my_db_name>      |                   |
+------------------------------+------------------------------------------------+-------------------+
| Schema Version Query         | ./csm_db_stats.sh –s <my_db_name>              | see details below |
|                              | ./csm_db_stats.sh --schemaversion <my_db_name> |                   |
+------------------------------+------------------------------------------------+-------------------+
| DB connections stats Query   | ./csm_db_stats.sh –c <my_db_name>              | see details below |
|                              | ./csm_db_stats.sh --connectionsdb <my_db_name> |                   |
+------------------------------+------------------------------------------------+-------------------+
| DB user stats query          | ./csm_db_stats.sh –u <my_db_name>              | see details below |
|                              | ./csm_db_stats.sh –usernamedb <my_db_name>     |                   |
+------------------------------+------------------------------------------------+-------------------+
| PostgreSQL Version Installed | ./csm_db_stats.sh -v csmdb                     | see details below |
|                              | ./csm_db_stats.sh --postgresqlversion csmdb    |                   |
+------------------------------+------------------------------------------------+-------------------+
| DB Archiving Stats           | ./csm_db_stats.sh -a csmdb                     | see details below |
|                              | ./csm_db_stats.sh --archivecount csmdb         |                   |
+------------------------------+------------------------------------------------+-------------------+
| --help                       | ./csm_db_stats.sh                              | see details below |
+------------------------------+------------------------------------------------+-------------------+

Example (usage)
^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_stats.sh -h
 ------------------------------------------------------------------------------------------------------------------------
 [Start ] Welcome to CSM datatbase automation stats script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info  ] Log Dir: /var/log/ibm/csm/db/csm_db_stats.log
 ------------------------------------------------------------------------------------------------------------------------
 [Info ] csm_db_stats.sh : List/Kill database user sessions
 [Usage] csm_db_stats.sh : [OPTION]... [DBNAME]
 ------------------------------------------------------------------------------------------------------------------------
   Argument               |  DB Name  | Description
 -------------------------|-----------|----------------------------------------------------------------------------------
  -t, --tableinfo         | [db_name] | Populates Database Table Stats:
                          |           | Live Row Count, Inserts, Updates, Deletes, and Table Size
  -i, --indexinfo         | [db_name] | Populates Database Index Stats:
                          |           | tablename, indexname, num_rows, tbl_size, ix_size, uk,
                          |           | num_scans, tpls_read, tpls_fetched
  -x, --indexanalysis     | [db_name] | Displays the index usage analysis
  -l, --lockinfo          | [db_name] | Displays any locks that might be happening within the DB
  -s, --schemaversion     | [db_name] | Displays the current CSM DB version
  -c, --connectionsdb     | [db_name] | Displays the current DB connections
  -u, --usernamedb        | [db_name] | Displays the current DB user names and privileges
  -v, --postgresqlversion | [db_name] | Displays the current version of PostgreSQL installed
                          |           | along with environment details
  -a, --archivecount      | [db_name] | Displays the archived and non archive record counts
  -d, --deletecount       | [db_name] | Displays the total record count based on time interval
  -k, --vacuumstats       | [db_name] | Displays the DB vacuum statistics
  -h, --help              |           | help
 -------------------------|-----------|----------------------------------------------------------------------------------
 [Examples]
 ------------------------------------------------------------------------------------------------------------------------
    csm_db_stats.sh -t, --tableinfo         | [dbname] |        | Database table stats
    csm_db_stats.sh -i, --indexinfo         | [dbname] |        | Database index stats
    csm_db_stats.sh -x, --indexanalysisinfo | [dbname] |        | Database index usage analysis stats
    csm_db_stats.sh -l, --lockinfo          | [dbname] |        | Database lock stats
    csm_db_stats.sh -s, --schemaversion     | [dbname] |        | Database schema version (CSM_DB only)
    csm_db_stats.sh -c, --connectionsdb     | [dbname] |        | Database connections stats
    csm_db_stats.sh -u, --usernamedb        | [dbname] |        | Database user stats
    csm_db_stats.sh -v, --postgresqlversion | [dbname] |        | Database (PostgreSQL) version
    csm_db_stats.sh -a, --archivecount      | [dbname] |        | Database archive stats
    csm_db_stats.sh -d, --deletecount       | [dbname] | [time] | Database delete count stats
    csm_db_stats.sh -k, --vacuumstats       | [dbname] |        | Database vacuum stats
    csm_db_stats.sh -h, --help              | [dbname] |        | Help menu
 ------------------------------------------------------------------------------------------------------------------------

1. Table data activity
-----------------------

.. code-block:: bash

 /opt/ibm/csm/db/csm_db_stats.sh –t <my_db_name>
 /opt/ibm/csm/db/csm_db_stats.sh --tableinfo <my_db_name>

Example (Query details)
^^^^^^^^^^^^^^^^^^^^^^^
+--------------------+-------------------------------------------------+
|   Column_Name      |                   Description                   |
+--------------------+-------------------------------------------------+
| ``tablename``      | table name                                      |
+--------------------+-------------------------------------------------+
| ``live_row_count`` | current row count in the CSM_DB                 |
+--------------------+-------------------------------------------------+
| ``insert_count``   | number of rows inserted into each of the tables |
+--------------------+-------------------------------------------------+
| ``update_count``   | number of rows updated in each of the tables    |
+--------------------+-------------------------------------------------+
| ``delete_count``   | number of rows deleted in each of the tables    |
+--------------------+-------------------------------------------------+
| ``table_size``     | table size                                      |
+--------------------+-------------------------------------------------+

.. note:: This query will display information related to the CSM DB tables (or other specified DB). The query will display results based on if the insert, update, and delete count is > ``0``.  If there is no data in a particular table it will be omitted from the results.

Example (DB Table info.)
^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_stats.sh -t csmdb
 ------------------------------------------------------------------------------------------------------------------------
 [Start ] Welcome to CSM datatbase automation stats script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info  ] Log Dir: /var/log/ibm/csm/db/csm_db_stats.log
 ------------------------------------------------------------------------------------------------------------------------
         relname        | live_row_count | insert_count | update_count | delete_count | dead_tuples | table_size
 -----------------------+----------------+--------------+--------------+--------------+-------------+------------
  csm_db_schema_version |              1 |            1 |            0 |            0 |           0 | 8192 bytes
  csm_ras_type          |            744 |          744 |            0 |            0 |           0 | 112 kB
  csm_ras_type_audit    |            744 |          744 |            0 |            0 |           0 | 128 kB
 (3 rows)
 
 ------------------------------------------------------------------------------------------------------------------------

2. Index related information
----------------------------

.. code-block:: bash	
 
 /opt/ibm/csm/db/csm_db_stats.sh –i <my_db_name>
 /opt/ibm/csm/db/csm_db_stats.sh --indexinfo <my_db_name>

Example (Query details)
^^^^^^^^^^^^^^^^^^^^^^^
+--------------------+--------------------------------------------+
|   Column_Name      |                 Description                |
+--------------------+--------------------------------------------+
| ``tablename``      | table name                                 |
+--------------------+--------------------------------------------+
| ``indexname``      | index name                                 |
+--------------------+--------------------------------------------+
| ``num_rows``       | number of rows within the table            |
+--------------------+--------------------------------------------+
| ``table_size``     | table size                                 |
+--------------------+--------------------------------------------+
| ``index_size``     | index size                                 |
+--------------------+--------------------------------------------+
| ``unique``         | if the index is unique (‘T’ or ‘F’)        |
+--------------------+--------------------------------------------+
| ``number_of_scans``| the number returned is the amount of       |
|                    | times the index was used                   |
+--------------------+--------------------------------------------+

.. note:: This query will display information about indexes related to each table in use.  Results will be displayed if the ``num_rows``, ``num_scans``, ``tuples_read``, and ``tuples_fetched`` count are > ``0``.   If there is no data in a particular table it will be omitted from the results.

Example (Indexes)
^^^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_stats.sh -i csmdb
 ------------------------------------------------------------------------------------------------------------------------
 [Start ] Welcome to CSM datatbase automation stats script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info  ] Log Dir: /var/log/ibm/csm/db/csm_db_stats.log
 ------------------------------------------------------------------------------------------------------------------------
      tablename      |        indexname        | num_rows | table_size | index_size | unique | number_of_scans | tuples_read | tuples_fetched
 --------------------+-------------------------+----------+------------+------------+--------+-----------------+-------------+----------------
  csm_ras_type       | csm_ras_type_pkey       |      744 | 112 kB     | 56 kB      | Y      |               0 |           0 |              0
  csm_ras_type_audit | csm_ras_type_audit_pkey |      744 | 128 kB     | 32 kB      | Y      |               0 |           0 |              0
 (2 rows)
 
 ------------------------------------------------------------------------------------------------------------------------
 
3. Index Analysis Usage Information
-----------------------------------

.. code-block:: bash	
 
 /opt/ibm/csm/db/csm_db_stats.sh –x <my_db_name>
 /opt/ibm/csm/db/csm_db_stats.sh --indexanalysis <my_db_name>

Example (Query details)
^^^^^^^^^^^^^^^^^^^^^^^
+--------------------+----------------------------------------------------------+
|   Column_Name      |                 Description                              |
+--------------------+----------------------------------------------------------+
| ``relname``        | table name                                               |
+--------------------+----------------------------------------------------------+
| ``too_much_seq``   | case when seq_scan - idx_scan > 0                        |
+--------------------+----------------------------------------------------------+
| ``case``           | If Missing Index or is Ok                                |
+--------------------+----------------------------------------------------------+
| ``rel_size``       | OID of a table, index returns the on-disk size in bytes. |
+--------------------+----------------------------------------------------------+
| ``seq_scan``       | Number of sequential scans initiated on this table.      |
+--------------------+----------------------------------------------------------+
| ``idx_scan``       | Number of index scans initiated on this index            |
+--------------------+----------------------------------------------------------+

.. note:: This query checks if there are more sequence scans being performed instead of index scans.  Results will be displayed if the ``relname``, ``too_much_seq``, ``case``, ``rel_size``, ``seq_scan``, and ``idx_scan``. This query helps analyze database.

Example (Indexes Usage)
^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_stats.sh -x csmdb
 ------------------------------------------------------------------------------------------------------------------------
 [Start ] Welcome to CSM datatbase automation stats script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info  ] Log Dir: /var/log/ibm/csm/db/csm_db_stats.log
 ------------------------------------------------------------------------------------------------------------------------
               relname         | too_much_seq |      case      |  rel_size   | seq_scan | idx_scan
 ------------------------------+--------------+----------------+-------------+----------+----------
  csm_step_node                |     16280094 | Missing Index? |      245760 | 17438931 |  1158837
  csm_allocation_history       |      3061025 | Missing Index? |    57475072 |  3061787 |      762
  csm_allocation_state_history |         3276 | Missing Index? |    35962880 |    54096 |    50820
  csm_vg_history               |         1751 | Missing Index? |      933888 |     1755 |        4
  csm_vg_ssd_history           |         1751 | Missing Index? |      819200 |     1755 |        4
  csm_ssd_history              |         1749 | Missing Index? |     1613824 |     1755 |        6
  csm_dimm_history             |         1652 | Missing Index? |    13983744 |     1758 |      106
  csm_gpu_history              |         1645 | Missing Index? |    24076288 |     1756 |      111
  csm_hca_history              |         1643 | Missing Index? |     8167424 |     1754 |      111
  csm_ras_event_action         |         1549 | Missing Index? |   263143424 |     1854 |      305
  csm_node_state_history       |          401 | Missing Index? |    78413824 |      821 |      420
  csm_node_history             |       -31382 | OK             |   336330752 |      879 |    32261
  csm_ras_type_audit           |       -97091 | OK             |       98304 |   793419 |   890510
  csm_step_history             |      -227520 | OK             |   342327296 |      880 |   228400
  csm_vg_ssd                   |      -356574 | OK             |      704512 |   125588 |   482162
  csm_vg                       |      -403370 | OK             |      729088 |    86577 |   489947
  csm_hca                      |      -547463 | OK             |     1122304 |        1 |   547464
  csm_ras_type                 |      -942966 | OK             |       81920 |       23 |   942989
  csm_ssd                      |     -1242433 | OK             |     1040384 |    85068 |  1327501
  csm_step_node_history        |     -1280913 | OK             |  2865987584 |    49335 |  1330248
  csm_allocation_node_history  |     -1664023 | OK             | 21430599680 |      887 |  1664910
  csm_gpu                      |     -2152044 | OK             |     5996544 |        1 |  2152045
  csm_dimm                     |     -2239777 | OK             |     7200768 |   118280 |  2358057
  csm_allocation_node          |    -52187077 | OK             |      319488 |  1727675 | 53914752
  csm_node                     |    -78859700 | OK             |     2768896 |   127214 | 78986914
 (25 rows)
 --------------------------------------------------------------------------------------------------


4. Table Lock Monitoring
------------------------

.. code-block:: bash

 /opt/ibm/csm/db/csm_db_stats.sh –l <my_db_name>
 /opt/ibm/csm/db/csm_db_stats.sh --lockinfo <my_db_name>
 
Example (Query details)
^^^^^^^^^^^^^^^^^^^^^^^
+-----------------------------------------------------+-----------------------------------------------------------------+
|                   Column_Name                       |                           Description                           |
+-----------------------------------------------------+-----------------------------------------------------------------+
| ``blocked_pid``                                     | Process ID of the server process holding or awaiting this lock, |
|                                                     | or null if the lock is held by a prepared transaction.          |
+-----------------------------------------------------+-----------------------------------------------------------------+
| ``blocked_user``                                    | The user that is being blocked.                                 |
+-----------------------------------------------------+-----------------------------------------------------------------+
| ``current_or_recent_statement_in_blocking_process`` | The query statement that is displayed as a result.              |
+-----------------------------------------------------+-----------------------------------------------------------------+
| ``state_of_blocking_process``                       | Current overall state of this backend.                          |
+-----------------------------------------------------+-----------------------------------------------------------------+
| ``blocking_duration``                               | Evaluates when the process begin and subtracts from the current |
|                                                     | time when the query began.                                      |
+-----------------------------------------------------+-----------------------------------------------------------------+
| ``blocking_pid``                                    | Process ID of this backend.                                     |
+-----------------------------------------------------+-----------------------------------------------------------------+
| ``blocking_user``                                   | The user that is blocking other transactions.                   |
+-----------------------------------------------------+-----------------------------------------------------------------+
| ``blocked_statement``                               | The query statement that is displayed as a result.              |
+-----------------------------------------------------+-----------------------------------------------------------------+
| ``blocked_duration``                                | Evaluates when the process begin and subtracts from the         |
|                                                     | current time when the query began.                              |
+-----------------------------------------------------+-----------------------------------------------------------------+

Example (Lock Monitoring)
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_stats.sh -l csmdb
 ------------------------------------------------------------------------------------------------------------------------
 [Start ] Welcome to CSM datatbase automation stats script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info  ] Log Dir: /var/log/ibm/csm/db/csm_db_stats.log
 ------------------------------------------------------------------------------------------------------------------------
 -[ RECORD 1 ]-----------------------------------+--------------------------------------------------------------
 blocked_pid                                     | 38351
 blocked_user                                    | postgres
 current_or_recent_statement_in_blocking_process | update csm_processor set status=’N’ where serial_number=3;
 state_of+blocking_process                       | active
 blocking_duration                               | 01:01:11.653697
 blocking_pid                                    | 34389
 blocking_user                                   | postgres
 blocked_statement                               | update csm_processor set status=’N’ where serial_number=3;
 blocked_duration                                | 00:01:09.048478
 ------------------------------------------------------------------------------------------------------------------------

.. note:: This query displays relevant information related to lock monitoring.  It will display the current blocked and blocking rows affected along with each duration.   A systems administrator can run the query and evaluate what is causing the results of a “hung” procedure and determine the possible issue.

5. DB schema Version Query
--------------------------

.. code-block:: bash

 /opt/ibm/csm/db/csm_db_stats.sh –s <my_db_name>
 /opt/ibm/csm/db/csm_db_stats.sh --schemaversion <my_db_name>
 
Example (Query details)
^^^^^^^^^^^^^^^^^^^^^^^
+-----------------+----------------------------------------------------------------------+
| ``version``     | This provides the current CSM DB version that is current being used. |
+-----------------+----------------------------------------------------------------------+
| ``create_time`` | This column indicated when the database was created.                 |
+-----------------+----------------------------------------------------------------------+
| ``comment``     | This column indicates the “current version” as comment.              |
+-----------------+----------------------------------------------------------------------+

Example (DB Schema Version)
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_stats.sh -s csmdb
 ------------------------------------------------------------------------------------------------------------------------
 [Start ] Welcome to CSM datatbase automation stats script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info  ] Log Dir: /var/log/ibm/csm/db/csm_db_stats.log
 ------------------------------------------------------------------------------------------------------------------------
 -bash-4.2$ ./csm_db_stats.sh -s csmdb
 -------------------------------------------------------------------------------------
  version |        create_time         |     comment
 ---------+----------------------------+-----------------
   17.0   | 2019-02-14 17:31:10.079585 | current_version
 (1 row)
 ------------------------------------------------------------------------------------------------------------------------

.. note:: This query provides the current database version the system is running along with its creation time.

6. DB Connections with details
------------------------------

.. code-block:: bash

 /opt/ibm/csm/db/./csm_db_stats.sh –c <my_db_name>
 /opt/ibm/csm/db/./csm_db_stats.sh --connectionsdb <my_db_name>

Example (Query details)
^^^^^^^^^^^^^^^^^^^^^^^
+-------------------+--------------------------------------------------------------+
| ``pid``           | Process ID of this backend.                                  |
+-------------------+--------------------------------------------------------------+
| ``dbname``        | Name of the database this backend is connected to.           |
+-------------------+--------------------------------------------------------------+
| ``username``      | Name of the user logged into this backend.                   |
+-------------------+--------------------------------------------------------------+
| ``backend_start`` | Time when this process was started, i.e., when the client    |
|                   | connected to the server.                                     |
+-------------------+--------------------------------------------------------------+
| ``query_start``   | Time when the currently active query was started, or if      |
|                   | state is not active, when the last query was started.        |
+-------------------+--------------------------------------------------------------+
| ``state_change``  | Time when the state was last changed.                        |
+-------------------+--------------------------------------------------------------+
| ``wait``          | True if this backend is currently waiting on a lock.         |
+-------------------+--------------------------------------------------------------+
| ``query``         | Text of this backends most recent query. If state is active  |
|                   | this field shows the currently executing query. In all other |
|                   | states, it shows the last query that was executed.           |
+-------------------+--------------------------------------------------------------+

Example (database connections)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_stats.sh -c csmdb
 ------------------------------------------------------------------------------------------------------------------------
 [Start ] Welcome to CSM datatbase automation stats script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info  ] Log Dir: /var/log/ibm/csm/db/csm_db_stats.log
 ------------------------------------------------------------------------------------------------------------------------
   pid  | dbname | usename  |         backend_start      |          query_start       |         state_change       | wait |            query
 -------+--------+----------+----------------------------+----------------------------+----------------------------+------+---------------------------------
  61427 | xcatdb | xcatadm  | 2017-11-01 13:42:53.931094 | 2017-11-02 10:15:04.617097 | 2017-11-02 10:15:04.617112 | f    | DEALLOCATE
        |        |          |                            |                            |                            |      | dbdpg_p17050_384531
  61428 | xcatdb | xcatadm  | 2017-11-01 13:42:53.932721 | 2017-11-02 10:15:04.616291 | 2017-11-02 10:15:04.616313 | f    | SELECT 'DBD::Pg ping test'
  55753 | csmdb  | postgres | 2017-11-02 10:15:06.619898 | 2017-11-02 10:15:06.620889 | 2017-11-02 10:15:06.620891 | f    |
        |        |          |                            |                            |                            |      | SELECT pid,datname AS dbname,
        |        |          |                            |                            |                            |      | usename,backend_start, q.
        |        |          |                            |                            |                            |      |.uery_start, state_change,
        |        |          |                            |                            |                            |      | waiting AS wait,query FROM pg.
        |        |          |                            |                            |                            |      |._stat_activity;
 (3 rows)
 ------------------------------------------------------------------------------------------------------------------------

 
.. note:: This query will display information about the database connections that are in use on the system.  The pid (Process ID), database name, user name, backend start time, query start time, state change, waiting status, and query will display statistics about the current database activity.

7. PostgreSQL users with details
--------------------------------

.. code-block:: bash

 /opt/ibm/csm/db/./csm_db_stats.sh –u <my_db_name>
 /opt/ibm/csm/db/./csm_db_stats.sh --usernamedb <my_db_name>

Example (Query details)
^^^^^^^^^^^^^^^^^^^^^^^
+-------------------+--------------------------------------------------------------------------+
|   Column_Name     |                                Description                               |
+-------------------+--------------------------------------------------------------------------+
| ``rolname``       | Role name (t/f).                                                         |
+-------------------+--------------------------------------------------------------------------+
| ``rolsuper``      | Role has superuser privileges (t/f).                                     |
+-------------------+--------------------------------------------------------------------------+
| ``rolinherit``    | Role automatically inherits privileges of roles it is a member of (t/f). |
+-------------------+--------------------------------------------------------------------------+
| ``rolcreaterole`` | Role can create more roles (t/f).                                        |
+-------------------+--------------------------------------------------------------------------+
| ``rolcreatedb``   | Role can create databases (t/f).                                         |
+-------------------+--------------------------------------------------------------------------+
| ``rolcatupdate``  | Role can update system catalogs directly.                                |
|                   | (Even a superuser cannot do this unless this column is true) (t/f).      |
+-------------------+--------------------------------------------------------------------------+
| ``rolcanlogin``   | Role can log in. That is, this role can be given as the initial session  |
|                   | authorization identifier (t/f).                                          |
+-------------------+--------------------------------------------------------------------------+
| ``rolreplication``| Role is a replication role. That is, this role can initiate streaming    |
|                   | replication and set/unset the system backup mode using pg_start_backup   |
|                   | and pg_stop_backup (t/f).                                                |
+-------------------+--------------------------------------------------------------------------+
| ``rolconnlimit``  | For roles that can log in, this sets maximum number of concurrent        |
|                   | connections this role can make. -1 means no limit.                       |
+-------------------+--------------------------------------------------------------------------+
| ``rolpassword``   | Not the password (always reads as ********).                             |
+-------------------+--------------------------------------------------------------------------+
| ``rolvaliduntil`` | Password expiry time (only used for password authentication);            |
|                   | null if no expiration.                                                   |
+-------------------+--------------------------------------------------------------------------+
| ``rolconfig``     | Role-specific defaults for run-time configuration variables.             |
+-------------------+--------------------------------------------------------------------------+
| ``oid``           | ID of role.                                                              |
+-------------------+--------------------------------------------------------------------------+

Example (DB users with details)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_stats.sh -u postgres
 ------------------------------------------------------------------------------------------------------------------------
 [Start ] Welcome to CSM datatbase automation stats script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info  ] Log Dir: /var/log/ibm/csm/db/csm_db_stats.log
 ------------------------------------------------------------------------------------------------------------------------
  rolname  | rolsuper | rolinherit | rolcreaterole | rolcreatedb | rolcatupdate | rolcanlogin | rolreplication | rolconnlimit | rolpassword | rolvaliduntil | rolconfig |  oid
 ----------+----------+------------+---------------+-------------+--------------+-------------+----------------+--------------+-------------+---------------+-----------+--------
  postgres | t        | t          | t             | t           | t            | t           | t              |           -1 | ********    |               |           |     10
  xcatadm  | f        | t          | f             | f           | f            | t           | f              |           -1 | ********    |               |           |  16385
  root     | f        | t          | f             | f           | f            | t           | f              |           -1 | ********    |               |           |  16386
  csmdb    | f        | t          | f             | f           | f            | t           | f              |           -1 | ********    |               |           | 704142
 (4 rows)
 ------------------------------------------------------------------------------------------------------------------------

.. note:: This query will display specific information related to the users that are currently in the postgres database.  These fields will appear in the query: rolname, rolsuper, rolinherit, rolcreaterole, rolcreatedb, rolcatupdate, rolcanlogin, rolreplication, rolconnlimit, rolpassword, rolvaliduntil, rolconfig, and oid. See below for details.

8. PostgreSQL Version Installed
-------------------------------

.. code-block:: bash

 /opt/ibm/csm/db/./csm_db_stats.sh –v <my_db_name>
 /opt/ibm/csm/db/./csm_db_stats.sh --postgresqlversion <my_db_name>

+--------------+--------------------------------------------------------------+
| Column_Name  |                          Description                         |
+--------------+--------------------------------------------------------------+
| ``version``  | This provides the current PostgreSQL installed on the system |
|              | along with other environment details.                        |
+--------------+--------------------------------------------------------------+

Example (DB Schema Version)
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_stats.sh -v csmdb
 ------------------------------------------------------------------------------------------------------------------------
 [Start ] Welcome to CSM datatbase automation stats script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info  ] Log Dir: /var/log/ibm/csm/db/csm_db_stats.log
 ------------------------------------------------------------------------------------------------------------------------
                                                       version
 -------------------------------------------------------------------------------------------------
  PostgreSQL 9.2.18 on powerpc64le-redhat-linux-gnu, compiled by gcc (GCC) 4.8.5 20150623 (Red Hat 4.8.5-9), 64-bit
 (1 row)
 ------------------------------------------------------------------------------------------------------------------------

.. note:: This query provides the current version of PostgreSQL installed on the system along with environment details.

9. DB Archiving Stats
-------------------------------

.. code-block:: bash

 /opt/ibm/csm/db/./csm_db_stats.sh –a <my_db_name>
 /opt/ibm/csm/db/./csm_db_stats.sh --indexanalysis <my_db_name>

Example (Query details)
^^^^^^^^^^^^^^^^^^^^^^^
+-----------------------+--------------------------------------------+
|   Column_Name         |            Description                     |
+-----------------------+--------------------------------------------+
| ``table_name``        | Table name.                                |
+-----------------------+--------------------------------------------+
| ``total_rows``        | Total Rows in DB.                          |
+-----------------------+--------------------------------------------+
| ``not_archived``      | Total rows not archived in the DB.         |
+-----------------------+--------------------------------------------+
| ``archived``          | Total rows archived in the DB.             |
+-----------------------+--------------------------------------------+
| ``last_archive_time`` | Last archived process time.                |
+-----------------------+--------------------------------------------+

.. warning:: This query could take several minutes to execute depending on the total size of each table.

Example (DB archive count with details)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_stats.sh -a csmdb
 ------------------------------------------------------------------------------------------------------------------------
 [Start ] Welcome to CSM datatbase automation stats script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info  ] Log Dir: /var/log/ibm/csm/db/csm_db_stats.log
 ------------------------------------------------------------------------------------------------------------------------
 	  table_name            | total_rows | not_archived | archived | last_archive_time
 -------------------------------+------------+--------------+----------+----------------------------
  csm_allocation_history        |      94022 |            0 |    94022 | 2018-10-09 16:00:01.912545
  csm_allocation_node_history   |   73044162 |            0 | 73044162 | 2018-10-09 16:00:02.06098
  csm_allocation_state_history  |     281711 |            0 |   281711 | 2018-10-09 16:01:03.685959
  csm_config_history            |          0 |            0 |        0 |
  csm_db_schema_version_history |          2 |            0 |        2 | 2018-10-03 10:38:45.294172
  csm_diag_result_history       |         12 |            0 |       12 | 2018-10-03 10:38:45.379335
  csm_diag_run_history          |          8 |            0 |        8 | 2018-10-03 10:38:45.464976
  csm_dimm_history              |      76074 |            0 |    76074 | 2018-10-03 10:38:45.550827
  csm_gpu_history               |      58773 |            0 |    58773 | 2018-10-03 10:38:47.486974
  csm_hca_history               |      23415 |            0 |    23415 | 2018-10-03 10:38:50.574223
  csm_ib_cable_history          |          0 |            0 |        0 |
  csm_lv_history                |          0 |            0 |        0 |
  csm_lv_update_history         |          0 |            0 |        0 |
  csm_node_history              |     536195 |            0 |   536195 | 2018-10-09 14:10:40.423458
  csm_node_state_history        |     966991 |            0 |   966991 | 2018-10-09 15:30:40.886846
  csm_processor_socket_history  |          0 |            0 |        0 |
  csm_ras_event_action          |    1115253 |            0 |  1115253 | 2018-10-09 15:30:50.514246
  csm_ssd_history               |       4723 |            0 |     4723 | 2018-10-03 10:39:47.963564
  csm_ssd_wear_history          |          0 |            0 |        0 |
  csm_step_history              |     456080 |            0 |   456080 | 2018-10-09 16:01:05.797751
  csm_step_node_history         |   25536362 |            0 | 25536362 | 2018-10-09 16:01:06.216121
  csm_switch_history            |          0 |            0 |        0 |
  csm_switch_inventory_history  |          0 |            0 |        0 |
  csm_vg_history                |       4608 |            0 |     4608 | 2018-10-03 10:44:25.837201
  csm_vg_ssd_history            |       4608 |            0 |     4608 | 2018-10-03 10:44:26.047599
 (25 rows)
 ------------------------------------------------------------------------------------------------------------------------

.. note:: This query provides statistical information related to the DB archiving count and processing time.


10. DB Delete Count Stats
-------------------------------

.. code-block:: bash

 /opt/ibm/csm/db/./csm_db_stats.sh –d <my_db_name> <interval_time> (in minutes - example 43800 mins = 1 month)
 /opt/ibm/csm/db/./csm_db_stats.sh --deletecount <my_db_name> <interval_time>

Example (Query details)
^^^^^^^^^^^^^^^^^^^^^^^
+-----------------------+--------------------------------------------------------+
|   Column_Name         |            Description                                 |
+-----------------------+--------------------------------------------------------+
| ``Table Name``        | Table name.                                            |
+-----------------------+--------------------------------------------------------+
| ``Time interval``     | The time (in mins.) of oldest records which to delete. |
+-----------------------+--------------------------------------------------------+
| ``Total Records``     | Returns total records which would be delete.           |
+-----------------------+--------------------------------------------------------+

.. warning:: This query could take several minutes to execute depending on the total size of each table.

Example (DB record count delete estimator)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

  -bash-4.2$ ./csm_db_stats.sh -d csmdb 1
 ------------------------------------------------------------------------------------------------------------------------
 [Start ] Welcome to CSM datatbase automation stats script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info  ] Log Dir: /var/log/ibm/csm/db/csm_db_stats.log
 ------------------------------------------------------------------------------------------------------------------------
 [Info  ]   Table Name:                      | Time interval:     | Total Records:
 ------------------------------------------------------------------------------------------------------------------------
 [Info  ]   csm_allocation_history           | 1 (mins)           |      0
 [Info  ]   csm_allocation_node_history      | 1 (mins)           |      0
 [Info  ]   csm_allocation_state_history     | 1 (mins)           |      0
 [Info  ]   csm_config_history               | 1 (mins)           |      0
 [Info  ]   csm_db_schema_version_history    | 1 (mins)           |      0
 [Info  ]   csm_diag_result_history          | 1 (mins)           |      0
 [Info  ]   csm_diag_run_history             | 1 (mins)           |      0
 [Info  ]   csm_dimm_history                 | 1 (mins)           |      0
 [Info  ]   csm_gpu_history                  | 1 (mins)           |      0
 [Info  ]   csm_hca_history                  | 1 (mins)           |      0
 [Info  ]   csm_ib_cable_history             | 1 (mins)           |      0
 [Info  ]   csm_lv_history                   | 1 (mins)           |      0
 [Info  ]   csm_lv_update_history            | 1 (mins)           |      0
 [Info  ]   csm_node_history                 | 1 (mins)           |      0
 [Info  ]   csm_node_state_history           | 1 (mins)           |      0
 [Info  ]   csm_processor_socket_history     | 1 (mins)           |      0
 [Info  ]   csm_ssd_history                  | 1 (mins)           |      0
 [Info  ]   csm_ssd_wear_history             | 1 (mins)           |      0
 [Info  ]   csm_step_history                 | 1 (mins)           |      0
 [Info  ]   csm_step_node_history            | 1 (mins)           |      0
 [Info  ]   csm_switch_history               | 1 (mins)           |      0
 [Info  ]   csm_switch_inventory_history     | 1 (mins)           |      0
 [Info  ]   csm_vg_history                   | 1 (mins)           |      0
 [Info  ]   csm_vg_ssd_history               | 1 (mins)           |      0
 [Info  ]   csm_ras_event_action             | 1 (mins)           |      0
 ------------------------------------------------------------------------------------------------------------------------

.. note:: This query provides statistical information related to the DB deletion script count. It provides a system admin or analyst accurate information of the total records which would be deleted based on a specific time range.

If the user does not specify the time interval then an error message will display.

.. code-block:: bash

  -bash-4.2$ ./csm_db_stats.sh -d csmdb
 ------------------------------------------------------------------------------------------------------------------------
 [Start ] Welcome to CSM datatbase automation stats script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info  ] Log Dir: /tmp/csm_db_stats.log
 ------------------------------------------------------------------------------------------------------------------------
 [Error ] Please specify the time interval
 [Info  ] Example: ./csm_db_stats.sh -d csmdb 1 [min(s)]
 ------------------------------------------------------------------------------------------------------------------------

11. DB Vacuum Stats
-------------------------------

.. code-block:: bash

 /opt/ibm/csm/db/./csm_db_stats.sh –k <my_db_name>
 /opt/ibm/csm/db/./csm_db_stats.sh --vacuumstats <my_db_name>

Example (Query details)
^^^^^^^^^^^^^^^^^^^^^^^
+-----------------------+---------------------------------------------------------------------------------+
|   Column_Name         |            Description                                                          |
+-----------------------+---------------------------------------------------------------------------------+
| ``table_name``        | Table name.                                                                     |
+-----------------------+---------------------------------------------------------------------------------+
| ``last_vacuum``       | Last time at which this table was manually vacuumed (not counting VACUUM FULL). |
+-----------------------+---------------------------------------------------------------------------------+
| ``last_autovacuum``   | Last time at which this table was vacuumed by the autovacuum daemon.            |
+-----------------------+---------------------------------------------------------------------------------+
| ``last_analyze``      | Last time at which this table was manually analyzed.                            |
+-----------------------+---------------------------------------------------------------------------------+
| ``last_autoanalyze``  | Last time at which this table was analyzed by the autovacuum daemon.            |
+-----------------------+---------------------------------------------------------------------------------+

Example (DB vacuum stats)
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_stats.sh -k bills_db
 ------------------------------------------------------------------------------------------------------------------------
 [Start ] Welcome to CSM datatbase automation stats script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info  ] Log Dir: /var/log/ibm/csm/db/csm_db_stats.log
 ------------------------------------------------------------------------------------------------------------------------
           table_name           |          last_vacuum          |        last_autovacuum        |         last_analyze          |       last_autoanalyze
 -------------------------------+-------------------------------+-------------------------------+-------------------------------+-------------------------------
  csm_gpu_history               | 2018-12-02 01:51:52.40607-05  | 2019-01-04 09:21:37.459804-05 |                               | 2019-01-04 09:03:08.694441-05
  csm_step_node_history         | 2018-12-02 01:51:44.219276-05 | 2018-11-03 21:28:09.828034-04 |                               | 2018-12-02 01:52:30.650538-05
  csm_step                      | 2018-12-02 01:16:25.870369-05 | 2018-10-25 16:56:00.464431-04 |                               | 2018-10-25 16:56:00.4645-04
  csm_step_history              | 2018-12-02 01:16:25.855684-05 | 2018-11-04 02:45:05.236657-05 |                               | 2018-12-02 01:22:45.147226-05
  csm_allocation_state_history  | 2018-12-01 22:59:33.799716-05 | 2018-11-29 21:29:29.254385-05 | 2018-10-31 15:26:34.900721-04 | 2018-12-02 01:18:57.064656-05
  csm_vg_ssd_history            | 2018-12-02 01:52:00.202179-05 |                               |                               |
  csm_switch                    | 2018-12-02 01:52:00.334302-05 |                               |                               |
  csm_switch_history            | 2018-12-02 04:52:17.664809-05 |                               |                               |
  csm_diag_run_history          | 2018-12-02 00:06:47.725711-05 |                               |                               |
  csm_processor_socket_history  | 2018-12-02 04:52:17.664267-05 | 2019-01-04 08:57:40.818141-05 |                               | 2019-01-04 08:51:11.275687-05
  csm_node_state_history        | 2018-12-11 08:38:59.515742-05 | 2018-11-30 04:46:26.791063-05 | 2018-10-31 15:32:35.631237-04 | 2018-12-02 01:20:09.060344-05
  csm_ib_cable_history          | 2018-12-02 01:52:00.333189-05 |                               |                               |
  csm_switch_inventory          | 2018-12-02 01:52:00.334752-05 |                               |                               |
  csm_node_history              | 2018-12-11 08:43:11.872734-05 | 2018-11-30 06:52:44.334165-05 | 2018-10-31 15:31:47.598393-04 | 2018-12-01 21:16:11.64458-05
  csm_diag_result               | 2018-12-02 04:52:15.50969-05  |                               |                               |
  csm_vg_history                | 2018-12-02 01:52:00.201805-05 |                               |                               |
  csm_node                      | 2018-12-02 01:52:00.243946-05 |                               |                               |
  csm_lv_history                | 2018-12-02 01:52:00.25459-05  |                               |                               |
  csm_hca_history               | 2018-12-02 01:51:55.096052-05 | 2019-01-04 09:16:14.732882-05 |                               | 2019-01-04 09:17:45.783809-05
  csm_ssd_history               | 2018-12-02 01:52:00.179348-05 | 2019-01-04 09:18:52.69561-05  |                               | 2019-01-04 09:02:52.365255-05
  csm_vg                        | 2018-12-02 01:52:00.255147-05 |                               |                               |
  csm_diag_run                  | 2018-12-02 00:06:47.753997-05 |                               |                               |
  csm_vg_ssd                    | 2018-12-02 01:52:00.255371-05 |                               |                               |
  csm_dimm_history              | 2018-12-02 01:51:57.461387-05 | 2019-01-04 09:13:25.641405-05 |                               | 2019-01-04 08:53:14.499878-05
  csm_gpu                       | 2018-12-02 04:52:15.507073-05 |                               |                               |
  csm_ssd_wear_history          | 2018-12-02 01:52:00.179796-05 |                               |                               |
  csm_config                    | 2018-12-02 04:52:15.50911-05  |                               |                               |
  csm_ssd                       | 2018-12-02 04:52:15.472697-05 |                               |                               |
  csm_allocation_node           | 2018-12-02 01:52:00.244288-05 | 2018-10-25 16:56:00.47555-04  |                               | 2018-10-25 16:56:00.475655-04
  csm_allocation_node_history   | 2018-12-02 00:06:47.71226-05  | 2018-11-03 08:15:52.870707-04 |                               | 2018-12-02 01:22:10.362188-05
  csm_hca                       | 2018-12-02 04:52:15.507982-05 |                               |                               |
  csm_switch_inventory_history  | 2018-12-02 01:52:00.333537-05 |                               |                               |
  csm_config_bucket             | 2018-12-02 01:52:00.334076-05 |                               |                               |
  csm_processor_socket          | 2018-12-02 04:52:15.506159-05 |                               |                               |
  csm_ras_type                  | 2018-12-13 14:29:01.272399-05 | 2018-12-13 14:48:46.295323-05 |                               | 2019-02-05 14:11:43.368311-05
  csm_ib_cable                  | 2018-12-02 01:52:00.334527-05 |                               |                               |
  csm_dimm                      | 2018-12-02 04:52:15.508856-05 |                               |                               | 2018-10-25 16:52:00.289175-04
  csm_db_schema_version_history | 2018-12-02 01:52:00.363837-05 |                               |                               | 2018-12-02 00:27:34.520137-05
  csm_allocation_history        | 2018-12-02 04:52:15.372503-05 | 2018-11-24 19:33:37.983472-05 | 2018-10-31 15:29:11.390241-04 | 2018-12-02 04:54:48.182816-05
  csm_allocation                | 2018-12-02 01:52:00.201378-05 | 2018-10-25 16:56:00.442126-04 |                               | 2018-10-25 16:56:00.442232-04
  csm_lv_update_history         | 2018-12-02 01:52:00.254908-05 |                               |                               |
  csm_config_history            | 2018-12-02 01:52:00.333859-05 |                               |                               |
  csm_ras_event_action          | 2018-12-02 08:16:49.327258-05 | 2018-12-13 16:28:03.802508-05 |                               | 2018-12-13 16:17:22.491009-05
  csm_diag_result_history       | 2018-12-02 00:06:47.754361-05 |                               |                               |
  csm_step_node                 | 2018-12-02 01:52:00.244617-05 | 2018-10-25 16:56:00.453388-04 |                               | 2018-10-25 16:56:00.453442-04
  csm_ras_type_audit            | 2018-12-13 11:39:26.292136-05 | 2018-12-13 16:13:05.113418-05 |                               | 2019-02-05 14:11:43.413383-05
  csm_db_schema_version         | 2018-12-02 04:52:15.45307-05  |                               |                               |
  csm_lv                        | 2018-12-02 04:52:15.509398-05 |                               |                               |
 (48 rows)
 ------------------------------------------------------------------------------------------------------------------------

