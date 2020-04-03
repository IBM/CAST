/******************************************************************************
 |    chkpnt_write.c
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

    void* memory = NULL;
	void* header = NULL;
    rc = posix_memalign(&memory, 0x10000, mem_size);
    Check(rc == 0, "posix_memalign(memory) failed", NULL, rc);
    header = NULL;
    if (HeaderSize > 0) {
	rc = posix_memalign(&header, 0x10000, HeaderSize);
	Check(rc == 0, "posix_memalign(header) failed", NULL, rc);
    }

    BBTransferHandle_t handle = -1;

    int n;
    for (n = ChkpntStart; n < (ChkpntStart + ChkpntCount); n++) {
	void *mem_start;
	uint64_t file_start, stripe_size, size, offset, delta, i;

	// "compute" for the specified number of seconds
	Print(&timer, "Starting computation round %d.\n", n);
	struct timespec time0, time1;
	clock_gettime(CLOCK_REALTIME, &time0);
	// fill memory stripes as part of the computation
	for (i = 0; i < stripe_count; i++) {
	    mem_start = memory + (i * StripeSize);
	    file_start = HeaderSize + (((i * RankCount) + Rank) * StripeSize);
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
	    rc = BSCFS_AwaitLocalTransfer(handle);
	    Check(rc == 0, "BSCFS_AwaitLocalTransfer(%s) failed",
							Chkpnt(n-1), rc);
	}

	Print(&timer, "Barrier.\n", 0);
	MPI_Barrier(MPI_COMM_WORLD);

	if ((n-1) >= ChkpntStart) {
	    // the previous checkpoint has been transferred by all nodes
	    Print(&timer, "Finalizing checkpoint %d.\n", n-1);
	    rc = BSCFS_GlobalFlushCompleted(Chkpnt(n-1));
	    Check(rc == 0, "BSCFS_GlobalFlushCompleted(%s) failed",
							Chkpnt(n-1), rc);

	    if (Rank == 0) {
		// document chkpnt_file_<n-1>
		rc = chmod(ChkpntPFS(n-1), 0444);
		Check(rc == 0, "chmod(%s) failed", ChkpntPFS(n-1), errno);
	    }

	    if ((n-2) >= ChkpntStart) {
		// drop the now-superseded checkpoint from the SSD
		Print(&timer, "Deleting checkpoint %d.\n", n-2);
		rc = BSCFS_Forget(Chkpnt(n-2));
		Check(rc == 0, "BSCFS_Forget(%s) failed", Chkpnt(n-2), rc);

		if ((Rank == 0) && (!KeepAll)) {
		    // delete superseded checkpoint
		    rc = unlink(ChkpntPFS(n-2));
		    Check(rc == 0, "unlink(%s) failed", ChkpntPFS(n-2), errno);
		    rc = unlink(Mapfile(n-2));
		    Check(rc == 0, "unlink(%s) failed", Mapfile(n-2), errno);
		}
	    }
	}

	Print(&timer, "Writing checkpoint %d.\n", n);
	struct timespec bw_timer = timer;
	int fd = open(Chkpnt(n), O_WRONLY|O_CREAT, 0640);
	Check(fd >= 0, "open(%s) failed", Chkpnt(n), errno);
	if ((Rank == 0) && (HeaderSize > 0)) {
	    rc = pwrite(fd, header, HeaderSize, 0);
	    Check(rc == HeaderSize, "pwrite(%s header) failed", Chkpnt(n), 0);
	}
	for (i = 0; i < stripe_count; i++) {
	    mem_start = memory + (i * StripeSize);
	    file_start = HeaderSize + (((i * RankCount) + Rank) * StripeSize);
	    stripe_size = Min(StripeSize, mem_size - ((uint64_t) mem_start));
	    for (offset = 0; offset < stripe_size; offset += ChunkSize) {
		size = Min(ChunkSize, stripe_size - offset);
		rc = pwrite(fd, mem_start + offset, size, file_start + offset);
		Check(rc >= 0, "pwrite(%s) failed", Chkpnt(n), errno);
		Check(rc == size, "pwrite(%s) truncated", Chkpnt(n), 0);
	    }
	}
	rc = fsync(fd);
	Check(rc == 0, "fsync(%s) failed", Chkpnt(n), errno);
	rc = close(fd);
	Check(rc == 0, "close(%s) failed", Chkpnt(n), errno);

	Print(&timer, "Barrier.\n", 0);
	MPI_Barrier(MPI_COMM_WORLD);

	PrintBandwidth(&timer, &bw_timer);

	// start transferring the current checkpoint
	Print(NULL, "Starting checkpoint %d transfer.\n", n);
	rc = BSCFS_StartLocalFlush(Chkpnt(n), Mapfile(n), Cleanup(), &handle);
	Check(rc == 0, "BSCFS_StartLocalFlush(%s) failed", Chkpnt(n), rc);
    }

    // final checkpoint will be documented by cleanup script on the FEN

    MPI_Finalize();
    Print(&timer, "Finished.\n", 0);
    return 0;
}
