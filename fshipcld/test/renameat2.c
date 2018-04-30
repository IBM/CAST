/*******************************************************************************
 |    renameat2.c
 |
 |  © Copyright IBM Corporation 2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/
/* compile with: gcc -O2 -W -Wall -g -o rnameat2 renameat2.c */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/syscall.h>

#define MEM_SIZE (100*1024*1024*1024L)

int main() {
    char file1[]="joe";
    char file2[]="jeff";
    unsigned int flags = 0;
    errno=0;
    int l_RCrenameat2 = syscall(SYS_renameat2,-1,file1,-1,file2,flags);
    printf("l_RCrenameat2=%d for file1=%s file2=%s flags=%x errno=%d(%s)\n", l_RCrenameat2,file1,file2,flags,errno,strerror(errno) );
    return errno;
}
