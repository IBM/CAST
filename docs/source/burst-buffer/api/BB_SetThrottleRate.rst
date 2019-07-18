
##################
BB_SetThrottleRate
##################


****
NAME
****


BB_SetThrottleRate - Sets the maximum transfer rate for a given tag.


********
SYNOPSIS
********


#include <bb/include/bbapi.h>

int BB_SetThrottleRate(const char \*mountpoint, uint64_t rate)


***********
DESCRIPTION
***********


The BB_SetThrottleRate routine sets the upper bound on the transfer rate for the provided transfer handle.
Actual transfer rate may vary based on file server load, congestion, other BB transfers, etc. Multiple concurrent transfer handles can each have their own rate, which are additive from a bbServer perspective. The bbServer rate won't exceed the configured maximum transfer rate. Setting the transfer rate to zero will have the effect of pausing the transfer.
Error code

param
=====


mountpoint = compute node mountpoint to set a maximum rate

rate = New transfer rate, in bytes-per-second


retval
======


0 = Success

errno = Positive non-zero values correspond with errno. strerror() can be used to interpret.


