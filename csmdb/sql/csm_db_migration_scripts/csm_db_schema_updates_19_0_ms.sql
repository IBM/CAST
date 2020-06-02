--===============================================================================
--
--   csm_db_schema_updates_19_0_ms.sql
--
-- © Copyright IBM Corporation 2020. All Rights Reserved
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
--   usage:         ./csm_db_schema_version_upgrade_18_1.sh
--   Purpose:		Upgrades associated with DB schema 18.0
--   Last modified:	06-01-2020
--===============================================================================

SET client_min_messages TO WARNING;
\set ON_ERROR_STOP on
BEGIN;

---------------------------------------------------------------------------------
-- CSM db migration updates for 19.0
---------------------------------------------------------------------------------
-- Removing (ctid) indexes as they can not
-- be created in the latest version of
-- RedHat 8.1
---------------------------------------------------------------------------------
DROP INDEX IF EXISTS ix_csm_allocation_history_c;            --csm_allocation_history (ctid)
DROP INDEX IF EXISTS ix_csm_allocation_node_history_c;       --csm_allocation_node_history (ctid)
DROP INDEX IF EXISTS ix_csm_allocation_state_history_c;      --csm_allocation_state_history (ctid)
DROP INDEX IF EXISTS ix_csm_config_history_c;                --csm_config_history (ctid)
DROP INDEX IF EXISTS ix_csm_db_schema_version_history_c;     --csm_db_schema_version_history (ctid)
DROP INDEX IF EXISTS ix_csm_diag_result_history_c;           --csm_diag_result_history (ctid)
DROP INDEX IF EXISTS ix_csm_diag_run_history_d;              --csm_diag_run_history (ctid)
DROP INDEX IF EXISTS ix_csm_dimm_history_c;                  --csm_dimm_history (ctid)
DROP INDEX IF EXISTS ix_csm_gpu_history_d;                   --csm_gpu_history (ctid)
DROP INDEX IF EXISTS ix_csm_hca_history_c;                   --csm_hca_history (ctid)
DROP INDEX IF EXISTS ix_csm_ib_cable_history_c;              --csm_ib_cable_history (ctid)
DROP INDEX IF EXISTS ix_csm_lv_history_c;                    --csm_lv_history (ctid)
DROP INDEX IF EXISTS ix_csm_lv_update_history_c;             --csm_lv_update_history (ctid)
DROP INDEX IF EXISTS ix_csm_node_history_c;                  --csm_node_history (ctid)
DROP INDEX IF EXISTS ix_csm_node_state_history_c;            --csm_node_state_history (ctid)
DROP INDEX IF EXISTS ix_csm_processor_socket_history_c;      --csm_processor_socket_history (ctid)
DROP INDEX IF EXISTS ix_csm_ras_event_action_g;              --csm_ras_event_action (ctid)
DROP INDEX IF EXISTS ix_csm_ssd_history_c;                   --csm_ssd_history (ctid)
DROP INDEX IF EXISTS ix_csm_ssd_wear_history_c;              --csm_ssd_wear_history (ctid)
DROP INDEX IF EXISTS ix_csm_step_history_f;                  --csm_step_history (ctid)
DROP INDEX IF EXISTS ix_csm_step_node_history_d;             --csm_step_node_history (ctid)
DROP INDEX IF EXISTS ix_csm_switch_history_c;                --csm_switch_history (ctid)
DROP INDEX IF EXISTS ix_csm_switch_inventory_history_c;      --csm_switch_inventory_history (ctid)
DROP INDEX IF EXISTS ix_csm_vg_history_c;                    --csm_vg_history (ctid)
DROP INDEX IF EXISTS ix_csm_vg_ssd_history_c;                --csm_vg_ssd_history (ctid)
COMMIT;
