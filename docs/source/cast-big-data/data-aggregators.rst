Data Aggregation
================

Data Aggregation in CAST utilizes the Logstash pipeline to process events and pass it along to 
Elasticsearch.

.. note:: In the following documentation, examples requiring replacement will be annotated with the bash style 
    `${variable_name}` and followed by an explanation of the variable.

.. contents::
   :local:

.. _SyslogDataAgg:

Syslog
------

:Logstash Port: 10515

Syslog is generally aggregated through the use of the rsyslog daemon. 

Most devices are capable of producing syslogs, and it is sugggested that syslogs should be sent
to Logstash via a redirection hierarchy outlined in the table below: 

+----------------+--------------------+
|  Type of Node  | Syslog Destination |
+----------------+--------------------+
|  Service Node  |    Logstash        |
+----------------+--------------------+
|  Compute Node  |    Service Node    |
+----------------+--------------------+
|  Utility Node  |    Service Node    |
+----------------+--------------------+
|   UFM Server   |    Service Node    | 
+----------------+--------------------+
|   IB/Ethernet  |    Logstash Node   | 
+----------------+--------------------+
|      PDUs      |    Logstash Node   | 
+----------------+--------------------+

Please note that this table only deals with Syslog redirection, not logs in the cluster will 
be logged via syslog. See `UFM Logs`_ for one such example.

Syslog Redirection
^^^^^^^^^^^^^^^^^^

.. warning:: This step should not be performed on compute nodes in xCAT clusters!

To redirect a syslog so it is accepted by Logstash the following must be added to the 
`/etc/rsyslog.conf` file:

.. code-block:: bash

    $template logFormat, "%TIMESTAMP:::date-rfc3339% %HOSTNAME% %APP-NAME% %PROCID% %syslogseverity-text% %msg%\n"
    
    *.*;cron.none @@${logstash_node}:${syslog_port};logFormat

    /bin/systemctl restart  rsyslog.service

**Field Description**

:logstash_node: Replace with the hostname or IP address of the Logstash Server, on service nodes 
    this is typically *localhost*.
:syslog_port: Replace with the port set in the Logstash Configuration File [ default: 10515 ]. 

**Format**

The format of the syslog is parsed in the CAST model by Logstash. CAST provides a grok for this 
syslog format in the pattern list provided by the CAST repository and rpm. The grok pattern is
reproduced below with the types matching directly to the types in 
:ref:`the syslog elastic documentation <SyslogElastic>`.

.. code-block:: bash

    RSYSLOGDSV ^(?m)%{TIMESTAMP_ISO8601:timestamp} %{HOSTNAME:hostname} %{DATA:program_name} %{INT:process_id} %{DATA:severity} %{GREEDYDATA:message}$

.. note:: This pattern has a 1:1 relationship with the template given above and a 1:many relationship with
        the index data mapping. Logstash appends some additional fields for metadata analysis.

GPFS Logging
^^^^^^^^^^^^

To redirect the GPFS logging data to the syslog please do the following on the Management node for GPFS:

.. code-block:: bash

    /usr/lpp/mmfs/bin/mmchconfig systemLogLevel=notice

After completing this process the gpfs log should now be forwarded to the `syslog` for the configured node.

.. note:: Refer to `Syslog Redirection`_ for gpfs log forwarding, the default syslog port is recommended (10515).

.. note:: The `systemLogLevel` attribute will forward logs of the specified level and higher to the syslog. It supports the following options: **alert**, **critical**, **error**, **warning**, **notice**, **configuration**, **informational**, **detail**, and **debug**.

.. note:: This data type will inhabit the same index as the *syslog* documents due to data similarity.


UFM Logs
--------

.. note:: This document assumes that the UFM daemon is up and running on the UFM Server.

:Relevant Directories:
    | `/opt/ibm/csm/bigdata/DataAggregators/config_scripts`
    
The `Unified Fabric Manager` (UFM) has several distinct data logs to aggregate for the big data store.

System Event Log
^^^^^^^^^^^^^^^^

:Logstash Port: 10515

:Configuration Script: `/opt/ibm/csm/bigdata/DataAggregators/config_scripts/ufm_config.sh`

The System Event Log will report various fabric events that occur in the UFM's network: 

*  A link coming up.
*  A link going down.
*  UFM module problems.
*  ...

A sample output showing a downed link can be seen below:

.. code-block:: none

    Oct 17 15:56:33 c931hsm04 eventlog[30300]: WARNING - 2016-10-17 15:56:33.245 [5744] [112] 
        WARNING [Hardware] IBPort [default(34) / Switch: c931ibsw-leaf01 / NA / 16] 
        [dev_id: 248a0703006d40f0]: Link-Downed counter delta threshold exceeded. 
        Threshold is 0, calculated delta is 1. Peer info: Computer: c931f03p08 HCA-1 / 1.


.. note:: The above example is in the Syslog format.

To send this log to the Logstash data aggregation the `/opt/ufm/files/conf/gv.cfg` file must be 
modified and `/etc/rsyslog.conf` should be modified as described in `Syslog Redirection`_.

.. TODO 
.. To modify `/opt/ufm/files/conf/gv.cfg` for System Event Log redirection execute the 
.. `/opt/ibm/csm/bigdata/DataAggregators/config_scripts/ufm_config.sh` script on the UFM Server.
.. The config file may be changed by providing a full file path in the script execution.

CAST recommends setting the following attributes in `/opt/ufm/files/conf/gv.cfg`:

.. code-block:: none

    [Logging]
    level = INFO
    syslog = true
    event_syslog = true

    [CSV]
    write_interval = 30
    ext_ports_only = yes
    max_files = 10

    [MonitoringHistory]
    history_configured = true


.. note:: `write_interval and `max_files` were set as a default, change these fields as needed.

After configuring `/opt/ufm/files/conf/gv.cfg` restart the ufm daemon.

.. code-block:: none

    /etc/init.d/ufmd restart

**Format**

CAST recommends using the same syslog format as shown in `Syslog Redirection`_, however, the message
in the case of the mellanox event log has a consistent structure which may be parsed by Logstash.
The pattern and substitutions are used below. Please note that the *timestamp*, *severity* and
*message* fields are all overwritten from the default syslog pattern.

Please consult the event log table in `the elasticsearch documentation <melElastic>` for details on
the message fields.

.. code-block:: bash

    MELLANOXMSG %{MELLANOXTIME:timestamp} \[%{NUMBER:log_counter}\] \[%{NUMBER:event_id}\] %{WORD:severity} \[%{WORD:event_type}\] %{WORD:category} %{GREEDYDATA:message}

.. _ConsoleDataAggregator:

Console Logs
------------

.. note:: This document is designed to configure the xCAT service nodes to ship goconserver output to logstash 
    (written using xCAT 2.13.11).

:Logstash Port: 10520

:Relevant Directories:
    | `/etc/goconserver`

CSM recommends using the goconserver bundled in the xCAT dependicies and documented in xCat-GoConserver_.
A limited configuration guide is provided below, but for gaps or more details please refer to the 
the xCAT read the docs.

1. Install the goconserver and start it:

.. code-block:: bash
    
    yum install goconserver
    systemctl stop conserver.service
    makegocons

2. Configure the `/etc/goconserver` to send messages to the Logstash server associated with the 
    service node (generally localhost):

.. code-block:: YAML

    # For options above this line refer to the xCAT read-the-docs
    logger:
        tcp:
            - name: Logstash
              host: <Logstash-Server>
              port: 10520             # This is the port in the sample configuration.
              timeout: 3              # Default timeout time.


3. Restart the goconserver:

.. code-block:: bash
    
    service goconserver restart

The goconserver will now start sending data to the Logstash server in the form of JSON messages:

.. code-block:: javascript
    
    {
        "type"    : "console"
        "message" : "c650f04p23 login: jdunham"
        "node"    : "c650f04p23"
        "date"    : "2018-05-08T09:49:36.530886-04"
    }

The CAST logstash filter then mutates this data to properly store it in the elasticsearch backing 
store:

+--------+------------+
| Field  | New Field  | 
+========+============+
| node   | hostname   |
+--------+------------+
| date   | @timestamp |
+--------+------------+

Sensor Monitors
---------------

.. warning:: This documentation is now deprecated. 

In the data aggregation samples an out of band technique is employed to polls the 
management device for sensor data. This data varies device to device and requires separate techniques depending on how densor data is aggregated on the device.

This document details the following sample aggregation techniques:

.. contents::
   :local:


BMC Sensor Data
^^^^^^^^^^^^^^^

:Logstash Port: 10516

:Configuration File: 
    | `/opt/ibm/csm/bigdata/DataAggregators/sensor_monitoring/bmc_temperature_poll.cfg`

:Auxillary Files: 
    | `/opt/ibm/csm/bigdata/DataAggregators/sensor_monitoring/bmc_hosts`
    | `/opt/ibm/csm/bigdata/DataAggregators/sensor_monitoring/bmc_temperature_parse.awk`

:Script File: `/opt/ibm/csm/bigdata/DataAggregators/sensor_monitoring/bmc_temperature_poll.sh`

.. note:: This document assumes that the BMC has been configured with the latest firmware revision
    and is the BMC of a Power 8 node.

.. warning:: The supplied scripts require that `/opt/xcat/bin/ipmitool-xcat` is present on the 
    node (Service or Management Node).

.. warning:: This section is subject to change pending an update that allows in 
    band sensor aggregation.

The BMC Sensor Data Record is accessible through the `ipmitool sdr` command. This record can
contain anything from power usage and temperature to button events. The Data Aggregated in
the sample is from the node's temperature sensor array.

The following temperature sensors are tracked in the data aggregation sample:

* Ambient Temperature
* CPU Temperatures
* CPU Core Temperatures
* DIMM Temperatures
* GPU Temperatures
* Memory Buffer Temperatures

For the full list of sensors tracked run :

.. code-block:: none

     `/opt/xcat/bin/ipmitool-xcat -H {BMC ADDRESS} -U {BMC USER} -P {BMC PASSWORD sdr`

BMC Configuration File
######################

.. code-block:: none

    `/opt/ibm/csm/bigdata/DataAggregators/sensor_monitoring/bmc_temperature_poll.cfg`

The configuration file will set the the following attributes when executing the sensor poll.

:max_parallel_threads: The number of parallel subprocesses the script will spawn.

:csv_headers: Sets the order of the of the temperatures aggregated; comma separated 
    and may have any number of the following values (no duplicates):

    * Ambient
    * CPU 
    * CPU_Core
    * DIMM
    * GPU
    * Mem_Buff

    .. warning:: If this field is changed, the Logstash :ref: `logstash-patterns` file will need to be changed.

:hosts_file: The file containing the list of hosts to poll for sensor data, 
    hostnames are separated by new lines. This is the hostname tracked by xCAT,
    as the `BMC Polling Script`_ queries xCAT for BMC information.

:logstash_server: The hostname or IP address of the Logstash Server.

    .. warning:: This field MUST be changed.

:logstash_port: Replace with the port set in the Logstash :ref: `logstash-configuration-file` for BMC Temperature Sensor.

:default_passwd: The default IPMI password for the node, this will be used if no bmc password is found in xCAT.

:default_user: The defualt IPMI user for the node, this will be used is if no bmc user is found in xCAT.

.. warning:: Make sure the hosts file is changed when setting up this data aggregator.

BMC Polling Script
##################

.. code-block:: none

    `/opt/ibm/csm/bigdata/DataAggregators/sensor_monitoring/bmc_temperature_poll.sh`

The `bmc_temperature_poll.sh` script performs a query to the ipmitool sdr command aggregating 
temperature data. Due to potential mismatches between the different BMC devices this aggregator
computes the minimum and maximum values of each temperature module and presents them as a 
csv to the the Logstash Server.

The full list of values gathered by this script to send follows:

* bmc_hostname
* bmc_ip 
* bmc_temp_ambient
* bmc_temp_CPU_min
* bmc_temp_CPU_max
* bmc_temp_CPU_Core_min
* bmc_temp_CPU_Core_max
* bmc_temp_DIMM_min
* bmc_temp_DIMM_max
* bmc_temp_GPU_min
* bmc_temp_GPU_max
* bmc_temp_Mem_Buff_min
* bmc_temp_Mem_Buff_max

All of the above temperature values are recorded in Celsius. If a module has lost its reading the 
value sent/stored in the Big Data Store will be an empty string. By design bad module reads are
not filtered out during the parse of the results to allow the end user a chance to detect issues
with the sdr.

This script must be executed from the service node that manages the Nodes specified in the 
`hosts_file`. The script will query xcat to get the BMC IP, Username and Password.


.. warning:: This script will not work if it attempts to query hostnames without the following 
    details in the xCAT database:

    * bmc
    * bmcpassword
    * bmcusername


To set up the automation add the following to the `crontab` on either the Sevice or Management
Node that manages the BMCs to be polled by this script.

.. code-block:: none

    */30 * * * * cd /opt/ibm/csm/bigdata/DataAggregators/sensor_monitoring; \
      /bin/bash /opt/ibm/csm/bigdata/DataAggregators/sensor_monitoring/bmc_temperature_poll.sh >/dev/null 2>&1

.. note:: This cron entry will poll the listed nodes once every 30 minutes.



IB Switch Sensor Data
^^^^^^^^^^^^^^^^^^^^^

:Logstash Port: 10517

:Auxillary Files:
    | `/opt/ibm/csm/bigdata/DataAggregators/sensor_monitoring/ib_temperature_parse.awk`

:Script File: `/opt/ibm/csm/bigdata/DataAggregators/sensor_monitoring/ib_temperature_poll.sh`


Infiniband Switch temperature data is accessible through the commandline function `show temperature`. A finite number of readings are returned by this query as follows:

* CPU_Core_Sensor_T1
* CPU_Core_Sensor_T2
* CPU_package_Sensor
* power-mon_PS1 
* power-mon_PS2
* Board_AMB_temp
* Ports_AMB_temp
* SIB

As with the BMC temperature this temperature data is recorded in Celsius.

.. warning:: This Data Aggregation sample only supports Mellanox Switches!

Configuring ssh for an IB Switch
################################

By default ssh is **not** enabled on Mellanox branded Infiniband switches. Please do the 
following if your switch hasn't been configured before attempting to poll IB Switch Sensor Data.

xCAT is used to drive this data aggregation, as it has a procedure for interacting with infiniband
switches as detailed in xCAT-Infiniband_.


.. code-block:: Bash

    # Make an entry for the switches.
    chdef -t node <switch-name> groups=all,mswitch nodetype=switch mgt=switch

    # Note the mswitch group as switches and add the default admin password for mellanox.
    tabch switch=mswitch switches.sshusername=admin switches.sshpassword=admin switches.switchtype=MellanoxIB
    tabch key=mswitch  passwd.username=admin passwd.password=admin

    # Configure and enable ssh on the switches.
    rspconfig <IB Switches/Groups> sshcfg=enable

At this point the IB Switch should be set up for use with ssh.

.. warning:: Try to ssh to the switch before moving on to the next step.
.. warning:: Make sure the password and username attributes are set for the switch in xCAT.

.. _xCat-Infiniband: http://xcat-docs.readthedocs.io/en/stable/advanced/networks/infiniband/index.html

IB Polling Script
##################

.. code-block:: none

    `/opt/ibm/csm/bigdata/DataAggregators/sensor_monitoring/ib_temperature_poll.sh`


:max_parallel_threads: The number of parallel subprocesses the script will spawn.

:csv_headers: Sets the order of the of the temperatures aggregated; comma separated 
    and may have any number of the following values (no duplicates):
    
    * CPU_Core_Sensor_T1
    * CPU_Core_Sensor_T2
    * CPU_package_Sensor
    * power-mon_PS1
    * power-mon_PS2
    * Board_AMB_temp
    * Ports_AMB_temp
    * SIB
    
    .. warning:: If this field is changed, the Logstash :ref: `logstash-patterns` file will need to be changed.

:logstash_server: The hostname or IP address of the Logstash Server.

    .. warning:: This field MUST be changed.

:logstash_port: Replace with the port set in the Logstash :ref: `logstash-configuration-file` for BMC Temperature Sensor.

:xcat_switch_user: The user name to access the switch with, this should be in xCAT.

:xcat_groups: The group of switches to query, may be an xCAT group or a csv list of hostnames.


.. warning:: Make sure the IB Switches being targeted are managed by the Service or Management Node this polling utility is run on.


The `ib_temperature_poll.sh` script executes `show temperature` on each IB Switch specified 
over ssh. The actual values are captured, due to the small number of available sensors.
As mentioned above, all temperature data is in Celsius.

The full list of values gathered by this script to send follows:

* sensor_unix_time 
* ib_hostname
* ib_temp_cpu_core_t1
* ib_temp_cpu_core_t2
* ib_temp_cpu_package
* ib_temp_power_mon_ps1
* ib_temp_power_mon_ps2
* ib_temp_board_ambient
* ib_temp_ports_ambient
* ib_temp_SIB

This script must be executed from the Service or Management node that manages the 
IB Switches specified in `xcat_groups`.

To set up the automation add the following to the `crontab` on either the Sevice or Management
Node that manages the IB Switches to be polled by this script.

.. code-block:: none

    */30 * * * * cd /opt/ibm/csm/bigdata/DataAggregators/sensor_monitoring/; \
      /bin/bash /opt/ibm/csm/bigdata/DataAggregators/sensor_monitoring/ib_temperature_poll.sh >/dev/null 2>&1

.. note:: This cron entry will poll the listed nodes once every 30 minutes.

Zimon
-----

:Logstash Port: 10519

:Configuration File: 
    | `/opt/ibm/csm/bigdata/DataAggregators/zimon/zimon_hosts`

:Script File: `/opt/ibm/csm/bigdata/DataAggregators/zimon/data_collection.sh`

.. note:: This Documentation assumes that a gpfs server capable of performing 
   zimon queries is provisioned in the cluster.

.. warning:: This Data Aggregation must be run from a gpfs node with the perfmon 
   designation. If the gpfs cluster is an ESS, use the management node.

.. note:: If the perfmon designation is not set for your management node it may be set with:
    `mmchnode --quorum --perfmon  -N <Daemon node name>`

Zimon is the performance monitoring tool used by GPFS to aggregate sensor data.
To configure your GPFS cluster to use zimon for the Data Aggregation Sample, please complete the following steps:

1. Install GPFS in your cluster ( this document was written with an ESS Configuration ).
2. Configure `/opt/IBM/zimon/ZIMonSensors.cfg` to monitor the appropriate `Sensors`_. 
3. Run the following to start the performance monitor sensors:

.. code-block:: none

    /bin/systemctl enable pmsensors.service
    /bin/systemctl start  pmsensors.service
    /bin/systemctl enable pmcollector.service
    /bin/systemctl start pmcollector.service

.. note:: `GPFSNSDDisk` is the only change recommended at this time: `period` 1=>0

.. note:: The `host` attribute in `collectors` should be the node that `data_collection.sh`_ is run from.

At this point the performance data should be being aggregated by Zimon, to send this 
data to the Big Data Store, please consult `data_collection.sh`_.

.. note::
    The breadth of sensors tracked by this tool exceeds the scope of this documentation,
    for more details please visit the IBM Knowledge Center and review `Performance Monitoring`_.

.. _Performance Monitoring:
   http://www.ibm.com/support/knowledgecenter/STXKQY_4.1.1/com.ibm.spectrum.scale.v4r11.adv.doc/bl1adv_perfandhealthmonitoring.htm

.. _Sensors:
   http://www.ibm.com/support/knowledgecenter/STXKQY_4.2.1/com.ibm.spectrum.scale.v4r21.doc/bl1hlp_monnodesoverviewcharts.htm

zimon_hosts
^^^^^^^^^^^

.. code-block:: none

   `/opt/ibm/csm/bigdata/DataAggregators/zimon/zimon_hosts`

A flat file which stores the list of hostnames to execute the zimon metrics query on.

.. warning:: This must be set for the cluster environment 

data_collection.sh
^^^^^^^^^^^^^^^^^^

.. code-block:: none

    `/opt/ibm/csm/bigdata/DataAggregators/zimon/data_collection.sh`

The data collection script is run at an interval, polling sensor data with a zimon 
query. The metics polled, and the order in which they are presented to the 
Big Data Store are outlined in this script.

The following attributes must be set before execution:

:BIG_DATA_STORE_IP:
    The IP address or hostname of the Big Data Store. Used to determine the 
    address to write the zimon output to. Should be a logstash server.

:BIG_DATA_STORE_PORT:
    The port monitoring zimon data logs on the big data store. 
    See :ref: `logstash-configuration-file` for details.
    `10519` is the recommended port number.

:HOSTS_FILE:
    The file that tracks the nodes to gather zimon data from. 
    See `zimon_hosts`_ for details.

When executed the script will aggregate the following metrics for each node in `zimon_hosts_`:

* hostname
* cpu_system
* cpu_user
* mem_active
* gpfs_ns_bytes_read
* gpfs_ns_bytes_written
* gpfs_ns_tot_queue_wait_rd
* gpfs_ns_tot_queue_wait_wr

To set up the automation add the following to the `crontab` on the zimon collector node.

.. code-block:: none

    */30 * * * * cd /opt/ibm/csm/bigdata/DataAggregators/zimon/; \
        /bin/bash /opt/ibm/csm/bigdata/DataAggregators/zimon/data_collection.sh >/dev/null 2>&1


.. note:: This cron entry will poll the listed nodes once every 30 minutes, but the granularity depends on the zimon configuration.


.. Links
.. _xCat-GoConserver: http://xcat-docs.readthedocs.io/en/stable/advanced/goconserver/

