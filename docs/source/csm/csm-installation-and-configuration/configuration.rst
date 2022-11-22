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
  [Info    ] csmdb DB schema version (19.0)
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

SSL Configuration
~~~~~~~~~~~~~~~~~

If an SSL setup is desired, the csm.net.ssl section of the config file(s) needs to be set up. 

.. code-block:: json

  {
    "ssl":
    {
        "ca_file" : "<full path to CA file>",
        "cred_pem" : "<full path to credentials in pem format>"
    }
  }

If the strings are non-empty, the daemon assumes that SSL is requested. This means if the SSL setup fails, it will not fall back to non-SSL communication.

Heartbeat interval
~~~~~~~~~~~~~~~~~~

CSM daemons heartbeat interval can be configure on csm.net section of the config file(s). 

.. code-block:: json

  {
    "net" :
    {
        "heartbeat_interval" : 15,
        "local_client_listen" :
        {
            "socket"      : "/run/csmd.sock",
            "permissions" : 777,
            "group"       : ""
        }
     }
  }

The heartbeat interval setting defines the time between 2 subsequent unidirectional heartbeat messages in case there's no other network traffic on a connection. The default is 15 Seconds. If two daemons are configured with a different interval, they will use the minimum of the two settings for the heartbeat on the connection. This allows to configure a short interval between Aggregator and Master and a longer interval between Aggregator and Compute to reduce network interference on compute nodes.

It might take up to 3 intervals to detect a dead connection because of the following heartbeat process: After receiving a message the daemon waits one interval to send its heartbeat one way. If it doesn't get any heartbeat after one more interval, it will retry and wait for another interval before declaring the connection broken. This setting needs to balance the requirements between fast detection of dead connections and network traffic overhead. Note that if a daemon fails or is shut down, the closed socket will be detected immediately in many cases. The heartbeat-based detection is mostly only needed for errors in the network hardware itself (e.g. broken or disconnected cable, switch, port).

Environmental Buckets
~~~~~~~~~~~~~~~~~~~~~

The daemons are enabled to execute predefined environmental data collection. The execution is controlled in the configuration files in section *csm.data_collection* with can list a number of buckets each with a list of the predefined items. Currently CSM supports three types of data collection items: "gpu", "environmental" and “ssd”.

Prolog/Epilog Scripts Compute
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In order to create allocations both a **privileged_prolog** and **privileged_epilog** script must be present in ``/opt/ibm/csm/prologs/`` on the compute nodes. 

Review the sample scripts and make any customization required.

To use the packaged sample scripts ``/opt/ibm/csm/share/prologs/`` run the following on the management node:

.. code-block:: bash

  $ xdcp compute /opt/ibm/csm/share/prologs/* /opt/ibm/csm/prologs/

This will copy the following files to the compute nodes:

* privileged_prolog ( access: 700 )
* privileged_prolog ( access: 700 )
* privileged.ini 	   ( access: 600 )

The *privileged_prolog* and *privileged_epilog* files are python scripts that have command line arguments for type, user flags and system flags. The type is either *allocation* or *step*, and the flags are space delimited alpha-numeric strings of flags. If a new version of one of these scripts are written it must implement the options as below:


.. code-block:: bash

  --type [allocation|step]

  --user_flags "[string of flags, spaces allowed]"

  --sys_flags "[string of flags, spaces allowed]"

The *privileged.ini* file configures the logging levels for the script. It is only needed if extending or using the packaged scripts. For more details see the comments in the bundled scripts, the packaged **POST_README.md** file or the Configuring allocation prolog and epilog scripts section in the *CSM User Guide*.

CSM PAM Module
~~~~~~~~~~~~~~

The ibm-csm-core rpm does install a PAM module that may be enabled by a system administrator. This module performs two operations: prevent unauthorized users from obtaining access to a compute node and placing users who have active allocations into the correct cgroup.

To enable the CSM PAM Module for ssh sessions:

1.	Add always authorized users to /etc/pam.d/csm/whitelist (newline delimited).
2.	Uncomment the following line from /etc/pam.d/sshd:

.. code-block:: bash

  account    required     libcsmpam.so
  session    required     libcsmpam.so

3.	Restart the ssh daemon:

.. code-block:: bash

  $ systemctl restart  sshd.service


Non root users who do not have an active allocation on the node and are not whitelisted will now be prevented from logging into the node via ssh. Users who have an active allocation will be placed into the appropriate cgroup when logging in via ssh.

For more details on the behavior and configuration of this module please refer to ``/etc/pam.d/csm/README.md``, :ref:`CSMPAM`, or the Configuring the CSM PAM module section in the CSM User Guide.

**WARNING:** The CSM PAM module should only be enabled on nodes that will run the CSM compute daemon, as ssh logins will be restricted to root and users specified in whitelist.

Start CSM Daemons
-----------------

Before we start CSM daemon we need to start CSM daemons dependency:

**Login, Launch and Compute node**

Start NVIDIA persistence and DCGM

.. code-block:: bash

  $ xdsh compute,service,utility "systemctl start nvidia-persistenced"
  $ xdsh compute,service,utility "systemctl start dcgm"

**Management node**

Start the master daemon

.. code-block:: bash

  $ systemctl start csmd-master

Start the aggregator daemon

.. code-block:: bash

  $ systemctl start csmd-aggregator

**Login and Launch node**

Start the utility daemon

.. code-block:: bash

  $ xdsh login,launch "systemctl start csmd-utility"


**Compute node**

Start the compute daemon

.. code-block:: bash

  $ xdsh compute "systemctl start csmd-compute"

Run the Infrastructure Health Check
-----------------------------------

Run CSM infrastructure health check on the login / launch node to verify the infrastructure status:

.. code-block:: bash
  
 # /opt/ibm/csm/bin/csm_infrastructure_health_check -v
 Starting. Contacting local daemon...
 Connected. Checking infrastructure... (this may take a moment. Please be patient...)

 ###### RESPONSE FROM THE LOCAL DAEMON #######
 MASTER: c650f99p06 (bounced=0; version=1.8.3)
        DB_free_conn_size: 10
        DB_locked_conn_pool_size: 0
        Timer_test: success
        DB_sql_query_test: success
        Multicast_test: success
        Network_vchannel_test: success
        User_permission_test: success
        UniqueID_test: success

 Aggregators:2
    AGGREGATOR: c650f99p06 (bounced=1; version=1.8.3)
        Active_primary: 2
        Unresponsive_primary: 0
        Active_secondary: 2
        Unresponsive_secondary: 0

        Primary Nodes:
                Active: 2
                        COMPUTE: c650f99p18 (bounced=1; version=1.8.3; link=PRIMARY)
                        COMPUTE: c650f99p26 (bounced=1; version=1.8.3; link=SECONDARY)
                Unresponsive: 0

        Secondary Nodes:
                Active: 2
                        COMPUTE: c650f99p36 (bounced=1; version=1.8.3; link=SECONDARY)
                        COMPUTE: c650f99p28 (bounced=1; version=1.8.3; link=SECONDARY)
                Unresponsive: 0

    AGGREGATOR: c650f99p30 (bounced=1; version=1.8.3)
        Active_primary: 2
        Unresponsive_primary: 0
        Active_secondary: 2
        Unresponsive_secondary: 0

        Primary Nodes:
                Active: 2
                        COMPUTE: c650f99p36 (bounced=1; version=1.8.3; link=PRIMARY)
                        COMPUTE: c650f99p28 (bounced=1; version=1.8.3; link=PRIMARY)
                Unresponsive: 0

        Secondary Nodes:
                Active: 2
                        COMPUTE: c650f99p18 (bounced=1; version=1.8.3; link=SECONDARY)
                        COMPUTE: c650f99p26 (bounced=1; version=1.8.3; link=PRIMARY)
                Unresponsive: 0


  Unresponsive Aggregators: 0

 Utility Nodes:1
    UTILITY: c650f99p16 (bounced=1; version=1.8.3)

  Unresponsive Utility Nodes: 0

 Local_daemon: MASTER: c650f99p06 (bounced=0; version=1.8.3)
        Status:
 #############################################

 Finished. Cleaning up...
 Test complete: rc=0
 #

Note that is some cases the list and status of nodes might not be 100% accurate if there were infrastructure changes immediately before or during the test. This usually results in timeout warnings and a rerun of the test should return an updated status.

Another important thing to note happens if there are any unresponsive compute nodes. First, unresponsive nodes will not show a daemon build version and will also not list the connection type as primary or secondary. Additionally, the unresponsive nodes are unable to provide info about their configured primary or secondary aggregator. Instead the aggregators report the last known connection status of those compute nodes. For example, if the compute node did use a connection as the primary link even if the compute configuration defines the connection as secondary, the aggregator will show this compute as an unresponsive primary node. 

Environment Setup for Job Launch
--------------------------------

Use CSM API command line ``csm_node_attributes_update`` to update the compute nodes state to ``IN_SERVICE``. 

.. code-block:: bash

  $ /opt/ibm/csm/bin/csm_node_attributes_update –s IN_SERVICE -n c650f99p28

CSM REST Daemon Installation and Configuration
----------------------------------------------

The CSM REST daemon should be installed and configured on the management node and service nodes. CSM REST daemon enables CSM RAS events to be created by IBM crassd for events detected from compute node BMCs. It also enables CSM RAS events to be created via the CSM Event Correlator from console logs. For example, GPU XID errors are monitored via the CSM Event Correlator mechanism. 

**On the service nodes:**

Install the ibm-csm-restd rpm if it is not already installed:

.. code-block:: bash

  $ rpm -ivh ibm-csm-restd-1.8.3-*.ppc64le.rpm


Copy the default configuration file from /opt/ibm/csm/share/etc to /etc/ibm/csm:

.. code-block:: bash

  $ cp /opt/ibm/csm/share/etc/csmrestd.cfg /etc/ibm/csm/


Edit /etc/ibm/csm/csmrestd.cfg and replace ``__CSMRESTD_IP__`` with ``127.0.0.1``.
The CSM REST daemon requires that the local CSM daemon is running before it is started. 

Start csmrestd using systemctl:

.. code-block:: bash

  $ systemctl start csmrestd


**On the management node (optional):**

If the CSM DB on your management node was not re-created as part of the CSM installation and you intend to enable IBM POWER HW Monitor collection of BMC RAS events from your service nodes, you can manually update the CSM RAS types in the CSM DB using the following process:
With all daemons stopped:

.. code-block:: bash

  $ /opt/ibm/csm/db/csm_db_ras_type_script.sh -l csmdb csm_ras_type_data.csv


This will import any CSM RAS types into the CSM DB that were added on later releases, and it is a no-op for any events that already exist in the CSM DB.























