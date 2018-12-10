Configuring user control, security, and access level
====================================================

To use CSM APIs at proper security and control levels, an ACL file needs to be configured. Using a combination of a user privilege level and API access level, CSM determines what happens when users call APIs. For example, if the user doesn’t have the proper level of privilege on a private API, then information returned will be limited or maybe denied all together.  

A default ACL file `csm_api.acl` is included with CSM and located at `/opt/ibm/csm/share/etc/`. It is reproduced below for easy reference.

.. code-block:: JSON

    {
        "privileged_user_id": "root",
        "privileged_group_id": "root",
        "private": [
            "csm_allocation_query_details",
            "csm_allocation_step_query_details"],
        "public": [
            "csm_allocation_step_cgroup_create",
            "csm_allocation_step_cgroup_delete",
            "csm_allocation_query",
            "csm_allocation_query_active_all",
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

For standard usage of CSM, this default ACL file may be used. System Administrators may alter this file to fit their environment. The user and user group *root* will always be considered privileged whether listed in this ACL file or not. 

Instead of altering the default ACL file, a system administrator may choose to point to a different ACL file for CSM APIs to use. The ``api_permission_file`` field in the daemon configuration file's ``csm`` section determines the file used by the daemon for ACL.

An excerpt of the default `csm_master.cfg` is reproduced below as an example.

.. code-block:: JSON

    {
        "role": "Master",
        "thread_pool_size" : 1,
        "api_permission_file": "/etc/ibm/csm/csm_api.acl",
        "api_configuration_file": "/etc/ibm/csm/csm_api.cfg",

An example of editing tthis field is shown below:

.. code-block:: JSON

    {
        "role": "Master",
        "thread_pool_size" : 1,
        "api_permission_file": "/etc/ibm/csm/csm_api_custom_permissions.acl",
        "api_configuration_file": "/etc/ibm/csm/csm_api.cfg",

If you have trouble finding the config files, then daemon config files are located:

- source repo: “bluecoral/csmconf/”
- ship to: “/opt/ibm/csm/share/”
- run from: “etc/ibm/csm/”

In section 4.2 of the “Installation and Configuration Guide”, a description is provided for adding a new Linux group and making that group privileged in this file. 

.. note:: Make the above reference a link.





