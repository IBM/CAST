/*******************************************************************************
 |    mmap.c
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
/* gcc -O2 -W -Wall -g -o mmap mmap.c */

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>

const long long int GiB = 1024*1024*1024;

int main() {
    int fd = open("mmaptest", O_RDWR | O_CREAT, 0600);
    if (ftruncate(fd, GiB) != 0){ 
           perror("ftruncate");
           exit(1);
    }
    else printf("ftruncate success\n ");
    void *map = mmap(NULL, GiB, PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED){
        printf("mmap MAP_FAILED \n");
        perror("mmap");
        exit(2);
    }
    else printf("mmap success \n");
    if (munmap(map, GiB) != 0) perror("munmap");
    close(fd);
    return 0;
}
