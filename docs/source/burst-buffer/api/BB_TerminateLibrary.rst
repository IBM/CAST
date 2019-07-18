
###################
BB_TerminateLibrary
###################


****
NAME
****


BB_TerminateLibrary - Terminate library.


********
SYNOPSIS
********


#include <bb/include/bbapi.h>

int BB_TerminateLibrary()


***********
DESCRIPTION
***********


The BB_TerminateLibrary routine closes the connection to the local bbProxy and releases any internal storage, such as existing transfer definitions and handles.
Error code

retval
======


0 = Success

errno = Positive non-zero values correspond with errno. strerror() can be used to interpret.


