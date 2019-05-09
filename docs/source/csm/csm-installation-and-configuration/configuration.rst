.. _CSM_INSTALLATION_AND_CONFIGURATION_configuration:

Configuration
=============

Now that everything needed for :ref:`CSM` has been installed, CSM needs to be configured.

:ref:`CSMDConfig`

General Configuration
---------------------

Suggested general configuration to verify:

Open files limit:

.. code-block:: bash

  $ ulimit –n
  500000

CSM DB Configuration
--------------------

On the management node create the csmdb schema by running the csm_db_script.sh. This script assumes that xCAT has migrated to postgresql. Details on this migration process can be found in the `xCAT read the docs <https://xcat-docs.readthedocs.io/en/stable/advanced/hierarchy/databases/postgres_configure.html>`_ .

.. code-block:: bash

  
  # /opt/ibm/csm/db/csm_db_script.sh
  --------------------------------------------------------------------------------------------
  [Start   ] Welcome to CSM database automation script.
  [Info    ] Log Dir: /var/log/ibm/csm/db/csm_db_script.log
  [Info    ] PostgreSQL is installed
  [Info    ] csmdb database user: csmdb already exists
  [Complete] csmdb database created.
  [Complete] csmdb database tables created.
  [Complete] csmdb database functions and triggers created.
  [Complete] csmdb table data loaded successfully into csm_db_schema_version
  [Complete] csmdb table data loaded successfully into csm_ras_type
  [Info    ] csmdb DB schema version (17.0)
  --------------------------------------------------------------------------------------------
  # 

This will create csmdb database and configure it with default settings. 

Default Configuration Files
---------------------------

A detailed description of CSM daemon configuration options can be found at: :ref:`CSMDInfrastructure`

On the management node copy default configuration and ACL (Access Control List) files from ``/opt/ibm/csm/share/etc`` to ``/etc/ibm/csm``.

.. code-block:: bash

  $ cp /opt/ibm/csm/share/etc/*.cfg /etc/ibm/csm/
  $ cp /opt/ibm/csm/share/etc/csm_api.acl /etc/ibm/csm/

Review the configuration and ACL files. Make the following suggested updates (note that hostnames can also be IP addresses, especially if a particular network interface is desired for CSM communication):

  #. Substitute all ``__MASTER__`` occurrences in the configuration files with the management node hostname.
  #. On aggregator configurations, substitute ``__AGGREGATOR__`` with the corresponding service node hostname.
  #. On compute configurations, substitute ``__AGGREGATOR_A__`` with the assigned primary aggregator.
  #. On compute configurations, substitute ``__AGGREGATOR_B__`` with the secondary aggregator or leave it untouched if you set up a system without failover.
  #. If an aggregator is run on the management node too, make sure to provide a unique entry for csm.net.local_client_listen.socket in order to avoid name collision and   strange behavior.
  #. Create a new Linux group for privileged access.

     a. Add users to this group.
     b. Make this group privileged in the ACL file. (For more information see Section “6.3.1 Configuring user control, security, and access level” of the “CSM User Guide”)

Review all configuration and ACL files.

Copy the configuration files to the proper nodes:

On management node:

.. code-block:: bash

  $ xdcp compute /etc/ibm/csm/csm_compute.cfg /etc/ibm/csm/csm_compute.cfg
  $ xdcp login,launch /etc/ibm/csm/csm_utility.cfg /etc/ibm/csm/csm_utility.cfg
  $ xdcp compute,login,launch /etc/ibm/csm/csm_api.acl /etc/ibm/csm/csm_api.acl
  $ xdcp compute,login,launch /etc/ibm/csm/csm_api.cfg /etc/ibm/csm/csm_api.cfg






















