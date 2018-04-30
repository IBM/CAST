/******************************************************************************
 |    bscfs_mapfile.h
 |
 |  © Copyright IBM Corporation 2015,2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 ******************************************************************************/

#include <stdint.h>

typedef struct bscfs_mapfile_region {
    uint64_t offset;
    uint64_t length;
} bscfs_mapfile_region_t;

typedef struct bscfs_mapfile_node_header {
    uint64_t region_offset;
    uint64_t region_count;
    uint64_t total_data_size;
} bscfs_mapfile_node_header_t;

typedef struct bscfs_mapfile_header {
    uint64_t node_count;
    struct bscfs_mapfile_node_header node[];
} bscfs_mapfile_header_t;
