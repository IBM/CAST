.. _CASTLogstash:

Logstash
========

`Logstash`_ is an open-source data processing pipeline used in the E\ **L**\ K stack. The core function
of this service is to process unstructured data, typically syslogs, and then pass the newly structured
text to the elasticsearch service.

Typically, in the CAST design, the Logstash service is run on the service nodes in the xCAT 
infrastructure. This design is to reduce the number of servers communicating with each instance of 
Logstash, distributing the workload. xCAT service nodes have failover capabilities removing the 
need for HAProxies to reduce the risk of data loss. Finally, in using the service node the total 
cost of the Big Data Cluster is reduced as the need for a dedicated node for data processing is 
removed.

CAST provides an event correlator for Logstash to assist in the generation of RAS events for
specific messages.

Installation and Configuration
------------------------------

Installation
^^^^^^^^^^^^

.. note:: This guide has been tested using Logstash 7.5.1, the latest RPM may be downloaded from
   `the Elastic Site <https://www.elastic.co/downloads/logstash>`_.

For the official install guide of Logstash in the ELK stack go to: `Installing Logstash`_

The following is a brief guide to the installation of Logstash with relation to CAST. The user should use the offical ELK documentation above as the main reference of information for installing Logstash.
CAST provides a set of sample configuration files in the repository at `csm_big_data/logstash/`.
If the |csm-bds| rpm has been installed the sample configurations may be found 
in `/opt/ibm/csm/bigdata/logstash/`.

Preparing for installation

.. code-block:: bash

    # groupadd -g 1900 elasticsearch; groupadd -g 1901 kibana; groupadd -g 1902 logstash
    # useradd -g 1900 -u 1900 elasticsearch; useradd -g 1901 -u 1901 kibana; useradd -g 1902 -u 1902 logstash

.. code-block:: bash

    grep 'logstash\|elasticsearch\|kibana' /etc/passwd /etc/group
    /etc/passwd:elasticsearch:x:1900:1900::/home/elasticsearch:/bin/bash
    /etc/passwd:kibana:x:1901:1901::/home/kibana:/bin/bash
    /etc/passwd:logstash:x:1902:1902::/home/logstash:/bin/bash
    /etc/group:elasticsearch:x:1900:
    /etc/group:kibana:x:1901:
    /etc/group:logstash:x:1902:

1. Install the logstash rpm and java-11 (command run from directory with logstash rpm):

.. code-block:: bash

     yum install -y logstash-*.rpm java-11*

After the JAVA-11 installation

.. note:: After installing `Java`, set up the `JAVA` environment variables in root's `.bashrc` file. Assuming `Java` is installed in `/usr/lib/jvm/java-11`.
Additionally, bundler and jruby could be installed outside of logstash, so these could also be include in the `/root/.bashrc` file as well.
There could be incompatibility issues because logstash installs its own bundler and jruby. They have to be added in-front of the PATH so they are found first.
The admin can try to run the two different versions by specifying absolute paths.

.. code-block:: bash

    export JAVA_HOME=/usr/lib/jvm/java-11
    export PATH=/usr/lib/jvm/java-11/bin:$PATH
    export BUNDLE_DIR=/usr/share/logstash/vendor/bundle/jruby/2.5.0/bin
    export JRUBY_DIR=/usr/share/logstash/vendor/jruby/bin
    export PATH=$BUNDLE_DIR:$JRUBY_DIR:$PATH

The above dir. could also be a symbolic link. There could be multiple versions of `Java` installed by other software. Find out where java is:

.. code-block:: bash

    # which java
    /usr/lib/jvm/java-11/bin/java
     
    # ls -ld /usr/lib/jvm/java-11
    lrwxrwxrwx 1 root root 29 Jul 12 15:59 /usr/lib/jvm/java-11 -> /etc/alternatives/java_sdk_11

2. Copy the Logstash pipeline configuration files to the appropriate directories. 

    This step is ultimately optional, however it is recommended that these files be reviewed and 
    modified by the system administrator at this phase:

    +-----------------------------+-----------+-----------+
    | Target file                 | Repo Dir  | RPM Dir   |
    +=============================+===========+===========+
    | logstash.yml(see note)      | config/   | config/   |
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
   `csm_big_data/logstash`. RPM Directories are relative to `/opt/ibm/csm/bigdata/logstash/`.

3. Install the `CSM Event Correlator`_ 
    
.. code:: bash

    rpm -ivh ibm-csm-bds-logstash*.noarch.rpm

.. note:: This change is effective in the 1.3.0 release of the CAST rpms.

Please refer to `CSM Event Correlator`_ for more details.

.. note:: The bin directory is relative to your logstash install location.

Configuration
^^^^^^^^^^^^^

.. note:: The `conf.d/logstash.conf` file requires the ELASTIC-INSTANCE field be replaced with
   your cluster's Elasticsearch nodes.
   
.. note:: `logstash.yml` is not shipped with this version of the RPM please use the following config for logstash.

.. code-block:: bash

   # logstash.yml
   ---
   path.data: /var/lib/logstash
   path.config: /etc/logstash/conf.d/*conf
   path.logs: /var/log/logstash
   pipeline.workers: 2
   pipeline.batch.size: 2000 # This is the MAXIMUM, to prevent exceedingly long waits a delay is supplied.  
   pipeline.batch.delay: 50  # Maximum time to wait to execute an underfilled queue in milliseconds.
   queue.type: persisted
   ...

Tuning logstash is highly dependant on your use case and environment. What follows is a set of
recommendations based on the research and experimentation of the CAST Big Data team.

Here are some useful resources for learning more about profiling and tuning logstash:

* `Do you grok Grok?`_
* `Tuning Logstash`_

logstash.yml
~~~~~~~~~~~~

This configuration file specifies details about the Logstash service:

1. Path locations (as a rule of thumb these files should be owned by the `logstash` user).
2. Pipeline details (e.g. workers, threads, etc.)
3. Logging levels.

For more details please refer to the `Logstash settings file`_ documentation.

.. TODO Add more to this as CAST learns more

jvm.options
~~~~~~~~~~~

The configuration file for the Logstash JVM. The supplied settings are CAST's recommendation,
however, the efficacy of these settings entirely depends on your Logstash node.

logstash.conf
~~~~~~~~~~~~~

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
|    json_data    |       10522        |
+-----------------+--------------------+
|  transactions   |       10523        |
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

The user *must* replace `_ELASTIC_IP_PORT_LIST_` with a comma delimited list of `hostname`:`port`
string pairs refering to the nodes in the elasticsearch cluster. Generally if using the default 
configuration the port should be `9200`. An example of this configuration is as follows:

.. code-block:: bash

   hosts => [ "10.7.4.14:9200", "10.7.4.15:9200", "10.7.4.19:9200" ]


grok
~~~~

Logstash provides a `grok` utility to perform regular expression pattern recognition and extraction.
When writing grok patterns several rules of thumb are recommended by the CAST team:

1. Profile your patterns, `Do you grok Grok?`_ discusses a mechanism for profiling.
2. Grok failure can be expensive, use anchors (^ and $) to make string matches precise to reduce failure costs.
3. _groktimeout tagging can set an upper bound time limit for grok operations.
4. Avoid `DATA` and `GREEDYDATA` if possible.

Starting Logstash
-----------------

Now that every thing has been installed and configured. You can start Logstash. 

.. code-block:: bash

    systemctl enable logstash
    systemctl start logstash

Logstash should now be operational. At this point data aggregators should be configured to point
to your Logstash node as appropriate.

.. note:: In ELK 7.5.1, Logstash may not start and run on Power, due to an arch issue. Please see: :ref:`Logstash_Not_Starting`

CSM Event Correlator
---------------------

CSM Event Correlator (CEC) is the CAST solution for event correlation in the logstash pipeline.
CEC is written in ruby to leverage the existing Logstash plugin system. At its core CEC is a pattern
matching engine using `grok`_ to handle pattern matching. 

A sample configuration of CEC is provided as the `events.yml` file described in the `Configuration`_
section of the document. 

There's an extensive asciidoc for usage of the `CSM Event Correlator plugin`_. The following 
documentation is an abridged version.



.. Links
.. _Logstash: https://www.elastic.co/products/logstash
.. _Do you grok Grok?: https://www.elastic.co/blog/do-you-grok-grok
.. _Installing Logstash: https://www.elastic.co/guide/en/logstash/current/installing-logstash.html#installing-logstash
.. _Tuning Logstash: https://www.elastic.co/guide/en/logstash/current/tuning-logstash.html
.. _configuration file structure: https://www.elastic.co/guide/en/logstash/current/configuration-file-structure.html
.. _elasticsearch plugin: https://www.elastic.co/guide/en/logstash/current/plugins-outputs-elasticsearch.html
.. _CSM Event Correlator plugin: https://github.com/IBM/CAST/blob/master/csm_big_data/Logstash/plugins/csm_event_correlator/doc/index.asciidoc
.. _Logstash settings file: https://www.elastic.co/guide/en/logstash/current/logstash-settings-file.html
.. _filebeats: https://www.elastic.co/guide/en/beats/filebeat/current/filebeat-getting-started.html
