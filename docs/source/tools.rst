Tools
=====

CSM logging tools
-----------------

To use the CSM logging tools run: ``opt/csm/tools/API_Statistics.py`` python script.

This python script parses log files to calculate the start and end time of API calls on the different types of nodes that generate these logs. From the start and end time, the script calculates:

- ``frequency`` at which the API was called

- ``mean`` run time

- ``median`` run time

- ``minimum`` run time

- ``maximum`` run time

- ``standard deviation`` run time

The script also captures job ID collisions when start and end API's do not match.

.. note:: Run the script with -h for help. 

.. code-block:: none

	[root@c650f03p41 tools]# python API_Statistics.py -h
	usage: API_Statistics.py [-h] [-p path] [-s start] [-e end] [-o order]
	                         [-r reverse]

	A tool for parsing daemon logs for API statistics.

	optional arguments:
	  -h, --help  show this help message and exit
	  -p path     The directory path to where the logs are located. Defaults to:
	              '/var/log/ibm/csm'
	  -s start    start of search range. Defaults to: '1000-01-01 00:00:00.0000'
	  -e end      end of search range. Defaults to: '9999-01-01 00:00:00.0000'
	  -o order    order the results by a field. Defaults to alphabetical by API
	              name. Valid values: 0 = alphabetical, 1 = Frequency, 2 = Mean, 3
	              = Max, 4 = Min, 5 = Std
	  -r reverse  reverse the order of the data. Defaults to 0. Set to 1 to turn
	              on.




Obtaining Log Statistics
************************

Setup
^^^^^

This script handles `Master`, `Computer`, `Utility`, and, `Aggregator` logs. These must be placed under the ``opt/csm/tools/Logs`` directory unders their respective types. 

.. note:: As of CSM 1.4, the script can be pointed to a directory where the log files are located, and by default the program will use ``/var/log/ibm/csm``.

Running the script
^^^^^^^^^^^^^^^^^^

There are three ways of running the logs with time formats:

.. code-block:: none

    Format: <Start Date> <Start Time>
    Format: YYYY-MM-DD HH:MM::SS

1. Run through the logs in its entirety:

.. code-block:: none

	python API_Statistics.py

2. Run through the logs with a specific start time:

.. code-block:: none

	python API_Statistics.py <Start Date> <Start Time> 


3. Run through the logs with a specific start and end time:

.. code-block:: none

	python API_Statistics.py <Start Date> <Start Time> <End Date> <End Time>


.. note:: As of CSM 1.4 the time ranges of the script has been updated to use flags. 

Output
^^^^^^
Reports will be caluclated and saved to individual files under ``opt/csm/tools/Reports`` under their respective log types. (The script will output to the screen as well). The report includes errors and calculated statistics.

.. _CSM_standalone_inventory_collection:

CSM standalone inventory collection
-----------------------------------

This tool connects to UFM and collects inventory information on all Mellanox hardware in the system. You can find it located at: ``/opt/ibm/csm/sbin``. This program has a ``-h, --help`` flag to display helpful information and should be viewed to familiarize a user with the program's features. 

Setup and Configuration
***********************

This tool requires a properly set up ``master.cfg`` file. The system administrator should update the following ``ufm`` section with the proper ``rest_address`` and ``rest_port``. Without the address for the UFM server, this tool can not communicate with UFM.

.. code-block:: json

	{
	    "ufm" :
	    {
	        "rest_address"  : "__UFM_REST_ADDRESS__",
	        "rest_port"     : 80,
	        "ufm_ssl_file_path" : "/etc/ibm/csm",
	        "ufm_ssl_file_name" : "csm_ufm_ssl_key.txt"
	    }
	}

The system administrator must also create an SSL key for their UFM username and password. You can read more on how to do that here: :ref:`UFM_Credentials`.

A system administrator can also configure this tool's output. CSM may detect some network hardware that is not Mellanox hardware or may collect incomplete records for Mellanox hardware. If this is the case, CSM will capture what data it can and output those records to a `bad_records` file. The default location and names of these files are copied from the ``master.cfg`` file and reproduced below. A system administrator can update the ``master.cfg`` file to customize or change these filenames should they choose. 

.. code-block:: json

    {
        "inventory" :
        {
            "csm_inv_log_dir" : "/var/log/ibm/csm/inv",
            "ufm":
            {
                "ib_cable_errors" : "bad_ib_cable_records.txt",
                "switch_errors"   : "bad_switch_records.txt"
            }
        }
    }

Using the Tool
**************

The UFM Inventory collection tool has multiple flags. 

The first flag is ``-c, --config``. This flag tells the tool where your ``master.cfg`` file is located. If this flag is not provided, then the tool will look in the default location of: ``/etc/ibm/csm/csm_master.cfg``. 

The second flag is ``-t, --type``. This flag determines what type of inventory should be collected. 1 = ib cables, 2 = switches, 3 = ib cables and switches. If this flag is not provided, then the tool will default to type 3, collecting information on both ib cables and switches. 

Output
******

All output information for this tool is printed to the console. The ``-d, --details`` flag can be used to turn on extra information. If there are bad or incomplete records for hardware inventory they will not be copied into the :ref:`CSM_Database` and instead placed into the `bad_records` files specified in the ``master.cfg`` file.



