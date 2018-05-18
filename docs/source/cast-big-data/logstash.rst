Logstash
========

`Logstash`_ is an open-source data processing pipeline used in the E\ **L**\ K stack. The core function
of this service is to process unstructured data, typically syslogs, and then pass the newly structured
text to the elasticsearch service.

Typically the Logstash service is run on the service nodes in the xCAT infrastructure. This design
is to reduce the number of servers communicating with each instance of Logstash, distributing the 
workload. xCAT service nodes have failover capabilities removing the need for HAProxies to reduce the
risk of data loss. Finally, in using the service node the total cost of the Big Data Cluster
is reduced as the need for a dedicated node for data processing is removed.

CAST provides an event correlator for Logstash to assist in the generation of RAS events for
specific messages.

Configuration
-------------

.. note:: This guide has been tested using Logstash 6.2.3, the latest RPM may be downloaded from
   `the Elastic Site <https://www.elastic.co/downloads/logstash>`_.

The following is a brief introduction to the installation and configuration of the logstash service.
CAST provides a set of sample configuration files in the repository at `csm_big_data/Logstash/`.
If the `ibm-csm-bds-*.noarch.rpm` has been installed the sample configurations may be found 
in `/opt/ibm/csm/bigdata/Logstash/`.

1. Install the logstash rpm and java 1.8.1+ (command run from directory with logstash rpm):

.. code-block:: bash

    $ yum install -y logstash-*.rpm java-1.8.*-openjdk

2. Copy the Logstash pipeline configuration files to the appropriate directories. This step
    is ultimately optional, however it is recommended that these files be reviewed and modified
    by the system administrator at this phase:

    +-----------------------------+-----------+-----------+
    | Target file                 | Repo Dir  | RPM Dir   |
    +=============================+===========+===========+
    | logstash.yml                | config/   | config/   |
    +-----------------------------+-----------+-----------+
    | jvm.options                 | config/   | config/   |
    +-----------------------------+-----------+-----------+
    | conf.d/logstash.conf        | config/   | config/   |
    +-----------------------------+-----------+-----------+
    | patterns/ibm_grok.conf      | patterns/ | patterns/ |
    +-----------------------------+-----------+-----------+
    | patterns/mellanox_grok.conf | patterns/ | patterns/ |
    +-----------------------------+-----------+-----------+
    | patterns/events.yml         | patterns/ | patterns/ |
    +-----------------------------+-----------+-----------+

.. note:: Target files are relative to `/etc/logstash`. Repo Directories are relative to 
   `csm_big_data/Logstash`. RPM Directories are relative to `/opt/ibm/csm/bigdata/Logstash/`.

.. note:: The `conf.d/logstash.conf` file requires the ELASTIC-INSTANCE field be replaced with
   your cluster's elastic search nodes.

3. Install the `CSM Event Correlator`_
    
.. code:: bash

   $ bin/logstash-plugin install logstash-filter-csm-event-correlator-*.gem

.. note:: The bin directory is relative to your logstash install location.


4. Start Logstash:

.. code-block:: bash

    $ systemctl enable logstash
    $ systemctl start logstash

Logstash should now be operational. At this point data aggregators should be configured to point
to your Logstash node as appropriate.

Tuning Logstash
---------------

Tuning logstash is highly dependant on your use case and environment. What follows is a set of
recommendations based on the research and experimentation of the CAST Big Data team.

Here are some useful resources for learning more about profiling and tuning logstash:

* `Do you grok Grok?`_
* `Tuning Logstash`_

logstash.yml
^^^^^^^^^^^^

This configuration file specifies details about the Logstash service:

1. Path locations (as a rule of thumb these files should be owned by the `logstash` user).
2. Pipeline details (e.g. workers, threads, etc.)
3. Logging levels.

For more details please refer to the `Logstash settings file`_ documentation.

.. TODO Add more to this as CAST learns more

jvm.options
^^^^^^^^^^^

The configuration file for the Logstash JVM. The supplied settings are CAST's recommendation,
however, the efficacy of these settings entirely depends on your Logstash node.

logstash.conf
^^^^^^^^^^^^^

The `logstash.conf` is the core configuration file for determining the behavior of the Logstash
pipeline in the default CAST configuration. This configuration file is split into three components:
`input`_, `filter`_ and `output`_.

input
*****

The input section defines how the pipeline may ingest data. In the CAST sample only the `tcp` input
plugin is used. CAST currently uses different ports to assign tagging to facilitate simpler `filter`_
configuration. For a more in depth description of this section please refer to the 
`configuration file structure`_ in the official Logstash documentation.

The default ports and data tagging are as follows:

+--------------------------------------+
|       Default Port Values            |
+-----------------+--------------------+
|        Tag      |     Port Number    |
+=================+====================+
|      syslog     |       10515        |
+-----------------+--------------------+
| bmc_temp_sensor |       10516        |
+-----------------+--------------------+
| ib_temp_sensor  |       10517        |
+-----------------+--------------------+
|     bmc_sel     |       10518        |
+-----------------+--------------------+
|      zimon      |       10519        |
+-----------------+--------------------+
|      gocons     |       10520        |
+-----------------+--------------------+

filter
******

The filter section defines the data enrichment step of the pipeline. In the CAST sample the
following operations are performed:

#. Unstructured events are parsed with the `grok`_ utility.
#. Timestamps are reformatted (as needed).
#. Events with JSON formatting are parsed.
#. `CSM Event Correlator`_ is invoked on properly ingested logs.

Generally speaking care must be taken in this section to leverage branch prediction. Additionally,
it is easy to malform the `grok`_ plugin to result in slow downs in the pipeline performance.
Please consult `configuration file structure`_ in the official Logstash documentation for more
details.

output
******

The output section defines the target for the data processed through the pipeline. In the CAST
sample the `elasticsearch plugin`_ is used, for more details please refer to the linked documentation.

grok
^^^^

Logstash provides a `grok` utility to perform regular expression pattern recognition and extraction.
When writing grok patterns several rules of thumb are recommended by the CAST team:

1. Profile your patterns, `Do you grok Grok?`_ discusses a mechanism for profiling.
2. Grok failure can be expensive, use anchors (^ and $) to make string matches precise to reduce failure costs.
3. _groktimeout tagging can set an upper bound time limit for grok operations.
4. Avoid `DATA` and `GREEDYDATA` if possible.

CSM Event Correlator
---------------------

CSM Event Correlator (CEC) is the CAST solution for event correlation in the logstash pipeline.
CEC is written in ruby to leverage the existing Logstash plugin system. At its core CEC is a pattern
matching engine using `grok`_ to handle pattern matching. 

A sample configuration of CEC is provided as the `events.yml` file described in the `Configuration`_
section of the document. 

There's an extensive asciidoc for usage of the `CSM Event Correlator plugin`_. The following 
documentation is an abridged version.


Building CEC
^^^^^^^^^^^^^

CEC currently needs to be built before installation. This build process requires that ruby be installed
on the node. The following instructions assume that the build is being run on a node with the 
git repository.

.. code:: bash

   $ yum install -y ruby  
   $ cd csm_big_data/Logstash/plugins/csm_event_correlator
   $ gem build logstash-filter-csm_event_correlator.gemspec

.. TODO: Rename csm_event_correlator to cast_event_correlator.

After the plugin has been built it may then be installed with the steps described in 
:ref:`csm-event-correlator-config`


.. Links
.. _Logstash: https://www.elastic.co/products/logstash
.. _Do you grok Grok?: https://www.elastic.co/blog/do-you-grok-grok
.. _Tuning Logstash: https://www.elastic.co/guide/en/logstash/current/tuning-logstash.html
.. _configuration file structure: https://www.elastic.co/guide/en/logstash/current/configuration-file-structure.html
.. _elasticsearch plugin: https://www.elastic.co/guide/en/logstash/current/plugins-outputs-elasticsearch.html
.. _CSM Event Correlator plugin: https://github.com/IBM/CAST/blob/master/csm_big_data/Logstash/plugins/csm_event_correlator/doc/index.asciidoc
.. _Logstash settings file: https://www.elastic.co/guide/en/logstash/current/logstash-settings-file.html
