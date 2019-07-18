
##################
BB_FreeTransferDef
##################


****
NAME
****


BB_FreeTransferDef - Releases storage for BBTransferDef_t.


********
SYNOPSIS
********


#include <bb/include/bbapi.h>

int BB_FreeTransferDef(BBTransferDef_t \*transfer)


***********
DESCRIPTION
***********


The BB_FreeTransferDef routine releases storage for BBTransferDef_t.
Error code

param
=====


transfer = Pointer to reclaim


retval
======


0 = Success

errno = Positive non-zero values correspond with errno. strerror() can be used to interpret.


