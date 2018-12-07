Configuring API timeouts
========================

CSM APIs return a timeout failure if their execution does not complete in a set amount of time. By default every API times out at 30 seconds. If a system admin wants to extend or shorten this execution window, then they can edit a timeout configuration file.

The packaged configuration can be found here: ``etc/ibm/csm/csm_api.cfg``
A default configuration file is included with CSM at ``/opt/ibm/csm/share/etc/csm_api.cfg``, reproduced below for easy reference.

.. code-block:: JSON

    {
       "csm_allocation_create" : 120,
       "csm_allocation_delete" : 120,
       "csm_allocation_update_state" : 120,
       "csm_allocation_step_end" : 120,
       "csm_allocation_step_begin" : 120,
       "csm_allocation_query" : 120
    }

For standard usage of CSM, this default cfg file may be used. System Administrators may alter this file to fit their environment. 

This file works by specifying an api and a number (in seconds) for the API to time out. By default every API times out at 30 seconds if not over ridden here. 

If you wish to change the default API timeout from 30 seconds to something else, then you have to edit the code here: ``bluecoral/csmnet/include/csm_timing.h`` and tweak the default macros to your new values.

.. code-block:: C

    // Default timeout.
    #define CSM_RECV_TIMEOUT_GRANULARITY ( 5 ) // min and granularity
    #define CSM_RECV_TIMEOUT_MIN ( CSM_RECV_TIMEOUT_GRANULARITY ) // shortest possible timeout
    #define CSM_RECV_TIMEOUT_SECONDS ( CSM_RECV_TIMEOUT_GRANULARITY * 6 ) // 30 Secs
    #define CSM_RECV_TIMEOUT_MILLISECONDS ( CSM_RECV_TIMEOUT_SECONDS * 1000 ) 
    #define CSM_RECV_TIMEOUT_MILLI_HALF   ( CSM_RECV_TIMEOUT_MILLISECONDS / 2 )

We recommend using the configuration file and not touching the source code.

Instead of altering the default configuration file, a system administrator may choose to point to a different file for CSM APIs to use. If this is the case, then the system administrator must update the appropriate daemon configuration files. ``api_configuration_file`` is the field you would need to change. It is found in all the CSM daemon config files, under the ``csm`` level. 

An excerpt of the ``csm_master.cfg`` is reproduced below as an example.

.. code-block:: json

	{
	    "csm" :
	    {
	        "role": "Master",
	        "thread_pool_size" : 1,
	        "api_permission_file": "/etc/ibm/csm/csm_api.acl",
	        "api_configuration_file": "/etc/ibm/csm/csm_api.cfg",

An example of editing this field is shown below.

.. code-block:: json

	{
	    "csm" :
	    {
	        "role": "Master",
	        "thread_pool_size" : 1,
	        "api_permission_file": "/etc/ibm/csm/csm_api.acl",
	        "api_configuration_file": "/etc/ibm/csm/csm_api_custom_timeout.cfg",

If you have trouble finding the config files, then daemon config files are located:

* source repo: ``bluecoral/csmconf/``

* ship to: ``/opt/ibm/csm/share/``

* run from: ``etc/ibm/csm/``


This feature is primarily used in the context of extending the length of time allocated for Multicast APIs. This is because some of the calls that happen inside of these APIs, such as calling user admin scripts, could vary and take a long time to complete execution.








