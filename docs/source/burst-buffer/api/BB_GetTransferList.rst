
##################
BB_GetTransferList
##################


****
NAME
****


BB_GetTransferList - Obtain the list of transfers.


********
SYNOPSIS
********


#include <bb/include/bbapi.h>

int BB_GetTransferList(BBSTATUS matchstatus, uint64_t \*numHandles, BBTransferHandle_t array_of_handles[], uint64_t \*numAvailHandles)


***********
DESCRIPTION
***********


The BB_GetTransferList routine obtains the list of transfers within the job that match the status criteria. BBSTATUS values are powers-of-2 so they can be bitwise OR'd together to form a mask (matchstatus). For each of the job's transfers, this mask is bitwise AND'd against the status of the transfer and if non-zero, the transfer handle for that transfer is returned in the array_of_handles.

Transfer handles are associated with a jobid and jobstepid. Only those transfer handles that were generated for the current jobid and jobstepid are returned.
If array_of_handles==NULL, then only the matching numHandles is returned. 
Error code

param
=====


matchstatus = Only transfers with a status that match matchstatus will be returned. matchstatus can be a OR'd mask of several BBSTATUS values.

numHandles = Populated with the number of handles returned. Upon entry, contains the number of handles allocated to the array_of_handles.

array_of_handles = Returns an array of handles that match matchstatus. The caller provides storage for the array_of_handles and indicates the number of available elements in numHandles.


param
=====


numAvailHandles = Populated with the number of handles available to be returned that match matchstatus.


retval
======


0 = Success

errno = Positive non-zero values correspond with errno. strerror() can be used to interpret.


