/*******************************************************************************
|    bbapi.h
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
 *  \file bbapi.h
 *  This file contains the user burst buffer APIs.
 *
 *  \defgroup bbapi Burst Buffer API
 */

#ifndef BB_BBAPI_H_
#define BB_BBAPI_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "bbapi_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "bbapi_version.h"

// Constants


/**
 *  \brief Initialize bbAPI
 *  \par Description
 *  The BB_InitLibrary routine performs basic initialization of the library.  During initialization,
 *  it opens a connection to the local bbProxy.
 *
 *  \param[in] contribId     Contributor Id
 *  \param[in] clientVersion bbAPI header version used when the application was compiled.
 *  \see BBAPI_CLIENTVERSIONSTR
 *
 *  \return Error code
 *  \retval 0 Success
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
 */
extern int BB_InitLibrary(uint32_t contribId, const char* clientVersion);

/**
 *  \brief Terminate library
 *  \par Description
 *  The BB_TerminateLibrary routine closes the connection to the local bbProxy and releases any internal storage,
 *  such as existing transfer definitions and handles.
 *
 *  \return Error code
 *  \retval 0 Success
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
 *
 */
extern int BB_TerminateLibrary();

/**
 *  \brief Obtain error details of the last bbAPI call
 *  The BB_GetLastErrorDetails routine provides contextual details of the last API call to help the
 *  caller determine the failure.  The failure information is returned in a C string in the format
 *  specified by the caller.
 *  The last error details are _thread local_.  Each thread has its separate and distinct copy of a "last error" string.
 *  A thread invoking a burst buffer API will get a different "last error" string than another thread invoking burst buffer APIs.
 *
 *
 *
 *  Only details from the last bbAPI call performed on that software thread are returned.  If the process
 *  is multi-threaded, the error information is tracked separately between the threads.
 *
 *  \param[in]     format                   Format of data to return.  (See BBERRORFORMAT for possible values.)
 *  \param[out]    numBytesAvail            Number of bytes available to return.
 *  \param[in]     buffersize               Size of buffer
 *  \param[in/out] bufferForErrorDetails    Pointer to buffer
 *  \return Error code
 *  \retval 0 Success
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
 */
extern int BB_GetLastErrorDetails(BBERRORFORMAT format, size_t* numAvailBytes, size_t buffersize, char* bufferForErrorDetails);

/**
 *  \brief Fetch the expected version string
 *  \par Description
 *  The BB_GetVersion routine returns the expected version string for the API.  This routine is
 *  intended for version mismatch debug.  It is not intended to generate the string to pass into BB_InitLibrary.
 *
 *  \param[in] size The amount of space provided to hold APIVersion
 *  \param[out] APIVersion The string containing the bbAPI's expected version.
 *
 *  \return Error code
 *  \retval 0 Success
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
 */
extern int BB_GetVersion(size_t size, char* APIVersion);

/**
 *  \brief Create storage for a transfer definition
 *  \par Description
 *  The BB_CreateTransferDef routine creates storage for a transfer definition. The caller
 *  provides storage for BBTransferDef_t* and BB_CreateTransferDef() will allocate and initialize
 *  storage for BBTransferDef_t.
 *
 *  \param[out] transfer User provides storage for BBTransferDef_t*
 *
 *  \return Error code
 *  \retval 0 Success
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
 */
extern int BB_CreateTransferDef(BBTransferDef_t** transfer);

/**
 *  \brief Adds files to a transfer definition
 *  \par Description
 *
 *  The BB_AddFiles routine adds a file or a directory to the transfer definition
 *
 *  \note
 *  If source and target reside on the local SSD, BB_StartTransfer() may perform a synchronous 'cp'.
 *  If source and target reside on GPFS, bbServer may perform a server-side 'cp' command rather than an NVMe over Fabrics transfer.
 *  All parent directories in the target must exist prior to BB_StartTransfer() call.
 *
 *  When adding a directory, all contents of the directory will be transferred to the target location.  If BBRecursive flags
 *  is specified, then any subdirectories will be added.  Only files present in the directories at the time of the BB_AddFiles
 *  call will be added to the transfer definition.
 *
 *  \param[in] transfer Transfer definition
 *  \param[in] source   Full path to the source file location
 *  \param[in] target   Full path to the target file location
 *  \param[in] flags    Any flags for this file.  (See BBFILEFLAGS for possible values.)
 *  \return Error code
 *  \retval 0 Success
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
 */
extern int BB_AddFiles(BBTransferDef_t* transfer, const char* source, const char* target, BBFILEFLAGS flags);

/**
 *  \brief Adds identification keys to a transfer definition
 *  \par Description
 *  The BB_AddKeys routine allows the user to add a custom key-value to the transfer.  Keys for the same tag are
 *  merged on the bbServer.  If a key is a duplicate, the result is non-deterministic as to which value will prevail.
 *
 *  \param[in] transfer Transfer definition
 *  \param[in] key      Pointer to string with the key name
 *  \param[in] value    Pointer to string with the key's value
 *  \return Error code
 *  \retval 0 Success
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
 */
extern int BB_AddKeys(BBTransferDef_t* transfer, const char* key, const char* value);

/**
 *  \brief Gets the identification keys for a transfer handle
 *  \par Description
 *  The BB_GetTransferKeys routine allows the user to retrieve the custom key-value pairs for a transfer.
 *  The invoker of this API provides the storage for the returned data using the bufferForKeyData parameter.
 *  The buffersize parameter is input as the size available for the returned data.  If not enough space is
 *  provided, then the API call fails.  The format for the returned key-value pairs is JSON.
 *
 *  \param[in]     handle           Opaque handle to the transfer
 *  \param[in]     buffersize       Size of buffer
 *  \param[in/out] bufferForKeyData Pointer to buffer
 *  \return Error code
 *  \retval 0  Success
 *  \retval -1 Failure
 *  \retval -2 Key data not returned because additional space is required to return the data
 *  \ingroup bbapi
 */
extern int BB_GetTransferKeys(BBTransferHandle_t handle, size_t buffersize, char* bufferForKeyData);

/**
 *  \brief Releases storage for BBTransferDef_t
 *  \par Description
 *  The BB_FreeTransferDef routine releases storage for BBTransferDef_t.
 *
 *  \param[in] transfer Pointer to reclaim
 *  \return Error code
 *  \retval 0 Success
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
 */
extern int BB_FreeTransferDef(BBTransferDef_t* transfer);

/**
 *  \brief Retrieves a transfer handle
 *  \par Description
 *  The BB_GetTransferHandle routine retrieves a transfer handle based upon the input criteria.
 *  If this is the first request made for this job using the input tag and contrib values, a transfer
 *  handle will be generated and returned.  Subsequent requests made for the job using the
 *  same input tag and contrib values will return the prior generated value as the transfer handle.
 *
 *  Transfer handles are associated with the current jobid and jobstepid.
 *
 *  \note If numcontrib==1 and contrib==NULL, the invoker's contributor id is assumed.
 *
 *  \param[in]  tag         User-specified tag
 *  \param[in]  numcontrib  Number of entries in the contrib[] array
 *  \param[in]  contrib     Array of contributor indexes
 *  \param[out] handle      Opaque handle to the transfer
 *  \return Error code
 *  \retval 0 Success
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
 */
extern int BB_GetTransferHandle(BBTAG tag, uint64_t numcontrib, uint32_t contrib[], BBTransferHandle_t* handle);

/**
 *  \brief Starts transfer of a file
 *  \par Description
 *  The BB_StartTransfer routine starts an asynchronous transfer between the parallel file system and local SSD
 *  for the specified transfer definition.
 *
 *  \note BB_StartTransfer will fail if the contribid for a process has already contributed to this tag.
 *  BB_StartTransfer will fail if the bbAPI process has an open file descriptor for one of the specified files.
 *
 *  \param[in] transfer   Pointer to the structure that defines the transfer
 *  \param[in] handle     Opaque handle to the transfer
 *  \return Error code
 *  \retval 0 Success
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
 */
extern int BB_StartTransfer(BBTransferDef_t* transfer, BBTransferHandle_t handle);

/**
 *  \brief Cancels an active file transfer
 *  \par Description
 *  The BB_CancelTransfer routine cancels an existing asynchronous file transfers specified by
 *  the transfer handle. When the call returns, the transfer has been stopped or an error has
 *  occurred.  As part of the cancel, any parts of the files that have been transferred will
 *  have been deleted from the PFS target location.
 *
 *  \param[in] handle   Transfer handle from BB_StartTransfer.  All transfers matching the tag will be canceled.
 *  \param[in] scope    Specifies the scope of the cancel.  (See BBCANCELSCOPE for possible values.)
 *  \return Error code
 *  \retval 0 Success
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
 */
extern int BB_CancelTransfer(BBTransferHandle_t handle, BBCANCELSCOPE scope);

/**
 *  \brief Obtain the list of transfers
 *  \par Description
 *  The BB_GetTransferList routine obtains the list of transfers within the job that match the
 *  status criteria.  BBSTATUS values are powers-of-2 so they can be bitwise OR'd together to
 *  form a mask (matchstatus).  For each of the job's transfers, this mask is bitwise AND'd
 *  against the status of the transfer and if non-zero, the transfer handle for that transfer
 *  is returned in the array_of_handles.
 *
 *  Transfer handles are associated with a jobid and jobstepid.  Only those transfer handles that were
 *  generated for the current jobid and jobstepid are returned.
 *
 *  \param[in] matchstatus Only transfers with a status that match matchstatus will be returned.  matchstatus can be a OR'd mask of several BBSTATUS values.
 *  \param[inout] numHandles Populated with the number of handles returned.  Upon entry, contains the number of handles allocated to the array_of_handles.
 *  \param[out] array_of_handles Returns an array of handles that match matchstatus.  The caller provides storage for the array_of_handles and indicates the number of available elements in numHandles.  \note If array_of_handles==NULL, then only the matching numHandles is returned.
 *  \param[out] numAvailHandles Populated with the number of handles available to be returned that match matchstatus.
 *  \return Error code
 *  \retval 0 Success
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
 */
extern int BB_GetTransferList(BBSTATUS matchstatus, uint64_t* numHandles, BBTransferHandle_t array_of_handles[], uint64_t* numAvailHandles);

/**
 *  \brief Gets the status of an active file transfer
 *  \par Description
 *  The BB_GetTransferInfo routine gets the status of an active file transfer, given the transfer handle.
 *
 *  \note If multiple files are being transferred within a tag, tags are tracked for the life of the associated job,
 *  \note and purged when the logical volume is removed.
 *
 *  \param[in]  handle      Transfer handle
 *  \param[out] info        Information on the transfer identified by the handle.  Caller provides storage for BBTransferInfo_t.
 *  \return Error code
 *  \retval 0 Success
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
 */
extern int BB_GetTransferInfo(BBTransferHandle_t handle, BBTransferInfo_t* info);

/**
 *  \brief Gets the transfer rate for a given tag
 *  \par Description
 *   The BB_GetThrottleRate routine retrieves the throttled transfer rate for the specified transfer handle.
 *
 *  \note Actual transfer rate may vary based on file server load, congestion, other BB transfers, etc.
 *
 *  \param[in] mountpoint   compute node mountpoint to retrieve the throttle rate
 *  \param[in] rate Current transfer rate throttle, in bytes-per-second
 *  \return Error code
 *  \retval 0 Success
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
 */
extern int BB_GetThrottleRate(const char* mountpoint, uint64_t* rate);

/**
 *  \brief Sets the maximum transfer rate for a given tag
 *  \par Description
 *  The BB_SetThrottleRate routine sets the upper bound on the transfer rate for the provided transfer handle.
 *
 *  \note Actual transfer rate may vary based on file server load, congestion, other BB transfers,
 *  etc. Multiple concurrent transfer handles can each have their own rate, which are
 *  additive from a bbServer perspective.  The bbServer rate won't exceed the configured
 *  maximum transfer rate.  Setting the transfer rate to zero will have the effect of
 *  pausing the transfer.
 *
 *  \param[in] mountpoint   compute node mountpoint to set a maximum rate
 *  \param[in] rate New transfer rate, in bytes-per-second
 *  \return Error code
 *  \retval 0 Success
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
 */
extern int BB_SetThrottleRate(const char* mountpoint, uint64_t rate);

/**
 *  \brief Get SSD Usage
 *  \par Description
 *  The BB_GetUsage routine returns the SSD usage for the given logical volume on the compute node.
 *  When the LSF job exits, all logical volumes created under that LSF job ID will be queried to
 *  provide a summary SSD usage for the compute node.
 *
 *  \param[in] mountpoint Logical volume to get user-level usage information
 *  \param[out] usage Usage information
 *  \return Error code
 *  \retval 0 Success
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
 */
extern int BB_GetUsage(const char* mountpoint, BBUsage_t* usage);

/**
 *  \brief Sets the usage threshold
 *  \par Description
 *  The BB_SetUsageLimit routine sets the SSD usage threshold.  When either usage->totalBytesRead or usage->totalBytesWritten is non-zero and exceeds the current threshold for the specified mount point, a RAS event will be generated.
 *  \param[in] mountpoint The path to the mount point that will be monitored
 *  \param[in] usage The SSD activity limits that should be enforced
 *  \return Error code
 *  \retval 0 Success
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
*/
extern int BB_SetUsageLimit(const char* mountpoint, BBUsage_t* usage);

/**
 *  \brief Get NVMe SSD device usage
 *  \par Description
 *  The BB_GetDeviceUsage routine returns the NVMe device statistics.
 *  \param[in] devicenum The index of the NVMe device on the compute node
 *  \param[out] usage The current NVMe device statistics
 *  \return Error code
 *  \retval 0 Success
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
*/
extern int BB_GetDeviceUsage(uint32_t devicenum, BBDeviceUsage_t* usage);


#ifdef __cplusplus
}
#endif

#endif /* BB_BBAPI_H_ */
