.. _CSM_USER_GUIDE_Job_Launch:

Job Launch
==========

Any jobs submitted to LSF queues will use the integrated support between LSF and :ref:`CSM`. Depending on the user specifications this integration may also include JSM and :ref:`Burst_Buffer`. 

There are few steps required before a job can be launched:

#. The CSM :ref:`CSMDInfrastructure` needs to be operational. CSM master, aggregator, utility, and compute daemons need to be up and operational.
#. The CSM compute daemons must collect inventory on the compute nodes and update the CSM :ref:`CSM_Database`.
#. The system administrator must change the :ref:`state<CSM_USER_GUIDE_CSM_Daemon_States>` of the compute node to ``IN_SERVICE``, using the command line interface of the CSM API *csm_node_attributes_update.*

.. code-block:: bash

	$ /opt/ibm/csm/bin/csm_node_attributes_update –s IN_SERVICE -n c650f02p09