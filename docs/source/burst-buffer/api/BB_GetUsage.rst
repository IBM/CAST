
###########
BB_GetUsage
###########


****
NAME
****


BB_GetUsage - Get SSD Usage.


********
SYNOPSIS
********


#include <bb/include/bbapi.h>

int BB_GetUsage(const char \*mountpoint, BBUsage_t \*usage)


***********
DESCRIPTION
***********


The BB_GetUsage routine returns the SSD usage for the given logical volume on the compute node. When the LSF job exits, all logical volumes created under that LSF job ID will be queried to provide a summary SSD usage for the compute node.
Error code

param
=====


mountpoint = Logical volume to get user-level usage information

usage = Usage information


retval
======


0 = Success

errno = Positive non-zero values correspond with errno. strerror() can be used to interpret.


