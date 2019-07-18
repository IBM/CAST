
############
BSCFS_Forget
############


****
NAME
****


BSCFS_Forget - Release all resources associated with a BSCFS file.


********
SYNOPSIS
********


#include <bscfs/include/bscfsAPI.h>

static int BSCFS_Forget(const char \*pathname)


***********
DESCRIPTION
***********


BSCFS_Forget releases all resources associated with the BSCFS file named pathname, and returns the file to INACTIVE state.

BSCFS_Forget attempts to clean up no matter the state in which it finds the pathname file. It will free any data buffers associated with the file, as well as the in-memory index, and it will unlink the SSD data and index files if they exist. If the file is FLUSHING or PREFETCHING, BSCFS_Forget will attempt to cancel the outstanding burst-buffer transfer. If the file is in FLUSH_PENDING state, its planned post-job flush will be suppressed.
Error code

param
=====


pathname = BSCFS file to be forgotten


retval
======


0 = Success

errno = Positive non-zero values correspond with errno.


