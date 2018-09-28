Using csm_db_backup_script_v1.sh
================================

To manually perform a cold backup a CSM database on the system the following script may be run.

.. code-block:: bash

    /opt/ibm/csm/db/csm_db_backup_script_v1.sh
 
.. note:: This script should be run as the root or postgres user. 

.. attention:: There are a few step that should be taken before backing up a CSM or related DB on the system.

Backup script actions
---------------------

The following steps are behaviors recommended for use of the back up script:

1. Stop all CSM daemons.
2. Run the backup script.

:Invocation: ``/opt/ibm/csm/db/csm_db_backup_script_v1.sh [DBNAME] [/DIR/]``
:Default Directory: */var/lib/pgsql/backups/*
    
The script will check the DB connections and if there are no active connections then the backup 
process will begin. If there are any active connections to the DB, an Error message will be 
displayed and the program will exit.

To terminate active connections: :ref:`csm_db_connections_script.sh <csm_db_connections_script_usage>`

3. Once the DB been successfully backed-up then the admin can restart the daemons. 

.. _csm_db_backup_script_v1.sh:

Running the csm_db_backup_script_v1.sh
--------------------------------------

Example (-h, --help)
^^^^^^^^^^^^^^^^^^^^
.. code-block:: bash
 
 ./csm_db_backup_script_v1.sh –h, --help
 ===============================================================================================================
 [Info ] csm_db_backup_script_v1.sh : csmdb /tmp/csmdb_backup/
 [Info ] csm_db_backup_script_v1.sh : csmdb
 [Usage] csm_db_backup_script_v1.sh : [OPTION]... [/DIR/]
 ---------------------------------------------------------------------------------------------------------------
 [Options]
 ----------------|----------------------------------------------------------------------------------------------
   Argument      | Description
 ----------------|----------------------------------------------------------------------------------------------
    -h, --help   | help menu
 ----------------|----------------------------------------------------------------------------------------------
 [Examples]
 ---------------------------------------------------------------------------------------------------------------
 csm_db_backup_script_v1.sh [DBNAME] 		        | (default) will backup database to/var/lib/pgpsql/backups/ (directory)
 csm_db_backup_script_v1.sh [DBNAME] [/DIRECTORY/	| will backup database to specified directory
                                                    	| if the directory doesnt exist then it will be mode and   
                                                        | written.
 ==============================================================================================================

.. attention:: Common errors

If the user tries to run the script as local user without PostgreSQL installed and does not provide a database name:

 #. An info message will prompt ([Info   ] Database name is required)
 #. The usage message will also prompt the usage help menu

Example (no options, usage)
^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. code-block:: bash

 bash-4.1$ ./csm_db_backup_script_v1.sh
 [Info   ] Database name is required
 ===============================================================================================================
 [Info ] csm_db_backup_script_v1.sh : csmdb /tmp/csmdb_backup/
 [Info ] csm_db_backup_script_v1.sh : csmdb
 [Usage] csm_db_backup_script_v1.sh : [OPTION]... [/DIR/]
 ---------------------------------------------------------------------------------------------------------------
 [Options]
 ----------------|----------------------------------------------------------------------------------------------
   Argument      | Description
 ----------------|----------------------------------------------------------------------------------------------
    -h, --help   | help menu
 ----------------|----------------------------------------------------------------------------------------------
 [Examples]
 ---------------------------------------------------------------------------------------------------------------
 csm_db_backup_script_v1.sh [DBNAME]       	       	| (default) will backup database to/var/lib/pgpsql/backups/ (directory)
 csm_db_backup_script_v1.sh [DBNAME] [/DIRECTORY/	| will backup database to specified directory
                                                   	| if the directory doesnt exist then it will be mode and 
                                                        | written.

.. note:: If the user tries to run the script as local user (non-root and postgresql not installed):

Example (postgreSQL not installed)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. code-block:: bash

 bash-4.1$ ./csm_db_backup_script_v1.sh csmdb /tmp/
 -----------------------------------------------------------------------------------------
 [Error ] PostgreSQL may not be installed. Please check configuration settings
 -----------------------------------------------------------------------------------------

.. note:: If the user tries to run the script as local user (non-root and postgresql not installed)and doesnt specify a directory (default directory: ``/var/lib/pgsql/backups``

Example (no directory specified)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. code-block:: bash

 bash-4.1$ ./csm_db_backup_script_v1.sh csmdb
 -----------------------------------------------------------------------------------------
 [Error  ] make directory failed for: /var/lib/pgsql/backups/
 [Info   ] User: csmcarl does not have permission to write to this directory
 [Info   ] Please specify a valid directory
 [Info   ] Or log in as the appropriate user
 -----------------------------------------------------------------------------------------
