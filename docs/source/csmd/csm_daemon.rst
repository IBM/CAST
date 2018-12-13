CSMD Executable
===============

The ``csmd`` executable is bundled in the |csm-core| at ``/opt/ibm/csm/sbin/csmd``.

This executable has been daemonized to run the CSM Infrastructure. 

.. contents::
   :local:


CSMD Command line options
-------------------------

.. code-block:: none

    Supported Command Line Options:
  -h [ --help ]            Show this help
  -f [ --file ] arg        Specify configuration file
                                  (default: /etc/ibm/csm/csm_master.cfg)
  -r [ --role ] arg        Set the role of the daemon (M|m)[aster] |
                                  (A|a)[ggregator] | (C|c)[ompute] |
                                  (U|u)[tility]

.. note::
  * The role is determined by the first letter of the role argument.
  * The file path should be an absolute path to avoid confusion.

CSMD Services
-------------

CSM defines four service types that are accessible through ``systemctl``.

+------------+---------------------------------+-------------------------+
| Type       | Config                          | Service                 |
+============+=================================+=========================+
| Utility    | /etc/ibm/csm/csm_utility.cfg    | csmd-utility.service    |
+------------+---------------------------------+-------------------------+
| Master     | /etc/ibm/csm/csm_master.cfg     | csmd-master.service     |
+------------+---------------------------------+-------------------------+
| Aggregator | /etc/ibm/csm/csm_aggregator.cfg | csmd-aggregator.service |
+------------+---------------------------------+-------------------------+
| Compute    | /etc/ibm/csm/csm_compute.cfg    | csmd-compute.service    |
+------------+---------------------------------+-------------------------+

The following is a sample how to manipulate these services:

.. code-block:: bash

   systemctl [status|start|stop|restart] csmd-utility


CSMD Configuration
------------------

To configure the ``csmd`` daemon please refer to :ref:`CSMDConfig`.


.. _CSMDACLConf:

ACL Configuration
-----------------

To use the CSM API with proper security an ACL file must be configured. Using a combination of 
user privilege level and API access level, CSM determines what the actions to perform when 
an API is called by a user. 

For example, if the user doesn't have the proper privilege on a private API, the returned 
information will be limited or denied all together.

A user can be either privileged or non-privileged.  To become a privileged user, either the user 
name must be listed as a privileged user in the ACL file or the user needs to be a member of a 
group that's listed as a privileged group.

A template or default ACL file is included in the installation and can be found under 
``/opt/ibm/share/etc/csm_api.acl``.

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

:privileged_user_id:
    Lists the users that will be allowed to perform administrator tasks in terms of calling
    privileged CSM APIs. The user root will always be able to call APIs regardless of the 
    configured privilege level. 
    
    If more than one user needs to be listed, use the ``[..,..]`` format for json lists.

:privileged_user_group: 
    Lists the groups which will be allowed to perform administrator tasks in terms of calling
    privileged CSM APIs. Users in group `root` will always be able to call APIs independent of the 
    configured privilege level. 
    
    If more than one user needs to be listed, use the ``[..,..]`` format for json lists.

:private: 
    Specifies a list of CSM APIs that are private. A private API can only be called by
    privileged users or owners of the corresponding resources. 
    
    For example, `csm_allocation_query_details` can only be called by the owner of the 
    requested allocation.

:public: 
    Specifies a list of CSM APIs that can be called by any user who has access to the node and
    the client_listen socket of the CSM daemon.

:privileged: 
    Explicitly configure a list of CSM APIs as privileged APIs. The section is not present in
    the template ACL file because any API will be `privileged` unless listed as `private` or `public`.

.. warning::
    The ACL files should be synchronized between all nodes of the CSM infrastructure. Each daemon 
    will attempt to enforce as many of the permissions as possible before routing the request to 
    other daemons for furtherprocessing. 
    
    For example, if a user calls an API on a utility node where the API is configured `public`,
    there will be no further permission check if that request is forwarded to the master even if 
    the ACL config on the master configures the API as private or privileged. 
    
    The permissions of a request are determined at the point of entry to the infrastructure. 
    Enforcement is based on the effective user id and group id on the machine that runs
    the requesting client process.

.. _CSMDAPIConf:

API Configuration
-----------------

The CSM API configuration file (json) allows the admin to set a number of API-specific parameters.

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
   "csm_jsrun_cmd" : 60,
   "csm_soft_failure_recovery" : 240
 }

At the moment this only includes the timeout for CSM APIs (in seconds).  The API config file path 
and name is defined in the CSM config file setting ``csm.api_configuration_file``. 

.. warning::
    The API configuration files should be synchronized between all nodes of the CSM infrastructure 
    to avoid unexpected API timeout behavior. 
    
    The current version of CSM calculates daemon-role-specific, fixed API timeouts based on the 
    configuration file. Meaning the actual timeouts will be different (lower) than the configured 
    time to account for delays in the communication, processing, or number of internal round-trips 
    for certain APIs. 
        
    For example, an API called from the utility node is configured with a 120s timeout. 
    Once the request is forwarded to the master, the master will enforce a timeout of 119s 
    accounting for network and processing delays. 
    
    If the request requires the master to reach out to compute nodes the aggregators will enforce 
    a timeout of 58s because the aggregator accounts for some APIs requiring 2 round trips and 1
    additional network hop.
    
    
    Generally, the expected enforced timeout is:
    <value> / 2 - 2s.


