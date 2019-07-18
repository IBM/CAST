
#################
BB_GetDeviceUsage
#################


****
NAME
****


BB_GetDeviceUsage - Get NVMe SSD device usage.


********
SYNOPSIS
********


#include <bb/include/bbapi.h>

int BB_GetDeviceUsage(uint32_t devicenum, BBDeviceUsage_t \*usage)


***********
DESCRIPTION
***********


The BB_GetDeviceUsage routine returns the NVMe device statistics. 
Error code

param
=====


devicenum = The index of the NVMe device on the compute node

usage = The current NVMe device statistics


retval
======


0 = Success

errno = Positive non-zero values correspond with errno. strerror() can be used to interpret.


