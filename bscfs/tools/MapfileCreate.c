/*******************************************************************************
 |    MapfileCreate.c
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
#include <errno.h>
#include <ctype.h>
#include <string.h>

#include "MapfileUtil.h"

char *ProgName = NULL;

// Maximum number of destinations in a command <dest_list>
#define DEST_COUNT_MAX (NODE_COUNT_MAX * 64)

void Usage(char *errmsg)
{
    if (errmsg != NULL) {
	fprintf(stderr, "%s: %s\n", ProgName, errmsg);
    }
    fprintf(stderr,
	    "Usage:\n"
            "    %s\n"
            "        [--help]\n"
            "        [--input <base_mapfile>]\n"
            "        [--node_count <count>]\n"
            "        [--proc_group_size <number>]\n"
            "        [--cmd <command> | --cmdfile <command_file>] ...\n"
            "        <mapfile>\n"
	    "\n"
	    "    <command_file> is a text file with one <command> per line\n"
	    "\n"
	    "    <command> has the form\n"
	    "        <offset> (<length> [x <count>]) : <dest_list>\n"
	    "\n"
	    "    <dest_list> is a comma-separated list of integers\n"
	    "        or integer ranges, with the integers being interpreted\n"
	    "        as node numbers or process numbers depending on whether\n"
	    "        --proc_group_size is specified\n"
	    "\n",
	    ProgName);
}

static uint64_t NodeCount = 0;
static uint64_t ProcGroupSize = 0;

uint64_t DestNode(uint64_t dest)
{
    if (ProcGroupSize > 0) {
	dest /= ProcGroupSize;
	if (NodeCount > 0) {
	    dest %= NodeCount;
	}
    } else if ((NodeCount > 0) && (dest >= NodeCount)) {
	fprintf(stderr, "%s: destination node %ld out of range\n",
		ProgName, dest);
	exit(-1);
    }
    return dest;
}

void ConsumeSuffix(char **pp, uint64_t *numberp,
		   uint64_t binary_multiplier, uint64_t decimal_multiplier)
{
    (*pp)++;
    if (tolower(**pp) == 'b') {
	(*pp)++;
	(*numberp) *= decimal_multiplier;
    } else {
	if ((tolower(**pp) == 'i') && (tolower(*((*pp) + 1)) == 'b')) {
	    (*pp) += 2;
	}
	(*numberp) *= binary_multiplier;
    }
}

int ConsumeNumber(char **pp, uint64_t *numberp)
{
    while (isspace(**pp)) (*pp)++;
    if (!isdigit(**pp)) return 0;
    (*numberp) = strtoul(*pp, pp, 0);

    char c = tolower(**pp);
    if      (c == 'k') ConsumeSuffix(pp, numberp, (1ul<<10), (uint64_t) 1e3);
    else if (c == 'm') ConsumeSuffix(pp, numberp, (1ul<<20), (uint64_t) 1e6);
    else if (c == 'g') ConsumeSuffix(pp, numberp, (1ul<<30), (uint64_t) 1e9);
    else if (c == 't') ConsumeSuffix(pp, numberp, (1ul<<40), (uint64_t) 1e12);

    return 1;
}

int ConsumeChar(char **pp, char c)
{
    while (isspace(**pp)) (*pp)++;
    if ((**pp) == c) {
	(*pp)++;
	return 1;
    }
    return 0;
}

void CmdError(char *cmd, char *tail, char *msg)
{
    fprintf(stderr, "%s: error parsing command: %s\n", ProgName, msg);
    fprintf(stderr, "    command: %s\n", cmd);
    fprintf(stderr, "    error occurred near character %ld\n", tail - cmd);
    exit(-1);
}

void ProcessCommand(char *cmd)
{
    char *p = cmd;

    // ConsumeChar will consume all white space, whether or not it sees a '#'
    if (ConsumeChar(&p, '#') || ((*p) == '\0')) {
	// blank line or comment line
	return;
    }

    uint64_t offset;
    if (!ConsumeNumber(&p, &offset)) CmdError(cmd, p, "expected <offset>");

    if (!ConsumeChar(&p, '(')) CmdError(cmd, p, "expected '('");

    uint64_t length;
    if (!ConsumeNumber(&p, &length)) CmdError(cmd, p, "expected <length>");
    if (length == 0) CmdError(cmd, p, "<length> must be non-zero");

    uint64_t count = 1;

    if (ConsumeChar(&p, 'x')) {
	if (!ConsumeNumber(&p, &count)) CmdError(cmd, p, "expected <count>");
	if (count == 0) CmdError(cmd, p, "<count> must be non-zero");
    }

    if (!ConsumeChar(&p, ')')) CmdError(cmd, p, "expected ')'");
    if (!ConsumeChar(&p, ':')) CmdError(cmd, p, "expected ':'");

    static int dest[DEST_COUNT_MAX];
    uint64_t dest_count = 0;
    do {
	uint64_t first;
	if (!ConsumeNumber(&p, &first)) CmdError(cmd, p, "expected <dest>");
	uint64_t last = first;
	if (ConsumeChar(&p, '-')) {
	    if (!ConsumeNumber(&p, &last)) CmdError(cmd, p, "expected <dest>");
	    if (last < first) CmdError(cmd, p, "illegal destination range");
	}
	uint64_t d;
	for (d = first; d <= last; d++) {
	    if (dest_count >= DEST_COUNT_MAX) {
		CmdError(cmd, p, "too many destinations");
	    }
	    dest[dest_count++] = d;
	}
    } while (ConsumeChar(&p, ','));

    if ((*p) != '\0') CmdError(cmd, p, "extraneous characters");

    uint64_t total_blocks = 0;
    uint64_t total_dests = 0;
    uint64_t current_block = 0;
    uint64_t current_dest = 0;
    // loop until the longer list is exhausted
    while ((total_blocks < count) || (total_dests < dest_count)) {
	// reset the shorter list to the beginning if necessary
	if (current_block >= count) current_block = 0;
	if (current_dest >= dest_count) current_dest = 0;
	MapfileAddRegion(DestNode(dest[current_dest]),
			 offset + (current_block * length),
			 length);
	current_block++;
	current_dest++;
	total_blocks++;
	total_dests++;
    }
}

int main(int argc, char **argv)
{
    ProgName = argv[0];

    enum {
	OPT_HELP,
	OPT_INPUT,
	OPT_NODE_COUNT,
	OPT_PROC_GROUP_SIZE,
	OPT_CMD,
	OPT_CMDFILE
    };

    static struct option options[] = {
	{"help",            0, NULL, OPT_HELP},
	{"input",           1, NULL, OPT_INPUT},
	{"node_count",      1, NULL, OPT_NODE_COUNT},
	{"proc_group_size", 1, NULL, OPT_PROC_GROUP_SIZE},
	{"cmd",             1, NULL, OPT_CMD},
	{"cmdfile",         1, NULL, OPT_CMDFILE},
	{0, 0, 0, 0}
    };

    typedef struct {
	int is_file;
	char *name;
    } CmdArg;

    char *input_mapfile = NULL;
    CmdArg *cmd_arg = NULL;
    int cmd_arg_max = 0;
    int cmd_arg_cnt = 0;
    char *mapfile = NULL;

    int opt;
    while ((opt = getopt_long_only(argc, argv, "", options, NULL)) != -1) {

	switch (opt) {
	case OPT_INPUT:
	    input_mapfile = optarg;
	    break;

	case OPT_NODE_COUNT:
	    NodeCount = strtoul(optarg, NULL, 0);
	    if (NodeCount <= 0) {
		Usage("--node_count value must be positive\n");
	    }
	    break;

	case OPT_PROC_GROUP_SIZE:
	    ProcGroupSize = strtoul(optarg, NULL, 0);
	    if (ProcGroupSize <= 0) {
		Usage("--proc_group_size value must be positive\n");
	    }
	    break;

	case OPT_CMD:
	case OPT_CMDFILE:
	    if (cmd_arg_cnt >= cmd_arg_max) {
		cmd_arg_max += 1000;
		cmd_arg = (CmdArg *)
			    realloc(cmd_arg, cmd_arg_max * sizeof(CmdArg));
	    }
	    cmd_arg[cmd_arg_cnt].is_file = (opt == OPT_CMDFILE);
	    cmd_arg[cmd_arg_cnt].name = optarg;
	    cmd_arg_cnt++;
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

    MapfileInit(ProgName);
    if (input_mapfile != NULL) {
	MapfileLoad(input_mapfile, 0);
    }

    if (NodeCount > 0) {
	MapfileSetNodeCount(NodeCount);
    }

    uint64_t i;
    for (i = 0; i < cmd_arg_cnt; i++) {
	if (cmd_arg[i].is_file) {
	    FILE *cf = fopen(cmd_arg[i].name, "r");
	    if (cf == NULL) {
		fprintf(stderr, "%s: fopen(\"%s\") failed: %s\n",
			ProgName, cmd_arg[i].name, strerror(errno));
		exit(-1);
	    }
	    char *cmd = NULL;
	    size_t cmd_size = 0;
	    while (getline(&cmd, &cmd_size, cf) != -1) {
		ProcessCommand(cmd);
	    }
	    if (cmd != NULL) {
		free(cmd);
	    }
	    fclose(cf);
	} else {
	    ProcessCommand(cmd_arg[i].name);
	}
    }

    MapfileSave(mapfile);

    if (cmd_arg != NULL) free(cmd_arg); // to keep clang happy
    return 0;
}
