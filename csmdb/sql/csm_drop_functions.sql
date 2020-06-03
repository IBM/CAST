--===============================================================================
--
--   csm_drop_functions.sql
--
-- © Copyright IBM Corporation 2015-2020. All Rights Reserved
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
--   usage:             ./csm_db_script.sh <----- -f (force) will drop all functions in DB
--   current_version:   19.0
--   create:            06-13-2016
--   last modified:     06-01-2020
--   change log:
--     19.0  -  Moving this version to sync with DB schema version.
--     18.0  -  Moving this version to sync with DB schema version.
--           -  Added in fn_csm_node_state_history_temp_table
--           -  Updated the fn_csm_switch_attributes_query_details function
--           -  Added 2 new input fields to fn_csm_switch_children_inventory_collection: text[], text[]
--           -  Added new input field to fn_csm_allocation_update_state: OUT o_core_blink boolean
--     17.0  -  Moving this version to sync with DB schema version.
--           -  fn_csm_allocation_update_state - added in:  o_smt_mode smallint 
--           -  fn_csm_lv_history_dump - added in: bigint x2 (num_reads, num_writes)
--     16.2  -  Moving this version to sync with DB schema version.
--     16.1  -  added 'fn_csm_ssd_dead_records'
--           -  added 'fn_csm_allocation_dead_records_on_lv'
--     16.0  -  Moving this version to sync with DB schema version.
--           -  added fields to fn_csm_allocation_history_dump,fn_csm_allocation_create_data_aggregator, and fn_csm_allocation_finish_data_stats 
--     04.28 -  fn_csm_allocation_delete_start and cleaned up some other data types.
--     04.27 -  added fields to fn_csm_step_begin and fn_csm_step_end.
--     04.26 -  added fields to fn_csm_allocation_history_dump,fn_csm_allocation_create_data_aggregator and fn_csm_allocation_finish_data_stats
--     04.25 -  added in fn_csm_allocation_delete_start and cleaned up some other data types
--     04.24 -  updated fn_csm_lv_history_dump function header
--     04.23 -  updated fn_csm_switch_children_inventory_collection function header
--     04.22 -  updated fn_csm_switch_inventory_collection function header
--     04.21 - removed -
--              fn_csm_lv_history_insert.
--              fn_csm_step_history_insert 
--     04.20 -  added - fn_csm_lv_history_insert + fn_csm_step_history_insert
--     04.19 -  removed fn_csm_switch_ports_history_dump
--     04.18 -  updated fn_csm_processor_socket_history_dump + fn_csm_node_state_history
--     04.17 -  updated fn_csm_vg_delete function header
--     04.16 -  created a drop function to handle fn_csm_ssd_wear_history
--     04.15 -  updated fn_csm_vg_create: added in new fields ssd_count, scheduler
--===============================================================================

\set ON_ERROR_STOP on
BEGIN;

------------------------------------------------------------------------------------------------
-- The triggers will be dropped automatically when the automated script is executed.
-- (If the table is dropped, so is the triggers)
------------------------------------------------------------------------------------------------
-- DROP TRIGGER IF EXISTS tr_csm_allocation_history_dump ON csm_allocation;
-- DROP TRIGGER IF EXISTS tr_csm_allocation_node_state_change ON C11;
-- DROP TRIGGER IF EXISTS tr_csm_allocation_state_change ON csm_allocation;
-- DROP TRIGGER IF EXISTS tr_csm_config_history_dump ON csm_config;
-- DROP TRIGGER IF EXISTS tr_csm_dimm_history_dump ON csm_dimm;
-- DROP TRIGGER IF EXISTS tr_csm_gpu_history_dump ON csm_gpu;
-- DROP TRIGGER IF EXISTS tr_csm_hca_history_dump ON csm_hca;
-- DROP TRIGGER IF EXISTS tr_csm_ib_cable_history_dump ON csm_ib_cable;
-- DROP TRIGGER IF EXISTS tr_csm_map_tag_history_dump ON csm_map_tag;
-- DROP TRIGGER IF EXISTS tr_csm_node_ready ON csm_node;
-- DROP TRIGGER IF EXISTS tr_csm_processor_history_dump ON csm_processor;
-- DROP TRIGGER IF EXISTS tr_csm_ras_type_history_dump ON csm_ras_type;
-- DROP TRIGGER IF EXISTS tr_csm_ssd_history_dump ON csm_ssd_history;
-- DROP TRIGGER IF EXISTS tr_csm_step_history_dump ON csm_step;
-- DROP TRIGGER IF EXISTS tr_csm_step_node_history_dump ON csm_step;
-- DROP TRIGGER IF EXISTS tr_csm_switch_history_dump ON csm_switch;
-- DROP TRIGGER IF EXISTS tr_csm_vg_history_dump ON csm_vg;
-- DROP TRIGGER IF EXISTS tr_csm_vg_ssd_history_dump ON csm_vg_ssd;

------------------------------------------------------------------------------------------------
-- The functions will remain even if the tables are dropped.
-- They can be removed by using the autoscript with the -f -e -d commands
-- -f force - rewrites the database with all features
-- -e eliminates tables - drops CSM tables from the database
-- -d totally removes the database from the system
------------------------------------------------------------------------------------------------

-- CSM API database helper functions
DROP FUNCTION IF EXISTS fn_csm_allocation_node_sharing_status(i_allocation_id bigint,i_type text,i_state text,i_shared boolean,variadic i_nodenames text[]);
DROP FUNCTION IF EXISTS fn_csm_allocation_finish_data_stats(allocationid bigint, i_state text, node_names text[], ib_rx_list bigint[], ib_tx_list bigint[], gpfs_read_list bigint[], gpfs_write_list bigint[], energy_list bigint[], pc_hit_list bigint[], gpu_usage_list bigint[], cpu_usage_list bigint[], mem_max_list bigint[], gpu_energy_list bigint[], OUT o_end_time timestamp without time zone, OUT o_final_state text);
DROP FUNCTION IF EXISTS fn_csm_allocation_create_data_aggregator(i_allocation_id bigint,i_state text,i_node_names text[],i_ib_rx_list bigint[],i_ib_tx_list bigint[],i_gpfs_read_list bigint[],i_gpfs_write_list bigint[],i_energy bigint[],i_power_cap integer[],i_ps_ratio integer[],i_power_cap_hit bigint[],i_gpu_usage bigint[], out o_timestamp timestamp);
DROP FUNCTION IF EXISTS fn_csm_allocation_node_change();
DROP FUNCTION IF EXISTS fn_csm_allocation_revert(allocationid bigint);
DROP FUNCTION IF EXISTS fn_csm_step_begin(i_step_id bigint,i_allocation_id bigint,i_status text,i_executable text,i_working_directory text,i_argument text,i_environment_variable text,i_num_nodes integer,i_num_processors integer,i_num_gpus integer,i_projected_memory integer,i_num_tasks integer,i_user_flags text,i_node_names text[], OUT o_begin_time timestamp);
DROP FUNCTION IF EXISTS fn_csm_step_end(IN i_stepid bigint,IN i_allocationid bigint,IN i_exitstatus int,IN i_errormessage text,IN i_cpustats text,IN i_totalutime double precision,IN i_totalstime double precision,IN i_ompthreadlimit text,IN i_gpustats text,IN i_memorystats text,IN i_maxmemory bigint,IN i_iostats text,OUT o_user_flags text,OUT o_num_nodes int,OUT o_nodes text, OUT o_end_time timestamp);
DROP FUNCTION IF EXISTS fn_csm_allocation_update();
DROP FUNCTION IF EXISTS fn_csm_allocation_state_history_state_change();
DROP FUNCTION IF EXISTS fn_csm_allocation_update_state(IN i_allocationid bigint,IN i_state text,OUT o_primary_job_id bigint,OUT o_secondary_job_id integer,OUT o_user_flags text,OUT o_system_flags text,OUT o_num_nodes integer,OUT o_nodes text,OUT o_isolated_cores integer,OUT o_user_name text,OUT o_shared boolean,OUT o_num_gpus integer,OUT o_num_processors integer,OUT o_projected_memory integer,OUT o_state text, OUT o_runtime bigint, OUT o_smt_mode smallint, OUT o_core_blink boolean);
DROP FUNCTION IF EXISTS fn_csm_allocation_dead_records_on_lv(i_allocation_id bigint);
DROP FUNCTION IF EXISTS fn_csm_lv_upsert(l_logical_volume_name text,l_node_name text,l_allocation_id bigint,l_vg_name text,l_state char(1),l_current_size bigint,l_max_size bigint,l_begin_time timestamp,l_updated_time timestamp,l_file_system_mount text,l_file_system_type text);
DROP FUNCTION IF EXISTS fn_csm_node_update();
DROP FUNCTION IF EXISTS fn_csm_node_delete(i_node_names text[]);
DROP FUNCTION IF EXISTS fn_csm_node_state();
DROP FUNCTION IF EXISTS fn_csm_node_attributes_query_details(text);
DROP FUNCTION IF EXISTS fn_csm_node_state_history_temp_table(i_state compute_node_states, i_start_t timestamp, i_end_t timestamp, OUT node_name text, OUT state compute_node_states, OUT hours_of_state numeric, OUT total_range_time numeric, OUT "%_of_state" numeric);
DROP TYPE IF EXISTS ras_event_severity;
DROP TYPE IF EXISTS compute_node_states;
DROP TYPE IF EXISTS node_details;
DROP FUNCTION IF EXISTS fn_csm_ras_type_update();
DROP FUNCTION IF EXISTS fn_csm_switch_attributes_query_details(text);
DROP TYPE IF EXISTS switch_details;
DROP FUNCTION IF EXISTS fn_csm_vg_create(i_available_size bigint,i_node_name text,i_ssd_count int,i_ssd_serial_numbers text[],i_ssd_allocations bigint[],i_total_size bigint,i_vg_name text,i_is_scheduler boolean);
DROP FUNCTION IF EXISTS fn_csm_vg_delete(i_node_name text,i_vg_name text);
-- CSM database history dump functions
DROP FUNCTION IF EXISTS fn_csm_allocation_history_dump(allocationid bigint, endtime timestamp without time zone, exitstatus integer, i_state text, finalize boolean, node_names text[], ib_rx_list bigint[], ib_tx_list bigint[], gpfs_read_list bigint[], gpfs_write_list bigint[], energy_list bigint[], pc_hit_list bigint[], gpu_usage_list bigint[], cpu_usage_list bigint[], mem_max_list bigint[], gpu_energy_list bigint[], OUT o_end_time timestamp without time zone);
DROP FUNCTION IF EXISTS fn_csm_config_history_dump();
DROP FUNCTION IF EXISTS fn_csm_dimm_history_dump();
DROP FUNCTION IF EXISTS fn_csm_gpu_history_dump();
DROP FUNCTION IF EXISTS fn_csm_hca_history_dump();
DROP FUNCTION IF EXISTS fn_csm_switch_inventory_history_dump();
DROP FUNCTION IF EXISTS fn_csm_ib_cable_history_dump();
DROP FUNCTION IF EXISTS fn_csm_lv_history_dump(text,text,bigint,timestamp,timestamp,bigint,bigint,bigint,bigint);
DROP FUNCTION IF EXISTS fn_csm_lv_update_history_dump();
DROP FUNCTION IF EXISTS fn_csm_lv_modified_history_dump();
DROP FUNCTION IF EXISTS fn_csm_processor_socket_history_dump();
DROP FUNCTION IF EXISTS fn_csm_ssd_wear();
DROP FUNCTION IF EXISTS fn_csm_ssd_dead_records(i_sn text);
DROP FUNCTION IF EXISTS fn_csm_ssd_history_dump();
DROP FUNCTION IF EXISTS fn_csm_step_history_dump(i_stepid bigint,i_allocationid bigint,i_endtime timestamp with time zone,i_exitstatus int,i_errormessage text,i_cpustats text,i_totalutime double precision,i_totalstime double precision,i_ompthreadlimit text,i_gpustats text,i_memorystats text,i_maxmemory bigint,i_iostats text);
DROP FUNCTION IF EXISTS fn_csm_step_node_history_dump();
DROP FUNCTION IF EXISTS fn_csm_switch_history_dump();
DROP FUNCTION IF EXISTS fn_csm_vg_history_dump();
DROP FUNCTION IF EXISTS fn_csm_vg_ssd_history_dump();
DROP FUNCTION IF EXISTS fn_csm_diag_run_history_dump(_run_id bigint,_end_time timestamp with time zone,_status text,_inserted_ras boolean);
DROP FUNCTION IF EXISTS fn_csm_diag_result_history_dump();
DROP FUNCTION IF EXISTS fn_csm_db_schema_version_history_dump();
-- CSM INVENTORY COLLECTION RELATED FUNCTIONS
DROP FUNCTION IF EXISTS fn_csm_switch_inventory_collection(int,text[],text[],text[],text[],text[],text[],boolean[],text[],text[],text[],int[],text[],text[],text[],text[],text[],text[],text[],text[],text[],text[],int[],text[],text[]);
DROP FUNCTION IF EXISTS fn_csm_switch_children_inventory_collection(int,text[],text[],text[],text[],text[],text[],text[],int[],int[],int[],text[],text[],text[],text[], text[], text[]);
DROP FUNCTION IF EXISTS fn_csm_ib_cable_inventory_collection(int,text[],text[],text[],text[],text[],text[],text[],text[],text[],text[],text[],text[],text[],text[]);
DROP FUNCTION IF EXISTS fn_csm_allocation_delete_start(i_allocation_id bigint,i_primary_job_id bigint,i_secondary_job_id integer,i_timeout_time bigint,OUT o_allocation_id bigint,OUT o_primary_job_id bigint,OUT o_secondary_job_id integer,OUT o_user_flags text,OUT o_system_flags text,OUT o_num_nodes integer,OUT o_state text,OUT o_type text,OUT o_isolated_cores integer,OUT o_user_name text,OUT o_nodelist text, OUT o_runtime bigint) CASCADE;

--DROP FUNCTION IF EXISTS fn_csm_allocation_delete_start(IN i_allocation_id bigint,IN i_primary_job_id bigint, IN i_secondary_job_id integer,OUT o_allocation_id bigint,OUT o_primary_job_id bigint,OUT o_secondary_job_id integer,OUT o_user_flags text,OUT o_system_flags text,OUT o_num_nodes integer,OUT o_state text,OUT o_type text,OUT o_isolated_cores integer,OUT o_user_name text,OUT o_nodelist text);

COMMIT;
