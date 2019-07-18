
##########################
BSCFS_GlobalFlushCompleted
##########################


****
NAME
****


BSCFS_GlobalFlushCompleted - Inform BSCFS that a flush operation has completed globally.


********
SYNOPSIS
********


#include <bscfs/include/bscfsAPI.h>

static int BSCFS_GlobalFlushCompleted(const char \*pathname)


***********
DESCRIPTION
***********


BSCFS_GlobalFlushCompleted informs the local BSCFS instance that the transfer of the BSCFS file named pathname to the PFS has been completed by all participating nodes.

The application is responsible for ensuring that all its compute nodes' contributions to a shared file are individually transferred and incorporated into the actual file in the PFS. Once it has done so, it must make a BSCFS_GlobalFlushCompleted call on every node to let the BSCFS instances know that the shared file is available for reading.
The first call to BSCFS_GlobalFlushCompleted for a given file on any node should find the file in FLUSH_COMPLETED state. The call will change the state to STABLE, and subsequent calls, if any, will succeed without actually doing anything.
BSCFS_GlobalFlushCompleted will fail with an error indication if the target file is found to be in INACTIVE, MODIFIED, FLUSH_PENDING, FLUSHING, or PREFETCHING state.
Error code

param
=====


pathname = BSCFS file to be prefetched


retval
======


0 = Success

errno = Positive non-zero values correspond with errno.


