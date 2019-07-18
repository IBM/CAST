
##########################
BSCFS_InstallInternalFiles
##########################


****
NAME
****


BSCFS_InstallInternalFiles - Activate a BSCFS file using the provided set of internal files.


********
SYNOPSIS
********


#include <bscfs/include/bscfsAPI.h>

static int BSCFS_InstallInternalFiles(const char \*pathname, size_t files_length, const char \*files, int state)


***********
DESCRIPTION
***********


BSCFS_InstallInternalFiles provides the names of a set of internal files that are to be associated with the BSCFS file named pathname. Files is an array of files_length bytes that holds a NULL-separated list of file names. The list should be terminated with a double NULL. The routine activates the target file in the given state.

The pathname file is expected to be INACTIVE when this call is made. For the initial implementation, files is expected to hold exactly two file names, an index file and a data file, in that order. BSCFS will incorporate the two internal files and activate the target file in the specified state, which should be either MODIFIED or STABLE. If the state is MODIFIED, the situation will be the same as if the application had written the data file content on the local compute node and not flushed it to the PFS. If the state is STABLE, the situation will be the same as if the application had prefetched the data file content from the PFS (or written and globally flushed it).
WARNING: It is the responsibility of the caller to provide internal files that are properly formatted and internally consistent, and also to provide files that are consistent across nodes. For example, it would be an (undetected) error to establish a target file as MODIFIED on one node and STABLE on another.
Error code

param
=====


pathname = BSCFS file to be activated

files_length = size (in bytes) of the files array

files = space for provided file names

state = state of the activated file


retval
======


0 = Success

errno = Positive non-zero values correspond with errno.


