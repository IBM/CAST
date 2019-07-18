Using csm_db_script.sh
======================

.. note::
  For a quick overview of the script functionality:
  
.. code-block:: bash

 /opt/ibm/csm/db/csm_db_script.sh –h
 /opt/ibm/csm/db/csm_db_script.sh --help

 This help command <-h, --help> specifies each of the options available to use.

Usage Overview
--------------

+--------------------------------------+---------------------------------------------------+--------------------------------------------------+
|     A new DB set up <default db>     |                      Command                      |                      Result                      |
+======================================+===================================================+==================================================+
| running the script with no options   | ./csm_db_script.sh                                | This will create a default db with tables and    |
|                                      |                                                   | populated data <specified by user or db admin>   |
+--------------------------------------+---------------------------------------------------+--------------------------------------------------+
| running the script with              | ./csm_db_script.sh –x                             | This will create a default db with tables and no |
| –x, --nodata                         | ./csm_db_script.sh --nodata                       | populated data                                   |
+--------------------------------------+---------------------------------------------------+--------------------------------------------------+

+--------------------------------------+---------------------------------------------------+--------------------------------------------------+
|     A new DB set up <new user db>    |                      Command                      |                      Result                      |
+======================================+===================================================+==================================================+
| running the script with              | ./csm_db_script.sh -n <my_db_name>                | This will create a new db with tables            |
| –n, --newdb                          | ./csm_db_script.sh --newdb <my_db_name>           | and populated data.                              |
+--------------------------------------+---------------------------------------------------+--------------------------------------------------+
| running the script with              | ./csm_db_script.sh -n <my_db_name> –x             | This will create a new db with tables            |
| –n, --newdb, -x, --nodata            | ./csm_db_script.sh --newdb <my_db_name> --nodata  | and no populated data.                           |
+--------------------------------------+---------------------------------------------------+--------------------------------------------------+

+--------------------------------------+---------------------------------------------------+--------------------------------------------------+
|        If a DB already exists        |                      Command                      |                      Result                      |
+======================================+===================================================+==================================================+
| Drop DB totally                      | ./csm_db_script.sh -d <my_db_name>                | This will totally remove the DB                  |
|                                      | ./csm_db_script.sh --delete <my_db_name>          | from the system                                  |
+--------------------------------------+---------------------------------------------------+--------------------------------------------------+
| Drop only the existing CSM DB tables | ./csm_db_script.sh -e <my_db_name>                | This will only drop the specified CSM DB tables. |
|                                      | ./csm_db_script.sh --eliminatetables <my_db_name> | <useful if integrated within another DB          |
|                                      |                                                   | <e.x. “XCATDB”>                                  |
+--------------------------------------+---------------------------------------------------+--------------------------------------------------+
| Force overwrite of existing DB.      | ./csm_db_script.sh -f <my_db_name>                | This will totally drop the existing tables       |
|                                      | ./csm_db_script.sh --force <my_db_name>           | in the DB and recreate them with populated       |
|                                      |                                                   | table data.                                      |
+--------------------------------------+---------------------------------------------------+--------------------------------------------------+
| Force overwrite of existing DB.      | ./csm_db_script.sh -f <my_db_name> –x             | This will totally drop the existing tables       |
|                                      | ./csm_db_script.sh --force <my_db_name> --nodata  | in the DB and recreate them without table        |
|                                      |                                                   | data.                                            |
+--------------------------------------+---------------------------------------------------+--------------------------------------------------+
| Remove just the data from all        | ./csm_db_script.sh -r <my_db_name>                | This will totally remove all data                |
| the tables in the DB                 | ./csm_db_script.sh --removetabledata <my_db_name> | from all the tables within the DB.               |
+--------------------------------------+---------------------------------------------------+--------------------------------------------------+

Example (usage)
^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_script.sh -h
 ------------------------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM database automation script.
 [Info    ] Log Dir: /tmp/csm_db_script.log
 -----------------------------------------------------------------------------------------------------------------
 [Info ] csm_db_script.sh : CSM database creation script with additional features
 [Usage] csm_db_script.sh : [OPTION]... [DBNAME]... [OPTION]
 -----------------------------------------------------------------------------------------------------------------
 [Options]
 -----------------------|-----------|-----------------------------------------------------------------------------
   Argument             | DB Name   | Description
 -----------------------|-----------|-----------------------------------------------------------------------------
   -x, --nodata         | [DEFAULT] | creates database with tables and does not pre populate table data
                        | [db_name] | this can also be used with the -f --force, -n --newdb option when
                        |           | recreating a DB. This should follow the specified DB name
   -d, --delete         | [db_name] | totally removes the database from the system
   -e, --eliminatetables| [db_name] | drops CSM tables from the database
   -f, --force          | [db_name] | drops the existing tables in the DB, recreates and populates with table data
   -n, --newdb          | [db_name] | creates a new database with tables and populated data
   -r, --removetabledata| [db_name] | removes data from all database tables
   -h, --help           |           | help
 -----------------------|-----------|-----------------------------------------------------------------------------
 [Examples]
 -----------------------------------------------------------------------------------------------------------------
   [DEFAULT] csm_db_script.sh                         |          |
   [DEFAULT] csm_db_script.sh -x, --nodata            |          |
             csm_db_script.sh -d, --delete            | [DBNAME] |
             csm_db_script.sh -e, --eliminatetables   | [DBNAME] |
             csm_db_script.sh -f, --force             | [DBNAME] |
             csm_db_script.sh -f, --force             | [DBNAME] | -x, --nodata
             csm_db_script.sh -n, --newdb             | [DBNAME] |
             csm_db_script.sh -n, --newdb             | [DBNAME] | -x, --nodata
             csm_db_script.sh -r, --removetabledata   | [DBNAME] |
             csm_db_script.sh -h, --help              |          |
 -----------------------------------------------------------------------------------------------------------------

.. note:: Setting up or creating a new DB <manually>

To create your own DB
---------------------

.. code-block:: bash

 /opt/ibm/csm/db/db_script.sh –n <my_db_name>
 /opt/ibm/csm/db/db_script.sh --newdb <my_db_name>

 By default if no DB name is specified, then the script will
 create a DB called csmdb. 

Example (successful DB creation):
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 $ /opt/ibm/csm/db/csm_db_script.sh
 ------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM database automation script.
 [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_script.log
 [Info    ] PostgreSQL is installed
 [Info    ] csmdb database user: csmdb already exists
 [Complete] csmdb database created.
 [Complete] csmdb database tables created.
 [Complete] csmdb database functions and triggers created.
 [Complete] csmdb table data loaded successfully into csm_db_schema_version
 [Complete] csmdb table data loaded successfully into csm_ras_type
 [Info    ] csmdb DB schema version <16.2>
 ------------------------------------------------------------------------------------------------------

.. note::
 The script checks to see if the given name exists.
 If the database does not exist, then it will be created.
 If the database already exists, then the script prompts an
 error message indicating a database with this name already
 exists and exits the program.

Example (DB already exists)
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 $ /opt/ibm/csm/db/csm_db_script.sh
 ------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM database automation script.
 [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_script.log
 [Info    ] PostgreSQL is installed
 [Error   ] Cannot perform action because the csmdb database already exists. Exiting.
 ------------------------------------------------------------------------------------------------------

2.	The script automatically populates data in specified tables using csv files. 

.. code-block:: bash
        
  For example, ras message type data, into the ras message type table.
  If a user does not want to populate these tables, then they should indicate a
  -x, --nodata in the command line during the initial setup process.

  /opt/ibm/csm/db/csm_db_script.sh -x
  /opt/ibm/csm/db/csm_db_script.sh --nodata

Example (Default DB creation without loaded data option)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 $ /opt/ibm/csm/db/csm_db_script.sh –x
 ------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM database automation script.
 [Info    ] PostgreSQL is installed
 [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_script.log
 [Info    ] csmdb database user: csmdb already exists
 [Complete] csmdb database created.
 [Complete] csmdb database tables created.
 [Complete] csmdb database functions and triggers created.
 [Info    ] csmdb skipping data load process.  <----------[when running the -x, --nodata option]
 [Complete] csmdb initialized csm_db_schema_version data
 [Info    ] csmdb DB schema version <16.2>
 ------------------------------------------------------------------------------------------------------

Existing DB Options
-------------------

.. note::
 There are some other features in this script that will assist users in a “clean-up” process.
 If the database already exists, then these actions will work.

1.    Delete the database

.. code-block:: bash
 
 /opt/ibm/csm/db/csm_db_script.sh –d <my_db_name>
 /opt/ibm/csm/db/csm_db_script.sh --delete <my_db_name>

Example (Delete existing DB)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 $ /opt/ibm/csm/db/csm_db_script.sh –d csmdb
 ------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM database automation script.
 [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_script.log
 [Info    ] PostgreSQL is installed
 [Info    ] This will drop csmdb database including all tables and data. Do you want to continue [y/n]?y
 [Complete] csmdb database deleted
 ------------------------------------------------------------------------------------------------------

2.    Remove just data from all the tables
 
.. code-block:: bash
 
 /opt/ibm/csm/db/csm_db_script.sh –r <my_db_name>
 /opt/ibm/csm/db/csm_db_script.sh --removetabledata <my_db_name>

Example (Remove data from DB tables)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 $ /opt/ibm/csm/db/csm_db_script.sh –r csmdb
 ------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM database automation script.
 [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_script.log
 [Info    ] PostgreSQL is installed
 [Complete] csmdb database data deleted from all tables excluding csm_schema_version and csm_db_schema_version_history tables
 ------------------------------------------------------------------------------------------------------

3.   Force a total overwrite of the database <drops tables and recreates them>.

.. code-block:: bash

 /opt/ibm/csm/db/csm_db_script.sh –f <my_db_name> 
 /opt/ibm/csm/db/csm_db_script.sh --force <my_db_name> (which auto populates table data).

Example (Force DB receation)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 $ /opt/ibm/csm/db/csm_db_script.sh –f csmdb
 ------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM database automation script.
 [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_script.log
 [Info    ] PostgreSQL is installed
 [Info    ] csmdb database user: csmdb already exists
 [Complete] csmdb database tables and triggers dropped
 [Complete] csmdb database functions dropped
 [Complete] csmdb database tables recreated.
 [Complete] csmdb database functions and triggers recreated.
 [Complete] csmdb table data loaded successfully into csm_db_schema_version
 [Complete] csmdb table data loaded successfully into csm_ras_type
 [Info    ] csmdb DB schema version <16.2>
 ------------------------------------------------------------------------------------------------------

4.    Force a total overwrite of the database <drops tables and recreates them without
prepopulated data>.

.. code-block:: bash

 /opt/ibm/csm/db/csm_db_script.sh –f <my_db_name> -x
 /opt/ibm/csm/db/csm_db_script.sh --force <my_db_name --nodata (which does not populate table data).

Example (Force DB recreation without preloaded table data)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 $ /opt/ibm/csm/db/csm_db_script.sh –f csmdb –x
 ------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM database automation script.
 [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_script.log
 [Info    ] PostgreSQL is installed
 [Info    ] csmdb database user: csmdb already exists
 [Complete] csmdb database tables and triggers dropped
 [Complete] csmdb database functions dropped
 [Complete] csmdb database tables recreated.
 [Complete] csmdb database functions and triggers recreated.
 [Complete] csmdb skipping data load process.
 [Complete] csmdb table data loaded successfully into csm_db_schema_version
 [Info    ] csmdb DB schema version <16.2>
 ------------------------------------------------------------------------------------------------------

CSMDB user info.
----------------
5.    The ``"csmdb"`` user will remain in the system unless an admin manually deletes this option.
If the user has to be deleted for any reason the Admin can run this command inside the psql postgres DB connection. ``DROP USER csmdb``.  If any current database are running with this user, then the user will
get a response similar to the example below

.. code-block:: bash

 ERROR:  database "csmdb" is being accessed by other users
 DETAIL:  There is 1 other session using the database.

.. warning::
 It is not recommended to delete the csmdb user.

| If the process has to be done manually then the admin
  can run these commands as logged in a postgres super user.

| Manual process
 As root user log into postgres:

.. code-block:: bash
 
 su – postgres

 psql -t -q -U postgres -d postgres -c "DROP USER csmdb;"
 psql -t -q -U postgres -d postgres -c "CREATE USER csmdb;"


.. note::
 The command below can be executed if specific privileges are needed.

.. code-block:: bash

 psql -t -q -U postgres -d postgres -c "GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA public TO csmdb"

.. note::
 If admin wants to change the ownership of the DB to postgres then use the command below.

.. code-block:: bash

 ALTER DATABASE csmdb OWNER TO postgres
 ALTER DATABASE csmdb OWNER TO csmdb

| The automated DB script will check the existence of the DB user.  If the user is already created then the process will be skipped. 

Please see the log file for details:
``/var/log/ibm/csm/csm_db_script.log``
