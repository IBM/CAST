.. _CSM_USER_GUIDE_Infrastructure:

CSM Infrastructure
==================

.. note::  This page is under re-work and moving to: :ref:`CSMDInfrastructure`.

Overview 
--------

The :ref:`CSM` infrastructure consists of master, aggregator, utility, and compute daemons.

The CSM master daemon runs on the management node. Aggregators run on the service nodes (optional: run on management node too). The CSM utility daemon runs on the login and launch node. The CSM compute daemon runs on the compute node. This is illustrated below:


.. image:: https://user-images.githubusercontent.com/4662139/57104405-4365a400-6cf6-11e9-9acd-aaba571d06f9.png

As shown above, all daemons communicate directly point to point. The compute daemon communicates directly to one aggregator daemon (the primary) and can be configured to connect to a secondary aggregator for fault tolerance however, almost all communication will go through the primary. The aggregator communicates directly to the master daemon. The utility daemon communicates directly to the master daemon. Only the master daemon is allowed to communicate to the CSM database.

Configuration
--------------

Each type of daemon has its own configuration file. Default configuration files can be found here: ``/opt/ibm/csm/share/etc``

CSM Daemons and Corresponding Configuration Files

================== ============================================================================================
Master Daemon      | `csm_master.cfg <https://github.com/IBM/CAST/blob/master/csmconf/csm_master.cfg>`_
Aggregator Daemon  | `csm_aggregator.cfg <https://github.com/IBM/CAST/blob/master/csmconf/csm_aggregator.cfg>`_
Utility Daemon     | `csm_utility.cfg <https://github.com/IBM/CAST/blob/master/csmconf/csm_utility.cfg>`_
Compute Daemon     | `csm_compute.cfg <https://github.com/IBM/CAST/blob/master/csmconf/csm_compute.cfg>`_
================== ============================================================================================

If edits are made to a configuration file, then a daemon must be killed and started again.

A detailed explanation of configuration settings can also be found at: :ref:`CSMDInfrastructure`


Daemon Functionality
--------------------

Master daemon
~~~~~~~~~~~~~

CSM master daemon runs on the management node and supports the following activities:

* CSM DB access
* CSM API support
* CSM RAS master functions

Aggregator daemon
~~~~~~~~~~~~~~~~~

The CSM aggregator daemon runs on the management node and facilitates communication between the master daemon and the compute daemons. The aggregator daemon supports the following activities:

* Forwarding all the messages from the compute daemons on compute nodes to the master daemon on the management node without any aggregation functionally. 
* Supporting the fan-out of internal CSM multicast messages from the master daemon to a list of compute nodes.
* Keep track of active and inactive compute nodes during its livetime. (This data is not persisted. Therefore, if an aggregator is restarted, this info is only partially recaptured based on the current active set of compute nodes.)
* Allow CSM clients to call APIs similar to the utility daemon functionality.
* Allows to connect to Logstash and send environmental data.

A new connection from compute nodes is considered a secondary connection until the compute node tells the aggregator otherwise. This assures that any messages along the secondary path between compute nodes and master get filtered.

Utility daemon
~~~~~~~~~~~~~~

The CSM utility daemon runs on the login and launch nodes and supports the following activities:

* Node discovery
* Node inventory
* Steps
* CSM API support
* Environmental data collection

Compute daemon
~~~~~~~~~~~~~~

The CSM compute daemon run on the compute node and support the following activities: 

* Node discovery
* Node inventory
* Allocation create and delete
* Environmental data bucket execution
* Preliminary environmental data collection (only GPU data)
* Aggregator failover

The CSM compute, aggregator and utility daemons collect the node inventory and send it to the CSM master daemon where it is stored in the CSM DB. This collection process takes place every time the CSM daemon starts or reconnects after a complete disconnect.

The daemons support authentication and encryption via SSL/TLS. To enable, the administrator has to configure the daemons to use SSL. The CA and certificate files have to be the same for use with point-to-point connections between all daemons. 

All daemons are enabled to schedule predefined environmental data collection items. Each of the items can be part of buckets where each bucket can be configured with an execution interval. Currently, CSM only has only one predefined bucket for GPU data collection on the compute and utility daemons.

Compute nodes are able to connect to two aggregators if configured. The compute daemon will try to connect to the first configured aggregator. If this succeeds it will also establish the connection to the secondary aggregator and will then be fully connected. If a secondary aggregator fails, the compute will keep the regular operation with the primary and try to reconnect to the secondary in a preset interval (~10s). If and only if a primary aggregator fails, the compute daemon will perform an automatic failover by telling its current secondary aggregator to take over the role as the primary for this compute node. If the initial primary aggregator restarts, the compute node will connect to it as a secondary. It will only switch to the other aggregator if the currently active primary connection fails. This behavior has an important system level consequence: once a primary aggregator fails and gets restarted there’s currently no other way to restore the configured aggregator-compute relation without restarting the compute daemon.

API Timeout Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~

This section has been moved to: :ref:`CSMDAPIConf`


Daemon-to-Daemon Heartbeat
~~~~~~~~~~~~~~~~~~~~~~~~~~

This section has been moved to: :ref:`CSMDNetBlock`



