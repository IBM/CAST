Using csm_db_stats.sh script
============================

This script will gather statistical information related to the CSM DB which includes, table data activity, index related information, and table lock monitoring, CSM DB schema version, DB connections stats query, DB user stats query, and PostgreSQL version installed .


.. note::
 For a quick overview of the script functionality, 


.. code-block:: bash

 run /opt/ibm/csm/db/csm_db_stats.sh –h, --help.
 This help command <-h, --help> will specify each of the options available to use.

The ``csm_db_stats.sh`` script creates a log file for each query executed. (Please see the log file for details): `` /var/log/ibm/csm/csm_db_stats.log``

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
| --help                       | ./csm_db_stats.sh                              | see details below |
+------------------------------+------------------------------------------------+-------------------+

Example (usage)
^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_stats.sh --help
 =================================================================================================
 [Info ] csm_db_stats.sh : List/Kill database user sessions
 [Usage] csm_db_stats.sh : [OPTION]... [DBNAME]
 -------------------------------------------------------------------------------------------------
   Argument               |  DB Name  | Description
 -------------------------|-----------|-----------------------------------------------------------
  -t, --tableinfo         | [db_name] | Populates Database Table Stats:
                          |           | Live Row Count, Inserts, Updates, Deletes, and Table Size
  -i, --indexinfo         | [db_name] | Populates Database Index Stats:
                          |           | tablename, indexname, num_rows, tbl_size, ix_size, uk,
                          |           | num_scans, tpls_read, tpls_fetched
  -l, --lockinfo          | [db_name] | Displays any locks that might be happening within the DB
  -s, --schemaversion     | [db_name] | Displays the current CSM DB version
  -c, --connectionsdb     | [db_name] | Displays the current DB connections
  -u, --usernamedb        | [db_name] | Displays the current DB user names and privileges
  -v, --postgresqlversion | [db_name] | Displays the current version of PostgreSQL installed
                          |           | along with environment details
  -h, --help              |           | help
 -------------------------|-----------|-----------------------------------------------------------
 [Examples]
 -------------------------------------------------------------------------------------------------
    csm_db_stats.sh -t, --tableinfo          [dbname]    | Database table stats
    csm_db_stats.sh -i, --indexinfo          [dbname]    | Database index stats
    csm_db_stats.sh -l, --lockinfo           [dbname]    | Database lock stats
    csm_db_stats.sh -s, --schemaversion      [dbname]    | Database schema version (CSM_DB only)
    csm_db_stats.sh -c, --connectionsdb      [dbname]    | Database connections stats
    csm_db_stats.sh -u, --usernamedb         [dbname]    | Database user stats
    csm_db_stats.sh -v, --postgresqlversion  [dbname]    | Database (PostgreSQL) version
    csm_db_stats.sh -h, --help               [dbname]    | Help menu
 =================================================================================================
 
1. Table data activity
-----------------------

.. code-block:: bash

 Run: /opt/ibm/csm/db/csm_db_stats.sh –t, --tableinfo [my_db_name]

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
 --------------------------------------------------------------------------------------------------
         relname        | live_row_count | insert_count | update_count | delete_count | table_size
 -----------------------+----------------+--------------+--------------+--------------+------------
  csm_db_schema_version |              1 |            1 |            0 |            0 | 8192 bytes
  csm_gpu               |              4 |            4 |            0 |            0 | 8192 bytes
  csm_hca               |              2 |            2 |            0 |            0 | 8192 bytes
  csm_node              |              2 |            2 |            0 |            0 | 8192 bytes
  csm_ras_type          |              4 |            4 |            0 |            0 | 8192 bytes
  csm_ras_type_audit    |              4 |            4 |            0 |            0 | 8192 bytes
 (6 rows)
 --------------------------------------------------------------------------------------------------

2. Index related information
----------------------------

.. code-block:: bash	
 
 Run: /opt/ibm/csm/db/./csm_db_stats.sh –i, --indexinfo <my_db_name>

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
 --------------------------------------------------------------------------------------------------------------------------
  tablename |   indexname   | num_rows | table_size | index_size | unique | number_of_scans | tuples_read | tuples_fetched
 -----------+---------------+----------+------------+------------+--------+-----------------+-------------+----------------
  csm_node  | ix_csm_node_a |        0 | 8192 bytes | 16 kB      | Y      |              15 |           9 |              9
  csm_gpu   | csm_gpu_pkey  |        0 | 8192 bytes | 16 kB      | Y      |               8 |           4 |              4
  csm_hca   | csm_hca_pkey  |        0 | 8192 bytes | 16 kB      | Y      |               6 |           2 |              2
  csm_ssd   | ix_csm_ssd_a  |        0 | 0 bytes    | 8192 bytes | Y      |               3 |           0 |              0
 (4 rows)
 --------------------------------------------------------------------------------------------------------------------------
 
 3. Table Lock Monitoring
 ------------------------

.. code-block:: bash

 Run: /opt/ibm/csm/db/./csm_db_stats.sh –l, --lockinfo <my_db_name>
 
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
 ---------------------------------------------------------------------------------------------------------------
.. note:: This query displays relevant information related to lock monitoring.  It will display the current blocked and blocking rows affected along with each duration.   A systems administrator can run the query and evaluate what is causing the results of a “hung” procedure and determine the possible issue.

4. DB schema Version Query
--------------------------

.. code-block:: bash

 Run: /opt/ibm/csm/db/./csm_db_stats.sh –s, --schemaversion <my_db_name>
 
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
 -------------------------------------------------------------------------------------
  version |        create_time         |     comment
 ---------+----------------------------+-----------------
   16.0   | 2018-04-04 09:41:57.784378 | current_version
 (1 row)
 -------------------------------------------------------------------------------------
.. note:: This query provides the current database version the system is running along with its creation time.

5. DB Connections with details
------------------------------

.. code-block:: bash

 Run: /opt/ibm/csm/db/./csm_db_stats.sh –c, --connectionsdb <my_db_name>

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
 -----------------------------------------------------------------------------------------------------------------------------------------------------------
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
 -----------------------------------------------------------------------------------------------------------------------------------------------------------

 
.. note:: This query will display information about the database connections that are in use on the system.  The pid (Process ID), database name, user name, backend start time, query start time, state change, waiting status, and query will display statistics about the current database activity.

6. PostgreSQL users with details
--------------------------------

.. code-block:: bash

 Run: /opt/ibm/csm/db/./csm_db_stats.sh –u, --usernamedb <my_db_name>

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
 -----------------------------------------------------------------------------------------------------------------------------------
  rolname  | rolsuper | rolinherit | rolcreaterole | rolcreatedb | rolcatupdate | rolcanlogin | rolreplication | rolconnlimit | rolpassword | rolvaliduntil | rolconfig |  oid
 ----------+----------+------------+---------------+-------------+--------------+-------------+----------------+--------------+-------------+---------------+-----------+--------
  postgres | t        | t          | t             | t           | t            | t           | t              |           -1 | ********    |               |           |     10
  xcatadm  | f        | t          | f             | f           | f            | t           | f              |           -1 | ********    |               |           |  16385
  root     | f        | t          | f             | f           | f            | t           | f              |           -1 | ********    |               |           |  16386
  csmdb    | f        | t          | f             | f           | f            | t           | f              |           -1 | ********    |               |           | 704142
 (4 rows)
 -----------------------------------------------------------------------------------------------------------------------------------

.. note:: This query will display specific information related to the users that are currently in the postgres database.  These fields will appear in the query: rolname, rolsuper, rolinherit, rolcreaterole, rolcreatedb, rolcatupdate, rolcanlogin, rolreplication, rolconnlimit, rolpassword, rolvaliduntil, rolconfig, and oid. See below for details.

7. PostgreSQL Version Installed
-------------------------------

.. code-block:: bash

 Run: /opt/ibm/csm/db/./csm_db_stats.sh –v, --postgresqlversion <my_db_name>

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
 -------------------------------------------------------------------------------------------------
                                                       version
 -------------------------------------------------------------------------------------------------
  PostgreSQL 9.2.18 on powerpc64le-redhat-linux-gnu, compiled by gcc (GCC) 4.8.5 20150623 (Red Hat 4.8.5-9), 64-bit
 (1 row)
 -------------------------------------------------------------------------------------------------

.. note:: This query provides the current version of PostgreSQL installed on the system along with environment details.
