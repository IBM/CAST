--===============================================================================
--
--   csm_drop_tables.sql
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
--   usage:             ./csm_db_script.sh <----- -e (drop all the csm_db tables)
--   current_version:   16.2
--   create:            04-25-2015
--   last modified:     10-24-2018
--   log change:
--      16.2 -          Moving this version to sync with DB schema version
--      16.1 -          Moving this version to sync with DB schema version
--      16.0 -          Moving this version to sync with DB schema version
--      3.9  -          Removed csm_switch_ports + csm_switch_ports_history;
--      3.8  -          table name change
--                      csm_processor + history = csm_processor_socket
--                      csm_node_ready_history = csm_node_state_history
--      3.7  -          added in csm_ssd_wear_history table
--      3.6  -          changed the order in which the csm_vg, csm_vg_ssd are dropped
--===============================================================================

\set ON_ERROR_STOP on
BEGIN;
DROP TABLE IF EXISTS csm_lv;
DROP TABLE IF EXISTS csm_lv_update_history;
DROP TABLE IF EXISTS csm_vg_ssd;
DROP TABLE IF EXISTS csm_vg_ssd_history;
DROP TABLE IF EXISTS csm_vg;
DROP TABLE IF EXISTS csm_vg_history;
DROP TABLE IF EXISTS csm_allocation_state_history;
DROP TABLE IF EXISTS csm_step_node;
DROP TABLE IF EXISTS csm_step_node_history;
DROP TABLE IF EXISTS csm_allocation_node;
DROP TABLE IF EXISTS csm_allocation_node_history;
DROP TABLE IF EXISTS csm_allocation_history;
DROP TABLE IF EXISTS csm_step;
DROP TABLE IF EXISTS csm_step_history;
DROP TABLE IF EXISTS csm_allocation;
DROP TABLE IF EXISTS csm_diag_result;
DROP TABLE IF EXISTS csm_diag_result_history;
DROP TABLE IF EXISTS csm_diag_run;
DROP TABLE IF EXISTS csm_diag_run_history;
DROP TABLE IF EXISTS csm_ras_event_action CASCADE;
DROP TABLE IF EXISTS csm_ras_type;
DROP TABLE IF EXISTS csm_ras_type_audit;
DROP TABLE IF EXISTS csm_node_history;
DROP TABLE IF EXISTS csm_node_state_history;
DROP TABLE IF EXISTS csm_gpu;
DROP TABLE IF EXISTS csm_gpu_history;
DROP TABLE IF EXISTS csm_processor_socket;
DROP TABLE IF EXISTS csm_processor_socket_history;
DROP TABLE IF EXISTS csm_ssd_wear_history;
DROP TABLE IF EXISTS csm_ssd;
DROP TABLE IF EXISTS csm_ssd_history;
DROP TABLE IF EXISTS csm_dimm;
DROP TABLE IF EXISTS csm_dimm_history;
DROP TABLE IF EXISTS csm_lv_history;
DROP TABLE IF EXISTS csm_switch_inventory;
DROP TABLE IF EXISTS csm_switch_inventory_history;
DROP TABLE IF EXISTS csm_switch;
DROP TABLE IF EXISTS csm_switch_history;
DROP TABLE IF EXISTS csm_hca;
DROP TABLE IF EXISTS csm_hca_history;
DROP TABLE IF EXISTS csm_ib_cable;
DROP TABLE IF EXISTS csm_ib_cable_history;
DROP TABLE IF EXISTS csm_node;
DROP TABLE IF EXISTS csm_config;
DROP TABLE IF EXISTS csm_config_history;
DROP TABLE IF EXISTS csm_config_bucket;
DROP TABLE IF EXISTS csm_db_schema_version;
DROP TABLE IF EXISTS csm_db_schema_version_history;
COMMIT;
