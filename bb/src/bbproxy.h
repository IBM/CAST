/*******************************************************************************
 |    bbproxy.h
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
 * \file bbproxy.h
 * This file is the burst buffer proxy header file.
 */

#ifndef BB_BBPROXY_H_
#define BB_BBPROXY_H_

#include <sys/types.h>

#include "bbinternal.h"
#include "Uuid.h"

const ssize_t SECTOR_SIZE = 512;

const std::string DEFAULT_VOLUME_GROUP_NAME = "bb";

const useconds_t DEFAULT_DELAY_BETWEEN_CREATE_RETRIES = 0;
const useconds_t DEFAULT_DELAY_PRIOR_TO_INIT_FS = 500000;
const int DEFAULT_MASTER_LOGICAL_VOLUME_NUMBER = 0;
const int DEFAULT_NUMBER_OF_CREATE_RETRIES = 2;
const int DEFAULT_ON_START_LOGICAL_VOLUME_CLEANUP = 1;
const ssize_t MINIMUM_LOGICAL_VOLUME_NUMBER_OF_SECTORS = 132*1024*1024/SECTOR_SIZE;
const int DEFAULT_LOGBLOCKSIZE=16; //Default Block Size of 2**16 is 65536 bytes


/**
 * \class LogicalVolumeNumber
 * Uniquely identifies each logical volume.
 */
class LogicalVolumeNumber {
public:
	// Non-static methods

    /**
     * \brief Returns logical volume number
     *
     * \return   int32_t Logical volume number
     */
	inline int32_t get() { return value; };

    /**
     * \brief Increments and returns logical volume number
     *
     * \return   int32_t Next logical volume number
     */
	inline int32_t getNext() { return incr(); };

    /**
     * \brief Increments the message number object.
     */
	int32_t incr();

    /**
     * \brief Sets the logical volume number
     *
     * Sets the logical volume number to the input value.
     * If the input value is negative, the logical volume
     * number is set to zero.
     *
     * \param[in]   pValue Input value
     */
	inline void set(const int32_t pValue) {
	    if (pValue > 0) {
	        value = pValue;
	    } else {
	        value = 0;
	    }

	    return;
	}

	// Constructors

    /**
     * \brief Default constructor
     *
     * Sets the logical volume number to zero.
     */
	LogicalVolumeNumber() : value(0) {};

    /**
     * \brief Constructor with starting value
     *
     * Sets the logical volume number to the input value.
     *
     * \param[in]   pValue Starting value
    */
	LogicalVolumeNumber(int32_t pValue) : value(pValue) {};

    /**
     * \brief Destructor
     */
	virtual ~LogicalVolumeNumber() {};

	// Data members
	int32_t value;      //! Logical volume number value
};


/**
 * \class LV_Data
 * Uniquely identifies each burst buffer logical volume.
 */
class LV_Data {
public:
	// Constructors

    /**
     * \brief Default constructor
     */
	LV_Data() : mountpoint("")
	{
	    lvuuid = Uuid();
	    jobid = UNDEFINED_JOBID;
	    groupid = (gid_t)-1;
	    userid = (uid_t)-1;
	};

    /**
     * \brief Constructor with specified values
     *
     * \param[in]   pValue Starting value
    */
	LV_Data(std::string pMountPoint, Uuid& pUuid, uint64_t pJobId, gid_t pGroupId, uid_t pUserId) :
	    mountpoint(pMountPoint)
    {
        lvuuid = pUuid;
        jobid = pJobId;
        groupid = pGroupId;
        userid = pUserId;
	};

    /**
     * \brief Destructor
     */
	virtual ~LV_Data() {};

	// Data members
	std::string mountpoint;     //! Mount point for logical volume device
	Uuid        lvuuid;         //! UUID for logical volume
	uint64_t    jobid;          //! JobId for logical volume
	gid_t       groupid;        //! GroupId of mountpoint at create logical volume time
	uid_t       userid;         //! UserId of mountpoint at create logical volume time
};


/**
 \brief Change Owner
 \par Description
 Changes the uid/gid for a path under sudo.

 \param[in] pPathName Path name.
 \param[in] pNewOwner New owner gid_t.
 \param[in] pNewGroup New group uid_t.
 \return Error code
 \retval 0 Success
 \retval -1 Error condition
 \ingroup bbproxy
 */
int changeOwner(const char* pPathName, const uid_t pNewOwner, const gid_t pNewGroup);

/**
 \brief Create Logical Volume
 \par Description
 Performs the preparatory work before actually creating a logical volume.

 \param[in] pOwner owner uid_t.
 \param[in] pGroup group gid_t.
 \param[in] pMountPoint After creating the logical volume and initializing a file system, the file system will be mounted to this mount point.
 \param[in] pMountSize Size for the logical volume.
 \param[in] pFlags Flags for the create operation, including the file system type.  See BBCREATEFLAGS.
 \return Error code
 \retval 0 Success
 \retval -3 Unexpected error condition
 \retval -19 Mount point already mounted.
 \retval -20 Unexpected error condition when attempting to determine if mount point was already mounted.
 \retval -21 Mount size invalid.
 \retval -44 Insufficient free space
 \retval -45 Insufficient suitable contiguous allocatable extents for logical volume
 \ingroup bbproxy
 */
int createLogicalVolume(const uid_t pOwner, const gid_t pGroup, const char* pMountPoint, const char* pMountSize, const uint64_t pFlags);

/**
 \brief Do Change Logical Volume
 \par Description
 Changes options for a logical volume.

 \param[in] pVolumeGroupName Volume group name.
 \param[in] pDevName Device name.
 \param[in] pOptions Char array of options and values for those options.
 \return Error code
 \retval 0 Success
 \retval -2 Unexpected error condition
 \ingroup bbproxy
 */
int doChangeLogicalVolume(const char* pVolumeGroupName, const char* pDevName, const char* pOptions);

/**
 \brief Do Change Owner
 \par Description
 Changes the uid/gid for a path.

 \param[in] pPathName Path name.
 \param[in] pNewOwner New owner.
 \param[in] pNewGroup New group.
 \return Error code
 \retval 0 Success
 \retval -1 Error condition
 \ingroup bbproxy
 */
int doChangeOwner(const char* pPathName, const uid_t pNewOwner, const gid_t pNewGroup);

/**
 \brief Do Change Owner
 \par Description
 Changes the uid/gid for a path.

 \param[in] pPathName Path name.
 \param[in] pNewOwner New owner.
 \param[in] pNewGroup New group.
 \return Error code
 \retval 0 Success
 \retval -1 Error condition
 \ingroup bbproxy
 */
int doChangeOwner(const char* pPathName, const char* pNewOwner, const char* pNewGroup);

/**
 \brief Do Create Logical Volume
 \par Description
 Create a logical volume.

 \param[in] pVolumeGroupName Volume group name.
 \param[in] pDevName Device name.
 \param[in] pMountSize Size for the logical volume.
 \param[in] pOptions Char array of options and values for those options.
 \return Error code
 \retval 0 Success
 \retval -3 Unexpected error condition
 \retval -44 Insufficient free space
 \retval -45 Insufficient suitable contiguous allocatable extents for logical volume
 \ingroup bbproxy
 */
int doCreateLogicalVolume(const char* pVolumeGroupName, const char* pDevName, const char* pMountSize, const char* pOptions);

/**
 \brief Do Initialize File System
 \par Description
 Initialze a file system.

 \param[in] pVolumeGroupName Volume group name.
 \param[in] pDevName Device name.
 \param[in] pMountFlags Flags passed on the create logical volume.
 \param[in] pLogBlockSize log 2 block size for xfs.  Note mkfs.xfs restrictions apply
 \return Error code
 \retval 0 Success
 \retval -4 Unexpected error condition
 \retval -5 Unexpected/invalid flag value
 \retval -6 Unexpected/invalid device name
 \ingroup bbproxy
 */
int doInitializeFileSystem(const char* pVolumeGroupName, const char* pDevName, const uint64_t pMountFlags, const int pLogBlockSize);

/**
 \brief Do Mount
 \par Description
 Mount a file system.

 \param[in] pVolumeGroupName Volume group name.
 \param[in] pDevName Device name.
 \param[in] pMountPoint Mount point.
 \return Error code
 \retval 0 Success
 \retval -7 Unexpected error condition
 \ingroup bbproxy
 */
int doMount(const char* pVolumeGroupName, const char* pDevName, const char* pMountPoint);

/**
 \brief Do Remove Logical Volume
 \par Description
 Remove a logical volume.

 \param[in] pVolumeGroupName Volume group name.
 \param[in] pDevName Device name.
 \return Error code
 \retval 0 Success
 \retval -8 Unexpected error condition
 \ingroup bbproxy
 */
int doRemoveLogicalVolume(const char* pVolumeGroupName, const char* pDevName);

/**
 \brief Do Resize File System
 \par Description
 Resize a file system.

 \param[in] pVolumeGroupName Volume group name.
 \param[in] pDevName Device name.
 \param[in] pFileSysType File system type.
 \param[in] pMountPoint Mount point for the logical volume to be resized.
 \param[in] pMountSize Size for the logical volume.
 \param[in] pFlags Flags for the resize operation.  See BBRESIZEFLAGS.
 \return Error code
 \retval 0 Success
 \retval -9 Unexpected error condition
 \retval -35 Logical volume contains an unsupported file type for resize
 \retval -42 Cannot shrink an XFS file systme and preserve the file system
 \ingroup bbproxy
 */
int doResizeFileSystem(const char* pVolumeGroupName, const char* pDevName, const char* pFileSysType, const char* pMountPoint, const char* pMountSize, const uint64_t pFlags);

/**
 \brief Do Resize Logical Volume
 \par Description
 Resize a logical volume.

 \param[in] pVolumeGroupName Volume group name.
 \param[in] pDevName Device name.
 \param[in] pMountSize Size for the logical volume.
 \param[in] pResizeFileSystemWithLogicalVolume Flag indicating if the file system should be resized in concert via LVM with the logical volume.
 \return Error code
 \retval 0 Success
 \retval -10 Unexpected error condition
 \retval -41 fsadm failed
 \retval -46 Insufficient free space
 \retval -47 Insufficient suitable contiguous allocatable extents for logical volume
 \ingroup bbproxy
 */
int doResizeLogicalVolume(const char* pVolumeGroupName, const char* pDevName, const char* pMountSize, const bool pResizeFileSystemWithLogicalVolume);

/**
 \brief Do Unmount
 \par Description
 Unmount a file system.

 \param[in] pDevName Device name.
 \param[in] pMountPoint Mount point.
 \return Error code
 \retval 0 Success
 \retval -11 Unexpected error condition during the unmount
 \ingroup bbproxy
 */
int doUnmount(const char* pDevName, const char* pMountPoint);

/**
 \brief Find Alpha Value
 \par Description
 Returns the position in the input line where the first alpha character exists.

 \param[in] pLine Input line.
 \param[in] pLength Length of input line.
 \return Address of first alpha character in the input array
 \ingroup bbproxy
 */
char* findAlphaValue(char* pLine, const size_t pLength);

//ssize_t getBlockSize(const char* pVolumeGroupName, const char* pDevName, const char* pFileSysType);

/**
 \brief Find Numeric Value
 \par Description
 Returns the position in the input line where the first numeric character exists.

 \param[in] pLine Input line.
 \param[in] pLength Length of input line.
 \return Address of first numeric character in the input array
 \ingroup bbproxy
 */
ssize_t findNumericValue(const char* pLine, const size_t pLength);

/**
 \brief Get New Size in Sectors
 \par Description
 Calculate the input new size in terms of sectors.

 \param[in] pNewSize New size.
 \param[in] pCurrentSize Current size.
 \return Number of sectors
 \ingroup bbproxy
 */
ssize_t getNewSizeInSectors(const char* pNewSize, const ssize_t pCurrentSize);

/**
 \brief Get Number of Allocated Sectors
 \par Description
 Get the number of currently allocated sectors.

 \param[in] pVolumeGroupName Volume group name.
 \param[in] pDevName Device name.
 \return Number of allocated sectors
 \ingroup bbproxy
 */
ssize_t getNumberOfAllocatedSectors(const char* pVolumeGroupName, const char* pDevName);

//ssize_t getNumberOfBlocksAllocated(const char* pVolumeGroupName, const char* pDevName);

/**
 \brief Get Number of Sectors
 \par Description
 Get the number of sectors fo rthe input new size.

 \param[in] pNewSize New size.
 \return Number of sectors
 \ingroup bbproxy
 */
ssize_t getNumberOfSectors(const char* pNewSize);

/**
 \brief Get User and Group Ids
 \par Description
 Gets the user and group ids for a path.

 \param[in] pPathName Path name.
 \param[in/out] pUserId Returned uid_t.
 \param[in/out] pGroupId Returned gid_t.
 \return Error code
 \retval 0 Success
 \retval -1 Error condition
 \ingroup bbproxy
 */
int getUserAndGroupIds(const char* pPathName, uid_t& pUserId, gid_t& pGroupId);

/**
 \brief Is Mounted
 \par Description
 Check if a logical volume is 'mounted'.

 \param[in] pVolumeGroupName Volume group name.
 \param[in] pLogicalVolume Logical volume name.
 \return Return code
 \retval 0 Not mounted
 \retval 1 Mounted
 \ingroup bbproxy
 */
int isMounted(const char* pVolumeGroupName, const char* pLogicalVolume);

/**
 \brief Is Mounted
 \par Description
 Check if a mount point is mounted.

 \param[in] pMountPoint Mount point.
 \param[in] pDevName Device name.
 \param[in] pDevNameLength Device name length.
 \param[out] pFileSysType Returned file system type.
 \param[out] pFileSysTypeLength Length of space to return file system type.
 \return Error code
 \retval 0 Not mounted
 \retval 1 Is mounted
 \retval -12 Unexpected error when attempting to determine if the mount point is mounted
 \retval -13 Mount point does not exist
 \retval -14 Mount point not specified
 \ingroup bbproxy
 */
int isMounted(const char* pMountPoint, char* &pDevName, const size_t &pDevNameLength, char* &pFileSysType, const size_t &pFileSysTypeLength);

/**
 \brief Logical Volume Cleanup
 \par Description
 Cleanup code for a partially 'created' logical volume.

 \ingroup bbproxy
 */
void logicalVolumeCleanup();

/**
 \brief Logical Volume Exists
 \par Description
 Check for the existence of a logical volume.

 \param[in] pVolumeGroupName Volume group name.
 \param[in] pDevName Device name.
 \return Return code
 \retval 0 Logical volume does not exist
 \retval 1 Logical volume exists
 \retval -15 Unexpected error condition
 \retval -16 Unexpected output from lvdisplay
 \retval -17 Unexpected output from lvdisplay
 \retval -18 Unexpected error condition from from lvdisplay
 \ingroup bbproxy
 */
int logicalVolumeExists(const char* pVolumeGroupName, const char* pDevName);

/**
 \brief Remove Logical Volume
 \par Description
 Performs the preparatory work before actually removing a logical volume.

 \param[in] pMountPoint Mount point of associated logical folume to be removed.
 \return Return code
 \retval 0 Success
 \retval -2 Unexpected error condition during change logical volume
 \retval -8 Unexpected error condition during the remove of the logical volume
 \retval -11 Unexpected error condition during the unmount
 \retval -25 Mount point is not associated with a burst buffer logical volume
 \retval -26 Mount point is not mounted
 \retval -27 Unexpected error condition when trying to determine if the mount point is mounted
 \retval -28 Invalid mount point specified
 \ingroup bbproxy
 */
int removeLogicalVolume(const char* pMountPoint);

/**
 \brief Resize Logical Volume
 \par Description
 Performs the preparatory work before actually resizing a logical volume.

 \param[in] pMountPoint Mount point of associated logical folume to be removed.
 \param[in] pLogicalVolume Logical volume name.
 \param[in] pMountSize New size for the logical volume.
 \param[in] pFlags Flags for the resize operation.  See BBRESIZEFLAGS.
 \return Error code
 \retval 0 Success
 \retval -2 Unexpected error condition when changing the logical volume
 \retval -7 Unexpected error condition when mounting the file system
 \retval -9 Unexpected error condition when resizing the file system
 \retval -10 Unexpected error condition
 \retval -11 Unexpected error condition during the unmount
 \retval -12 Unexpected error when attempting to determine if the mount point is mounted
 \retval -13 Mount point does not exist
 \retval -14 Mount point not specified
 \retval -15 Unexpected error condition when trying to determine if logical volume exists.
 \retval -16 Unexpected output from lvdisplay
 \retval -17 Unexpected output from lvdisplay
 \retval -18 Unexpected error condition from from lvdisplay
 \retval -22 Resize for mount point: Logical volume not associated with a burst buffer mount point
 \retval -23 Resize for mount point: Mount point not mounted
 \retval -24 Resize for mount point: Unexpected error when attempting to determine if the mount point is mounted
 \retval -35 Logical volume contains an unsupported file type for resize
 \retval -37 Resize for mount point: Logical volume contains a file system not supported for this resize operaiton
 \retval -38 Could not determine the allocated number of sectors for device
 \retval -39 Could not calculate a positive number of resulting sectors for the specified new size
 \retval -41 fsadm failed on resize of logical volume
 \retval -42 Cannot shrink an XFS file systme and preserve the file system
 \retval -43 Cannot shrink the size of a logical volume containing an XFS file system
 \retval -46 Insufficient free space on resize of logical volume
 \retval -47 Insufficient suitable contiguous allocatable extents for logical volume
 \retval -48 Resize logical volume: Logical volume cannot be mounted for this resize operation
 \retval -49 Resize logical volume: Unexpected error when attempting to determine if the mount point is mounted
 \retval -50 Either mount point or logical volume must be specified, but not both
 \retval -51 Either mount point or logical volume must be specified
 \retval -52 Cannot resize an unmounted logical volume and the specified new size greater than the current size
 \retval -53 Resize logical volume: Logical volume does not exist
 \ingroup bbproxy
 */
int resizeLogicalVolume(const char* pMountPoint, char* &pLogicalVolume, const char* pMountSize, const uint64_t pFlags);

#endif /* BB_BBPROXY_H_ */
