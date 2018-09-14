Cluster Administration Service Tools 
====================================

CAST stands for **C**\ luster **A**\ dministration **S**\ ervice **T**\ ools.

CAST is comprised of several open source components:

**CSM - Cluster System Management**

A C API for managing a large cluster. Offers a suite of tools for maintaining the cluster:

 * Discovery and management of system resources
 * Database integration (PostgreSQL)
 * Job launch support (workload management APIs)
 * Node diagnostics (diag APIs and scripts)
 * RAS events and actions
 * Infrastructure Health checks
 * Python Bindings for C APIs


**Burst Buffer**

A cost-effective mechanism that can improve I/O performance for a large class of high-performance 
computing applications without requirement of intermediary hardware. Burst Buffer provides:

 * A fast storage tier between compute nodes and the traditional parallel file system
 * Overlapping job stage-in and stage-out of data for checkpoint and restart
 * Scratch volumes
 * Extended memory I/O workloads


**Function Shipping**

A file I/O forwarding layer for Linux that aims to provide low-jitter access to remote parallel file system while retaining common POSIX semantics.


Table of Contents
==================

.. toctree::
    :maxdepth: 2
    
    csm/index.rst
    csmdb/index.rst
    cast-big-data/index.rst
