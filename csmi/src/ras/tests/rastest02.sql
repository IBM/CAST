/*================================================================================

    csmi/src/ras/tests/rastest02.sql

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

DROP FUNCTION IF EXISTS upsert_ras_event_type(imsg_id TEXT, 
                                                 iMinTimeInPool INT,
                                                 iSuppressIds TEXT,
                                                 iSeverity TEXT,
                                                 iMessage TEXT,
                                                 iDescription TEXT,
                                                 iDecoder TEXT,
                                                 icontrol_action TEXT,
                                                 ithreshold_count INT,
                                                 ithreshold_period TEXT,
                                                 irelevant_diags TEXT);

-- function to insert or replace a ras event type...
CREATE OR REPLACE FUNCTION upsert_ras_event_type(imsg_id TEXT, 
                                                 iMinTimeInPool INT,
                                                 iSuppressIds TEXT,
                                                 iSeverity TEXT,
                                                 iMessage TEXT,
                                                 iDescription TEXT,
                                                 iDecoder TEXT,
                                                 icontrol_action TEXT,
                                                 ithreshold_count INT,
                                                 ithreshold_period TEXT,
                                                 irelevant_diags TEXT)
  RETURNS void AS
$$
BEGIN
    IF EXISTS(SELECT msg_id FROM CSM_RAS_TYPE WHERE msg_id = imsg_id)
        THEN
            UPDATE CSM_RAS_TYPE SET min_time_in_pool = iMinTimeInPool, 
                                    suppress_ids = iSuppressIds,
                                    Message = iMessage,
                                    Description = iDescription,
                                    Decoder = iDecoder,
                                    control_action = icontrol_action,
                                    threshold_count = ithreshold_count,
                                    threshold_period = ithreshold_period, 
                                    relevant_diags = irelevant_diags WHERE msg_id = imsg_id;
        RETURN;
    ELSE
        INSERT INTO CSM_RAS_TYPE( msg_id,
            min_time_in_pool,
            suppress_ids,
            Severity,
            Message,
            Description,
            Decoder,
            control_action,
            threshold_count,
            threshold_period,
            relevant_diags) 
        VALUES (imsg_id,
            iMinTimeInPool,
            iSuppressIds,
            iSeverity,
            iMessage,
            iDescription,
            iDecoder,
            icontrol_action,
            ithreshold_count,
            ithreshold_period,
            irelevant_diags);
        RETURN;
    END IF;
END;
$$
LANGUAGE plpgsql;

SELECT upsert_ras_event_type(
        'csm.status.up',
        20,
        '',
        'INFO',
        '$(location_name) csm agent is up',
        'csm agent daemon has started up and signaled the system it is available.',
        'none',
        'none',
        0,
        '0',
        'NONE'
        );

-- what happens to this during a normal shutdown..??
-- should this have an action, or do we start building conditional actions into
-- the action field...
SELECT upsert_ras_event_type(
        'csm.status.down',
        '20',
        '',
        'INFO',
        '$(location_name) csm agent is down',
        'csm agent daemon has stopped.',
        'none',
        'none',
        0,
        '0',
        'NONE'
        );

SELECT upsert_ras_event_type(
        'bmc.system.bootinit',
        '20',
        '',
        'INFO',
        '$(location_name) BMC system boot initiated',
        'BMC reported system boot initiated in the system event log',
        'none',
        'none',
        0,
        '0',
        'NONE'
        );

SELECT upsert_ras_event_type(
        'bmc.system.ac_lost',
        20,
        'csm.status.down',
        'INFO',
        '$(location_name) BMC Power Supply AC lost',
        'BMC Power Supply AC lost in the system event log',
        'none',
        'none',
        0,
        '0',
        'NONE'
        );

SELECT upsert_ras_event_type(
        'bmc.system.ac_watchdog',
        20,
        '',
        'INFO',
        '$(location_name) BMC Watchdog 2, hard reset',
        'BMC watchdog initiated a hard reset',
        'none',
        'none',
        0,
        '0',
        'NONE'
        );



-- generic test ras events.....
SELECT upsert_ras_event_type(
        'test.testcat01.test01',
        5,
        'test.testcat01.test02',
        'FATAL',
        'test mesage for $(location_name) $(k1) $(k2)',
        'test message for RAS system.',
        'none',
        'test_kill_job',
        0,
        '0',
        'NONE'
        );
SELECT upsert_ras_event_type(
        'test.testcat01.test02',
        5,
        'test.testcat01.test03',
        'FATAL',
        'test mesage for $(location_name) $(k1) $(k2)',
        'test message for RAS system.',
        'none',
        '',
        0,
        '0',
        'NONE'
        );

SELECT upsert_ras_event_type(
        'test.testcat01.test03',
        5,
        '',
        'FATAL',
        'test mesage for $(location_name) $(k1) $(k2)',
        'test message for RAS system.',
        'none',
        '',
        5,
        '0',
        'NONE'
        );


SELECT upsert_ras_event_type(
        'test.testcat01.test04',
        5,
        '',
        'FATAL',
        'test mesage for $(location_name) $(k1) $(k2)',
        'test message for RAS system.',
        'none',
        '',
        0,
        '0',
        'NONE'
        );


SELECT upsert_ras_event_type(
        'unknown.ras.msg',
        0,
        '',
        'INFO',
        'unknown ras message $(location_name)',
        'unknown ras message',
        'none',
        '',
        0,
        '0',
        'NONE'
        );


DROP FUNCTION IF EXISTS upsert_ras_event_type(imsg_id TEXT, 
                                                 iMinTimeInPool INT,
                                                 iSuppressIds TEXT,
                                                 iSeverity TEXT,
                                                 iMessage TEXT,
                                                 iDescription TEXT,
                                                 iDecoder TEXT,
                                                 icontrol_action TEXT,
                                                 ithreshold_count INT,
                                                 ithreshold_period TEXT,
                                                 irelevant_diags TEXT);

