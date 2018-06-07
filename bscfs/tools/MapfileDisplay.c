/*******************************************************************************
 |    MapfileDisplay.c
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
#include <stdint.h>
#include <getopt.h>
#include <ctype.h>

#include "MapfileUtil.h"

char *ProgName = NULL;

void Usage(char *errmsg)
{
    if (errmsg != NULL) {
	fprintf(stderr, "%s: %s\n", ProgName, errmsg);
    }
    fprintf(stderr,
	    "Usage:\n"
	    "    %s\n"
    	    "        [--help]\n"
    	    "        [[--by_offset] [--range <offset_range>] [--cmdfile]]\n"
    	    "        [--by_node [--nodes <node_set>] [--summary]]\n"
    	    "        <mapfile>\n",
	    ProgName);
}

typedef struct {
    uint64_t offset;
    uint64_t length : 48;
    uint64_t node : 16;
} Region;

int CmpRegion(const void *r1, const void *r2)
{
    Region *reg1 = (Region *) r1;
    Region *reg2 = (Region *) r2;

    return (reg1->offset < reg2->offset) ? -1 :
	    ((reg1->offset > reg2->offset) ? 1 : 0);
}

void PrintList(const int *list, int count)
{
    int i, j;
    i = 0;
    while (i < count) {
	// find the end of any consecutive sequence
	for (j = i+1; (j < count) && ((list[j] - list[i]) == (j - i)); j++);
	if (i != 0) printf(",");
	printf("%d", list[i]);
	if ((j - 1) > i) printf("-%d", list[j-1]);
	i = j;
    }
}

void DisplayByOffset(char *mapfile, char *range, int cmdfile)
{
    int64_t n, i, j, k, keep;
    uint64_t offset, length;

    uint64_t range_start = 0;
    uint64_t range_end = UINT64_MAX;
    if (range != NULL) {
	char *p;
	range_start = strtoul(range, &p, 0);
	while (isspace(*p)) p++;
	if ((*p) != ',') {
	    fprintf(stderr, "%s: bad range (\"%s\"): expected comma\n",
		    ProgName, range);
	    exit(-1);
	}
	p++;
	range_end = strtoul(p, NULL, 0);
    }

    MapfileLoad(mapfile, 0);
    uint64_t node_count = MapfileNodeCount();

    // allocate enough space to hold all regions from all nodes
    uint64_t total_region_count = 0;
    for (n = 0; n < node_count; n++) {
	total_region_count += MapfileRegionCount(n);
    }
    Region *region = (Region *) malloc(total_region_count * sizeof(Region));
    if (region == NULL) {
	fprintf(stderr, "%s: could not allocate region space\n", ProgName);
	exit(-1);
    }

    // extract all regions from all nodes, keeping only those that are in range
    Region *reg = region;
    for (n = 0; n < node_count; n++) {
	uint64_t region_count = MapfileRegionCount(n);
	for (i = 0; i < region_count; i++) {
	    MapfileGetRegion(n, i, &offset, &length);
	    if (((offset + length) > range_start) && (offset < range_end)) {
		reg->offset = offset;
		reg->length = length;
		reg->node = n;
		reg++;
	    }
	}
    }
    // reset total_region_count to the number of in-range regions
    total_region_count = reg - region;

    // sort the regions into increasing file-offset order
    qsort(region, total_region_count, sizeof(Region), CmpRegion);

    // tmp array to holde the nodes associated with a particular offset
    static int node[NODE_COUNT_MAX];
    uint64_t node_cnt;

    // end of the previous displayed range -- used to detect and display
    // gaps between ranges
    uint64_t prev_end = range_start;

    // In --cmdfile mode we try to identify sequences of ranges that can be
    // covered by a single command. These variables accumulate ranges into
    // a growing command.
    uint64_t cmd_count = 0;
    uint64_t cmd_offset = 0;
    uint64_t cmd_length = 0;
    uint64_t cmd_node_cnt = 0;
    static int cmd_node[NODE_COUNT_MAX];

    i = 0;
    while (i < total_region_count) {
	// Region[i].offset is the next offset to be displayed.
	// Search forward, collecting the nodes that are associated with that
	// offset. Also identify the smallest region that has that offset and
	// the first region that has a larger offset.
	j = i;
	node_cnt = 0;
	node[node_cnt++] = region[i].node;
	for (k = i+1; (k < total_region_count) &&
			(region[k].offset == region[i].offset); k++) {
	    if (node_cnt >= node_count) {
		fprintf(stderr, "%s; too many nodes at file offset 0x%lx\n",
			ProgName, region[i].offset);
		exit(-1);
	    }
	    node[node_cnt++] = region[k].node;
	    if (region[k].length < region[j].length) j = k;
	}
	
	// At this point, region[j] is smallest region that has the same offset
	// as region[i]. Region[k] is the first region with a larger offset.
	// The length of the current region is determined either by the end
	// of region[j] or by the beginning of region[k].
	offset = region[j].offset;
	length = region[j].length;
	if ((k < total_region_count) &&
			    ((region[k].offset - offset) < length)) {
	    length = region[k].offset - offset;
	}

	// Walk backwards through all the regions that start at the current
	// offset, keeping any that extend beyond the current length. Any
	// region we keep we trim forward so that it starts where the
	// current region ends.
	keep = k;
	for (j = k-1; j >= i; j--) {
	    if (region[j].length > length) {
		keep--;
		region[keep].offset = region[j].offset + length;
		region[keep].length = region[j].length - length;
		region[keep].node = region[j].node;
	    }
	}

	// The next region will start with the regions we kept (if any) or with
	// region[k].
	i = keep;

	// Sort the nodes associated with this offset. A simple insertion sort
	// should be tolerable here.
	for (j = 1; j < node_cnt; j++) {
	    int t = node[j];
	    for (k = j; (k > 0) && (node[k-1] > t); k--) {
		node[k] = node[k-1];
	    }
	    node[k] = t;
	}

	if (cmdfile) {
	    if ((node_cnt == 1) &&
		(cmd_count > 0) &&
		(length == cmd_length) &&
		(offset == (cmd_offset + (cmd_length * cmd_count))))
	    {
		// Current region is associated with just one node and is
		// contiguous with the command we are building.
		if (node[0] == cmd_node[cmd_count % cmd_node_cnt]) {
		    // Cycling through the nodes we have already collected.
		    cmd_count++;
		    continue;
		}
		if ((cmd_node_cnt == cmd_count) &&
				    (cmd_node_cnt < node_count)) {
		    // Add a new node to the command destinations list.
		    cmd_node[cmd_node_cnt++] = node[0];
		    cmd_count++;
		    continue;
		}
	    }

	    // If we get here, we were not able to add the current region
	    // to the current command.
	    if (cmd_count > 0) {
		// We have accumulated a command, so print it and clear it.
		if (cmd_count == 1) {
		    printf("0x%lx(0x%lx): %d\n",
			    cmd_offset, cmd_length, cmd_node[0]);
		} else {
		    printf("0x%lx(0x%lx x %ld): ",
			    cmd_offset, cmd_length, cmd_count);
		    PrintList(cmd_node, cmd_node_cnt);
		    printf("\n");
		}
		cmd_count = 0;
		cmd_offset = 0;
		cmd_length = 0;
		cmd_node_cnt = 0;
	    }
	    if (node_cnt == 1) {
		// There is just one destination node, so start a new command.
		cmd_count = 1;
		cmd_offset = offset;
		cmd_length = length;
		cmd_node_cnt = 1;
		cmd_node[0] = node[0];
	    } else {
		// Multiple destinations, so just print the region.
		printf("0x%lx(0x%lx): ", offset, length);
		PrintList(node, node_cnt);
		printf("\n");
	    }
	} else {
	    // Not in --cmdfile mode.
	    if (offset > prev_end) {
		// Indicate a gap as an offset with no destination nodes.
		printf("0x%016lx:\n", prev_end);
	    }

	    // Print the offset with its destinations.
	    printf("0x%016lx: ", offset);
	    PrintList(node, node_cnt);
	    printf("\n");

	    // Record the end of the region.
	    prev_end = offset + length;
	}
    }

    // Print any accumulated command (in --cmdfile mode), or show the end
    // of the final region.
    if (cmdfile) {
	if (cmd_count > 0) {
	    if (cmd_count == 1) {
		printf("0x%lx(0x%lx): %d\n",
			cmd_offset, cmd_length, cmd_node[0]);
	    } else {
		printf("0x%lx(0x%lx x %ld): ",
			cmd_offset, cmd_length, cmd_count);
		PrintList(cmd_node, cmd_node_cnt);
		printf("\n");
	    }
	}
    } else {
	printf("0x%016lx:\n", prev_end);
    }

    free(region);
}

void DisplayByNode(char *mapfile, char *nodes, int summary)
{
    MapfileLoad(mapfile, summary);
    uint64_t node_count = MapfileNodeCount();
    printf("node count: %ld\n", node_count);

    char *p = nodes;
    if (p == NULL) {
	// Node list not provided. Assume a single, "infinite" range.
	p = "0 - 1000000000";
    }
    while ((*p) != '\0') {
	// Extract a node number or range from the list
	int64_t first, last, n;
	while (isspace(*p)) p++;
	if (!isdigit(*p)) break;
	first = strtol(p, &p, 0);
	last = first;
	while (isspace(*p)) p++;
	if ((*p) == '-') {
	    p++;
	    while (isspace(*p)) p++;
	    if (!isdigit(*p)) break;
	    last = strtol(p, &p, 0);
	}

	if (first < 0) first = 0;
	if (last >= node_count) last = node_count - 1;
	for (n = first; n <= last; n++) {
	    // Print summary information for the node.
	    uint64_t regcnt = MapfileRegionCount(n);
	    printf("node %4ld: %8ld region%s, total size 0x%lx bytes\n",
		   n, regcnt, (regcnt == 1) ? "" : "s",
		   MapfileTotalDataSize(n));

	    if (!summary) {
		// Print the individual regions for the node.
		int64_t r;
		for (r = 0; r < regcnt; r++) {
		    uint64_t offset, length;
		    MapfileGetRegion(n, r, &offset, &length);
		    printf("%10ld: 0x%lx (0x%lx)\n", r, offset, length);
		}
	    }
	}

	// Move to the next node number or range.
	while (isspace(*p)) p++;
	if ((*p) != ',') break;
	p++;
    }

    if ((*p) != '\0') {
	fprintf(stderr, "%s: could not parse node list \"%s\"\n",
		ProgName, nodes);
	exit(-1);
    }
}

int main(int argc, char **argv)
{
    ProgName = argv[0];

    enum {
	OPT_HELP,
	OPT_BY_OFFSET,
	OPT_RANGE,
	OPT_CMDFILE,
	OPT_BY_NODE,
	OPT_NODES,
	OPT_SUMMARY
    };

    static struct option options[] = {
	{"help",      0, NULL, OPT_HELP},
	{"by_offset", 0, NULL, OPT_BY_OFFSET},
	{"range",     1, NULL, OPT_RANGE},
	{"cmdfile",   0, NULL, OPT_CMDFILE},
	{"by_node",   0, NULL, OPT_BY_NODE},
	{"nodes",     1, NULL, OPT_NODES},
	{"summary",   0, NULL, OPT_SUMMARY},
	{0, 0, 0, 0}
    };

    int by_offset = 0;
    char *range = NULL;
    int cmdfile = 0;
    int by_node = 0;
    char *nodes = NULL;
    int summary = 0;
    char *mapfile = NULL;

    int opt;
    while ((opt = getopt_long_only(argc, argv, "", options, NULL)) != -1) {

	switch (opt) {
	case OPT_BY_OFFSET:
	    by_offset = 1;
	    break;

	case OPT_RANGE:
	    range = optarg;
	    break;

	case OPT_CMDFILE:
	    cmdfile = 1;
	    break;

	case OPT_BY_NODE:
	    by_node = 1;
	    break;

	case OPT_NODES:
	    nodes = optarg;
	    break;

	case OPT_SUMMARY:
	    summary = 1;
	    break;

	case OPT_HELP:
	default:
	    Usage(NULL);
	    exit(-1);
	}
    }

    if (optind < (argc - 1)) {
	Usage("too many non-option arguments");
	exit(-1);
    }
    if (optind > (argc - 1)) {
	Usage("missing <mapfile> argument");
	exit(-1);
    }
    mapfile = argv[optind];

    if (by_offset && by_node) {
	Usage("--by_offset and --by_node are mutually exclusive");
	exit(-1);
    }
    if (!(by_offset || by_node)) by_offset = 1;

    if ((range != NULL) && !by_offset) {
	Usage("--range is valid only in by_offset mode");
	exit(-1);
    }

    if (cmdfile && !by_offset) {
	Usage("--cmdfile is valid only in by_offset mode");
	exit(-1);
    }

    if ((nodes != NULL) && !by_node) {
	Usage("--nodes is valid only in by_node mode");
	exit(-1);
    }

    if (summary && !by_node) {
	Usage("--summary is valid only in by_node mode");
	exit(-1);
    }

    MapfileInit(ProgName);

    if (by_offset) {
	DisplayByOffset(mapfile, range, cmdfile);
    } else {
	DisplayByNode(mapfile, nodes, summary);
    }

    return 0;
}
