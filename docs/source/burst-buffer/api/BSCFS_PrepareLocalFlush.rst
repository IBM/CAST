
#######################
BSCFS_PrepareLocalFlush
#######################


****
NAME
****


BSCFS_PrepareLocalFlush - Prepare for a transfer of a BSCFS file to the parallel file system.


********
SYNOPSIS
********


#include <bscfs/include/bscfsAPI.h>

static int BSCFS_PrepareLocalFlush(const char \*pathname, const char \*mapfile, const char \*cleanup_script)


***********
DESCRIPTION
***********


BSCFS_PrepareLocalFlush prepares for, but does not initiate, a transfer to the PFS of any content cached locally for the BSCFS file named pathname, with information about this node's contribution to the shared file to be added to mapfile. It causes transfer to be initiated from the FEN if this flush operation is still pending when the application terminates, and in that case it also causes cleanup_script to be invoked on the FEN when the transfer eventually completes.

The arguments and operation of BSCFS_PrepareLocalFlush are identical to those of BSCFS_StartLocalFlush defined in the previous section, except that the background transfer of the target file is not actually started and no handle is returned. The transfer will remain pending until either the target file is "forgotten" (see BSCFS_Forget) or the application terminates. In the latter case, the BSCFS stage-out script running on the FEN will initiate the transfer and invoke the cleanup script when it completes. BSCFS_PrepareLocalFlush leaves the target file in FLUSH_PENDING state, unless it was already in FLUSHING or FLUSH_COMPLETED state.
Multiple calls to BSCFS_StartLocalFlush and BSCFS_PrepareLocalFlush for the same target file are allowed. The mapfile and cleanup_script provided with the first such call are the only ones that count. These parameters will be ignored for the second and subsequent calls for the same file. A call to BSCFS_StartLocalFlush can be used to initiate a pending transfer that was previously set up by BSCFS_PrepareLocalFlush.
Error code

param
=====


pathname = BSCFS file to be flushed

mapfile = mapfile to be created for this flush

cleanup_script = script to be executed on the FEN


retval
======


0 = Success

errno = Positive non-zero values correspond with errno.


