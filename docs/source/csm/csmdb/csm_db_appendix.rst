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
              | generate within PostgreSQL starting      | *csm_node_pkey*
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

csm_node (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                                               Table "public.csm_node"
          Column          |            Type             | Modifiers | Storage  | Stats target |                                                     Description
 -------------------------+-----------------------------+-----------+----------+--------------+---------------------------------------------------------------------------------------------------------------------
  node_name               | text                        | not null  | extended |              | identifies which node this information is for
  machine_model           | text                        |           | extended |              | machine type model information for this node
  serial_number           | text                        |           | extended |              | witherspoon boards serial number
  collection_time         | timestamp without time zone |           | plain    |              | the time the node was collected at inventory
  update_time             | timestamp without time zone |           | plain    |              | the time the node was updated
  state                   | compute_node_states         |           | plain    |              | state of the node - DISCOVERED, IN_SERVICE, ADMIN_RESERVED, MAINTENANCE, SOFT_FAILURE, OUT_OF_SERVICE, HARD_FAILURE
  type                    | text                        |           | extended |              | management, service, login, workload manager, launch, compute
  primary_agg             | text                        |           | extended |              | primary aggregate
  secondary_agg           | text                        |           | extended |              | secondary aggregate
  hard_power_cap          | integer                     |           | plain    |              | hard power cap for this node
  installed_memory        | bigint                      |           | plain    |              | amount of installed memory on this node (in kB)
  installed_swap          | bigint                      |           | plain    |              | amount of available swap space on this node (in kB)
  discovered_sockets      | integer                     |           | plain    |              | number of processors on this node (processor sockets, non-uniform memory access (NUMA) nodes)
  discovered_cores        | integer                     |           | plain    |              | number of physical cores on this node from all processors
  discovered_gpus         | integer                     |           | plain    |              | number of gpus available
  discovered_hcas         | integer                     |           | plain    |              | number of IB HCAs discovered in this node during the most recent inventory collection
  discovered_dimms        | integer                     |           | plain    |              | number of dimms discovered in this node during the most recent inventory collection
  discovered_ssds         | integer                     |           | plain    |              | number of ssds discovered in this node during the most recent inventory collection
  os_image_name           | text                        |           | extended |              | xCAT os image name being run on this node, diskless images only
  os_image_uuid           | text                        |           | extended |              | xCAT os image uuid being run on this node, diskless images only
  kernel_release          | text                        |           | extended |              | kernel release being run on this node
  kernel_version          | text                        |           | extended |              | linux kernel version being run on this node
  physical_frame_location | text                        |           | extended |              | physical frame number where the node is located
  physical_u_location     | text                        |           | extended |              | physical u location (position in the frame) where the node is located
  feature_1               | text                        |           | extended |              | reserved fields for future use
  feature_2               | text                        |           | extended |              | reserved fields for future use
  feature_3               | text                        |           | extended |              | reserved fields for future use
  feature_4               | text                        |           | extended |              | reserved fields for future use
  comment                 | text                        |           | extended |              | comment field for system administrators
 Indexes:
     "csm_node_pkey" PRIMARY KEY, btree (node_name)
     "ix_csm_node_a" btree (node_name, state)
 Check constraints:
     "csm_not_blank" CHECK (btrim(node_name, ' '::text) <> ''::text)
     "csm_not_null_string" CHECK (node_name <> ''::text)
 Referenced by:
     TABLE "csm_allocation_node" CONSTRAINT "csm_allocation_node_node_name_fkey" FOREIGN KEY (node_name) REFERENCES csm_node(node_name)
     TABLE "csm_dimm" CONSTRAINT "csm_dimm_node_name_fkey" FOREIGN KEY (node_name) REFERENCES csm_node(node_name)
     TABLE "csm_gpu" CONSTRAINT "csm_gpu_node_name_fkey" FOREIGN KEY (node_name) REFERENCES csm_node(node_name)
     TABLE "csm_hca" CONSTRAINT "csm_hca_node_name_fkey" FOREIGN KEY (node_name) REFERENCES csm_node(node_name)
     TABLE "csm_processor_socket" CONSTRAINT "csm_processor_socket_node_name_fkey" FOREIGN KEY (node_name) REFERENCES csm_node(node_name)
     TABLE "csm_ssd" CONSTRAINT "csm_ssd_node_name_fkey" FOREIGN KEY (node_name) REFERENCES csm_node(node_name)
 Triggers:
     tr_csm_node_state BEFORE INSERT OR UPDATE OF state ON csm_node FOR EACH ROW EXECUTE PROCEDURE fn_csm_node_state()
     tr_csm_node_update BEFORE INSERT OR DELETE OR UPDATE OF node_name, machine_model, serial_number, collection_time,
	 type, primary_agg, secondary_agg, hard_power_cap, installed_memory, installed_swap, discovered_sockets,
	 discovered_cores, discovered_gpus, discovered_hcas, discovered_dimms, discovered_ssds, os_image_name,
	 os_image_uuid, kernel_release, kernel_version, physical_frame_location, physical_u_location, feature_1,
	 feature_2, feature_3, feature_4, comment ON csm_node FOR EACH ROW EXECUTE PROCEDURE fn_csm_node_update()
 Has OIDs: no

csm_node_history
""""""""""""""""

**Description**
 This table contains the historical information related to node attributes.

=========== ================================================= ==========================
 Table      Overview                                          Action On:
=========== ================================================= ==========================
 Usage      | Low (When hardware changes and to query         |
            | historical information)                         |
 Size       | 5000+ rows (Based on hardware changes)          |
 Index      | ix_csm_node_history_a on (history_time)         |
            | ix_csm_node_history_b on (node_name)            |
            | ix_csm_node_history_c on (ctid)                 |
            | ix_csm_node_history_d on (archive_history_time) |
=========== ================================================= ==========================
 
csm_node_history (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                                           Table "public.csm_node_history"
          Column          |            Type             | Modifiers | Storage  | Stats target |                                                     Description
 -------------------------+-----------------------------+-----------+----------+--------------+---------------------------------------------------------------------------------------------------------------------
  history_time            | timestamp without time zone |           | plain    |              | time when the node is entered into the history table
  node_name               | text                        |           | extended |              | identifies which node this information is for
  machine_model           | text                        |           | extended |              | machine type model information for this node
  serial_number           | text                        |           | extended |              | witherspoon boards serial number
  collection_time         | timestamp without time zone |           | plain    |              | the time the node was collected at inventory
  update_time             | timestamp without time zone |           | plain    |              | the time the node was updated
  state                   | compute_node_states         |           | plain    |              | state of the node - DISCOVERED, IN_SERVICE, ADMIN_RESERVED, MAINTENANCE, SOFT_FAILURE, OUT_OF_SERVICE, HARD_FAILURE
  type                    | text                        |           | extended |              | management, service, login, workload manager, launch, compute
  primary_agg             | text                        |           | extended |              | primary aggregate
  secondary_agg           | text                        |           | extended |              | secondary aggregate
  hard_power_cap          | integer                     |           | plain    |              | hard power cap for this node
  installed_memory        | bigint                      |           | plain    |              | amount of installed memory on this node (in kB)
  installed_swap          | bigint                      |           | plain    |              | amount of available swap space on this node (in kB)
  discovered_sockets      | integer                     |           | plain    |              | number of processors on this node (processor sockets, non-uniform memory access (NUMA) nodes)
  discovered_cores        | integer                     |           | plain    |              | number of physical cores on this node from all processors
  discovered_gpus         | integer                     |           | plain    |              | number of gpus available
  discovered_hcas         | integer                     |           | plain    |              | number of IB HCAs discovered in this node during inventory collection
  discovered_dimms        | integer                     |           | plain    |              | number of dimms discovered in this node during inventory collection
  discovered_ssds         | integer                     |           | plain    |              | number of ssds discovered in this node during inventory collection
  os_image_name           | text                        |           | extended |              | xCAT os image name being run on this node, diskless images only
  os_image_uuid           | text                        |           | extended |              | xCAT os image uuid being run on this node, diskless images only
  kernel_release          | text                        |           | extended |              | linux kernel release being run on this node
  kernel_version          | text                        |           | extended |              | linux kernel version being run on this node
  physical_frame_location | text                        |           | extended |              | physical frame number where the node is located
  physical_u_location     | text                        |           | extended |              | physical u location (position in the frame) where the node is located
  feature_1               | text                        |           | extended |              | reserved fields for future use
  feature_2               | text                        |           | extended |              | reserved fields for future use
  feature_3               | text                        |           | extended |              | reserved fields for future use
  feature_4               | text                        |           | extended |              | reserved fields for future use
  comment                 | text                        |           | extended |              | comment field for system administrators
  operation               | character(1)                | not null  | extended |              | operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)
  archive_history_time    | timestamp without time zone |           | plain    |              | timestamp when the history data has been archived and sent to: BDS, archive file, and or other
 Indexes:
     "ix_csm_node_history_a" btree (history_time)
     "ix_csm_node_history_b" btree (node_name)
     "ix_csm_node_history_c" btree (ctid)
     "ix_csm_node_history_d" btree (archive_history_time)
 Has OIDs: no

csm_node_state_history
""""""""""""""""""""""

**Description**
 This table contains historical information related to the node state status.  This table will be updated each time the node status status changes.

=========== ======================================================= ==========================
 Table      Overview                                                Action On:
=========== ======================================================= ==========================
 Usage      | Med-High                                              |
 Size       | (Based on how often a node ready status changes)      |
 Index      | ix_csm_node_ready_history_a on (history_time)         |
            | ix_csm_node_ready_history_b on (node_name, ready)     |
            | ix_csm_node_ready_history_c on (ctid)                 |
            | ix_csm_node_ready_history_d on (archive_history_time) |
=========== ======================================================= ==========================  

csm_node_state_history (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                                      Table "public.csm_node_state_history"
         Column        |            Type             | Modifiers | Storage  | Stats target |                                                     Description
 ----------------------+-----------------------------+-----------+----------+--------------+---------------------------------------------------------------------------------------------------------------------
  history_time         | timestamp without time zone |           | plain    |              | time when the node ready status is entered into the history table
  node_name            | text                        |           | extended |              | identifies which node this information is for
  state                | compute_node_states         |           | plain    |              | state of the node - DISCOVERED, IN_SERVICE, ADMIN_RESERVED, MAINTENANCE, SOFT_FAILURE, OUT_OF_SERVICE, HARD_FAILURE
  operation            | character(1)                | not null  | extended |              | operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)
  archive_history_time | timestamp without time zone |           | plain    |              | timestamp when the history data has been archived and sent to: BDS, archive file, and or other
 Indexes:
     "ix_csm_node_state_history_a" btree (history_time)
     "ix_csm_node_state_history_b" btree (node_name, state)
     "ix_csm_node_state_history_c" btree (ctid)
     "ix_csm_node_state_history_d" btree (archive_history_time)
 Has OIDs: no

csm_processor_socket
""""""""""""""""""""

**Description**
 This table contains information on the processors of a node.

=========== ================================================== ==========================
 Table      Overview                                           Action On:
=========== ================================================== ==========================
 Usage      | Low                                              |
 Size       | 25,000+ rows (Witherspoon will consist of        |
            | 256 processors per node. (based on 5000 nodes)   |
 Key(s)     | PK: serial_number, node_name                     |
            | FK: csm_node (node_name)                         |
 Index      | csm_processor_pkey on (serial_number, node_name) |
 Functions  | fn_csm_processor_history_dump                    |
 Triggers   | tr_csm_processor_history_dump                    | update/delete
=========== ================================================== ==========================   

csm_processor_socket (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash
  
                                       Table "public.csm_processor_socket"
       Column       |  Type   | Modifiers | Storage  | Stats target |                Description
 -------------------+---------+-----------+----------+--------------+--------------------------------------------
  serial_number     | text    | not null  | extended |              | unique identifier for this processor
  node_name         | text    | not null  | extended |              | where does this processor reside
  physical_location | text    |           | extended |              | physical location of the processor
  discovered_cores  | integer |           | plain    |              | number of physical cores on this processor
 Indexes:
     "csm_processor_socket_pkey" PRIMARY KEY, btree (serial_number, node_name)
 Foreign-key constraints:
     "csm_processor_socket_node_name_fkey" FOREIGN KEY (node_name) REFERENCES csm_node(node_name)
 Triggers:
     tr_csm_processor_socket_history_dump BEFORE INSERT OR DELETE OR UPDATE ON csm_processor_socket FOR EACH ROW EXECUTE PROCEDURE fn_csm_processor_socket_history_dump()
 Has OIDs: no

csm_processor_socket_history
""""""""""""""""""""""""""""

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
            | ix_csm_processor_history_c on (ctid)                     |
            | ix_csm_processor_history_d on (archive_history_time)     |
=========== ========================================================== ==============

csm_processor_socket_history (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash
 
                                                                         Table "public.csm_processor_socket_history"
         Column        |            Type             | Modifiers | Storage  | Stats target |                                          Description
 ----------------------+-----------------------------+-----------+----------+--------------+------------------------------------------------------------------------------------------------
  history_time         | timestamp without time zone | not null  | plain    |              | the time when the processor is entering the history table
  serial_number        | text                        | not null  | extended |              | unique identifier for this processor
  node_name            | text                        |           | extended |              | where does this processor reside
  physical_location    | text                        |           | extended |              | physical location of the processor
  discovered_cores     | integer                     |           | plain    |              | number of physical cores on this processor
  operation            | character(1)                | not null  | extended |              | operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)
  archive_history_time | timestamp without time zone |           | plain    |              | timestamp when the history data has been archived and sent to: BDS, archive file, and or other
 Indexes:
     "ix_csm_processor_socket_history_a" btree (history_time)
     "ix_csm_processor_socket_history_b" btree (serial_number, node_name)
     "ix_csm_processor_socket_history_c" btree (ctid)
     "ix_csm_processor_socket_history_d" btree (archive_history_time)
 Has OIDs: no

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

csm_gpu (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                             Table "public.csm_gpu"
         Column         |  Type   | Modifiers | Storage  | Stats target |                              Description
 -----------------------+---------+-----------+----------+--------------+-----------------------------------------------------------------------
  serial_number         | text    | not null  | extended |              | unique identifier for this gpu
  node_name             | text    | not null  | extended |              | where does this gpu reside
  gpu_id                | integer | not null  | plain    |              | gpu identification number
  device_name           | text    | not null  | extended |              | indicates the device name
  pci_bus_id            | text    | not null  | extended |              | Peripheral Component Interconnect bus identifier
  uuid                  | text    | not null  | extended |              | universally unique identifier
  vbios                 | text    | not null  | extended |              | Video BIOS
  inforom_image_version | text    | not null  | extended |              | version of the infoROM
  hbm_memory            | bigint  |           | plain    |              | high bandwidth memory: amount of available memory on this gpu (in kB)
 Indexes:
     "csm_gpu_pkey" PRIMARY KEY, btree (node_name, gpu_id)
 Foreign-key constraints:
     "csm_gpu_node_name_fkey" FOREIGN KEY (node_name) REFERENCES csm_node(node_name)
 Triggers:
     tr_csm_gpu_history_dump BEFORE INSERT OR DELETE OR UPDATE ON csm_gpu FOR EACH ROW EXECUTE PROCEDURE fn_csm_gpu_history_dump()
 Has OIDs: no

csm_gpu_history
"""""""""""""""

**Description**
 This table contains historical information associated with individual GPUs. The GPU will be recorded and also be timestamped.

=========== ================================================ ==========================
 Table      Overview                                         Action On:
=========== ================================================ ==========================
 Usage      | Low                                            |
 Size       | (based on how often changed)                   |
 Index      | ix_csm_gpu_history_a on (history_time)         |
            | ix_csm_gpu_history_b on (serial_number)        |
            | ix_csm_gpu_history_c on (node_name, gpu_id)    |
            | ix_csm_gpu_history_d on (ctid)                 |
            | ix_csm_gpu_history_e on (archive_history_time) |
=========== ================================================ ==========================   

csm_gpu_history (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash
 
                                                                                Table "public.csm_gpu_history"
         Column         |            Type             | Modifiers | Storage  | Stats target |                                          Description
 -----------------------+-----------------------------+-----------+----------+--------------+------------------------------------------------------------------------------------------------
  history_time          | timestamp without time zone | not null  | plain    |              | the time when the gpu is entering the history table
  serial_number         | text                        | not null  | extended |              | unique identifier for this gpu
  node_name             | text                        |           | extended |              | where does this gpu reside
  gpu_id                | integer                     | not null  | plain    |              | gpu identification number
  device_name           | text                        | not null  | extended |              | indicates the device name
  pci_bus_id            | text                        | not null  | extended |              | Peripheral Component Interconnect bus identifier
  uuid                  | text                        | not null  | extended |              | universally unique identifier
  vbios                 | text                        | not null  | extended |              | Video BIOS
  inforom_image_version | text                        | not null  | extended |              | version of the infoROM
  hbm_memory            | bigint                      |           | plain    |              | high bandwidth memory: amount of available memory on this gpu (in kB)
  operation             | character(1)                | not null  | extended |              | operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)
  archive_history_time  | timestamp without time zone |           | plain    |              | timestamp when the history data has been archived and sent to: BDS, archive file, and or other
 Indexes:
     "ix_csm_gpu_history_a" btree (history_time)
     "ix_csm_gpu_history_b" btree (serial_number)
     "ix_csm_gpu_history_c" btree (node_name, gpu_id)
     "ix_csm_gpu_history_d" btree (ctid)
     "ix_csm_gpu_history_e" btree (archive_history_time)
 Has OIDs: no
 
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

csm_ssd (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash 
  
                                                                             Table "public.csm_ssd"
             Column             |            Type             | Modifiers | Storage  | Stats target |                                Description
 -------------------------------+-----------------------------+-----------+----------+--------------+---------------------------------------------------------------------------
  serial_number                 | text                        | not null  | extended |              | unique identifier for this ssd
  node_name                     | text                        | not null  | extended |              | where does this ssd reside
  update_time                   | timestamp without time zone | not null  | plain    |              | timestamp when ssd was updated
  device_name                   | text                        |           | extended |              | product device name
  pci_bus_id                    | text                        |           | extended |              | PCI bus id
  fw_ver                        | text                        |           | extended |              | firmware version
  size                          | bigint                      | not null  | plain    |              | total capacity (in bytes) of this ssd, for example, 800 gbs
  wear_lifespan_used            | double precision            |           | plain    |              | estimate of the amount of SSD life consumed (w.l.m. will
  use - 0-255 per)
  wear_total_bytes_written      | bigint                      |           | plain    |              | number of bytes written to the SSD over the life of the device
  wear_total_bytes_read         | bigint                      |           | plain    |              | number of bytes read from the SSD over the life of the device
  wear_percent_spares_remaining | double precision            |           | plain    |              | amount of SSD capacity over-provisioning that remains
 Indexes:
     "csm_ssd_pkey" PRIMARY KEY, btree (serial_number, node_name)
     "uk_csm_ssd_a" UNIQUE, btree (serial_number, node_name)
 Foreign-key constraints:
     "csm_ssd_node_name_fkey" FOREIGN KEY (node_name) REFERENCES csm_node(node_name)
 Referenced by:
     TABLE "csm_vg_ssd" CONSTRAINT "csm_vg_ssd_serial_number_fkey" FOREIGN KEY (serial_number, node_name) REFERENCES csm_ssd(serial_number, node_name)
 Triggers:
     tr_csm_ssd_history_dump BEFORE INSERT OR DELETE OR UPDATE OF serial_number, node_name, device_name, pci_bus_id, fw_ver, size ON csm_ssd FOR EACH ROW EXECUTE PROCEDURE fn_csm_ssd_history_dump()
     tr_csm_ssd_wear BEFORE UPDATE OF wear_lifespan_used, wear_total_bytes_written, wear_total_bytes_read, wear_percent_spares_remaining ON csm_ssd FOR EACH ROW EXECUTE PROCEDURE fn_csm_ssd_wear()
 Has OIDs: no

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
            | ix_csm_ssd_history_c on (ctid)                     |
            | ix_csm_ssd_history_d on (archive_history_time)     |
=========== ==================================================== ==========================

csm_ssd_history (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash 
 
                                                                                    Table "public.csm_ssd_history"
             Column             |            Type             | Modifiers | Storage  | Stats target |                                          Description
 -------------------------------+-----------------------------+-----------+----------+--------------+------------------------------------------------------------------------------------------------
  history_time                  | timestamp without time zone | not null  | plain    |              | timestamp
  serial_number                 | text                        | not null  | extended |              | unique identifier for this ssd
  node_name                     | text                        |           | extended |              | where does this ssd reside
  update_time                   | timestamp without time zone | not null  | plain    |              | timestamp when the ssd was updated
  device_name                   | text                        |           | extended |              | product device name
  pci_bus_id                    | text                        |           | extended |              | PCI bus id
  fw_ver                        | text                        |           | extended |              | firmware version
  size                          | bigint                      | not null  | plain    |              | total capacity (in bytes) of this ssd, for example, 800 gbs
  wear_lifespan_used            | double precision            |           | plain    |              | estimate of the amount of SSD life consumed (w.l.m. will
  use - 0-255 per)
  wear_total_bytes_written      | bigint                      |           | plain    |              | number of bytes written to the SSD over the life of the device
  wear_total_bytes_read         | bigint                      |           | plain    |              | number of bytes read from the SSD over the life of the device
  wear_percent_spares_remaining | double precision            |           | plain    |              | amount of SSD capacity over-provisioning that remains
  operation                     | character(1)                | not null  | extended |              | operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)
  archive_history_time          | timestamp without time zone |           | plain    |              | timestamp when the history data has been archived and sent to: BDS, archive file, and or other
 Indexes:
     "ix_csm_ssd_history_a" btree (history_time)
     "ix_csm_ssd_history_b" btree (serial_number, node_name)
     "ix_csm_ssd_history_c" btree (ctid)
     "ix_csm_ssd_history_d" btree (archive_history_time)
 Has OIDs: no

csm_ssd_wear_history
""""""""""""""""""""

**Description**
 This table contains historical information on the ssds wear known to the system.

=========== ========================================================= ==========================
 Table      Overview                                                  Action On:
=========== ========================================================= ==========================
 Usage      | Low                                                     |
 Size       | 5000+ rows                                              |
 Index      | ix_csm_ssd_wear_history_a on (history_time)             |
            | ix_csm_ssd_wear_history_b on (serial_number, node_name) |
            | ix_csm_ssd_wear_history_c on (ctid)                     |
            | ix_csm_ssd_wear_history_d on (archive_history_time)     |
=========== ========================================================= ==========================

csm_ssd_wear_history (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash 
 
                                                                                 Table "public.csm_ssd_wear_history"
             Column             |            Type             | Modifiers | Storage  | Stats target |                                          Description
 -------------------------------+-----------------------------+-----------+----------+--------------+------------------------------------------------------------------------------------------------
  history_time                  | timestamp without time zone | not null  | plain    |              | timestamp
  serial_number                 | text                        | not null  | extended |              | unique identifier for this ssd
  node_name                     | text                        |           | extended |              | where does this ssd reside
  wear_lifespan_used            | double precision            |           | plain    |              | estimate of the amount of SSD life consumed (w.l.m. will
  use - 0-255 per)
  wear_total_bytes_written      | bigint                      |           | plain    |              | number of bytes written to the SSD over the life of the device
  wear_total_bytes_read         | bigint                      |           | plain    |              | number of bytes read from the SSD over the life of the device
  wear_percent_spares_remaining | double precision            |           | plain    |              | amount of SSD capacity over-provisioning that remains
  operation                     | character(1)                | not null  | extended |              | operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)
  archive_history_time          | timestamp without time zone |           | plain    |              | timestamp when the history data has been archived and sent to: BDS, archive file, and or other
 Indexes:
     "ix_csm_ssd_wear_history_a" btree (history_time)
     "ix_csm_ssd_wear_history_b" btree (serial_number, node_name)
     "ix_csm_ssd_wear_history_c" btree (ctid)
     "ix_csm_ssd_wear_history_d" btree (archive_history_time)
 Has OIDs: no

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

csm_hca (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash 
 
                                      Table "public.csm_hca"
     Column     | Type | Modifiers | Storage  | Stats target |            Description
 ---------------+------+-----------+----------+--------------+-----------------------------------
  serial_number | text | not null  | extended |              | unique serial number for this HCA
  node_name     | text | not null  | extended |              | node this HCA is installed in
  device_name   | text |           | extended |              | product device name for this HCA
  pci_bus_id    | text | not null  | extended |              | PCI bus id for this HCA
  guid          | text | not null  | extended |              | sys_image_guid for this HCA
  part_number   | text |           | extended |              | part number for this HCA
  fw_ver        | text |           | extended |              | firmware version for this HCA
  hw_rev        | text |           | extended |              | hardware revision for this HCA
  board_id      | text |           | extended |              | board id for this HCA
 Indexes:
     "csm_hca_pkey" PRIMARY KEY, btree (node_name, serial_number)
 Foreign-key constraints:
     "csm_hca_node_name_fkey" FOREIGN KEY (node_name) REFERENCES csm_node(node_name)
 Triggers:
     tr_csm_hca_history_dump BEFORE INSERT OR DELETE OR UPDATE ON csm_hca FOR EACH ROW EXECUTE PROCEDURE fn_csm_hca_history_dump()
 Has OIDs: no

csm_hca_history
"""""""""""""""

**Description**
 This table contains historical information associated with the HCA (Host Channel Adapters).

=========== ==================================================== ==========================
 Table      Overview                                             Action On:
=========== ==================================================== ==========================
 Usage      | Low                                                |
 Size       | (Based on how many are changed out)                |
 Index      | ix_csm_hca_history_a on (history_time)             |
            | ix_csm_hca_history_b on (node_name, serial_number) |
            | ix_csm_hca_history_c on (ctid)                     |
            | ix_csm_hca_history_d on (archive_history_time)     |
=========== ==================================================== ==========================

csm_hca_history (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash 
 
                                                                               Table "public.csm_hca_history"
         Column        |            Type             | Modifiers | Storage  | Stats target |                                          Description
 ----------------------+-----------------------------+-----------+----------+--------------+------------------------------------------------------------------------------------------------
  history_time         | timestamp without time zone | not null  | plain    |              | the time when the HCA is entering the history table
  serial_number        | text                        | not null  | extended |              | unique serial number for this HCA
  node_name            | text                        |           | extended |              | node this HCA is installed in
  device_name          | text                        |           | extended |              | product device name for this HCA
  pci_bus_id           | text                        | not null  | extended |              | PCI bus id for this HCA
  guid                 | text                        | not null  | extended |              | sys_image_guid for this HCA
  part_number          | text                        |           | extended |              | part number for this HCA
  fw_ver               | text                        |           | extended |              | firmware version for this HCA
  hw_rev               | text                        |           | extended |              | hardware revision for this HCA
  board_id             | text                        |           | extended |              | board id for this HCA
  operation            | character(1)                | not null  | extended |              | operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)
  archive_history_time | timestamp without time zone |           | plain    |              | timestamp when the history data has been archived and sent to: BDS, archive file, and or other
 Indexes:
     "ix_csm_hca_history_a" btree (history_time)
     "ix_csm_hca_history_b" btree (node_name, serial_number)
     "ix_csm_hca_history_c" btree (ctid)
     "ix_csm_hca_history_d" btree (archive_history_time)
 Has OIDs: no

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

csm_dimm (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash 

                                              Table "public.csm_dimm"
       Column       |  Type   | Modifiers | Storage  | Stats target |                 Description
 -------------------+---------+-----------+----------+--------------+----------------------------------------------
  serial_number     | text    | not null  | extended |              | this is the dimm serial number
  node_name         | text    | not null  | extended |              | where does this dimm reside
  size              | integer | not null  | plain    |              | the size can be 4, 8, 16, 32 GB
  physical_location | text    | not null  | extended |              | phyical location where the dimm is installed
 Indexes:
     "csm_dimm_pkey" PRIMARY KEY, btree (serial_number, node_name)
 Foreign-key constraints:
     "csm_dimm_node_name_fkey" FOREIGN KEY (node_name) REFERENCES csm_node(node_name)
 Triggers:
     tr_csm_dimm_history_dump BEFORE INSERT OR DELETE OR UPDATE ON csm_dimm FOR EACH ROW EXECUTE PROCEDURE fn_csm_dimm_history_dump()
 Has OIDs: no

csm_dimm_history
""""""""""""""""

**Description** 
 This table contains historical information related to the DIMM "Dual In-Line Memory Module" attributes.

=========== ===================================================== ==========================
 Table      Overview                                              Action On:
=========== ===================================================== ==========================
 Usage      | Low                                                 |
 Size       | (Based on how many are changed out)                 |
 Index      | ix_csm_dimm_history_a on (history_time)             |
            | ix_csm_dimm_history_b on (node_name, serial_number) |
            | ix_csm_dimm_history_c on (ctid)                     |
            | ix_csm_dimm_history_d on (archive_history_time)     |
=========== ===================================================== ==========================  

csm_dimm_history (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                               Table "public.csm_dimm_history"
         Column        |            Type             | Modifiers | Storage  | Stats target |                                          Description
 ----------------------+-----------------------------+-----------+----------+--------------+------------------------------------------------------------------------------------------------
  history_time         | timestamp without time zone | not null  | plain    |              | this is when the information is entered into the history table
  serial_number        | text                        | not null  | extended |              | this is the dimm serial number
  node_name            | text                        |           | extended |              | where does this dimm reside
  size                 | integer                     | not null  | plain    |              | the size can be 4, 8, 16, 32 GB
  physical_location    | text                        | not null  | extended |              | physical location where the dimm is installed
  operation            | character(1)                | not null  | extended |              | operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)
  archive_history_time | timestamp without time zone |           | plain    |              | timestamp when the history data has been archived and sent to: BDS, archive file, and or other
 Indexes:
     "ix_csm_dimm_history_a" btree (history_time)
     "ix_csm_dimm_history_b" btree (node_name, serial_number)
     "ix_csm_dimm_history_c" btree (ctid)
     "ix_csm_dimm_history_d" btree (archive_history_time)
 Has OIDs: no

Allocation tables
^^^^^^^^^^^^^^^^^

csm_allocation
""""""""""""""

**Description**
 This table contains the information about the system’s current allocations. See table below for details.

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

csm_allocation (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                                                                           Table "public.csm_allocation"
         Column        |            Type             |                               Modifiers                                | Storage  | Stats target |                                                  Description
 ----------------------+-----------------------------+------------------------------------------------------------------------+----------+--------------+--------------------------------------------------------------------------------------------------------------------------
  allocation_id        | bigint                      | not null default nextval('csm_allocation_allocation_id_seq'::regclass) | plain    |              | unique identifier for this allocation
  primary_job_id       | bigint                      | not null                                                               | plain    |              | primary job id (for lsf this will be the lsf job id)
  secondary_job_id     | integer                     |                                                                        | plain    |              | secondary job id (for lsf this will be the lsf job index for job arrays)
  ssd_file_system_name | text                        |                                                                        | extended |              | the filesystem name that the user wants (ssd)
  launch_node_name     | text                        | not null                                                               | extended |              | launch node name
  isolated_cores       | integer                     | default 0                                                              | plain    |              | cgroup: 0 - No cgroups, 1 - Allocation Cgroup, 2 - Allocation and Core Isolation Cgroup, >2 || <0 unsupported
  user_flags           | text                        |                                                                        | extended |              | user
  space prolog/epilog flags
  system_flags         | text                        |                                                                        | extended |              | system space prolog/epilog flags
  ssd_min              | bigint                      |                                                                        | plain    |              | minimum ssd size (in bytes) for this allocation
  ssd_max              | bigint                      |                                                                        | plain    |              | maximum ssd size (in bytes) for this allocation
  num_nodes            | integer                     | not null                                                               | plain    |              | number of nodes in this allocation,also see csm_node_allocation
  num_processors       | integer                     | not null                                                               | plain    |              | total number of processes running in this allocation
  num_gpus             | integer                     | not null                                                               | plain    |              | the number of gpus that are available
  projected_memory     | integer                     | not null                                                               | plain    |              | the amount of memory available
  state                | text                        | not null                                                               | extended |              | state can be: stage in allocation, running allocation, stage out allocation
  type                 | text                        | not null                                                               | extended |              | shared allocation, user managed sub-allocation, pmix managed allocation, pmix managed allocation with c groups for steps
  job_type             | text                        | not null                                                               | extended |              | the type of job (batch or interactive)
  user_name            | text                        | not null                                                               | extended |              | user name
  user_id              | integer                     | not null                                                               | plain    |              | user identification
  user_group_id        | integer                     | not null                                                               | plain    |              | user group identification
  user_group_name      | text                        |                                                                        | extended |              | user group name
  user_script          | text                        | not null                                                               | extended |              | user script information
  begin_time           | timestamp without time zone | not null                                                               | plain    |              | timestamp when this allocation was created
  account              | text                        | not null                                                               | extended |              | account the job ran under
  comment              | text                        |                                                                        | extended |              | comments for the allocation
  job_name             | text                        |                                                                        | extended |              | jobname
  job_submit_time      | timestamp without time zone | not null                                                               | plain    |              | the time and data stamp the job was submitted
  queue                | text                        |                                                                        | extended |              | identifies the partition (queue) on which the job ran
  requeue              | text                        |                                                                        | extended |              | identifies (requeue) if the allocation is requeued it will attempt to have the previous allocation id
  time_limit           | bigint                      | not null                                                               | plain    |              | the time limit requested or imposed on the job
  wc_key               | text                        |                                                                        | extended |              | arbitrary string for grouping orthogonal accounts together
  smt_mode             | smallint                    | default 0                                                              | plain    |              | the smt mode of the allocation
  core_blink           | boolean                     | not null                                                               | plain    |              | flag indicating whether or not to run a blink operation on allocation cores.

 Indexes:
     "csm_allocation_pkey" PRIMARY KEY, btree (allocation_id)
 Referenced by:
     TABLE "csm_allocation_node" CONSTRAINT "csm_allocation_node_allocation_id_fkey" FOREIGN KEY (allocation_id) REFERENCES csm_allocation(allocation_id)
     TABLE "csm_step" CONSTRAINT "csm_step_allocation_id_fkey" FOREIGN KEY (allocation_id) REFERENCES csm_allocation(allocation_id)
 Triggers:
     tr_csm_allocation_state_change BEFORE INSERT OR UPDATE OF state ON csm_allocation FOR EACH ROW EXECUTE PROCEDURE fn_csm_allocation_state_history_state_change()
     tr_csm_allocation_update BEFORE UPDATE OF allocation_id, primary_job_id, secondary_job_id, ssd_file_system_name, launch_node_name, isolated_cores, user_flags, system_flags, ssd_min, ssd_max, num_nodes, num_processors, num_gpus, projected_memory, type, job_type, user_name, user_id, user_group_id, user_group_name, user_script, begin_time, account, comment, job_name, job_submit_time, queue, requeue, time_limit, wc_key, smt_mode, core_blink ON csm_allocation FOR EACH ROW EXECUTE PROCEDURE fn_csm_allocation_update()
 Has OIDs: no

csm_allocation_history
""""""""""""""""""""""

**Description**
 This table contains the information about the no longer current allocations on the system.  Essentially this is the historical information about allocations. This table will increase in size only based on how many allocations are deployed on the life cycle of the machine/system.  This table will also be able to determine the total energy consumed per allocation (filled in during "free of allocation").

=========== ======================================================= ==========================
 Table      Overview                                                Action On:
=========== ======================================================= ==========================
 Usage      | High                                                  |
 Size       | (Depending on customers work load (100,000+ rows))    |
 Index      | ix_csm_allocation_history_a on (history_time)         |
            | ix_csm_allocation_history_b on (allocation_id)        |
            | ix_csm_allocation_history_c on (ctid)                 |
            | ix_csm_allocation_history_d on (archive_history_time) |
=========== ======================================================= ==========================

csm_allocation_history (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                                   Table "public.csm_allocation_history"
         Column        |            Type             | Modifiers | Storage  | Stats target |                                                  Description
 ----------------------+-----------------------------+-----------+----------+--------------+---------------------------------------------------------------------------------------------------------------
  history_time         | timestamp without time zone |           | plain    |              | time when the allocation is entered into the history table
  allocation_id        | bigint                      |           | plain    |              | unique identifier for this allocation
  primary_job_id       | bigint                      | not null  | plain    |              | primary job id (for lsf this will be the lsf job id)
  secondary_job_id     | integer                     |           | plain    |              | secondary job id (for lsf this will be the lsf job index)
  ssd_file_system_name | text                        |           | extended |              | the filesystem name that the user wants (ssd)
  launch_node_name     | text                        | not null  | extended |              | launch node name
  isolated_cores       | integer                     |           | plain    |              | cgroup: 0 - No cgroups, 1 - Allocation Cgroup, 2 - Allocation and
  Core Isolation Cgroup, >2 || <0 unsupported
  user_flags           | text                        |           | extended |              | user space prolog/epilog flags
  system_flags         | text                        |           | extended |              | system space prolog/epilog flags
  ssd_min              | bigint                      |           | plain    |              | minimum ssd size (in bytes) for this allocation
  ssd_max              | bigint                      |           | plain    |              | maximum ssd size (in bytes) for this allocation
  num_nodes            | integer                     | not null  | plain    |              | number of nodes in allocation, see csm_node_allocation
  num_processors       | integer                     | not null  | plain    |              | total number of processes running in this allocation
  num_gpus             | integer                     | not null  | plain    |              | the number of gpus that are available
  projected_memory     | integer                     | not null  | plain    |              | the amount of memory available
  state                | text                        | not null  | extended |              | state of the node - stage in allocation, running allocation, stage out allocation
  type                 | text                        | not null  | extended |              | user managed sub-allocation, pmix managed allocation, pmix managed allocation with c groups for steps
  job_type             | text                        | not null  | extended |              | the type of job (batch or interactive)
  user_name            | text                        | not null  | extended |              | username
  user_id              | integer                     | not null  | plain    |              | user identification id
  user_group_id        | integer                     | not null  | plain    |              | user group identification
  user_group_name      | text                        |           | extended |              | user group name
  user_script          | text                        | not null  | extended |              | user script information
  begin_time           | timestamp without time zone | not null  | plain    |              | timestamp when this allocation was created
  end_time             | timestamp without time zone |           | plain    |              | timestamp when this allocation was freed
  exit_status          | integer                     |           | plain    |              | allocation exit status
  account              | text                        | not null  | extended |              | account the job ran under
  comment              | text                        |           | extended |              | comments for the allocation
  job_name             | text                        |           | extended |              | job name
  job_submit_time      | timestamp without time zone | not null  | plain    |              | the time and date stamp the job was submitted
  queue                | text                        |           | extended |              | identifies the partition (queue) on which the job ran
  requeue              | text                        |           | extended |              | identifies (requeue) if the allocation is requeued it will attempt to have the previous allocation id
  time_limit           | bigint                      | not null  | plain    |              | the time limit requested or imposed on the job
  wc_key               | text                        |           | extended |              | arbitrary string for grouping orthogonal accounts together
  archive_history_time | timestamp without time zone |           | plain    |              | timestamp when the history data has been archived and sent to: BDS, archive file, and or other
  smt_mode             | smallint                    | default 0 | plain    |              | the smt mode of the allocation
  core_blink           | boolean                     | not_null  | plain    |              | flag indicating whether or not to run a blink operation on allocation cores.
 Indexes:
     "ix_csm_allocation_history_a" btree (history_time)
     "ix_csm_allocation_history_b" btree (allocation_id)
     "ix_csm_allocation_history_c" btree (ctid)
     "ix_csm_allocation_history_d" btree (archive_history_time)
 Has OIDs: no

Step tables
^^^^^^^^^^^

csm_step
""""""""

**Description**
 This table contains information on active steps within the CSM database. See table below for details.

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

csm_step (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                 Table "public.csm_step"
         Column        |            Type             | Modifiers | Storage  | Stats target |                        Description
 ----------------------+-----------------------------+-----------+----------+--------------+------------------------------------------------------------
  step_id              | bigint                      | not null  | plain    |              | uniquely identify this step
  allocation_id        | bigint                      | not null  | plain    |              | allocation that this step is part of
  begin_time           | timestamp without time zone | not null  | plain    |              | timestamp when this job step started
  status               | text                        | not null  | extended |              | the active status of the step
  executable           | text                        | not null  | extended |              | executable / command name / application name
  working_directory    | text                        | not null  | extended |              | working directory
  argument             | text                        | not null  | extended |              | arguments / parameters
  environment_variable | text                        | not null  | extended |              | environment variables
  num_nodes            | integer                     | not null  | plain    |              | the specific number of nodes that are involved in the step
  num_processors       | integer                     | not null  | plain    |              | total number of processes running in this step
  num_gpus             | integer                     | not null  | plain    |              | the number of gpus that are available
  projected_memory     | integer                     | not null  | plain    |              | the projected amount of memory available for the step
  num_tasks            | integer                     | not null  | plain    |              | total number of tasks in a job or step
  user_flags           | text                        |           | extended |              | user space prolog/epilog flags
 Indexes:
     "csm_step_pkey" PRIMARY KEY, btree (step_id, allocation_id)
     "uk_csm_step_a" UNIQUE, btree (step_id, allocation_id)
 Foreign-key constraints:
     "csm_step_allocation_id_fkey" FOREIGN KEY (allocation_id) REFERENCES csm_allocation(allocation_id)
 Referenced by:
     TABLE "csm_step_node" CONSTRAINT "csm_step_node_step_id_fkey" FOREIGN KEY (step_id, allocation_id) REFERENCES csm_step(step_id, allocation_id)
 Has OIDs: no

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
            | ix_csm_step_history_f on (ctid)                          |
            | ix_csm_step_history_g on (archive_history_time)          |
=========== ========================================================== ==========================

csm_step_history (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                                             Table "public.csm_step_history"
         Column        |            Type             | Modifiers | Storage  | Stats target |                                                        Description
 ----------------------+-----------------------------+-----------+----------+--------------+----------------------------------------------------------------------------------------------------------------------------
  history_time         | timestamp without time zone | not null  | plain    |              | timestamp when it enters the history table
  step_id              | bigint                      | not null  | plain    |              | uniquely identify this step
  allocation_id        | bigint                      | not null  | plain    |              | allocation that this step is part of
  begin_time           | timestamp without time zone | not null  | plain    |              | timestamp when this job step started
  end_time             | timestamp without time zone |           | plain    |              | timestamp when this step ended
  status               | text                        | not null  | extended |              | the active operating status of the state
  executable           | text                        | not null  | extended |              | executable / command name / application name
  working_directory    | text                        | not null  | extended |              | working directory
  argument             | text                        | not null  | extended |              | arguments / parameters
  environment_variable | text                        | not null  | extended |              | environment variables
  num_nodes            | integer                     | not null  | plain    |              | the specific number of nodes that are involved in the step
  num_processors       | integer                     | not null  | plain    |              | total number of processes running in this step
  num_gpus             | integer                     | not null  | plain    |              | the number of gpus available
  projected_memory     | integer                     | not null  | plain    |              | the number of memory available
  num_tasks            | integer                     | not null  | plain    |              | total number of tasks in a job or step
  user_flags           | text                        |           | extended |              | user space prolog/epilog flags
  exit_status          | integer                     |           | plain    |              | step/s exit status. will be tracked and given to csm by job leader
  error_message        | text                        |           | extended |              | step/s error text. will be tracked and given to csm by job leader. the following columns need their proper data types tbd:
  cpu_stats            | text                        |           | extended |              | statistics gathered from the CPU for the step.
  total_u_time         | double precision            |           | plain    |              | relates to the (us) (aka: user mode) value of %cpu(s) of the (top) linux cmd. todo: design how we get this data
  total_s_time         | double precision            |           | plain    |              | relates to the (sy) (aka: system mode) value of %cpu(s) of the (top) linux cmd. todo: design how we get this data
  omp_thread_limit     | text                        |           | extended |              | max number of omp threads used by the step.
  gpu_stats            | text                        |           | extended |              | statistics gathered from the GPU for the step.
  memory_stats         | text                        |           | extended |              | memory statistics for the the step (bytes).
  max_memory           | bigint                      |           | plain    |              | the maximum memory usage of the step (bytes).
  io_stats             | text                        |           | extended |              | general input output statistics for the step.
  archive_history_time | timestamp without time zone |           | plain    |              | timestamp when the history data has been archived and sent to: BDS, archive file, and or other
 Indexes:
     "ix_csm_step_history_a" btree (history_time)
     "ix_csm_step_history_b" btree (begin_time, end_time)
     "ix_csm_step_history_c" btree (allocation_id, end_time)
     "ix_csm_step_history_d" btree (end_time)
     "ix_csm_step_history_e" btree (step_id)
     "ix_csm_step_history_f" btree (ctid)
     "ix_csm_step_history_g" btree (archive_history_time)
 Has OIDs: no

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

csm_allocation_node (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                    Table "public.csm_allocation_node"
         Column        |  Type   | Modifiers | Storage  | Stats target |                                           Description
 ----------------------+---------+-----------+----------+--------------+--------------------------------------------------------------------------------------------------
  allocation_id        | bigint  | not null  | plain    |              | allocation that node_name is part of
  node_name            | text    | not null  | extended |              | identifies which node this is
  state                | text    | not null  | extended |              | state can be: stage in allocation, running allocation, stage out allocation
  shared               | boolean | not null  | plain    |              | indicates if the node resources are shareable
  energy               | bigint  |           | plain    |              | the total energy used by the node in joules during the allocation
  gpfs_read            | bigint  |           | plain    |              | bytes read counter (net) at the start of the allocation.
  gpfs_write           | bigint  |           | plain    |              | bytes written counter (net) at the start of the allocation.
  ib_tx                | bigint  |           | plain    |              | count of data octets transmitted on all port VLs (1/4 of a byte) at the start of the allocation.
  ib_rx                | bigint  |           | plain    |              | Count of data octets received on all port VLs (1/4 of a byte) at the start of the allocation.
  power_cap            | integer |           | plain    |              | power cap currently in effect for this node (in watts)
  power_shifting_ratio | integer |           | plain    |              | power power shifting ratio currently in effect for this node
  power_cap_hit        | bigint  |           | plain    |              | total number of windowed ticks the processor frequency was reduced
  gpu_usage            | bigint  |           | plain    |              | the total usage aggregated across all GPUs in the node in microseconds during the allocation
  gpu_energy           | bigint  |           | plain    |              | the total energy used across all GPUs in the node in joules during the allocation
  cpu_usage            | bigint  |           | plain    |              | the cpu usage in nanoseconds
  memory_usage_max     | bigint  |           | plain    |              | The high water mark for memory usage (bytes).
 Indexes:
     "uk_csm_allocation_node_b" UNIQUE, btree (allocation_id, node_name)
     "ix_csm_allocation_node_a" btree (allocation_id)
 Foreign-key constraints:
     "csm_allocation_node_allocation_id_fkey" FOREIGN KEY (allocation_id) REFERENCES csm_allocation(allocation_id)
     "csm_allocation_node_node_name_fkey" FOREIGN KEY (node_name) REFERENCES csm_node(node_name)
 Referenced by:
     TABLE "csm_lv" CONSTRAINT "csm_lv_allocation_id_fkey" FOREIGN KEY (allocation_id, node_name) REFERENCES csm_allocation_node(allocation_id, node_name)
     TABLE "csm_step_node" CONSTRAINT "csm_step_node_allocation_id_fkey" FOREIGN KEY (allocation_id, node_name) REFERENCES csm_allocation_node(allocation_id, node_name)
 Triggers:
     tr_csm_allocation_node_change BEFORE UPDATE ON csm_allocation_node FOR EACH ROW EXECUTE PROCEDURE fn_csm_allocation_node_change()
 Has OIDs: no

csm_allocation_node_history
"""""""""""""""""""""""""""

**Description**
 This table maps history allocations to the compute nodes that make up the allocation.

=========== ============================================================ ==========================
 Table      Overview                                                     Action On:
=========== ============================================================ ==========================
 Usage      | High                                                       |
 Size       | 1-5000 rows                                                |
 Index      | ix_csm_allocation_node_history_a on (history_time)         |
            | ix_csm_allocation_node_history_b on (allocation_id)        |
            | ix_csm_allocation_node_history_c on (ctid)                 |
            | ix_csm_allocation_node_history_d on (archive_history_time) |
=========== ============================================================ ==========================

csm_allocation_node_history (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                                                                          Table "public.csm_allocation_node_history"
         Column        |            Type             | Modifiers | Storage  | Stats target |                         Description
 ----------------------+-----------------------------+-----------+----------+--------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  history_time         | timestamp without time zone | not null  | plain    |              | timestamp when it enters the history table
  allocation_id        | bigint                      | not null  | plain    |              | allocation that node_name is part of
  node_name            | text                        | not null  | extended |              | identifies which node this is
  state                | text                        | not null  | extended |              | state can be: stage in allocation, running allocation, stage out allocation
  shared               | boolean                     |           | plain    |              | indicates if the node resources are shareable
  energy               | bigint                      |           | plain    |              | the total energy used by the node in joules during the allocation
  gpfs_read            | bigint                      |           | plain    |              | total bytes read counter (net) at the during the allocation. Negative values represent the start reading, indicating the end was never writen to the database.
  gpfs_write           | bigint                      |           | plain    |              | total bytes written counter (net) at the during the allocation. Negative values represent the start reading, indicating the end was never writen to the database.
  ib_tx                | bigint                      |           | plain    |              | total count of data octets transmitted on all port VLs (1/4 of a byte) during the allocation. Negative values represent the start reading, indicating the end was never writen to the database.
  ib_rx                | bigint                      |           | plain    |              | total count of data octets received on all port VLs (1/4 of a byte) during the allocation. Negative values represent the start reading, indicating the end was never writen to the database.
  power_cap            | integer                     |           | plain    |              | power cap currently in effect for this node (in watts)
  power_shifting_ratio | integer                     |           | plain    |              | power power shifting ratio currently in effect for this node
  power_cap_hit        | bigint                      |           | plain    |              | total number of windowed ticks the processor frequency was reduced
  gpu_usage            | bigint                      |           | plain    |              | the total usage aggregated across all GPUs in the node in microseconds during the allocation
  gpu_energy           | bigint                      |           | plain    |              | the total energy used across all GPUs in the node in joules during the allocation
  cpu_usage            | bigint                      |           | plain    |              | the cpu usage in nanoseconds
  memory_usage_max     | bigint                      |           | plain    |              | The high water mark for memory usage (bytes).
  archive_history_time | timestamp without time zone |           | plain    |              | timestamp when the history data has been archived and sent to: BDS, archive file, and or other
 Indexes:
     "ix_csm_allocation_node_history_a" btree (history_time)
     "ix_csm_allocation_node_history_b" btree (allocation_id)
     "ix_csm_allocation_node_history_c" btree (ctid)
     "ix_csm_allocation_node_history_d" btree (archive_history_time)
 Has OIDs: no

csm_allocation_state_history
""""""""""""""""""""""""""""

**Description**
 This table contains the state of the active allocations history. A timestamp of when the information enters the table along with a state indicator.

=========== ============================================================= ==========================
 Table      Overview                                                      Action On:
=========== ============================================================= ==========================
 Usage      | High                                                        |
 Size       | 1-5000 rows (one per allocation)                            |
 Index      | ix_csm_allocation_state_history_a on (history_time)         |
            | ix_csm_allocation_state_history_b on (allocation_id)        |
            | ix_csm_allocation_state_history_c on (ctid)                 |
            | ix_csm_allocation_state_history_d on (archive_history_time) |
=========== ============================================================= ==========================

csm_allocation_state_history (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                         Table "public.csm_allocation_state_history"
         Column        |            Type             | Modifiers | Storage  | Stats target |                                          Description
 ----------------------+-----------------------------+-----------+----------+--------------+------------------------------------------------------------------------------------------------
  history_time         | timestamp without time zone | not null  | plain    |              | timestamp when this allocation changes state
  allocation_id        | bigint                      |           | plain    |              | uniquely identify this allocation
  exit_status          | integer                     |           | plain    |              | the error code returned at the end of the allocation state
  state                | text                        | not null  | extended |              | state of this allocation (stage-in, running, stage-out)
  archive_history_time | timestamp without time zone |           | plain    |              | timestamp when the history data has been archived and sent to: BDS, archive file, and or other
 Indexes:
     "ix_csm_allocation_state_history_a" btree (history_time)
     "ix_csm_allocation_state_history_b" btree (allocation_id)
     "ix_csm_allocation_state_history_c" btree (ctid)
     "ix_csm_allocation_state_history_d" btree (archive_history_time)
 Has OIDs: no

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
            | ix_csm_step_node_b on (allocation_id)                     |
            | ix_csm_step_node_c on (allocation_id, step_id)            |
 Functions  | fn_csm_step_node_history_dump                             |
 Triggers   | tr_csm_step_node_history_dump                             | delete
=========== =========================================================== ==============

csm_step_node (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                     Table "public.csm_step_node"
     Column     |  Type  | Modifiers | Storage  | Stats target |             Description
 ---------------+--------+-----------+----------+--------------+--------------------------------------
  step_id       | bigint | not null  | plain    |              | uniquely identify this step
  allocation_id | bigint | not null  | plain    |              | allocation that this step is part of
  node_name     | text   | not null  | extended |              | identifies the node
 Indexes:
     "uk_csm_step_node_a" UNIQUE, btree (step_id, allocation_id, node_name)
     "ix_csm_step_node_b" btree (allocation_id)
     "ix_csm_step_node_c" btree (allocation_id, step_id)
 Foreign-key constraints:
     "csm_step_node_allocation_id_fkey" FOREIGN KEY (allocation_id, node_name) REFERENCES csm_allocation_node(allocation_id, node_name)
     "csm_step_node_step_id_fkey" FOREIGN KEY (step_id, allocation_id) REFERENCES csm_step(step_id, allocation_id)
 Triggers:
     tr_csm_step_node_history_dump BEFORE DELETE ON csm_step_node FOR EACH ROW EXECUTE PROCEDURE fn_csm_step_node_history_dump()
 Has OIDs: no

csm_step_node_history
"""""""""""""""""""""

**Description**
 This table maps historical allocations to jobs steps and nodes.

=========== ======================================================== ==========================
 Table      Overview                                                 Action On:
=========== ======================================================== ==========================
 Usage      | High                                                   |
 Size       | 5000+ rows (based on steps)                            |
 Index      | ix_csm_step_node_history_a on (history_time)           |
            | ix_csm_step_node_history_b on (allocation_id)          |
            | ix_csm_step_node_history_c on (allocation_id, step_id) |
            | ix_csm_step_node_history_d on (ctid)                   |
            | ix_csm_step_node_history_e on (archive_history_time)   |
=========== ======================================================== ==========================

csm_step_node_history (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                            Table "public.csm_step_node_history"
         Column        |            Type             | Modifiers | Storage  | Stats target |                                          Description
 ----------------------+-----------------------------+-----------+----------+--------------+------------------------------------------------------------------------------------------------
  history_time         | timestamp without time zone |           | plain    |              | historical time when information is added to the history table
  step_id              | bigint                      |           | plain    |              | uniquely identify this step
  allocation_id        | bigint                      |           | plain    |              | allocation that this step is part of
  node_name            | text                        |           | extended |              | identifies the node
  archive_history_time | timestamp without time zone |           | plain    |              | timestamp when the history data has been archived and sent to: BDS, archive file, and or other
 Indexes:
     "ix_csm_step_node_history_a" btree (history_time)
     "ix_csm_step_node_history_b" btree (allocation_id)
     "ix_csm_step_node_history_c" btree (allocation_id, step_id)
     "ix_csm_step_node_history_d" btree (ctid)
     "ix_csm_step_node_history_e" btree (archive_history_time)
 Has OIDs: no

RAS tables
^^^^^^^^^^

csm_ras_type
""""""""""""

**Description**
 This table contains the description and details for each of the possible RAS event types. See table below for details.

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

csm_ras_type (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                                                    Table "public.csm_ras_type"
       Column      |        Type         | Modifiers | Storage  | Stats target |                                                                  Description
 ------------------+---------------------+-----------+----------+--------------+------------------------------------------------------------------------------------------------------------------------------------------------
  msg_id           | text                | not null  | extended |              | the identifier string for this RAS event. It must be unique.  typically it consists of three parts separated by periods (system.component.id).
  severity         | ras_event_severity  | not null  | plain    |              | severity of the RAS event. INFO/WARNING/FATAL
  message          | text                |           | extended |              | ras message to display to the user (pre-variable substitution)
  description      | text                |           | extended |              | description of the ras event
  control_action   | text                |           | extended |              | name of control action script to invoke for this event.
  threshold_count  | integer             |           | plain    |              | number of times this event has to occur during the (threshold_period) before taking action on the RAS event.
  threshold_period | integer             |           | plain    |              | period in seconds over which to compare the number of event occurences to the threshold_count ).
  enabled          | boolean             |           | plain    |              | events will be processed if enabled=true and suppressed if enabled=false
  set_state        | compute_node_states |           | plain    |              | setting the state according to the node, DISCOVERED, IN_SERVICE, ADMIN_RESERVED, MAINTENANCE, SOFT_FAILURE, OUT_OF_SERVICE, HARD_FAILURE
  visible_to_users | boolean             |           | plain    |              | when visible_to_users=true, RAS events of this type will be returned in the response to csm_ras_event_query_allocation
 Indexes:
     "csm_ras_type_pkey" PRIMARY KEY, btree (msg_id)
 Triggers:
     tr_csm_ras_type_update AFTER INSERT OR DELETE OR UPDATE ON csm_ras_type FOR EACH ROW EXECUTE PROCEDURE fn_csm_ras_type_update()
 Has OIDs: no

csm_ras_type_audit
""""""""""""""""""

**Description**
 This table contains historical descriptions and details for each of the possible RAS event types. See table below for details.

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

csm_ras_type_audit (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                                                                                Table "public.csm_ras_type_audit"
       Column      |            Type             |                                Modifiers                                | Storage  | Stats target |                                                       Description
 ------------------+-----------------------------+-------------------------------------------------------------------------+----------+--------------+------------------------------------------------------------------------------------------------------------------------------------------
  msg_id_seq       | bigint                      | not null default nextval('csm_ras_type_audit_msg_id_seq_seq'::regclass) | plain    |              | a unique sequence number used to index the csm_ras_type_audit table
  operation        | character(1)                | not null                                                                | extended |              | I/D/U indicates whether the change to the csm_ras_type table was an INSERT, DELETE, or UPDATE
  change_time      | timestamp without time zone | not null                                                                | plain    |              | time_stamp indicating when this change occurred
  msg_id           | text                        | not null                                                                | extended |              | the identifier string for this RAS event. typically it consists of three parts separated by periods (system.component.id).
  severity         | ras_event_severity          | not null                                                                | plain    |              | severity of the RAS event. INFO/WARNING/FATAL
  message          | text                        |                                                                         | extended |              | ras message to display to the user (pre-variable substitution)
  description      | text                        |                                                                         | extended |              | description of the ras event
  control_action   | text                        |                                                                         | extended |              | name of control action script to invoke for this event.
  threshold_count  | integer                     |                                                                         | plain    |              | number of times this event has to occur during the (threshold_period) before taking action on the RAS event.
  threshold_period | integer                     |                                                                         | plain    |              | period in seconds over which to compare the number of event occurences to the threshold_count ).
  enabled          | boolean                     |                                                                         | plain    |              | events will be processed if enabled=true and suppressed if enabled=false
  set_state        | compute_node_states         |                                                                         | plain    |              | setting the state according to the node, DISCOVERED, IN_SERVICE, ADMIN_RESERVED, MAINTENANCE, SOFT_FAILURE, OUT_OF_SERVICE, HARD_FAILURE
  visible_to_users | boolean                     |                                                                         | plain    |              | when visible_to_users=true, RAS events of this type will be returned in the response to csm_ras_event_query_allocation
 Indexes:
     "csm_ras_type_audit_pkey" PRIMARY KEY, btree (msg_id_seq)
 Referenced by:
     TABLE "csm_ras_event_action" CONSTRAINT "csm_ras_event_action_msg_id_seq_fkey" FOREIGN KEY (msg_id_seq) REFERENCES csm_ras_type_audit(msg_id_seq)
 Has OIDs: no

.. _csm_ras_event_action:

csm_ras_event_action
""""""""""""""""""""

**Description**
 This table contains all RAS events. This table will populate an enormous amount of records due to continuous event cycle.  A solution needs to be in place to accommodate the mass amount of data produced. See table below for details.

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
            | ix_csm_ras_event_action_f on (master_time_stamp)         |
            | ix_csm_ras_event_action_g on (ctid)                      |
            | ix_csm_ras_event_action_h on (archive_history_time)      |
=========== ========================================================== ==========================

csm_ras_event_action (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                                                                                   Table "public.csm_ras_event_action"
         Column        |            Type             |                               Modifiers                               | Storage  | Stats target |                                                            Description
 ----------------------+-----------------------------+-----------------------------------------------------------------------+----------+--------------+------------------------------------------------------------------------------------------------------------------------------------------------
  rec_id               | bigint                      | not null default nextval('csm_ras_event_action_rec_id_seq'::regclass) | plain    |              | unique identifier for this specific ras event
  msg_id               | text                        | not null                                                              | extended |              | type of ras event
  msg_id_seq           | integer                     | not null                                                              | plain    |              | a unique sequence number used to index the csm_ras_type_audit table
  time_stamp           | timestamp without time zone | not null                                                              | plain    |              | The time supplied by the caller of csm_ras_event_create. Used for correlating between events based on the local time of the event source.
  master_time_stamp    | timestamp without time zone | not null                                                              | plain    |              | The time when the event is process by the CSM master daemon. Used for correlating node state changes with CSM master processing of RAS events.
  location_name        | text                        | not null                                                              | extended |              | this field can be a node name or location name
  count                | integer                     |                                                                       | plain    |              | how many times this event reoccurs
  message              | text                        |                                                                       | extended |              | message text
  kvcsv                | text                        |                                                                       | extended |              | event specific keys and values in a comma separated list
  raw_data             | text                        |                                                                       | extended |              | event/s raw data
  archive_history_time | timestamp without time zone |                                                                       | plain    |              | timestamp when the history data has been archived and sent to: BDS, archive file, and or other
 Indexes:
     "csm_ras_event_action_pkey" PRIMARY KEY, btree (rec_id)
     "ix_csm_ras_event_action_a" btree (msg_id)
     "ix_csm_ras_event_action_b" btree (time_stamp)
     "ix_csm_ras_event_action_c" btree (location_name)
     "ix_csm_ras_event_action_d" btree (time_stamp, msg_id)
     "ix_csm_ras_event_action_e" btree (time_stamp, location_name)
     "ix_csm_ras_event_action_f" btree (master_time_stamp)
     "ix_csm_ras_event_action_g" btree (ctid)
     "ix_csm_ras_event_action_h" btree (archive_history_time)
 Foreign-key constraints:
     "csm_ras_event_action_msg_id_seq_fkey" FOREIGN KEY (msg_id_seq) REFERENCES csm_ras_type_audit(msg_id_seq)
 Has OIDs: no

CSM diagnostic tables
^^^^^^^^^^^^^^^^^^^^^

csm_diag_run
""""""""""""

**Description**
 This table contains information about each of the diagnostic runs. See table below for details.

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

csm_diag_run (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                              Table "public.csm_diag_run"
     Column     |            Type             |             Modifiers              | Storage  | Stats target |                              Description
 ---------------+-----------------------------+------------------------------------+----------+--------------+------------------------------------------------------------------------
  run_id        | bigint                      | not null                           | plain    |              | diagnostic/s run id
  allocation_id | bigint                      |                                    | plain    |              | allocation that this diag_run is part of
  begin_time    | timestamp without time zone | not null default now()             | plain    |              | this is when the diagnostic run begins
  status        | character(16)               | not null default 'RUNNING'::bpchar | extended |              | diagnostic/s status (RUNNING,COMPLETED,FAILED,CANCELED,COMPLETED_FAIL)
  inserted_ras  | boolean                     | not null default false             | plain    |              | inserted diagnostic ras events t/f
  log_dir       | text                        | not null                           | extended |              | location of diagnostic/s log files
  cmd_line      | text                        |                                    | extended |              | how diagnostic program was invoked: program and
  arguments
 Indexes:
     "csm_diag_run_pkey" PRIMARY KEY, btree (run_id)
 Referenced by:
     TABLE "csm_diag_result" CONSTRAINT "csm_diag_result_run_id_fkey" FOREIGN KEY (run_id) REFERENCES csm_diag_run(run_id)
 Has OIDs: no

csm_diag_run_history
""""""""""""""""""""

**Description**
 This table contains historical information about each of the diagnostic runs. See table below for details.

=========== ===================================================== ==========================
 Table      Overview                                              Action On:
=========== ===================================================== ==========================
 Usage      | Low                                                 |
 Size       | 1000+ rows                                          |
 Index      | ix_csm_diag_run_history_a on (history_time)         |
            | ix_csm_diag_run_history_b on (run_id)               |
            | ix_csm_diag_run_history_c on (allocation_id)        |
            | ix_csm_diag_run_history_d on (ctid)                 |
            | ix_csm_diag_run_history_e on (archive_history_time) |
=========== ===================================================== ==========================  

csm_diag_run_history (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                             Table "public.csm_diag_run_history"
         Column        |            Type             | Modifiers | Storage  | Stats target |                                          Description
 ----------------------+-----------------------------+-----------+----------+--------------+------------------------------------------------------------------------------------------------
  history_time         | timestamp without time zone | not null  | plain    |              | timestamp when it enters the history table
  run_id               | bigint                      | not null  | plain    |              | diagnostic/s run id
  allocation_id        | bigint                      |           | plain    |              | allocation that this diag_run is part of
  begin_time           | timestamp without time zone | not null  | plain    |              | this is when the diagnostic run begins
  end_time             | timestamp without time zone |           | plain    |              | this is when the diagnostic run ends
  status               | character(16)               | not null  | extended |              | diagnostic/s status (RUNNING,COMPLETED,FAILED,CANCELED,COMPLETED_FAIL)
  inserted_ras         | boolean                     | not null  | plain    |              | inserted diagnostic ras events t/f
  log_dir              | text                        | not null  | extended |              | location of diagnostic/s log files
  cmd_line             | text                        |           | extended |              | how diagnostic program was invoked: program and arguments
  archive_history_time | timestamp without time zone |           | plain    |              | timestamp when the history data has been archived and sent to: BDS, archive file, and or other
 Indexes:
     "ix_csm_diag_run_history_a" btree (history_time)
     "ix_csm_diag_run_history_b" btree (run_id)
     "ix_csm_diag_run_history_c" btree (allocation_id)
     "ix_csm_diag_run_history_d" btree (ctid)
     "ix_csm_diag_run_history_e" btree (archive_history_time)
 Has OIDs: no
  
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

csm_diag_result (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                                Table "public.csm_diag_result"
     Column     |            Type             |         Modifiers         | Storage  | Stats target |                                      Description
 ---------------+-----------------------------+---------------------------+----------+--------------+----------------------------------------------------------------------------------------
  run_id        | bigint                      |                           | plain    |              | diagnostic/s run id
  test_name     | text                        | not null                  | extended |              | the name of the specific testcase
  node_name     | text                        | not null                  | extended |              | identifies which node
  serial_number | text                        |                           | extended |              | serial number of the field replaceable unit (fru) that this diagnostic was run against
  begin_time    | timestamp without time zone |                           | plain    |              | the time when the task begins
  end_time      | timestamp without time zone | default now()             | plain    |              | the time when the task is complete
  status        | character(16)               | default 'unknown'::bpchar | extended |              | test status after the diagnostic finishes (pass, fail, completed_fail)
  log_file      | text                        |                           | extended |              | location of diagnostic/s log file
 Indexes:
     "ix_csm_diag_result_a" btree (run_id, test_name, node_name)
 Foreign-key constraints:
     "csm_diag_result_run_id_fkey" FOREIGN KEY (run_id) REFERENCES csm_diag_run(run_id)
 Triggers:
     tr_csm_diag_result_history_dump BEFORE DELETE ON csm_diag_result FOR EACH ROW EXECUTE PROCEDURE fn_csm_diag_result_history_dump()
 Has OIDs: no

csm_diag_result_history
"""""""""""""""""""""""

**Description**
 This table contains historical results of a specific instance of a diagnostic.

=========== ======================================================== ==========================
 Table      Overview                                                 Action On:
=========== ======================================================== ==========================
 Usage      | Low                                                    |
 Size       | 1000+ rows                                             |
 Index      | ix_csm_diag_result_history_a on (history_time)         |
            | ix_csm_diag_result_history_b on (run_id)               |
            | ix_csm_diag_result_history_c on (ctid)                 |
            | ix_csm_diag_result_history_d on (archive_history_time) |
=========== ======================================================== ==========================

csm_diag_result_history (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                                   Table "public.csm_diag_result_history"
         Column        |            Type             |         Modifiers         | Storage  | Stats target |                                          Description
 ----------------------+-----------------------------+---------------------------+----------+--------------+------------------------------------------------------------------------------------------------
  history_time         | timestamp without time zone | not null                  | plain    |              | timestamp when it enters the history table
  run_id               | bigint                      |                           | plain    |              | diagnostic/s run id
  test_name            | text                        | not null                  | extended |              | the name of the specific testcase
  node_name            | text                        | not null                  | extended |              | identifies which node
  serial_number        | text                        |                           | extended |              | serial number of the field replaceable unit (fru) that this diagnostic was run against
  begin_time           | timestamp without time zone |                           | plain    |              | the time when the task begins
  end_time             | timestamp without time zone | default now()             | plain    |              | the time when the task is complete
  status               | character(16)               | default 'unknown'::bpchar | extended |              | test status after the diagnostic finishes (pass, fail, completed_fail)
  log_file             | text                        |                           | extended |              | location of diagnostic/s log file
  archive_history_time | timestamp without time zone |                           | plain    |              | timestamp when the history data has been archived and sent to: BDS, archive file, and or other
 Indexes:
     "ix_csm_diag_result_history_a" btree (history_time)
     "ix_csm_diag_result_history_b" btree (run_id)
     "ix_csm_diag_result_history_c" btree (ctid)
     "ix_csm_diag_result_history_d" btree (archive_history_time)
 Has OIDs: no

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

csm_lv (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                              Table "public.csm_lv"
        Column        |            Type             | Modifiers | Storage  | Stats target |                     Description
 ---------------------+-----------------------------+-----------+----------+--------------+-----------------------------------------------------
  logical_volume_name | text                        | not null  | extended |              | unique identifier for this ssd partition
  node_name           | text                        | not null  | extended |              | node a part of this group
  allocation_id       | bigint                      | not null  | plain    |              | unique identifier for this allocation
  vg_name             | text                        | not null  | extended |              | volume group name
  state               | character(1)                | not null  | extended |              | state: (c)reated, (m)ounted, (s)hrinking, (r)emoved
  current_size        | bigint                      | not null  | plain    |              | current size (in bytes)
  max_size            | bigint                      | not null  | plain    |              | max size (in bytes) at runtime
  begin_time          | timestamp without time zone | not null  | plain    |              | when the partitioning begins
  updated_time        | timestamp without time zone |           | plain    |              | when it was last updated
  file_system_mount   | text                        |           | extended |              | identifies the file system and mount point
  file_system_type    | text                        |           | extended |              | identifies the file system and its partition
 Indexes:
     "csm_lv_pkey" PRIMARY KEY, btree (logical_volume_name, node_name)
     "ix_csm_lv_a" btree (logical_volume_name)
 Foreign-key constraints:
     "csm_lv_allocation_id_fkey" FOREIGN KEY (allocation_id, node_name) REFERENCES csm_allocation_node(allocation_id, node_name)
     "csm_lv_node_name_fkey" FOREIGN KEY (node_name, vg_name) REFERENCES csm_vg(node_name, vg_name)
 Triggers:
     tr_csm_lv_update_history_dump BEFORE UPDATE OF state, current_size, updated_time ON csm_lv FOR EACH ROW EXECUTE PROCEDURE fn_csm_lv_update_history_dump()
 Has OIDs: no

csm_lv_history
""""""""""""""

**Description**
 This table contains historical information associated with previously active logical volumes.

=========== =============================================== ==========================
 Table      Overview                                        Action On:
=========== =============================================== ==========================
 Usage      | Medium                                        |
 Size       | 5000+ rows (depending on step usage)          |
 Index      | ix_csm_lv_history_a on (history_time)         |
            | ix_csm_lv_history_b on (logical_volume_name)  |
            | ix_csm_lv_history_c on (ctid)                 |
            | ix_csm_lv_history_d on (archive_history_time) |
=========== =============================================== ==========================  

csm_lv_history (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                                Table "public.csm_lv_history"
         Column        |            Type             | Modifiers | Storage  | Stats target |                                          Description
 ----------------------+-----------------------------+-----------+----------+--------------+------------------------------------------------------------------------------------------------
  history_time         | timestamp without time zone | not null  | plain    |              | this is when the lv enters the history table
  logical_volume_name  | text                        | not null  | extended |              | unique identifier for this ssd partition
  node_name            | text                        | not null  | extended |              | node a part of this group
  allocation_id        | bigint                      |           | plain    |              | unique identifier for this allocation
  vg_name              | text                        |           | extended |              | volume group name
  state                | character(1)                | not null  | extended |              | state: (c)reated, (m)ounted, (s)hrinking, (r)emoved
  current_size         | bigint                      | not null  | plain    |              | current size (in bytes)
  max_size             | bigint                      | not null  | plain    |              | max size (in bytes) at runtime
  begin_time           | timestamp without time zone | not null  | plain    |              | when the partitioning begins
  updated_time         | timestamp without time zone |           | plain    |              | when it was last updated
  end_time             | timestamp without time zone |           | plain    |              | when the partitioning stage ends
  file_system_mount    | text                        |           | extended |              | identifies the file system and mount point
  file_system_type     | text                        |           | extended |              | identifies the file system and its partition
  num_bytes_read       | bigint                      |           | plain    |              | number of bytes read during the life of this partition
  num_bytes_written    | bigint                      |           | plain    |              | number of bytes written during the life of this partition
  operation            | character(1)                |           | extended |              | operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)
  archive_history_time | timestamp without time zone |           | plain    |              | timestamp when the history data has been archived and sent to: BDS, archive file, and or other
  num_reads            | bigint                      |           | plain    |              | number of read during the life of this partition
  num_writes           | bigint                      |           | plain    |              | number of writes during the life of this partition
 Indexes:
     "ix_csm_lv_history_a" btree (history_time)
     "ix_csm_lv_history_b" btree (logical_volume_name)
     "ix_csm_lv_history_c" btree (ctid)
     "ix_csm_lv_history_d" btree (archive_history_time)
 Has OIDs: no

csm_lv_update_history
"""""""""""""""""""""

**Description**
 This table contains historical information associated with lv updates.

=========== ====================================================== ========================
 Table      Overview                                               Action On:
=========== ====================================================== ========================
 Usage      | Medium                                               |
 Size       | 5000+ rows (depending on step usage)                 |
 Index      | ix_csm_lv_update_history_a on (history_time)         |
            | ix_csm_lv_update_history_b on (logical_volume_name)  |
            | ix_csm_lv_update_history_c on (ctid)                 |
            | ix_csm_lv_update_history_d on (archive_history_time) |
=========== ====================================================== ========================

csm_lv_update_history (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                            Table "public.csm_lv_update_history"
         Column        |            Type             | Modifiers | Storage  | Stats target |                                          Description
 ----------------------+-----------------------------+-----------+----------+--------------+------------------------------------------------------------------------------------------------
  history_time         | timestamp without time zone | not null  | plain    |              | this is when the lv update enters the history table
  logical_volume_name  | text                        | not null  | extended |              | unique identifier for this ssd partition
  allocation_id        | bigint                      | not null  | plain    |              | unique identifier for this allocation
  state                | character(1)                | not null  | extended |              | state: (c)reate, (m)ounted, (s)hrinking, (r)emoved
  current_size         | bigint                      | not null  | plain    |              | current size (in bytes)
  updated_time         | timestamp without time zone |           | plain    |              | when it was last updated
  operation            | character(1)                |           | extended |              | operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)
  archive_history_time | timestamp without time zone |           | plain    |              | timestamp when the history data has been archived and sent to: BDS, archive file, and or other
 Indexes:
     "ix_csm_lv_update_history_a" btree (history_time)
     "ix_csm_lv_update_history_b" btree (logical_volume_name)
     "ix_csm_lv_update_history_c" btree (ctid)
     "ix_csm_lv_update_history_d" btree (archive_history_time)
 Has OIDs: no

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

csm_vg_ssd (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                                                                    Table "public.csm_vg_ssd"
      Column     |  Type  | Modifiers | Storage  | Stats target |                                                                                          Description
 ----------------+--------+-----------+----------+--------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  vg_name        | text   | not null  | extended |              | unique identifier for this ssd partition
  node_name      | text   | not null  | extended |              | identifies which node
  serial_number  | text   | not null  | extended |              | serial number for the ssd
  ssd_allocation | bigint | not null  | plain    |              | the amount of space (in bytes) that this ssd contributes to the volume group. Can not be less than zero. The total sum of these fields should equal total_size of the this vg in the vg table
 Indexes:
     "uk_csm_vg_ssd_a" UNIQUE, btree (vg_name, node_name, serial_number)
 Foreign-key constraints:
     "csm_vg_ssd_serial_number_fkey" FOREIGN KEY (serial_number, node_name) REFERENCES csm_ssd(serial_number, node_name)
     "csm_vg_ssd_vg_name_fkey" FOREIGN KEY (vg_name, node_name) REFERENCES csm_vg(vg_name, node_name)
 Triggers:
     tr_csm_vg_ssd_history_dump BEFORE INSERT OR DELETE OR UPDATE ON csm_vg_ssd FOR EACH ROW EXECUTE PROCEDURE fn_csm_vg_ssd_history_dump()
 Has OIDs: no

csm_vg_ssd_history
""""""""""""""""""

**Description**
 This table contains historical information associated with SSD and logical volume tables.

=========== =================================================== ==========================
 Table      Overview                                            Action On:
=========== =================================================== ==========================
 Usage      | Medium                                            |
 Size       | 5000+ rows (depending on step usage)              |
 Index      | ix_csm_vg_ssd_history_a on (history_time)         |
            | ix_csm_vg_ssd_history_b on (vg_name, node_name)   |
            | ix_csm_vg_ssd_history_c on (ctid)                 |
            | ix_csm_vg_ssd_history_d on (archive_history_time) |
=========== =================================================== ==========================

csm_vg_ssd_history (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                                                                             Table "public.csm_vg_ssd_history"
         Column        |            Type             | Modifiers | Storage  | Stats target |                        Description
 ----------------------+-----------------------------+-----------+----------+--------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  history_time         | timestamp without time zone | not null  | plain    |              | time when enters into the history table
  vg_name              | text                        | not null  | extended |              | unique identifier for this ssd partition
  node_name            | text                        | not null  | extended |              | identifies which node
  serial_number        | text                        | not null  | extended |              | serial number for the ssd
  ssd_allocation       | bigint                      | not null  | plain    |              | the amount of space (in bytes) that this ssd contributes to the volume group. Can not be less than zero. The total sum of these fields should equal total_size of the this vg in the vg table
  operation            | character(1)                | not null  | extended |              | operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)
  archive_history_time | timestamp without time zone |           | plain    |              | timestamp when the history data has been archived and sent to: BDS, archive file, and or other
 Indexes:
     "ix_csm_vg_ssd_history_a" btree (history_time)
     "ix_csm_vg_ssd_history_b" btree (vg_name, node_name)
     "ix_csm_vg_ssd_history_c" btree (ctid)
     "ix_csm_vg_ssd_history_d" btree (archive_history_time)
 Has OIDs: no

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

csm_vg (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                    Table "public.csm_vg"
      Column     |            Type             | Modifiers | Storage  | Stats target |                             Description
 ----------------+-----------------------------+-----------+----------+--------------+----------------------------------------------------------------------
  vg_name        | text                        | not null  | extended |              | unique identifier for this ssd partition
  node_name      | text                        | not null  | extended |              | identifies which node
  total_size     | bigint                      | not null  | plain    |              | volume group size. measured in bytes
  available_size | bigint                      | not null  | plain    |              | remaining bytes available out of total size.
  scheduler      | boolean                     | not null  | plain    |              | tells CSM whether or not this is the volume group for the scheduler.
  update_time    | timestamp without time zone |           | plain    |              | timestamp when the vg was updated
 Indexes:
     "csm_vg_pkey" PRIMARY KEY, btree (vg_name, node_name)
 Check constraints:
     "csm_available_size_should_be_less_than_total_size" CHECK (available_size <= total_size)
 Referenced by:
     TABLE "csm_lv" CONSTRAINT "csm_lv_node_name_fkey" FOREIGN KEY (node_name, vg_name) REFERENCES csm_vg(node_name, vg_name)
     TABLE "csm_vg_ssd" CONSTRAINT "csm_vg_ssd_vg_name_fkey" FOREIGN KEY (vg_name, node_name) REFERENCES csm_vg(vg_name, node_name)
 Triggers:
     tr_csm_vg_history_dump BEFORE INSERT OR DELETE OR UPDATE ON csm_vg FOR EACH ROW EXECUTE PROCEDURE fn_csm_vg_history_dump()
 Has OIDs: no

csm_vg_history
""""""""""""""

**Description**
 This table contains historical information associated with SSD and logical volume tables.

=========== =============================================== ==========================
 Table      Overview                                        Action On:
=========== =============================================== ==========================
 Usage      | Medium                                        |
 Size       | 5000+ rows (depending on step usage)          |
 Index      | ix_csm_vg_history_a on (history_time)         |
            | ix_csm_vg_history_b on (vg_name, node_name)   |
            | ix_csm_vg_history_c on (ctid)                 |
            | ix_csm_vg_history_d on (archive_history_time) |
=========== =============================================== ==========================

csm_vg_history (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                                Table "public.csm_vg_history"
         Column        |            Type             | Modifiers | Storage  | Stats target |                                          Description
 ----------------------+-----------------------------+-----------+----------+--------------+------------------------------------------------------------------------------------------------
  history_time         | timestamp without time zone | not null  | plain    |              | time when enters into the history table
  vg_name              | text                        | not null  | extended |              | unique identifier for this ssd partition
  node_name            | text                        | not null  | extended |              | identifies which node
  total_size           | bigint                      | not null  | plain    |              | volume group size. measured in bytes
  available_size       | bigint                      | not null  | plain    |              | remaining bytes available out of total size.
  scheduler            | boolean                     | not null  | plain    |              | tells CSM whether or not this is the volume group for the scheduler.
  update_time          | timestamp without time zone |           | plain    |              | timestamp when the vg was updated
  operation            | character(1)                | not null  | extended |              | operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)
  archive_history_time | timestamp without time zone |           | plain    |              | timestamp when the history data has been archived and sent to: BDS, archive file, and or other
 Indexes:
     "ix_csm_vg_history_a" btree (history_time)
     "ix_csm_vg_history_b" btree (vg_name, node_name)
     "ix_csm_vg_history_c" btree (ctid)
     "ix_csm_vg_history_d" btree (archive_history_time)
 Has OIDs: no

Switch & ib cable tables
^^^^^^^^^^^^^^^^^^^^^^^^

.. _csm_switch_table:

csm_switch
""""""""""

**Description**
 This table contain information about the switch and it attributes. See table below for details.

=========== ============================================= ==========================
 Table      Overview                                      Action On:
=========== ============================================= ==========================
 Usage      | Low                                         |
 Size       | 500 rows (Switches on a CORAL system)       |
 Key(s)     | PK: switch_name                             |
 Index      | csm_switch_pkey on (switch_name)            |
 Functions  | fn_csm_switch_history_dump                  |
 Triggers   | tr_csm_switch_history_dump                  | update/delete
=========== ============================================= ==========================  
 
+-----------------------+--------------------------------------------+------------------+-------+
| *Referenced by table* |           *Constraint*                     | *Fields*         | *Key* |
+=======================+============================================+==================+=======+
| csm_switch_inventory  | csm_switch_inventory_host_system_guid_fkey | host_system_guid | (FK)  |
+-----------------------+--------------------------------------------+------------------+-------+

csm_switch (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                                                                            Table "public.csm_switch"
          Column          |            Type             | Modifiers | Storage  | Stats target |                    Description
 -------------------------+-----------------------------+-----------+----------+--------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  switch_name             | text                        | not null  | extended |              | switch name: Identification of the system For hosts, it is caguid, For 1U switch, it is switchguid, For modular switches, is it sysimgguid
  serial_number           | text                        |           | extended |              | identifies the switch this information is for
  discovery_time          | timestamp without time zone |           | plain    |              | time the switch collected at inventory time
  collection_time         | timestamp without time zone |           | plain    |              | time the switch was initially connected
  comment                 | text                        |           | extended |              | a comment can be generated for this field
  description             | text                        |           | extended |              | description of system – system type of this systems (More options: SHArP, MSX1710 , CS7520)
  fw_version              | text                        |           | extended |              | firmware version of the Switch or HCA
  gu_id                   | text                        |           | extended |              | Node guid of the system. In case of HCA, it is the caguid. In case of Switch, it is the switchguid
  has_ufm_agent           | boolean                     |           | plain    |              | indicate if system (Switch or Host) is running a UFM Agent
  hw_version              | text                        |           | extended |              | hardware version related to the switch
  ip                      | text                        |           | extended |              | ip address of the system (Switch or Host)  (0.0.0.0 in case ip address not available)
  model                   | text                        |           | extended |              | system model – in case of switch, it is the switch model, For hosts – Computer
  num_modules             | integer                     |           | plain    |              | number of modules attached to this switch. This is the number of expected records in the csm_switch inventory table associated with this switch name.
  physical_frame_location | text                        |           | extended |              | where the switch is located
  physical_u_location     | text                        |           | extended |              | physical u location (position in the frame) where the switch is located
  ps_id                   | text                        |           | extended |              | PSID (Parameter-Set IDentification) is a 16-ascii character string embedded in the firmware image which provides a unique identification for the configuration of the firmware.
  role                    | text                        |           | extended |              | Type/Role of system in the current fabric topology: Tor / Core / Endpoint (host). (Optional Values: core, tor, endpoint)
  server_operation_mode   | text                        |           | extended |              | Operation mode of system. (Optional Values: Stand_Alone, HA_Active, HA_StandBy, Not_UFM_Server, Router, Gateway, Switch)
  sm_mode                 | text                        |           | extended |              | Indicate if SM is running on that system. (Optional Values: no SM, activeSM, hasSM)
  state                   | text                        |           | extended |              | runtime state of the system. (Optional Values: active, rebooting, down, error (failed to reboot))
  sw_version              | text                        |           | extended |              | software version of the system – full MLNX_OS version. Relevant only for MLNX-OS systems (Not available for Hosts)
  system_guid             | text                        |           | extended |              | system image guid for that system
  system_name             | text                        |           | extended |              | system name as it appear on the system node description
  total_alarms            | integer                     |           | plain    |              | total number of alarms which are currently exist on the system
  type                    | text                        |           | extended |              | type of system. (Optional Values: switch, host, gateway)
  vendor                  | text                        |           | extended |              | system vendor
 Indexes:
     "csm_switch_pkey" PRIMARY KEY, btree (switch_name)
     "uk_csm_switch_gu_id_a" UNIQUE CONSTRAINT, btree (gu_id)
 Referenced by:
     TABLE "csm_switch_inventory" CONSTRAINT "csm_switch_inventory_host_system_guid_fkey" FOREIGN KEY (host_system_guid) REFERENCES csm_switch(gu_id)
 Triggers:
     tr_csm_switch_history_dump BEFORE INSERT OR DELETE OR UPDATE ON csm_switch FOR EACH ROW EXECUTE PROCEDURE fn_csm_switch_history_dump()
 Has OIDs: no

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
            | ix_csm_switch_history_c on (ctid)                        |
            | ix_csm_switch_history_d on (archive_history_time)        |
=========== ========================================================== ==========================

csm_switch_history (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                                                                        Table "public.csm_switch_history"
          Column          |            Type             | Modifiers | Storage  | Stats target |                    Description
 -------------------------+-----------------------------+-----------+----------+--------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  history_time            | timestamp without time zone | not null  | plain    |              | the time when the switch enters the history table
  switch_name             | text                        | not null  | extended |              | switch name: Identification of the system For hosts, it is caguid, For 1U switch, it is switchguid, For modular switches, is it sysimgguid
  serial_number           | text                        |           | extended |              | identifies the switch this information is for
  discovery_time          | timestamp without time zone |           | plain    |              | time the switch collected at inventory time
  collection_time         | timestamp without time zone |           | plain    |              | time the switch was initially connected
  comment                 | text                        |           | extended |              | a comment can be generated for this field
  description             | text                        |           | extended |              | Description of system – system type of this systems (More options: SHArP, MSX1710 , CS7520)
  fw_version              | text                        |           | extended |              | firmware version of the Switch or HCA
  gu_id                   | text                        |           | extended |              | Node guid of the system. In case of HCA, it is the caguid. In case of Switch, it is the switchguid
  has_ufm_agent           | boolean                     |           | plain    |              | Indicate if system (Switch or Host) is running a UFM Agent
  hw_version              | text                        |           | extended |              | hardware version related to the switch
  ip                      | text                        |           | extended |              | ip address of the system (Switch or Host)  (0.0.0.0 in case ip address not available)
  model                   | text                        |           | extended |              | System model – in case of switch, it is the switch model, For hosts – Computer
  num_modules             | integer                     |           | plain    |              | number of modules attached to this switch. This is the number of expected records in the csm_switch inventory table associated with this switch name.
  physical_frame_location | text                        |           | extended |              | where the switch is located
  physical_u_location     | text                        |           | extended |              | physical u location (position in the frame) where the switch is located
  ps_id                   | text                        |           | extended |              | PSID (Parameter-Set IDentification) is a 16-ascii character string embedded in the firmware image which provides a unique identification for the configuration of the firmware.
  role                    | text                        |           | extended |              | Type/Role of system in the current fabric topology: Tor / Core / Endpoint (host). (Optional Values: core, tor, endpoint)
  server_operation_mode   | text                        |           | extended |              | Operation mode of system. (Optional Values: Stand_Alone, HA_Active, HA_StandBy, Not_UFM_Server, Router, Gateway, Switch)
  sm_mode                 | text                        |           | extended |              | Indicate if SM is running on that system. (Optional Values: no SM, activeSM, hasSM)
  state                   | text                        |           | extended |              | runtime state of the system. (Optional Values: active, rebooting, down, error (failed to reboot))
  sw_version              | text                        |           | extended |              | software version of the system – full MLNX_OS version. Relevant only for MLNX-OS systems (Not available for Hosts)
  system_guid             | text                        |           | extended |              | system image guid for that system
  system_name             | text                        |           | extended |              | system name as it appear on the system node description
  total_alarms            | integer                     |           | plain    |              | total number of alarms which are currently exist on the system
  type                    | text                        |           | extended |              | type of system. (Optional Values: switch, host, gateway)
  vendor                  | text                        |           | extended |              | system_vendor
  operation               | character(1)                | not null  | extended |              | operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)
  archive_history_time    | timestamp without time zone |           | plain    |              | timestamp when the history data has been archived and sent to:
  BDS, archive file, and or other
 Indexes:
     "ix_csm_switch_history_a" btree (history_time)
     "ix_csm_switch_history_b" btree (switch_name, history_time)
     "ix_csm_switch_history_c" btree (ctid)
     "ix_csm_switch_history_d" btree (archive_history_time)
 Has OIDs: no

.. _csm_ib_cable_table:

csm_ib_cable
""""""""""""

**Description**
 This table contains information about the InfiniBand cables. See table below for details.

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

csm_ib_cable (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                                    Table "public.csm_ib_cable"
      Column      |            Type             | Modifiers | Storage  | Stats target |                                                Description
 -----------------+-----------------------------+-----------+----------+--------------+------------------------------------------------------------------------------------------------------------
  serial_number   | text                        | not null  | extended |              | identifies the cables serial number
  discovery_time  | timestamp without time zone |           | plain    |              | First time the ib cable was found in the system
  collection_time | timestamp without time zone |           | plain    |              | Last time the ib cable inventory was collected
  comment         | text                        |           | extended |              | comment can be generated for this field
  guid_s1         | text                        |           | extended |              | guid: side 1 of the cable
  guid_s2         | text                        |           | extended |              | guid: side 2 of the cable
  identifier      | text                        |           | extended |              | cable identifier (example value: QSFP+)
  length          | text                        |           | extended |              | the length of the cable
  name            | text                        |           | extended |              | name (Id) of link object in UFM. Based on link sorce and destination.
  part_number     | text                        |           | extended |              | part number of this particular ib cable
  port_s1         | text                        |           | extended |              | port: side 1 of the cable
  port_s2         | text                        |           | extended |              | port: side 2 of the cable
  revision        | text                        |           | extended |              | hardware revision associated with this ib cable
  severity        | text                        |           | extended |              | severity associated with this ib cable (severity of link according to highest severity of related events)
  type            | text                        |           | extended |              | field from UFM (technology ) - the specific type of cable used (example used : copper cable - unequalized)
  width           | text                        |           | extended |              | the width of the cable - physical state of IB port (Optional Values: IB_1x ,IB_4x, IB_8x, IB_12x)
 Indexes:
     "csm_ib_cable_pkey" PRIMARY KEY, btree (serial_number)
 Triggers:
     tr_csm_ib_cable_history_dump BEFORE INSERT OR DELETE OR UPDATE ON csm_ib_cable FOR EACH ROW EXECUTE PROCEDURE fn_csm_ib_cable_history_dump()
 Has OIDs: no

csm_ib_cable_history
""""""""""""""""""""

**Description**
 This table contains historical information about the InfiniBand cables.

=========== ===================================================== ==========================
 Table      Overview                                              Action On:
=========== ===================================================== ==========================
 Usage      | Low                                                 |
 Size       | 25,000+ rows (Based on switch topology and          |
            | or configuration)                                   |
 Index      | ix_csm_ib_cable_history_a on (history_time)         |
            | ix_csm_ib_cable_history_b on (serial_number)        |
            | ix_csm_ib_cable_history_c on (ctid)                 |
            | ix_csm_ib_cable_history_d on (archive_history_time) |
=========== ===================================================== ==========================

csm_ib_cable_history (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                                   Table "public.csm_ib_cable_history"
         Column        |            Type             | Modifiers | Storage  | Stats target |                                                Description
 ----------------------+-----------------------------+-----------+----------+--------------+------------------------------------------------------------------------------------------------------------
  history_time         | timestamp without time zone | not null  | plain    |              | the time when the cable enters the history table
  serial_number        | text                        | not null  | extended |              | identifies the cables serial number
  discovery_time       | timestamp without time zone |           | plain    |              | first time the ib cable was found in the system
  collection_time      | timestamp without time zone |           | plain    |              | comment can be generated for this field
  comment              | text                        |           | extended |              | comment can be generated for this field
  guid_s1              | text                        |           | extended |              | guid: side 1 of the cable
  guid_s2              | text                        |           | extended |              | guid: side 2 of the cable
  identifier           | text                        |           | extended |              | cable identifier (example value: QSFP+)
  length               | text                        |           | extended |              | the length of the cable
  name                 | text                        |           | extended |              | name (Id) of link object in UFM. Based on link sorce and destination.
  part_number          | text                        |           | extended |              | part number of this particular ib cable
  port_s1              | text                        |           | extended |              | port: side 1 of the cable
  port_s2              | text                        |           | extended |              | port: side 2 of the cable
  revision             | text                        |           | extended |              | hardware revision associated with this ib cable
  severity             | text                        |           | extended |              | severity associated with this ib cable (severity of link according to highest severity of related events)
  type                 | text                        |           | extended |              | field from UFM (technology ) - the specific type of cable used (example used : copper cable - unequalized)
  width                | text                        |           | extended |              | the width of the cable - physical state of IB port (Optional Values: IB_1x ,IB_4x, IB_8x, IB_12x)
  operation            | character(1)                | not null  | extended |              | operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)
  archive_history_time | timestamp without time zone |           | plain    |              | timestamp when the history data has been archived and sent to: BDS, archive file, and or other
 Indexes:
     "ix_csm_ib_cable_history_a" btree (history_time)
     "ix_csm_ib_cable_history_b" btree (serial_number)
     "ix_csm_ib_cable_history_c" btree (ctid)
     "ix_csm_ib_cable_history_d" btree (archive_history_time)
 Has OIDs: no

.. _csm_switch_inventory_table:

csm_switch_inventory
""""""""""""""""""""

**Description**
 This table contains information about the switch inventory. See table below for details.

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

csm_switch_inventory (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                                                                    Table "public.csm_switch_inventory"
       Column      |            Type             | Modifiers | Storage  | Stats target |              Description
 ------------------+-----------------------------+-----------+----------+--------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  name             | text                        | not null  | extended |              | name (identifier) of this module in UFM.
  host_system_guid | text                        | not null  | extended |              | the system image guid of the hosting system.
  discovery_time   | timestamp without time zone |           | plain    |              | first time the module was found in the system
  collection_time  | timestamp without time zone |           | plain    |              | last time the module inventory was collected
  comment          | text                        |           | extended |              | system administrator comment about this module
  description      | text                        |           | extended |              | description type of module - can be the module type: system, FAN, MGMT,PS or the type of module in case of line / spine modules: SIB7510(Barracud line), SIB7520(Barracuda spine)
  device_name      | text                        |           | extended |              | name of device containing this module.
  device_type      | text                        |           | extended |              | type of device module belongs to.
  hw_version       | text                        |           | extended |              | hardware version related to the switch
  max_ib_ports     | integer                     |           | plain    |              | maximum number of external ports of this module.
  module_index     | integer                     |           | plain    |              | index of module. Each module type has separate index: FAN1,FAN2,FAN3…PS1,PS2
  number_of_chips  | integer                     |           | plain    |              | number of chips which are contained in this module. (relevant only for line / spine modules, for all other modules number_of_chips=0)
  path             | text                        |           | extended |              | full path of module object. Path format: site-name (number of devices) / device type: device-name / module description module index.
  serial_number    | text                        |           | extended |              | serial_number of the module.
  severity         | text                        |           | extended |              | severity of the module according to the highest severity of related events. values: Info, Warning, Minor, Critical
  status           | text                        |           | extended |              | current module status. valid values: ok, fault
  type             | text                        |           | extented |              | The category of this piece of hardware inventory. For example: "FAN", "PS", "SYSTEM", or "MGMT".
  fw_version       | text                        |           | extented |              | The firmware version on this piece of inventory.
 Indexes:
     "csm_switch_inventory_pkey" PRIMARY KEY, btree (name)
 Foreign-key constraints:
     "csm_switch_inventory_host_system_guid_fkey" FOREIGN KEY (host_system_guid) REFERENCES csm_switch(gu_id)
 Triggers:
     tr_csm_switch_inventory_history_dump BEFORE INSERT OR DELETE OR UPDATE ON csm_switch_inventory FOR EACH ROW EXECUTE PROCEDURE fn_csm_switch_inventory_history_dump()
 Has OIDs: no

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
            | ix_csm_switch_inventory_history_b on (name)                 |
            | ix_csm_switch_inventory_history_c on (ctid)                 |
            | ix_csm_switch_inventory_history_d on (archive_history_time) |
=========== ============================================================= ==========================  

csm_switch_inventory_history (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                                                                  Table "public.csm_switch_inventory_history"
         Column        |            Type             | Modifiers | Storage  | Stats target |                  Description
 ----------------------+-----------------------------+-----------+----------+--------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  history_time         | timestamp without time zone | not null  | plain    |              | the time when the inventory record enters the history table
  name                 | text                        | not null  | extended |              | name (identifier) of this module in UFM.
  host_system_guid     | text                        | not null  | extended |              | the system image guid of the hosting system.
  discovery_time       | timestamp without time zone |           | plain    |              | first time the module was found in the system
  collection_time      | timestamp without time zone |           | plain    |              | last time the module inventory was collected
  comment              | text                        |           | extended |              | system administrator comment about this module
  description          | text                        |           | extended |              | description type of module - can be the module type: system, FAN, MGMT,PS or the type of module in case of line / spine modules: SIB7510(Barracud line), SIB7520(Barracuda spine)
  device_name          | text                        |           | extended |              | name of device containing this module.
  device_type          | text                        |           | extended |              | type of device module belongs to.
  hw_version           | text                        |           | extended |              | hardware version related to the switch
  max_ib_ports         | integer                     |           | plain    |              | maximum number of external ports of this module.
  module_index         | integer                     |           | plain    |              | index of module. Each module type has separate index: FAN1,FAN2,FAN3…PS1,PS2
  number_of_chips      | integer                     |           | plain    |              | number of chips which are contained in this module. (relevant only for line / spine modules, for all other modules number_of_chips=0)
  path                 | text                        |           | extended |              | full path of module object. Path format: site-name (number of devices) / device type: device-name / module description module index.
  serial_number        | text                        |           | extended |              | serial_number of the module.
  severity             | text                        |           | extended |              | severity of the module according to the highest severity of related events. values: Info, Warning, Minor, Critical
  status               | text                        |           | extended |              | current module status. valid values: ok, fault
  operation            | character(1)                | not null  | extended |              | operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE)
  archive_history_time | timestamp without time zone |           | plain    |              | timestamp when the history data has been archived and sent to: BDS, archive file, and or other
  type                 | text                        |           | extented |              | The category of this piece of hardware inventory. For example: "FAN", "PS", "SYSTEM", or "MGMT".
  fw_version           | text                        |           | extented |              | The firmware version on this piece of inventory.
 Indexes:
     "ix_csm_switch_inventory_history_a" btree (history_time)
     "ix_csm_switch_inventory_history_b" btree (name)
     "ix_csm_switch_inventory_history_c" btree (ctid)
     "ix_csm_switch_inventory_history_d" btree (archive_history_time)
 Has OIDs: no

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

csm_config (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                                                   Table "public.csm_config"
          Column         |            Type             |                             Modifiers                              | Storage  | Stats target |                       Description
 ------------------------+-----------------------------+--------------------------------------------------------------------+----------+--------------+-----------------------------------------------------------------------
  csm_config_id          | bigint                      | not null default nextval('csm_config_csm_config_id_seq'::regclass) | plain    |              | the configuration identification
  local_socket           | text                        |                                                                    | extended |              | socket to use to local csm daemon
  mqtt_broker            | text                        |                                                                    | extended |              | ip: port
  log_level              | text[]                      |                                                                    | extended |              | db#, daemon.compute, daemon.aggragator, daemon.master, daemon.utility
  buckets                | text[]                      |                                                                    | extended |              | list of items to execute in buckets
  jitter_window_interval | integer                     |                                                                    | plain    |              | jitter interval for compute agent (how often to wake up)
  jitter_window_duration | integer                     |                                                                    | plain    |              | jitter duration for compute agent (duration of the window)
  path_certificate       | text                        |                                                                    | extended |              | location of certificates for authentication
  path_log               | text                        |                                                                    | extended |              | path where the daemon will log
  create_time            | timestamp without time zone |                                                                    | plain    |              | when these logs were created
 Indexes:
     "csm_config_pkey" PRIMARY KEY, btree (csm_config_id)
 Triggers:
     tr_csm_config_history_dump BEFORE DELETE OR UPDATE ON csm_config FOR EACH ROW EXECUTE PROCEDURE fn_csm_config_history_dump()
 Has OIDs: no

csm_config_history
""""""""""""""""""

**Description**
 This table contains historical information about the CSM configuration.

=========== =================================================== ==========================
 Table      Overview                                            Action On:
=========== =================================================== ==========================
 Usage      | Medium                                            |
 Size       | 1-100 rows                                        |
 Index      | ix_csm_config_history_a on (history_time)         |
            | ix_csm_config_history_b on (csm_config_id)        |
            | ix_csm_config_history_c on (ctid)                 |
            | ix_csm_config_history_d on (archive_history_time) |
=========== =================================================== ==========================   

csm_config_history (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                               Table "public.csm_config_history"
          Column         |            Type             | Modifiers | Storage  | Stats target |                                          Description
 ------------------------+-----------------------------+-----------+----------+--------------+------------------------------------------------------------------------------------------------
  history_time           | timestamp without time zone | not null  | plain    |              | the time when the configuration enters the history table
  csm_config_id          | bigint                      |           | plain    |              | the configuration identification
  local_socket           | text                        |           | extended |              | socket to use to local csm daemon
  mqtt_broker            | text                        |           | extended |              | ip: port
  log_level              | text[]                      |           | extended |              | db#, daemon.compute, daemon.aggragator, daemon.master, daemon.utility
  buckets                | text[]                      |           | extended |              | list of items to execute in buckets
  jitter_window_interval | integer                     |           | plain    |              | jitter interval for compute agent (how often to wake up)
  jitter_window_duration | integer                     |           | plain    |              | jitter duration for compute agent (duration of the window)
  path_certificate       | text                        |           | extended |              | location of certificates for authentication
  path_log               | text                        |           | extended |              | path where the daemon will log
  create_time            | timestamp without time zone |           | plain    |              | when these logs were created
  archive_history_time   | timestamp without time zone |           | plain    |              | timestamp when the history data has been archived and sent to: BDS, archive file, and or other
 Indexes:
     "ix_csm_config_history_a" btree (history_time)
     "ix_csm_config_history_b" btree (csm_config_id)
     "ix_csm_config_history_c" btree (ctid)
     "ix_csm_config_history_d" btree (archive_history_time)
 Has OIDs: no

csm_config_bucket
"""""""""""""""""

**Description**
 This table is the list of items that will placed in the bucket. See table below for details.

=========== ============================================= ==========================
 Table      Overview                                      Action On:
=========== ============================================= ==========================
 Usage      | Medium                                      |
 Size       | 1-400 rows (Based on configuration changes) |
 Index      | ix_csm_config_bucket_a on                   |
            | (bucket_id, item_list, time_stamp)          |
=========== ============================================= ==========================  

csm_config_bucket (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                  Table "public.csm_config_bucket"
        Column       |            Type             | Modifiers | Storage  | Stats target |               Description
 --------------------+-----------------------------+-----------+----------+--------------+------------------------------------------
  bucket_id          | integer                     |           | plain    |              | this is the identification of the bucket
  item_list          | bigint                      |           | plain    |              | the item list within in the bucket
  execution_interval | text                        |           | extended |              | execution interval (the counter)
  time_stamp         | timestamp without time zone |           | plain    |              | time when the process takes place
 Indexes:
     "ix_csm_config_bucket_a" btree (bucket_id, item_list, time_stamp)
 Has OIDs: no

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

csm_db_schema_version (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                Table "public.csm_db_schema_version"
    Column    |            Type             |   Modifiers   | Storage  | Stats target |                 Description
 -------------+-----------------------------+---------------+----------+--------------+---------------------------------------------
  version     | text                        | not null      | extended |              | this is the current database schema version
  create_time | timestamp without time zone | default now() | plain    |              | time when the db was created
  comment     | text                        |               | extended |              | comment
 Indexes:
     "csm_db_schema_version_pkey" PRIMARY KEY, btree (version)
     "ix_csm_db_schema_version_a" btree (version, create_time)
 Triggers:
     tr_csm_db_schema_version_history_dump BEFORE DELETE OR UPDATE ON csm_db_schema_version FOR EACH ROW EXECUTE PROCEDURE fn_csm_db_schema_version_history_dump()
 Has OIDs: no

csm_db_schema_version_history
"""""""""""""""""""""""""""""

**Description**
 This is the historical database schema version (if changes have been made)

=========== ============================================================== ==========================
 Table      Overview                                                       Action On:
=========== ============================================================== ==========================
 Usage      | Low                                                          |
 Size       | 1-100 rows (Based on CSM DB changes/updates)                 |
 Index      | ix_csm_db_schema_version_history_a on (history_time)         |
            | ix_csm_db_schema_version_history_b on (version)              |
            | ix_csm_db_schema_version_history_c on (ctid)                 |
            | ix_csm_db_schema_version_history_d on (archive_history_time) |
=========== ============================================================== ==========================

csm_db_schema_version_history (DB table overview)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

                                                                        Table "public.csm_db_schema_version_history"
         Column        |            Type             | Modifiers | Storage  | Stats target |                                          Description
 ----------------------+-----------------------------+-----------+----------+--------------+------------------------------------------------------------------------------------------------
  history_time         | timestamp without time zone | not null  | plain    |              | the time when the schema version enters the history table
  version              | text                        |           | extended |              | this is the current database schema version
  create_time          | timestamp without time zone |           | plain    |              | time when the db was created
  comment              | text                        |           | extended |              | comment
  archive_history_time | timestamp without time zone |           | plain    |              | timestamp when the history data has been archived and sent to: BDS, archive file, and or other
 Indexes:
     "ix_csm_db_schema_version_history_a" btree (history_time)
     "ix_csm_db_schema_version_history_b" btree (version)
     "ix_csm_db_schema_version_history_c" btree (ctid)
     "ix_csm_db_schema_version_history_d" btree (archive_history_time)
 Has OIDs: no

PK, FK, UK keys and Index Charts
--------------------------------

Primary Keys (default Indexes)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. csv-table::
   :file: csm_db_appendix_table_csv_files/csm_db_pks_06_03_2019_11_47_00.csv
   :header-rows: 1
   :class: longtable
   :widths: 1 1 1 1

Foreign Keys
^^^^^^^^^^^^
.. csv-table::
   :file: csm_db_appendix_table_csv_files/csm_db_fks_06_03_2019_11_47_00.csv
   :header-rows: 1
   :class: longtable
   :widths: 1 1 1 1 1

Indexes
^^^^^^^
.. csv-table::
   :file: csm_db_appendix_table_csv_files/csm_db_indexes_06_03_2019_11_47_00.csv
   :header-rows: 1
   :class: longtable
   :widths: 1 1 1 1 

Unique UKs
^^^^^^^^^^^^^^
.. csv-table::
   :file: csm_db_appendix_table_csv_files/csm_db_uks_06_03_2019_11_47_00.csv
   :header-rows: 1
   :class: longtable
   :widths: 1 1 1 1 

Functions and Triggers
^^^^^^^^^^^^^^^^^^^^^^
.. csv-table::
   :file: csm_db_appendix_table_csv_files/csm_triggers_functions_06_03_2019_11_47_00.csv
   :header-rows: 1
   :class: longtable
   :widths: 1 1 1 1 1 1 1 1 

CSM DB Schema (pdf)
-------------------
(CSM DB schema version 19.0):

.. image:: https://user-images.githubusercontent.com/6536949/83457321-3f431300-a42f-11ea-9e5e-e6e893f5112f.jpeg
              :width: 600px
              :height: 500px
              :scale: 100%
              :alt: Screenshot of Image Window in OpenCV
              :align: left
