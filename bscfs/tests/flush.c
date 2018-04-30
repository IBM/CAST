/******************************************************************************
 |    flush.c
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
#include <getopt.h>
#include <string.h>

#include "bscfsAPI.h"

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
            "        --bscfs_file <path>\n"
            "        --map_file <path>\n"
            "        --cleanup_file <path>\n",
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

int main(int argc, char **argv)
{
    ProgName = argv[0];

    enum {
	OPT_HELP,
	OPT_BSCFS_FILE,
	OPT_MAP_FILE,
	OPT_CLEANUP_FILE
    };

    static struct option options[] = {
	{"help",         0, NULL, OPT_HELP},
	{"bscfs_file",   1, NULL, OPT_BSCFS_FILE},
	{"map_file",     1, NULL, OPT_MAP_FILE},
	{"cleanup_file", 1, NULL, OPT_CLEANUP_FILE},
	{0, 0, 0, 0}
    };

    char *bscfs_file = NULL;
    char *map_file = NULL;
    char *cleanup_file = NULL;

    int opt;
    while ((opt = getopt_long_only(argc, argv, "", options, NULL)) != -1) {

	switch (opt) {
	case OPT_BSCFS_FILE:
	    bscfs_file = optarg;
	    break;

	case OPT_MAP_FILE:
	    map_file = optarg;
	    break;

	case OPT_CLEANUP_FILE:
	    cleanup_file = optarg;
	    break;

	case OPT_HELP:
	default:
	    Usage(NULL);
	    exit(-1);
	}
    }

    Check(bscfs_file != NULL, "bscfs_file param check", -1);
    Check(map_file != NULL, "map_file param check", -1);
    Check(cleanup_file != NULL, "cleanup_file param check", -1);

    printf("%s:\n", ProgName);
    printf("    bscfs_file \"%s\"\n", bscfs_file);

    Check(strnlen(bscfs_file, BSCFS_PATH_MAX) < BSCFS_PATH_MAX,
	  "bscfs_file length check", -1);
    Check(strnlen(map_file, BSCFS_PATH_MAX) < BSCFS_PATH_MAX,
	  "map_file length check", -1);
    Check(strnlen(cleanup_file, BSCFS_PATH_MAX) < BSCFS_PATH_MAX,
	  "cleanup_file length check", -1);

    BBTransferHandle_t handle = -1;
    int rc = BSCFS_StartLocalFlush(bscfs_file, map_file, cleanup_file, &handle);
    Check(rc == 0, "BSCFS_StartLocalFlush", rc);

    rc = BSCFS_AwaitLocalTransfer(handle);
    Check(rc == 0, "BSCFS_AwaitLocalTransfer", rc);

    return 0;
}
