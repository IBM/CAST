.. _CSM_USER_GUIDE_Compute_node_states:

Compute node states
===================

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

This is a node.