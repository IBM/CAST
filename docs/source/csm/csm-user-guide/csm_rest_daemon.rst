.. _CSM_USER_GUIDE_REST_daemon:

CSM REST Daemon
===============

Overview
--------

The CSM REST daemon (csmrestd) is optional and is not required for normal cluster operation. Csmrestd is used to enable RAS events to be created from servers that do not run CSM infrastructure daemons. The CSM REST daemon can be installed on the service nodes to allow BMC RAS events to be reported by the IBM POWER LC Cluster RAS Service (ibm-crassd).


Packaging and Installation
--------------------------

All required binaries, configuration files, and example scripts are packaged in ``ibm-csm-restd-1.5.0-*.ppc64le.rpm``.
To install and configure csmrestd, please refer to :ref:`CSM_INSTALLATION_AND_CONFIGURATION`.

Creating a CSM RAS event via the REST API
-----------------------------------------

Once csmrestd is installed and configured on the management node, an example RAS event can be created from any server that can reach the csmrestd server ip address. This test can be run locally on the management node or from any other node.

By default the CSM RAS msg type settings for the event created by the create_node_leave_event.sh example script will not impact the cluster. However, this event may be fatal in future releases. In future releases, this test may cause a node to be removed from the list of nodes ready to run jobs.

On a service node:

Copy the sample script from ``/opt/ibm/csm/share/rest_scripts/spectrum_scale`` to some other location for editing.

.. code-block:: bash

  $ cp /opt/ibm/csm/share/rest_scripts/spectrum_scale/create_node_leave_event.sh ~/


Edit the copy of create_node_leave_event.sh and replace ``__CSMRESTD_IP__`` with the IP address that was configured for csmrestd to listen on in /etc/ibm/csm/csmrestd.cfg. Optionally, the LOCATION_NAME can also be modified to refer to a real node_name in the csm_node table. 
Start the local CSM daemon, then csmrestd if either of them are not currently running.

.. code-block:: bash

  $ systemctl start csmd-aggregator
  $ systemctl start csmrestd


Run the example script and observe a new event get created in /var/log/ibm/csm/csm_ras_events.log:

.. code-block:: bash

  $ ~/create_node_leave_event.sh
  $ cat /var/log/ibm/csm/csm_ras_events.log | grep spectrumscale.node.nodeLeave

Example output:

.. code-block:: bash

  {"time_stamp":"2017-04-25 09:48:37.829407","msg_id":"spectrumscale.node.nodeLeave",
   "location_name":"c931f04p08vm03","raw_data":"","ctxid":"9","min_time_in_pool":"1",
   "suppress_ids":"","severity":"WARNING","message":"c931f04p08-vm03 has left the cluster.","decoder":"none","control_action":"NONE","description":"The specified node has left the cluster.","relevant_diags":"NONE","threshold_count":"1","threshold_period":"0"}


Stop csmrestd:

.. code-block:: bash

  $ systemctl stop csmrestd



