/*******************************************************************************
 |    LVUtils.cc
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

#include <grp.h>
#include <limits.h>
#include <mntent.h>
#include <pwd.h>
#include <semaphore.h>
#include <stdio.h>

#include <iostream>
#include <fstream>
#include <map>
#include <string>

#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
namespace bfs = boost::filesystem;
namespace bs  = boost::system;

#include <linux/magic.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <uuid/uuid.h>

#ifdef PROF_TIMING
#include <ctime>
#include <chrono>
#endif

#include "connections.h"
#include "bbapi.h"
#include "bbinternal.h"
#include "bbproxy.h"
#include "bbproxy_flightlog.h"
#include "BBTransferDef.h"
#include "fh.h"
#include "identity.h"
#include "logging.h"
#include "LVExtent.h"
#include "LVLookup.h"
#include "Msg.h"
#include "usage.h"
#include "nodecontroller.h"


#ifndef XFS_SUPER_MAGIC
#define XFS_SUPER_MAGIC 0x58465342
#endif

char MINIMUM_LOGICAL_VOLUME_SIZE[4] = "16M";

extern thread_local uid_t threadLocaluid;
extern thread_local gid_t threadLocalgid;

extern LogicalVolumeNumber MasterLogicalVolumeNumber;


int doChangeLogicalVolume(const char* pVolumeGroupName, const char* pDevName, const char* pOptions) {
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;

    int l_Length = 1+strlen(pVolumeGroupName)+1+strlen(DEVICE_DIRECTORY)+1+strlen(pDevName)+1;
    char* l_DevName = new char[l_Length];
    snprintf(l_DevName, l_Length, "/%s/%s/%s", DEVICE_DIRECTORY, pVolumeGroupName, pDevName);

    char l_Cmd[1024] = {'\0'};
    snprintf(l_Cmd, sizeof(l_Cmd), "lvchange %s %s 2>&1;", pOptions, l_DevName);

    for (auto& l_Line : runCommand(l_Cmd)) {
        // No expected output...

        vector<std::string> l_Output1 = {"File descriptor", "leaked on lvchange invocation."};
        if (!fuzzyMatch(l_Line, l_Output1)) {
            // \todo - This could be part of the removal for a logical volume that should 'always' work...
            //         Need to investigate this path more to allow for successful removal...  @DLH

            if (l_Line.size() > 1) {
                LOG(bb,error) << l_Line;
            } else {
                LOG(bb,error) << std::hex << std::uppercase << setfill('0') << "One byte rc from lvchange: 0x" << setw(2) << l_Line[0] << setfill(' ') << std::nouppercase << std::dec;
            }
            rc = -1;
        }
    }

    if (l_DevName) {
        delete[] l_DevName;
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}


int doChangeOwner(const char* pPathName, const uid_t pNewOwner, const gid_t pNewGroup) {
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;
    stringstream errorText;

    rc = chown(pPathName, pNewOwner, pNewGroup);
    if(rc)
    {
        rc = errno;
        errorText << "chown failed";
        bberror << err("error.pathname", pPathName) << err("error.uid", pNewOwner) << err("error.gid", pNewGroup);
        LOG_ERROR_TEXT_ERRNO(errorText, errno);
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}


int doChangeOwner(const char* pPathName, const char* pNewOwner, const char* pNewGroup) {
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;
    stringstream errorText;

    struct passwd* pwd;
    struct group*  grp;

    pwd = getpwnam(pNewOwner);
    if(pwd == NULL)
    {
        rc = -1;
        bberror << err("error.pathname", pPathName) << err("error.newowner", pNewOwner) << err("error.newgroup", pNewGroup);
        errorText << "getpwnam failed";
        LOG_ERROR_TEXT_RC(errorText, rc);
    }

    grp = getgrnam(pNewGroup);
    if(grp == NULL)
    {
        rc = -1;
        bberror << err("error.pathname", pPathName) << err("error.newowner", pNewOwner) << err("error.newgroup", pNewGroup);
        errorText << "getgrnam failed";
        LOG_ERROR_TEXT_RC(errorText, rc);
    }

    // NOTE: If doChangeOwner fails, it sets bberror
    if (!rc) rc = doChangeOwner(pPathName, pwd->pw_uid, grp->gr_gid);

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}


int doCreateLogicalVolume(const char* pVolumeGroupName, const char* pDevName, const char* pMountSize, const char* pOptions) {
    ENTRY(__FILE__,__FUNCTION__);
    int rc = -4;

    char l_Options[128] = {'\0'};
    int32_t l_ReadAhead = config.get(process_whoami + ".lvreadahead", DEFAULT_LOGICAL_VOLUME_READAHEAD);
    if (l_ReadAhead < 0) {
        snprintf(l_Options, sizeof(l_Options), "--readahead auto --activate n --zero n --permission rw");
    } else {
        snprintf(l_Options, sizeof(l_Options), "--readahead %d --activate n --zero n --permission rw", l_ReadAhead);
    }

    char l_Cmd[1024] = {'\0'};
    snprintf(l_Cmd, sizeof(l_Cmd), "lvcreate --name %s --size %s %s %s %s 2>&1;", pDevName, pMountSize, l_Options, pOptions, pVolumeGroupName);

    for (auto& l_Line : runCommand(l_Cmd)) {
        vector<std::string> l_Error = {ERROR_PREFIX};
        if (!fuzzyMatch(l_Line, l_Error)) {
            vector<std::string> l_Output1 = {"File descriptor", "leaked on lvcreate invocation."};
            if (!fuzzyMatch(l_Line, l_Output1)) {

                // Unexpected output...
                vector<std::string> l_Output2 = {"Volume group", pVolumeGroupName, "has insufficient free space"};
                if (fuzzyMatch(l_Line, l_Output2)) {
                    LOG(bb,error) << l_Line;
                    rc = -2;
                    break;
                }

                // Unexpected output...
                vector<std::string> l_Output3 = {"Insufficient suitable contiguous allocatable extents for logical volume"};
                if (fuzzyMatch(l_Line, l_Output3)) {
                    LOG(bb,error) << l_Line;
                    rc = -3;
                    break;
                }

                LOG(bb,info) << l_Line;

                // Expected output...
                vector<std::string> l_Output4 = {"Logical volume", pDevName, "created"};
                if (fuzzyMatch(l_Line, l_Output4)) {
                    rc = 0;
                }
            } else {
                // Just skip over leaked file descriptors...
            }
        } else {
            LOG(bb,error) << l_Line;
            rc = -1;
            break;
        }
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}


int doInitializeFileSystem(const char* pVolumeGroupName, const char* pDevName, const uint64_t pFlags, const int pLogBlockSize) {
    ENTRY(__FILE__,__FUNCTION__);
    int rc = -1;

    if(strstr(pDevName, "sd") == NULL) {
        switch (pFlags) {
            case BBXFS:
            {
                //https://linux.die.net/man/8/mkfs.xfs
                //
                /*mkfs.xfs [ -b block_size ] [ -d data_section_options ] [ -f ] [ -i inode_options ] [ -l log_section_options ] [ -n naming_options ] [ -p protofile ] [ -q ]
                           [ -r realtime_section_options ] [ -s sector_size ] [ -L label ] [ -N ] [ -K ] device

                -s sector_size
                    This option specifies the fundamental sector size of the filesystem. The sector_size is specified either as a value in bytes with size=value or
                    as a base two logarithm value with log=value. The default sector_size is 512 bytes. The minimum value for sector size is 512; the maximum is 32768 (32 KiB).
                    The sector_size must be a power of 2 size and cannot be made larger than the filesystem block size.

                -b block_size_options
                    This option specifies the fundamental block size of the filesystem. The valid block_size_options are: log=value or size=value and only one can be supplied.
                    The block size is specified either as a base two logarithm value with log=, or in bytes with size=. The default value is 4096 bytes (4 KiB),
                    the minimum is 512, and the maximum is 65536 (64 KiB). XFS on Linux currently only supports pagesize or smaller blocks.
                */
                int l_Length = 1+strlen(pVolumeGroupName)+1+strlen(DEVICE_DIRECTORY)+1+strlen(pDevName)+1;
                char* l_DevName = new char[l_Length];
                snprintf(l_DevName, l_Length, "/%s/%s/%s", DEVICE_DIRECTORY, pVolumeGroupName, pDevName);
                char l_Cmd[1024] = {'\0'};
                snprintf(l_Cmd, sizeof(l_Cmd), "mkfs.xfs -f -b log=%d %s 2>&1;", pLogBlockSize, l_DevName);
                LOG(bb,always) << "mkfs cmd= "<<l_Cmd;

                for (auto& l_Line : runCommand(l_Cmd)) {
                    vector<std::string> l_Error = {ERROR_PREFIX};
                    if (!fuzzyMatch(l_Line, l_Error)) {
                        // This output indicates failure...
                        vector<std::string> l_Error2 = {"No such file or directory"};
                        if (fuzzyMatch(l_Line, l_Error2)) {
                            LOG(bb,error) << l_Line;
                            break;
                        }
                        LOG(bb,info) << l_Line;
                        // This output indicates success...
                        if(l_Line.find("realtime") != string::npos) {
                            rc = 0;
                        }
                    } else {
                        LOG(bb,error) << l_Line;
                        break;
                    }
                }

                if (l_DevName) {
                    delete[] l_DevName;
                }
            }
                break;

            case BBEXT4:
            {
                int l_Length = 1+strlen(pVolumeGroupName)+1+strlen(DEVICE_DIRECTORY)+1+strlen(pDevName)+1;
                char* l_DevName = new char[l_Length];
                snprintf(l_DevName, l_Length, "/%s/%s/%s", DEVICE_DIRECTORY, pVolumeGroupName, pDevName);
                char l_Cmd[1024] = {'\0'};
                snprintf(l_Cmd, sizeof(l_Cmd), "mkfs.ext4 -F %s 2>&1;", l_DevName);

                for (auto& l_Line : runCommand(l_Cmd)) {
                    vector<std::string> l_Error = {ERROR_PREFIX};
                    if (!fuzzyMatch(l_Line, l_Error)) {
                        LOG(bb,info) << l_Line;

                        // This output indicates success...
                        vector<std::string> l_Output = {"Writing superblocks and filesystem accounting information:", "done"};
                        if (fuzzyMatch(l_Line, l_Output)) {
                            rc = 0;
                        }
                    } else {
                        LOG(bb,error) << l_Line;
                        break;
                    }
                }

                if (l_DevName) {
                    delete[] l_DevName;
                }
            }
                break;

            case BBFSCUSTOM1:
            case BBFSCUSTOM2:
            case BBFSCUSTOM3:
            case BBFSCUSTOM4:
                rc = 0;
                break;

            default:
                // NOTE: errstate filled in by invoker...
                LOG(bb,error) << "Invalid pFlags value of " << pFlags << " passed";
        }
    } else {
        rc = -2;
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}


int doMount(const char* pVolumeGroupName, const char* pDevName, const char* pMountPoint) {
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;

    int l_Length = 1+strlen(pVolumeGroupName)+1+strlen(DEVICE_DIRECTORY)+1+strlen(pDevName)+1;
    char* l_DevName = new char[l_Length];
    snprintf(l_DevName, l_Length, "/%s/%s/%s", DEVICE_DIRECTORY, pVolumeGroupName, pDevName);

    char l_GeneralOptions[64] = {'\0'};
    snprintf(l_GeneralOptions, sizeof(l_GeneralOptions), "-t auto -w");
    bool l_UseDiscardOption = config.get(process_whoami + ".use_discard_on_mount", DEFAULT_USE_DISCARD_ON_MOUNT_OPTION);
    char l_DiscardOption[64] = {'\0'};
    snprintf(l_DiscardOption, sizeof(l_DiscardOption), (l_UseDiscardOption ? " -o discard" : ""));


    bool l_Continue = true;
    while ((!rc) && (l_Continue)) {
        LOOP_COUNT(__FILE__,__FUNCTION__,"attempts");
        l_Continue = false;

        char l_Cmd[1024] = {'\0'};
        snprintf(l_Cmd, sizeof(l_Cmd), "mount %s %s %s %s 2>&1;", l_GeneralOptions, (l_UseDiscardOption ? l_DiscardOption : ""), l_DevName, pMountPoint);

        for (auto& l_Line : runCommand(l_Cmd)) {
            // No expected output...

            if (l_Continue) {
                LOG(bb,info) << l_Line;
            } else {
                if (l_UseDiscardOption) {
                    if (l_Line.size() > 1) {
                        LOG(bb,info) << l_Line;
                    } else {
                        LOG(bb,info) << std::hex << std::uppercase << setfill('0') << "One byte rc from mount: 0x" << l_Line[0] << setfill(' ') << std::nouppercase << std::dec;
                    }
                    LOG(bb,info) << "Attempt the mount operation again, but without the 'discard' option";
                    l_UseDiscardOption = 0;
                    l_Continue = true;
                    rc = 0;
                } else {
                    if (l_Line.size() > 1) {
                        LOG(bb,error) << l_Line;
                    } else {
                        LOG(bb,error) << std::hex << std::uppercase << setfill('0') << "One byte rc from mount: 0x" << l_Line[0] << setfill(' ') << std::nouppercase << std::dec;
                    }
                    rc = -1;
                }
            }
        }
    }

    if (l_DevName) {
        delete[] l_DevName;
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}


int doRemoveLogicalVolume(const char* pVolumeGroupName, const char* pDevName) {
    ENTRY(__FILE__,__FUNCTION__);
    int rc = -2;

    char* l_DevName = 0;
    if (pVolumeGroupName) {
        int l_Length = 1+strlen(pVolumeGroupName)+1+strlen(DEVICE_DIRECTORY)+1+strlen(pDevName)+1;
        l_DevName = new char[l_Length];
        snprintf(l_DevName, l_Length, "/%s/%s/%s", DEVICE_DIRECTORY, pVolumeGroupName, pDevName);
    } else {
        l_DevName = new char[strlen(pDevName)+1];
        strCpy(l_DevName, pDevName, strlen(pDevName)+1);
    }

    char l_Options[64] = {'\0'};
    snprintf(l_Options, sizeof(l_Options), "-vf");

    char l_Cmd[1024] = {'\0'};
    snprintf(l_Cmd, sizeof(l_Cmd), "lvremove %s %s;", l_Options, l_DevName);

    //  NOTE: Cannot search for "Failed to find logical volume" as the output is preceded by null characters
    //        and runCommand will not return those output strings.  In that case, rc is set to -2 as
    //        not being able to remove a logical volume, for any reason, may be tolerable by our invoker.
    for (auto& l_Line : runCommand(l_Cmd)) {
        vector<std::string> l_Error = {ERROR_PREFIX};
        if (!fuzzyMatch(l_Line, l_Error)) {
            vector<std::string> l_Output1 = {"File descriptor", "leaked on lvremove invocation."};
            if (!fuzzyMatch(l_Line, l_Output1)) {
                LOG(bb,info) << l_Line;

                // Expected output...
                vector<std::string> l_Output4 = {"Logical volume", "successfully removed"};
                if (fuzzyMatch(l_Line, l_Output4)) {
                    rc = 0;
                }
            } else {
                // Just skip over leaked file descriptors...
            }
        } else {
            LOG(bb,error) << l_Line;
            break;
        }
    }

    if (l_DevName) {
        delete[] l_DevName;
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}


int doResizeFileSystem(const char* pVolumeGroupName, const char* pDevName, const char* pFileSysType, const char* pMountPoint, const char* pMountSize, const uint64_t pFlags) {
    ENTRY(__FILE__,__FUNCTION__);
    int rc = -1;
    stringstream errorText;

    if(strstr(pDevName, "sd") == NULL) {
        if ((BBRESIZEFLAGS)pFlags != BB_DO_NOT_PRESERVE_FS) {
            if (strncmp(pFileSysType, "xfs", strlen(pFileSysType)) == 0) {
                // xfs file systems are resized here after lvresize...
                // NOTE:  If the file system is being preserved, then the size of the logical
                //        volume must be expanding for an XFS file system.
                if (pMountPoint) {
                    char l_Cmd[1024] = {'\0'};
                    snprintf(l_Cmd, sizeof(l_Cmd), "xfs_growfs -d %s 2>&1;", pMountPoint);

                    for (auto& l_Line : runCommand(l_Cmd)) {
                        vector<std::string> l_Error = {ERROR_PREFIX};
                        if (!fuzzyMatch(l_Line, l_Error)) {
                            vector<std::string> l_Output = {"File descriptor", "leaked on lvresize invocation."};
                            if (!fuzzyMatch(l_Line, l_Output)) {
                                LOG(bb,info) << l_Line;
                                rc = 0;
                            } else {
                                // Just skip over leaked file descriptors...
                            }
                        } else {
                            errorText << l_Line;
                            LOG_ERROR_TEXT_RC(errorText, rc);
                            break;
                        }
                    }
                } else {
                    errorText << "Could not resize the XFS file system because the specified size will shrink the logical volume/file system.";
                    LOG_ERROR_TEXT_RC(errorText, rc);
                }
            } else {
                if (strncmp(pFileSysType, "ext4", strlen(pFileSysType)) == 0) {
                    // ext4 file systems are resized by lvresize...  Nothing more to do...  @@DLH
                    rc = 0;
                } else {
                    // All other file systems...
                    errorText << "Could not resize the file system mounted at " << pDevName << " because it has a type of " << pFileSysType;
                    LOG_ERROR_TEXT_RC(errorText, rc);
                }
            }
        } else {
            if (!pMountPoint) {
                LOG(bb,info) << "The file system is not to be preserved, so it will not be resized prior to shrinking the logical volume.";
            } else {
                LOG(bb,info) << "The file system is not to be preserved, so it will not be resized to use all of the space available on the logical volume.";
            }
            rc = 0;
        }
    } else {
        errorText << "The file system to resize must be contained with the storage for the burst buffer volume group.";
        LOG_ERROR_TEXT_RC(errorText, rc);
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}


int doResizeLogicalVolume(const char* pVolumeGroupName, const char* pDevName, const char* pMountSize, const bool pResizeFileSystemWithLogicalVolume) {
    ENTRY(__FILE__,__FUNCTION__);
    int rc = -1;

    int l_Length = 1+strlen(pVolumeGroupName)+1+strlen(DEVICE_DIRECTORY)+1+strlen(pDevName)+1;
    char* l_DevName = new char[l_Length];
    snprintf(l_DevName, l_Length, "/%s/%s/%s", DEVICE_DIRECTORY, pVolumeGroupName, pDevName);

    char l_Options[64] = {'\0'};
    snprintf(l_Options, sizeof(l_Options), "--force");

    char l_Options2[64] = {'\0'};
    snprintf(l_Options2, sizeof(l_Options2), (pResizeFileSystemWithLogicalVolume ? "--resizefs" : ""));

    char l_Cmd[1024] = {'\0'};
    snprintf(l_Cmd, sizeof(l_Cmd), "lvresize --size %s %s %s %s 2>&1;", pMountSize, l_Options, l_Options2, l_DevName);

    for (auto& l_Line : runCommand(l_Cmd)) {
        vector<std::string> l_Error = {ERROR_PREFIX};
        if (!fuzzyMatch(l_Line, l_Error)) {
            vector<std::string> l_Output1 = {"File descriptor", "leaked on lvresize invocation."};
            if (!fuzzyMatch(l_Line, l_Output1)) {

                // Ignored output...
                vector<std::string> l_Output2 = {"THIS MAY DESTROY YOUR DATA"};
                if (fuzzyMatch(l_Line, l_Output2)) {
                    break;
                }

                // Unexpected output...
                vector<std::string> l_Output3 = {"Insufficient free space:"};
                if (fuzzyMatch(l_Line, l_Output3)) {
                    LOG(bb,error) << l_Line;
                    rc = -2;
                    break;
                }

                // Unexpected output...
                vector<std::string> l_Output4 = {"Insufficient suitable contiguous allocatable extents for logical volume"};
                if (fuzzyMatch(l_Line, l_Output4)) {
                    LOG(bb,error) << l_Line;
                    rc = -3;
                    break;
                }

                // Unexpected output...
                vector<std::string> l_Output5 = {"fsadm failed:"};
                if (fuzzyMatch(l_Line, l_Output5)) {
                    LOG(bb,error) << l_Line;
                    rc = -4;
                    break;
                }

                LOG(bb,info) << l_Line;

                // Expected output...
                vector<std::string> l_Output6 = {"Logical volume", pDevName, "successfully resized"};
                vector<std::string> l_Output7 = {"New size", " matches existing size"};
                if((fuzzyMatch(l_Line, l_Output6)) || (fuzzyMatch(l_Line, l_Output7))) {
                    rc = 0;
                }
            } else {
                // Just skip over leaked file descriptors...
            }
        } else {
            LOG(bb,error) << l_Line;
            break;
        }
    }

    if (l_DevName) {
        delete[] l_DevName;
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}
#include <sys/mount.h>

int lsofRunCmd( const char* pDirectory)
{
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;

    for (auto& l_Line : runCommand("lsof | grep /mnt"))
    {
        LOG(bb,info) << "lsof: " << l_Line;
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}

int doUnmount(const char* pDevName, const char* pMountPoint) {
    ENTRY(__FILE__,__FUNCTION__);

    int rc = umount(pMountPoint);
    if (rc) {
        stringstream errorText;
        errorText<<"umount of mountpoint="<<pMountPoint<<" had errno="<<errno<<"("<<strerror(errno)<<")";
        bberror << err("error.pathname", pMountPoint);
        if (errno==EBUSY){
            lsofRunCmd(pMountPoint);
        }
        LOG_ERROR_TEXT_ERRNO(errorText, errno);
    }
    EXIT(__FILE__,__FUNCTION__);
    return rc;
}

int doUnmountRunCmd(const char* pDevName, const char* pMountPoint) {
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;

    char l_Options[64] = {'\0'};
    snprintf(l_Options, sizeof(l_Options), "-vf");

    struct stat statinfo;
    rc = stat(pMountPoint, &statinfo);

    if (!rc) {
        char l_Cmd[1024] = {'\0'};
        snprintf(l_Cmd, sizeof(l_Cmd), "umount %s %s 2>&1;", l_Options, pMountPoint);

        for (auto& l_Line : runCommand(l_Cmd)) {
            // \todo - This could be part of the removal for a logical volume that should 'always' work...
            //         Need to investigate this path more to allow for successful removal...  @DLH

            if (l_Line.size() > 1) {
                vector<std::string> l_Error = {ERROR_PREFIX};
                if (!fuzzyMatch(l_Line, l_Error)) {
                    vector<std::string> l_Output1 = {"umount:", pMountPoint, "unmounted"};
                    if (fuzzyMatch(l_Line, l_Output1)) {
                        LOG(bb,info) << l_Line;
                        continue;
                    } else {
                        LOG(bb,error) << l_Line;
                        rc = -1;
                    }
                } else {
                    LOG(bb,error) << l_Line;
                    rc = -1;
                    break;
                }
            } else {
                LOG(bb,error) << std::hex << std::uppercase << setfill('0') << "One byte rc from unmount: 0x" << setw(2) << l_Line[0] << setfill(' ') << std::nouppercase << std::dec;
                rc = -1;
            }
        }
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}


int getFileSystemType(const char* pDevName, char* pFileSystemType, size_t pFileSystemTypeLength)
{
    ENTRY(__FILE__,__FUNCTION__);
    int rc = -1;

    char l_Cmd[1024] = {'\0'};

    snprintf(l_Cmd, sizeof(l_Cmd), "blkid %s 2>&1;", pDevName);
    LOG(bb,debug) << l_Cmd;

    for (auto& l_Line : runCommand(l_Cmd)) {
        vector<std::string> l_Error = {ERROR_PREFIX};
        if (!fuzzyMatch(l_Line, l_Error)) {
            char l_FileSystemType[32] = {'\0'};
            if (sscanf(l_Line.c_str(), "%*s %*s TYPE=\"%s[^\"]\"", l_FileSystemType) == 1) {
                l_FileSystemType[strlen(l_FileSystemType)-1] = 0;
                strCpy(pFileSystemType, l_FileSystemType, pFileSystemTypeLength);
                rc = 0;
            } else {
                break;
            }
        } else {
            break;
        }
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}

bool isLocalFile(const string& filename) {
    ENTRY(__FILE__,__FUNCTION__);
    bool rc = false;
    struct statfs statbuf;

    bfs::path fpath(filename);
    int rc2 = statfs(filename.c_str(), &statbuf);
    while((rc2) && ((errno == ENOENT)))
    {
        fpath = fpath.parent_path();
        if(fpath.string() == "")
            break;
        rc2 = statfs(fpath.string().c_str(), &statbuf);
    }

    if(rc2)
    {
        FL_Write(FLProxy, StatfsFailed, "Statfs failed",0,0,0,0);
        throw runtime_error(string("Unable to statfs file '") + filename + string("'.  errno=") + to_string(errno));
    }

    if((statbuf.f_type == EXT4_SUPER_MAGIC) || (statbuf.f_type == XFS_SUPER_MAGIC))
        rc = true;

    FL_Write(FLProxy, Statfs4isLocalFile, "isLocalFile=%ld  (magic=%lx)", rc, statbuf.f_type, 0, 0);

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}

#if 0
bool isLocalFile(const filehandle& fileptr)
{
    ENTRY(__FILE__,__FUNCTION__);
    bool rc = false;
    struct statfs statbuf;

    int rc2 = fstatfs(fileptr.getfd(), &statbuf);
    if(rc2)
        throw runtime_error(string("Unable to statfs the file.  errno=") + to_string(errno));

    if((statbuf.f_type == EXT4_SUPER_MAGIC) || (statbuf.f_type == XFS_SUPER_MAGIC))
        rc = true;

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}
#endif

int isMounted(const char* pVolumeGroupName, const char* pLogicalVolume, char* pMountPoint, size_t pMountPointLength) {
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;

    int l_Length = 1+strlen(DEVICE_MAPPER_DIRECTORY)+1+strlen(pVolumeGroupName)+1+strlen(pLogicalVolume)+1;
    char* l_DevName = new char[l_Length];
    snprintf(l_DevName, l_Length, "/%s/%s-%s", DEVICE_MAPPER_DIRECTORY, pVolumeGroupName, pLogicalVolume);

    rc = isDeviceMounted(l_DevName, pMountPoint, pMountPointLength);

    if (l_DevName) {
        delete[] l_DevName;
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}


int isMounted(const char* pMountPoint, char* pDevName, const size_t pDevNameLength, char* pFileSysType, const size_t pFileSysTypeLength) {
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;   // Negative value = error, 0 = not mounted, 1 = mounted;
    // NOTE: errstate filled in by invoker

    struct mntent* l_Entry;
    FILE* l_File;

    if (pMountPoint) {
        char* l_MountPoint = 0;
        l_MountPoint = realpath(pMountPoint, NULL);

        if (l_MountPoint) {
            l_File = setmntent(MOUNTS_DIRECTORY, "r");
            if (l_File) {
                while ((l_Entry = getmntent(l_File)) != NULL && !rc) {
                    LOOP_COUNT(__FILE__,__FUNCTION__,"mounts_processed");
                    if (strncmp(l_MountPoint, l_Entry->mnt_dir, strlen(l_MountPoint)) == 0 &&
                        strlen(l_MountPoint) == strlen(l_Entry->mnt_dir)) {
                        rc = 1;
                        if (strstr(l_Entry->mnt_fsname, "mapper")) {
                            // Get volume group name and length
                            size_t l_VolumeGroupNameLen = config.get(process_whoami + ".volumegroup", DEFAULT_VOLUME_GROUP_NAME).length();
                            char* l_VolumeGroupName = new char[l_VolumeGroupNameLen+1];
                            strncpy(l_VolumeGroupName, config.get(process_whoami + ".volumegroup", DEFAULT_VOLUME_GROUP_NAME).c_str(), l_VolumeGroupNameLen);
                            l_VolumeGroupName[l_VolumeGroupNameLen] = 0;

                            // Find the volume group name in the file system name
                            char* l_Index = strstr(l_Entry->mnt_fsname, l_VolumeGroupName);
                            if (l_Index) {
                                // NOTE:  Only return the VG_# value, where VG is the volume group name and # is the logical volume number
                                size_t l_DevNameLength = strlen(l_Entry->mnt_fsname)-(l_Index-l_Entry->mnt_fsname)-l_VolumeGroupNameLen-1;
                                if (pDevName && l_DevNameLength < pDevNameLength) {
                                    strncpy(pDevName, l_Index+l_VolumeGroupNameLen+1, l_DevNameLength);
                                    pDevName[l_DevNameLength] = 0;
                                }
                            }

                            // Return the file system type
                            if (pFileSysType && strlen(l_Entry->mnt_type) < pFileSysTypeLength) {
                                strncpy(pFileSysType, l_Entry->mnt_type, strlen(l_Entry->mnt_type));
                                pFileSysType[strlen(l_Entry->mnt_type)] = 0;
                            }

                            delete[] l_VolumeGroupName;
                        }
                    }
                }
                endmntent(l_File);
            } else {
                rc = -1;
                LOG(bb,error) << "For mount point " << l_MountPoint << ", error occurred when attempting to locate it within all mount points";
            }

            free(l_MountPoint);
        } else {
            rc = -1;
            LOG(bb,error) << "Could not determine the absolute path for " << pMountPoint;
        }
    } else {
        rc = -1;
        LOG(bb,error) << "Mount point not specified";
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}


int logicalVolumeExists(const char* pVolumeGroupName, const char* pDevName) {
    ENTRY(__FILE__,__FUNCTION__);
    int rc = -2; // Negative = error, 0 = does not exist, 1 = exists;
    stringstream errorText;

    int l_Length = 1+strlen(pVolumeGroupName)+1+strlen(DEVICE_DIRECTORY)+1+strlen(pDevName)+1;
    char* l_DevName = new char[l_Length];
    snprintf(l_DevName, l_Length, "/%s/%s/%s", DEVICE_DIRECTORY, pVolumeGroupName, pDevName);

    char l_Cmd[1024] = {'\0'};
    snprintf(l_Cmd, sizeof(l_Cmd), "lvdisplay %s 2>&1;", l_DevName);

    int l_Count = 1;
    for (auto& l_Line : runCommand(l_Cmd)) {
        if (rc == -2) {
            vector<std::string> l_Error = {ERROR_PREFIX};
            if (!fuzzyMatch(l_Line, l_Error)) {
                vector<std::string> l_Output1 = {"File descriptor", "leaked on lvdisplay invocation."};
                if (!fuzzyMatch(l_Line, l_Output1)) {

                    switch (l_Count) {
                        case 1:
                        {
                            vector<std::string> l_Output1 = {"One or more specified logical volume(s) not found."};
                            vector<std::string> l_Output2 = {"Failed to find logical volume"};
                            if (fuzzyMatch(l_Line, l_Output1) || fuzzyMatch(l_Line, l_Output2)) {
                                rc = 0;
                            } else {
                                vector<std::string> l_Output3 = {"--- Logical volume ---"};
                                if (fuzzyMatch(l_Line, l_Output3)) {
                                    ++l_Count;
                                } else {
                                    rc = -1;
                                    errorText << l_Line;
                                    LOG_ERROR_TEXT_RC(errorText, rc);
                                }
                            }
                            break;
                        }
                        case 2:
                        {
                            vector<std::string> l_Output4 = {l_DevName};
                            if (fuzzyMatch(l_Line, l_Output4)) {
                                rc = 1;
                            } else {
                                rc = -1;
                                errorText << l_Line;
                                LOG_ERROR_TEXT_RC(errorText, rc);
                            }
                            break;
                        }
                        default:
                            // Not possible...
                            break;
                    }
                }
            } else {
                rc = -1;
                errorText << "lvdisplay failed";
                LOG_ERROR_TEXT_RC(errorText, rc);
                break;
            }
        } else {
            break;
        }
    }

    if (rc == -2) {
        rc = -1;
        errorText << "lvdisplay failed (2)";
        LOG_ERROR_TEXT_RC(errorText, rc);
    }

    if (l_DevName) {
        delete[] l_DevName;
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}


int createLogicalVolume(const uid_t pOwner, const gid_t pGroup, const char* pMountPoint, const char* pMountSize, const uint64_t pFlags) {
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;
    stringstream errorText;

    char* l_MountPoint = 0;
    l_MountPoint = realpath(pMountPoint, NULL);

    if (l_MountPoint) {
        size_t l_VolumeGroupNameLen = config.get(process_whoami + ".volumegroup", DEFAULT_VOLUME_GROUP_NAME).length();
        char* l_VolumeGroupName = new char[l_VolumeGroupNameLen+1];
        strncpy(l_VolumeGroupName, config.get(process_whoami+".volumegroup", DEFAULT_VOLUME_GROUP_NAME).c_str(), l_VolumeGroupNameLen);
        l_VolumeGroupName[l_VolumeGroupNameLen] = 0;

        char* l_MountSize = const_cast<char*>(pMountSize);
        if (l_MountSize) {
            ssize_t l_NumberOfSectors = getNumberOfSectors(l_MountSize);
            if (l_NumberOfSectors >= 0) {
                if (l_NumberOfSectors < MINIMUM_LOGICAL_VOLUME_NUMBER_OF_SECTORS) {
                    // NOTE:  The real restriction is just for XFS file systems, but for simplicity,
                    //        we enforce a minimum size of 16M for ALL file systems.
                    l_MountSize = MINIMUM_LOGICAL_VOLUME_SIZE;
                    LOG(bb,info) << "Mount size of " << pMountSize << " is less than the minimum size for a file system.  Mount size modified to " << l_MountSize << ".";
                }
                char l_DevName[LINE_MAX] = {'\0'};
                size_t l_DevNameLength = sizeof(l_DevName);
                char l_FileSysType[LINE_MAX] = {'\0'};
                size_t l_FileSysTypeLength = sizeof(l_FileSysType);
                rc = isMounted(l_MountPoint, l_DevName, l_DevNameLength, l_FileSysType, l_FileSysTypeLength);
                if (rc == 0) {
                    int rc2 = 0;
                    bool l_AllDone = false;
                    int l_OriginalMaxRetries = config.get(process_whoami+".lvcreatemaxretries", DEFAULT_NUMBER_OF_CREATE_RETRIES);
                    int l_CurrentMaxRetries = l_OriginalMaxRetries;
                    bool l_Contiguous = true;
                    while((!rc) && (!l_AllDone)) {
                        LOOP_COUNT(__FILE__,__FUNCTION__,"attempts");
                        snprintf(l_DevName, sizeof(l_DevName), "%s_%d", l_VolumeGroupName, MasterLogicalVolumeNumber.getNext());
                        rc = logicalVolumeExists(l_VolumeGroupName, l_DevName);
                        if (rc == 0) {
                            if (l_Contiguous) {
                                rc = doCreateLogicalVolume(l_VolumeGroupName, l_DevName, l_MountSize, "--contiguous y");
                            } else {
                                rc = doCreateLogicalVolume(l_VolumeGroupName, l_DevName, l_MountSize, "--contiguous n");
                            }
                            if (!rc) {
                                rc = doChangeLogicalVolume(l_VolumeGroupName, l_DevName, "--activate y");
                                if (!rc) {
                                    /*
                                     * An anomaly occurs here when we attempt to initialize the file system immediately
                                     * after it's activation.
                                     * Sometimes, the initialization fails with the following message:
                                     *   " mkfs.ext4: No such file or directory while trying to determine filesystem size"
                                     * If that happens and we have retries left, we usually can eventually initialze the created
                                     * logical volume.  However, if we put in a small delay, it seems to greatly reduce the occurrence
                                     * of the error.  @@DLH
                                     */
                                    usleep((useconds_t)config.get(process_whoami+".delaybeforeinitfs", DEFAULT_DELAY_PRIOR_TO_INIT_FS));
                                    int l_LogBlockSize = config.get(process_whoami+".logblocksize", DEFAULT_LOGBLOCKSIZE);
                                    rc = doInitializeFileSystem(l_VolumeGroupName, l_DevName, pFlags,l_LogBlockSize);
                                    if (!rc) {
                                        rc = doMount(l_VolumeGroupName, l_DevName, l_MountPoint);
                                        if (!rc) {
                                            rc = becomeUser(0, 0);
                                            if(!rc) {
                                                rc = doChangeOwner(l_MountPoint, pOwner, pGroup);
                                                if (!rc) {
                                                    rc = becomeUser(threadLocaluid, threadLocalgid);
                                                    if (!rc) {
                                                        l_AllDone = true;
                                                    } else {
                                                        // becomeUser() to the uid/gid of the requestor failed
                                                        errorText << "becomeUser(" << pOwner << ", " << pGroup << ") failed";
                                                        LOG_ERROR_TEXT_RC(errorText, rc);
                                                    }
                                                } else {
                                                    // Could not change owner
                                                    // NOTE:  errstate already filled in...
                                                    errorText << "doChangeOwner() failed";
                                                    LOG_ERROR(errorText);
                                                }
                                            }
                                            if (rc) {
                                                // Undo the previous mount operation...
                                                rc2 = doUnmount(l_DevName, l_MountPoint);
                                                if (rc2) {
                                                    LOG(bb,error) << "Could not change ownership of the mountpoint " << l_MountPoint << " back to it's original owner after a failure, rc=" << rc2;
                                                }
                                            }
                                        } else {
                                            // Could not mount the mountpoint to the logical volume
                                            errorText << "Could not mount " << l_MountPoint << " on " << l_DevName;
                                            LOG_ERROR_TEXT_RC(errorText, rc);
                                        }
                                    } else {
                                        // Could not initialize the file system
                                        errorText << "File system could not be initialized for " << l_DevName << ", with mount flags of " << pFlags;
                                        LOG_ERROR_TEXT_RC(errorText, rc);
                                    }
                                } else {
                                    // Could not activate the logical volume
                                    errorText << "Logical volume " << l_DevName << " could not be activated";
                                    LOG_ERROR_TEXT_RC(errorText, rc);
                                }
                                if (rc) {
                                    // Because of a failure after the logical volume was created,
                                    // attempt to remove that logical volume...
                                    rc2 = doRemoveLogicalVolume(l_VolumeGroupName, l_DevName);
                                    if (rc2) {
                                        // \todo - Cleanup failed...  what to do...  @@DLH
                                        LOG(bb,error) << "Logical volume name of " << l_DevName << " could not be removed from volume group " << l_VolumeGroupName;
                                    }
                                }
                            } else {
                                if (rc != -2) {  //  Not insufficient free space
                                    if (--l_CurrentMaxRetries >= 0) {
                                        usleep((useconds_t)config.get(process_whoami+".lvcreateretrydelay", DEFAULT_DELAY_BETWEEN_CREATE_RETRIES));
                                        if (rc == -3) {  //  Insufficient suitable contiguous allocatable extents for logical volume
                                            LOG(bb,info) << "Attempt the create operation again, but specify non-contiguous storage.";
                                            l_Contiguous = false;
                                        }
                                        LOG(bb,info) << "Could not create logical volume named " << l_DevName << " with size of " << l_MountSize << " from volume group " << l_VolumeGroupName << ". " << l_CurrentMaxRetries << " of " << l_OriginalMaxRetries << " retries left...";
                                        rc = 0;   // Continue, attempting to create the logical volume to use...
                                    } else {
                                        // No more retries...
                                        errorText << "Could not create logical volume named " << l_DevName << " with size of " << l_MountSize << " from volume group " << l_VolumeGroupName << ". Maximum total attempts (" << l_OriginalMaxRetries+1 << ") to create a logical volume have been performed.";
                                        LOG_ERROR_TEXT_RC(errorText, rc);
                                    }
                                } else {
                                    rc = -1;
                                    errorText << "Could not create logical volume named " << l_DevName << " with size of " << l_MountSize << " from volume group " << l_VolumeGroupName << " because of insufficient free space.";
                                    LOG_ERROR_TEXT_RC(errorText, rc);
                                }
                            }
                        } else {
                            if (rc > 0) {
                                LOG(bb,info) << "Logical volume with suffix " << l_DevName << " already exists.";
                                rc = 0;   // Retry with a new logical volume name...
                            } else {
                                //  Something unexpected happened and was logged.  Bailout...
                                //  NOTE:  errstate already filled in...
                            }
                        }
                    }
                } else {
                    if (rc == 1) {
                        rc = -1;
                        errorText << l_MountPoint << " already mounted on " << l_DevName;
                        LOG_ERROR_TEXT_RC(errorText, rc);
                    } else {
                        // NOTE: pMountPoint probably doesn't exist, so l_MountPoint is a null char array.  Just use pMountPoint for the message...
                        errorText << "Error occurred when trying to determine if " << pMountPoint << " is already mounted. The directory for the mount point may no longer exist.";
                        LOG_ERROR_TEXT_RC(errorText, rc);
                    }
                }
            } else {
                rc = -1;
                errorText << "Invalid specification " << l_MountSize << " for mount size";
                LOG_ERROR_TEXT_RC(errorText, rc);
            }
        } else {
            rc = -1;
            errorText << "Mount size passed as " << l_MountSize << ". It must be passed as a positive value";
            LOG_ERROR_TEXT_RC(errorText, rc);
        }

        free(l_MountPoint);
        delete[] l_VolumeGroupName;

    } else {
        rc = -1;
        errorText << "Could not determine the absolute path for " << pMountPoint;
        LOG_ERROR_TEXT_RC(errorText, rc);
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}


char* findAlphaValue(char* pLine, const size_t pLength) {
    ENTRY(__FILE__,__FUNCTION__);
    char* rc = 0;

    size_t i;
    for (i=0; i<pLength && (!rc); ++i) {
        if (isalpha(pLine[i])) {
            rc = &(pLine[i]);
        }
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}


void findBB_DevNames(vector<string>& pDevNames, const FIND_BB_DEVNAMES_OPTION pOption)
{
    char l_DevName[1024] = {'\0'};

    size_t l_VolumeGroupNameLen = config.get(process_whoami+".volumegroup", DEFAULT_VOLUME_GROUP_NAME).length();
    char* l_VolumeGroupName = new char[l_VolumeGroupNameLen+1];
    strncpy(l_VolumeGroupName, config.get(process_whoami+".volumegroup", DEFAULT_VOLUME_GROUP_NAME).c_str(), l_VolumeGroupNameLen);
    l_VolumeGroupName[l_VolumeGroupNameLen] = 0;

    char l_Cmd[1024] = {'\0'};
    snprintf(l_Cmd, sizeof(l_Cmd), "lvdisplay %s 2>&1;", l_VolumeGroupName);

    bool l_FindNextVolume = true;
    for (auto& l_Line : runCommand(l_Cmd))
    {
        vector<std::string> l_Error = {ERROR_PREFIX};
        if (!fuzzyMatch(l_Line, l_Error))
        {
            char* l_Index = 0;
            if (l_FindNextVolume)
            {
                l_Index = strstr(const_cast<char*>(l_Line.c_str()), LV_DISPLAY_PREFIX);
                if (l_Index)
                {
                    l_Index = findAlphaValue(l_Index+strlen(LV_DISPLAY_PREFIX), l_Line.size()-(l_Index-l_Line.c_str())-sizeof(LV_DISPLAY_PREFIX)+2);
                    snprintf(l_DevName, sizeof(l_DevName), "/%s", l_Index);

                    // Now, check to make sure this is a logical volume that bb created...
                    int l_Length = 1+(strlen(DEVICE_DIRECTORY))+1+(strlen(l_VolumeGroupName))+1+(strlen(l_VolumeGroupName))+1+1;
                    char* l_DevName2 = new char[l_Length];
                    snprintf(l_DevName2, l_Length, "/%s/%s/%s_", DEVICE_DIRECTORY, l_VolumeGroupName, l_VolumeGroupName);
                    if (l_DevName == strstr(l_DevName, l_DevName2) && isdigit(l_DevName[l_Length-1]))
                    {
                        // Looks like this is a burst buffer logical volume...
                        // Indicate to parse through the remaining lines of output to see
                        // if we should attempt to remove it...
                        l_FindNextVolume = false;
                    }
                    else
                    {
                        // Logical volume not recognized...  Continue on...
//                        LOG(bb,warning) << "findBB_DevNames(): Logical volume name of " << l_DevName << " not recognized";
                    }
                    delete[] l_DevName2;
                }
                else
                {
                    // Log any messages about the volume group...
                    vector<std::string> l_Output1 = {"Volume group", l_VolumeGroupName};
                    if (fuzzyMatch(l_Line, l_Output1))
                    {
                        LOG(bb,debug) << l_Line;
                    }
                }
            }
            else
            {
                bool l_FoundBB_DevName = false;
                l_Index = strstr(const_cast<char*>(l_Line.c_str()), LV_DISPLAY_OPEN_PREFIX);
                if (l_Index)
                {
                    int l_NumOpens = (int)findNumericValue(l_Index+strlen(LV_DISPLAY_OPEN_PREFIX), l_Line.size()-(l_Index-l_Line.c_str())-sizeof(LV_DISPLAY_OPEN_PREFIX)+2);
                    if ((pOption == FIND_ALL_BB_DEVICES) ||
                        (pOption == FIND_ALL_UNMOUNTED_BB_DEVICES && (!l_NumOpens)) ||
                        (pOption == FIND_ALL_MOUNTED_BB_DEVICES && l_NumOpens))
                    {
                        l_FoundBB_DevName = true;
                    }
                    else
                    {
                        // Logical volume is mounted... Continue on...
                        LOG(bb,debug) << "findBB_DevNames(): Skipping logical volume " << l_DevName;
                        l_DevName[0] = 0;
                        l_FindNextVolume = true;
                    }
                }
                else
                {
                    vector<std::string> l_Output1 = {"LV Status"};
                    if (fuzzyMatch(l_Line, l_Output1))
                    {
                        vector<std::string> l_Output2 = {"NOT available"};
                        if (fuzzyMatch(l_Line, l_Output2))
                        {
                            l_FoundBB_DevName = true;
                        }
                    }
                }
                if (l_FoundBB_DevName)
                {
                    pDevNames.push_back(std::string(l_DevName));
                    l_DevName[0] = 0;
                    l_FindNextVolume = true;
                }
            }
        }
        else
        {
            LOG(bb,error) << l_Line;
            break;
        }
    }
    delete[] l_VolumeGroupName;
}


ssize_t getNewSizeInSectors(const char* pNewSize, const ssize_t pCurrentSize) {
    ENTRY(__FILE__,__FUNCTION__);
    ssize_t rc = 0;

    if (pNewSize[0] == '+' || pNewSize[0] == '-') {
        if (strlen(pNewSize) > 1) {
            rc = (pNewSize[0] == '+' ? pCurrentSize + getNumberOfSectors(&(pNewSize[1])) : pCurrentSize - getNumberOfSectors(&(pNewSize[1])));
        }
    } else {
        rc = getNumberOfSectors(pNewSize);
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}

int getUserAndGroupIds(const char* pPathName, uid_t& pUserId, gid_t& pGroupId)
{
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;
    stringstream errorText;

    LOG(bb,debug) << "getUserAndGroupIds(): pPathName=" << pPathName;
    try
    {
        struct stat statbuf;
        if((rc = stat(pPathName, &statbuf)) != 0)
        {
            rc = -1;
            errorText << "stat failed errno="<<errno<< " (" << strerror(errno) << ")";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }
        pUserId  = statbuf.st_uid;
        pGroupId = statbuf.st_gid;
    }
    catch(ExceptionBailout& e) { }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}

ssize_t findNumericValue(const char* pLine, const size_t pLength) {
    ENTRY(__FILE__,__FUNCTION__);
    ssize_t rc = -1;

    size_t i;
    for (i=0; i<pLength; ++i) {
        if (isdigit(pLine[i])) {
            break;
        }
    }

    if (i < pLength) {
        char* l_End = 0;
        rc = strtoll(&(pLine[i]), &l_End, 10);
        if ((errno == ERANGE && (rc == LONG_MAX || rc == LONG_MIN)) ||
            (errno != 0 && rc == 0) ||
            ((&(pLine[i]) == l_End))) {
            rc = -1;
        }
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}


ssize_t getNumberOfSectors(const char* pNewSize) {
    ENTRY(__FILE__,__FUNCTION__);
    ssize_t rc = 0;

    char* l_End = const_cast<char*>(pNewSize);
    float l_Value;
    l_Value = strtof(pNewSize, &l_End);
    if ((errno == ERANGE && (l_Value == LONG_MAX || l_Value == LONG_MIN)) ||
        (errno != 0 && l_Value == 0) ||
        (pNewSize == l_End)) {
        rc = -1;
    }

    if (!rc) {
        char l_Suffix = l_End[0];
        switch (l_Suffix) {
            case 'b':
            case 'B':
                l_Value = (l_Value+(SECTOR_SIZE-1)) / SECTOR_SIZE;
                break;
            case 's':
            case 'S':
                break;
            case 'k':
            case 'K':
                l_Value = l_Value*1024 / SECTOR_SIZE;
                break;
            case 'm':
            case 'M':
                l_Value = l_Value*1024*1024 / SECTOR_SIZE;
                break;
            case 'g':
            case 'G':
                l_Value = l_Value*1024*1024*1024 / SECTOR_SIZE;
                break;
            case 't':
            case 'T':
                l_Value = l_Value*1024*1024*1024*1024 / SECTOR_SIZE;
                break;
            case 'p':
            case 'P':
//                l_Value = l_Value*1024*1024*1024*1024*1024 / SECTOR_SIZE;
//                break;
            case 'e':
            case 'E':
//                l_Value = l_Value*1024*1024*1024*1024*1024*1024 / SECTOR_SIZE;
//                break;
            default:
                // NOTE: errstate filled in by invoker
                LOG(bb,error) << "Suffix vlaue of " << l_Suffix << " is not supported for the initial size or a resize specification.";
                rc = -1;
                break;
        }
        if (!rc) {
            rc = (ssize_t)(ceil(l_Value));
        }
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}


ssize_t getNumberOfAllocatedSectors(const char* pVolumeGroupName, const char* pDevName) {
    ENTRY(__FILE__,__FUNCTION__);
    ssize_t rc = -1;

    int l_Length = 1+strlen(pVolumeGroupName)+1+strlen(DEVICE_DIRECTORY)+1+strlen(pDevName)+1;
    char* l_DevName = new char[l_Length];
    snprintf(l_DevName, l_Length, "/%s/%s/%s", DEVICE_DIRECTORY, pVolumeGroupName, pDevName);

    char l_Cmd[1024] = {'\0'};
    snprintf(l_Cmd, sizeof(l_Cmd), "blockdev --getsz %s 2>&1;", l_DevName);

    for (auto& l_Line : runCommand(l_Cmd)) {
        vector<std::string> l_Error = {ERROR_PREFIX};
        if (!fuzzyMatch(l_Line, l_Error)) {
            rc = findNumericValue(l_Line.c_str(), l_Line.size());
        } else {
            break;
        }
    }

    if (l_DevName) {
        delete[] l_DevName;
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}


void logicalVolumeCleanup() {
    ENTRY(__FILE__,__FUNCTION__);

    int l_PerformCleanup = config.get(process_whoami+".onstartlogicalvolumecleanup", DEFAULT_ON_START_LOGICAL_VOLUME_CLEANUP);

    if (l_PerformCleanup)
    {
        LOG(bb,info) << "Logical volume cleanup started";
        FL_Write(FLProxy, Msg_LglVolClnUpStart, "Logical volume cleanup started",0,0,0,0);
        size_t l_VolumeGroupNameLen = config.get(process_whoami+".volumegroup", DEFAULT_VOLUME_GROUP_NAME).length();
        char* l_VolumeGroupName = new char[l_VolumeGroupNameLen+1];
        strncpy(l_VolumeGroupName, config.get(process_whoami+".volumegroup", DEFAULT_VOLUME_GROUP_NAME).c_str(), l_VolumeGroupNameLen);
        l_VolumeGroupName[l_VolumeGroupNameLen] = 0;

        vector<string> l_BB_DevNames;
        findBB_DevNames(l_BB_DevNames, FIND_ALL_UNMOUNTED_BB_DEVICES);

        for (size_t i=0; i<l_BB_DevNames.size(); ++i)
        {
            int rc = doRemoveLogicalVolume(0, l_BB_DevNames[i].c_str());
            if (rc)
            {
                LOG(bb,error) << "Logical volume name of " << l_BB_DevNames[i] << " could not be removed from volume group " << l_VolumeGroupName << ", rc = " << rc;
            }
        }

        delete[] l_VolumeGroupName;
        l_VolumeGroupName = 0;

        LOG(bb,info) << "Logical volume cleanup completed";
        FL_Write(FLProxy, Msg_LglVolClnUpComplete, "Logical volume cleanup completed",0,0,0,0);
    }
    else
    {
        LOG(bb,info) << "Logical volume cleanup has been bypassed";
        FL_Write(FLProxy, Msg_NoLglVolClnUp, "Logical volume cleanup has been bypassed",0,0,0,0);
    }

    EXIT(__FILE__,__FUNCTION__);
    return;
}


int resizeLogicalVolume(const char* pMountPoint, char* &pLogicalVolume, const char* pMountSize, const uint64_t pFlags) {
    ENTRY(__FILE__,__FUNCTION__);

    int rc = 0;
    stringstream errorText;

    char* l_MountPoint = 0;     // NOTE:  Only used in the pMountPoint not null paths...
    char l_DevName[LINE_MAX] = {'\0'};
    size_t l_DevNameLength = sizeof(l_DevName);
    char l_FileSysType[LINE_MAX] = {'\0'};
    size_t l_FileSysTypeLength = sizeof(l_FileSysType);

    size_t l_VolumeGroupNameLen = config.get(process_whoami+".volumegroup", DEFAULT_VOLUME_GROUP_NAME).length();
    char* l_VolumeGroupName = new char[l_VolumeGroupNameLen+1];
    strncpy(l_VolumeGroupName, config.get(process_whoami+".volumegroup", DEFAULT_VOLUME_GROUP_NAME).c_str(), l_VolumeGroupNameLen);
    l_VolumeGroupName[l_VolumeGroupNameLen] = 0;

    char* l_MountSize = const_cast<char*>(pMountSize);

    FL_Write(FLProxy, RESIZELOGICALVOLUME, "resizeLogicalVolume pFlags=%ld pLogicalVolume=%p pMountPoint=%p pMountSize=%p",pFlags,(uint64_t)pLogicalVolume,(uint64_t)pMountPoint,(uint64_t)pMountSize);
    LOG(bb,always) << "resizeLogicalVolume parms: pFlags=" << pFlags << " pMountSize="<< (pMountSize?pMountSize:"NULL")<< " pMountPoint="<< (pMountPoint?pMountPoint:"NULL")<< " pLogicalVolume="<< (pLogicalVolume?pLogicalVolume:"NULL");

    if (pMountPoint || pLogicalVolume) {
        if (pLogicalVolume && !pMountPoint) {
            // Resize logical volume.
            rc = logicalVolumeExists(l_VolumeGroupName, pLogicalVolume);
            if (rc == 1) {
                rc = isMounted(l_VolumeGroupName, pLogicalVolume, l_MountPoint, 0);
                if (!rc) {
                    strCpy(l_DevName, pLogicalVolume, l_DevNameLength);
                } else if (rc == 1) {
                    rc = -1;
                    errorText << "Logical volume with a suffix of " << pLogicalVolume << " is currently mounted.  The logical volume cannot be mounted for this resize operation.";
                    LOG_ERROR_TEXT_RC(errorText, rc);
                } else {
                    errorText << "Error occurred when trying to determine if logical volume " << pLogicalVolume << " is mounted.";
                    LOG_ERROR_TEXT_RC(errorText, rc);
                }
            } else {
                if (rc == 0) {
                    rc = -1;
                    LOG(bb,info) << "Logical volume with suffix of " << pLogicalVolume << " does not exist";
                } else {
                    //  Something unexpected happened and was logged.  Return already set return code.
                    //  NOTE: errstate already filled in...
                }
            }
        } else {
            if (pMountPoint && !pLogicalVolume) {
                // Resize mount point
                l_MountPoint = realpath(pMountPoint, NULL);

                if (l_MountPoint)
                {
                    rc = isMounted(l_MountPoint, l_DevName, l_DevNameLength, l_FileSysType, l_FileSysTypeLength);
                    if (rc == 1) {
                        //  Mount point has an associated device
                        rc = 0;
                        if (strncmp(l_DevName, l_VolumeGroupName, l_VolumeGroupNameLen) == 0) {
                            if ((strncmp(l_FileSysType, "ext4", l_FileSysTypeLength) == 0) || (strncmp(l_FileSysType, "xfs", l_FileSysTypeLength) == 0)) {
                                // \todo - Need to code for the relative cases where the size would be below the minimum...  @@DLH
                                if (pMountSize[0] != '-' && pMountSize[0] != '+') {
                                    ssize_t l_NumberOfSectors = getNumberOfSectors(l_MountSize);
                                    if (l_NumberOfSectors >= 0) {
                                        if (l_NumberOfSectors < MINIMUM_LOGICAL_VOLUME_NUMBER_OF_SECTORS) {
                                            l_MountSize = MINIMUM_LOGICAL_VOLUME_SIZE;
                                            LOG(bb,info) << "Mount size of " << pMountSize << " is less than the minimum size for a file system.  Mount size modified to " << l_MountSize << ".";
                                        }
                                    } else {
                                        rc = -1;
                                        errorText << "Invalid specification " << l_MountSize << " for mount size";
                                        LOG_ERROR_TEXT_RC(errorText, rc);
                                    }
                                }
                            } else {
                                rc = -1;
                                errorText << "Mount point " << l_MountPoint << " is associated with device " << l_DevName << " which has a file system type of " << l_FileSysType << " which is not supported for this resize operation";
                                LOG_ERROR_TEXT_RC(errorText, rc);
                            }
                        } else {
                            rc = -1;
                            errorText << "Mount point " << l_MountPoint << " is associated with device " << l_DevName << " which is not associated with volume group " << l_VolumeGroupName;
                            LOG_ERROR_TEXT_RC(errorText, rc);
                        }
                    } else {
                        if (rc == 0) {
                            rc = -1;
                            errorText << l_MountPoint << " is not currently mounted to a device.";
                            LOG_ERROR_TEXT_RC(errorText, rc);
                        } else {
                            // NOTE: pMountPoint probably doesn't exist, so l_MountPoint is a null char array.  Just use pMountPoint for the message...
                            errorText << "Error occurred when trying to determine if " << pMountPoint << " is already mounted to a device. The directory for the mount point may no longer exist.";
                            LOG_ERROR_TEXT_RC(errorText, rc);
                        }
                    }
                } else {
                    rc = -1;
                    errorText << "Could not determine the absolute path for " << pMountPoint;
                    LOG_ERROR_TEXT_RC(errorText, rc);
                }
            } else {
                // Must either specify pMountPoint or pLogicalVolume, but not both...
                rc = -1;
                errorText << "Either mount point or logical volume must be specified, but not both. Specified mount point is " << (const void *)pMountPoint << " and specified logicial volume is " << (const void *)pLogicalVolume << ".";
                LOG_ERROR_TEXT_RC(errorText, rc);
            }
        }
    } else {
        // Must either specify pMountPoint or pLogicalVolume...
        rc = -1;
        errorText << "Either mount point or logical volume must be specified.";
        LOG_ERROR_TEXT_RC(errorText, rc);
    }

    if (!rc) {
        bool l_PerformUnmount, l_Continue, l_Remount, l_ResizeLogicalVolume, l_ResizeFileSystem, l_ResizeFileSystemWithLogicalVolume;
        // NOTE:  The l_CurrentSize and l_NewSize values are determined from the file system, with a unit of sectors.
        ssize_t l_CurrentSize = getNumberOfAllocatedSectors(l_VolumeGroupName, l_DevName);
        if (l_CurrentSize > 0) {
            ssize_t l_NewSize = getNewSizeInSectors(l_MountSize, l_CurrentSize);
            if (l_NewSize > 0) {
                LOG(bb,info) << "Request changes the logical storage allocation from " << l_CurrentSize << " sectors to " << l_NewSize << " sectors."<<"flags="<<pFlags;
                if ((BBRESIZEFLAGS)pFlags != BB_DO_NOT_PRESERVE_FS) {
                    if ((l_NewSize < l_CurrentSize) && (strncmp(l_FileSysType, "xfs", l_FileSysTypeLength) == 0)) {
                        rc = -1;
                        errorText << "The request is to shrink and preserve an XFS file system.  The file system cannot be preserved in this case.  Resize request not performed.";
                        LOG_ERROR_TEXT_RC(errorText, rc);
                    }
                }
                if (pLogicalVolume && (l_NewSize > l_CurrentSize)) {
                    rc = -1;
                    errorText << "The request is to resize an unmounted logical volume and the specified new size is greater than the current size.  The specified new size must be less than or equal to the current size.";
                    LOG_ERROR_TEXT_RC(errorText, rc);
                }
                l_PerformUnmount = false;
                l_Continue = true;
                while ((!rc) && (l_Continue)) {
                    LOOP_COUNT(__FILE__,__FUNCTION__,"attempts_1");
                    l_Continue = false;
                    l_Remount = false;
                    l_ResizeLogicalVolume = true;
                    l_ResizeFileSystem = false;
                    l_ResizeFileSystemWithLogicalVolume = (((BBRESIZEFLAGS)pFlags == BB_DO_NOT_PRESERVE_FS) || (strncmp(l_FileSysType, "xfs", l_FileSysTypeLength) == 0) || pLogicalVolume) ? false : true;
                    if ((l_PerformUnmount) || (l_NewSize < l_CurrentSize)) {
                        if (!pLogicalVolume) {
                            rc = doUnmount(l_DevName, l_MountPoint);
                            if (!rc) {
                                // NOTE:  If the file system is being shrunk and is not to be preserved, then we
                                //        do NOT remount the device to the mount point.  Otherwise, we attempt to remount.
                                //        The reason for this is a subsequent resize request could then ask to preserve the
                                //        file system and that may/probably would fail.
                                if ((BBRESIZEFLAGS)pFlags != BB_DO_NOT_PRESERVE_FS) {
                                    l_Remount = true;
                                } else {
                                    LOG(bb,info) << "The file system is being shrunk and not to be preserved.  Therefore, after the resize operation, the device will NOT be remounted to the mount point.";
                                }
                                rc = doResizeFileSystem(l_VolumeGroupName, l_DevName, l_FileSysType, NULL, l_MountSize, pFlags);
                                if (rc) {
                                    // \todo - Not sure about retries here...  @@DLH
                                    // NOTE: errstate filled in by doResizeFileSystem()
                                    LOG(bb,error) << "Could not resize file system for " << l_MountPoint;
                                }
                            } else {
                                errorText << "Specified new size for the device is less than the current size. However, mount point " << l_MountPoint << " could not be unmounted from device " << l_DevName;
                                LOG_ERROR_TEXT_RC(errorText, rc);
                            }
                        }
                    } else {
                        if (l_NewSize > l_CurrentSize) {
                            l_ResizeFileSystem = true;
                        } else {
                            l_ResizeLogicalVolume = false;
                        }
                    }
                    if (!rc) {
                        if (l_ResizeLogicalVolume) {
                            bool l_AllDone = false;
                            int l_OriginalMaxRetries = config.get(process_whoami+".lvresizemaxretries", 1);
                            int l_CurrentMaxRetries = l_OriginalMaxRetries;
                            while((!rc) && (!l_AllDone)){
                                LOOP_COUNT(__FILE__,__FUNCTION__,"attempts_2");
                                rc = doResizeLogicalVolume(l_VolumeGroupName, l_DevName, l_MountSize, l_ResizeFileSystemWithLogicalVolume);
                                if (rc) {
                                    if (rc == -4 && (!l_Remount)) {
                                        //  Took an error on the resize operation...  If we did not unmount the file system first,
                                        //  attempt to unmount the file system and then retry the resize operation...  @@DLH
                                        //  NOTE:  We should never get into this leg when resizing an unmounted logical volume.
                                        LOG(bb,info) << "Attempt the resize operation again with an unmounted file system.";
                                        l_PerformUnmount = true;
                                        l_Continue = true;
                                        rc = 0;
                                        break;
                                    } else {
                                        if (rc != -2) {  //  Not insufficient free space
                                            if (--l_CurrentMaxRetries >= 0) {
                                                LOG(bb,error) << "Could not resize logical volume " << l_DevName << " to a size of " << l_MountSize << " from volume group " << l_VolumeGroupName << ". " << l_CurrentMaxRetries+1 << " of " << l_OriginalMaxRetries << " retries left...";
                                                if (rc == -3) {  //  Insufficient suitable contiguous allocatable extents for logical volume
                                                    LOG(bb,info) << "Attempt the resize operation again, but specify non-contiguous storage.";
                                                    doChangeLogicalVolume(l_VolumeGroupName, l_DevName, "--contiguous n");
                                                }
                                                if (config.get(process_whoami+".lvresizeretrydelay", 0)) {
                                                    sleep(config.get(process_whoami+".lvresizeretrydelay", 0));
                                                }
                                                rc = 0;   // Continue on, attempting to resize the logical volume...
                                            } else {
                                                // No more retries...
                                                errorText << "Could not resize logical volume " << l_DevName << " to a size of " << l_MountSize << " from volume group " << l_VolumeGroupName << ". Maximum total attempts (" << l_OriginalMaxRetries+1 << ") to resize the logical volume have been performed.";
                                                LOG_ERROR_TEXT_RC(errorText, rc);
                                            }
                                        }
                                    }
                                } else {
                                    l_AllDone = true;
                                }
                            }
                        } else {
                            LOG(bb,info) << "Current size for the device is the same as the specified new size.  The logical volume/file system do not need to be resized.";
                        }

                        if (!rc) {
                            if (l_ResizeFileSystem) {
                                //  NOTE:  We should never get into this leg when resizing an unmounted logical volume.
                                rc = doResizeFileSystem(l_VolumeGroupName, l_DevName, l_FileSysType, l_MountPoint, l_MountSize, pFlags);
                                if (rc) {
                                    // \todo - Not sure about retries here...  @@DLH
                                    // NOTE: errstate filled in by doResizeFileSystem()
                                    LOG(bb,error) << "Could not resize file system for " << l_MountPoint;
                                }
                            }
                        }
                    }

                    if (l_Remount) {
                        //  NOTE:  We should never get into this leg when resizing an unmounted logical volume.
                        l_Remount = false;
                        int rc2 = doMount(l_VolumeGroupName, l_DevName, l_MountPoint);
                        if (rc2) {
                            rc = -1;
                            errorText << "Device and file system were resized, but " << l_MountPoint << " could not be remounted on device " << l_DevName;
                            LOG_ERROR_TEXT_RC(errorText, rc);
                        }
                        (void)l_Remount;
                    }
                }
            } else {
                rc = -1;
                errorText << "Could not calculate a positive number of resulting sectors that will be allocated based upon the new storage allocation size of " << l_MountSize;
                LOG_ERROR_TEXT_RC(errorText, rc);
            }
        } else {
            rc = -1;
            errorText << "Could not determine the allocated number of sectors for device " << l_DevName;
            LOG_ERROR_TEXT_RC(errorText, rc);
        }
    }

    if(l_MountPoint)
    {
        free(l_MountPoint);
        l_MountPoint = NULL;
    }

    delete[] l_VolumeGroupName;

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}


int removeLogicalVolume(const char* pMountPoint, Uuid pLVUuid, ON_ERROR_FILL_IN_ERRSTATE pOnErrorFillInErrState) {
    ENTRY(__FILE__,__FUNCTION__);

    int rc = 0;
    stringstream errorText;

    char l_DevName[LINE_MAX] = {'\0'};
    size_t l_DevNameLength = sizeof(l_DevName);

    size_t l_VolumeGroupNameLen = config.get(process_whoami+".volumegroup", DEFAULT_VOLUME_GROUP_NAME).length();
    char* l_VolumeGroupName = new char[l_VolumeGroupNameLen+1];
    strncpy(l_VolumeGroupName, config.get(process_whoami+".volumegroup", DEFAULT_VOLUME_GROUP_NAME).c_str(), l_VolumeGroupNameLen);
    l_VolumeGroupName[l_VolumeGroupNameLen] = 0;

    char* l_MountPoint = 0;
    l_MountPoint = realpath(pMountPoint, NULL);

    if (l_MountPoint) {
        char l_FileSysType[LINE_MAX] = {'\0'};
        size_t l_FileSysTypeLength = sizeof(l_FileSysType);
        rc = isMounted(l_MountPoint, l_DevName, l_DevNameLength, l_FileSysType, l_FileSysTypeLength);
        if (rc == 1) {
            if (strncmp(l_DevName, l_VolumeGroupName, l_VolumeGroupNameLen) == 0) {
                rc = doUnmount(l_DevName, l_MountPoint);
                if (!rc) {
                    rc = doChangeLogicalVolume(l_VolumeGroupName, l_DevName, "--activate n");
                    if (!rc) {
                        rc = doRemoveLogicalVolume(l_VolumeGroupName, l_DevName);
                        if (rc) {
                            errorText << "Logical volume name of " << l_DevName << " could not be removed from volume group " << l_VolumeGroupName;
                            LOG(bb,error) << errorText.str();
                            if (pOnErrorFillInErrState) {
                                bberror << err("error.text", errorText.str()) << errloc(rc);
                            }
                        }
                    } else {
                        errorText << "Could not deactivate logical volume " << l_DevName;
                        LOG(bb,error) << errorText.str();
                        if(pOnErrorFillInErrState) {
                            bberror << err("error.text", errorText.str()) << errloc(rc);
                        }
                    }
                } else {
                    errorText << "Could not unmount " << l_MountPoint << " from " << l_DevName;
                    LOG(bb,error) << errorText.str();
                    if (pOnErrorFillInErrState) {
                        bberror << err("error.text", errorText.str()) << errloc(rc);
                    }
                }
            } else {
                rc = -1;
                if (pOnErrorFillInErrState) {
                    errorText << "Mount point " << l_MountPoint << " is associated with device " << l_DevName << " which is not associated with volume group " << l_VolumeGroupName;
                    LOG(bb,error) << errorText.str();
                    bberror << err("error.text", errorText.str()) << errloc(rc);
                }
            }
        } else {
            if (rc == 0) {
                getLogicalVolumeDevName(pLVUuid, l_DevName, sizeof(l_DevName));
                rc = doRemoveLogicalVolume(l_VolumeGroupName, l_DevName);
                if (rc) {
                    errorText << "Logical volume name of " << l_DevName << " could not be removed from volume group " << l_VolumeGroupName;
                    LOG(bb,error) << errorText.str();
                    if (pOnErrorFillInErrState) {
                        bberror << err("error.text", errorText.str()) << errloc(rc);
                    }
                }
            } else {
                // NOTE: pMountPoint probably doesn't exist, so l_MountPoint is a null char array.  Just use pMountPoint for the message...
                rc = -2;
                errorText << "Error occurred when trying to determine if " << pMountPoint << " is already mounted. The mount point may no longer exist.";
                LOG(bb,error) << errorText.str();
                if (pOnErrorFillInErrState) {
                    bberror << err("error.text", errorText.str()) << errloc(rc);
                }
            }
        }
    } else {
        rc = -1;
        errorText << "Could not determine an absolute path for " << pMountPoint;
        LOG(bb,error) << errorText.str();
        if (pOnErrorFillInErrState) {
            bberror << err("error.text", errorText.str()) << errloc(rc);
        }
    }

    if(l_MountPoint)
    {
        free(l_MountPoint);
        l_MountPoint = NULL;
    }

    delete[] l_VolumeGroupName;

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}


int processContrib(const uint64_t pNumContrib, uint32_t pContrib[])
{
    ENTRY(__FILE__,__FUNCTION__);

    int rc = 0;

    uint64_t i;
    uint64_t j;
    uint32_t* x;
    uint32_t* y;
    uint32_t z;
    stringstream errorText;

    // Bubble sort the list of contrib values in ascending order...
    for (i=0; (!rc) && i<pNumContrib-1; ++i) {
        LOOP_COUNT(__FILE__,__FUNCTION__,"contributors");
        x = &pContrib[i];
        for (j=i+1; j<pNumContrib; ++j) {
            y = &pContrib[j];
            if (i==0) {
                // Special checks for first time through...
                if (*x==*y) {
                    // Duplicate values found
                    rc = -1;
                    errorText << "Duplicate value of " << *x << " found in the list of contributors";
                    LOG_ERROR_TEXT_RC(errorText, rc);
                    break;
                }
            }
            if (*x > *y) {
                z = *x;
                *x = *y;
                *y = z;
            }
        }
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}


int setupTransfer(BBTransferDef* transfer, Uuid &lvuuid, const uint64_t pJobId, const uint64_t pHandle, const uint32_t pContribId, vector<struct stat*>& pStats, const uint32_t pPerformOperation)
{
    ENTRY(__FILE__,__FUNCTION__);

    int rc = 0;
    stringstream errorText;

    Uuid l_lvuuid, l_current_lvuuid;
    vector<Extent> newlist;
    // NOTE: The checks involving l_Flags may not be necessary below as the l_lvuuid check may catch the same situation...
    // \todo - Need to investigate more...  @DLH
    uint64_t l_Flags = 0;

    bool l_BothLocal = false;
    bool l_SourceFileIsLocal = false;
    bool l_TargetFileIsLocal = false;
    bool l_SimulateStageIn = false;
    bool l_SimulateStageOut = false;
    bool l_SimulateFileStageIn = false;
    bool l_SimulateFileStageOut = false;
    bool l_AllowLocalCopy = true;
    bool l_SkipFile = false;
    bool l_SavedAllowLocalCopy;

    if (config.get(process_whoami+".bringup.useTestVariables", 0) > 0) {
        l_AllowLocalCopy = readVar("nolocalcopy") > 0 ? false : true;
    }
    if (!l_AllowLocalCopy)
    {
        l_SimulateStageIn = readVar("stagein") > 0 ? true : false;
        l_SimulateStageOut = l_SimulateStageIn ? false : readVar("stageout") > 0 ? true : false;
    }

    if (!pPerformOperation)
    {
        size_t l_Size = transfer->files.size();
        for(size_t i=0; i<l_Size; ++i)
        {
            pStats.push_back(0);
            transfer->sizeTransferred.push_back(0);
        }
    }

    if (transfer->hasFilesInRequest())
    {
        transfer->setAll_CN_CP_TransfersInDefinition();
        transfer->setNoStageinOrStageoutTransfersInDefinition();
        uint32_t l_NextSourceIndexToProcess = 0;
        for(auto& e : transfer->extents)
        {
            LOOP_COUNT(__FILE__,__FUNCTION__,"input extents");
            filehandle* srcfile_ptr = 0;
            filehandle* dstfile_ptr = 0;
            l_SkipFile = false;

            l_SavedAllowLocalCopy = l_AllowLocalCopy;
            if (config.get(process_whoami+".bringup.useTestVariables", 0) > 0) {
                l_SimulateFileStageIn = e.flags & 0x0000000000000004 ? true : false;
                l_SimulateFileStageOut = l_SimulateFileStageIn ? false : e.flags & 0x0000000000000008 ? true : false;
                if (l_SimulateFileStageIn || l_SimulateFileStageOut)
                    l_AllowLocalCopy = false;
            }

            l_SourceFileIsLocal = isLocalFile(transfer->files[e.sourceindex]);
            l_TargetFileIsLocal = isLocalFile(transfer->files[e.targetindex]);
            l_BothLocal = (l_SourceFileIsLocal && l_TargetFileIsLocal) ? true : false;

            if (!pPerformOperation)
            {
                LOG(bb,info) << "srcfile: " << "index=" << e.sourceindex << ", isLocal=" << l_SourceFileIsLocal;
                LOG(bb,info) << "dstfile: " << "index=" << e.targetindex << ", isLocal=" << l_TargetFileIsLocal;

                if (l_SourceFileIsLocal || l_SimulateFileStageOut)
                {
                    // Local cp or stageout processing...
                    if (transfer->builtViaRetrieveTransferDefinition())
                    {
                        // NOTE:  For restart, we have to close the original/first
                        //        instance of the open for the source file...
                        filehandle* fh;
                        // Remove the source file (if open)
                        if (!removeFilehandle(fh, pJobId, pHandle, pContribId, e.sourceindex))
                        {
                            LOG(bb,info) << "Releasing source filehandle '" << transfer->files[e.sourceindex] << "' before restart";
                            rc = fh->release(BBFILE_STOPPED);
                            delete fh;
                            fh = NULL;
                            if (rc)
                            {
                                LOG(bb,error) << "Releasing the filehandle for srcfile " << transfer->files[e.sourceindex] << " failed, rc " << rc;
                                rc = -1;
                                break;
                            }
                        }
                        else
                        {
                            rc = 0;
                            LOG(bb,info) << "Could not find prior source filehandle for '" << transfer->files[e.sourceindex] << "' before restart";
                        }
                        if (!rc)
                        {
                            // Now, just to make sure, attempt to find a filehandle for the source file...
                            rc = findFilehandle(srcfile_ptr, pJobId, pHandle, pContribId, e.sourceindex);
                            if (rc == -1)
                            {
                                // As expected, not found...
                                rc = 0;
                            }
                            else
                            {
                                // Should never be the case...
                                LOG(bb,error) << "Checking for the removal of the filehandle for srcfile " << transfer->files[e.sourceindex] << " failed, rc " << rc;
                                rc = -1;
                                break;
                            }
                        }
                    }

                    if (!rc)
                    {
                        // Open the source file...
                        srcfile_ptr = new filehandle(transfer->files[e.sourceindex], O_RDONLY, 0);
                        if (srcfile_ptr->getfd() < 0)
                        {
                            rc = -1;
                            LOG(bb,error) << "Creating the filehandle for srcfile: " << transfer->files[e.sourceindex] << " failed.";

                            delete srcfile_ptr;
                            break;
                        }
                    }
                }
                else
                {
                    // Create a filehandle for the source file to house the stats...
                    srcfile_ptr = new filehandle(transfer->files[e.sourceindex]);
                }

                if ((!rc) && (l_TargetFileIsLocal && (!l_SimulateFileStageOut)))
                {
                    // Stagein processing
                    if (transfer->builtViaRetrieveTransferDefinition())
                    {
                        // NOTE:  For restart, we have to close the original/first
                        //        instance of the open for the target file...
                        filehandle* fh;
                        // Remove the target file (if open)
                        if (!removeFilehandle(fh, pJobId, pHandle, pContribId, e.targetindex))
                        {
                            LOG(bb,info) << "Releasing target filehandle '" << transfer->files[e.targetindex] << "' before restart";
                            rc = fh->release(BBFILE_STOPPED);
                            delete fh;
                            fh = NULL;
                            if (rc)
                            {
                                LOG(bb,error) << "Releasing the filehandle for dstfile: " << transfer->files[e.targetindex] << " failed, rc " << rc;
                                rc = -1;

                                delete srcfile_ptr;
                                break;
                            }
                        }
                        else
                        {
                            rc = 0;
                            LOG(bb,info) << "Could not find prior target filehandle for '" << transfer->files[e.targetindex] << "' before restart";
                        }

                        if (!rc)
                        {
                            // Now, just to make sure, attempt to find a filehandle for the target file...
                            rc = findFilehandle(dstfile_ptr, pJobId, pHandle, pContribId, e.targetindex);
                            if (rc == -1)
                            {
                                // As expected, not found...
                                rc = 0;
                            }
                            else
                            {
                                // Should never be the case...
                                LOG(bb,error) << "Checking for the removal of the filehandle for dstfile " << transfer->files[e.targetindex] << " failed, rc " << rc;
                                rc = -1;

                                delete srcfile_ptr;
                                break;
                            }
                        }
                    }
                }

                if (!rc)
                {
                    addFilehandle(srcfile_ptr, pJobId, pHandle, pContribId, e.sourceindex);

                    if (l_SourceFileIsLocal && (!l_SimulateFileStageIn))
                    {
                        // Local cp or stageout processing...
                        // If not already done, get stats for the local source file on the SSD...
                        if (pStats[e.sourceindex] == 0)
                        {
                            pStats[e.sourceindex] = new(struct stat);
                            srcfile_ptr->getstats(*pStats[e.sourceindex]);
                        }
                    }
                }
            }
            else
            {
                // NOTE: Not sure if we need the if block below.  Only if
                //       sourcefile indices do not exist in the extent list...
                if (transfer->builtViaRetrieveTransferDefinition())
                {
                    // Indicate that bbproxy will skip all source/target file pairs from
                    // the last pair processed up through the current source/target file pair...
                    // NOTE: We have a dependency that bbServer will always insert extents into
                    //       the extent vector in ascending sourcefile index order...
                    while (l_NextSourceIndexToProcess < e.sourceindex)
                    {
                        LOG(bb,info) << "bbProxy will not restart the transfer for the source file associated with jobid " \
                                     << pJobId << ", handle " << pHandle << ", contrib " << pContribId \
                                     << ", source index " << l_NextSourceIndexToProcess << ", transfer type UNKNOWN";
                        l_NextSourceIndexToProcess += 2;
                    }
                }

                // First, determine if bbServer has indicated to not perform a copy/transfer
                // operation for the source/target file pair.  If this is a rebuilt transfer
                // definition from bbServer metadata, restart should not perform the copy/transfer
                // if the file was not 'stopped'.
                if (!(pStats[e.sourceindex]->st_dev == DO_NOT_TRANSFER_FILE &&
                      pStats[e.sourceindex]->st_ino == DO_NOT_TRANSFER_FILE))
                {
                    // We are to transfer this file...
                    //
                    // Next, look for the filehandle to the source file...
                    // NOTE:  Even if we didn't perform an open for the source file,
                    //        a filehandle was created during the first pass to house the stats...
                    rc = findFilehandle(srcfile_ptr, pJobId, pHandle, pContribId, e.sourceindex);
                    if (rc == 0)
                    {
                        // We have a filehandle to the source file...  If stagein, copy the stats
                        // passed back from bbserver into the filehandle for the source file.
                        if ((e.flags & BBI_TargetSSD) || l_SimulateFileStageIn)
                        {
                            srcfile_ptr->updateStats(pStats[e.sourceindex]);
                        }

                        if (l_TargetFileIsLocal && (!l_SimulateFileStageOut))
                        {
                            // Now, attempt to find a filehandle for the target file...
                            rc = findFilehandle(dstfile_ptr, pJobId, pHandle, pContribId, e.targetindex);
                            if (rc == -1)
                            {
                                rc = 0;
                                if(e.flags & BBRecursive)
                                {
                                    bfs::create_directories(bfs::path(transfer->files[e.targetindex]).parent_path());
                                }

                                int l_OpenFlags = (l_TargetFileIsLocal ? O_CREAT | O_TRUNC | O_WRONLY : O_RDONLY);
                                // Open the target file...
                                dstfile_ptr = new filehandle(transfer->files[e.targetindex], l_OpenFlags, srcfile_ptr->getmode());
                                if (dstfile_ptr->getfd() >= 0)
                                {
                                    addFilehandle(dstfile_ptr, pJobId, pHandle, pContribId, e.targetindex);
                                }
                                else
                                {
                                    rc = -1;
                                    LOG(bb,error) << "Creating the filehandle for dstfile: " << transfer->files[e.targetindex] << " failed.";
                                    break;
                                }
                            }
                            else
                            {
                                // Must be a local cp...
                                l_SkipFile = true;
                            }
                        }
                    }
                    else
                    {
                        rc = -1;
                        LOG(bb,error) << "Cannot find source filehandle for jobid " << pJobId << ", handle " << pHandle << ", contribid " << pContribId << ", source index " << e.sourceindex;
                        break;
                    }
                }
                else
                {
                    l_SkipFile = true;
                }

                if (transfer->builtViaRetrieveTransferDefinition())
                {
                    char l_TransferType[64] = {'\0'};
                    if (!l_SkipFile)
                    {
                        getStrFromTransferType(e.flags, l_TransferType, sizeof(l_TransferType));
                        LOG(bb,info) << "bbProxy will restart the transfer for the source file associated with jobid " \
                                     << pJobId << ", handle " << pHandle << ", contrib " << pContribId \
                                     << ", source index " << e.sourceindex << ", transfer type " << l_TransferType;
                    }
                    else
                    {
                        getStrFromTransferType(e.flags, l_TransferType, sizeof(l_TransferType));
                        LOG(bb,info) << "bbProxy will not restart the transfer for the source file associated with jobid " \
                                     << pJobId << ", handle " << pHandle << ", contrib " << pContribId \
                                     << ", source index " << e.sourceindex << ", transfer type " << l_TransferType;
                    }
                }
                l_NextSourceIndexToProcess = e.sourceindex + 2;
            }

            if (!rc)
            {
                if (!l_SkipFile)
                {
                    e.flags = transfer->mergeFileFlags(e.flags);
                    e.handle = pHandle;
                    e.contribid = pContribId;

                    if (l_BothLocal && l_AllowLocalCopy)
                    {
                        e.flags |= BBI_TargetSSDSSD;

                        // Local cp processing...
                        if (pPerformOperation)
                        {
                            BBFILESTATUS l_FileStatus = BBFILE_SUCCESS;

                            bs::error_code err;
                            bfs::copy_file(bfs::path(srcfile_ptr->getfn()), bfs::path(dstfile_ptr->getfn()), bfs::copy_option::overwrite_if_exists, err);
                            if (err.value())
                            {
                                l_FileStatus = BBFILE_FAILED;
                            }

                            // bbServer needs this extent so that it can send the appropriate completion messages back
                            // to bbProxy and update status for the transfer definition, LVKey, and handle.
                            // Note that this dummy extent will be marked to actually transfer no data.
                            // Also note that the length of the local copy is still filled into the extent
                            // object for status purposes and updating the SSD usage information.
                            e.setCP_Tansfer();
                            e.len = 0;

                            e.flags |= BBI_First_Extent;
                            e.flags |= BBI_Last_Extent;

                            switch(l_FileStatus) {
                                case BBFILE_SUCCESS:
                                    e.len = srcfile_ptr->getsize();
                                    LOG(bb,info) << "Local copy complete for file " << srcfile_ptr->getfn() << ", handle = " << pHandle \
                                    << ", contribid = " << pContribId << ", sourceindex = " << e.sourceindex << ", size copied = " << e.len;
                                    break;

                                case BBFILE_FAILED:
                                    e.flags |= BBTD_Failed;
                                    LOG(bb,info) << "Local copy failed for file " << srcfile_ptr->getfn() << ", handle = " << pHandle \
                                    << ", contribid = " << pContribId << ", sourceindex = " << e.sourceindex;
                                    break;

                                case BBFILE_CANCELED:
                                case BBFILE_STOPPED:
                                default:
                                    // Not possible...
                                    break;
                            }

                            newlist.push_back(e);

                            // Cleanup the file handles...
                            filehandle* fh = 0;
                            rc = removeFilehandle(fh, pJobId, pHandle, pContribId, e.sourceindex);
                            if (rc == 0)
                            {
                                LOG(bb,info) << "Releasing filehandle " << transfer->files[e.sourceindex];
                                rc = fh->release(l_FileStatus);
                                if (rc)
                                {
                                    LOG(bb,error) << "Releasing the filehandle for srcfile " << transfer->files[e.sourceindex] << " failed, rc" << rc;
                                }
                                delete fh;
                                fh = 0;
                            }
                            rc = removeFilehandle(fh, pJobId, pHandle, pContribId, e.targetindex);
                            if (rc == 0)
                            {
                                LOG(bb,info) << "Releasing filehandle " << transfer->files[e.targetindex];
                                rc = fh->release(l_FileStatus);
                                if (rc)
                                {
                                    LOG(bb,error) << "Releasing the filehandle for dstfile " << transfer->files[e.targetindex] << " failed, rc" << rc;
                                }
                                delete fh;
                                fh = 0;
                            }
                            rc = 0;
                        }
                    }
                    else if((l_SourceFileIsLocal && (!l_TargetFileIsLocal)) ||
                            (l_BothLocal && (l_SimulateStageOut || l_SimulateFileStageOut)))
                    {
                        // Stageout processing...
                        transfer->setAll_CN_CP_TransfersInDefinition(0);
                        transfer->setNoStageinOrStageoutTransfersInDefinition(0);

                        e.flags |= BBI_TargetPFS;
                        rc = getLogicalVolumeUUID(transfer->files[e.sourceindex], l_current_lvuuid);
                        if (!rc) {
                            if (l_lvuuid.is_null()) {
                                // NOTE:  We get the lvuuid from the first extent transfer local source/target file.
                                //        Long term, we probably need to verify that the lvuuid we found is indeed for
                                //        the LV for this job...
                                // \todo - @DLH
                                l_lvuuid = l_current_lvuuid;
                            } else {
                                if (l_lvuuid == l_current_lvuuid) {
                                    l_Flags |= BBI_TargetPFS;
                                } else {
                                    rc = -1;
                                    errorText << "All files in the transfer request being transferred from SSD -> PFS must reside on the same burst buffer logical volume";
                                    LOG_ERROR_TEXT_RC(errorText, rc);
                                    break;
                                }
                            }

                            if (pPerformOperation)
                            {
                                rc = srcfile_ptr->protect(0, srcfile_ptr->getsize(), false, e, newlist);
                                if (!rc) {
                                    e.lba.maxkey = e.lba.start+e.len;
                                    if (e.isBSCFS_Extent()) {
                                        transfer->setBSCFS_InRequest();
                                    }
                                } else {
                                    errorText << "Could not build extents for sourceindex " << e.sourceindex;
                                    LOG_ERROR_TEXT_RC(errorText, rc);
                                    break;
                                }
                            }
                        } else {
                            errorText << "Could not determine the uuid for the logical volume associated with sourceindex " << e.sourceindex << ". The mountpoint may not exist.";
                            LOG_ERROR_TEXT_RC(errorText, rc);
                            break;
                        }
                    }
                    else if(l_TargetFileIsLocal)
                    {
                        // Stagein processing...
                        transfer->setAll_CN_CP_TransfersInDefinition(0);
                        transfer->setNoStageinOrStageoutTransfersInDefinition(0);

                        e.flags |= BBI_TargetSSD;
                        rc = getLogicalVolumeUUIDForPath(transfer->files[e.targetindex], l_current_lvuuid);
                        if (!rc)
                        {
                            if (l_lvuuid.is_null())
                            {
                                // NOTE:  We get the lvuuid from the first extent transfer local source/target file.
                                //        Long term, we probably need to verify that the lvuuid we found is indeed for
                                //        the LV for this job...
                                // \todo - @DLH
                                l_lvuuid = l_current_lvuuid;
                            }
                            else
                            {
                                if (l_lvuuid == l_current_lvuuid)
                                {
                                    l_Flags |= BBI_TargetSSD;
                                }
                                else
                                {
                                    rc = -1;
                                    errorText << "All files in the transfer request being transferred from PFS -> SSD must reside on the same burst buffer logical volume";
                                    LOG_ERROR_TEXT_RC(errorText, rc);
                                    break;
                                }
                            }

                            if (!rc)
                            {
                                if (pPerformOperation)
                                {
                                    if (transfer->builtViaRetrieveTransferDefinition())
                                    {
                                        // NOTE:  For restart, we have to close and re-open the
                                        //        target file so that finalize will run to allow
                                        //        a truncate and resize...

                                        filehandle* fh;
                                        // Remove the target file (if open)
                                        if (!removeFilehandle(fh, pJobId, pHandle, pContribId, e.targetindex))
                                        {
                                            LOG(bb,info) << "Releasing target filehandle '" << transfer->files[e.targetindex] << "' before restart";
                                            rc = fh->release(BBFILE_STOPPED);
                                            delete fh;
                                            fh = NULL;
                                            if (rc)
                                            {
                                                LOG(bb,error) << "Releasing the filehandle for dstfile " << transfer->files[e.targetindex] << " failed, rc " << rc;
                                                rc = -1;
                                                break;
                                            }
                                        }

                                        if (!rc)
                                        {
                                            // Now, just to make sure, attempt to find a filehandle for the target file...
                                            rc = findFilehandle(dstfile_ptr, pJobId, pHandle, pContribId, e.targetindex);
                                            if (rc == -1)
                                            {
                                                // As expected, not found...
                                                rc = 0;
                                                if (e.flags & BBRecursive)
                                                {
                                                    bfs::create_directories(bfs::path(transfer->files[e.targetindex]).parent_path());
                                                }

                                                int l_OpenFlags = (O_CREAT | O_TRUNC | O_WRONLY);
                                                // Open the target file...
                                                dstfile_ptr = new filehandle(transfer->files[e.targetindex], l_OpenFlags, srcfile_ptr->getmode());
                                                if (dstfile_ptr->getfd() >= 0)
                                                {
                                                    addFilehandle(dstfile_ptr, pJobId, pHandle, pContribId, e.targetindex);
                                                }
                                                else
                                                {
                                                    rc = -1;
                                                    LOG(bb,error) << "Creating the filehandle for dstfile: " << transfer->files[e.targetindex] << " failed.";
                                                    break;
                                                }
                                            }
                                            else
                                            {
                                                // Should never be the case...
                                                rc = -1;
                                                LOG(bb,error) << "Finding the filehandle for dstfile: " << transfer->files[e.targetindex] << " failed.";
                                                break;
                                            }
                                        }
                                    }

                                    if (!rc)
                                    {
                                        dstfile_ptr->setsize(srcfile_ptr->getsize());
                                        rc = dstfile_ptr->protect(0, srcfile_ptr->getsize(), true, e, newlist);
                                        if(!rc)
                                        {
                                            e.lba.maxkey = e.lba.start+e.len;
                                            if (e.isBSCFS_Extent())
                                            {
                                                transfer->setBSCFS_InRequest();
                                            }
                                        }
                                        else
                                        {
                                            errorText << "Could not build extents for targetindex " << e.targetindex;
                                            LOG_ERROR_TEXT_RC(errorText, rc);
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            errorText << "Could not determine the uuid for the logical volume associated with targetindex " << e.targetindex << ". The mountpoint may not exist.";
                            LOG_ERROR_TEXT_RC(errorText, rc);
                            break;
                        }
                    }
                    else
                    {
                        // Remote->remote copy processing...
                        transfer->setAll_CN_CP_TransfersInDefinition(0);
                        e.flags |= BBI_TargetPFSPFS;

                        if (pPerformOperation)
                        {
                            e.setCP_Tansfer();
                            e.len = srcfile_ptr->getsize();

                            e.flags |= BBI_First_Extent;
                            e.flags |= BBI_Last_Extent;

                            LOG(bb,info) << "Files reside on PFS, copy performed on PFS";

                            newlist.push_back(e);
                        }
                    }
                }
                else
                {
                    // File being skipped...  Perform closes...

                    if ((e.flags & BBI_TargetSSD) || (e.flags & BBI_TargetPFS))
                    {
                        try
                        {
                            filehandle* fh;
                            // Remove both source and target files (if open)
                            if (!removeFilehandle(fh, pJobId, pHandle, pContribId, e.sourceindex))
                            {
                                LOG(bb,info) << "Releasing filehandle " << fh->getfn();
                                rc = fh->release(BBFILE_NOT_TRANSFERRED);
                                delete fh;
                                fh=NULL;
                                if (rc)
                                {
                                    LOG(bb,error) << "Releasing the filehandle for srcfile (for skipped file) " << transfer->files[e.sourceindex] << " failed, rc " << rc;
                                    rc = -1;
                                    break;
                                }
                            }

                            if (!removeFilehandle(fh, pJobId, pHandle, pContribId, e.targetindex))
                            {
                                LOG(bb,info) << "Releasing filehandle " << fh->getfn();
                                rc = fh->release(BBFILE_NOT_TRANSFERRED);
                                delete fh;
                                fh = NULL;
                                if (rc)
                                {
                                    LOG(bb,error) << "Releasing the filehandle for tgtfile (for skipped file) " << transfer->files[e.targetindex] << " failed, rc " << rc;
                                    rc = -1;
                                    break;
                                }
                            }
                        }
                        catch(exception& e)
                        {
                            LOG(bb,warning) << "Exception thrown when processing transfer complete for file: " << e.what();
                        }
                    }

                    LOG(bb,info) << "Transfer of source file " << transfer->files[e.sourceindex] << " to " << transfer->files[e.targetindex] << " is not being restarted because" \
                                 << " the source file was not in a stopped state. It may not have been in a stopped state because the transfer of all extents" \
                                 << " had already completed. See previous messages.";
                }

                l_AllowLocalCopy = l_SavedAllowLocalCopy;
            }
        }

        if (!rc)
        {
            if (pPerformOperation)
            {
                // Transfer the newly constructed vector of extents into the Extent vector for the transfer...
                transfer->extents.clear();

                LOG(bb,info) << "bbProxy has determined that " << newlist.size() \
                             << " extent(s) will be passed to bbServer for the " << (transfer->files.size())/2 \
                             << " source file(s) associated with jobid " \
                             << pJobId << ", handle " << pHandle << ", contrib " << pContribId;

                for(auto e : newlist)
                {
                    LOOP_COUNT(__FILE__,__FUNCTION__,"extents_to_transfer");
                    transfer->extents.push_back(e);
                }

                if (transfer->builtViaRetrieveTransferDefinition())
                {
                    // Indicate that bbproxy will skip all source/target file pairs from
                    // the last pair processed up through the end of the file list...
                    // NOTE: We have a dependency that bbServer will always insert extents into
                    //       the extent vector in ascending sourcefile index order...
                    while ((size_t)l_NextSourceIndexToProcess < (transfer->files).size())
                    {
                        LOG(bb,info) << "bbProxy will not restart the transfer for the source file associated with jobid " \
                                     << pJobId << ", handle " << pHandle << ", contrib " << pContribId \
                                     << ", source index " << l_NextSourceIndexToProcess << ", transfer type UNKNOWN";
                        l_NextSourceIndexToProcess += 2;
                    }
                }
            }

            // NOTE: If BSCFS files end up also being transferred to bbServer for this LVUuid, bbServer will
            //       disable the BBTD_Resize_Logical_Volume_During_Stageout flag for that particular work queue.
            if (config.get(process_whoami+".freeStorageDuringStageOut", false) && (l_Flags & BBI_TargetPFS))
            {
                transfer->setResizeLogicalVolumeDuringStageout();
            }

            lvuuid = l_lvuuid;
            LOG(bb,info) << "setupTransfer: LV uuid = " << lvuuid;
        }
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}


int startTransfer(BBTransferDef* transfer, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId)
{
    int rc = 0;
    stringstream errorText;

    uint32_t l_PerformOperation = 0;
    uint32_t l_MarkFailed = 0;

    vector<struct stat*> l_Stats;
    char l_Empty = '\0';
    bool releaseFileHandles = false;

    ResponseDescriptor reply;
    txp::Msg* msgserver = 0;
    std::string l_ConnectionName;

    try
    {
        for (uint32_t i=0; i<2; ++i)
        {
            LOOP_COUNT(__FILE__,__FUNCTION__,"passes");
            vector<txp::CharArray> freelist;    ///< Deallocates any marshalled CharArrays between message sends
            {
                l_PerformOperation = i;
                if ((!l_PerformOperation) || transfer->files.size())
                {
                    Uuid lv_uuid;
                    rc = setupTransfer(transfer, lv_uuid, pJobId, pHandle, pContribId, l_Stats, l_PerformOperation);
                    if (rc)
                    {
                        // NOTE:  errstate should already be filled in, but if not, set error text here...
                        errorText << "setupTransfer failure";

                        // NOTE: We cannot simply error out here.  We need to send a message
                        //       to bbServer indicating that bbProxy has encountered an error when
                        //       attempting to start this transfer definition.  bbServer will either
                        //       return an overridding condition which may or may not mark the
                        //       transfer definition and handle as failed -or- if bbServer does not have
                        //       an overriding condition, it will return rc as -1 and mark the
                        //       transfer definition and handle as failed (due to the bbProxy error).
                        l_MarkFailed = 1;
                        rc = 0;
                    }

                    txp::Msg::buildMsg(txp::BB_STARTTRANSFER, msgserver);

                    msgserver->addAttribute(txp::handle, pHandle);
                    msgserver->addAttribute(txp::contribid, pContribId);
                    char lv_uuid_str[LENGTH_UUID_STR] = {'\0'};
                    lv_uuid.copyTo(lv_uuid_str);

                    LOG(bb,debug) << "LV Uuid = " << lv_uuid;

                    // NOTE:  The char array is copied to heap by addAttribute and the storage for
                    //        the logical volume uuid attribute is owned by the message facility.
                    //        Our copy can then go out of scope...
                    msgserver->addAttribute(txp::uuid, lv_uuid_str, sizeof(lv_uuid_str), txp::COPY_TO_HEAP);
                    string l_HostName;
                    activecontroller->gethostname(l_HostName);
                    msgserver->addAttribute(txp::hostname, l_HostName.c_str(), l_HostName.size()+1, txp::COPY_TO_HEAP);
                    msgserver->addAttribute(txp::performoperation, l_PerformOperation);
                    msgserver->addAttribute(txp::markfailed, l_MarkFailed);

                    vector<txp::CharArray> l_StatArray;
                    l_StatArray.resize(1);
                    for(auto e : l_Stats)
                    {
                        if (e != NULL)
                        {
                            l_StatArray[0].push_back(make_pair(sizeof(*e), (char*)e));
                        }
                        else
                        {
                            l_StatArray[0].push_back(make_pair(1, &l_Empty));
                        }
                    }
                    msgserver->addAttribute(txp::statinfo, &l_StatArray[0]);

//                    LOG(bb,info) << "Before marshallTransferDef: l_Transfer.extents.size()=" << l_Transfer.extents.size();
                    BBTransferDef::marshallTransferDef(msgserver, transfer, freelist);

                    // Send the message to bbserver
                    // First sendMessage uses DEFAULT_SERVER_ALIAS and then the real name after
                    if (l_ConnectionName.empty())
                    {
                        l_ConnectionName = connectionNameFromAlias();
                    }
                    rc=sendMessage2bbserver(l_ConnectionName, msgserver, reply);
                    delete msgserver;
                    msgserver=NULL;
                    if (rc)
                    {
                        errorText << "sendMessage to server failed";
                        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                    }

                    // Wait for the response
                    rc = waitReplyNoErase(reply, msgserver);
                    if (rc)
                    {
                        errorText << "waitReply failure";
                        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                    }

                    if (!msgserver)
                    {
                        rc = -1;
                        errorText << "waitReply failure - null message returned";
                        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                    }

                    // Process response data
                    // NOTE: If l_MarkFailed was on, then bbServer will return the rc
                    //       as the value from an overridding error condition on bbServer -or-
                    //       as -1 if there was no additional error on bbServer and
                    //       it was determined that the transfer definition was still
                    //       to be started.  If it was determined that the transfer
                    //       definition was not to be started (restart case), then
                    //       rc will be set to -2 (tolerated exception) and the
                    //       l_MarkFailed indication will have been turned off by bbServer.
                    rc = bberror.merge(msgserver);

                    if (!rc)
                    {
                        l_MarkFailed = ((txp::Attr_uint32*)msgserver->retrieveAttrs()->at(txp::markfailed))->getData();
                        if (!l_PerformOperation)
                        {
                            txp::CharArray* l_StatArray = (txp::CharArray*)msgserver->retrieveAttrs()->at(txp::statinfo)->getDataPtr();
                            size_t l_NumStats = l_Stats.size();
                            for(size_t i=0; i<l_NumStats; i++)
                            {
                                if ((*l_StatArray)[i].first != 1)
                                {
                                    if (l_Stats[i] == NULL)
                                    {
                                        l_Stats[i] = new(struct stat);
                                    }
                                    // NOTE: Must copy the stats back even if bbProxy already has the stats structure
                                    //       for a given source file.  Restart may have bbServer override and not have
                                    //       one or more source files restarted and that is conveyed via the stats structure.
                                    memcpy((void*)l_Stats[i], ((*l_StatArray)[i].second), ((*l_StatArray)[i].first));
                                }
                            }
                        }
                    }
                    else
                    {
                        BAIL;
                    }
                    delete msgserver;
                    msgserver = NULL;
                }
            }
        }
    }
    catch(ExceptionBailout& e)
    {
        releaseFileHandles = true;
    }
    catch(exception& e)
    {
        rc = -1;
        releaseFileHandles = true;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }
    waitReplyErase(reply);

    if (releaseFileHandles)
    {
        if (rc != -2)
        {
            LOG(bb, error) << "Failure occurred during StartTransfer.  Removing any opened files for jobid=" << pJobId << "  handle=" << pHandle << "  contrib=" << pContribId;
        }

        for(unsigned int index=0; index<transfer->files.size(); index++)
        {
            LOOP_COUNT(__FILE__,__FUNCTION__,"released_handles");
            filehandle* fh;
            if(!removeFilehandle(fh, pJobId, pHandle, pContribId, index))
            {
                string l_FileName = fh->getfn();
                LOG(bb,info) << "Releasing filehandle '" << l_FileName << "'";
                rc = fh->release(BBFILE_FAILED);
                if (rc)
                {
                    errorText << "Releasing the filehandle " << l_FileName << " failed, rc " << rc;
                    LOG_ERROR_TEXT(errorText);
                }
                delete fh;
                fh = NULL;
            }
        }
    }

    if (msgserver)
    {
        delete msgserver;
        msgserver = NULL;
    }

    for(auto e : l_Stats)
    {
        if (e)
        {
            delete e;
        }
    }

    if (rc < 0 && l_MarkFailed)
    {
        // bbProxy still 'owns' the error condition.
        // The error text may not have been set yet so
        // set it with the error condition captured above.
        LOG_ERROR_TEXT(errorText);
    }

    return rc;
}


int timeToResizeLogicalVolume(const uint64_t pLastByteTransferred, const uint64_t pLVStartByte, const size_t pLVTotalSize, char* pNewSize, size_t pNewSizeLength) {
    ENTRY(__FILE__,__FUNCTION__);

    int rc = 0;

    // NOTE:  pLastByteTransferred will be passed as zero if l_MaxLBA or l_Offset or l_Length was not passed back from bbServer.
    //        In that case, we do not attempt a resize operation...
    //        pNewSizeLength should always be bassed as a positive value.
    if (pLastByteTransferred && pNewSizeLength)
    {
        // LVM resize requires that if you specify the new size in 'bytes', that it be a multiple of the sector size.
        // Therefore, we add the length of one sector to the last byte transferred, and then round down to get
        // correct sector aligned address.  Note, that adding the length of an entire sector ensures that we at least
        // truncate the LV back to one byte past the last byte transferred.  @DLH
        snprintf(pNewSize, pNewSizeLength, "%luB", ((pLastByteTransferred-pLVStartByte+SECTOR_SIZE)/SECTOR_SIZE)*SECTOR_SIZE);
        rc = 1;
    }

    EXIT(__FILE__,__FUNCTION__);
    return rc;
}
