Network Inventory
==================

Overview
--------

The network inventoy (such as: switches, switch modules, and cables) can be collected and stored into the :ref:`CSM_Database`.

CSM supports inventory collection for the following network hardware:

* Mellanox

.. note:: If your network hardware is not listed here, then please create an issue on the CAST |git-repo| page. 

One type of network hardware is Mellanox. CSM worked closely with Mellanox to streamline Mellanox integration. CSM created a tool to help collect Mellanox based inventory. You can find documentation on that tool here: :ref:`CSM_standalone_inventory_collection`.

Collectable Inventory 
---------------------

Here we describe what CSM is able to detect and collect inventory on in regards to the network.

Switches
^^^^^^^^

CSM can collect information about a physical switch and store it the the :ref:`CSM_Database` inside the :ref:`csm_switch_table` table.

Switch Modules
^^^^^^^^^^^^^^

CSM can collect information about hardware components of a switch and store them in the :ref:`CSM_Database` inside the :ref:`csm_switch_inventory_table` table.

Examples of hardware components found on a switch are:

* Fans
* Power Supplies

Cables
^^^^^^

CSM can collect information about the physical cables in your system and store them in the :ref:`CSM_Database` inside the :ref:`csm_ib_cable_table` table.

.. note:: At this time, only Mellanox IB cables are fully supported by CSM inventory collection. 

HCA
^^^

HCA hardware inventory is collected, but is collected with :ref:`Node_Inventory_Collection`.