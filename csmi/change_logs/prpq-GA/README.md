Overview 
========

CSM has several changes that must be made in preparation to become a GA product. This document outlines the changes
that must be performed in APIs that currently have known users.


Table of Contents
=================

* [Table of Contents](#table-of-contents)
* [Tag Key](#tag-key)
* [Versioning](#versioning)
* [LSF](#lsf)
  * [csm_allocation_create](#csm_allocation_create)
  * [csm_allocation_delete](#csm_allocation_delete)
  * [csm_allocation_update_state](#csm_allocation_update_state)
  * [csm_node_resources_query_all](#csm_node_resources_query_all)
  * [csm_allocation_query_details](#csm_allocation_query_details)
  * [csm_db_consts.h](#csm_db_consts)
* [JSM](#JSM)
  * [csm_allocation_step_begin](#csm_allocation_step_begin)
  * [csm_allocation_step_end](#csm_allocation_step_end)
  * [csm_allocation_step_cgroup_create](#csm_allocation_step_cgroup_create)
  * [csm_allocation_step_cgroup_delete](#csm_allocation_step_cgroup_delete)
  * [csm_allocation_query](#csm_allocation_query)
* [Burst Buffer](#burst-buffer)
  * [csm_bb_cmd](#csm_bb_cmd)
  * [csm_bb_lv_create](#csm_bb_lv_create)
  * [csm_bb_lv_query](#csm_bb_lv_query)
  * [csm_bb_lv_update](#csm_bb_lv_update)
  * [csm_bb_vg_create](#csm_bb_vg_create)
  * [csm_bb_vg_delete](#csm_bb_vg_delete)
  * [csm_bb_vg_query](#csm_bb_vg_query)
* [Misc](#misc)
  * [CSM Versioning Sample](CSM_versioning_sample)
  * [csm_serialization_macros](#csm_serialization_macros)
  * [Boolean Data Type](boolean_data_type)
  * [Renamed files, functions and macros](Renamed_files,_functions_and_macros)
  
  
Tag Key
=======

 Tag          | Description
--------------|-------------------------------------------------
[New]         | The field is new
[Rename]      | The field is being renamed.
[Remove]      | The field is being removed.
[Combined]    | Two fields are consolidated into one.
[Type Change] | The type of a field is being changed.
[Value Change]| The value of a field is being changed.
[Notes]       | Code doesn't change, but nature of the field may.
[Question]    | General question to users/internal developers.
[Check]       | General question to internal developers.
[Branch]      | Change in branch.
[Complete]    | Change in `dev`.

Versioning
==========
TODO Move this to an official readme for CSM

A major issue for GA is the addition of some kind of versioning support for CSM. The transition from Beta 2 to PRPQ 
introduced integration issues, as the final layout, contents and data types for the structs and enumerated types were not
complete during integration.

Since the issue of version mismatches occurs when headers do not match the solution must be bound to the header files in some manner.
 
Compatibility Promise
----------------------
It is essential that CSM as an API establish compatibility guidelines for users of the API. 

The CSM versioning code schema is formulated as follows:

`[Release].[Cumulative Fix].[EFix]`

Using this schema a version code of 0.3.1 (hot fix for PRPQ) would represent release 0 (pre-GA), cumulative fix 3 (PRPQ) and efix 1.
The user can then make assumptions about their upgrade paths from these details.

### Release
Represents a collection of fixes that may resolve bugs, introduce new (major) features, and may perform changes to the public API interfaces.
This is considered a **MAJOR** version change, indicating a large degree of changes. Due to the
nature of the changes CSM makes no guarantee of compatibility across release levels.

### Cumulative Fix
Represents a collection of fixes that may resolve bugs, introduce new features, and changes to the public interface are non destructive.
This is considered a minor change, supporting backwards compatibility.

### EFix
Represents a minor fix that resolves bugs, introduces no new features and only changes public interfaces if a critical error is discovered (EXTREMELY RARE). Unless stated changes to this version should be backwards and forward compatible.

Proposal #1 : Versioned struct names
------------------------------------

Each struct would carry the version of that particular struct: `csm_allocation_v2_t`, `csm_allocation_v3_t`.
Every time the struct was updated the version number would be updated in turn to prevent a user from using an old header with a 
newer build of the API.

### PROS
 * It's clear which version of the API the struct is from.
 * Collisions will occur sooner in the development process.

### CONS
 * Refactors take more time for developers (internal and external).

### John's Notes
We could take this a step further and typedef the versioned struct with the unversioned struct's name and only have one such struct
available in the public headers, then have a redefine which expands to a version of the API call using the versioned struct.
Internally we would then have behavior for the current version of the struct plus some number of other versions, all of which are internally 
defined. Our frontend interface would look the way it currently does, but the library has the historic versions. 

Another major benefit is that the source code of all internal APIs would only need to be updated to address internal changes to the API.

```C++ 
 typedef csm_allocation_v2_t csm_allocation_t;
  
 #define csm_allocation_query(obj, allocation)\
   csm_allocation_query_v2(obj,allocation);
```

Proposal #2 : Rules for modifying structs and constructor functions
-------------------------------------------------------------------

The easiest change would be to enforce a behavior for updating structs in struct generators. 
All future enumerated type and struct modifications should be postpended to end of the respective data type (This should be the case regardless herein).
Users of the API would then allocate API structs using constructor functions CSM provides, ensuring that the struct always allocates the 
correct amount of data.

### PROS:
* If postpended output struct behavior would see no meaningful change by the user.
* Minimal book keeping in 



Proposal #3: Opaque Datatypes
------------------------------
This seems to be the best way to handle problems with forward and backwards compatibility, but it's a little late to implement in a meaningful way.

The core problem is that we have a pretty bad code smell in the form of [Inappropriate Intimacy](http://wiki.c2.com/?InappropriateIntimacy)

Proposal #4: Field Metadata ** PLANED IMPLEMENTATION **
---------------------------

Add a `_metadata` metadata field to the front of every struct. This field will be a <strike>byte</strike> `uint64_t`
and the user will need to initialize this value using the header provided field count. If using the supplied `init_struct` macro this is unnecessary.

This method would be combined with the rules in proposal #3, minimizing changes to internal APIs across versions.

Conformance to this standard would be handled by the struct generators.

### PROS:
* Users can allocate the structs as is appropriate for their use case.
* Serialization can ignore fields that haven't been initialized with minimal difficulty.
* Backend handlers don't require any changes to function properly.
* Frontend handlers require changes only when directly accessing new fields in the function body.
  * Frontend changes only need to occur when a change is made.
* Localizes major programming changes to input structs in early adoption

### CONS:
* Users now have an additional step to initialize a struct for the API.
* Adds a byte (depending on memory alignment) to the struct in memory and in the serialization.

### John's notes
This is my current candidate for how CSM should handle versoining.



LSF
===

csm_allocation_create 
---------------------
### return code changes

* Falures on the Compute daemon will now result in the API returning `CSMERR_MULTI_RESP_ERROR` unilaterally.

### struct changes 

#### csm_allocation_t | csm_allocation_create_output_t

* [Remove] **[Branch]** ***power_cap***
* [Remove] **[Branch]** ***power_shifting_ratio***
* [Type Change] **[Branch]** ***state***
  * `char*` => `csmi_state_t` 
  * The enumerated type is simpler to use/more appropriate.
* [Type Change]**[Branch]** ***job_type*** 
  * **LSF QUESTION** Do you populate this field?
  * An enumerated type will replace this field:
    ``` C
    typedef enum {
       CSM_BATCH=0,          ///< 0 - Denotes a batch job.
       CSM_INTERACTIVE=1,    ///< 1 - Denotes an interactive job.
       csmi_job_type_t_MAX=2 ///< 2 - Bounding Value
    } csmi_job_type_t;
    ```
* [Type Change]**[Branch]** ***type*** 
  * An enumerated type will replace this field:
    ``` C
    typedef enum {
       CSM_USER_MANAGED=0,          ///< 0 - Denotes a user managed allocation.
       CSM_JSM=1,                   ///< 1 - Denotes an allocation managed by JSM.
       CSM_JSM_CGROUP_STEP=2,       ///< 2 - Denotes an allocation managed by JSM with step cgroups.
       CSM_DIAGNOSTICS=3,           ///< 3 - Denotes a diagnostic allocation run.
       csmi_allocation_type_t_MAX=4 ///< 4 - Bounding Value
    } csmi_allocation_type_t;
    ```
* [Type Change] [Rename] **[Branch]** ***cgroup_type***
  * `cgroup_type` => `isolated_cores`
  * `csmi_cgroup_type_t` => `int32_t`
  * This is also changed in the database.
  * With this new change the allocation cgroup will always be created, however the system cgroup will only be created if this value is greater than zero and less than or equal to 4.
* [Type Change] ***shared***
  * See [csm_bool](boolean_data_type) for details.
* [Rename] **[Branch]** ***num_memory*** => ***projected_memory***
* [Remove] **[Branch]** ***eligible_time***
* [Remove] **[Branch]** ***wct_reservation***
* [Remove] **[Branch]** ***reservation***
* [Rename] **[Branch]** ***ssd_size*** => ***ssd_min*** and ***ssd_max***
* [Rename] ***file_system_name*** => ***ssd_file_system_name***

#### csmi_allocation_history_t

* [Remove] **[Branch]** ***end_time***
* [Remove] **[Branch]** **energy_consumed**
* [Remove] **[Branch]** **power_cap_hit**
  
### enum changes

#### csmi_cgroup_types

The name of this enumerated type is going to be renamed to **csmi_cgroup_types_t**.

#### csmi_state_t

* [New] **[Branch]** Added 4 new transition states:
  * `CSM_TO_RUNNING`
  * `CSM_TO_STAGING_OUT`
  * `CSM_TO_COMPLETE`
  * `CSM_TO_FAILED`
  
* [Rename] **[Branch]** Every enum value is getting prepended with `CSM_` as the old names are too generic.

    Old Name   | New Name
    -----------|------------------
    STAGING_IN | CSM_STAGING_IN
    RUNNING    | CSM_RUNNING
    STAGING_OUT| CSM_STAGING_OUT
    COMPLETE   | CSM_COMPLETE
    FAILED     | CSM_FAILED
    
* [Value Change] **[Branch]** Spaces are being replaced with `-` characters for the enum array.

    Enum        | Old Value   | New Value   
    ------------|-------------|--------------
    STAGING_IN  | staging in  | staging-in  
    STAGING_OUT | staging out | staging-out 


csm_allocation_delete
---------------------
### struct_changes ###

#### csm_allocation_delete_input_t ####
* [New] **[Branch]** ***exit_status*** 
  * The exit status of the allocation, initializes to 0 in the command line.

csm_allocation_update_state
---------------------------
No changes will be made.

csm_node_resources_query_all
----------------------------

### struct changes

#### csm_node_resources_query_all_input_t

No changes will be made.

#### csm_node_resources_query_all_output_t

* [Type Change] **[Branch]** ***results_count***
  * `int` => `uint32_t`
  
#### csmi_node_resources_record_t

* [New] **[Branch]** Several new fields are to be added after January:

    Field             | Type    | Description 
    ------------------|---------|-------------------
    vg_available_size | int64_t | Available size remaining (in bytes) in the volume group on this node.
    vg_total_size     | int64_t | Total size (in bytes) of the volume group on this node.
    vg_update_time    | char*   | Last time all the VG related information was updated.

* [Remove] **[Branch]**  ***node_current_power_cap***
* [Remove] **[Branch]**  ***node_current_power_shifting_ratio***
* [Remove] **[Branch]**  ***node_hard_power_cap***
* [Rename] **[Branch]**  ***node_last_updated_time*** => ***node_update_time***

* [Type Change] **[Branch]** ***node_type*** 
  ``` C 
  typedef enum {
     CSM_NODE_MANAGEMENT=0,       ///< 0 - Denotes a management node.
     CSM_NODE_SERVICE=1,          ///< 1 - Denotes a service node.
     CSM_NODE_LOGIN=2,            ///< 2 - Denotes a login node.
     CSM_NODE_WORKLOAD_MANAGER=3, ///< 3 - Denotes a workload manager node.
     CSM_NODE_LAUNCH=4,           ///< 4 - Denotes a launch node.
     CSM_NODE_COMPUTE=5,          ///< 5 - Denotes a compute node.
     CSM_NODE_UTILITY=6,          ///< 6 - Denotes a utility node.
     CSM_NODE_AGGREGATOR=7,       ///< 7 - Denotes an aggregator node.
     csmi_node_type_t_MAX=8       ///< 8 - Bounding Value    
  } csmi_node_type_t;
  ```
  
* [Type Change] **[Branch]** ***node_state*** 
 ``` C 
 typedef enum {
    CSM_NODE_NO_DEF=0,             ///< 0 - Node has no specified state.
    CSM_NODE_DISCOVERED=1,         ///< 1 - Node was discovered by CSM's inventory.
    CSM_NODE_IN_SERVICE=2,         ///< 2 - Node has been marked as in service.
    CSM_NODE_OUT_OF_SERVICE=3,     ///< 3 - Node has been marked as out of service.
    CSM_NODE_SYS_ADMIN_RESERVED=4, ///< 4 - Node has been marked as reserved by the sys admin.
    CSM_NODE_SOFT_FAILURE=5,       ///< 5 - Node has been placed into a soft failure.
    csmi_node_state_t_MAX=6        ///< 6 - Bounding Value
 } csmi_node_state_t;
  ```
  
* [Type Change] ***node_ready***
  * See [csm_bool](boolean_data_type) for details.
  
#### csmi_ssd_resources_record_t

* [Remove] **[Branch]**  ***available_size***
* [Remove] **[Branch]**  ***size***
* [Remove] **[Branch]**  ***total_size***
* [Rename] **[Branch]**  ***last_updated_time*** => ***update_time***


csm_allocation_query_details
----------------------------
### struct change  

#### csm_allocation_query_details_output_t

Please consult the `csm_allocation_t` section above, and `csmi_allocation_details_t` section below.

#### csmi_allocation_details_t

* [Check] [Remove] **[Branch]** ***maptags***
* [Check] [Remove] **[Branch]** ***num_maptags***
  
#### csmi_allocation_accounting_t

* [Combined] **[Branch]** Combining begin/end to make usage fields.

    Field 1          | Field 2          | New Field
    -----------------|------------------|---------------
    ib_rx_begin      | ib_rx_end        | ib_rx
    ib_tx_begin      | ib_tx_end        | ib_tx
    gpfs_read_begin  | gpfs_read_begin  | gpfs_read
    gpfs_write_begin | gpfs_write_begin | gpfs_write
    
* [New] **[Branch]** Several new fields are to be added for GA to increase inventory precision:

    Field     | Type    | Description 
    ----------|---------|-------------------
    ssd_read  | int64_t | Bytes read from the ssd during the job execution.
    ssd_write | int64_t | Bytes written to the ssd during the job execution.
    gpu_usage | int64_t | Usage of GPU during the job execution (may be renamed).
    
* [Note] **[Branch]** Total counter fields will be set to 0 if the job is active (start time is not totally useful).
* [Note] **[Branch]** This replaces the fields in the database as well, with the stored procedures computing the differences.
    * Negative values will now indicate the begin reading if the end reading wasn't written to the database.
* <strike>[Check] For active allocations what should these fields store? </strike>
    * <strike>Probably 0, because it's not used.</strike>
    * <strike>Do we want to see begin?</strike>
  
#### csmi_allocation_step_list_t

No changes will be made.

#### csm_allocation_query_details_input_t

No changes will be made.


csm_db_consts
----------------

TODO

JSM 
===

csm_allocation_step_begin
-------------------------

### struct changes

#### csmi_allocation_step_t | csm_allocation_step_begin_input_t

* [Question] ***state***
  * What states are supported? Should an enumerated type be used?
* [Check] [Remove] ***system_flags***
  * This might not be supported, should this be removed.
  * Being removed.
* [Check] ***num_memory*** 
  * This field needs a better description.
  * ***projected_memory*** The projected memory usage for a step.
* [Rename] [Type_Change] ***state*** => ***status***
```C
 typedef enum {
    CSM_STEP_COMPLETED=0,      ///< 0 - The step has been completed.
    CSM_STEP_RUNNING=1,        ///< 1 - The step is currently running.
    CSM_STEP_KILLED=2,         ///< 2 - The step has been killed.
    csmi_step_status_t_MAX=3   ///< 3 - Bounding Value
 } csmi_node_state_t;
  ```
 * [Remove] ***network_bandwidth***
 * [Remove] ***level_gpu_usage***

 
 
#### csmi_allocation_step_history_t
  
 * [Remove] ***history_time***
 * [Type_Change] ***max_memory*** `int64_t`
 * [Rename] ***err_text*** => ***error_message***
 * [Rename] ***total_num_threads*** => ***omp_thread_limit***


csm_allocation_step_end
-----------------------

### struct changes

#### csm_allocation_step_end_input_t 

* [New] ***status***
```C
 typedef enum {
    CSM_STEP_COMPLETED=0,      ///< 0 - The step has been completed.
    CSM_STEP_RUNNING=1,        ///< 1 - The step is currently running.
    CSM_STEP_KILLED=2,         ///< 2 - The step has been killed.
    csmi_step_status_t_MAX=3   ///< 3 - Bounding Value
 } csmi_node_state_t;
  ```

Please consult the `csmi_allocation_step_history_t` section above.


csm_allocation_step_cgroup_create
---------------------------------

### struct changes

#### csm_allocation_step_cgroup_create_input_t 

Please consult the `csmi_cgroup_t` section below.

#### csmi_cgroup_t

* [Question] Should CSM consolidate **params** and **values** into array of structs?
  * Whether this change occurs is at the discretion of JSM.
  * The new struct would be:
    ```C
        typedef struct {
            char* param;
            char* value;
        } csmi_cgroup_parameter_t;
    ```
  * The `csmi_cgroup_t` struct would then be:
    ```C
    typedef struct {
        uint32_t num_params;
        csmi_cgroup_controller_t type;
        csmi_cgroup_parameter_t** params;
    } csmi_cgroup_t;
    ```
## enum changes

### csmi_cgroup_controllers_t

* [Rename] **[Branch]** 
  * `csmi_cgroup_controllers_t` => `csmi_cgroup_controller_t`
  
csm_allocation_step_cgroup_delete
---------------------------------

### struct changes

#### csm_allocation_step_cgroup_delete_input_t

No changes will be made.


csm_allocation_query
--------------------

### struct changes

#### csm_allocation_query_input_t

No changes will be made.

#### csm_allocation_t | csm_allocation_query_input_t

Please consult the `csm_allocation_t` section above.

### enum changes

Please consult the `csm_allocation_create` `enum changes` section above.



Burst Buffer
============

csm_bb_cmd
----------

### struct changes

#### csm_bb_cmd_input_t

* [Type Change] **[Branch]** ***node_names_count***
  * `int` => `uint32_t`

#### csm_bb_cmd_ouput_t

No changes will be made.


csm_bb_lv_create
----------------

### struct changes

#### csm_bb_lv_create_input_t

* [Question] **[Answered]** ***state*** 
  * <strike>Should this be an enumerated type? Since it's just a char, maybe not?</strike> It will remain a char, per discussion.


csm_bb_lv_query
---------------

### struct changes

#### csm_bb_lv_query_input_t 
* [Type Change] **[Branch]** ***allocation_ids_count***
  * `int32_t` => `uint32_t`
* [Type Change] **[Branch]** ***logical_volume_names_count***
  * `int32_t` => `uint32_t`
* [Type Change] **[Branch]** ***node_names_count***
  * `int32_t` => `uint32_t`

#### csm_bb_lv_query_output_t

* [Type Change] **[Branch]** ***results_count***
  * `int32_t` => `uint32_t`
  
#### csmi_lv_record_t
 
No changes will be made.


csm_bb_lv_update
----------------

### struct changes

#### csm_bb_lv_update_input_t

* `logical_volume_name` and `node_name` should probably be `const char *` [Tom] 
  * Not sure if the the serializer supports this, will need to look into it. [John]

csm_bb_vg_create
----------------

### struct changes

#### csm_bb_vg_create_input_t

* [Type Change] **[Branch]** ***ssd_info_count***
  * `int32_t` => `uint32_t`

* [New] **[Branch]** New fields are to be added after January:

    Field     | Type     | Description 
    ----------|----------|-------------------
    scheduler | csm_bool | True or false value. Tells CSM whether or not this is the volume group for the scheduler. Defaults to false. (char: 0 = false, !0 = true)???

#### csmi_bb_vg_ssd_info_t

* `ssd_serial_number` should probably be `const char *` [Tom] 
  * Not sure if the the serializer supports this, will need to look into it. [John]


csm_bb_vg_delete
----------------

### struct changes

#### csm_bb_vg_delete_input_t

* [Type Change] **[Branch]** ***vg_names_count***
  * `int32_t` => `uint32_t`
  
#### csm_bb_vg_delete_output_t
* [Type Change] **[Branch]** ***failure_count***
  * `int32_t` => `uint32_t`


csm_bb_vg_query
---------------

### struct changes

#### csm_bb_vg_query_input_t 

* [Type Change] **[Branch]** ***vg_names_count***
  * `int32_t` => `uint32_t`
* [Type Change] **[Branch]** ***node_names_count***
  * `int32_t` => `uint32_t`
  
#### csm_bb_vg_query_output_t

* [Type Change] **[Branch]** ***results_count***
  * `int` => `uint32_t`
  
#### csmi_vg_record_t

No changes will be made.

Misc
====
CSM Versioning Sample
---------------------
Example: `bluecoral/csmi/src/wm/tests/test_csmi_sample.c`
A sample using the csm versioning macro is shown in the above file, with the pertinent section reproduced below:

 ```C
 csm_init_lib(); ///< Initializes the connection to the Backend.

 /// ========================================================================================
 /// =                                csm_allocation_create                                 =
 /// ========================================================================================
 /// Example of an API invocation that takes a struct malloc'd by the user as input and reuses
 /// the struct for output. The user is repsonsible for struct destruction in this case.
 /// 'csm_api_object_destroy' must be invoked to clean up the csm_obj.

 csmi_allocation_t *allocation; ///< A Struct for holding an allocation. csmi/include/csm_apis.h
 csm_api_object   *csm_obj;    ///< An object for holding error codes and returned data.

 /*
  * The following macro is defined in csmi/include/csm_api_helper_macros.h.
  * It will allocate the struct on the heap and then set the contents of the
  * struct to the default values. These defaults are null pointers for
  * components of pointer types (ie strings).
  * Using this macro is not required, but recommended if using the free macro.
  */
 csm_init_struct_ptr(csmi_allocation_t, allocation);

 /*
  * Initialize the _metadata field to be the API version code
  * in /include/csm_api_version.h. If the struct is initialized with the above
  * csm_init_struct_ptr this step is automatically performed.
  */
 csm_init_struct_versioning(allocation);
 ```

csm_serialization_macros
-------------------------
* [Rename] **[Branch]**  get_enum_string => get_enum_from_string
* [Rename] **[Branch]**  get_string_enum => get_string_from_enum

Boolean Data Type
---------------------------
CSM now has a `csm_bool` datatype which stores the truth of a field.
CSM recommends setting this type with the `CSM_TRUE` and `CSM_FALSE` constants.
If an API has an optional boolean field `CSM_UNDEF_BOOL` should be provided
to ignore the field.



Renamed files, functions and macros
-----------------------------------
Refactored the includes a bit, if using an API the user now only needs to include the header containing the API function
and all of the common csm api structs, macros and constants will be included.
Refactored the naming to reduce risk of namespace pollution.

### File Renames
* `csm_serialization_macros.h` => `csm_api_macros.h`
* `csm_serialization_macros.c` => `csm_api_macros.c`
* `csm_db_consts.h` => `csm_api_consts.h`
* `csm_api_handler.h` => `csm_api_common.h`
* `csm_init_term.h` => `csm_api_common.h`
* `csm_init_term.c` => `csm_api_common.c`

### Macro Renames
* `str_to_int32` => ` csm_str_to_int32`
* `str_to_int64` => ` csm_str_to_int64`
* `str_to_double` => ` csm_str_to_double`
* `str_to_char` => ` csm_str_to_char`
* `csm_stringArrayContainer_trim` => ` csm_trim_array_string`
* `optarg_test` => ` csm_optarg_test`
* `FUNCT_CAT` => ` CSM_FUNCT_CAT`
* `enum_from_string` => ` csm_enum_from_string`
* `serialize_str_array` => ` csm_serialize_str_array`
* `deserialize_str_array` => ` csm_deserialize_str_array`
* `primative_serializer` => ` csm_primative_serializer`
* `primative_deserializer` => ` csm_primative_deserializer`
* `get_enum_from_string` => ` csm_get_enum_from_string`
* `get_string_from_enum` => ` csm_get_string_from_enum`
* `get_enum_bit_flag` => ` csm_get_enum_bit_flag`
* `enum_max` => ` csm_enum_max`
* `enum_bit_count` => ` csm_enum_bit_count`
* `init_struct` => ` csm_init_struct`
* `free_struct` => ` csm_free_struct`
* `serialize_struct` => ` csm_serialize_struct`
* `deserialize_struct` => ` csm_deserialize_struct`

### Constant Renames
* `STATUS_MAX` => ` CSM_STATUS_MAX`
* `NODE_NAME_MAX` => ` CSM_NODE_NAME_MAX`
* `STATE_MAX` => ` CSM_STATE_MAX`
* `TYPE_MAX` => ` CSM_TYPE_MAX`
* `MACHINE_MODEL_MAX` => ` CSM_MACHINE_MODEL_MAX`
* `SERIAL_NUMBER_MAX` => ` CSM_SERIAL_NUMBER_MAX`
* `DISCOVERY_TIME_MAX` => ` CSM_DISCOVERY_TIME_MAX`
* `KERNEL_RELEASE_MAX` => ` CSM_KERNEL_RELEASE_MAX`
* `KERNEL_VERSION_MAX` => ` CSM_KERNEL_VERSION_MAX`
* `OS_IMAGE_NAME_MAX` => ` CSM_OS_IMAGE_NAME_MAX`
* `OS_IMAGE_UUID_MAX` => ` CSM_OS_IMAGE_UUID_MAX`
* `GPU_MAX_DEVICES` => ` CSM_GPU_MAX_DEVICES`
* `GPU_DEVICE_NAME_MAX` => ` CSM_GPU_DEVICE_NAME_MAX`
* `GPU_PCI_BUS_ID_MAX` => ` CSM_GPU_PCI_BUS_ID_MAX`
* `GPU_SERIAL_NUMBER_MAX` => ` CSM_GPU_SERIAL_NUMBER_MAX`
* `GPU_UUID_MAX` => ` CSM_GPU_UUID_MAX`
* `GPU_VBIOS_MAX` => ` CSM_GPU_VBIOS_MAX`
* `GPU_INFOROM_IMAGE_VERSION_MAX` => ` CSM_GPU_INFOROM_IMAGE_VERSION_MAX`
* `GPU_HBM_MEMORY_SIZE_MAX` => ` CSM_GPU_HBM_MEMORY_SIZE_MAX`
* `HCA_MAX_DEVICES` => ` CSM_HCA_MAX_DEVICES`
* `HCA_SERIAL_NUMBER_MAX` => ` CSM_HCA_SERIAL_NUMBER_MAX`
* `HCA_DEVICE_NAME_MAX` => ` CSM_HCA_DEVICE_NAME_MAX`
* `HCA_PCI_BUS_ID_MAX` => ` CSM_HCA_PCI_BUS_ID_MAX`
* `HCA_GUID_MAX` => ` CSM_HCA_GUID_MAX`
* `HCA_PART_NUMBER_MAX` => ` CSM_HCA_PART_NUMBER_MAX`
* `HCA_FW_VER_MAX` => ` CSM_HCA_FW_VER_MAX`
* `HCA_HW_REV_MAX` => ` CSM_HCA_HW_REV_MAX`
* `HCA_BOARD_ID_MAX` => ` CSM_HCA_BOARD_ID_MAX`
* `HCA_SWITCH_GUID_MAX` => ` CSM_HCA_SWITCH_GUID_MAX`
* `SSD_MAX_DEVICES` => ` CSM_SSD_MAX_DEVICES`
* `SSD_SERIAL_NUMBER_MAX` => ` CSM_SSD_SERIAL_NUMBER_MAX`
* `SSD_DEVICE_NAME_MAX` => ` CSM_SSD_DEVICE_NAME_MAX`
* `SSD_PCI_BUS_ID_MAX` => ` CSM_SSD_PCI_BUS_ID_MAX`
* `SSD_FW_VER_MAX` => ` CSM_SSD_FW_VER_MAX`

