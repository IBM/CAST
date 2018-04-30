/*******************************************************************************
 |  app_common.c
 |
 |  The purpose of the progam is to verify that the burst buffer API interfaces,
 |  as defined in bbapi.h, match the actual implementations for those APIs.
 |  The intent is that this program should successfully compile and link.
 |  It is not intended for this program to be run.
 |
 |  � Copyright IBM Corporation 2015,2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/

#define _GNU_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

#include "app_common.h"
#include "mpi.h"



/* reliable read from file descriptor (retries, if necessary, until hard error) */
ssize_t reliable_read(int fd, void* buf, size_t size) {
    ssize_t n = 0;
    int retries = 10;
    char host[128];
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    gethostname(host, sizeof(host));
    while (n < size)
        {
            ssize_t rc = read(fd, (char*) buf + n, size - n);
            if (rc > 0) {
                n += rc;
            } else if (rc == 0) {
                /* EOF */
                return n;
            } else { /* (rc < 0) */
                /* got an error, check whether it was serious */
                if (errno == EINTR || errno == EAGAIN)
                    continue;
                
                /* something worth printing an error about */
                retries--;
                if (retries) {
                    /* print an error and try again */
                    fprintf(stdout,
                            "%d on %s: ERROR: Error reading: read(%d, %p, %ld): %s\n",
                            rank, host, fd, (char*) buf + n, size - n, strerror(errno));
                } else {
                    /* too many failed retries, give up */
                    fprintf(stdout,
                            "%d on %s: ERROR: Giving up after reading %zu bytes: read(%d, %p, %ld): %s\n",
                            rank, host, n, fd, (char*) buf + n, size - n, strerror(errno));
                    return -1;
                }
            }
	}
    return size;
}

/* reliable write to file descriptor (retries, if necessary, until hard error) */
ssize_t reliable_write(int fd, const void* buf, size_t size) {
    size_t n = 0;
    ssize_t rc = 0;
    int retries = 10;
    int rank;
    char host[128];
    while (n < size) {
        rc = write(fd, (char*) buf + n, size - n);
        if (rc > 0) {
            n += rc;
        } else if (rc == 0) {
            /* something bad happened, print an error and abort */
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            gethostname(host, sizeof(host));
            fprintf(stdout,
                    "%d on %s: ERROR: Error writing: write(%d, %p, %ld) returned 0 @ %s:%d\n",
                    rank, host, fd, (char*) buf + n, size - n, __FILE__,
                    __LINE__);
            MPI_Abort(MPI_COMM_WORLD, 0);
        } else { /* (rc < 0) */
            /* got an error, check whether it was serious */
            if (errno == EINTR || errno == EAGAIN)
                continue;
            
            /* something worth printing an error about */
            retries--;
            if (retries) {
                /* print an error and try again */
                MPI_Comm_rank(MPI_COMM_WORLD, &rank);
                gethostname(host, sizeof(host));
                fprintf(stdout,
                        "%d on %s: ERROR: Error writing: write(%d, %p, %ld): %s\n",
                        rank, host, fd, (char*) buf + n, size - n, strerror(errno) );
            } else {
                /* too many failed retries, give up */
                MPI_Comm_rank(MPI_COMM_WORLD, &rank);
                gethostname(host, sizeof(host));
                fprintf(stdout,
                        "%d on %s: ERROR: Giving up write: write(%d, %p, %ld): %s\n",
                        rank, host, fd, (char*) buf + n, size - n, strerror(errno));
                return -1;
            }
        }
    }
    return n;
}


/* write the checkpoint data to fd, and return whether the write was successful */
int write_checkpoint(int fd, void* buf, size_t size) 
{
    ssize_t rc;
    int valid = 1;

    /* write the checkpoint data */
    rc = reliable_write(fd, buf, size);
    if (rc < 0) {
        valid = 0;
    }

    return valid;
}


int write_checkpoint_fragmented(int fd, void* buf, size_t size, int hard_fragmentation)
{
    ssize_t rc, total;
    int valid = 1;
    char * offset;
    total = 0;
    offset = buf;
    /* write the checkpoint data */
    while(total != size)
        {            
            rc = reliable_write(fd, offset, ((size - total) < 65536)? (size - total) : 65536 );
            if(hard_fragmentation)
                {
                    syncfs(fd);
                    MPI_Barrier(MPI_COMM_WORLD);
                }
            if (rc < 0)
                {
                    valid = 0;
                    break;
                }
            total +=rc;
            offset +=rc;
        }
    return valid;
}


/* read the checkpoint data from file into buf, and return whether the read was successful */
int read_checkpoint(int fd, void* buf, size_t size) 
{
    ssize_t n;
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    n = reliable_read(fd, buf, size);
    if (n != size)
        {
            fprintf(stderr,  "Filesize not correct. [size: %zu, n: %zu, rank: %d]\n",
                    size, n, rank);
            fflush(stdout);
            return 0;
        }
    return 1;
}

