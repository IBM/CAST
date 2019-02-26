--===============================================================================
--
--   csm_drop_triggers_functions.sql
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
--   usage:         ./csm_db_schema_version_upgrade_.sh
--   Purpose:		Drop triggers and functions associated with DB schema 15.0
--===============================================================================

\set ON_ERROR_STOP on
BEGIN;

------------------------------------------------------------------------------------------------------------
-- Function to drop all the existing triggers in the db
------------------------------------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION func_csm_drop_all_triggers()
RETURNS text AS $$

DECLARE
    fn_csm_trigger_name_rec RECORD;
    fn_csm_trigger_table_rec RECORD;

BEGIN
    FOR fn_csm_trigger_name_rec IN select distinct(trigger_name) from information_schema.triggers where trigger_schema = 'public' LOOP
        FOR fn_csm_trigger_table_rec IN SELECT distinct(event_object_table) from information_schema.triggers where trigger_name = fn_csm_trigger_name_rec.trigger_name LOOP
            EXECUTE 'DROP TRIGGER ' || fn_csm_trigger_name_rec.trigger_name || ' ON ' || fn_csm_trigger_table_rec.event_object_table || ';';
        END LOOP;
    END LOOP;

    RETURN 'done';
END;
$$ LANGUAGE plpgsql;

------------------------------------------------------------------------------------------------------------
-- Function to drop all the existing functions in the db
------------------------------------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION func_csm_delete_func(_name text, OUT func_dropped int)
AS $$

DECLARE
   _sql text;

   BEGIN
   SELECT count(*)::int,
    'DROP FUNCTION ' || string_agg(oid::regprocedure::text, '; DROP FUNCTION ')
   FROM   pg_proc
   WHERE  proname LIKE '%fn_csm%'
   AND    pg_function_is_visible(oid)
   INTO   func_dropped, _sql;  -- only returned if trailing DROPs succeed

   IF func_dropped > 0 THEN    -- only if function(s) found
     EXECUTE _sql;
   END IF;
END
$$ LANGUAGE plpgsql;


----------------------------------------------------------------------------------------------------------------------------------
-- Function to append to an existing db TYPE
----------------------------------------------------------------------------------------------------------------------------------

CREATE OR REPLACE function func_alt_type_val(_type regtype, _val  text)
RETURNS void
AS $$
BEGIN
    IF NOT EXISTS 
        (SELECT enumlabel FROM pg_enum 
            WHERE enumtypid = _type
            AND enumlabel = _val) THEN
            INSERT INTO pg_enum (enumtypid, enumlabel, enumsortorder) SELECT _type::regtype::oid, _val, ( SELECT MAX(enumsortorder) + 1 FROM pg_enum WHERE enumtypid = _type::regtype );
    END IF;
    EXCEPTION
    WHEN others THEN
        RAISE EXCEPTION
        USING ERRCODE = sqlstate,
            MESSAGE = 'error_handling_test: ' || sqlstate || '/' || sqlerrm;
END
$$ LANGUAGE plpgsql;

COMMIT;
