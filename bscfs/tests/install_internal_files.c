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
            "        --index_file <path>\n"
            "        --data_file <path>\n"
            "        --state STABLE|MODIFIED\n",
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
	OPT_INDEX_FILE,
	OPT_DATA_FILE,
	OPT_STATE
    };

    static struct option options[] = {
	{"help",       0, NULL, OPT_HELP},
	{"bscfs_file", 1, NULL, OPT_BSCFS_FILE},
	{"index_file", 1, NULL, OPT_INDEX_FILE},
	{"data_file",  1, NULL, OPT_DATA_FILE},
	{"state",      1, NULL, OPT_STATE},
	{0, 0, 0, 0}
    };

    char *bscfs_file = NULL;
    char *index_file = NULL;
    char *data_file = NULL;
    char *state = NULL;

    int opt;
    while ((opt = getopt_long_only(argc, argv, "", options, NULL)) != -1) {

	switch (opt) {
	case OPT_BSCFS_FILE:
	    bscfs_file = optarg;
	    break;

	case OPT_INDEX_FILE:
	    index_file = optarg;
	    break;

	case OPT_DATA_FILE:
	    data_file = optarg;
	    break;

	case OPT_STATE:
	    state = optarg;
	    break;

	case OPT_HELP:
	default:
	    Usage(NULL);
	    exit(-1);
	}
    }

    Check(bscfs_file != NULL, "bscfs_file param check", -1);
    Check(index_file != NULL, "index_file param check", -1);
    Check(data_file != NULL, "data_file param check", -1);
    Check(state != NULL, "state param check", -1);

    printf("%s:\n", ProgName);
    printf("    bscfs_file \"%s\"\n", bscfs_file);
    printf("    index_file \"%s\"\n", index_file);
    printf("    data_file  \"%s\"\n", data_file);
    printf("    state      \"%s\"\n", state);

    Check(strnlen(bscfs_file, BSCFS_PATH_MAX) < BSCFS_PATH_MAX,
	  "bscfs_file length check", -1);
    Check(strnlen(index_file, BSCFS_PATH_MAX) < BSCFS_PATH_MAX,
	  "index_file length check", -1);
    Check(strnlen(data_file, BSCFS_PATH_MAX) < BSCFS_PATH_MAX,
	  "data_file length check", -1);

    Check((strcmp(state, "STABLE") == 0) || (strcmp(state, "MODIFIED") == 0),
	  "state check", -1);

    size_t index_file_length = strlen(index_file) + 1; // include null
    size_t data_file_length = strlen(data_file) + 1; // include null
    size_t files_length = index_file_length + data_file_length + 1;
    char *files = (char *) malloc(files_length);
    memcpy(files,                     index_file, index_file_length);
    memcpy(files + index_file_length, data_file,  data_file_length);
    files[files_length - 1] = '\0';

    int s = (strcmp(state, "STABLE") == 0) ? BSCFS_STABLE : BSCFS_MODIFIED;

    int rc = BSCFS_InstallInternalFiles(bscfs_file, files_length, files, s);
    Check(rc == 0, "BSCFS_InstallInternalFiles", rc);

    return 0;
}
