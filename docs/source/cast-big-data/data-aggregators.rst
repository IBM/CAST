Data Aggregation
================

Data Aggregation in CAST utilizes the Logstash pipeline to process events and pass it along to 
Elasticsearch.

.. note:: In the following documentation, examples requiring replacement will be annotated with the bash style 
    `${variable_name}` and followed by an explanation of the variable.

.. contents::
    :local:


Logs
----

The default configuration of the CAST Big Data Store has support for a number of logging types,
most of which are processed through the syslog utilty and then enriched by Logstash and 
the CAST Event Correlator.

.. TODO: Add more context?

.. _SyslogDataAgg:

Syslog
******

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

GPFS 
****

To redirect the GPFS logging data to the syslog please do the following on the Management node for GPFS:

.. code-block:: bash

    /usr/lpp/mmfs/bin/mmchconfig systemLogLevel=notice

After completing this process the gpfs log should now be forwarded to the `syslog` for the configured node.

.. note:: Refer to `Syslog Redirection`_ for gpfs log forwarding, the default syslog port is recommended (10515).

.. note:: The `systemLogLevel` attribute will forward logs of the specified level and higher to the syslog. It supports the following options: **alert**, **critical**, **error**, **warning**, **notice**, **configuration**, **informational**, **detail**, and **debug**.

.. note:: This data type will inhabit the same index as the *syslog* documents due to data similarity.


.. _UFMLogsDataAgg:

UFM
***

.. note:: This document assumes that the UFM daemon is up and running on the UFM Server.

The `Unified Fabric Manager` (UFM) has several distinct data logs to aggregate for the big data store.

System Event Log
^^^^^^^^^^^^^^^^

:Logstash Port: 10515

.. :Configuration Script: `/opt/ibm/csm/bigdata/DataAggregators/config_scripts/ufm_config.sh`

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

Console
*******

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

**Format**

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

Counters
--------

The default configuration of the CAST Big Data Store has support for a number of counter types,
most of which are processed through Logstash and the CAST Event Correlator.

GFPS
****

.. attention:: This section is currently a work in progress.

.. note:: The CAST team is currently in the process of reviewing the aggregation methodology.

UFM
***

.. attention:: This section is currently a work in progress.

.. note:: The CAST team is currently in the process of reviewing the aggregation methodology.

GPU
***

.. attention:: This section is currently a work in progress.

.. note:: The CAST team is currently in the process of reviewing the aggregation methodology.

Environmental
-------------

The default configuration of the CAST Big Data Store has support for a number of environmental types,
most of which are processed through Logstash and the CAST Event Correlator.

Node
****

.. attention:: This section is currently a work in progress.

.. note:: The CAST team is currently in the process of reviewing the aggregation methodology.

.. Links
.. _xCat-GoConserver: http://xcat-docs.readthedocs.io/en/stable/advanced/goconserver/

