
########################
BSCFS_StartLocalPrefetch
########################


****
NAME
****


BSCFS_StartLocalPrefetch - Initiate a prefetch of a BSCFS file from the parallel file system.


********
SYNOPSIS
********


#include <bscfs/include/bscfsAPI.h>

static int BSCFS_StartLocalPrefetch(const char \*pathname, const char \*mapfile, BBTransferHandle_t \*handle_p)


***********
DESCRIPTION
***********


BSCFS_StartLocalPrefetch initiates a prefetch of the BSCFS file named pathname from the PFS to the local node, using mapfile to determine what parts of the file to transfer. It returns a handle via handle_p that can be used to query the transfer status.

Pathname should refer to a file in BSCFS, while mapfile should name a file in the PFS. The value returned in the location pointed to by handle_p can later be passed to BSCFS_AwaitLocalTransfer when the process needs to wait for the transfer to finish.
The pathname file must not be open in any process on the local compute node when the BSCFS_StartLocalPrefetch call is issued. Typically, the file will be in INACTIVE state or already PREFETCHING, and it will be in PREFETCHING state as a result of the call. Initiating a prefetch on a STABLE file is not permitted, except in the case that a prefetch for the file has already been started AND awaited. In that case, the handle for the completed prefetch is returned, and the file remains STABLE. Prefetching a MODIFIED target file or one that is in any stage of FLUSHING is not permitted.
To initiate a prefetch, BSCFS will construct a transfer bundle (of type "BSCFS") that names the shared file and the specified mapfile as source files, and names local data and index files as targets. It will submit the bundle to the burst-buffer infrastructure and return the handle that represents the transfer to the caller. Note that a call to BSCFS_StartLocalPrefetch results only in the transfer of shared-file content destined for the local compute node, as specified by the named mapfile. Separate transfers must be initiated on each compute node to which content is directed. For prefetching, there is no requirement that the same mapfile be used on all compute nodes.
Error code

param
=====


pathname = BSCFS file to be prefetched

mapfile = mapfile to be used to direct this prefetch

handle_p = handle that represents this prefetch


retval
======


0 = Success

errno = Positive non-zero values correspond with errno.


