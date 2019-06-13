Change Log
==========

.. contents::
   :local:

1.6.0
-----

The following document has been automatically generated to act as a change log for CSM version 1.6.0.

Enum Types
~~~~~~~~~~

Struct Types
~~~~~~~~~~~~

Workload Management
^^^^^^^^^^^^^^^^^^^

csmi_allocation_t
#################

**Added 1**

  * csm_bool core_blink

csmi_allocation_mcast_context_t
###############################

**Added 1**

  * csm_bool core_blink

csmi_allocation_mcast_payload_request_t
#######################################

**Added 1**

  * csm_bool core_blink


Burst Buffer 
^^^^^^^^^^^^

csm_bb_lv_delete_input_t
########################

**New Data Type**

  * int64_t allocation_id 
  * int64_t num_bytes_read 
  * int64_t num_bytes_written 
  * char* logical_volume_name 
  * char* node_name 
  * int64_t num_reads 
  * int64_t num_writes


Inventory 
^^^^^^^^^

csmi_switch_inventory_record_t
##############################

**New Data Type**

  * char* name 
  * char* host_system_guid 
  * char* discovery_time 
  * char* collection_time 
  * char* comment 
  * char* description 
  * char* device_name 
  * char* device_type 
  * char* hw_version 
  * int32_t max_ib_ports 
  * int32_t module_index 
  * int32_t number_of_chips 
  * char* path 
  * char* serial_number 
  * char* severity 
  * char* status 
  * char* type 
  * char* fw_version

csm_ib_cable_query_input_t
##########################

**New Data Type**

  * int32_t limit 
  * int32_t offset 
  * uint32_t serial_numbers_count 
  * char** serial_numbers 
  * uint32_t comments_count 
  * char** comments 
  * uint32_t guids_count 
  * char** guids 
  * uint32_t identifiers_count 
  * char** identifiers 
  * uint32_t lengths_count 
  * char** lengths 
  * uint32_t names_count 
  * char** names 
  * uint32_t part_numbers_count 
  * char** part_numbers 
  * uint32_t ports_count 
  * char** ports 
  * uint32_t revisions_count 
  * char** revisions 
  * uint32_t severities_count 
  * char** severities 
  * uint32_t types_count 
  * char** types 
  * uint32_t widths_count 
  * char** widths 
  * char order_by



1.4.0
-----
The following document has been automatically generated to act as a change log for CSM version 1.4.0.

Enum Types
~~~~~~~~~~

Workload Management 
^^^^^^^^^^^^^^^^^^^

csmi_allocation_type_t
######################

**Added 1**

  * CSM_CGROUP_STEP=4

Common 
^^^^^^

csmi_cmd_err_t
##############

**Added 10**
  * CSMERR_ALLOC_INVALID_NODES=46
  * CSMERR_ALLOC_OCCUPIED_NODES=47
  * CSMERR_ALLOC_UNAVAIL_NODES=48
  * CSMERR_ALLOC_BAD_FLAGS=49
  * CSMERR_ALLOC_MISSING=50
  * CSMERR_EPILOG_EPILOG_COLLISION=51
  * CSMERR_EPILOG_PROLOG_COLLISION=52
  * CSMERR_PROLOG_EPILOG_COLLISION=53
  * CSMERR_PROLOG_PROLOG_COLLISION=54
  * CSMERR_SOFT_FAIL_RECOVERY_AGENT=55

csmi_node_state_t
#################

**New Data Type**
  * CSM_NODE_NO_DEF=0
  * CSM_NODE_DISCOVERED=1
  * CSM_NODE_IN_SERVICE=2
  * CSM_NODE_OUT_OF_SERVICE=3
  * CSM_NODE_SYS_ADMIN_RESERVED=4
  * CSM_NODE_SOFT_FAILURE=5
  * CSM_NODE_MAINTENANCE=6
  * CSM_NODE_DATABASE_NULL=7
  * CSM_NODE_HARD_FAILURE=8


Struct Types
~~~~~~~~~~~~

Workload Management 
^^^^^^^^^^^^^^^^^^^

csmi_allocation_gpu_metrics_t
#############################

**New Data Type**
  * int64_t num_gpus 
  * int32_t* gpu_id 
  * int64_t* gpu_usage 
  * int64_t* max_gpu_memory 
  * int64_t num_cpus 
  * int64_t* cpu_usage

csmi_allocation_mcast_context_t
###############################

**New Data Type**
  * int64_t allocation_id 
  * int64_t primary_job_id 
  * int32_t num_processors 
  * int32_t num_gpus 
  * int32_t projected_memory 
  * int32_t secondary_job_id 
  * int32_t isolated_cores 
  * uint32_t num_nodes 
  * csmi_state_t state 
  * csmi_allocation_type_t type 
  * int64_t* ib_rx 
  * int64_t* ib_tx 
  * int64_t* gpfs_read 
  * int64_t* gpfs_write 
  * int64_t* energy 
  * int64_t* gpu_usage 
  * int64_t* cpu_usage 
  * int64_t* memory_max 
  * int64_t* power_cap_hit 
  * int32_t* power_cap 
  * int32_t* ps_ratio 
  * csm_bool shared 
  * char save_allocation 
  * char** compute_nodes 
  * char* user_flags 
  * char* system_flags 
  * char* user_name 
  * int64_t* gpu_energy 
  * char* timestamp 
  * csmi_state_t start_state 
  * int64_t runtime 
  * csmi_allocation_gpu_metrics_t** gpu_metrics

csmi_allocation_mcast_payload_request_t
#######################################

**Added 1**
  * int64_t runtime

csmi_allocation_mcast_payload_response_t
########################################

**New Data Type**

  * int64_t energy 
  * int64_t pc_hit 
  * int64_t gpu_usage 
  * int64_t ib_rx 
  * int64_t ib_tx 
  * int64_t gpfs_read 
  * int64_t gpfs_write 
  * int64_t cpu_usage 
  * int64_t memory_max 
  * int32_t power_cap 
  * int32_t ps_ratio 
  * char create 
  * char* hostname 
  * int64_t gpu_energy 
  * csmi_cmd_err_t error_code 
  * char* error_message 
  * csmi_allocation_gpu_metrics_t* gpu_metrics

csmi_jsrun_cmd_payload_t
########################

**Added 4**

  * uint32_t num_nodes 
  * char** compute_nodes 
  * char* launch_node 
  * csmi_allocation_type_t type

csmi_soft_failure_recovery_payload_t
####################################

**New Data Type**

  * char* hostname 
  * csmi_cmd_err_t error_code 
  * char* error_message

csm_soft_failure_recovery_node_t
################################

**New Data Type**

  * int errcode 
  * char* errmsg 
  * char* source

csm_soft_failure_recovery_input_t
#################################

**New Data Type**

  * uint32_t retry_count

csm_soft_failure_recovery_output_t
##################################

**New Data Type**

  * uint32_t error_count 
  * csm_soft_failure_recovery_node_t** node_errors


Inventory 
^^^^^^^^^

csm_ib_cable_inventory_collection_output_t
##########################################

**New Data Type**

  * int32_t insert_count 
  * int32_t update_count 
  * int32_t delete_count

csm_switch_attributes_query_input_t
###################################

**New Data Type**

  * int32_t limit 
  * int32_t offset 
  * uint32_t switch_names_count 
  * char* state 
  * char** switch_names 
  * char* serial_number 
  * char order_by 
  * uint32_t roles_count 
  * char** roles

csm_switch_inventory_collection_output_t
########################################

**New Data Type**

  * char TBD 
  * int32_t insert_count 
  * int32_t update_count 
  * int32_t delete_count 
  * int32_t delete_module_count

csm_switch_children_inventory_collection_output_t
#################################################

**New Data Type**

  * int32_t insert_count 
  * int32_t update_count 
  * int32_t delete_count


Common 
^^^^^^

csm_node_error_t
################

**New Data Type**

  * int errcode 
  * char* errmsg 
  * char* source

csmi_err_t
##########

**New Data Type**

  * int errcode 
  * char* errmsg 
  * uint32_t error_count 
  * csm_node_error_t** node_errors

