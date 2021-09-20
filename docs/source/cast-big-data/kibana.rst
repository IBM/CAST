.. _cast-kibana:

Kibana
======

`Kibana`_ is an open-sourced data visualization tool used in the EL\ **K** stack.

CAST provides a utility plugin for multistep searches of CSM jobs in Kibana dashboards.

Configuration
-------------

.. note:: This guide has been tested using Kibana 7.5.1, the latest RPM may be downloaded from
    `the Elastic Site <https://www.elastic.co/downloads/kibana>`_.

The following is a brief introduction to the installation and configuration of the Kibana service.

At the current time CAST does not provide a configuration file in its RPM.

.. note:: `Kibana` installation has dependency on `node.js`. Make sure the RH BASEOS media or repository is available.

1. Install the Kibana rpm:

.. code-block:: bash

    yum install -y kibana-*.rpm

.. code-block:: bash

    # yum install kibana-7.5.1-1_ol001.el8.ppc64le.rpm
    Updating Subscription Management repositories.
    Unable to read consumer identity
     
    This system is not registered to Red Hat Subscription Management. You can use subscription-manager to register.
     
    Last metadata expiration check: 0:24:48 ago on Mon 12 Jul 2021 03:49:51 PM EDT.
    Dependencies resolved.
    =============================================================================================================================================================
     Package  Arch      Version                                               Repository                                                                    Size
    =============================================================================================================================================================
    Installing:
     kibana   ppc64le   7.5.1-1_ol001.el8                                     @commandline                                                                 202 M
    Installing dependencies:
     nodejs   ppc64le   1:10.24.0-1.module+el8.3.0+10166+b07ac28e             local-rhels8.4-ppc64le--install-REPO-os-rhels-8.4-rhels8.4-ga-repo-ppc64le   9.2 M
     npm      ppc64le   1:6.14.11-1.10.24.0.1.module+el8.3.0+10166+b07ac28e   local-rhels8.4-ppc64le--install-REPO-os-rhels-8.4-rhels8.4-ga-repo-ppc64le   3.7 M
    Enabling module streams:
     nodejs             10
     
    Transaction Summary
    =============================================================================================================================================================
    Install  3 Packages
     
    Total size: 214 M
    Total download size: 13 M
    Installed size: 661 M
    Is this ok [y/N]: y

.. note:: `Kibana` requires the dir `/var/run/kibana` to store its pid. Create the directory if not there.

.. code-block:: bash
 
    mkdir /var/run/kibana; chown -R kibana:kibana /var/run/kibana
    # ls -l /var/run/ | grep kibana
    drwxr-xr-x  2 kibana        kibana          60 Sep 15 16:18 kibana

After successful `Kibana` installation, check following directory.

.. code-block:: bash

    # ls -ld /usr/share/kibana
    drwxr-xr-x 11 root root 4096 Jul 12 16:15 /usr/share/kibana
    # ls -ld /var/run/kibana
    drwxr-xr-x 2 kibana kibana 60 Sep 15 16:18 /var/run/kibana

Each installation also put configuration files and templates into /etc. For example:

.. code-block:: bash

    # ls -l /etc/kibana
    -rw-r--r-- 1 kibana kibana  344 Jul 12 16:26 kibana.yml
    -rw-r--r-- 1 kibana kibana 5149 Jul 12 16:26 kibana.yml.inst

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
