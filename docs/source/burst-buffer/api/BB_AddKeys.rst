
##########
BB_AddKeys
##########


****
NAME
****


BB_AddKeys - Adds identification keys to a transfer definition.


********
SYNOPSIS
********


#include <bb/include/bbapi.h>

int BB_AddKeys(BBTransferDef_t \*transfer, const char \*key, const char \*value)


***********
DESCRIPTION
***********


The BB_AddKeys routine allows the user to add a custom key-value to the transfer. Keys for the same tag are merged on the bbServer. If a key is a duplicate, the result is non-deterministic as to which value will prevail.
Error code

param
=====


transfer = Transfer definition

key = Pointer to string with the key name

value = Pointer to string with the key's value


retval
======


0 = Success

errno = Positive non-zero values correspond with errno. strerror() can be used to interpret.


