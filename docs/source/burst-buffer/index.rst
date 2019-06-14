.. _BURST_BUFFER:

Burst Buffer
============

The Burst Buffer is an I/O data caching technology which can improve I/O performance for a large class of high-performance 
computing applications without requirement of intermediary hardware. 
Using an SSD drive per compute node and NVMe over Fabrics, the burst buffer can asynchronously transfer data to or from the drive 
before the application reads a file or after it writes a file. The result is that the application benefits from native SSD 
performance for a portion of its I/O requests.  Applications can create, read, and write data on the burst buffer using standard Linux file I/O system calls.  

Burst Buffer provides:

 * A fast storage tier between compute nodes and the traditional parallel file system
 * Overlapping job stage-in and stage-out of data for checkpoint and restart
 * Scratch volumes
 * Extended memory I/O workloads
 * Usage and SSD endurance monitoring


**Table of Contents**

 .. toctree::
    :maxdepth: 2

    bbinstall.rst
    bbapi.rst
    bbcommands.rst
