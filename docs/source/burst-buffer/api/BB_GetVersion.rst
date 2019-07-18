
#############
BB_GetVersion
#############


****
NAME
****


BB_GetVersion - Fetch the expected version string.


********
SYNOPSIS
********


#include <bb/include/bbapi.h>

int BB_GetVersion(size_t size, char \*APIVersion)


***********
DESCRIPTION
***********


The BB_GetVersion routine returns the expected version string for the API. This routine is intended for version mismatch debug. It is not intended to generate the string to pass into BB_InitLibrary.
Error code

param
=====


size = The amount of space provided to hold APIVersion

APIVersion = The string containing the bbAPI's expected version.


retval
======


0 = Success

errno = Positive non-zero values correspond with errno. strerror() can be used to interpret.


