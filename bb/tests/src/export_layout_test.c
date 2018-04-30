/*******************************************************************************
 |    export_layout_test.c
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


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "export_layout.h"

int main(int argc, char *argv[])
{
    struct export_layout_transfer_setup *setup;
    struct export_layout_transfer_finalize finalize;
    char *file_name, *blkdev_name;
    unsigned long offset, length, writing, delay;
    int exlo, file_fd, oflags;
    FILE *blkdev_file;
    long count, rc, i, j;

    if (argc != 7) {
        fprintf(stderr,
            "Usage: %s <file> <offset> <length> "
            "<writing> <delay> <blkdev>\n",
            argv[0]);
        exit(-1);
    }

    file_name = argv[1];
    offset = strtoul(argv[2], NULL, 0);
    length = strtoul(argv[3], NULL, 0);
    writing = strtoul(argv[4], NULL, 0);
    delay = strtoul(argv[5], NULL, 0);
    blkdev_name = argv[6];

    exlo = open("/dev/export_layout", O_RDWR, 0);
    if (exlo < 0) {
        fprintf(stderr, "open /dev/export_layout failed, errno %d\n",
            errno);
        exit(-1);
    }

    if (writing) {
        oflags = O_CREAT | O_WRONLY;
    } else {
        oflags = O_RDONLY;
    }
    file_fd = open(file_name, oflags, 0644);
    if (file_fd < 0) {
        fprintf(stderr, "open \"%s\" failed, errno %d\n",
            argv[1], errno);
        exit(-1);
    }

    count = 0;
    setup = NULL;
    while (setup == NULL) {
        setup = (struct export_layout_transfer_setup *)
        malloc(sizeof(struct export_layout_transfer_setup) +
               (count * sizeof(struct export_layout_extent)));
        if (setup == NULL) {
            fprintf(stderr, "malloc failed\n");
            exit(-1);
        }
        setup->fd = file_fd;
        setup->offset = offset;
        setup->length = length;
        setup->writing = writing;
        setup->extent_count_max = count;
        rc = ioctl(exlo, EXPORT_LAYOUT_IOC_TRANSFER_SETUP, setup);
        if (rc < 0) {
            free(setup);
            fprintf(stderr, "ioctl SETUP failed, errno %d\n",
                    errno);
            exit(-1);
        }
        if (rc > 0) {
            fprintf(stderr, "too many extents, retrying\n");
            free(setup);
            setup = NULL;
            count = rc;
        }
    }

    printf("transfer_setup succeeded, with %d extent%s:\n",
            setup->extent_count, (setup->extent_count == 1) ? "" : "s");
    for (i = 0; i < setup->extent_count; i++) {
#if __powerpc64__
        printf("    file_offset %12lx, blkdev_offset %12lx, "
               "length %12lx\n",
        setup->extent[i].file_offset,
        setup->extent[i].blkdev_offset,
        setup->extent[i].length);
#else
        printf("    file_offset %12llx, blkdev_offset %12llx, "
               "length %12llx\n",
        setup->extent[i].file_offset,
        setup->extent[i].blkdev_offset,
        setup->extent[i].length);
#endif
    }

    sleep((delay + 1) / 2);

    if (writing) {
        printf("Writing file via blkdev %s\n", blkdev_name);

        blkdev_file = fopen(blkdev_name, "w");
        if (blkdev_file == NULL) {
            fprintf(stderr, "fopen failed for blkdev %s\n",
                    blkdev_name);
            exit(-1);
        }

        for (i = 0; i < setup->extent_count; i++) {
            rc = fseek(blkdev_file, setup->extent[i].blkdev_offset,
                       SEEK_SET);
            if (rc < 0) {
                fprintf(stderr, "fseek in blkdev failed\n");
                exit(-1);
            }
            for (j = 0; j < setup->extent[i].length; j += 16) {
#if __powerpc64__
                fprintf(blkdev_file, "%15lx\n",
                        setup->extent[i].file_offset + j);
#else
                fprintf(blkdev_file, "%15llx\n",
                        setup->extent[i].file_offset + j);
#endif
            }
        }

        fclose(blkdev_file);
    }

    sleep((delay + 1) / 2);

    printf("Finalizing transfer\n");
    finalize.status = 0;
    rc = ioctl(exlo, EXPORT_LAYOUT_IOC_TRANSFER_FINALIZE, &finalize);
    if (rc < 0) {
        fprintf(stderr, "ioctl FINALIZE failed, errno %d\n",
                errno);
        exit(-1);
    }

    printf("Success!\n");

    return 0;
}
