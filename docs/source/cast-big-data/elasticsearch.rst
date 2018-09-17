Elasticsearch
=============

`Elasticsearch`_ is a distributed analytics and search engine and the core component of the **E**\ LK 
stack. Elastic search ingests structured data (typically JSON or key value pairs) and stores the 
data in distributed index shards. 

In the CAST design the more Elasticsearch nodes the better. Generally speaking nodes with attached
storage or large numbers of drives are prefered. 

.. contents::


Configuration
-------------

.. note:: This guide has been tested using Elasticsearch 6.3.2, the latest RPM may be downloaded from
    `the Elastic Site <https://www.elastic.co/downloads/elasticsearch>`_.

The following is a brief introduction to the installation and configuration of the elasticsearch service.
It is generally assumed that elasticsearch is to be installed on multiple Big Data Nodes to take
advantage of the distributed nature of the service. Additionally, in the CAST configuration data drives
are assumed to be JBOD.

CAST provides a set of sample configuration files in the repository at `csm_big_data/elasticsearch/`
If the `ibm-csm-bds-*.noarch.rpm` rpm as been installed the sample configurations may be found
in `/opt/ibm/csm/bigdata/elasticsearch/`.

1. Install the elasticsearch rpm and java 1.8.1+ (command run from directory with elasticsearch rpm):

.. code-block:: bash

    yum install -y elasticsearch-*.rpm java-1.8.*-openjdk

2. Copy the elastic search configuration files to the `/etc/elasticsearch` directory. 

    It is recommended that the system administrator review these configurations at this phase.

    :jvm.options: jvm options for the Elasticsearch service.
    :elasticsearch.yml: Configuration of the service specific attributes, please see 
        `elasticsearch.yml`_ for details.

3. Make an ext4 filesystem on each hard drive designated to be in the elastic search JBOD. 

    The mounted names for these file systems should match the names spcified in `path.data`. 
    Additionally, these mounted file systems should be owned by the `elasticsearch` user and in the 
    `elasticsearch` group.

4. Start Elasticsearch:

.. code-block:: bash

    systemctl enable elasticsearch
    systemctl start elasticsearch

Elasticsearch should now be operational. If Logstash was properly configured there should already
be data being written to your index.

Tuning Elasticsearch
--------------------

The process of tuning and configuring Elasticsearch is incredibly dependen on the volume and
type of data ingested the Big Data Store. Due to the nuance of this process it is **STRONGLY** 
recommended that the system administrator familiarize themselves with `Configuring Elasticsearch`_.

The following document outlines the defaults and recommendations of CAST in the configuration of the 
Big Data Store.

elasticsearch.yml
^^^^^^^^^^^^^^^^^

.. note:: The following section outline's CAST's recommendations for the Elasticsearch configuration
    it is STRONGLY recommended that the system administrator familiarize themselves with 
    `Configuring Elasticsearch`_.

The Elasticsearch configuration sample shipped by CAST marks fields that need to be set by a 
system administrator. A brief rundown of the fields to modify is as follows:


:cluster.name: The name of the cluster. Nodes may only join clusters with the name in this field.
                Generally it's a good idea to give this a descriptive name.

:node.name: The name of the node in the elasticsearch cluster. CAST defaults to `${HOSTNAME}`.

:path.log: The logging directory, needs elasticsearch read write access.

:path.data: A comma separated listing of data directories, needs elasticsearch read write access. 
                CAST recommends a JBOD model where each disk has a file system.

:network.host: The address to bind the Elasticsearch model to. CAST defaults to `_site_`.

:http.port: The port to bind Elasticsearch to. CAST defaults to `9200`.

:discovery.zen.ping.unicast.hosts: A list of nodes likely to be active, comma delimited array.
                CAST defaults to `cast.elasticsearch.nodes`..

:discovery.zen.minimum_master_nodes: Number of nodes with the`node.master` setting set to true that
                must be connected to before starting. Elastic search recommends `(master_eligible_nodes/2)+1`

:gateway.recover_after_nodes: Number of nodes to wait for before begining recovery after cluster-wide restart.

:xpack.ml.enabled: Enables/disables the Machine Learning utility in xpack, 
                this should be disabled on ppc64le installations.

:xpack.security.enabled: Enables/disables security in elasticsearch.

:xpack.license.self_generated.type: Sets the license of xpack for the cluster, if the user has
                no license it should be set to `basic`.

.. TODO: Determine what the rpm install will do with this.
.. TODO: Determine some logical defaults.

jvm.options
^^^^^^^^^^^

The configuration file for the Logstash JVM. The supplied settings are CAST's recommendation,
however, the efficacy of these settings entirely depends on your elasticsearch node.

Generally speaking the only field to be changed is the heap size:

.. code-block:: none

   -Xms[HEAP MIN]
   -Xmx[HEAP MAX]


System Settings
^^^^^^^^^^^^^^^

.. TODO: Add more details.

Indices
--------

:Elasticsearch Templates: `/opt/ibm/csm/bigdata/elasticsearch/templates/cast-*.json`

CAST has specified a suite of data mappings for use in separate indices. Each of these indices is
documented below, with a *JSON* mapping file provided in the repository and *rpm*.

CAST uses *cast-<class>-<description>-<date>* naming schema for indices to leverage templates when creating
the indices in Elasticsearch. The *class* is one of the three primary classifications determined
by CAST: *log*, *counters*, *environmental*. The *description* is typically a one to two word description
of the type of data: *syslog*, *node*, *mellanox-event*, etc.


A collection of templates is provided in the CAST big data store RPM which set up aliases and data
type mappings. These temlates do not set sharding or replication factors, as these settings should 
be tuned to the user's data retention and index sizing needs.

The specified templates match indices generated in  :ref:`the data aggregators documentation <CASTDataAgg>`.
As different data sources produce different volumes of data in different environments, 
this document will make no recommendation on sharding or replication.


.. note:: These templates may be found on the git repo in `csm_big_data/elasticsearch/mappings/templates`.

.. note:: Cast has elected to use lowercase and '-' characters to separate words. This is not mandatory
    for your index naming and creation.

scripts
^^^^^^^

:Elasticsearch Index Scripts: `/opt/ibm/csm/bigdata/elasticsearch/`

CAST provides a set of scripts which allow the user to easily manipulate the elasticsearch indices
from the command line.

createIndices.sh
++++++++++++++++

removeIndices.sh
++++++++++++++++

reindexIndices.py
+++++++++++++++++


cast-log
^^^^^^^^

:Elasticsearch Templates: `/opt/ibm/csm/bigdata/elasticsearch/templates/cast-log*.json`

The *cast-log-* indices represent a set of logging indices produced by CAST supported data sources.

.. _SyslogElastic:

cast-log-syslog
+++++++++++++++

:alias: cast-log-syslog

The syslog index is designed to capture generic syslog messages. The contents of the syslog index is
considered by CAST to be the most useful data points for syslog analysis. CAST supplies both an 
rsyslog template and Logstash pattern, for details on these configurations please consult 
:ref:`the data aggregators documentation <SyslogDataAgg>`.

The mapping for the index contains the following fields:

+---------------+--------+----------------------------------------------------------------+
| Field         | Type   | Description                                                    |
+===============+========+================================================================+
| @timestamp    | *date* | The timestamp of the message, generated by the syslog utility. | 
+---------------+--------+----------------------------------------------------------------+
| host          | *text* | The host of the relay host.                                    |
+---------------+--------+----------------------------------------------------------------+
| hostname      | *text* | The hostname of the syslog origination.                        | 
+---------------+--------+----------------------------------------------------------------+
| program_name  | *text* | The name of the program which generated the log.               | 
+---------------+--------+----------------------------------------------------------------+
| process_id    | *long* | The process id of the program which generated the log.         | 
+---------------+--------+----------------------------------------------------------------+
| severity      | *text* | The severity level of the log.                                 | 
+---------------+--------+----------------------------------------------------------------+
| message       | *text* | The body of the message.                                       | 
+---------------+--------+----------------------------------------------------------------+
| tags          | *text* | Tags containing additional metadata about the message.         |
+---------------+--------+----------------------------------------------------------------+

.. note:: Currently mmfs and CAST logs will be stored in the syslog index (due to similarity of the data mapping).

cast-log-mellanox-event
+++++++++++++++++++++++

:alias: cast-log-mellanox-event

The mellanox event log is a superset of the `cast-log-syslog`_ index, an artifact of the event log being 
transmitted through syslog. In the CAST Big Data Pipeline this log will be ingested and parsed by
the Logstash service then transmitted to the Elasticsearch index.

+---------------+--------+----------------------------------------------------------------+
| Field         | Type   | Description                                                    |
+===============+========+================================================================+
| @timestamp    | *date* | When the message was written to the event log.                 | 
+---------------+--------+----------------------------------------------------------------+
| hostname      | *text* | The hostname of the ufm aggregating the events.                | 
+---------------+--------+----------------------------------------------------------------+
| program_name  | *text* | The name of the generating program, should be *event_log*      | 
+---------------+--------+----------------------------------------------------------------+
| process_id    | *long* | The process id of the program which generated the log.         | 
+---------------+--------+----------------------------------------------------------------+
| severity      | *text* | The severity level of the log, pulled from message.            | 
+---------------+--------+----------------------------------------------------------------+
| message       | *text* | The body of the message (unstructured).                        | 
+---------------+--------+----------------------------------------------------------------+
| log_counter   | *long* | A counter tracking the log number.                             |
+---------------+--------+----------------------------------------------------------------+
| event_id      | *long* | The unique identifier for the event in the mellanox event log. | 
+---------------+--------+----------------------------------------------------------------+
| event_type    | *text* | The type of event (e.g. HARDWARE) in the event log.            |
+---------------+--------+----------------------------------------------------------------+
| category      | *text* | The categorization of the error in the event log typing        | 
+---------------+--------+----------------------------------------------------------------+
| tags          | *text* | Tags containing additional metadata about the message.         |
+---------------+--------+----------------------------------------------------------------+

cast-log-console
++++++++++++++++

:alias: cast-log-console

CAST recommends the usage of the goconserver bundled in the xCAT dependicies, documented in xCat-GoConserver_.
Configuration of the goconserver should be performed on the xCAT service nodes in the cluster.
CAST has created a `limited configuration guide <ConsoleDataAggregator>`, please consult for 
a basic rundown on the utility.

The mapping for the *console* index is provided below:

+---------------+--------+----------------------------------------------------------------+
| Field         | Type   | Description                                                    |
+===============+========+================================================================+
| @timestamp    | *date* | When console event occured.                                    | 
+---------------+--------+----------------------------------------------------------------+
| type          | *text* | The type of the event (typically *console*).                   | 
+---------------+--------+----------------------------------------------------------------+
| message       | *text* | The console event data, typically a console line.              | 
+---------------+--------+----------------------------------------------------------------+
| hostname      | *text* | The hostname generating the console.                           | 
+---------------+--------+----------------------------------------------------------------+
| tags          | *text* | Tags containing additional metadata about the console log.     |
+---------------+--------+----------------------------------------------------------------+

cast-csm
^^^^^^^^

:Elasticsearch Templates: `/opt/ibm/csm/bigdata/elasticsearch/templates/cast-csm*.json`

The *cast-csm-* indices represent a set of metric indices produced by CSM. Indices matching this
pattern will be created unilaterally by the CSM Daemon. Typically records in this type of index
are generated by the Aggregator Daemon.


cast-csm-dimm-env
+++++++++++++++++

:alias: cast-csm-dimm-env

The mapping for the *cast-csm-dimm-env* index is provided below:

+-------------------------+--------+-------------------------------------------------+
| Field                   | Type   | Description                                     |
+=========================+========+=================================================+
| @timestamp              | *date* | Ingestion time of the dimm environment counters.|
+-------------------------+--------+-------------------------------------------------+
| timestamp               | *date* | When environment counters were gathered.        |
+-------------------------+--------+-------------------------------------------------+
| type                    | *text* | The type of the event (*csm-dimm-env*).         | 
+-------------------------+--------+-------------------------------------------------+
| source                  | *text* | The source of the counters.                     |
+-------------------------+--------+-------------------------------------------------+
| data.dimm_id            | *long* | The id of dimm being aggregated.                |
+-------------------------+--------+-------------------------------------------------+
| data.dimm_temp          | *long* | The temperature of the dimm.                    |
+-------------------------+--------+-------------------------------------------------+
| data.dimm_temp_max      | *long* | The max temperature of the dimm                 |
|                         |        | over the collection period.                     |
+-------------------------+--------+-------------------------------------------------+
| data.dimm_temp_min      | *long* | The min temperature of the dimm                 |
|                         |        | over the collection period.                     |
+-------------------------+--------+-------------------------------------------------+

cast-csm-gpu-env
++++++++++++++++

:alias: cast-csm-gpu-env

The mapping for the *cast-csm-gpu-env* index is provided below:

+-------------------------+--------+-------------------------------------------------+
| Field                   | Type   | Description                                     |
+=========================+========+=================================================+
| @timestamp              | *date* | Ingestion time of the gpu environment counters. |
+-------------------------+--------+-------------------------------------------------+
| timestamp               | *date* | When environment counters were gathered.        |
+-------------------------+--------+-------------------------------------------------+
| type                    | *text* | The type of the event (*csm-gpu-env*).          | 
+-------------------------+--------+-------------------------------------------------+
| source                  | *text* | The source of the counters.                     |
+-------------------------+--------+-------------------------------------------------+
| data.gpu_id             | *long* | The id of the GPU record being aggregated.      |
+-------------------------+--------+-------------------------------------------------+
| data.gpu_mem_temp       | *long* | The memory temperature of the GPU.              |
+-------------------------+--------+-------------------------------------------------+
| data.gpu_mem_temp_max   | *long* | The max memory temperature of the GPU           |
|                         |        | over the collection period.                     |
+-------------------------+--------+-------------------------------------------------+
| data.gpu_mem_temp_min   | *long* | The min memory temperature of the GPU           |
|                         |        | over the collection period.                     |
+-------------------------+--------+-------------------------------------------------+
| data.gpu_temp           | *long* | The temperature of the GPU.                     |
+-------------------------+--------+-------------------------------------------------+
| data.gpu_temp_max       | *long* | The max temperature of the GPU                  |
|                         |        | over the collection period.                     |
+-------------------------+--------+-------------------------------------------------+
| data.gpu_temp_min       | *long* | The min temperature of the GPU                  |
|                         |        | over the collection period.                     |
+-------------------------+--------+-------------------------------------------------+

cast-csm-node-env
+++++++++++++++++

:alias: cast-csm-node-env

The mapping for the *cast-csm-node-env* index is provided below:

+-------------------------+--------+-------------------------------------------------+
| Field                   | Type   | Description                                     |
+=========================+========+=================================================+
| @timestamp              | *date* | Ingestion time of the node environment counters.|
+-------------------------+--------+-------------------------------------------------+
| timestamp               | *date* | When environment counters were gathered.        |
+-------------------------+--------+-------------------------------------------------+
| type                    | *text* | The type of the event (*csm-node-env*).         | 
+-------------------------+--------+-------------------------------------------------+
| source                  | *text* | The source of the counters.                     |
+-------------------------+--------+-------------------------------------------------+
| data.system_energy      | *long* | The energy of the system at ingestion time.     |
+-------------------------+--------+-------------------------------------------------+

cast-csm-gpu-counters
+++++++++++++++++++++

:alias: cast-csm-gpu-counters

A listing of DCGM counters.

+---------------------------------+----------+-------------------------------------------------+
| Field                           | Type     | Description                                     |
+=================================+==========+=================================================+
| @timestamp                      | *date*   | Ingestion time of the gpu environment counters. |
+---------------------------------+----------+-------------------------------------------------+

.. note:: The data fields have been separated for compactness.

+---------------------------------+----------+-------------------------------------------------+
| Data Field                      | Type     | Description                                     |
+=================================+==========+=================================================+
| nvlink_recovery_error_count_l1  | *long*   | Total number of NVLink recovery errors.         |
+---------------------------------+----------+-------------------------------------------------+
| sync_boost_violation            | *long*   | Throttling duration due to                      |
|                                 |          | sync-boost constraints (in us)                  |
+---------------------------------+----------+-------------------------------------------------+
| gpu_temp                        | *long*   | GPU temperature (in C).                         |
+---------------------------------+----------+-------------------------------------------------+
| nvlink_bandwidth_l2             | *long*   | Total number of NVLink bandwidth counters.      | 
+---------------------------------+----------+-------------------------------------------------+
| dec_utilization                 | *long*   | Decoder utilization.                            |
+---------------------------------+----------+-------------------------------------------------+
| nvlink_recovery_error_count_l2  | *long*   | Total number of NVLink recovery errors.         |
+---------------------------------+----------+-------------------------------------------------+
| nvlink_bandwidth_l1             | *long*   | Total number of NVLink bandwidth counters.      |
+---------------------------------+----------+-------------------------------------------------+
| mem_copy_utilization            | *long*   | Memory utilization.                             |
+---------------------------------+----------+-------------------------------------------------+
| gpu_util_samples                | *double* | GPU utilization sample count.                   |
+---------------------------------+----------+-------------------------------------------------+
| nvlink_replay_error_count_l1    | *long*   | Total number of NVLink retries.                 |
+---------------------------------+----------+-------------------------------------------------+
| nvlink_data_crc_error_count_l1  | *long*   | Total number of NVLink data CRC errors.         |
+---------------------------------+----------+-------------------------------------------------+
| nvlink_replay_error_count_l0    | *long*   | Total number of NVLink retries.                 |
+---------------------------------+----------+-------------------------------------------------+
| nvlink_bandwidth_l0             | *long*   | Total number of NVLink bandwidth counters.      |
+---------------------------------+----------+-------------------------------------------------+
| nvlink_data_crc_error_count_l3  | *long*   | Total number of NVLink data CRC errors.         |
+---------------------------------+----------+-------------------------------------------------+
| nvlink_flit_crc_error_count_l3  | *long*   | Total number of NVLink flow-control CRC errors. | 
+---------------------------------+----------+-------------------------------------------------+
| nvlink_bandwidth_l3             | *long*   | Total number of NVLink bandwidth counters.      |
+---------------------------------+----------+-------------------------------------------------+
| nvlink_replay_error_count_l2    | *long*   | Total number of NVLink retries.                 |
+---------------------------------+----------+-------------------------------------------------+
| nvlink_replay_error_count_l3    | *long*   | Total number of NVLink retries.                 |
+---------------------------------+----------+-------------------------------------------------+
| nvlink_data_crc_error_count_l0  | *long*   | Total number of NVLink data CRC errors.         |
+---------------------------------+----------+-------------------------------------------------+
| nvlink_recovery_error_count_l0  | *long*   | Total number of NVLink recovery errors.         |
+---------------------------------+----------+-------------------------------------------------+
| enc_utilization                 | *long*   | Encoder utilization.                            |
+---------------------------------+----------+-------------------------------------------------+
| power_usage                     | *double* | Power draw (in W).                              |
+---------------------------------+----------+-------------------------------------------------+
| nvlink_recovery_error_count_l3  | *long*   | Total number of NVLink recovery errors.         |
+---------------------------------+----------+-------------------------------------------------+
| nvlink_data_crc_error_count_l2  | *long*   | Total number of NVLink data CRC errors.         |
+---------------------------------+----------+-------------------------------------------------+
| nvlink_flit_crc_error_count_l2  | *long*   | Total number of NVLink flow-control CRC errors. |
+---------------------------------+----------+-------------------------------------------------+
| serial_number                   | *text*   | The serial number of the GPU.                   |
+---------------------------------+----------+-------------------------------------------------+
| power_violation                 | *long*   | Throttling duration due to                      |
|                                 |          | power constraints (in us).                      |
+---------------------------------+----------+-------------------------------------------------+
| xid_errors                      | *long*   | Value of the last XID error encountered.        |
+---------------------------------+----------+-------------------------------------------------+
| gpu_utilization                 | *long*   | GPU utilization.                                |
+---------------------------------+----------+-------------------------------------------------+
| nvlink_flit_crc_error_count_l0  | *long*   | Total number of NVLink flow-control CRC errors. |
+---------------------------------+----------+-------------------------------------------------+
| nvlink_flit_crc_error_count_l1  | *long*   | Total number of NVLink flow-control CRC errors. |
+---------------------------------+----------+-------------------------------------------------+
| mem_util_samples                | *double* | The sample rate of the memory utilization.      |
+---------------------------------+----------+-------------------------------------------------+
| thermal_violation               | *long*   | Throttling duration due to                      |
|                                 |          | thermal constraints (in us).                    |
+---------------------------------+----------+-------------------------------------------------+

cast-counters
^^^^^^^^^^^^^

:Elasticsearch Templates: `/opt/ibm/csm/bigdata/elasticsearch/templates/cast-ccounters*.json`

A class of index representing counter aggregation from non CSM data flows. Generally indices 
following this naming pattern contain data from standalone data aggregation utilities.

cast-counters-gpfs
++++++++++++++++++

:alias: cast-counters-gpfs

A collection of counter data from gpfs. The script outlined in :ref:`the data aggregators documentation <CASTDataAgg>`
leverages zimon to perform the collection. The following is the index generated by the default 
script bundled in the CAST rpm.


+---------------------------------+----------+-------------------------------------------------+
| Field                           | Type     | Description                                     |
+=================================+==========+=================================================+
| @timestamp                      | *date*   | Ingestion time of the gpu environment counters. |
+---------------------------------+----------+-------------------------------------------------+

.. note:: The data fields have been separated for compactness.

+---------------------------------+----------+-------------------------------------------------+
| Data Field                      | Type     | Description                                     |
+=================================+==========+=================================================+
| cpu_system                      | *long*   | The system space usage of the CPU.              |
+---------------------------------+----------+-------------------------------------------------+
| cpu_user                        | *long*   | The user space usage of the CPU.                |
+---------------------------------+----------+-------------------------------------------------+
| mem_active                      | *long*   | Active memory usage.                            |
+---------------------------------+----------+-------------------------------------------------+
| gpfs_ns_bytes_read              | *long*   | Networked bytes read.                           |
+---------------------------------+----------+-------------------------------------------------+
| gpfs_ns_bytes_written           | *long*   | Networked bytes written.                        |
+---------------------------------+----------+-------------------------------------------------+
| gpfs_ns_tot_queue_wait_rd       | *long*   | Total time spent waiting in the network queue   |
|                                 |          | for read operations.                            |
+---------------------------------+----------+-------------------------------------------------+
| gpfs_ns_tot_queue_wait_wr       | *long*   | Total time spent waiting in the network queue   |
|                                 |          | for write operations.                           |
+---------------------------------+----------+-------------------------------------------------+

cast-counters-ufm
+++++++++++++++++

:alias: cast-counters-ufm

Due to the wide variety of counters that may be gathered checking the data aggregation script
is strongly recommended.

The mapping for the *cast-counters-ufm* index is provided below:

+-------------------------+--------+-------------------------------------------------+
| Field                   | Type   | Description                                     |
+=========================+========+=================================================+
| @timestamp              | *date* | Ingestion time of the ufm environment counters. |
+-------------------------+--------+-------------------------------------------------+
| timestamp               | *date* | When environment counters were gathered.        |
+-------------------------+--------+-------------------------------------------------+
| type                    | *text* | The type of the event (*cast-counters-ufm*).    | 
+-------------------------+--------+-------------------------------------------------+
| source                  | *text* | The source of the counters.                     |
+-------------------------+--------+-------------------------------------------------+

cast-db
^^^^^^^

CSM history tables are archived in Elasticsearch as separate indices.
CAST provides a document on `configuring CSM database data archival <DataArchiving>`.

The mapping shared between the indices is as follows:

+---------------+--------+----------------------------------------------------------------+
| Field         | Type   | Description                                                    |
+===============+========+================================================================+
| @timestamp    | *date* | When archival event occured.                                   | 
+---------------+--------+----------------------------------------------------------------+
| tags          | *text* | Tags about the archived data.                                  |
+---------------+--------+----------------------------------------------------------------+
| type          | *text* | The originating table, drives index assignment.                |
+---------------+--------+----------------------------------------------------------------+
| data          | *doc*  | The mapping of table columns, contents differ for each table.  |
+---------------+--------+----------------------------------------------------------------+

.. attention:: These indicies will match CSM database history tables, contents not replicated for
    brevity.

cast-ibm-crasssd-bmc-alerts
^^^^^^^^^^^^^^^^^^^^^^^^^^^

While not managed by CAST crassd will ship bmc alerts to the big data store. 

.. Links
.. _Elasticsearch: https://www.elastic.co/products/elasticsearch
.. _Configuring Elasticsearch: https://www.elastic.co/guide/en/elasticsearch/reference/current/settings.html
.. _xCat-GoConserver: http://xcat-docs.readthedocs.io/en/stable/advanced/goconserver/

