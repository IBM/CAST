/*================================================================================

    csmi/src/ras/tests/rastest01.sql

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

-- postgres sql db records for ras testing in this record.


DELETE FROM CSM_RAS_TYPE WHERE msg_id = 'ufmevents.iblink.linkdown';

INSERT 
   INTO CSM_RAS_TYPE (
      msg_id,
   	Category,
   	Component,
   	Severity,
   	Message,
   	Description,
   	Decoder,
   	control_action,
   	threshold_count,
   	threshold_period,
   	relevant_diags) VALUES(
        'ufmevents.iblink.linkdown',
        'ufmevents',
        'iblink',
        'FATAL',
        'ib link for $(location) is down',
        'UFM has detected the ib link is down',
        'none',
        'kill_job',
        0,
        '0',
        'NONE'
        );

-- test records...
DELETE FROM CSM_RAS_TYPE WHERE msg_id = 'test.testcat01.test01';

INSERT 
   INTO CSM_RAS_TYPE (
      msg_id,
   	Category,
   	Component,
   	Severity,
   	Message,
   	Description,
   	Decoder,
   	control_action,
   	threshold_count,
   	threshold_period,
   	relevant_diags) VALUES(
        'test.testcat01.test01',
        'test',
        'testcat1',
        'FATAL',
        'test mesage for $(location) $(k1) $(k2)',
        'test message for RAS system.',
        'none',
        'kill_job',
        0,
        '0',
        'NONE'
        );


