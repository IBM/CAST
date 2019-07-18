
######################
BB_GetLastErrorDetails
######################


****
NAME
****


BB_GetLastErrorDetails - Obtain error details of the last bbAPI call.


********
SYNOPSIS
********


#include <bb/include/bbapi.h>

int BB_GetLastErrorDetails(BBERRORFORMAT format, size_t \*numAvailBytes, size_t buffersize, char \*bufferForErrorDetails)


***********
DESCRIPTION
***********


The BB_GetLastErrorDetails routine provides contextual details of the last API call to help the caller determine the failure. The failure information is returned in a C string in the format specified by the caller.
The last error details are 
. Each thread has its separate and distinct copy of a "last error" string. A thread invoking a burst buffer API will get a different "last error" string than another thread invoking burst buffer APIs.
thread local

Only details from the last bbAPI call performed on that software thread are returned. If the process is multi-threaded, the error information is tracked separately between the threads.
Error code

param
=====


format = Format of data to return. (See BBERRORFORMAT for possible values.)

numAvailBytes = Number of bytes available to return

buffersize = Size of buffer

in/out] = bufferForErrorDetails Pointer to buffer


retval
======


0 = Success

errno = Positive non-zero values correspond with errno. strerror() can be used to interpret.


