/*================================================================================
   
    csmi/include/csmi_type_bb.h

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
/** @file csmi_type_bb.h
 * @brief A collection of structs for @ref bb_apis.
 */
#ifndef _CSMI_BB_TYPES_H_
#define _CSMI_BB_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

// Begin special_preprocess directives
#include "csm_api_common.h"

/** @addtogroup bb_apis
 * @{
 */

// End special_preprocess directives

/**
 * @brief Represents an entry in the *csm_vg* table in the CSM Database.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t total_size; /**< Volume group size, in bytes.*/
    int64_t available_size; /**< Bytes available in volume group, subset of @ref total_size. */
    csm_bool scheduler; /**< Tells CSM whether or not this is the volume group for the scheduler. Defaults to false. ( @ref csm_bool). */
    char* vg_name; /**< Name of the volume group. */
    char* node_name; /**< Identifies which node has this volume group. */
} csmi_vg_record_t;
/**
 * @brief Represents an entry in the *csm_lv* table in the CSM Database.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t allocation_id; /**< Unique identifier for an allocation associated with the logical volume. */
    int64_t current_size; /**< Current size (in bytes). */
    int64_t max_size; /**< Max size (in bytes) achieved at run time.*/
    char state; /**< State of the logical volume - [C]reated, [M]ounted, [S]hrinking, [R]emoved */
    char* logical_volume_name; /**< Unique identifier for this ssd partition. */
    char* vg_name; /**< Volume group name. */
    char* node_name; /**< Node with this logical volume. */
    char* file_system_mount; /**< Identifies the file system and mount point. */
    char* file_system_type; /**< Identifies the file system and its partition. */
    char* begin_time; /**< Timestamp when the lv was created. */
    char* updated_time; /**< When the lv was last updated. */
} csmi_lv_record_t;
/**
 * @brief Help construct the VG SSD relations in the csm database.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t ssd_allocation; /**< The amount of space (in bytes) that this ssd contributes to the volume group. Can not be less than zero. The total sum of these fields should equal 'total_size' in the @ref csm_bb_vg_create_input_t struct. API will check this and throw error if mismatched. */
    char* ssd_serial_number; /**< Unique identifier for this ssd partition. Can not be NULL. API will throw error if left NULL.*/
} csmi_bb_vg_ssd_info_t;
/**
 * @brief An input wrapper for @ref csm_bb_cmd.
 *
 * Encapsulates a burst buffer command to be sent to a compute node.
 *
 * Executes: < @ref command_arguments > < @ref command >; 
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t node_names_count; /**< Number of  nodes to issue the burst buffer command on, size of @ref node_names. */
    char* command_arguments; /**< The arguments for the command executable( [a-zA-Z -_]* ) */
    char** node_names; /**< Compute nodes to receive this command, size defined in @ref node_names_count.*/
} csm_bb_cmd_input_t;
/**
 * @brief A wrapper for the output of @ref csm_bb_cmd.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    char* command_output; /**< The output from the command executable. */
} csm_bb_cmd_output_t;
/**
 * @brief An input wrapper for @ref csm_bb_lv_create.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t allocation_id; /**< Unique identifier of an allocation. */
    int64_t current_size; /**< Current logical volume size (bytes). */
    char state; /**< State of the logical volume - [C]reated, [M]ounted, [S]hrinking, or [R]emoved. */
    char* file_system_mount; /**< Identifies the file system and mount point. */
    char* file_system_type; /**< Identifies the file system and its partition. */
    char* logical_volume_name; /**< Unique identifier for this ssd partition. */
    char* node_name; /**< Node to create the logical volume on. */
    char* vg_name; /**< Volume group name. */
} csm_bb_lv_create_input_t;
/**
 * @brief Specifies a logical volume to delete for @ref csm_bb_lv_delete.
 * @brief An input wrapper for @ref csm_bb_lv_delete.
 * 
 * Specifies a logical volume to delete and any metrics associated with.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t allocation_id; /**< Unique identifier of an allocation. */
    int64_t num_bytes_read; /**< Number of bytes read during the life of this partition.*/
    int64_t num_bytes_written; /**< Number of bytes written during the life of this partition.*/
    char* logical_volume_name; /**< Unique identifier for this ssd partition.*/
    char* node_name; /**< Name of the node where this logical volume is located.*/
    int64_t num_reads; /**< Number of reads during the life of this partition. - OPTIONAL - defaults to '-1' if not provided. values less than 0 will be inserted into csm database as NULL.*/
    int64_t num_writes; /**< Number of writes during the life of this partition. - OPTIONAL - defaults to '-1' if not provided. values less than 0 will be inserted into csm database as NULL.*/
} csm_bb_lv_delete_input_t;
/**
 * @brief An input wrapper for @ref csm_bb_lv_query.
 *
 * To perform a query one or more of the encapsulated arrays must be non null. 
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t limit; /**< SQL 'LIMIT' numeric value. API will ignore values less than 1.*/
    int32_t offset; /**< SQL 'OFFSET' numeric value. API will ignore values less than 1.*/
    uint32_t allocation_ids_count; /**< Number of allocations to perform query on, size of @ref allocation_ids. */
    uint32_t logical_volume_names_count; /**< Number of logical volumes to perform query on, size of @ref logical_volume_names. */
    uint32_t node_names_count; /**< Number of nodes to perform the query on, size of @ref node_names.*/
    int64_t* allocation_ids; /**< Filter results to only include records that have a matching allocation_ids. Size defined by @ref allocation_ids_count. */
    char** logical_volume_names; /**< Filter results to only include records that have a matching logical_volume_name. Size defined by @ref logical_volume_names_count. */
    char** node_names; /**< Filter results to only include records that have a matching node_name. Size defined by @ref node_names_count.*/
} csm_bb_lv_query_input_t;
/**
 * @brief A wrapper for the output of @ref csm_bb_lv_query.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t results_count; /**< Number of logical volumes retrieved, size of @ref results. */
    csmi_lv_record_t** results; /**< An array of all the records returned from the SQL query. */
} csm_bb_lv_query_output_t;
/**
 * @brief An input wrapper for @ref csm_bb_lv_update.
 *
 * Specifies a logical volume and parameters to update it.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t allocation_id; /**< Unique identifier for this allocation. Can not be less than zero. */
    int64_t current_size; /**< Current size (in bytes) Can not be less than zero. */
    char state; /**< State of the logical volume - [C]reated, [M]ounted, [S]hrinking, or [R]emoved. If left NULL, then API will fail.*/
    char* logical_volume_name; /**< Unique identifier for this ssd partition. If left NULL, then API will fail. */
    char* node_name; /**< Name of the node where this LV is located.*/
} csm_bb_lv_update_input_t;
/**
 * @brief An input wrapper for @ref csm_bb_vg_create.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t available_size; /**< Available size remaining in this volume group. Can not be greater than 'total_size'. Values less than zero are not valid. */
    int64_t total_size; /**< Total size of this volume group. Values less than zero are not valid. */
    uint32_t ssd_info_count; /**< Number of entries in the ssd_info list, size of @ref ssd_info. */
    csm_bool scheduler; /**< Tells CSM whether or not this is the volume group for the scheduler. Defaults to false. ( @ref csm_bool). */
    char* node_name; /**< This volume group is attached to this node. Can not be NULL. 'node_name' must exist in 'csm_node' table. */
    char* vg_name; /**< Unique identifier for this volume group. Can not be NULL. */
    csmi_bb_vg_ssd_info_t** ssd_info; /**< List of ssd information belonging to this volume group. Can not be NULL. Must contain at least one entry. Size defined in @ref ssd_info_count. */
} csm_bb_vg_create_input_t;
/**
 * @brief An input wrapper for @ref csm_bb_vg_delete.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    char* node_name; /**< Name of node where the VG is located. Can not be NULL.*/
    char* vg_name; /**< Name of volume group to delete. Can not be NULL.*/
} csm_bb_vg_delete_input_t;
/**
 * @brief A wrapper for the output of  @ref csm_bb_vg_delete.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t failure_count; /**< Number of volume groups that failed to be deleted. Size of @ref failure_vg_names. */
    char** failure_vg_names; /**< Volume groups that failed to be deleted, size defined in @ref failure_count. */
} csm_bb_vg_delete_output_t;
/**
 * @brief An input wrapper for @ref csm_bb_vg_query.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t limit; /**< SQL 'LIMIT' numeric value. API will ignore values less than 1.*/
    int32_t offset; /**< SQL 'OFFSET' numeric value. API will ignore values less than 1.*/
    uint32_t vg_names_count; /**< Number of volume names to filter on, size of @ref vg_names.*/
    uint32_t node_names_count; /**< Number of node names to filter on, size of @ref node_names.*/
    char** vg_names; /**< Filter results to only include records that have a matching vg_name. Size defined in @ref vg_names_count. */
    char** node_names; /**< Filter results to only include records that have a matching node_name. Size defined in @ref node_names_count.*/
} csm_bb_vg_query_input_t;
/**
 * @brief A wrapper for the output of @ref csm_bb_vg_query.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t results_count; /**< The number of records retrieved, size of @ref results. */
    csmi_vg_record_t** results; /**< An array of the records returned from the SQL query. Size defined in @ref results_count. */
} csm_bb_vg_query_output_t;
/** @} */

#ifdef __cplusplus
}
#endif
#endif
