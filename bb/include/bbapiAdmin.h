/*******************************************************************************
|    bbapiAdmin.h
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
 *  \file bbapiAdmin.h
 *  This file contains the administrator burst buffer APIs.
 *  These APIs must be invoked with the root userid.  For those
 *  userids in the sudos list, see the bbapiAdminProc interfaces.
 *
 *  \defgroup bbapi Burst Buffer API
 */

#ifndef BB_BBAPIADMIN_H_
#define BB_BBAPIADMIN_H_

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


/*******************************************************************************
 | Enumerators
 *******************************************************************************/
/**
 *  \brief Burst buffer options for retrieving transfer definitions
 *  \see BB_RetrieveTransfers
 */
enum BB_RTV_TRANSFERDEFS_FLAGS
{
    ALL_DEFINITIONS                         = 0x0001,   ///< All definitions
    ONLY_DEFINITIONS_WITH_UNFINISHED_FILES  = 0x0002,   ///< Only definitions with files having outstanding extents to be transferred
    ONLY_DEFINITIONS_WITH_STOPPED_FILES     = 0x0003    ///< Only definitions with files that are already stopped
};
typedef enum BB_RTV_TRANSFERDEFS_FLAGS BB_RTV_TRANSFERDEFS_FLAGS;


/*******************************************************************************
 | Constants
 *******************************************************************************/


/**
 *  \brief Create logical volume on the local SSD
 *  \par Description
 *  The BB_CreateLogicalVolume routine creates a Linux LVM logical volume on the local SSD. The size of the
 *  logical volume will not be smaller than the specified size. The file system type can be specified
 *  via the flags field.  The file system indicated on the flag will launch a system
 *  administrator customizable mount script.  The location of those scripts is defined by a
 *  configuration setting.
 *
 *  \warning Administrator privileges required
 *  \param[in] mountpoint   Desired mountpoint for the new logical volume
 *  \param[in] size         Size for the new logical volume.  Syntax is the same as on the LVM lvcreate command.
 *  \param[in] flags        Flags
 *    * BBXFS  - Default value. Create a logical volume and initialize an xfs file system.
 *    * BBEXT4 - Create a logical volume and initialize an ext4 file system.
 *  \return Error code
 *  \retval 0 Success
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
 */
extern int BB_CreateLogicalVolume(const char* mountpoint, const char* size, BBCREATEFLAGS flags);

/**
 *  \brief Resize a logical volume associated with a mountpoint on the local SSD
 *  \par Description
 *  The BB_ResizeMountPoint routine resizes a Linux LVM logical volume on the local SSD. The new size may be less than or
 *  greater than the current size of the logical volume.  If the new size is less than the current size, the
 *  logical volume will first be unmounted before it is resized.  If the file system is to be preserved, then the file
 *  system will be re-mounted to the same directory before the command returns.  If the new size is zero, the logical
 *  volume will be shrunk to the minimum allowed size.
 *
 *  \warning Administrator privileges required to increase the size of the logical volume.
 *  \param[in] mountpoint Mount point of the logical volume to be resized
 *  \param[in] newsize New size for the logical volume.  Syntax is the same as on the LVM lvcreate command.
 *  \param[in] flags (optional)
 *    * BB_NONE - Default value.  No special flags are specified and, by default, the file system will be preserved
 *                as part of the resize operation.
 *    * BB_DO_NOT_PRESERVE_FS - Do not preserve the file system.  Upon exit, the logical volume will be resized but
 *                              will no longer be mounted to the input mount point.
 *  \return Error code
 *  \retval 0 Success
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
 */
extern int BB_ResizeMountPoint(const char* mountpoint, const char* newsize, BBRESIZEFLAGS flags);

/**
 *  \brief Remove logical volume on the local SSD
 *  \par Description
 *  The BB_RemoveLogicalVolume routine removes a Linux LVM logical volume on the local SSD. Any remaining
 *  content on the logical volume will be wiped from the drive. The SSD discard routine may be used
 *  to reclaim the physical blocks.
 *
 *  \note When the logical volume is removed, SSD usage statistics are transmitted to CSM. They will no longer
 *  by available by BB_GetUsage() for mountpoint.
 *  After BB_RemoveLogicalVolume() all metadata (by tags) about transfers associated with this
 *  mountpoint will be purged from bbProxy.
 *  Any active transfers to this logical volume will be cancelled.
 *  Any transfers started after/during the removal of the logical volume will fail.
 *
 *  \warning Administrator privileges required
 *  \param[in] mountpoint Desired mountpoint for the new logical volume
 *  \return Error code
 *  \retval 0 Success
 *  \retval -1 Failure
 *  \retval -2 Mountpoint may not exist
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
 */
extern int BB_RemoveLogicalVolume(const char* mountpoint);

/**
 *  \brief Create Directory
 *  \par Description
 *  The BB_CreateDirectory routine creates a directory on the compute node's SSD logical volume.
 *
 *  \note When used from the compute node, this routine is functionally identical to mkdir.  However, the intention
 *  was to provide a bbAPI routine for bbCmd to target from FEN during stage-in / stage-out scripts. It is presumed
 *  that compute node applications will prefer to use mkdir().  This API only handles mkdir locally on the compute
 *  node's SSD logical volume.  FEN-based scripts should use mkdir for GPFS storage.
 *
 *  \param[in] newpathname Pathname for the new directory
 *  \return Error code
 *  \retval 0 Success
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
 */
extern int BB_CreateDirectory(const char* newpathname);

/**
 *  \brief Change Owner
 *  \par Description
 *  The BB_ChangeOwner routine changes the ownership of the specified file or directory.
 *
 *  \param[in] pathname Pathname for the file or directory
 *  \param[in] newuser  Username of the new owner
 *  \param[in] newgroup Groupname of the new owner
 *  \return Error code
 *  \retval 0 Success
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
 */
extern int BB_ChangeOwner(const char* pathname, const char* newuser, const char* newgroup);

/**
 *  \brief Change Mode
 *  \par Description
 *  The BB_ChangeOwner routine changes the ownership of the specified file or directory.
 *
 *  \param[in] pathname Pathname for the directory
 *  \param[in] mode New mode for the file or directory
 *  \return Error code
 *  \retval 0 Success
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
 */
extern int BB_ChangeMode(const char* pathname, mode_t mode);

/**
 *  \brief Remove Directory
 *  \par Description
 *  The BB_RemoveDirectory routine removes a directory on the compute node's SSD logical volume.
 *
 *  \note When used from the compute node, this routine is functionally identical to rmdir.  However, the intention
 *  was to provide a bbAPI routine for bbCmd to target from FEN during stage-in / stage-out scripts. It is presumed
 *  that compute node applications will prefer to use rmdir().  This API only handles rmdir locally on the compute
 *  node's SSD logical volume.  FEN-based scripts should use rmdir for GPFS storage.
 *
 *  \param[in] pathname Pathname for the directory to be removed
 *  \return Error code
 *  \retval 0 Success
 *  \retval -1 Failure
 *  \retval -2 No such directory
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
 */
extern int BB_RemoveDirectory(const char* pathname);

/**
 *  \brief Remove Job Information
 *  \par Description
 *  The BB_RemoveJobInfo routine removes job information from the metadata being maintained on the bbServers
 *  for the current job.
 *
 *  \return Error code
 *  \retval 0 Success
 *  \retval -1 Failure
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 *  \ingroup bbapi
 */
extern int BB_RemoveJobInfo();

/**
 * \brief Query bbproxy about bbserver information.
 * \par Description
 * Query for active, primary, or backup bbserver info.
 *
 * \param[in] BBServerQuery See values in bbapi_types.h for this enum.
 * \param[in] bufsize Size of the buffer provided for the response information
 * \param[in] buffer Points to memory for the response
 *  \return Error code
 *  \retval 0 Success
 *  \retval -1 Failure
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 **/
extern int BB_QueryServer(enum BBServerQuery, size_t bufsize, char* buffer);

/**
 * \brief Request bbproxy to switch to a different bbserver
 * \par Description
 * Switch to an inactive bbserver described by buffer from BB_QueryServer
 *
 * \param[in] buffer Contains an inactive bbserver config returned by BB_QueryServer
 *  \return Error code
 *  \retval 0 Success
 *  \retval -1 Failure
 *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
 **/
extern int BB_SwitchServer(const char* buffer);

/**
 *  \brief Suspends the transfer of data for transfer definitions that had previously been started on
 *         one or more bbServers and also prevents additional transfer definitions from being started from
 *         that CN hostname to the servicing bbServer.
 *  \par Description
 *  The BB_Suspend routine allows the user to suspend the transfer of data for transfer definitions that
 *  were previously started on one or more bbServers.  Additional transfer definitions are also prevented
 *  from being started for that CN hostname to the serviceing bbServer.
 *
 *  \param[in]     pHostName            Only transfer definitions originating from this
 *                                      CN hostname value are returned.  "*" can be specified to
 *                                      return transfer definitions for all hostnames.
 *                                      "" can be specified to only return transfer definitions
 *                                      for the hostname associated with servicing bbproxy.
 *  \return Error code
 *  \retval 0  Success
 *  \retval -1 Failure
 */
extern int BB_Suspend(const char* pHostName);

/**
 *  \brief Resumes the transfer of data for transfer definitions that had previously been suspended on
 *         one or more bbServers and also allows additional transfer definitions to be started from
 *         that CN hostname to the servicing bbServer.
 *  \par Description
 *  The BB_Resume routine allows the user to resume the transfer of data for transfer definitions that
 *  were previously suspended on one or more bbServers.  Additional transfer definitions are also allowed
 *  to be started for that CN hostname to the serviceing bbServer.
 *
 *  \param[in]     pHostName    Only transfer definitions originating from this
 *                              CN hostname value are returned.  "*" can be specified to
 *                              return transfer definitions for all hostnames.
 *                              "" can be specified to only return transfer definitions
 *                              for the hostname associated with servicing bbproxy.
 *  \return Error code
 *  \retval 0  Success
 *  \retval -1 Failure
 */
extern int BB_Resume(const char* pHostName);

/**
 *  \brief Retrieves transfer definitions from already started transfer definitions on
 *         one or more bbServers.
 *  \par Description
 *  The BB_RetrieveTransfers routine allows the user to retrieve the transfer definitions from
 *  one or more bbServers.  Selection criteria is used to determine which transfer definitions
 *  are candidates to be retrieved.  The selection criteria is an ANDing of hostname, jobid,
 *  jobstepid, handle, and contribid values. The jobid, jobstepid, and contribid values are
 *  determined from the processes' associated environment variables.  The output is returned
 *  as a boost archieve of the object defined by BB_TransferDefs and in the GetLastErrorDetails
 *  output field out.transferdefs.
 *
 *  \param[in]     pHostName            Only transfer definitions originating from this
 *                                      CN hostname value are returned.  "*" can be specified to
 *                                      return transfer definitions for all hostnames.
 *                                      "" can be specified to only return transfer definitions
 *                                      for the CN hostname associated with issuing bbproxy.
 *  \param[in]     pHandle              Only transfer definitions for this specified
 *                                      handle value are returned.  UNDEFINED_HANDLE can be specified to
 *                                      return transfer definitions for all handle values.
 *  \param[in]     pFlags               Options
 *    * ALL_DEFINITIONS                 - All definitions containing all of the original files are returned.
 *    * ONLY_DEFINITIONS_WITH_UNFINISHED_FILES
 *                                      - Only those definitions that have files with extents not yet
 *                                        transferred are returned.
 *  \param[out]    pNumTransferDefs     Number of transfer definitions in the returned buffer.
 *  \param[out]    pTransferDefsSize    Number of bytes available for returned transfer definitions.
 *  \param[in]     pBufferSize          Number of bytes provided in the buffer for returned transfer definitions.
 *  \param[in]     pBuffer              Buffer for returned transfer definitions.
 *  \return Error code
 *  \retval 0  Success
 *  \retval -1 Failure
 */
extern int BB_RetrieveTransfers(const char* pHostName, const uint64_t pHandle, const BB_RTV_TRANSFERDEFS_FLAGS pFlags, uint32_t* pNumTransferDefs, size_t* pTransferDefsSize, const size_t pBufferSize, char* pBuffer);

/**
 *  \brief Stops the transfer of data for transfer definitions on one or more bbServers.
 *  \par Description
 *  The BB_StopTransfers routine allows the user to stop the transfer of data for transfer definitions that
 *  were previously started on one or more bbServers.  The transfer definitions to consider is the
 *  output from BB_RetrieveTransfers.  Selection criteria is used to determine which transfer definitions are
 *  candidates to be stopped.  The selection criteria is an ANDing of hostname, jobid, jobstepid, handle,
 *  and contribid values.  The jobid, jobstepid, and contribid values are determined from the processes'
 *  associated environment variables.
 *
 *  \param[in]     pHostName                Only transfer definitions originating from this
 *                                          hostname value are returned.  "*" can be specified to
 *                                          return transfer definitions for all hostnames.
 *                                          "" can be specified to only return transfer definitions
 *                                          for the hostname associated with servicing bbproxy.
 *  \param[in]     pHandle                  Only transfer definitions for this specified
 *                                          handle value are returned.  UNDEFINED_HANDLE can be specified to
 *                                          return transfer definitions for all handles.
 *  \param[out]    pNumStoppedTransferDefs  Number of transfer definitions that were already found to be stopped plus those
 *                                          stopped by this operation.
 *  \param[in]     pTransferDefs            Output from BB_RetrieveTransfers.
 *  \param[in]     pTransferDefsSize        Size in bytes of the output from BB_RetrieveTransfers.
 *  \return Error code
 *  \retval 0  Success
 *  \retval -1 Failure
 */
extern int BB_StopTransfers(const char* pHostName, const uint64_t pHandle, uint32_t* pNumStoppedTransferDefs, const char* pTransferDefs, const size_t pTransferDefsSize);

/**
 *  \brief Restarts the transfer of data for transfer definitions that had previously been stopped on
 *         one or more bbServers.
 *  \par Description
 *  The BB_RestartTransfers routine allows the user to restart the transfer of data for transfer definitions that
 *  were previously stopped on one or more bbServers.  The transfer definitions to consider for restart is the
 *  output from BB_RetrieveTransfers.  Selection criteria is used to determine which transfer definitions are
 *  candidates to be restarted.  The selection criteria is an ANDing of hostname, jobid, jobstepid, handle,
 *  and contribid values.  The jobid, jobstepid, and contribid values are determined from the processes'
 *  associated environment variables.
 *
 *  \param[in]     pHostName                    Only transfer definitions originating from this
 *                                              CN hostname value are returned.  "*" can be specified to
 *                                              return transfer definitions for all hostnames.
 *                                              "" can be specified to only return transfer definitions
 *                                              for the hostname associated with servicing bbproxy.
 *  \param[in]     pHandle                      Only transfer definitions for this specified
 *                                              handle value are returned.  UNDEFINED_HANDLE can be specified to
 *                                              return transfer definitions for all handles.
 *  \param[out]    pNumRestartedTransferDefs    Number of transfer definitions restarted by this operation.
 *  \param[in]     pTransferDefs                Output from BB_RetrieveTransfers.
 *  \param[in]     pTransferDefsSize            Size in bytes of the output from BB_RetrieveTransfers.
 *  \return Error code
 *  \retval 0  Success
 *  \retval -1 Failure
 */
extern int BB_RestartTransfers(const char* pHostName, const uint64_t pHandle, uint32_t* pNumRestartedTransferDefs, const char* pTransferDefs, const size_t pTransferDefsSize);

    /**
     * \brief Query bbproxy about a bbserver
     * \par Description
     * Get the list of bbservers connected the the bbproxy
     *
     * \param[in] bbserverName  The JSON name bb.<server name>
     * \param[in] type  The type of list
     * -"waitforreplycount" Get the number of waiters for reply for connection bbserverName
     * \param[in] bufsize Size of the buffer provided for the response information
     * \param[in] buffer Points to memory for the response
     *  \return Error code
     *  \retval 0 Success
     *  \retval -1 Failure
     *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
     * \Note:
     * - waitforreplycount bberror JSON out.bbserverName.waitforreplycount contains the count
     **/
    extern int BB_GetServerByName(const char* bbserverName, const char* type,size_t bufsize, char* buffer);
    
    /**
     * \brief List of connected servers for the bbproxy
     * \par Description
     * Get the list of bbservers connected the the bbproxy
     *
     * \param[in] type  The type of list
     * -"all"  List all bbservers connected
     * -"active" Which bbserver is in use
     * -"ready" Which bbserver connections are ready for activation
     * -"backup" Which bbserver is configured as backup for the bbproxy
     * -"primary" Which bbserver is configured for initial use for bbproxy
     * \param[in] bufsize Size of the buffer provided for the response information
     * \param[in] buffer Points to memory for the response
     *  \return Error code
     *  \retval 0 Success
     *  \retval -1 Failure
     *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
     **/
  
    extern int BB_GetServer(const char * type, size_t bufsize, char* buffer);

    /**
     * \brief Request bbproxy for a list of servers by type
     * \par Description
     * List of server names requested by type
     *
     * \param[in] type Specifiy the type of action for set.
     *-"activate" Make the bbserver in name active
     *-"offline"  Take an active bbserver offline from active use w/o disconnecting
     * \param[in] name Is the name of the bbserver for the set operation
     *  \return Error code
     *  \retval 0 Success
     *  \retval -1 Failure
     *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
     **/

    extern int BB_SetServer(const char * type, const char* name);

    /**
     * \brief Request bbproxy to connect to a bbserver
     * \par Description
     *  Create a bbproxy socket connection to a bbserver by JSON name bb.<server name>
     *
     * \param[in] name The name of the bbserver
     *  \return Error code
     *  \retval 0 Success
     *  \retval -1 Failure
     *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
     **/

    extern int BB_OpenServer(const char* name);

    /**
     * \brief Request bbproxy to disconnect from a bbserver
     * \par Description
     *  Close a bbproxy socket connection to a bbserver by JSON name bb.<server name>
     *
     * \param[in] name The name of the bbserver
     *  \return Error code
     *  \retval 0 Success
     *  \retval -1 Failure
     *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
     **/

    extern int BB_CloseServer(const char* name);

    
    extern int BB_GetServerKey(const char * key);
    extern int BB_SetServerKey(const char * key,const char * value);


    /**
     * \brief Return information on open files in bbProxy
     * \par Description
     *  
     * 
     *  \return Error code
     *  \retval 0 Success
     *  \retval -1 Failure
     *  \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
     **/
    extern int BB_GetFileInfo();


#ifdef __cplusplus
}
#endif

#endif /* BB_BBAPIADMIN_H_ */
