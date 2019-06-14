
##################
BB_GetThrottleRate
##################


****
NAME
****


BB_GetThrottleRate - Gets the transfer rate for a given tag.


********
SYNOPSIS
********


#include <bb/include/bbapi.h>

int BB_GetThrottleRate(const char \*mountpoint, uint64_t \*rate)


***********
DESCRIPTION
***********


The BB_GetThrottleRate routine retrieves the throttled transfer rate for the specified transfer handle.
Actual transfer rate may vary based on file server load, congestion, other BB transfers, etc.
Error code

param
=====


mountpoint = compute node mountpoint to retrieve the throttle rate

rate = Current transfer rate throttle, in bytes-per-second


retval
======


0 = Success

errno = Positive non-zero values correspond with errno. strerror() can be used to interpret.


