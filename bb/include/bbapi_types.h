/*******************************************************************************
|    bbapi_types.h
|
|  � Copyright IBM Corporation 2015,2016. All Rights Reserved
|
|    This program is licensed under the terms of the Eclipse Public License
|    v1.0 as published by the Eclipse Foundation and available at
|    http://www.eclipse.org/legal/epl-v10.html
|
|    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
|    restricted by GSA ADP Schedule Contract with IBM Corp.
*******************************************************************************/

/**
 *  \file bbapi_types.h
 *  This file contains the 'internal' user burst buffer APIs
 *  (NOTE: These are internal interfaces provided to bbcmd that are not to be
 *         made generally available.)
 */

#ifndef BB_BBAPI_TYPES_H_
#define BB_BBAPI_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "BBStatus.h"

/**
 *  \brief User-specified tag for identifying transfer to/from the burst buffer
 *  \ingroup bbapi
 */
typedef unsigned long BBTAG;

/**
 *  \brief Burst buffer flags for file transfers
 *  \see BB_AddFiles
 *  \note BundleID only applies for BSCFS files.  Regular files are not bundled.
 *  \note For BSCFS, the scope for priority is within a given BundleId.  For a normal file, the scope for priority is across all normal files in the transfer definition.
 *  \ingroup bbapi
 */
enum BBFILEFLAGS
{
    BBRecursive          = 0x0001, ///< Recursively transfer all files and subdirectories
};
typedef enum BBFILEFLAGS BBFILEFLAGS;

/**
 *  \brief Burst buffer flags for logical volume creation
 *  \see BB_CreateLogicalVolume
 *  \note Bit encoded values
 *  \ingroup bbapi
 */
enum BBCREATEFLAGS
{
    BBXFS       = 0x0001,   ///< Create xfs logical volume
    BBEXT4      = 0x0002,   ///< Create ext4 logical volume
    BBFSCUSTOM1 = 0x0100,   ///< Site custom logical value 1
    BBFSCUSTOM2 = 0x0200,   ///< Site custom logical value 2
    BBFSCUSTOM3 = 0x0400,   ///< Site custom logical value 3
    BBFSCUSTOM4 = 0x0800    ///< Site custom logical value 4
};
typedef enum BBCREATEFLAGS BBCREATEFLAGS;

/**
 *  \brief Burst buffer flags for logical volume resize
 *  \see BB_ResizeMountPoint
 *  \note Bit encoded values
 *  \ingroup bbapi
 */
enum BBRESIZEFLAGS
{   BB_NONE                 = 0x0000,
    BB_DO_NOT_PRESERVE_FS   = 0x0001,
};
typedef enum BBRESIZEFLAGS BBRESIZEFLAGS;

/**
 *  \brief Burst Buffer Cancelation Scope
 *  \see BB_CancelTransfer
 *  \ingroup bbapi
 */
enum BBCANCELSCOPE
{
    BBSCOPETRANSFER = 1,   ///< Cancel only impacts the local contribution
    BBSCOPETAG      = 2    ///< Cancel impacts all transfers associated by the tag
};
typedef enum BBCANCELSCOPE BBCANCELSCOPE;


/**
 *  \brief Burst Buffer Detailed Error format
 *  \see BB_CancelTransfer
 *  \ingroup bbapi
 */
enum BBERRORFORMAT
{
    BBERRORJSON   = 1,   ///< Output string will be in JSON format
    BBERRORXML    = 2,   ///< Output string will be in XML format
    BBERRORFLAT   = 3    ///< Output string will be in a non-hierarchical name value format
};
typedef enum BBERRORFORMAT BBERRORFORMAT;
    
    enum BBServerQuery{
        BBALLCONNECTED=0,
        BBACTIVE =1,
        BBREADY=2,
        BBBACKUP=3,
        BBPRIMARY=4,
        BBWAITFOREPLYCOUNT=5
    };
    
    typedef enum BBServerQuery BBServerQuery;

#define BBUSAGE_COUNT 1

/**
 *  \brief Burst Buffer SSD usage
 *  \ingroup bbapi
 */
typedef struct BBusage
{
    uint64_t totalBytesRead;     ///< Number of bytes written to the logical volume
    uint64_t totalBytesWritten;  ///< Number of bytes read from the logical volume

    uint64_t localBytesRead;     ///< Number of bytes written to the logical volume via compute node
    uint64_t localBytesWritten;  ///< Number of bytes read from the logical volume via compute node
    uint64_t burstBytesRead;     ///< Number of bytes written to the logical volume via burst buffer transfers
    uint64_t burstBytesWritten;  ///< Number of bytes read from the logical volume via burst buffer transfers
#if BBUSAGE_COUNT
    uint64_t localReadCount;     ///< Number of write operations to the logical volume via burst buffer transfers
    uint64_t localWriteCount;    ///< Number of read operations to the logical volume via burst buffer transfers
#endif
#ifdef __cplusplus
    BBusage()
{
   totalBytesRead=0;
   totalBytesWritten=0;
   localBytesRead=0;
   localBytesWritten=0;
   burstBytesRead=0;
   burstBytesWritten=0;
#if BBUSAGE_COUNT
   localReadCount=0;
   localWriteCount=0;
#endif
}
#endif
} BBUsage_t;

/**
 *  \brief Burst Buffer NVMe SSD device usage
 *  \ingroup bbapi
 */
typedef struct
{
    uint64_t critical_warning;    ///< Fault register for the SSD
    double   temperature;         ///< Temperature of the SSD in degrees C
    double   available_spare;     ///< Amount of SSD capacity over-provisioning that remains
    double   percentage_used;     ///< Estimate of the amount of SSD life consumed.
    uint64_t data_read;           ///< Number of bytes read from the SSD over the life of the device.
    uint64_t data_written;        ///< Number of bytes written to the SSD over the life of the device.
    uint64_t num_read_commands;   ///< Number of I/O read commands received from the compute node.
    uint64_t num_write_commands;  ///< Number of completed I/O write commands from the compute node.
    uint64_t busy_time;           ///< Amount of time that the I/O controller was handling I/O requests.
    uint64_t power_cycles;        ///< Number of power on events for the SSD.
    uint64_t power_on_hours;      ///< Number of hours that the SSD has power.
    uint64_t unsafe_shutdowns;    ///< Number of unexpected power OFF events.
    uint64_t media_errors;        ///< Count of all data unrecoverable events.
    uint64_t num_err_log_entries; ///< Number of error log entries available.
} BBDeviceUsage_t;

/**
 *  \brief opaque handle that references the transfer
 */
typedef uint64_t BBTransferHandle_t;

/**
 *  \brief Burst Buffer transfer information
 *  \see BB_GetTransferList
 *  \see BBTransferInfo_t
 *  \ingroup bbapi
 */
typedef struct
{
    BBTransferHandle_t  handle;                 ///< Transfer handle
    uint32_t            contribid;              ///< Contributor Id
    uint64_t            jobid;                  ///< Job Id
    uint64_t            jobstepid;              ///< Jobstep Id
    BBTAG               tag;                    ///< User specified tag
    uint64_t            numcontrib;             ///< Number of contributors
    BBSTATUS            status;                 ///< Current status of the transfer
    uint64_t            numreportingcontribs;   ///< Number of reporting contributors
    size_t              totalTransferKeyLength; ///< Total number of bytes required to retrieve all
                                                ///<   key:value pairs using BB_GetTransferKeys()
    size_t              totalTransferSize;      ///< Total number of bytes for the files associated with
                                                ///<   all transfer definitions
    BBSTATUS            localstatus;            ///< Current status of the transfer for the requesting contributor
    size_t              localTransferSize;      ///< Total number of bytes for the files associated with
                                                ///<   the transfer definition for the requesting contributor
} BBTransferInfo_t;

/**
 *  \brief Structure that defines the burst buffer transfer
 */
typedef void* BBTransferDef_t;

/*******************************************************************************
 | Constants
 *******************************************************************************/
#define DEFAULT_CANCELSCOPE BBSCOPETAG;
#define DEFAULT_COPTION BBXFS;
#define DEFAULT_ROPTION BB_NONE;

#ifdef __cplusplus
}
#endif

#endif /* BB_BBAPI_TYPES_H_ */
