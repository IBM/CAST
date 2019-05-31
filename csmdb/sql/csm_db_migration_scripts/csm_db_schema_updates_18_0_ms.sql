--===============================================================================
--
--   csm_db_schema_updates_18_0_ms.sql
--
-- © Copyright IBM Corporation 2019. All Rights Reserved
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
--   usage:         ./csm_db_schema_version_upgrade_18_0.sh
--   Purpose:		Upgrades associated with DB schema 18.0
--===============================================================================

SET client_min_messages TO WARNING;
\set ON_ERROR_STOP on
BEGIN;

---------------------------------------------------------------------------------
-- CSM db migration updates for 18.0
---------------------------------------------------------------------------------
-- csm_allocation and csm_allocation_history tables: include field core_blink
-- csm_switch_inventory and csm_switch_inventory_history new field: type
-- csm_switch_inventory and csm_switch_inventory_history new field: fw_version
---------------------------------------------------------------------------------
-- Other changes related to the csm_create_function.sql file.
-- (which get auto generated through the acutal migration script)
---------------------------------------------------------------------------------
-- switch_details type added in hw_version
-- COMMENT ON FUNCTION fn_csm_switch_children_inventory_collection (added in hw_version field)
-- updated the fn_csm_switch_attributes_query_details function
-- fn_csm_switch_children_inventory_collection(added in hw_version, type, and fw_version fields)
-- fn_csm_allocation_update (added field core_blink)
-- fn_csm_allocation_history_dump (added field core_blink)
---------------------------------------------------------------------------------
-- Additional descriptions that have been updated include the following fields
-- in the csm_step_history table:
---------------------------------------------------------------------------------
-- cpu_stats            | 'statistics gathered from the CPU for the step.';
-- omp_thread_limit     | 'max number of omp threads used by the step.';
-- gpu_stats            | 'statistics gathered from the GPU for the step.';
-- memory_stats         | 'memory statistics for the the step.';
-- max_memory           | 'the maximum memory usage of the step.';
-- io_stats             | 'general input output statistics for the step.';
---------------------------------------------------------------------------------

DO $$ 
    BEGIN
        BEGIN
            ALTER TABLE csm_allocation ADD COLUMN core_blink boolean NOT NULL DEFAULT FALSE;
        EXCEPTION
            WHEN duplicate_column THEN RAISE INFO 'column core_blink already exists in csm_allocation, skipping.';
        END;
        BEGIN
            ALTER TABLE csm_allocation_history ADD COLUMN core_blink boolean;
        EXCEPTION
            WHEN duplicate_column THEN RAISE INFO 'column core_blink already exists in csm_allocation_history, skipping.';
        END;
        BEGIN
            ALTER TABLE csm_switch_inventory ADD COLUMN type text;
        EXCEPTION
            WHEN duplicate_column THEN RAISE INFO 'column type already exists in csm_switch_inventory, skipping.';
        END;
        BEGIN
            ALTER TABLE csm_switch_inventory_history ADD COLUMN type text;
        EXCEPTION
            WHEN duplicate_column THEN RAISE INFO 'column type already exists in csm_switch_inventory_history, skipping.';
        END;
        
        BEGIN
            ALTER TABLE csm_switch_inventory ADD COLUMN fw_version text;
        EXCEPTION
            WHEN duplicate_column THEN RAISE INFO 'column fw_version already exists in csm_switch_inventory, skipping.';
        END;
        BEGIN
            ALTER TABLE csm_switch_inventory_history ADD COLUMN fw_version text;
        EXCEPTION
            WHEN duplicate_column THEN RAISE INFO 'column fw_version already exists in csm_switch_inventory_history, skipping.';
        END;
    END;
$$;
COMMENT ON COLUMN csm_allocation.core_blink is 'flag indicating whether or not to run a blink operation on allocation cores.';
COMMENT ON COLUMN csm_allocation_history.core_blink is 'flag indicating whether or not to run a blink operation on allocation cores.';

COMMENT ON COLUMN csm_switch_inventory.type is 'The category of this piece of hardware inventory. For example: "FAN", "PS", "SYSTEM", or "MGMT".';
COMMENT ON COLUMN csm_switch_inventory_history.type is 'The category of this piece of hardware inventory. For example: "FAN", "PS", "SYSTEM", or "MGMT".';

COMMENT ON COLUMN csm_switch_inventory.fw_version is 'The firmware version on this piece of inventory.';
COMMENT ON COLUMN csm_switch_inventory_history.fw_version is 'The firmware version on this piece of inventory.';

-- Additional descriptions that have been updated include:
COMMENT ON COLUMN csm_step_history.cpu_stats is 'statistics gathered from the CPU for the step.';
COMMENT ON COLUMN csm_step_history.omp_thread_limit is 'max number of omp threads used by the step.';
COMMENT ON COLUMN csm_step_history.gpu_stats is 'statistics gathered from the GPU for the step.';
COMMENT ON COLUMN csm_step_history.memory_stats is 'memory statistics for the the step.';
COMMENT ON COLUMN csm_step_history.max_memory is 'the maximum memory usage of the step.';
COMMENT ON COLUMN csm_step_history.io_stats is 'general input output statistics for the step.';

COMMIT;
