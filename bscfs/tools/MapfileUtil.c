/*******************************************************************************
 |    MapfileUtil.c
 |
 |  © Copyright IBM Corporation 2015,2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <endian.h>
#include <string.h>
#include <unistd.h>

#include "bscfs_mapfile.h"
#include "MapfileUtil.h"

typedef bscfs_mapfile_region_t RegionInfo;

typedef struct {
    RegionInfo *region;
    uint64_t region_count_max;
    uint64_t region_count;
    uint64_t total_data_size;
} NodeInfo;

static char *ProgName = NULL;

static uint64_t NodeCount = 0;
static NodeInfo Node[NODE_COUNT_MAX];

static void Clear()
{
    uint64_t n;
    for (n = 0; n < NodeCount; n++) {
	if (Node[n].region != NULL) {
	    free(Node[n].region);
	}
    }
    for (n = 0; n < NODE_COUNT_MAX; n++) {
	Node[n].region = NULL;
	Node[n].region_count_max = 0;
	Node[n].region_count = 0;
	Node[n].total_data_size = 0;
    }
    NodeCount = 0;
}

void MapfileInit(char *program)
{
    // ProgName is used in error messages
    ProgName = program;
    Clear();
}

void MapfileLoad(char *mapfile, int summary)
{
    ssize_t rc;

    Clear();

    int map = open(mapfile, O_RDONLY, 0);
    if (map < 0) {
	fprintf(stderr, "%s: open(\"%s\") failed: %s\n",
		ProgName, mapfile, strerror(errno));
	exit(-1);
    }

    bscfs_mapfile_header_t file_header;
    rc = read(map, &file_header, sizeof(file_header));
    if (rc != sizeof(file_header)) {
	fprintf(stderr, "%s: read(bscfs_mapfile_header) failed: %s\n",
		ProgName, strerror(errno));
	exit(-1);
    }

    NodeCount = le64toh(file_header.node_count);
    if (NodeCount > NODE_COUNT_MAX) {
	fprintf(stderr, "%s: node_count (%ld) too large\n",
		ProgName, NodeCount);
	exit(-1);
    }

    uint64_t node_header_size =
	NodeCount * sizeof(bscfs_mapfile_node_header_t);
    bscfs_mapfile_node_header_t *node_header =
	(bscfs_mapfile_node_header_t *) malloc(node_header_size);
    if (node_header == NULL) {
    	fprintf(stderr, "%s: malloc(bscfs_mapfile_node_header array) failed\n",
		ProgName);
	exit(-1);
    }
    rc = read(map, node_header, node_header_size);
    if (rc != node_header_size) {
	fprintf(stderr,
		"%s: read(bscfs_mapfile_node_header array) failed: %s\n",
		ProgName, strerror(errno));
	exit(-1);
    }

    off_t file_size = lseek(map, 0, SEEK_END);
    if (file_size < 0) {
	fprintf(stderr, "%s: lseek(\"%s\") failed: %s\n",
		ProgName, mapfile, strerror(errno));
	exit(-1);
    }

    uint64_t n;
    for (n = 0; n < NodeCount; n++) {
	off_t offset = le64toh(node_header[n].region_offset);
	uint64_t count = le64toh(node_header[n].region_count);
	uint64_t data_size = le64toh(node_header[n].total_data_size);
	Node[n].region_count = count;
	Node[n].total_data_size = data_size;
	if (summary || (count == 0)) {
	    Node[n].region_count_max = 0;
	    Node[n].region = NULL;
	} else {
	    if ((offset < (sizeof(file_header) + node_header_size)) ||
		(count >= ((file_size - offset) / sizeof(RegionInfo))))
	    {
		fprintf(stderr,
			"%s: bad region_offset (%ld) or region_count (%ld) "
			"for node %ld\n",
			ProgName, offset, count, n);
		exit(-1);
	    }
	    Node[n].region_count_max = count;
	    uint64_t region_size = count * sizeof(RegionInfo);
	    Node[n].region = (RegionInfo *) malloc(region_size);
	    if (Node[n].region == NULL) {
		fprintf(stderr,
			"%s: malloc(region array for node %ld) failed\n",
			ProgName, n);
		exit(-1);
	    }
	    uint64_t bytes = 0;
	    while (bytes < region_size) {
		rc = pread(map, ((void *) Node[n].region) + bytes,
			   region_size - bytes, offset + bytes);
		if (rc < 0) {
		    fprintf(stderr,
			    "%s: read(region array for node %ld) failed: %s\n",
			    ProgName, n, strerror(errno));
		    exit(-1);
		}
		bytes += rc;
	    }
	    // convert the region info to host endianess in place, if necessary
	    if (le64toh(0x0123456789abcdefull) != 0x0123456789abcdefull) {
		uint64_t r;
		for (r = 0; r < Node[n].region_count; r++) {
		    RegionInfo *reg = &Node[n].region[r];
		    reg->offset = le64toh(reg->offset);
		    reg->length = le64toh(reg->length);
		}
	    }
	}
    }

    free(node_header);
    close(map);
}

static int CmpRegionInfo(const void *r1, const void *r2)
{
    RegionInfo *reg1 = (RegionInfo *) r1;
    RegionInfo *reg2 = (RegionInfo *) r2;

    return (reg1->offset < reg2->offset) ? -1 :
	    ((reg1->offset > reg2->offset) ? 1 : 0);
}

void MapfileSave(char *mapfile)
{
    ssize_t rc;

    int map = open(mapfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (map < 0) {
	fprintf(stderr, "%s: open(\"%s\") failed: %s\n",
		ProgName, mapfile, strerror(errno));
	exit(-1);
    }

    uint64_t n;
    for (n = 0; n < NodeCount; n++) {
	uint64_t count = Node[n].region_count;
	uint64_t total = 0;
	if (count > 0) {
	    // merge adjacent and overlapping regions
	    RegionInfo *region = Node[n].region;
	    if (region == NULL) {
		// this could happen if MapfileLoad was used in "summary" mode
		fprintf(stderr, "%s: region info was not loaded\n", ProgName);
		exit(-1);
	    }
	    // sort the regions into file-offset order
	    qsort(region, count, sizeof(RegionInfo), CmpRegionInfo);
	    // next indicates where the next maximal region goes in the array
	    uint64_t next = 0;
	    // start and end delimit the current candidate maximal region
	    uint64_t start = region[0].offset;
	    uint64_t end = start + region[0].length;
	    uint64_t r;
	    for (r = 1; r < count; r++) {
		if (region[r].offset <= end) {
		    // region[r] overlaps current candidate
		    if ((region[r].offset + region[r].length) > end) {
			// extend the candidate to cover region[r]
			end = region[r].offset + region[r].length;
		    }
		} else {
		    // region[r] is disjoint from the current candidate
		    // save the current candidate and tally its size
		    region[next].offset = start;
		    region[next].length = end - start;
		    total += (end - start);
		    next++;
		    // start a new candidate maximal region with region[r]
		    start = region[r].offset;
		    end = start + region[r].length;
		}
	    }
	    // save the final maximal region and tally its size
	    region[next].offset = start;
	    region[next].length = end - start;
	    total += (end - start);
	    next++;
	    // update the region count
	    count = next;
	}
	// update node header information
	Node[n].region_count = count;
	Node[n].total_data_size = total;
    }

    // allocate a file header that includes the array of node headers
    uint64_t header_size =
	sizeof(bscfs_mapfile_header_t) +
	    (NodeCount * sizeof(bscfs_mapfile_node_header_t));
    bscfs_mapfile_header_t *header =
	(bscfs_mapfile_header_t *) malloc(header_size);
    if (header == NULL) {
    	fprintf(stderr, "%s: malloc(bscfs_mapfile_header) failed\n", ProgName);
	exit(-1);
    }
    header->node_count = htole64(NodeCount);
    // per-node region info segments will be written following the header
    uint64_t region_offset = header_size;
    for (n = 0; n < NodeCount; n++) {
	header->node[n].region_count = htole64(Node[n].region_count);
	header->node[n].total_data_size = htole64(Node[n].total_data_size);

	if (Node[n].region_count == 0) {
	    header->node[n].region_offset = htole64(0);
	} else {
	    header->node[n].region_offset = htole64(region_offset);
	    uint64_t region_size = Node[n].region_count * sizeof(RegionInfo);
	    // convert the region info to little endian in place, if necessary
	    if (htole64(0x0123456789abcdefull) != 0x0123456789abcdefull) {
		uint64_t r;
		for (r = 0; r < Node[n].region_count; r++) {
		    RegionInfo *reg = &Node[n].region[r];
		    reg->offset = htole64(reg->offset);
		    reg->length = htole64(reg->length);
		}
	    }
	    rc = pwrite(map, Node[n].region, region_size, region_offset);
	    if (rc != region_size) {
		fprintf(stderr,
			"%s: write(region array for node %ld) failed: %s\n",
			ProgName, n, strerror(errno));
		exit(-1);
	    }
	    region_offset += region_size;
	    // switch back to host endianess in case the caller continues
	    //     to develop the loaded mapfile
	    if (le64toh(0x0123456789abcdefull) != 0x0123456789abcdefull) {
		uint64_t r;
		for (r = 0; r < Node[n].region_count; r++) {
		    RegionInfo *reg = &Node[n].region[r];
		    reg->offset = le64toh(reg->offset);
		    reg->length = le64toh(reg->length);
		}
	    }
	}
    }

    rc = pwrite(map, header, header_size, 0);
    if (rc != header_size) {
	fprintf(stderr, "%s: write(file header) failed: %s\n",
		ProgName, strerror(errno));
	exit(-1);
    }

    free(header);
    close(map);
}

void MapfileSetNodeCount(uint64_t node_count)
{
    if (node_count > NODE_COUNT_MAX) {
	fprintf(stderr, "%s: <node_count> %ld too large\n",
		ProgName, node_count);
	exit(-1);
    }
    if (node_count < NodeCount) {
	fprintf(stderr, "%s: WARNING: reducing node_count from %ld to %ld\n",
		ProgName, NodeCount, node_count);
    }
    NodeCount = node_count;
}

uint64_t MapfileNodeCount()
{
    return NodeCount;
}

uint64_t MapfileRegionCount(uint64_t node)
{
    if (node >= NodeCount) {
	fprintf(stderr, "%s: node %ld out of range\n", ProgName, node);
	exit(-1);
    }
    return Node[node].region_count;
}

uint64_t MapfileTotalDataSize(uint64_t node)
{
    if (node >= NodeCount) {
	fprintf(stderr, "%s: node %ld out of range\n", ProgName, node);
	exit(-1);
    }
    return Node[node].total_data_size;
}

void MapfileGetRegion(uint64_t node, uint64_t reg,
		      uint64_t *offset_p, uint64_t *length_p)
{
    if (node >= NodeCount) {
	fprintf(stderr, "%s: node %ld out of range\n", ProgName, node);
	exit(-1);
    }
    if (reg >= Node[node].region_count) {
	fprintf(stderr, "%s: reg %ld out of range for node %ld\n",
		ProgName, reg, node);
	exit(-1);
    }
    if (Node[node].region == NULL) {
	fprintf(stderr, "%s: region information not loaded\n", ProgName);
	exit(-1);
    }
    (*offset_p) = Node[node].region[reg].offset;
    (*length_p) = Node[node].region[reg].length;
}

void MapfileAddRegion(uint64_t node, uint64_t offset, uint64_t length)
{
    if (node >= NodeCount) {
	if (node >= NODE_COUNT_MAX) {
	    fprintf(stderr, "%s: node %ld too large\n", ProgName, node);
	    exit(-1);
	}
	NodeCount = node + 1;
    }

    if (Node[node].region_count >= Node[node].region_count_max) {
	// enlarge the region array to make room for a new region
	if (Node[node].region_count_max < 1024) {
	    Node[node].region_count_max = 1024;
	} else {
	    Node[node].region_count_max *= 2;
	}
	Node[node].region = (RegionInfo *)
	    realloc(Node[node].region,
		    Node[node].region_count_max * sizeof(RegionInfo));
	if (Node[node].region == NULL) {
	    fprintf(stderr, "%s: realloc(region array for node %ld) failed\n",
		    ProgName, node);
	    exit(-1);
	}
    }

    Node[node].region[Node[node].region_count].offset = offset;
    Node[node].region[Node[node].region_count].length = length;
    Node[node].region_count++;
    Node[node].total_data_size += length;
}
