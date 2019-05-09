.. _CSM_Database_schema_version_upgrade:

Using csm_db_schema_version_upgrade_17_0.sh
===========================================

.. important:: Prior steps before migrating to the newest DB schema version.

#. Stop all CSM daemons
#. Run a cold backup of the csmdb or specified DB (:ref:`csm_db_backup_script_v1.sh <csm_db_backup_script_v1.sh>`)
#. Install the newest RPMs
#. Run the :ref:`csm_db_schema_version_upgrade_17_0.sh <usage>`
#. Start CSM daemons

.. attention:: To migrate the CSM database from ``15.0, 15.1, 16.0, 16.1, or 16.2`` to the newest schema version

.. code-block:: bash

 /opt/ibm/csm/db/csm_db_schema_version_upgrade_17_0.sh <my_db_name>
 
.. note:: The ``csm_db_schema_version_upgrade_17_0.sh`` script creates a log file: ``/var/log/ibm/csm/csm_db_schema_upgrade_script.log``

| This script upgrades the CSM (or other specified) DB to the newest schema version (``17.0``).

| The script has the ability to alter tables, field types, indexes, triggers, functions, and any other relevant DB updates or requests. The script will only modify or add specificfields to the database and never eliminating certain fields.
  
.. note:: For a quick overview of the script functionality:

.. code-block:: bash

 /opt/ibm/csm/db/ csm_db_schema_version.sh –h
 /opt/ibm/csm/db/ csm_db_schema_version.sh --help
 If the script is ran without any options, then the usage function is displayed.

.. _usage:

Usage Overview
--------------

If a database name is not present then the usage message will appear with an ``[Error  ]`` message which is included below.

.. code-block:: bash

 -bash-4.2$ ./csm_db_schema_version_upgrade_17_0.sh -h
 ------------------------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM database schema version upgrade schema script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_schema_upgrade_script.log
 [Error   ] Please specify DB name
 ------------------------------------------------------------------------------------------------------------------------
 [Info    ] csm_db_schema_version_upgrade.sh : Load CSM DB upgrade schema file
 [Usage   ] csm_db_schema_version_upgrade.sh : csm_db_schema_version_upgrade.sh [DBNAME]
 ------------------------------------------------------------------------------------------------------------------------
   Argument       |  DB Name  | Description
 -----------------|-----------|------------------------------------------------------------------------------------------
   script_name    | [db_name] | Imports sql upgrades to csm db table(s) (appends)
                  |           | fields, indexes, functions, triggers, etc
 -----------------|-----------|------------------------------------------------------------------------------------------
 ------------------------------------------------------------------------------------------------------------------------

Upgrading CSM DB (manual process)
---------------------------------

.. note:: To upgrade the CSM or specified DB:

.. code-block:: bash
 
 /opt/ibm/csm/db/csm_db_schema_version_upgrade_17_0.sh <my_db_name> (where my_db_name is the name of your DB).
 
.. note:: The script will check to see if the given DB name exists. If the database name does not exist, then it will exit with an error message.

Example (non DB existence):
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_schema_version_upgrade_17_0.sh csmdb
 ------------------------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM database schema version upgrate script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_schema_upgrade_script.log
 [Error   ] PostgreSQL may not be installed or DB: asdf may not exist.
 [Error   ] Please check configuration settings or psql -l
 ------------------------------------------------------------------------------------------------------------------------

.. note::
  The script will check for the existence of these files:
   * ``csm_db_schema_version_data.csv``
   * ``csm_create_tables.sql``
   * ``csm_create_triggers.sql``
 
When an upgrade process happens, the new RPM will consist of a new schema version csv, DB create tables file, and or create triggers/functions file to be loaded into a (completley new) DB.
 
| Once these files have been updated then the migration script can be executed.  There is a built in check that does a comparison againt the DB schema version and the associated files. (These are just a couple of the check processes that takes place)

.. note:: The same error message will prompt if the csm_create_tables.sql and or csm_create_triggers.sql file(s) do not exist in the directory.

Example (non file existence):
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_schema_version_upgrade_17_0.sh csmdb
 ------------------------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM database schema version upgrate script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_schema_upgrade_script.log
 [Info    ] csmdb current_schema_version is running: 16.2
 ------------------------------------------------------------------------------------------------------------------------
 [Error   ] Cannot perform action because the csm_db_schema_version_data.csv file does not exist.
 ------------------------------------------------------------------------------------------------------------------------

.. note:: The second check makes sure the file exists and compares the actual SQL upgrade version to the hardcoded version number. If the criteria is met successfully, then the script will proceed.  If the process fails, then an error message will prompt.

Example (non compatible migration):
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_schema_version_upgrade_17_0.sh csmdb
 ------------------------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM database schema version upgrate script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_schema_upgrade_script.log
 [Info    ] csmdb current_schema_version is running: 16.2
 ------------------------------------------------------------------------------------------------------------------------
 [Error   ] Cannot perform action because not compatible.
 [Info    ] Required: appropriate files in directory
 [Info    ] csm_create_tables.sql file currently in the directory is: 16.2 (required version) 17.0
 [Info    ] csm_create_triggers.sql file currently in the directory is: 17.0 (required version) 17.0
 [Info    ] csm_db_schema_version_data.csv file currently in the directory is: 17.0 (required version) 17.0
 [Info    ] Please make sure you have the latest RPMs installed and latest DB files.
 ------------------------------------------------------------------------------------------------------------------------

.. note:: If the user selects the ``"n/no"`` option when prompted to migrate to the newest DB schema upgrade, then the program will exit with the message below.

Example (user prompt execution with “n/no” option):
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_schema_version_upgrade_17_0.sh csmdb
 ------------------------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM database schema version upgrate script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_schema_upgrade_script.log
 [Info    ] csmdb current_schema_version is running: 16.2
 ------------------------------------------------------------------------------------------------------------------------
 [Warning ] This will migrate csmdb database to schema version 17.0. Do you want to continue [y/n]?:
 [Info    ] User response: n
 [Error   ] Migration session for DB: csmdb User response: ****(NO)****  not updated
 ------------------------------------------------------------------------------------------------------------------------

.. note:: If the user selects the ``"y/yes"`` option when prompted to migrate to the newest DB schema upgrade, then the program will begin execution. An additional section has been added to the migration script to update existing ras message types or to insert new cases.  The user will have to specify ``y/yes`` for these changes or ``n/no`` to skip the process. If there are no changes to the RAS message types or no new cases then the information will be displayed accordingly.

Example (user prompt execution with “y/yes” options for both):
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_schema_version_upgrade_17_0.sh csmdb
 ------------------------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM database schema version upgrade script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_schema_upgrade_script.log
 [Info    ] csmdb current_schema_version is running: 16.2
 ------------------------------------------------------------------------------------------------------------------------
 [Warning ] This will migrate csmdb database to schema version 17.0. Do you want to continue [y/n]?:
 [Info    ] User response: y
 [Info    ] csmdb migration process begin.
 ------------------------------------------------------------------------------------------------------------------------
 [Info    ] Migration from 16.2 to 17.0 [Complete]
 ------------------------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM database ras type automation script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_ras_type_script.log
 [Info    ] csm_ras_type_data.csv file exists
 [Warning ] This will load and or update csm_ras_type table data into csmdb database. Do you want to continue [y/n]?
 [Info    ] User response: y
 [Info    ] csm_ras_type record count before script execution:   744
 [Info    ] Record import count from csm_ras_type_data.csv: 744
 [Info    ] Record update count from csm_ras_type_data.csv: 0
 [Info    ] csm_ras_type live row count after script execution: 744
 [Info    ] csm_ras_type_audit live row count: 744
 [Info    ] Database: csmdb csv upload process complete for csm_ras_type table.
 ------------------------------------------------------------------------------------------------------------------------
 [End     ] Database: csmdb csv upload process complete for csm_ras_type table.
 ------------------------------------------------------------------------------------------------------------------------
 [Complete] csmdb database schema update 17.0.
 ------------------------------------------------------------------------------------------------------------------------
 [Timing  ] 0:00:00:6.3400
 ------------------------------------------------------------------------------------------------------------------------

Example (user prompt execution with “y/yes” for the migration and “n/no” for the RAS section):
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_schema_version_upgrade_17_0.sh csmdb
 ------------------------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM database schema version upgrade script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_schema_upgrade_script.log
 [Info    ] csmdb current_schema_version is running: 16.2
 [Info    ] -------------------------------------------------------------------------------------------------------------
 [Warning ] This will migrate csmdb database to schema version 17.0. Do you want to continue [y/n]?:
 [Info    ] User response: y
 [Info    ] csmdb migration process begin.
 [Info    ] -------------------------------------------------------------------------------------------------------------
 [Info    ] Migration from 16.2 to 17.0 [Complete]
 ------------------------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM database ras type automation script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_ras_type_script.log
 [Info    ] csm_ras_type_data.csv file exists
 [Warning ] This will load and or update csm_ras_type table data into csmdb database. Do you want to continue [y/n]?
 [Info    ] User response: n
 [Info    ] Skipping the csm_ras_type table data import/update process
 ------------------------------------------------------------------------------------------------------------------------
 [End     ] Database: csmdb csv upload process complete for csm_ras_type table.
 ------------------------------------------------------------------------------------------------------------------------
 [Complete] csmdb database schema update 17.0.
 ------------------------------------------------------------------------------------------------------------------------
 [Timing  ] 0:00:00:3.4347
 ------------------------------------------------------------------------------------------------------------------------

.. attention:: It is not recommended to select ``n/no`` for the RAS section during the migration script process.  If this process does occur, then the RAS script can be ran alone by the system admin.

To run the RAS script by itself please refer to link: :ref:`csm_ras_type_script_sh <csm_ras_type_script_usage>`

.. note:: If the migration script has already ran already or a new database has been created with the latest schema version of ``17.0`` then this message will be prompted to the user.
 
Running the script with existing newer version
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. code-block:: bash
 
 -bash-4.2$ ./csm_db_schema_version_upgrade_17_0.sh csmdb
 ------------------------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM database schema version upgrade script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info    ] Log Dir: /tmp/csm_db_schema_upgrade_script.log
 [Info    ] -------------------------------------------------------------------------------------------------------------
 [Info    ] csmdb is currently running db schema version: 17.0
 ------------------------------------------------------------------------------------------------------------------------

.. warning:: If there are existing DB connections, then the migration script will prompt a message and the admin will have to kill connections before proceeding.

.. hint:: The csm_db_connections_script.sh script can be used with the –l option to quickly list the current connections. (Please see user guide or ``–h`` for usage function).  This script has the ability to terminate user sessions based on pids, users, or a ``–f`` force option will kill all connections if necessary.  Once the connections are terminated then the ``csm_db_schema_version_upgrade_17_0.sh`` script can be executed. The log message will display current connection of user, database name, connection count, and duration.

Example (user prompt execution with “y/yes” option and existing DB connection(s)):
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_schema_version_upgrade_17_0.sh csmdb
 ------------------------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM database schema version upgrate script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info    ] Log Dir: /tmp/csm_db_schema_upgrade_script.log
 [Info    ] csmdb current_schema_version is running: 16.2
 [Info    ] -------------------------------------------------------------------------------------------------------------
 [Error   ] csmdb has existing connection(s) to the database.
 [Error   ] User: csmdb has 1 connection(s)
 [Info    ] See log file for connection details
 ------------------------------------------------------------------------------------------------------------------------

Running the script with older schema versions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. attention:: It is possible to migrate older database versions to the latest schema release (ex. 17.0). Supporting databased include version 15.0, 15.1, 16.0, 16.1, and 16.2. The migration script will check previous versions and update accordingly. This script only supports bringing a previous version to the latest version, so if the current database version is at 15.0 then it will migrate to 17.0.

.. code-block:: bash

 -bash-4.2$ ./csm_db_schema_version_upgrade_17_0.sh csmdb
 ------------------------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM database schema version upgrade script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_schema_upgrade_script.log
 [Info    ] csmdb current_schema_version is running: 15.0
 [Info    ] -------------------------------------------------------------------------------------------------------------
 [Info    ] There are critical migration steps needed to get to the latest schema version: 17.0
 [Info    ] These include versions 15.1, 16.0, 16.1 and 16.2
 [Warning ] Do you want to continue [y/n]?:
 [Info    ] User response: y
 [Info    ] csmdb migration process begin.
 [Info    ] -------------------------------------------------------------------------------------------------------------
 [Info    ] Migration from 15.0 to 16.2 [Complete]
 [Info    ] -------------------------------------------------------------------------------------------------------------
 [Info    ] Migration from 16.2 to 17.0 [Complete]
 ------------------------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM database ras type automation script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_ras_type_script.log
 [Info    ] csm_ras_type_data.csv file exists
 [Warning ] This will load and or update csm_ras_type table data into csmdb database. Do you want to continue [y/n]?
 [Info    ] User response: y
 [Info    ] csm_ras_type record count before script execution:   744
 [Info    ] Record import count from csm_ras_type_data.csv: 744
 [Info    ] Record update count from csm_ras_type_data.csv: 0
 [Info    ] csm_ras_type live row count after script execution: 744
 [Info    ] csm_ras_type_audit live row count: 744
 ------------------------------------------------------------------------------------------------------------------------
 [End     ] Database: csmdb csv upload process complete for csm_ras_type table.
 ------------------------------------------------------------------------------------------------------------------------
 [Complete] csmdb database schema update 17.0.
 ------------------------------------------------------------------------------------------------------------------------
 [Timing  ] 0:00:00:2.7539
 ------------------------------------------------------------------------------------------------------------------------
