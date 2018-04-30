/******************************************************************************
 |    chkpnt_read_PFS.c
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

    void *memory, *header;
    rc = posix_memalign(&memory, 0x10000, mem_size);
    Check(rc == 0, "posix_memalign(memory) failed", NULL, rc);
    header = NULL;
    if (HeaderSize > 0) {
	rc = posix_memalign(&header, 0x10000, HeaderSize);
	Check(rc == 0, "posix_memalign(header) failed", NULL, rc);
    }

    int n;
    for (n = ChkpntStart; n < (ChkpntStart + ChkpntCount); n++) {
	void *mem_start;
	uint64_t file_start, stripe_size, size, offset, delta, i;

	Print(&timer, "Reading checkpoint %d.\n", n);
	struct timespec bw_timer = timer;
	int fd = open(ChkpntPFS(n), O_RDONLY, 0);
	Check(fd >= 0, "open(%s) failed", ChkpntPFS(n), errno);
	if (HeaderSize > 0) {
	    rc = pread(fd, header, HeaderSize, 0);
	    Check(rc == HeaderSize, "pread(%s header) failed", ChkpntPFS(n), 0);
	}
	for (i = 0; i < stripe_count; i++) {
	    mem_start = memory + (i * StripeSize);
	    file_start = HeaderSize + (((i * RankCount) + Rank) * StripeSize);
	    stripe_size = Min(StripeSize, mem_size - ((uint64_t) mem_start));
	    for (offset = 0; offset < stripe_size; offset += ChunkSize) {
		size = Min(ChunkSize, stripe_size - offset);
		rc = pread(fd, mem_start + offset, size, file_start + offset);
		Check(rc >= 0, "pread(%s) failed", ChkpntPFS(n), errno);
		Check(rc == size, "pread(%s) truncated", ChkpntPFS(n), 0);
	    }
	}
	rc = close(fd);
	Check(rc == 0, "close(%s) failed", ChkpntPFS(n), errno);

	Print(&timer, "Barrier.\n", 0);
	MPI_Barrier(MPI_COMM_WORLD);

	PrintBandwidth(&timer, &bw_timer);

	// "compute" for the specified number of seconds
	Print(&timer, "Starting computation round %d.\n", n);
	struct timespec time0, time1;
	clock_gettime(CLOCK_REALTIME, &time0);
	// validate checkpoint as part of the computation
	uint64_t error_count = 0;
	for (i = 0; i < stripe_count; i++) {
	    mem_start = memory + (i * StripeSize);
	    file_start = HeaderSize + (((i * RankCount) + Rank) * StripeSize);
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
    }

    MPI_Finalize();
    Print(&timer, "Finished.\n", 0);
    return 0;
}
