/******************************************************************************
 |    chkpnt_write_multi.c
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

char *ChkpntStripePFS(int n, int s)
{
    char space[BSCFS_PATH_MAX];
    snprintf(space, BSCFS_PATH_MAX, "%s%s%s/chkpnt_%03d_%04d",
	     BSCFS_PFS_PATH, ChkpntDirSep, ChkpntDir, n, s);
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

void *WriteStripe(void *arg)
{
    void *mem_start;
    uint64_t file_start, stripe_size, size, offset;
    int rc;
    int n = ((uint64_t) arg) >> 32;
    uint64_t s = ((uint64_t) arg) & 0xfffffffful;
    int fd = open(ChkpntStripe(n, s), O_WRONLY|O_CREAT, 0640);
    Check(fd >= 0, "open(%s) failed", ChkpntStripe(n, s), errno);
    if ((Rank == 0) && (HeaderSize > 0)) {
	rc = pwrite(fd, header, HeaderSize, 0);
	Check(rc == HeaderSize, "pwrite(%s header) failed",
					ChkpntStripe(n, s), 0);
    }
    mem_start = memory + (s * StripeSize);
    file_start = HeaderSize + (Rank * StripeSize);
    stripe_size = Min(StripeSize, mem_size - ((uint64_t) mem_start));
    for (offset = 0; offset < stripe_size; offset += ChunkSize) {
	size = Min(ChunkSize, stripe_size - offset);
	rc = pwrite(fd, mem_start + offset, size, file_start + offset);
	Check(rc >= 0, "pwrite(%s) failed", ChkpntStripe(n, s), errno);
	Check(rc == size, "pwrite(%s) truncated",
					ChkpntStripe(n, s), 0);
    }
    rc = fsync(fd);
    Check(rc == 0, "fsync(%s) failed", ChkpntStripe(n, s), errno);
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

    mem_size = MemSize(Rank);
    uint64_t stripe_count = (mem_size + StripeSize - 1) / StripeSize;

    rc = posix_memalign(&memory, 0x10000, mem_size);
    Check(rc == 0, "posix_memalign(memory) failed", NULL, rc);
    header = NULL;
    if (HeaderSize > 0) {
	rc = posix_memalign(&header, 0x10000, HeaderSize);
	Check(rc == 0, "posix_memalign(header) failed", NULL, rc);
    }

    BBTransferHandle_t handle[16384];

    int n;
    for (n = ChkpntStart; n < (ChkpntStart + ChkpntCount); n++) {
	void *mem_start;
	uint64_t file_start, stripe_size, offset, delta, s;

	// "compute" for the specified number of seconds
	Print(&timer, "Starting computation round %d.\n", n);
	struct timespec time0, time1;
	clock_gettime(CLOCK_REALTIME, &time0);
	// fill memory stripes as part of the computation
	for (s = 0; s < stripe_count; s++) {
	    mem_start = memory + (s * StripeSize);
	    file_start = HeaderSize + (((s * RankCount) + Rank) * StripeSize);
	    stripe_size = Min(StripeSize, mem_size - ((uint64_t) mem_start));
	    for (offset = 0; offset < stripe_size; offset += 8) {
		// each word contains its own file offset and the chkpnt number
		*((uint64_t *) (mem_start + offset)) =
		    (((uint64_t) n) << 56) | (file_start + offset);
	    }
	}
	// fill the header if rank 0
	if ((Rank == 0) && (HeaderSize > 0)) {
	    ((uint64_t *) header)[0] = n;
	    for (offset = 8; offset < HeaderSize; offset += 8) {
		*((uint64_t *) (header + offset)) = -1ul;
	    }
	}
	do {
	    clock_gettime(CLOCK_REALTIME, &time1);
	    delta = ((1000000000 * (time1.tv_sec - time0.tv_sec)) +
					time1.tv_nsec) - time0.tv_nsec;
	} while (delta < (1000000000 * ComputeTime));

	if ((n-1) >= ChkpntStart) {
	    // wait for the transfer of the previous checkpoint to complete
	    Print(&timer, "Awaiting checkpoint %d local transfer.\n", n-1);
	    for (s = 0; s < stripe_count; s++) {
		rc = BSCFS_AwaitLocalTransfer(handle[s]);
		Check(rc == 0, "BSCFS_AwaitLocalTransfer(%s) failed",
						    ChkpntStripe(n-1, s), rc);
	    }
	}

	Print(&timer, "Barrier.\n", 0);
	MPI_Barrier(MPI_COMM_WORLD);

	if ((n-1) >= ChkpntStart) {
	    // the previous checkpoint has been transferred by all nodes
	    Print(&timer, "Finalizing checkpoint %d.\n", n-1);
	    for (s = 0; s < stripe_count; s++) {
		rc = BSCFS_GlobalFlushCompleted(ChkpntStripe(n-1, s));
		Check(rc == 0, "BSCFS_GlobalFlushCompleted(%s) failed",
						    ChkpntStripe(n-1, s), rc);
	    }

	    if (Rank == 0) {
		// document chkpnt_file_<n-1>
		for (s = 0; s < stripe_count; s++) {
		    rc = chmod(ChkpntStripePFS(n-1, s), 0444);
		    Check(rc == 0, "chmod(%s) failed",
					    ChkpntStripePFS(n-1, s), errno);
		}
	    }

	    if ((n-2) >= ChkpntStart) {
		// drop the now-superseded checkpoint from the SSD
		Print(&timer, "Deleting checkpoint %d.\n", n-2);
		for (s = 0; s < stripe_count; s++) {
		    rc = BSCFS_Forget(ChkpntStripe(n-2, s));
		    Check(rc == 0, "BSCFS_Forget(%s) failed",
						ChkpntStripe(n-2, s), rc);

		    if ((Rank == 0) && (!KeepAll)) {
			// delete superseded checkpoint
			rc = unlink(ChkpntStripePFS(n-2, s));
			Check(rc == 0, "unlink(%s) failed",
					    ChkpntStripePFS(n-2, s), errno);
			rc = unlink(MapfileStripe(n-2, s));
			Check(rc == 0, "unlink(%s) failed",
					    MapfileStripe(n-2, s), errno);
		    }
		}
	    }
	}

	Print(&timer, "Writing checkpoint %d.\n", n);
	struct timespec bw_timer = timer;
	pthread_t th[16384];
	for (s = 0; s < stripe_count; s++) {
	    rc = pthread_create(&th[s], NULL, WriteStripe, (void *) ((((uint64_t) n) << 32) + s));
	    Check(rc == 0, "pthread_create failed", NULL, 0);
	}
	for (s = 0; s < stripe_count; s++) {
	    rc = pthread_join(th[s], NULL);
	    Check(rc == 0, "pthread_join failed", NULL, 0);
	}

	Print(&timer, "Barrier.\n", 0);
	MPI_Barrier(MPI_COMM_WORLD);

	PrintBandwidth(&timer, &bw_timer);

	// start transferring the current checkpoint
	Print(NULL, "Starting checkpoint %d transfer.\n", n);
	for (s = 0; s < stripe_count; s++) {
	    rc = BSCFS_StartLocalFlush(ChkpntStripe(n, s), MapfileStripe(n, s),
				       Cleanup(), &handle[s]);
	    Check(rc == 0, "BSCFS_StartLocalFlush(%s) failed",
						    ChkpntStripe(n, s), rc);
	}
    }

    // final checkpoint will be documented by cleanup script on the FEN

    MPI_Finalize();
    Print(&timer, "Finished.\n", 0);
    return 0;
}
