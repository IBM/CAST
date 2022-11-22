.. _CSM_INSTALLATION_AND_CONFIGURATION_installation:

Installation
============

List of CSM RPMs
----------------

The following RPMs are all of the required RPMs for :ref:`CSM`. These RPMs can be found on Box in the same folder as this document. Please verify that you have all required RPMs. 

For CSM, all CSM rpms have a version of ibm-csm-component-1.8.3-commit_sequence.ppc64le.rpm, for example, ibm-csm-core-1.8.3-3605.ppc64le.rpm.  The commit_sequence portion is an increasing value that changes every time a new commit is added to the repository. Because the commit_sequence changes so frequently, this document makes use of a wildcard for this portion of the rpm name.

**ibm-csm-core** holds CSM infrastructure daemon and configuration file examples.

**ibm-csm-api** holds all CSM APIs and the corresponding command line interfaces.

**ibm-csm-hcdiag** holds the diagnostics and health check framework with all available tests interface.

**ibm-csm-db** holds CSM DB configuration files and scripts.

**ibm-csm-bds** holds BDS configuration files, scripts, and documentation.

**ibm-csm-bds-logstash** holds logstash plugins, configuration files and scripts.

**ibm-csm-bds-kibana** holds kibana plugins, configuration files and scripts.

**ibm-csm-restd** holds the CSM REST daemon, configuration file example, and a sample script.

Below is a chart indicating where each of these RPMs must be installed. 

.. list-table:: Install guide for RPMs
   :header-rows: 1

   * - 
     - Management Node
     - Service Node
     - Login Node
     - Launch Node
     - Compute Node
     - ESS Servers
     - UFM Servers
     - BDS Servers
   * - ibm-csm-core
     - x
     - x 
     - x
     - x
     - x
     - 
     - 
     - 
   * - ibm-csm-api
     - x
     - x 
     - x
     - x
     - x
     - 
     - 
     - 
   * - ibm-csm-hcdiag
     - x
     - x 
     - x
     - x
     - x
     - 
     - 
     - 
   * - ibm-csm-db
     - x
     - 
     - 
     - 
     - 
     - 
     - 
     - 
   * - ibm-csm-bds
     - 
     - 
     - 
     - 
     - 
     - 
     - 
     - x
   * - ibm-csm-bds-logstash
     - 
     - x
     - 
     - 
     - 
     - 
     - 
     - x
   * - ibm-csm-bds-kibana
     - 
     - 
     - 
     - 
     - 
     - 
     - 
     - x
   * - ibm-csm-restd
     - x 
     - x
     - 
     - 
     - 
     - 
     - 
     - 

Installing CSM onto the Management Node
---------------------------------------

On the management node:

Install the cast-boost RPMs, which can be found on Box


.. code-block:: bash

 $ rpm -ivh cast-boost-*.rpm

Install the flightlog RPM and the following CSM RPMs, which can be found on Box.

* ibm-flightlog
* ibm-csm-core
* ibm-csm-api
* ibm-csm-db
* ibm-csm-bds
* ibm-csm-hcdiag
* ibm-csm-restd

.. code-block:: bash

  $ rpm -ivh ibm-flightlog-1.8.3-*.ppc64le.rpm \
  ibm-csm-core-1.8.3-*.ppc64le.rpm \
  ibm-csm-api-1.8.3-*.ppc64le.rpm  \
  ibm-csm-db-1.8.3-*.noarch.rpm \
  ibm-csm-bds-1.8.3-*.noarch.rpm \
  ibm-csm-hcdiag-1.8.3-*.noarch.rpm \
  ibm-csm-restd-1.8.3-*.ppc64le.rpm

Installing CSM onto the Service Nodes
-------------------------------------

For service nodes, CSM supports diskless and full disk install:

Diskless
^^^^^^^^

Clone existing images, add the following packages to the “otherpkgs” directory and list, then run genimage. See Appendix 7.1 for more details.

.. code-block:: bash

  cast-boost-*
  ibm-flightlog-1.8.3-*.ppc64le
  ibm-csm-core-1.8.3-*.ppc64le
  ibm-csm-api-1.8.3-*.ppc64le
  ibm-csm-bds-1.8.3-*.noarch
  ibm-csm-bds-logstash-1.8.3-*.noarch
  ibm-csm-hcdiag-1.8.3-*.noarch
  ibm-csm-restd-1.8.3-*.ppc64le

Full Disk
^^^^^^^^^

Install the cast-boost RPMs, which can be found on Box

.. code-block:: bash

  $ rpm -ivh cast-boost-*.rpm

Install the flightlog RPM and the following CSM RPMs, which can be found on Box.

* ibm-flightlog
* ibm-csm-core
* ibm-csm-api
* ibm-csm-bds
* ibm-csm-bds-logstash
* ibm-csm-hcdiag
* ibm-csm-restd

.. code-block:: bash

  $ rpm -ivh ibm-flightlog-1.8.3-*.ppc64le.rpm \
  ibm-csm-core-1.8.3-*.ppc64le.rpm \
  ibm-csm-api-1.8.3-*.ppc64le.rpm \
  ibm-csm-bds-1.8.3-*.noarch.rpm \
  ibm-csm-bds-logstash-1.8.3-*.noarch.rpm \
  ibm-csm-hcdiag-1.8.3-*.noarch.rpm \
  ibm-csm-restd-1.8.3-*.ppc64le.rpm

Installing CSM onto the Login, Launch, and Workload manager Nodes
-----------------------------------------------------------------

For login, launch and workload manager nodes, CSM supports diskless and full disk install:

Diskless
^^^^^^^^

Clone existing images, add the following packages to the “otherpkgs” directory and list, then run genimage. See Appendix 7.1 for more details.

.. code-block:: bash

  cast-boost-*
  ibm-flightlog-1.8.3-*.ppc64le
  ibm-csm-core-1.8.3-*.ppc64le
  ibm-csm-api-1.8.3-*.ppc64le
  ibm-csm-hcdiag-1.8.3-*.noarch

Full Disk
^^^^^^^^^

Install the cast-boost RPMs, which can be found on Box

.. code-block:: bash

  $ rpm -ivh cast-boost-*.rpm

* ibm-flightlog
* ibm-csm-core
* ibm-csm-api
* ibm-csm-hcdiag

.. code-block:: bash

  $ rpm -ivh ibm-flightlog-1.8.3-*.ppc64le.rpm \
  ibm-csm-core-1.8.3-*.ppc64le.rpm \
  ibm-csm-api-1.8.3-*.ppc64le.rpm \
  ibm-csm-hcdiag-1.8.3-*.noarch.rpm

Installing CSM onto the Compute Nodes
-------------------------------------

For compute nodes, CSM supports diskless and full disk install:

Diskless
^^^^^^^^

Clone existing images, add the following packages to the “otherpkgs” directory and list, then run genimage. See Appendix 7.1 for more details.

.. code-block:: bash

  cast-boost-*
  ibm-flightlog-1.8.3-*.ppc64le
  ibm-csm-core-1.8.3-*.ppc64le
  ibm-csm-api-1.8.3-*.ppc64le
  ibm-csm-hcdiag-1.8.3-*.noarch

Full disk
^^^^^^^^^

Install the cast-boost RPMs, which can be found on Box.

Note: replace "/path/to/rpms" with the appropriate location for your system.

.. code-block:: bash

  $ xdsh compute "cd /path/to/rpms; rpm -ivh cast-boost-*.rpm"

Install the flightlog RPM and the following CSM RPMs, can be found on Box. 

Note: replace ``/path/to/rpms`` with the appropriate location for your system.

* ibm-flightlog
* ibm-csm-core
* ibm-csm-api
* ibm-csm-hcdiag

.. code-block:: bash


  $ xdsh compute "cd /path/to/rpms; \
  rpm -ivh ibm-flightlog-1.8.3-*.ppc64le.rpm \
  ibm-csm-core-1.8.3-*.ppc64le.rpm \
  ibm-csm-api-1.8.3-*.ppc64le.rpm \
  ibm-csm-hcdiag-1.8.3-*.noarch"



























