Using csm_ras_type_script.sh
============================

This script is for importing or removing records in the csm_ras_type table.
The csm_db_ras_type_script.sh creates a log file:
``/var/log/ibm/csm/csm_db_ras_type_script.log``

.. note::
 * ``csm_ras_type`` table is pre populated which, contains the description and details for each of the possible RAS event types. This may change over time and new message types can be imported into the table. The script is ran and a temp table is created and appends the csv file data with the current records in the ``csm_ras_type`` table.  If any duplicate (key) values exist in the process, they will get dismissed and the rest of the records get imported.  A total record count is displayed and logged, along with the after live ``csm_ras_type`` count and also for the ``csm_ras_type_audit`` table.

 * A complete cleanse of the ``csm_ras_type`` table may also need to take place.  If this step is necessary then the auto script can be ran with the ``–r`` option.  A ``"y/n"`` prompt will display to the admins to ensure this execution is really what they want. If the ``n`` option is selected then the process is aborted and results are logged accordingly.

Usage Overview
--------------

.. code-block:: bash

 /opt/ibm/csm/db/ csm_db_ras_type_script.sh –h
 /opt/ibm/csm/db/ csm_db_ras_type_script.sh --help
 
.. note:: This help command (``-h, --help``) will specify each of the options available to use.

Example (Usage)
^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_ras_type_script.sh -h
 ------------------------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM datatbase ras type automation script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_ras_type_script.log
 ------------------------------------------------------------------------------------------------------------------------
 [Info ] csm_db_ras_type_script.sh : Load/Remove data from csm_ras_type table
 [Usage] csm_db_ras_type_script.sh : [OPTION]... [DBNAME]... [CSV_FILE]
 ------------------------------------------------------------------------------------------------------------------------
   Argument               |  DB Name  | Description
 -------------------------|-----------|----------------------------------------------------------------------------------
  -l, --loaddata          | [db_name] | Imports CSV data to csm_ras_type table (appends)
                          |           | Live Row Count, Inserts, Updates, Deletes, and Table Size
  -r, --removedata        | [db_name] | Removes all records from the csm_ras_type table
  -h, --help              |           | help
 -------------------------|-----------|----------------------------------------------------------------------------------
 [Examples]
 ------------------------------------------------------------------------------------------------------------------------
    csm_db_ras_type_script.sh -l, --loaddata           [dbname]    | [csv_file_name]
    csm_db_ras_type_script.sh -r, --removedata         [dbname]    |
    csm_db_ras_type_script.sh -h, --help               [dbname]    |
 ------------------------------------------------------------------------------------------------------------------------

.. _csm_ras_type_script_usage:

Importing records into csm_ras_type table (manually)
----------------------------------------------------

1.	To import data to the ``csm_ras_type`` table:

.. code-block:: bash

 /opt/ibm/csm/db/csm_db_ras_type_script.sh –l  my_db_name (where my_db_name is the name of your DB) and the csv_file_name.
 /opt/ibm/csm/db/csm_db_ras_type_script.sh --loaddata my_db_name (where my_db_name is the name of your DB) and the csv_file_name.
 
.. note:: The script will check to see if the given name is available and if the database does not exist then it will exit with an error message.  

Example (non DB existence):
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_ras_type_script.sh -l csmdb csm_ras_type_data.csv
 ------------------------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM datatbase ras type automation script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_ras_type_script.log
 [Error   ] PostgreSQL may not be installed or DB: bills_test_d may not exist.
 [Info    ] Please check configuration settings or psql -l
 ------------------------------------------------------------------------------------------------------------------------

.. note:: Make sure PostgreSQL is installed on the system.
 
Example (non csv_file_name existence):
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_ras_type_script.sh -l csmdb csm_ras_type_data_file.csv
 ------------------------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM datatbase ras type automation script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_ras_type_script.log
 [Error   ] File csm_ras_type_data_file.csv can not be located or doesnt exist
 [Info    ] Please choose another file or check path
 ------------------------------------------------------------------------------------------------------------------------

.. note:: Make sure the latest csv file exists in the appropriate working directory

Example (successful execution):
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_ras_type_script.sh -l csmdb csm_ras_type_data.csv
 ------------------------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM database ras type automation script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_ras_type_script.log
 [Info    ] csm_ras_type_data.csv file exists
 [Warning ] This will load and or update csm_ras_type table data into csmdb database. Do you want to continue [y/n]?
 [Info    ] User response: y
 [Info    ] csm_ras_type record count before script execution: 744
 [Info    ] Record import count from csm_ras_type_data.csv: 744
 [Info    ] Record update count from csm_ras_type_data.csv: 0
 [Info    ] csm_ras_type live row count after script execution: 744
 [Info    ] csm_ras_type_audit live row count: 744
 [Info    ] Database: csmdb csv upload process complete for csm_ras_type table.
 ------------------------------------------------------------------------------------------------------------------------
 [End     ] Database: bills_test_db csv upload process complete for csm_ras_type table.
 ------------------------------------------------------------------------------------------------------------------------

Removing records from csm_ras_type table (manually)
---------------------------------------------------

1.  The script will remove records from the ``csm_ras_type`` table. The option (``-r, --removedata``) can be executed.
    A prompt message will appear and the admin has the ability to choose ``"y/n"``.
    Each of the logging message will be logged accordingly.  

.. code-block:: bash

 /opt/ibm/csm/db/csm_db_ras_type_script.sh –r  my_db_name (where my_db_name is the name of your DB).

 Example (successful execution):
 
 -bash-4.2$ ./csm_db_ras_type_script.sh -r csmdb
 ------------------------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM database ras type automation script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_ras_type_script.log
 [Warning ] This will drop csm_ras_type table data from csmdb database. Do you want to continue [y/n]?
 [Info    ] User response: y
 [Info    ] Record delete count from the csm_ras_type table: 744
 [Info    ] csm_ras_type live row count: 0
 [Info    ] csm_ras_type_audit live row count: 1488
 [Info    ] Data from the csm_ras_type table has been successfully removed
 ------------------------------------------------------------------------------------------------------------------------

2.  The script will remove records from the ``csm_ras_type`` table and repopulate when a given csv file is present after the db_name. The option (``-r, --removedata``) can be executed.
    A prompt message will appear and the admin has the ability to choose ``"y/n"``.
    Each of the logging message will be logged accordingly.  

.. code-block:: bash

 /opt/ibm/csm/db/csm_db_ras_type_script.sh –r  my_db_name <ras_csv_file> (where my_db_name is the name of your DB and the csv_file_name).

 Example (successful execution): 

 -bash-4.2$ ./csm_db_ras_type_script.sh -r csmdb csm_ras_type_data.csv
 ------------------------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM database ras type automation script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_ras_type_script.log
 [Info    ] csm_ras_type_data.csv file exists
 [Warning ] This will drop csm_ras_type table data from csmdb database. Do you want to continue [y/n]?
 [Info    ] User response: y
 [Info    ] Record delete count from the csm_ras_type table: 744
 [Info    ] csm_ras_type live row count: 0
 [Info    ] csm_ras_type_audit live row count: 1488
 [Info    ] Data from the csm_ras_type table has been successfully removed
 ------------------------------------------------------------------------------------------------------------------------
 [Info    ] csm_ras_type record count before script execution: 0
 [Info    ] Record import count from csm_ras_type_data.csv: 744
 [Info    ] csm_ras_type live row count after script execution: 744
 [Info    ] csm_ras_type_audit live row count: 2232
 [Info    ] Database: csmdb csv upload process complete for csm_ras_type table.
 ------------------------------------------------------------------------------------------------------------------------

Example (unsuccessful execution):
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

 -bash-4.2$ ./csm_db_ras_type_script.sh -r csmdb
 ------------------------------------------------------------------------------------------------------------------------
 [Start   ] Welcome to CSM datatbase ras type automation script.
 ------------------------------------------------------------------------------------------------------------------------
 [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_ras_type_script.log
 [Info    ] csm_ras_type_data.csv file exists
 [Warning ] This will drop csm_ras_type table data from csmdb database. Do you want to continue [y/n]?
 [Info    ] User response: n
 [Info    ] Data removal from the csm_ras_type table has been aborted
 ------------------------------------------------------------------------------------------------------------------------
