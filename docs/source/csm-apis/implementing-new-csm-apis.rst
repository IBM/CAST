Implementing New CSM APIs
=========================

CSM is an open source project that can be contributed to by the community. 
This section is a guide on how to contribute a new CSM API to this project.

Contributors should visit the |git-repo| and follow the instructions in the **How to Contribute** 
section of the repository readme. 

Front-end
----------

This is the API an end user would interact with. The front end interacts with the 
:ref:`CSMDInfrastructure` through network connections to varying results.

Follow these steps to create/edit an api. The diagram below shows where to find the 
appropriate files in the |git-repo| repository.

.. image:: https://user-images.githubusercontent.com/4662139/49670824-efe77680-fa33-11e8-9703-170a022e9c5c.png
   :height: 400px

The following numbers reference the chart above.

:1: When creating an API it should be determined whether it accepts input and produces output. 
    The CSM design follows the pattern of **<API_Name>_input_t** for input structs and 
    **<API_Name>_output_t** for output structs. These structs should be defined through use of an 
    x-macro in the appropriate folder for the API type under the ``csmi/include/csm_types/struct_defs`` 
    directory. 
    
    A `struct README`_ is provided in this directory with an in-depth description of the 
    struct definition process. 

.. code-block:: none

    /*================================================================================*/
    
    /**
     * CSMI_COMMENT
     * @brief An input wrapper for @ref csm_example_api.
     */
    
    #ifndef CSMI_STRUCT_NAME
    // ! The name of the struct to be generated !
    #define CSMI_STRUCT_NAME csm_example_api_input_t 
    
    #undef CSMI_BASIC              
    #undef CSMI_STRING             
    #undef CSMI_STRING_FIXED       
    #undef CSMI_ARRAY              
    #undef CSMI_ARRAY_FIXED        
    #undef CSMI_ARRAY_STR          
    #undef CSMI_ARRAY_STR_FIXED    
    #undef CSMI_STRUCT             
    #undef CSMI_ARRAY_STRUCT       
    #undef CSMI_ARRAY_STRUCT_FIXED 
    #undef CSMI_NONE  
    
    // ! Set to 1 (true) when a field matching the type is present !
    #define CSMI_BASIC               1
    #define CSMI_STRING              1
    #define CSMI_STRING_FIXED        0
    #define CSMI_ARRAY               0
    #define CSMI_ARRAY_FIXED         0
    #define CSMI_ARRAY_STR           1
    #define CSMI_ARRAY_STR_FIXED     0
    #define CSMI_STRUCT              0
    #define CSMI_ARRAY_STRUCT        0
    #define CSMI_ARRAY_STRUCT_FIXED  0
    #define CSMI_NONE                0
    #endif 
    
    // CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, extra ) /**< comment */
    CSMI_VERSION_START(CSM_VERSION_1_0_0)
    CSMI_STRUCT_MEMBER(int32_t , my_first_int         , BASIC    ,                      , -1  , ) /**< Example int32_t value. API will ignore values less than 1.*/
    CSMI_STRUCT_MEMBER(uint32_t, my_string_array_count, BASIC    ,                      , 0   , ) /**< Number of elements in the 'my_string_array' array. Must be     greater than zero. Size of @ref my_string_array.*/
    CSMI_STRUCT_MEMBER(char**  , my_string_array      , ARRAY_STR, my_string_array_count, NULL, ) /**< comment for my_string_array*/
    CSMI_VERSION_END(fc57b7dafbe3060895b8d4b2113cbbf0)
    
    CSMI_VERSION_START(CSM_DEVELOPMENT)
    CSMI_STRUCT_MEMBER(int32_t, another_int, BASIC, , -1, ) /**< Another int.*/
    CSMI_VERSION_END(0)
    
    #undef CSMI_VERSION_START
    #undef CSMI_VERSION_END
    #undef CSMI_STRUCT_MEMBER

    .. attention:: Follow the existing `struct README`_ in the code source for supplemental details.


:2: The X-Macro definition files will be collated by their ordering in the local ``type_order.def``
    file. New files added to this ordering should just be the file name. 
    
    Specific details for this file are in the `struct README`_.

:3: The ``special_preprocess.def`` file is prepended to the generated header. 
    This file should only be modified if your struct uses a special header or requires some 
    preprocessor directive. Please note that this will apply globally to the generated header file.

:4: After defining the X-Macro files the developer should run the ``regenerate_headers.sh`` script 
    located at ``bluecoral/csmi/include/struct_generator/``. This script will prepare the structs 
    and enumerated types for use in the CSM APIs and infrastructure. Serialization functions and 
    python bindings will also be generated. 

    The files modified by this script include:
    
    :Common:
    
        +--------------------+------------------------------+
        | Type Header        | csmi_type_common.h           |
        +--------------------+------------------------------+
        | Function Header    | csmi_type_common_funct.h     |
        +--------------------+------------------------------+
        | Serialization Code | csmi_common_serial.c         |
        +--------------------+------------------------------+
    
    :Workload Manager:
    
        +--------------------+------------------------------+
        | Type Header        | csmi_type_wm.h               |
        +--------------------+------------------------------+
        | Function Header    | csmi_type_wm_funct.h         |
        +--------------------+------------------------------+
        | Serialization Code | csmi_wm_serialization.c      |
        +--------------------+------------------------------+
    
    :Inventory:
    
        +--------------------+------------------------------+
        | Type Header        | csmi_type_inv.h              |
        +--------------------+------------------------------+
        | Function Header    | csmi_type_inv_funct.h        |
        +--------------------+------------------------------+
        | Serialization Code | csmi_inv_serialization.c     |
        +--------------------+------------------------------+
    
    :Burst Buffer:
    
        +--------------------+-----------------------------+ 
        | Type Header        | csmi_type_bb.h              |
        +--------------------+-----------------------------+
        | Function Header    | csmi_type_bb_funct.h        |
        +--------------------+-----------------------------+
        | Serialization Code | csmi_bb_serialization.c     |
        +--------------------+-----------------------------+
    
    :RAS:
    
        +--------------------+-----------------------------+
        | Type Header        | csmi_type_ras.h             |
        +--------------------+-----------------------------+
        | Function Header    | csmi_type_ras_funct.h       |
        +--------------------+-----------------------------+
        | Serialization Code | csmi_ras_serialization.c    |
        +--------------------+-----------------------------+
    
    :Diagnostic:
    
        +--------------------+-----------------------------+
        | Type Header        | csmi_type_diag.h            |
        +--------------------+-----------------------------+
        | Function Header    | csmi_type_diag_funct.h      |
        +--------------------+-----------------------------+
        | Serialization Code | csmi_diag_serialization.c   |
        +--------------------+-----------------------------+
    
    :Launch:
    
        +--------------------+-----------------------------+
        | Type Header        | csmi_type_launch.h          |
        +--------------------+-----------------------------+
        | Function Header    | csmi_type_launch_funct.h    |
        +--------------------+-----------------------------+
        | Serialization Code | csmi_launch_serialization.c |
        +--------------------+-----------------------------+


:5: Add the API function declaration to the appropriate API file, consult the table below 
    for the correct file to add your API to (in the ``bluecoral/csmi/include`` directory):

    +------------------+----------------------------+
    | API Type         | API File                   |
    +==================+============================+
    | Common           | csm_api_common.h           |
    +------------------+----------------------------+
    | Workload Manager | csm_api_workload_manager.h |
    +------------------+----------------------------+
    | Inventory        | csm_api_inventory.h        |
    +------------------+----------------------------+
    | Burst Buffer     | csm_api_burst_buffer.h     |
    +------------------+----------------------------+
    | RAS              | csm_api_ras.h              |
    +------------------+----------------------------+
    | Diagnostic       | csm_api_diagnostic.h       |
    +------------------+----------------------------+

:6: Add a command to the ``csmi/src/common/include/csmi_cmds_def.h`` X-Macro. This will generate an 
    enumerated type in the format of **CSM_CMD_<csm-contents>** [cmd(<csm-contents>)] on 
    compilation and used in the front and backend API. 

:7: The implementation of the C API should be placed in the appropriate src directory:

    +------------------+----------------------------+
    | API Type         | Source Directory           |
    +==================+============================+
    | Common           | csmi/src/common/src        |
    +------------------+----------------------------+
    | Workload Manager | csmi/src/wm/src            |
    +------------------+----------------------------+
    | Inventory        | csmi/src/inv/src           |
    +------------------+----------------------------+
    | Burst Buffer     | csmi/src/bb/src            |
    +------------------+----------------------------+
    | RAS              | csmi/src/ras/src           |
    +------------------+----------------------------+
    | Diagnostic       | csmi/src/diag/src          |
    +------------------+----------------------------+

Generally speaking the frontend C API implementation should follow a mostly 
standard pattern as outlined below:

.. code-block:: C

	#include "csmutil/include/csmutil_logging.h"
	#include "csmutil/include/timing.h"
	#include "csmi/src/common/include/csmi_api_internal.h"
	#include "csmi/src/common/include/csmi_common_utils.h"
	#include "csmi/include/“<API_HEADER>

	// The expected command, defined in “csmi/src/common/include/csmi_cmds_def.h”
	const static csmi_cmd_t expected_cmd = <CSM_CMD>;

	// This function must be definedand supplied to the create_csm_api_object 
	// function if the API specifies an output.
	void csmi_<api>_destroy(csm_api_object *handle);

	// The actual implementation of the API.
	int csm_<api>( csm_api_object **handle, <input_type> *input, <output_type> ** output) 
	{
		START_TIMING()
		
		char     *buffer        = NULL; // A buffer to store the serialized input struct.
	 	uint32_t buffer_length  = 0;	  // The length of the buffer.
	 	char     *return_buffer = NULL; // A return buffer for output from the backend.
	 	uint32_t return_buffer_len = 0; // The length of the return buffer.
	 	Int.     error_code  = CSMI_SUCCESS;	// The error code, should be of type
	                                             // csmi_cmd_err_t.
		// EARLY RETURN
		// Create a csm_api_object and sets its csmi cmd and the destroy function.
		create_csm_api_object(handle, expected_cmd, csmi_<api>_destroy);

		// Test the input to the API, expand this to test input contents.
		if (!input)
		{
			csmutil_logging(error, "The supplied input was null.");	
			
			// The error codes are listed in “csmi/include/csmi_type_common.h”.
			csm_api_object_errcode_set(*handle, CSMERR_INVALID_PARAM);
			csm_api_object_errmsg_set(*handle,
		         strdup(csm_get_string_from_enum(csmi_cmd_err_t, CSMERR_INVALID_PARAM))); 
		}
		
		// EARLY RETURN
	        // Serialize the input struct and then test the serialization.
		csm_serialize_struct(<input_type>, input, &buffer, &buffer_length);
		test_serialization(handle, buffer);

		// Execute the send receive command (this is blocking).
		error_code = csmi_sendrecv_cmd(*handle, expected_cmd, 
			buffer, buffer_length, &return_buffer, &return_buffer_len);
	    	
		// Based on the error code unpack the results or set the error code.
		if ( error_code == CSMI_SUCCESS )
		{
			if ( return_buffer && csm_deserialize_struct(<output_type>, output, 
			(const char *)return_buffer, return_buffer_len) == 0 )
			{
				// ATTENTION: This is key, the CSM API makes a promise that the
	                      // output of the API will be stored in the csm_api_object!
				csm_api_object_set_retdata(*handle, 1, *output);
			}
			else
			{
				csmutil_logging(error, "Deserialization failed");
	            		csm_api_object_errcode_set(*handle, CSMERR_MSG_UNPACK_ERROR);
	           		csm_api_object_errmsg_set(*handle,
	                			strdup(csm_get_string_from_enum(csmi_cmd_err_t, 
	                                        CSMERR_MSG_UNPACK_ERROR)));
	            		error_code = CSMERR_MSG_UNPACK_ERROR;		
			}	
		}
		else
		{
			csmutil_logging(error, "csmi_sendrecv_cmd failed: %d - %s",
	            		error_code, csm_api_object_errmsg_get(*handle));
		}

		// Free the buffers.
		if(return_buffer)free(return_buffer);
		free(buffer);

		END_TIMING( csmapi, trace, csm_api_object_traceid_get(*handle), expected_cmd, api )
		
		return error_code;
	}

	// This function should destroy any data stored in the csm_api_object by the API call.
	void csmi_<api>_destroy(csm_api_object *handle)
	{
	    csmi_api_internal *csmi_hdl;
	    <output_type> *output;

	     // free the CSMI dependent data
	     csmi_hdl = (csmi_api_internal *) handle->hdl;
	     if (csmi_hdl->cmd != expected_cmd)
	     {
	         csmutil_logging(error, "%s-%d: Unmatched CSMI cmd\n", __FILE__, __LINE__);
	         return;
	    }

	    // free the returned data specific to this csmi cmd
	    output = (<output_type> *) csmi_hdl->ret_cdata;
	     csm_free_struct_ptr( <output_type>, output);

	     csmutil_logging(info, "csmi_<api>_destroy called");
	}

:8: Optionally, the developer may implement command line interface to the 
    C API. For implementing an API please refer to existing API implementations.



Back-end
--------

The he part of the API that the user will not interact with directly. The back end will be invoked
by the :ref:`CSMDInfrastructure` after receiving user requests.

This diagram below shows where to find the appropriate files in the |git-repo| repository.

.. image:: https://user-images.githubusercontent.com/4662139/49670832-f4139400-fa33-11e8-8591-c53e50c79f05.png
   :height: 350px

When implementing a backend API the developer must determine several key details:

* Does the API handler access the database? How many times?
* What daemon will the API handler operate on? 
* Does the API need a privilege mode?
* Will the API perform a multicast?

These questions will drive the development process, which in the case of most database APIs 
is boiler plate as shown in the following sections.

Determining the Base Handler Class
**********************************

In the :ref:`CSMDInfrastructure` the back-end API is implemented as an API Handler. 
This **handler** may be considered a static object which maintains no volatile state. 
The state of API execution is managed by a context **object** initialized when a request is first 
received by a back-end handler.

CSM has defined several implementations of handler class to best facilitate the rapid creation 
of back-end handlers. Unless otherwise specified these handlers are located in 
``csmd/src/daemon/src/csmi_request_handler`` and handler implementations should be placed in 
the same directory. 

CSMIStatefulDB (csmi_stateful_db.h)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If an API needs to access the database, it is generally recommended to use this handler as a base 
class. This class provides four virtual functions: 

:CreatePayload: 
    Parses the incoming API request, then generates the SQL query.

:CreateByteArray: 
    Parses the response from the database, then generates the serialized response.

:RetrieveDataForPrivateCheck: 
    Generates a query to the database to check the user’s privilege level (optional).

:CompareDataForPrivateCheck: 
    Checks the results of the query in ``RetrieveDataForPrivateCheck``
    returning true or false based on the results (optional).

In the simplest Database APIs, the developer needs to only implement two functions: 
``CreatePayload`` and ``CreateByteArray``. In the case of privileged APIs, the 
``RetrieveDataForPrivateCheck`` and ``CompareDataForPrivateCheck`` must be implemented.

This handler actually represents a state machine consisting of three states which generalize the 
most commonly used database access path. If your application requires multiple database accesses or 
multicasts this state machine may be extended by overriding the constructor. 

.. graphviz::

    digraph G {
      DB_INIT -> DB_RECV_PRI [color="#993300" labelfontcolor="#993300" label="Privileged"];
      DB_INIT -> DB_RECV_DB;
      DB_RECV_PRI -> DB_RECV_DB;
      DB_RECV_DB -> DB_DONE;
    }  

To facilitate multiple database accesses in a single API call CSM has implemented 
``StatefulDBRecvSend``. ``StatefulDBRecvSend`` takes a static function as a template parameter 
which defines the processing logic for the SQL executed by ``CreatePayload``. The constructor for 
``StatefulDBRecvSend`` then takes an assortment of state transitions for the state machine 
which will depend on the state machine used for the API. 

An example of this API implementation style can be found in ``CSMIAllocationQuery.cc``. 
The pertinent section showing expansion of the state machine with the constructor is 
reproduced and annotated below:

.. code-block:: C++

	#define EXTRA_STATES 1 // There’s one additional state being used over the normal StatefulDB.

	// Note: CSM_CMD_allocation_query matches the version on the front-end.
	CSMIAllocationQuery::CSMIAllocationQuery(csm::daemon::HandlerOptions& options) :
	CSMIStatefulDB(CSM_CMD_allocation_query, options, 
	STATEFUL_DB_DONE + EXTRA_STATES) // Send the total number of states to super.
	 {
	     const uint32_t final_state = STATEFUL_DB_DONE + EXTRA_STATES;
	     uint32_t current_state = STATEFUL_DB_RECV_DB;
	     uint32_t next_state = current_state + 1;

	     SetState( current_state++,
	         new StatefulDBRecvSend<CreateResponsePayload>(
	             next_state++,    // Successful state.
	             final_state,     // Failure state.
	 	      final_state ) ); // Final state.
	 }
	#undef EXTRA_STATES

	bool CSMIAllocationQuery::CreateResponsePayload(
	  const std::vector<csm::db::DBTuple *>&tuples,
	  csm::db::DBReqContent **dbPayload,
	  csm::daemon::EventContextHandlerState_sptr ctx )
	{
	   // ….
	}

Multicast operations will follow a largely similar behavior, however they exceed the scope of this 
document, for more details refer to ``csmd/src/daemon/src/csmi_request_handler/csmi_mcast``.

CSMIStateful (csmi_stateful.h)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This handler should be used as a base class in handlers where no database operations are required 
(see `CSMIAllocationStepCGROUPDelete.h`_).  Generally, most API implementations will not use this 
as a base class. If an API is being implemented as *CSMIStateful* it is recommended to refer the 
source of `CSMIAllocationStepCGROUPDelete.h`_ and `CSMIAllocationStepCGROUPCreate.h`_.

.. _CSMIAllocationStepCGROUPDelete.h: https://github.com/IBM/CAST/blob/master/csmd/src/daemon/src/csmi_request_handler/CSMIAllocationStepCGROUPDelete.h

.. _CSMIAllocationStepCGROUPCreate.h: https://github.com/IBM/CAST/blob/master/csmd/src/daemon/src/csmi_request_handler/CSMIAllocationStepCGROUPCreate.h

Adding Handler to Compliation
*****************************

To add the handler to the compilation path for the daemon add it to the 
``csmd/src/daemon/src/CMakeLists.txt`` file’s **CSM_DAEMON_SRC** file GLOB.

Registering with a Daemon
**************************

After implementing the back-end API the user must then register the API with the daemon routing. 
Most APIs will only need to be registered on the Master Daemon, however, if the API performs 
multicasts it will need to be registered on the Agent and Aggregator Daemons as well. The routing 
tables are defined in ``csmd/src/daemon/src`` :

+------------------+------------------------------+
| Daemon           | Routing File                 |
+==================+==============================+
| Agent            | csm_event_routing_agent.cc   |
+------------------+------------------------------+
| Aggregator       | csm_event_routing_agg.cc     |
+------------------+------------------------------+
| Master           | csm_event_routing_master.cc  |
+------------------+------------------------------+
| Utility          | csm_event_routing_utility.cc |
+------------------+------------------------------+

Generally speaking registering a handler to a router is as simple as adding the following line to 
the RegisterHandlers function: *Register* < **Handler_Class** > **(CSM_CMD_<api>)** *;*


Return Codes
------------

As with all data types that will exist in both the C front-end and C++ back-end return codes are 
defined with an X-Macro solution. The return code X-Macro file can be located at: 
`csmi/include/csm_types/enum_defs/common/csmi_errors.def`_

To protect backwards compatibility this file is guarded by with versioning blocks, for details on 
how to add error codes please consult the README: `csmi/include/csm_types/enum_defs/README.md`_

The generated error codes may be included from the ``csmi/include/csmi_type_common.h`` header. 
Generally, the ``CSMI_SUCCESS`` error code should be used in cases of successful execution. Errors 
should be more granular to make error determination easier for users of the API, consult the list 
of errors before adding a new one to prevent duplicate error codes.

.. _csmi/include/csm_types/enum_defs/common/csmi_errors.def: https://github.com/IBM/CAST/blob/master/csmi/include/csm_types/enum_defs/common/csmi_errors.def

.. _csmi/include/csm_types/enum_defs/README.md: https://github.com/IBM/CAST/blob/master/csmi/include/csm_types/enum_defs/README.md


CSM API Wrappers
----------------

There exist two documented methodologies for wrapping a CSM API to reduce the barrier of usage for 
system administrators: python bindings and command line interfaces. Generally speaking python 
bindings are preferred, as they provide more flexibility to system administrators and end users. 

Command line interfaces are generally written in C and are used to expose basic functionality to an API.

Command Line Interfaces
***********************

Command line interfaces in CSM are generally written using native C and expose basic functionality 
to the API, generally simplifying inputs or control over the output. When properly compiled a 
native C command line interface will be placed in ``/csm/bin/`` relative to the root of the 
compiled output. Please consult `csmi/src/wm/cmd/CMakeLists.txt`_ for examples of compilation settings.

.. _csmi/src/wm/cmd/CMakeLists.txt: https://github.com/IBM/CAST/blob/master/csmi/src/wm/cmd/CMakeLists.txt

Naming
^^^^^^

The name of the CSM command line interface should be matched one to one to the name of the API, 
especially in cases where the command line interface simply exposes the function of the API with 
no special modifications. For example, the ``csm_allocation_create`` API is literally 
``csm_allocation_create`` on the command line. 

Parameters
^^^^^^^^^^

CSM command line interfaces must provide long options for all command line parameters. 
Short options are optional but preferred for more frequently used fields. A sample pairing of 
short and long options would be in the case of the help flag: ``-h, --help``.  i

The ``-h, --help`` and ``-v, --verbose`` flag pairings are reserved, always correspond to help 
and verbose. These flags should be supported in all CSM command line interfaces. 

All options should use the ``getopts`` utility, no options should be position dependent.

Good:

.. code-block:: none

	csm_command --node_name node1 --state "some string"
	csm_command --state "some string" –node_name node1

Bad: 

.. code-block:: none

	csm_command node1 --state "some string"

Output
^^^^^^

CSM command line requires that the YAML format is a supported output option. This is to facilitate 
command line parsers. In cases where YAML output is not ideal for command line readability the 
format may be changed as in the case of ``csm_node_query_state_history``.

In the following sample output the output is still considered valid YAML (note the open and close 
tokens). Data that is not YAML formatted will be commented out with the # character.

.. code-block:: none

	[root@c650f03p41 bin]# ./csm_node_query_state_history -n c650f03p41
	---
	node_name: c650f03p41
	#         history_time        |      state     |      alteration      | RAS_rec_id, RAS_msg_id 
	# ----------------------------+----------------+----------------------+------------------------
	#  2018-03-26 14:28:25.032879 | DISCOVERED     | CSM INVENTORY        | 
	#  2018-03-28 19:34:14.037409 | SOFT_FAILURE   | RAS EVENT            | 7, csm.status.down
	...


By default, YAML is not presented on the command line. It is supported through a flag.

.. code-block:: none

	GENERAL OPTIONS:
	[-h, --help]                  | Help.
	[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}
	[-Y, --YAML]                  | Set output to YAML. By default for this API, we have a custom output for ease of reading the long transaction history.

By setting the ``–Y`` flag, the command line will then display in YAML.

.. code-block:: none

	[root@c650f03p41 bin]# ./csm_node_query_state_history -n c650f03p41 -Y
	---
	Total_Records: 2
	Record_1:
	  history_time: 2018-03-26 14:28:25.032879
	  node_name:    c650f03p41
	  state:        DISCOVERED
	  alteration:   CSM INVENTORY
	  RAS_rec_id:   
	  RAS_msg_id:   
	Record_2:
	  history_time: 2018-03-28 19:34:14.037409
	  node_name:    c650f03p41
	  state:        SOFT_FAILURE
	  alteration:   RAS EVENT
	  RAS_rec_id:   7
	  RAS_msg_id:   csm.status.down
	...


Python Interfaces
*****************

CSM uses Boost.Python to generate the Python interfaces.  Struct bindings occur automatically when 
running the ``csmi/include/struct_generator/regenerate_headers.sh`` script. Each API type has its 
own file to which the struct bindings will be placed by the automated script and function bindings 
will be placed by the developer. 

The following documentation assumes the python bindings are being added to one of the following files:

+---------------------+-------------------------------------------+-------------------+
| API Type            | Python Binding File                       | Python Library    |
+=====================+===========================================+===================+
| Burst Buffer        | csmi/src/bb/src/csmi_bb_python.cc         | lib_csm_bb_py     |
+---------------------+-------------------------------------------+-------------------+
| Common              | csmi/src/common/src/csmi_python.cc        | lib_csm_py        |
+---------------------+-------------------------------------------+-------------------+
| Diagnostics         | csmi/src/diag/src/csmi_diag_python.cc     | lib_csm_diag_py   |
+---------------------+-------------------------------------------+-------------------+
| Inventory           | csmi/src/inv/src/csmi_inv_python.cc       | lib_csm_inv_py    |
+---------------------+-------------------------------------------+-------------------+
| Launch              | csmi/src/launch/src/csmi_launch_python.cc | lib_csm_launch_py |
+---------------------+-------------------------------------------+-------------------+
| RAS                 | csmi/src/ras/src/csmi_ras_python.cc       | lib_csm_ras_py    |
+---------------------+-------------------------------------------+-------------------+
| Workload Management | csmi/src/wm/src/csmi_wm_python.cc         | lib_csm_wm_py     |
+---------------------+-------------------------------------------+-------------------+

Function Binding
^^^^^^^^^^^^^^^^

Function binding with the Boost.Python library is boilerplate:

.. code-block:: C++

	tuple wrap_<api>(<input-struct> input)
	{
	    // Always sets the metadata. 
	    // Ensures that the python binding always matches what it was designed for.
	    input._metadata=CSM_VERSION_ID;

	    // Output objects.
	    csm_api_object * updated_handle;
	    <output-struct> * output= nullptr;  

	    // Run the API
	    int return_code = <api>( (csm_api_object**)&updated_handle, &input, &output);
	    
	    // A singleton is used to track CSM object handles.
	    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle); 

	    // Returned tuples should always follow the pattern: 
	    // <return code, handler id, output values (optional)>
	    return make_tuple(return_code, oid, *output);
	}

	BOOST_PYTHON_MODULE(lib_csm_<api-type>_py)
	{
		def("<api-no-csm>", wrap_<api>, CSM_GEN_DOCSTRING("docstring", ",<output_type>"));
	}

Python Binding Limitations
^^^^^^^^^^^^^^^^^^^^^^^^^^

As CSM was designed predominantly around its use of pointers, and is a C native API, 
certain operations using the python bindings are not currently Pythonic.

:1:	The output of the apis must be destroyed using ``csm.api_object_destroy(handler_id)``.

:2:	Array access/creation must be performed through get and set functions. Once an array is set 
    it is currently immutable from python.

These limitations are subject to change.

.. _struct README: https://github.com/IBM/CAST/blob/master/csmi/include/csm_types/struct_defs/README.md

