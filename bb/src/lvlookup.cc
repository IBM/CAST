/*******************************************************************************
 |    lvlookup.cc
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

#include <stdio.h>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "bbproxy_flightlog.h"
#include "logging.h"
#include "LVLookup.h"

using namespace std;
#include "Extent.h"
#include "fh.h"

int LVLookup::build(const string& pVolumeGroup, const string& pLogicalVolumeName)
{
    setLVName(pVolumeGroup + string("-") + pLogicalVolumeName);

    return getLVExtents(pVolumeGroup);
}


int LVLookup::build(const filehandle& fh, const string& vg)
{
    int rc = getLVName(fh);
    if (!rc)
    {
        rc = getLVExtents(vg);
    }
    return rc;
}


int LVLookup::getLVName(const filehandle& fileh)
{
    FILE* f;
    char* buffer = NULL;
    size_t buffersize = 0;
    ssize_t frc;
    lvname = "unknown";

    struct stat statbuf;
    int rc = fstat(fileh.getfd(), &statbuf);
    if (rc)
    {
        FL_Write(FLExtents, fstatFailed, "getLVName: fstat returned rc=%d", rc, 0, 0, 0);
        string filename = fileh.getfn();
        LOG(bb,error) << "getLVName: fstat for " << filename << ", file descriptor " << fileh.getfd() << " returned rc=" << rc;

        return rc;
    }

    string dmpath = string("/sys/dev/block/") + to_string(major(statbuf.st_dev)) + ":" + to_string(minor(statbuf.st_dev)) + "/dm/name";
    f = fopen(dmpath.c_str(), "r");
    if (f)
    {
        frc = getline(&buffer, &buffersize, f);
        fclose(f);
        if(frc < 0)
        {
            FL_Write(FLExtents, getLVNameFailed1, "getLVName: Reading /sys/dev/block/%d:%d/dm/name for fd=%d returned no data", major(statbuf.st_dev), minor(statbuf.st_dev), fileh.getfd(), 0);
            string filename = fileh.getfn();
            LOG(bb,error) << "getLVName: " << dmpath.c_str() << " returned no data for file " << filename << ", file descriptor " << fileh.getfd();

            return -1;
        }
        setLVName(buffer);
        free(buffer);
        while(lvname.back() == '\n')
            lvname.pop_back();
        return 0;
    }

    FL_Write(FLExtents, getLVNameFailed2, "getLVName: Reading /sys/dev/block/%d:%d/dm/name for fd=%d failed", major(statbuf.st_dev), minor(statbuf.st_dev), fileh.getfd(), 0);
    string filename = fileh.getfn();
    LOG(bb,error) << "getLVName: " << dmpath.c_str() << " failed for file " << filename << ", file descriptor " << fileh.getfd();

    return -1;
}


// For input sizes only, which are base-2 only regardless of capitalization
// For lvs/vgs utilities, we'd use a different method.  Those tools display uppercase are base-10, lowercase are base-2.  
static map<string, size_t> unitconvert = {{"k",1ULL<<10}, {"m",1ULL<<20}, {"g",1ULL<<30}, {"t",1ULL<<40},
                                          {"K",1ULL<<10}, {"M",1ULL<<20}, {"G",1ULL<<30}, {"T",1ULL<<40},
                                          {"s",     512}, {"S", 512}, {"b", 1}, {"B", 1}};
int convertLVMSizeToBytes(const string& size, ssize_t& bytes)
{
    double value;
    string unit;

    value = stod(size);
    unit = size.substr(size.find_last_of("0123456789")+1);
    if(unitconvert.find(unit) != unitconvert.end())
        bytes = value * unitconvert[unit];
    else
        bytes = value;

    return 0;
}

int getVGSize(const string& vg, ssize_t& vgfree, ssize_t& vgtotal)
{
    string cmd = string("vgdisplay -C --noheadings --units b --nosuffix -o vg_free,vg_size ") + vg;

    for (const auto& line : runCommand(cmd))
    {
        auto toks = buildTokens(line, " ");
        LOG(bb,debug) << "tok1=" << toks[0] << "   tok2=" << toks[1];
        vgfree = stoul(toks[0]);
        vgtotal= stoul(toks[1]);
    }
    return 0;
}

int LVLookup::getData(uint64_t& pFirstByte, size_t& pSize)
{
    int rc = 0;

    if (LVMapping.find(lvname) != LVMapping.end())
    {
        pFirstByte = 0;
        pSize = 0;
        for(const auto& e : LVMapping[lvname])
        {
            // NOTE:  First byte should never be zero from the extent data...
            //        There is always an LVM header at least...
            if(!pFirstByte)
            {
                pFirstByte = e.pstart;
            }
            pSize += e.len;
        }
    }
    else
    {
        FL_Write(FLExtents, LVNameNotFound2, "Could not find LV name for translation", 0, 0, 0, 0);
        LOG(bb,error) << "Could not find LV '" << lvname << "'";
        rc = -1;
    }

    return rc;
}

int LVLookup::getLVExtents(const string& vg)
{
    string cmd = "lvs --noheadings --units b --nosuffix -o lv_name,seg_pe_ranges,seg_start_pe,vg_extent_size " + vg;
    FILE* f;
    ssize_t frc;
    char name[64];
    char device[64];
    int rc = 0;
    int pstart = 0;
    int pend = 0;
    int lstart;
    char size[64];
    ssize_t bytes;
    LVExtent tmp;

    f = popen(cmd.c_str(), "re");
    if (f)
    {
        char* buffer = NULL;
        size_t buffersize = 0;
        bool lineReturned = false, warningSent = false;
        while((frc = getline(&buffer, &buffersize, f)) >= 0)
        {
            rc = sscanf(buffer, "%s %[^:]:%d-%d %d %s", name, device, &pstart, &pend, &lstart, size);
            if (rc == 6)
            {
                bytes = stoul(size);
                
                tmp.device = device;
                tmp.pstart = pstart * bytes;
                tmp.lstart = lstart * bytes;
                tmp.len    = (pend-pstart+1) * bytes;

                // correct for LVM header size.
                // \todo retrieve offset via "sudo vgs --noheadings -opv_name,pe_start" command
                tmp.pstart += 1024*1024;

                struct stat statbuf;
                rc = stat(device, &statbuf);
                if (rc)
                {
                    free(buffer);
                    FL_Write(FLExtents, statFailed, "getLVExtents: stat command failed with rc=%d", rc, 0, 0, 0);
                    LOG(bb,error) << "getLVExtents: stat command failed with rc=" << rc;
                    return rc;
                }

                FL_Write6(FLExtents, LVFound, "Device %ld:%ld lstart=%p pstart=%p len=%ld bytes", major(statbuf.st_rdev), minor(statbuf.st_rdev), tmp.pstart, tmp.lstart, tmp.len,0);

                // correct for the start of the logical volume /sys/dev/block/xxx:xx/start  (if present)
                uint64_t devstart = 0;
                FILE* devf;
                string devpath = string("/sys/dev/block/") + to_string(major(statbuf.st_rdev)) + ":" + to_string(minor(statbuf.st_rdev));

                char realdevpath[PATH_MAX+1];
                string mydevice;
                frc = readlink(devpath.c_str(), realdevpath, PATH_MAX);//allow for /0 at PATH_MAX+1
                if(frc >= 0)
                {
                    realdevpath[frc] = 0;  // readlink does not NULL terminate!
                    string tmp = realdevpath;
                    mydevice = tmp.substr(tmp.rfind("/")+1);
                }
                else {
                    stringstream errorText;
                    errorText << __PRETTY_FUNCTION__<<" devpath="<<devpath<<" readlink had errno="<<errno<<" "<<strerror(errno);
                    LOG_ERROR_TEXT_ERRNO_AND_RAS(errorText, errno, bb.devpath.readlinkFailed);
                }

                devpath += "/start";
                devf = fopen(devpath.c_str(), "r");
                if (devf)
                {
                    int fscanfrc = fscanf(devf, "%ld", &devstart);
                    fclose(devf);
                    if(fscanfrc != 1)
                    {
                        free(buffer);
                        FL_Write(FLExtents, fstatFailed2, "getLVExtents: fstat command failed with rc=%d", rc, 0, 0, 0);
                        LOG(bb, error) << "getLVExtents: fstat command failed with rc=" << rc;
                        return rc;
                    }
                    devstart *= 512;

                    // mydevice currently points to the logical volume, but serial number resolution requires the actual raw device.
                    // Go up 1 level to obtain the name of the parent volume group.
                    string tmp = realdevpath;
                    tmp = tmp.substr(0, tmp.rfind("/"));
                    mydevice = tmp.substr(tmp.rfind("/")+1);
                }
                tmp.pstart += devstart;
                tmp.serial = getSerialByDevice(string("/dev/") + mydevice);
                FL_Write(FLExtents, LogicalVolumeOffset, "Device %ld:%ld has logical volume start at %p", major(statbuf.st_rdev), minor(statbuf.st_rdev), devstart,0);

                LOG(bb,debug) << "name=" << name << " devname=/dev/" << mydevice << " dev=" << tmp.device
                              << std::hex << std::uppercase << setfill('0')
                              << "  pstart=0x" << setw(16) << tmp.pstart
                              << "  lstart=0x" << setw(16) << tmp.lstart
                              << ", len=0x" << setw(12) << tmp.len
                              << setfill(' ') << std::nouppercase << std::dec
                              << "  serial=" << tmp.serial;

                LVMapping[vg + string("-") + name].push_back(tmp);
                lineReturned = true;
            } else {
                FL_Write(FLExtents, lvsUnexpectedData, "getLVExtents: Unexpected data returned from lvs command", 0, 0, 0, 0);
                LOG(bb,warning) << "getLVExtents: Unexpected data returned from lvs command: " << buffer;
                warningSent = true;
            }

            if (buffer)
            {
                free(buffer);
                buffer = NULL;
                buffersize = 0;
            }
        }

        fclose(f);

        if (warningSent) {
            LOG(bb,error) << "Warning messages send during the parsing of lvs data for volume group " << vg;
            rc = -1;
        }
        if (!lineReturned) {
            FL_Write(FLExtents, LVS_CmdNoData, "lvs command returned no data", 0, 0, 0, 0);
            LOG(bb,error) << "lvs command returned no data for volume group " << vg;
            rc = -1;
        }
    } else {
        FL_Write(FLExtents, LVS_CmdFailed, "lvs command failed", 0, 0, 0, 0);
        LOG(bb,error) << "lvs command failed for volume group " << vg;
        rc = -1;
    }

    return rc;
}

int LVLookup::translate(Extent& input, vector<Extent>& list)
{
    if (LVMapping.find(lvname) == LVMapping.end())
    {
        FL_Write(FLExtents, LVNameNotFound, "Could not find LV name for translation", 0, 0, 0, 0);
        LOG(bb,error) << "Could not find LV '" << lvname << "'";
        return -1;
    }

    FL_Write(FLExtents, FindLVExtent, "Searching for LVM extent(s) for lbastart=%ld   start=%ld  len=%ld", input.lba.start, input.start, input.len,0);
    while(input.len > 0)
    {
        for(const auto& e : LVMapping[lvname])
        {
            if((input.lba.start >= e.lstart) && (input.lba.start < e.lstart + e.len))
            {
                Extent tmp = input;
                tmp.lba.start = (input.lba.start - e.lstart) + e.pstart;
                tmp.len = MIN(e.len, tmp.len);
                tmp.lba.maxkey = tmp.lba.start + tmp.len;
                FL_Write6(FLExtents, FoundLVMExtent, "Found LVM extent needed for lbastart=%p.  Extent info: lstart=%p pstart=%p, len=%ld bytes.  New lbastart=%p for len=%ld bytes", input.lba.start, e.lstart, e.pstart, e.len, tmp.lba.start, tmp.len);
                input.lba.start += tmp.len;
                input.start += tmp.len;
                input.len -= tmp.len;

                if(e.serial.size()+1 >= sizeof(tmp.serial))
                    throw runtime_error(string("SSD serial number (" + e.serial + ") length is larger than maximum " + to_string(sizeof(tmp.serial)) + " bytes"));

                memset(tmp.serial, 0, sizeof(tmp.serial));
                strncpy(tmp.serial, e.serial.c_str(), sizeof(tmp.serial));

                list.push_back(tmp);
            }
        }
    }
    return 0;
}
