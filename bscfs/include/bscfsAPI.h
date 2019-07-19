/******************************************************************************
 |    bscfsAPI.h
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

/**
 *  \file bscfsAPI.h
 *  This file contains the BSCFS client API.
 *
 *  \defgroup bscfsapi BSCFS client API
 */

#ifndef BSCFS_BSCFSAPI_H_
#define BSCFS_BSCFSAPI_H_

#include "bb/include/bbapi.h"

#include "bscfsAPI_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  \brief Initiate a transfer of a BSCFS file to the parallel file system.
 *  \par Description
 *  BSCFS_StartLocalFlush initiates a transfer to the PFS of any content cached
 *  locally for the BSCFS file named pathname, and adds information about this
 *  node's contribution to the shared file to mapfile. It causes cleanup_script
 *  to be invoked on the FEN if this flush is still in progress when the
 *  application terminates. It returns a handle via handle_p that can be used
 *  to query the transfer status.
 *
 *  Pathname should refer to a file in BSCFS, while mapfile should name a file
 *  in the PFS. Mapfile may be null, in which case no mapping information for
 *  this node will be recorded for this transfer. Mapfile will be created if it
 *  is not null and does not already exist. All nodes involved in a given flush
 *  operation should specify the same mapfile. Otherwise multiple, incomplete
 *  mapfiles will be created.
 *
 *  The value returned in the location to which handle_p points can later be
 *  passed to BSCFS_AwaitLocalTransfer when the process needs to wait for the
 *  transfer to finish. The returned handle can also be used directly with
 *  burst-buffer infrastructure routines defined in <bbAPI.h> to modify or
 *  query the transfer.
 *
 *  The pathname file must not be open in any process on the local compute node
 *  when the transfer is initiated. Typically, the file will be in MODIFIED
 *  state, and BSCFS_StartLocalFlush will change its state to FLUSHING. At this
 *  point BSCFS will append any partially-filled data block to the data file in
 *  the SSD, and it will write out the in-memory index as a separate SSD file.
 *  It will then initiate a transfer of the data and index files to the PFS by
 *  presenting the pair to the burst-buffer infrastructure as a BSCFS-type
 *  transfer bundle.
 *
 *  If the target file is already in any of the FLUSHING-related states, the
 *  routine will simply return the handle representing the transfer that was
 *  already set up. It will first initiate the pending transfer and change the
 *  state to FLUSHING if the initial state was FLUSH_PENDING.
 *
 *  It may seem logical to do nothing for a flush request that targets an
 *  INACTIVE file (because there is no local content that needs flushing), but
 *  in fact BSCFS will create an empty data file and index and then proceed to
 *  flush the empty files normally. The reason for this choice is that it
 *  allows a node to participate in the flush protocol even when it did not
 *  actually have anything to contribute to the shared file. Without this
 *  choice, the application would have to handle the "no contribution"
 *  situation as a special case, which might complicate its algorithm. Of
 *  course, the application is free to avoid unnecessary flushes if it is able
 *  to do so.
 *
 *  A flush request that targets a PREFETCHING or STABLE file will fail with an
 *  error indication.
 *
 *  If cleanup_script is not NULL, it should name a user-supplied program,
 *  accessible on the FEN, that may be invoked by the stage-out script after
 *  the job terminates. The script will be executed if and only if the transfer
 *  of the target file is still in progress when the application exits. If it
 *  is needed, cleanup_script will be invoked after all nodes' outstanding
 *  flushes for the given target file are complete. It will be called with
 *  three arguments, the PFS names of the newly-transferred shared file and the
 *  associated mapfile, and a return code indicating whether the global
 *  transfer completed successfully. It is expected that all nodes will name
 *  the same cleanup script for a given target file, but if different scripts
 *  are named, each script will be invoked exactly once.
 *
 *  \param[in]  pathname       BSCFS file to be flushed
 *  \param[in]  mapfile        mapfile to be created for this flush
 *  \param[in]  cleanup_script script to be executed on the FEN
 *  \param[out] handle_p       handle that represents this flush
 *
 *  \return Error code
 *  \retval 0     Success
 *  \retval errno Positive non-zero values correspond with errno.
 *  \ingroup bscfsapi
 */
static inline int BSCFS_StartLocalFlush(const char *pathname,
					const char *mapfile,
					const char *cleanup_script,
					BBTransferHandle_t *handle_p)
{
    return __BSCFS_StartLocalFlush(pathname, mapfile, cleanup_script,
				   handle_p);
}

/**
 *  \brief Prepare for a transfer of a BSCFS file to the parallel file system.
 *  \par Description
 *  BSCFS_PrepareLocalFlush prepares for, but does not initiate, a transfer to
 *  the PFS of any content cached locally for the BSCFS file named pathname,
 *  with information about this node's contribution to the shared file to be
 *  added to mapfile. It causes transfer to be initiated from the FEN if this
 *  flush operation is still pending when the application terminates, and in
 *  that case it also causes cleanup_script to be invoked on the FEN when the
 *  transfer eventually completes.
 *
 *  The arguments and operation of BSCFS_PrepareLocalFlush are identical to
 *  those of BSCFS_StartLocalFlush defined in the previous section, except that
 *  the background transfer of the target file is not actually started and no
 *  handle is returned. The transfer will remain pending until either the
 *  target file is "forgotten" (see BSCFS_Forget) or the application
 *  terminates. In the latter case, the BSCFS stage-out script running on the
 *  FEN will initiate the transfer and invoke the cleanup script when it
 *  completes. BSCFS_PrepareLocalFlush leaves the target file in FLUSH_PENDING
 *  state, unless it was already in FLUSHING or FLUSH_COMPLETED state.
 *
 *  Multiple calls to BSCFS_StartLocalFlush and BSCFS_PrepareLocalFlush for the
 *  same target file are allowed. The mapfile and cleanup_script provided with
 *  the first such call are the only ones that count. These parameters will be
 *  ignored for the second and subsequent calls for the same file. A call to
 *  BSCFS_StartLocalFlush can be used to initiate a pending transfer that was
 *  previously set up by BSCFS_PrepareLocalFlush.
 *
 *  \param[in]  pathname       BSCFS file to be flushed
 *  \param[in]  mapfile        mapfile to be created for this flush
 *  \param[in]  cleanup_script script to be executed on the FEN
 *
 *  \return Error code
 *  \retval 0     Success
 *  \retval errno Positive non-zero values correspond with errno.
 *  \ingroup bscfsapi
 */
static inline int BSCFS_PrepareLocalFlush(const char *pathname,
					  const char *mapfile,
					  const char *cleanup_script)
{
    return __BSCFS_PrepareLocalFlush(pathname, mapfile, cleanup_script);
}

/**
 *  \brief Initiate a prefetch of a BSCFS file from the parallel file system.
 *  \par Description
 *  BSCFS_StartLocalPrefetch initiates a prefetch of the BSCFS file named
 *  pathname from the PFS to the local node, using mapfile to determine what
 *  parts of the file to transfer. It returns a handle via handle_p that can be
 *  used to query the transfer status.
 *
 *  Pathname should refer to a file in BSCFS, while mapfile should name a file
 *  in the PFS. The value returned in the location pointed to by handle_p can
 *  later be passed to BSCFS_AwaitLocalTransfer when the process needs to wait
 *  for the transfer to finish.
 *
 *  The pathname file must not be open in any process on the local compute node
 *  when the BSCFS_StartLocalPrefetch call is issued. Typically, the file will
 *  be in INACTIVE state or already PREFETCHING, and it will be in PREFETCHING
 *  state as a result of the call. Initiating a prefetch on a STABLE file is
 *  not permitted, except in the case that a prefetch for the file has already
 *  been started AND awaited. In that case, the handle for the completed
 *  prefetch is returned, and the file remains STABLE. Prefetching a MODIFIED
 *  target file or one that is in any stage of FLUSHING is not permitted.
 *
 *  To initiate a prefetch, BSCFS will construct a transfer bundle (of type
 *  "BSCFS") that names the shared file and the specified mapfile as source
 *  files, and names local data and index files as targets. It will submit the
 *  bundle to the burst-buffer infrastructure and return the handle that
 *  represents the transfer to the caller. Note that a call to
 *  BSCFS_StartLocalPrefetch results only in the transfer of shared-file
 *  content destined for the local compute node, as specified by the named
 *  mapfile. Separate transfers must be initiated on each compute node to which
 *  content is directed. For prefetching, there is no requirement that the same
 *  mapfile be used on all compute nodes.
 *
 *  \param[in]  pathname       BSCFS file to be prefetched
 *  \param[in]  mapfile        mapfile to be used to direct this prefetch
 *  \param[out] handle_p       handle that represents this prefetch
 *
 *  \return Error code
 *  \retval 0     Success
 *  \retval errno Positive non-zero values correspond with errno.
 *  \ingroup bscfsapi
 */
static inline int BSCFS_StartLocalPrefetch(const char *pathname,
					   const char *mapfile,
					   BBTransferHandle_t *handle_p)
{
    return __BSCFS_StartLocalPrefetch(pathname, mapfile, handle_p);
}

/**
 *  \brief Wait for a previously-started transfer to complete.
 *  \par Description
 *  BSCFS_AwaitLocalTransfer waits for a previously-started flush or prefetch
 *  operation to complete. The handle should be one that was returned by a
 *  prior call to BSCFS_StartLocalFlush or BSCFS_StartLocalPrefetch.
 *
 *  A successful call to BSCFS_AwaitLocalTransfer indicates that the current
 *  compute node's contribution to a shared file has been incorporated into the
 *  actual file and associated mapfile on the PFS, or that the shared-file
 *  content directed to the local compute node is now available for reading. In
 *  either case, the completion refers only to file content transferred from or
 *  to the current node. It says nothing about other nodes' transfers.
 *  Prefetches by different nodes are essentially independent, so there is
 *  never a need to coordinate them globally, but the story is different for
 *  flush operations. The application is responsible for ensuring that all
 *  nodes' contributions to a shared file are transferred successfully before
 *  declaring the file complete.
 *
 *  For a flush operation, BSCFS_AwaitLocalTransfer will fail with an error
 *  indication if the target file is in any state other than one of the
 *  FLUSHING-related states. It will also report an error if the transfer fails
 *  for any reason. BSCFS_AwaitLocalTransfer leaves the file in FLUSH_COMPLETED
 *  state after waiting for the local transfer to complete.
 *
 *  For a prefetch operation, BSCFS_AwaitLocalTransfer will wait for the local
 *  transfer to complete and will then change the state of the target file from
 *  PREFETCHING to STABLE (unless it was already changed by a prior call).
 *  PREFETCHING and STABLE are the only states allowed by
 *  BSCFS_AwaitLocalTransfer for a prefetch. Prefetching differs from flushing
 *  in that there is no prefetching analog of the FLUSH_COMPLETED state. There
 *  is no requirement for global synchronization for prefetches, so the target
 *  file is considered STABLE as soon as the local prefetch completes.
 *
 *  \param[in]  handle         handle that represents the awaited transfer
 *
 *  \return Error code
 *  \retval 0     Success
 *  \retval errno Positive non-zero values correspond with errno.
 *  \ingroup bscfsapi
 */
static inline int BSCFS_AwaitLocalTransfer(BBTransferHandle_t handle)
{
    return __BSCFS_AwaitLocalTransfer(handle);
}

/**
 *  \brief Inform BSCFS that a flush operation has completed globally.
 *  \par Description
 *  BSCFS_GlobalFlushCompleted informs the local BSCFS instance that the
 *  transfer of the BSCFS file named pathname to the PFS has been completed by
 *  all participating nodes.
 *
 *  The application is responsible for ensuring that all its compute nodes'
 *  contributions to a shared file are individually transferred and
 *  incorporated into the actual file in the PFS. Once it has done so, it must
 *  make a BSCFS_GlobalFlushCompleted call on every node to let the BSCFS
 *  instances know that the shared file is available for reading.
 *
 *  The first call to BSCFS_GlobalFlushCompleted for a given file on any node
 *  should find the file in FLUSH_COMPLETED state. The call will change the
 *  state to STABLE, and subsequent calls, if any, will succeed without
 *  actually doing anything.
 *
 *  BSCFS_GlobalFlushCompleted will fail with an error indication if the target
 *  file is found to be in INACTIVE, MODIFIED, FLUSH_PENDING, FLUSHING, or
 *  PREFETCHING state.
 *
 *  \param[in]  pathname       BSCFS file to be prefetched
 *
 *  \return Error code
 *  \retval 0     Success
 *  \retval errno Positive non-zero values correspond with errno.
 *  \ingroup bscfsapi
 */
static inline int BSCFS_GlobalFlushCompleted(const char *pathname)
{
    return __BSCFS_GlobalFlushCompleted(pathname);
}

/**
 *  \brief Release all resources associated with a BSCFS file.
 *  \par Description
 *  BSCFS_Forget releases all resources associated with the BSCFS file named
 *  pathname, and returns the file to INACTIVE state.
 *
 *  BSCFS_Forget attempts to clean up no matter the state in which it finds the
 *  pathname file. It will free any data buffers associated with the file, as
 *  well as the in-memory index, and it will unlink the SSD data and index
 *  files if they exist. If the file is FLUSHING or PREFETCHING, BSCFS_Forget
 *  will attempt to cancel the outstanding burst-buffer transfer. If the file
 *  is in FLUSH_PENDING state, its planned post-job flush will be suppressed.
 *
 *  \param[in]  pathname       BSCFS file to be forgotten
 *
 *  \return Error code
 *  \retval 0     Success
 *  \retval errno Positive non-zero values correspond with errno.
 *  \ingroup bscfsapi
 */
static inline int BSCFS_Forget(const char *pathname)
{
    return __BSCFS_Forget(pathname);
}

/**
 *  \brief Return the set of internal files associated with a BSCFS file.
 *  \par Description
 *  BSCFS_QueryInternalFiles returns the names of the set of internal files
 *  associated with the BSCFS file named pathname. Files is an array of
 *  files_length bytes that is to be filled in with a NULL-separated list of
 *  file names. The list will be terminated with a double NULL. The routine
 *  also returns the current state of the file via the state_p pointer.
 *
 *  The pathname file should be in MODIFIED or STABLE state, or in any of the
 *  FLUSHING-related states. If necessary, any partially-filled data block will
 *  be appended to the SSD data file associated with the target file, and the
 *  index will be written out as a separate SSD file. The names of the index
 *  and data files, in that order, are copied into the files character array,
 *  assuming they fit in the space provided. The names are separated by a NULL
 *  character and terminated with a double NULL. Note that while the interface
 *  allows for an arbitrary number of internal files to be returned for a given
 *  target, in the initial implementation there will always be exactly two, and
 *  they will be listed with the index file first and the data file second.
 *
 *  The current state of the target file (one of the states listed above) will
 *  be stored in the location indicated by state_p if the pointer is non-null.
 *
 *  WARNING: It is the responsibility of the caller to ensure that the
 *  application does not write to the target file before the caller finishes
 *  accessing the internal files.
 *
 *  \param[in]  pathname      BSCFS file to be queried
 *  \param[in]  files_length  size (in bytes) of the files array
 *  \param[out] files         space for returned file names
 *  \param[out] state_p       returned state of the queried file
 *
 *  \return Error code
 *  \retval 0     Success
 *  \retval errno Positive non-zero values correspond with errno.
 *  \ingroup bscfsapi
 */
static inline int BSCFS_QueryInternalFiles(const char *pathname,
					   size_t files_length,
					   char *files,
					   int *state_p)
{
    return __BSCFS_QueryInternalFiles(pathname, files_length, files, state_p);
}

/**
 *  \brief Activate a BSCFS file using the provided set of internal files.
 *  \par Description
 *  BSCFS_InstallInternalFiles provides the names of a set of internal files
 *  that are to be associated with the BSCFS file named pathname. Files is an
 *  array of files_length bytes that holds a NULL-separated list of file names.
 *  The list should be terminated with a double NULL. The routine activates the
 *  target file in the given state.
 *
 *  The pathname file is expected to be INACTIVE when this call is made. For
 *  the initial implementation, files is expected to hold exactly two file
 *  names, an index file and a data file, in that order. BSCFS will incorporate
 *  the two internal files and activate the target file in the specified state,
 *  which should be either MODIFIED or STABLE. If the state is MODIFIED, the
 *  situation will be the same as if the application had written the data file
 *  content on the local compute node and not flushed it to the PFS. If the
 *  state is STABLE, the situation will be the same as if the application had
 *  prefetched the data file content from the PFS (or written and globally
 *  flushed it).
 *
 *  WARNING: It is the responsibility of the caller to provide internal files
 *  that are properly formatted and internally consistent, and also to provide
 *  files that are consistent across nodes. For example, it would be an
 *  (undetected) error to establish a target file as MODIFIED on one node and
 *  STABLE on another.
 *
 *  \param[in]  pathname      BSCFS file to be activated
 *  \param[in]  files_length  size (in bytes) of the files array
 *  \param[out] files         space for provided file names
 *  \param[out] state         state of the activated file
 *
 *  \return Error code
 *  \retval 0     Success
 *  \retval errno Positive non-zero values correspond with errno.
 *  \ingroup bscfsapi
 */
static inline int BSCFS_InstallInternalFiles(const char *pathname,
					     size_t files_length,
					     const char *files,
					     int state)
{
    return __BSCFS_InstallInternalFiles(pathname, files_length, files, state);
}

/**
 *  \brief Query for parameters used in BSCFS setup.
 *  \par Description
 *  BSCFS_GetParameter provides the capability to query settings used by the BSCFS agent on the compute node.

 *  The available parameters are:
 *  - BSCFS_MNT_PATH   The mount point of the BSCFS file system, typically /bscfs
 *  - BSCFS_PFS_PATH   The mount point of the shadowed PFS file system
 *
 *  \param[in]  parameter     BSCFS parameter name to be returned
 *  \param[in]  value_length  size (in bytes) of the value array
 *  \param[out] value         space for provided parameter value
 *
 *  \return Error code
 *  \retval 0     Success
 *  \retval errno Positive non-zero values correspond with errno.
 *  \ingroup bscfsapi
 */
static inline int BSCFS_GetParameter(const char* parameter,
						size_t value_length,
						char* value)
{
    return __BSCFS_GetParameter(parameter, value_length, value);
}


#ifdef __cplusplus
}
#endif

#endif /* BSCFS_BSCFSAPI_H_ */
