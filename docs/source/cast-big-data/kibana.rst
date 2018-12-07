.. _cast-kibana:

Kibana
======

`Kibana`_ is an open-sourced data visualization tool used in the EL\ **K** stack.

CAST provides a utility plugin for multistep searches of CSM jobs in Kibana dashboards.

Configuration
-------------

.. note:: This guide has been tested using Kibana 6.3.2, the latest RPM may be downloaded from
    `the Elastic Site <https://www.elastic.co/downloads/kibana>`_.

The following is a brief introduction to the installation and configuration of the Kibana service.

At the current time CAST does not provide a configuration file in its RPM.

1. Install the Kibana rpm:

.. code-block:: bash

   yum install -y kibana-*.rpm

2. Configure the Kibana YAML file (`/etc/kibana/kibana.yml`)

CAST recommends the following four values be set before starting Kibana:

+------------------------+-------------------------------------------------------------+-------------------------+
| Setting                | Description                                                 | Sample Value            |
+========================+=============================================================+=========================+
| server.host            | The address the kibana server will bind on, needed for      | "10.7.4.30"             |
|                        | external access.                                            |                         |
+------------------------+-------------------------------------------------------------+-------------------------+
| elasticsearch.url      | The URL of an elasticsearch service, this should include    | "http://10.7.4.13:9200" |
|                        | the port number (9200 by default).                          |                         |
+------------------------+-------------------------------------------------------------+-------------------------+
| xpack.security.enabled | The xpack security setting, set to false if not being used. | false                   |
+------------------------+-------------------------------------------------------------+-------------------------+
| xpack.ml.enabled       | Sets the status of xpack Machine Learning. Please note      | false                   |
|                        | this must be set to false on ppc64le installations.         |                         |
+------------------------+-------------------------------------------------------------+-------------------------+

3. Install the `CAST Search`_ rpm:

.. code:: bash

    rpm -ivh ibm-csm-bds-kibana-*.noarch.rpm

4. Start Kibana:

.. code-block:: bash
    
    systemctl enable kibana.service
    systemctl start kibana.service


Kibana should now be running and fully featured. Searchs may now be performed on the Discover tab.

CAST Search
-----------

CAST Search is a React plugin designed for interfacing with elastic search an building filters for 
Kibana Dashboards. To maxmize the value of the plugin the `cast-allocation` index pattern should be 
specified.

.. TODO describe funciton and feature in greater depth.


.. Links
.. _Kibana: https://www.elastic.co/products/Kibana
