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

This tool requires a properly set up ``csm_master.cfg`` file. The system administrator should update the following ``ufm`` section with the proper ``rest_address`` and ``rest_port``. Without the address for the UFM server, this tool can not communicate with UFM. For more information about configuration files, look here: :ref:`CSMDConfig`.

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

A system administrator can also configure this tool's output. CSM may detect some network hardware that is not Mellanox hardware or may collect incomplete records for Mellanox hardware. If this is the case, CSM will capture what data it can and output those records to a `bad_records` file. The default location and names of these files are copied from the ``csm_master.cfg`` file and reproduced below. A system administrator can update the ``csm_master.cfg`` file to customize or change these filenames should they choose. 

.. code-block:: json

    {
        "inventory" :
        {
            "csm_inv_log_dir" : "/var/log/ibm/csm/inv",
            "ufm":
            {
                "ib_cable_errors" : "bad_ib_cable_records.txt",
                "switch_errors"   : "bad_switch_records.txt",
                "ufm_switch_output_file_name" : "ufm_switch_output_file.json",
                "ufm_switch_input_file_name" : "ufm_switch_output_file.json"
            }
        }
    }

:csm_inv_log_dir:
    The absolute path for inventory collection logs.

:ufm:
    :ib_cable_errors: 
        Output file location for records of bad IB cables as detected by CSM.
        
        Relative to the ``csm_inv_log_dir``.

    :switch_errors: 
        Output file location for records of IB switch errors as detected by CSM.

        Relative to the ``csm_inv_log_dir``.

    :ufm_switch_output_file_name: 
        During inventory collection, CSM calls a ufm restfulAPI. The restfulAPI outputs json. CSM saves the json output to a file. CSM will use this value to name that file.
        
        Relative to the ``csm_inv_log_dir``.

    :ufm_switch_input_file_name: 
        During inventory collection, CSM needs to read from a json file that contains inventory data. This value is the name of the file to read from. Most of the time it should be the same as the output file above. As step 1 is collect the info and save it, then step 2 is to read that info, parse it, and send it to CSM database. CSM team has seperated these two values to give the system admin an opportunity to read from a different file other than what was collected and saved in step 1.

        Relative to the ``csm_inv_log_dir``.

.. note:: For CSM 1.7, this tool only supports an ``http`` connection to UFM. UFM's ``gv.cfg`` file must be configured for http connection. Specifically, ``ws_protocol = http``. And also the MLNX-OS section must be set to http. Specifically, ``protocol = http``, and ``port = 80``.

Example:

.. code-block:: none

    ws_protocol = http

and

.. code-block:: none

    # default MLNX-OS access point for all Mellanox switches
    # important: this section parameters are used for ufm initialization only !!!
    #            Please use ufm GUI/API for editing parameters values.
    [MLNX_OS]
    protocol = http
    port = 80

.. note:: For CSM 1.8, this tool supports http and https connection to the UFM daemon, but still requires the ufmd to communicate to a managed switch via http. Please ensure that the MLNX-OS section is set to http. Specifically, ``protocol = http``, and ``port = 80``.

Example:

.. code-block:: none

    # default MLNX-OS access point for all Mellanox switches
    # important: this section parameters are used for ufm initialization only !!!
    #            Please use ufm GUI/API for editing parameters values.
    [MLNX_OS]
    protocol = http
    port = 80


Using the Tool
**************

The UFM Inventory collection tool has multiple flags. 

For help run the tool with the ``-h, --help`` flag. This will give useful help for all flags and example values.

The first flag is ``-c, --config``. This flag tells the tool where your ``csm_master.cfg`` file is located. If this flag is not provided, then the tool will look in the default location of: ``/etc/ibm/csm/csm_master.cfg``. 

The second flag is ``-t, --type``. This flag determines what type of inventory should be collected. 1 = ib cables, 2 = switches, 3 = ib cables and switches. If this flag is not provided, then the tool will default to type 3, collecting information on both ib cables and switches. 

Another flag, ``-i, --input_override``, overrides the value for ``ufm_switch_input_file_name`` defined in the ``csm_master.cfg``. This is a direct and literal full path including the filename and extension. This is useful if the tool needs to be passed switch inventory information from a seperate origin source for a single run. 

Output
******

All output information for this tool is printed to the console. The ``-d, --details`` flag can be used to turn on extra information. If there are bad or incomplete records for hardware inventory they will not be copied into the :ref:`CSM_Database` and instead placed into the `bad_records` files specified in the ``csm_master.cfg`` file.

.. _CSM_standalone_inventory_collection_FAQ:

FAQ - Frequently Asked Questions
********************************

Why 'bad record' and N/A' Serial Numbers?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Sometimes CSM can detect a switch in your system that is incomplete. It is missing a serial number, which CSM will consider invalid data, and therefor not insert it into the :ref:`CSM_Database`.

Example:

.. code-block:: none

    UFM reported 7 switch records.
    This report from UFM can be found in 'ufm_switch_output_file.json' located at '/var/log/ibm/csm/inv'
    WARNING: 2 Switches found with 'N/A' serial numbers and have been removed from CSM inventory collection data.
    These records copied into 'bad_switch_records.txt' located at '/var/log/ibm/csm/inv'

This is usually caused by switches not correctly reporting :ref:`CSM_Network_Inventory_Switch_Module`. If the system module is not found, then CSM can not collect the serial number.

Below is an example of JSON data returned from UFM. The first is one missing modules, the second is what we expect in a good case. 

Bad case - as you can see the module array is NOT populated. :

.. code-block:: json

    [
        {
            "cpus_number": 0,
            "ip": "10.7.3.2",
            "ram": 0,
            "fw_version": "15.2000.2626",
            "mirroring_template": false,
            "cpu_speed": 0,
            "is_manual_ip": true,
            "technology": "EDR",
            "psid": "MT_2630110032",
            "guid": "248a070300fcccd0",
            "severity": "Warning",
            "script": "N/A",
            "capabilities": [
                "Provisioning"
            ],
            "state": "active",
            "role": "tor",
            "type": "switch",
            "sm_mode": "noSM",
            "vendor": "Mellanox",
            "description": "MSB7800",
            "has_ufm_agent": false,
            "server_operation_mode": "Switch",
            "groups": [
                "Alarmed_Devices"
            ],
            "total_alarms": 1,
            "temperature": "N/A",
            "system_name": "c650f03ib-root01-M",
            "sw_version": "N/A",
            "system_guid": "248a070300fcccd0",
            "name": "248a070300fcccd0",
            "url": "",
            "modules": [],
            "cpu_type": "any",
            "is_managed": true,
            "model": "MSB7800",
                        "ports": [
                "248a070300fcccd0_9",
                "248a070300fcccd0_8",
                "248a070300fcccd0_7",
                "248a070300fcccd0_6",
                "248a070300fcccd0_5",
                "248a070300fcccd0_4",
                "248a070300fcccd0_3",
                "248a070300fcccd0_2",
                "248a070300fcccd0_1",
                "248a070300fcccd0_28",
                "248a070300fcccd0_29",
                "248a070300fcccd0_26",
                "248a070300fcccd0_27",
                "248a070300fcccd0_24",
                "248a070300fcccd0_25",
                "248a070300fcccd0_22",
                "248a070300fcccd0_23",
                "248a070300fcccd0_20",
                "248a070300fcccd0_21",
                "248a070300fcccd0_37",
                "248a070300fcccd0_36",
                "248a070300fcccd0_16",
                "248a070300fcccd0_15",
                "248a070300fcccd0_10",
                "248a070300fcccd0_31",
                "248a070300fcccd0_30",
                "248a070300fcccd0_33",
                "248a070300fcccd0_32",
                "248a070300fcccd0_35",
                "248a070300fcccd0_34",
                "248a070300fcccd0_19",
                "248a070300fcccd0_18"
            ]
        }
    ]

How you would see this JSON represented in the `bad_records` files of CSM. Notice the missing modules section.

.. code-block:: none

    CSM switch inventory collection
    File created: Fri Feb 28 13:40:59 2020

    The following records are incomplete and can not be inserted into CSM database.

    Switch: 2
    ip:                    10.7.3.2
    fw_version:            15.2000.2626
    total_alarms:          1
    psid:                  MT_2630110032
    guid:                  248a070300fcccd0
    state:                 active
    role:                  tor
    type:                  switch
    vendor:                Mellanox
    description:           MSB7800
    has_ufm_agent:         false
    server_operation_mode: Switch
    sm_mode:               noSM
    system_name:           c650f03ib-root01-M
    sw_version:            N/A
    system_guid:           248a070300fcccd0
    name:                  248a070300fcccd0
    modules:               ???
    serial_number:         N/A
    model:                 MSB7800

Good case - as you can see the module array is populated. :

.. code-block:: json

    [
        {
            "cpus_number": 0, 
            "ip": "10.7.4.2", 
            "ram": 0, 
            "fw_version": "15.2000.2626", 
            "mirroring_template": false, 
            "cpu_speed": 0, 
            "is_manual_ip": true, 
            "technology": "EDR", 
            "psid": "MT_2630110032", 
            "guid": "248a070300fd6100", 
            "severity": "Info", 
            "script": "N/A", 
            "capabilities": [
                "ssh", 
                "sysinfo", 
                "reboot", 
                "mirroring", 
                "sw_upgrade", 
                "Provisioning"
            ], 
            "state": "active", 
            "role": "tor", 
            "type": "switch", 
            "sm_mode": "noSM", 
            "vendor": "Mellanox", 
            "description": "MSB7800", 
            "has_ufm_agent": false, 
            "server_operation_mode": "Switch", 
            "groups": [], 
            "total_alarms": 0, 
            "temperature": "56", 
            "system_name": "c650f04ib-leaf02-M", 
            "sw_version": "3.8.2102-X86_64", 
            "system_guid": "248a070300fd6100", 
            "name": "248a070300fd6100", 
            "url": "", 
            "modules": [
                {
                    "status": "OK", 
                    "sw_version": "N/A", 
                    "hw_version": "MSB7800-ES2F", 
                    "name": "248a070300fd6100_4000_01", 
                    "hosting_system_guid": "248a070300fd6100", 
                    "number_of_chips": 0, 
                    "description": "MGMT - 1", 
                    "max_ib_ports": 0, 
                    "module_index": 1, 
                    "temperature": "N/A", 
                    "device_type": "Switch", 
                    "serial_number": "MT1706X02692", 
                    "path": "default(86) / Switch: c650f04ib-leaf02-M / MGMT 1", 
                    "device_name": "c650f04ib-leaf02-M", 
                    "type": "MGMT", 
                    "severity": "Info"
                }, 
                {
                    "status": "OK", 
                    "sw_version": "N/A", 
                    "hw_version": "MTEF-PSF-AC-A", 
                    "name": "248a070300fd6100_2005_01", 
                    "hosting_system_guid": "248a070300fd6100", 
                    "number_of_chips": 0, 
                    "description": "PS - 1", 
                    "max_ib_ports": 0, 
                    "module_index": 1, 
                    "temperature": "N/A", 
                    "device_type": "Switch", 
                    "serial_number": "MT1706X07347", 
                    "path": "default(86) / Switch: c650f04ib-leaf02-M / PS 1", 
                    "device_name": "c650f04ib-leaf02-M", 
                    "type": "PS", 
                    "severity": "Info"
                }, 
                {
                    "status": "OK", 
                    "sw_version": "3.8.2102-X86_64", 
                    "hw_version": "MSB7800-ES2F", 
                    "name": "248a070300fd6100_1007_01", 
                    "hosting_system_guid": "248a070300fd6100", 
                    "number_of_chips": 0, 
                    "description": "SYSTEM", 
                    "max_ib_ports": 0, 
                    "module_index": 1, 
                    "temperature": "56", 
                    "device_type": "Switch", 
                    "serial_number": "MT1706X02692", 
                    "path": "default(86) / Switch: c650f04ib-leaf02-M / system 1", 
                    "device_name": "c650f04ib-leaf02-M", 
                    "type": "SYSTEM", 
                    "severity": "Info"
                }, 
                {
                    "status": "OK", 
                    "sw_version": "N/A", 
                    "hw_version": "MTEF-PSF-AC-A", 
                    "name": "248a070300fd6100_2005_02", 
                    "hosting_system_guid": "248a070300fd6100", 
                    "number_of_chips": 0, 
                    "description": "PS - 2", 
                    "max_ib_ports": 0, 
                    "module_index": 2, 
                    "temperature": "N/A", 
                    "device_type": "Switch", 
                    "serial_number": "MT1706X07348", 
                    "path": "default(86) / Switch: c650f04ib-leaf02-M / PS 2", 
                    "device_name": "c650f04ib-leaf02-M", 
                    "type": "PS", 
                    "severity": "Info"
                }, 
                {
                    "status": "OK", 
                    "sw_version": "N/A", 
                    "hw_version": "MTEF-FANF-A", 
                    "name": "248a070300fd6100_4001_03", 
                    "hosting_system_guid": "248a070300fd6100", 
                    "number_of_chips": 0, 
                    "description": "FAN - 3", 
                    "max_ib_ports": 0, 
                    "module_index": 3, 
                    "temperature": "N/A", 
                    "device_type": "Switch", 
                    "serial_number": "MT1706X08121", 
                    "path": "default(86) / Switch: c650f04ib-leaf02-M / FAN 3", 
                    "device_name": "c650f04ib-leaf02-M", 
                    "type": "FAN", 
                    "severity": "Info"
                }, 
                {
                    "status": "OK", 
                    "sw_version": "N/A", 
                    "hw_version": "MTEF-FANF-A", 
                    "name": "248a070300fd6100_4001_02", 
                    "hosting_system_guid": "248a070300fd6100", 
                    "number_of_chips": 0, 
                    "description": "FAN - 2", 
                    "max_ib_ports": 0, 
                    "module_index": 2, 
                    "temperature": "N/A", 
                    "device_type": "Switch", 
                    "serial_number": "MT1706X08119", 
                    "path": "default(86) / Switch: c650f04ib-leaf02-M / FAN 2", 
                    "device_name": "c650f04ib-leaf02-M", 
                    "type": "FAN", 
                    "severity": "Info"
                }, 
                {
                    "status": "OK", 
                    "sw_version": "N/A", 
                    "hw_version": "MTEF-FANF-A", 
                    "name": "248a070300fd6100_4001_01", 
                    "hosting_system_guid": "248a070300fd6100", 
                    "number_of_chips": 0, 
                    "description": "FAN - 1", 
                    "max_ib_ports": 0, 
                    "module_index": 1, 
                    "temperature": "N/A", 
                    "device_type": "Switch", 
                    "serial_number": "MT1706X08117", 
                    "path": "default(86) / Switch: c650f04ib-leaf02-M / FAN 1", 
                    "device_name": "c650f04ib-leaf02-M", 
                    "type": "FAN", 
                    "severity": "Info"
                }, 
                {
                    "status": "OK", 
                    "sw_version": "N/A", 
                    "hw_version": "MTEF-FANF-A", 
                    "name": "248a070300fd6100_4001_04", 
                    "hosting_system_guid": "248a070300fd6100", 
                    "number_of_chips": 0, 
                    "description": "FAN - 4", 
                    "max_ib_ports": 0, 
                    "module_index": 4, 
                    "temperature": "N/A", 
                    "device_type": "Switch", 
                    "serial_number": "MT1706X08120", 
                    "path": "default(86) / Switch: c650f04ib-leaf02-M / FAN 4", 
                    "device_name": "c650f04ib-leaf02-M", 
                    "type": "FAN", 
                    "severity": "Info"
                }
            ], 
            "cpu_type": "any", 
            "is_managed": true, 
            "model": "MSB7800", 
            "ports": [
                "248a070300fd6100_33", 
                "248a070300fd6100_1", 
                "248a070300fd6100_3", 
                "248a070300fd6100_2", 
                "248a070300fd6100_5", 
                "248a070300fd6100_4", 
                "248a070300fd6100_7", 
                "248a070300fd6100_6", 
                "248a070300fd6100_9", 
                "248a070300fd6100_8", 
                "248a070300fd6100_15", 
                "248a070300fd6100_14", 
                "248a070300fd6100_17", 
                "248a070300fd6100_16", 
                "248a070300fd6100_11", 
                "248a070300fd6100_10", 
                "248a070300fd6100_13", 
                "248a070300fd6100_12", 
                "248a070300fd6100_37", 
                "248a070300fd6100_19", 
                "248a070300fd6100_18", 
                "248a070300fd6100_36", 
                "248a070300fd6100_35", 
                "248a070300fd6100_34", 
                "248a070300fd6100_20", 
                "248a070300fd6100_21", 
                "248a070300fd6100_23", 
                "248a070300fd6100_24", 
                "248a070300fd6100_26", 
                "248a070300fd6100_27"
            ]
        }
    ]

Modules will sometimes not be reported via UFM. One of the most common causes of this is a communication issue with the ufm daemon, ``ufmd``. See :ref:`CSM_standalone_inventory_collection_FAQ_CONNECTOR_ACCESS_fail` for more information.

.. _CSM_standalone_inventory_collection_FAQ_CONNECTOR_ACCESS_fail:

What is a `CONNECTOR_ACCESS fail`?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If :ref:`CSM_standalone_inventory_collection` reports a ``connector_access fail`` then it probably failed to connect to the ``ufmd``. 

For example, a system administrator may see:

.. code-block:: none

    Response returned with status code 403
    INV_IB_CONNECTOR_ACCESS failed

or 

.. code-block:: none

    Response returned with status code 400
    INV_SWITCH_CONNECTOR_ACCESS failed

If :ref:`CSM_standalone_inventory_collection` reports an error connecting to the ``ufmd`` or an error in the 400s range, then it may be a communication issue. CSM tries to anticipate the multiple forms of communication, but sometimes a system admin will need to tweak the configuration file for ufm and restart the ``ufmd``. 

On the server running ``ufmd`` the system administrator should look to find the ufm config file, ``gv.cfg``. It should be located at ``/opt/ufm/conf``. In that file the system administrator may need to configure a few fields. 

The first field to check is ``ws_protocol``. This is how external programs, like CSM, communicate with ``ufmd``. It should be set to ``https`` by default. But if it isn't working, then try setting it to ``http``. 

Example:

.. code-block:: none

    ws_protocol = http

If CSM seems to be communicating with the ``ufmd`` fine, but some of the managed switches are still not reporting modules, then the system administrator needs to look at another section of the ufm config file, ``gv.cfg``. The next section to look at is ``[MLNX_OS]``. This section deals with the OS that runs on managed switches. 

.. code-block:: none

    # default MLNX-OS access point for all Mellanox switches
    # important: this section parameters are used for ufm initialization only !!!
    #            Please use ufm GUI/API for editing parameters values.
    [MLNX_OS]
    protocol = https
    port = 443
    user = admin
    credentials = admin
    timeout = 10

It has two fields that deal with how ``ufmd`` communicates with the Mellanox OS. If , then try changing the protocol and port to ``http`` and ``80``. 

.. code-block:: none

    protocol = http
    port = 80

CSM should be able to openly communicate with UFM at this point. But as this is a system administrator and configuration issue, and not a CSM issue, please consult Mellanox and System Administrator support for resolution of this issue. 


