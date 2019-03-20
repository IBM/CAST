.. _CSMDConfig:

CSMD Configuration
==================

Each type of daemon is set up via a dedicated configuration file
(default location: `/etc/ibm/csm/csm_*.cfg`).  The format of the config file is json, json parse
errors indicate formatting problems in the config file.


.. warning::
  The CSM daemon needs to be restarted for any changes to the configuration to take effect.

**The `csm` Block**

.. code-block:: json

  {
    "csm" :
    {
        "role": "<daemon_role>",
        "thread_pool_size" : 1,
        "api_permission_file": "/etc/ibm/csm/csm_api.acl",
        "api_configuration_file": "/etc/ibm/csm/csm_api.cfg",
        "log" : { },
        "db" :  { },
        "inventory" : { },
        "net" : { },
        "ras" : { },
        "ufm" : { },
        "bds" : { },
        "recurring_tasks": { },
        "data_collection" : { },
        "jitter_mitigation" : { }
    }
  }


Beginning with the top-level configuration section `csm`.

:role:
    Sets the role of the daemon (``master``, ``utility``, ``compute``, ``aggregator``). 
    If the role is provided in both command line and config file, the command line setting 
    overrides this setting.

:thread_pool_size: 
    Controls the number of worker threads that are used to process any CSM API calls and event handling.
    A setting of 1 should generally suffice. However, if there are some CSM API calls that spawn 
    external processes which in turn might call other CSM APIs (e.g. 
    `csm_allocation_create() spawning the prolog`). 
    
    The worker thread waits for the completion of the spawned process and with only one available 
    worker, there will be no resources left to process the additional API call. This is why a 
    setting of at least 4 is recommended for compute nodes.

:api_permission_file: 
    Points to the file that controls the permissions of API calls to specify admin users/groups and 
    classify CSM APIs as public, private, or privileged. 
    
    See :ref:`CSMDACLConf` for details.

:api_configuration_file: 
    Points to the file that contains detailled configuration settings for CSM API calls. 
    If an API requires a non-default timeout, it should be configured in that file. 
    
    See :ref:`CSMDAPIConf` for details.

:log: 
    A subsection that defines the logging level of various components of the daemon. 
    
    See `The log Block`_.

:db: 
    A master-specific block to set up the data base connection. 
    
    See `The Database[db] Block`_.

:inventory: 
    Configures the inventory collection component. 
    
    See `The inventory block`_.

:net: 
    Configures the network collection component to define the interconnectivity of the CSM infrastructure. 
    
    See `The network[net] block`_.

:ras: 
    Documentation ongoing.

:ufm: 
    Configures access to UFM. 

    See `The UFM block`_.

:bds: 
    Addresses, ports, and other settings for BDS access. 
    
    See `The BDS block`.

:recurring_tasks: 
    Sets up intervals and types of predefined recurring tasks to be triggered by the daemon. 
    
    See `The Recurring_Tasks Block`_.

:data_collection: 
    Enables and configures predefined buckets for environmental data collection. 
    
    See `The data_collection block`_.

:jitter_mitigation: 
    Configures the Mitigation strategy for core isolation and CGroups.

    See `The jitter_mitigation block`_.

.. _CSMDLogBlock:

The ``log`` block
_________________

The log block determines what amount of logging goes to which files and console.
This block also specifies log rotation options.

.. code-block:: json

        {
            "format"                    :   "%TimeStamp% %SubComponent%::%Severity% | %Message%",
            "consoleLog"                :   false,
            "sysLog"                    :   true,
            "fileLog"                   :   "/var/log/ibm/csm/csm_master.log",
            "#rotationSize_comment_1"   :   "Maximum size (in bytes) of the log file, ~1GB",
            "rotationSize"              :   1000000000,
            "default_sev"               :   "warning",
            "csmdb"                     :   "info",
            "csmnet"                    :   "info",
            "csmd"                      :   "info",
            "csmras"                    :   "info",
            "csmapi"                    :   "info",
            "csmenv"                    :   "info",
            "transaction"               :   true,
            "transaction_file"          :   "/var/log/ibm/csm/csm_transaction.log",
            "transaction_rotation_size" :   1000000000
            "allocation_metrics"                :   true,
            "allocation_metrics_file"           :   "/var/log/ibm/csm/csm_allocation_metrics.log",
            "allocation_metrics_rotation_size"  :   1000000000

        }

:format: 
    Defines a template for the format of the CSM log lines. In the given example, a log 
    `Message` is prefixed with the `TimeStamp` followed the name of the `SubComponent` and the 
    `Severity`. The `SubComponent` helps to identify the source of the message (e.g. the csmnet = 
    Network component; csmapi = CSM API call processing).

:consoleLog: 
    Determines whether the logs should go to the console or not. Can be ``true`` or ``false``.

:fileLog: 
    Determine whether the logs should go to syslog or not. Can be ``true`` or ``false``.

:rotationSize: 
    Limits the size (bytes) of the log file before starting a new log file.
    If set to -1 the file is allowed to grow without limit.
    
:default_sev: 
    Set the logging level/verbosity for any component that's not mentioned explicitly. 
    
    Options include:

    +--------------+-----------------------------------------------------------+
    | ``critical`` |  Log only very critical and fatal errors.                 |           
    +--------------+-----------------------------------------------------------+
    | ``error``    |  Errors and critical messages.                            |
    +--------------+-----------------------------------------------------------+
    | ``warning``  |  Warnings and everything above.                           |
    +--------------+-----------------------------------------------------------+
    | ``info``     |  Info messages and everything above.                      |
    +--------------+-----------------------------------------------------------+
    | ``debug``    |  Debug level messages and everything above; very verbose. |
    +--------------+-----------------------------------------------------------+
    | ``trace``    |  Very detailed logging including everything.              |
    |              |  Intended for tracing analysis.                           |
    +--------------+-----------------------------------------------------------+

:csmdb: 
    Log level of the database component. Includes messages about database access and request handling.
:csmnet: 
    Log level of the network component. Includes messages about the network interaction between 
    daemons and daemons and client processes.
:csmd: 
    Log level of the core daemon. Includes messages from the core of the infrastructure 
    handling and management.
:csmras: 
    Log level of the RAS component. Includes messages about RAS events and their processing 
    within the daemon.
:csmapi: 
    Log level of CSM API handling. Includes messages about API call processing.
:csmenv: 
    Log level of environmental data handling. Includes messages related primarily to data 
    collection and shipping from compute to aggregators.
:transaction: 
    Enables the mechanism transaction log mechanism.
:transaction_file: 
    Specifies the location the transaction log will be saved to.
:transaction_rotation_size: 
    The size of the file (in bytes) to rotate the log at.
:allocation: 
    Enables the mechanism allocation metrics log mechanism.
:allocation_file: 
    Specifies the location the allocation metrics log will be saved to.
:allocation_rotation_size: 
    The size of the file (in bytes) to rotate the log at.


The Database[``db``] Block
__________________________

The database block configures the location and access parameters of the CSM database.
The settings are specific and relevant to the master daemon only.

.. code-block:: json

        {
            "connection_pool_size" : 10,
            "host"                 : "127.0.0.1",
            "name"                 : "csmdb",
            "user"                 : "csmdb",
            "password"             : "",
            "schema_name"          : ""
        }

:connection_pool_size: 
    Configures the number of connections to the database. This number also specifies the number of 
    database worker threads for concurrent access and parallel processing of requests. 
    
    CSM recommends empirical adjustments to this size depending on system demand and spec.
    Demand will grow with size of the system and frequency of CSM API calls.

:host: 
    The hostname or IP address of the database server.

:name: 
    The name of the database on the ``host`` server.

:user: 
    The username that CSM should use to access the database.

:password: 
    The password to access the database. 
    
    .. attention:: Be sure to set permissions of the file when the ``password`` field is set!

:schema_name: in case there is a named schema in use, this configures the name
    The named schema in the database (optional in the default configuration).


The ``inventory`` Block
_______________________

The inventory block configures the location of files that are used for collection of the 
network inventory.

.. code-block:: json

        {
            "csm_inv_log_dir" : "/var/log/ibm/csm/inv",
            "ufm":
            {
                "ib_cable_errors" : "bad_ib_cable_records.txt",
                "switch_errors"   : "bad_switch_records.txt"
            }
        }


:csm_inv_log_dir:
    The absolute path for inventory collection logs.

:ufm:
    :ib_cable_errors: 
        Output file location for records of bad IB cables as detected by CSM.
        
        Relative to the ``csm_inv_log_dir``.

    :switch_errors: 
        Output file location for records of IB switch errors as detected by CSM.

        Relative to the ``csm_inv_log_dir``.
        

The Network[``net``] Block
__________________________

The network block defines the hostnames, ports, and other important parameters of the
CSM daemon infrastructure. Several subsections are specific to the role of the daemon.

.. code-block:: json

        {
            "heartbeat_interval" : 15,
            "local_client_listen" :
            {
                "socket"      : "/run/csmd.sock",
                "permissions" : 777,
                "group"       : ""
            },
            "ssl":
            {
                "ca_file"  : "",
                "cred_pem" : ""
            }
        }

General settings available for all daemon roles:

:heartbeat_interval: 
    Determines the interval (in seconds) that this daemon will use for
    any connections to other CSM daemon(s) of the infrastructure. The actual interval of a
    connection will be the minimum interval of the 2 peers of that connection. 
    
    For example, if one daemon initiates a connection with an interval of 60s while the peer daemon 
    is configured to use 15s, both daemons will use a 15s interval for this connection.
    
    .. note::
       It takes about 3 intervals for a daemon to consider a connection as dead. Because each
       connection's heartbeat is the minimum one can run different intervals between different 
       daemons if necessary or desired.

:local_client_listen: 
    This subsection configures a unix domain socket where the daemon will receive requests from
    local clients. This subsection is available for all daemon roles. 
    
    .. note:: 
       If you run multiple daemons on the same node, this section needs a dedicated 
       setting for each daemon.

    :socket: 
        Defines the absolute path  to socket file (name included).

    :permissions: 
        Defines the access permissions of the socket. This is one way to limit the
        ability to call CSM APIs on a particular node.

    :group: 
        Specifies the group owner of the socket file.

:ssl: 
    This subsection allows the user to enable SSL encryption and authentication between daemons.
    If any of the two settings below are non-empty, the CSM daemon will enable SSL for 
    daemon-to-daemon connections by using the specified files.

    .. note:: 
        Since there's only one certificate entry in the configuration, the same certificate has to 
        serve as client and server certificate at the same time. This puts some limitations on the 
        configuration of the certificate infrastructure.

    :ca_file: 
        Specifies the file whic contains the Certificate Authority to check the validity of certificates.

    :cred_pem: 
        Specifies the file which contains the signed credentials/the certificate in PEM format.
      
        This certificate is presented to the passive/listening peer to prove that the daemon is 
        allowed to connect to the infrastructure. It is presented to the active/connecting peer to
        prove that the infrastructure is the one the daemon is looking for.


.. note::
   Note that the heartbeat is not determining the overall health of a
   peer daemon. The daemon might be able to respond to heartbeats..
   while still impeded to respond to API calls. A successful exchange
   of heartbeats tells the daemon that there’s a functional network
   connection and the network mgr thread is able to process inbound
   and outbound messages. To check if a daemon is able to process API
   calls, you might use the infrastructure health check tool.

.. note::
   The following is an explaination of the heartbeat mechanism  to show why it takes about 3 
   intervals to detect a dead connection.  
   
   The heartbeat between daemons works as follows:

    * After creating the connection, the daemons negotiate the smallest interval and start the timer.
    * Whenever a message arrives at one daemon, the timer is reset.

    * If the timer triggers, the daemon sends a heartbeat message to the peer and sets the 
      connection status as `UNSURE` (as in unsure whether the peer is still alive) and 
      resets the timer.

    * If the peer receives the heartbeat, it will reset its timer. 
      After the timer triggers, it will send a heartbeat back.

    * If the peer responds, the timer is reset and the connection status is `HAPPY`.

    * If the peer doesn't respond and the timer triggers again, the daemon will send a 
      second heartbeat, reset the timer, and change the status to `MISSING_RECV`.

    * If the timer triggers without a response, the connection will be considered `DEAD` and torn down.

------------------------------------------------------------------

Network Destination Blocks
^^^^^^^^^^^^^^^^^^^^^^^^^^

The following blocks unilaterally use the following two fields:

:host: 
    Determines the hostname or IP address of the listening socket.
    
    .. note:: 
        To bind a particular interface, it is recommended to use an explicit IP address. 
        Template entries like `__MASTER__` and `__AGGREGATOR__` are placeholders for the 
        IP or host of a CSM daemon with that role. 
        
        A host entry which is set to ``NONE`` will disable any attempt to connect.

:port: 
    Specifies the port of a socket, it is used as both a listening and destination port.

.. code-block:: json

        {
            "aggregator_listen":
            {
                "host": "__MASTER__",
                "port": 9815
            },

            "utility_listen":
            {
                "host": "__MASTER__",
                "port": 9816
            },

            "compute_listen":
            {
                "host": "__AGGREGATOR__",
                "port": 9800
            },

            "master":
            {
                "host": "__MASTER__",
                "port": 9815
            },

            "aggregatorA" :
            {
                "host": "__AGGREGATOR_A__",
                "port": 9800
            },
            "aggregatorB" :
            {
                "host": "__AGGREGATOR_B__",
                "port": 9800
            }
        }

Possible connection configuration sections:

:aggregator_listen: 
    [``master``] Specifies the interface and port where the master expects aggregators to connect.

:utility_listen:
    [``master``] Specifies the interface and port where the master expects utility daemons to connect.

:compute_listen:
    [``aggregator``] Specifies the interface and port where an aggregator expects compute nodes to connect.

:master: 
    [``utility``, ``aggregator``]
    Configures the coordinates of the master daemon. 

:aggregatorA: 
    [``compute``]
    Configures the coordinates of the primary aggregator. 
    The primary aggregator must be configured to allow the compute node to work (required to start). 

:aggregatorB:
    [``compute``]
    Configures the coordinates of the secondary aggregator. 
    Setting the ``host`` of this section to ``NONE`` will disable the compute daemons' attempt to 
    create and maintain a redundant path through a secondary aggregator.

The ``ufm`` Block
_________________

The ufm block configures the location and access to ufm.

.. code-block:: json

        {
            "rest_address"  : "__UFM_REST_ADDRESS__",
            "rest_port"     : 80,
            "ufm_ssl_file_path" : "/etc/ibm/csm",
            "ufm_ssl_file_name" : "csm_ufm_ssl_key.txt"
        }

:rest_address: The hostname of the UFM server.
:rest_port: The port UFM is serving the RESTful interface on (generally ``80``).
:ufm_ssl_file_path: The path to the SSL file for UFM access.
:ufm_ssl_file_name: An SSL file for UFM Access.

    May be generated using the following command:

    .. code-block:: bash

        openssl base64 -e <<< ${username}:${password} > /etc/ibm/csm/csm_ufm_ssl_key.txt;

.. _CSMD_BDS_Block:

The ``bds`` Block
_________________

The BDS block configures the access to the Big Data Store. 

.. code-block:: json

        {
                "host" : "__LOGSTASH__",
                "port" : 10522,
                "reconnect_interval_max" : 5,
                "data_cache_expiration" : 600
        }



:host: 
    Points to the host or IP address of the Logstash service. 
    
    If following the configuration section in :ref:`CASTLogstash` this should be ``localhost``.

:port: 
    The port that CSM should send entries to on the ``host``.

    If following the configuration section in :ref:`CASTLogstash` this should be ``10522``

:reconnect_interval_max:
    Reconnect interval in seconds to the Logstash server.

    Limits the frequency of reconnect attempts to the Logstash server in the event the service is
    down.  If the `aggregator` daemon is unable to connect, it will delay the next
    attempt for 1s. If the next attempt fails, it will wait 2s before retrying. This retry attempt
    will continue until ``reconnect_interval_max`` is reached.

:data_cache_expiration: 
    The number of seconds the daemon will keep any environmental data that failed to get sent to 
    Logstash. To limit the loss of environmental data, it is recommended to set the expiration to 
    be longer than the maximum reconnect interval.

.. note:: 
    This block is only leveraged on the Aggregator. 

The ``recurring_tasks`` Block
_____________________________

.. code-block:: json
    
    {
        "enabled" : false,
        "soft_fail_recovery" :
        {
            "enabled" : false,
            "interval" : "00:01:00",
            "retry" : 3
        }
    }

The recurring tasks configuration block, schedules recurring tasks that are supported by CSM.

:enabled: 
    Indicates whether or not recurring tasks will be processed by the daemons.


.. _csm_soft_failure_recovery-config :

``soft_fail_recovery``
^^^^^^^^^^^^^^^^^^^^^^

The soft failure recovery task executes the `soft_failure_recovery` API over the 
specified interval for the number of retries specified. For s

.. code-block:: json 

    {
        "enabled" : false,
        "interval" : "00:01:00",
        "retry" : 3
    }

:enabled: 
    Indicates whether or not this task will be processed by the daemons.
:interval: 
    The interval time between recurring tasks, format: `HH:mm:ss`.
:retry: 
    The number of times to retry the task on a specific node before placing the node into soft failure, 
    if the daemon is restarted the retry count for the node will be restarted.

.. attention:: This is only defined on the Master Daemon.

.. _CSMD_datacollection_Block:

The ``data_collection`` Block
_____________________________

The data collection block configures environmental data collection on compute nodes. It has no
effect on other daemon roles.

.. code-block:: json

        {
            "buckets":
                [
                    {
                        "execution_interval":"00:10:00",
                        "item_list": ["gpu", "environmental"]
                    },
                    {
                        "execution_interval":"24:00:00",
                        "item_list": ["ssd"]
                    }
                ]
        }

:buckets: 
    A json array of buckets for collection of environmental data. Each array element or bucket
    is configured as follows:

    :execution_interval: 
        Sets the interval (ISO format) that this bucket is supposed to be collected.

    :item_list: Specifies a json array of predefined items to collect. Currently available items are:

        +-------------------+----------------------------------------------+
        | ``gpu``           | A set of GPU stats and counters.             |
        +-------------------+----------------------------------------------+
        | ``environmental`` | A set of CPU and machine stats and counters. |
        +-------------------+----------------------------------------------+
        | ``ssd``           | A set of SSD wear stats and counters.        |
        +-------------------+----------------------------------------------+
        
.. _CSMD_jittermitigation_Block:

The ``jitter_mitigation`` block
________________________________

The jitter mitigation block is used to configure how core isolation functions in regards to 
Allocations. This block will only be required on Compute Node configurations.


.. code-block:: JavaScript

    "jitter_mitigation" :
    {
        "enabled"            : true,
        "blink_enabled"      : true,
        "system_smt"         : 0,
        "irq_affinity"       : true,
        "core_isolation_max" : 4,
        "socket_order"       : "00"
    }

:enabled:
    Toggle for whether or not the cgroup mitigation should be executed.
    Default is true (executes cgroup code).

:blink_enabled:
    Toggle for the blink feature when setting SMT mode, which shut downs cores to blink.
    Default is true (executes the cgroup blink).
     

:system_smt:
    The SMT mode of the system cgroup, if unset this will use the maximum SMT mode.

    Setting this option to ``0`` will maximize the SMT mode.
    Setting this option higher than the allowed SMT mode will clamp to the maximum SMT mode.

:irq_affinity:
    Flag determining the behavior of Allocation Creation in relation to IRQ rebalancing. 

    If set to true the cores in system cgroup will have all IRQ rebalanced to them and the IRQ Balance
    Daemon will be shut down. 

    If no core isolation occurs a rebalance across all cores will be performed and the IRQ Balance
    Daemon will be reset.

    If set to false, no rebalancing occurs.

:core_isolation_max:
    The maximum number of cores allowed on the node to be set aside for the system cgroup.
    By default this will be set to ``4``.

:socket_order:
    A mask determining the direction which the isolated cores will be allocated for the 
    system cgroup, per socket. ``0`` indicates the cores will be allocated by the cgroup left to right.
    ``1`` indicates that the cores will be allocated by the cgroup right to left. 

    Each character in the mapping string corresponds to a socket on the node. If a socket is not
    defined by the mapping it will be set to ``0``.


    


    


