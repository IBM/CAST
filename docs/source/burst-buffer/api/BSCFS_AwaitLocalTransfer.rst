
########################
BSCFS_AwaitLocalTransfer
########################


****
NAME
****


BSCFS_AwaitLocalTransfer - Wait for a previously-started transfer to complete.


********
SYNOPSIS
********


#include <bscfs/include/bscfsAPI.h>

static int BSCFS_AwaitLocalTransfer(BBTransferHandle_t handle)


***********
DESCRIPTION
***********


BSCFS_AwaitLocalTransfer waits for a previously-started flush or prefetch operation to complete. The handle should be one that was returned by a prior call to BSCFS_StartLocalFlush or BSCFS_StartLocalPrefetch.

A successful call to BSCFS_AwaitLocalTransfer indicates that the current compute node's contribution to a shared file has been incorporated into the actual file and associated mapfile on the PFS, or that the shared-file content directed to the local compute node is now available for reading. In either case, the completion refers only to file content transferred from or to the current node. It says nothing about other nodes' transfers. Prefetches by different nodes are essentially independent, so there is never a need to coordinate them globally, but the story is different for flush operations. The application is responsible for ensuring that all nodes' contributions to a shared file are transferred successfully before declaring the file complete.
For a flush operation, BSCFS_AwaitLocalTransfer will fail with an error indication if the target file is in any state other than one of the FLUSHING-related states. It will also report an error if the transfer fails for any reason. BSCFS_AwaitLocalTransfer leaves the file in FLUSH_COMPLETED state after waiting for the local transfer to complete.
For a prefetch operation, BSCFS_AwaitLocalTransfer will wait for the local transfer to complete and will then change the state of the target file from PREFETCHING to STABLE (unless it was already changed by a prior call). PREFETCHING and STABLE are the only states allowed by BSCFS_AwaitLocalTransfer for a prefetch. Prefetching differs from flushing in that there is no prefetching analog of the FLUSH_COMPLETED state. There is no requirement for global synchronization for prefetches, so the target file is considered STABLE as soon as the local prefetch completes.
Error code

param
=====


handle = handle that represents the awaited transfer


retval
======


0 = Success

errno = Positive non-zero values correspond with errno.


