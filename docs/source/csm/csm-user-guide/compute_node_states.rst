.. _CSM_USER_GUIDE_Compute_node_states:

Compute node states
===================

.. list-table:: CSM Daemon States
   :widths: 25 25 50
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
     - Reserved for system administrator activity. Processing RAS events.

This is a node.