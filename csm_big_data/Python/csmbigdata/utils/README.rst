csmbigdata.utils
****************
This package is designed to consolidate modules that handle interfacing between Python and 
various `Big Data Store` targets. 

Currently the following Interfaces have been implemented:

* Log Analysis (Fully Functional)
* CSM API (Ongoing Development)

Future work is currently being planned for integration with Hadoop, pending revisions to 
how Log Analysis connections function.


**Contents**

.. contents::
    :local:

Connection Modules
==================
These modules facilitate the interface to `Big Data Store` targets in a Pythonic manner.

CSM API Tool (csmbigdata.utils.csmi_tool)
-----------------------------------------
This module allows the user to access the CSM API commandline interfaces more easily. 
Each function will build the command line argument string based on inputs from the user,
execute the command line and parse the YAML output into a dictionary.

Future iterations will integrate other command lines and improve general quality of
life for this module. 

Currently supported command line interfaces include:

* csm_allocation_query
* csm_node_attributes_query

.. warning:: In this iteration of this module the `CSMITool.CSM_HOME` variable is set to
   `/opt/ibm/csm/bin/`, if the CSM APIs command line directory is located anywhere else
   it **MUST** be updated in this file to use the module. Future iterations of this module
   will relocate this assignment to a more easily modified environment location.


.. automodule:: csmbigdata.utils.csmi_tool
    :members:
    :noindex:

Log Analysis Payload (csmbigdata.utils.unity_payload)
-----------------------------------------------------
This module is a front end to the unity module (omitted from this documentation) supplied with 
`Log Analysis`. To reduce the complexity of formulating POST queries by hand this module
has implemented a variety of methods for constructing queries which verify and sanitize
user input to reduce errors.

It is generally recommended that only one instance of this object be used in a script, with the 
`initialize` command being invoked between queries. Edge cases do exist where multiple 
queries and reusing payload settings are necessary, but these cases are left to the user to 
determine.


.. automodule:: csmbigdata.utils.unity_payload
    :members:
    :noindex:

Hive Payload (csmbigdata.utils.hive_payload)
--------------------------------------------
A wrapper module for `pyHive`. This allows the user to easily connect to and execute Hive
Queries.

.. automodule:: csmbigdata.utils.hive_payload
    :members:
    :noindex:

Helper Modules
==============
These modules offer structure and assistance in writing scripts to interface with `Big Data Store`
targets.

Log Analysis Helper Functions (csmbigdata.utils.unity_helpers)
--------------------------------------------------------------

A module that consolidates frequently used functions, such as determining the start time of a 
job. This module will be regularly updated with helper functions for use in scripts that 
interact with the `Big Data Store` or `CSM APIs`. 

.. automodule:: csmbigdata.utils.unity_helpers
    :members:
    :noindex:
 
Output Helpers (csmbigdata.utils.output_helpers)
------------------------------------------------
This module contains a collection of data output helpers: timestamp printers, stderr printers, 
csv makers, etc. To standardize output it is recommended to first check this file before
writing anything that outputs to the screen or a file.

.. automodule:: csmbigdata.utils.output_helpers
   :members:
   :noindex:
