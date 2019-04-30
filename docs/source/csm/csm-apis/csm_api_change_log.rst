Change Log
==========

.. contents::
   :local:


1.4.0
-----
The following document has been automatically generated to act as a change log for CSM version 1.4.0.

Enum Types
__________

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
____________

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

