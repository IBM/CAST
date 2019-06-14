
####################
BB_CreateTransferDef
####################


****
NAME
****


BB_CreateTransferDef - Create storage for a transfer definition.


********
SYNOPSIS
********


#include <bb/include/bbapi.h>

int BB_CreateTransferDef(BBTransferDef_t \*\*transfer)


***********
DESCRIPTION
***********


BB_CreateTransferDef()
The BB_CreateTransferDef routine creates storage for a transfer definition. The caller provides storage for BBTransferDef_t\* and will allocate and initialize storage for BBTransferDef_t.
Error code

param
=====


transfer = User provides storage for BBTransferDef_t\*


retval
======


0 = Success

errno = Positive non-zero values correspond with errno. strerror() can be used to interpret.


