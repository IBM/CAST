.. _CSM:

Cluster System Management (CSM) 
===============================

.. image:: https://user-images.githubusercontent.com/4662139/49670811-e958ff00-fa33-11e8-92c2-3bf00e8d1001.png
    :height: 200px

CSM is a cognitive self learning system for managing and overseeing a HPC cluster. CSM interacts with a variety of open source IBM tools for supporting and maintaining a cluster, such as:

 * Discovery and management of system resources
 * Database integration (PostgreSQL)
 * Job launch support (workload management, cluster, and allocation APIs)
 * Node diagnostics (diag APIs and scripts)
 * RAS events and actions
 * Infrastructure Health checks
 * Python Bindings for C APIs

**Table of Contents**

 .. toctree::
    :maxdepth: 2

    csm-user-guide/index.rst
    csm-apis/index.rst
    csmdb/index.rst
    csmd/index.rst
    csm-inventory/index.rst
    csm-integration/index.rst
    tools.rst