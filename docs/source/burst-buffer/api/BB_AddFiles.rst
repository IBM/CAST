
###########
BB_AddFiles
###########


****
NAME
****


BB_AddFiles - Adds files to a transfer definition.


********
SYNOPSIS
********


#include <bb/include/bbapi.h>

int BB_AddFiles(BBTransferDef_t \*transfer, const char \*source, const char \*target, BBFILEFLAGS flags)


***********
DESCRIPTION
***********


The BB_AddFiles routine adds a file or a directory to the transfer definition
BB_StartTransfer()
BB_StartTransfer()
If source and target reside on the local SSD, may perform a synchronous 'cp'. If source and target reside on GPFS, bbServer may perform a server-side 'cp' command rather than an NVMe over Fabrics transfer. All parent directories in the target must exist prior to call.

When adding a directory, all contents of the directory will be transferred to the target location. If BBRecursive flags is specified, then any subdirectories will be added. Only files present in the directories at the time of the BB_AddFiles call will be added to the transfer definition.
Error code

param
=====


transfer = Transfer definition

source = Full path to the source file location

target = Full path to the target file location

flags = Any flags for this file. (See BBFILEFLAGS for possible values.)


retval
======


0 = Success

errno = Positive non-zero values correspond with errno. strerror() can be used to interpret.


