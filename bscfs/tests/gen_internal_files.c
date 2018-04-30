/******************************************************************************
 |    gen_internal_files.c
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
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>

#include "bscfs_index.h"
#include "../tools/MapfileUtil.h"

char *ProgName = NULL;

#define BUFFER_SIZE 0x100000
char Buffer[BUFFER_SIZE];

void Usage(char *errmsg)
{
    if (errmsg != NULL) {
	fprintf(stderr, "%s: %s\n", ProgName, errmsg);
    }
    fprintf(stderr,
	    "Usage:\n"
            "    %s\n"
            "        [--help]\n"
            "        --shared_file <path>\n"
            "        --map_file <path>\n"
            "        --index_file <path>\n"
            "        --data_file <path>\n"
            "        [--node <number>]\n",
	    ProgName);
}

void Check(int success, char *operation, int errcode)
{
    if (!success) {
	if (errcode >= 0) {
	    fprintf(stderr, "%s: %s failed: %s\n",
		    ProgName, operation, strerror(errcode));
	} else {
	    fprintf(stderr, "%s: %s failed\n",
		    ProgName, operation);
	}
	exit(-1);
    }
}

void Copy(int shared_fd, uint64_t sf_offset, uint64_t length, FILE *data_stream)
{
    uint64_t copied = 0;
    while (copied < length) {
	uint64_t chunk = length - copied;
	if (chunk > BUFFER_SIZE) chunk = BUFFER_SIZE;
	ssize_t bytes;
	bytes = pread(shared_fd, Buffer, chunk, sf_offset + copied);
	Check(bytes == chunk, "pread(shared_fd)", (bytes < 0) ? errno : -1);
	bytes = fwrite(Buffer, 1, chunk, data_stream);
	Check(bytes == chunk, "fwrite(data_stream)", ferror(data_stream));
	copied += chunk;
    }
}

// Copied from bscfs/src/bscfs_index.cc
void bscfs_index_convert_host_to_little_endian(bscfs_index_t *index)
{
    if (htole64(0x0123456789abcdefull) == 0x0123456789abcdefull) return;

    uint64_t i;
    for (i = 0; i < index->mapping_count; i++) {
	bscfs_mapping_t *m = &index->mapping[i];
	m->sf_offset = htole64(m->sf_offset);
	m->df_offset = htole64(m->df_offset);
	m->length = htole64(m->length);
    }

    index->finalized = htole32(index->finalized);
    index->normalized = htole32(index->normalized);
    index->mapping_count = htole64(index->mapping_count);
}

int main(int argc, char **argv)
{
    ProgName = argv[0];

    enum {
	OPT_HELP,
	OPT_SHARED_FILE,
	OPT_MAP_FILE,
	OPT_INDEX_FILE,
	OPT_DATA_FILE,
	OPT_NODE,
    };

    static struct option options[] = {
	{"help",        0, NULL, OPT_HELP},
	{"shared_file", 1, NULL, OPT_SHARED_FILE},
	{"map_file",    1, NULL, OPT_MAP_FILE},
	{"index_file",  1, NULL, OPT_INDEX_FILE},
	{"data_file",   1, NULL, OPT_DATA_FILE},
	{"node",        1, NULL, OPT_NODE},
	{0, 0, 0, 0}
    };

    char *shared_file = NULL;
    char *map_file = NULL;
    char *index_file = NULL;
    char *data_file = NULL;
    uint64_t node = 0;

    int opt;
    while ((opt = getopt_long_only(argc, argv, "", options, NULL)) != -1) {

	switch (opt) {
	case OPT_SHARED_FILE:
	    shared_file = optarg;
	    break;

	case OPT_MAP_FILE:
	    map_file = optarg;
	    break;

	case OPT_INDEX_FILE:
	    index_file = optarg;
	    break;

	case OPT_DATA_FILE:
	    data_file = optarg;
	    break;

	case OPT_NODE:
	    node = strtoul(optarg, NULL, 0);
	    break;

	case OPT_HELP:
	default:
	    Usage(NULL);
	    exit(-1);
	}
    }

    Check(shared_file != NULL, "shared_file param check", -1);
    Check(map_file != NULL, "map_file param check", -1);
    Check(index_file != NULL, "index_file param check", -1);
    Check(data_file != NULL, "data_file param check", -1);

    printf("%s:\n", ProgName);
    printf("    shared_file         \"%s\"\n", shared_file);
    printf("    map_file            \"%s\"\n", map_file);
    printf("    index_file          \"%s\"\n", index_file);
    printf("    data_file           \"%s\"\n", data_file);
    printf("    node                %ld\n", node);

    MapfileInit(ProgName);

    MapfileLoad(map_file, 0);
    Check(node < MapfileNodeCount(), "node range check", -1);

    int shared_fd = open(shared_file, O_RDONLY, 0);
    Check(shared_fd >= 0, "open(shared_file)", errno);

    int index_fd = open(index_file, O_WRONLY|O_CREAT|O_TRUNC, 0644);
    Check(index_fd >= 0, "open(index_file)", errno);

    FILE *data_stream = fopen(data_file, "w");
    Check(data_stream != NULL, "fopen(data_file)", errno);

    uint64_t regcnt = MapfileRegionCount(node);
    uint64_t index_size = BSCFS_INDEX_SIZE(regcnt);

    bscfs_index_t *index = (bscfs_index_t *) malloc(index_size);
    Check(index != NULL, "malloc(index_size)", -1);

    index->mapping_count = regcnt;
    index->normalized = 1;
    index->finalized = 1;

    uint64_t df_offset = 0;
    uint64_t r;
    for (r = 0; r < regcnt; r++) {
	uint64_t offset, length;
	MapfileGetRegion(node, r, &offset, &length);
	Copy(shared_fd, offset, length, data_stream);
	index->mapping[r].sf_offset = offset;
	index->mapping[r].df_offset = df_offset;
	index->mapping[r].length = length;
	df_offset += length;
    }

    Check(df_offset == MapfileTotalDataSize(node), "data file size check", -1);

    bscfs_index_convert_host_to_little_endian(index);

    uint64_t written = 0;
    while (written < index_size) {
	ssize_t bytes = pwrite(index_fd, (void *)(((char *) index) + written),
			       index_size - written, written);
	Check(bytes >= 0, "pwrite(index_fd)", errno);
	written += bytes;
    }

    fclose(data_stream);
    close(index_fd);
    close(shared_fd);

    return 0;
}
