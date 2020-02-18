/******************************************************************************
 |    chkpnt_common.h
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <libgen.h>
#include <time.h>
#include <mpi.h>

#include "bscfsAPI.h"

char *ProgName = NULL;
char *ProgDir = NULL;
int Rank = -1;
int RankCount = 0;
char BSCFS_MNT_PATH[256];
char BSCFS_PFS_PATH[256];
char *ChkpntDirSep = "";

uint64_t ChkpntCount = 1;
int64_t ChkpntStart = 1;
uint64_t PrimaryInterval = 1;
int KeepAll = 0;
int FillBuffers = 1;
char *ChkpntDir = "";
uint64_t ChkpntSize = 1 * (1024ul * 1024ul * 1024ul);
uint64_t StripeSize = -1ul;
uint64_t HeaderSize = 0;
uint64_t ChunkSize = 32 * (1024ul * 1024ul);
uint64_t ComputeTime = 0;

static inline uint64_t Min(uint64_t a, uint64_t b)
{
    return (a <= b) ? a : b;
}

void Usage(char *errmsg)
{
    char *optional_args, *description;

    if (strstr(ProgName, "chkpnt_write") != NULL) {
	optional_args = "        [--keep_all]\n";
	description =
	    "    Generate <chkpnt_count> checkpoints starting with checkpoint\n"
	    "    number <chkpnt_start>. Superseded checkpoints are deleted\n"
	    "    unless --keep_all is specified.\n";
    } else if (strstr(ProgName, "chkpnt_read") != NULL) {
	optional_args = "";
	description =
	    "    Read and verify <chkpnt_count> checkpoints starting with\n"
	    "    checkpoint number <chkpnt_start>.\n";
    } else if (strstr(ProgName, "chkpnt_combo") != NULL) {
	optional_args = "        [--primary_interval <number>\n";
	description =
	    "    Read and verify checkpoint number <chkpnt_start> (if\n"
	    "    <chkpnt_start> is greater than 0) and then generate\n"
	    "    <chkpnt_count> checkpoints starting with checkpoint number\n"
	    "    <chkpnt_start>+1. Checkpoints whose numbers equal 0 modulo\n"
	    "    <primary_interval> are proactively transferred to the PFS.\n"
	    "    All others are readied for transfer and remain ready until\n"
	    "    they are superseded. Those that exist when the job exits\n"
	    "    are transferred to the PFS during post-job stage-out.\n";
    } else {
	optional_args = "";
	description =
	    "    <description missing>\n";
    }

    if (errmsg != NULL) {
	fprintf(stderr, "%s: %s\n", ProgName, errmsg);
    }

    fprintf(stderr,
	    "Usage:\n"
            "    %s\n"
            "        [--help]\n"
            "        [--chkpnt_count <number>]\n"
            "        [--chkpnt_start <number>]\n"
	    "%s"
            "        [--chkpnt_dir <dir>]\n"
            "        [--chkpnt_size <number>[K|M|G]]\n"
            "        [--stripe_size <number>[K|M|G]]\n"
            "        [--header_size <number>[K|M|G]]\n"
            "        [--chunk_size <number>[K|M|G]]\n"
            "        [--compute_time <seconds>]\n"
	    "\n"
	    "%s"
	    "\n"
	    "    BSCFS_PFS_PATH and BSCFS_MNT_PATH must be set in the\n"
	    "    environment. The checkpoint directory (--chkpnt_dir) is\n"
	    "    interpreted relative to BSCFS_PFS_PATH and BSCFS_MNT_PATH.\n"
	    "    Checkpoint files are located in <chkpnt_dir> and have names\n"
	    "    of the form \"chkpnt_<n>\", where <n> is a 3-digit\n"
	    "    checkpoint number. The mapfile associated with a particular\n"
	    "    checkpoint has the same name but with a \".mapfile\" suffix.\n"
	    "\n"
	    "    A checkpoint consists of a header of size <header_size>\n"
	    "    (which may be zero) followed by checkpoint data of size\n"
	    "    <chkpnt_size>. The data is striped across all ranks in\n"
	    "    stripes of size <stripe_size>.  Checkpoint content is\n"
	    "    independent of the number of ranks. The checkpoint number\n"
	    "    <n> is written in the first word of the header and in the\n"
	    "    high-order byte of every word of the data. The rest of the\n"
	    "    bytes in each data word hold the file offset of the word.\n"
	    "    Files are written or read in units of <chunk_size> bytes.\n"
	    "\n"
	    "    The program will spin for <compute_time> seconds after\n"
	    "    reading or before writing a checkpoint.\n",
	    ProgName, optional_args, description);
}

void Check(int success, char *msg, char *arg, int errcode)
{
    if (!success) {
	char message[4096];
	snprintf(message, 4096, msg, arg);
	if (errcode > 0) {
	    fprintf(stderr, "%s (rank %d): %s: %s\n",
		    ProgName, Rank, message, strerror(errcode));
	} else {
	    fprintf(stderr, "%s (rank %d): %s\n",
		    ProgName, Rank, message);
	}
	exit(-1);
    }
}

char *Chkpnt(int n)
{
    static char space[BSCFS_PATH_MAX];
    snprintf(space, BSCFS_PATH_MAX, "%s%s%s/chkpnt_%03d",
	     BSCFS_MNT_PATH, ChkpntDirSep, ChkpntDir, n);
    return space;
}

char *ChkpntPFS(int n)
{
    static char space[BSCFS_PATH_MAX];
    snprintf(space, BSCFS_PATH_MAX, "%s%s%s/chkpnt_%03d",
	     BSCFS_PFS_PATH, ChkpntDirSep, ChkpntDir, n);
    return space;
}

char *Mapfile(int n)
{
    static char space[BSCFS_PATH_MAX];
    snprintf(space, BSCFS_PATH_MAX, "%s%s%s/chkpnt_%03d.mapfile",
	     BSCFS_PFS_PATH, ChkpntDirSep, ChkpntDir, n);
    return space;
}

char *Cleanup()
{
    static char space[BSCFS_PATH_MAX];
    snprintf(space, BSCFS_PATH_MAX, "%s/chkpnt_cleanup",
	     ProgDir);
    return space;
}

void PrintElapsed(struct timespec *timerp, struct timespec *nowp)
{
    if (timerp != NULL) {
	if (Rank == 0) {
	    double seconds = (((double) (nowp->tv_sec - timerp->tv_sec))
				    + (((double) nowp->tv_nsec) / 1e9))
					- (((double) timerp->tv_nsec) / 1e9);
	    printf("\t\t\t\t(%.6f seconds)\n", seconds);
	}
	(*timerp) = (*nowp);
    }
}

void Print(struct timespec *timerp, char *msg, uint64_t arg)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    PrintElapsed(timerp, &now);
    if (Rank == 0) {
	printf(msg, arg);
    }
}

void PrintBandwidth(struct timespec *timerp, struct timespec *startp)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    PrintElapsed(timerp, &now);
    if (Rank == 0) {
	double seconds = (((double) (now.tv_sec - startp->tv_sec))
				    + (((double) now.tv_nsec) / 1e9))
					- (((double) startp->tv_nsec) / 1e9);
	double gigabytes = ((double) (HeaderSize + ChkpntSize)) / 1e9;
	printf("\t\t[%.2f GB/s (aggregate)]\n", gigabytes / seconds);
    }
}

uint64_t MemSize(int rank)
{
    uint64_t seg_size = RankCount * StripeSize;
    uint64_t full_segs = ChkpntSize / seg_size;
    uint64_t seg_remainder = ChkpntSize % seg_size;
    uint64_t full_stripes = seg_remainder / StripeSize;
    uint64_t stripe_remainder = seg_remainder % StripeSize;
    uint64_t mem_size = (full_segs * StripeSize);
    if (rank < full_stripes) mem_size += StripeSize;
    if (rank == full_stripes) mem_size += stripe_remainder;
    return mem_size;
}

void chkpnt_init(int argc, char **argv)
{
    ProgName = argv[0];
    ProgDir = dirname(realpath(ProgName, NULL));

    enum {
	OPT_HELP,
	OPT_CHKPNT_COUNT,
	OPT_CHKPNT_START,
	OPT_KEEP_ALL,
	OPT_PRIMARY_INTERVAL,
	OPT_CHKPNT_DIR,
	OPT_CHKPNT_SIZE,
	OPT_STRIPE_SIZE,
	OPT_HEADER_SIZE,
	OPT_CHUNK_SIZE,
	OPT_COMPUTE_TIME,
	OPT_NOFILL_BUFFERS
    };

    static struct option options[] = {
	{"help",             0, NULL, OPT_HELP},
	{"chkpnt_count",     1, NULL, OPT_CHKPNT_COUNT},
	{"chkpnt_start",     1, NULL, OPT_CHKPNT_START},
	{"keep_all",         0, NULL, OPT_KEEP_ALL},
	{"primary_interval", 1, NULL, OPT_PRIMARY_INTERVAL},
	{"chkpnt_dir",       1, NULL, OPT_CHKPNT_DIR},
	{"chkpnt_size",     1, NULL, OPT_CHKPNT_SIZE},
	{"stripe_size",     1, NULL, OPT_STRIPE_SIZE},
	{"header_size",      1, NULL, OPT_HEADER_SIZE},
	{"chunk_size",       1, NULL, OPT_CHUNK_SIZE},
	{"compute_time",     1, NULL, OPT_COMPUTE_TIME},
	{"nofill",           0, NULL, OPT_NOFILL_BUFFERS},
	{0, 0, 0, 0}
    };

    int opt;
    char *p;
    while ((opt = getopt_long_only(argc, argv, "", options, NULL)) != -1) {

	switch (opt) {
	case OPT_CHKPNT_COUNT:
	    ChkpntCount = strtoul(optarg, &p, 0);
	    break;

	case OPT_CHKPNT_START:
	    ChkpntStart = strtoul(optarg, &p, 0);
	    break;

	case OPT_KEEP_ALL:
	    KeepAll = 1;
	    break;

	case OPT_NOFILL_BUFFERS:
		FillBuffers = 0;
		break;

	case OPT_PRIMARY_INTERVAL:
	    PrimaryInterval = strtoul(optarg, &p, 0);
	    break;

	case OPT_CHKPNT_DIR:
	    ChkpntDir = optarg;
	    break;

	case OPT_CHKPNT_SIZE:
	    ChkpntSize = strtoul(optarg, &p, 0);
	    switch (tolower(*p)) {
		case 'k': ChkpntSize <<= 10; break;
		case 'm': ChkpntSize <<= 20; break;
		case 'g': ChkpntSize <<= 30; break;
	    }
	    break;

	case OPT_STRIPE_SIZE:
	    StripeSize = strtoul(optarg, &p, 0);
	    switch (tolower(*p)) {
		case 'k': StripeSize <<= 10; break;
		case 'm': StripeSize <<= 20; break;
		case 'g': StripeSize <<= 30; break;
	    }
	    break;

	case OPT_HEADER_SIZE:
	    HeaderSize = strtoul(optarg, &p, 0);
	    switch (tolower(*p)) {
		case 'k': HeaderSize <<= 10; break;
		case 'm': HeaderSize <<= 20; break;
		case 'g': HeaderSize <<= 30; break;
	    }
	    break;

	case OPT_CHUNK_SIZE:
	    ChunkSize = strtoul(optarg, &p, 0);
	    switch (tolower(*p)) {
		case 'k': ChunkSize <<= 10; break;
		case 'm': ChunkSize <<= 20; break;
		case 'g': ChunkSize <<= 30; break;
	    }
	    break;

	case OPT_COMPUTE_TIME:
	    ComputeTime = strtoul(optarg, &p, 0);
	    break;

	case OPT_HELP:
	default:
	    Usage(NULL);
	    exit(-1);
	}
    }

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &Rank);
    MPI_Comm_size(MPI_COMM_WORLD, &RankCount);

	Check((BSCFS_GetParameter("BSCFS_MNT_PATH", sizeof(BSCFS_MNT_PATH), BSCFS_MNT_PATH) == 0), "BSCFS_GetParameter(%s) failed", "BSCFS_MNT_PATH", -1);
	Check((BSCFS_GetParameter("BSCFS_PFS_PATH", sizeof(BSCFS_PFS_PATH), BSCFS_PFS_PATH) == 0), "BSCFS_GetParameter(%s) failed", "BSCFS_PFS_PATH", -1);

    if (ChkpntDir[0] != '\0') {
	ChkpntDirSep = "/";
    }

    if (StripeSize == -1ul) {
	StripeSize = (((ChkpntSize + RankCount - 1) / RankCount) + 7) & ~7ul;
    }

    Check((ChkpntSize & 7) == 0, "%s not 8-byte aligned", "<chkpnt_size>", -1);
    Check((StripeSize & 7) == 0, "%s not 8-byte aligned", "<stripe_size>", -1);
    Check((HeaderSize & 7) == 0, "%s not 8-byte aligned", "<header_size>", -1);

    if (Rank == 0) {
	printf("%s (rank %d):\n", ProgName, Rank);
	printf("    rank_count %d\n", RankCount);
	printf("    BSCFS_MNT_PATH \"%s\"\n", BSCFS_MNT_PATH);
	printf("    BSCFS_PFS_PATH \"%s\"\n", BSCFS_PFS_PATH);
	printf("    chkpnt_count %ld\n", ChkpntCount);
	printf("    chkpnt_start %ld\n", ChkpntStart);
	if (strstr(ProgName, "chkpnt_write") != NULL) {
	    printf("    keep_all %d\n", KeepAll);
	}
	if (strstr(ProgName, "chkpnt_combo") != NULL) {
	    printf("    primary_interval %ld\n", PrimaryInterval);
	}
	printf("    chkpnt_dir \"%s\"\n", ChkpntDir);
	printf("    chkpnt_size 0x%lx bytes\n", ChkpntSize);
	printf("    stripe_size 0x%lx bytes\n", StripeSize);
	printf("    header_size 0x%lx bytes\n", HeaderSize);
	printf("    chunk_size 0x%lx bytes\n", ChunkSize);
	printf("    compute_time %ld seconds\n", ComputeTime);
    }
}
