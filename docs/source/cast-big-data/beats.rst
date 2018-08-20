Beats
=====

:Official Documentation: `Beats Reference`_

Beats are a collection of open source data shippers. CAST employs a subset of these beats to facilitate
data aggregation.


Filebeats
---------

:Official Documentation: `Filebeats Reference`_

Filebeats is used to ship the CSM transactional log to the big data store. It was selected for 
its high reliabilty in data transmission and existing integration in the elastic stack.


Installation
************

The following installation guide will deal with configuring filebeats for the CSM transaction log
for a more generalized installation guide please consult the official `Filebeats Reference`_.

1. Install the filebeats rpm on the node:

.. code-block:: bash
    
   rpm -ivh filebeat-*.rpm 

2. Configure the `/etc/filebeat/filebeat.yml` file:
    
CAST ships a sample configuration file in the `ibm-csm-bds-*.noarch` rpm at
`/opt/ibm/csm/bigdata/beats/config/filebeat.yml`. This file is preconfigured to point
at the CSM database archive files and the csm transaction logs. Users will need to replace two
keywords before using this configuration:

+-------------------------+---------------------------------------------------------+---------------------+
| Keyword                 | Description                                             | Sample Value        |
+=========================+=========================================================+=====================+
| _KIBANA_HOST_PORT_      | A string containing the "hostname:port" pairing of the  | "10.7.4.30:5601"    |
|                         | Kibana server.                                          |                     |
+-------------------------+---------------------------------------------------------+---------------------+
| _LOGSTASH_IP_PORT_LIST_ | A list of "hostname:port" pairs pointing to Logstash    | ["10.7.4.41:10523"] |
|                         | servers to ingest the data (current CAST recommendation |                     |
|                         | is a single instance of Logstash).                      |                     |
+-------------------------+---------------------------------------------------------+---------------------+
        
3. Start the filebeats service.

.. code-block:: bash

    service start filebeat.service 

Filebeats should now be sending injested data  to the Logstash instances specified in the 
configuation file.




.. Links

.. _Beats Reference: https://www.elastic.co/guide/en/beats/libbeat/current/beats-reference.html
.. _Filebeats Reference: https://www.elastic.co/guide/en/beats/filebeat/6.3/filebeat-getting-started.html
