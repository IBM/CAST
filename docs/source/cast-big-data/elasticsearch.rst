Elasticsearch
=============

`Elasticsearch`_ is a distributed analytics and search engine and the core component of the **E**\ LK 
stack. Elastic search ingests structured data (typically JSON or key value pairs) and stores the 
data in distributed index shards. 

In the CAST design the more Elasticsearch nodes the better. Generally speaking nodes with attached
storage or large numbers of drives are prefered. 


Configuration
-------------

.. note:: This guide has been tested using Elasticsearch 6.2.3, the latest RPM may be downloaded from
    `the Elastic Site <https://www.elastic.co/downloads/elasticsearch>`_.

The following is a brief introduction to the installation and configuration of the elasticsearch service.
It is generally assumed that elasticsearch is to be installed on multiple Big Data Nodes to take
advantage of the distributed nature of the service. Additionally, in the CAST configuration data drives
are assumed to be JBOD.

CAST provides a set of sample configuration files in the repository at `csm_big_data/Elasticsearch/`
If the `ibm-csm-bds-*.noarch.rpm` has been installed the sample configurations may be found
in `/opt/ibm/csm/bigdata/Elasticsearch/`.

1. Install the elasticsearch rpm and java 1.8.1+ (command run from directory with elasticsearch rpm):

.. code-block:: bash

    $ yum install -y elasticsearch-*.rpm java-1.8.*-openjdk

2. Copy the elastic search configuration files to the `/etc/elasticsearch` directory.
    It is recommended that the system administrator review these configurations at this 
    phase.

    * jvm.options
        * jvm options for the Elasticsearch service.
    * elasticsearch.yml
        * Configuration of the service specific attributes, please see `elasticsearch.yml`_ for details.

3. Make an ext4 filesystem on each hard drive designated to be in the elastic search JBOD. The
    mounted names for these file systems should match the names spcified in `path.data`. Additionally,
    these mounted file systems should be owned by the `elasticsearch` user and in the 
    `elasticsearch` group.

4. Start Elasticsearch:

.. code-block:: bash

    $ systemctl enable elasticsearch
    $ systemctl start elasticsearch

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

.. :bootstrap.system_call_filter: CAST defaults to false. 

.. TODO: Why is bootstrap.system_call_filter set to false (got this setting from lab).
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

CAST has specified a suite of data mappings for use in separate indices. Each of these indices is
documented below, with a *JSON* mapping file provided in the repository and *rpm*.

CAST uses *cast-<class>-<description>-<date>* naming schema for indices to leverage templates when creating
the indices in Elasticsearch. The *class* is one of the three primary classifications determined
by CAST: *log*, *counters*, *environmental*. The *description* is typically a one to two word description
of the type of data: *syslog*, *node*, *mellanox-event*, etc.

.. note:: Cast has elected to use lowercase and '-' characters to separate words. This is not mandatory
    for your index naming and creation.


Templates
^^^^^^^^^

CAST leverages the elasticsearch index templating system to assist in the defintion of "proper" indices.
Today each of the cast specified indices is given the *cast* template, followed by the exact specialization.

.. TODO: Flesh this section out. 


.. _SyslogElastic:

cast-log-syslog
^^^^^^^^^^^^^^^

:alias: cast-log-syslog
:shards: 5
:replication: 1

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

.. TODO: Should mmfs log inhabit the same index? The data has a 1:1 parity.

cast-log-mellanox-event
^^^^^^^^^^^^^^^^^^^^^^^

:alias: cast-log-mellanox-event
:shards: 5
:replication: 1

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
^^^^^^^^^^^^^^^^

:alias: cast-log-console
:shards: 5
:replication: 1

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
| hostname      | *text* | The hostname of the ufm aggregating the events.                | 
+---------------+--------+----------------------------------------------------------------+
| tags          | *text* | Tags containing additional metadata about the console log.     |
+---------------+--------+----------------------------------------------------------------+

cast-counters-gpfs
^^^^^^^^^^^^^^^^^^

:alias: cast-counters-gpfs
:shards: 5
:replication: 1

.. attention:: This section is currently a work in progress.

.. note:: The CAST team is currently in the process of reviewing the GPFS counter polling process
    it is likely that this index will be modified in the forseeable future.

cast-counters-ufm
^^^^^^^^^^^^^^^^^

:alias: cast-counters-ufm
:shards: 5
:replication: 1

.. attention:: This section is currently a work in progress.

.. note:: The CAST team is currently in the process of reviewing the ufm counters being aggregated
    it is likely that this index will be modified in the forseeable future.


cast-counters-gpu
^^^^^^^^^^^^^^^^^

:alias: cast-counters-gpu
:shards: 5
:replication: 1

.. attention:: This section is currently a work in progress.

.. note:: The CAST team is currently in the process of reviewing the GPU counters and the process
    of using DCGM to perform the aggregation operation.

cast-environmental-node
^^^^^^^^^^^^^^^^^^^^^^^

:alias: cast-environmental-node
:shards: 5
:replication: 1

.. attention:: This section is currently a work in progress.

.. note:: The CAST team is currently in the process of reviewing the inband aggregation of the node
    environmental data.

cast-csmdb
^^^^^^^^^^

.. attention:: This section is currently a work in progress

:alias: cast-csmdb
:shards: 1
:replication: 1


CSM history tables are archived in Elasticsearch as separate indices.
CAST provides a document on `configuring CSM database data archival <DataArchiving>`.

The mapping shared between the indices is as follows:

+---------------+--------+----------------------------------------------------------------+
| Field         | Type   | Description                                                    |
+===============+========+================================================================+
| @timestamp    | *date* | When console event occured.                                    | 
+---------------+--------+----------------------------------------------------------------+
| tags          | *text* | Tags containing additional metadata about the console log.     |
+---------------+--------+----------------------------------------------------------------+
| _table        | *text* | The originating table, drives index assignment.                |
+---------------+--------+----------------------------------------------------------------+

.. note:: The indices will have the following name format `cast-%{type}-%{+YYYY.MM.dd}`.


.. Links
.. _Elasticsearch: https://www.elastic.co/products/elasticsearch
.. _Configuring Elasticsearch: https://www.elastic.co/guide/en/elasticsearch/reference/current/settings.html
.. _xCat-GoConserver: http://xcat-docs.readthedocs.io/en/stable/advanced/goconserver/

