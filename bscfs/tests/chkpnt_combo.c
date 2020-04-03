/******************************************************************************
 |    chkpnt_combo.c
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

    int64_t K = PrimaryInterval;
    int n;
    void *mem_start;
    uint64_t file_start, stripe_size, size, offset, delta, i;

    if (ChkpntStart > 0) {
	n = ChkpntStart;
	Print(&timer, "Reading checkpoint %d.\n", n);
	struct timespec bw_timer = timer;
	int fd = open(Chkpnt(n), O_RDONLY, 0);
	Check(fd >= 0, "open(%s) failed", Chkpnt(n), errno);
	if (HeaderSize > 0) {
	    rc = pread(fd, header, HeaderSize, 0);
	    Check(rc == HeaderSize, "pread(%s header) failed", Chkpnt(n), 0);
	}
	for (i = 0; i < stripe_count; i++) {
	    mem_start = memory + (i * StripeSize);
	    file_start = HeaderSize + (((i * RankCount) + Rank) * StripeSize);
	    stripe_size = Min(StripeSize, mem_size - ((uint64_t) mem_start));
	    for (offset = 0; offset < stripe_size; offset += ChunkSize) {
		size = Min(ChunkSize, stripe_size - offset);
		rc = pread(fd, mem_start + offset, size, file_start + offset);
		Check(rc >= 0, "pread(%s) failed", Chkpnt(n), errno);
		Check(rc == size, "pread(%s) truncated", Chkpnt(n), 0);
	    }
	}
	rc = close(fd);
	Check(rc == 0, "close(%s) failed", Chkpnt(n), errno);

	Print(&timer, "Barrier.\n", 0);
	MPI_Barrier(MPI_COMM_WORLD);

	PrintBandwidth(&timer, &bw_timer);

	// checkpoint no longer needed; drop it from the SSD
	Print(NULL, "Clearing checkpoint %d from the SSD.\n", n);
	rc = BSCFS_Forget(Chkpnt(n));
	Check(rc == 0, "BSCFS_Forget(%s) failed", Chkpnt(n), rc);

	// validate the ingested checkpoint
	Print(&timer, "Validating checkpoint %d.\n", n);
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
    }

    ChkpntStart++;
    BBTransferHandle_t handle = -1;

    for (n = ChkpntStart; n < (ChkpntStart + ChkpntCount); n++) {

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

        if (((n % K) == 0) && ((n-K) >= ChkpntStart)) {
            // wait for transfer of previous primary chkpnt to complete
	    Print(&timer, "Awaiting checkpoint %d local transfer.\n", n-K);
	    rc = BSCFS_AwaitLocalTransfer(handle);
	    Check(rc == 0, "BSCFS_AwaitLocalTransfer(%s) failed",
							Chkpnt(n-K), rc);
	}

	Print(&timer, "Barrier.\n", 0);
	MPI_Barrier(MPI_COMM_WORLD);

        if (((n % K) == 0) && ((n-K) >= ChkpntStart)) {
            // previous primary chkpnt has been transferred by all nodes
	    Print(&timer, "Finalizing checkpoint %d.\n", n-K);
	    rc = BSCFS_GlobalFlushCompleted(Chkpnt(n-K));
	    Check(rc == 0, "BSCFS_GlobalFlushCompleted(%s) failed",
							Chkpnt(n-K), rc);

	    if (Rank == 0) {
		// document chkpnt_file_<n-K>
		rc = chmod(ChkpntPFS(n-K), 0444);
		Check(rc == 0, "chmod(%s) failed", ChkpntPFS(n-K), errno);
	    }

            // drop the now-transferred primary chkpnt from the SSD
	    Print(&timer, "Clearing checkpoint %d from the SSD.\n", n-K);
	    rc = BSCFS_Forget(Chkpnt(n-K));
	    Check(rc == 0, "BSCFS_Forget(%s) failed", Chkpnt(n-K), rc);
        }

        if (((n % K) != 2) && ((n-2) >= ChkpntStart)) {
            // drop the now-superseded intermediate chkpnt from the SSD
	    Print(&timer, "Clearing checkpoint %d from the SSD.\n", n-2);
	    rc = BSCFS_Forget(Chkpnt(n-2));
	    Check(rc == 0, "BSCFS_Forget(%s) failed", Chkpnt(n-2), rc);

	    if (Rank == 0) {
		// delete superseded checkpoint; it should be empty
		rc = unlink(ChkpntPFS(n-2));
		Check(rc == 0, "unlink(%s) failed", ChkpntPFS(n-2), errno);
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

	// start, or prepare for, transferring the current checkpoint
        if ((n % K) == 0) {
            // primary chkpnt: start transferring
	    Print(NULL, "Starting checkpoint %d transfer.\n", n);
	    rc = BSCFS_StartLocalFlush(Chkpnt(n), Mapfile(n), Cleanup(),
				       &handle);
	    Check(rc == 0, "BSCFS_StartLocalFlush(%s) failed", Chkpnt(n), rc);
        } else {
            // intermediate chkpnt: prepare for possible post-job retrieval
	    Print(NULL, "Preparing checkpoint %d for transfer.\n", n);
	    rc = BSCFS_PrepareLocalFlush(Chkpnt(n), Mapfile(n), Cleanup());
	    Check(rc == 0, "BSCFS_PrepareLocalFlush(%s) failed", Chkpnt(n), rc);
        }
    }

    // final primary chkpnt will be documented by cleanup script on the FEN
    // final intermediate chkpnts will also be transferred and documented

    MPI_Finalize();
    Print(&timer, "Finished.\n", 0);
    return 0;
}
