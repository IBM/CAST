/*******************************************************************************
 |    MapfileUtil.h
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

#include <stdint.h>

#define NODE_COUNT_MAX 16384

void MapfileInit(char *program);
void MapfileLoad(char *mapfile, int summary);
void MapfileSave(char *mapfile);
void MapfileSetNodeCount(uint64_t node_count);
uint64_t MapfileNodeCount();
uint64_t MapfileRegionCount(uint64_t node);
uint64_t MapfileTotalDataSize(uint64_t node);
void MapfileGetRegion(uint64_t node, uint64_t reg,
		      uint64_t *offset_p, uint64_t *length_p);
void MapfileAddRegion(uint64_t node, uint64_t offset, uint64_t length);
