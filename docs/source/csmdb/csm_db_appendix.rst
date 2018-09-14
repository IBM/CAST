CSM Database Appendix
=====================

Naming conventions
------------------

 *CSM Database Overview*

============= ========================================== ===================================================
Table         | Table names start with "csm" prefix,     | *csm_node_history*
              | example csm_node. History table
              | names add "_history" suffix, example:
Primary Key   | Primary key names are automatically      | *${table name}_pkey*
              | generate within PostgreSQLstartingi      | *csm_node_pkey*
              | with table name and followed by pkey.
Unique Key    | Unique key name start with "uk" followed | *uk_${table name}_b*
              | with table name and a letter indicating  | *uk_csm_allocation_b*
              | the sequence (a, b, c, etc.).
Foreign key   | Foreign key names are automatically      | *${table}_${name_column name\s}_fkey*
              | generate within PostgreSQL starting      | *csm_allocation_node_allocation_id_fkey*
              | with the table name and followed
              | with a list of field(s) and followed
              | by fkey.
Index         | Index name starts with a prefix "ix"     | *ix_${table name}_a*
              | followed by a table name and a letter    | *ix_csm_node_history_a*
              | indicating the sequence.(a, b, c, etc.).
Functions     | Function names will start with a prefix  | *fn_function_name_purpose*
              | with the prefix "fn" and followed by a   | *fn_csm_allocation_history_dump*
              | name, usually related to the table and   |
              | purpose or arguments if any.
Triggers      | Trigger names will start with a prefix   | *tr_trigger_name_purpose*
              | with the prefix "tr" and followed by a
              | name, usually related to the table and
              | purpose.
============= ========================================== ===================================================

.. _history_tables:

History Tables
^^^^^^^^^^^^^^

 CSM DB keeps track of data as it change over time. History tables will be used to store these records and a history time stamp is generated to indicate the transaction has completed. The information will remain in this table until further action is taken.

Usage and Size
^^^^^^^^^^^^^^

 The usage and size of each table will vary depending on system size and system activity.  This document tries to estimate the usage and size of the tables. Usage is defined as how often a table is accessed and is recorded as ``Low``, ``Medium``, or ``High``.  Size indicates how many rows are within the database tables and is recorded as total number of rows.  

Table Categories
^^^^^^^^^^^^^^^^

 The CSM database tables are grouped and color coordinated to demonstrate which category they belong to within the schema.  These categories include, 

 * `Node attributes tables`_
 * `Allocation tables`_
 * `Step tables`_
 * `Allocation node, allocation state history, step node tables`_
 * `RAS tables`_
 * `CSM diagnostic tables`_
 * `SSD partition and SSD logical volume tables`_
 * `Switch & ib cable tables`_
 * `CSM configuration tables`_
 * `CSM DB schema version tables`_
	
Tables
------

Node attributes tables
^^^^^^^^^^^^^^^^^^^^^^

csm_node
""""""""

**Description**
 
 This table contains the attributes of all the nodes in the CORAL system including: *management node*, *service node*, *login node*, *work load manager*, *launch node*, and *compute node*.

=========== ============================================= ==========================
 Table      Overview                                             Action On:
=========== ============================================= ==========================
 Usage      | High (CSM APIs access this table regularly) |
 Size       | 1-5000 rows (total nodes in a CORAL System) |
 Key(s)     | PK: node_name                               |
 Index      | csm_node_pkey on (node_name)                |
            | ix_csm_node_a on (node_name, ready)         |
 Functions  | fn_csm_node_ready                           |
            | fn_csm_node_update                          |
            | fn_csm_node_delete                          |
 Triggers   | tr_csm_node_ready on (csm_node)             | update/delete
            | tr_csm_node_update                          | update/delete
=========== ============================================= ==========================

+-----------------------+------------------------------------+-----------+-------+
| *Referenced by table* |           *Constraint*             | *Fields*  | *Key* |
+=======================+====================================+===========+=======+
| csm_allocation_node   | csm_allocation_node_node_name_fkey | node_name | (FK)  |
+-----------------------+------------------------------------+-----------+-------+
| csm_dimm              | csm_dimm_node_name_fkey            | node_name | (FK)  |
+-----------------------+------------------------------------+-----------+-------+
| csm_gpu               | csm_gpu_node_name_fkey             | node_name | (FK)  |
+-----------------------+------------------------------------+-----------+-------+
| csm_hca               | csm_hca_node_name_fkey             | node_name | (FK)  |
+-----------------------+------------------------------------+-----------+-------+
| csm_processor         | csm_processor_node_name_fkey       | node_name | (FK)  |
+-----------------------+------------------------------------+-----------+-------+
| csm_ssd               | csm_ssd_node_name_fkey             | node_name | (FK)  |
+-----------------------+------------------------------------+-----------+-------+

csm_node_history
""""""""""""""""

**Description**
 This table contains the historical information related to node attributes.

=========== ============================================= ==========================
 Table      Overview                                      Action On:
=========== ============================================= ==========================
 Usage      | Low (When hardware changes and to query     |
            | historical information)                     |
 Size       | 5000+ rows (Based on hardware changes)      |
 Index      | ix_csm_node_history_a on (history_time)     |
            | ix_csm_node_history_a on (history_time)     |
=========== ============================================= ==========================
 
csm_node_ready_history
""""""""""""""""""""""

**Description**
 This table contains historical information related to the node ready status.  This table will be updated each time the node ready status changes.

=========== =================================================== ==========================
 Table      Overview                                            Action On:
=========== =================================================== ==========================
 Usage      | Med-High                                          |
 Size       | (Based on how often a node ready status changes)  |
 Index      | ix_csm_node_ready_history_a on (history_time)     |
            | ix_csm_node_ready_history_b on (node_name, ready) |
=========== =================================================== ==========================  
 
csm_processor
"""""""""""""

**Description**
 This table contains information on the processors of a node.

=========== ================================================== ==========================
 Table      Overview                                           Action On:
=========== ================================================== ==========================
 Usage      | Low                                              |
 Size       | 25,000+ rows (Witherspoon will consist of        |
            | 256 processors per node. (based on 5000 nodes)   |
 Key(s)     | PK: serial_number                                |
            | FK: csm_node (node_name)                         |
 Index      | csm_processor_pkey on (serial_number)            |
            | ix_csm_processor_a on (serial_number, node_name) |
 Functions  | fn_csm_processor_history_dump                    |
 Triggers   | tr_csm_processor_history_dump                    | update/delete
=========== ================================================== ==========================   

csm_processor_history
"""""""""""""""""""""

**Description**
 This table contains historical information associated with individual processors.

=========== ========================================================== ==============
 Table      Overview                                                   Action On:
=========== ========================================================== ==============
 Usage      | Low                                                      |
 Size       | 25,000+ rows (Based on how often a processor             |
            | is changed or its failure rate)                          |
 Index      | ix_csm_processor_history_a on (history_time)             |
            | ix_csm_processor_history_b on (serial_number, node_name) |
=========== ========================================================== ==============

csm_gpu
"""""""

**Description**
 This table contains information on the GPUs on the node.


=========== ============================================= ==========================
 Table      Overview                                      Action On:
=========== ============================================= ==========================
 Usage      | Low                                         |
 Size       | 30,000+ rows                                |
            | (Max per load =                             |
            | 6 (If there are 5000 nodes than             |
            | 30,000 on Witherspoons)                     |
 Key(s)     | PK: node_name, gpu_id                       |
            | FK: csm_node (node_name)                    |
 Index      | csm_gpu_pkey on (node_name, gpu_id)         |
 Functions  | fn_csm_gpu_history_dump                     |
 Triggers   | tr_csm_gpu_history_dump                     | update/delete
=========== ============================================= ==========================

csm_gpu_history
"""""""""""""""

**Description**
 This table contains historical information associated with individual GPUs. The GPU will be recorded and also be timestamped.

=========== ============================================= ==========================
 Table      Overview                                      Action On:
=========== ============================================= ==========================
 Usage      | Low                                         |
 Size       | (based on how often changed)                |
 Index      | ix_csm_gpu_history_a on (history_time)      |
            | ix_csm_gpu_history_b on (serial_number)     |
=========== ============================================= ==========================   
 
csm_ssd
"""""""

**Description**
 This table contains information on the SSDs on the system. This table contains the current status of the SSD along with its capacity and wear.

=========== ============================================= ==========================
 Table      Overview                                      Action On:
=========== ============================================= ==========================
 Usage      | Medium                                      |
 Size       | 1-5000 rows (one per node)                  |
 Key(s)     | PK: serial_number                           |
            | FK: csm_node (node_name)                    |
 Index      | csm_ssd_pkey on (serial_number)             |
            | ix_csm_ssd_a on (serial_number, node_name)  |
 Functions  | fn_csm_ssd_history_dump                     |
 Triggers   | tr_csm_ssd_history_dump                     | update/delete
=========== ============================================= ==========================  
 
+-----------------------+----------------------------------------+--------------------------+-------+
| *Referenced by table* |           *Constraint*                 | *Fields*                 | *Key* |
+=======================+========================================+==========================+=======+
| csm_vg_ssd            | csm_vg_ssd_serial_number_fkey          | serial_number, node_name | (FK)  |
+-----------------------+----------------------------------------+--------------------------+-------+

csm_ssd_history
"""""""""""""""

**Description**
 This table contains historical information associated with individual SSDs.

=========== ==================================================== ==========================
 Table      Overview                                             Action On:
=========== ==================================================== ==========================
 Usage      | Low                                                |
 Size       | 5000+ rows                                         |
 Index      | ix_csm_ssd_history_a on (history_time)             |
            | ix_csm_ssd_history_b on (serial_number, node_name) |
=========== ==================================================== ==========================

csm_hca
"""""""

**Description**
 This table contains information about the HCA (Host Channel Adapters).  Each HC adapter has a unique identifier (serial number).  The table has a status indicator, board ID (for the IB adapter), and Infiniband (globally unique identifier (GUID)).

=========== ============================================= ==========================
 Table      Overview                                      Action On:
=========== ============================================= ==========================
 Usage      | Low                                         |
 Size       | 1-10K – 1 or 2 per node                     |
 Key(s)     | PK: serial_number                           |
            | FK: csm_node (node_name)                    |        
 Index      | csm_hca_pkey on (serial_number)             |
 Functions  | fn_csm_hca_history_dump                     |
 Triggers   | tr_csm_hca_history_dump                     | update/delete
=========== ============================================= ==========================   

csm_hca_history
"""""""""""""""

**Description**
 This table contains historical information associated with the HCA (Host Channel Adapters).

=========== ============================================= ==========================
 Table      Overview                                      Action On:
=========== ============================================= ==========================
 Usage      | Low                                         |
 Size       | (Based on how many are changed out)         |
 Key(s)     |                                             |
 Index      | ix_csm_hca_history_a on (history_time)      |
=========== ============================================= ==========================

csm_dimm
""""""""

**Description**
 This table contains information related to the DIMM “"Dual In-Line Memory Module” attributes.

=========== ============================================= ==========================
 Table      Overview                                      Action On:
=========== ============================================= ==========================
 Usage      | Low                                         |
 Size       | 1-80K+ (16 DIMMs per node)                  |
 Key(s)     | PK: serial_number                           |
            | FK: csm_node (node_name)                    |
 Index      | csm_dimm_pkey on (serial_number)            |
 Functions  | fn_csm_dimm_history_dum                     |
 Triggers   | tr_csm_dimm_history_dump                    | update/delete
=========== ============================================= ==========================   

csm_dimm_history
""""""""""""""""

**Description** 
 This table contains historical information related to the DIMM "Dual In-Line Memory Module" attributes.

=========== ============================================= ==========================
 Table      Overview                                      Action On:
=========== ============================================= ==========================
 Usage      | Low                                         |
 Size       | (Based on how many are changed out)         |
 Index      | ix_csm_dimm_history_a on (history_time)     |
=========== ============================================= ==========================  

Allocation tables
^^^^^^^^^^^^^^^^^

csm_allocation
""""""""""""""

**Description**
 This table contains the information about the system’s current allocations. Specific attributes include: primary job ID, secondary job ID, user and system flags, number of nodes, state, username, start time stamp, power cap, power shifting ratio, authorization token, account, comments, eligible, job name, reservation, Wall clock time reservation, job_submit_time, queue, time_limit, WC Key, type.

=========== =========================================================== ==========================
 Table      Overview                                                    Action On:
=========== =========================================================== ==========================
 Usage      | High (Every time allocated and allocation query)          |
 Size       | 1-5000 rows (1 allocation per node (5000 max per 1 node)) |
 Key(s)     | PK: allocation_id                                         |
 Index      | csm_allocation_pkey on (allocation_id)                    |
 Functions  | fn_csm_allocation_history_dump                            | insert/update/delete (API call)
            | fn_csm_allocation_state_history_state_change              |
            | fn_csm_allocation_update                                  |
 Triggers   | tr_csm_allocation_state_change			        | delete
            | tr_csm_allocation_update				        | update
=========== =========================================================== ==========================  
 
+-----------------------+----------------------------------------+---------------+-------+
| *Referenced by table* |           *Constraint*                 | *Fields*      | *Key* |
+=======================+========================================+===============+=======+
| csm_allocation_node   | csm_allocation_node_allocation_id_fkey | allocation_id | (FK)  |
+-----------------------+----------------------------------------+---------------+-------+
| csm_step              | csm_step_allocation_id_fkey            | allocation_id | (FK)  |
+-----------------------+----------------------------------------+---------------+-------+

csm_allocation_history
""""""""""""""""""""""

**Description**
 This table contains the information about the no longer current allocations on the system.  Essentially this is the historical information about allocations. This table will increase in size only based on how many allocations are deployed on the life cycle of the machine/system.  This table will also be able to determine the total energy consumed per allocation (filled in during "free of allocation").

=========== ==================================================== ==========================
 Table      Overview                                             Action On:
=========== ==================================================== ==========================
 Usage      | High                                               |
 Size       | (Depending on customers work load (100,000+ rows)) |
 Index      | ix_csm_allocation_history_a on (history_time)      |
=========== ==================================================== ==========================

Step tables
^^^^^^^^^^^

csm_step
""""""""

**Description**
 This table contains information on active steps within the CSM database.  Featured attributes include: step id, allocation id, begin time, state, executable, working directory, arguments, environment variables, sequence ID, number of nodes, number of processes (that can run on each compute node), number of GPU’s, number of memory, number of tasks, user flags, system flags, and launch node name.

=========== ============================================= ==========================
 Table      Overview                                      Action On:
=========== ============================================= ==========================
 Usage      | High                                        |
 Size       | 5000+ rows (depending on the steps)         |
 Key(s)     | PK: step_id, allocation_id                  |
            | FK: csm_allocation (allocation_id)	  |
 Index      | csm_step_pkey on (step_id, allocation_id)   |
            | uk_csm_step_a on (step_id, allocation_id)   |
 Functions  | fn_csm_step_history_dump                    | insert/update/delete (API call)
=========== ============================================= ==========================  
 
+-----------------------+---------------------------------------+-----------+-------+
| *Referenced by table* |           *Constraint*                | *Fields*  | *Key* |
+=======================+=======================================+===========+=======+
| csm_step_node         | csm_step_node_step_id_fkey            | step_id   | (FK)  |
+-----------------------+---------------------------------------+-----------+-------+

csm_step_history
""""""""""""""""

**Description**
 This table contains the information for steps that have terminated.  There is some additional information from the initial step that has been added to the history table.  These attributes include: end time, compute nodes, level gpu usage, exit status, error text, network band width, cpu stats, total U time, total S time, total number of threads, gpu stats, memory stats, max memory, max swap, ios stats.

=========== ========================================================== ==========================
 Table      Overview                                                   Action On:
=========== ========================================================== ==========================
 Usage      | High                                                     |
 Size       | Millions of rows (depending on the customer’s work load) |
 Index      | ix_csm_step_history_a on (history_time)                  |
            | ix_csm_step_history_b on (begin_time, end_time)          |
            | ix_csm_step_history_c on (allocation_id, end_time)       |
            | ix_csm_step_history_d on (end_time)                      |
            | ix_csm_step_history_e on (step_id)                       |
=========== ========================================================== ==========================

Allocation node, allocation state history, step node tables
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

csm_allocation_node
"""""""""""""""""""

**Description**
 This table maps current allocations to the compute nodes that make up the allocation.  This information is later used when populating the csm_allocation_history table.

=========== ======================================================== ====================
 Table      Overview                                                 Action On:
=========== ======================================================== ====================
 Usage      | High                                                   |
 Size       | 1-5000 rows                                            |
 Key(s)     | FK: csm_node (node_name)                               |
            | FK: csm_allocation (allocation_id)                     |
 Index      | ix_csm_allocation_node_a on (allocation_id)            |
            | uk_csm_allocation_node_b on (allocation_id, node_name) | insert (API call)
 Functions  | fn_csm_allocation_node_sharing_status                  | 
            | fn_csm_allocation_node_change                          |
 Triggers   | tr_csm_allocation_node_change                          | update
=========== ======================================================== ====================
 
+-----------------------+---------------------------------------+--------------------------+-------+
| *Referenced by table* |           *Constraint*                | *Fields*                 | *Key* |
+=======================+=======================================+==========================+=======+
| csm_lv                | csm_lv_allocation_id_fkey             | allocation_id, node_name | (FK)  |
+-----------------------+---------------------------------------+--------------------------+-------+
| csm_step_node         | csm_step_node_allocation_id_fkey      | allocation_id, node_name | (FK)  |
+-----------------------+---------------------------------------+--------------------------+-------+

csm_allocation_node_history
"""""""""""""""""""""""""""

**Description**
 This table maps history allocations to the compute nodes that make up the allocation.

=========== ==================================================== ==========================
 Table      Overview                                             Action On:
=========== ==================================================== ==========================
 Usage      | High                                               |
 Size       | 1-5000 rows                                        |
 Index      | ix_csm_allocation_node_history_a on (history_time) |
=========== ==================================================== ==========================

csm_allocation_state_history
""""""""""""""""""""""""""""

**Description**
 This table contains the state of the active allocations history. A timestamp of when the information enters the table along with a state indicator.

=========== ===================================================== ==========================
 Table      Overview                                              Action On:
=========== ===================================================== ==========================
 Usage      | High                                                |
 Size       | 1-5000 rows (one per allocation)                    |
 Index      | ix_csm_allocation_state_history_a on (history_time) |
=========== ===================================================== ==========================

csm_step_node
"""""""""""""

**Description**
 This table maps active allocations to jobs steps and nodes.

=========== =========================================================== ==============
 Table      Overview                                                    Action On:
=========== =========================================================== ==============
 Usage      | High                                                      |
 Size       | 5000+ rows (based on steps)                               |
 Key(s)     | FK: csm_step (step_id, allocation_id)                     |
            | FK: csm_allocation (allocation_id, node_name)             |
 Index      | uk_csm_step_node_a on (step_id, allocation_id, node_name) |
 Functions  | fn_csm_step_node_history_dump                             |
 Triggers   | tr_csm_step_node_history_dump                             | delete
=========== =========================================================== ==============

csm_step_node_history
"""""""""""""""""""""

**Description**
 This table maps historical allocations to jobs steps and nodes.

=========== ============================================== ==========================
 Table      Overview                                       Action On:
=========== ============================================== ==========================
 Usage      | High                                         |
 Size       | 5000+ rows (based on steps)                  |
 Index      | ix_csm_step_node_history_a on (history_time) |
=========== ============================================== ==========================

RAS tables
^^^^^^^^^^

csm_ras_type
""""""""""""

**Description**
 This table contains the description and details for each of the possible RAS event types.  Specific attribute in this table include: msg_id, severity, message, description, control_action, threshold_count, threshold_period, enabled, set_not_ready, set_ready, viable_to_users.

=========== =================================================== ==========================
 Table      Overview                                            Action On:
=========== =================================================== ==========================
 Usage      | Low                                               |
 Size       | 1000+ rows (depending on the different RAS types) |
 Key(s)     | PK: msg_id                                        |
 Index      | csm_ras_type_pkey on (msg_id)                     |
 Functions  | fn_csm_ras_type_update                            |
 Triggers   | tr_csm_ras_type_updat                             | insert/update/delete
=========== =================================================== ==========================

csm_ras_type_audit
""""""""""""""""""

**Description**
 This table contains historical descriptions and details for each of the possible RAS event types.  Specific attribute in this table include: msg_id_seq, operation, change_time, msg_id, severity, message, description, control_action, threshold_count, threshold_period, enabled, set_not_ready, set_ready, visible_to_users.

=========== =================================================== ==========================
 Table      Overview                                            Action On:
=========== =================================================== ==========================
 Usage      | Low                                               |
 Size       | 1000+ rows (depending on the different RAS types) |
 Key(s)     | PK: msg_id_seq                                    |
 Index      | csm_ras_type_audit_pkey on (msg_id_seq)           |
=========== =================================================== ==========================

+-----------------------+---------------------------------------+------------+-------+
| *Referenced by table* |           *Constraint*                | *Fields*   | *Key* |
+=======================+=======================================+============+=======+
| csm_ras_event_action  | csm_ras_event_action_msg_id_seq_fkey  | msg_id_seq | (FK)  |
+-----------------------+---------------------------------------+------------+-------+

.. _csm_ras_event_action:

csm_ras_event_action
""""""""""""""""""""

**Description**
 This table contains all RAS events.  Key attributes that are a part of this table include: rec id, msg id, msg_id_seq, timestamp, count, message, and raw data.  This table will populate an enormous amount of records due to continuous event cycle.  A solution needs to be in place to accommodate the mass amount of data produced.

=========== ========================================================== ==========================
 Table      Overview                                                   Action On:
=========== ========================================================== ==========================
 Usage      | High                                                     |
 Size       | Million ++ rows                                          |
 Key(s)     | PK: rec_id                                               |
            | FK: csm_ras_type (msg_id_seq)                            |
 Index      | csm_ras_event_action_pkey on (rec_id)                    |
            | ix_csm_ras_event_action_a on (msg_id)                    |
            | ix_csm_ras_event_action_b on (time_stamp)                |
            | ix_csm_ras_event_action_c on (location_name)             |
            | ix_csm_ras_event_action_d on (time_stamp, msg_id)        |
            | ix_csm_ras_event_action_e on (time_stamp, location_name) |
=========== ========================================================== ==========================

CSM diagnostic tables
^^^^^^^^^^^^^^^^^^^^^

csm_diag_run
""""""""""""

**Description**
 This table contains information about each of the diagnostic runs. Specific attributes including: run id, allocation_id, begin time, status, inserted RAS, log directory, and command line.

=========== ============================================= ==========================
 Table      Overview                                      Action On:
=========== ============================================= ==========================
 Usage      | Low                                         |
 Size       | 1000+ rows                                  |
 Key(s)     | PK: run_id                                  |
 Index      | csm_diag_run_pkey on (run_id)               |
 Functions  | fn_csm_diag_run_history_dump                | insert/update/delete (API call)
=========== ============================================= ==========================  
 
+-----------------------+---------------------------------------+-----------+-------+
| *Referenced by table* |           *Constraint*                | *Fields*  | *Key* |
+=======================+=======================================+===========+=======+
| csm_diag_result       | csm_diag_result_run_id_fkey           | run_id    | (FK)  |
+-----------------------+---------------------------------------+-----------+-------+

csm_diag_run_history
""""""""""""""""""""

**Description**
 This table contains historical information about each of the diagnostic runs. Specific attributes including: run id, allocation_id, begin time, end_time, status, inserted RAS, log directory, and command line.

=========== ============================================= ==========================
 Table      Overview                                      Action On:
=========== ============================================= ==========================
 Usage      | Low                                         |
 Size       | 1000+ rows                                  |
 Index      | ix_csm_diag_run_history_a on (history_time) |
=========== ============================================= ==========================  
  
csm_diag_result
"""""""""""""""

**Description**
 This table contains the results of a specific instance of a diagnostic.

=========== ======================================================== ==============
 Table      Overview                                                 Action On:
=========== ======================================================== ==============
 Usage      | Low                                                    |
 Size       | 1000+ rows                                             |
 Key(s)     | FK: csm_diag_run (run_id)                              |
 Index      | ix_csm_diag_result_a on (run_id, test_case, node_name) |
 Functions  | fn_csm_diag_result_history_dump                        |
 Triggers   | tr_csm_diag_result_history_dump                        | delete
=========== ======================================================== ==============

csm_diag_result_history
"""""""""""""""""""""""

**Description**
 This table contains historical results of a specific instance of a diagnostic.

=========== ================================================ ==========================
 Table      Overview                                         Action On:
=========== ================================================ ==========================
 Usage      | Low                                            |
 Size       | 1000+ rows                                     |
 Index      | ix_csm_diag_result_history_a on (history_time) |
=========== ================================================ ==========================

SSD partition and SSD logical volume tables
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

csm_lv
""""""

**Description**
 This table contains information about the logical volumes that are created within the compute nodes.

=========== ================================================= ==========================
 Table      Overview                                          Action On:
=========== ================================================= ==========================
 Usage      | Medium                                          |
 Size       | 5000+ rows (depending on SSD usage)             |
 Key(s)     | PK: logical_volume_name, node_name              |
            | FK: csm_allocation (allocation_id)              |                                            
            | FK: csm_vg (node_name, vg_name)                 |
 Index      | csm_lv_pkey on (logical_volume_name, node_name) |
            | ix_csm_lv_a on (logical_volume_name)            |
 Functions  | fn_csm_lv_history_dump                          | insert/update/delete (API call)
            | fn_csm_lv_modified_history_dump                 |
            | fn_csm_lv_update_history_dump                   |
 Triggers   | tr_csm_lv_modified_history_dump                 | update
            | tr_csm_lv_update_history_dump                   | update
=========== ================================================= ==========================

csm_lv_history
""""""""""""""

**Description**
 This table contains historical information associated with previously active logical volumes.

=========== ============================================== ==========================
 Table      Overview                                       Action On:
=========== ============================================== ==========================
 Usage      | Medium                                       |
 Size       | 5000+ rows (depending on step usage)         |
 Index      | ix_csm_lv_history_a on (history_time)        |
            | ix_csm_lv_history_b on (logical_volume_name) |
=========== ============================================== ==========================  

csm_lv_update_history
"""""""""""""""""""""

**Description**
 This table contains historical information associated with lv updates.

=========== ===================================================== ========================
 Table      Overview                                               Action On:
=========== ===================================================== ========================
 Usage      | Medium                                              |
 Size       | 5000+ rows (depending on step usage)                |
 Index      | ix_csm_lv_update_history_a on (history_time)        |
            | ix_csm_lv_update_history_b on (logical_volume_name) |
=========== ===================================================== ========================

csm_vg_ssd
""""""""""

**Description**
 This table contains information that references both the SSD logical volume tables.

=========== ======================================================== ==========================
 Table      Overview                                                 Action On:
=========== ======================================================== ==========================
 Usage      | Medium                                                 |
 Size       | 5000+ rows (depending on SSD usage)                    |
 Key(s)     | FK: csm_ssd (serial_number, node_name)                 |
 Index      | csm_vg_ssd_pkey on (vg_name, node_name, serial_number) |
            | ix_csm_vg_ssd_a on (vg_name, node_name, serial_number) |
            | uk_csm_vg_ssd_a on (vg_name, node_name)                |
 Functions  | fn_csm_vg_ssd_history_dump                             |
 Triggers   | tr_csm_vg_ssd_history_dump                             | update/delete
=========== ======================================================== ==========================

csm_vg_ssd_history
""""""""""""""""""

**Description**
 This table contains historical information associated with SSD and logical volume tables.

=========== ============================================= ==========================
 Table      Overview                                      Action On:
=========== ============================================= ==========================
 Usage      | Medium                                      |
 Size       | 5000+ rows (depending on step usage)        |
 Index      | ix_csm_vg_ssd_history_a on (history_time)   |
=========== ============================================= ==========================

csm_vg
""""""

**Description**
 This table contains information that references both the SSD logical volume tables.

=========== ============================================= ==========================
 Table      Overview                                      Action On:
=========== ============================================= ==========================
 Usage      | Medium                                      |
 Size       | 5000+ rows (depending on step usage)        |
 Key(s)     | PK: vg_name, node_name                      |
            | FK: csm_node (node_name)                    |
 Index      | csm_vg_pkey on (vg_name, node_name)         |
 Functions  | fn_csm_vg_history_dump                      |
 Triggers   | tr_csm_vg_history_dump                      | update/delete
=========== ============================================= ==========================  

+-----------------------+--------------------------+--------------------+-------+
| *Referenced by table* |     *Constraint*         | *Fields*           | *Key* |
+=======================+==========================+====================+=======+
| csm_lv                | csm_lv_node_name_fkey    | node_name, vg_name | (FK)  |
+-----------------------+--------------------------+--------------------+-------+

csm_vg_history
""""""""""""""

**Description**
 This table contains historical information associated with SSD and logical volume tables.

=========== ============================================= ==========================
 Table      Overview                                      Action On:
=========== ============================================= ==========================
 Usage      | Medium                                      |
 Size       | 5000+ rows (depending on step usage)        |
 Index      | ix_csm_vg_history_a on (history_time)       |
=========== ============================================= ==========================

Switch & ib cable tables
^^^^^^^^^^^^^^^^^^^^^^^^

csm_switch
""""""""""

**Description**
 This table contain information about the switch and it attributes including; switch_name, discovery_time, collection_time, comment, description, fw_version, gu_id, has_ufm_agent, ip, model, num_modles, num_ports, physical_frame_location, physical_u_location, ps_id, role, server_operation_mode, sm_version, system_guid, system_name, total_alarms, type, and vendor.

=========== ============================================= ==========================
 Table      Overview                                      Action On:
=========== ============================================= ==========================
 Usage      | Low                                         |
 Size       | 500 rows (Switches on a CORAL system)       |
 Key(s)     | PK: switch_name                             |
 Index      | csm_switch_pkey on (serial_number)          |
 Functions  | fn_csm_switch_history_dump                  |
 Triggers   | tr_csm_switch_history_dump                  | update/delete
=========== ============================================= ==========================  
 
+-----------------------+--------------------------------------------+------------------+-------+
| *Referenced by table* |           *Constraint*                     | *Fields*         | *Key* |
+=======================+============================================+==================+=======+
| csm_switch_inventory  | csm_switch_inventory_host_system_guid_fkey | host_system_guid | (FK)  |
+-----------------------+--------------------------------------------+------------------+-------+

csm_switch_history
""""""""""""""""""

**Description**
 This table contains historical information associated with individual switches.

=========== ========================================================== ==========================
 Table      Overview                                                   Action On:
=========== ========================================================== ==========================
 Usage      | Low                                                      |
 Size       | (Based on failure rate/ or how often changed out)        |
 Index      | ix_csm_switch_history_a on (history_time)                |
            | ix_csm_switch_history_b on (serial_number, history_time) |
=========== ========================================================== ==========================

csm_ib_cable
""""""""""""

**Description**
 This table contains information about the InfiniBand cables including; serial_number, discovery_time, collection_time, comment, guid_s1, guid_s2, identifier, length, name, part_number, port_s1, port_s2, revision, severity, type, and width.

=========== ============================================= ==========================
 Table      Overview                                      Action On:
=========== ============================================= ==========================
 Usage      | Low                                         |
 Size       | 25,000+ rows (Based on switch topology and  |
            | or configuration)                           |
 Key(s)     | PK: serial_number                           |
 Index      | csm_ib_cable_pkey on (serial_number)        |
 Functions  | fn_csm_ib_cable_history_dump                |
 Triggers   | tr_csm_ib_cable_history_dump                | update/delete
=========== ============================================= ==========================

csm_ib_cable_history
""""""""""""""""""""

**Description**
 This table contains historical information about the InfiniBand cables.

=========== ============================================= ==========================
 Table      Overview                                      Action On:
=========== ============================================= ==========================
 Usage      | Low                                         |
 Size       | 25,000+ rows (Based on switch topology and  |
            | or configuration)                           |
 Index      | ix_csm_ib_cable_history_a on (history_time) |
=========== ============================================= ==========================

csm_switch_inventory
""""""""""""""""""""

**Description**
 This table contains information about the switch inventory including; name, host_system_guid, discovery_time, collection_time, comment, description, device_name, device_type, max_ib_ports, module_index, number_of_chips, path, serial_number, severity, and status.

=========== ============================================= ==========================
 Table      Overview                                      Action On:
=========== ============================================= ==========================
 Usage      | Low                                         |
 Size       | 25,000+ rows (Based on switch topology and  |
            | or configuration)                           |
 Key(s)     | PK: name                                    |
            | FK: csm_switch (switch_name)                |
 Index      | csm_switch_inventory_pkey on (name)         |
 Functions  | fn_csm_switch_inventory_history_dump        |
 Triggers   | tr_csm_switch_inventory_history_dump        | update/delete
=========== ============================================= ==========================

csm_switch_inventory_history
""""""""""""""""""""""""""""

**Description**
 This table contains historical information about the switch inventory. 

=========== ============================================================= ==========================
 Table      Overview                                                      Action On:
=========== ============================================================= ==========================
 Usage      | Low                                                         |
 Size       | 25,000+ rows (Based on switch topolog and or configuration) |
 Index      | ix_csm_switch_inventory_history_a on (history_time)         |
=========== ============================================================= ==========================  
 
csm_switch_ports
""""""""""""""""

**Description**
 This table contains information about the switch ports including; name, parent, discovery_time, collection_time, active_speed, comment, description, enabled_speed, external_number, guid, lid, max_supported_speed, logical_state, mirror, mirror_traffic, module, mtu, number, physical_state, peer, severity, supported_speed, system_guid, tier, width_active, width_enabled, and width_supported.

=========== ============================================= ==========================
 Table      Overview                                      Action On:
=========== ============================================= ==========================
 Usage      | Low                                         |
 Size       | 25,000+ rows (Based on switch topology and  |
            | or configuration)                           |
 Key(s)     | PK: name                                    |
            | FK: csm_switch (switch_name)                |
 Index      | csm_switch_ports_pkey on (name)             |
 Functions  | fn_csm_switch_ports_history_dump            |
 Triggers   | tr_csm_switch_ports_history_dump            | update/delete
=========== ============================================= ==========================

csm_switch_ports_history
""""""""""""""""""""""""

**Description**
 This table contains historical information about the switch ports.

=========== ============================================= ==========================
 Table      Overview                                      Action On:
=========== ============================================= ==========================
 Usage      | Low                                         |
 Size       | 25,000+ rows (Based on switch topology and  |
            | or configuration)                           |
 Index      | ix_csm_switch_ports_history_a on            |
            | (history_time)                              |
=========== ============================================= ==========================

CSM configuration tables
^^^^^^^^^^^^^^^^^^^^^^^^

csm_config
""""""""""

**Description**
 This table contains information about the CSM configuration.

=========== ============================================= ==========================
 Table      Overview                                      Action On:
=========== ============================================= ==========================
 Usage      | Medium                                      |
 Size       | 1 row (Based on configuration changes)      |
 Key(s)     | PK: config_id                               |
 Index      | csm_config_pkey on (csm_config_id)          |
 Functions  | fn_csm_config_history_dump                  |
 Triggers   | tr_csm_config_history_dump                  | update/delete
=========== ============================================= ==========================  
 
csm_config_history
""""""""""""""""""

**Description**
 This table contains historical information about the CSM configuration.

=========== ============================================= ==========================
 Table      Overview                                      Action On:
=========== ============================================= ==========================
 Usage      | Medium                                      |
 Size       | 1-100 rows                                  |
 Index      | ix_csm_config_history_a on (history_time)   |
=========== ============================================= ==========================   

csm_config_bucket
"""""""""""""""""

**Description**
 This table is the list of items that will placed in the bucket.  Some of the attributes include: bucket id, item lists, execution interval, and time stamp.

=========== ============================================= ==========================
 Table      Overview                                      Action On:
=========== ============================================= ==========================
 Usage      | Medium                                      |
 Size       | 1-400 rows (Based on configuration changes) |
 Index      | ix_csm_config_bucket_a on                   |
            | (bucket_id, item_list, time_stamp)          |
=========== ============================================= ==========================  
 
CSM DB schema version tables
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

csm_db_schema_version
"""""""""""""""""""""

**Description**
 This is the current database schema version when loaded.

=========== ====================================================== ==========================
 Table      Overview                                               Action On:
=========== ====================================================== ==========================
 Usage      | Low                                                  |
 Size       | 1-100 rows (Based on CSM DB changes)                 |
 Key(s)     | PK: version                                          |
 Index      | csm_db_schema_version_pkey on (version)              |
            | ix_csm_db_schema_version_a on (version, create_time) |
 Functions  | fn_csm_db_schema_version_history_dump                |
 Triggers   | tr_csm_db_schema_version_history_dump                | update/delete
=========== ====================================================== ==========================

csm_db_schema_version_history
"""""""""""""""""""""""""""""

**Description**
 This is the historical database schema version (if changes have been made)

=========== ===================================================== ==========================
 Table      Overview                                              Action On:
=========== ===================================================== ==========================
 Usage      | Low                                                 |
 Size       | 1-100 rows (Based on CSM DB changes/updates)        |
 Index      | ix_csm_db_schema_version_history_a on history_time) |
=========== ===================================================== ==========================

PK, FK, UK keys and Index Charts
--------------------------------

Primary Keys (default Indexes)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
+---------------------------+-----------------------+---------------+-----------------------------------+
| Name	                    | Table	            | Index on	    | Description                       | 
+===========================+=======================+===============+===================================+
| csm_allocation_pkey       | csm_allocation        | pkey index on | allocation_id                     |
+---------------------------+-----------------------+---------------+-----------------------------------+
| csm_config_pkey	    | csm_config            | pkey index on | csm_config_id                     |
+---------------------------+-----------------------+---------------+-----------------------------------+
| csm_db_schema_version_pkey| csm_db_schema_version | pkey index on | version                           |
+---------------------------+-----------------------+---------------+-----------------------------------+
| csm_diag_run_pkey         | csm_diag_run          | pkey index on | run_id                            |
+---------------------------+-----------------------+---------------+-----------------------------------+
| csm_dimm_pkey             | csm_dimm              | pkey index on | serial_number                     |
+---------------------------+-----------------------+---------------+-----------------------------------+
| csm_gpu_pkey              | csm_gpu               | pkey index on | node_name, gpu_id                 |
+---------------------------+-----------------------+---------------+-----------------------------------+
| csm_hca_pkey              | csm_hca               | pkey index on | node_name, serial_number          |
+---------------------------+-----------------------+---------------+-----------------------------------+
| csm_ib_cable_pkey         | csm_ib_cable          | pkey index on | serial_number                     |
+---------------------------+-----------------------+---------------+-----------------------------------+
| csm_lv_pkey               | csm_lv                | pkey index on | logical_volume_name, node_name    |
+---------------------------+-----------------------+---------------+-----------------------------------+
| csm_node_pkey             | csm_node              | pkey index on | node_name                         |
+---------------------------+-----------------------+---------------+-----------------------------------+
| csm_processor_pkey        | csm_processor         | pkey index on | serial_number                     |
+---------------------------+-----------------------+---------------+-----------------------------------+
| csm_ras_event_action_pkey | csm_ras_event_action  | pkey index on | rec_id                            |
+---------------------------+-----------------------+---------------+-----------------------------------+
| csm_ras_type_audit_pkey   | csm_ras_type_audit    | pkey index on | msg_id_seq                        |
+---------------------------+-----------------------+---------------+-----------------------------------+
| csm_ras_type_pkey         | csm_ras_type          | pkey index on | msg_id                            |
+---------------------------+-----------------------+---------------+-----------------------------------+
| csm_ssd_pkey              | csm_ssd               | pkey index on | serial_number                     |
+---------------------------+-----------------------+---------------+-----------------------------------+
| csm_step_pkey             | csm_step              | pkey index on | step_id, allocation_id            |
+---------------------------+-----------------------+---------------+-----------------------------------+
| csm_switch_inventory_pkey | csm_switch_inventory  | pkey index on | name                              |
+---------------------------+-----------------------+---------------+-----------------------------------+
| csm_switch_pkey           | csm_switch            | pkey index on | switch_name                       |
+---------------------------+-----------------------+---------------+-----------------------------------+
| csm_switch_ports_pkey     | csm_switch_ports      | pkey index on | name                              |
+---------------------------+-----------------------+---------------+-----------------------------------+
| csm_vg_ssd_pkey           | csm_vg_ssd            | pkey index on | vg_name, node_name, serial_number |
+---------------------------+-----------------------+---------------+-----------------------------------+

Foreign Keys
^^^^^^^^^^^^
+-------------------------------------------+---------------------+-------------------------+--------------------+----------------------------+
| Name	                                    | From Table	  | From Cols	            | To Table	         | To Cols                    |
+===========================================+=====================+=========================+====================+============================+
| csm_allocation_node_allocation_id_fkey    | csm_allocation_node | allocation_id           | csm_allocation     | allocation_id              |
+-------------------------------------------+---------------------+-------------------------+--------------------+----------------------------+
| csm_allocation_node_node_name_fkey        | csm_allocation_node | node_name               | csm_node	         | node_name                  |
+-------------------------------------------+---------------------+-------------------------+--------------------+----------------------------+
| csm_diag_result_run_id_fkey	            | csm_diag_result	  | run_id                  | csm_diag_run	 | run_id                     |
+-------------------------------------------+---------------------+-------------------------+--------------------+----------------------------+
| csm_dimm_node_name_fkey	            | csm_dimm	          | node_name               | csm_node	         | node_name                  |
+-------------------------------------------+---------------------+-------------------------+--------------------+----------------------------+
| csm_gpu_node_name_fkey	            | csm_gpu	          | node_name               | csm_node	         | node_name                  |
+-------------------------------------------+---------------------+-------------------------+--------------------+----------------------------+
| csm_hca_node_name_fkey 	            | csm_hca	          | node_name               | csm_node	         | node_name                  |
+-------------------------------------------+---------------------+-------------------------+--------------------+----------------------------+
| csm_lv_allocation_id_fkey	            | csm_lv	          | allocation_id, node_name| csm_allocation_node| allocation_id, node_name   |
+-------------------------------------------+---------------------+-------------------------+--------------------+----------------------------+
| csm_lv_node_name_fkey	                    | csm_lv	          | node_name, vg_name      | csm_vg	         | node_name, vg_name         |
+-------------------------------------------+---------------------+-------------------------+--------------------+----------------------------+
| csm_processor_node_name_fkey	            | csm_processor	  | node_name               | csm_node	         | node_name                  |
+-------------------------------------------+---------------------+-------------------------+--------------------+----------------------------+
| csm_ras_event_action_msg_id_seq_fkey	    | csm_ras_event_action| msg_id_seq              | csm_ras_type_audit | msg_id_seq                 |
+-------------------------------------------+---------------------+-------------------------+--------------------+----------------------------+
| csm_ssd_node_name_fkey	            | csm_ssd	          | node_name               | csm_node	         | node_name                  |
+-------------------------------------------+---------------------+-------------------------+--------------------+----------------------------+
| csm_step_allocation_id_fkey	            | csm_step	          | allocation_id           | csm_allocation     | allocation_id              |
+-------------------------------------------+---------------------+-------------------------+--------------------+----------------------------+
| csm_step_node_allocation_id_fkey 	    | csm_step_node	  | allocation_id, node_name| csm_allocation_node| allocation_id, node_name   |
+-------------------------------------------+---------------------+-------------------------+--------------------+----------------------------+
| csm_step_node_step_id_fkey	            | csm_step_node	  | step_id, allocation_id  | csm_step	         | step_id, allocation_id     |
+-------------------------------------------+---------------------+-------------------------+--------------------+----------------------------+
| csm_switch_inventory_host_system_guid_fkey| csm_switch_inventory| host_system_guid        | csm_switch         | switch_name                |
+-------------------------------------------+---------------------+-------------------------+--------------------+----------------------------+
| csm_switch_ports_parent_fkey	            | csm_switch_ports	  | parent                  | csm_switch         | switch_name                |
+-------------------------------------------+---------------------+-------------------------+--------------------+----------------------------+
| csm_vg_ssd_serial_number_fkey 	    | csm_vg_ssd	  | serial_number, node_name| csm_ssd	         | serial_number, node_name   |
+-------------------------------------------+---------------------+-------------------------+--------------------+----------------------------+
| csm_vg_vg_name_fkey	                    | csm_vg	          | vg_name, node_name      | csm_vg_ssd	 | vg_name, node_name         |
+-------------------------------------------+---------------------+-------------------------+--------------------+----------------------------+

Indexes
^^^^^^^
+-----------------------------------+------------------------------+------------+-----------------------------------+
| Name	                            | Table	                   | Index on	| Description field                 |
+===================================+==============================+============+===================================+
| ix_csm_allocation_history_a       | csm_allocation_history       | index on	| history_time                      |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_allocation_state_history_a | csm_allocation_state_history | index on	| history_time                      |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_config_bucket_a            | csm_config_bucket            | index on	| bucket_id, item_list, time_stamp  |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_config_history_a           | csm_config_history           | index on	| history_time                      |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_db_schema_version_a        | csm_db_schema_version        | index on	| version, create_time              |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_db_schema_version_history_a| csm_db_schema_version_history| index on	| history_time                      |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_diag_result_a              | csm_diag_result              | index on	| run_id, test_name, node_name      |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_diag_result_history_a      | csm_diag_result_history      | index on	| history_time                      |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_diag_run_history_a         | csm_diag_run_history         | index on	| history_time                      |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_dimm_history_a             | csm_dimm_history             | index on	| history_time                      |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_gpu_history_a              | csm_gpu_history              | index on	| history_time                      |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_gpu_history_b              | csm_gpu_history              | index on	| serial_number                     |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_hca_history_a              | csm_hca_history              | index on	| history_time                      |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_ib_cable_history_a         | csm_ib_cable_history         | index on	| history_time                      |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_lv_a                       | csm_lv                       | index on	| logical_volume_name               |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_lv_history_a               | csm_lv_history               | index on	| history_time                      |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_lv_history_b               | csm_lv_history               | index on	| logical_volume_name               |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_lv_update_history_a        | csm_lv_update_history        | index on	| history_time                      |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_lv_update_history_b        | csm_lv_update_history        | index on	| logical_volume_name               |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_node_a                     | csm_node                     | index on	| node_name, ready                  |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_node_history_a             | csm_node_history             | index on	| history_time                      |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_node_history_b             | csm_node_history             | index on	| node_name                         |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_node_ready_history_a       | csm_node_ready_history       | index on	| history_time                      |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_node_ready_history_b       | csm_node_ready_history       | index on	| node_name, ready                  |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_processor_a                | csm_processor                | index on	| serial_number, node_name          |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_processor_history_a        | csm_processor_history        | index on	| history_time                      |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_processor_history_b        | csm_processor_history        | index on	| serial_number, node_name          |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_ras_event_action_a         | csm_ras_event_action         | index on	| msg_id                            |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_ras_event_action_b         | csm_ras_event_action         | index on	| time_stamp                        |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_ras_event_action_c         | csm_ras_event_action         | index on	| location_name                     |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_ras_event_action_d         | csm_ras_event_action         | index on	| time_stamp, msg_id                |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_ras_event_action_e         | csm_ras_event_action         | index on	| time_stamp, location_name         |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_ssd_a                      | csm_ssd                      | index on	| serial_number, node_name          |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_ssd_history_a              | csm_ssd_history              | index on	| history_time                      |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_ssd_history_b              | csm_ssd_history              | index on	| serial_number, node_name          |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_step_history_a             | csm_step_history             | index on	| history_time                      |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_step_history_b             | csm_step_history             | index on	| begin_time, end_time              |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_step_history_c             | csm_step_history             | index on	| allocation_id, end_time           |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_step_history_d             | csm_step_history             | index on	| end_time                          |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_step_history_e             | csm_step_history             | index on	| step_id                           |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_step_node_history_a        | csm_step_node_history        | index on	| history_time                      |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_switch_history_a           | csm_switch_history           | index on	| history_time                      |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_switch_history_b           | csm_switch_history           | index on	| switch_name, history_time         |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_switch_inventory_history_a | csm_switch_inventory_history | index on	| history_time                      |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_switch_ports_history_a     | csm_switch_ports_history     | index on	| history_time                      |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_vg_history_a               | csm_vg_history               | index on	| history_time                      |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_vg_ssd_a                   | csm_vg_ssd                   | index on	| vg_name, node_name, serial_number |
+-----------------------------------+------------------------------+------------+-----------------------------------+
| ix_csm_vg_ssd_history_a           | csm_vg_ssd_history           | index on	| history_time                      |
+-----------------------------------+------------------------------+------------+-----------------------------------+

Unique Indexes
^^^^^^^^^^^^^^
+-------------------------+---------------------+---------------+----------------------------------+
| Name	                  | Table	        | Index on	| Description field                |
+=========================+=====================+===============+==================================+
| uk_csm_allocation_node_b| csm_allocation_node | uniqueness on	| allocation_id, node_name         |
+-------------------------+---------------------+---------------+----------------------------------+
| uk_csm_ssd_a            | csm_ssd             | uniqueness on	| serial_number, node_name         |
+-------------------------+---------------------+---------------+----------------------------------+
| uk_csm_step_a           | csm_step            | uniqueness on	| step_id, allocation_id           |
+-------------------------+---------------------+---------------+----------------------------------+
| uk_csm_step_node_a      | csm_step_node       | uniqueness on	| step_id, allocation_id, node_name|
+-------------------------+---------------------+---------------+----------------------------------+
| uk_csm_vg_a             | csm_vg              | uniqueness on	| vg_name, node_name               |
+-------------------------+---------------------+---------------+----------------------------------+
| uk_csm_vg_ssd_a         | csm_vg_ssd          | uniqueness on	| vg_name, node_name               |
+-------------------------+---------------------+---------------+----------------------------------+

Functions and Triggers
^^^^^^^^^^^^^^^^^^^^^^
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| Function Name                                | Trigger Name                          | Table On                                                | Tr Type | Result Data Type | Action On             | Argument Data Type                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                | Description                                                                                                   |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_allocation_create_data_aggregator     | (Stored Procedure)                    | csm_allocation_node                                     |         | void             |                       | i_allocation_id bigint, i_node_names text[], i_ib_rx_list bigint[], i_ib_tx_list bigint[], i_gpfs_read_list bigint[], i_gpfs_write_list bigint[], i_energy bigint[], i_power_cap integer[], i_ps_ratio integer[]                                                                                                                                                                                                                                                                                                  | csm_allocation_node function to populate the data aggregator fields in csm_allocation_node.                   |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_allocation_finish_data_stats          | (Stored Procedure)                    | csm_allocation_node                                     |         | void             |                       | allocationid bigint, node_names text[], ib_rx_list bigint[], ib_tx_list bigint[], gpfs_read_list bigint[], gpfs_write_list bigint[], energy_list bigint[]                                                                                                                                                                                                                                                                                                                                                         | csm_allocation function to finalize the data aggregator fields.                                               |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_allocation_history_dump               | (Stored Procedure)                    | csm_allocation                                          |         | void             |                       | allocationid bigint, endtime timestamp without time zone, exitstatus integer, i_state text, node_names text[], ib_rx_list bigint[], ib_tx_list bigint[], gpfs_read_list bigint[], gpfs_write_list bigint[], energy_list bigint[]                                                                                                                                                                                                                                                                                  | csm_allocation function to amend summarized column(s) on DELETE. (csm_allocation_history_dump)                |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_allocation_node_change                | tr_csm_allocation_node_change         | csm_allocation_node                                     | BEFORE  | trigger          | DELETE                |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   | csm_allocation_node trigger to amend summarized column(s) on UPDATE and DELETE.                               |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_allocation_node_sharing_status        | (Stored Procedure)                    | csm_allocation_node                                     |         | void             |                       | i_allocation_id bigint, i_type text, i_state text, i_shared boolean, i_nodenames text[]                                                                                                                                                                                                                                                                                                                                                                                                                           |                                                                                                               |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_allocation_state_history_state_change | tr_csm_allocation_state_change        | csm_allocation                                          | BEFORE  | trigger          | UPDATE                |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   | csm_allocation trigger to amend summarized column(s) on UPDATE.                                               |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_allocation_update                     | tr_csm_allocation_update              | csm_allocation                                          | BEFORE  | trigger          | UPDATE                |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   | csm_allocation_update trigger to amend summarized column(s) on UPDATE.                                        |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_allocation_update_state               | (Stored Procedure)                    | csm_allocation,csm_allocation_node                      |         | record           |                       | i_allocationid bigint, i_state text, OUT o_primary_job_id bigint, OUT o_secondary_job_id integer, OUT o_user_flags text, OUT o_system_flags text, OUT o_num_nodes integer, OUT o_nodes text, OUT o_isolated_cores integer, OUT o_user_name text                                                                                                                                                                                                                                                                   | csm_allocation_update_state function that ensures the allocation can be legally updated to the supplied state |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_config_history_dump                   | tr_csm_config_history_dump            | csm_config                                              | BEFORE  | trigger          | UPDATE, DELETE        |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   | csm_config trigger to amend summarized column(s) on UPDATE and DELETE.                                        |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_db_schema_version_history_dump        | tr_csm_db_schema_version_history_dump | csm_db_schema_version                                   | BEFORE  | trigger          | UPDATE, DELETE        |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   | csm_db_schema_version trigger to amend summarized column(s) on UPDATE and DELETE.                             |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_diag_result_history_dump              | tr_csm_diag_result_history_dump       | csm_diag_result                                         | BEFORE  | trigger          | DELETE                |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   | csm_diag_result trigger to amend summarized column(s) on DELETE.                                              |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_diag_run_history_dump                 | (Stored Procedure)                    | csm_diag_run                                            |         | void             |                       | _run_id bigint, _end_time timestamp with time zone, _status text, _inserted_ras boolean                                                                                                                                                                                                                                                                                                                                                                                                                           | csm_diag_run function to amend summarized column(s) on UPDATE and DELETE. (csm_diag_run_history_dump)         |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_dimm_history_dump                     | tr_csm_dimm_history_dump              | csm_dimm                                                | BEFORE  | trigger          | UPDATE, DELETE        |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   | csm_dimm trigger to amend summarized column(s) on UPDATE and DELETE.                                          |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_gpu_history_dump                      | tr_csm_gpu_history_dump               | csm_gpu                                                 | BEFORE  | trigger          | UPDATE, DELETE        |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   | csm_gpu trigger to amend summarized column(s) on UPDATE and DELETE.                                           |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_hca_history_dump                      | tr_csm_hca_history_dump               | csm_hca                                                 | BEFORE  | trigger          | UPDATE, DELETE        |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   | csm_hca trigger to amend summarized column(s) on UPDATE and DELETE.                                           |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_ib_cable_history_dump                 | tr_csm_ib_cable_history_dump          | csm_ib_cable                                            | BEFORE  | trigger          | UPDATE, DELETE        |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   | csm_ib_cable trigger to amend summarized column(s) on UPDATE and DELETE.                                      |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_ib_cable_inventory_collection         | (Stored Procedure)                    | csm_ib_cable                                            |         | record           |                       | i_record_count integer, i_serial_number text[], i_comment text[], i_guid_s1 text[], i_guid_s2 text[], i_identifier text[], i_length text[], i_name text[], i_part_number text[], i_port_s1 text[], i_port_s2 text[], i_revision text[], i_severity text[], i_type text[], i_width text[], OUT o_insert_count integer, OUT o_update_count integer                                                                                                                                                                  | function to INSERT and UPDATE ib cable inventory.                                                             |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_lv_history_dump                       | (Stored Procedure)                    | csm_lv                                                  |         | void             |                       | _logicalvolumename text, _node_name text, _allocationid bigint, _state character, _currentsize bigint, _updatedtime timestamp without time zone, _endtime timestamp without time zone, _numbytesread bigint, _numbyteswritten bigint                                                                                                                                                                                                                                                                              | csm_lv function to amend summarized column(s) on DELETE. (csm_lv_history_dump)                                |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_lv_modified_history_dump              | tr_csm_lv_modified_history_dump       | csm_lv                                                  | BEFORE  | trigger          | UPDATE                |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   | csm_lv_modified_history_dump trigger to amend summarized column(s) on UPDATE.                                 |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_lv_update_history_dump                | tr_csm_lv_update_history_dump         | csm_lv                                                  | BEFORE  | trigger          | UPDATE                |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   | csm_lv_update_history_dump trigger to amend summarized column(s) on UPDATE.                                   |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_lv_upsert                             | (Stored Procedure)                    | csm_lv                                                  |         | void             |                       | l_logical_volume_name text, l_node_name text, l_allocation_id bigint, l_vg_name text, l_state character, l_current_size bigint, l_max_size bigint, l_begin_time timestamp without time zone, l_updated_time timestamp without time zone, l_file_system_mount text, l_file_system_type text                                                                                                                                                                                                                        | csm_lv_upsert function to amend summarized column(s) on INSERT. (csm_lv table)                                |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_node_attributes_query_details         | (Stored Procedure)                    | csm_node,csm_dimm,csm_gpu,csm_hca,csm_processor,csm_ssd |         | node_details     |                       | i_node_name text                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  | csm_node_attributes_query_details function to HELP CSM API.                                                   |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_node_delete                           | (Stored Procedure)                    | csm_node,csm_dimm,csm_gpu,csm_hca,csm_processor,csm_ssd |         | record           |                       | i_node_names text[], OUT o_not_deleted_node_names_count integer, OUT o_not_deleted_node_names text                                                                                                                                                                                                                                                                                                                                                                                                                | Function to delete a vg, and remove records in the csm_vg and csm_vg_ssd tables                               |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_node_ready                            | tr_csm_node_ready                     | csm_node                                                | BEFORE  | trigger          | UPDATE                |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   | csm_node_ready trigger to amend summarized column(s) on UPDATE.                                               |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_node_update                           | tr_csm_node_update                    | csm_node                                                | BEFORE  | trigger          | UPDATE, DELETE        |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   | csm_node_update trigger to amend summarized column(s) on UPDATE and DELETE.                                   |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_processor_history_dump                | tr_csm_processor_history_dump         | csm_processor                                           | BEFORE  | trigger          | UPDATE, DELETE        |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   | csm_processor trigger to amend summarized column(s) on UPDATE and DELETE.                                     |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_ras_type_update                       | tr_csm_ras_type_update                | csm_ras_type                                            | AFTER   | trigger          | INSERT, UPDATE,DELETE |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   | csm_ras_type trigger to add rows to csm_ras_type_audit on INSERT and UPDATE and DELETE. (csm_ras_type_update) |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_ssd_history_dump                      | tr_csm_ssd_history_dump               | csm_ssd                                                 | BEFORE  | trigger          | UPDATE, DELETE        |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   | csm_ssd trigger to amend summarized column(s) on UPDATE and DELETE.                                           |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_step_begin                            | (Stored Procedure)                    | csm_step                                                |         | void             |                       | i_step_id bigint, i_allocation_id bigint, i_status text, i_executable text, i_working_directory text, i_argument text, i_environment_variable text, i_num_nodes integer, i_num_processors integer, i_num_gpus integer, i_projected_memory integer, i_num_tasks integer, i_user_flags text, i_node_names text[]                                                                                                                                                                                                    | csm_step_begin function to begin a step, adds the step to csm_step and csm_step_node                          |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_step_end                              | (Stored Procedure)                    | csm_step_node,csm_step                                  |         | record           |                       | i_stepid bigint, i_allocationid bigint, i_exitstatus integer, i_errormessage text, i_cpustats text, i_totalutime double precision, i_totalstime double precision, i_ompthreadlimit text, i_gpustats text, i_memorystats text, i_maxmemory bigint, i_iostats text, OUT o_user_flags text, OUT o_num_nodes integer, OUT o_nodes text                                                                                                                                                                                | csm_step_end function to delete the step from the nodes table (fn_csm_step_end)                               |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_step_history_dump                     | (Stored Procedure)                    | csm_step                                                |         | void             |                       | i_stepid bigint, i_allocationid bigint, i_endtime timestamp with time zone, i_exitstatus integer, i_errormessage text, i_cpustats text, i_totalutime double precision, i_totalstime double precision, i_ompthreadlimit text, i_gpustats text, i_memorystats text, i_maxmemory bigint, i_iostats text                                                                                                                                                                                                              | csm_step function to amend summarized column(s) on DELETE. (csm_step_history_dump)                            |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_step_node_history_dump                | tr_csm_step_node_history_dump         | csm_step_node                                           | BEFORE  | trigger          | DELETE                | i_switch_name text                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                | csm_step_node trigger to amend summarized column(s) on DELETE. (csm_step_node_history_dump)                   |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_switch_attributes_query_details       | (Stored Procedure)                    | csm_switch,csm_switch_inventory,csm_switch_ports        |         | switch_details   |                       | i_record_count integer, i_name text[], i_host_system_guid text[], i_comment text[], i_description text[], i_device_name text[], i_device_type text[], i_max_ib_ports text[], i_module_index text[], i_number_of_chips text[], i_path text[], i_serial_number text[], i_severity text[], i_status text[]                                                                                                                                                                                                           | csm_switch_attributes_query_details function to HELP CSM API.                                                 |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_switch_children_inventory_collection  | (Stored Procedure)                    | csm_switch_inventory                                    |         | void             |                       | i_record_count integer, i_switch_name text[], i_comment text[], i_description text[], i_fw_version text[], i_gu_id text[], i_has_ufm_agent text[], i_ip text[], i_model text[], i_num_modules text[], i_num_ports text[], i_physical_frame_location text[], i_physical_u_location text[], i_ps_id text[], i_role text[], i_server_operation_mode text[], i_sm_mode text[], i_state text[], i_sw_version text[], i_system_guid text[], i_system_name text[], i_total_alarms text[], i_type text[], i_vendor text[] | function to INSERT and UPDATE switch children inventory.                                                      |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_switch_history_dump                   | tr_csm_switch_history_dump            | csm_switch                                              | BEFORE  | trigger          | UPDATE, DELETE        |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   | csm_switch trigger to amend summarized column(s) on UPDATE and DELETE.                                        |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_switch_inventory_collection           | (Stored Procedure)                    | csm_switch                                              |         | void             |                       |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   | function to INSERT and UPDATE switch inventory.                                                               |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_switch_inventory_history_dump         | tr_csm_switch_inventory_history_dump  | csm_switch_inventory                                    | BEFORE  | trigger          | UPDATE, DELETE        | i_available_size bigint, i_node_name text, i_ssd_count integer, i_ssd_serial_numbers text[], i_ssd_allocations bigint[], i_total_size bigint, i_vg_name text, i_is_scheduler boolean                                                                                                                                                                                                                                                                                                                              | csm_switch_inventory trigger to amend summarized column(s) on UPDATE and DELETE.                              |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_switch_ports_history_dump             | tr_csm_switch_ports_history_dump      | csm_switch_ports                                        | BEFORE  | trigger          | UPDATE, DELETE        |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   | csm_switch_ports trigger to amend summarized column(s) on UPDATE and DELETE.                                  |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
| fn_csm_vg_create                             | (Stored Procedure)                    | csm_vg_ssd,csm_vg,csm_ssd                               |         | void             |                       |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   | Function to create a vg, adds the vg to csm_vg_ssd and csm_vg                                                 |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
|                                              |                                       |                                                         |         |                  |                       |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |                                                                                                               |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
|                                              |                                       |                                                         |         |                  |                       |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |                                                                                                               |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
|                                              |                                       |                                                         |         |                  |                       |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |                                                                                                               |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+
|                                              |                                       |                                                         |         |                  |                       |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |                                                                                                               |
+----------------------------------------------+---------------------------------------+---------------------------------------------------------+---------+------------------+-----------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------+


CSM DB Schema (pdf)
-------------------
(CSM DB schema version 16.1):
-- Coming soon --

..     .. image:: CSM_DB_08-10-2018_v16.1.jpg
..              :width: 600px
..              :height: 500px
..              :scale: 100%
..              :alt: Screenshot of Image Window in OpenCV
..              :align: left
