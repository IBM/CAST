CSM Daemon
==========

CSMD Command line options
-------------------------

.. code-block:: none

    Supported Command Line Options:
  -h [ --help ]                  Show this help
  -f [ --file ] arg (=coral.cfg) Specify configuration file (default:
                                 coral.cfg)
  -r [ --role ] arg              Set the role of the daemon (M|m)[aster] |
                                 (A|a)[ggregator] | (C|c)[ompute] |
                                 (U|u)[tility]

.. note::
  * The role is determined by the first letter of the role argument. If you like, you can start your master by using `-r minion`.
  * The file path should be an absolute path to avoid confusion.


CSMD Configuration
------------------

Each type of daemon is set up via a dedicated configuration file (default location: `/etc/ibm/csm/csm_*.cfg`).
The format of the config file is json. Which means you'll get json parsing errors in case there are formatting problems in the config file.
Below, the sections of the configuration file are explained in detail:

The csm block
^^^^^^^^^^^^^

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
        "data_collection" : { }
    }
  }



Beginning with the top-level configuration section `csm`.
  * ``role`` is setting the role of the daemon the same way as via the command line.
    Note that if the role is provided in both command line and config file, the command line setting overrides what's in the config.

  * ``thread_pool_size`` controls the number of worker threads that are used to process any CSM API calls and event handling.
    A setting of 1 should generally suffice. However, if there are some CSM API calls that spawn external processes which
    in turn might call other CSM APIs (e.g. `csm_allocation_create() spawning the prolog`). The worker thread waits for the completion
    of the spawned process and with only one available worker, there will be no resources left to process the additional API call.
    This is why a setting of at least 4 is recommended for compute nodes.

  * ``api_permission_file`` points to the file that controls the permissions of API calls to specify admin users/groups and classify
    CSM APIs as public, private, or privileged. See `ACL Configuration`_ for details.

  * ``api_configuration_file`` points to the file that contains detailled configuration settings for CSM API calls. For example, if
    an API requires a non-default timeout, it might be configured in that file. See `API Configuration`_ for details.

  * ``log`` is a subsection that defines the logging level of various components of the daemon. See `The log block`_.
  * ``db`` is a master-specfic block to set up the data base connection. See `The database block`_.
  * ``inventory`` configures the inventory collection component. See `The inventory block`_.
  * ``net`` configures the network collection component to define the interconnectivity of the CSM infrastructure. See `The network block`_.
  * ``ras`` tbd
  * ``ufm`` configure access to UFM. See `The UFM block`_.
  * ``bds`` addresses, ports, and other settings for BDS access. See `The BDS block`.
  * ``recurring_tasks`` setting up intervals and types of predefined recurring tasks to be triggered by the daemon. See `The recurring tasks block`_.
  * ``data_collection`` enable and configure predefined buckets for environmental data collection. See `The data collection block`_.


The log block
^^^^^^^^^^^^^

.. code-block:: json

        {
            "format"                    :   "%TimeStamp% %SubComponent%::%Severity% | %Message%",
            "consoleLog"                :   false,
            "sysLog"                    :   true,
            "fileLog"                   :   "/var/log/ibm/csm/csm_master.log",
            "#rotationSize_comment_1"  :   "Maximum size (in bytes) of the log file, 1000000000 bytes is ~1GB",
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
        }

The log block determines what amount of logging goes to which files and/or console and also specifies log rotation options.
  * ``format`` defines a template for the format of the CSM log lines. In the given example, a log `Message` is prefixed with
    the `TimeStamp` followed the name of the `SubComponent` and the `Severity`. The `SubComponent` helps to identify the source
    of the message (e.g. the csmnet = Network component; csmapi = CSM API call processing).

  * ``consoleLog`` determines whether the logs should go to the console or not. Can be ``true`` or ``false``.

  * ``fileLog`` determine whether the logs should go to syslog or not. Can be ``true`` or ``false``.
  * ``rotationSize`` limits the size (bytes) of the log file before starting a new log file.
    If set to -1 the file is allowed to grow without limit.
  * ``default_sev`` set the logging level/verbosity for any component that's not mentioned explicitly. Options are:

      * ``critical`` to log only very critical and fatal errors
      * ``error`` to log errors and critical messages
      * ``warning`` to log warnings and everything above
      * ``info`` log info messages and everything above
      * ``debug`` log debug level messages and everything above; very verbose.
      * ``trace`` very detailled logging including everything. Intended for tracing analysis. Will create a lot of output.

  * ``csmdb`` log level of the database component. Includes messages about database access and request handling.
  * ``csmnet`` log level of the network component. Includes messages about the network interaction between daemons and daemons and client processes.
  * ``csmd`` log level of the core daemon. Includes messages from the core of the infrastructure handling and management.
  * ``csmras`` log level of the RAS component. Includes messages about RAS events and their processing within the daemon.
  * ``csmapi`` log level of CSM API handling. Includes messages about API call processing.
  * ``csmenv`` log level of environmental data handling. Includes messages related primarily to data collection and shipping from compute to aggregators.
  * ``transaction`` tbd
  * ``transaction_file`` tdb
  * ``transaction_rotation_size`` transaction log rotation settings; similar to csm log rotation settings


The database block
^^^^^^^^^^^^^^^^^^

.. code-block:: json

        {
            "connection_pool_size" : 10,
            "host"                 : "127.0.0.1",
            "name"                 : "csmdb",
            "user"                 : "csmdb",
            "password"             : "",
            "schema_name"          : ""
        }

The database block configures the location and access parameters of the CSM database.
The settings are specific and relevant to the master daemon only.

  * ``connection_pool_size`` configures the number of connections to the database.
    The number also specifies the number of database worker thread for concurrent access
    and parallel processing of requests. The number should be empirically adjusted to the
    demand your system creates. Demand will grow with size of the system and/or with the
    frequency of CSM API calls.
  * ``host`` the hostname or IP address of the database server
  * ``name`` the name of the database in your postgresql server
  * ``user`` the username that CSM should use to access the database
  * ``password`` the password to access the database. Having this setting in the file obviously
    makes it important to adjust the permissions to prevent unathorized users from viewing the
    config file
  * ``schema_name`` in case there are named schemas in use, this configures the name


The inventory block
^^^^^^^^^^^^^^^^^^^

.. code-block:: json

        {
            "csm_inv_log_dir" : "/var/log/ibm/csm/inv",
            "ufm":
            {
                "ib_cable_errors" : "bad_ib_cable_records.txt",
                "switch_errors"   : "bad_switch_records.txt"
            }
        }

The inventory block configures the location of files that are used for collection of the network inventory like
InfiniBand cables and switches.

  * ``csm_inv_log_dir`` absolute path of inventory collection logs
  * ``ufm``

    * ``ib_cable_errors`` tbd
    * ``switch_errors`` tbd


The network block
^^^^^^^^^^^^^^^^^

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

  * ``heartbeat_interval`` determines the interval (in seconds) that this daemon will use for
    any connections to other CSM daemon(s) of the infrastructure. However, the actual interval of a
    connection will be the minimum interval of the 2 peers of that connection. For example If one daemon
    initiates the connection with an interval of 60s while the peer daemon is configured to use 15s,
    both daemons will use a 15s interval for this connection. Note that it takes about 3 intervals
    for a daemon to consider a connection as dead.

  * ``local_client_listen`` subsection configures a unix domain socket where the daemon will receive
    requests from local clients. This subsection is available for all daemon roles. Note that if you
    run multiple daemons on the same node, this section needs a dedicated setting for each daemon.

    * ``socket`` defines the path+name of the socket file
    * ``permissions`` defines the access permissions of the socket. This is one way to limit the
      ability to call CSM APIs on a particular node.
    * ``group`` allows to specify the group of the owner of the socket file

  * ``ssl`` subsection allows to enable SSL encryption and authentication between daemons.
    Note: Since there's only one certificate entry in the configuration, the same certificate has to serve
    as client and server certificate at the same time. This puts some limitations on the configuration of
    the certificate infrastructure.

    * ``ca_file`` specifies the file that contains the CA to check the validity of certificates
    * ``cred_pem`` specifies the file that contains the signed credentials/the certificate in PEM format.
      This certificate is presented to the passive/listening peer to proof that the daemon is allowed to
      connect to the infrastructure. And it is presented to the active/connecting peer to proof that the
      infrastructure is the one the daemon is looking for.


The following subsections are specific to certain daemon roles with each of them requiring the following settings:

  * ``host`` determines the hostname or IP address of the listening socket. Note if you want to be sure to bind to
    particular interface, it is recommended to use an explicit IP address. Template entries like `__MASTER__`
    or `__AGGREGATOR__` are placeholders for the IP or host of a CSM daemon with that role. A host entry that is
    set to ``NONE`` would disable any attempt to connect.

  * ``port`` specifies the port of a socket. It's being used for both cases listening and destination port.

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
            }
        }

Listening socket configurations for some daemons:

  * ``aggregator_listen`` specifies the interface and port where the master expects aggregators to connect (master only).
  * ``utility_listen`` specifies the interface and port where the master expects utility daemons to connect (master only).
  * ``compute_listen`` specifies the interface and port where an aggregator expects compute nodes to connect (aggregator only).

.. code-block:: json

        {
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

Connection destinations for some daemons:
  * ``master`` configures the coordinates of the master daemon. (utility and aggregator only)
  * ``aggregatorA`` configures the coordinates of the primary aggregator (compute only)
  * ``aggregatorB`` configures the coordinates of the secondary aggregator (compute only)


The UFM block
^^^^^^^^^^^^^

.. code-block:: json

        {
            "rest_address"  : "__UFM_REST_ADDRESS__",
            "rest_port"     : 80,
            "rest_user"     : "admin",
            "rest_password" : "123456"
        }

The ufm block configures the location and access to ufm.
  * ``rest_address`` tbd
  * ``rest_port`` tbd
  * ``rest_user`` tbd
  * ``rest_password`` tbd

The BDS block
^^^^^^^^^^^^^

.. code-block:: json

        {
                "host" : "__LOGSTASH__",
                "port" : 10522,
                "reconnect_interval_max" : 5,
                "data_cache_expiration" : 600
        }

The BDS block configures the access to the Big Data Store. The settings are only relevant
on the aggregator daemon at the moment.

  * ``host`` points to the host or IP address of the Logstash service. In recommended the setup
    with Logstash running on the service nodes, this would point to localhost.

  * ``port`` defines the port of the Logstash service

  * ``reconnect_interval_max`` limits the frequency of reconnection attempts in case the Logstash
    service is not reachable. If the aggregator daemon is unable to connect, it will delay the next
    attempt for 1s. If that next attempt fails, it will wait 2s before retrying. It will keep
    increasing this reconnect delay until the configured maximum (in seconds) is reached.

  * ``data_cache_expiration`` allows to specify the number of seconds the daemon will keep any
    environmental data that failed to get send to Logstash. To limit the loss of environmental data,
    it is recommended to set the expiration to be longer than the maximum reconnect interval.


The recurring tasks block
^^^^^^^^^^^^^^^^^^^^^^^^^

This is currently a placeholder for an upcoming feature of the CSM daemon.

The data collection block
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: json

        {
            "buckets":
                [
                    {
                        "execution_interval":"00:10:00",
                        "item_list": ["gpu", "environmental"]
                    }
                ]
        }

The data collection block configures environmental data collection on compute nodes. It has no
effect on other daemon roles.

  * ``buckets`` is a json array of buckets for collection of environmental data. Each array element or bucket
    is configured as follows:

    * ``execution_interval`` sets the interval (ISO time format) that this bucket is supposed to get collected
    * ``item_list`` specifies a json array of predefined items to collect. Currently available items are:

      * ``gpu`` a set of GPU stats and counters
      * ``environmental`` a set of CPU and machine stats and counters

ACL Configuration
-----------------

.. code-block:: json

  {
    "privileged_user_id": "root",

    "privileged_group_id": "root",

    "private":
    ["csm_allocation_query_details",
     "csm_allocation_delete",
     "csm_allocation_update_state",
     "csm_bb_cmd",
     "csm_jsrun_cmd",
     "csm_allocation_step_query_details"],

    "public":
    ["csm_allocation_step_cgroup_create",
     "csm_allocation_step_cgroup_delete",
     "csm_allocation_query",
     "csm_allocation_query_active_all",
     "csm_allocation_resources_query",
     "csm_allocation_step_begin",
     "csm_allocation_step_end",
     "csm_allocation_step_query",
     "csm_allocation_step_query_active_all",
     "csm_diag_run_query",
     "csm_node_attributes_query",
     "csm_node_attributes_query_history",
     "csm_node_resources_query",
     "csm_node_resources_query_all"]
  }

The CSM API ACL configuration is done through the file pointed at by the
setting in the csm config file (``csm.api_permission_file``). It is required
to be in json format. The main entries are:

  * ``privileged_user_id`` lists a number of users that will be allowed to perform administrator
    tasks in terms of calling privileged CSM APIs.

  * ``privileged_user_group`` lists a number of groups that will be allowed to perform administrator
    tasks in terms of calling privileged CSM APIs.

  * ``private`` specifies a list of CSM APIs that are private. A private API can only be called by
    privileged users or owners of the corresponding resources. For example, `csm_allocation_query_details`
    can only be called by the owner of the requested allocation.

  * ``public`` specifies a list of CSM APIs that can be called by any user who has access to the node and
    the client_listen socked of the CSM daemon.

  * ``privileged`` explicitly configure a list of CSM APIs as privileged APIs. The section is not present in
    the template ACL file because any API will be `privileged` unless it's listed as `private` or `public`.

.. note::
  The ACL files should be synchronized between all nodes of the CSM infrastructure. Each daemon will attempt
  to enforce as many of the permissions as possible before routing the request to other daemons for further
  processing. For example, if a user calls an API on a utility node where the API is configured `public`,
  there will be no further permission check if that request is forwarded to the master even if the ACL config
  on the master configures the API as private or privileged. Explained differently: The permissions of a request
  are determined at the point of entry to the infrastructure, i.e. by the CSM daemon that sees the client request
  first. The enforcement is based on the effective user id and the effective group id an the machine that runs
  the requesting client process.


API Configuration
-----------------

.. code-block:: json

 {
   "#comment_1" : "This will be ignored",
   "csm_allocation_create" : 120,
   "csm_allocation_delete" : 120,
   "csm_allocation_update_state" : 120,
   "csm_allocation_step_end" : 120,
   "csm_allocation_step_begin" : 120,
   "csm_allocation_query" : 120,
   "csm_bb_cmd" : 120,
   "csm_jsrun_cmd" : 60
 }

The CSM API configuration file allows the admin to set a number of API-specific parameters.
At the moment this only includes the timeout for CSM APIs. The file format is json. The API config file path and name
is defined in the CSM config file setting ``csm.api_configuration_file``.

The timeout is given in seconds.

.. note::
  The API configuration files should be synchronized between all nodes of the CSM infrastructure to avoid unexpected
  API timeout behavior. The current version of CSM calculates daemon-role-specific, fixed API timeouts based on the
  configuration file. That means the actual timeouts will be different (lower) than the configured time to account for
  delays in the communication, processing, or number of internal round-trips for certain APIs. For example, an API called
  from the utility node is configured with a 120s timeout. Once the request is forwarded to the master, the master will enforce
  a timeout of 119s accounting for network and processing delays. If the request requires the master to reach out to compute nodes
  the aggregators will enforce a timeout of 58s because the aggregator accounts for some APIs requiring 2 round trips and 1
  additional network hop.
  Generally, you should expect the actually enforced timeout to be: <value> / 2 - 2s.

