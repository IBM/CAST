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



















