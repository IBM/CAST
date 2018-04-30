--===============================================================================
--
--   csm_delete_data.sql
--
-- © Copyright IBM Corporation 2015-2018. All Rights Reserved
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
--   usage:         ./csm_db_script.sh <----- delete data in the csm_db tables
--   version:       03.7
--   create:        04-25-2015
--   last modified: 02-26-2018
--   change log:    
--   03.7 -         modified or changed these tables
--                  csm_node_ready_history = csm_node_state_history
--                  csm_processor = csm_processor_socket
--   03.6 -         added csm_ssd_wear_history table
--===============================================================================

\set ON_ERROR_STOP on
BEGIN;
    DELETE FROM csm_vg_ssd;
    DELETE FROM csm_vg_ssd_history;
    DELETE FROM csm_ssd;
    DELETE FROM csm_ssd_history;
    DELETE FROM csm_ssd_wear_history;
    DELETE FROM csm_lv;
    DELETE FROM csm_vg;
    DELETE FROM csm_vg_history;
    DELETE FROM csm_map_tag;
    DELETE FROM csm_map_tag_history;
    DELETE FROM csm_allocation_state_history;
    DELETE FROM csm_step_node;
    DELETE FROM csm_step_node_history;
    DELETE FROM csm_allocation_node;
    DELETE FROM csm_allocation_node_history;
    DELETE FROM csm_allocation_history;
    DELETE FROM csm_step;
    DELETE FROM csm_step_history;
    DELETE FROM csm_allocation;
    DELETE FROM csm_diag_result;
    DELETE FROM csm_diag_result_history;
    DELETE FROM csm_diag_run;
    DELETE FROM csm_diag_run_history;
    DELETE FROM csm_ras_event_action;
    DELETE FROM csm_ras_type;
    DELETE FROM csm_ras_type_audit;
    DELETE FROM csm_gpu;
    DELETE FROM csm_gpu_history;
    DELETE FROM csm_processor_socket;
    DELETE FROM csm_processor_socket_history;
    DELETE FROM csm_dimm;
    DELETE FROM csm_dimm_history;
    DELETE FROM csm_lv_history;
    DELETE FROM csm_lv_update_history;
    DELETE FROM csm_switch;
    DELETE FROM csm_switch_history;
    DELETE FROM csm_hca;
    DELETE FROM csm_hca_history;
    DELETE FROM csm_ib_cable;
    DELETE FROM csm_ib_cable_history;
    DELETE FROM csm_node;
    DELETE FROM csm_node_state_history;
    DELETE FROM csm_node_history;
    DELETE FROM csm_config;
    DELETE FROM csm_config_history;
    DELETE FROM csm_config_bucket;
    -- DELETE FROM csm_db_schema_version;
    -- DELETE FROM csm_db_schema_version_history;

COMMIT;
