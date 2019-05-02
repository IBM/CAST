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

