
################
BB_SetUsageLimit
################


****
NAME
****


BB_SetUsageLimit - Sets the usage threshold.


********
SYNOPSIS
********


#include <bb/include/bbapi.h>

int BB_SetUsageLimit(const char \*mountpoint, BBUsage_t \*usage)


***********
DESCRIPTION
***********


The BB_SetUsageLimit routine sets the SSD usage threshold. When either usage->totalBytesRead or usage->totalBytesWritten is non-zero and exceeds the current threshold for the specified mount point, a RAS event will be generated. 
Error code

param
=====


mountpoint = The path to the mount point that will be monitored

usage = The SSD activity limits that should be enforced


retval
======


0 = Success

errno = Positive non-zero values correspond with errno. strerror() can be used to interpret.


