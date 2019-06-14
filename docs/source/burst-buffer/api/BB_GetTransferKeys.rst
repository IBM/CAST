
##################
BB_GetTransferKeys
##################


****
NAME
****


BB_GetTransferKeys - Gets the identification keys for a transfer handle.


********
SYNOPSIS
********


#include <bb/include/bbapi.h>

int BB_GetTransferKeys(BBTransferHandle_t handle, size_t buffersize, char \*bufferForKeyData)


***********
DESCRIPTION
***********


The BB_GetTransferKeys routine allows the user to retrieve the custom key-value pairs for a transfer. The invoker of this API provides the storage for the returned data using the bufferForKeyData parameter. The buffersize parameter is input as the size available for the returned data. If not enough space is provided, then the API call fails. The format for the returned key-value pairs is JSON.
Error code

param
=====


handle = Opaque handle to the transfer

buffersize = Size of buffer

in/out] = bufferForKeyData Pointer to buffer


retval
======


0 = Success

-1 = Failure

-2 = Key data not returned because additional space is required to return the data


