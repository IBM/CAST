
##################
BB_GetTransferInfo
##################


****
NAME
****


BB_GetTransferInfo - Gets the status of an active file transfer.


********
SYNOPSIS
********


#include <bb/include/bbapi.h>

int BB_GetTransferInfo(BBTransferHandle_t handle, BBTransferInfo_t \*info)


***********
DESCRIPTION
***********


The BB_GetTransferInfo routine gets the status of an active file transfer, given the transfer handle.
If multiple files are being transferred within a tag, tags are tracked for the life of the associated job, 
and purged when the logical volume is removed.

Error code

param
=====


handle = Transfer handle

info = Information on the transfer identified by the handle. Caller provides storage for BBTransferInfo_t.


retval
======


0 = Success

errno = Positive non-zero values correspond with errno. strerror() can be used to interpret.


