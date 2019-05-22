--===============================================================================
--
--   csm_create_tables.sql
--
-- © Copyright IBM Corporation 2015-2019. All Rights Reserved
--
--   This program is licensed under the terms of the Eclipse Public License
--   v1.0 as published by the Eclipse Foundation and available at
--   http://www.eclipse.org/legal/epl-v10.html
--
--   U.S. Government Users Restricted Rights:  Use, duplication or disclosure
--   restricted by GSA ADP Schedule Contract with IBM Corp.
--
--===============================================================================

--===============================================================================
--   usage:             run ./csm_db_script.sh <----- to create the csm_db with tables
--   current_version:   18.0
--   create:            12-14-2015
--   last modified:     05-22-2019
--   change log:
--   18.0   Added core_blink (boolean) field to the csm_allocation and csm_allocation_history tables with comments.
--          Added in type and fw_version to the csm_switch_inventory and csm_inventory_history tables with comments.
--   17.0   Added smt_mode to csm_allocation and csm_allocation_history
--	    Added new fields to csm_lv_history - num_reads, num_writes (01-25-2019)
--   16.2   Modified TYPE csm_compute_node_states - added in HARD_FAILURE (Included below is the updated comments)
--          COMMENT ON COLUMN csm_node.state
--          COMMENT ON COLUMN csm_node_history.state
--          COMMENT ON COLUMN csm_node_state_history.state
--          COMMENT ON COLUMN csm_ras_type.set_state
--          COMMENT ON COLUMN csm_ras_type_audit.set_state
--          DB performance modifying and adding in indexes within history tables along with comments.
--          ix_csm_allocation_history_b (allocation_id)                ix_csm_allocation_history_c (ctid)
--          ix_csm_allocation_history_d (archive_history_time)         ix_csm_allocation_node_history_b (allocation_id)
--          ix_csm_allocation_node_history_c (ctid)                    ix_csm_allocation_node_history_d (archive_history_time)
--          ix_csm_allocation_state_history_b (allocation_id)          ix_csm_allocation_state_history_c (ctid)
--          ix_csm_allocation_state_history_d (archive_history_time)   ix_csm_config_history_b (csm_config_id)
--          ix_csm_config_history_c (ctid)                             ix_csm_config_history_d (archive_history_time)
--          ix_csm_db_schema_version_history_b (version)               ix_csm_db_schema_version_history_c (ctid)
--          ix_csm_db_schema_version_history_d (archive_history_time)  ix_csm_diag_result_history_b (run_id)
--          ix_csm_diag_result_history_c (ctid)                        ix_csm_diag_result_history_d (archive_history_time)
--          ix_csm_diag_run_history_b (run_id)                         ix_csm_diag_run_history_c (allocation_id)
--          ix_csm_diag_run_history_d (ctid)                           ix_csm_diag_run_history_e (archive_history_time)
--          ix_csm_dimm_history_b (node_name, serial_number)           ix_csm_dimm_history_c (ctid)
--          ix_csm_dimm_history_d (archive_history_time)               ix_csm_gpu_history_c (node_name, gpu_id)
--          ix_csm_gpu_history_d (ctid)                                ix_csm_gpu_history_e (archive_history_time)
--          ix_csm_hca_history_b (node_name, serial_number)            ix_csm_hca_history_c (ctid)
--          ix_csm_hca_history_d (archive_history_time)                ix_csm_ib_cable_history_b (serial_number)
--          ix_csm_ib_cable_history_c (ctid)                           ix_csm_ib_cable_history_d (archive_history_time)
--          ix_csm_lv_history_c (ctid)                                 ix_csm_lv_history_d (archive_history_time)
--          ix_csm_lv_update_history_c (ctid)                          ix_csm_lv_update_history_d (archive_history_time)
--          ix_csm_node_history_c (ctid)                               ix_csm_node_history_d (archive_history_time)
--          ix_csm_node_state_history_c (ctid)                         ix_csm_node_state_history_d (archive_history_time)
--          ix_csm_processor_socket_history_c (ctid)                   ix_csm_processor_socket_history_d (archive_history_time)
--          ix_csm_ras_event_action_f (master_time_stamp)              ix_csm_ras_event_action_g (ctid)
--          ix_csm_ras_event_action_h (archive_history_time)           ix_csm_ssd_history_c (ctid)
--          ix_csm_ssd_history_d (archive_history_time)                ix_csm_ssd_wear_history_c (ctid)
--          ix_csm_ssd_wear_history_d (archive_history_time)           ix_csm_step_history_f (ctid)
--          ix_csm_step_history_g (archive_history_time)               ix_csm_step_node_b (allocation_id)
--          ix_csm_step_node_c (allocation_id, step_id)                ix_csm_step_node_history_b (allocation_id)
--          ix_csm_step_node_history_c (allocation_id, step_id)        ix_csm_step_node_history_d (ctid)
--          ix_csm_step_node_history_e (archive_history_time)          ix_csm_switch_history_c (ctid)
--          ix_csm_switch_history_d (archive_history_time)             ix_csm_switch_inventory_history_b (name);
--          ix_csm_switch_inventory_history_c (ctid)                   ix_csm_switch_inventory_history_c (archive_history_time)
--          ix_csm_vg_history_b (vg_name, node_name)                   ix_csm_vg_history_c (ctid)
--          ix_csm_vg_history_d (archive_history_time)                 ix_csm_vg_ssd_history_b (vg_name, node_name)
--          ix_csm_vg_ssd_history_c (ctid)                             ix_csm_vg_ssd_history_d (archive_history_time)
--          DROP INDEXES that are not needed or are auto generated
--          ix_csm_processor_socket_a, ix_csm_switch_a
--   16.1   upgraded and added function to support API inventory
--          added in csm_db_schema_version history_time comment.
--   16.0   upgrade to functions to support API changes
--          csm_dimm, csm_socket_processor PKs constraint updated (including node_name)
--          csm_allocation_node + history modified energy, gpu_usage, and gpu_energy comments
--   15.1   GA release version - upgrade
--   15.0   GA release version
--   14.25  csm_node + history table included: discovered_ssds
--   14.24  csm_node + history table included: discovered_dimms
--   14.23      To Do List: (03-21-2018)
--          csm_lv_update_history - added in allocation_id to distinguish allocations that are apart of lv's
--          csm_switch + history - added in updated descriptions to the 'state' and 'type' fields.
--------------------------------------------------------------------------------------------------------------------------
--   14.22      To Do List: (03-19-2018) Done
--              remove operation fields from these tables (logic does not apply to related tables)
--              csm_allocation_history
--              csm_allocation_node_history
--              csm_allocation_state_history
--              csm_step_history
--              csm_step_node_history
--              csm_diag_run_history
--              csm_diag_result_history
--              csm_config_history
--              csm_db_schema_version_history
--------------------------------------------------------------------------------------------------------------------------
--          Done list: 03-16-2018
--          csm_switch + history table
--              modified comments to match UFM rest API specs
--              has_ufm_agent - changed text to boolean
--              num_modules - changed text to int.
--              removed - num_ports field
--              total_alarms - changed text to int.
--          csm_switch_inventory + history
--              max_ib_ports - changed text to int.
--              module_index - changed text to int.
--              number_of_chips - changed text to int.
--------------------------------------------------------------------------------------------------------------------------
--   14.21  csm_allocation_node + history
--              added in new field - gpu_energy (bigint) not null
--   14.20  csm_ras_event_action
--              added in master_time_stamp - not null (see comment on table for more details)
--          csm_ras_type_audit
--              removed - set_not_ready, set_ready fields (replaced with below)
--              added - set_state (enum type using node states having the ability to be null)
--              added - severity - create new enum type - (INFO, WARNING, FATAL) 
--   14.19
--          csm_switch + history and csm_switch_inventory + history
--              added in hw_version (waiting on UFM to supply related info)
--              serial_number - changed the position of the field (towards top of table now)
--   14.18
--          csm_processor_socket + history
--              rename field: socket = physical_loaction
--          csm_switch + history table
--              remove - os_version field
--   14.17
--          removed - total_size from csm_ssd + history table
--   14.16
--          added in 'operation' field to handle new trigger function
--          operation of transaction = (I - INSERT), (U - UPDATE), (D - DELETE)
--              csm_allocation_history
--              csm_allocation_node_history
--              csm_allocation_state_history
--              csm_config_history
--              csm_db_schema_version_history
--              csm_diag_result_history
--              csm_diag_run_history
--              csm_dimm_history
--              csm_gpu_history
--              csm_hca_history
--              csm_ib_cable_history
--              csm_lv_history
--              csm_lv_update_history
--              csm_node_history
--              csm_node_state_history
--              csm_processor_socket_history
--              csm_ssd_history
--              csm_ssd_wear_history
--              csm_step_history
--              csm_step_node_history
--              csm_switch_history
--              csm_switch_inventory_history
--              csm_vg_history
--              csm_vg_ssd_history
--
--              csm_processor_socket + history
--                  modified_field name - available_cores = discovered_cores
--              csm_switch + history
--                  added back in these 2 fields to the table - serial_number, os_version
--------------------------------------------------------------------------------------------------------------------------
--   14.15
--                  csm_allocation_node + hist
--                              memory_usage_max    bigint      (both_tables)
--                              cpu_usage           bigint      (both_tables)
--                              power_cap_hit       bigint      (both tables)
--                              gpu_usage           bigint      (both tables)
--                  csm_ssd + hist
--                              removed - discovery_time field
--                              removed - status field
--                  csm_node _ hist
--                              modified field names
--                              available_processors = discovered_sockets
--                              available_cores = discovered_cores
--                              available_gpus = discovered_gpus
--                              discovered_time = collection_time
--                              removed - ready field
--                  csm_node_ready_history
--                              table name modified
--                              csm_node_ready_history = csm_node_state_history
--                              modified - ready field = state field
--                  csm_processor + hist
--                              table name modified
--                              csm_processor = csm_processor_socket
--                              removed - status field
--                  csm_gpu + hist
--                              remove - status field
--                              modified field ordering - serial_number, node_name
--                              (consistent with other node attribute tables)
--                  csm_hca + hist
--                              modified field ordering - serial_number, node_name
--                              (consistent with other node attribute tables)
--                  csm_dimm + hist
--                              removing - status field
-------------------------------------------------------------------------------------------------------------
--   14.14 -        csm_ssd + hist
--                              removed available_size field
--                  csm_ssd_wear_history
--                              Created this table to handle any updated related to the wear fields in the csm_ssd table.
--                  csm_ras_event_action
--                              added in kvcsv field - text data type and added to the csm_ras_event_action_view.
--   14.13 -        csm_ssd -   Modified the index to - CREATE UNIQUE INDEX uk_csm_ssd_a (serial_number, node_name)
--                              added - not null to available_size (handles empty array values for vg_create function)
--
--                  csm_vg_ssd- Removed - CONSTRAINT uk_csm_vg_ssd_a UNIQUE (vg_name, node_name),
--                              PRIMARY KEY (vg_name, node_name, serial_number),
--                              Modified the index to - CREATE UNIQUE INDEX uk_csm_vg_ssd_a (vg_name, node_name, serial_number)
--
--                  csm_vg -    Removed - FOREIGN KEY (vg_name, node_name) references csm_vg_ssd(vg_name, node_name)
--                              CONSTRAINT uk_csm_vg_a UNIQUE (vg_name, node_name)
--                              FOREIGN KEY (node_name) references csm_node(node_name)
--                  Oredering:  Changed the creation order of csm_vg, csm_vg_ssd to handle the above changed.
--                 
--   14.12  - modified csm_vg + hist table(s):   added update_time timestamp
--   14.11  - modified csm_vg + hist table(s):   added scheduler field with boolean type
--                    csm_vg_ssd:               updated the ssd_allocation comment
--   14.10 - Modified allocation/step tables 
--           csm_allocation 
--            Rename:
--              file_system_name -> ssd_file_system_name
--              num_memory -> projected_memory
--            Split:
--             ssd_size : 
--                - ssd_min : bigint
--                - ssd_max : bigint
--           Removed:
--             eligible_time
--             reservation
--             wct_reserve  
--           
--           csm_allocation_history
--             Rename:
--               file_system_name -> ssd_file_system_name
--               num_memory -> projected_memory
--             Split:
--               ssd_size : 
--                 - ssd_min : bigint
--                 - ssd_max : bigint
--             Removed:
--               power_cap_hit
--               energy_consumed
--               eligible_time
--               reservation
--               wct_reserve
--           
--           csm_allocation_state_history
--             New:
--               error_status
--           
--           csm_step
--            Type Change:
--               state : text
--            Rename:
--               num_memory -> projected_memory
--               state -> status
--            Remove:
--               system_flags
--               seq_id
--           
--           csm_step_history
--             Type Change:
--               max_memory : bigint
--             Rename:
--               num_memory -> projected_memory
--               err_text -> error_message
--               total_num_threads -> omp_thread_limit
--               state -> status
--             Remove:
--               max_swap
--               level_gpu_usage
--               system_flags
--               seq_id
--               network_bandwidth
--===============================================================================

\set ON_ERROR_STOP on
BEGIN;

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_allocation (
---------------------------------------------------------------------------------------------------
    allocation_id                   bigserial,
    primary_job_id                  bigint      not null,    
    secondary_job_id                int,
    ssd_file_system_name            text,
    launch_node_name                text        not null,
    isolated_cores                  int         default 0,
    user_flags                      text,
    system_flags                    text,
    ssd_min                         bigint,
    ssd_max                         bigint,
    num_nodes                       int         not null,
    num_processors                  int         not null,
    num_gpus                        int         not null,
    projected_memory                int         not null,
    state                           text        not null,
    type                            text        not null,
    job_type                        text        not null,
    user_name                       text        not null,
    user_id                         int         not null,
    user_group_id                   int         not null,
    user_group_name                 text,
    user_script                     text        not null,
    begin_time                      timestamp   not null,
    account                         text        not null,
    comment                         text,
    job_name                        text,
    job_submit_time                 timestamp   not null,
    queue                           text,
    requeue                         text,
    time_limit                      bigint      not null,
    wc_key                          text,
    smt_mode                        smallint    not null    default 0,
    core_blink                      boolean     not null    default FALSE,

    
    -- resource_comments            tbd     not null,
    -- health_check_allocation      tbd     not null,
    -- health_check_deallocation    tbd     not null,
    -- health_check_timeout         tbd     not null,

    PRIMARY KEY (allocation_id)
);

-------------------------------------------------
-- csm_allocation_indexes
-------------------------------------------------

-- automatically created index ON pkey (allocation_id);

-- CREATE UNIQUE INDEX uk_csm_allocation_b
--    on csm_allocation (primary_job_id, secondary_job_id);

-------------------------------------------------
-- csm_allocation_comments
-------------------------------------------------
    COMMENT ON TABLE csm_allocation is 'information about the system/s active allocations';
    COMMENT ON COLUMN csm_allocation.allocation_id is 'unique identifier for this allocation';
    COMMENT ON COLUMN csm_allocation.primary_job_id is 'primary job id (for lsf this will be the lsf job id)';
    COMMENT ON COLUMN csm_allocation.secondary_job_id is 'secondary job id (for lsf this will be the lsf job index for job arrays)';
    COMMENT ON COLUMN csm_allocation.ssd_file_system_name is 'the filesystem name that the user wants (ssd)';    
    COMMENT ON COLUMN csm_allocation.launch_node_name is 'launch node name';
    COMMENT ON COLUMN csm_allocation.isolated_cores is 'cgroup: 0 - No cgroups, 1 - Allocation Cgroup, 2 - Allocation and Core Isolation Cgroup, >2 || <0 unsupported';
    COMMENT ON COLUMN csm_allocation.user_flags is 'user space prolog/epilog flags';
    COMMENT ON COLUMN csm_allocation.system_flags is 'system space prolog/epilog flags';
    COMMENT ON COLUMN csm_allocation.ssd_min is 'minimum ssd size (in bytes) for this allocation';
    COMMENT ON COLUMN csm_allocation.ssd_max is 'maximum ssd size (in bytes) for this allocation';
    COMMENT ON COLUMN csm_allocation.num_nodes is 'number of nodes in this allocation,also see csm_node_allocation';
    COMMENT ON COLUMN csm_allocation.num_processors is 'total number of processes running in this allocation';
    COMMENT ON COLUMN csm_allocation.num_gpus is 'the number of gpus that are available';
    COMMENT ON COLUMN csm_allocation.projected_memory is 'the amount of memory available';
    COMMENT ON COLUMN csm_allocation.state is 'state can be: stage in allocation, running allocation, stage out allocation';
    COMMENT ON COLUMN csm_allocation.type is 'shared allocation, user managed sub-allocation, pmix managed allocation, pmix managed allocation with c groups for steps';
    COMMENT ON COLUMN csm_allocation.job_type is 'the type of job (batch or interactive)';
    COMMENT ON COLUMN csm_allocation.user_name is 'username';
    COMMENT ON COLUMN csm_allocation.user_id is 'user identification';
    COMMENT ON COLUMN csm_allocation.user_group_id is 'user group identification';    
    COMMENT ON COLUMN csm_allocation.user_group_name is 'user group name';    
    COMMENT ON COLUMN csm_allocation.user_script is 'user script information';
    COMMENT ON COLUMN csm_allocation.begin_time is 'timestamp when this allocation was created';
    COMMENT ON COLUMN csm_allocation.account is 'account the job ran under';
    COMMENT ON COLUMN csm_allocation.comment is 'comments for the allocation';
    COMMENT ON COLUMN csm_allocation.job_name is 'job name';
    COMMENT ON COLUMN csm_allocation.job_submit_time is 'the time and data stamp the job was submitted';
    COMMENT ON COLUMN csm_allocation.queue is 'identifies the partition (queue) on which the job ran';
    COMMENT ON COLUMN csm_allocation.requeue is 'identifies (requeue) if the allocation is requeued it will attempt to have the previous allocation id';
    COMMENT ON COLUMN csm_allocation.time_limit is 'the time limit requested or imposed on the job';
    COMMENT ON COLUMN csm_allocation.wc_key is 'arbitrary string for grouping orthogonal accounts together';
    COMMENT ON COLUMN csm_allocation.smt_mode is 'the smt mode of the allocation';
    COMMENT ON COLUMN csm_allocation.core_blink is 'flag indicating whether or not to run a blink operation on allocation cores.';
    COMMENT ON INDEX csm_allocation_pkey IS 'pkey index on allocation_id';
--  COMMENT ON INDEX uk_csm_allocation_b IS 'uniqueness on primary_job_id, secondary_job_id';
    COMMENT ON SEQUENCE csm_allocation_allocation_id_seq IS 'used to generate primary keys on allocation ids';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_allocation_state_history (
---------------------------------------------------------------------------------------------------
    history_time            timestamp       not null,
    allocation_id           bigint,
    exit_status             int,
    state                   text            not null,
    archive_history_time    timestamp
);

-------------------------------------------------
-- csm_allocation_state_history_indexes
-------------------------------------------------

CREATE INDEX ix_csm_allocation_state_history_a
    on csm_allocation_state_history (history_time);

CREATE INDEX ix_csm_allocation_state_history_b
    on csm_allocation_state_history (allocation_id);

CREATE INDEX ix_csm_allocation_state_history_c
    on csm_allocation_state_history (ctid);

CREATE INDEX ix_csm_allocation_state_history_d
    on csm_allocation_state_history (archive_history_time);

-------------------------------------------------
-- csm_allocation_state_history_comments
-------------------------------------------------
    COMMENT ON TABLE csm_allocation_state_history is 'state of the active allocations';
    COMMENT ON COLUMN csm_allocation_state_history.history_time is 'timestamp when this allocation changes state';
    COMMENT ON COLUMN csm_allocation_state_history.allocation_id is 'uniquely identify this allocation';
    COMMENT ON COLUMN csm_allocation_state_history.exit_status is 'the error code returned at the end of the allocation state';
    COMMENT ON COLUMN csm_allocation_state_history.state is 'state of this allocation (stage-in, running, stage-out)';
    COMMENT ON COLUMN csm_allocation_state_history.archive_history_time is 'timestamp when the history data has been archived and sent to: BDS, archive file, and or other';    
    COMMENT ON INDEX ix_csm_allocation_state_history_a IS 'index on history_time';
    COMMENT ON INDEX ix_csm_allocation_state_history_b IS 'index on allocation_id';
    COMMENT ON INDEX ix_csm_allocation_state_history_c IS 'index on ctid';
    COMMENT ON INDEX ix_csm_allocation_state_history_d IS 'index on archive_history_time';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_allocation_history (
---------------------------------------------------------------------------------------------------
    history_time                    timestamp,   --not null,
    allocation_id                   bigint,
    primary_job_id                  bigint      not null,
    secondary_job_id                int,
    ssd_file_system_name            text,
    launch_node_name                text        not null,
    isolated_cores                  int,
    user_flags                      text,
    system_flags                    text,
    ssd_min                         bigint,
    ssd_max                         bigint,
    num_nodes                       int         not null,
    num_processors                  int         not null,
    num_gpus                        int         not null,
    projected_memory                int         not null,
    state                           text        not null,
    type                            text        not null,
    job_type                        text        not null,
    user_name                       text        not null,
    user_id                         int         not null,
    user_group_id                   int         not null,
    user_group_name                 text,
    user_script                     text        not null,
    begin_time                      timestamp   not null,
    end_time                        timestamp,   --not null,
    exit_status                     int,         --not null,
    account                         text        not null,
    comment                         text,
    job_name                        text,
    job_submit_time                 timestamp   not null,
    queue                           text,
    requeue                         text,
    time_limit                      bigint      not null,
    wc_key                          text,
    archive_history_time            timestamp,
    smt_mode                        smallint,
    core_blink                      boolean

);

-------------------------------------------------
-- csm_allocation_history_indexes
-------------------------------------------------

CREATE INDEX ix_csm_allocation_history_a
    on csm_allocation_history (history_time);

CREATE INDEX ix_csm_allocation_history_b
    on csm_allocation_history (allocation_id);

CREATE INDEX ix_csm_allocation_history_c
    on csm_allocation_history (ctid);

CREATE INDEX ix_csm_allocation_history_d
    on csm_allocation_history (archive_history_time);

-------------------------------------------------
-- csm_allocation_history_comments
-------------------------------------------------
    COMMENT ON TABLE csm_allocation_history is 'historical information about allocations.';
    COMMENT ON COLUMN csm_allocation_history.history_time is 'time when the allocation is entered into the history table';
    COMMENT ON COLUMN csm_allocation_history.allocation_id is 'unique identifier for this allocation';
    COMMENT ON COLUMN csm_allocation_history.primary_job_id is 'primary job id (for lsf this will be the lsf job id)';
    COMMENT ON COLUMN csm_allocation_history.secondary_job_id is 'secondary job id (for lsf this will be the lsf job index)';
    COMMENT ON COLUMN csm_allocation_history.ssd_file_system_name is 'the filesystem name that the user wants (ssd)';
    COMMENT ON COLUMN csm_allocation_history.launch_node_name is 'launch node name';
    COMMENT ON COLUMN csm_allocation_history.isolated_cores is 'cgroup: 0 - No cgroups, 1 - Allocation Cgroup, 2 - Allocation and Core Isolation Cgroup, >2 || <0 unsupported';
    COMMENT ON COLUMN csm_allocation_history.user_flags is 'user space prolog/epilog flags';
    COMMENT ON COLUMN csm_allocation_history.system_flags is 'system space prolog/epilog flags';
    COMMENT ON COLUMN csm_allocation_history.ssd_min is 'minimum ssd size (in bytes) for this allocation';
    COMMENT ON COLUMN csm_allocation_history.ssd_max is 'maximum ssd size (in bytes) for this allocation';
    COMMENT ON COLUMN csm_allocation_history.num_nodes is 'number of nodes in allocation, see csm_node_allocation';
    COMMENT ON COLUMN csm_allocation_history.num_processors is 'total number of processes running in this allocation';
    COMMENT ON COLUMN csm_allocation_history.num_gpus is 'the number of gpus that are available';
    COMMENT ON COLUMN csm_allocation_history.projected_memory is 'the amount of memory available';
    COMMENT ON COLUMN csm_allocation_history.state is 'state of the node - stage in allocation, running allocation, stage out allocation';
    COMMENT ON COLUMN csm_allocation_history.type is 'user managed sub-allocation, pmix managed allocation, pmix managed allocation with c groups for steps';
    COMMENT ON COLUMN csm_allocation_history.job_type is 'the type of job (batch or interactive)';
    COMMENT ON COLUMN csm_allocation_history.user_name is 'username';
    COMMENT ON COLUMN csm_allocation_history.user_id is 'user identification id';
    COMMENT ON COLUMN csm_allocation_history.user_group_id is 'user group identification';
    COMMENT ON COLUMN csm_allocation_history.user_group_name is 'user group name';
    COMMENT ON COLUMN csm_allocation_history.user_script is 'user script information';
    COMMENT ON COLUMN csm_allocation_history.begin_time is 'timestamp when this allocation was created';
    COMMENT ON COLUMN csm_allocation_history.end_time is 'timestamp when this allocation was freed';
    COMMENT ON COLUMN csm_allocation_history.exit_status is 'allocation exit status';
    COMMENT ON COLUMN csm_allocation_history.account is 'account the job ran under';
    COMMENT ON COLUMN csm_allocation_history.comment is 'comments for the allocation';
    COMMENT ON COLUMN csm_allocation_history.job_name is 'job name';
    COMMENT ON COLUMN csm_allocation_history.job_submit_time is 'the time and date stamp the job was submitted';
    COMMENT ON COLUMN csm_allocation_history.queue is 'identifies the partition (queue) on which the job ran';
    COMMENT ON COLUMN csm_allocation_history.requeue is 'identifies (requeue) if the allocation is requeued it will attempt to have the previous allocation id';
    COMMENT ON COLUMN csm_allocation_history.time_limit is 'the time limit requested or imposed on the job';
    COMMENT ON COLUMN csm_allocation_history.wc_key is 'arbitrary string for grouping orthogonal accounts together';
    COMMENT ON COLUMN csm_allocation_history.archive_history_time is 'timestamp when the history data has been archived and sent to: BDS, archive file, and or other';    
    COMMENT ON COLUMN csm_allocation_history.smt_mode is 'the smt mode of the allocation';
    COMMENT ON COLUMN csm_allocation_history.core_blink is 'flag indicating whether or not to run a blink operation on allocation cores.';
    COMMENT ON INDEX ix_csm_allocation_history_a IS 'index on history_time';
    COMMENT ON INDEX ix_csm_allocation_history_b IS 'index on allocation_id';
    COMMENT ON INDEX ix_csm_allocation_history_c IS 'index on ctid';
    COMMENT ON INDEX ix_csm_allocation_history_d IS 'index on archive_history_time';

---------------------------------------------------------------------------------------------------
-- The different states a CSM node may be in.
---------------------------------------------------------------------------------------------------
CREATE TYPE compute_node_states AS ENUM (
---------------------------------------------------------------------------------------------------
  'DISCOVERED',
  'IN_SERVICE',
  'ADMIN_RESERVED',
  'MAINTENANCE',
  'SOFT_FAILURE',
  'OUT_OF_SERVICE',
  'HARD_FAILURE'
);

-----------------------------------------------------------
-- compute_node_states TYPE comments
-----------------------------------------------------------

COMMENT ON TYPE compute_node_states IS 'compute_node_states type to help identify the states of the compute nodes';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_node (
---------------------------------------------------------------------------------------------------
    node_name                       text,               -- not null,
    machine_model                   text,
    serial_number                   text,
    collection_time                 timestamp,          -- not null,
    update_time                     timestamp,          -- not null,
    state                           compute_node_states,-- not null,
    type                            text,               -- not null,
    primary_agg                     text,
    secondary_agg                   text,
    hard_power_cap                  int,                -- not null,
    installed_memory                bigint,             -- not null,
    installed_swap                  bigint,             -- not null,
    discovered_sockets              int,                -- not null,
    discovered_cores                int,                -- not null,
    discovered_gpus                 int,                -- not null,
    discovered_hcas                 int,
    discovered_dimms                int,
    discovered_ssds                 int,
    os_image_name                   text, 
    os_image_uuid                   text, 
    kernel_release                  text,               -- not null,
    kernel_version                  text,               -- not null,
    physical_frame_location         text,
    physical_u_location             text,
    feature_1                       text,
    feature_2                       text,
    feature_3                       text,
    feature_4                       text,
    comment                         text,
    PRIMARY KEY (node_name),
    CONSTRAINT csm_not_null_string CHECK (node_name <> ''),
    CONSTRAINT csm_not_blank CHECK (trim(' ' from node_name) <> '')
);

-------------------------------------------------
-- csm_node_index
-------------------------------------------------

-- automatically created index ON pkey (node_name)

CREATE INDEX ix_csm_node_a
    on csm_node (node_name, state);

-------------------------------------------------
-- csm_node_comments
-------------------------------------------------
    COMMENT ON TABLE csm_node is 'logical extension of the xcat node table';    
    COMMENT ON COLUMN csm_node.node_name is 'identifies which node this information is for';
    COMMENT ON COLUMN csm_node.machine_model is 'machine type model information for this node';
    COMMENT ON COLUMN csm_node.serial_number is 'witherspoon boards serial number';
    COMMENT ON COLUMN csm_node.collection_time is 'the time the node was collected at inventory';
    COMMENT ON COLUMN csm_node.update_time is 'the time the node was updated';
    COMMENT ON COLUMN csm_node.state is 'state of the node - DISCOVERED, IN_SERVICE, ADMIN_RESERVED, MAINTENANCE, SOFT_FAILURE, OUT_OF_SERVICE, HARD_FAILURE';
    COMMENT ON COLUMN csm_node.type is 'management, service, login, workload manager, launch, compute';
    COMMENT ON COLUMN csm_node.primary_agg is 'primary aggregate';
    COMMENT ON COLUMN csm_node.secondary_agg is 'secondary aggregate';
    COMMENT ON COLUMN csm_node.hard_power_cap is 'hard power cap for this node';
    COMMENT ON COLUMN csm_node.installed_memory is 'amount of installed memory on this node (in kB)';
    COMMENT ON COLUMN csm_node.installed_swap is 'amount of available swap space on this node (in kB)';
    COMMENT ON COLUMN csm_node.discovered_sockets is 'number of processors on this node (processor sockets, non-uniform memory access (NUMA) nodes)';
    COMMENT ON COLUMN csm_node.discovered_cores is 'number of physical cores on this node from all processors';
    COMMENT ON COLUMN csm_node.discovered_gpus is 'number of gpus available';
    COMMENT ON COLUMN csm_node.discovered_hcas is 'number of IB HCAs discovered in this node during the most recent inventory collection';    
    COMMENT ON COLUMN csm_node.discovered_dimms is 'number of dimms discovered in this node during the most recent inventory collection';    
    COMMENT ON COLUMN csm_node.discovered_ssds is 'number of ssds discovered in this node during the most recent inventory collection';    
    COMMENT ON COLUMN csm_node.os_image_name is 'xCAT os image name being run on this node, diskless images only';
    COMMENT ON COLUMN csm_node.os_image_uuid is 'xCAT os image uuid being run on this node, diskless images only';
    COMMENT ON COLUMN csm_node.kernel_release is 'kernel release being run on this node';
    COMMENT ON COLUMN csm_node.kernel_version is 'linux kernel version being run on this node';
    COMMENT ON COLUMN csm_node.physical_frame_location is 'physical frame number where the node is located';
    COMMENT ON COLUMN csm_node.physical_u_location is 'physical u location (position in the frame) where the node is located';
    COMMENT ON COLUMN csm_node.feature_1 is 'reserved fields for future use';
    COMMENT ON COLUMN csm_node.feature_2 is 'reserved fields for future use';
    COMMENT ON COLUMN csm_node.feature_3 is 'reserved fields for future use';
    COMMENT ON COLUMN csm_node.feature_4 is 'reserved fields for future use';
    COMMENT ON COLUMN csm_node.comment is 'comment field for system administrators';
    COMMENT ON INDEX csm_node_pkey IS 'pkey index on node_name';
    COMMENT ON INDEX ix_csm_node_a IS 'index on node_name, ready';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_node_history (
---------------------------------------------------------------------------------------------------
    history_time                    timestamp,          -- not null,
    node_name                       text,               -- not null,
    machine_model                   text,
    serial_number                   text,               -- not null,
    collection_time                 timestamp,          -- not null,
    update_time                     timestamp,          -- not null,
    state                           compute_node_states,-- not null,
    type                            text,               -- not null,
    primary_agg                     text,
    secondary_agg                   text,
    hard_power_cap                  int,                -- not null,
    installed_memory                bigint,             -- not null,
    installed_swap                  bigint,             -- not null,
    discovered_sockets              int,                -- not null,
    discovered_cores                int,                -- not null,
    discovered_gpus                 int,                -- not null,
    discovered_hcas                 int,
    discovered_dimms                int,
    discovered_ssds                 int,
    os_image_name                   text, 
    os_image_uuid                   text, 
    kernel_release                  text,               -- not null,
    kernel_version                  text,               -- not null,
    physical_frame_location         text,
    physical_u_location             text,
    feature_1                       text,
    feature_2                       text,
    feature_3                       text,
    feature_4                       text,
    comment                         text,
    operation                       char(1)                not null,
    archive_history_time            timestamp
);

-------------------------------------------------
-- csm_node_history_indexes
-------------------------------------------------

CREATE INDEX ix_csm_node_history_a
    on csm_node_history (history_time);

CREATE INDEX ix_csm_node_history_b
    on csm_node_history (node_name);

CREATE INDEX ix_csm_node_history_c
    on csm_node_history (ctid);

CREATE INDEX ix_csm_node_history_d
    on csm_node_history (archive_history_time);

-------------------------------------------------
-- csm_node_history_comments
-------------------------------------------------
    COMMENT ON TABLE csm_node_history is 'historical information related to node attributes';
    COMMENT ON COLUMN csm_node_history.history_time is 'time when the node is entered into the history table';
    COMMENT ON COLUMN csm_node_history.node_name is 'identifies which node this information is for';
    COMMENT ON COLUMN csm_node_history.machine_model is 'machine type model information for this node';
    COMMENT ON COLUMN csm_node_history.serial_number is 'witherspoon boards serial number';
    COMMENT ON COLUMN csm_node_history.collection_time is 'the time the node was collected at inventory';
    COMMENT ON COLUMN csm_node_history.update_time is 'the time the node was updated';
    COMMENT ON COLUMN csm_node_history.state is 'state of the node - DISCOVERED, IN_SERVICE, ADMIN_RESERVED, MAINTENANCE, SOFT_FAILURE, OUT_OF_SERVICE, HARD_FAILURE';
    COMMENT ON COLUMN csm_node_history.type is 'management, service, login, workload manager, launch, compute';
    COMMENT ON COLUMN csm_node_history.primary_agg is 'primary aggregate';
    COMMENT ON COLUMN csm_node_history.secondary_agg is 'secondary aggregate';
    COMMENT ON COLUMN csm_node_history.hard_power_cap is 'hard power cap for this node';
    COMMENT ON COLUMN csm_node_history.installed_memory is 'amount of installed memory on this node (in kB)';
    COMMENT ON COLUMN csm_node_history.installed_swap is 'amount of available swap space on this node (in kB)';
    COMMENT ON COLUMN csm_node_history.discovered_sockets is 'number of processors on this node (processor sockets, non-uniform memory access (NUMA) nodes)';
    COMMENT ON COLUMN csm_node_history.discovered_cores is 'number of physical cores on this node from all processors';
    COMMENT ON COLUMN csm_node_history.discovered_gpus is 'number of gpus available';
    COMMENT ON COLUMN csm_node_history.discovered_hcas is 'number of IB HCAs discovered in this node during inventory collection';
    COMMENT ON COLUMN csm_node_history.discovered_dimms is 'number of dimms discovered in this node during inventory collection';
    COMMENT ON COLUMN csm_node_history.discovered_ssds is 'number of ssds discovered in this node during inventory collection';
    COMMENT ON COLUMN csm_node_history.os_image_name is 'xCAT os image name being run on this node, diskless images only';
    COMMENT ON COLUMN csm_node_history.os_image_uuid is 'xCAT os image uuid being run on this node, diskless images only';
    COMMENT ON COLUMN csm_node_history.kernel_release is 'linux kernel release being run on this node';
    COMMENT ON COLUMN csm_node_history.kernel_version is 'linux kernel version being run on this node';
    COMMENT ON COLUMN csm_node_history.physical_frame_location is 'physical frame number where the node is located';
    COMMENT ON COLUMN csm_node_history.physical_u_location is 'physical u location (position in the frame) where the node is located';
    COMMENT ON COLUMN csm_node_history.feature_1 is 'reserved fields for future use';
    COMMENT ON COLUMN csm_node_history.feature_2 is 'reserved fields for future use';
    COMMENT ON COLUMN csm_node_history.feature_3 is 'reserved fields for future use';
    COMMENT ON COLUMN csm_node_history.feature_4 is 'reserved fields for future use';
    COMMENT ON COLUMN csm_node_history.comment is 'comment field for system administrators';
    COMMENT ON COLUMN csm_node_history.operation is 'operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)';
    COMMENT ON COLUMN csm_node_history.archive_history_time is 'timestamp when the history data has been archived and sent to: BDS, archive file, and or other';
    COMMENT ON INDEX ix_csm_node_history_a IS 'index on history_time';
    COMMENT ON INDEX ix_csm_node_history_b IS 'index on node_name';
    COMMENT ON INDEX ix_csm_node_history_c IS 'index on ctid';
    COMMENT ON INDEX ix_csm_node_history_d IS 'index on archive_history_time';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_node_state_history (
---------------------------------------------------------------------------------------------------
    history_time                    timestamp,          -- not null,
    node_name                       text,               -- not null,
    state                           compute_node_states,-- not null,
    operation                       char(1)                not null,
    archive_history_time            timestamp
);
-------------------------------------------------
-- csm_node_state_history_indexes
-------------------------------------------------

CREATE INDEX ix_csm_node_state_history_a
    on csm_node_state_history (history_time);

CREATE INDEX ix_csm_node_state_history_b
    on csm_node_state_history (node_name, state);

CREATE INDEX ix_csm_node_state_history_c
    on csm_node_state_history (ctid);

CREATE INDEX ix_csm_node_state_history_d
    on csm_node_state_history (archive_history_time);

-------------------------------------------------
-- csm_node_state_history_comments
-------------------------------------------------
    COMMENT ON TABLE csm_node_state_history is 'historical information related to node state';
    COMMENT ON COLUMN csm_node_state_history.history_time is 'time when the node ready status is entered into the history table';
    COMMENT ON COLUMN csm_node_state_history.node_name is 'identifies which node this information is for';
    COMMENT ON COLUMN csm_node_state_history.state is 'state of the node - DISCOVERED, IN_SERVICE, ADMIN_RESERVED, MAINTENANCE, SOFT_FAILURE, OUT_OF_SERVICE, HARD_FAILURE';
    COMMENT ON COLUMN csm_node_state_history.operation is 'operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)';
    COMMENT ON COLUMN csm_node_state_history.archive_history_time is 'timestamp when the history data has been archived and sent to: BDS, archive file, and or other';
    COMMENT ON INDEX ix_csm_node_state_history_a IS 'index on history_time';
    COMMENT ON INDEX ix_csm_node_state_history_b IS 'index on node_name, state';
    COMMENT ON INDEX ix_csm_node_state_history_c IS 'index on ctid';
    COMMENT ON INDEX ix_csm_node_state_history_d IS 'index on archive_history_time';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_allocation_node (
---------------------------------------------------------------------------------------------------
    allocation_id           bigint      not null        references csm_allocation(allocation_id),
    node_name               text        not null        references csm_node(node_name),
    state                   text        not null,
    shared                  boolean     not null,
--  status                  text        not null, (This is referenced issue #302 for Beta 2, to keep track of allocation create, delete, and update)
    energy                  bigint,
    gpfs_read               bigint,
    gpfs_write              bigint,
    ib_tx                   bigint,
    ib_rx                   bigint,
    power_cap               int,        -- not null,
    power_shifting_ratio    int,        -- not null,
    power_cap_hit           bigint,
    gpu_usage               bigint,
    gpu_energy              bigint,
    cpu_usage               bigint,
    memory_usage_max        bigint
);

-------------------------------------------------
-- csm_allocation_node_index
-------------------------------------------------

CREATE INDEX ix_csm_allocation_node_a 
    on csm_allocation_node (allocation_id);

CREATE UNIQUE INDEX uk_csm_allocation_node_b 
    on csm_allocation_node (allocation_id, node_name); --WHERE shared IS NOT NULL;

-------------------------------------------------
-- csm_allocation_node_comments
-------------------------------------------------
    COMMENT ON TABLE csm_allocation_node is 'maps active allocations to the compute nodes that make up the allocation';     
    COMMENT ON COLUMN csm_allocation_node.allocation_id is 'allocation that node_name is part of';
    COMMENT ON COLUMN csm_allocation_node.node_name is 'identifies which node this is';
    COMMENT ON COLUMN csm_allocation_node.state is 'state can be: stage in allocation, running allocation, stage out allocation';
    COMMENT ON COLUMN csm_allocation_node.shared is 'indicates if the node resources are shareable';
--  COMMENT ON COLUMN csm_allocation_node.status is 'status of allocation (protect against master daemon crashing.(creating, created, updating, updated, deleting, deleted).';
    COMMENT ON COLUMN csm_allocation_node.energy is 'the total energy used by the node in joules during the allocation';
    COMMENT ON COLUMN csm_allocation_node.gpfs_read is 'bytes read counter (net) at the start of the allocation.';
    COMMENT ON COLUMN csm_allocation_node.gpfs_write is 'bytes written counter (net) at the start of the allocation.';
    COMMENT ON COLUMN csm_allocation_node.ib_tx is 'count of data octets transmitted on all port VLs (1/4 of a byte) at the start of the allocation.';
    COMMENT ON COLUMN csm_allocation_node.ib_rx is 'Count of data octets received on all port VLs (1/4 of a byte) at the start of the allocation.';
    COMMENT ON COLUMN csm_allocation_node.power_cap is 'power cap currently in effect for this node (in watts)';
    COMMENT ON COLUMN csm_allocation_node.power_shifting_ratio is 'power power shifting ratio currently in effect for this node';
    COMMENT ON COLUMN csm_allocation_node.power_cap_hit is 'total number of windowed ticks the processor frequency was reduced';
    COMMENT ON COLUMN csm_allocation_node.power_shifting_ratio is 'power power shifting ratio currently in effect for this node';
    COMMENT ON COLUMN csm_allocation_node.gpu_usage is 'the total usage aggregated across all GPUs in the node in microseconds during the allocation';
    COMMENT ON COLUMN csm_allocation_node.gpu_energy is 'the total energy used across all GPUs in the node in joules during the allocation';
    COMMENT ON COLUMN csm_allocation_node.cpu_usage is 'the cpu usage in nanoseconds';
    COMMENT ON COLUMN csm_allocation_node.memory_usage_max is 'The high water mark for memory usage (bytes).';
    COMMENT ON INDEX ix_csm_allocation_node_a IS 'index on allocation_id';
    COMMENT ON INDEX uk_csm_allocation_node_b IS 'uniqueness on allocation_id, node_name';
    
---------------------------------------------------------------------------------------------------
CREATE TABLE csm_allocation_node_history (
---------------------------------------------------------------------------------------------------
    history_time                timestamp   not null,
    allocation_id               bigint      not null,
    node_name                   text        not null,
    state                       text        not null,
    shared                      boolean,
--  status                      text        not null, (This is referenced issue #302 for Beta 2, to keep track of allocation create, delete, and update)
    energy                      bigint,
    gpfs_read                   bigint,
    gpfs_write                  bigint,
    ib_tx                       bigint,
    ib_rx                       bigint,
    power_cap                   int,        -- not null,
    power_shifting_ratio        int,        -- not null,
    power_cap_hit               bigint,
    gpu_usage                   bigint,
    gpu_energy                  bigint,
    cpu_usage                   bigint,
    memory_usage_max            bigint,
    archive_history_time        timestamp
);

-------------------------------------------------
-- csm_allocation_node_history_index
-------------------------------------------------

CREATE INDEX ix_csm_allocation_node_history_a
    on csm_allocation_node_history (history_time);

CREATE INDEX ix_csm_allocation_node_history_b
    on csm_allocation_node_history (allocation_id);

CREATE INDEX ix_csm_allocation_node_history_c
    on csm_allocation_node_history (ctid);

CREATE INDEX ix_csm_allocation_node_history_d
    on csm_allocation_node_history (archive_history_time);

-------------------------------------------------
-- csm_allocation_node_history_comments
-------------------------------------------------
    COMMENT ON TABLE csm_allocation_node_history is 'historical information that mapped allocations to the compute nodes that made up the allocation';
    COMMENT ON COLUMN csm_allocation_node_history.history_time is 'timestamp when it enters the history table';
    COMMENT ON COLUMN csm_allocation_node_history.allocation_id is 'allocation that node_name is part of';
    COMMENT ON COLUMN csm_allocation_node_history.node_name is 'identifies which node this is';
    COMMENT ON COLUMN csm_allocation_node_history.state is 'state can be: stage in allocation, running allocation, stage out allocation';
    COMMENT ON COLUMN csm_allocation_node_history.shared is 'indicates if the node resources are shareable';
--  COMMENT ON COLUMN csm_allocation_node_history.status is 'status of allocation (protect against master daemon crashing.(creating, created, updating, updated, deleting, deleted).';
    COMMENT ON COLUMN csm_allocation_node_history.energy is 'the total energy used by the node in joules during the allocation';
    COMMENT ON COLUMN csm_allocation_node_history.gpfs_read is 'total bytes read counter (net) at the during the allocation. Negative values represent the start reading, indicating the end was never writen to the database.';
    COMMENT ON COLUMN csm_allocation_node_history.gpfs_write is 'total bytes written counter (net) at the during the allocation. Negative values represent the start reading, indicating the end was never writen to the database.';
    COMMENT ON COLUMN csm_allocation_node_history.ib_tx is 'total count of data octets transmitted on all port VLs (1/4 of a byte) during the allocation. Negative values represent the start reading, indicating the end was never writen to the database.';
    COMMENT ON COLUMN csm_allocation_node_history.ib_rx is 'total count of data octets received on all port VLs (1/4 of a byte) during the allocation. Negative values represent the start reading, indicating the end was never writen to the database.';
    COMMENT ON COLUMN csm_allocation_node_history.power_cap is 'power cap currently in effect for this node (in watts)';
    COMMENT ON COLUMN csm_allocation_node_history.power_shifting_ratio is 'power power shifting ratio currently in effect for this node';
    COMMENT ON COLUMN csm_allocation_node_history.power_cap_hit is 'total number of windowed ticks the processor frequency was reduced';
    COMMENT ON COLUMN csm_allocation_node_history.gpu_usage is 'the total usage aggregated across all GPUs in the node in microseconds during the allocation';
    COMMENT ON COLUMN csm_allocation_node_history.gpu_energy is 'the total energy used across all GPUs in the node in joules during the allocation';
    COMMENT ON COLUMN csm_allocation_node_history.cpu_usage is 'the cpu usage in nanoseconds';
    COMMENT ON COLUMN csm_allocation_node_history.memory_usage_max is 'The high water mark for memory usage (bytes).';
    COMMENT ON COLUMN csm_allocation_node_history.archive_history_time is 'timestamp when the history data has been archived and sent to: BDS, archive file, and or other';
    COMMENT ON INDEX ix_csm_allocation_node_history_a IS 'index on history_time';
    COMMENT ON INDEX ix_csm_allocation_node_history_b IS 'index on allocation_id';
    COMMENT ON INDEX ix_csm_allocation_node_history_c IS 'index on ctid';
    COMMENT ON INDEX ix_csm_allocation_node_history_d IS 'index on archive_history_time';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_diag_run (
---------------------------------------------------------------------------------------------------
    run_id              bigint      not null,
    allocation_id       bigint,
    begin_time          timestamp   not null    default current_timestamp,
    status              char(16)    not null    default 'RUNNING',
    inserted_ras        boolean     not null    default false,
    log_dir             text        not null,
    cmd_line            text,
    PRIMARY KEY (run_id)
);

-------------------------------------------------
-- csm_diag_run_index
-------------------------------------------------

-- automatically created index ON pkey (run_id)

-------------------------------------------------
-- csm_diag_run_comments
-------------------------------------------------
    COMMENT ON TABLE csm_diag_run is 'information about each of the diagnostic run';
    COMMENT ON COLUMN csm_diag_run.run_id is 'diagnostic/s run id';
    COMMENT ON COLUMN csm_diag_run.allocation_id is 'allocation that this diag_run is part of';
    COMMENT ON COLUMN csm_diag_run.begin_time is 'this is when the diagnostic run begins';
    COMMENT ON COLUMN csm_diag_run.status is 'diagnostic/s status (RUNNING,COMPLETED,FAILED,CANCELED,COMPLETED_FAIL)';
    COMMENT ON COLUMN csm_diag_run.inserted_ras is 'inserted diagnostic ras events t/f';
    COMMENT ON COLUMN csm_diag_run.log_dir is 'location of diagnostic/s log files';
    COMMENT ON COLUMN csm_diag_run.cmd_line is 'how diagnostic program was invoked: program and arguments';
    COMMENT ON INDEX csm_diag_run_pkey IS 'pkey index on run_id';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_diag_run_history (
---------------------------------------------------------------------------------------------------
    history_time            timestamp   not null,
    run_id                  bigint      not null,
    allocation_id           bigint,
    begin_time              timestamp   not null,
    end_time                timestamp,
    status                  char(16)    not null,
    inserted_ras            boolean     not null,
    log_dir                 text        not null,
    cmd_line                text,
    archive_history_time    timestamp
);

-------------------------------------------------
-- csm_diag_run_history_index
-------------------------------------------------

CREATE INDEX ix_csm_diag_run_history_a
    on csm_diag_run_history (history_time);

CREATE INDEX ix_csm_diag_run_history_b
    on csm_diag_run_history (run_id);

CREATE INDEX ix_csm_diag_run_history_c
    on csm_diag_run_history (allocation_id);

CREATE INDEX ix_csm_diag_run_history_d
    on csm_diag_run_history (ctid);

CREATE INDEX ix_csm_diag_run_history_e
    on csm_diag_run_history (archive_history_time);

-------------------------------------------------
-- csm_diag_run_history comments
-------------------------------------------------
    COMMENT ON TABLE csm_diag_run_history is 'historical information about each of the diagnostic runs';
    COMMENT ON COLUMN csm_diag_run_history.history_time is 'timestamp when it enters the history table';
    COMMENT ON COLUMN csm_diag_run_history.run_id is 'diagnostic/s run id';
    COMMENT ON COLUMN csm_diag_run_history.allocation_id is 'allocation that this diag_run is part of';
    COMMENT ON COLUMN csm_diag_run_history.begin_time is 'this is when the diagnostic run begins';
    COMMENT ON COLUMN csm_diag_run_history.end_time is 'this is when the diagnostic run ends';
    COMMENT ON COLUMN csm_diag_run_history.status is 'diagnostic/s status (RUNNING,COMPLETED,FAILED,CANCELED,COMPLETED_FAIL)';
    COMMENT ON COLUMN csm_diag_run_history.inserted_ras is 'inserted diagnostic ras events t/f';
    COMMENT ON COLUMN csm_diag_run_history.log_dir is 'location of diagnostic/s log files';
    COMMENT ON COLUMN csm_diag_run_history.cmd_line is 'how diagnostic program was invoked: program and arguments';
    COMMENT ON COLUMN csm_diag_run_history.archive_history_time is 'timestamp when the history data has been archived and sent to: BDS, archive file, and or other';
    COMMENT ON INDEX ix_csm_diag_run_history_a IS 'index on history_time';
    COMMENT ON INDEX ix_csm_diag_run_history_b IS 'index on run_id';
    COMMENT ON INDEX ix_csm_diag_run_history_c IS 'index on allocation_id';
    COMMENT ON INDEX ix_csm_diag_run_history_d IS 'index on ctid';
    COMMENT ON INDEX ix_csm_diag_run_history_e IS 'index on archive_history_time';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_diag_result (
---------------------------------------------------------------------------------------------------
    run_id              bigint      references csm_diag_run(run_id),
    test_name           text        not null,
    node_name           text        not null,
    serial_number       text,
    begin_time          timestamp,   
    end_time            timestamp   default current_timestamp,
    status              char(16)    default 'unknown',
    log_file            text
);

-------------------------------------------------
-- csm_diag_result_index
-------------------------------------------------

CREATE INDEX ix_csm_diag_result_a
    on csm_diag_result (run_id, test_name, node_name);

-------------------------------------------------
-- csm_diag_result_comments
-------------------------------------------------

    COMMENT ON TABLE csm_diag_result is 'result of a specific instance of a diagnostic';
    COMMENT ON COLUMN csm_diag_result.run_id is 'diagnostic/s run id';
    COMMENT ON COLUMN csm_diag_result.test_name is 'the name of the specific testcase';    
    COMMENT ON COLUMN csm_diag_result.node_name is 'identifies which node';
    COMMENT ON COLUMN csm_diag_result.serial_number is 'serial number of the field replaceable unit (fru) that this diagnostic was run against';
    COMMENT ON COLUMN csm_diag_result.begin_time is 'the time when the task begins';
    COMMENT ON COLUMN csm_diag_result.end_time is 'the time when the task is complete';
    COMMENT ON COLUMN csm_diag_result.status is 'test status after the diagnostic finishes (pass, fail, completed_fail)';
    COMMENT ON COLUMN csm_diag_result.log_file is 'location of diagnostic/s log file';
    COMMENT ON INDEX ix_csm_diag_result_a IS 'index on run_id, test_name, node_name';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_diag_result_history (
---------------------------------------------------------------------------------------------------
    history_time            timestamp   not null,
    run_id                  bigint,
    test_name               text        not null,
    node_name               text        not null,
    serial_number           text,
    begin_time              timestamp,   
    end_time                timestamp   default current_timestamp,
    status                  char(16)    default 'unknown',
    log_file                text,
    archive_history_time    timestamp
);

-------------------------------------------------
-- csm_diag_result_history_index
-------------------------------------------------

CREATE INDEX ix_csm_diag_result_history_a
    on csm_diag_result_history (history_time);

CREATE INDEX ix_csm_diag_result_history_b
    on csm_diag_result_history (run_id);

CREATE INDEX ix_csm_diag_result_history_c
    on csm_diag_result_history (ctid);

CREATE INDEX ix_csm_diag_result_history_d
    on csm_diag_result_history (archive_history_time);

-------------------------------------------------
-- csm_diag_result_history_comments
-------------------------------------------------

    COMMENT ON TABLE csm_diag_result_history is 'historical result of a specific instance of a diagnostic';
    COMMENT ON COLUMN csm_diag_result_history.history_time is 'timestamp when it enters the history table';
    COMMENT ON COLUMN csm_diag_result_history.run_id is 'diagnostic/s run id';
    COMMENT ON COLUMN csm_diag_result_history.test_name is 'the name of the specific testcase';    
    COMMENT ON COLUMN csm_diag_result_history.node_name is 'identifies which node';
    COMMENT ON COLUMN csm_diag_result_history.serial_number is 'serial number of the field replaceable unit (fru) that this diagnostic was run against';
    COMMENT ON COLUMN csm_diag_result_history.begin_time is 'the time when the task begins';
    COMMENT ON COLUMN csm_diag_result_history.end_time is 'the time when the task is complete';
    COMMENT ON COLUMN csm_diag_result_history.status is 'test status after the diagnostic finishes (pass, fail, completed_fail)';
    COMMENT ON COLUMN csm_diag_result_history.log_file is 'location of diagnostic/s log file';
    COMMENT ON COLUMN csm_diag_result_history.archive_history_time is 'timestamp when the history data has been archived and sent to: BDS, archive file, and or other';
    COMMENT ON INDEX ix_csm_diag_result_history_a IS 'index on history_time';
    COMMENT ON INDEX ix_csm_diag_result_history_b IS 'index on run_id';
    COMMENT ON INDEX ix_csm_diag_result_history_c IS 'index on ctid';
    COMMENT ON INDEX ix_csm_diag_result_history_d IS 'index on archive_history_time';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_step (
---------------------------------------------------------------------------------------------------
    step_id                 bigint          not null,
    allocation_id           bigint          not null,
    begin_time              timestamp       not null,
    status                  text            not null,
    executable              text            not null,
    working_directory       text            not null,
    argument                text            not null,
    environment_variable    text            not null,
    num_nodes               int             not null,
    num_processors          int             not null,
    num_gpus                int             not null,
    projected_memory        int             not null,
    num_tasks               int             not null,
    user_flags              text,
    PRIMARY KEY (step_id, allocation_id),
    FOREIGN KEY (allocation_id) references csm_allocation(allocation_id)
);

-------------------------------------------------
-- csm_step_index
-------------------------------------------------

-- automatically created index ON pkey (step_id)

CREATE UNIQUE INDEX uk_csm_step_a
    on csm_step (step_id, allocation_id);

-------------------------------------------------
-- csm_step_comments
-------------------------------------------------
    COMMENT ON TABLE csm_step is 'information on active steps';
    COMMENT ON COLUMN csm_step.step_id is 'uniquely identify this step';    
    COMMENT ON COLUMN csm_step.allocation_id is 'allocation that this step is part of';
    COMMENT ON COLUMN csm_step.begin_time is 'timestamp when this job step started';
    COMMENT ON COLUMN csm_step.status is 'the active status of the step';
    COMMENT ON COLUMN csm_step.executable is 'executable / command name / application name';
    COMMENT ON COLUMN csm_step.working_directory is 'working directory';
    COMMENT ON COLUMN csm_step.argument is 'arguments / parameters';
    COMMENT ON COLUMN csm_step.environment_variable is 'environment variables';
    COMMENT ON COLUMN csm_step.num_nodes is 'the specific number of nodes that are involved in the step';    
    COMMENT ON COLUMN csm_step.num_processors is 'total number of processes running in this step';
    COMMENT ON COLUMN csm_step.num_gpus is 'the number of gpus that are available';
    COMMENT ON COLUMN csm_step.projected_memory is 'the projected amount of memory available for the step';
    COMMENT ON COLUMN csm_step.num_tasks is 'total number of tasks in a job or step';
    COMMENT ON COLUMN csm_step.user_flags is 'user space prolog/epilog flags';
    COMMENT ON INDEX csm_step_pkey IS 'pkey index on step_id, allocation_id';
    COMMENT ON INDEX uk_csm_step_a IS 'uniqueness on step_id, allocation_id';
    

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_step_history (
---------------------------------------------------------------------------------------------------
    history_time            timestamp       not null,
    step_id                 bigint          not null,
    allocation_id           bigint          not null,
    begin_time              timestamp       not null,
    end_time                timestamp,
    status                  text            not null,
    executable              text            not null,
    working_directory       text            not null,
    argument                text            not null,
    environment_variable    text            not null,
    num_nodes               int             not null,
    num_processors          int             not null,
    num_gpus                int             not null,
    projected_memory        int             not null,
    num_tasks               int             not null,
    user_flags              text,
    exit_status             int,
    error_message           text,
    cpu_stats               text,
    total_u_time            double precision,
    total_s_time            double precision,
    omp_thread_limit        text,
    gpu_stats               text,
    memory_stats            text,
    max_memory              bigint,
    io_stats                text,
    archive_history_time    timestamp
);

-------------------------------------------------
-- csm_step_history_indexes
-------------------------------------------------

CREATE INDEX ix_csm_step_history_a
    on csm_step_history (history_time);

CREATE INDEX ix_csm_step_history_b
    on csm_step_history (begin_time, end_time);

CREATE INDEX ix_csm_step_history_c
    on csm_step_history (allocation_id, end_time);

CREATE INDEX ix_csm_step_history_d
    on csm_step_history (end_time);

CREATE INDEX ix_csm_step_history_e
    on csm_step_history (step_id);

CREATE INDEX ix_csm_step_history_f
    on csm_step_history (ctid);

CREATE INDEX ix_csm_step_history_g
    on csm_step_history (archive_history_time);

-------------------------------------------------
-- csm_step_history_comments
-------------------------------------------------
    COMMENT ON TABLE csm_step_history is 'information for steps that have been terminated';
    COMMENT ON COLUMN csm_step_history.history_time is 'timestamp when it enters the history table';
    COMMENT ON COLUMN csm_step_history.step_id is 'uniquely identify this step';
    COMMENT ON COLUMN csm_step_history.allocation_id is 'allocation that this step is part of';
    COMMENT ON COLUMN csm_step_history.begin_time is 'timestamp when this job step started';
    COMMENT ON COLUMN csm_step_history.end_time is 'timestamp when this step ended';
    COMMENT ON COLUMN csm_step_history.status is 'the active operating status of the state';
    COMMENT ON COLUMN csm_step_history.executable is 'executable / command name / application name';
    COMMENT ON COLUMN csm_step_history.working_directory is 'working directory';
    COMMENT ON COLUMN csm_step_history.argument is 'arguments / parameters';
    COMMENT ON COLUMN csm_step_history.environment_variable is 'environment variables';
    COMMENT ON COLUMN csm_step_history.num_nodes is 'the specific number of nodes that are involved in the step';
    COMMENT ON COLUMN csm_step_history.num_processors is 'total number of processes running in this step';
    COMMENT ON COLUMN csm_step_history.num_gpus is 'the number of gpus available';
    COMMENT ON COLUMN csm_step_history.projected_memory is 'the number of memory available';
    COMMENT ON COLUMN csm_step_history.num_tasks is 'total number of tasks in a job or step';
    COMMENT ON COLUMN csm_step_history.user_flags is 'user space prolog/epilog flags';
    COMMENT ON COLUMN csm_step_history.exit_status is 'step/s exit status. will be tracked and given to csm by job leader';
    COMMENT ON COLUMN csm_step_history.error_message is 'step/s error text. will be tracked and given to csm by job leader. the following columns need their proper data types tbd:';
    COMMENT ON COLUMN csm_step_history.cpu_stats is 'will be tracked and given to csm by job leader';
    COMMENT ON COLUMN csm_step_history.total_u_time is 'relates to the (us) (aka: user mode) value of %cpu(s) of the (top) linux cmd. todo: design how we get this data';
    COMMENT ON COLUMN csm_step_history.total_s_time is 'relates to the (sy) (aka: system mode) value of %cpu(s) of the (top) linux cmd. todo: design how we get this data';
    COMMENT ON COLUMN csm_step_history.omp_thread_limit is 'will be tracked and given to csm by job leader';
    COMMENT ON COLUMN csm_step_history.gpu_stats is 'will be tracked and given to csm by job leader';
    COMMENT ON COLUMN csm_step_history.memory_stats is 'will be tracked and given to csm by job leader';
    COMMENT ON COLUMN csm_step_history.max_memory is 'will be tracked and given to csm by job leader';
    COMMENT ON COLUMN csm_step_history.io_stats is 'will be tracked and given to csm by job leader';
    COMMENT ON COLUMN csm_step_history.archive_history_time is 'timestamp when the history data has been archived and sent to: BDS, archive file, and or other';    
    COMMENT ON INDEX ix_csm_step_history_a IS 'index on history_time';
    COMMENT ON INDEX ix_csm_step_history_b IS 'index on begin_time, end_time';
    COMMENT ON INDEX ix_csm_step_history_c IS 'index on allocation_id, end_time';
    COMMENT ON INDEX ix_csm_step_history_d IS 'index on end_time';
    COMMENT ON INDEX ix_csm_step_history_e IS 'index on step_id';
    COMMENT ON INDEX ix_csm_step_history_f IS 'index on ctid';
    COMMENT ON INDEX ix_csm_step_history_g IS 'index on archive_history_time';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_step_node (
---------------------------------------------------------------------------------------------------
    step_id             bigint      not null,
    allocation_id       bigint      not null,
    node_name           text        not null,
    FOREIGN KEY (step_id, allocation_id) references csm_step(step_id, allocation_id),
    FOREIGN KEY (allocation_id, node_name) references csm_allocation_node(allocation_id, node_name)
);


-------------------------------------------------
-- csm_step_node_index
-------------------------------------------------

CREATE UNIQUE INDEX uk_csm_step_node_a
   on csm_step_node (step_id, allocation_id, node_name);

CREATE INDEX ix_csm_step_node_b
    on csm_step_node (allocation_id);

CREATE INDEX ix_csm_step_node_c
    on csm_step_node (allocation_id, step_id);

-------------------------------------------------
-- csm_step_node_comments
-------------------------------------------------
    COMMENT ON TABLE csm_step_node is 'maps active allocations to steps and nodes';
    COMMENT ON COLUMN csm_step_node.step_id is 'uniquely identify this step';
    COMMENT ON COLUMN csm_step_node.allocation_id is 'allocation that this step is part of';    
    COMMENT ON COLUMN csm_step_node.node_name is 'identifies the node';
    COMMENT ON INDEX uk_csm_step_node_a IS 'uniqueness on step_id, allocation_id, node_name';
    COMMENT ON INDEX ix_csm_step_node_b IS 'index on allocation_id';
    COMMENT ON INDEX ix_csm_step_node_c IS 'index on allocation_id, step_id';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_step_node_history (
---------------------------------------------------------------------------------------------------
    history_time            timestamp,
    step_id                 bigint,
    allocation_id           bigint,
    node_name               text,
    archive_history_time    timestamp
);

-------------------------------------------------
-- csm_step_node_history_index
-------------------------------------------------

CREATE INDEX ix_csm_step_node_history_a
    on csm_step_node_history (history_time);

CREATE INDEX ix_csm_step_node_history_b
    on csm_step_node_history (allocation_id);

CREATE INDEX ix_csm_step_node_history_c
    on csm_step_node_history (allocation_id, step_id);

CREATE INDEX ix_csm_step_node_history_d
    on csm_step_node_history (ctid);

CREATE INDEX ix_csm_step_node_history_e
    on csm_step_node_history (archive_history_time);

-------------------------------------------------
-- csm_step_node_history_comments
-------------------------------------------------
    COMMENT ON TABLE csm_step_node_history is 'historical maps active allocations to steps and nodes';
    COMMENT ON COLUMN csm_step_node_history.history_time is 'historical time when information is added to the history table';
    COMMENT ON COLUMN csm_step_node_history.step_id is 'uniquely identify this step';
    COMMENT ON COLUMN csm_step_node_history.allocation_id is 'allocation that this step is part of';
    COMMENT ON COLUMN csm_step_node_history.node_name is 'identifies the node';
    COMMENT ON COLUMN csm_step_node_history.archive_history_time is 'timestamp when the history data has been archived and sent to: BDS, archive file, and or other';    
    COMMENT ON INDEX ix_csm_step_node_history_a IS 'index on history_time';
    COMMENT ON INDEX ix_csm_step_node_history_b IS 'index on allocation_id';
    COMMENT ON INDEX ix_csm_step_node_history_c IS 'index on allocation_id, step_id';
    COMMENT ON INDEX ix_csm_step_node_history_d IS 'index on ctid';
    COMMENT ON INDEX ix_csm_step_node_history_e IS 'index on archive_history_time';

---------------------------------------------------------------------------------------------------
-- The different severity levels in a RAS event.
---------------------------------------------------------------------------------------------------
CREATE TYPE ras_event_severity AS ENUM (
---------------------------------------------------------------------------------------------------
  'INFO',
  'WARNING',
  'FATAL'
);

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_ras_type (
---------------------------------------------------------------------------------------------------
    msg_id              text                not null,
    severity            ras_event_severity  not null,
    message             text,
    description         text,
    control_action      text,
    threshold_count     int,
    threshold_period    int,
    enabled             boolean,
    set_state           compute_node_states,
    visible_to_users    boolean,
    PRIMARY KEY (msg_id)
);
    
-------------------------------------------------
-- csm_ras_type_index
-------------------------------------------------

-- automatically created index ON pkey (msg_id)

-------------------------------------------------
-- csm_ras_type_comments
-------------------------------------------------
    COMMENT ON TABLE csm_ras_type is 'contains the description and details for each of the possible ras event types';
    COMMENT ON COLUMN csm_ras_type.msg_id is 'the identifier string for this RAS event. It must be unique.  typically it consists of three parts separated by periods (system.component.id).';
    COMMENT ON COLUMN csm_ras_type.severity is 'severity of the RAS event. INFO/WARNING/FATAL';
    COMMENT ON COLUMN csm_ras_type.message is 'ras message to display to the user (pre-variable substitution)';
    COMMENT ON COLUMN csm_ras_type.description is 'description of the ras event';   
    COMMENT ON COLUMN csm_ras_type.control_action is 'name of control action script to invoke for this event.';
    COMMENT ON COLUMN csm_ras_type.threshold_count is 'number of times this event has to occur during the (threshold_period) before taking action on the RAS event.';
    COMMENT ON COLUMN csm_ras_type.threshold_period is 'period in seconds over which to compare the number of event occurences to the threshold_count ).';
    COMMENT ON COLUMN csm_ras_type.enabled is 'events will be processed if enabled=true and suppressed if enabled=false';
    COMMENT ON COLUMN csm_ras_type.set_state is 'setting the state according to the node, DISCOVERED, IN_SERVICE, ADMIN_RESERVED, MAINTENANCE, SOFT_FAILURE, OUT_OF_SERVICE, HARD_FAILURE';
    COMMENT ON COLUMN csm_ras_type.visible_to_users is 'when visible_to_users=true, RAS events of this type will be returned in the response to csm_ras_event_query_allocation';
    COMMENT ON INDEX csm_ras_type_pkey IS 'pkey index on msg_id';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_ras_type_audit (
---------------------------------------------------------------------------------------------------
    msg_id_seq          bigserial           not null,
    operation           char(1)             not null,
    change_time         timestamp           not null,
    msg_id              text                not null,
    severity            ras_event_severity  not null,
    message             text,
    description         text,
    control_action      text,
    threshold_count     int,
    threshold_period    int,
    enabled             boolean,
    set_state           compute_node_states,
    visible_to_users    boolean,
    PRIMARY KEY (msg_id_seq)
);

-------------------------------------------------
-- csm_ras_type_audit_comments
-------------------------------------------------
    COMMENT ON TABLE csm_ras_type_audit is 'records all of the changes to the ras event types in the csm_ras_type table';
    COMMENT ON COLUMN csm_ras_type_audit.msg_id_seq is 'a unique sequence number used to index the csm_ras_type_audit table';
    COMMENT ON COLUMN csm_ras_type_audit.operation is 'I/D/U indicates whether the change to the csm_ras_type table was an INSERT, DELETE, or UPDATE';
    COMMENT ON COLUMN csm_ras_type_audit.change_time is 'time_stamp indicating when this change occurred';
    COMMENT ON COLUMN csm_ras_type_audit.msg_id is 'the identifier string for this RAS event. typically it consists of three parts separated by periods (system.component.id).';
    COMMENT ON COLUMN csm_ras_type_audit.severity is 'severity of the RAS event. INFO/WARNING/FATAL';
    COMMENT ON COLUMN csm_ras_type_audit.message is 'ras message to display to the user (pre-variable substitution)';
    COMMENT ON COLUMN csm_ras_type_audit.description is 'description of the ras event'; 
    COMMENT ON COLUMN csm_ras_type_audit.control_action is 'name of control action script to invoke for this event.';
    COMMENT ON COLUMN csm_ras_type_audit.threshold_count is 'number of times this event has to occur during the (threshold_period) before taking action on the RAS event.';
    COMMENT ON COLUMN csm_ras_type_audit.threshold_period is 'period in seconds over which to compare the number of event occurences to the threshold_count ).';
    COMMENT ON COLUMN csm_ras_type_audit.enabled is 'events will be processed if enabled=true and suppressed if enabled=false';
    COMMENT ON COLUMN csm_ras_type_audit.set_state is 'setting the state according to the node, DISCOVERED, IN_SERVICE, ADMIN_RESERVED, MAINTENANCE, SOFT_FAILURE, OUT_OF_SERVICE, HARD_FAILURE';
    COMMENT ON COLUMN csm_ras_type_audit.visible_to_users is 'when visible_to_users=true, RAS events of this type will be returned in the response to csm_ras_event_query_allocation';
    COMMENT ON INDEX csm_ras_type_audit_pkey IS 'pkey index on msg_id_seq';
    COMMENT ON SEQUENCE csm_ras_type_audit_msg_id_seq_seq IS 'used to generate primary keys on msg ids';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_ras_event_action (
---------------------------------------------------------------------------------------------------
    rec_id                bigserial,
    msg_id                text        not null,
    msg_id_seq            int         not null,
    time_stamp            timestamp   not null,
    master_time_stamp     timestamp   not null,
    location_name         text        not null,
    count                 int,
    message               text,
    kvcsv                 text,
    raw_data              text,
    archive_history_time  timestamp,
    PRIMARY KEY (rec_id),
    FOREIGN KEY (msg_id_seq) references csm_ras_type_audit(msg_id_seq)
);

-------------------------------------------------
-- csm_ras_event_action_indexes
-------------------------------------------------

-- automatically created index ON pkey (rec_id)

CREATE INDEX ix_csm_ras_event_action_a
    on csm_ras_event_action (msg_id);

CREATE INDEX ix_csm_ras_event_action_b
    on csm_ras_event_action (time_stamp);

CREATE INDEX ix_csm_ras_event_action_c
    on csm_ras_event_action (location_name);

CREATE INDEX ix_csm_ras_event_action_d
    on csm_ras_event_action (time_stamp, msg_id);

CREATE INDEX ix_csm_ras_event_action_e
    on csm_ras_event_action (time_stamp, location_name);

CREATE INDEX ix_csm_ras_event_action_f
    on csm_ras_event_action (master_time_stamp);

CREATE INDEX ix_csm_ras_event_action_g
    on csm_ras_event_action (ctid);

CREATE INDEX ix_csm_ras_event_action_h
    on csm_ras_event_action (archive_history_time);

-------------------------------------------------
-- csm_ras_event_action_comments
-------------------------------------------------
    COMMENT ON TABLE csm_ras_event_action is 'contains all ras events';
    COMMENT ON COLUMN csm_ras_event_action.rec_id is 'unique identifier for this specific ras event';
    COMMENT ON COLUMN csm_ras_event_action.msg_id is 'type of ras event';
    COMMENT ON COLUMN csm_ras_event_action.msg_id_seq is 'a unique sequence number used to index the csm_ras_type_audit table';
    COMMENT ON COLUMN csm_ras_event_action.time_stamp is 'The time supplied by the caller of csm_ras_event_create. Used for correlating between events based on the local time of the event source.';
    COMMENT ON COLUMN csm_ras_event_action.master_time_stamp is 'The time when the event is process by the CSM master daemon. Used for correlating node state changes with CSM master processing of RAS events.';
    COMMENT ON COLUMN csm_ras_event_action.location_name is 'this field can be a node name or location name';
    COMMENT ON COLUMN csm_ras_event_action.count is 'how many times this event reoccurs';
    COMMENT ON COLUMN csm_ras_event_action.message is 'message text';
    COMMENT ON COLUMN csm_ras_event_action.kvcsv is 'event specific keys and values in a comma separated list';
    COMMENT ON COLUMN csm_ras_event_action.raw_data is 'event/s raw data';
    COMMENT ON COLUMN csm_ras_event_action.archive_history_time is 'timestamp when the history data has been archived and sent to: BDS, archive file, and or other';    
    COMMENT ON INDEX csm_ras_event_action_pkey IS 'pkey index on rec_id';
    COMMENT ON INDEX ix_csm_ras_event_action_a IS 'index on msg_id';
    COMMENT ON INDEX ix_csm_ras_event_action_b IS 'index on time_stamp';
    COMMENT ON INDEX ix_csm_ras_event_action_c IS 'index on location_name';
    COMMENT ON INDEX ix_csm_ras_event_action_d IS 'index on time_stamp, msg_id';
    COMMENT ON INDEX ix_csm_ras_event_action_e IS 'index on time_stamp, location_name';
    COMMENT ON INDEX ix_csm_ras_event_action_f IS 'index on master_time_stamp';
    COMMENT ON INDEX ix_csm_ras_event_action_g IS 'index on ctid';
    COMMENT ON INDEX ix_csm_ras_event_action_h IS 'index on archive_history_time';
    COMMENT ON SEQUENCE csm_ras_event_action_rec_id_seq IS 'used to generate primary keys on rec ids';

CREATE VIEW csm_ras_event_action_view AS
    SELECT
        crea.rec_id,
        crea.msg_id_seq,
        crea.msg_id,
        crea.time_stamp,
        crea.master_time_stamp,
        crea.location_name,
        crea.count,
        crea.message,
        crea.kvcsv,
        crea.raw_data,
        crta.severity,
        crta.description,
        crta.control_action,
        crta.threshold_count,
        crta.threshold_period,
        crta.enabled,
        crta.set_state, 
        crta.visible_to_users
    FROM csm_ras_event_action crea
    JOIN csm_ras_type_audit crta ON crta.msg_id_seq = crea.msg_id_seq;

    COMMENT ON VIEW csm_ras_event_action_view IS 'view of records that are a part of the csm_ras_event_action and csm_ras_type_audit tables';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_gpu (
---------------------------------------------------------------------------------------------------
    serial_number           text        not null,
    node_name               text        references csm_node(node_name),
    gpu_id                  int         not null,
    device_name             text        not null,
    pci_bus_id              text        not null,
    uuid                    text        not null,
    vbios                   text        not null,
    inforom_image_version   text        not null,
    hbm_memory              bigint,
    PRIMARY KEY (node_name, gpu_id)
);

-------------------------------------------------
-- csm_gpu_index
-------------------------------------------------

-- automatically created index ON pkey (serial_number)

-- CREATE INDEX ix_csm_gpu_a
--    on csm_gpu (node_name, serial_number);

-------------------------------------------------
-- csm_gpu_comments
-------------------------------------------------
    COMMENT ON TABLE csm_gpu is 'contains information on the gpus known to the system';
    COMMENT ON COLUMN csm_gpu.serial_number is 'unique identifier for this gpu';
    COMMENT ON COLUMN csm_gpu.node_name is 'where does this gpu reside';
    COMMENT ON COLUMN csm_gpu.gpu_id is 'gpu identification number';
    COMMENT ON COLUMN csm_gpu.device_name is 'indicates the device name';
    COMMENT ON COLUMN csm_gpu.pci_bus_id is 'Peripheral Component Interconnect bus identifier';
    COMMENT ON COLUMN csm_gpu.uuid is 'universally unique identifier';
    COMMENT ON COLUMN csm_gpu.vbios is 'Video BIOS';
    COMMENT ON COLUMN csm_gpu.inforom_image_version is 'version of the infoROM';
    COMMENT ON COLUMN csm_gpu.hbm_memory is 'high bandwidth memory: amount of available memory on this gpu (in kB)'; 
    COMMENT ON INDEX csm_gpu_pkey IS 'pkey index on node_name, gpu_id';
--  COMMENT ON INDEX ix_csm_gpu_a IS 'index on node_name, serial_number';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_gpu_history (
---------------------------------------------------------------------------------------------------
    history_time            timestamp   not null,
    serial_number           text        not null,
    node_name               text,
    gpu_id                  int         not null,
    device_name             text        not null,
    pci_bus_id              text        not null,
    uuid                    text        not null,
    vbios                   text        not null,
    inforom_image_version   text        not null,
    hbm_memory              bigint,
    operation               char(1)     not null,
    archive_history_time    timestamp
);

-------------------------------------------------
-- csm_gpu_history_indexes
-------------------------------------------------

CREATE INDEX ix_csm_gpu_history_a
    on csm_gpu_history (history_time);

CREATE INDEX ix_csm_gpu_history_b
    on csm_gpu_history (serial_number);

CREATE INDEX ix_csm_gpu_history_c
    on csm_gpu_history (node_name, gpu_id);

CREATE INDEX ix_csm_gpu_history_d
    on csm_gpu_history (ctid);

CREATE INDEX ix_csm_gpu_history_e
    on csm_gpu_history (archive_history_time);

-------------------------------------------------
-- csm_gpu_history_comments
-------------------------------------------------
    COMMENT ON TABLE csm_gpu_history is 'contains historical information associated with individual gpus';
    COMMENT ON COLUMN csm_gpu_history.history_time is 'the time when the gpu is entering the history table';
    COMMENT ON COLUMN csm_gpu_history.serial_number is 'unique identifier for this gpu';
    COMMENT ON COLUMN csm_gpu_history.node_name is 'where does this gpu reside';
    COMMENT ON COLUMN csm_gpu_history.gpu_id is 'gpu identification number';
    COMMENT ON COLUMN csm_gpu_history.device_name is 'indicates the device name';
    COMMENT ON COLUMN csm_gpu_history.pci_bus_id is 'Peripheral Component Interconnect bus identifier';
    COMMENT ON COLUMN csm_gpu_history.uuid is 'universally unique identifier';
    COMMENT ON COLUMN csm_gpu_history.vbios is 'Video BIOS';
    COMMENT ON COLUMN csm_gpu_history.inforom_image_version is 'version of the infoROM';
    COMMENT ON COLUMN csm_gpu_history.hbm_memory is 'high bandwidth memory: amount of available memory on this gpu (in kB)';
    COMMENT ON COLUMN csm_gpu_history.operation is 'operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)';
    COMMENT ON COLUMN csm_gpu_history.archive_history_time is 'timestamp when the history data has been archived and sent to: BDS, archive file, and or other';
    COMMENT ON INDEX ix_csm_gpu_history_a IS 'index on history_time';
    COMMENT ON INDEX ix_csm_gpu_history_b IS 'index on serial_number';
    COMMENT ON INDEX ix_csm_gpu_history_c IS 'index on node_name, gpu_id';
    COMMENT ON INDEX ix_csm_gpu_history_d IS 'index on ctid';
    COMMENT ON INDEX ix_csm_gpu_history_e IS 'index on archive_history_time';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_processor_socket (
---------------------------------------------------------------------------------------------------
    serial_number       text        not null,
    node_name           text        references csm_node(node_name),
    physical_location   text,
    discovered_cores    int,
    PRIMARY KEY (serial_number, node_name)
);

-------------------------------------------------
-- csm_processor_socket_index
-------------------------------------------------

-- automatically created index ON pkey (serial_number, node_name)

-- CREATE INDEX ix_csm_processor_socket_a
--    on csm_processor_socket (serial_number, node_name);

-------------------------------------------------
-- csm_processor_socket_comments
-------------------------------------------------
    COMMENT ON TABLE csm_processor_socket is 'contains information on the processors/p9s known to the system';
    COMMENT ON COLUMN csm_processor_socket.serial_number is 'unique identifier for this processor';
    COMMENT ON COLUMN csm_processor_socket.node_name is 'where does this processor reside';
    COMMENT ON COLUMN csm_processor_socket.physical_location is 'physical location of the processor';
    COMMENT ON COLUMN csm_processor_socket.discovered_cores is 'number of physical cores on this processor';
    COMMENT ON INDEX csm_processor_socket_pkey IS 'pkey index on serial_number';
--  COMMENT ON INDEX ix_csm_processor_socket_a IS 'index on serial_number, node_name';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_processor_socket_history (
---------------------------------------------------------------------------------------------------
    history_time            timestamp   not null,
    serial_number           text        not null,
    node_name               text,
    physical_location       text,
    discovered_cores        int,
    operation               char(1)     not null,
    archive_history_time    timestamp
);

-------------------------------------------------
-- csm_processor_socket_history_indexes
-------------------------------------------------

CREATE INDEX ix_csm_processor_socket_history_a
    on csm_processor_socket_history (history_time);

CREATE INDEX ix_csm_processor_socket_history_b
    on csm_processor_socket_history (serial_number, node_name);

CREATE INDEX ix_csm_processor_socket_history_c
    on csm_processor_socket_history (ctid);

CREATE INDEX ix_csm_processor_socket_history_d
    on csm_processor_socket_history (archive_history_time);

-------------------------------------------------
-- csm_processor_socket_history_comments
-------------------------------------------------
    COMMENT ON TABLE csm_processor_socket_history is 'contains historical information associated with individual processors';
    COMMENT ON COLUMN csm_processor_socket_history.history_time is 'the time when the processor is entering the history table';
    COMMENT ON COLUMN csm_processor_socket_history.serial_number is 'unique identifier for this processor';
    COMMENT ON COLUMN csm_processor_socket_history.node_name is 'where does this processor reside';
    COMMENT ON COLUMN csm_processor_socket_history.physical_location is 'physical location of the processor';
    COMMENT ON COLUMN csm_processor_socket_history.discovered_cores is 'number of physical cores on this processor';
    COMMENT ON COLUMN csm_processor_socket_history.operation is 'operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)';
    COMMENT ON COLUMN csm_processor_socket_history.archive_history_time is 'timestamp when the history data has been archived and sent to: BDS, archive file, and or other';
    COMMENT ON INDEX ix_csm_processor_socket_history_a IS 'index on history_time';
    COMMENT ON INDEX ix_csm_processor_socket_history_b IS 'index on serial_number, node_name';
    COMMENT ON INDEX ix_csm_processor_socket_history_c IS 'index on ctid';
    COMMENT ON INDEX ix_csm_processor_socket_history_d IS 'index on archive_history_time';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_ssd (
---------------------------------------------------------------------------------------------------
    serial_number                   text                not null,
    node_name                       text                not null,
    update_time                     timestamp           not null,
    device_name                     text,
    pci_bus_id                      text,
    fw_ver                          text,
    size                            bigint              not null,
    wear_lifespan_used              double precision,   --not null,
    wear_total_bytes_written        bigint,
    wear_total_bytes_read           bigint,
    wear_percent_spares_remaining   double precision,   --not null,
    PRIMARY KEY (serial_number, node_name),
    FOREIGN KEY (node_name) references csm_node(node_name)
);

-------------------------------------------------
-- csm_ssd_index
-------------------------------------------------

-- automatically created index ON pkey (serial_number)
CREATE UNIQUE INDEX uk_csm_ssd_a
    on csm_ssd (serial_number, node_name);

-------------------------------------------------
-- csm_ssd_comments
-------------------------------------------------
    COMMENT ON TABLE csm_ssd is 'contains information on the ssds known to the system';
    COMMENT ON COLUMN csm_ssd.serial_number is 'unique identifier for this ssd';
    COMMENT ON COLUMN csm_ssd.node_name is 'where does this ssd reside';
    COMMENT ON COLUMN csm_ssd.update_time is 'timestamp when ssd was updated';
    COMMENT ON COLUMN csm_ssd.device_name is 'product device name';
    COMMENT ON COLUMN csm_ssd.pci_bus_id is 'PCI bus id';
    COMMENT ON COLUMN csm_ssd.fw_ver is 'firmware version';
    COMMENT ON COLUMN csm_ssd.size is 'total capacity (in bytes) of this ssd, for example, 800 gbs';
    COMMENT ON COLUMN csm_ssd.wear_lifespan_used is 'estimate of the amount of SSD life consumed (w.l.m. will use - 0-255 per)';
    COMMENT ON COLUMN csm_ssd.wear_total_bytes_written is 'number of bytes written to the SSD over the life of the device';
    COMMENT ON COLUMN csm_ssd.wear_total_bytes_read is 'number of bytes read from the SSD over the life of the device';
    COMMENT ON COLUMN csm_ssd.wear_percent_spares_remaining is 'amount of SSD capacity over-provisioning that remains';
    COMMENT ON INDEX csm_ssd_pkey IS 'pkey index on serial_number, node_name';
    COMMENT ON INDEX uk_csm_ssd_a IS 'index on serial_number, node_name';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_hca (
---------------------------------------------------------------------------------------------------
    serial_number       text        not null,
    node_name           text        not null,
    device_name         text,
    pci_bus_id          text        not null,
    guid                text        not null,
    part_number         text,
    fw_ver              text,
    hw_rev              text,
    board_id            text,
    PRIMARY KEY (node_name, serial_number),
    FOREIGN KEY (node_name) references csm_node(node_name)
);

-------------------------------------------------
-- csm_hca_index
-------------------------------------------------

-- automatically created index ON pkey (node_name, serial_number)

-------------------------------------------------
-- csm_hca_comments
-------------------------------------------------
    COMMENT ON TABLE csm_hca is 'contains inventory information about the InfiniBand (IB) HCAs (Host Channel Adapters)';
    COMMENT ON COLUMN csm_hca.serial_number is 'unique serial number for this HCA';
    COMMENT ON COLUMN csm_hca.node_name is 'node this HCA is installed in';
    COMMENT ON COLUMN csm_hca.device_name is 'product device name for this HCA';
    COMMENT ON COLUMN csm_hca.pci_bus_id is 'PCI bus id for this HCA';
    COMMENT ON COLUMN csm_hca.guid is 'sys_image_guid for this HCA';
    COMMENT ON COLUMN csm_hca.part_number is 'part number for this HCA';
    COMMENT ON COLUMN csm_hca.fw_ver is 'firmware version for this HCA';
    COMMENT ON COLUMN csm_hca.hw_rev is 'hardware revision for this HCA';
    COMMENT ON COLUMN csm_hca.board_id is 'board id for this HCA';
    COMMENT ON INDEX csm_hca_pkey IS 'pkey index on serial_number';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_hca_history (
---------------------------------------------------------------------------------------------------
    history_time            timestamp   not null,
    serial_number           text        not null,
    node_name               text,
    device_name             text,
    pci_bus_id              text        not null,
    guid                    text        not null,
    part_number             text,
    fw_ver                  text,
    hw_rev                  text,
    board_id                text,
    operation               char(1)     not null,
    archive_history_time    timestamp
);

-------------------------------------------------
-- csm_hca_history_index
-------------------------------------------------

CREATE INDEX ix_csm_hca_history_a
    on csm_hca_history (history_time);

CREATE INDEX ix_csm_hca_history_b
    on csm_hca_history (node_name, serial_number);

CREATE INDEX ix_csm_hca_history_c
    on csm_hca_history (ctid);

CREATE INDEX ix_csm_hca_history_d
    on csm_hca_history (archive_history_time);

-------------------------------------------------
-- csm_hca_history_comments
-------------------------------------------------
    COMMENT ON TABLE csm_hca_history is 'contains historical inventory information about the InfiniBand (IB) HCAs (Host Channel Adapters)';
    COMMENT ON COLUMN csm_hca_history.history_time is 'the time when the HCA is entering the history table';
    COMMENT ON COLUMN csm_hca_history.serial_number is 'unique serial number for this HCA';
    COMMENT ON COLUMN csm_hca_history.node_name is 'node this HCA is installed in';
    COMMENT ON COLUMN csm_hca_history.device_name is 'product device name for this HCA';
    COMMENT ON COLUMN csm_hca_history.pci_bus_id is 'PCI bus id for this HCA';
    COMMENT ON COLUMN csm_hca_history.guid is 'sys_image_guid for this HCA';
    COMMENT ON COLUMN csm_hca_history.part_number is 'part number for this HCA';
    COMMENT ON COLUMN csm_hca_history.fw_ver is 'firmware version for this HCA';
    COMMENT ON COLUMN csm_hca_history.hw_rev is 'hardware revision for this HCA';
    COMMENT ON COLUMN csm_hca_history.board_id is 'board id for this HCA';
    COMMENT ON COLUMN csm_hca_history.operation is 'operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)';
    COMMENT ON COLUMN csm_hca_history.archive_history_time is 'timestamp when the history data has been archived and sent to: BDS, archive file, and or other';
    COMMENT ON INDEX ix_csm_hca_history_a IS 'index on history_time';
    COMMENT ON INDEX ix_csm_hca_history_b IS 'index on node_name, serial_number';
    COMMENT ON INDEX ix_csm_hca_history_c IS 'index on ctid';
    COMMENT ON INDEX ix_csm_hca_history_d IS 'index on archive_history_time';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_dimm (
---------------------------------------------------------------------------------------------------
    serial_number       text        not null,
    node_name           text        references csm_node(node_name),
    size                int         not null,
    physical_location   text        not null,
    PRIMARY KEY (serial_number, node_name)
);

-------------------------------------------------
-- csm_dimm_index
-------------------------------------------------

-- automatically created index ON pkey (serial_number, node_name)

-------------------------------------------------
-- csm_dimm_comments
-------------------------------------------------
    COMMENT ON TABLE csm_dimm is 'dual in-line memory module';
    COMMENT ON COLUMN csm_dimm.serial_number is 'this is the dimm serial number';
    COMMENT ON COLUMN csm_dimm.node_name is 'where does this dimm reside';
    COMMENT ON COLUMN csm_dimm.size is 'the size can be 4, 8, 16, 32 GB';
    COMMENT ON COLUMN csm_dimm.physical_location is 'phyical location where the dimm is installed';
    COMMENT ON INDEX csm_dimm_pkey IS 'pkey index on serial_number';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_dimm_history (
---------------------------------------------------------------------------------------------------
    history_time            timestamp   not null,
    serial_number           text        not null,
    node_name               text,
    size                    int         not null,
    physical_location       text        not null,
    operation               char(1)     not null,
    archive_history_time    timestamp
);

-------------------------------------------------
-- csm_dimm_history_index
-------------------------------------------------

CREATE INDEX ix_csm_dimm_history_a
    on csm_dimm_history (history_time);

CREATE INDEX ix_csm_dimm_history_b
    on csm_dimm_history (node_name, serial_number);

CREATE INDEX ix_csm_dimm_history_c
    on csm_dimm_history (ctid);

CREATE INDEX ix_csm_dimm_history_d
    on csm_dimm_history (archive_history_time);

-------------------------------------------------
-- csm_dimm_history_comments
-------------------------------------------------
    COMMENT ON TABLE csm_dimm_history is 'historical information related to the dual in-line memory module';
    COMMENT ON COLUMN csm_dimm_history.history_time is 'this is when the information is entered into the history table';
    COMMENT ON COLUMN csm_dimm_history.serial_number is 'this is the dimm serial number';
    COMMENT ON COLUMN csm_dimm_history.node_name is 'where does this dimm reside';
    COMMENT ON COLUMN csm_dimm_history.size is 'the size can be 4, 8, 16, 32 GB';
    COMMENT ON COLUMN csm_dimm_history.physical_location is 'physical location where the dimm is installed';
    COMMENT ON COLUMN csm_dimm_history.operation is 'operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)';
    COMMENT ON COLUMN csm_dimm_history.archive_history_time is 'timestamp when the history data has been archived and sent to: BDS, archive file, and or other';
    COMMENT ON INDEX ix_csm_dimm_history_a IS 'index on history_time';
    COMMENT ON INDEX ix_csm_dimm_history_b IS 'index on node_name, serial_number';
    COMMENT ON INDEX ix_csm_dimm_history_c IS 'index on ctid';
    COMMENT ON INDEX ix_csm_dimm_history_d IS 'index on archive_history_time';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_ssd_history (
---------------------------------------------------------------------------------------------------
    history_time                    timestamp           not null,
    serial_number                   text                not null,
    node_name                       text,
    update_time                     timestamp           not null,
    device_name                     text,
    pci_bus_id                      text,
    fw_ver                          text,
    size                            bigint              not null,
    wear_lifespan_used              double precision,   --not null,
    wear_total_bytes_written        bigint,
    wear_total_bytes_read           bigint,
    wear_percent_spares_remaining   double precision,    --not null,
    operation                       char(1)             not null,
    archive_history_time            timestamp
);

-------------------------------------------------
-- csm_ssd_history_indexes
-------------------------------------------------

CREATE INDEX ix_csm_ssd_history_a
    on csm_ssd_history (history_time);

CREATE INDEX ix_csm_ssd_history_b
    on csm_ssd_history (serial_number, node_name);

CREATE INDEX ix_csm_ssd_history_c
    on csm_ssd_history (ctid);

CREATE INDEX ix_csm_ssd_history_d
    on csm_ssd_history (archive_history_time);

-------------------------------------------------
-- csm_ssd_history_comments
-------------------------------------------------
    COMMENT ON TABLE csm_ssd_history is 'historical information on the ssds known to the system';
    COMMENT ON COLUMN csm_ssd_history.history_time is 'timestamp';
    COMMENT ON COLUMN csm_ssd_history.serial_number is 'unique identifier for this ssd';
    COMMENT ON COLUMN csm_ssd_history.node_name is 'where does this ssd reside';
    COMMENT ON COLUMN csm_ssd_history.update_time is 'timestamp when the ssd was updated';
    COMMENT ON COLUMN csm_ssd_history.device_name is 'product device name';
    COMMENT ON COLUMN csm_ssd_history.pci_bus_id is 'PCI bus id';
    COMMENT ON COLUMN csm_ssd_history.fw_ver is 'firmware version';
    COMMENT ON COLUMN csm_ssd_history.size is 'total capacity (in bytes) of this ssd, for example, 800 gbs';
    COMMENT ON COLUMN csm_ssd_history.wear_lifespan_used is 'estimate of the amount of SSD life consumed (w.l.m. will use - 0-255 per)';
    COMMENT ON COLUMN csm_ssd_history.wear_total_bytes_written is 'number of bytes written to the SSD over the life of the device';
    COMMENT ON COLUMN csm_ssd_history.wear_total_bytes_read is 'number of bytes read from the SSD over the life of the device';
    COMMENT ON COLUMN csm_ssd_history.wear_percent_spares_remaining is 'amount of SSD capacity over-provisioning that remains';
    COMMENT ON COLUMN csm_ssd_history.operation is 'operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)';
    COMMENT ON COLUMN csm_ssd_history.archive_history_time is 'timestamp when the history data has been archived and sent to: BDS, archive file, and or other';
    COMMENT ON INDEX ix_csm_ssd_history_a IS 'index on history_time';
    COMMENT ON INDEX ix_csm_ssd_history_b IS 'index on serial_number, node_name';
    COMMENT ON INDEX ix_csm_ssd_history_c IS 'index on ctid';
    COMMENT ON INDEX ix_csm_ssd_history_d IS 'index on archive_history_time';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_ssd_wear_history (
--------------------------------------------------------------------------------------------------- 
    history_time                    timestamp           not null,
    serial_number                   text                not null,
    node_name                       text,
    wear_lifespan_used              double precision,   --not null,
    wear_total_bytes_written        bigint,
    wear_total_bytes_read           bigint,
    wear_percent_spares_remaining   double precision,    --not null,
    operation                       char(1)             not null,
    archive_history_time            timestamp
);  
    
-------------------------------------------------
-- csm_ssd_wear_history_indexes
-------------------------------------------------

CREATE INDEX ix_csm_ssd_wear_history_a
    on csm_ssd_wear_history (history_time);

CREATE INDEX ix_csm_ssd_wear_history_b
    on csm_ssd_wear_history (serial_number, node_name); 
    
CREATE INDEX ix_csm_ssd_wear_history_c
    on csm_ssd_wear_history (ctid);

CREATE INDEX ix_csm_ssd_wear_history_d
    on csm_ssd_wear_history (archive_history_time);

-------------------------------------------------
-- csm_ssd_wear_history_comments
-------------------------------------------------
    COMMENT ON TABLE csm_ssd_wear_history is 'historical information on the ssds wear known to the system';
    COMMENT ON COLUMN csm_ssd_wear_history.history_time is 'timestamp';
    COMMENT ON COLUMN csm_ssd_wear_history.serial_number is 'unique identifier for this ssd';
    COMMENT ON COLUMN csm_ssd_wear_history.node_name is 'where does this ssd reside';
    COMMENT ON COLUMN csm_ssd_wear_history.wear_lifespan_used is 'estimate of the amount of SSD life consumed (w.l.m. will use - 0-255 per)';
    COMMENT ON COLUMN csm_ssd_wear_history.wear_total_bytes_written is 'number of bytes written to the SSD over the life of the device';
    COMMENT ON COLUMN csm_ssd_wear_history.wear_total_bytes_read is 'number of bytes read from the SSD over the life of the device';
    COMMENT ON COLUMN csm_ssd_wear_history.wear_percent_spares_remaining is 'amount of SSD capacity over-provisioning that remains';
    COMMENT ON COLUMN csm_ssd_wear_history.operation is 'operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)';
    COMMENT ON COLUMN csm_ssd_wear_history.archive_history_time is 'timestamp when the history data has been archived and sent to: BDS, archive file, and or other';
    COMMENT ON INDEX ix_csm_ssd_wear_history_a IS 'index on history_time';
    COMMENT ON INDEX ix_csm_ssd_wear_history_b IS 'index on serial_number, node_name';
    COMMENT ON INDEX ix_csm_ssd_wear_history_c IS 'index on ctid';
    COMMENT ON INDEX ix_csm_ssd_wear_history_d IS 'index on archive_history_time';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_vg (
---------------------------------------------------------------------------------------------------
    vg_name             text        not null,
    node_name           text        not null,
    total_size          bigint      not null,
    available_size      bigint      not null,
    scheduler           boolean     not null,
    update_time         timestamp,
    CONSTRAINT csm_available_size_should_be_less_than_total_size CHECK (available_size <= total_size), --,
    PRIMARY KEY (vg_name, node_name)
);

-------------------------------------------------
-- csm_vg_index
-------------------------------------------------

-- automatically created index ON pkey (vg_name, node_name)

-------------------------------------------------
-- csm_vg_comments
-------------------------------------------------
    COMMENT ON TABLE csm_vg is 'this table contains information that references both the SSD logical volume tables';
    COMMENT ON COLUMN csm_vg.vg_name is 'unique identifier for this ssd partition';
    COMMENT ON COLUMN csm_vg.node_name is 'identifies which node';
    COMMENT ON COLUMN csm_vg.total_size is 'volume group size. measured in bytes';
    COMMENT ON COLUMN csm_vg.available_size is 'remaining bytes available out of total size.';
    COMMENT ON COLUMN csm_vg.scheduler is 'tells CSM whether or not this is the volume group for the scheduler.';
    COMMENT ON COLUMN csm_vg.update_time is 'timestamp when the vg was updated';
    COMMENT ON CONSTRAINT csm_available_size_should_be_less_than_total_size ON csm_vg IS 'csm_available_size_should_be_less_than_total_size';
    COMMENT ON INDEX csm_vg_pkey IS 'pkey index on vg_name, node_name';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_vg_history (
---------------------------------------------------------------------------------------------------
    history_time            timestamp   not null,
    vg_name                 text        not null,
    node_name               text        not null,
    total_size              bigint      not null,
    available_size          bigint      not null,
    scheduler               boolean     not null,
    update_time             timestamp,
    operation               char(1)     not null,
    archive_history_time    timestamp
);

-------------------------------------------------
-- csm_vg_history_index
-------------------------------------------------

CREATE INDEX ix_csm_vg_history_a
    on csm_vg_history (history_time);

CREATE INDEX ix_csm_vg_history_b
    on csm_vg_history (vg_name, node_name);

CREATE INDEX ix_csm_vg_history_c
    on csm_vg_history (ctid);

CREATE INDEX ix_csm_vg_history_d
    on csm_vg_history (archive_history_time);

-------------------------------------------------
-- csm_vg_history_comments
-------------------------------------------------
    COMMENT ON TABLE csm_vg_history is 'this table contains historical information that references both the SSD logical volume tables';
    COMMENT ON COLUMN csm_vg_history.history_time is 'time when enters into the history table';
    COMMENT ON COLUMN csm_vg_history.vg_name is 'unique identifier for this ssd partition';
    COMMENT ON COLUMN csm_vg_history.node_name is 'identifies which node';
    COMMENT ON COLUMN csm_vg_history.total_size is 'volume group size. measured in bytes';
    COMMENT ON COLUMN csm_vg_history.available_size is 'remaining bytes available out of total size.';
    COMMENT ON COLUMN csm_vg_history.scheduler is 'tells CSM whether or not this is the volume group for the scheduler.';
    COMMENT ON COLUMN csm_vg_history.update_time is 'timestamp when the vg was updated';
    COMMENT ON COLUMN csm_vg_history.operation is 'operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)';
    COMMENT ON COLUMN csm_vg_history.archive_history_time is 'timestamp when the history data has been archived and sent to: BDS, archive file, and or other';
    COMMENT ON INDEX ix_csm_vg_history_a IS 'index on history_time';
    COMMENT ON INDEX ix_csm_vg_history_b IS 'index on vg_name, node_name';
    COMMENT ON INDEX ix_csm_vg_history_c IS 'index on ctid';
    COMMENT ON INDEX ix_csm_vg_history_d IS 'index on archive_history_time';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_vg_ssd (
---------------------------------------------------------------------------------------------------
    vg_name             text    not null,
    node_name           text    not null,
    serial_number       text    not null,
    ssd_allocation      bigint  not null,
    FOREIGN KEY (serial_number,node_name) references csm_ssd(serial_number, node_name),
    FOREIGN KEY (vg_name, node_name) references csm_vg(vg_name, node_name)
);

-------------------------------------------------
-- csm_vg_ssd_index
-------------------------------------------------

CREATE UNIQUE INDEX uk_csm_vg_ssd_a
    on csm_vg_ssd (vg_name, node_name, serial_number);

-------------------------------------------------
-- csm_vg_ssd_comments
-------------------------------------------------
    COMMENT ON TABLE csm_vg_ssd is 'this table contains information that references both the SSD logical volume tables';
    COMMENT ON COLUMN csm_vg_ssd.vg_name is 'unique identifier for this ssd partition';
    COMMENT ON COLUMN csm_vg_ssd.node_name is 'identifies which node';
    COMMENT ON COLUMN csm_vg_ssd.serial_number is 'serial number for the ssd';
    COMMENT ON COLUMN csm_vg_ssd.ssd_allocation is 'the amount of space (in bytes) that this ssd contributes to the volume group. Can not be less than zero. The total sum of these fields should equal total_size of the this vg in the vg table';
    COMMENT ON INDEX uk_csm_vg_ssd_a IS 'index on vg_name, node_name, serial_number';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_vg_ssd_history (
---------------------------------------------------------------------------------------------------
    history_time            timestamp   not null,
    vg_name                 text        not null,
    node_name               text        not null,
    serial_number           text        not null,
    ssd_allocation          bigint      not null,
    operation               char(1)     not null,
    archive_history_time    timestamp
);

-------------------------------------------------
-- csm_vg_ssd_history_index
-------------------------------------------------

CREATE INDEX ix_csm_vg_ssd_history_a
    on csm_vg_ssd_history(history_time);

CREATE INDEX ix_csm_vg_ssd_history_b
    on csm_vg_ssd_history (vg_name, node_name);

CREATE INDEX ix_csm_vg_ssd_history_c
    on csm_vg_ssd_history (ctid);

CREATE INDEX ix_csm_vg_ssd_history_d
    on csm_vg_ssd_history (archive_history_time);

-------------------------------------------------
-- csm_vg_ssd_history_comments
-------------------------------------------------
    COMMENT ON TABLE csm_vg_ssd_history is 'this table contains historical information that references both the SSD logical volume tables';
    COMMENT ON COLUMN csm_vg_ssd_history.history_time is 'time when enters into the history table';
    COMMENT ON COLUMN csm_vg_ssd_history.vg_name is 'unique identifier for this ssd partition';
    COMMENT ON COLUMN csm_vg_ssd_history.node_name is 'identifies which node';
    COMMENT ON COLUMN csm_vg_ssd_history.serial_number is 'serial number for the ssd';
    COMMENT ON COLUMN csm_vg_ssd_history.ssd_allocation is 'the amount of space (in bytes) that this ssd contributes to the volume group. Can not be less than zero. The total sum of these fields should equal total_size of the this vg in the vg table';
    COMMENT ON COLUMN csm_vg_ssd_history.operation is 'operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)';
    COMMENT ON COLUMN csm_vg_ssd_history.archive_history_time is 'timestamp when the history data has been archived and sent to: BDS, archive file, and or other';
    COMMENT ON INDEX ix_csm_vg_ssd_history_a IS 'index on history_time';
    COMMENT ON INDEX ix_csm_vg_ssd_history_b IS 'index on vg_name, node_name';
    COMMENT ON INDEX ix_csm_vg_ssd_history_c IS 'index on ctid';
    COMMENT ON INDEX ix_csm_vg_ssd_history_d IS 'index on archive_history_time';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_lv (
---------------------------------------------------------------------------------------------------
    logical_volume_name     text        not null,
    node_name               text        not null,
    allocation_id           bigint      not null,
    vg_name                 text        not null,
    state                   char(1)     not null,
    current_size            bigint      not null,
    max_size                bigint      not null,
    begin_time              timestamp   not null,
    updated_time            timestamp,
    file_system_mount       text,
    file_system_type        text,
    PRIMARY KEY (logical_volume_name, node_name),
    FOREIGN KEY (node_name, vg_name) references csm_vg(node_name, vg_name),
    FOREIGN KEY (allocation_id, node_name) references csm_allocation_node(allocation_id, node_name)
);

-------------------------------------------------
-- csm_lv_index
-------------------------------------------------

-- automatically created index ON pkey (logical_volume_name, node_name)

CREATE INDEX ix_csm_lv_a
    on csm_lv (logical_volume_name);

-------------------------------------------------
-- csm_lv_comments
-------------------------------------------------
    COMMENT ON TABLE csm_lv is 'contains information on the active ssd partitions in the system';
    COMMENT ON COLUMN csm_lv.logical_volume_name is 'unique identifier for this ssd partition';
    COMMENT ON COLUMN csm_lv.node_name is 'node a part of this group';
    COMMENT ON COLUMN csm_lv.allocation_id is 'unique identifier for this allocation';
    COMMENT ON COLUMN csm_lv.vg_name is 'volume group name';
    COMMENT ON COLUMN csm_lv.state is 'state: (c)reated, (m)ounted, (s)hrinking, (r)emoved';    
    COMMENT ON COLUMN csm_lv.current_size is 'current size (in bytes)';
    COMMENT ON COLUMN csm_lv.max_size is 'max size (in bytes) at runtime';
    COMMENT ON COLUMN csm_lv.begin_time is 'when the partitioning begins';
    COMMENT ON COLUMN csm_lv.updated_time is 'when it was last updated';
    COMMENT ON COLUMN csm_lv.file_system_mount is 'identifies the file system and mount point';
    COMMENT ON COLUMN csm_lv.file_system_type is 'identifies the file system and its partition';
    COMMENT ON INDEX csm_lv_pkey IS 'pkey index on logical_volume_name, node_name';
    COMMENT ON INDEX ix_csm_lv_a IS 'index on logical_volume_name';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_lv_history (
---------------------------------------------------------------------------------------------------
    history_time            timestamp   not null,
    logical_volume_name     text        not null,
    node_name               text        not null,
    allocation_id           bigint,
    vg_name                 text,
    state                   char(1)     not null,
    current_size            bigint      not null,
    max_size                bigint      not null,
    begin_time              timestamp   not null,
    updated_time            timestamp,
    end_time                timestamp,  --not null,
    file_system_mount       text,
    file_system_type        text,
    num_bytes_read          bigint,     --not null,
    num_bytes_written       bigint,     --not null,
    operation               char(1),    --not null,
    archive_history_time    timestamp,
    num_reads               bigint,
    num_writes              bigint
);

-------------------------------------------------
-- csm_lv_history_indexes
-------------------------------------------------

CREATE INDEX ix_csm_lv_history_a
    on csm_lv_history (history_time);

CREATE INDEX ix_csm_lv_history_b
    on csm_lv_history (logical_volume_name);

CREATE INDEX ix_csm_lv_history_c
    on csm_lv_history (ctid);

CREATE INDEX ix_csm_lv_history_d
    on csm_lv_history (archive_history_time);

-------------------------------------------------
-- csm_lv_history_comments
-------------------------------------------------
    COMMENT ON TABLE csm_lv_history is 'contains historical information associated with previously active ssd partitions';
    COMMENT ON COLUMN csm_lv_history.history_time is 'this is when the lv enters the history table';
    COMMENT ON COLUMN csm_lv_history.logical_volume_name is 'unique identifier for this ssd partition';
    COMMENT ON COLUMN csm_lv_history.node_name is 'node a part of this group';
    COMMENT ON COLUMN csm_lv_history.allocation_id is 'unique identifier for this allocation';
    COMMENT ON COLUMN csm_lv_history.vg_name is 'volume group name';
    COMMENT ON COLUMN csm_lv_history.state is 'state: (c)reated, (m)ounted, (s)hrinking, (r)emoved';
    COMMENT ON COLUMN csm_lv_history.current_size is 'current size (in bytes)';
    COMMENT ON COLUMN csm_lv_history.max_size is 'max size (in bytes) at runtime';
    COMMENT ON COLUMN csm_lv_history.begin_time is 'when the partitioning begins';
    COMMENT ON COLUMN csm_lv_history.updated_time is 'when it was last updated';
    COMMENT ON COLUMN csm_lv_history.end_time is 'when the partitioning stage ends';
    COMMENT ON COLUMN csm_lv_history.file_system_mount is 'identifies the file system and mount point';
    COMMENT ON COLUMN csm_lv_history.file_system_type is 'identifies the file system and its partition';
    COMMENT ON COLUMN csm_lv_history.num_bytes_read is 'number of bytes read during the life of this partition';
    COMMENT ON COLUMN csm_lv_history.num_bytes_written is 'number of bytes written during the life of this partition';
    COMMENT ON COLUMN csm_lv_history.operation is 'operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)';
    COMMENT ON COLUMN csm_lv_history.archive_history_time is 'timestamp when the history data has been archived and sent to: BDS, archive file, and or other';
    COMMENT ON COLUMN csm_lv_history.num_reads is 'number of read during the life of this partition';
    COMMENT ON COLUMN csm_lv_history.num_writes is 'number of writes during the life of this partition';
    COMMENT ON INDEX ix_csm_lv_history_a IS 'index on history_time';
    COMMENT ON INDEX ix_csm_lv_history_b IS 'index on logical_volume_name';
    COMMENT ON INDEX ix_csm_lv_history_c IS 'index on ctid';
    COMMENT ON INDEX ix_csm_lv_history_d IS 'index on archive_history_time';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_lv_update_history (
---------------------------------------------------------------------------------------------------
    history_time            timestamp   not null,
    logical_volume_name     text        not null,
    allocation_id           bigint      not null,
    state                   char(1)     not null,
    current_size            bigint      not null,
    updated_time            timestamp,
    operation               char(1),    --not null,
    archive_history_time    timestamp
);

-------------------------------------------------
-- csm_lv_update_history_indexes
-------------------------------------------------

CREATE INDEX ix_csm_lv_update_history_a
    on csm_lv_update_history (history_time);

CREATE INDEX ix_csm_lv_update_history_b
    on csm_lv_update_history (logical_volume_name);

CREATE INDEX ix_csm_lv_update_history_c
    on csm_lv_update_history (ctid);

CREATE INDEX ix_csm_lv_update_history_d
    on csm_lv_update_history (archive_history_time);

-------------------------------------------------
-- csm_lv_update_history_comments
-------------------------------------------------
    COMMENT ON TABLE csm_lv_update_history is 'contains historical information associated with lv updates';
    COMMENT ON COLUMN csm_lv_update_history.history_time is 'this is when the lv update enters the history table';
    COMMENT ON COLUMN csm_lv_update_history.logical_volume_name is 'unique identifier for this ssd partition';
    COMMENT ON COLUMN csm_lv_update_history.allocation_id is 'unique identifier for this allocation';
    COMMENT ON COLUMN csm_lv_update_history.state is 'state: (c)reate, (m)ounted, (s)hrinking, (r)emoved';
    COMMENT ON COLUMN csm_lv_update_history.current_size is 'current size (in bytes)';
    COMMENT ON COLUMN csm_lv_update_history.updated_time is 'when it was last updated';
    COMMENT ON COLUMN csm_lv_update_history.operation is 'operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)';
    COMMENT ON COLUMN csm_lv_update_history.archive_history_time is 'timestamp when the history data has been archived and sent to: BDS, archive file, and or other';
    COMMENT ON INDEX ix_csm_lv_update_history_a IS 'index on history_time';
    COMMENT ON INDEX ix_csm_lv_update_history_b IS 'index on logical_volume_name';
    COMMENT ON INDEX ix_csm_lv_update_history_c IS 'index on ctid';
    COMMENT ON INDEX ix_csm_lv_update_history_d IS 'index on archive_history_time';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_switch (
---------------------------------------------------------------------------------------------------
    switch_name                 text        not null,
    serial_number               text,
    discovery_time              timestamp,
    collection_time             timestamp,
    comment                     text,
    description                 text,
    fw_version                  text,
    gu_id                       text,
    has_ufm_agent               boolean,
    hw_version                  text,
    ip                          text,
    model                       text,
    num_modules                 int,
    physical_frame_location     text,
    physical_u_location         text,
    ps_id                       text,
    role                        text,
    server_operation_mode       text,
    sm_mode                     text,
    state                       text,
    sw_version                  text,
    system_guid                 text,
    system_name                 text,
    total_alarms                int,
    type                        text,
    vendor                      text,
PRIMARY KEY (switch_name)
);

-------------------------------------------------
-- csm_switch_indexes
-------------------------------------------------

-- automatically created index ON pkey (serial_number)

-- CREATE INDEX ix_csm_switch_a
--    on csm_switch (switch_name);

-------------------------------------------------
-- csm_switch_comments
-------------------------------------------------
    COMMENT ON TABLE csm_switch is 'logical extension of the xcat switch table';
    COMMENT ON COLUMN csm_switch.switch_name is 'switch name: Identification of the system For hosts, it is caguid, For 1U switch, it is switchguid, For modular switches, is it sysimgguid';
    COMMENT ON COLUMN csm_switch.serial_number is 'identifies the switch this information is for';
    COMMENT ON COLUMN csm_switch.discovery_time is 'time the switch collected at inventory time';
    COMMENT ON COLUMN csm_switch.collection_time is 'time the switch was initially connected';
    COMMENT ON COLUMN csm_switch.comment is 'a comment can be generated for this field';
    COMMENT ON COLUMN csm_switch.description is 'description of system – system type of this systems (More options: SHArP, MSX1710 , CS7520)';
    COMMENT ON COLUMN csm_switch.fw_version is 'firmware version of the Switch or HCA';
    COMMENT ON COLUMN csm_switch.gu_id is 'Node guid of the system. In case of HCA, it is the caguid. In case of Switch, it is the switchguid';
    COMMENT ON COLUMN csm_switch.has_ufm_agent is 'indicate if system (Switch or Host) is running a UFM Agent';
    COMMENT ON COLUMN csm_switch.hw_version is 'hardware version related to the switch';
    COMMENT ON COLUMN csm_switch.ip is 'ip address of the system (Switch or Host)  (0.0.0.0 in case ip address not available)';
    COMMENT ON COLUMN csm_switch.model is 'system model – in case of switch, it is the switch model, For hosts – Computer';
    COMMENT ON COLUMN csm_switch.num_modules is 'number of modules attached to this switch. This is the number of expected records in the csm_switch inventory table associated with this switch name.';
    COMMENT ON COLUMN csm_switch.physical_frame_location is 'where the switch is located';
    COMMENT ON COLUMN csm_switch.physical_u_location is 'physical u location (position in the frame) where the switch is located';
    COMMENT ON COLUMN csm_switch.ps_id is 'PSID (Parameter-Set IDentification) is a 16-ascii character string embedded in the firmware image which provides a unique identification for the configuration of the firmware.';
    COMMENT ON COLUMN csm_switch.role is 'Type/Role of system in the current fabric topology: Tor / Core / Endpoint (host). (Optional Values: core, tor, endpoint)';
    COMMENT ON COLUMN csm_switch.server_operation_mode is 'Operation mode of system. (Optional Values: Stand_Alone, HA_Active, HA_StandBy, Not_UFM_Server, Router, Gateway, Switch)';
    COMMENT ON COLUMN csm_switch.sm_mode is 'Indicate if SM is running on that system. (Optional Values: noSM, activeSM, hasSM)';
    COMMENT ON COLUMN csm_switch.state is 'runtime state of the system. (Optional Values: active, rebooting, down, error (failed to reboot))';
    COMMENT ON COLUMN csm_switch.sw_version is 'software version of the system – full MLNX_OS version. Relevant only for MLNX-OS systems (Not available for Hosts)';
    COMMENT ON COLUMN csm_switch.system_guid is 'system image guid for that system';
    COMMENT ON COLUMN csm_switch.system_name is 'system name as it appear on the system node description';
    COMMENT ON COLUMN csm_switch.total_alarms is 'total number of alarms which are currently exist on the system';
    COMMENT ON COLUMN csm_switch.type is 'type of system. (Optional Values: switch, host, gateway)';
    COMMENT ON COLUMN csm_switch.vendor is 'system vendor';
    COMMENT ON INDEX csm_switch_pkey IS 'pkey index on switch_name';
--  COMMENT ON INDEX ix_csm_switch_a IS 'index on switch_name';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_switch_history (
---------------------------------------------------------------------------------------------------
    history_time                timestamp   not null,
    switch_name                 text        not null,
    serial_number               text,
    discovery_time              timestamp,
    collection_time             timestamp,
    comment                     text,
    description                 text,
    fw_version                  text,
    gu_id                       text,
    has_ufm_agent               boolean,
    hw_version                  text,
    ip                          text,
    model                       text,
    num_modules                 int,
    physical_frame_location     text,
    physical_u_location         text,
    ps_id                       text,
    role                        text,
    server_operation_mode       text,
    sm_mode                     text,
    state                       text,
    sw_version                  text,
    system_guid                 text,
    system_name                 text,
    total_alarms                int,
    type                        text,
    vendor                      text,
    operation                   char(1)     not null,
    archive_history_time        timestamp
);

-------------------------------------------------
-- csm_switch_history_indexes
-------------------------------------------------

CREATE INDEX ix_csm_switch_history_a
    on csm_switch_history (history_time);

CREATE INDEX ix_csm_switch_history_b
    on csm_switch_history (switch_name, history_time);

CREATE INDEX ix_csm_switch_history_c
    on csm_switch_history (ctid);

CREATE INDEX ix_csm_switch_history_d
    on csm_switch_history (archive_history_time);

-------------------------------------------------
-- csm_switch_history_comments
-------------------------------------------------
    COMMENT ON TABLE csm_switch_history is 'contains historical information associated with individual switches';
    COMMENT ON COLUMN csm_switch_history.history_time is 'the time when the switch enters the history table';
    COMMENT ON COLUMN csm_switch_history.switch_name is 'switch name: Identification of the system For hosts, it is caguid, For 1U switch, it is switchguid, For modular switches, is it sysimgguid';
    COMMENT ON COLUMN csm_switch_history.serial_number is 'identifies the switch this information is for';
    COMMENT ON COLUMN csm_switch_history.discovery_time is 'time the switch collected at inventory time';
    COMMENT ON COLUMN csm_switch_history.collection_time is 'time the switch was initially connected';
    COMMENT ON COLUMN csm_switch_history.comment is 'a comment can be generated for this field';
    COMMENT ON COLUMN csm_switch_history.description is 'Description of system – system type of this systems (More options: SHArP, MSX1710 , CS7520)';
    COMMENT ON COLUMN csm_switch_history.fw_version is 'firmware version of the Switch or HCA';
    COMMENT ON COLUMN csm_switch_history.gu_id is 'Node guid of the system. In case of HCA, it is the caguid. In case of Switch, it is the switchguid';
    COMMENT ON COLUMN csm_switch_history.has_ufm_agent is 'Indicate if system (Switch or Host) is running a UFM Agent';
    COMMENT ON COLUMN csm_switch_history.hw_version is 'hardware version related to the switch';
    COMMENT ON COLUMN csm_switch_history.ip is 'ip address of the system (Switch or Host)  (0.0.0.0 in case ip address not available)';
    COMMENT ON COLUMN csm_switch_history.model is 'System model – in case of switch, it is the switch model, For hosts – Computer';
    COMMENT ON COLUMN csm_switch_history.num_modules is 'number of modules attached to this switch. This is the number of expected records in the csm_switch inventory table associated with this switch name.';
    COMMENT ON COLUMN csm_switch_history.physical_frame_location is 'where the switch is located';
    COMMENT ON COLUMN csm_switch_history.physical_u_location is 'physical u location (position in the frame) where the switch is located';
    COMMENT ON COLUMN csm_switch_history.ps_id is 'PSID (Parameter-Set IDentification) is a 16-ascii character string embedded in the firmware image which provides a unique identification for the configuration of the firmware.';
    COMMENT ON COLUMN csm_switch_history.role is 'Type/Role of system in the current fabric topology: Tor / Core / Endpoint (host). (Optional Values: core, tor, endpoint)';
    COMMENT ON COLUMN csm_switch_history.server_operation_mode is 'Operation mode of system. (Optional Values: Stand_Alone, HA_Active, HA_StandBy, Not_UFM_Server, Router, Gateway, Switch)';
    COMMENT ON COLUMN csm_switch_history.sm_mode is 'Indicate if SM is running on that system. (Optional Values: noSM, activeSM, hasSM)';
    COMMENT ON COLUMN csm_switch_history.state is 'runtime state of the system. (Optional Values: active, rebooting, down, error (failed to reboot))';
    COMMENT ON COLUMN csm_switch_history.sw_version is 'software version of the system – full MLNX_OS version. Relevant only for MLNX-OS systems (Not available for Hosts)';
    COMMENT ON COLUMN csm_switch_history.system_guid is 'system image guid for that system';
    COMMENT ON COLUMN csm_switch_history.system_name is 'system name as it appear on the system node description';
    COMMENT ON COLUMN csm_switch_history.total_alarms is 'total number of alarms which are currently exist on the system';
    COMMENT ON COLUMN csm_switch_history.type is 'type of system. (Optional Values: switch, host, gateway)';
    COMMENT ON COLUMN csm_switch_history.vendor is 'system_vendor';
    COMMENT ON COLUMN csm_switch_history.operation is 'operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)';
    COMMENT ON COLUMN csm_switch_history.archive_history_time is 'timestamp when the history data has been archived and sent to: BDS, archive file, and or other';
    COMMENT ON INDEX ix_csm_switch_history_a IS 'index on history_time';
    COMMENT ON INDEX ix_csm_switch_history_b IS 'index on switch_name, history_time';
    COMMENT ON INDEX ix_csm_switch_history_c IS 'index on ctid';
    COMMENT ON INDEX ix_csm_switch_history_d IS 'index on archive_history_time';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_ib_cable (
---------------------------------------------------------------------------------------------------
    serial_number       text        not null,
    discovery_time      timestamp,
    collection_time     timestamp,
    comment             text,
    guid_s1             text,
    guid_s2             text,
    identifier          text, 
    length              text,
    name                text, 
    part_number         text,
    port_s1             text,
    port_s2             text,
    revision            text,
    severity            text,
    type                text,
    width               text,
    PRIMARY KEY (serial_number)
);

-------------------------------------------------
-- csm_ib_cable_index
-------------------------------------------------

-- automatically created index ON pkey (serial_number)

-------------------------------------------------
-- csm_ib_cable_comments
-------------------------------------------------
    COMMENT ON TABLE csm_ib_cable is 'contains information about the infiniband cable';
    COMMENT ON COLUMN csm_ib_cable.serial_number is 'identifies the cables serial number';
    COMMENT ON COLUMN csm_ib_cable.discovery_time is 'First time the ib cable was found in the system';
    COMMENT ON COLUMN csm_ib_cable.collection_time is 'Last time the ib cable inventory was collected';
    COMMENT ON COLUMN csm_ib_cable.comment is 'comment can be generated for this field';
    COMMENT ON COLUMN csm_ib_cable.guid_s1 is 'guid: side 1 of the cable';
    COMMENT ON COLUMN csm_ib_cable.guid_s2 is 'guid: side 2 of the cable';
    COMMENT ON COLUMN csm_ib_cable.identifier is 'cable identifier (example value: QSFP+)';
    COMMENT ON COLUMN csm_ib_cable.length is 'the length of the cable';
    COMMENT ON COLUMN csm_ib_cable.name is 'name (Id) of link object in UFM. Based on link sorce and destination.';
    COMMENT ON COLUMN csm_ib_cable.part_number is 'part number of this particular ib cable';
    COMMENT ON COLUMN csm_ib_cable.port_s1 is 'port: side 1 of the cable';
    COMMENT ON COLUMN csm_ib_cable.port_s2 is 'port: side 2 of the cable';
    COMMENT ON COLUMN csm_ib_cable.revision is 'hardware revision associated with this ib cable';
    COMMENT ON COLUMN csm_ib_cable.severity is 'severity associated with this ib cable (severity of link according to highest severity of related events)';
    COMMENT ON COLUMN csm_ib_cable.type is 'field from UFM (technology ) - the specific type of cable used (example used : copper cable - unequalized)';
    COMMENT ON COLUMN csm_ib_cable.width is 'the width of the cable - physical state of IB port (Optional Values: IB_1x ,IB_4x, IB_8x, IB_12x)';
    COMMENT ON INDEX csm_ib_cable_pkey IS 'pkey index on serial_number';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_ib_cable_history (
---------------------------------------------------------------------------------------------------
    history_time            timestamp   not null,
    serial_number           text        not null,
    discovery_time          timestamp,
    collection_time         timestamp,
    comment                 text,
    guid_s1                 text,
    guid_s2                 text,
    identifier              text,
    length                  text,
    name                    text,
    part_number             text,
    port_s1                 text,
    port_s2                 text,
    revision                text,
    severity                text,
    type                    text,
    width                   text,
    operation               char(1)     not null,
    archive_history_time    timestamp    
);

-------------------------------------------------
-- csm_ib_cable_history_index
-------------------------------------------------

CREATE INDEX ix_csm_ib_cable_history_a
    on csm_ib_cable_history (history_time);

CREATE INDEX ix_csm_ib_cable_history_b
    on csm_ib_cable_history (serial_number);

CREATE INDEX ix_csm_ib_cable_history_c
    on csm_ib_cable_history (ctid);

CREATE INDEX ix_csm_ib_cable_history_d
    on csm_ib_cable_history (archive_history_time);

-------------------------------------------------
-- csm_ib_cable_history_comments
-------------------------------------------------
    COMMENT ON TABLE csm_ib_cable_history is 'contains historical information about the infiniband cable';
    COMMENT ON COLUMN csm_ib_cable_history.history_time is 'the time when the cable enters the history table';
    COMMENT ON COLUMN csm_ib_cable_history.serial_number is 'identifies the cables serial number';
    COMMENT ON COLUMN csm_ib_cable_history.discovery_time is 'first time the ib cable was found in the system';
    COMMENT ON COLUMN csm_ib_cable_history.collection_time is 'comment can be generated for this field';
    COMMENT ON COLUMN csm_ib_cable_history.comment is 'comment can be generated for this field';
    COMMENT ON COLUMN csm_ib_cable_history.guid_s1 is 'guid: side 1 of the cable';
    COMMENT ON COLUMN csm_ib_cable_history.guid_s2 is 'guid: side 2 of the cable';
    COMMENT ON COLUMN csm_ib_cable_history.identifier is 'cable identifier (example value: QSFP+)';
    COMMENT ON COLUMN csm_ib_cable_history.length is 'the length of the cable';
    COMMENT ON COLUMN csm_ib_cable_history.name is 'name (Id) of link object in UFM. Based on link sorce and destination.';
    COMMENT ON COLUMN csm_ib_cable_history.part_number is 'part number of this particular ib cable';
    COMMENT ON COLUMN csm_ib_cable_history.port_s1 is 'port: side 1 of the cable';
    COMMENT ON COLUMN csm_ib_cable_history.port_s2 is 'port: side 2 of the cable';
    COMMENT ON COLUMN csm_ib_cable_history.revision is 'hardware revision associated with this ib cable';
    COMMENT ON COLUMN csm_ib_cable_history.severity is 'severity associated with this ib cable (severity of link according to highest severity of related events)';
    COMMENT ON COLUMN csm_ib_cable_history.type is 'field from UFM (technology ) - the specific type of cable used (example used : copper cable - unequalized)';
    COMMENT ON COLUMN csm_ib_cable_history.width is 'the width of the cable - physical state of IB port (Optional Values: IB_1x ,IB_4x, IB_8x, IB_12x)';
    COMMENT ON COLUMN csm_ib_cable_history.operation is 'operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)';
    COMMENT ON COLUMN csm_ib_cable_history.archive_history_time is 'timestamp when the history data has been archived and sent to: BDS, archive file, and or other';
    COMMENT ON INDEX ix_csm_ib_cable_history_a IS 'index on history_time';
    COMMENT ON INDEX ix_csm_ib_cable_history_b IS 'index on serial_number';
    COMMENT ON INDEX ix_csm_ib_cable_history_c IS 'index on ctid';
    COMMENT ON INDEX ix_csm_ib_cable_history_d IS 'index on archive_history_time';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_switch_inventory (
---------------------------------------------------------------------------------------------------
    name                text        not null,
    host_system_guid    text        not null,
    discovery_time      timestamp,
    collection_time     timestamp,
    comment             text,
    description         text,
    device_name         text,
    device_type         text,
    hw_version          text,
    max_ib_ports        int,
    module_index        int,
    number_of_chips     int,
    path                text,
    serial_number       text,
    severity            text,
    status              text,
    type                text,
    fw_version          text,
    PRIMARY KEY (name),
    FOREIGN KEY (host_system_guid) references csm_switch(switch_name)
);

-------------------------------------------------
-- csm_switch_inventory_index
-------------------------------------------------

-- automatically created index ON pkey (name)

-------------------------------------------------
-- csm_switch_inventory_comments
-------------------------------------------------
    COMMENT ON TABLE csm_switch_inventory is 'contains information about the switch inventory';
    COMMENT ON COLUMN csm_switch_inventory.name is 'name (identifier) of this module in UFM.';
    COMMENT ON COLUMN csm_switch_inventory.host_system_guid is 'the system image guid of the hosting system.';
    COMMENT ON COLUMN csm_switch_inventory.discovery_time is 'first time the module was found in the system';
    COMMENT ON COLUMN csm_switch_inventory.collection_time is 'last time the module inventory was collected';
    COMMENT ON COLUMN csm_switch_inventory.comment is 'system administrator comment about this module';
    COMMENT ON COLUMN csm_switch_inventory.description is 'description type of module - can be the module type: system, FAN, MGMT,PS or the type of module in case of line / spine modules: SIB7510(Barracud line), SIB7520(Barracuda spine)';
    COMMENT ON COLUMN csm_switch_inventory.device_name is 'name of device containing this module.';
    COMMENT ON COLUMN csm_switch_inventory.device_type is 'type of device module belongs to.';
    COMMENT ON COLUMN csm_switch_inventory.hw_version is 'hardware version related to the switch';
    COMMENT ON COLUMN csm_switch_inventory.max_ib_ports is 'maximum number of external ports of this module.';
    COMMENT ON COLUMN csm_switch_inventory.module_index is 'index of module. Each module type has separate index: FAN1,FAN2,FAN3…PS1,PS2';
    COMMENT ON COLUMN csm_switch_inventory.number_of_chips is 'number of chips which are contained in this module. (relevant only for line / spine modules, for all other modules number_of_chips=0)';
    COMMENT ON COLUMN csm_switch_inventory.path is 'full path of module object. Path format: site-name (number of devices) / device type: device-name / module description module index.';
    COMMENT ON COLUMN csm_switch_inventory.serial_number is 'serial_number of the module.';
    COMMENT ON COLUMN csm_switch_inventory.severity is 'severity of the module according to the highest severity of related events. values: Info, Warning, Minor, Critical';
    COMMENT ON COLUMN csm_switch_inventory.status is 'current module status. valid values: ok, fault';
    COMMENT ON COLUMN csm_switch_inventory.type is 'The category of this piece of hardware inventory. For example: "FAN", "PS", "SYSTEM", or "MGMT".';
    COMMENT ON COLUMN csm_switch_inventory.fw_version is 'The firmware version on this piece of inventory.';
    COMMENT ON INDEX csm_switch_inventory_pkey IS 'pkey index on name';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_switch_inventory_history (
---------------------------------------------------------------------------------------------------
    history_time            timestamp   not null,
    name                    text        not null,
    host_system_guid        text        not null,
    discovery_time          timestamp,
    collection_time         timestamp,
    comment                 text,
    description             text,
    device_name             text,
    device_type             text,
    hw_version              text,
    max_ib_ports            int,
    module_index            int,
    number_of_chips         int,
    path                    text,
    serial_number           text,
    severity                text,
    status                  text,
    operation               char(1)     not null,
    archive_history_time    timestamp,
    type                    text,
    fw_version              text
);

-------------------------------------------------
-- csm_switch_inventory_history_index
-------------------------------------------------

CREATE INDEX ix_csm_switch_inventory_history_a
    on csm_switch_inventory_history (history_time);

CREATE INDEX ix_csm_switch_inventory_history_b
    on csm_switch_inventory_history (name);

CREATE INDEX ix_csm_switch_inventory_history_c
    on csm_switch_inventory_history (ctid);

CREATE INDEX ix_csm_switch_inventory_history_d
    on csm_switch_inventory_history (archive_history_time);

-------------------------------------------------
-- csm_switch_inventory_comments
-------------------------------------------------
    COMMENT ON TABLE csm_switch_inventory_history is 'contains historical information about the switch inventory';
    COMMENT ON COLUMN csm_switch_inventory_history.history_time is 'the time when the inventory record enters the history table';
    COMMENT ON COLUMN csm_switch_inventory_history.name is 'name (identifier) of this module in UFM.';
    COMMENT ON COLUMN csm_switch_inventory_history.host_system_guid is 'the system image guid of the hosting system.';
    COMMENT ON COLUMN csm_switch_inventory_history.discovery_time is 'first time the module was found in the system';
    COMMENT ON COLUMN csm_switch_inventory_history.collection_time is 'last time the module inventory was collected';
    COMMENT ON COLUMN csm_switch_inventory_history.comment is 'system administrator comment about this module';
    COMMENT ON COLUMN csm_switch_inventory_history.description is 'description type of module - can be the module type: system, FAN, MGMT,PS or the type of module in case of line / spine modules: SIB7510(Barracud line), SIB7520(Barracuda spine)';
    COMMENT ON COLUMN csm_switch_inventory_history.device_name is 'name of device containing this module.';
    COMMENT ON COLUMN csm_switch_inventory_history.device_type is 'type of device module belongs to.';
    COMMENT ON COLUMN csm_switch_inventory_history.hw_version is 'hardware version related to the switch';
    COMMENT ON COLUMN csm_switch_inventory_history.max_ib_ports is 'maximum number of external ports of this module.';
    COMMENT ON COLUMN csm_switch_inventory_history.module_index is 'index of module. Each module type has separate index: FAN1,FAN2,FAN3…PS1,PS2';
    COMMENT ON COLUMN csm_switch_inventory_history.number_of_chips is 'number of chips which are contained in this module. (relevant only for line / spine modules, for all other modules number_of_chips=0)';
    COMMENT ON COLUMN csm_switch_inventory_history.path is 'full path of module object. Path format: site-name (number of devices) / device type: device-name / module description module index.';
    COMMENT ON COLUMN csm_switch_inventory_history.serial_number is 'serial_number of the module.';
    COMMENT ON COLUMN csm_switch_inventory_history.severity is 'severity of the module according to the highest severity of related events. values: Info, Warning, Minor, Critical';
    COMMENT ON COLUMN csm_switch_inventory_history.status is 'current module status. valid values: ok, fault';
    COMMENT ON COLUMN csm_switch_inventory_history.operation is 'operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)';
    COMMENT ON COLUMN csm_switch_inventory_history.archive_history_time is 'timestamp when the history data has been archived and sent to: BDS, archive file, and or other';
    COMMENT ON COLUMN csm_switch_inventory_history.type is 'The category of this piece of hardware inventory. For example: "FAN", "PS", "SYSTEM", or "MGMT".';
    COMMENT ON COLUMN csm_switch_inventory_history.fw_version is 'The firmware version on this piece of inventory.';
    COMMENT ON INDEX ix_csm_switch_inventory_history_a IS 'index on history_time';
    COMMENT ON INDEX ix_csm_switch_inventory_history_b IS 'index on name';
    COMMENT ON INDEX ix_csm_switch_inventory_history_c IS 'index on ctid';
    COMMENT ON INDEX ix_csm_switch_inventory_history_d IS 'index on archive_history_time';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_config (
---------------------------------------------------------------------------------------------------
    csm_config_id               bigserial,
    local_socket                text,
    mqtt_broker                 text,
    log_level                   text        array,
    buckets                     text        array,
    jitter_window_interval      int,
    jitter_window_duration      int,
    path_certificate            text,
    path_log                    text,
    create_time                 timestamp,
    PRIMARY KEY (csm_config_id)
);

-------------------------------------------------
-- csm_config_index
-------------------------------------------------

-- automatically created index ON pkey (csm_config_id)

-------------------------------------------------
-- csm_config_comments
-------------------------------------------------
    COMMENT ON TABLE csm_config is 'this table holds the csm_configuration data';
    COMMENT ON COLUMN csm_config.csm_config_id is 'the configuration identification';
    COMMENT ON COLUMN csm_config.local_socket is 'socket to use to local csm daemon';
    COMMENT ON COLUMN csm_config.mqtt_broker is 'ip: port';
    COMMENT ON COLUMN csm_config.log_level is 'db#, daemon.compute, daemon.aggragator, daemon.master, daemon.utility';
    COMMENT ON COLUMN csm_config.buckets is 'list of items to execute in buckets';
    COMMENT ON COLUMN csm_config.jitter_window_interval is 'jitter interval for compute agent (how often to wake up)';
    COMMENT ON COLUMN csm_config.jitter_window_duration is 'jitter duration for compute agent (duration of the window)';
    COMMENT ON COLUMN csm_config.path_certificate is 'location of certificates for authentication';
    COMMENT ON COLUMN csm_config.path_log is 'path where the daemon will log';
    COMMENT ON COLUMN csm_config.create_time is 'when these logs were created';
    COMMENT ON INDEX csm_config_pkey IS 'pkey index on csm_config_id';
    COMMENT ON SEQUENCE csm_config_csm_config_id_seq IS 'used to generate primary keys on config ids';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_config_history (
---------------------------------------------------------------------------------------------------
    history_time                timestamp   not null,
    csm_config_id               bigint,
    local_socket                text,
    mqtt_broker                 text,
    log_level                   text        array,
    buckets                     text        array,
    jitter_window_interval      int,
    jitter_window_duration      int,
    path_certificate            text,
    path_log                    text,
    create_time                 timestamp,
    archive_history_time        timestamp
);

-------------------------------------------------
-- csm_config_history_index
-------------------------------------------------

CREATE INDEX ix_csm_config_history_a
    on csm_config_history (history_time);

CREATE INDEX ix_csm_config_history_b
    on csm_config_history (csm_config_id);

CREATE INDEX ix_csm_config_history_c
    on csm_config_history (ctid);

CREATE INDEX ix_csm_config_history_d
    on csm_config_history (archive_history_time);

-------------------------------------------------
-- csm_config_history_comments
-------------------------------------------------
    COMMENT ON TABLE csm_config_history is 'this table holds the csm_configuration history data';
    COMMENT ON COLUMN csm_config_history.history_time is 'the time when the configuration enters the history table';
    COMMENT ON COLUMN csm_config_history.csm_config_id is 'the configuration identification';
    COMMENT ON COLUMN csm_config_history.local_socket is 'socket to use to local csm daemon';
    COMMENT ON COLUMN csm_config_history.mqtt_broker is 'ip: port';
    COMMENT ON COLUMN csm_config_history.log_level is 'db#, daemon.compute, daemon.aggragator, daemon.master, daemon.utility';
    COMMENT ON COLUMN csm_config_history.buckets is 'list of items to execute in buckets';
    COMMENT ON COLUMN csm_config_history.jitter_window_interval is 'jitter interval for compute agent (how often to wake up)';
    COMMENT ON COLUMN csm_config_history.jitter_window_duration is 'jitter duration for compute agent (duration of the window)';
    COMMENT ON COLUMN csm_config_history.path_certificate is 'location of certificates for authentication';
    COMMENT ON COLUMN csm_config_history.path_log is 'path where the daemon will log';
    COMMENT ON COLUMN csm_config_history.create_time is 'when these logs were created';
    COMMENT ON COLUMN csm_config_history.archive_history_time is 'timestamp when the history data has been archived and sent to: BDS, archive file, and or other';
    COMMENT ON INDEX ix_csm_config_history_a IS 'index on history_time';
    COMMENT ON INDEX ix_csm_config_history_b IS 'index on csm_config_id';
    COMMENT ON INDEX ix_csm_config_history_c IS 'index on ctid';
    COMMENT ON INDEX ix_csm_config_history_d IS 'index on archive_history_time';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_config_bucket (
---------------------------------------------------------------------------------------------------
    bucket_id               int,
    item_list               bigint,
    execution_interval      text,
    time_stamp              timestamp
);

-------------------------------------------------
-- csm_config_bucket_index
-------------------------------------------------

CREATE INDEX ix_csm_config_bucket_a
    on csm_config_bucket (bucket_id, item_list, time_stamp);

-------------------------------------------------
-- csm_config_bucket_comments
-------------------------------------------------
    COMMENT ON TABLE csm_config_bucket is 'this is the list of items that will placed in the bucket';
    COMMENT ON COLUMN csm_config_bucket.bucket_id is 'this is the identification of the bucket';
    COMMENT ON COLUMN csm_config_bucket.item_list is 'the item list within in the bucket';
    COMMENT ON COLUMN csm_config_bucket.execution_interval is 'execution interval (the counter)';
    COMMENT ON COLUMN csm_config_bucket.time_stamp is 'time when the process takes place';
    COMMENT ON INDEX ix_csm_config_bucket_a IS 'index on bucket_id, item_list, time_stamp';


---------------------------------------------------------------------------------------------------
CREATE TABLE csm_db_schema_version (
---------------------------------------------------------------------------------------------------
    version                     text,
    create_time                 timestamp default current_timestamp,
    comment                     text,
    PRIMARY KEY (version)
);

-------------------------------------------------
-- csm_db_schema_version_index
-------------------------------------------------

-- automatically created index ON pkey (version)

CREATE INDEX ix_csm_db_schema_version_a
    on csm_db_schema_version (version, create_time);

-------------------------------------------------
-- csm_db_schema_version_comments
-------------------------------------------------
    COMMENT ON TABLE csm_db_schema_version is 'this is the current database schema version';
    COMMENT ON COLUMN csm_db_schema_version.version is 'this is the current database schema version';
    COMMENT ON COLUMN csm_db_schema_version.create_time is 'time when the db was created';
    COMMENT ON COLUMN csm_db_schema_version.comment is 'comment';
    COMMENT ON INDEX csm_db_schema_version_pkey IS 'pkey index on version';
    COMMENT ON INDEX ix_csm_db_schema_version_a IS 'index on version, create_time';

---------------------------------------------------------------------------------------------------
CREATE TABLE csm_db_schema_version_history (
---------------------------------------------------------------------------------------------------
    history_time                timestamp   not null,
    version                     text,
    create_time                 timestamp,
    comment                     text,
    archive_history_time        timestamp
);

-------------------------------------------------
-- csm_db_schema_version_history_index
-------------------------------------------------

CREATE INDEX ix_csm_db_schema_version_history_a
    on csm_db_schema_version_history (history_time);
    
CREATE INDEX ix_csm_db_schema_version_history_b
    on csm_db_schema_version_history (version);

CREATE INDEX ix_csm_db_schema_version_history_c
    on csm_db_schema_version_history (ctid);

CREATE INDEX ix_csm_db_schema_version_history_d
    on csm_db_schema_version_history (archive_history_time);

-------------------------------------------------
-- csm_db_schema_version_history_comments
-------------------------------------------------
    COMMENT ON TABLE csm_db_schema_version_history is 'this is the historical database schema version';
    COMMENT ON COLUMN csm_db_schema_version_history.history_time is 'the time when the schema version enters the history table';
    COMMENT ON COLUMN csm_db_schema_version_history.version is 'this is the current database schema version';
    COMMENT ON COLUMN csm_db_schema_version_history.create_time is 'time when the db was created';
    COMMENT ON COLUMN csm_db_schema_version_history.comment is 'comment';
    COMMENT ON COLUMN csm_db_schema_version_history.archive_history_time is 'timestamp when the history data has been archived and sent to: BDS, archive file, and or other';
    COMMENT ON INDEX ix_csm_db_schema_version_history_a IS 'index on history_time';
    COMMENT ON INDEX ix_csm_db_schema_version_history_b IS 'index on version';
    COMMENT ON INDEX ix_csm_db_schema_version_history_c IS 'index on ctid';
    COMMENT ON INDEX ix_csm_db_schema_version_history_d IS 'index on archive_history_time';

COMMIT;
