/*******************************************************************************
 |    bbcounters.h
 |
 |  © Copyright IBM Corporation 2020,2020. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/

#ifndef MKBBCOUNTER
#include "utilities/include/atomics.h"
enum WEAK_COUNTERS
#define BBCOUNTERS_INIT 1
#define MKBBCOUNTER(name) BB_COUNTERS_##name,
{
#endif

MKBBCOUNTER(fopen64)
MKBBCOUNTER(rename)
MKBBCOUNTER(unlink)
MKBBCOUNTER(xstat)
MKBBCOUNTER(xstat64)
MKBBCOUNTER(statvfs64)
MKBBCOUNTER(inbound_message)
MKBBCOUNTER(bbio_bscfs_write)
MKBBCOUNTER(bbio_bscfs_write_bytes)
MKBBCOUNTER(bbio_regular_write)
MKBBCOUNTER(bbio_regular_write_bytes)
MKBBCOUNTER(bbio_regular_read)
MKBBCOUNTER(bbio_regular_read_bytes)

#ifdef BBCOUNTERS_INIT
    BB_COUNTER_MAX
};
extern unsigned long bbcounters[BB_COUNTER_MAX];
#ifdef __powerpc64__
#define BUMPCOUNTER(id) Fetch_and_Add(&bbcounters[BB_COUNTERS_##id], 1)
#define BUMPCOUNTER_by(id,amount) Fetch_and_Add(&bbcounters[BB_COUNTERS_##id], amount)
#else
#define BUMPCOUNTER(id) 
#define BUMPCOUNTER_by(id,amount) 
#endif
#endif
#undef MKBBCOUNTER
#undef BBCOUNTERS_INIT
