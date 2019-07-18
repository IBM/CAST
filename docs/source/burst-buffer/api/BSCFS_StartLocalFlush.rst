
#####################
BSCFS_StartLocalFlush
#####################


****
NAME
****


BSCFS_StartLocalFlush - Initiate a transfer of a BSCFS file to the parallel file system.


********
SYNOPSIS
********


#include <bscfs/include/bscfsAPI.h>

static int BSCFS_StartLocalFlush(const char \*pathname, const char \*mapfile, const char \*cleanup_script, BBTransferHandle_t \*handle_p)


***********
DESCRIPTION
***********


BSCFS_StartLocalFlush initiates a transfer to the PFS of any content cached locally for the BSCFS file named pathname, and adds information about this node's contribution to the shared file to mapfile. It causes cleanup_script to be invoked on the FEN if this flush is still in progress when the application terminates. It returns a handle via handle_p that can be used to query the transfer status.

Pathname should refer to a file in BSCFS, while mapfile should name a file in the PFS. Mapfile may be null, in which case no mapping information for this node will be recorded for this transfer. Mapfile will be created if it is not null and does not already exist. All nodes involved in a given flush operation should specify the same mapfile. Otherwise multiple, incomplete mapfiles will be created.
The value returned in the location to which handle_p points can later be passed to BSCFS_AwaitLocalTransfer when the process needs to wait for the transfer to finish. The returned handle can also be used directly with burst-buffer infrastructure routines defined in <bbAPI.h> to modify or query the transfer.
The pathname file must not be open in any process on the local compute node when the transfer is initiated. Typically, the file will be in MODIFIED state, and BSCFS_StartLocalFlush will change its state to FLUSHING. At this point BSCFS will append any partially-filled data block to the data file in the SSD, and it will write out the in-memory index as a separate SSD file. It will then initiate a transfer of the data and index files to the PFS by presenting the pair to the burst-buffer infrastructure as a BSCFS-type transfer bundle.
If the target file is already in any of the FLUSHING-related states, the routine will simply return the handle representing the transfer that was already set up. It will first initiate the pending transfer and change the state to FLUSHING if the initial state was FLUSH_PENDING.
It may seem logical to do nothing for a flush request that targets an INACTIVE file (because there is no local content that needs flushing), but in fact BSCFS will create an empty data file and index and then proceed to flush the empty files normally. The reason for this choice is that it allows a node to participate in the flush protocol even when it did not actually have anything to contribute to the shared file. Without this choice, the application would have to handle the "no contribution" situation as a special case, which might complicate its algorithm. Of course, the application is free to avoid unnecessary flushes if it is able to do so.
A flush request that targets a PREFETCHING or STABLE file will fail with an error indication.
If cleanup_script is not NULL, it should name a user-supplied program, accessible on the FEN, that may be invoked by the stage-out script after the job terminates. The script will be executed if and only if the transfer of the target file is still in progress when the application exits. If it is needed, cleanup_script will be invoked after all nodes' outstanding flushes for the given target file are complete. It will be called with three arguments, the PFS names of the newly-transferred shared file and the associated mapfile, and a return code indicating whether the global transfer completed successfully. It is expected that all nodes will name the same cleanup script for a given target file, but if different scripts are named, each script will be invoked exactly once.
Error code

param
=====


pathname = BSCFS file to be flushed

mapfile = mapfile to be created for this flush

cleanup_script = script to be executed on the FEN

handle_p = handle that represents this flush


retval
======


0 = Success

errno = Positive non-zero values correspond with errno.


