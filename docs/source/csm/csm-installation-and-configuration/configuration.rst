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