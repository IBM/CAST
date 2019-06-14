.. _CAST:

Cluster Administration Storage Tools (CAST) 
===========================================

CAST is comprised of several open source components:

**Cluster System Management (CSM)**

:ref:`CSM` is a cognitive self learning system for managing and overseeing a HPC cluster. CSM interacts with a variety of open source IBM tools for supporting and maintaining a cluster, such as:

 * Discovery and management of system resources
 * Database integration (PostgreSQL)
 * Job launch support (workload management, cluster, and allocation APIs)
 * Node diagnostics (diag APIs and scripts)
 * RAS events and actions
 * Infrastructure Health checks
 * Python Bindings for C APIs


**Burst Buffer**

The Burst Buffer is an I/O data caching technology which can improve I/O performance for a large class of high-performance 
computing applications without requirement of intermediary hardware. 

Burst Buffer provides:

 * A fast storage tier between compute nodes and the traditional parallel file system
 * Overlapping job stage-in and stage-out of data for checkpoint and restart
 * Scratch volumes
 * Extended memory I/O workloads
 * Usage and SSD endurance monitoring


Table of Contents
=================

.. toctree::
    :maxdepth: 2
    
    csm/index.rst
    burst-buffer/index.rst
    cast-big-data/index.rst
    releases/index.rst
