--===============================================================================
--
--   csm_db_schema_updates_17_0_ms.sql
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
--   usage:         ./csm_db_schema_version_upgrade_17_0.sh
--   Purpose:		Upgrades associated with DB schema 17.0
--===============================================================================

SET client_min_messages TO WARNING;
\set ON_ERROR_STOP on
BEGIN;

---------------------------------------------------------------------------------
-- CSM db migration updates for 17.0
-- csm_allocation and csm_allocation_history tables: include field smt_mode
-- csm_lv_history new fields: num_reads and num_writes
---------------------------------------------------------------------------------

DO $$ 
    BEGIN
        BEGIN
            ALTER TABLE csm_allocation ADD COLUMN smt_mode smallint NOT NULL DEFAULT 0;
        EXCEPTION
            WHEN duplicate_column THEN RAISE NOTICE 'column smt_mode already exists in csm_allocation.';
        END;
        BEGIN
            ALTER TABLE csm_allocation_history ADD COLUMN smt_mode smallint;
        EXCEPTION
            WHEN duplicate_column THEN RAISE NOTICE 'column smt_mode already exists in csm_allocation_history.';
        END;
        BEGIN
            ALTER TABLE csm_lv_history ADD COLUMN num_reads bigint;
        EXCEPTION
            WHEN duplicate_column THEN RAISE NOTICE 'column num_reads already exists in csm_lv_history.';
        END;
        BEGIN
            ALTER TABLE csm_lv_history ADD COLUMN num_writes bigint;
        EXCEPTION
            WHEN duplicate_column THEN RAISE NOTICE 'column num_writes already exists in csm_lv_history.';
        END;
    END;
$$;
COMMENT ON COLUMN csm_allocation.smt_mode is 'the smt mode of the allocation';
COMMENT ON COLUMN csm_allocation_history.smt_mode is 'the smt mode of the allocation';

COMMENT ON COLUMN csm_lv_history.num_reads is 'number of read during the life of this partition';
COMMENT ON COLUMN csm_lv_history.num_writes is 'number of writes during the life of this partition';

COMMIT;
