
#################
BB_CancelTransfer
#################


****
NAME
****


BB_CancelTransfer - Cancels an active file transfer.


********
SYNOPSIS
********


#include <bb/include/bbapi.h>

int BB_CancelTransfer(BBTransferHandle_t handle, BBCANCELSCOPE scope)


***********
DESCRIPTION
***********


The BB_CancelTransfer routine cancels an existing asynchronous file transfers specified by the transfer handle. When the call returns, the transfer has been stopped or an error has occurred. As part of the cancel, any parts of the files that have been transferred will have been deleted from the PFS target location.
Error code

param
=====


handle = Transfer handle from BB_StartTransfer. All transfers matching the tag will be canceled.

scope = Specifies the scope of the cancel. (See BBCANCELSCOPE for possible values.)


retval
======


0 = Success

errno = Positive non-zero values correspond with errno. strerror() can be used to interpret.


