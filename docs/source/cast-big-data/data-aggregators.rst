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
+================+====================+
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


The rsyslog utility must then be restarted for the changes to take effect:

.. code-block:: bash
   
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

.. note:: The `systemLogLevel` attribute will forward logs of the specified level and higher to the 
   syslog. It supports the following options: **alert**, **critical**, **error**, **warning**, 
   **notice**, **configuration**, **informational**, **detail**, and **debug**.

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

:Logstash Port: 10522

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
              port: 10522             # This is the port in the sample configuration.
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


Cumulus Switch
**************

.. attention:: The CAST documentation was written using Cumulus Linux 3.5.2, please ensure the switch
    is at this level or higher.

Cumulus switch logging is performed through the usage of the rsyslog service. CAST recommends placing
Cumulus logging in the *syslog-log* indices at this time. 

Configuration of the logging on the switch can be achieved through the *net* command:

.. code-block:: bash

   $ net add syslog host ipv4 ${logstash_node} port tcp ${syslog_port}
   $ net commit 

This command will populate the  */etc/rsyslog.d/11-remotesyslog.conf* file with a rule to
export the syslog to the supplied hostname and port. If using the default CAST syslog configuration
this file will need to be modified to have the CAST syslog template:

.. code-block:: bash

    $ vi /etc/rsyslog.d/11-remotesyslog.conf
        
        $template logFormat, "%TIMESTAMP:::date-rfc3339% %HOSTNAME% %APP-NAME% %PROCID% %syslogseverity-text% %msg%\n"
        *.*;cron.none @@${logstash_node}:${syslog_port};logFormat
    $ sudo service rsyslog restart


.. note:: For more configuration details please refer to the official `Cumulus Linux User Guide`_.




Counters
--------

The default configuration of the CAST Big Data Store has support for a number of counter types,
most of which are processed through Logstash and the CAST Event Correlator.

GFPS
****

.. attention:: This section is currently a work in progress.

.. note:: The CAST team is currently in the process of reviewing the aggregation methodology.

To detect failures of the power hardware the following mustn be prepared on the management node
of the GPFS cluster:

.. code-block:: bash
   
   $ vi /var/mmfs/mmsysmon/mmsysmonitor.conf
        [general]
        powerhw_enabled=True
   
   $ mmsysmoncontrol restart

collector
^^^^^^^^^

.. code-block:: bash

    $ systemctl start pmcollector
    $ systemctl enable pmcollector

.. serivce nodes need `gpfs.gss.pmcollector`

`csm_big_data/data-aggregators/gpfs/zimon_collector.py`

sensors
^^^^^^^

.. `gpfs.gss.pmsensors`

.. mmchnode --perfmon -N <nodes>


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




JSON Data Sources
-----------------

:Logstash Port: 10522
:Required Field: `type`
:Recommended Fields: `@timestamp`

.. attention:: This section is currently a work in progress.

CAST recommends JSON data sources be shipped to Logstash to leverage the batching and data enrichment
tool. The default logstash configuration shipped with CAST will designate port `10522`. JSON shipped
to this port should have the `type` field specified. This `type` field will be used in defining the
name of the index.

Data Aggregators shipping to this port will generate indices with the following name format:
`cast-%{type}-%{+YYYY.MM.dd}`


CAST Data Sources
-----------------

csmd syslog
***********

:Logstash Port: 10515

CAST has enabled the boost syslog utility through use of the *csmd* configuration file.

.. code-block:: bash
    
    "csm" : {
        ...
        "log" : {
            ... 
            "sysLog" : true,
            "server" : "127.0.0.1",
            "port"   : "514"
        }
        ...
    }

By default enabling syslog will write to the localhost syslog port using UDP. The target may
be changed by the *server* and *port* options.

The syslog will follow the *RFC 3164* syslog protocol. After being filtered through the 
`Syslog Redirection`_ template the log will look something like this:

.. code-block:: bash

    2018-05-17T11:17:32-04:00 c650f03p37-mgt CAST - debug     csmapi TIMING: 1525910812,17,2,1526570252507364568,1526570252508039085,674517
    2018-05-17T11:17:32-04:00 c650f03p37-mgt CAST - info     csmapi [1525910812]; csm_allocation_query_active_all end
    2018-05-17T11:17:32-04:00 c650f03p37-mgt CAST - info     csmapi CSM_CMD_allocation_query_active_all[1525910812]; Client Recv; PID: 14921; UID:0; GID:0

These logs will then stored in the *cast-log-syslog* index using the default CAST configuration.



.. _DataArchiving:

Database Archiving
******************

:Logstash Port: 10523
:Script Location: /opt/ibm/csm/db/csm_db_history_archive.sh
:Script RPM: csm-csmdb-*.rpm

CAST supplies a commandline utility for archiving the contents of the database history tables. 
When run the utility (`csm_db_history_archive.sh`) will appened to a daily JSON dump file 
(`<table>.archive.<YYYY>-<MM>-<DD>.json`) the contents of all history tables and the RAS event 
action table. The content appended is the next `n` records without a archive time as provided to 
the commandline utility.Any records archived in this manner are then marked with an archive time 
for their eventaul removal from the database. The utility should be executed on the node running
the CSM Postgres database.

Each row archived in this way will be converted to a JSON document with the following pattern:

.. code-block:: javascript
    
    # type helps define the index in Logstash ingestion
    # data isolates column names to prevent collisions with any logstash enrichment.  
    { "type": "db-<table-name>", "data": { <table-row-contents>} } 


CAST recommends the use of a cron job to run this archival. The following sample runs every 
five minutes, gathers up to 100 unarchived records from the csmdb tables, then appends the JSON
formatted records to the daily dump file in the `/var/log/ibm/csm/archive` directory.

.. code-block:: bash

   $ crontab -e 
    */5 * * * * /opt/ibm/csm/db/csm_db_history_archive.sh -d csmdb -n 100 -t /var/log/ibm/csm/archive

CAST recommends ingesting this data through the `filebeats`_ utility. A sample log configuration is 
given below:

.. code-block:: YAML

    filebeat.inputs:
    - type: log 
      enabled: true
      paths:
        - "/var/log/ibm/csm/archive/*.json"
      # CAST recommends tagging all filebeats input sources.
      tags: ["archive"]

.. note:: For the sake of brevity further filebeats configuration documentation will be ommited. 
    Please refer to the `filebeats`_ documentation for more details.

To configure logstash to ingest the archives the `beats` input plugin must be used, CAST recommends
port `10523` for ingesting `beats` records as shown below:

.. code-block:: none

    input
    {
        beats { 
            port => 10523
            codec=>"json"
        }
    
    }
    filter
    {
        mutate {
            remove_field => [ "beat", "host", "source", "offset", "prospector"]
        }
    
    }
    output
    {
        elasticsearch { 
            hosts => ['10.7.4.15:9200','10.7.4.17:9200','10.7.4.19:9200']
            index => "cast-%{type}-%{+YYYY.MM.dd}"
            http_compression =>true
            document_type => "_doc"
        }
    }

In this sample configuration the archived history will be stored in the *cast-db-<table_name>* indices.


Transaction Log
***************

:Logstash Port:  10523 

.. note:: CAST only ships the transaction log to a local file, a utility such as Filebeats or
    a local Logstash service would be needed to ship the log to a Big Data Store.


CAST offers a transaction log for select CSM API events. Today the following events are tracked:

* Allocation create/delete/update

This transaction log represents a set of events that may be assembled to create the current state of
an event in a Big Data Store. 

In the CSM design these transactions are intended to be stored in a single elasticsearch index
each transaction should be identified by a `uid` in the index.

Configuration
^^^^^^^^^^^^^

CSM Configuration
#################

CAST has enabled the boost syslog utility through use of the *csmd* configuration file.

.. code-block:: bash
    
    "csm" : {
        ...
        "log" : {
            ... 
             "transaction"               :   true,
             "transaction_file"          :   "/var/log/ibm/csm/csm_transaction.log",
             "transaction_rotation_size" :   1000000000 
        }
        ...
    }

This configuration will generate a transaction log at `/var/log/ibm/csm/csm_transaction.log`,
rotating at 1GB.

+---------------------------+---------+--------------------------------------------------------------------------------------------+
| Field                     | Type    | Description                                                                                |
+---------------------------+---------+--------------------------------------------------------------------------------------------+
| transaction               | Boolean | Generates a transaction log if set to true.                                                |
+---------------------------+---------+--------------------------------------------------------------------------------------------+
| transaction_file          | Text    | The file to write the transaction log to.                                                  |
+---------------------------+---------+--------------------------------------------------------------------------------------------+
| transaction_rotation_size | Numeric | The file size in bytes at which to rotate the transaction log (by default doesn't rotate). |
+---------------------------+---------+--------------------------------------------------------------------------------------------+



Filebeats Configuration
#######################

To export this data to Logstash/Elasticsearch the CAST team recommends the use of the Filebeats
utility to export the data to logstash where the pipeline may properly enrich and target the correct
index for the transaction.

.. code-block:: YAML

    paths:
        - /var/log/ibm/csm/csm_transaction.log
    
    output.logstash:
        # The Logstash hosts
        hosts: ["<logstash-host>:10523"]


Events
^^^^^^

Transaction events will have the following pattern:

.. code-block:: javascript

    {
        "type": "type-of-event"
        "traceid": <API traceid (for correlation) >,
        "uid": <unique identifier, numeric>,
        "data": {
        }
    }

:type:  The type of the transaction, used to determine the appropriate index.
:traceid: Traceid for correlating a transaction with a logged message in the CSM log.
:uid: Unique id for the transaction (usually correlates to something in `data`).
:data: The actual data changes in the transaction.


Allocation
##########

.. TODO: How do I document this well?


CSM Buckets
***********

:Logstash Port: 10522


.. code-block:: javascript

    {
        "type": "type-of-record"
        "timestamp": "Timestamp of record"
        "data": {
            ...
        }
    }

:type:  The type of the data, used to determine the appropriate index.
:timestamp: The timestamp of the collection
:data: The actual data from the bucket run.

.. Links
.. _xCat-GoConserver: http://xcat-docs.readthedocs.io/en/stable/advanced/goconserver/
.. _Cumulus Linux User Guide:  https://docs.cumulusnetworks.com/display/DOCS/Cumulus+Linux+User+Guide
.. _filebeats: https://www.elastic.co/guide/en/beats/filebeat/current/filebeat-getting-started.html
