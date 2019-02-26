Mellanox and Infiniband
=======================

CSM can be integrated with Mellanox to enhance usability of your cluster. CSM can collect inventory on Mellanox hardware and convert Mellanox events into corresponding CSM RAS events. 

.. Replace below with above when we add diags.

.. CSM can be integrated with Mellanox to enhance usability of your cluster. CSM can collect inventory on Mellanox hardware, listen to Mellanox events and create relative CSM RAS events, and run diagnostics and health check tests with regards to Mellanox systems. 

.. _UFM_Credentials:

Credentials
-----------

For CSM to communicate with Mellanox systems and access the Mellanox UFM restful APIs, a user must have proper credentials. CSM will attempt to connect to UFM via an SSL key. The location of your SSL key can be configured via the ``csm_master.cfg`` file. Using the ``ufm_ssl_file_path`` and ``ufm_ssl_file_name`` fields. Default values have been reproduced below for reference.

.. code-block:: json

	{
	    "ufm" :
	    {
	        "rest_address"  : "__UFM_REST_ADDRESS__",
	        "rest_port"     : 80,
	        "ufm_ssl_file_path" : "/etc/ibm/csm",
	        "ufm_ssl_file_name" : "csm_ufm_ssl_key.txt"
	    }
	}

An SSL key must be generated and placed in this file or CSM will not be able to communicate with UFM restful APIs. 

SSL key generation can be done via the ``openssl`` command found in UNIX. Creating a key for the default username of ``admin`` and the default password of ``123456`` is shown below: 

.. code-block:: bash

	openssl base64 -e <<< admin:123456

It should generate a key for you: ``YWRtaW46MTIzNDU2Cg==`` When you generate your key, please use your username and password. It will generate a different key.

To simplify steps further, you can also directly pipe the output into your key file. 

Example:

.. code-block:: bash

	openssl base64 -e <<< admin:123456 > /etc/ibm/csm/csm_ufm_ssl_key.txt

Network Inventory Collection
----------------------------

The :ref:`CSM_Network_Inventory` (such as: switches, switch modules, and cables) can be collected and stored into the :ref:`CSM_Database`.

Inventory collection has been modularly developed. We separated the external inventory data collection from the internal CSM Database insertion. Once data has been collected, you can then insert that collected data into the CSM Database by using a CSM API. We do this for ease of future updates, should an external component change the way it collects and presents its data. 

Because of this development choice, CSM can easily be adapted to work with multiple external programs and services. 

For ease of use, CSM provides a tool: :ref:`CSM_standalone_inventory_collection` which will collect inventory information from Mellanox and insert that data into the CSM Database. 

RAS Events
----------

CSM monitors Mellanox events and creates corresponding CSM RAS events when a Mellanox event triggers. This CSM RAS event is recorded into the :ref:`CSM_Database`.

.. Diagnostics
  -----------


