
################
BB_StartTransfer
################


****
NAME
****


BB_StartTransfer - Starts transfer of a file.


********
SYNOPSIS
********


#include <bb/include/bbapi.h>

int BB_StartTransfer(BBTransferDef_t \*transfer, BBTransferHandle_t handle)


***********
DESCRIPTION
***********


The BB_StartTransfer routine starts an asynchronous transfer between the parallel file system and local SSD for the specified transfer definition.
BB_StartTransfer will fail if the contribid for a process has already contributed to this tag. BB_StartTransfer will fail if the bbAPI process has an open file descriptor for one of the specified files.
Error code

param
=====


transfer = Pointer to the structure that defines the transfer

handle = Opaque handle to the transfer


retval
======


0 = Success

errno = Positive non-zero values correspond with errno. strerror() can be used to interpret.


