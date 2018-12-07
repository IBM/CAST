Configuring API control level
=============================

An API must be set to one of three control levels: *priviledged*, *private*, or *public*. If an API is not specified, then it defaults to the privileged level. Below is a capture of the control level section of the default CSM ACL file.

.. code:: json

    "private":
    [
        "csm_allocation_details",
        "csm_allocation_step_query_details"
    ]
    "public":
    [
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
	    "csm_node_resources_query_all"
    ]

