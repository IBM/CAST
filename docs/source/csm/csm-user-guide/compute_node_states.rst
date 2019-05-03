.. _CSM_USER_GUIDE_Compute_node_states:

Compute node states
===================

.. _CSM_USER_GUIDE_CSM_Daemon_States:

.. list-table:: CSM Daemon States
   :widths: 20 10 50
   :header-rows: 1

   * - State
     - Ready
     - Comments
   * - DISCOVERED
     - No
     - First time CSM sees the node.
   * - IN_SERVICE
     - Yes
     - Node is healthy for scheduler to use. This is the only state scheduler consider the node for scheduling.
   * - ADMIN_RESERVE
     - No
     - Reserved for system administrator activities. Processes RAS events.
   * - MAINTENANCE
     - No
     - Reserved for system administrator activities. Does **NOT** process RAS events.
   * - SOFT_FAILURE 
     - No 
     - CSM reserved.
   * - HARD_FAILURE
     - No
     - CSM reserved.
   * - OUT_OF_SERVICE
     - No 
     - Hardware / Software problem. Does **NOT** process RAS events.

.. _CSM_USER_GUIDE_CSM_Daemon_State_Transitions:

.. list-table:: CSM Daemon State Transitions
   :widths: 10 20 20 50
   :header-rows: 1

   * - Number
     - Start State
     - End State
     - Comments
   * - 1 
     - 
     - DISCOVERED
     - By CSM inventory
   * - 2 
     - DISCOVERED
     - IN_SERVICE
     - By system admin action
   * - 3 
     - IN_SERVICE
     - ADMIN_RESERVE
     - By system admin action
   * - 4
     - ADMIN_RESERVE
     - IN_SERVICE
     - By system admin action
   * - 5
     - IN_SERVICE
     - MAINTENANCE
     - By system admin action
   * - 6
     - MAINTENANCE
     - IN_SERVICE
     - By system admin action
   * - 7
     - IN_SERVICE
     - OUT_OF_SERVICE
     - By system admin action
   * - 8
     - OUT_OF_SERVICE
     - IN_SERVICE
     - By system admin action
   * - 9
     - HARD_FAILURE
     - OUT_OF_SERVICE
     - By system admin action
   * - 10
     - HARD_FAILURE
     - IN_SERVICE
     - By system admin action
   * - 11
     - IN_SERVICE
     - SOFT_FAILURE
     - By CSM RAS subsystem
   * - 12
     - DISCOVERED
     - HARD_FAILURE
     - By CSM RAS subsystem
   * - 13
     - IN_SERVICE
     - HARD_FAILURE
     - By CSM RAS subsystem
   * - 14
     - DISCOVERED
     - SOFT_FAILURE
     - By CSM RAS subsystem
   * - 15
     - SOFT_FAILURE
     - IN_SERVICE
     - CSM soft recovery

Below is a visual graph of daemon state transitions. The numbers in this graph correspond to the number in column 1 of the :ref:`CSM_USER_GUIDE_CSM_Daemon_State_Transitions` table shown above.

.. image:: https://user-images.githubusercontent.com/4662139/57146316-0bf90500-6d93-11e9-8a72-a227bacfab51.png
   :alt: Visual Map of Daemon State Transitions



