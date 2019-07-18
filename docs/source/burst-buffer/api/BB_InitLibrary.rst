
##############
BB_InitLibrary
##############


****
NAME
****


BB_InitLibrary - Initialize bbAPI.


********
SYNOPSIS
********


#include <bb/include/bbapi.h>

int BB_InitLibrary(uint32_t contribId, const char \*clientVersion)


***********
DESCRIPTION
***********


The BB_InitLibrary routine performs basic initialization of the library. During initialization, it opens a connection to the local bbProxy.
BBAPI_CLIENTVERSIONSTR
Error code

param
=====


contribId = Contributor Id

clientVersion = bbAPI header version used when the application was compiled.


retval
======


0 = Success

errno = Positive non-zero values correspond with errno. strerror() can be used to interpret.


