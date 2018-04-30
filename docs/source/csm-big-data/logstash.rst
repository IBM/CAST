Logstash
========

`Logstash`_ is an open-source data processing pipeline used in the E**L**K stack. The core function
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
CSM provides a set of sample configuration files in `csm_big_data/Logstash/` in the repository.
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
    +-----------------------------+-----------+-----------+
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

3. Install the `Simple Event Correlator` 

.. TODO document process

4. Start Logstash:
.. code-block:: bash

    $ systemctl enable logstash
    $ systemctl start logstash

Logstash should now be operational. At this point data aggregators should be configured to point
to your Logstash node as appropriate.
.. TODO write data aggregators file.


Tuning Logstash
---------------

logstash.yml
^^^^^^^^^^^^

jvm.options
^^^^^^^^^^^

logstash.conf
^^^^^^^^^^^^^

grok
^^^^

Simple Event Correlator
^^^^^^^^^^^^^^^^^^^^^^^



.. Links
.. _Logstash: https://www.elastic.co/products/logstash
