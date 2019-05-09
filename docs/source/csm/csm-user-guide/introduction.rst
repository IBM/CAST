.. _CSM_USER_GUIDE_Introduction:

Introduction
============

The purpose of this document is to familiarize the system administrator with :ref:`CSM`, and act as a reference for when CSM Developers can not be reached.

CSM is developed for use on POWER systems to provide a test bed for the CORAL specific software on non-production systems. This will allow the national laboratories to provide early feedback on the software. This feedback will be used to guide further software development. These POWER systems consist of:

* Witherspoon as compute and utility (Login, Launch and Workload manager) nodes
* Boston as management node and Big Data Store

This document assumes POWER systems are fully operational and running Red Hat Pegas 1.0 or higher.

Functions and Features
----------------------

CSM includes support for the following functions and features:

* CSM API support for Burst Buffer, diagnostics and health check, job launch, node inventory, and RAS.
* CSM DB support for CSM APIs, diagnostics and health check, job launch, node inventory, and RAS.
* CSM Infrastructure support for CSM APIs, job launch, node inventory, and RAS.
* RAS event generation, actions, and query.
* Diagnostics and Health Check framework support for CSM prolog/epilog, HTX exercisers.

Restrictions, Limitations, and Known Issues
-------------------------------------------

CSM does not include support for the following items:

* Shared allocation

Known Issues:

* Whenever CSM is upgraded to a new version, all software that is dependent on CSM must be restarted. LSF and Burst Buffer have daemon processes that must be restarted. JSM is also dependent on CSM, but does not include any daemon processes that need to be explicitly restarted.
* CSM does not support rpm upgrade (rpm –U), uninstall and re-install is required.
* CSM API responses failing with the error: "recvmsg timed out. rc=-1" when the response message to the API grows beyond the maximum supported message size. To work around this issue reduce the number of returned records adding addition filters and/or using the limit option on the API.
* There are several daemons that all start during system boot when enabled via systemd (xcatd, nvidia-persistenced, dcgm, csmd-master, csmd-aggregator, csmd-utility, csmd-compute, csmrestd, ibmpowerhwmon). Currently the startup of these daemons does not occur in controlled fashion. As a temporary workaround, it may be required to start these services using a script that inserts appropriate wait times between dependent daemons.
* Chance of CSM daemon ID of collision

  - There’s a ``1:3.4*10^36`` chance that generated daemonIDs collide. The csm_infrastructure_health_check performs a uniqueness-test to confirm that this didn't happen.

Reporting issues with CSM
-------------------------

To obtain support and report issues with CSM, please contact IBM Support Service.

CSM Installation and configuration
----------------------------------

When you are ready to install CSM, please see the :ref:`CSM_INSTALLATION_AND_CONFIGURATION`, which can be found on Box site.
At this point in time we suggest installing CSM. The following chapters of this user guide provide additional knowledge and reference for sub systems of CSM.
