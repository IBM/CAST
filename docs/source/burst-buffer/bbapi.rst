Burst Buffer API
================

The burst buffer API is the interface between users and the burst buffer service. It allows users to define, start, cancel, and query information on their transfers. The API is also intended to be consistent with the bbCmd utility component that executes on the front-end nodes, allowing for inspection and modification of the burst buffer transfers during all stages of the job.

Burst Buffer Shared Checkpoint File System (BSCFS) uses the Burst Buffer APIs to allow shared (N:1 or N:M) checkpoints across multiple nodes to leverage node-local non-volatile storage. 

Compilation
-----------
Adding the following options to the compile command-line should include bbAPI headers and library, as well as setting rpath to point to the bbAPI library.  
-I/opt/ibm -Wl,--rpath /opt/ibm/bb/lib -L/opt/ibm/bb/lib -lbbAPI

An example testcase can be built via:
$ mpicc /opt/ibm/bb/tests/test_basic_xfer.c -o test_basic_xfer -I/opt/ibm -L/opt/ibm/bb/lib -Wl,--rpath=/opt/ibm/bb/lib -lbbAPI



**Burst Buffer**

 .. toctree::
    :maxdepth: 1

    api/BB_AddFiles.rst
    api/BB_AddKeys.rst
    api/BB_CancelTransfer.rst
    api/BB_CreateTransferDef.rst
    api/BB_FreeTransferDef.rst
    api/BB_GetDeviceUsage.rst
    api/BB_GetLastErrorDetails.rst
    api/BB_GetThrottleRate.rst
    api/BB_GetTransferHandle.rst
    api/BB_GetTransferInfo.rst
    api/BB_GetTransferKeys.rst
    api/BB_GetTransferList.rst
    api/BB_GetUsage.rst
    api/BB_GetVersion.rst
    api/BB_InitLibrary.rst
    api/BB_SetThrottleRate.rst
    api/BB_SetUsageLimit.rst
    api/BB_StartTransfer.rst
    api/BB_TerminateLibrary.rst

**Burst Buffer Shared Checkpoint File System (BSCFS) APIs**

 .. toctree::
   :maxdepth: 1

   api/BSCFS_AwaitLocalTransfer.rst
   api/BSCFS_Forget.rst
   api/BSCFS_GlobalFlushCompleted.rst
   api/BSCFS_InstallInternalFiles.rst
   api/BSCFS_PrepareLocalFlush.rst
   api/BSCFS_QueryInternalFiles.rst
   api/BSCFS_StartLocalFlush.rst
   api/BSCFS_StartLocalPrefetch.rst
