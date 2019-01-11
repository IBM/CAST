Node Inventory
==============

Overview
--------

CSM can collect and store hardware information about a node and store it in the :ref:`CSM_Database`. 

Below is a table outlining what information is collected and where in the :ref:`CSM_Database` it is stored:

+--------------------+------------------------------+
| *INFORMATION*      | *Database Table*             |
+--------------------+------------------------------+
| Core Node          | csm_node                     |
+--------------------+------------------------------+
| Dimm               | csm_dimm                     |
+--------------------+------------------------------+
| GPU                | csm_gpu                      |
+--------------------+------------------------------+
| HCA                | csm_hca                      |
+--------------------+------------------------------+
| Processor          | csm_processor_socket         |
+--------------------+------------------------------+
| SSD                | csm_ssd                      |
+--------------------+------------------------------+

Collection
----------

Core Node inventory collection begins when a `CSM Daemon`_ is booted on a node. 

.. _CSM Daemon: https://cast.readthedocs.io/en/latest/csmd/csm_daemon.html

