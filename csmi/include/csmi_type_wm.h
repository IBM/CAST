/*================================================================================
   
    csmi/include/csmi_type_wm.h

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
/** @file csmi_type_wm.h
 * @brief A collection of structs for @ref wm_apis.
 */
#ifndef _CSMI_WM_TYPES_H_
#define _CSMI_WM_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

// Begin special_preprocess directives
// Used for node_resources
// Include the database constants for the CSM_STATE_MAX.
#include <sys/types.h>
#include "csm_api_common.h"

/** @addtogroup  wm_apis 
 * @{
 */

// End special_preprocess directives

/** defgroup csmi_state_t csmi_state_t
 * @{
 */
/**
  * @brief The running state of a csm allocation.
 * @todo Post-PRPQ: prepend CSM to enum names.
  */
typedef enum {
   CSM_STAGING_IN=0, ///< 0 - The allocation is staging in.
   CSM_TO_RUNNING=1, ///< 1 - The allocation is transitioning to running.
   CSM_RUNNING=2, ///< 2 - The allocation is running.
   CSM_TO_STAGING_OUT=3, ///< 3 - The allocation is transitioning to staging out.
   CSM_STAGING_OUT=4, ///< 4 - The allocation is staging in.
   CSM_TO_COMPLETE=5, ///< 5 - The allocation is transitioning to complete.
   CSM_COMPLETE=6, ///< 6 - The allocation is complete.
   CSM_TO_FAILED=7, ///< 7 - The allocation is transitioning to failed.
   CSM_FAILED=8, ///< 8 - The allocation is failed.
   CSM_DELETING=9, ///< 9 - The allocation is being deleted, no mcast required.
   CSM_DELETING_MCAST=10, ///< 10 - The allocation is being deleted, multicast required. 
   CSM_RUNNING_FAILED=11, ///< 11 - The allocation failed a running transition.
   CSM_STAGING_OUT_FAILED=12, ///< 12 - The allocation failed a staging out transition.
   csmi_state_t_MAX=13 ///< 13 - Bounding Value
} csmi_state_t;



/**
 * @brief Maps enum csmi_state_t value to string.
 */
extern const char* csmi_state_t_strs [];
/** @}*/


/** defgroup csmi_cgroup_controller_t csmi_cgroup_controller_t
 * @{
 */
/**
  * @brief The cgroup controllers supported by csm.
 * Used in APIs that interact with cgroups.
  */
typedef enum {
   CG_CPUSET=0, ///< 0 - Confines processes to processor and memory node subsets.
   CG_MEMORY=1, ///< 1 - Used to set limits on memory for tasks.
   CG_DEVICES=2, ///< 2 - Allows or denies access to a device for tasks.
   CG_CPUACCT=3, ///< 3 - Accounting information for the CPU.
   csmi_cgroup_controller_t_MAX=4 ///< 4 - Bounding Value
} csmi_cgroup_controller_t;



/**
 * @brief Maps enum csmi_cgroup_controller_t value to string.
 */
extern const char* csmi_cgroup_controller_t_strs [];
/** @}*/


/** defgroup csmi_allocation_type_t csmi_allocation_type_t
 * @{
 */
/**
  * @brief The different types of allocations supported by CSM, for use in allocation creation.
 *
 * The enumerated string for an enum is stored in the database.
  */
typedef enum {
   CSM_USER_MANAGED=0, ///< 0 - Denotes a user managed allocation. 
   CSM_JSM=1, ///< 1 - Denotes an allocation managed by JSM.
   CSM_JSM_CGROUP_STEP=2, ///< 2 - Denotes an allocation managed by JSM with step cgroups. 
   CSM_DIAGNOSTICS=3, ///< 3 - Denotes a diagnostic allocation run.
   CSM_CGROUP_STEP=4, ///< 4 - Denotes an allocation with step cgroups.
   csmi_allocation_type_t_MAX=5 ///< 5 - Bounding Value
} csmi_allocation_type_t;



/**
 * @brief Maps enum csmi_allocation_type_t value to string.
 */
extern const char* csmi_allocation_type_t_strs [];
/** @}*/


/** defgroup csmi_job_type_t csmi_job_type_t
 * @{
 */
/**
  * @brief The different types of jobs supported by CSM, for use in allocation creation.
 *
 * The enumerated string for an enum is stored in the database.
  */
typedef enum {
   CSM_BATCH=0, ///< 0 - Denotes a batch job.
   CSM_INTERACTIVE=1, ///< 1 - Denotes an interactive job.
   csmi_job_type_t_MAX=2 ///< 2 - Bounding Value
} csmi_job_type_t;



/**
 * @brief Maps enum csmi_job_type_t value to string.
 */
extern const char* csmi_job_type_t_strs [];
/** @}*/


/** defgroup csmi_step_status_t csmi_step_status_t
 * @{
 */
/**
  * @brief 
  */
typedef enum {
   CSM_STEP_RUNNING=0, ///< 0 - The step is currently running.
   CSM_STEP_COMPLETED=1, ///< 1 - The step has been completed.
   CSM_STEP_KILLED=2, ///< 2 - The step has been killed.
   csmi_step_status_t_MAX=3 ///< 3 - Bounding Value
} csmi_step_status_t;



/**
 * @brief Maps enum csmi_step_status_t value to string.
 */
extern const char* csmi_step_status_t_strs [];
/** @}*/


/**
 * @brief Defines a structure for containing the fields from the *csm_allocation_history* table distinct from *csm_allocation*.
 *
 * This structure is designed to be used in conjunction with @ref csmi_allocation_t, as the 
 * *csm_allocation_history* table is a super set of the *csm_allocation* table.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t exit_status;  /**< Allocation exit status. */
    char* archive_history_time;  /**< Timestamp when the allocation was archived. */
    char* end_time;  /**< Timestamp when the allocation was moved to the history table. */
} csmi_allocation_history_t;
/**
 * @brief Defines a CSM Allocation in the CSM Database.
 * 
 * The fields defined in this struct represent an entry in the *csm_allocation* table. If this
 * allocation represents a historic allocation (e.g. not active) the @ref csmi_allocation_t.history
 * field will be populated. The resultant combination will represent an entry in the 
 * *csm_allocation_history* table.
 *
 * The @ref csmi_allocation_t.compute_nodes field represents any nodes that have participated in 
 * the allocation, with the size of this array being represented by @ref csmi_allocation_t.num_nodes.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t allocation_id; /**< The unique identifier for the allocation. */
    int64_t primary_job_id; /**< Primary job id (for LSF this will be the LSF job ID). */
    int64_t ssd_min; /**< Minimum ssd size for the allocation (bytes). */
    int64_t ssd_max; /**< Maximum ssd size for the allocation (bytes).*/
    int64_t time_limit; /**< The time limit requested or imposed on the job */
    int32_t secondary_job_id; /**< Secondary job id (for LSF this will be the LSF job index for job arrays). */
    int32_t num_processors; /**< Number of processors for use in the allocation (required). */
    int32_t num_gpus; /**< Number of gpus to be used for the node (optional). */
    int32_t projected_memory; /**< Projected memory usage on the node for the allocation. */
    int32_t user_id; /**< User id of allocation owner. */
    int32_t user_group_id; /**< User group of allocation owner. */
    int32_t isolated_cores; /**< Specifies the number of cores to isolate in the system cgroup per socket: 0 indicates no core isolation, 1 indicates 1 per socket, etc. Current maximum of 4 per socket. */
    uint32_t num_nodes; /**< Number of nodes, size of @ref compute_nodes. */
    csmi_allocation_type_t type; /**< Type of allocation, refer to @ref csmi_allocation_type_t for details. */
    csmi_job_type_t job_type; /**< Type of job, refer to @ref csmi_job_type_t for details. */
    csmi_state_t state; /**< State of allocation, refer to @ref csmi_state_t for details.*/
    csm_bool shared; /**< Flag for creating a shared allocation.*/
    char* ssd_file_system_name; /**< SSD file system name that user wants. */
    char* launch_node_name; /**< The launch node name. */
    char* user_flags; /**< User flags for the epilog/prolog. */
    char* system_flags; /**< System flags for the epilog/prolog. */
    char* user_name; /**< User name of allocation owner. */
    char* user_script; /**< The user script invoked fot the allocation. */
    char* begin_time; /**< Timestamp for when the allocation was reserved in the database.*/
    char* account; /**< Account of the allocation owner. */
    char* comment; /**< Comments for the allocation. */
    char* job_name; /**< Job name associated with allocation. */
    char* job_submit_time; /**< The timestamp when job was submitted. */
    char* queue; /**< Identifies the partition (queue) on which the job ran. */
    char* requeue; /**< Identifies whether or not the job was requeued. */
    char* wc_key; /**< Arbitrary string for grouping orthogonal accounts. */
    char** compute_nodes; /**< List of nodes that participated in the allocation, size stored in @ref num_nodes. */
    csmi_allocation_history_t* history; /**< The history component of the allocation, if the allocation is active this will be **NULL**. */
    int16_t smt_mode; /**< The SMT Mode of the allocation. 0 - all cores, 1+ - smt_mode cores, <0 use system default. */
} csmi_allocation_t;
/**
 * @brief Defines the accounting values for a compute node.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t ib_rx; /**< Total count of Data Octets received on all Infiniband ports, 0 if the job is active ( multiply by 4 for bytes ).*/  
    int64_t ib_tx; /**< Total count of Data Octets transmitted on all Infiniband ports, 0 if the job is active ( multiply by 4 for bytes ).*/  
    int64_t gpfs_read; /**< Total counter for number of bytes read over network via gpfs, 0 if the job is active. Join on details. */  
    int64_t gpfs_write; /**< Total counter for number of bytes written over network via gpfs, 0 if the job is active. Join on details. */  
    int64_t ssd_read; /**< Total counter for number of bytes read from the ssds, 0 if the job is active. */  
    int64_t ssd_write; /**< Total counter for number of bytes written to the ssds, 0 if the job is active. */  
    int64_t gpu_usage; /**< Total usage of the GPU (TODO Change), 0 if the job is active. */  
    int64_t energy_consumed; /**< Energy consumed by the node. */
    int64_t cpu_usage; /**< The cpu usage in nanoseconds.*/
    int64_t memory_usage_max; /**< The maximum memory usage in bytes. */
    int64_t power_cap_hit; /**< Counter indicating the number of times the power cap has been hit on the node.*/
    int32_t power_cap; /**< Power Cap - measured in watts. */
    int32_t power_shifting_ratio; /**< Power Shifting Ratio - a ratio [0-100] used by OCC for power capping. 0 is all CPU, 100 is all GPU. */
    int64_t gpu_energy; /**< GPU usage in watts. */
} csmi_allocation_accounting_t;
/**
 * @brief A structure for tracking the nodes associated with a particular step of an allocation.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t step_id; /**< The unique identifier for the step. */
    uint32_t num_nodes; /**< Number of nodes in the step, size of @ref compute_nodes. */
    char* end_time; /**< The time at which the step entered history, **NULL** if the step is active. */
    char* compute_nodes; /**< List of nodes the step is run on, size defined in @ref num_nodes. */
} csmi_allocation_step_list_t;
/**
 * @brief Tracks the transition history of the Allocation.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    char* history_time; /**< Time at which the state change occured.*/
    csmi_state_t state; /**< State of allocation, refer to @ref csmi_state_t for details.*/
} csmi_allocation_state_history_t;
/**
 * @brief Contains the steps and and accounting details for an allocation defined by @ref csmi_allocation_details_t.allocation_id.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint64_t power_cap_hit; /**< Counter indicating the number of times the power cap was hit by nodes in the allocation.*/
    uint64_t ssd_read;  /**< SSD reads for this allocation (in Gigabytes). */
    uint64_t ssd_write;  /**< SSD writes for this allocation (in Gigabytes) */
    uint32_t num_steps; /**< Number of steps in allocation, size of @ref steps. */
    uint32_t num_nodes; /**< Number of nodes in allocation, size of @ref node_accounting. */
    csmi_allocation_step_list_t** steps; /**< Collection of steps for allocation, size in @ref num_steps  */
    csmi_allocation_accounting_t** node_accounting; /**< Accounting data, each array index represents a nod in parallel to the @ref csmi_allocation_t::compute_nodes array. Size defined in @ref num_nodes. */
    uint32_t num_transitions; /**< Number of transitions for the allocation. */
    csmi_allocation_state_history_t** state_transitions; /**< State transtion list. Tracks time at which the allocation left that state. Size defined in @ref num_transitions.*/
} csmi_allocation_details_t;
/**
 * @brief Configuration struct for deleting a CSM Step.
 * 
 * This represents the fields in a *csm_step_history* entry distinct from a *csm_step*.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t exit_status; /**< step/s exit status. will be tracked and given to csm by job leader */
    double total_u_time; /**< Relates to the 'us' (user mode) value of %Cpu(s) of the 'top' Linux cmd. */
    double total_s_time; /**< Relates to the 'sy' (system mode) value of %Cpu(s) of the 'top' Linux cmd. */
    int64_t max_memory; /**< The maximum memory usage of the step. */
    char* error_message; /**< Error text from a step. */
    char* omp_thread_limit; /**< Max number of omp threads used by the step. */
    char* cpu_stats; /**< Statistics gathered from the CPU for the step. */
    char* gpu_stats; /**< Statistics gathered from the GPU for the step. */
    char* memory_stats; /**< Memory statistics for the the step. */
    char* io_stats; /**< General input output statistics for the step.? */
    char* end_time; /**< **History Only**\n Timestamp when this step ended. Unused by @ref csm_allocation_step_end. */
    char* archive_history_time; /**< **History Only**\n Timestamp when the history data has been archived and sent to: BDS, archive file, and or other. Unused by @ref csm_allocation_step_end. */
} csmi_allocation_step_history_t;
/**
 * @brief A struct representing a step in the csm database.
 * 
 * This struct represents a row in both in the active and history tables.
 * If the step is active the history pointer will be set to null in query APIs.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t step_id; /**< The identifier of the step, combined with @ref allocation_id to form a unique identifier. */
    int64_t allocation_id; /**< Allocation that the step is a member of, when combined with @ref step_id represents a unique step.*/
    int32_t num_nodes; /**< Number of nodes, size of @ref compute_nodes.*/
    int32_t num_processors; /**< Number of CPUs assigned to a job. */
    int32_t num_gpus; /**< Number of gpus. */
    int32_t projected_memory; /**< Projected memory availablity for the step.*/
    int32_t num_tasks; /**< Number of tasks in the step*/
    csmi_step_status_t status; /**< The status of the step, @ref csmi_step_status.def */
    char* executable; /**< Executable/command/application name of the step. */
    char* working_directory; /**< Working directory of the step. */
    char* user_flags; /**< User prolog/epilog flags. If NULL the prolog and epilog are not to be run.*/
    char* argument; /**< Arguments/parameters to the step. */
    char* environment_variable; /**< Environment variables for the step. */
    char* begin_time; /**< Timestamp when this job step started. Unused in @ref csm_allocation_step_begin. */
    char** compute_nodes; /**< The list of nodes participating in the allocation, size stored in @ref num_nodes. */
    csmi_allocation_step_history_t* history; /**< Represents the rows from the step's history table, NULL if the step is active. Unused in @ref csm_allocation_step_begin .*/
} csmi_allocation_step_t;
/**
 * @brief Fields from the *csm_ssd* tables to represent the resources of a node.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    double wear_lifespan_used; /**< Estimate of the amount of SSD life consumed (w.l.m. will use. valid range 0-255 percent) 0 = new, 100 = completely used, 100+ = over est life time. */
    char* serial_number; /**< Unique identifier for this ssd. */
    char* update_time; /**< The time the ssd information was last updated in the database. */
} csmi_ssd_resources_record_t;
/**
 * @brief Combines fields from the *csm_node* and *csm_ssd* tables to represent the resources of a node.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t node_installed_memory; /**< Amount of installed memory on this node (in kB). */
    int64_t vg_available_size; /**< Available size remaining (in bytes) in the volume group on this node. */
    int64_t vg_total_size; /**< Total size (in bytes) of the volume group on this node. */
    int32_t node_available_cores; /**< Deprecated after CSM_VERSION_0_4_1. Number of physical cores on this node from all processors. */
    int32_t node_available_gpus; /**< Deprecated after CSM_VERSION_0_4_1. Number of gpus available. */
    int32_t node_available_processors; /**< Deprecated after CSM_VERSION_0_4_1. Number of processors on this node (processor sockets, non-uniform memory access (NUMA) nodes). */
    uint32_t ssds_count; /**< Number of elements in the ssds array. Size of @ref ssds. */
    csmi_node_type_t node_type; /**< The type of the node, see @ref csmi_node_type_t for details. */
    csmi_node_state_t node_state; /**< State of the node, see @ref csmi_node_state_t for details. */
    csm_bool node_ready; /**< After CSM_VERSION_0_4_1, this field is populated indirectly via 'node_state'. - Flag indicating whether or not the node is ready ( @ref csm_bool). */
    char* node_name; /**< The name of the node described by this struct. */
    char* node_update_time; /**< The time the node information was last updated in the database. */
    char* vg_update_time; /**< Last time all the VG related information was updated. */
    csmi_ssd_resources_record_t** ssds; /**< A list of ssd resources (for each ssd on this node) relevant to a job scheduler. Size defined in @ref ssds_count. */
    int32_t node_discovered_cores; /**< replacement for 'available_cores' after CSM_VERSION_0_4_1. Number of physical cores on this node from all processors. */
    int32_t node_discovered_gpus; /**< replacement for 'available_gpus' after CSM_VERSION_0_4_1. Number of gpus on node. */
    int32_t node_discovered_sockets; /**< replacement for 'available_processors' after CSM_VERSION_0_4_1. Number of processors on the node.*/
} csmi_node_resources_record_t;
/**
 * @brief Defines a cgroup and any associated parameter value pairs.
 *
 * @todo Post-PRPQ: Should params and values be one array of tuples?
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t num_params; /**< The number of parameters/values to update. Size of @ref params, and @ref values. */
    csmi_cgroup_controller_t type; /**< The type of the cgroup controller to update. */
    char** params; /**< The parameters to set the value of. @ref num_params is the array size. 1:1 match with @ref values.*/
    char** values; /**< The values to update the parameters to. @ref num_params is the array size. 1:1 match with @ref params.*/
} csmi_cgroup_t;
/**
 * @brief Represents a snapshot of a node relative to an allocation.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    csm_bool ready; /**< Flag indicating whether or not the node is ready ( @ref csm_bool). */
    char* node_name; /**< The node this resource is associated with.*/
} csmi_allocation_resources_record_t;
/**
 * @brief An input wrapper for @ref csm_allocation_create.
 *
 * An alias to @ref csmi_allocation_t. 
 *
 * The @ref csmi_allocation_t.allocation_id field will
 * be overwritten by @ref csm_allocation_create.
 */
typedef csmi_allocation_t csm_allocation_create_input_t;
/**
 * @brief An input wrapper for @ref csm_allocation_query_details.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t allocation_id; /**< The allocation id to search for.*/
} csm_allocation_query_details_input_t;
/**
 * @brief A wrapper for the output of @ref csm_allocation_query_details.
 *
 * Aggregates @ref csmi_allocation_t and @ref csmi_allocation_details_t.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    csmi_allocation_t* allocation; /**< The base allocation information. */
    csmi_allocation_details_t* allocation_details; /**< Additional details regarding the allocation. */
} csm_allocation_query_details_output_t;
/**
 * @brief A wrapper struct for use with @ref csm_allocation_update_state.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t allocation_id; /**< The identifier for the allocation to update. */
    int32_t exit_status; /**< Allocation exit status, initializes to 0 */
    csmi_state_t new_state; /**< New Allocation state. Supported States: @ref csmi_state_t.RUNNING, @ref csmi_state_t.STAGING_OUT */
} csm_allocation_update_state_input_t;
/**
 * @brief An input wrapper for @ref csm_allocation_step_begin
 *
 * An alias to @ref csmi_allocation_step_t.
 */
typedef csmi_allocation_step_t csm_allocation_step_begin_input_t;
/**
 * @brief Wrapper for the input arguments to @ref csm_allocation_step_end.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t step_id; /**< The identifier of the step, combined with @ref allocation_id to form a unique identifier.*/
    int64_t allocation_id; /**< Allocation that the step is a member of, when combined with @ref step_id represents a unique step.*/
    csmi_step_status_t status; /**< The status of the step, @ref csmi_step_status_t */
    csmi_allocation_step_history_t* history; /**< Specifies values to provide to the allocation step as it is ended. */
} csm_allocation_step_end_input_t;
/**
 * @brief An input wrapper for @ref csm_allocation_step_query_input_t.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t allocation_id; /**< Allocation that this step is a member of. If @ref step_id is < 0 all steps for the allocating will be retrieved. */
    int64_t step_id; /**< The step id, combined with @ref allocation_id to uniquely identify a step in the database. Ignored if < 0. */
    int32_t limit; /**< SQL 'LIMIT' numeric value. API will ignore values less than 1.*/
    int32_t offset; /**< SQL 'OFFSET' numeric value. API will ignore values less than 1.*/
} csm_allocation_step_query_input_t;
/**
 * @brief A wrapper for the output of @ref csm_allocation_step_query.
 *
 * If @ref csmi_allocation_step_t.history is not null the step is in the history
 * table.
 *
 * @ref csmi_allocation_step_t.node_names aggregates the entries found in
 * either the *csm_step_node* or *csm_step_node_history* which match the targeted step.
 * This should not be populated when using @ref csm_allocation_step_query.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t num_steps; /**< The number of steps retrieved. */
    csmi_allocation_step_t** steps; /**< The active steps retrieved by the api. */
} csm_allocation_step_query_output_t;
/**
 * @brief An input wrapper for @ref csm_allocation_step_query_details.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t allocation_id; /**< Allocation that this step is a member of. If @ref step_id is < 0 all steps for the allocation will be retrieved. */
    int64_t step_id; /**< The step id, combined with @ref allocation_id to uniquely identify a step in the database. Ignored if < 0. */
    int32_t limit; /**< SQL 'LIMIT' numeric value. API will ignore values less than 1.*/
    int32_t offset; /**< SQL 'OFFSET' numeric value. API will ignore values less than 1.*/
} csm_allocation_step_query_details_input_t;
/**
 * @brief A wrapper for the output of @ref csm_allocation_step_query_details.
 *
 * An alias to @ref csm_allocation_step_query_output_t.
 * 
 * @ref csmi_allocation_step_t.node_names Is expected to be populated 
 * by @ref csm_allocation_step_query_details.
 */
typedef csm_allocation_step_query_output_t csm_allocation_step_query_details_output_t;
/**
 * @brief An input wrapper for @ref csm_allocation_step_query_active_all.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t allocation_id; /**< The allocation to retrieve all active steps for. */
    int32_t limit; /**< SQL 'LIMIT' numeric value. API will ignore values less than 1.*/
    int32_t offset; /**< SQL 'OFFSET' numeric value. API will ignore values less than 1.*/
} csm_allocation_step_query_active_all_input_t;
/**
 * @brief A wrapper for the output of @ref csm_allocation_step_query_active_all.
 *
 * An alias to @ref csm_allocation_step_query_output_t.
 *
 * @ref csmi_allocation_step_t.node_names is not expected to be populated by
 * @ref csm_allocation_step_query_active_all
 */
typedef csm_allocation_step_query_output_t csm_allocation_step_query_active_all_output_t;
/**
 * @brief An input wrapper for @ref csm_node_resources_query.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t limit; /**< SQL 'LIMIT' numeric value. API will ignore values less than 1.*/
    int32_t offset; /**< SQL 'OFFSET' numeric value. API will ignore values less than 1.*/
    uint32_t node_names_count; /**< Number of elements in the 'node_names' array. Must be greater than zero. Size of @ref node_names.*/
    char** node_names; /**< Filter results to only include records that have a matching node_name. Must contain at least one node_name. If left NULL, then API will fail. Size defined in @ref node_names_count.  */
} csm_node_resources_query_input_t;
/**
 * @brief A collection of  @ref csmi_node_resources_record_t structs.
 * 
 * Represents the results of a @ref csm_node_resources_query run.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t results_count; /**< Number of nodes recovered, size of @ref results. */
    csmi_node_resources_record_t** results; /**< An array of all the records returned from the SQL query. Size defined in @ref results_count. */
} csm_node_resources_query_output_t;
/**
 * @brief An input wrapper for @ref csm_node_resources_query_all.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t limit; /**< SQL 'LIMIT' numeric value. API will ignore values less than 1.*/
    int32_t offset; /**< SQL 'OFFSET' numeric value. API will ignore values less than 1.*/
} csm_node_resources_query_all_input_t;
/**
 * @brief A collection of  @ref csmi_node_resources_record_t structs.
 * 
 * Represents the results of a @ref csm_node_resources_query_all run.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t results_count; /**< Number of results recovered, size of @ref results.*/
    csmi_node_resources_record_t** results; /**< An array of all the records returned from the SQL query. Size defined in @ref results_count.  */
} csm_node_resources_query_all_output_t;
/**
 * @brief Defines a new control group which is a child of an allocation control group.
 *
 * This struct will define cgroups in the following manner.
 * 
 * If @ref allocation_id >= 0:
 *
 * <pre>
 * <components[i]->name>
 *   |
 *   |- allocation_<allocation_id>
 *   |  |
 *   |  |- <cgroup_name>
 *   |
 *   ...
 * </pre>
 *
 * If the @ref allocation_id is < 0:
 * 
 * <pre>
 * <components[i]->name>
 *   |
 *   |- <cgroup_name>
 *   |
 *   ...
 * </pre>
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t allocation_id; /**< The allocation id of the allocation requesting a cgroup, if a cgroup for this allocation id doesn't exist this container will not be able to create the requested cgroup. If -1 is supplied operations will be performed on the base cgroup.*/ 
    uint32_t num_components; /**< The number of components [e.g. cpuset, devices, etc.] defined for this control group.*/
    pid_t pid; /**< The process id to associate components with [this pid is placed in each cgroup's tasks file]*/
    char* cgroup_name; /**< The name of the cgroup to be produced in the components specified, this will be a child of the allocation cgroup matching the supplied @ref allocation_id. If this is null operations will be performed on the allocation cgroup.*/
    csmi_cgroup_t** components; /**< The components to create the requested cgroups in, each component will be given the pid and any values specified in the component structure. */
} csm_allocation_step_cgroup_create_input_t;
/**

 * @brief Defines a set parameters for the deletion of a cgroup on a node with an active step.
 *
 * The structure of the cgroups being deleted are as follows:
 *
 * If @ref allocation_id >= 0:
 *
 * <pre>
 * <components[i]->name>
 *   |
 *   |- allocation_<allocation_id>
 *   |  |
 *   |  |- <cgroup_name>
 *   |
 *   ...
 * </pre>
 *
 * If the @ref allocation_id is < 0:
 * 
 * <pre>
 * <components[i]->name>
 *   |
 *   |- <cgroup_name>
 *   |
 *   ...
 * </pre>
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t allocation_id; /**< The allocation id, representing parent of the cgroup to delete. */
    uint32_t num_types; /**< The number of controller types to run delete on, if 0 all control groups are deleted (make sure @ref controller_types is NULL). */
    char* cgroup_name; /**< The name of the cgroup to be deleted. */
    csmi_cgroup_controller_t* controller_types; /**< The controller types associated with the cgroup.*/
} csm_allocation_step_cgroup_delete_input_t;
/**
 * @brief An input wrapper for @ref csm_allocation_resources_query.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t allocation_id; /**< Allocation id to search for. */
    int32_t limit; /**< SQL 'LIMIT' numeric value. API will ignore values less than 1.*/
    int32_t offset; /**< SQL 'OFFSET' numeric value. API will ignore values less than 1.*/
} csm_allocation_resources_query_input_t;
/**
 * @brief A wrapper for the output of @ref csm_allocation_resources_query
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t results_count; /**< Resources retrieved by the query, size of @ref results.*/
    csmi_allocation_resources_record_t** results; /**< A list of resource records returned by the query, size defined by @ref results_count.*/
} csm_allocation_resources_query_output_t;
/**
 * @brief Contains arguments for the query executed by @ref csm_allocation_update_history.
 *
 * The @ref csm_allocation_update_history api uses optional parameters: strings are not 
 * used if set to NULL and numeric types are unused if set to -1.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t allocation_id; /**< Allocation id to update the history of. */
    int32_t user_id; /**< Optional, user id of allocation owner. Values < 0 will not be used.*/
    int32_t user_group_id; /**< Optional, user group of allocation owner. Values < 0 will not be used.*/
    char* user_name; /**< Optional, user name of allocation owner. NULL will not be used.*/
    char* account; /**< Optional, account of the allocation owner. NULL will not be used.*/
    char* comment; /**< Optional, comments for the allocation, destructive. NULL will not be used.*/
    char* job_name; /**< Optional, job name for the allocation. NULL will not be used.*/
    char* reservation; /**< Optional, reservation name for the allocation. NULL will not be used.*/
} csm_allocation_update_history_input_t;
/**
 * @brief An input wrapper for @ref csm_allocation_query.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t allocation_id; /**< The allocation id to search on, optional if @ref primary_job_id  and  @ref secondary_job_id are set. */
    int64_t primary_job_id; /**< The primary job id of the query, ignored if @ref allocation_id was set.*/
    int32_t secondary_job_id;  /**< The secondary job id of the query, ignored if @ref allocation_id was set.*/
} csm_allocation_query_input_t;
/**
 * @brief A wrapper for the output of @ref csm_allocation_query.
 * @todo Post-PRPQ: Should this return more than one allocation if the allocation query had collisions? 
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    csmi_allocation_t* allocation; /**< Retrieved allocation. */
} csm_allocation_query_output_t;
/**
 * @brief An input wrapper for @ref csm_allocation_query_active_all.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t limit; /**< SQL 'LIMIT' numeric value. API will ignore values less than 1.*/
    int32_t offset; /**< SQL 'OFFSET' numeric value. API will ignore values less than 1.*/
} csm_allocation_query_active_all_input_t;
/**
 * @brief A wrapper for the output of @ref csm_allocation_query_active_all. 
 * @todo Post-PRPQ: Should we replace num_allocations and allocations with result_count and results?
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t num_allocations; /**< Number of allocations found. */
    csmi_allocation_t** allocations; /**< Active allocations found. */
} csm_allocation_query_active_all_output_t;
/**
 * @brief An input wrapper for @ref csm_allocation_delete.
 *
 * @todo Post-PRPQ: Should this take the end time?
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t allocation_id; /**< The allocation id to delete, set to zero if @ref primary_job_id is supplied.  */
    int32_t exit_status; /**< Allocation exit status, initializes to 0 */
    int64_t primary_job_id; /**< Primary Job ID, set to zero if @ref allocation_id is supplied. */
    int32_t secondary_job_id; /**< Secondary Job ID, ignored if @ref primary_job_id is not supplied.*/
} csm_allocation_delete_input_t;
/**
 * @brief An input wrapper for @ref csm_cgroup_login.
 *
 * This struct is used internally for the PAM module.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t allocation_id; /**< Allocation ID to prefer (rejected if >0 and not found in daemon). */
    pid_t pid; /**< The process id to push to the allocation cgroup. */
    char* user_name; /**< User name for logging in. */
    char migrate_pid; /**< Flag to migrate pid in to cgroup if true. */
} csm_cgroup_login_input_t;
/**
 * @brief An input wrapper for @ref csm_jsrun_cmd.
 *
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t allocation_id; /**< The Allocation id for the JSM run. Exported to **CSM_ALLOCATION_ID**. */
    char* kv_pairs; /**< Arguments to the JSM run: Supports alphanumeric, ',' , and  '='. Exported to **CSM_JSM_ARGS**. */
    char* jsm_path; /**< The fully qualified path to the JSM executable, if NULL ignored and the default path is used ( /opt/ibm/spectrum_mpi/jsm_pmix/bin/jsm ). */
} csm_jsrun_cmd_input_t;
/**
 * @brief A container for a CSM error, encapsulates the source error code and message.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int errcode; ///< The error code of this error message. 
    char* errmsg; ///< The error message. 
    char* source; ///< The host reporting the error.
} csm_soft_failure_recovery_node_t;
/**
 * @brief A wrapper for the output of 
 *
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t retry_count; ///< The number of times to retry soft failure recovery.
} csm_soft_failure_recovery_input_t;
/**
 * @brief A wrapper for the output of 
 *
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t error_count; ///< A count of errors.
    csm_soft_failure_recovery_node_t** node_errors; ///< Collection of errors which occured on nodes.
} csm_soft_failure_recovery_output_t;
/** @} */

#ifdef __cplusplus
}
#endif
#endif
