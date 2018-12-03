.. _CASTDataAgg:

Data Aggregation
================

Data Aggregation in CAST utilizes the logstash pipeline to process events and pass it along to 
Elasticsearch.

.. note:: In the following documentation, examples requiring replacement will be annotated with the 
    bash style `${variable_name}` and followed by an explanation of the variable.

.. contents::
    :local:

Logs
----

The default configuration of the CAST Big Data Store has support for a number of logging types,
most of which are processed through the syslog utility and then enriched by Logstash and 
the CAST Event Correlator.

.. TODO: Add more context?

.. _SyslogDataAgg:

Syslog
******

:Logstash Port: 10515

Syslog is generally aggregated through the use of the rsyslog daemon. 

Most devices are capable of producing syslogs, and it is suggested that syslogs should be sent
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

CSM recommends using the goconserver bundled in the xCAT dependencies and documented in xCat-GoConserver_.
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

   net add syslog host ipv4 ${logstash_node} port tcp ${syslog_port}
   net commit 

This command will populate the  */etc/rsyslog.d/11-remotesyslog.conf* file with a rule to
export the syslog to the supplied hostname and port. If using the default CAST syslog configuration
this file will need to be modified to have the CAST syslog template:

.. code-block:: bash

    vi /etc/rsyslog.d/11-remotesyslog.conf
        
        $template logFormat, "%TIMESTAMP:::date-rfc3339% %HOSTNAME% %APP-NAME% %PROCID% %syslogseverity-text% %msg%\n"
        *.*;cron.none @@${logstash_node}:${syslog_port};logFormat
    sudo service rsyslog restart


.. note:: For more configuration details please refer to the official `Cumulus Linux User Guide`_.


Counters
--------

The default configuration of the CAST Big Data Store has support for a number of counter types,
most of which are processed through Logstash and the CAST Event Correlator.

GPFS
****

In order to collect counters from the GPFS file system CAST leverages the zimon utility. A python
script interacting with this utility is provided in the `ibm-csm-bds-*.noarch.rpm`.

The following document assumes that the cluster's service nodes be running the `pmcollector`
service and any nodes requiring metrics be running `pmsensors`.


.. _zimon.collector:

Collector
^^^^^^^^^

:rpms: * `gpfs.gss.pmcollector.ppc64le` (Version 5.0 or greater)
       * `gpfs.base.ppc64le`  (Version 5.0 or greater)
:config: `/opt/IBM/zimon/ZIMonCollector.cfg`

In the CAST architecture a `pmcollector` should be run on each of the service node in federated mode.
To configure federated mode on the collector add all of the nodes configured as collectors to the
*/opt/IBM/zimon/ZIMonCollector.cfg* this configuration should be then propagated to all of the
collector nodes in the cluster.

.. code-block:: none

    peers = {
        host = "collector1"
        port = "9085"
    },
    {
        host = "collector2"
        port = "9085"
    },
    {
        host = "collector3"
        port = "9085"
    }

After configuring the collector start and enable the pmcollectors.

.. code-block:: bash

    systemctl start pmcollector
    systemctl enable pmcollector

Sensors
^^^^^^^

:RPMs: `gpfs.gss.pmsensors.ppc64le` (Version 5.0 or greater)
:Config: `/opt/IBM/zimon/ZIMonSensors.cfg`

It is recommended to use the GPFS managed configuration file through use of the `mmperfmon` command.
Before setting the node to do performance monitoring it's recommended that at least the following
command be run:

.. code-block:: bash

   /usr/lpp/mmfs/bin/mmperfmon config generate --collectors ${collectors}
   /usr/lpp/mmfs/bin/mmperfmon config update GPFSNode.period=0

It's recommended to specify at least two collectors defined in the `zimon.collector`_ section of this
document. The `pmsensor` service will attempt to distribute the load and account for failover in 
the event of a downed collector.

After generating the sensor configuration the nodes must then be set to `perfmon`:

.. code-block:: bash

   $ /usr/lpp/mmfs/bin/mmchnode --perfmon -N ${nodes}

Assuming `/opt/IBM/zimon/ZIMonSensors.cfg` has been properly distributed the sensors may then
be started on the nodes.

.. code-block:: bash

    $ systemctl start pmsensors
    $ systemctl enable pmsensors

.. attention:: To detect failures of the power hardware the following must be prepared on the 
   management node of the GPFS cluster.

.. code-block:: bash
   
   $ vi /var/mmfs/mmsysmon/mmsysmonitor.conf
        [general]
        powerhw_enabled=True
   
   $ mmsysmoncontrol restart


Python Script
^^^^^^^^^^^^^

:CAST RPM: `ibm-csm-bds-*.noarch.rpm`
:Script Location: `/opt/ibm/csm/bigdata/data-aggregators/zimonCollector.py`
:Dependencies: `gpfs.base.ppc64le`  (Version 5.0 or greater)

CAST provides a script for easily querying zimon, then sending the results to Big Data Store.
The `zimonCollector.py` python script leverages the python interface to zimon bundled in the 
`gpfs.base` rpm. The help output for this script is duplicated below:

.. code-block:: none

    A tool for extracting zimon sensor data from a gpfs collector node and shipping it in a json 
    format to logstash. Intended to be run from a cron job.

    Options:
    Flag                              | Description < default >
    ==================================|============================================================
    -h, --help                        | Displays this message.
    --collector <host>                | The hostname of the gpfs collector. <127.0.0.1>
    --collector-port <port>           | The collector port for gpfs collector. <9084>
    --logstash <host>                 | The logstash instance to send the JSON to. <127.0.0.1>
    --logstash-port <port>            | The logstash port to send the JSON to. <10522>
    --bucket-size <int>               | The size of the bucket accumulation in seconds. <60>
    --num-buckets <int>               | The number of buckets to retrieve in the query. <10>
    --metrics <Metric1[,Metric2,...]> | A comma separated list of zimon sensors to get metrics from.
                                      |  <cpu_system,cpu_user,mem_active,gpfs_ns_bytes_read,
                                      |      gpfs_ns_bytes_written,gpfs_ns_tot_queue_wait_rd,
                                      |      gpfs_ns_tot_queue_wait_wr>


CAST expects this script to be run from a service node configured for both logstash and zimon collection.
In this release this script need only be executed on one service node in the cluster to gather sensor data.

The recommended cron configuration for this script is as follows:

.. code-block:: bash

   */10 * * * * /opt/ibm/csm/bigdata/data-aggregators/zimonCollector.py

The output of this script is a newline delimited list of JSON designed for easy ingestion by the 
logstash pipeline. A sample from the default script configuration is as follows:

.. code-block:: javascript

    {
        "type": "zimon",
        "source": "c650f99p06",
        "data": {
          "gpfs_ns_bytes_written": 0,
          "mem_active": 1769963,
          "cpu_system": 0.015,
          "cpu_user": 0.004833,
          "gpfs_ns_tot_queue_wait_rd": 0,
          "gpfs_ns_bytes_read": 0,
          "gpfs_ns_tot_queue_wait_wr": 0
        },
        "timestamp": 1529960640
    }

In the default configuration of this script records will be shipped as `JSONDataSources`_.


UFM
***

:CAST RPM: `ibm-csm-bds-*.noarch.rpm`
:Script Location: `/opt/ibm/csm/bigdata/data-aggregators/ufmCollector.py`

CAST provides a python script to gather UFM counter data. The script is intended to be run
from either a service node running logstash or the UFM node as a cron job. A description of 
the script from the help functionality is reproduced below:

.. code-block:: none

 Purpose: Simple script that is packaged with BDS. Can be run individually and
 independantly when ever called upon.

 Usage:
 - Run the program.
   - pass in parameters.
      - REQUIRED [--ufm] : This tells program where UFM is (an IP address)
      - REQUIRED [--logstash] : This tells program where logstash is (an IP address)
      - OPTIONAL [--logstash-port] : This specifies the port for logstash
      - OPTIONAL [--ufm_restAPI_args-attributes] : attributes for ufm restAPI
        - CSV
          Example:
            - Value1
            - Value1,Value2
      - OPTIONAL [--ufm_restAPI_args-functions] : functions for ufm restAPI
        - CSV
      - OPTIONAL [--ufm_restAPI_args-scope_object] : scope_object for ufm restAPI
        - single string
      - OPTIONAL [--ufm_restAPI_args-interval] : interval for ufm restAPI
        - int
      - OPTIONAL [--ufm_restAPI_args-monitor_object] : monitor_object for ufm restAPI
        - single string
      - OPTIONAL [--ufm_restAPI_args-objects] : objects for ufm restAPI
        - CSV
      FOR ALL ufm_restAPI related arguments:
        - see ufm restAPI for documentation
        - json format
        - program provides default value if no user provides


The recommended cron configuration for this script is as follows:

.. code-block:: bash

   */10 * * * * /opt/ibm/csm/bigdata/data-aggregators/ufmCollector.py

The output of this script is a newline delimited list of JSON designed for easy ingestion by the 
logstash pipeline. A sample from the default script configuration is as follows:

.. code-block:: javascript

    {
        "type": "counters-ufm",
        "source": "port2",
        "statistics": {
           ... 
        },
        "timestamp": 1529960640
    }

In the default configuration of this script records will be shipped as `JSONDataSources`_.

.. _JSONDataSources:

JSON Data Sources
-----------------

:Logstash Port: 10522
:Required Field: `type`
:Recommended Fields: `timestamp`

.. attention:: This section is currently a work in progress.

CAST recommends JSON data sources be shipped to Logstash to leverage the batching and data enrichment
tool. The default logstash configuration shipped with CAST will designate port `10522`. JSON shipped
to this port should have the `type` field specified. This `type` field will be used in defining the
name of the index.

Data Aggregators shipping to this port will generate indices with the following name format:
`cast-%{type}-%{+YYYY.MM.dd}`

crass bmc alerts
****************

While not bundled with CAST the crass daemon is used to monitor BMC events and counters.
The following document is written assuming you have access to an `ibm-crassd-*.ppc64le` rpm.

1. Install the rpm:

.. code-block:: bash

   yum install -y ibm-crassd-*.ppc64le.rpm 

2. Edit the configuration file located at `/opt/ibm/ras/etc/ibm-crassd.config`:

This file neds the `[logstash]` configuration section configured and  `logstash=True` in
the `[notify]` section.

3. Start crassd:

.. code-block:: bash

   systemctl start ibm-crassd

.. attention:: The above section is a limited rundown of crassd configuration, for greater detail
    consult the official documentation for crassd.
  

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

CSM Buckets
***********

:Logstash Port: 10522

CSM provides a mechanism for running buckets to aggregate environmental and counter data from 
a variety of sources in the cluster. This data will be aggregated and shipped by the CSM 
aggregator to a logstash server (typically the local logstash server).


Each run of a bucket will be encapsulated in a JSON document with the following pattern:

.. code-block:: javascript

    {
        "type": "type-of-record",
        "source": "source-of-record",
        "timestamp": "timestamp-of-record",
        "data": {
            ...
        }
    }

:type:  The type of the bucket, used to determine the appropriate index.
:source: The source of the bucket run (typically a hostname, but can depend on the bucket).
:timestamp: The timestamp of the collection
:data: The actual data from the bucket run.

.. note:: Each JSON document is newline delimited.

CSM Configuration
^^^^^^^^^^^^^^^^^

In the aggregator configuration file the following must be configured to enable this feature:

.. code-block:: javascript
    
    "bds" : {
        "host" : "__LOGSTASH_IP__"
        "port" : 10522
    }

:host: The hostname the logstash server is configured on.
:port: A tcp port capable of receiving a JSON encoded message. `10522` is the default port in CAST
    logstash configuration files.

This will ship the environmental data to the specified ip and port. Officially CAST suggests the
use of logstash for this feature and suggests targeting the local logstash instance running on the
service node.

.. attention:: For users not employing logstash in their solution the output of this feature is
   a newline delimited list of JSON documents formatted as seen above.

Logstash Configuration
^^^^^^^^^^^^^^^^^^^^^^

CAST uses a generic port (`10522`) for processing data matching the `JSONDataSources`_ pattern. 
The default logstash configuration file specifies the following in the `input` section of the
configuration file:

.. code-block:: none
    
    tcp {
        port => 10522
        codec => "json"
    }

Default Buckets
^^^^^^^^^^^^^^^

CSM supplies several default buckets for environmental collection:

+-------------+----------+-----------------------------------------------+
| Bucket Type | Source   | Description                                   |
+=============+==========+===============================================+
| csm-env-gpu | Hostname | Environmental counters about the node's GPUs. |
+-------------+----------+-----------------------------------------------+

.. _DataArchiving:

Database Archiving
******************

:Logstash Port: 10523
:Script Location: /opt/ibm/csm/db/csm_db_history_archive.sh
:Script RPM: `csm-csmdb-*.rpm`

CAST supplies a command line utility for archiving the contents of the CSM database history tables. 
When run the utility (`csm_db_history_archive.sh`) will append to a daily JSON dump file 
(`<table>.archive.<YYYY>-<MM>-<DD>.json`) the contents of all history tables and the RAS event 
action table. The content appended is the next `n` records without a archive time as provided to 
the command line utility.Any records archived in this manner are then marked with an archive time 
for their eventual removal from the database. The utility should be executed on the node running
the CSM Postgres database.

Each row archived in this way will be converted to a JSON document with the following pattern:

.. code-block:: javascript
    
    { 
        "type": "db-<table-name>", 
        "data": { "<table-row-contents>" } 
    } 

:type: The table in the database, converted to index in default configuration.
:data: Encapsulates the row data.

CAST recommends the use of a cron job to run this archival. The following sample runs every 
five minutes, gathers up to 100 unarchived records from the csmdb tables, then appends the JSON
formatted records to the daily dump file in the `/var/log/ibm/csm/archive` directory.

.. code-block:: bash

   $ crontab -e 
    */5 * * * * /opt/ibm/csm/db/csm_db_history_archive.sh -d csmdb -n 100 -t /var/log/ibm/csm/archive

CAST recommends ingesting this data through the `filebeats`_ utility. A sample log configuration is 
given below:

.. code-block:: YAML

    filebeat.prospectors:
    - type: log 
      enabled: true
      paths:
        - "/var/log/ibm/csm/archive/*.json"
      # CAST recommends tagging all filebeats input sources.
      tags: ["archive"]

.. note:: For the sake of brevity further filebeats configuration documentation will be omitted. 
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
            hosts => [<elastic-server>:<port>]
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
* Allocation step begin/end

This transaction log represents a set of events that may be assembled to create the current state of
an event in a Big Data Store. 

In the CSM design these transactions are intended to be stored in a single elasticsearch index
each transaction should be identified by a `uid` in the index.

CSM Configuration
^^^^^^^^^^^^^^^^^

To enable the transaction logging mechanism the following configuration settings must be specified
in the CSM master configuration file:

.. code-block:: javascript

    "log" :
    {
        "transaction"                : true,
        "transaction_file"           : "/var/log/ibm/csm/csm_transaction.log",
        "transaction_rotation_size"  : 1000000000,
         "allocation"                : true,
         "allocation_file"           : "/var/log/ibm/csm/csm_allocation_metrics.log",
         "allocation_rotation_size"  : 1000000000
    }

:transaction: Enables the mechanism transaction log mechanism. 
:transaction_file: Specifies the location the transaction log will be saved to.
:transaction_rotation_size: The size of the file (in bytes) to rotate the log at.

:allocation: Enables the mechanism allocation metrics log mechanism. 
:allocation_file: Specifies the location the allocation metrics log will be saved to.
:allocation_rotation_size: The size of the file (in bytes) to rotate the log at.

Each transaction record will follow the following pattern:

.. code-block:: javascript
    
    { 
        "type": "<transaction-type>", 
        "data": { <table-row-contents>},
        "traceid":<traceid-api>,
        "uid": <unique-id>
    }

:type: The type of the transaction, converted to index in default configuration.
:data: Encapsulates the transactional data.
:traceid: The API's trace id as used in the CSM API trace functionality.
:uid: A unique identifier for the record in the elasticsearch index.

Allocation metrics will match the following pattern:

.. code-block:: javascript

    { 
        "type": "<metric-type>", 
        "data": { <metric data> },
    }

:type: The type of the allocation metric, converted to index in default configuration.
:data: Encapsulates the allocation metric data.


Filebeats Configuration
^^^^^^^^^^^^^^^^^^^^^^^

CAST recommends ingesting this data through the `filebeats`_ utility. A sample log configuration is 
given below:

.. code-block:: YAML

    filebeat.prospectors:
    - type: log
      enabled: true
      paths:
        - /var/log/ibm/csm/csm_transaction.log
      tags: ["transaction"]

.. note:: For the sake of brevity further filebeats configuration documentation will be omitted. 
    Please refer to the `filebeats`_ documentation for more details.

.. warning:: Filebeats has some difficulty with rollover events.

Logstash Configuration
^^^^^^^^^^^^^^^^^^^^^^

To configure logstash to ingest the archives the `beats` input plugin must be used, CAST recommends
port `10523` for ingesting `beats` records. Please note that this configuration only creates one
index for each transaction log type, this is to prevent transactions that span days from duplicating
logs.

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
            hosts => [<elastic-server>:<port>]
            action => "update"
            index => "cast-%{type}"
            http_compression =>true
            doc_as_upsert => true
            document_id => "%{uid}"
            document_type => "_doc"
        }
    }

The resulting indices for this configuration will be one per transaction type with each document 
corresponding to the current state of a set of transactions.


Supported Transactions
^^^^^^^^^^^^^^^^^^^^^^

The following transactions currently tracked by CSM are as follows:

+-----------------+---------------------------+-------------------------------------------------------------+
| `type`          | `uid`                     | `data`                                                      |
+=================+===========================+=============================================================+
| allocation      | <allocation_id>           | Superset of `csmi_allocation_t`.                            |
|                 |                           | Adds `running-start-timestamp` and `running-end-timestamp`. |
|                 |                           | Failed allocation creates have special `state`: `reverted`. |
+-----------------+---------------------------+-------------------------------------------------------------+
| allocation-step | <allocation_id>-<step_id> | Direct copy of `csmi_allocation_step_t`.                    |
+-----------------+---------------------------+-------------------------------------------------------------+


.. Links
.. _xCat-GoConserver: http://xcat-docs.readthedocs.io/en/stable/advanced/goconserver/
.. _Cumulus Linux User Guide:  https://docs.cumulusnetworks.com/display/DOCS/Cumulus+Linux+User+Guide
.. _filebeats: https://www.elastic.co/guide/en/beats/filebeat/current/filebeat-getting-started.html
