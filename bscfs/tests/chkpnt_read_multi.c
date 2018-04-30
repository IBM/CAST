/******************************************************************************
 |    chkpnt_read_multi.c
 |
 |  © Copyright IBM Corporation 2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 ******************************************************************************/

#include "chkpnt_common.h"
#include "bscfsAPI.h"
#include <pthread.h>

char *ChkpntStripe(int n, int s)
{
    char space[BSCFS_PATH_MAX];
    snprintf(space, BSCFS_PATH_MAX, "%s%s%s/chkpnt_%03d_%04d",
	     BSCFS_MNT_PATH, ChkpntDirSep, ChkpntDir, n, s);
    return strdup(space);
    // memory leak
}

char *MapfileStripe(int n, int s)
{
    char space[BSCFS_PATH_MAX];
    snprintf(space, BSCFS_PATH_MAX, "%s%s%s/chkpnt_%03d_%04d.mapfile",
	     BSCFS_PFS_PATH, ChkpntDirSep, ChkpntDir, n, s);
    return strdup(space);
    // memory leak
}

void *memory, *header;
uint64_t mem_size;

void *ReadStripe(void *arg)
{
    void *mem_start;
    uint64_t file_start, stripe_size, size, offset;
    int rc;
    int n = ((uint64_t) arg) >> 32;
    uint64_t s = ((uint64_t) arg) & 0xfffffffful;
    int fd = open(ChkpntStripe(n, s), O_RDONLY, 0);
    Check(fd >= 0, "open(%s) failed", ChkpntStripe(n, s), errno);
    if (HeaderSize > 0) {
	rc = pread(fd, header, HeaderSize, 0);
	Check(rc == HeaderSize, "pread(%s header) failed", ChkpntStripe(n, s), errno);
    }
    mem_start = memory + (s * StripeSize);
    file_start = HeaderSize + (Rank * StripeSize);
    stripe_size = Min(StripeSize, mem_size - ((uint64_t) mem_start));
    for (offset = 0; offset < stripe_size; offset += ChunkSize) {
	size = Min(ChunkSize, stripe_size - offset);
	rc = pread(fd, mem_start + offset, size, file_start + offset);
	Check(rc >= 0, "pread(%s) failed", ChkpntStripe(n, s), errno);
	Check(rc == size, "pread(%s) truncated", ChkpntStripe(n, s), 0);
    }
    rc = close(fd);
    Check(rc == 0, "close(%s) failed", ChkpntStripe(n, s), errno);
    return NULL;
}

int main(int argc, char **argv)
{
    chkpnt_init(argc, argv);

    if (Rank == 0) {
	printf("Initializing.\n");
    }

    struct timespec timer;
    clock_gettime(CLOCK_REALTIME, &timer);

    int rc;

    uint64_t mem_size = MemSize(Rank);
    uint64_t stripe_count = (mem_size + StripeSize - 1) / StripeSize;

    rc = posix_memalign(&memory, 0x10000, mem_size);
    Check(rc == 0, "posix_memalign(memory) failed", NULL, rc);
    header = NULL;
    if (HeaderSize > 0) {
	rc = posix_memalign(&header, 0x10000, HeaderSize);
	Check(rc == 0, "posix_memalign(header) failed", NULL, rc);
    }

    // first input file will have been pre-staged into the SSD

    int n;
    for (n = ChkpntStart; n < (ChkpntStart + ChkpntCount); n++) {
	void *mem_start;
	uint64_t file_start, stripe_size, offset, delta, s;

	Print(&timer, "Reading checkpoint %d.\n", n);
	struct timespec bw_timer = timer;
	pthread_t th[16384];
	for (s = 0; s < stripe_count; s++) {
	    rc = pthread_create(&th[s], NULL, ReadStripe, (void *) ((((uint64_t) n) << 32) + s));
	    Check(rc == 0, "pthread_create failed", NULL, 0);
	}
	for (s = 0; s < stripe_count; s++) {
	    rc = pthread_join(th[s], NULL);
	    Check(rc == 0, "pthread_join failed", NULL, 0);
	}

	Print(&timer, "Barrier.\n", 0);
	MPI_Barrier(MPI_COMM_WORLD);

	PrintBandwidth(&timer, &bw_timer);

	// input file no longer needed; drop it from the SSD
	Print(NULL, "Clearing checkpoint %d from the SSD.\n", n);
	for (s = 0; s < stripe_count; s++) {
	    rc = BSCFS_Forget(ChkpntStripe(n, s));
	    Check(rc == 0, "BSCFS_Forget(%s) failed", ChkpntStripe(n, s), rc);
	}

	// start prefetching the next input file into the SSD
	BBTransferHandle_t handle[16384];
	if ((n+1) < (ChkpntStart + ChkpntCount)) {
	    Print(&timer, "Starting checkpoint %d prefetch.\n", n+1);
	    for (s = 0; s < stripe_count; s++) {
		rc = BSCFS_StartLocalPrefetch(ChkpntStripe(n+1, s), MapfileStripe(n+1, s), &handle[s]);
		Check(rc == 0, "BSCFS_StartLocalPrefetch(%s) failed",
							    ChkpntStripe(n+1, s), rc);
	    }
	}

	// "compute" for the specified number of seconds
	Print(&timer, "Starting computation round %d.\n", n);
	struct timespec time0, time1;
	clock_gettime(CLOCK_REALTIME, &time0);
	// validate checkpoint as part of the computation
	uint64_t error_count = 0;
	for (s = 0; s < stripe_count; s++) {
	    mem_start = memory + (s * StripeSize);
	    file_start = HeaderSize + (((s * RankCount) + Rank) * StripeSize);
	    stripe_size = Min(StripeSize, mem_size - ((uint64_t) mem_start));
	    for (offset = 0; offset < stripe_size; offset += 8) {
		error_count +=
		    ((*((uint64_t *) (mem_start + offset))) !=
			((((uint64_t) n) << 56) | (file_start + offset)));
	    }
	}
	if (HeaderSize > 0) {
	    error_count += ((((uint64_t *) header)[0]) != n);
	    for (offset = 8; offset < HeaderSize; offset += 8) {
		error_count += ((*((uint64_t *) (header + offset))) != -1ul);
	    }
	}
	Check(error_count == 0, "memory check failed", NULL, -1);
	do {
	    clock_gettime(CLOCK_REALTIME, &time1);
	    delta = ((1000000000 * (time1.tv_sec - time0.tv_sec)) +
					time1.tv_nsec) - time0.tv_nsec;
	} while (delta < (1000000000 * ComputeTime));

	// wait here for the prefetch to complete
	if ((n+1) < (ChkpntStart + ChkpntCount)) {
	    Print(&timer, "Awaiting checkpoint %d prefetch.\n", n+1);
	    for (s = 0; s < stripe_count; s++) {
		rc = BSCFS_AwaitLocalTransfer(handle[s]);
		Check(rc == 0, "BSCFS_AwaitLocalTransfer(%s) failed",
							    ChkpntStripe(n+1, s), rc);
	    }
	}
    }

    MPI_Finalize();
    Print(&timer, "Finished.\n", 0);
    return 0;
}
