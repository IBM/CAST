/*******************************************************************************
 |    util.cc
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


#include <cstring>
#include <mntent.h>
#include <stdio.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <linux/types.h>
#include <linux/fuse.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <vector>

#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <boost/filesystem.hpp>

#include "stdint.h"
#include "stdlib.h"
#include "util.h"
#include "logging.h"
#include "tstate.h"

namespace bfs = boost::filesystem;

//*****************************************************************************
//  Static data (symbols are exported)
//*****************************************************************************
Config curConfig = Config();
pt::ptree config;

//*****************************************************************************
//  Data members
//*****************************************************************************
void copyStat2attr_out(const struct stat& s,struct fuse_attr* attr) {
  attr->ino = s.st_ino;
  attr->size = (__u64) s.st_size;
  attr->blocks = (__u64)s.st_blocks; /* number of 512B blocks allocated */
  attr->atime = (__u64)s.st_atime;
  attr->mtime = (__u64)s.st_mtime;
  attr->ctime = (__u64)s.st_ctime;
#ifndef __APPLE__
  attr->atimensec = (__u32)s.st_atim.tv_nsec; /*nanoseconds*/
  attr->mtimensec = (__u32)s.st_mtim.tv_nsec;
  attr->ctimensec = (__u32)s.st_ctim.tv_nsec;
#endif
  attr->mode = s.st_mode;
  attr->nlink = (__u32)s.st_nlink; /* number of hard links */
  attr->uid = s.st_uid;
  attr->gid = s.st_gid;
  attr->rdev = (__u32)s.st_rdev; /* device ID if special file */
  attr->blksize = (__u32)s.st_blksize;
}

void getLogicalVolumeDevName(const Uuid& uuid, char* pDevName, size_t pLength)
{
    bfs::path uuidpath("/dev/disk/by-uuid");
    bfs::path mapper("/dev/mapper");
    uuidpath /= uuid.str();
    for (const auto& vglv : boost::make_iterator_range(bfs::directory_iterator(mapper), {}))
    {
        if(bfs::equivalent(uuidpath, vglv))
        {
            size_t l_Index1 = vglv.path().string().rfind("/");
            size_t l_Index2 = vglv.path().string().rfind("-");
            std::string vg = vglv.path().string().substr(l_Index1+1, ((l_Index2-l_Index1)-1));
            std::string lv = vglv.path().string().substr(vglv.path().string().rfind('-')+1);
            snprintf(pDevName, pLength, "/dev/%s/%s", vg.c_str(), lv.c_str());
            break;
        }
    }

    return;
}

int getIPPort(std::string pUrl, std::string& pIPAddress, uint16_t& pPort)
{
    int l_RC = 0;

    boost::regex ex("^(.+?):([0-9]+)$");
    boost::match_results<std::string::const_iterator> what;
    if(boost::regex_search(pUrl, what, ex)) {
        pIPAddress = std::string(what[1].first, what[1].second);
        pPort = atoi(std::string(what[2].first, what[2].second).c_str());
    } else {
        l_RC = -1;
    }

    return l_RC;
}

int getIPAddrByInterface(const std::string& pInterface, std::string& pIPAddress)
{
    int l_RC = 0;

    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, pInterface.c_str(), IFNAMSIZ-1);
    if (!ioctl(fd, SIOCGIFADDR, &ifr)) {
//        printf("getIPAddrByInterface: pInterface=%s, pIPAddress=%s\n", pInterface.c_str(), inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
        pIPAddress.assign(inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    } else {
        l_RC = -errno;
    }

    close(fd);
    return l_RC;
}

int getLogicalVolumeDeviceName(const std::string pFile, std::string &pLogicalVolumeDeviceName)
{
    int rc;
    struct stat statbuf;
    rc = stat(pFile.c_str(), &statbuf);
    if (rc)
        return rc;

    std::string dmpath = std::string("/sys/dev/block/") + std::to_string(major(statbuf.st_dev)) + ":" + std::to_string(minor(statbuf.st_dev)) + "/dm/name";
    auto lines = runCommand(dmpath, true);
    if(lines.size() == 0)
        return -1;

    pLogicalVolumeDeviceName = std::string("/dev/mapper/") + lines[0];
    return 0;
}

int getUUID(const char* pLogicalVolumeDeviceName, Uuid& pUuid)
{
    bfs::path uuidpath("/dev/disk/by-uuid");
    bfs::path vglv(pLogicalVolumeDeviceName);
    for(const auto& uuid : boost::make_iterator_range(bfs::directory_iterator(uuidpath), {}))
    {
        if(bfs::equivalent(uuid, vglv))
        {
            LOG(bb,always) << "uuid=" << uuid.path().filename().string();
            pUuid.copyFrom(uuid.path().filename().string().c_str());
            return 0;
        }
    }
    return -1;
}

int getLogicalVolumeUUID(const std::string& pFile, Uuid& pUuid)
{
    int rc = 0;
    pUuid.clear();

    std::string l_LogicalVolumeDeviceName;
    rc = getLogicalVolumeDeviceName(pFile, l_LogicalVolumeDeviceName);
    if (!rc) {
        rc = getUUID(l_LogicalVolumeDeviceName.c_str(), pUuid);
    }

    return rc;
}

int getLogicalVolumeUUIDForPath(const std::string& pFile, Uuid& pUuid)
{
    int rc = 0;
    pUuid.clear();

    bfs::path l_File = bfs::path(pFile);
    if (bfs::exists(l_File.parent_path()))
    {
        std::string l_LogicalVolumeDeviceName;
        rc = getLogicalVolumeDeviceName(l_File.parent_path().string(), l_LogicalVolumeDeviceName);
        if (!rc) {
            rc = getUUID(l_LogicalVolumeDeviceName.c_str(), pUuid);
        }
    }

    return rc;
}

int isDeviceMounted(char* pDevName, char* &pMountPoint, size_t pMountPointLength)
{
    int l_RC = 0;

    struct mntent* l_Entry;
    FILE* l_File;

    if (pMountPoint && pMountPointLength>0) {
        pMountPoint[0] = 0;
    }

    l_File = setmntent("/proc/mounts", "r");
    if (l_File) {
        while ((l_Entry = getmntent(l_File)) != NULL && !l_RC) {
            if (strstr(l_Entry->mnt_fsname, pDevName)) {
                l_RC = 1;
                if (pMountPoint && pMountPointLength>0) {
                    strCpy(pMountPoint, l_Entry->mnt_dir, pMountPointLength);
                }
                break;
            }
        }
    }

    return l_RC;
}

int isFileSystemMounted(char* pPath)
{
    int l_RC = 0;

    struct mntent* l_Entry;
    FILE* l_File;

    l_File = setmntent("/proc/mounts", "r");
    if (l_File) {
        while ((l_Entry = getmntent(l_File)) != NULL && !l_RC) {
            if (strstr(l_Entry->mnt_dir, pPath)) {
                l_RC = 1;
                break;
            }
        }
    }

    return l_RC;
}

int leaveLVM_BreadCrumb(const char* pMountPoint, const Uuid& uuid, const char* pDevName)
{
    int l_RC = 0;

    // Leave a bread crumb in the mount point directory so we know the name of the last associated logical volume
    char l_Cmd[1024] = {'\0'};
    snprintf(l_Cmd, sizeof(l_Cmd), "sudo echo > %s/%s %s %s >> %s/%s 2>&1;", pMountPoint, CORAL_LVM_BREADCRUMB.c_str(), uuid.str().c_str(), pDevName, pMountPoint, CORAL_LVM_BREADCRUMB.c_str());

    auto lines = runCommand(l_Cmd);
    if(lines.size() > 0)
    {
        // No expected output...
        l_RC = -1;
    }
    return l_RC;
}

int retrieveLVM_BreadCrumb(const std::string pMountPoint, Uuid* pUuid, std::string* pDevName)
{
    int l_RC = 0;
    if (pUuid)
        pUuid->clear();
    if (pDevName)
        *pDevName = "";

    FILE* l_Fp;
    char l_Line[LINE_MAX] = {'\0'};
    char lv_uuid_str[LENGTH_UUID_STR] = {'\0'};
    char l_DevName[PATH_MAX] = {'\0'};

    char l_LVM_BreadCrumbFile[PATH_MAX] = {'\0'};
    snprintf(l_LVM_BreadCrumbFile, sizeof(l_LVM_BreadCrumbFile), "%s/%s", pMountPoint.c_str(), CORAL_LVM_BREADCRUMB.c_str());

    l_Fp = fopen(l_LVM_BreadCrumbFile, "r");
    if (l_Fp) {
        fgets(l_Line, sizeof(l_Line), l_Fp);
        fclose(l_Fp);

        l_RC = sscanf(l_Line, "%s %s", lv_uuid_str, l_DevName);
        if (l_RC == 2) {
            l_RC = 0;
            if (pUuid)
                *pUuid = Uuid(lv_uuid_str);
            if (pDevName)
                *pDevName = l_DevName;
        } else {
            l_RC = -2;
        }

    } else {
        l_RC = -1;
    }

    return l_RC;
}


const char ERROR_PREFIX[] = "ERROR - ";

/* NOTE:  This routine will *ALWAYS* return a vector of strings.
 *        If the returned vector has a size of zero, then the command ran successfully,
 *        but yielded no output.
 *
 *        For each 'line' of output returned by the command, a string is built and inserted
 *        into the vector to be returned.  Any trailing line feed character is stripped from
 *        the output before the string is constructed.  If stripping the line feed character
 *        leaves an empty string, that empty string is *NOT* inserted into the vector.
 *
 *        If the open failed for the command or there was a read failure, a string is built
 *        containing the error information and is inserted into the vector of strings with
 *        a predefined prefix given by ERROR_PREFIX.  Note that such error strings are *NOT*
 *        logged and it is recommended that the invoker of this routine log such error strings.
 */
std::vector<std::string> runCommand(const std::string& cmd, bool flatfile,bool noException)
{
    FILE* f = NULL;
    ssize_t frc;
    char* buffer = NULL;
    size_t buffersize = 0;
    std::vector<std::string> output;
    TSHandler runCommandError;
    int readError=0;

    if(flatfile)
    {
        LOG(bb,always) << "Reading: " << cmd;
        f = fopen(cmd.c_str(), "r");
    }
    else
    {
        LOG(bb,always) << "Executing: " << cmd;
        f = popen(cmd.c_str(), "r");
    }

    if (f)
    {
        while((frc = getline(&buffer, &buffersize, f)) >= 0)
        {
            if (frc)
            {
                char* nl = strchr(buffer+frc-1, '\n');
                if (nl)
                    *nl = 0;
                if (strlen(buffer))
                    output.push_back(buffer);
            }

            if (buffer)
            {
                free(buffer);
                buffer = NULL;
            }
            buffersize = 0;
        }

        // Even if getline() 'fails', we should check to free the buffer...
        if (buffer)
        {
            free(buffer);
            buffer = NULL;
        }

        if (ferror(f))
        {
            std::stringstream errorText;
            errorText << ERROR_PREFIX << "Read failure, errno=" << errno << " (" << strerror(errno) << ")";
            output.push_back(errorText.str());
            runCommandError << err("error.cmd",cmd)<<errloc(errno) << err("error.read", strerror(errno))<<bailout;
            if (errno) errno=readError;
        }

        if (flatfile)
            fclose(f);
        else
            pclose(f);
    }
    else
    {
        std::stringstream errorText;
        errorText << ERROR_PREFIX << "Open failure, errno=" << errno << " (" << strerror(errno) << ")";
        output.push_back(errorText.str());
        runCommandError << err("error.cmd",cmd) <<errloc(errno) << err("error.open", strerror(errno)) << bailout;
    }

    return output;
}

std::vector<std::string> buildTokens(const std::string& str, const char* sepchar)
{
    std::vector<std::string> values;
    boost::char_separator<char> sep(sepchar);
    boost::tokenizer< boost::char_separator<char>  > tok(str, sep);
    for(boost::tokenizer< boost::char_separator<char> >::iterator beg=tok.begin(); beg!=tok.end();++beg)
        values.push_back(*beg);

    return values;
}

int fuzzyMatch(const std::string& pLine, std::vector<std::string>& pSubStrings)
{
    int l_RC = 1;

    size_t l_StartPos = 0, l_PrevStartPos = 0;
    for (auto& l_SubString : pSubStrings)
    {
        size_t l_Pos = pLine.find(l_SubString, l_StartPos);
        if (l_Pos != std::string::npos)
        {
            l_StartPos += (l_Pos - l_PrevStartPos) + l_SubString.size();
            l_PrevStartPos = l_StartPos;
        }
        else
        {
            l_RC = 0;
            break;
        }
    }

    return l_RC;
}
