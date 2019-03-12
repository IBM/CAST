--===============================================================================
--
--   csm_db_schema_updates_16_0_ms.sql
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
--   usage:         ./csm_db_schema_version_upgrade_16_0.sh
--   Purpose:		Upgrades associated with DB schema 16.0
--===============================================================================

\set ON_ERROR_STOP on
BEGIN;

ALTER TABLE csm_processor_socket DROP CONSTRAINT csm_processor_socket_pkey;
ALTER TABLE csm_processor_socket ADD PRIMARY KEY (node_name, serial_number);
ALTER TABLE csm_dimm DROP CONSTRAINT csm_dimm_pkey;
ALTER TABLE csm_dimm ADD PRIMARY KEY (node_name, serial_number);
COMMENT ON COLUMN csm_allocation_node.energy is 'the total energy used by the node in joules during the allocation';
COMMENT ON COLUMN csm_allocation_node.gpu_usage is 'the total usage aggregated across all GPUs in the node in microseconds during the allocation';
COMMENT ON COLUMN csm_allocation_node.gpu_energy is 'the total energy used across all GPUs in the node in joules during the allocation';
COMMENT ON COLUMN csm_allocation_node_history.energy is 'the total energy used by the node in joules during the allocation'; 
COMMENT ON COLUMN csm_allocation_node_history.gpu_usage is 'the total usage aggregated across all GPUs in the node in microseconds during the allocation';
COMMENT ON COLUMN csm_allocation_node_history.gpu_energy is 'the total energy used across all GPUs in the node in joules during the allocation';

COMMIT;
