
####################
BB_GetTransferHandle
####################


****
NAME
****


BB_GetTransferHandle - Retrieves a transfer handle.


********
SYNOPSIS
********


#include <bb/include/bbapi.h>

int BB_GetTransferHandle(BBTAG tag, uint64_t numcontrib, uint32_t contrib[], BBTransferHandle_t \*handle)


***********
DESCRIPTION
***********


The BB_GetTransferHandle routine retrieves a transfer handle based upon the input criteria. If this is the first request made for this job using the input tag and contrib values, a transfer handle will be generated and returned. Subsequent requests made for the job using the same input tag and contrib values will return the prior generated value as the transfer handle.

Transfer handles are associated with the current jobid and jobstepid.
If numcontrib==1 and contrib==NULL, the invoker's contributor id is assumed.
Error code

param
=====


tag = User-specified tag

numcontrib = Number of entries in the contrib[] array

contrib = Array of contributor indexes

handle = Opaque handle to the transfer


retval
======


0 = Success

errno = Positive non-zero values correspond with errno. strerror() can be used to interpret.


