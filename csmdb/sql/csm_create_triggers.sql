--===============================================================================
--
--   csm_create_triggers.sql
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
--   usage:                 run ./csm_db_script.sh <----- to create the csm_db with triggers
--   current_version:       17.0
--   create:                06-22-2016
--   last modified:         01-25-2019
--   change log:
--     17.0   - Moving this version to sync with DB schema version
--            - fn_csm_allocation_history_dump -        added field:    smt_mode
--            - fn_csm_allocation_update -              added field:    smt_mode
--            - fn_csm_allocation_update_state -        added field:    o_smt_mode
--            - fn_csm_allocation_finish_data_stats -   disables trigger to prevent duplication.
--            - fn_csm_lv_history_dump - 		added fields:	num_reads, num_writes
--            - fn_csm_allocation_dead_records_on_lv -  added in 'null' values for PERFORM fn_csm_lv_history_dump
--            - fn_csm_ssd_dead_records -               added in 'null' values for PERFORM fn_csm_lv_history_dump
--     16.2   - Moving this version to sync with DB schema version
--            fn_csm_switch_inventory_history_dump
--            - (Transactions were being recorded into the history table if a particular field was 'NULL')
--            fn_csm_allocation_delete_start -              Added in
--                                                        INVALID_STATE       CONSTANT integer := 1;
--                                                        INVALID_ALLOCATION  CONSTANT integer := 2;
--                                                        USING HINT = INVALID_STATE;
--                                                        USING HINT = INVALID_ALLOCATION; 
--                                                        Added OUT o_runtime bigint;
--            fn_csm_allocation_update_state -            Added OUT o_runtime bigint;
--            fn_csm_allocation_history_dump -            Removed Older exception message.
--                                                        USING HINT = INVALID_ALLOCATION; 
--            fn_csm_allocation_node_sharing_status -     error_code integer;
--                                                        INVALID_NODES  CONSTANT integer := 1;
--                                                        ABSENT_NODES  CONSTANT integer := 2;
--                                                        OCCUPIED_NODES CONSTANT integer := 3;
--                                                        BAD_STATE CONSTANT integer := 4;
--                                                        Included new error logic releted to the existence of nodes.
--                                                        USING HINT = OCCUPIED_NODES;
--                                                        USING HINT = BAD_STATE;
--                                                        Removed Older exception message
--     16.1   - Updated fn_csm_switch_children_inventory_collection to remove old records from database and have more user feedback data to CSM API
--            - Updated fn_csm_ib_cable_inventory_collection to remove old records from database and have more user feedback data to CSM API
--            - Updated fn_csm_switch_inventory_collection to remove old records from database and have more user feedback data to CSM API
--            - Added more verbose err_text to allocation dumped steps.
--            - Update 'fn_csm_ssd_dead_records' and 'fn_csm_ssd_history_dump'
--            - now clean up any lvs and vgs on an ssd before we delete the ssd from table.
--            - Included additional logic to fn_csm_switch_history_dump to handle 'NULL' fields during inventory collection UPDATES.
--            - (Transactions were being recorded into the history table if a particular field was 'NULL')
--            - Also removed the 'state' field from the UPDATE logic - as not needed.
--            - fn_csm_node_delete_function - updated comment with appropriate content.
--            - Update 'fn_csm_allocation_dead_records_on_lv' and 'fn_csm_allocation_dead_records_on_lv'
--            - now clean up any lvs before we delete the allocations from csm_allocation_node and csm_allocation tables.
--            - cleaned up fn_csm_lv_history_dump with additional checks.
--     16.0   - Moving this version to sync with DB schema version
--            - fn_csm_allocation_finish_data_stats - updated handling of gpu_usage and gpu_energy 
--            - fn_csm_allocation_history_dump - updated handling of gpu_usage and gpu_energy
--            - fn_csm_allocation_create_data_aggregator - updated handling of gpu_energy
--     4.3.88 - fn_csm_db_chema_version_history_dump. fix defect on UPDATE trigger. 
--     4.3.87 - fn_csm_ib_cable_inventory_collection. fix defect. duplicated port info. 
--     4.3.86 - Added fields to fn_csm_step_begin and fn_csm_step_end, fn_csm_allocation_node_sharing_status for diagnostics
--     4.3.85 - Added fields to fn_csm_allocation_history_dump, fn_csm_allocation_create_data_aggregator and fn_csm_allocation_finish_data_stats
--     4.3.84 - fn_csm_allocation_node_sharing_status - Improved the node sharing test to account for failed allocation transitions.
--              fn_csm_allocation_update_state - Tests to verify that the node states are valid (whitelist).
--     4.3.83 - fn_csm_allocation_delete_start - Added i_timeout_time to function, now tests for timeouts on deletes.
--     4.3.82 - tr_csm_allocation_node_change - took off DELETE triggers regarding duplicated records in history table.
--     4.3.81 - fn_csm_allocation_delete_start added conditional to prevent writing deleting twice.
--     4.3.80 - added functionality for discovered_dimms and discovered_ssds for fn_csm_node_attributes_query_details
--            - uncommented discovered_dimms and discovered_ssds from the node_details type
--     4.3.79 - commented out discovered_dimms from the node_details type
--     4.3.78 - fn_csm_node_update Added in discovered_ssds
--            - added discovered_ssds to TYPE node_details
--     4.3.77 - fn_csm_node_update Added in discovered_dimms
--            - added discovered_dimms to TYPE node_details
--     4.3.76 - fn_csm_allocation_revert Added to revert the allocation and allocation_state_history.
--              tr_csm_allocation_update Updated to trigger to react to initial inserts to staging-in.
--     4.3.75 - fn_csm_allocation_update_state Added legal staging-in to staging-out transition.
--     4.3.74 - fn_csm_lv_history_dump updates for 'transaction history' corrections
--     4.3.73 - updates for new database 14.23
--              fn_csm_lv_update_history_dump - added in allocation_id
--              fn_csm_vg_history_dump - added in logic to trigger function to handle non duplicated records
--              fn_csm_vg_ssd_history_dump - added in logic to trigger function to handle non duplicated records
--              fn_csm_lv_modified_history_dump - added in logic to trigger function to handle non duplicated records
--              fn_csm_lv_update_history_dump - added in logic to trigger function to handle non duplicated records
--              fn_csm_lv_history_dump – added in final transactional ‘state’ to the csm_lv_update_history table.
--     4.3.72 - fn_csm_allocation_update_state : Added an IN_SERVICE test for the nodes and switched to row locking.
--              fn_csm_allocation_node_sharing_status : Switched to row locking.
--     4.3.71 - fn_csm_switch_children_inventory_collection updates for new database 14.22
--     4.3.70 - Added mechanisms to fn_csm_allocation_history_dump and fn_csm_allocation_finish_data_stats to process staging-out pre-emption.
--     4.3.69 - fn_csm_switch_inventory_collection updates for CSM APIs and database 14.22.
--     4.3.68 - fn_csm_allocation_delete_start added for delete operation, locks rows. Added state to fn_csm_allocation_create_data_aggregator.
--     4.3.67 - fn_csm_switch_attributes_query_details and switch_details updates for CSM APIs and database 14.22
--     4.3.66 - revert some of the trigger functions back to the original design as requested (operation field does not appy to these tables)
--                  fn_csm_allocation_history_dump          - removed 'D' operation from step_history clean-up. and removed 'D' from allocation history dump.
--                  fn_csm_allocation_update                - removed - removed TG_OP on 'INSERT' into history table operation 'I'.
--                                                          - removed 'D' operation from final insert into history.
--                  fn_csm_allocation_node_change
--                  fn_csm_switch_history_dump              - removed num_ports
--                  fn_csm_step_history_insert              - removed entire function as not needed.
--                  fn_csm_step_history_dump                - removed 'D' operation from final insert into history.
--                  fn_csm_step_node_history_dump           - removed TG_OP on 'INSERT' into history table operation 'I'.
--                                                          - removed 'D' operation from final insert into history.
--                  fn_csm_diag_result_history_dump         - removed TG_OP on 'INSERT' into history table operation 'I'.
--                  fn_csm_lv_history_insert                - removed entire function as not needed.
--                  fn_csm_lv_upsert                        - added in 'INSERT' into csm_lv_update_history table to track initial 'I' operation.
--                  fn_csm_config_history_dump              - removed TG_OP on 'INSERT' into history table operation 'I'.
--                                                          - removed 'D' operation from final insert into history.
--                  fn_csm_db_schema_version_history_dump   - removed TG_OP on 'INSERT' into history table operation 'I'.
--                                                          - removed 'D' operation from final insert into history.
-------------------------------------------------------------------------------------------------------------------
--     4.3.65 - fn_csm_allocation_node_change: added new field - gpu_energy
--     4.3.64 - update
--              fn_csm_processor_socket_history_dump,
--              fn_csm_gpu_history_dump,
--              fn_csm_hca_history_dump,
--              fn_csm_dimm_history_dump
--              added in logic to trigger function to handle non duplicated records in the
--              history tables if values haven't changed. (helper for inventory collection)
-------------------------------------------------------------------------------------------------------------------
--     4.3.63 - update fn_csm_switch_history_dump - added in lost field hw_version.
--     4.3.62 - update fn_csm_node_update, fn_csm_ssd_history_dump, fn_csm_wear
--                  added in logic to trigger function to handle non duplicated records in the
--                  history tables if values haven't changed. (helper for inventory collection)
--     4.3.61 - update fn_csm_ras_type_update
--                  removed set_not_ready, set_ready
--                  added - set_state (same states defined for a node)
--     4.3.60 - update fn_csm_switch_inventory_history_dump - added in hw_version field
--              update fn_csm_switch_history_dump
--                  added in hw_version field
--                  modified field order - serial_number is towards the top under switch_name.
--     4.3.59 - update fn_csm_switch_attributes_query_details and type switch_details for db 14-18
--     4.3.58 - update fn_csm_node_attributes_query_details and type node_details for db 14-18
--     4.3.57 - update fn_csm_processor_socket_history_dump - field change - socket = physical_location
--              update fn_csm_switch_history_dump - removed os_version field
--     4.3.56 - update fn_csm_node_attributes_query_details and type node_details for db 14-17
--     4.3.55 - functions/triggers modified:
--              fn_csm_ssd_history_dump - removed total_size
--              fn_csm_node_state_history / tr_csm_node_state_history - modified to support INSERT of initial state transation
--              fn_csm_node_update/ tr_csm_node_update - modified and removed update_time (as automatically set with now())
--              fn_csm_node_delete - commented out the states/status change + modified csm_processor table name to csm_processor_socket
--     4.3.54 - functions/triggers new proposed implementation (added in operation field)
--              ( 'I' - INSERT, 'U' - UPDATE, 'D' - DELETE)
--              fn_csm_node_update + tr_csm_node_update
--              fn_csm_node_state + tr_csm_node_state
--              fn_csm_ssd_history_dump + tr_csm_ssd_history_dump
--              fn_csm_ssd_wear + tr_csm_ssd_wear
--              fn_csm_processor_socket_history_dump + tr_csm_processor_socket_history_dump
--              fn_csm_gpu_history_dump + tr_csm_gpu_history_dump
--              fn_csm_hca_history_dump + tr_csm_hca_history_dump 
--              fn_csm_dimm_history_dump + tr_csm_dimm_history_dump
--              fn_csm_db_schema_version_history_dump + tr_csm_db_schema_version_history_dump 
--              fn_csm_config_history_dump + tr_csm_config_history_dump
--              fn_csm_vg_history_dump + tr_csm_vg_history_dump
--              fn_csm_vg_ssd_history_dump + tr_csm_vg_ssd_history_dump
--              fn_csm_diag_run_history_dump + tr_csm_diag_run_history_dump
--              fn_csm_diag_result_history_dump + tr_csm_diag_result_history_dump
--              fn_csm_allocation_history_dump + tr_csm_allocation_history_dump
--              fn_csm_allocation_update + tr_csm_allocation_update
--              fn_csm_allocation_state_history_state_change + tr_csm_allocation_state_history_state_change
--              fn_csm_allocation_node_change + tr_csm_allocation_node_change
--              fn_csm_step_history_dump (added in operation 'D') + tr_csm_step_history_dump
--              fn_csm_step_node_history_dump + tr_csm_step_node_history_dump
--              fn_csm_switch_history_dump + tr_csm_switch_history_dump
--              fn_csm_ib_cable_history_dump + tr_csm_ib_cable_history_dump
--              fn_csm_lv_modified_history_dump + tr_csm_lv_modified_history_dump
--              fn_csm_lv_update_history_dump + tr_csm_lv_update_history_dump
--              fn_csm_lv_history_dump - added in operation field
--              removed - anything related to map_tag
--              removed - anything related to csm_switch_ports
--              fn_csm_switch_history_dump - added in serial_number, os_version
--              fn_csm_processor_socket_history_dump - modified available_cores = discovered_cores
-------------------------------------------------------------------------------------------------------------------
--     4.3.53 - update to fn_csm_node_attributes_query_details to deal with db 14-15
--     4.3.52 - update to all table changes
-------------------------------------------------------------------------------------------------------------------
-- 1.           fn_csm_node_update (field changes)
--              discovery_time          =   collection_time
--              available_processors    =   discovered_sockets
--              available_cores         =   discovered_cores
--              available_gpus          =   discovered_gpus
--              removed 'ready' field
-------------------------------------------------------------------------------------------------------------------
-- 2.           trigger and function name change
--              fn_csm_node_ready = fn_csm_node_state
--              tr_csm_node_ready = tr_csm_node_state
--              field modified - ready = state
-------------------------------------------------------------------------------------------------------------------
-- 3.           fn_csm_ssd_history_dump + tr_csm_ssd_history_dump
--              fields removed
--              discover_time, status
-------------------------------------------------------------------------------------------------------------------
-- 4.           trigger and function name change
--              fn_csm_processor_history_dump = fn_csm_processor_socket_history_dump
--              tr_csm_processor_history_dump = tr_csm_processor_socket_history_dump
--              removed 'status' field
-------------------------------------------------------------------------------------------------------------------
-- 5.           fn_csm_gpu_history_dump
--              changed the order of fields (ex. serial_number, node_name)
--              removed 'status' field
-------------------------------------------------------------------------------------------------------------------
-- 6.           fn_csm_dimm_history_dump - removed 'status' field
-------------------------------------------------------------------------------------------------------------------
-- 7.           fn_csm_allocation_node_
-------------------------------------------------------------------------------------------------------------------
--     4.3.51 - update to fn_csm_node_attributes_query_details remove available size
--     4.3.50 - major updates to fn_csm_vg_create new available size check and deal 
--              with available size no longer in ssd table 
--            - updated fn_csm_vg_delete to deal with available size no longer in ssd table
--     4.3.49 - updated fn_csm_vg_create variable sumcheck type from int to bigint
--     4.3.48 - added fn_csm_ssd_wear function (to handle only historical information related to the wear fields)
--              modified fn_csm_ssd_history_dump (to handle the function fn_csm_ssd_wear_history)
--              removed available_size field from csm_ssd + history table
--     4.3.47 - added fn_csm_vg_create parameter checks
--     4.3.46 - improved fn_csm_vg_create and fn_csm_vg_delete to work with new db schema
--     4.3.45 - improved error messages in fn_csm_allocation_node_sharing_status.
--     4.3.44 - updated fn_csm_vg_create to include update time
--     4.3.43 - updated fn_csm_vg_history_dump: added support for update_time field
--              also added in on update trigger "update_time" becomes now();
--     4.3.42 - updated fn_csm_vg_create: added support for only 1 scheduler VG per node
--     4.3.41 - updated fn_csm_vg_delete: changed math to properly update available size
--     4.3.40 - updated fn_csm_vg_create: added in new fields ssd_count, scheduler 
--     4.3.39 - updated fn_csm_vg_history_dump: added in new field scheduler
--     4.3.38 - updated fn_csm_step_begin, fn_csm_step_end, and fn_csm_step_history_dump 
--            - to address api refactor.
--     4.3.37 - updated function fn_csm_node_attributes_query_details and type node_details
--            - to address changes to ssd table
--     4.3.36 - added fields to ssd and history table
--     4.3.35 - added new features to the ib_cable and switch tables functions and triggers
--            - update/delete based on API calls (only recording specific changes see below
--            - for details)
--     4.3.34 - added new function 'fn_csm_switch_children_inventory_collection'  
--     4.3.33 - added new function 'fn_csm_switch_attributes_query_details'
--            - added new type 'switch_details'
--     4.3.32 - introduced 'change log' comment at top of file
--===============================================================================

\set ON_ERROR_STOP on
BEGIN;

---------------------------------------------------------------------------------------------------
-- csm_allocation function to finalize the data aggregator fields (fn_csm_allocation_finish_data_stats)
---------------------------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION fn_csm_allocation_finish_data_stats(
        allocationid    bigint,
        i_state         text,
        node_names      text[],
        ib_rx_list      bigint[],
        ib_tx_list      bigint[],
        gpfs_read_list  bigint[],
        gpfs_write_list bigint[],
        energy_list     bigint[],
        pc_hit_list     bigint[],
        gpu_usage_list  bigint[],
        cpu_usage_list  bigint[],
        mem_max_list    bigint[],
        gpu_energy_list  bigint[],
        out o_end_time timestamp, 
        out o_final_state text
)
RETURNS record AS $$
DECLARE 
    BIGINT_MAX CONSTANT bigint := 9223372036854775807; -- Maximum value for bigint
    current_state text; -- current state of the allocation.
BEGIN
    ALTER TABLE csm_allocation_node DISABLE TRIGGER tr_csm_allocation_node_change;

    -- UPDATE the node table, if the incoming value is greater than the original, assume the
    -- counter overflowed. In the event of an overflow subtract the difference from BIGINT_MAX
    UPDATE csm_allocation_node
        SET
            ib_tx      = (CASE WHEN(d.tx > 0 AND ib_tx >= 0) THEN 
                            CASE WHEN ( d.tx  >= ib_tx ) THEN d.tx - ib_tx
                            ELSE BIGINT_MAX - (ib_tx - d.tx) END
                          ELSE -1 * ABS(ib_tx) END ),

            ib_rx      = (CASE WHEN(d.rx > 0 AND ib_rx >= 0) THEN 
                            CASE WHEN ( d.rx  >= ib_rx ) THEN d.rx - ib_rx 
                            ELSE BIGINT_MAX - (ib_rx - d.rx) END 
                          ELSE -1 * ABS(ib_rx) END ),

            gpfs_read  = (CASE WHEN(d.g_read > 0 AND gpfs_read  >= 0) THEN 
                            CASE WHEN ( d.g_read >= gpfs_read ) THEN d.g_read  - gpfs_read
                            ELSE BIGINT_MAX - (gpfs_read - d.g_read) END
                          ELSE -1 * ABS(gpfs_read ) END ),

            gpfs_write = (CASE WHEN(d.g_write> 0 AND gpfs_write >= 0) THEN 
                            CASE WHEN (d.g_write >= gpfs_write) THEN  d.g_write - gpfs_write 
                            ELSE BIGINT_MAX - (gpfs_write - d.g_write) END
                          ELSE -1 * ABS(gpfs_write) END ),

            energy     = (CASE WHEN(d.l_energy > 0 AND energy   >= 0) THEN 
                            CASE WHEN ( d.l_energy >= energy ) THEN  d.l_energy  - energy 
                            ELSE BIGINT_MAX - (energy - d.l_energy) END
                          ELSE -1 * ABS(energy    ) END ),

            power_cap_hit = (CASE WHEN(d.pc_hit > 0 AND power_cap_hit >= 0) THEN 
                            CASE WHEN ( d.pc_hit >= power_cap_hit ) THEN  d.pc_hit  - power_cap_hit
                            ELSE BIGINT_MAX - (power_cap_hit- d.pc_hit) END
                          ELSE -1 * ABS(power_cap_hit) END ), 

            gpu_usage = d.gpu_use,
            cpu_usage = d.cpu_use,
            memory_usage_max = d.mem_max,

            gpu_energy = (CASE WHEN(d.l_gpu_energy > 0 AND gpu_energy >= 0) THEN 
                            CASE WHEN ( d.l_gpu_energy >= gpu_energy ) THEN  d.l_gpu_energy - gpu_energy
                            ELSE BIGINT_MAX - (gpu_energy- d.l_gpu_energy) END
                          ELSE -1 * ABS(gpu_energy ) END )
        FROM
            ( SELECT
                unnest(node_names) as node,
                unnest(ib_rx_list) as rx,
                unnest(ib_tx_list) as tx,
                unnest(gpfs_read_list) as g_read,
                unnest(gpfs_write_list) as g_write,
                unnest(energy_list) as l_energy,
                unnest(pc_hit_list) as pc_hit,
                unnest(gpu_usage_list) as gpu_use,
                unnest(cpu_usage_list) as cpu_use,
                unnest(mem_max_list) as mem_max,
                unnest(gpu_energy_list) as l_gpu_energy
            ) d
        WHERE allocation_id = allocationid AND node_name = d.node;

    ALTER TABLE csm_allocation_node ENABLE TRIGGER tr_csm_allocation_node_change;

    -- If this state was called directly by the API.
    IF (i_state != '' ) THEN
        -- Verify the state is not "deleting"
        SELECT state INTO current_state FROM csm_allocation WHERE allocation_id=allocationid;

        o_end_time = now();

        -- IF the current state is not deleting, finalize it.
        -- ELSE finish the delete.
        IF (current_state != 'deleting' ) THEN
            o_final_state = i_state;
            -- Update the state of the chosen allocation.
            UPDATE csm_allocation SET state = i_state WHERE allocation_id=allocationid;
        ELSE
            o_final_state = 'complete';
            PERFORM fn_csm_allocation_history_dump (allocationid, o_end_time, 0, 'complete', false,
                 node_names, ib_rx_list, ib_tx_list, gpfs_read_list, gpfs_write_list, 
                 energy_list, pc_hit_list, gpu_usage_list, cpu_usage_list, mem_max_list, gpu_energy_list);
        END IF;

        UPDATE csm_allocation_node SET state=i_state WHERE allocation_id=allocationid;
    END IF;
END;
$$ LANGUAGE 'plpgsql';

COMMENT ON FUNCTION fn_csm_allocation_finish_data_stats( allocationid bigint, i_state text, node_names text[], 
    ib_rx_list bigint[], ib_tx_list bigint[], gpfs_read_list bigint[], gpfs_write_list bigint[],
    energy_list bigint[], pc_hit_list bigint[], gpu_usage_list bigint[], cpu_usage_list bigint[],
    mem_max_list bigint[], gpu_energy_list bigint[], out o_end_time timestamp, out o_final_state text )
    is 
    'csm_allocation function to finalize the data aggregator fields.';

---------------------------------------------------------------------------------------------------
-- fn_csm_allocation_delete_start - Used to retrieve data for delete and update the state.
---------------------------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION fn_csm_allocation_delete_start(
    i_allocation_id    bigint,
    i_primary_job_id   bigint,
    i_secondary_job_id int,
    i_timeout_time     bigint,
    OUT o_allocation_id    bigint,
    OUT o_primary_job_id   bigint,
    OUT o_secondary_job_id int,
    OUT o_user_flags       text,
    OUT o_system_flags     text,
    OUT o_num_nodes        int,
    OUT o_state            text,
    OUT o_type             text,
    OUT o_isolated_cores   int,
    OUT o_user_name        text,
    OUT o_nodelist         text,
    OUT o_runtime          bigint
)
RETURNS record AS $$
DECLARE
    real_allocation_id bigint;
    time_since_change  double precision;
    allocations_found  integer;
    INVALID_STATE       CONSTANT integer := 1;
    INVALID_ALLOCATION  CONSTANT integer := 2;
BEGIN
    IF (i_primary_job_id > 0) THEN
        SELECT allocation_id INTO real_allocation_id
        FROM csm_allocation 
        WHERE primary_job_id = i_primary_job_id AND secondary_job_id = i_secondary_job_id;
    ELSE
        real_allocation_id := i_allocation_id;
    END IF;

    -- Lock the row.
    PERFORM  state FROM csm_allocation WHERE allocation_id=real_allocation_id FOR UPDATE;

    -- Get the data for return.
    SELECT 
       a.allocation_id, a.primary_job_id, a.secondary_job_id, 
       a.user_flags, a.system_flags, a.num_nodes, 
       a.state, a.type, a.isolated_cores, a.user_name, 
       array_to_string(array_agg(an.node_name),',') as a_nodelist,
       (extract(EPOCH from  now() - begin_time))::bigint
    INTO 
        o_allocation_id, o_primary_job_id, o_secondary_job_id,
        o_user_flags, o_system_flags, o_num_nodes, 
        o_state, o_type, o_isolated_cores, o_user_name,
        o_nodelist, o_runtime
    FROM csm_allocation a 
    LEFT JOIN 
        csm_allocation_node an 
        ON a.allocation_id = an.allocation_id 
    WHERE a.allocation_id=real_allocation_id
    GROUP BY a.allocation_id
    LIMIT 1;

    -- Test the time difference for special 
    SELECT 
        EXTRACT (EPOCH FROM now() - history_time )
    INTO
        time_since_change
    FROM csm_allocation_state_history 
    WHERE allocation_id=real_allocation_id 
    ORDER BY history_time DESC 
    LIMIT 1;
    
    -- Verify the allocation was present.
    IF ( o_allocation_id > 0 ) THEN
        -- Test the state, raising an exception is not valid.
        IF ( (o_state = 'deleting-mcast' OR o_state = 'to-staging-out' OR o_state = 'to-running') 
            AND time_since_change < i_timeout_time ) THEN
            RAISE EXCEPTION 'Detected a multicast operation in progress for allocation, rejecting delete.'
                USING HINT = INVALID_STATE;
        END IF;

        -- TODO should this use an in ANY instead?
        IF ( o_state = 'to-staging-out' OR o_state = 'to-running' OR o_state = 'running' OR
            o_state = 'running-failed' OR o_state = 'staging-out-failed' OR o_state = 'deleting-mcast')
        THEN
            UPDATE csm_allocation SET state = 'deleting-mcast' WHERE allocation_id=real_allocation_id;
        ELSE
            UPDATE csm_allocation SET state = 'deleting' WHERE allocation_id=real_allocation_id;
        END IF;
    ELSE
        RAISE EXCEPTION 'Allocation unable to be found matching the supplied criteria, unable to delete.'
            USING HINT = INVALID_ALLOCATION;
    END IF;

END;
$$ LANGUAGE 'plpgsql';

COMMENT ON FUNCTION fn_csm_allocation_delete_start(
    i_allocation_id    bigint, i_primary_job_id   bigint, i_secondary_job_id int, 
    i_timeout_time     bigint,
    OUT o_allocation_id    bigint, OUT o_primary_job_id   bigint, OUT o_secondary_job_id int,
    OUT o_user_flags       text,   OUT o_system_flags     text,   OUT o_num_nodes        int,
    OUT o_state            text,   OUT o_type             text,   OUT o_isolated_cores   int,
    OUT o_user_name        text,   OUT o_nodelist         text,   OUT o_runtime          bigint ) is 
        'Retrieves allocation details for delete a d sets the state to deleteing.';

---------------------------------------------------------------------------------------------------
-- csm_allocation Function to amend summarized column(s) on DELETE. (csm_allocation_history_dump)
---------------------------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION fn_csm_allocation_history_dump(
        allocationid    bigint,
        endtime         timestamp, 
        exitstatus      int,
        i_state         text,
        finalize        boolean,
        node_names      text[],
        ib_rx_list      bigint[],
        ib_tx_list      bigint[],
        gpfs_read_list  bigint[],
        gpfs_write_list bigint[],
        energy_list     bigint[],
        pc_hit_list     bigint[],
        gpu_usage_list  bigint[],
        cpu_usage_list  bigint[],
        mem_max_list    bigint[],
        gpu_energy_list  bigint[],
        OUT o_end_time  timestamp
)

RETURNS timestamp AS $$

DECLARE 
   a "csm_allocation"%ROWTYPE;
   s "csm_step"%ROWTYPE;
   INVALID_STATE       CONSTANT integer := 1;
   INVALID_ALLOCATION  CONSTANT integer := 2;

BEGIN
    o_end_time = endtime;
    -- first, make sure no allocation steps are mistakenly still around
    IF EXISTS (SELECT allocation_id FROM csm_step_node WHERE allocation_id=$1) THEN
        -- csm_step_node DELETE trigger will handle moving to csm_step_node_history
        DELETE FROM csm_step_node WHERE allocation_id=$1;
    END IF;

    IF EXISTS (SELECT allocation_id FROM csm_step WHERE allocation_id=$1) THEN
        FOR s IN SELECT * FROM csm_step WHERE allocation_id=$1
        LOOP
            INSERT INTO csm_step_history VALUES(
                now(),
                s.step_id,
                allocationid,
                s.begin_time,
                endtime,
                s.status,
                s.executable,
                s.working_directory,
                s.argument,
                s.environment_variable,
                s.num_nodes,
                s.num_processors,
                s.num_gpus,
                s.projected_memory,
                s.num_tasks,
                s.user_flags,
                0,      -- exit_status
                'Removed by Allocation.', -- err_text
                'NA',     -- cpu_stats
                0.0,    -- total_u_time
                0.0,    -- total_s_time
                'NA',     -- total_num_threads
                'NA',     -- gpu_stats
                'NA',     -- memory_stats
                0,      -- max_memory
                'NA'      -- io_stats
                        -- archive_history_time not added
            );
            DELETE FROM csm_step WHERE step_id=s.step_id AND allocation_id=allocationid;
        END LOOP;
    END IF;

    -- first, make sure no lvs are mistakenly still around before the allocation delete
        PERFORM fn_csm_allocation_dead_records_on_lv(allocationid);
    
    -- now we can proceed with the csm_allocation processing
    IF EXISTS (SELECT allocation_id FROM csm_allocation WHERE allocation_id=allocationid) THEN
        -- Save the data aggregator stats.

        IF (finalize)  THEN
            -- Disable the trigger temporarily.
            PERFORM fn_csm_allocation_finish_data_stats( allocationid, '', node_names, ib_rx_list, 
                ib_tx_list, gpfs_read_list,gpfs_write_list, energy_list,
                pc_hit_list, gpu_usage_list, cpu_usage_list, mem_max_list, gpu_energy_list);
        END IF;

        -- Update the state value.
        UPDATE csm_allocation_node SET state=i_state  WHERE allocation_id=allocationid;
        DELETE FROM csm_allocation_node WHERE allocation_id=allocationid;
        
        SELECT * INTO a FROM csm_allocation WHERE allocation_id=allocationid;
        INSERT INTO csm_allocation_history VALUES(
            now(),
            allocationid,
            a.primary_job_id,
            a.secondary_job_id,
            a.ssd_file_system_name,
            a.launch_node_name,
            a.isolated_cores,
            a.user_flags,
            a.system_flags,
            a.ssd_min,
            a.ssd_max,
            a.num_nodes,
            a.num_processors,
            a.num_gpus,
            a.projected_memory,
            i_state,
            a.type,
            a.job_type,
            a.user_name,
            a.user_id,
            a.user_group_id,
            a.user_group_name,
            a.user_script,
            a.begin_time,
            endtime,  
            exitstatus,
            a.account,
            a.comment,
            a.job_name,
            a.job_submit_time,
            a.queue,
            a.requeue,
            a.time_limit,
            a.wc_key,
            NULL,
            a.smt_mode
        );
        DELETE FROM csm_allocation WHERE allocation_id=allocationid;

    ELSE
        RAISE EXCEPTION 'allocation_id does not exist.'
            USING HINT = INVALID_ALLOCATION;
    END IF;
    --EXCEPTION
    --    WHEN others THEN
    --        RAISE EXCEPTION
    --        USING ERRCODE = sqlstate,
    --                 MESSAGE = 'error_handling_test: ' || sqlstate || '/' || sqlerrm;
--    RETURN NULL;

END;
$$ LANGUAGE 'plpgsql';

-----------------------------------------------------------
-- csm_allocation_history_function_comments
-----------------------------------------------------------
COMMENT ON FUNCTION fn_csm_allocation_history_dump( allocationid bigint, endtime timestamp, 
    exitstatus int, i_state text, finalize boolean, node_names text[], 
    ib_rx_list bigint[], ib_tx_list bigint[], gpfs_read_list bigint[], gpfs_write_list bigint[],
    energy_list bigint[], pc_hit_list bigint[], gpu_usage_list bigint[], cpu_usage_list bigint[], 
    mem_max_list bigint[], gpu_energy_list bigint[], out o_end_time timestamp)
    is 
    'csm_allocation function to amend summarized column(s) on DELETE. (csm_allocation_history_dump)';

---------------------------------------------------------------------------------------------------
-- fn_csm_allocation_revert reverts a failed allocation.
---------------------------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION fn_csm_allocation_revert(
        allocationid    bigint)
RETURNS void AS $$
BEGIN
    DELETE FROM csm_allocation WHERE allocation_id=allocationid;
    DELETE FROM csm_allocation_state_history WHERE allocation_id=allocationid;
END
$$ LANGUAGE 'plpgsql';

COMMENT ON FUNCTION fn_csm_allocation_revert(allocationid bigint) is 'Removes all traces of an allocation that never multicasted.';
---------------------------------------------------------------------------------------------------
-- csm_allocation_update function to amend summarized column(s) on UPDATE.
---------------------------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION fn_csm_allocation_update()
    RETURNS trigger AS
$$
BEGIN
    IF (TG_OP = 'UPDATE') THEN
        INSERT INTO csm_allocation_history(
            history_time,
            allocation_id,
            primary_job_id,
            secondary_job_id,
            ssd_file_system_name,
            launch_node_name,
            isolated_cores,
            user_flags,
            system_flags,
            ssd_min,
            ssd_max,
            num_nodes,
            num_processors,
            num_gpus,
            projected_memory,
            state,
            type,
            job_type,
            user_name,
            user_id,
            user_group_id,
            user_group_name,
            user_script,
            begin_time,
            account,
            comment,
            job_name,
            job_submit_time,
            queue,
            requeue,
            time_limit,
            wc_key,
            smt_mode)
        VALUES
            (now(),
            NEW.allocation_id,
            NEW.primary_job_id,
            NEW.secondary_job_id,
            NEW.ssd_file_system_name,
            NEW.launch_node_name,
            NEW.isolated_cores,
            NEW.user_flags,
            NEW.system_flags,
            NEW.ssd_min,
            NEW.ssd_max,
            NEW.num_nodes,
            NEW.num_processors,
            NEW.num_gpus,
            NEW.projected_memory,
            NEW.state,
            NEW.type,
            NEW.job_type,
            NEW.user_name,
            NEW.user_id,
            NEW.user_group_id,
            NEW.user_group_name,
            NEW.user_script,
            NEW.begin_time,
            NEW.account,
            NEW.comment,
            NEW.job_name,
            NEW.job_submit_time,
            NEW.queue,
            NEW.requeue,
            NEW.time_limit,
            NEW.wc_key, 
            NEW.smt_mode);
        RETURN NEW;
    END IF;
RETURN NULL;
END;
$$
LANGUAGE 'plpgsql';

---------------------------------------------------------------------------------------------------
-- csm_allocation trigger to amend summarized column(s) on UPDATE.
---------------------------------------------------------------------------------------------------

CREATE TRIGGER tr_csm_allocation_update
    BEFORE UPDATE OF allocation_id,primary_job_id,secondary_job_id,ssd_file_system_name,launch_node_name,isolated_cores,user_flags,system_flags,ssd_min,ssd_max,num_nodes,num_processors,num_gpus,projected_memory,type,job_type,user_name,user_id,user_group_id,user_group_name,user_script,begin_time,account,comment,job_name,job_submit_time,queue,requeue,time_limit,wc_key
    ON csm_allocation
    FOR EACH ROW
    EXECUTE PROCEDURE fn_csm_allocation_update()
;
-----------------------------------------------------------
-- csm_allocation_update_trigger_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_allocation_update() is 'csm_allocation_update function to amend summarized column(s) on UPDATE.';
COMMENT ON TRIGGER tr_csm_allocation_update ON csm_allocation is 'csm_allocation_update trigger to amend summarized column(s) on UPDATE.';

---------------------------------------------------------------------------------------------------
-- csm_allocation_state_history function to amend summarized column(s) on UPDATE.
---------------------------------------------------------------------------------------------------

CREATE FUNCTION fn_csm_allocation_state_history_state_change()
    RETURNS trigger AS
$$
BEGIN
    IF (TG_OP = 'UPDATE') THEN
        INSERT INTO csm_allocation_state_history(
            history_time,
            allocation_id,
            state)
        VALUES(
            now(),
            NEW.allocation_id,
            NEW.state);
    ELSIF (TG_OP = 'INSERT') THEN
        INSERT INTO csm_allocation_state_history(
            history_time,
            allocation_id,
            state)
        VALUES(
            now(),
            NEW.allocation_id,
            NEW.state);
    END IF;
RETURN NEW;
END;
$$
LANGUAGE 'plpgsql';

---------------------------------------------------------------------------------------------------
-- csm_allocation_state_change trigger to amend summarized column(s) on UPDATE.
---------------------------------------------------------------------------------------------------

CREATE TRIGGER tr_csm_allocation_state_change
    BEFORE INSERT OR UPDATE OF state
    ON csm_allocation
    FOR EACH ROW
    EXECUTE PROCEDURE fn_csm_allocation_state_history_state_change()
;

-----------------------------------------------------------------------------------------------
-- csm_allocation function to handle updating the state of an allocation
-----------------------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION fn_csm_allocation_update_state(
    IN i_allocationid       bigint,
    IN i_state              text,
    OUT o_primary_job_id    bigint, 
    OUT o_secondary_job_id  integer,
    OUT o_user_flags        text, 
    OUT o_system_flags      text, 
    OUT o_num_nodes         integer, 
    OUT o_nodes             text, 
    OUT o_isolated_cores    integer,
    OUT o_user_name         text,
    OUT o_shared            boolean,
    OUT o_num_gpus          integer,
    OUT o_num_processors    integer,
    OUT o_projected_memory  integer,
    OUT o_state             text,
    OUT o_runtime           bigint,
    OUT o_smt_mode          smallint
)
RETURNS record AS $$
DECLARE
    nodes    text[];
    scratch  text;
BEGIN
    --LOCK TABLE csm_allocation_node IN EXCLUSIVE MODE;
    PERFORM 1 FROM csm_allocation_node WHERE allocation_id=i_allocationid FOR UPDATE;
    
    --- Get the old state and shared id.
    SELECT 
        state, isolated_cores,
        primary_job_id, secondary_job_id, user_flags,
        system_flags, num_nodes, user_name,
        num_gpus, num_processors, projected_memory, (extract(EPOCH from  now() - begin_time))::bigint,
        smt_mode
    INTO 
        o_state, o_isolated_cores,
        o_primary_job_id, o_secondary_job_id, o_user_flags,
        o_system_flags, o_num_nodes, o_user_name,
        o_shared, o_num_gpus, o_num_processors,
        o_projected_memory, o_runtime, o_smt_mode
    FROM csm_allocation a
    WHERE allocation_id = i_allocationid;

    --- If the state output is null the allocation could not be found and an exception should be raised.
    IF ( o_state IS NULL )
    THEN
        RAISE EXCEPTION using message = 
            'Allocation ' || i_allocationid || 
                ' was not in a valid state, verify the allocation exists and is active';
    END IF;

    --- Get the nodes from the csm_allocation_node table.
    SELECT 
        shared, array_agg(node_name)
    INTO
        o_shared, nodes
    FROM csm_allocation_node
    WHERE allocation_id = i_allocationid
    GROUP BY shared;

    -- TODO Should this only check on to-running?
    -- Verify all of the nodes are still in service.
    -- If any are in a bad state raise an exception.
    IF (  EXISTS ( SELECT state FROM csm_node WHERE node_name = ANY(nodes) AND state != 'IN_SERVICE') )
    THEN
        scratch := array_to_string( ARRAY(
            SELECT node_name
            FROM csm_node
            WHERE node_name = ANY(nodes) AND (state != 'IN_SERVICE') ), ',' );
        
        RAISE EXCEPTION using message = 'Some of the nodes in the allocation are no longer in service: ' ||
            scratch;
    END IF;
    
    --- If this is a transition to the running state from staging, verify that the allocation can be created.
    --- Else Verify that the state transition is the one other legal state
    IF ( o_state='staging-in' AND i_state='to-running' ) 
    THEN       
        --- If the allocation is not shared, verigy there is nothing on the nodes.
        --- Else verify no exclusive jobs are running on the nodes.
        IF( NOT( o_shared )) 
        THEN
            IF EXISTS (
                SELECT state
                    FROM csm_allocation_node
                    WHERE node_name = ANY(nodes) AND state!='staging-in' AND state!='staging-out' )
            THEN
                RAISE EXCEPTION using message = 
                    'Exclusive Request: A node was found to be busy, Allocation ' || i_allocationid || 
                    ' state could not be transitioned.';
            END IF;
        ELSIF EXISTS( 
            SELECT state 
                FROM csm_allocation_node 
                WHERE node_name = ANY(nodes) AND state!='staging-in' AND state!='staging-out' AND NOT shared ) 
        THEN
            RAISE EXCEPTION using message =
                'Shared Allocation ' || i_allocationid || 'could not be transitioned to running.';
        END IF;
    ELSIF  (o_state='staging-in' AND i_state='to-staging-out' )
    THEN
        i_state:='staging-out';
    ELSIF ( NOT ( o_state = 'running' AND i_state = 'to-staging-out' ) )  
    THEN
        RAISE EXCEPTION using message =
            'Illegal state transition detected: "' || o_state || '" TO "' || i_state || '" Illegal.';
    END IF;

    -- Transition to the new state.
    UPDATE csm_allocation 
       SET state = i_state
       WHERE allocation_id = i_allocationid;

    --- TODO Why is the trigger not triggering?
    UPDATE csm_allocation_node
       SET state = i_state
       WHERE allocation_id = i_allocationid;

    -- Convert the working array to a string.
    o_nodes := array_to_string(nodes,',');

    EXCEPTION 
        WHEN others THEN
            RAISE EXCEPTION
            USING ERRCODE = sqlstate,
                MESSAGE = 'error_handling_test: ' || sqlstate || '/' || sqlerrm;
END
$$ LANGUAGE 'plpgsql';



-----------------------------------------------------------
-- csm_allocation_state_change_trigger_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_allocation_state_history_state_change() is 'csm_allocation_state_change function to amend summarized column(s) on UPDATE.';
COMMENT ON TRIGGER tr_csm_allocation_state_change ON csm_allocation is 'csm_allocation trigger to amend summarized column(s) on UPDATE.';
COMMENT ON FUNCTION fn_csm_allocation_update_state(IN i_allocationid bigint, IN i_state text, OUT o_primary_job_id bigint, OUT o_secondary_job_id integer, OUT o_user_flags text, OUT o_system_flags text, OUT o_num_nodes integer, OUT o_nodes text, OUT o_isolated_cores integer, OUT o_user_name text, OUT o_runtime bigint, OUT o_smt_mode smallint) is 'csm_allocation_update_state function that ensures the allocation can be legally updated to the supplied state'; --TODO

-----------------------------------------------------------
-- fn_csm_allocation_dead_records_on_lv 
-----------------------------------------------------------
CREATE FUNCTION fn_csm_allocation_dead_records_on_lv(
    i_allocation_id bigint
)
RETURNS void AS $$
DECLARE
    nn_count int;
    lv_on_an int;
    allocationids bigint[];
    matching_node_names text[];
    t_lv_name text;
    t_node_name text;
    t_allocation_id bigint;
BEGIN

    SELECT
        count(node_name)
    INTO nn_count
    FROM
        csm_allocation_node
    WHERE (
        allocation_id = i_allocation_id
    );

    IF ( nn_count > 0) THEN
        -- look further
        SELECT
            array_agg(allocation_id), array_agg(node_name)
        INTO
            allocationids, matching_node_names
        FROM
            csm_allocation_node
        WHERE (
            allocation_id = i_allocation_id
        );
        --loop through that array
        -- find any lvs on those allocation_nodes
        FOR i IN 1..nn_count LOOP

            SELECT
                count(logical_volume_name)
            INTO lv_on_an
            FROM
                csm_lv
            WHERE (
                allocation_id = allocationids[i] AND
                node_name = matching_node_names[i]
            );

            IF (lv_on_an > 0) THEN

                FOR j IN 1..lv_on_an LOOP
                    SELECT
                        logical_volume_name, node_name, allocation_id
                    INTO
                        t_lv_name, t_node_name, t_allocation_id
                    FROM
                        csm_lv
                    WHERE (
                        allocation_id = allocationids[i] AND
                        node_name = matching_node_names[i]
                    );

                    PERFORM fn_csm_lv_history_dump(t_lv_name, t_node_name, t_allocation_id, 'now()', 'now()', null, null, null, null);
                END LOOP;
            END IF;
        END LOOP;
    END IF;
END;
$$ LANGUAGE 'plpgsql';

-----------------------------------------------------------
-- fn_csm_allocation_dead_records_on_lv comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_allocation_dead_records_on_lv( i_allocation_id bigint) is 'Delete any lvs on an allocation that is being deleted.';

---------------------------------------------------------------------------------------------------
-- csm_node function to amend summarized column(s) on UPDATE and DELETE.
---------------------------------------------------------------------------------------------------
-- This trigger covers three coditions
-- 1. Checks to see if any colums have changed besides the state field (Records into the csm_node_history table)
-- 2. Checks to see if any colums have changed including the state field (This records the change in the csm_node_state_history and csm_node_history table)
-- 3. Checks to see if just the state field has changed and is recorded in the csm_node_state_history table
---------------------------------------------------------------------------------------------------
-- Normal Default
--      1.) Update/delete a record
--          Do: push into history
-- Exceptions:
--      1.) Entering a record which is the same (no changes)
--          Do: Update collection time of active record
--      2.) API Update fields are the same
--          Do: Nothing
---------------------------------------------------------------------------------------------------

CREATE FUNCTION fn_csm_node_update()
    RETURNS trigger AS
$$
BEGIN
    IF (TG_OP = 'DELETE') THEN
        INSERT INTO csm_node_history(
            history_time,
            node_name,
            machine_model,
            serial_number,
            collection_time,
            update_time,
            state,
            type,
            primary_agg,
            secondary_agg,
            hard_power_cap,
            installed_memory,
            installed_swap,
            discovered_sockets,
            discovered_cores,
            discovered_gpus,
            discovered_hcas,
            discovered_dimms,
            discovered_ssds,
            os_image_name,
            os_image_uuid,
            kernel_release,
            kernel_version,
            physical_frame_location,
            physical_u_location,
            feature_1,
            feature_2,
            feature_3,
            feature_4,
            comment,
            operation)
        VALUES(
            now(),
            OLD.node_name,
            OLD.machine_model,
            OLD.serial_number,
            OLD.collection_time,
            OLD.update_time,
            OLD.state,
            OLD.type,
            OLD.primary_agg,
            OLD.secondary_agg,
            OLD.hard_power_cap,
            OLD.installed_memory,
            OLD.installed_swap,
            OLD.discovered_sockets,
            OLD.discovered_cores,
            OLD.discovered_gpus,
            OLD.discovered_hcas,
            OLD.discovered_dimms,
            OLD.discovered_ssds,
            OLD.os_image_name,
            OLD.os_image_uuid,
            OLD.kernel_release,
            OLD.kernel_version,
            OLD.physical_frame_location,
            OLD.physical_u_location,
            OLD.feature_1,
            OLD.feature_2,
            OLD.feature_3,
            OLD.feature_4,
            OLD.comment,
            'D');
        RETURN OLD;
    ELSEIF (TG_OP = 'UPDATE') THEN
        NEW.update_time = now();
        IF  (
            OLD.node_name IS DISTINCT FROM NEW.node_name OR
            OLD.machine_model IS DISTINCT FROM NEW.machine_model OR
            OLD.serial_number IS DISTINCT FROM NEW.serial_number OR
            OLD.type IS DISTINCT FROM NEW.type OR
            OLD.primary_agg IS DISTINCT FROM NEW.primary_agg OR
            OLD.secondary_agg  IS DISTINCT FROM NEW.secondary_agg OR
            OLD.hard_power_cap IS DISTINCT FROM NEW.hard_power_cap OR
            OLD.installed_memory IS DISTINCT FROM NEW.installed_memory OR
            OLD.installed_swap IS DISTINCT FROM NEW.installed_swap OR
            OLD.discovered_sockets IS DISTINCT FROM NEW.discovered_sockets OR
            OLD.discovered_cores IS DISTINCT FROM NEW.discovered_cores OR
            OLD.discovered_gpus IS DISTINCT FROM NEW.discovered_gpus OR
            OLD.discovered_hcas IS DISTINCT FROM NEW.discovered_hcas OR
            OLD.discovered_dimms IS DISTINCT FROM NEW.discovered_dimms OR
            OLD.discovered_ssds IS DISTINCT FROM NEW.discovered_ssds OR
            OLD.os_image_name IS DISTINCT FROM NEW.os_image_name OR
            OLD.os_image_uuid IS DISTINCT FROM NEW.os_image_uuid OR
            OLD.kernel_release IS DISTINCT FROM NEW.kernel_release OR
            OLD.kernel_version IS DISTINCT FROM NEW.kernel_version OR
            OLD.physical_frame_location IS DISTINCT FROM NEW.physical_frame_location OR
            OLD.physical_u_location IS DISTINCT FROM NEW.physical_u_location OR
            OLD.feature_1 IS DISTINCT FROM NEW.feature_1 OR
            OLD.feature_2 IS DISTINCT FROM NEW.feature_2 OR
            OLD.feature_3 IS DISTINCT FROM NEW.feature_3 OR
            OLD.feature_4 IS DISTINCT FROM NEW.feature_4 OR
            OLD.comment IS DISTINCT FROM NEW.comment) THEN
        INSERT INTO csm_node_history(
            history_time,
            node_name,
            machine_model,
            serial_number,
            collection_time,
            update_time,
            state,
            type,
            primary_agg,
            secondary_agg,
            hard_power_cap,
            installed_memory,
            installed_swap,
            discovered_sockets,
            discovered_cores,
            discovered_gpus,
            discovered_hcas,
            discovered_dimms,
            discovered_ssds,
            os_image_name,
            os_image_uuid,
            kernel_release,
            kernel_version,
            physical_frame_location,
            physical_u_location,
            feature_1,
            feature_2,
            feature_3,
            feature_4,
            comment,
            operation)
        VALUES
            (now(),
            NEW.node_name,
            NEW.machine_model,
            NEW.serial_number,
            NEW.collection_time,
            NEW.update_time,
            NEW.state,
            NEW.type,
            NEW.primary_agg,
            NEW.secondary_agg,
            NEW.hard_power_cap,
            NEW.installed_memory,
            NEW.installed_swap,
            NEW.discovered_sockets,
            NEW.discovered_cores,
            NEW.discovered_gpus,
            NEW.discovered_hcas,
            NEW.discovered_dimms,
            NEW.discovered_ssds,
            NEW.os_image_name,
            NEW.os_image_uuid,
            NEW.kernel_release,
            NEW.kernel_version,
            NEW.physical_frame_location,
            NEW.physical_u_location,
            NEW.feature_1,
            NEW.feature_2,
            NEW.feature_3,
            NEW.feature_4,
            NEW.comment,
            'U');
    END IF;
    RETURN NEW;
    ELSEIF (TG_OP = 'INSERT') THEN
     NEW.update_time = now();
        INSERT INTO csm_node_history(
            history_time,
            node_name,
            machine_model,
            serial_number,
            collection_time,
            update_time,
            state,
            type,
            primary_agg,
            secondary_agg,
            hard_power_cap,
            installed_memory,
            installed_swap,
            discovered_sockets,
            discovered_cores,
            discovered_gpus,
            discovered_hcas,
            discovered_dimms,
            discovered_ssds,
            os_image_name,
            os_image_uuid,
            kernel_release,
            kernel_version,
            physical_frame_location,
            physical_u_location,
            feature_1,
            feature_2,
            feature_3,
            feature_4,
            comment,
            operation)
        VALUES
            (now(),
            NEW.node_name,
            NEW.machine_model,
            NEW.serial_number,
            NEW.collection_time,
            NEW.update_time,
            NEW.state,
            NEW.type,
            NEW.primary_agg,
            NEW.secondary_agg,
            NEW.hard_power_cap,
            NEW.installed_memory,
            NEW.installed_swap,
            NEW.discovered_sockets,
            NEW.discovered_cores,
            NEW.discovered_gpus,
            NEW.discovered_hcas,
            NEW.discovered_dimms,
            NEW.discovered_ssds,
            NEW.os_image_name,
            NEW.os_image_uuid,
            NEW.kernel_release,
            NEW.kernel_version,
            NEW.physical_frame_location,
            NEW.physical_u_location,
            NEW.feature_1,
            NEW.feature_2,
            NEW.feature_3,
            NEW.feature_4,
            NEW.comment,
            'I');
    RETURN NEW;
    END IF;
END;
$$
LANGUAGE 'plpgsql';

---------------------------------------------------------------------------------------------------
-- csm_node trigger to amend summarized column(s) on UPDATE and DELETE.
---------------------------------------------------------------------------------------------------

CREATE TRIGGER tr_csm_node_update
    BEFORE INSERT OR DELETE OR UPDATE OF node_name,machine_model,serial_number,collection_time,type,primary_agg,secondary_agg,hard_power_cap,installed_memory,installed_swap,discovered_sockets,discovered_cores,discovered_gpus,discovered_hcas,discovered_dimms,discovered_ssds,os_image_name,os_image_uuid,kernel_release,kernel_version,physical_frame_location,physical_u_location,feature_1,feature_2,feature_3,feature_4,comment
    ON csm_node
    FOR EACH ROW
    EXECUTE PROCEDURE fn_csm_node_update()
;

-----------------------------------------------------------
-- csm_node_trigger_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_node_update() is 'csm_node_update function to amend summarized column(s) on UPDATE and DELETE.';
COMMENT ON TRIGGER tr_csm_node_update ON csm_node is 'csm_node_update trigger to amend summarized column(s) on UPDATE and DELETE.';

--------------------------------------------------------------------------------------------------
-- csm_node_delete:  Function to delete a node, and remove records in the
-- csm_node, csm_ssd, csm_processor, csm_gpu, csm_hca, csm_dimm tables
--------------------------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION fn_csm_node_delete(
    IN i_node_names text[], 
    OUT o_not_deleted_node_names_count int, 
    OUT o_not_deleted_node_names text
)
RETURNS record AS $$
BEGIN
    --Change the states to remove
--    IF EXISTS (SELECT node_name FROM csm_node WHERE node_name= ANY ($1)) THEN
--            UPDATE csm_node
--                SET
--                state = 'REMOVED'
--            WHERE node_name = ANY ($1);
--    END IF;
--    IF EXISTS (SELECT node_name FROM csm_ssd WHERE node_name= ANY ($1)) THEN
--        UPDATE csm_ssd
--            SET
--            status = 'R'
--        WHERE node_name = ANY ($1);
--    END IF;
--    IF EXISTS (SELECT node_name FROM csm_processor WHERE node_name= ANY ($1)) THEN
--        UPDATE csm_processor
--            SET
--            status = 'R'
--        WHERE node_name = ANY ($1);
--    END IF;
--    IF EXISTS (SELECT node_name FROM csm_gpu WHERE node_name= ANY ($1)) THEN
--        UPDATE csm_gpu
--            SET
--            status = 'R'
--        WHERE node_name = ANY ($1);
--    END IF;
--    IF EXISTS (SELECT node_name FROM csm_dimm WHERE node_name= ANY ($1)) THEN
--        UPDATE csm_dimm
--            SET
--            status = 'R'
--        WHERE node_name = ANY ($1);
--    END IF;

-- Removes node names from csm_node, csm_processor_socket, csm_gpu, csm_hca, csm_dimm tables and returns node_names 
    -- not deleted in output variable o_deleted_node_names
    WITH deleted AS (
            DELETE
            FROM csm_ssd
            WHERE ( node_name = ANY (i_node_names) )
            RETURNING node_name
        ), deleted2 AS (
            DELETE
            FROM csm_processor_socket
            WHERE ( node_name = ANY (i_node_names) )
            RETURNING node_name
        ), deleted3 AS (
            DELETE
            FROM csm_gpu
            WHERE ( node_name = ANY (i_node_names) )
            RETURNING node_name
        ), deleted4 AS (
            DELETE
            FROM csm_hca
            WHERE ( node_name = ANY (i_node_names) )
            RETURNING node_name
        ), deleted5 AS (
            DELETE
            FROM csm_dimm
            WHERE ( node_name = ANY (i_node_names) )
            RETURNING node_name
        ),    not_deleted AS (
            SELECT name
            FROM unnest (i_node_names) as input(name)
            LEFT JOIN deleted ON (deleted.node_name = input.name)
            LEFT JOIN deleted2 ON (deleted2.node_name = input.name)
            LEFT JOIN deleted3 ON (deleted3.node_name = input.name)
            LEFT JOIN deleted4 ON (deleted4.node_name = input.name)
            LEFT JOIN deleted5 ON (deleted5.node_name = input.name)
            WHERE deleted.node_name IS NULL
        )
    SELECT * INTO o_not_deleted_node_names FROM not_deleted;

    -- Removes node_name from csm_node table
    WITH new_delete AS (
        DELETE 
            FROM csm_node 
            WHERE (node_name =  ANY (i_node_names) ) 
            RETURNING * 
    ), new_not_delete AS (
        SELECT name 
        FROM unnest (i_node_names) as input(name)
        LEFT JOIN new_delete ON (new_delete.node_name = input.name)
        WHERE new_delete.node_name IS NULL
    )
    SELECT COUNT(name), string_agg(name,',') 
    INTO o_not_deleted_node_names_count, o_not_deleted_node_names 
    FROM new_not_delete;
    --- TODO add in the ssd debug info like above for node

    EXCEPTION
    WHEN others THEN
        RAISE EXCEPTION
        USING ERRCODE = sqlstate,
            MESSAGE = 'error_handling_test: ' || sqlstate || '/' || sqlerrm;

    --- TODO On Conflict.
END;
$$ LANGUAGE 'plpgsql';

-----------------------------------------------------------
-- csm_node_delete_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_node_delete(i_node_names text[]) is 'Function to delete a node, and remove records in the csm_node, csm_ssd, csm_processor, csm_gpu, csm_hca, csm_dimm tables';

---------------------------------------------------------------------------------------------------
-- csm_node_state function to amend summarized column(s) on UPDATE
---------------------------------------------------------------------------------------------------

CREATE FUNCTION fn_csm_node_state()
    RETURNS trigger AS
$$
BEGIN
    IF (TG_OP = 'UPDATE') THEN
    NEW.update_time = now();
         INSERT INTO csm_node_state_history(
            history_time,
            node_name,
            state,
            operation)
        VALUES(
            now(),
            NEW.node_name,
            NEW.state,
            'U');
    RETURN NEW;
    ELSEIF (TG_OP = 'INSERT') THEN
    NEW.update_time = now();
         INSERT INTO csm_node_state_history(
            history_time,
            node_name,
            state,
            operation)
        VALUES(
            now(),
            NEW.node_name,
            NEW.state,
            'I');
    RETURN NEW;
    END IF;
END;
$$
LANGUAGE 'plpgsql';

---------------------------------------------------------------------------------------------------
-- csm_node_state trigger to amend summarized column(s) on UPDATE and DELETE.
---------------------------------------------------------------------------------------------------

CREATE TRIGGER tr_csm_node_state
    BEFORE INSERT OR UPDATE OF state
    ON csm_node
    FOR EACH ROW
    EXECUTE PROCEDURE fn_csm_node_state()
;

-----------------------------------------------------------
-- csm_node_state_trigger_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_node_state() is 'csm_node function to amend summarized column(s) on UPDATE.';
COMMENT ON TRIGGER tr_csm_node_state ON csm_node is 'csm_node_state trigger to amend summarized column(s) on UPDATE.';

---------------------------------------------------------------------------------------------------
-- node_details type to HELP fn_csm_node_attributes_query_details
---------------------------------------------------------------------------------------------------

CREATE TYPE node_details as (
node_name                         text,
node_collection_time              timestamp,
node_update_time                  timestamp,
node_discovered_cores             int,
node_discovered_dimms             int,
node_discovered_gpus              int,
node_discovered_hcas              int,
node_discovered_sockets           int,
node_discovered_ssds              int,
node_comment                      text,
node_feature_1                    text,
node_feature_2                    text,
node_feature_3                    text,
node_feature_4                    text,
node_hard_power_cap               int,
node_installed_memory             bigint,
node_installed_swap               bigint,
node_kernel_release               text,
node_kernel_version               text,
node_machine_model                text,
node_os_image_name                text,
node_os_image_uuid                text,
node_physical_frame_location      text,
node_physical_u_location          text,
node_primary_agg                  text,
node_secondary_agg                text,
node_serial_number                text,
node_state                        text,
node_type                         text,
dimm_count                        int,
dimm_serial_number                text[],
dimm_physical_location            text[],
dimm_size                         int[],
gpu_count                         int,
gpu_gpu_id                        int[],
gpu_device_name                   text[],
gpu_hbm_memory                    bigint[],
gpu_inforom_image_version         text[],
gpu_pci_bus_id                    text[],
gpu_serial_number                 text[],
gpu_uuid                          text[],
gpu_vbios                         text[],
hca_count                         int,
hca_serial_number                 text[],
hca_board_id                      text[],
hca_device_name                   text[],
hca_fw_ver                        text[],
hca_guid                          text[],
hca_hw_rev                        text[],
hca_part_number                   text[],
hca_pci_bus_id                    text[],
processor_count                   int,
processor_serial_number           text[],
processor_discovered_cores        int[],
processor_physical_location       text[],
ssd_count                         int,
ssd_serial_number                 text[],
ssd_device_name                   text[],
ssd_fw_ver                        text[],
ssd_pci_bus_id                    text[],
ssd_size                          bigint[],
ssd_update_time                   timestamp[],
ssd_wear_lifespan_used            double precision[],
ssd_wear_percent_spares_remaining double precision[],
ssd_wear_total_bytes_read         bigint[],
ssd_wear_total_bytes_written      bigint[]
);

-----------------------------------------------------------
-- fn_csm_node_attributes_query_details function_comments
-----------------------------------------------------------

COMMENT ON TYPE node_details IS 'node_details type to help fn_csm_node_attributes_query_details(text)';

---------------------------------------------------------------------------------------------------
-- csm_node_attributes_query_details function to HELP CSM API
---------------------------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION fn_csm_node_attributes_query_details(
    IN i_node_name text
)
RETURNS node_details AS $r$
DECLARE
    r node_details%rowtype;
BEGIN
    --NODE--
    SELECT 
        n.node_name, n.collection_time     , n.update_time     , n.discovered_cores     , n.discovered_dimms     , n.discovered_gpus     , n.discovered_hcas     , n.discovered_sockets     , n.discovered_ssds     , n.comment     , n.feature_1     , n.feature_2     , n.feature_3     , n.feature_4     , n.hard_power_cap     , n.installed_memory     , n.installed_swap     , n.kernel_release     , n.kernel_version     , n.machine_model     , n.os_image_name     , n.os_image_uuid     , n.physical_frame_location     , n.physical_u_location     , n.primary_agg     , n.secondary_agg     , n.serial_number     , n.state     , n.type      
    INTO                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
        r.node_name, r.node_collection_time, r.node_update_time, r.node_discovered_cores, r.node_discovered_dimms, r.node_discovered_gpus, r.node_discovered_hcas, r.node_discovered_sockets, r.node_discovered_ssds, r.node_comment, r.node_feature_1, r.node_feature_2, r.node_feature_3, r.node_feature_4, r.node_hard_power_cap, r.node_installed_memory, r.node_installed_swap, r.node_kernel_release, r.node_kernel_version, r.node_machine_model, r.node_os_image_name, r.node_os_image_uuid, r.node_physical_frame_location, r.node_physical_u_location, r.node_primary_agg, r.node_secondary_agg, r.node_serial_number, r.node_state, r.node_type  
    FROM csm_node AS n
        WHERE ( n.node_name = i_node_name )
    ;
    --DIMM--
    SELECT 
        COUNT(DISTINCT d.serial_number), array_agg(d.serial_number), array_agg(d.physical_location), array_agg(d.size) 
    INTO                                                                                                               
        r.dimm_count                   , r.dimm_serial_number      , r.dimm_physical_location      , r.dimm_size       
    FROM 
        csm_dimm AS d
    WHERE ( d.node_name = i_node_name )
    ; 
    --GPU--
    SELECT 
        COUNT(DISTINCT g.gpu_id), array_agg(g.gpu_id), array_agg(g.device_name), array_agg(g.hbm_memory), array_agg(g.inforom_image_version), array_agg(g.pci_bus_id), array_agg(g.serial_number), array_agg(g.uuid), array_agg(g.vbios) 
    INTO 
        r.gpu_count             , r.gpu_gpu_id       , r.gpu_device_name       , r.gpu_hbm_memory       , r.gpu_inforom_image_version       , r.gpu_pci_bus_id       , r.gpu_serial_number       , r.gpu_uuid       , r.gpu_vbios 
    FROM 
        csm_gpu AS g
    WHERE ( g.node_name = i_node_name )
    ; 
    --HCA--
    SELECT 
        COUNT(DISTINCT h.serial_number), array_agg(h.serial_number), array_agg(h.board_id), array_agg(h.device_name), array_agg(h.fw_ver), array_agg(h.guid), array_agg(h.hw_rev), array_agg(h.part_number), array_agg(h.pci_bus_id) 
    INTO 
        r.hca_count                    , r.hca_serial_number       , r.hca_board_id       , r.hca_device_name       , r.hca_fw_ver       , r.hca_guid       , r.hca_hw_rev       , r.hca_part_number       , r.hca_pci_bus_id         
    FROM 
        csm_hca AS h
    WHERE ( h.node_name = i_node_name )
    ; 
    --PROCESSOR--
    SELECT 
        COUNT(DISTINCT p.serial_number), array_agg(p.serial_number), array_agg(p.discovered_cores), array_agg(p.physical_location) 
    INTO                                                                                                               
        r.processor_count              , r.processor_serial_number , r.processor_discovered_cores , r.processor_physical_location  
    FROM 
        csm_processor_socket AS p
    WHERE ( p.node_name = i_node_name )
    ; 
    --SSD--
    SELECT 
        COUNT(DISTINCT s.serial_number), array_agg(s.serial_number), array_agg(s.device_name), array_agg(s.fw_ver), array_agg(s.pci_bus_id), array_agg(s.size), array_agg(s.update_time), array_agg(s.wear_lifespan_used), array_agg(s.wear_percent_spares_remaining), array_agg(s.wear_total_bytes_read), array_agg(s.wear_total_bytes_written)    
    INTO 
        r.ssd_count                    , r.ssd_serial_number       , r.ssd_device_name       , r.ssd_fw_ver       , r.ssd_pci_bus_id       , r.ssd_size       , r.ssd_update_time       , r.ssd_wear_lifespan_used       , r.ssd_wear_percent_spares_remaining       , r.ssd_wear_total_bytes_read       , r.ssd_wear_total_bytes_written 
    FROM 
        csm_ssd AS s
    WHERE ( s.node_name = i_node_name )
    ; 
    
    return r;
    
END;
$r$ LANGUAGE 'plpgsql';

-----------------------------------------------------------
-- fn_csm_node_attributes_query_details function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_node_attributes_query_details(text) is 'csm_node_attributes_query_details function to HELP CSM API.';

-----------------------------------------------------------------------------------------------
-- csm_allocation_node function to handle exclusive usage of shared nodes on INSERT.
-----------------------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION fn_csm_allocation_node_sharing_status(
    i_allocation_id             bigint,
    i_type                      text,
    i_state                     text,
    i_shared                    boolean,
    i_nodenames                 text[] -- This needs to be an array?
) RETURNS void AS $$
DECLARE
    bad_nodes text[];
    missing_nodes text[];
    running_nodes text[];
    error_code integer;
    INVALID_NODES  CONSTANT integer := 1;
    ABSENT_NODES  CONSTANT integer := 2;
    OCCUPIED_NODES CONSTANT integer := 3;
    BAD_STATE CONSTANT integer := 4;
BEGIN
    --LOCK TABLE csm_allocation_node IN EXCLUSIVE MODE;
    PERFORM 1 FROM csm_allocation_node WHERE allocation_id=i_allocation_id FOR UPDATE;

    -- TODO Should this be consolidated into one Query with missing_nodes?
    -- Determine if any of the supplied nodes were not ready, or not computes.
    bad_nodes := ARRAY(
        SELECT node_name
        FROM csm_node
        WHERE node_name = ANY(i_nodenames) AND (state != 'IN_SERVICE' OR type != 'compute')
    );

    -- Check for any missing nodes.
    missing_nodes := ARRAY (
        SELECT p.node_name
        FROM (SELECT unnest(i_nodenames) as node_name) p
        LEFT JOIN csm_node n on n.node_name = p.node_name
        WHERE n.node_name IS NULL
    );

    IF (i_type = 'diagnostics')
    THEN
        UPDATE csm_allocation SET state = i_state WHERE allocation_id=i_allocation_id;
    -- If this is not a diagnostic and any bad nodes were found
    -- OR there were nodes that couldn't be found, raise an exception.
    ELSIF (array_length(bad_nodes, 1) > 0 )
        OR  array_length(missing_nodes,1) > 0 THEN
        
        IF( array_length(bad_nodes, 1) > 0 ) 
        THEN
            error_code := INVALID_NODES;
        ELSE
            error_code := ABSENT_NODES;
        END IF;

        RAISE EXCEPTION 'The following nodes were not available: % ;The following nodes were not found: %',
                array_to_string(bad_nodes, ', ', '*' ),
                array_to_string(missing_nodes, ', ', '*')
            USING HINT = error_code;
    END IF;

    -- If the allocation is being created in the running state.
    IF (i_state='running') THEN
        IF (NOT(i_shared)) THEN

            IF EXISTS (
                SELECT state
                FROM csm_allocation_node
                WHERE node_name = ANY(i_nodenames) AND state!='staging-in' AND state!='staging-out'  ) 
            THEN
                running_nodes := ARRAY( 
                    SELECT node_name
                    FROM csm_allocation_node
                    WHERE node_name = ANY(i_nodenames) AND state!='staging-in' AND state!='staging-out');

                RAISE EXCEPTION 'Node(s) are currently busy, unable to request exclusive job. Active Nodes: %',
                        array_to_string(running_nodes, ', ', '*')
                        USING HINT = OCCUPIED_NODES;
            END IF;

        ELSIF EXISTS (
            SELECT state
            FROM csm_allocation_node
            WHERE node_name = ANY(i_nodenames) AND state!='staging-in' AND state!='staging-out' AND NOT shared ) 
        THEN
            running_nodes := ARRAY( 
                SELECT node_name
                FROM csm_allocation_node
                WHERE node_name = ANY(i_nodenames) AND state!='staging-in' AND state!='staging-out');

            RAISE EXCEPTION 'Node(s) can not be shared because an exclusive job currently active. Active Nodes: %',
                array_to_string(running_nodes, ', ', '*')
                USING HINT = OCCUPIED_NODES;
        END IF;
    ELSIF i_state!='staging-in' THEN
        RAISE EXCEPTION 'Inserting into invalid state'
            USING HINT = BAD_STATE;
    --ELSIF i_state='stage-out' THEN
        --RAISE EXCEPTION using message = 'Inserting into the stage-out state';
    END IF;

    -- If no execption was raised insert into the allocation_node table.
    INSERT INTO csm_allocation_node(
        allocation_id,
        shared,
        state,
        node_name)
    SELECT
        i_allocation_id, i_shared, i_state, node
    FROM
        unnest(i_nodenames) as n(node);

    --EXCEPTION
    --    WHEN others THEN
    --        RAISE EXCEPTION
    --        USING ERRCODE = sqlstate,
    --            HINT = ?
    --            MESSAGE = 'error_handling_test: ' || sqlstate || '/' || sqlerrm;
END; -- releases all locks
$$ LANGUAGE plpgsql;

-----------------------------------------------------------
-- csm_allocation_node_sharing_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_allocation_node_sharing_status(i_allocation_id bigint, i_type text,
    i_state text, i_shared boolean, i_nodenames text[]) is
    'csm_allocation_sharing_status function to handle exclusive usage of shared nodes on INSERT.';


-----------------------------------------------------------------------------------------------
-- csm_allocation_node function to handle storing the results of data aggregation.
-----------------------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION fn_csm_allocation_create_data_aggregator(
    i_allocation_id   bigint,
    i_state           text,
    i_node_names      text[],
    i_ib_rx_list      bigint[],
    i_ib_tx_list      bigint[],
    i_gpfs_read_list  bigint[],
    i_gpfs_write_list bigint[],
    i_energy          bigint[],
    i_power_cap       integer[],
    i_ps_ratio        integer[],
    i_power_cap_hit   bigint[],
    i_gpu_energy      bigint[],
    out o_timestamp   timestamp   
) RETURNS timestamp AS $$
DECLARE
    current_state TEXT;
BEGIN
    -- Verify the state is not "deleting"
    SELECT state INTO current_state FROM csm_allocation WHERE allocation_id=i_allocation_id;

    IF (current_state = 'deleting')
    THEN
        RAISE EXCEPTION 'Allocation was deleted while transitioning to running.';
    END IF;

    -- Update the state of the chosen allocation.
    UPDATE csm_allocation SET state = i_state WHERE allocation_id=i_allocation_id;

    -- Update the allocation node tables.
    UPDATE csm_allocation_node
    SET 
        state                = i_state,
        ib_tx                = d.tx, 
        ib_rx                = d.rx, 
        gpfs_read            = d.g_read, 
        gpfs_write           = d.g_write,
        energy               = d.energy,
        power_cap            = d.power_cap,
        power_shifting_ratio = d.ps_ratio,
        power_cap_hit        = d.pc_hit,
        gpu_energy           = d.gpu_energy
    FROM 
        ( SELECT
            unnest(i_node_names)      as node,
            unnest(i_ib_rx_list)      as rx,
            unnest(i_ib_tx_list)      as tx,
            unnest(i_gpfs_read_list)  as g_read,
            unnest(i_gpfs_write_list) as g_write,
            unnest(i_energy)          as energy,
            unnest(i_power_cap)       as power_cap,
            unnest(i_ps_ratio)        as ps_ratio,
            unnest(i_power_cap_hit)   as pc_hit,
            unnest(i_gpu_energy)      as gpu_energy
        ) d
    WHERE allocation_id = i_allocation_id AND node_name = d.node;
    
    o_timestamp=now();

END;
$$ LANGUAGE plpgsql;

COMMENT ON FUNCTION fn_csm_allocation_create_data_aggregator( 
    i_allocation_id bigint, i_state text, i_node_names text[], i_ib_rx_list bigint[], i_ib_tx_list bigint[],
    i_gpfs_read_list bigint[], i_gpfs_write_list bigint[], i_energy bigint[], 
    i_power_cap integer[], i_ps_ratio integer[], i_power_cap_hit bigint[], i_gpu_energy bigint[]) 
    is 'csm_allocation_node function to populate the data aggregator fields in csm_allocation_node.';

-----------------------------------------------------------------------------------------------
-- csm_allocation_node function to amend summarized column(s) on DELETE.
-----------------------------------------------------------------------------------------------

CREATE FUNCTION fn_csm_allocation_node_change()
    RETURNS trigger AS
    $$
    BEGIN
    IF (TG_OP = 'DELETE') THEN
         INSERT INTO csm_allocation_node_history(
            history_time,
            allocation_id,
            node_name,
            state,
            shared,
            energy,
            gpfs_read,
            gpfs_write,
            ib_tx,
            ib_rx,
            power_cap,
            power_shifting_ratio,
            power_cap_hit,
            gpu_usage,
            gpu_energy,
            cpu_usage,
            memory_usage_max)
         VALUES(
            now(),
            OLD.allocation_id,
            OLD.node_name,
            OLD.state,
            OLD.shared,
            OLD.energy,
            OLD.gpfs_read,
            OLD.gpfs_write,
            OLD.ib_tx,
            OLD.ib_rx,
            OLD.power_cap,
            OLD.power_shifting_ratio,
            OLD.power_cap_hit,
            OLD.gpu_usage,
            OLD.gpu_energy,
            OLD.cpu_usage,
            OLD.memory_usage_max);
         RETURN OLD;
    ELSEIF (TG_OP = 'UPDATE') THEN
         INSERT INTO csm_allocation_node_history(
            history_time,
            allocation_id,
            node_name,
            state,
            shared,
            energy,
            gpfs_read,
            gpfs_write,
            ib_tx,
            ib_rx,
            power_cap,
            power_shifting_ratio,
            power_cap_hit,
            gpu_usage,
            gpu_energy,
            cpu_usage,
            memory_usage_max)
         VALUES(
            now(),
            NEW.allocation_id,
            NEW.node_name,
            NEW.state,
            NEW.shared,
            NEW.energy,
            NEW.gpfs_read,
            NEW.gpfs_write,
            NEW.ib_tx,
            NEW.ib_rx,
            NEW.power_cap,
            NEW.power_shifting_ratio,
            NEW.power_cap_hit,
            NEW.gpu_usage,
            NEW.gpu_energy,
            NEW.cpu_usage,
            NEW.memory_usage_max);
         RETURN NEW;
    END IF;
--RETURN NULL;
END;
$$
LANGUAGE 'plpgsql';

---------------------------------------------------------------------------------------------------
-- csm_allocation_node trigger to amend summarized column(s) on UPDATE and DELETE.
---------------------------------------------------------------------------------------------------

CREATE TRIGGER tr_csm_allocation_node_change
    BEFORE UPDATE -- OR DELETE(UPDATE was commented out due to replecation of records 02-28-2018)
    ON csm_allocation_node
    FOR EACH ROW
    EXECUTE PROCEDURE fn_csm_allocation_node_change()
;

-----------------------------------------------------------
-- csm_allocation_node_trigger_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_allocation_node_change() is 'csm_allocation_node function to amend summarized column(s) on UPDATE and DELETE.';
COMMENT ON TRIGGER tr_csm_allocation_node_change ON csm_allocation_node is 'csm_allocation_node trigger to amend summarized column(s) on UPDATE and DELETE.';

--------------------------------------------------------------------------------------------------
-- csm_step_begin:  Function to begin a step, adds the step to csm_step and csm_step_node
--------------------------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION fn_csm_step_begin(
    i_step_id                 bigint,
    i_allocation_id           bigint,
    i_status                  text,
    i_executable              text,
    i_working_directory       text,
    i_argument                text,
    i_environment_variable    text,
    i_num_nodes               integer,
    i_num_processors          integer,
    i_num_gpus                integer,
    i_projected_memory        integer,
    i_num_tasks               integer,
    i_user_flags              text,
    i_node_names              text[],
    OUT o_begin_time          timestamp
)
RETURNS timestamp AS $$
BEGIN
    o_begin_time = now();
    INSERT INTO csm_step (
        step_id, 
        allocation_id,
        begin_time, 
        status, 
        executable, 
        working_directory, 
        argument,
        environment_variable, 
        num_nodes,
        num_processors, 
        num_gpus, 
        projected_memory, 
        num_tasks,
        user_flags
    ) VALUES (
        i_step_id, 
        i_allocation_id,
        o_begin_time,
        i_status, 
        i_executable, 
        i_working_directory, 
        i_argument,
        i_environment_variable, 
        i_num_nodes,
        i_num_processors, 
        i_num_gpus, 
        i_projected_memory, 
        i_num_tasks,
        i_user_flags
    );
    
    INSERT INTO csm_step_node 
        ( step_id, allocation_id, node_name )
    SELECT
        i_step_id, i_allocation_id, node
    FROM unnest(i_node_names) as n(node);

    --- TODO On Conflict.
END

$$ LANGUAGE 'plpgsql';

--------------------------------------------------------------------------------------------------
-- csm_step function to delete the step from the nodes table  (fn_csm_step_end)
--------------------------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION fn_csm_step_end(
        IN  i_stepid                  bigint,
        IN  i_allocationid            bigint,
        IN  i_exitstatus              int,
        IN  i_errormessage            text,
        IN  i_cpustats                text,
        IN  i_totalutime              double precision,
        IN  i_totalstime              double precision,
        IN  i_ompthreadlimit          text,
        IN  i_gpustats                text,
        IN  i_memorystats             text,
        IN  i_maxmemory               bigint,
        IN  i_iostats                 text,
        OUT o_user_flags              text,
        OUT o_num_nodes               int,
        OUT o_nodes                   text,
        OUT o_end_time                timestamp
)
RETURNS record AS $$
BEGIN
    --- XXX Speed up candidate, might be able to grab from fn_csm_step_history_dump
    SELECT user_flags, num_nodes
        INTO o_user_flags, o_num_nodes
        FROM csm_step
        WHERE step_id = i_stepid AND allocation_id = i_allocationid;
    
    -- XXX Might be able to roll into the above query?
    SELECT string_agg(node_name,',')
        INTO o_nodes
        FROM csm_step_node
        WHERE step_id = i_stepid AND allocation_id = i_allocationid;

    DELETE 
        FROM csm_step_node *
        WHERE 
        step_id = i_stepid AND 
        allocation_id = i_allocationid;

    o_end_time = now();
    PERFORM fn_csm_step_history_dump( 
        i_stepid,
        i_allocationid,
        o_end_time,
        i_exitstatus,  
        i_errormessage,     
        i_cpustats,
        i_totalutime,
        i_totalstime,     
        i_ompthreadlimit,
        i_gpustats,
        i_memorystats,
        i_maxmemory,    
        i_iostats);

    EXCEPTION
        WHEN others THEN
            RAISE EXCEPTION
            USING ERRCODE = sqlstate,
                MESSAGE = sqlerrm;
END

$$ LANGUAGE 'plpgsql';

--------------------------------------------------------------------------------------------------
-- csm_step function to amend summarized column(s) on DELETE. (csm_step_history_dump)
--------------------------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION fn_csm_step_history_dump(
        i_stepid                  bigint,
        i_allocationid            bigint,
        i_endtime                 timestamp with time zone,
        i_exitstatus              int,
        i_errormessage            text,
        i_cpustats                text,
        i_totalutime              double precision,
        i_totalstime              double precision,
        i_ompthreadlimit          text,
        i_gpustats                text,
        i_memorystats             text,
        i_maxmemory               bigint,
        i_iostats                 text
)
--RETURNS int AS $$ (This worked before with Nicks steps if problems exist then change back 07_11_2016)
RETURNS void AS $$

DECLARE 
   a "csm_step"%ROWTYPE;

BEGIN
    IF EXISTS (SELECT step_id, allocation_id FROM csm_step WHERE step_id=$1 AND allocation_id=$2) THEN
        SELECT * INTO a FROM csm_step WHERE step_id=$1 AND allocation_id=$2;
        INSERT INTO csm_step_history VALUES(
            now(),
            i_stepid,
            i_allocationid,
            a.begin_time,
            i_endtime,
            a.status,
            a.executable,
            a.working_directory,
            a.argument,
            a.environment_variable,
            a.num_nodes,
            a.num_processors,
            a.num_gpus,
            a.projected_memory,
            a.num_tasks,
            a.user_flags,
            i_exitstatus,
            i_errormessage,
            i_cpustats,
            i_totalutime,
            i_totalstime,
            i_ompthreadlimit,
            i_gpustats,
            i_memorystats,
            i_maxmemory,
            i_iostats
    );
        DELETE FROM csm_step WHERE step_id=$1 AND allocation_id=$2;
    ELSE
        RAISE EXCEPTION using message = 'step_id and or allocation_id does not exist';
    END IF;
    EXCEPTION
        WHEN others THEN
            RAISE EXCEPTION
            USING ERRCODE = sqlstate,
                MESSAGE = 'error_handling_test: ' || sqlstate || '/' || sqlerrm;
--  RETURN NULL;

END;
$$ LANGUAGE 'plpgsql';

-----------------------------------------------------------
-- csm_step_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_step_begin(
    i_step_id bigint, i_allocation_id bigint, i_status text, i_executable text, i_working_directory text,
    i_argument text, i_environment_variable text, i_num_nodes integer, i_num_processors integer,
    i_num_gpus integer, i_projected_memory integer, i_num_tasks integer,i_user_flags text, 
    i_node_names text[], OUT o_begin_time timestamp)
is 'csm_step_begin function to begin a step, adds the step to csm_step and csm_step_node';

COMMENT ON FUNCTION fn_csm_step_end(
    IN i_stepid bigint, IN i_allocationid bigint, IN i_exitstatus int, IN i_errormessage text,
    IN i_cpustats text, IN i_totalutime double precision, IN i_totalstime double precision,
    IN i_ompthreadlimit text, IN i_gpustats text, IN i_memorystats text, IN i_maxmemory bigint, IN i_iostats text,
    OUT o_user_flags text, OUT o_num_nodes int, OUT o_nodes text, OUT o_end_time timestamp)
is 'csm_step_end function to delete the step from the nodes table (fn_csm_step_end)';

COMMENT ON FUNCTION fn_csm_step_history_dump(
    i_stepid bigint, i_allocationid bigint, i_endtime timestamp with time zone, i_exitstatus int,
    i_errormessage text, i_cpustats text, i_totalutime double precision, i_totalstime double precision,
    i_ompthreadlimit text, i_gpustats text, i_memorystats text, i_maxmemory bigint, i_iostats text) 
is 'csm_step function to amend summarized column(s) on DELETE. (csm_step_history_dump)';

---------------------------------------------------------------------------------------------------
-- csm_step_node function to amend summarized column(s) on DELETE. (csm_step_node_history_dump)
---------------------------------------------------------------------------------------------------

CREATE FUNCTION fn_csm_step_node_history_dump()
RETURNS trigger AS
$$
BEGIN
    IF (TG_OP = 'DELETE') THEN
        INSERT INTO csm_step_node_history(
            history_time,
            step_id,
            allocation_id,
            node_name)
        VALUES(
            now(),
            OLD.step_id,
            OLD.allocation_id,
            OLD.node_name);
        RETURN OLD;
    END IF;
-- RETURN NULL;
END;
$$
LANGUAGE 'plpgsql';

---------------------------------------------------------------------------------------------------
-- csm_step_node trigger to amend summarized column(s) on DELETE. (csm_step_node_history_dump)
---------------------------------------------------------------------------------------------------

CREATE TRIGGER tr_csm_step_node_history_dump
    BEFORE DELETE
    ON csm_step_node
    FOR EACH ROW
    EXECUTE PROCEDURE fn_csm_step_node_history_dump()
;

-----------------------------------------------------------
-- csm_step_node_trigger_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_step_node_history_dump() is 'csm_step_node function to amend summarized column(s) on DELETE. (csm_step_node_history_dump)';
COMMENT ON TRIGGER tr_csm_step_node_history_dump ON csm_step_node is 'csm_step_node trigger to amend summarized column(s) on DELETE. (csm_step_node_history_dump)';

-----------------------------------------------------------------------------------------------------------------
-- csm_ras_type function to add rows to csm_ras_type_audit on INSERT and UPDATE and DELETE. (csm_ras_type_update)
-----------------------------------------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION fn_csm_ras_type_update() RETURNS TRIGGER AS $$
    BEGIN
        -------------------------------------------------------------------------------------------
        -- Perform the required operation on csm_ras_type, and create a row in csm_ras_type_audit
        -- to reflect the change made to csm_ras_type.
        -------------------------------------------------------------------------------------------
        IF (TG_OP = 'DELETE') THEN
            INSERT INTO csm_ras_type_audit(
                msg_id_seq,
                operation,
                change_time,
                msg_id,
                severity,
                message,
                description,
                control_action,
                threshold_count,
                threshold_period,
                enabled,
                set_state,
                visible_to_users)
            VALUES( 
                nextval('csm_ras_type_audit_msg_id_seq_seq'), 
                'D', 
                now(),
                OLD.msg_id,
                OLD.severity,
                OLD.message,
                OLD.description,
                OLD.control_action,
                OLD.threshold_count,
                OLD.threshold_period,
                OLD.enabled,
                OLD.set_state,
                OLD.visible_to_users);
            RETURN OLD;
        ELSIF (TG_OP = 'UPDATE') THEN
            INSERT INTO csm_ras_type_audit(
                msg_id_seq,
                operation,
                change_time,
                msg_id,
                severity,
                message,
                description,
                control_action,
                threshold_count,
                threshold_period,
                enabled,
                set_state,
                visible_to_users)
            VALUES( 
                nextval('csm_ras_type_audit_msg_id_seq_seq'), 
                'U', 
                now(),
                NEW.msg_id,
                NEW.severity,
                NEW.message,
                NEW.description,
                NEW.control_action,
                NEW.threshold_count,
                NEW.threshold_period,
                NEW.enabled,
                NEW.set_state,
                NEW.visible_to_users);
            RETURN NEW;
        ELSIF (TG_OP = 'INSERT') THEN
            INSERT INTO csm_ras_type_audit(
                msg_id_seq,
                operation,
                change_time,
                msg_id,
                severity,
                message,
                description,
                control_action,
                threshold_count,
                threshold_period,
                enabled,
                set_state,
                visible_to_users)
            VALUES( 
                nextval('csm_ras_type_audit_msg_id_seq_seq'), 
                'I', 
                now(),
                NEW.msg_id,
                NEW.severity,
                NEW.message,
                NEW.description,
                NEW.control_action,
                NEW.threshold_count,
                NEW.threshold_period,
                NEW.enabled,
                NEW.set_state,
                NEW.visible_to_users);
            RETURN NEW;
        END IF;
    END;
$$ LANGUAGE plpgsql;

----------------------------------------------------------------------------------------------------------------
-- csm_ras_type trigger to add rows to csm_ras_type_audit on INSERT and UPDATE and DELETE. (csm_ras_type_update)
----------------------------------------------------------------------------------------------------------------

CREATE TRIGGER tr_csm_ras_type_update
AFTER INSERT OR UPDATE OR DELETE ON csm_ras_type
    FOR EACH ROW EXECUTE PROCEDURE fn_csm_ras_type_update();

-----------------------------------------------------------
-- csm_ras_type_trigger_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_ras_type_update() is 'csm_ras_type function to add rows to csm_ras_type_audit on INSERT and UPDATE and DELETE. (csm_ras_type_update)';
COMMENT ON TRIGGER tr_csm_ras_type_update ON csm_ras_type is 'csm_ras_type trigger to add rows to csm_ras_type_audit on INSERT and UPDATE and DELETE. (csm_ras_type_update)';

--------------------------------------------------------------------------------------------------
-- csm_vg_create:  Function to create a vg, adds the vg to csm_vg_ssd and csm_vg
--------------------------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION fn_csm_vg_create(
    i_available_size          bigint,
    i_node_name               text,
    i_ssd_count               int,
    i_ssd_serial_numbers      text[],
    i_ssd_allocations         bigint[],
    i_total_size              bigint,
    i_vg_name                 text,
    i_is_scheduler            boolean
)
RETURNS void AS $$
DECLARE
    sum_check bigint := 0;
BEGIN
    -----------------------------------------------------------------
    --ssd_serial_numbers length check
    IF (array_length(i_ssd_serial_numbers, 1) != i_ssd_count) 
    THEN RAISE EXCEPTION 'length of ssd_serial_numbers does not match ssd_count';
    END IF;
    -----------------------------------------------------------------
    --ssd_allocations length check
    IF (array_length(i_ssd_allocations, 1) != i_ssd_count) 
    THEN RAISE EXCEPTION 'length of ssd_allocations does not match ssd_count';
    END IF;
    -----------------------------------------------------------------
    -- enough total space on an ssd check
    FOR i IN 1..i_ssd_count LOOP 
        IF EXISTS (
            SELECT 
                serial_number
            FROM  
                csm_ssd                  
            WHERE ( 
                serial_number = i_ssd_serial_numbers[i] 
                AND
                i_ssd_allocations[i] > size
            )
        ) 
        THEN 
            RAISE EXCEPTION 'Requested space on ssd serial number: % is greater than its size', i_ssd_serial_numbers[i];
        END IF;
    END LOOP;
    
    -----------------------------------------------------------------
    -- enough allocated space on an ssd check
    
    --set up a temp table to store "used_size" of an ssd on the fly
    CREATE TABLE csm_fn_vg_create_temp (
        serial_number text,
        used_size     bigint
    );
    
    --fill that table
    INSERT INTO csm_fn_vg_create_temp (serial_number, used_size)
    SELECT 
        csm_vg_ssd.serial_number, SUM (csm_vg_ssd.ssd_allocation)
    FROM 
        csm_vg_ssd 
    WHERE ( 
        csm_vg_ssd.serial_number = ANY(i_ssd_serial_numbers)
    )
    GROUP BY 
        csm_vg_ssd.serial_number
    ;
    
    -- if this fires then there is an ssd with no available space for this vg create
    FOR i IN 1..i_ssd_count LOOP 
        IF EXISTS (
            SELECT 
                t.serial_number
            FROM 
                csm_fn_vg_create_temp AS t, 
                csm_ssd AS s                 
            WHERE ( 
                t.serial_number = i_ssd_serial_numbers[i] 
                AND
                t.serial_number = s.serial_number 
                AND
                (i_ssd_allocations[i] + t.used_size) > s.size
            )
        ) 
        THEN 
            DROP TABLE csm_fn_vg_create_temp;
            RAISE EXCEPTION 'Not enough space available on ssd serial number: % ', i_ssd_serial_numbers[i];
        END IF;
        -- add up allocations as we loop for a later check
        sum_check := sum_check + i_ssd_allocations[i];
    END LOOP;
    
    -- remove temp table
    DROP TABLE csm_fn_vg_create_temp;
    -----------------------------------------------------------------
    -- check that the sum of all i_ssd_allocations equals total_size
    IF i_total_size != sum_check 
    THEN 
    RAISE EXCEPTION 'Total size does not equal the sum of all ssd allocations provided.';
    END IF;
    -----------------------------------------------------------------    
    --check if a scheduler VG already exists on this node
    IF EXISTS (
        SELECT
            vg_name,
            node_name,
            scheduler 
        FROM 
            csm_vg 
        WHERE (
            node_name = i_node_name 
            AND 
            scheduler = TRUE 
            AND 
            i_is_scheduler = TRUE 
        )
    ) THEN 
        RAISE EXCEPTION 'A scheduler VG already exists on this node. There can only be one scheduler VG per node.';
    END IF;

    ---------------------------------------------------------------------------------------------
    -- All checks pass. Now run VG create functionality.       
    ---------------------------------------------------------------------------------------------
    --insert the vg
    INSERT INTO csm_vg (
        vg_name,
        node_name,
        total_size, 
        available_size, 
        scheduler, 
        update_time
        ) VALUES (
        i_vg_name,
        i_node_name,
        i_total_size,
        i_available_size, 
        i_is_scheduler,
        now() 
    );

    -- insert into the vg ssd relation
    INSERT INTO csm_vg_ssd 
        (vg_name, node_name, serial_number, ssd_allocation)
    SELECT
        i_vg_name, i_node_name, t.s, t.a
    FROM ( SELECT unnest(i_ssd_serial_numbers) as s, unnest(i_ssd_allocations) as a ) t ;
        
    ---------------------------------------------------------------------------------------------
    --- TODO On Conflict.
    ---------------------------------------------------------------------------------------------
    EXCEPTION
        WHEN others THEN
            RAISE EXCEPTION
            USING ERRCODE = sqlstate,
                MESSAGE = 'error_handling_test: ' || sqlstate || '/' || sqlerrm;
END;

$$ LANGUAGE 'plpgsql';

-----------------------------------------------------------
-- csm_vg_create_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_vg_create(i_available_size bigint, i_node_name text, i_ssd_count int, i_ssd_serial_numbers text[], i_ssd_allocations bigint[], i_total_size bigint, i_vg_name text, i_is_scheduler boolean) is 'Function to create a vg, adds the vg to csm_vg_ssd and csm_vg';


--------------------------------------------------------------------------------------------------
-- csm_vg_delete:  Function to delete a vg, and remove records in the csm_vg and csm_vg_ssd tables
--------------------------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION fn_csm_vg_delete(
    i_node_name text,
    i_vg_name   text
)
RETURNS void AS $$
BEGIN
    ---------------------------------------------------------
    -- Select from the table first to check if a matching vg exist on this node
    ---------------------------------------------------------
    IF EXISTS (
        SELECT
            v.vg_name
        FROM
            csm_vg v
        WHERE (
            v.node_name = i_node_name
        AND
            v.vg_name = i_vg_name
        ) 
    )
    THEN
        ------------------------------------------------------------
        -- Removes corresponding vg_ssd relation from csm_vg table
        ------------------------------------------------------------
        DELETE FROM csm_vg_ssd WHERE (vg_name = i_vg_name AND node_name = i_node_name);
        
        ---------------------------------------------------------------------------------------------------------
        -- Removes vg from csm_vg table 
        ---------------------------------------------------------------------------------------------------------
        DELETE FROM csm_vg WHERE ( vg_name = i_vg_name AND node_name = i_node_name );

    ------------------------------------------------------------
    --- TODO On Conflict.
    ------------------------------------------------------------
    ELSE
        RAISE EXCEPTION using message = 'vg_name does not exist on this node';
    END IF;
    EXCEPTION
        WHEN others THEN
            RAISE EXCEPTION
            USING ERRCODE = sqlstate,
                MESSAGE = 'error_handling_test: ' || sqlstate || '/' || sqlerrm;
END;
$$ LANGUAGE 'plpgsql';

-----------------------------------------------------------
-- csm_vg_delete_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_vg_delete( i_node_name text, i_vg_name text) is 'Function to delete a vg, and remove records in the csm_vg and csm_vg_ssd tables';

-----------------------------------------------------------------------------------------------
-- csm_vg_history_dump function to amend summarized column(s) on UPDATE and DELETE.
-----------------------------------------------------------------------------------------------

CREATE FUNCTION fn_csm_vg_history_dump()
     RETURNS trigger AS
     $$
     BEGIN
     IF (TG_OP = 'DELETE') THEN
        INSERT INTO csm_vg_history(
            history_time,
            vg_name,
            node_name,
            total_size,
            available_size,
            scheduler,
            update_time,
            operation)
        VALUES(
            now(),
            OLD.vg_name,
            OLD.node_name,
            OLD.total_size,
            OLD.available_size,
            OLD.scheduler,
            OLD.update_time,
            'D');
        RETURN OLD;
     ELSEIF (TG_OP = 'UPDATE') THEN
     NEW.update_time = now();
        IF  (
            OLD.vg_name IS DISTINCT FROM NEW.vg_name OR
            OLD.node_name IS DISTINCT FROM NEW.node_name OR
            OLD.total_size IS DISTINCT FROM NEW.total_size OR
            OLD.available_size IS DISTINCT FROM NEW.available_size OR
            OLD.scheduler IS DISTINCT FROM NEW.scheduler) THEN
        INSERT INTO csm_vg_history(
            history_time,
            vg_name,
            node_name,
            total_size,
            available_size,
            scheduler,
            update_time,
            operation)
        VALUES(
            now(),
            NEW.vg_name,
            NEW.node_name,
            NEW.total_size,
            NEW.available_size,
            NEW.scheduler,
            NEW.update_time,
            'U');
        END IF;
        RETURN NEW;
     ELSEIF (TG_OP = 'INSERT') THEN
     NEW.update_time = now();
        INSERT INTO csm_vg_history(
            history_time,
            vg_name,
            node_name,
            total_size,
            available_size,
            scheduler,
            update_time,
            operation)
        VALUES(
            now(),
            NEW.vg_name,
            NEW.node_name,
            NEW.total_size,
            NEW.available_size,
            NEW.scheduler,
            NEW.update_time,
            'I');
        RETURN NEW;
    END IF;
-- RETURN NULL;
END;
$$
LANGUAGE 'plpgsql';

---------------------------------------------------------------------------------------------------
-- csm_vg_history_dump trigger to amend summarized column(s) on UPDATE and DELETE.
---------------------------------------------------------------------------------------------------

CREATE TRIGGER tr_csm_vg_history_dump
    BEFORE INSERT OR DELETE OR UPDATE
    ON csm_vg
    FOR EACH ROW
    EXECUTE PROCEDURE fn_csm_vg_history_dump()
;

-----------------------------------------------------------
-- csm_vg_history_dump_trigger_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_vg_history_dump() is 'csm_vg function to amend summarized column(s) on UPDATE and DELETE.';
COMMENT ON TRIGGER tr_csm_vg_history_dump ON csm_vg is 'csm_vg trigger to amend summarized column(s) on UPDATE and DELETE.';

-----------------------------------------------------------------------------------------------
-- csm_vg_ssd_history_dump function to amend summarized column(s) on UPDATE and DELETE.
-----------------------------------------------------------------------------------------------

CREATE FUNCTION fn_csm_vg_ssd_history_dump()
     RETURNS trigger AS
     $$
     BEGIN
     IF (TG_OP = 'DELETE') THEN
        INSERT INTO csm_vg_ssd_history(
            history_time,
            vg_name,
            node_name,
            serial_number,
            ssd_allocation,
            operation)
        VALUES(
            now(),
            OLD.vg_name,
            OLD.node_name,
            OLD.serial_number,
            OLD.ssd_allocation,
            'D');
        RETURN OLD;
     ELSEIF (TG_OP = 'UPDATE') THEN
        IF (    
            OLD.vg_name IS DISTINCT FROM NEW.vg_name OR
            OLD.node_name IS DISTINCT FROM NEW.node_name OR
            OLD.serial_number IS DISTINCT FROM NEW.serial_number OR
            OLD.ssd_allocation IS DISTINCT FROM NEW.ssd_allocation) THEN
        INSERT INTO csm_vg_ssd_history(
            history_time,
            vg_name,
            node_name,
            serial_number,
            ssd_allocation,
            operation)
        VALUES(
            now(),
            NEW.vg_name,
            NEW.node_name,
            NEW.serial_number,
            NEW.ssd_allocation,
            'U');
        END IF;
        RETURN NEW;
     ELSEIF (TG_OP = 'INSERT') THEN
        INSERT INTO csm_vg_ssd_history(
            history_time,
            vg_name,
            node_name,
            serial_number,
            ssd_allocation,
            operation)
        VALUES(
            now(),
            NEW.vg_name,
            NEW.node_name,
            NEW.serial_number,
            NEW.ssd_allocation,
            'I');
        RETURN NEW;
    END IF;
-- RETURN NULL;
END;
$$
LANGUAGE 'plpgsql';

---------------------------------------------------------------------------------------------------
-- csm_vg_ssd_history_dump trigger to amend summarized column(s) on UPDATE and DELETE.
---------------------------------------------------------------------------------------------------

CREATE TRIGGER tr_csm_vg_ssd_history_dump
    BEFORE INSERT OR DELETE OR UPDATE
    ON csm_vg_ssd
    FOR EACH ROW
    EXECUTE PROCEDURE fn_csm_vg_ssd_history_dump()
;

-----------------------------------------------------------
-- csm_vg_ssd_history_dump_trigger_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_vg_ssd_history_dump() is 'csm_vg_ssd function to amend summarized column(s) on UPDATE and DELETE.';
COMMENT ON TRIGGER tr_csm_vg_ssd_history_dump ON csm_vg_ssd is 'csm_vg_ssd trigger to amend summarized column(s) on UPDATE and DELETE.';

-----------------------------------------------------------------------------------------------
-- csm_ssd_history_dump function to amend summarized column(s) on UPDATE and DELETE.
-- To Do: Built in check and to update "update_time" regardless of which fields get updated
-- This trigger covers three coditions
-- 1. Checks to see if any colums have changed besides the 'wear' fields (Records into the csm_ssd_history table)
-- 2. Checks to see if any colums have changed including the 'wear' fields (This records the change in the csm_ssd_wear_history and csm_ssd_history table)
-- 3. Checks to see if just the 'wear' fields have changed and is recorded in the csm_ssd_wear_history table
---------------------------------------------------------------------------------------------------
-- Normal Default
--      1.) Update/delete a record
--          Do: push into history
-- Exceptions:
--      1.) Entering a record which is the same (no changes)
--          Do: Update collection time of active record
--      2.) API Update fields are the same
--          Do: Nothing
---------------------------------------------------------------------------------------------------

CREATE FUNCTION fn_csm_ssd_history_dump()
     RETURNS trigger AS
     $$
     BEGIN
    IF (TG_OP = 'DELETE') THEN
        -- clean up any possible vg and lv on this hard drive
        PERFORM fn_csm_ssd_dead_records(OLD.serial_number); 
        -- standard dump practice
        INSERT INTO csm_ssd_history(
            history_time,
            serial_number,
            node_name,
            update_time,
            device_name,
            pci_bus_id,
            fw_ver,
            size,
            wear_lifespan_used,
            wear_total_bytes_written,
            wear_total_bytes_read,
            wear_percent_spares_remaining,
            operation)
        VALUES(
            now(),
            OLD.serial_number,
            OLD.node_name,
            OLD.update_time,
            OLD.device_name,
            OLD.pci_bus_id,
            OLD.fw_ver,
            OLD.size,
            OLD.wear_lifespan_used,
            OLD.wear_total_bytes_written,
            OLD.wear_total_bytes_read,
            OLD.wear_percent_spares_remaining,
            'D');
        RETURN OLD;
    ELSEIF (TG_OP = 'UPDATE') THEN
        NEW.update_time = now();
        IF  (
            OLD.serial_number IS DISTINCT FROM NEW.serial_number OR
            OLD.node_name IS DISTINCT FROM NEW.node_name OR
            OLD.device_name IS DISTINCT FROM NEW.device_name OR
            OLD.pci_bus_id IS DISTINCT FROM NEW.pci_bus_id OR
            OLD.fw_ver IS DISTINCT FROM NEW.fw_ver OR
            OLD.size IS DISTINCT FROM NEW.size) THEN
        INSERT INTO csm_ssd_history(
            history_time,
            serial_number,
            node_name,
            update_time,
            device_name,
            pci_bus_id,
            fw_ver,
            size,
            wear_lifespan_used,
            wear_total_bytes_written,
            wear_total_bytes_read,
            wear_percent_spares_remaining,
            operation)
        VALUES(
            now(),
            NEW.serial_number,
            NEW.node_name,
            NEW.update_time,
            NEW.device_name,
            NEW.pci_bus_id,
            NEW.fw_ver,
            NEW.size,
            NEW.wear_lifespan_used,
            NEW.wear_total_bytes_written,
            NEW.wear_total_bytes_read,
            NEW.wear_percent_spares_remaining,
            'U');
        END IF;
        RETURN NEW;
    ELSEIF (TG_OP = 'INSERT') THEN
        NEW.update_time = now();
        INSERT INTO csm_ssd_history(
            history_time,
            serial_number,
            node_name,
            update_time,
            device_name,
            pci_bus_id,
            fw_ver,
            size,
            wear_lifespan_used,
            wear_total_bytes_written,
            wear_total_bytes_read,
            wear_percent_spares_remaining,
            operation)
        VALUES(
            now(),
            NEW.serial_number,
            NEW.node_name,
            NEW.update_time,
            NEW.device_name,
            NEW.pci_bus_id,
            NEW.fw_ver,
            NEW.size,
            NEW.wear_lifespan_used,
            NEW.wear_total_bytes_written,
            NEW.wear_total_bytes_read,
            NEW.wear_percent_spares_remaining,
            'I');
        RETURN NEW;     
    END IF;
-- RETURN NULL;
END;
$$
LANGUAGE 'plpgsql';

---------------------------------------------------------------------------------------------------
-- csm_ssd_history_dump trigger to amend summarized column(s) on UPDATE and DELETE.
---------------------------------------------------------------------------------------------------

CREATE TRIGGER tr_csm_ssd_history_dump
    BEFORE INSERT OR DELETE OR UPDATE OF serial_number, node_name, device_name, pci_bus_id, fw_ver, size
    ON csm_ssd
    FOR EACH ROW
    EXECUTE PROCEDURE fn_csm_ssd_history_dump()
;

-----------------------------------------------------------
-- csm_ssd_history_dump_trigger_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_ssd_history_dump() is 'csm_ssd function to amend summarized column(s) on UPDATE and DELETE.';
COMMENT ON TRIGGER tr_csm_ssd_history_dump ON csm_ssd is 'csm_ssd trigger to amend summarized column(s) on UPDATE and DELETE.';

-----------------------------------------------------------
-- fn_csm_ssd_dead_records 
-----------------------------------------------------------

CREATE FUNCTION fn_csm_ssd_dead_records(
    i_sn text
)
RETURNS void AS $$
DECLARE
    vg_count int;
    lv_on_vg int;
    vg_names_on_ssd text[];
    matching_node_names text[];
    t_lv_name text;
    t_node_name text;
    t_allocation_id bigint;
BEGIN

    SELECT 
        count(vg_name)
    INTO vg_count
    FROM 
        csm_vg_ssd
    WHERE (
        serial_number = i_sn
    );

    IF ( vg_count > 0) THEN
        -- look further
        SELECT
            array_agg(vg_name), array_agg(node_name)
        INTO
            vg_names_on_ssd, matching_node_names
        FROM
            csm_vg_ssd
        WHERE (
            serial_number = i_sn
        );
        --loop through that array
        -- find any lvs on those vgs
        FOR i IN 1..vg_count LOOP

            SELECT
                count(logical_volume_name)
            INTO lv_on_vg
            FROM
                csm_lv
            WHERE (
                vg_name = vg_names_on_ssd[i] AND
                node_name = matching_node_names[i]
            );

            IF (lv_on_vg > 0) THEN 

                FOR j IN 1..lv_on_vg LOOP
                    SELECT
                        logical_volume_name, node_name, allocation_id
                    INTO
                        t_lv_name, t_node_name, t_allocation_id
                    FROM
                        csm_lv
                    WHERE (
                        vg_name = vg_names_on_ssd[j] AND
                        node_name = matching_node_names[j]
                    );

                    PERFORM fn_csm_lv_history_dump(t_lv_name, t_node_name, t_allocation_id, 'now()', 'now()', null, null, null, null);
                END LOOP;
            END IF;
            -- once cleaned up all lvs on a vg, then we can remove the vg itself
            PERFORM fn_csm_vg_delete(matching_node_names[i], vg_names_on_ssd[i]);
        END LOOP;
    END IF;
END;
$$
LANGUAGE 'plpgsql';

-----------------------------------------------------------
-- fn_csm_ssd_dead_records comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_ssd_dead_records( i_sn text ) is 'Delete any vg and lv on an ssd that is being deleted.';

---------------------------------------------------------------------------------------------------
-- csm_ssd_wear function to amend summarized column(s) on UPDATE
---------------------------------------------------------------------------------------------------

CREATE FUNCTION fn_csm_ssd_wear()
    RETURNS trigger AS
$$
BEGIN
    IF (TG_OP = 'UPDATE') THEN
    NEW.update_time = now();
        IF  (
            OLD.wear_lifespan_used IS DISTINCT FROM NEW.wear_lifespan_used OR
            OLD.wear_total_bytes_written IS DISTINCT FROM NEW.wear_total_bytes_written OR
            OLD.wear_total_bytes_read IS DISTINCT FROM NEW.wear_total_bytes_read OR
            OLD.wear_percent_spares_remaining IS DISTINCT FROM NEW.wear_percent_spares_remaining) THEN
        INSERT INTO csm_ssd_wear_history(
            history_time,
            serial_number,
            node_name,
            wear_lifespan_used,
            wear_total_bytes_written,
            wear_total_bytes_read,
            wear_percent_spares_remaining,
            operation)
        VALUES(
            now(),
            NEW.serial_number,
            NEW.node_name,
            NEW.wear_lifespan_used,
            NEW.wear_total_bytes_written,
            NEW.wear_total_bytes_read,
            NEW.wear_percent_spares_remaining,
            'U');
    END IF;
    RETURN NEW;
    END IF;
END;
$$
LANGUAGE 'plpgsql';

---------------------------------------------------------------------------------------------------
-- csm_ssd_wear trigger to amend summarized column(s) on UPDATE and DELETE.
---------------------------------------------------------------------------------------------------

CREATE TRIGGER tr_csm_ssd_wear
    BEFORE UPDATE OF wear_lifespan_used, wear_total_bytes_written, wear_total_bytes_read, wear_percent_spares_remaining
    ON csm_ssd
    FOR EACH ROW
    EXECUTE PROCEDURE fn_csm_ssd_wear()
;

-----------------------------------------------------------
-- csm_ssd_wear_trigger_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_ssd_wear() is 'csm_ssd function to amend summarized column(s) on UPDATE.';
COMMENT ON TRIGGER tr_csm_ssd_wear ON csm_ssd is 'csm_ssd_wear trigger to amend summarized column(s) on UPDATE.';

-----------------------------------------------------------------------------------------------
-- csm_processor_socket_history_dump function to amend summarized column(s) on UPDATE and DELETE.
-----------------------------------------------------------------------------------------------

CREATE FUNCTION fn_csm_processor_socket_history_dump()
RETURNS trigger AS
$$
BEGIN
    IF (TG_OP = 'DELETE') THEN
        INSERT INTO csm_processor_socket_history(
            history_time,
            serial_number,
            node_name,
            physical_location,
            discovered_cores,
            operation)
        VALUES(
            now(),
            OLD.serial_number,
            OLD.node_name,
            OLD.physical_location,
            OLD.discovered_cores,
            'D');
        RETURN OLD;
    ELSEIF (TG_OP = 'UPDATE') THEN
        IF (
            OLD.serial_number IS DISTINCT FROM NEW.serial_number OR
            OLD.node_name IS DISTINCT FROM NEW.node_name OR
            OLD.physical_location IS DISTINCT FROM NEW.physical_location OR
            OLD.discovered_cores IS DISTINCT FROM NEW.discovered_cores) THEN
        INSERT INTO csm_processor_socket_history(
            history_time,
            serial_number,
            node_name,
            physical_location,
            discovered_cores,
            operation)
        VALUES(
            now(),
            NEW.serial_number,
            NEW.node_name,
            NEW.physical_location,
            NEW.discovered_cores,
            'U');
        END IF;
        RETURN NEW;
    ELSEIF (TG_OP = 'INSERT') THEN
        INSERT INTO csm_processor_socket_history(
            history_time,
            serial_number,
            node_name,
            physical_location,
            discovered_cores,
            operation)
        VALUES(
            now(),
            NEW.serial_number,
            NEW.node_name,
            NEW.physical_location,
            NEW.discovered_cores,
            'I');
        RETURN NEW;
    END IF;
-- RETURN NULL;
END;
$$
LANGUAGE 'plpgsql';

---------------------------------------------------------------------------------------------------
-- csm_processor_socket_history_dump trigger to amend summarized column(s) on UPDATE and DELETE.
---------------------------------------------------------------------------------------------------

CREATE TRIGGER tr_csm_processor_socket_history_dump
    BEFORE INSERT OR DELETE OR UPDATE
    ON csm_processor_socket
    FOR EACH ROW
    EXECUTE PROCEDURE fn_csm_processor_socket_history_dump()
;

-----------------------------------------------------------
-- csm_processor_socket_history_dump_trigger_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_processor_socket_history_dump() is 'csm_processor_socket function to amend summarized column(s) on UPDATE and DELETE.';
COMMENT ON TRIGGER tr_csm_processor_socket_history_dump ON csm_processor_socket is 'csm_processor_socket trigger to amend summarized column(s) on UPDATE and DELETE.';

-----------------------------------------------------------------------------------------------
-- csm_gpu_history_dump function to amend summarized column(s) on UPDATE and DELETE.
-----------------------------------------------------------------------------------------------

CREATE FUNCTION fn_csm_gpu_history_dump()
    RETURNS trigger AS
    $$
    BEGIN
    IF (TG_OP = 'DELETE') THEN
        INSERT INTO csm_gpu_history(
            history_time,
            serial_number,
            node_name,
            gpu_id,
            device_name,
            pci_bus_id,
            uuid,
            vbios,
            inforom_image_version,
            hbm_memory,
            operation)
        VALUES(
            now(),
            OLD.serial_number,
            OLD.node_name,
            OLD.gpu_id,
            OLD.device_name,
            OLD.pci_bus_id,
            OLD.uuid,
            OLD.vbios,
            OLD.inforom_image_version,
            OLD.hbm_memory,
            'D');
        RETURN OLD;
    ELSEIF (TG_OP = 'UPDATE') THEN
        IF (
            OLD.serial_number IS DISTINCT FROM NEW.serial_number OR
            OLD.node_name IS DISTINCT FROM NEW.node_name OR
            OLD.gpu_id IS DISTINCT FROM NEW.gpu_id OR
            OLD.device_name IS DISTINCT FROM NEW.device_name OR
            OLD.pci_bus_id IS DISTINCT FROM NEW.pci_bus_id OR
            OLD.uuid IS DISTINCT FROM NEW.uuid OR
            OLD.vbios IS DISTINCT FROM NEW.vbios OR
            OLD.inforom_image_version IS DISTINCT FROM NEW.inforom_image_version OR
            OLD.hbm_memory IS DISTINCT FROM NEW.hbm_memory) THEN
        INSERT INTO csm_gpu_history(
            history_time,
            serial_number,
            node_name,
            gpu_id,
            device_name,
            pci_bus_id,
            uuid,
            vbios,
            inforom_image_version,
            hbm_memory,
            operation)
        VALUES(
            now(),
            NEW.serial_number,
            NEW.node_name,
            NEW.gpu_id,
            NEW.device_name,
            NEW.pci_bus_id,
            NEW.uuid,
            NEW.vbios,
            NEW.inforom_image_version,
            NEW.hbm_memory,
            'U');
        END IF;
        RETURN NEW;
     ELSEIF (TG_OP = 'INSERT') THEN
        INSERT INTO csm_gpu_history(
            history_time,
            serial_number,
            node_name,
            gpu_id,
            device_name,
            pci_bus_id,
            uuid,
            vbios,
            inforom_image_version,
            hbm_memory,
            operation)
        VALUES(
            now(),
            NEW.serial_number,
            NEW.node_name,
            NEW.gpu_id,
            NEW.device_name,
            NEW.pci_bus_id,
            NEW.uuid,
            NEW.vbios,
            NEW.inforom_image_version,
            NEW.hbm_memory,
            'I');
        RETURN NEW;
    END IF;
-- RETURN NULL;
END;
$$
LANGUAGE 'plpgsql';

---------------------------------------------------------------------------------------------------
-- csm_gpu_history_dump trigger to amend summarized column(s) on UPDATE and DELETE.
---------------------------------------------------------------------------------------------------

CREATE TRIGGER tr_csm_gpu_history_dump
    BEFORE INSERT OR DELETE OR UPDATE
    ON csm_gpu
    FOR EACH ROW
    EXECUTE PROCEDURE fn_csm_gpu_history_dump()
;

-----------------------------------------------------------
-- csm_gpu_history_dump_trigger_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_gpu_history_dump() is 'csm_gpu function to amend summarized column(s) on UPDATE and DELETE.';
COMMENT ON TRIGGER tr_csm_gpu_history_dump ON csm_gpu is 'csm_gpu trigger to amend summarized column(s) on UPDATE and DELETE.';

-----------------------------------------------------------------------------------------------
-- csm_hca_history_dump function to amend summarized column(s) on UPDATE and DELETE.
-----------------------------------------------------------------------------------------------

CREATE FUNCTION fn_csm_hca_history_dump()
     RETURNS trigger AS
     $$
     BEGIN
    IF (TG_OP = 'DELETE') THEN
        INSERT INTO csm_hca_history(
            history_time,
            serial_number,
            node_name,
            device_name,
            pci_bus_id,
            guid,
            part_number,
            fw_ver,
            hw_rev,
            board_id,
            operation)
        VALUES(
            now(),
            OLD.serial_number,
            OLD.node_name,
            OLD.device_name,
            OLD.pci_bus_id,
            OLD.guid,
            OLD.part_number,
            OLD.fw_ver,
            OLD.hw_rev,
            OLD.board_id,
            'D');
        RETURN OLD;
     ELSEIF (TG_OP = 'UPDATE') THEN
        IF (
            OLD.serial_number IS DISTINCT FROM NEW.serial_number OR
            OLD.node_name IS DISTINCT FROM NEW.node_name OR
            OLD.device_name IS DISTINCT FROM NEW.device_name OR
            OLD.pci_bus_id IS DISTINCT FROM NEW.pci_bus_id OR
            OLD.guid IS DISTINCT FROM NEW.guid OR
            OLD.part_number IS DISTINCT FROM NEW.part_number OR
            OLD.fw_ver IS DISTINCT FROM NEW.fw_ver OR
            OLD.hw_rev IS DISTINCT FROM NEW.hw_rev OR
            OLD.board_id IS DISTINCT FROM NEW.board_id) THEN
        INSERT INTO csm_hca_history(
            history_time,
            serial_number,
            node_name,
            device_name,
            pci_bus_id,
            guid,
            part_number,
            fw_ver,
            hw_rev,
            board_id,
            operation)
        VALUES(
            now(),
            NEW.serial_number,
            NEW.node_name,
            NEW.device_name,
            NEW.pci_bus_id,
            NEW.guid,
            NEW.part_number,
            NEW.fw_ver,
            NEW.hw_rev,
            NEW.board_id,
            'U');
        END IF;
        RETURN NEW;
     ELSEIF (TG_OP = 'INSERT') THEN
        INSERT INTO csm_hca_history(
            history_time,
            serial_number,
            node_name,
            device_name,
            pci_bus_id,
            guid,
            part_number,
            fw_ver,
            hw_rev,
            board_id,
            operation)
        VALUES(
            now(),
            NEW.serial_number,
            NEW.node_name,
            NEW.device_name,
            NEW.pci_bus_id,
            NEW.guid,
            NEW.part_number,
            NEW.fw_ver,
            NEW.hw_rev,
            NEW.board_id,
            'I');
        RETURN NEW;
    END IF;
-- RETURN NULL;
END;
$$
LANGUAGE 'plpgsql';

---------------------------------------------------------------------------------------------------
-- csm_hca_history_dump trigger to amend summarized column(s) on UPDATE and DELETE.
---------------------------------------------------------------------------------------------------

CREATE TRIGGER tr_csm_hca_history_dump
    BEFORE INSERT OR DELETE OR UPDATE
    ON csm_hca
    FOR EACH ROW
    EXECUTE PROCEDURE fn_csm_hca_history_dump()
;

-----------------------------------------------------------
-- csm_hca_history_dump_trigger_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_hca_history_dump() is 'csm_hca function to amend summarized column(s) on UPDATE and DELETE.';
COMMENT ON TRIGGER tr_csm_hca_history_dump ON csm_hca is 'csm_hca trigger to amend summarized column(s) on UPDATE and DELETE.';

-----------------------------------------------------------------------------------------------
-- csm_dimm_history_dump function to amend summarized column(s) on UPDATE and DELETE.
-----------------------------------------------------------------------------------------------

CREATE FUNCTION fn_csm_dimm_history_dump()
     RETURNS trigger AS
     $$
     BEGIN
     IF (TG_OP = 'DELETE') THEN
        INSERT INTO csm_dimm_history(
            history_time,
            serial_number,
            node_name,
            size,
            physical_location,
            operation)
        VALUES(
            now(),
            OLD.serial_number,
            OLD.node_name,
            OLD.size,
            OLD.physical_location,
            'D');
        RETURN OLD;
     ELSEIF (TG_OP = 'UPDATE') THEN
        IF (
            OLD.serial_number IS DISTINCT FROM NEW.serial_number OR
            OLD.node_name IS DISTINCT FROM NEW.node_name OR
            OLD.size IS DISTINCT FROM NEW.size OR
            OLD.physical_location IS DISTINCT FROM NEW.physical_location) THEN
        INSERT INTO csm_dimm_history(
            history_time,
            serial_number,
            node_name,
            size,
            physical_location,
            operation)
        VALUES(
            now(),
            NEW.serial_number,
            NEW.node_name,
            NEW.size,
            NEW.physical_location,
            'U');
        END IF;
        RETURN NEW;
     ELSEIF (TG_OP = 'INSERT') THEN
        INSERT INTO csm_dimm_history(
            history_time,
            serial_number,
            node_name,
            size,
            physical_location,
            operation)
        VALUES(
            now(),
            NEW.serial_number,
            NEW.node_name,
            NEW.size,
            NEW.physical_location,
            'I');
        RETURN NEW;
    END IF;
-- RETURN NULL;
END;
$$
LANGUAGE 'plpgsql';

---------------------------------------------------------------------------------------------------
-- csm_dimm_history_dump trigger to amend summarized column(s) on UPDATE and DELETE.
---------------------------------------------------------------------------------------------------

CREATE TRIGGER tr_csm_dimm_history_dump
    BEFORE INSERT OR DELETE OR UPDATE
    ON csm_dimm
    FOR EACH ROW
    EXECUTE PROCEDURE fn_csm_dimm_history_dump()
;

-----------------------------------------------------------
-- csm_dimm_history_dump_trigger_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_dimm_history_dump() is 'csm_dimm function to amend summarized column(s) on UPDATE and DELETE.';
COMMENT ON TRIGGER tr_csm_dimm_history_dump ON csm_dimm is 'csm_dimm trigger to amend summarized column(s) on UPDATE and DELETE.';

-----------------------------------------------------------------------------------------------
-- fn_csm_switch_inventory_collection function to INSERT and UPDATE switch inventory.
-----------------------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION fn_csm_switch_inventory_collection(
        IN i_record_count            int,
        IN i_switch_name             text[],
        IN i_serial_number           text[],
        IN i_comment                 text[],
        IN i_description             text[],
        IN i_fw_version              text[],
        IN i_gu_id                   text[],
        IN i_has_ufm_agent           boolean[],
        IN i_hw_version              text[],
        IN i_ip                      text[],
        IN i_model                   text[],
        IN i_num_modules             int[],
        IN i_physical_frame_location text[],
        IN i_physical_u_location     text[],
        IN i_ps_id                   text[],
        IN i_role                    text[],
        IN i_server_operation_mode   text[],
        IN i_sm_mode                 text[],
        IN i_state                   text[],
        IN i_sw_version              text[],
        IN i_system_guid             text[],
        IN i_system_name             text[],
        IN i_total_alarms            int[],
        IN i_type                    text[],
        IN i_vendor                  text[],
        OUT o_insert_count int,
        OUT o_update_count int,
        OUT o_delete_count int,
        OUT o_delete_module_count int
)
RETURNS record AS $$
DECLARE
    guids text[];
BEGIN
    o_insert_count := 0;
    o_update_count := 0;
    o_delete_count := 0;
    o_delete_module_count := 0;
    FOR i IN 1..i_record_count LOOP
        IF EXISTS (SELECT switch_name FROM csm_switch WHERE switch_name = i_switch_name[i]) THEN
            UPDATE csm_switch
            SET
                serial_number = i_serial_number[i], 
                collection_time = now(),
                -- Don't update the discovery_time
                comment = i_comment[i],
                description = i_description[i],
                fw_version = i_fw_version[i],
                gu_id = i_gu_id[i],
                has_ufm_agent = i_has_ufm_agent[i],
                hw_version = i_hw_version[i],
                ip = i_ip[i],
                model = i_model[i],
                num_modules = i_num_modules[i],
                -- Don't update the locations. The sys admin will be pissed if these reset every time inventory is collected.
                ps_id = i_ps_id[i],
                role = i_role[i],
                server_operation_mode = i_server_operation_mode[i],
                sm_mode = i_sm_mode[i],
                state = i_state[i],
                sw_version = i_sw_version[i],
                system_guid = i_system_guid[i],
                system_name = i_system_name[i],
                total_alarms = i_total_alarms[i],
                type = i_type[i],
                vendor = i_vendor[i]
            WHERE
                switch_name = i_switch_name[i];
            o_update_count := o_update_count + 1;
        ELSE
            INSERT INTO csm_switch
                (switch_name     , serial_number     , discovery_time, collection_time, comment     , description     , fw_version     , gu_id     , has_ufm_agent     , hw_version     , ip     , model     , num_modules     , physical_frame_location     , physical_u_location     , ps_id     , role     , server_operation_mode     , sm_mode     , state     , sw_version     , system_guid     , system_name     , total_alarms     , type     , vendor     ) VALUES
                (i_switch_name[i], i_serial_number[i], now()         , now()          , i_comment[i], i_description[i], i_fw_version[i], i_gu_id[i], i_has_ufm_agent[i], i_hw_version[i], i_ip[i], i_model[i], i_num_modules[i], i_physical_frame_location[i], i_physical_u_location[i], i_ps_id[i], i_role[i], i_server_operation_mode[i], i_sm_mode[i], i_state[i], i_sw_version[i], i_system_guid[i], i_system_name[i], i_total_alarms[i], i_type[i], i_vendor[i]);
            o_insert_count := o_insert_count + 1;
        END IF;
    END LOOP;
    -- Remove old records.
    -- Collect a list of switches that are old
    SELECT array_agg(gu_id) INTO guids FROM csm_switch WHERE collection_time < now();
    -- Set return data.
    SELECT count(switch_name) INTO o_delete_count FROM csm_switch WHERE collection_time < now();
    -- Set return data.
    SELECT count(name) INTO o_delete_module_count FROM csm_switch_inventory WHERE host_system_guid = ANY(guids);
    -- delete the children of these switches
    DELETE FROM csm_switch_inventory WHERE host_system_guid = ANY(guids);
    -- delete the switches in the list
    DELETE FROM csm_switch WHERE gu_id = ANY(guids);
END;
$$ LANGUAGE 'plpgsql';

-----------------------------------------------------------
-- csm_switch_inventory_collection_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_switch_inventory_collection(int, text[], text[], text[], text[], text[], text[], boolean[], text[], text[], text[], int[], text[], text[], text[], text[], text[], text[], text[], text[], text[], text[], int[], text[], text[]) is 'function to INSERT and UPDATE switch inventory.';

-----------------------------------------------------------------------------------------------
-- fn_csm_switch_children_inventory_collection function to INSERT and UPDATE switch inventory.
-----------------------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION fn_csm_switch_children_inventory_collection(
        IN i_record_count     int,
        IN i_name             text[],
        IN i_host_system_guid text[],
        IN i_comment          text[],
        IN i_description      text[],
        IN i_device_name      text[],
        IN i_device_type      text[],
        IN i_max_ib_ports     int[],
        IN i_module_index     int[],
        IN i_number_of_chips  int[],
        IN i_path             text[],
        IN i_serial_number    text[],
        IN i_severity         text[],
        IN i_status           text[],
        OUT o_insert_count int,
        OUT o_update_count int,
        OUT o_delete_count int
)
RETURNS record AS $$
BEGIN
    o_insert_count := 0;
    o_update_count := 0;
    o_delete_count := 0;
    FOR i IN 1..i_record_count LOOP
        IF EXISTS (SELECT name FROM csm_switch_inventory WHERE name = i_name[i]) THEN
            UPDATE csm_switch_inventory
            SET 
                host_system_guid = i_host_system_guid[i], 
                collection_time  = now(),
                -- Don't update the discovery_time
                comment          = i_comment[i],
                description      = i_description[i], 
                device_name      = i_device_name[i], 
                device_type      = i_device_type[i], 
                max_ib_ports     = i_max_ib_ports[i], 
                module_index     = i_module_index[i], 
                number_of_chips  = i_number_of_chips[i], 
                path             = i_path[i], 
                serial_number    = i_serial_number[i], 
                severity         = i_severity[i], 
                status           = i_status[i] 
            WHERE 
                name = i_name[i];
            o_update_count := o_update_count + 1;
        ELSE 
            INSERT INTO csm_switch_inventory 
            (name     , host_system_guid     , discovery_time, collection_time, comment     , description     , device_name     , device_type     , max_ib_ports     , module_index     , number_of_chips     , path     , serial_number     , severity     , status     ) VALUES
            (i_name[i], i_host_system_guid[i], now()         , now()          , i_comment[i], i_description[i], i_device_name[i], i_device_type[i], i_max_ib_ports[i], i_module_index[i], i_number_of_chips[i], i_path[i], i_serial_number[i], i_severity[i], i_status[i]);
            o_insert_count := o_insert_count + 1;
        END IF;
    END LOOP;
    -- Set return data.
    SELECT count(name) INTO o_delete_count FROM csm_switch_inventory WHERE collection_time < now();
    -- Remove old records. 
    DELETE FROM csm_switch_inventory WHERE collection_time < now();
END;
$$ LANGUAGE 'plpgsql';

-----------------------------------------------------------
-- fn_csm_switch_children_inventory_collection comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_switch_children_inventory_collection(int, text[], text[], text[], text[], text[], text[], int[], int[], int[], text[], text[], text[], text[]) is 'function to INSERT and UPDATE switch children inventory.';


-----------------------------------------------------------------------------------------------
-- csm_switch_history_dump function to amend summarized column(s) on UPDATE and DELETE.
-----------------------------------------------------------------------------------------------
-- Normal Default
--      1.) Update/delete a record
--          Do: push into history
-- Exceptions:
--      1.) Entering a record which is the same (no changes)
--          Do: Update collection time of active record
--      2.) API Update fields are the same
--          Do: Nothing
-----------------------------------------------------------------------------------------------

CREATE FUNCTION fn_csm_switch_history_dump()
     RETURNS trigger AS
     $$
     BEGIN
     IF (TG_OP = 'DELETE') THEN
        INSERT INTO csm_switch_history(
            history_time,
            switch_name,
            serial_number,
            discovery_time,
            collection_time,
            comment,
            description,
            fw_version,
            gu_id,
            has_ufm_agent,
            hw_version,
            ip,
            model,
            num_modules,
            physical_frame_location,
            physical_u_location,
            ps_id,
            role,
            server_operation_mode,
            sm_mode,
            state,
            sw_version,
            system_guid,
            system_name,
            total_alarms,
            type,
            vendor,
            operation)
        VALUES(
            now(),
            OLD.switch_name,
            OLD.serial_number,
            OLD.discovery_time,
            OLD.collection_time,
            OLD.comment,
            OLD.description,
            OLD.fw_version,
            OLD.gu_id,
            OLD.has_ufm_agent,
            OLD.hw_version,
            OLD.ip,
            OLD.model,
            OLD.num_modules,
            OLD.physical_frame_location,
            OLD.physical_u_location,
            OLD.ps_id,
            OLD.role,
            OLD.server_operation_mode,
            OLD.sm_mode,
            OLD.state,
            OLD.sw_version,
            OLD.system_guid,
            OLD.system_name,
            OLD.total_alarms,
            OLD.type,
            OLD.vendor,
            'D');
        RETURN OLD;
     ELSEIF (TG_OP = 'UPDATE') THEN
        IF  (
            (OLD.serial_number           = NEW.serial_number OR OLD.serial_number IS NULL) AND
            (OLD.comment                 = NEW.comment OR OLD.comment IS NULL) AND
            (OLD.description             = NEW.description OR OLD.description IS NULL) AND
            (OLD.fw_version              = NEW.fw_version OR OLD.fw_version IS NULL) AND
            (OLD.gu_id                   = NEW.gu_id OR OLD.gu_id IS NULL) AND
            (OLD.has_ufm_agent           = NEW.has_ufm_agent OR OLD.has_ufm_agent IS NULL) AND
            (OLD.hw_version              = NEW.hw_version OR OLD.hw_version IS NULL) AND
            (OLD.ip                      = NEW.ip OR OLD.ip IS NULL) AND
            (OLD.model                   = NEW.model OR OLD.model IS NULL) AND
            (OLD.num_modules             = NEW.num_modules OR OLD.num_modules IS NULL) AND
            (OLD.physical_frame_location = NEW.physical_frame_location OR OLD.physical_frame_location IS NULL) AND
            (OLD.physical_u_location     = NEW.physical_u_location OR OLD.physical_u_location IS NULL) AND
            (OLD.ps_id                   = NEW.ps_id OR OLD.ps_id IS NULL) AND
            (OLD.role                    = NEW.role OR OLD.role IS NULL) AND
            (OLD.server_operation_mode   = NEW.server_operation_mode OR OLD.server_operation_mode IS NULL) AND
            (OLD.sm_mode                 = NEW.sm_mode OR OLD.sm_mode IS NULL) AND
            (OLD.state                   = NEW.state OR OLD.state IS NULL) AND
            (OLD.sw_version              = NEW.sw_version OR OLD.sw_version IS NULL) AND
            (OLD.system_guid             = NEW.system_guid OR OLD.system_guid IS NULL) AND
            (OLD.system_name             = NEW.system_name OR OLD.system_name IS NULL) AND
            (OLD.total_alarms            = NEW.total_alarms OR OLD.total_alarms IS NULL) AND
            (OLD.type                    = NEW.type OR OLD.type IS NULL) AND
            (OLD.vendor                  = NEW.vendor OR OLD.vendor IS NULL)) THEN
            OLD.collection_time = now();
        ELSIF(
            OLD.comment                 <> NEW.comment OR
            OLD.physical_frame_location <> NEW.physical_frame_location OR
            OLD.physical_u_location     <> NEW.physical_u_location) THEN
        INSERT INTO csm_switch_history(
            history_time,
            switch_name,
            serial_number,
            discovery_time,
            collection_time,
            comment,
            description,
            fw_version,
            gu_id,
            has_ufm_agent,
            hw_version,
            ip,
            model,
            num_modules,
            physical_frame_location,
            physical_u_location,
            ps_id,
            role,
            server_operation_mode,
            sm_mode,
            state,
            sw_version,
            system_guid,
            system_name,
            total_alarms,
            type,
            vendor,
            operation)
        VALUES(
            now(),
            NEW.switch_name,
            NEW.serial_number,
            NEW.discovery_time,
            NEW.collection_time,
            NEW.comment,
            NEW.description,
            NEW.fw_version,
            NEW.gu_id,  
            NEW.has_ufm_agent,
            NEW.hw_version,
            NEW.ip,
            NEW.model,
            NEW.num_modules,
            NEW.physical_frame_location,
            NEW.physical_u_location,
            NEW.ps_id,
            NEW.role,
            NEW.server_operation_mode,
            NEW.sm_mode,
            NEW.state,
            NEW.sw_version,
            NEW.system_guid,
            NEW.system_name,
            NEW.total_alarms,
            NEW.type,
            NEW.vendor,
            'U');
        ELSE
        INSERT INTO csm_switch_history(
            history_time,
            switch_name,
            serial_number,
            discovery_time,
            collection_time,
            comment,
            description,
            fw_version,
            gu_id,
            has_ufm_agent,
            hw_version,
            ip,
            model,
            num_modules,
            physical_frame_location,
            physical_u_location,
            ps_id,
            role,
            server_operation_mode,
            sm_mode,
            state,
            sw_version,
            system_guid,
            system_name,
            total_alarms,
            type,
            vendor,
            operation)
        VALUES(
            now(),
            NEW.switch_name,
            NEW.serial_number,
            NEW.discovery_time,
            NEW.collection_time,
            NEW.comment,
            NEW.description,
            NEW.fw_version,
            NEW.gu_id,
            NEW.has_ufm_agent,
            NEW.hw_version,
            NEW.ip,
            NEW.model,
            NEW.num_modules,
            NEW.physical_frame_location,
            NEW.physical_u_location,
            NEW.ps_id,
            NEW.role,
            NEW.server_operation_mode,
            NEW.sm_mode,
            NEW.state,
            NEW.sw_version,
            NEW.system_guid,
            NEW.system_name,
            NEW.total_alarms,
            NEW.type,
            NEW.vendor,
            'U');
        RETURN NEW;
        END IF;
     ELSEIF (TG_OP = 'INSERT') THEN
        INSERT INTO csm_switch_history(
            history_time,
            switch_name,
            serial_number,
            discovery_time,
            collection_time,
            comment,
            description,
            fw_version,
            gu_id,
            has_ufm_agent,
            hw_version,
            ip,
            model,
            num_modules,
            physical_frame_location,
            physical_u_location,
            ps_id,
            role,
            server_operation_mode,
            sm_mode,
            state,
            sw_version,
            system_guid,
            system_name,
            total_alarms,
            type,
            vendor,
            operation)
        VALUES(
            now(),
            NEW.switch_name,
            NEW.serial_number,
            NEW.discovery_time,
            NEW.collection_time,
            NEW.comment,
            NEW.description,
            NEW.fw_version,
            NEW.gu_id,
            NEW.has_ufm_agent,
            NEW.hw_version,
            NEW.ip,
            NEW.model,
            NEW.num_modules,
            NEW.physical_frame_location,
            NEW.physical_u_location,
            NEW.ps_id,
            NEW.role,
            NEW.server_operation_mode,
            NEW.sm_mode,
            NEW.state,
            NEW.sw_version,
            NEW.system_guid,
            NEW.system_name,
            NEW.total_alarms,
            NEW.type,
            NEW.vendor,
            'I');
        RETURN NEW;
    END IF;
-- RETURN NULL;
    RETURN NEW;
END;
$$
LANGUAGE 'plpgsql';

---------------------------------------------------------------------------------------------------
-- csm_switch_history_dump trigger to amend summarized column(s) on UPDATE and DELETE.
---------------------------------------------------------------------------------------------------

CREATE TRIGGER tr_csm_switch_history_dump
    BEFORE INSERT OR UPDATE OR DELETE
    ON csm_switch
    FOR EACH ROW
    EXECUTE PROCEDURE fn_csm_switch_history_dump()
;

-----------------------------------------------------------
-- csm_switch_history_dump_trigger_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_switch_history_dump() is 'csm_switch function to amend summarized column(s) on UPDATE and DELETE.';
COMMENT ON TRIGGER tr_csm_switch_history_dump ON csm_switch is 'csm_switch trigger to amend summarized column(s) on UPDATE and DELETE.';

---------------------------------------------------------------------------------------------------
-- switch_details type to HELP fn_csm_switch_attributes_query_details
---------------------------------------------------------------------------------------------------

CREATE TYPE switch_details as (
switch_name                         text,
switch_serial_number                text,
switch_discovery_time               timestamp,
switch_collection_time              timestamp,
switch_comment                      text,
switch_description                  text,
switch_fw_version                   text,
switch_gu_id                        text,
switch_has_ufm_agent                boolean,
switch_hw_version                   text,
switch_ip                           text,
switch_model                        text,
switch_num_modules                  int,   
switch_physical_frame_location      text,
switch_physical_u_location          text,
switch_ps_id                        text,
switch_role                         text,
switch_server_operation_mode        text,
switch_sm_mode                      text,
switch_state                        text,
switch_sw_version                   text,
switch_system_guid                  text,
switch_system_name                  text,
switch_total_alarms                 int,
switch_type                         text,
switch_vendor                       text,
switch_inventory_count              int,
switch_inventory_name               text[],
switch_inventory_host_system_guid   text[],
switch_inventory_discovery_time     text[],
switch_inventory_collection_time    text[],
switch_inventory_comment            text[],
switch_inventory_description        text[],
switch_inventory_device_name        text[],
switch_inventory_device_type        text[],
switch_inventory_max_ib_ports       int[],
switch_inventory_module_index       int[],
switch_inventory_number_of_chips    int[],
switch_inventory_path               text[],
switch_inventory_serial_number      text[],
switch_inventory_severity           text[],
switch_inventory_status             text[]
);

-----------------------------------------------------------
-- switch_details type_comments
-----------------------------------------------------------

COMMENT ON TYPE switch_details IS 'switch_details type to help fn_csm_switch_attributes_query_details(text)';

---------------------------------------------------------------------------------------------------
-- csm_switch_attributes_query_details function to HELP CSM API
---------------------------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION fn_csm_switch_attributes_query_details(
    IN i_switch_name text
)
RETURNS switch_details AS $r$
DECLARE
    r switch_details%rowtype;
BEGIN
    --SWITCH--
    SELECT 
        s.switch_name, s.serial_number       , s.discovery_time       , s.collection_time       , s.comment       , s.description       , s.fw_version       , s.gu_id       , s.has_ufm_agent       , s.hw_version       , s.ip       , s.model       , s.num_modules       , s.physical_frame_location       , s.physical_u_location       , s.ps_id       , s.role       , s.server_operation_mode       , s.sm_mode       , s.state       , s.sw_version       , s.system_guid       , s.system_name       , s.total_alarms       , s.type       , s.vendor 
    INTO 
        r.switch_name, r.switch_serial_number, r.switch_discovery_time, r.switch_collection_time, r.switch_comment, r.switch_description, r.switch_fw_version, r.switch_gu_id, r.switch_has_ufm_agent, r.switch_hw_version, r.switch_ip, r.switch_model, r.switch_num_modules, r.switch_physical_frame_location, r.switch_physical_u_location, r.switch_ps_id, r.switch_role, r.switch_server_operation_mode, r.switch_sm_mode, r.switch_state, r.switch_sw_version, r.switch_system_guid, r.switch_system_name, r.switch_total_alarms, r.switch_type, r.switch_vendor 
    FROM csm_switch AS s
        WHERE ( s.switch_name = i_switch_name )
    ;
    --SWITCH_INVENTORY--
    SELECT 
        COUNT(DISTINCT si.name) , array_agg(si.name)     , array_agg(si.host_system_guid)     , array_agg(si.discovery_time)     , array_agg(si.collection_time)     , array_agg(si.comment)     , array_agg(si.description)     , array_agg(si.device_name)     , array_agg(si.device_type)     , array_agg(si.max_ib_ports)     , array_agg(si.module_index)     , array_agg(si.number_of_chips)     , array_agg(si.path)     , array_agg(si.serial_number)     , array_agg(si.severity)     , array_agg(si.status)      
    INTO                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         
        r.switch_inventory_count, r.switch_inventory_name, r.switch_inventory_host_system_guid, r.switch_inventory_discovery_time, r.switch_inventory_collection_time, r.switch_inventory_comment, r.switch_inventory_description, r.switch_inventory_device_name, r.switch_inventory_device_type, r.switch_inventory_max_ib_ports, r.switch_inventory_module_index, r.switch_inventory_number_of_chips, r.switch_inventory_path, r.switch_inventory_serial_number, r.switch_inventory_severity, r.switch_inventory_status 
    FROM 
        csm_switch_inventory AS si
    WHERE ( si.host_system_guid = i_switch_name )
    ; 
    
    return r;
    
END;
$r$ LANGUAGE 'plpgsql';

-----------------------------------------------------------
-- fn_csm_switch_attributes_query_details function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_switch_attributes_query_details(text) is 'csm_switch_attributes_query_details function to HELP CSM API.';

-----------------------------------------------------------------------------------------------
-- fn_csm_ib_cable_inventory_collection function to INSERT and UPDATE ib_cable inventory.
-----------------------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION fn_csm_ib_cable_inventory_collection(
    IN i_record_count  int,
    IN i_serial_number text[],
    IN i_comment       text[],
    IN i_guid_s1       text[],
    IN i_guid_s2       text[],
    IN i_identifier    text[],
    IN i_length        text[],
    IN i_name          text[],
    IN i_part_number   text[],
    IN i_port_s1       text[],
    IN i_port_s2       text[],
    IN i_revision      text[],
    IN i_severity      text[],
    IN i_type          text[],
    IN i_width         text[],
    OUT o_insert_count int,
    OUT o_update_count int,
    OUT o_delete_count int
)
RETURNS record AS $$
BEGIN
    o_insert_count := 0;
    o_update_count := 0;
    o_delete_count := 0;
    FOR i IN 1..i_record_count LOOP
        IF EXISTS (SELECT serial_number FROM csm_ib_cable WHERE serial_number = i_serial_number[i]) THEN
            UPDATE csm_ib_cable
            SET
                collection_time = now(),
                comment = i_comment[i],
                guid_s1 = i_guid_s1[i],
                guid_s2 = i_guid_s2[i],
                identifier = i_identifier[i], 
                length = i_length[i],
                name = i_name[i],
                part_number = i_part_number[i],
                port_s1 = i_port_s1[i],
                port_s2 = i_port_s2[i],
                revision = i_revision[i],
                severity = i_severity[i],
                type = i_type[i], 
                width = i_width[i] 
            WHERE
                serial_number = i_serial_number[i];
            o_update_count := o_update_count + 1;
        ELSE
            INSERT INTO csm_ib_cable
            (serial_number     , discovery_time, collection_time, comment     ,  guid_s1     , guid_s2     , identifier     , length     , name     , part_number     , port_s1     , port_s2     , revision     , severity     , type     , width     ) VALUES
            (i_serial_number[i], now()         , now()          , i_comment[i],  i_guid_s1[i], i_guid_s2[i], i_identifier[i], i_length[i], i_name[i], i_part_number[i], i_port_s1[i], i_port_s2[i], i_revision[i], i_severity[i], i_type[i], i_width[i]);
            o_insert_count := o_insert_count + 1;
        END IF;
    END LOOP;

    -- Set return data.
    SELECT count(serial_number) INTO o_delete_count FROM csm_ib_cable WHERE collection_time < now();
    -- Remove old records
    DELETE FROM csm_ib_cable WHERE collection_time < now();

END;
$$ LANGUAGE 'plpgsql';

-----------------------------------------------------------
-- csm_ib_cable_inventory_collection_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_ib_cable_inventory_collection(int, text[], text[], text[], text[], text[], text[], text[], text[], text[], text[], text[], text[], text[], text[]) is 'function to INSERT and UPDATE ib cable inventory.';

-----------------------------------------------------------------------------------------------
-- csm_ib_cable_history_dump function to amend summarized column(s) on UPDATE and DELETE.
-----------------------------------------------------------------------------------------------
-- Normal Default
--      1.) Update/delete a record
--          Do: push into history
-- Exceptions:
--      1.) Entering a record which is the same (no changes)
--          Do: Update collection time of active record
--      2.) API Update fields are the same
--          Do: Nothing
-----------------------------------------------------------------------------------------------

CREATE FUNCTION fn_csm_ib_cable_history_dump()
     RETURNS trigger AS
     $$
     BEGIN
     IF (TG_OP = 'DELETE') THEN
        INSERT INTO csm_ib_cable_history(
            history_time,
            serial_number,
            discovery_time,
            collection_time,
            comment,
            guid_s1,
            guid_s2,
            identifier,
            length,
            name,
            part_number,
            port_s1,
            port_s2,
            revision,
            severity,
            type,
            width,
            operation)
        VALUES(
            now(),
            OLD.serial_number,
            OLD.discovery_time,
            OLD.collection_time,
            OLD.comment,
            OLD.guid_s1,
            OLD.guid_s2,
            OLD.identifier, 
            OLD.length,
            OLD.name, 
            OLD.part_number,
            OLD.port_s1,
            OLD.port_s2,
            OLD.revision,
            OLD.severity,
            OLD.type,
            OLD.width,
            'D');
        RETURN OLD;
     ELSEIF (TG_OP = 'UPDATE') THEN
        IF (
            OLD.comment = NEW.comment AND
            OLD.guid_s1 = NEW.guid_s1 AND
            OLD.guid_s2 = NEW.guid_s2 AND
            OLD.identifier = NEW.identifier AND
            OLD.length = NEW.length AND
            OLD.name = NEW.name AND
            OLD.part_number = NEW.part_number AND
            OLD.port_s1 = NEW.port_s1 AND
            OLD.port_s2 = NEW.port_s2 AND
            OLD.revision = NEW.revision AND
            OLD.severity = NEW.severity AND
            OLD.type = NEW.type AND
            OLD.width = NEW.width) THEN
            OLD.collection_time = now();
        ELSIF(
            OLD.comment <> NEW.comment OR
            OLD.guid_s1 <> NEW.guid_s1 OR
            OLD.guid_s2 <> NEW.guid_s2 OR
            OLD.port_s1 <> NEW.port_s1 OR
            OLD.port_s2 <> NEW.port_s2) THEN
        INSERT INTO csm_ib_cable_history(
            history_time,
            serial_number,
            discovery_time,
            collection_time,
            comment,
            guid_s1,
            guid_s2,
            identifier,
            length,
            name,
            part_number,
            port_s1,
            port_s2,
            revision,
            severity,
            type,
            width,
            operation)
        VALUES(
            now(),
            NEW.serial_number,
            NEW.discovery_time,
            NEW.collection_time,
            NEW.comment,
            NEW.guid_s1,
            NEW.guid_s2,
            NEW.identifier,
            NEW.length,
            NEW.name,
            NEW.part_number,
            NEW.port_s1,
            NEW.port_s2,
            NEW.revision,
            NEW.severity,
            NEW.type,
            NEW.width,
            'U');
        ELSE
        INSERT INTO csm_ib_cable_history(
            history_time,
            serial_number,
            discovery_time,
            collection_time,
            comment,
            guid_s1,
            guid_s2,
            identifier,
            length,
            name,
            part_number,
            port_s1,
            port_s2,
            revision,
            severity,
            type,
            width,
            operation)
        VALUES(
            now(),
            NEW.serial_number,
            NEW.discovery_time,
            NEW.collection_time,
            NEW.comment,
            NEW.guid_s1,
            NEW.guid_s2,
            NEW.identifier, 
            NEW.length,
            NEW.name, 
            NEW.part_number,
            NEW.port_s1,
            NEW.port_s2,
            NEW.revision,
            NEW.severity,
            NEW.type,
            NEW.width,
            'U');
        RETURN NEW;
        END IF;
    ELSEIF (TG_OP = 'INSERT') THEN
        INSERT INTO csm_ib_cable_history(
            history_time,
            serial_number,
            discovery_time,
            collection_time,
            comment,
            guid_s1,
            guid_s2,
            identifier,
            length,
            name,
            part_number,
            port_s1,
            port_s2,
            revision,
            severity,
            type,
            width,
            operation)
        VALUES(
            now(),
            NEW.serial_number,
            NEW.discovery_time,
            NEW.collection_time,
            NEW.comment,
            NEW.guid_s1,
            NEW.guid_s2,
            NEW.identifier, 
            NEW.length,
            NEW.name, 
            NEW.part_number,
            NEW.port_s1,
            NEW.port_s2,
            NEW.revision,
            NEW.severity,
            NEW.type,
            NEW.width,
            'I');
        RETURN NEW;
    END IF;
-- RETURN NULL;
RETURN NEW;
END;
$$
LANGUAGE 'plpgsql';

---------------------------------------------------------------------------------------------------
-- csm_ib_cable_history_dump trigger to amend summarized column(s) on UPDATE and DELETE.
---------------------------------------------------------------------------------------------------

CREATE TRIGGER tr_csm_ib_cable_history_dump
    BEFORE INSERT OR UPDATE OR DELETE
    ON csm_ib_cable
    FOR EACH ROW
    EXECUTE PROCEDURE fn_csm_ib_cable_history_dump()
;

-----------------------------------------------------------
-- csm_ib_cable_history_dump_trigger_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_ib_cable_history_dump() is 'csm_ib_cable function to amend summarized column(s) on UPDATE and DELETE.';
COMMENT ON TRIGGER tr_csm_ib_cable_history_dump ON csm_ib_cable is 'csm_ib_cable trigger to amend summarized column(s) on UPDATE and DELETE.';

-----------------------------------------------------------------------------------------------
-- csm_switch_inventory_history_dump function to amend summarized column(s) on UPDATE and DELETE.
-----------------------------------------------------------------------------------------------
-- Normal Default
--      1.) Update/delete a record
--          Do: push into history
-- Exceptions:
--      1.) Entering a record which is the same (no changes)
--          Do: Update collection time of active record
-----------------------------------------------------------------------------------------------

CREATE FUNCTION fn_csm_switch_inventory_history_dump()
     RETURNS trigger AS
     $$
     BEGIN
     IF (TG_OP = 'DELETE') THEN
        INSERT INTO csm_switch_inventory_history(
            history_time,
            name,
            host_system_guid,
            discovery_time,
            collection_time,
            comment,
            description,
            device_name,
            device_type,
            hw_version,
            max_ib_ports,
            module_index,
            number_of_chips,
            path,
            serial_number,
            severity,
            status,
            operation)            
        VALUES(
            now(),
            OLD.name,
            OLD.host_system_guid,
            OLD.discovery_time,
            OLD.collection_time,
            OLD.comment,
            OLD.description,
            OLD.device_name,
            OLD.device_type,
            OLD.hw_version,
            OLD.max_ib_ports,
            OLD.module_index,
            OLD.number_of_chips,
            OLD.path,
            OLD.serial_number,
            OLD.severity,
            OLD.status,
            'D');
        RETURN OLD;
     ELSEIF (TG_OP = 'UPDATE') THEN
        IF  (
            (OLD.host_system_guid = NEW.host_system_guid OR OLD.host_system_guid IS NULL) AND
            (OLD.comment          = NEW.comment OR OLD.comment IS NULL) AND
            (OLD.description      = NEW.description OR OLD.description IS NULL) AND
            (OLD.device_name      = NEW.device_name OR OLD.device_name IS NULL) AND
            (OLD.device_type      = NEW.device_type OR OLD.device_type IS NULL) AND
            (OLD.hw_version       = NEW.hw_version OR OLD.hw_version IS NULL) AND
            (OLD.max_ib_ports     = NEW.max_ib_ports OR OLD.max_ib_ports IS NULL) AND
            (OLD.module_index     = NEW.module_index OR OLD.module_index IS NULL) AND
            (OLD.number_of_chips  = NEW.number_of_chips OR OLD.number_of_chips IS NULL) AND
            (OLD.path             = NEW.path OR OLD.path IS NULL) AND
            (OLD.serial_number    = NEW.serial_number OR OLD.serial_number IS NULL) AND
            (OLD.severity         = NEW.severity OR OLD.severity IS NULL) AND
            (OLD.status           = NEW.status OR OLD.status IS NULL)) THEN
            OLD.collection_time = now();
        ELSE
        INSERT INTO csm_switch_inventory_history(
            history_time,
            name,
            host_system_guid,
            discovery_time,
            collection_time,
            comment,
            description,
            device_name,
            device_type,
            hw_version,
            max_ib_ports,
            module_index,
            number_of_chips,
            path,
            serial_number,
            severity,
            status,
            operation)            
        VALUES(
            now(),
            NEW.name,
            NEW.host_system_guid,
            NEW.discovery_time,
            NEW.collection_time,
            NEW.comment,
            NEW.description,
            NEW.device_name,
            NEW.device_type,
            NEW.hw_version,
            NEW.max_ib_ports,
            NEW.module_index,
            NEW.number_of_chips,
            NEW.path,
            NEW.serial_number,
            NEW.severity,
            NEW.status,
            'U');
        RETURN NEW;
        END IF;
     ELSEIF (TG_OP = 'INSERT') THEN
        INSERT INTO csm_switch_inventory_history(
            history_time,
            name,
            host_system_guid,
            discovery_time,
            collection_time,
            comment,
            description,
            device_name,
            device_type,
            hw_version,
            max_ib_ports,
            module_index,
            number_of_chips,
            path,
            serial_number,
            severity,
            status,
            operation)            
        VALUES(
            now(),
            NEW.name,
            NEW.host_system_guid,
            NEW.discovery_time,
            NEW.collection_time,
            NEW.comment,
            NEW.description,
            NEW.device_name,
            NEW.device_type,
            NEW.hw_version,
            NEW.max_ib_ports,
            NEW.module_index,
            NEW.number_of_chips,
            NEW.path,
            NEW.serial_number,
            NEW.severity,
            NEW.status,
            'I');
        RETURN NEW;
    END IF;
-- RETURN NULL;
    RETURN NEW;
END;
$$
LANGUAGE 'plpgsql';

---------------------------------------------------------------------------------------------------
-- csm_switch_inventory_history_dump trigger to amend summarized column(s) on UPDATE and DELETE.
---------------------------------------------------------------------------------------------------

CREATE TRIGGER tr_csm_switch_inventory_history_dump
    BEFORE INSERT OR UPDATE OR DELETE
    ON csm_switch_inventory
    FOR EACH ROW
    EXECUTE PROCEDURE fn_csm_switch_inventory_history_dump()
;

-----------------------------------------------------------
-- csm_switch_inventory_history_dump_trigger_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_switch_inventory_history_dump() is 'csm_switch_inventory function to amend summarized column(s) on UPDATE and DELETE.';
COMMENT ON TRIGGER tr_csm_switch_inventory_history_dump ON csm_switch_inventory is 'csm_switch_inventory trigger to amend summarized column(s) on UPDATE and DELETE.';

-----------------------------------------------------------------------------------------------
-- csm_config_history_dump function to amend summarized column(s) on UPDATE and DELETE.
-----------------------------------------------------------------------------------------------

CREATE FUNCTION fn_csm_config_history_dump()
     RETURNS trigger AS
     $$
     BEGIN
     IF (TG_OP = 'DELETE') THEN
        INSERT INTO csm_config_history(
            history_time,
            csm_config_id,
            local_socket,
            mqtt_broker,
            log_level,
            buckets,
            jitter_window_interval,
            jitter_window_duration,
            path_certificate,
            path_log,
            create_time)
        VALUES(
            now(),
            OLD.csm_config_id,
            OLD.local_socket,
            OLD.mqtt_broker,
            OLD.log_level,
            OLD.buckets,
            OLD.jitter_window_interval,
            OLD.jitter_window_duration,
            OLD.path_certificate,
            OLD.path_log,
            OLD.create_time);
        RETURN OLD;
     ELSEIF (TG_OP = 'UPDATE') THEN
        INSERT INTO csm_config_history(
            history_time,
            csm_config_id,
            local_socket,
            mqtt_broker,
            log_level,
            buckets,
            jitter_window_interval,
            jitter_window_duration,
            path_certificate,
            path_log,
            create_time)
        VALUES(
            now(),
            NEW.csm_config_id,
            NEW.local_socket,
            NEW.mqtt_broker,
            NEW.log_level,
            NEW.buckets,
            NEW.jitter_window_interval,
            NEW.jitter_window_duration,
            NEW.path_certificate,
            NEW.path_log,
            NEW.create_time);
        RETURN NEW;
    END IF;
-- RETURN NULL;
END;
$$
LANGUAGE 'plpgsql';

---------------------------------------------------------------------------------------------------
-- csm_config_history_dump trigger to amend summarized column(s) on UPDATE and DELETE.
---------------------------------------------------------------------------------------------------

CREATE TRIGGER tr_csm_config_history_dump
    BEFORE DELETE OR UPDATE
    ON csm_config
    FOR EACH ROW
    EXECUTE PROCEDURE fn_csm_config_history_dump()
;

-----------------------------------------------------------
-- csm_config_history_dump_trigger_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_config_history_dump() is 'csm_config function to amend summarized column(s) on UPDATE and DELETE.';
COMMENT ON TRIGGER tr_csm_config_history_dump ON csm_config is 'csm_config trigger to amend summarized column(s) on UPDATE and DELETE.';

--------------------------------------------------------------------------------------------------
-- csm_lv_upsert function to amend summarized column(s) on INSERT.
--------------------------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION fn_csm_lv_upsert(
    l_logical_volume_name       text,
    l_node_name                 text,
    l_allocation_id             bigint,
    l_vg_name                   text,
    l_state                     char(1),
    l_current_size              bigint,
    l_max_size                  bigint,
    l_begin_time                timestamp,
    l_updated_time              timestamp,
    l_file_system_mount         text,
    l_file_system_type          text)
  RETURNS void AS
$$
DECLARE
    l "csm_lv"%ROWTYPE;
    v "csm_vg"%ROWTYPE;

BEGIN
     -- first, make sure there are enough resources that can be allocated to the lv being inserted
     IF EXISTS (SELECT
                    l_current_size,
                    csm_vg.available_size
                FROM
                    csm_vg
                WHERE (
                    csm_vg.vg_name = $4 -- l_vg_name
                AND
                    l_current_size > csm_vg.available_size
                AND
                    csm_vg.node_name = l_node_name
                )
                LIMIT 1)
        THEN
        RAISE EXCEPTION using message = 'Available size limit exceeded';
    ELSE
        INSERT INTO csm_lv(
            logical_volume_name,
            node_name,
            allocation_id,
            vg_name,
            state,
            current_size,
            max_size,
            begin_time,
            updated_time,
            file_system_mount,
            file_system_type)
        VALUES (    
            l_logical_volume_name,
            l_node_name,
            l_allocation_id,
            l_vg_name,
            l_state,
            l_current_size,
            l_max_size,
            l_begin_time,
            l_updated_time,
            l_file_system_mount,
            l_file_system_type);
        INSERT INTO csm_lv_update_history(
            history_time,
            logical_volume_name,
            allocation_id,
            state,
            current_size,
            updated_time,
            operation)
        VALUES (
            now(),
            l_logical_volume_name,
            l_allocation_id,
            l_state,
            l_current_size,
            l_updated_time,
            'I');
    -----------------------------------------------------i----------------------------
    -- Then do the calculations below to set new available_size value in csm_vg table      
    ----------------------------------------------------------------------------------
        UPDATE csm_vg
        SET
            available_size = csm_vg.available_size - l_current_size
        from
            csm_lv
        WHERE (
            csm_vg.vg_name = $4 -- l_vg_name
        AND
            csm_vg.node_name = l_node_name
        );
    -------------------------------------------------------------------------
    -- This handles the update to the max_size if the value being passed
    -------------------------------------------------------------------------
        IF (l_current_size > l_max_size) THEN
        UPDATE csm_lv
        SET
                max_size = current_size
            WHERE (
                max_size <= current_size
            AND
                logical_volume_name = $1 -- l_logical_volume_name
            AND
                node_name = l_node_name
            );
        END IF;
    END IF;
    EXCEPTION
            WHEN others THEN
                RAISE EXCEPTION
                USING ERRCODE = sqlstate,
                      MESSAGE = 'error_handling_test: ' || sqlstate || '/' || sqlerrm;
END;
$$
LANGUAGE plpgsql;

-----------------------------------------------------------
-- csm_lv_upsert_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_lv_upsert(
    l_logical_volume_name text, l_node_name text, l_allocation_id bigint, l_vg_name text, l_state char(1), l_current_size bigint, l_max_size bigint, l_begin_time timestamp, l_updated_time timestamp, l_file_system_mount text, l_file_system_type text)
    is 'csm_lv_upsert function to amend summarized column(s) on INSERT. (csm_lv table)';

--------------------------------------------------------------------------------------------------
-- csm_lv_history_dump function to amend summarized column(s) on DELETE. (csm_step_history_dump)
--------------------------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION fn_csm_lv_history_dump(
    i_logical_volume_name text,
    i_node_name           text,
    i_allocationid        bigint,
    i_updated_time        timestamp,
    i_end_time            timestamp,
    i_num_bytes_read      bigint,
    i_num_bytes_written   bigint,
    i_num_reads           bigint,
    i_num_writes          bigint)
RETURNS void AS
$$
DECLARE
    l "csm_lv"%ROWTYPE;
    lv_current_size_before_delete bigint;
    vg_name_of_the_lv text;
BEGIN
    lv_current_size_before_delete := 0; 
    IF EXISTS (SELECT logical_volume_name, node_name, allocation_id 
                FROM csm_lv 
                WHERE (
                    logical_volume_name = i_logical_volume_name AND 
                    node_name = i_node_name AND 
                    allocation_id = i_allocationid
                )
                LIMIT 1) 
    THEN
        SELECT current_size INTO lv_current_size_before_delete FROM csm_lv WHERE (logical_volume_name = i_logical_volume_name AND node_name = i_node_name AND allocation_id = i_allocationid);
        SELECT vg_name INTO vg_name_of_the_lv FROM csm_lv WHERE (logical_volume_name = i_logical_volume_name AND node_name = i_node_name AND allocation_id = i_allocationid);
        SELECT * INTO l FROM csm_lv WHERE logical_volume_name = i_logical_volume_name AND allocation_id = i_allocationid AND node_name = i_node_name;
        -- copy the 'active' record into history, and fill in new post job data.
        INSERT INTO csm_lv_history VALUES(
            now(),                 -- history time
            i_logical_volume_name, -- logical_volume_name
            i_node_name,           -- node_name
            i_allocationid,        -- allocation_id
            l.vg_name,             -- vg_name
            'R',                   -- state, NOTE: Manually set to 'R' for remove
            0,                     -- current_size, NOTE: Manually set to zero at this point.
            l.max_size,            -- max_size
            l.begin_time,          -- begin_time
            i_updated_time,        -- updated_time
            i_end_time,            -- end_time
            l.file_system_mount,   -- file_system_mount
            l.file_system_type,    -- file_system_mount
            i_num_bytes_read,      -- num_bytes_read
            i_num_bytes_written,   -- num_bytes_written
            'D',                   -- operation, NOTE: Manually set to 'D' for delete
            NULL,                  -- archive_history_time
            i_num_reads,           -- num_reads
            i_num_writes           -- num_writes
        );
        -- copy into the transaction history
        INSERT INTO csm_lv_update_history VALUES(
            now(),
            i_logical_volume_name,
            i_allocationid,
            'R',
            0,
            i_updated_time,
            'D');
        -- then do the calculations below to set new available_size value
        UPDATE csm_vg
        SET
            available_size = lv_current_size_before_delete + available_size
        WHERE (
                vg_name_of_the_lv = csm_vg.vg_name
            AND
                i_node_name = csm_vg.node_name
        ); 
        -- Clean up the old 'active' record from 'csm_lv'
        DELETE 
            FROM csm_lv 
            WHERE (
                logical_volume_name = i_logical_volume_name AND 
                node_name = i_node_name AND 
                allocation_id = i_allocationid
            );
    ELSE
        RAISE EXCEPTION 'logical_volume_name and allocation_id does not exist % %', i_logical_volume_name, i_allocationid;
    END IF;
    EXCEPTION
        WHEN others THEN
            RAISE EXCEPTION
            USING ERRCODE = sqlstate,
                MESSAGE = 'error_handling_test: ' || sqlstate || '/' || sqlerrm;
--    RETURN NULL;
END;
$$ LANGUAGE plpgsql;

-----------------------------------------------------------
-- csm_lv_history_dump_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_lv_history_dump(text, text, bigint, timestamp, timestamp, bigint, bigint, bigint, bigint)
    is 'csm_lv function to amend summarized column(s) on DELETE. (csm_lv_history_dump)';

-----------------------------------------------------------------------------------------------
-- csm_lv_modified_history_dump function to amend summarized column(s) on UPDATE.
-----------------------------------------------------------------------------------------------

CREATE FUNCTION fn_csm_lv_modified_history_dump()
    RETURNS trigger AS
    $$
    BEGIN
    IF (TG_OP = 'UPDATE') THEN
        IF (
            OLD.logical_volume_name IS DISTINCT FROM NEW.logical_volume_name OR
            OLD.node_name IS DISTINCT FROM NEW.node_name OR
            OLD.allocation_id IS DISTINCT FROM NEW.allocation_id OR
            OLD.vg_name IS DISTINCT FROM NEW.vg_name OR
            OLD.state IS DISTINCT FROM NEW.state OR
            OLD.current_size IS DISTINCT FROM NEW.current_size OR
            OLD.max_size IS DISTINCT FROM NEW.max_size OR
            OLD.file_system_mount IS DISTINCT FROM NEW.file_system_mount OR
            OLD.file_system_type IS DISTINCT FROM NEW.file_system_type) THEN
        INSERT INTO csm_lv_history(
            history_time,
            logical_volume_name,
            node_name,
            allocation_id,
            vg_name,
            state,
            current_size,
            max_size,
            begin_time,
            updated_time,
            file_system_mount,
            file_system_type,
            operation)
        VALUES(
            now(),
            NEW.logical_volume_name,
            NEW.node_name,
            NEW.allocation_id,
            NEW.vg_name,
            NEW.state,
            NEW.current_size,
            NEW.max_size,
            NEW.begin_time,
            NEW.updated_time,
            NEW.file_system_mount,
            NEW.file_system_type,
            'U');
        END IF;
            RETURN NEW;
    END IF;
END;
$$
LANGUAGE plpgsql;

---------------------------------------------------------------------------------------------------
-- csm_lv_modified_history_dump trigger to amend summarized column(s) on UPDATE.
---------------------------------------------------------------------------------------------------

--CREATE TRIGGER tr_csm_lv_modified_history_dump
--    BEFORE UPDATE OF logical_volume_name,node_name,allocation_id,vg_name,max_size,begin_time,file_system_mount,file_system_type
--    ON csm_lv
--    FOR EACH ROW
--    EXECUTE PROCEDURE fn_csm_lv_modified_history_dump()
--;

-----------------------------------------------------------
-- csm_lv_modified_history_dump_trigger_function_comments
-----------------------------------------------------------

--COMMENT ON FUNCTION fn_csm_lv_modified_history_dump() is 'csm_lv_modified_history_dump function to amend summarized column(s) on UPDATE.';
--COMMENT ON TRIGGER tr_csm_lv_modified_history_dump ON csm_lv is 'csm_lv_modified_history_dump trigger to amend summarized column(s) on UPDATE.';

-----------------------------------------------------------------------------------------------
-- csm_lv_update_history_dump function to amend summarized column(s) on UPDATE.
-----------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------
-- 1. First, make sure there are enough resources that can be allocated to the lv being inserted
-- 2. Calculates (+/-) and produces the updated available size in the csm_vg table
-- 3. Also sets the watermark value of the highest current_size value passed in
-- 4. If the updated value of the current_size is lower then the (higher) max_size remains
-----------------------------------------------------------------------------------------------

CREATE FUNCTION fn_csm_lv_update_history_dump()
    RETURNS trigger AS
    $$
    DECLARE
        delta_new_available_size bigint;
    BEGIN
        -- This sets the delta (the difference between new and old current_size)
        delta_new_available_size := NEW.current_size - OLD.current_size;
        -- This checks to make sure that the new current_size doesn't exceed available resources available.
         IF (SELECT
                NEW.current_size > (
                    SELECT
                        csm_vg.available_size
                    FROM
                        csm_vg,
                        csm_lv
                    WHERE (
                        csm_vg.available_size < delta_new_available_size 
                    AND csm_vg.vg_name = NEW.vg_name 
                    AND csm_vg.node_name = NEW.node_name
                    )
                    LIMIT 1)) THEN
                    -- This handles the error handling message.
                    RAISE EXCEPTION using message = 'Available size limit exceeded';
        ELSE
         -- An update will then take place if the criteria is met successfully.
            UPDATE csm_vg
            SET
                available_size = csm_vg.available_size - delta_new_available_size
            WHERE (
                csm_vg.vg_name = NEW.vg_name 
            AND
                csm_vg.node_name = NEW.node_name
            );
            END IF; 
     -- This section will then update the watermark value if the current_size is greater than max_size.
    IF OLD.max_size <= NEW.current_size THEN
        NEW.max_size = NEW.current_size;
    END IF;
    IF (TG_OP = 'UPDATE') THEN
                NEW.updated_time = now();
                IF (
                    OLD.logical_volume_name IS DISTINCT FROM NEW.logical_volume_name OR
                    OLD.allocation_id IS DISTINCT FROM NEW.allocation_id OR
                    OLD.state IS DISTINCT FROM NEW.state OR
                    OLD.current_size IS DISTINCT FROM NEW.current_size) THEN
                INSERT INTO csm_lv_update_history(
                    history_time,
                    logical_volume_name,
                    allocation_id,
                    state,
                    current_size,
                    updated_time,
                    operation)
                VALUES(
                    now(),
                    NEW.logical_volume_name,
                    NEW.allocation_id,
                    NEW.state,
                    NEW.current_size,
                    NEW.updated_time,
                    'U');
                END IF;
                RETURN NEW;
    END IF;
RETURN NEW;
RETURN NEW;
END;
$$
LANGUAGE plpgsql;

---------------------------------------------------------------------------------------------------
-- csm_lv_update_history_dump trigger to amend summarized column(s) on UPDATE and DELETE.
---------------------------------------------------------------------------------------------------

CREATE TRIGGER tr_csm_lv_update_history_dump
    BEFORE UPDATE OF state, current_size, updated_time
    ON csm_lv
    FOR EACH ROW
    EXECUTE PROCEDURE fn_csm_lv_update_history_dump()
;

-----------------------------------------------------------
-- csm_lv_update_history_dump_trigger_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_lv_update_history_dump() is 'csm_lv_update_history_dump function to amend summarized column(s) on UPDATE.';
COMMENT ON TRIGGER tr_csm_lv_update_history_dump ON csm_lv is 'csm_lv_update_history_dump trigger to amend summarized column(s) on UPDATE.';

-----------------------------------------------------------------------------------------------
-- csm_diag_result_history_dump function to amend summarized column(s) on DELETE (within fn_.
-----------------------------------------------------------------------------------------------

CREATE FUNCTION fn_csm_diag_result_history_dump()
RETURNS trigger AS
$$
BEGIN
    IF (TG_OP = 'DELETE') THEN
        INSERT INTO csm_diag_result_history(
            history_time,
            run_id,
            test_name,
            node_name,
            serial_number,
            begin_time,
            end_time,
            status,
            log_file)
        VALUES(
            now(),  
            OLD.run_id,
            OLD.test_name,
            OLD.node_name,
            OLD.serial_number,
            OLD.begin_time,
            OLD.end_time,  
            OLD.status, 
            OLD.log_file);
        RETURN OLD;
    END IF;
END;
$$
LANGUAGE plpgsql;

---------------------------------------------------------------------------------------------------
-- csm_diag_result trigger to amend summarized column(s) on DELETE. (csm_diag_result_history_dump)
---------------------------------------------------------------------------------------------------

CREATE TRIGGER tr_csm_diag_result_history_dump
    BEFORE DELETE
    ON csm_diag_result
    FOR EACH ROW
    EXECUTE PROCEDURE fn_csm_diag_result_history_dump()
;

-----------------------------------------------------------
-- csm_diag_result_history_dump_trigger_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_diag_result_history_dump() is 'csm_diag_result function to amend summarized column(s) on DELETE.';
COMMENT ON TRIGGER tr_csm_diag_result_history_dump ON csm_diag_result is 'csm_diag_result trigger to amend summarized column(s) on DELETE.';


---------------------------------------------------------------------------------------------------
-- csm_diag_run Function to amend summarized column(s) on UPDATE and DELETE. (csm_diag_run_history_dump)
---------------------------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION fn_csm_diag_run_history_dump(
    _run_id bigint,
    _end_time timestamp with time zone,
    _status text,
    _inserted_ras boolean)
RETURNS VOID AS
$$
DECLARE 
   a "csm_diag_run"%ROWTYPE;
   
BEGIN
    IF EXISTS (SELECT run_id FROM csm_diag_run WHERE run_id=$1) THEN
        UPDATE csm_diag_run
            SET
            status = _status,
            inserted_ras = _inserted_ras                
        WHERE   run_id = $1;
        SELECT * INTO a FROM csm_diag_run WHERE run_id=$1;
        INSERT INTO csm_diag_run_history VALUES(
            now(),
            $1,
            a.allocation_id,
            a.begin_time,
            $2,
            $3,
            $4,
            a.log_dir,
            a.cmd_line
        );  
            DELETE FROM csm_diag_result WHERE run_id=$1;
            DELETE FROM csm_diag_run WHERE run_id=$1;
ELSE
        RAISE EXCEPTION using message = 'run_id does not exist';
    END IF;
    EXCEPTION
        WHEN others THEN
            RAISE EXCEPTION
            USING ERRCODE = sqlstate,
                MESSAGE = 'error_handling_test: ' || sqlstate || '/' || sqlerrm;
--  RETURN NULL;
END;
$$ LANGUAGE plpgsql;

-----------------------------------------------------------
-- csm_diag_run_history_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_diag_run_history_dump(_run_id bigint,_end_time timestamp with time zone,_status text,_inserted_ras boolean) is 'csm_diag_run function to amend summarized column(s) on UPDATE and DELETE. (csm_diag_run_history_dump)';

-----------------------------------------------------------------------------------------------
-- csm_db_schema_version_history_dump function to amend summarized column(s) on UPDATE and DELETE.
-----------------------------------------------------------------------------------------------

CREATE FUNCTION fn_csm_db_schema_version_history_dump()
     RETURNS trigger AS
     $$
     BEGIN
     IF (TG_OP = 'DELETE') THEN
        INSERT INTO csm_db_schema_version_history(
            history_time,
            version,
            create_time,
            comment)
        VALUES(
            now(),
            OLD.version,
            OLD.create_time,
            OLD.comment);
        RETURN OLD;
     ELSEIF (TG_OP = 'UPDATE') THEN
        INSERT INTO csm_db_schema_version_history(
            history_time,
            version,
            create_time,
            comment)
        VALUES(
            now(),
            OLD.version,
            OLD.create_time,
            OLD.comment);
        RETURN NEW;
    END IF;
-- RETURN NULL;
END;
$$
LANGUAGE 'plpgsql';

---------------------------------------------------------------------------------------------------
-- csm_db_schema_version_history_dump trigger to amend summarized column(s) on UPDATE and DELETE.
---------------------------------------------------------------------------------------------------

CREATE TRIGGER tr_csm_db_schema_version_history_dump
    BEFORE DELETE OR UPDATE
    ON csm_db_schema_version
    FOR EACH ROW
    EXECUTE PROCEDURE fn_csm_db_schema_version_history_dump()
;

-----------------------------------------------------------
-- csm_db_schema_version_history_dump_trigger_function_comments
-----------------------------------------------------------

COMMENT ON FUNCTION fn_csm_db_schema_version_history_dump() is 'csm_db_schema_version function to amend summarized column(s) on UPDATE and DELETE.';
COMMENT ON TRIGGER tr_csm_db_schema_version_history_dump ON csm_db_schema_version is 'csm_db_schema_version trigger to amend summarized column(s) on UPDATE and DELETE.';

COMMIT;
