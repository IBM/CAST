
########################
BSCFS_QueryInternalFiles
########################


****
NAME
****


BSCFS_QueryInternalFiles - Return the set of internal files associated with a BSCFS file.


********
SYNOPSIS
********


#include <bscfs/include/bscfsAPI.h>

static int BSCFS_QueryInternalFiles(const char \*pathname, size_t files_length, char \*files, int \*state_p)


***********
DESCRIPTION
***********


BSCFS_QueryInternalFiles returns the names of the set of internal files associated with the BSCFS file named pathname. Files is an array of files_length bytes that is to be filled in with a NULL-separated list of file names. The list will be terminated with a double NULL. The routine also returns the current state of the file via the state_p pointer.

The pathname file should be in MODIFIED or STABLE state, or in any of the FLUSHING-related states. If necessary, any partially-filled data block will be appended to the SSD data file associated with the target file, and the index will be written out as a separate SSD file. The names of the index and data files, in that order, are copied into the files character array, assuming they fit in the space provided. The names are separated by a NULL character and terminated with a double NULL. Note that while the interface allows for an arbitrary number of internal files to be returned for a given target, in the initial implementation there will always be exactly two, and they will be listed with the index file first and the data file second.
The current state of the target file (one of the states listed above) will be stored in the location indicated by state_p if the pointer is non-null.
WARNING: It is the responsibility of the caller to ensure that the application does not write to the target file before the caller finishes accessing the internal files.
Error code

param
=====


pathname = BSCFS file to be queried

files_length = size (in bytes) of the files array

files = space for returned file names

state_p = returned state of the queried file


retval
======


0 = Success

errno = Positive non-zero values correspond with errno.


