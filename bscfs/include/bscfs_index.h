/******************************************************************************
 |    bscfs_index.h
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

#ifndef BSCFS_BSCFS_INDEX_H_
#define BSCFS_BSCFS_INDEX_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bscfs_mapping {
    uint64_t sf_offset;  // shared file offset
    uint64_t df_offset;  // data file offset
    uint64_t length;
} bscfs_mapping_t;

typedef struct bscfs_index {
    uint32_t node;       // node for which this index was created
    uint32_t node_count; // number of nodes in the job or covered by the mapfile
    int32_t normalized;  // index is sorted in sf_offset order, with no overlaps
    int32_t finalized;   // index is sorted in df_offset order, with no overlaps
                         // (index can be both normalized and finalized)
    uint64_t mapping_count;
    struct bscfs_mapping mapping[];
} bscfs_index_t;

#define BSCFS_INDEX_SIZE(n) \
    (sizeof(bscfs_index_t) + ((n)*sizeof(bscfs_mapping_t)))

#define BSCFS_INDEX_MAPPING_COUNT_MAX(size) \
    (((size) - sizeof(bscfs_index_t)) / sizeof(bscfs_mapping_t))

#define BSCFS_INDEX_BLOCK_SIZE (64 * 1024) // allocation unit for SSD space

#ifdef __cplusplus
}
#endif

#endif /* BSCFS_BSCFS_INDEX_H_ */
