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
.. TODO: Add a blurb here.

elasticsearch.yml
^^^^^^^^^^^^^^^^^

The Elasticsearch configuration sample shipped by CAST marks fields that need to be set by a 
system administrator. A brief rundown of the fields to modify is as follows:






jvm.options
^^^^^^^^^^^

The configuration file for the Logstash JVM. The supplied settings are CAST's recommendation,
however, the efficacy of these settings entirely depends on your elasticsearch node.







.. Links
.. _Elasticsearch: https://www.elastic.co/products/elasticsearch
