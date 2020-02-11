/*******************************************************************************
 |    flightlog.h
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


/**
 * \file flightlog.h
 * This file contains the flightlog.
 *
 * \defgroup flightlog Flightlog
 *
 * Flightlog provides a very lightweight logging mechanism.  It uses a circular buffer that is backed by an mmap()'d file.
 * Such that if the application were to crash, the flightlog is retained.
 * There is a scripting infrastructure that scans your source code for flightlog entries.
 *
 * \par CMakeLists.txt example
 * \verbatim
add_executable(mytarget mytarget.c)
flightgen(mytarget mytarget_flightlog.h)
\endverbatim
 *
 * \par mytarget.c
 * \verbatim
#include "mytarget_flightlog.h"
....
{
     FL_CreateAll("/coral/flightlogs/");
     FL_Write(FLTarget, FL_STARTUP, "startup mytarget executable. pid=%ld  uid=%ld", getpid(), getuid(),0,0);
}
\endverbatim
 *
 * \par Libraries
 * If the executable includes any libraries that contain flightlogs, they should be specified via the flightlib
 * cmake function.  This allows the FL_CreateAll() routine to setup and create space for any library-based flightlogs.
 * \par CMakeLists.txt example for libraries
 * \verbatim
add_executable(mytarget mytarget.c)
flightgen(mytarget mytarget_flightlog.h)
flightlib(mytarget tarlib)
target_link_libraries(mytarget tarlib)

add_library(tarlib tarlib.c)
flightgen(tarlib tarlib_flightlog.h)
\endverbatim
 *
 * \par Decoding Flightlogs
 * There is a standard commandline utility to decode flightlogs.
 * \verbatim
flightlog/bin/decoder <file1> <file2> <file3> ...
\endverbatim
 *
 * Each file is the mmap()'d file location.
 \par Example
 \verbatim
$ flightlog/bin/decoder /tmp/FLBBSERVER
 Reading flightlog /tmp/FLBBSERVER
 TB=0000000071c354da  FL_BEGIN_LOG:-- Starting log "Burst Buffer server major event Log"
 TB=0000000071c354da    FL_STARTUP:0  Starting burst buffer server  User=502


 *** END ***
\endverbatim
 *
 */

#ifndef	_FLIGHTLOG_H_ /* Prevent multiple inclusion */
#define	_FLIGHTLOG_H_

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __powerpc64__
#define  SPRN_TBRO                (0x10C)          // Time Base 64-bit                             User Read-only
#define  SPRN_USPRG3              (0x103)          // userspace SPR G3 - - Linux stores CPU        User Read-only
#endif

#define __INLINE__ static inline

#ifndef FLIGHTLOG_ASSUME_POWEROF2
#define FLIGHTLOG_ASSUME_POWEROF2 0  /// If flightlog is a power-of-2, we can save an expensive divide operation in FL_Write() path.
#endif

/**
 * \brief Flight recorder log entry
 *
 * Binary data structure for each log entry.
 * Comprises 16 bytes of metadata plus 48 bytes of data
 * Fits into a single 64-byte cacheline.  Flightlog is page-aligned.
 *
 */
typedef struct FlightRecorderLog
{
	uint64_t timestamp;  /// Timestamp in processor-cycles
	uint32_t hwthread;   /// hwthread ID that performed the FL_Write.  This is usually the CPU index.
	uint32_t id;         /// The ID of the event type
	uint64_t data[6];    /// Storage for the user-defined data
} FlightRecorderLog_t;

typedef void (*FlightRecorderFunc_t)(size_t bufsize, char* buffer, const FlightRecorderLog_t* logentry);

/**
 * \brief Information needed to format a specific flightlog entry
 */
typedef struct FlightRecorderFormatter
{
    const char                    id_str[64];
    const char                    formatString[256];
} FlightRecorderFormatter_t;

typedef struct
{
    uint32_t id;
    const char* functionName;
} FlightDecoderDesc_t;

int FL_AddDecoderLibPath(const char* searchpath);

extern int FL_openDecoder(int* count, FlightDecoderDesc_t** ptr);
#define FL_DefineDecoder(descArray) \
    int FL_openDecoder(int* count, FlightDecoderDesc_t** ptr) \
    { \
	*count = sizeof(descArray) / sizeof(FlightDecoderDesc_t); \
	*ptr   = descArray; \
	return 0; \
    }

/**
 * \brief Information needed to write/track a flightlog registry
 */
typedef struct FlightRecorderRegistry
{
    // below are fields needed for writing log entry
#ifdef __powerpc__
    uint64_t                       flightlock;           ///< flightlock needs to be 1st entry in FlightRecorderRegistry_t (ppc asm performance opt)
#elif __x86_64__
    uint32_t                       flightlock;
    uint32_t                       padding1;
#else
  #error not supported
#endif
    uint64_t                       flightchecksum;
    uint64_t                       flightsize;         ///< size of the flightlog, in entries  (circular buffer wrap size)

    // below are static fields needed for decoder
    uint64_t                       num_ids;            ///< maximum number of IDs in the registry.
    double                         timebaseScale;      ///< timebase scaling of the recorded system
    uint64_t                       timebaseAdjust;     ///< calculated adjustment for timebase
    uint64_t                       bootid;             ///< First 64-bits of the bootid
    uint64_t                       padding2;           ///< pad to 64 byte boundary
    const char                     decoderName[64];    ///< path to the flightlog decoder
    const char                     registryName[128];  ///< name of the registry
} FlightRecorderRegistry_t;

#define FLIGHTLOG_OFFSET sizeof(FlightRecorderRegistry_t)

typedef struct FlightRecorderRegistryList
{
	struct FlightRecorderRegistryList* nextRegistry;
	FlightRecorderRegistry_t*          reg;
	FlightRecorderFormatter_t*     flightformatter;
	FlightRecorderLog_t*           flightlog;
        size_t                         maxidlen;

	// below are temporary scratchspace for decoder
	uint32_t                       lastStateSet;
	uint64_t                       lastState;
	uint64_t                       lastStateTotal;
	uint64_t                       lastOffset;
        uint64_t                       count;
        uint64_t                       counttotal;
	uint64_t                       id;
} FlightRecorderRegistryList_t;

/*!
 * \brief Array of FlightRecorders to instanciate via FL_CreateAll()
 */
typedef struct FlightRecorderCreate
{
    FlightRecorderRegistry_t** reg;
    const char* name;
    const char* filename;
    const char* decoderName;
    unsigned int size;
    FlightRecorderFormatter_t** fmt;
    unsigned int numenums;
} FlightRecorderCreate_t;


/**
 * \brief Write information to a lightweight flight recorder log
 *
 * The following SPI routine is intended to provide a lightweight means of writing data into
 * a log.  The implementation should ideally make use of BGQ's L2 atomic support.
 *
 * The buffer is circular, so once 'flightsize' entries have been created, the next used log
 * entry is 0.  The timebase field in FlightRecorderLog_t can be used to sort the entries
 * in a chronological order.
 *
 * \param[in]   reg         Flight recorder registry
 * \param[in]   ID          A unique identifier for the entry.  Caller defines what this means.
 * \param[in]   data0       Caller-specific data field
 * \param[in]   data1       Caller-specific data field
 * \param[in]   data2       Caller-specific data field
 * \param[in]   data3       Caller-specific data field
 * \internal
 * This routine is called by FL_Write()
 * \endinternal
 *
 */
__INLINE__ uint64_t FL_Write_internal(FlightRecorderRegistry_t* reg,
				uint32_t ID, uint64_t data0, uint64_t data1, uint64_t data2, uint64_t data3)
{
    uint64_t timebase;
    uint64_t pir;

#ifdef __powerpc64__
    uint64_t myentry;
    asm volatile("1: ldarx   %0,0,%3;"
                 "addi 3, %0, 1;"
                 "stdcx. 3, 0, %3;"
                 "bne 1b;"
                 "mfspr %1,%4;"
                 "mfspr %2,%5;"
                 : "=&b" (myentry), "=&r" (timebase), "=&r" (pir) : "b" (reg), "i" (SPRN_TBRO), "i" (SPRN_USPRG3) : "cc", "memory", "r3");
#elif __x86_64__
    uint32_t myentry = 1;
    unsigned hi, lo;
    __asm__ __volatile__("lock; xaddl %0,%1"
			 : "+r" (myentry)
			 : "m" (*reg) : "memory");
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    timebase = ((uint64_t)hi << 32ull) | lo;
    pir = 0; // \todo get cpu#
#else
#error not supported
#endif

#if FLIGHTLOG_ASSUME_POWEROF2
    myentry &= (reg->flightsize-1);
#else
    myentry %= reg->flightsize;
#endif
    FlightRecorderLog_t* flightlog = (FlightRecorderLog_t*)((char*)reg + FLIGHTLOG_OFFSET);
    flightlog[myentry].timestamp = timebase;
    flightlog[myentry].id      = ID;
    flightlog[myentry].hwthread= pir&0x3ff;
    flightlog[myentry].data[0] = data0;
    flightlog[myentry].data[1] = data1;
    flightlog[myentry].data[2] = data2;
    flightlog[myentry].data[3] = data3;
    return myentry;
}


/**
 * \brief Write information to a lightweight flight recorder log
 *
 * The following SPI routine is intended to provide a lightweight means of writing data into
 * a log.  The implementation should ideally make use of BGQ's L2 atomic support.
 *
 * The buffer is circular, so once 'flightsize' entries have been created, the next used log
 * entry is 0.  The timebase field in FlightRecorderLog_t can be used to sort the entries
 * in a chronological order.
 *
 * \param[in]   reg         Flight recorder registry
 * \param[in]   ID          A unique identifier for the entry.  Caller defines what this means.
 * \param[in]   data0       Caller-specific data field
 * \param[in]   data1       Caller-specific data field
 * \param[in]   data2       Caller-specific data field
 * \param[in]   data3       Caller-specific data field
 * \param[in]   data4       Caller-specific data field
 * \param[in]   data5       Caller-specific data field
 * \internal
 * This is an internal API to the flightlog.
 * The 'str' field on the FL_Write() macro is consumed by the buildFlightRegistry scripting.
 * \endinternal
 *
 */
__INLINE__ uint64_t FL_Write6_internal(FlightRecorderRegistry_t* reg,
				       uint32_t ID, uint64_t data0, uint64_t data1, uint64_t data2, uint64_t data3, uint64_t data4, uint64_t data5)
{
    uint64_t timebase;
    uint64_t pir;

#ifdef __powerpc64__
    uint64_t myentry;
    asm volatile("1: ldarx   %0,0,%3;"
                 "addi 3, %0, 1;"
                 "stdcx. 3, 0, %3;"
                 "bne 1b;"
                 "mfspr %1,%4;"
                 "mfspr %2,%5;"
                 : "=&b" (myentry), "=&r" (timebase), "=&r" (pir) : "b" (reg), "i" (SPRN_TBRO), "i" (SPRN_USPRG3) : "cc", "memory", "r3");
#elif __x86_64__
    uint32_t myentry = 1;
    unsigned hi, lo;
    __asm__ __volatile__("lock; xaddl %0,%1"
			 : "+r" (myentry)
			 : "m" (*reg) : "memory");
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    timebase = ((uint64_t)hi << 32ull) | lo;
    pir = 0; // \todo get cpu#
#else
#error not supported
#endif

#if FLIGHTLOG_ASSUME_POWEROF2
    myentry &= (reg->flightsize-1);
#else
    myentry %= reg->flightsize;
#endif
    FlightRecorderLog_t* flightlog = (FlightRecorderLog_t*)((char*)reg + FLIGHTLOG_OFFSET);
    flightlog[myentry].timestamp = timebase;
    flightlog[myentry].id      = ID;
    flightlog[myentry].hwthread= pir&0x3ff;
    flightlog[myentry].data[0] = data0;
    flightlog[myentry].data[1] = data1;
    flightlog[myentry].data[2] = data2;
    flightlog[myentry].data[3] = data3;
    flightlog[myentry].data[4] = data4;
    flightlog[myentry].data[5] = data5;
    return myentry;
}

/**
 *  \brief Create all flightlog registries
 * \param[in] rootpath Directory that the logs will be stored
 * \ingroup flightlog
 */
#define FL_CreateAll(rootpath)
#undef FL_CreateAll  // real macro will be auto-generated by buildFlightRegistry.pl


/**
 * \brief Create Flightlog Registries
*/
extern int FL_CreateRegistries(const char* rootpath, unsigned int numreg, FlightRecorderCreate_t* flcreate, uint64_t csum);
extern int FL_AttachRegistry(FlightRecorderRegistryList_t** reglist, const char* filename, FlightRecorderRegistry_t* reg);
extern int FL_CreateRegistry(FlightRecorderRegistry_t** regHandle, const char* name, const char* filename, const char* decoder, uint64_t length, FlightRecorderFormatter_t* fmt, uint64_t numids, uint64_t csum);

/**
 * \brief Set the size of the flight log
 * Sets the maximum number of entries tracked in the flightlog ring buffer
 * Each entry is 64-bytes
 * \param[in] reg Registry name
 * \param[in] size Number of entries
 * \ingroup flightlog
 * \par Example
 * \verbatim
FL_SetSize(FLHelloWorld_High, 16384);
\endverbatim
 */
#define FL_SetSize(reg, size)

/**
 * \brief Set the name of the flight log
 * Sets the human-readable name for the flightlog
 * \param[in] reg Registry name
 * \param[in] name c-string containing the name of the flightlog
 * \ingroup flightlog
 * \par Example
 * \verbatim
FL_SetName(FLHelloWorld_High, "HelloWorld high-frequency flightlog");
\endverbatim
 */

#define FL_SetName(reg, name)
#define FL_SetPrefix(reg, prefix)
#define FL_SetDecoder(reg, prefix)

/**
 * \brief Create Flightlog Registry
*/
#define FL_Create(reg, name, filename, size) FL_CreateRegistry(&reg, name, filename, size, FLIGHTFMT_##reg, sizeof(FLIGHTFMT_##reg)/sizeof(FlightRecorderFormatter_t))

/**
 * \brief Attach to flightlog
 * \ingroup flightlog
 * \param[in] reglist registry list pointer
 * \param[in] filename Filename containing the flightlog
 */
#define FL_AttachFile(reglist, filename) FL_AttachRegistry(reglist, filename, NULL)

/**
 * \brief Attach to flightlog registry
 * \ingroup flightlog
 * \param[in] reglist registry list pointer
 * \param[in] reg  registry
 */
#define FL_Attach(reglist, reg)          FL_AttachRegistry(reglist, NULL, reg)


    /**
     * \brief Write information to a lightweight flight recorder log
     *
     * Provides a lightweight means of writing data into the specified flightlog.
     *
     * The buffer is circular, so once 'flightsize' entries have been created, the next used log
     * entry is 0.  The timebase field in FlightRecorderLog_t can be used to sort the entries
     * in a chronological order.
     *
     * \param[inout]   reg      Pointer the flightlog registry
     * \param[in]   ID          A unique identifier for the entry.  Caller defines what this means.
     * \param[in]   str         A format string (printf-style) for data0,data1,data2,data3
     * \param[in]   data0       Caller-specific data field
     * \param[in]   data1       Caller-specific data field
     * \param[in]   data2       Caller-specific data field
     * \param[in]   data3       Caller-specific data field
     * \ingroup flightlog
     * \par Example
     * \verbatim
FL_Write(mytestregistry, FL_STARTUP, "Starting example.  PID=%ld  UID=%ld  GID=%ld", (uint64_t)getpid(), (uint64_t)getuid(), (uint64_t)getgid(), 0, 0, 0);
\endverbatim
     * \ingroup flightlog
     *
     */
#define FL_Write(reg, ID, str, data0, data1, data2, data3)                FL_Write_internal(reg,ID,data0,data1,data2,data3)

/**
 * \brief Write information to a lightweight flight recorder log
 *
 * Provides a lightweight means of writing data into the specified flightlog.
 *
 * The buffer is circular, so once 'flightsize' entries have been created, the next used log
 * entry is 0.  The timebase field in FlightRecorderLog_t can be used to sort the entries
 * in a chronological order.
 *
 * \param[inout]   reg      Pointer the flightlog registry
 * \param[in]   ID          A unique identifier for the entry.  Caller defines what this means.
 * \param[in]   str         A format string (printf-style) for data0,data1,data2,data3,data4,data5
 * \param[in]   data0       Caller-specific data field
 * \param[in]   data1       Caller-specific data field
 * \param[in]   data2       Caller-specific data field
 * \param[in]   data3       Caller-specific data field
 * \param[in]   data4       Caller-specific data field
 * \param[in]   data5       Caller-specific data field
 * \par Example
 * \verbatim
FL_Write6(mytestregistry, FL_STARTUP, "Starting example.  PID=%ld  UID=%ld  GID=%ld", (uint64_t)getpid(), (uint64_t)getuid(), (uint64_t)getgid(), 0, 0, 0);
 \endverbatim
 * \ingroup flightlog
 *
*/
#define FL_Write6(reg, ID, str, data0, data1, data2, data3, data4, data5) FL_Write6_internal(reg,ID,data0,data1,data2,data3,data4,data5)

/*!
 * \brief Decode the flight recorder
 *  Decode the flight recorder into a buffer (for subsequent printing/fileoutput/post-processing by the calling function)
 * \ingroup flightlog
 * \par Example
\verbatim
#define BUFFERSIZE 65536
    uint64_t more;
    char buffer[BUFFERSIZE];
    do
    {
        more = 0;
        memset(buffer, 0, BUFFERSIZE);
        FL_Decode(list, BUFFERSIZE, buffer, &more);
        printf("%s\n", buffer);
    }
    while(more);
\endverbatim
 */
#define FLDECODEFLAGS_RAWMODE  1
  extern int FL_Decode(FlightRecorderRegistryList_t* logregistry, size_t bufferSize, char* buffer, uint64_t* moreData, uint64_t flags);

#ifdef __cplusplus
}
#endif

#endif /* _FLIGHTLOG_H_ */
