CSMIAllocationCreate_Master.cc
******************************
CreateDbNodeCheckSqlStmt
========================
select node_name, ready, state, type from csm_node where node_name in ( **nodes** );

CreateDbAllocNodeInsertSqlStmt
==============================
insert into csm_allocation_node values ( '**alloc_id**', '**compute_nodes**', '**state**' ),...

CreateDbAllocInsertSqlStmt
==========================
insert into csm_allocation values (default, '**primary_job_id**',
    '**secondary_job_id**', '**file_system_name**', '**launch_node_name**', 
    '**allocation_node_check_cmd**', '**deallocation_node_check_cmd**',
    '**ssd_size**', '**num_nodes**', '**num_processors**', '**num_gpus**',
    '**num_memory**', '**state**', '**type**', '**job_type**', '**user_name**',
    '**user_id**', '**user_group_id**', '**user_script**', 'now', '**power_cap**',
    '**power_shifting_ratio**', '**account**', '**comment**', '**eligible_time**',
    '**job_name**', '**reservation**', '**wct_reserve**', '**job_submit_time**',
    '**queue**', '**requeue**', '**time_limit**', '**wc_key**' ) returning allocation_id;


CreateUndoDbAllocInsertSqlStmt
==============================
delete from csm_allocation_node where allocation_id = '**alloc_id**';
select fn_csm_allocation_history_dump( '**alloc_id**', 'now', '-1', 'n', '**energy_consumed**' );

CSMIAllocationDelete_Master.cc
******************************
CreateDbNodeListSqlStmt
=======================
select node_name, state from csm_allocation_node where allocation_id = '**allocation_id**';


CreateDbAllocSelectSqlStmt
==========================
select allocation_id,primary_job_id,secondary_job_id,file_system_name,
    launch_node_name,allocation_node_check_cmd,deallocation_node_check_cmd,
    ssd_size,num_nodes,num_processors,num_gpus,num_memory,state,type,
    job_type,user_name,user_id,user_group_id,user_script,begin_time,
    power_cap,power_shifting_ratio,account,comment,eligible_time,
    job_name,reservation,wct_reserve,job_submit_time,queue,requeue,
    time_limit,wc_key from csm_allocation where allocation_id = '**allocation_id**';

CreateDbUpdateRowsSqlStmt
=========================
update csm_allocation_node;

CreateDbDeleteRowsSqlStmt
=========================
select fn_csm_allocation_history_dump( '**allocation_id**', 'now', '0', 'n', '**energy_consumed**');

.. note This is extremely similar to `CreateUndoDbAllocInsertSqlStmt`_.


CSMIAllocationQueryActiveAll.cc
*******************************

SELECT a.allocation_id, a.primary_job_id, a.secondary_job_id,
    a.file_system_name, a.launch_node_name, a.allocation_node_check_cmd,                
    a.deallocation_node_check_cmd, a.ssd_size, a.num_nodes, a.num_processors,           
    a.num_gpus, a.num_memory, a.state,a.type, a.job_type, a.user_name, a.user_id,   
    a.user_group_id, a.user_script, a.begin_time, a.power_cap, a.power_shifting_ratio,  
    a.account, a.comment, a.eligible_time, a.job_name, a.reservation, a.wct_reserve,
    a.job_submit_time, a.queue, a.requeue, a.time_limit, a.wc_key,
    array_to_string(array_agg(an.node_name), ',') as nodes  FROM csm_allocation a 
    JOIN csm_allocation_node an ON a.allocation_id = an.allocation_id GROUP BY a.allocation_id;

CSMIAllocationQuery.cc
**********************

CreateDbGetAllocationIdSqlStmt
==============================
Query Standard
--------------
select allocation_id from csm_allocation where 
    primary_job_id = '**primary_job_id**' and secondary_job_id = '**secondary_job_id**';

Query History
-------------
select allocation_id from csm_allocation_history where
    primary_job_id = '**primary_job_id**' and secondary_job_id = '**secondary_job_id**';

CreateAllocationQuerySqlStmt
============================

.. note This query seems to be a little too involved: Are all of the group bys really needed?

Query Standard
--------------
select a.allocation_id,a.primary_job_id,a.secondary_job_id,a.file_system_name,
    a.launch_node_name,a.allocation_node_check_cmd,a.deallocation_node_check_cmd,
    a.ssd_size,a.num_nodes,a.num_processors,a.num_gpus,a.num_memory,a.state,a.type,
    a.job_type,a.user_name,a.user_id,a.user_group_id,a.user_script,a.begin_time,
    a.power_cap,a.power_shifting_ratio,a.account,a.comment,a.eligible_time,
    a.job_name,a.reservation,a.wct_reserve,a.job_submit_time,a.queue,a.requeue,
    a.time_limit,a.wc_key,
    array_to_string(array_agg(an.node_name),',') as a_nodelist
    from csm_allocation a join csm_allocation_node an
    on a.allocation_id = an.allocation_id
    where a.allocation_id = '**allocation_id**'
    and primary_job_id = '**primary_job_id**'
    and secondary_job_id = '**secondary_job_id**'
    group by a.allocation_id,a.primary_job_id,a.secondary_job_id,a.file_system_name,
    a.launch_node_name,a.allocation_node_check_cmd,a.deallocation_node_check_cmd,
    a.ssd_size,a.num_nodes,a.num_processors,a.num_gpus,a.num_memory,a.state,a.type,
    a.job_type,a.user_name,a.user_id,a.user_group_id,a.user_script,a.begin_time,
    a.power_cap,a.power_shifting_ratio,a.account,a.comment,a.eligible_time,
    a.job_name,a.reservation,a.wct_reserve,a.job_submit_time,a.queue,a.requeue,
    a.time_limit,a.wc_key


Query History
-------------
select a.allocation_id,a.primary_job_id,a.secondary_job_id,a.file_system_name,
    a.launch_node_name,a.allocation_node_check_cmd,a.deallocation_node_check_cmd,
    a.ssd_size,a.num_nodes,a.num_processors,a.num_gpus,a.num_memory,a.state,a.type,
    a.job_type,a.user_name,a.user_id,a.user_group_id,a.user_script,a.begin_time,
    a.power_cap,a.power_shifting_ratio,a.account,a.comment,a.eligible_time,
    a.job_name,a.reservation,a.wct_reserve,a.job_submit_time,a.queue,a.requeue,
    a.time_limit,a.wc_key,
    array_to_string(array_agg(an.node_name),',') as a_nodelist
    ,a.history_time,a.end_time,a.exit_status,a.power_cap_hit,a.energy_consumed
    from csm_allocation_history a join csm_allocation_node_history an
    on a.allocation_id = an.allocation_id
    where a.allocation_id = '**allocation_id**'
    and primary_job_id = '**primary_job_id**'
    and secondary_job_id = '**secondary_job_id**'
    group by a.allocation_id,a.primary_job_id,a.secondary_job_id,a.file_system_name,
    a.launch_node_name,a.allocation_node_check_cmd,a.deallocation_node_check_cmd,
    a.ssd_size,a.num_nodes,a.num_processors,a.num_gpus,a.num_memory,a.state,a.type,
    a.job_type,a.user_name,a.user_id,a.user_group_id,a.user_script,a.begin_time,
    a.power_cap,a.power_shifting_ratio,a.account,a.comment,a.eligible_time,
    a.job_name,a.reservation,a.wct_reserve,a.job_submit_time,a.queue,a.requeue,
    a.time_limit,a.wc_key
    ,a.history_time,a.end_time,a.exit_status,a.power_cap_hit,a.energy_consumed



CSMIAllocationQueryDetails.cc
*****************************
Query Standard
--------------
SELECT s.step_id,s.num_nodes,
    array_to_string(array_agg(sn.node_name),',') AS nodelist FROM
    csm_step s JOIN csm_step_node sn
    ON s.allocation_id=sn.allocation_id AND
    s.step_id=sn.step_id WHERE 
    s.allocation_id='**allocation_id**'
    GROUP BY s.allocation_id,s.step_id,s.num_nodes
    ORDER BY s.allocation_id,s.step_id
    
Query History
-------------
SELECT s.step_id,s.num_nodes,
    array_to_string(array_agg(sn.node_name),',') AS nodelist FROM
    csm_step_history s JOIN csm_step_node_history sn
    ON s.allocation_id=sn.allocation_id AND
    s.step_id=sn.step_id WHERE 
    s.allocation_id='**allocation_id**'
    GROUP BY s.allocation_id,s.step_id,s.num_nodes
    ORDER BY s.allocation_id,s.step_id



CSMIAllocationStepBegin.cc
**************************
INSERT INTO csm_step VALUES ( 
    **step_id**, **allocation_id**, 'now', '**state**', '**executable**',
    '**working_directory**', '**argument**', '**environment_variable**',
    **seq_id**, **num_nodes**, **num_processors**, **num_gpus**, **num_memory**,
    **num_tasks**);

INSERT INTO csm_step_node VALUES ( **step_id**, **allocation_id**, **compute_node**)...
.. note:: The latter repeates for each compute node in the query.

CSMIAllocationStepEnd.cc
************************
"csm_step_node"

DELETE FROM csm_step_node * WHERE step_id = "**step_id**" AND allocation_id = "**allocation_id**";

SELECT fn_csm_step_history_dump( 
    **step_id**, **allocatoin_id**, 'now', '**level_gpu_usage**',
    **exit_status**, '**err_text**', '**network_bandwidth**', '**cpu_stats**',
    to_timestamp( **total_u_time** ),
    to_timestamp( **total_s_time** ),
    '**total_num_threads**', '**gpu_stats**', '**memory_stats**', '**max_memory**', '**max_swap**',
    '**io_stats**');



CSMIAllocationStepQuery.cc
**************************

SELECT
    null as history_time, 
    step_id as step_id, 
    allocation_id as allocation_id, 
    begin_time as begin_time, 
    null as end_time, 
    state as state, 
    executable as executable, 
    working_directory as working_directory, 
    argument as argument, 
    environment_variable as environment_variable, 
    seq_id as seq_id, 
    num_nodes as num_nodes, 
    num_processors as num_processors, 
    num_gpus as num_gpus, 
    num_memory as num_memory, 
    num_tasks as num_tasks, 
    null as level_gpu_usage, 
    null as exit_status, 
    null as err_text, 
    null as network_bandwidth, 
    null as cpu_stats, 
    null as total_u_time, 
    null as total_s_time, 
    null as total_num_threads, 
    null as gpu_stats, 
    null as memory_stats, 
    null as max_memory, 
    null as max_swap, 
    null as io_stats 
FROM csm_step WHERE
    step_id = '**step_id**' AND allocation_id ='**allocation_id**'
UNION SELECT
    history_time as history_time, 
    step_id as step_id, 
    allocation_id as allocation_id, 
    begin_time as begin_time, 
    end_time as end_time, 
    state as state, 
    executable as executable, 
    working_directory as working_directory, 
    argument as argument, 
    environment_variable as environment_variable, 
    seq_id as seq_id, 
    num_nodes as num_nodes, 
    num_processors as num_processors, 
    num_gpus as num_gpus, 
    num_memory as num_memory, 
    num_tasks as num_tasks, 
    level_gpu_usage as level_gpu_usage, 
    exit_status as exit_status, 
    err_text as err_text, 
    network_bandwidth as network_bandwidth, 
    cpu_stats as cpu_stats, 
    total_u_time as total_u_time, 
    total_s_time as total_s_time, 
    total_num_threads as total_num_threads, 
    gpu_stats as gpu_stats, 
    memory_stats as memory_stats, 
    max_memory as max_memory, 
    max_swap as max_swap, 
    io_stats as io_stats 
FROM csm_step_history WHERE
    step_id = '**step_id**' AND allocation_id ='**allocation_id**';


CSMIAllocationStepQueryDetails.cc
*********************************
RetrieveDataForPrivateCheck
===========================
SELECT user_id FROM csm_allocation WHERE allocation_id ='**allocation_id**';

CreateSqlStmt
=============
WITH s AS (
    SELECT
        CAST(NULL AS DATE) as history_time, 
        s.allocation_id, 
        s.step_id, 
        s.begin_time, 
        CAST(NULL AS DATE) as end_time, 
        s.state as state, 
        s.executable as executable, 
        s.working_directory as working_directory, 
        s.argument as argument, 
        s.environment_variable as environment_variable, 
        s.seq_id as seq_id, 
        s.num_nodes as num_nodes, 
        s.num_processors as num_processors, 
        s.num_gpus as num_gpus, 
        s.num_memory as num_memory, 
        s.num_tasks as num_tasks, 
        CAST(NULL AS TEXT) as level_gpu_usage, 
        CAST(NULL AS INT) as exit_status, 
        CAST(NULL AS TEXT) as err_text, 
        CAST(NULL AS TEXT) as network_bandwidth, 
        CAST(NULL AS TEXT) as cpu_stats, 
        CAST(NULL AS DATE) as total_u_time, 
        CAST(NULL AS DATE) as total_s_time, 
        CAST(NULL AS TEXT) as total_num_threads, 
        CAST(NULL AS TEXT) as gpu_stats, 
        CAST(NULL AS TEXT) as memory_stats, 
        CAST(NULL AS TEXT) as max_memory, 
        CAST(NULL AS TEXT) as max_swap, 
        CAST(NULL AS TEXT) as io_stats, 
        array_to_string(array_agg(sn.node_name),',') as a_nodelist 
    FROM csm_step AS s JOIN csm_step_node AS sn 
        ON s.allocation_id=sn.allocation_id AND s.step_id=sn.step_id
    WHERE s.allocation_id = '**allocation_id**' AND s.step_id = '**step_id**'
    GROUP BY s.allocation_id, s.step_id, s.begin_time
    ORDER BY s.allocation_id, s.step_id 
)
SELECT * FROM s 
UNION ALL (
    SELECT
        sh.history_time, 
        sh.allocation_id, 
        sh.step_id, 
        sh.begin_time, 
        sh.end_time, 
        sh.state as state, 
        sh.executable as executable, 
        sh.working_directory as working_directory, 
        sh.argument as argument, 
        sh.environment_variable as environment_variable, 
        sh.seq_id as seq_id, 
        sh.num_nodes as num_nodes, 
        sh.num_processors as num_processors, 
        sh.num_gpus as num_gpus, 
        sh.num_memory as num_memory, 
        sh.num_tasks as num_tasks, 
        sh.level_gpu_usage as level_gpu_usage, 
        sh.exit_status as exit_status, 
        sh.err_text as err_text, 
        sh.network_bandwidth as network_bandwidth, 
        sh.cpu_stats as cpu_stats, 
        sh.total_u_time as total_u_time, 
        sh.total_s_time as total_s_time, 
        sh.total_num_threads as total_num_threads, 
        sh.gpu_stats as gpu_stats, 
        sh.memory_stats as memory_stats, 
        sh.max_memory as max_memory, 
        sh.max_swap as max_swap, 
        sh.io_stats as io_stats, 
        array_to_string(array_agg(snh.node_name),',') as a_nodelist 
    FROM csm_step_history AS sh JOIN csm_step_node_history AS snh
        ON sh.allocation_id=snh.allocation_id AND sh.step_id=snh.step_id
    WHERE sh.allocation_id = '**allocation_id**' AND sh.step_id = '**step_id**'
    GROUP BY
        sh.history_time, 
        sh.allocation_id, 
        sh.step_id, 
        sh.begin_time, 
        sh.end_time, 
        sh.state, 
        sh.executable, 
        sh.working_directory, 
        sh.argument, 
        sh.environment_variable, 
        sh.seq_id, 
        sh.num_nodes, 
        sh.num_processors, 
        sh.num_gpus, 
        sh.num_memory, 
        sh.num_tasks, 
        sh.level_gpu_usage, 
        sh.exit_status, 
        sh.err_text, 
        sh.network_bandwidth, 
        sh.cpu_stats, 
        sh.total_u_time, 
        sh.total_s_time, 
        sh.total_num_threads, 
        sh.gpu_stats, 
        sh.memory_stats, 
        sh.max_memory, 
        sh.max_swap, 
        sh.io_stats 
    ORDER BY 
        sh.allocation_id, 
        sh.step_id 
);

CSMIAllocationUpdateState.cc
****************************
UPDATE csm_allocation SET state='**newState**' WHERE allocation_id=**allocationId**;

CSMIBBLVCreate.cc
*****************
INSERT INTO csm_lv VALUES (
    '**logical_volume_name**', '**node_name**', **allocation_id**, '**vg_name**',
    '**state**', **current_size**, 0, 'now', 'now', 'file_system_mount', 
    '**file_system_type**'
);

CSMIBBLVDelete.cc
*****************
UPDATE csm_lv SET state='R' updated_time='now'
    WHERE logical_volume_name='**logical_volume_name**' AND allocation_id=**allocationId**;

SELECT fn_csm_lv_history_dump('**logical_volume_name**', '**allocation_id**', 'now', 0, 0 );

CSMIBBLVUpdate.cc
*****************
UPDATE csm_lv SET state='**state**', current_size='**current_size**', updated_time='now'
    WHERE logical_volume_name = '**logical_volume_name**' AND allocation_id = **allocation_id**;


CSMIDiagResultCreate.cc
***********************
INSERT INTO csm_diag_result VALUES ( 
    **run_id**, '**test_name**', '**node_name**',
    p
    p
    '**serial_number**', '**begin_time**', 'now', '**status**', '**log_file**' );

CSMIDiagRunBegin.cc
*******************
INSERT INTO csm_diag_run VALUES (
    **run_id**, **allocation_id**, 'now', '**diag_status**', '0', '**log_dir**', '**cmd_line**' );

CSMIDiagRunEnd.cc
*****************
SELECT fn_csm_diag_run_history_dump( **run_id**, 'now', '**status**', '**inserted_ras**');

CSMIDiagRunQuery.cc
*******************
WITH r AS (
    SELECT 
        CAST(NULL AS DATE) as history_time, 
        r.run_id as run_id, 
        r.allocation_id as allocation_id, 
        r.begin_time as begin_time, 
        CAST(NULL AS DATE) as end_time, 
        r.status as status, 
        r.inserted_ras as inserted_ras, 
        r.log_dir as log_dir, 
        r.cmd_line as cmd_line 
    FROM csm_diag_run as r 
    WHERE r.run_id = **run_id** 
    ORDER BY history_time, run_id 
) 
SELECT * FROM r UNION ALL( 
    SELECT 
        rh.history_time as history_time, 
        rh.run_id as run_id, 
        rh.allocation_id as allocation_id, 
        rh.begin_time as begin_time, 
        rh.history_time as end_time, 
        rh.status as status, 
        rh.inserted_ras as inserted_ras, 
        rh.log_dir as log_dir, 
        rh.cmd_line as cmd_line 
    FROM csm_diag_run_history as rh 
    WHERE rh.run_id = **run_id**
    ORDER BY history_time, run_id 
);

CSMIDiagRunUpdateStatus.cc
**************************
UPDATE csm_diag_run SET diag_status = '**diag_status**' WHERE run_id = **run_id**;

CSMINodeAttributes.cc
*********************
.. todo:: What is this used for? We should probably remove this, because this a mystery.

CSMINodeAttributesQuery.cc
**************************
.. FIXME:: The * needs to be replaced.
SELECT * FROM csm_node WHERE node_name='**nodeName**' [OR...];

CSMINodeAttributesQueryHistory.cc
*********************************
.. FIXME:: The * needs to be replaced.
SELECT * FROM csm_node_history WHERE node_name='**nodeName**';

CSMINodeAttributesUpdate.cc
***************************
UPDATE csm_node SET 
    ready='**ready**', state='**state**', physical_frame_location='**physical_frame_location**',
    physical_u_location='**physical_u_location**', feature_1='**feature_1**', feature_2='**feature_2**',
    feature_3='**feature_3**', feature_4='**feature_4**', comment='**comment**'
WHERE node_name='**node_name**' RETURNING * ; ...


CSMINodeResourcesQueryAll.cc
****************************
SELECT 
    node_name, ready, state, type, hard_power_cap, current_power_cap, 
    current_power_shifting_ratio, installed_memory, available_processors, 
    available_cores, available_smt_threads, available_gpus
FROM csm_node WHERE type='compute';



CSMINodeResourcesQuery.cc
*************************
:Variable Count: n (where n is the number of nodes)

SELECT 
    node_name, ready, state, type, hard_power_cap, current_power_cap, 
    current_power_shifting_ratio, installed_memory, available_processors, 
    available_cores, available_smt_threads, available_gpus
FROM csm_node WHERE node_name='**node_name**' [OR...];

CSMIRasEventCreate.cc
*********************

getMsgTypeSql
=============
:Variable Count: 1

SELECT 
    msg_id,
    min_time_in_pool, 
    suppress_ids, 
    severity, 
    message, 
    description, 
    decoder, 
    control_action, 
    threshold_count, 
    threshold_period, 
    relevant_diags 
FROM csm_ras_type WHERE msg_id= '**msg_id**';
