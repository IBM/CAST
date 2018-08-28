/*******************************************************************************
 |    fh.cc
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


#define _XOPEN_SOURCE 600
#include <fcntl.h>
#include <map>
#include <pthread.h>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fiemap.h>
#include <linux/fs.h>
#include <linux/magic.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <sys/fsuid.h>

using namespace std;

#include "bbinternal.h"
#include "export_layout.h"
#include "Extent.h"
#include "fh.h"
#include "identity.h"
#include "logging.h"
#include "LVLookup.h"
#include "usage.h"
#include "util.h"
#include "connections.h"

#if BBAPI
#include "bbapi_flightlog.h"
#define FLCOMP FLapi
#endif

#if BBSERVER
#include "bbserver_flightlog.h"
#include "tracksyscall.h"
#endif

#if BBPROXY
#include "bbproxy_flightlog.h"
#include "tracksyscall.h"
#endif

// NOTE:  This mutex is acquired around access to the file handle registry
pthread_mutex_t FileHandleRegistryMutex = PTHREAD_MUTEX_INITIALIZER;


//*****************************************************************************
//  Static methods
//*****************************************************************************


void FileHandleRegistryLock() {
    pthread_mutex_lock(&FileHandleRegistryMutex);

    return;
}


void FileHandleRegistryUnlock() {
    pthread_mutex_unlock(&FileHandleRegistryMutex);

    return;
}


//*****************************************************************************
//  Classes
//*****************************************************************************


class extentLookup
{
protected:
    bool fetched;
    const filehandle* fh;
    virtual int fetch() = 0;
public:
    extentLookup(const filehandle* fileh) : fetched(false), fh(fileh) { };
    virtual ~extentLookup() {};
    virtual int size(unsigned& numentries) = 0;
    virtual int get(unsigned index, uint64_t& lba, uint64_t& start, uint64_t& len) = 0;
    virtual int release(BBFILESTATUS completion_status) = 0;
};

class filehandleLocator
{
public:
    uint64_t jobid;
    uint64_t handle;
    uint64_t contrib;
    uint64_t fileindex;

    inline bool operator== (const filehandleLocator& pOther) const
    {
        return (jobid == pOther.jobid && handle == pOther.handle && contrib == pOther.contrib && fileindex == pOther.fileindex);
    }

    inline bool operator!= (const filehandleLocator& pOther) const
    {
        return !(*this==pOther);
    }

    inline bool operator> (const filehandleLocator& pOther) const
    {
        if(jobid == pOther.jobid)
        {
            if(handle == pOther.handle)
            {
                if(contrib == pOther.contrib)
                {
                    return (fileindex > pOther.fileindex);
                }
                return (contrib > pOther.contrib);
            }
            return (handle > pOther.handle);
        }
        return (jobid > pOther.jobid);
    }

    inline bool operator< (const filehandleLocator& pOther) const
    {
        if(jobid == pOther.jobid)
        {
            if(handle == pOther.handle)
            {
                if(contrib == pOther.contrib)
                {
                    return (fileindex < pOther.fileindex);
                }
                return (contrib < pOther.contrib);
            }
            return (handle < pOther.handle);
        }
        return (jobid < pOther.jobid);
    }
};

map<filehandleLocator, filehandle*> fhregistry;

int addFilehandle(filehandle* fh, uint64_t jobid, uint64_t handle, uint32_t contrib, uint32_t index)
{
    filehandleLocator fl;
    fl.jobid = jobid;
    fl.handle = handle;
    fl.contrib= contrib;
    fl.fileindex = index;

    FileHandleRegistryLock();

    LOG(bb,info) << "addFilehandle:  jobid=" << jobid << "  handle=" << handle << "  index=" << index;
    {
        fhregistry[fl] = fh;
    }

    FileHandleRegistryUnlock();

    return 0;
}

int findFilehandle(filehandle* &fh, uint64_t jobid, uint64_t handle, uint32_t contrib, uint32_t index)
{
    int rc = 0;

    filehandleLocator fl;
    fl.jobid = jobid;
    fl.handle = handle;
    fl.contrib = contrib;
    fl.fileindex = index;

    FileHandleRegistryLock();

    {
        if(fhregistry.find(fl) != fhregistry.end())
        {
            fh = fhregistry[fl];
        }
        else
        {
            rc = -1;
        }
    }

    FileHandleRegistryUnlock();

    return rc;
}

void removeNextFilehandleByJobId(filehandle* &fh, uint64_t jobid)
{
    filehandleLocator fl;
    fl.jobid = jobid;

    FileHandleRegistryLock();

    {
        fh = 0;
        for (auto& fhentry : fhregistry) {
            if (fhentry.first.jobid == jobid)
            {
                fh = fhentry.second;
                fl.handle = fhentry.first.handle;
                fl.contrib = fhentry.first.contrib;
                fl.fileindex = fhentry.first.fileindex;
                fhregistry.erase(fl);
                break;
            }
        }
    }

    FileHandleRegistryUnlock();

    return;
}

int removeFilehandle(filehandle* &fh, uint64_t jobid, uint64_t handle, uint32_t contrib, uint32_t index)
{
    int rc = 0;

    filehandleLocator fl;
    fl.jobid = jobid;
    fl.handle = handle;
    fl.contrib = contrib;
    fl.fileindex = index;

    FileHandleRegistryLock();

    {
        if(fhregistry.find(fl) != fhregistry.end())
        {
            fh = fhregistry[fl];
            fhregistry.erase(fl);
        }
        else
        {
            rc = -1;
        }
    }

    FileHandleRegistryUnlock();

    return rc;
}


filehandle::filehandle(const string& fn, int oflag, mode_t mode) :
    fd(-1),
    filename(fn),
    openErrno(0)
{
    memset((void*)&(statinfo), 0, sizeof(statinfo));
    privateData = NULL;
    extlookup = NULL;
    fd_oflags = oflag;
    numWritesNoSync = 0;
    totalSizeWritesNoSync = 0;
    isdevzero = false;

    LOG(bb,debug) << "Opening file " << filename << " with flag=" << oflag << " and mode=" << std::oct << mode << std::dec;
    FL_Write(FLProxy, OpenFile, "Open for filehandle",(uint64_t)oflag,(uint64_t)mode,0,0);
#if (BBSERVER || BBPROXY)
    threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::opensyscall, filename.c_str(),__LINE__);
#endif
    fd = open(filename.c_str(), oflag | O_CLOEXEC, mode);
#if (BBSERVER || BBPROXY)
    threadLocalTrackSyscallPtr->clearTrack();
#endif
    if (fd >= 0)
    {
        LOG(bb,info) << "Opened file " << filename << " as fd=" << fd << " with flag=" << oflag << ", mode=" << std::oct << mode << std::dec;
        FL_Write(FLProxy, OpenFile_OK, "Open for filehandle successful fd=%ld oflag=%ld mode=%ld",(uint64_t)fd,oflag,mode,0);
        if(filename == "/dev/zero")
        {
            isdevzero = true;
        }
    }
    else
    {
        openErrno=errno;
        LOG(bb,info) << "File open failed for file " << filename << " using flag=" << oflag << ", mode=" << std::oct << mode << std::dec << ", yielding errno=" << errno << " (" << strerror(errno) << ")";
        FL_Write(FLProxy, OpenFile_FAIL, "Open for filehandle failed errno=%ld oflag=%ld mode=%ld",(uint64_t)errno,oflag,mode,0);
    }
}

filehandle::~filehandle()
{
    if(privateData)
        delete privateData;

    if (extlookup)
        delete extlookup;

    close();
}

int filehandle::close()
{
    if (fd >= 0)
    {
        LOG(bb,info) << "Closing file " << filename << ", fd=" << fd;
        FL_Write(FLProxy, CloseFile, "Close for filehandle %ld",(uint64_t)fd,0,0,0);
        ::close(fd);
        fd = -1;
    }

    return 0;
}

void filehandle::dump(const char* pSev, const char* pPrefix) {
    if (!strcmp(pSev,"debug")) {
        LOG(bb,error) << "Start: " << (pPrefix ? pPrefix : "filehandle");
        LOG(bb,error) << "                   fd: " << fd;
        LOG(bb,error) << "             filename: " << filename;
        LOG(bb,error) << "             statinfo: st_dev=" << statinfo.st_dev << ", st_mode=" << std::oct << statinfo.st_mode << std::dec << ", st_size=" << statinfo.st_size;
        LOG(bb,error) << "            extlookup: " << extlookup;
        LOG(bb,error) << "            fd_oflags: " << fd_oflags;
        LOG(bb,error) << "      numWritesNoSync: " << numWritesNoSync;
        LOG(bb,error) << "totalSizeWritesNoSync: " << totalSizeWritesNoSync;
        LOG(bb,error) << "            isdevzero: " << isdevzero;
        LOG(bb,error) << " End: " << (pPrefix ? pPrefix : "filehandle");
    } else if (!strcmp(pSev,"info")) {
        LOG(bb,info) << "Start: " << (pPrefix ? pPrefix : "filehandle");
        LOG(bb,info) << "                   fd: " << fd;
        LOG(bb,info) << "             filename: " << filename;
        LOG(bb,info) << "             statinfo: st_dev=" << statinfo.st_dev << ", st_mode=" << std::oct << statinfo.st_mode << std::dec << ", st_size=" << statinfo.st_size;
        LOG(bb,info) << "            extlookup: " << extlookup;
        LOG(bb,info) << "            fd_oflags: " << fd_oflags;
        LOG(bb,info) << "      numWritesNoSync: " << numWritesNoSync;
        LOG(bb,info) << "totalSizeWritesNoSync: " << totalSizeWritesNoSync;
        LOG(bb,info) << "            isdevzero: " << isdevzero;
        LOG(bb,info) << "  End: " << (pPrefix ? pPrefix : "filehandle");
    }

    return;
}

int filehandle::getstats(struct stat& statbuf)
{   int rc = 0;

    if(fd >= 0)
    {
        FL_Write(FLXfer, BBIOR_FSTAT, "Calling fstat(%ld)",fd,0,0,0);
#if (BBSERVER || BBPROXY)
        threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::fstatsyscall, fd,__LINE__);
#endif
        rc = fstat(fd, &statinfo);
#if (BBSERVER || BBPROXY)
        threadLocalTrackSyscallPtr->clearTrack();
#endif
        FL_Write6(FLXfer, BBIOR_FSTATCMP, "Called fstat(%ld) rc=%ld errno=%ld size=%ld", fd, rc, errno, statinfo.st_size,0,0);
        if (!rc)
        {
            if(isdevzero)
            {
                statinfo.st_size = config.get(process_whoami+".devzerosize", 0ULL);
                LOG(bb,info) << "Size of /dev/zero artifically set to " << statinfo.st_size << "  " << process_whoami+".devzerosize";
            }
            LOG(bb,debug) << "fstat(" << fd << "), for " << filename << ", st_dev=" << statinfo.st_dev << ", st_mode=" << std::oct << statinfo.st_mode << std::dec << ", st_size=" << statinfo.st_size << ", rc=" << rc << ", errno=" << errno;
            statbuf = statinfo;
        }
        else
        {
            rc = errno;
            LOG(bb,error) << filename << " fstat failed.  errno=" << errno;
        }
    }
    else
    {
        rc = -1;
        LOG(bb,error) << "filehandle::getstats(): Called without open file descriptor.  fd=" << fd;

    }

    return rc;
}

int filehandle::setsize(size_t newsize)
{
    int rc;
    LOG(bb,info) << "Truncating " << filename << ", fd=" << fd << ", newsize=" << newsize;
    rc = ftruncate(fd, newsize);
    if (rc)
    {
        LOG(bb,error) << "Truncate of target file " << filename << " failed.  fd=" << fd << " newsize=" << newsize << ", errno=" << errno;
        throw runtime_error(string("Truncate failed.  errno=") + to_string(errno));
    }
    LOG(bb,debug) << "Target file " << filename << " truncated";
#if (BBSERVER || BBPROXY)
    threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::fstatsyscall, fd,__LINE__);
#endif
    rc = fstat(fd, &statinfo);
#if (BBSERVER || BBPROXY)
    threadLocalTrackSyscallPtr->clearTrack();
#endif
    if(rc)
    {
        LOG(bb,error) << filename << " fstat failed.  errno=" << errno;
        throw runtime_error(string("fstat failed in filehandke::setsize.  errno=") + to_string(errno));
    }
    LOG(bb,info) << "Target file " << filename << ", fstat size=" << getsize() << ", fd=" << fd;

    return rc;
}

void filehandle::updateStats(struct stat* stats)
{
    if (stats)
    {
        memcpy(&statinfo, stats, sizeof(struct stat));
        LOG(bb,debug) << "filehandle::updateStats(): " << filename << ": st_dev=" << statinfo.st_dev << ", st_mode=" << std::oct << statinfo.st_mode << std::dec << ", st_size=" << statinfo.st_size;
    }

    return;
}


/***************************************************************/
/* Below this point is bbProxy compiled code only              */
/***************************************************************/

#if BBPROXY
class extentLookup_fiemap : public extentLookup
{
protected:
    uint64_t  numextents;
    fiemap_t* extents;

    virtual int fetch()
    {
        int       rc;
        int       fd;
        size_t    mapsize;

        if(fetched)
            return 0;

        fd = fh->getfd();

        mapsize = sizeof(fiemap_t);
        extents = (fiemap_t*)malloc(mapsize);
        memset(extents, 0, mapsize);
        extents->fm_length = FIEMAP_MAX_OFFSET;
        rc = ioctl(fd, FS_IOC_FIEMAP, extents);
        if(rc < 0)
        {
            // fail
            free(extents);
            throw runtime_error(string("ioctl(FS_IOC_FIEMAP failed. errno=") + to_string(errno));
            return rc;
        }
        numextents = extents->fm_mapped_extents;
        mapsize += numextents * sizeof(fiemap_extent_t);
        extents = (fiemap_t*)realloc(extents, mapsize);
        memset(extents, 0, mapsize);
        extents->fm_length = FIEMAP_MAX_OFFSET;
        extents->fm_extent_count = numextents;
        rc = ioctl(fd, FS_IOC_FIEMAP, extents);
        if(rc < 0)
        {
            // fail
            free(extents);
            throw runtime_error(string("ioctl(FS_IOC_FIEMAP failed. errno=") + to_string(errno));
            return rc;
        }
        fetched = true;

        return 0;
    }

public:
    extentLookup_fiemap(const filehandle* fileh) : extentLookup(fileh) { };
    virtual ~extentLookup_fiemap() { free(extents); };

    virtual int size(unsigned& numentries)
    {
       fetch();
       numentries = numextents;

       return 0;
    }

    virtual int get(unsigned index, uint64_t& lba, uint64_t& start, uint64_t& len)
    {
        int rc;
        rc = fetch();
        if(rc)
        {
            lba   = 0;
            start = 0;
            len   = 0;
            return rc;
        }
        if(index >= numextents)
        {
            lba   = 0;
            start = 0;
            len   = 0;
            return -1;
        }
        uint64_t flags = extents->fm_extents[index].fe_flags;
        if(flags & FIEMAP_EXTENT_UNWRITTEN)
        {
            lba   = 0;
            start = 0;
            len   = 0;
            FL_Write(FLExtents, ExtentUnwritten, "Extent %ld has FIEMAP_EXTENT_UNWRITTEN set.  flags=%lx", index, flags,0,0);
            return -2;
        }

        lba   = extents->fm_extents[index].fe_physical;
        start = extents->fm_extents[index].fe_logical;
        len   = extents->fm_extents[index].fe_length;

        return 0;
    }

    virtual int release(BBFILESTATUS completion_status)
    {
        return 0;
    }
};

class extentLookup_exlayout : public extentLookup
{
protected:
    int       exlo;
    bool      writing;
    struct export_layout_transfer_setup* setup;

    virtual int fetch()
    {
        int rc;
        uint64_t  numextents = 0;
        if(fetched)
            return 0;

        if(exlo < 0) // handle partial failure (fetched==false, but we partially executed this flow)
        {
            /* Momentarily become fs_root so that the export_layout device can be opened */
            uid_t olduid = setfsuid(~0);
            gid_t oldgid = setfsgid(~0);
            becomeUser(0,0);

            errno=0;
            threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::openexlayout, fh->getfd(), __LINE__);
            exlo = open("/dev/export_layout", O_RDWR | O_CLOEXEC, 0);
            threadLocalTrackSyscallPtr->clearTrack();
            LOG(bb,debug) << "Open /dev/export_layout fd=" << fh->getfd() << ", exlo=" << exlo << ", errno=" << errno;
            FL_Write(FLExtents, ExportOpen, "Open /dev/export_layout  fd=%ld  exlo=%ld errno=%ld", fh->getfd(), exlo, errno, 0);

            if (exlo < 0)
            {
                throw runtime_error(string("open /dev/export_layout failed. errno=") + to_string(errno));
            }

            /* back to user */
            becomeUser(olduid, oldgid);
        }

        if(setup) // handle partial failure (fetched==false, but we partially executed this flow)
        {
            free(setup);
            setup = NULL;
        }

        while (setup == NULL)
        {
            size_t setupsize = sizeof(struct export_layout_transfer_setup) + (numextents * sizeof(struct export_layout_extent));
            setup = (struct export_layout_transfer_setup *)malloc(setupsize);
            if (setup == NULL)
                throw runtime_error(string("malloc failed.  errno=") + to_string(errno));

            memset(setup, 0, setupsize);  // zero storage to suppress valgrind warning
            setup->fd = fh->getfd();
            setup->offset = 0;
            setup->length = fh->getsize();
            setup->writing = writing;
            setup->extent_count_max = numextents;
            if (numextents)
            {
                LOG(bb,debug) << "Querying export_layout fd=" << setup->fd << ", offset=" << setup->offset << ", length=" << setup->length << ", writing=" << setup->writing << ", extent_count_max=" << setup->extent_count_max;
            }
            FL_Write6(FLExtents, ExportQuery, "Querying export_layout.  fd=%ld  offset=%ld  length=%ld  writing=%ld  extent_max=%ld", setup->fd, setup->offset, setup->length, setup->writing, setup->extent_count_max, 0);

            threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::setupexlayout, setup->fd, __LINE__, setup->length, setup->offset);
            rc = ioctl(exlo, EXPORT_LAYOUT_IOC_TRANSFER_SETUP, setup);
            threadLocalTrackSyscallPtr->clearTrack();
            LOG(bb,debug) << "Query of export_layout completed.  rc=" << rc;
            FL_Write(FLExtents, ExportQueryCmpl, "Query of export_layout completed.  rc=%ld", rc,0,0,0);

            if (rc < 0)
            {
                free(setup);
                setup = NULL;
                throw runtime_error(string("ioctl SETUP failed.  errno=") + to_string(errno));
            }

            if (rc > 0)
            {
                free(setup);
                setup = NULL;
                numextents = rc;
            }
        }
        fetched = true;

        return 0;
    }

public:
    extentLookup_exlayout(const filehandle* fileh, bool _writing) : extentLookup(fileh), writing(_writing) { setup = NULL; exlo = -1; };

    virtual ~extentLookup_exlayout()
    {
        if(setup)
        {
            free(setup);
            setup = NULL;
        }
        if(exlo > 0)
        {
            LOG(bb,debug) << "Closing /dev/export_layout device fd=" << fh->getfd() << ", exlo=" << exlo;
            FL_Write(FLExtents, ExportClose, "Closing /dev/export_layout device.  fd=%ld  exlo=%ld", fh->getfd(), exlo, 0, 0);
            LOG(bb,debug) << "Closing complete for /dev/export_layout device fd=" << fh->getfd() << ", exlo=" << exlo;
            close(exlo);
        }
    };

    virtual int size(unsigned& numentries)
    {
        fetch();
        numentries = setup->extent_count;

        return 0;
    }

    virtual int get(unsigned index, uint64_t& lba, uint64_t& start, uint64_t& len)
    {
        fetch();

        if(index >= setup->extent_count)
        {
            lba   = 0;
            start = 0;
            len   = 0;
            return -1;
        }

        lba   = setup->extent[index].blkdev_offset;
        start = setup->extent[index].file_offset;
        len   = setup->extent[index].length;

        return 0;
    }

    virtual int release(BBFILESTATUS completion_status)
    {
        int rc;
        struct export_layout_transfer_finalize finalize;
        LOG(bb,debug) << "Releasing the file fd=" << fh->getfd() << ", exlo=" << exlo << ", status=" << (int)completion_status;
        FL_Write(FLExtents, ExportRelease, "Releasing the file fd=%ld, exlo=%ld  status=%ld", fh->getfd(), exlo, 0, (int)completion_status);
        if(exlo > 0)
        {
            finalize.status = (completion_status == BBFILE_SUCCESS ? 0 : -1);
            threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::finalizeexlayout, fh->getfd() , __LINE__);
            rc = ioctl(exlo, EXPORT_LAYOUT_IOC_TRANSFER_FINALIZE, &finalize);
            threadLocalTrackSyscallPtr->clearTrack();
            if (rc < 0 && completion_status == BBFILE_SUCCESS)
            {
                // NOTE: Only throw the runtime error if we think everything is OK up to this FINALIZE.
                //       Otherwise, we will instead report any prior failure...
                throw runtime_error(string("ioctl FINALIZE failed.  errno=") + to_string(errno));
            }
        }
        LOG(bb,debug) << "Releasing the file complete for fd=" << fh->getfd() << ", exlo=" << exlo << ", status=" << (int)completion_status;

        return 0;
    }
};

int   filehandle::release(BBFILESTATUS completion_status)
{
    int rc = 0;
    if(extlookup)
    {
        rc = extlookup->release(completion_status);
    }

    return rc;
}


int filehandle::protect(off_t start, size_t len, bool writing, Extent& input, vector<Extent>& result)
{
    int rc;
    uint64_t bytesRead    = 0;
    uint64_t bytesWritten = 0;

    if(!config.get(process_whoami+".use_export_layout", false))
    {
        extlookup = new extentLookup_fiemap(this);
        if(writing)
        {
            LOG(bb,debug) << "Performing fallocate(fd=" << fd << ", start=" << start << ", size=" << len << ")";
            FL_Write(FLExtents, DoFAllocate, "Performing fallocate(fd=%ld, start=%ld  size=%ld)", fd, start, len, 0);
            if (len)
            {
                rc = fallocate(fd, 0, start, len);
                if (rc)
                {
                    throw runtime_error(string("fallocate failed.  errno=") + to_string(errno));
                }
            }

            FL_Write(FLExtents, HaveNoFSPatch, "Performing FS allocation workaround", 0,0,0,0);
            char buffer[1024];
            size_t remainder = len;
            memset(buffer,0,sizeof(buffer));
            lseek(fd, start, SEEK_SET);
            while(remainder > 0)
            {
                rc = write(fd, buffer, MIN(sizeof(buffer), remainder));
                if(rc <= 0)
                {
                    throw runtime_error(string("write failed.  errno=") + to_string(errno));
                }

                if(rc > 0)
                {
                    remainder -= rc;
                }
            }
        }

        //
        // Discussion on fdatasync and posix_advise found here:
        //             http://insights.oetiker.ch/linux/fadvise.html
        //  Webpage indicates posix_fadvise(fd,0,0,POSIX_ADV_DONTNEED) is sufficient.  But
        //  it seems like specifying the correct offset and filesize is important.
        //
        LOG(bb,debug) << "Perform fd=" << fd << ", datasync() and fadvise(), start=" << start << ", len=" << len;
        FL_Write(FLExtents, DataSyncTgt, "Perform fd=%ld datasync() and fadvise()  start=%ld  len=%ld",fd,start,len,0);
        fdatasync(fd);
        posix_fadvise(fd, start, len, POSIX_FADV_DONTNEED);
    }
    else
    {
        extlookup = new extentLookup_exlayout(this, writing);
    }

    unsigned numextents = 0;
    extlookup->size(numextents);
    string vg = config.get(process_whoami+".volumegroup", "bb");
    LVLookup lookup;
    try
    {
        rc = lookup.build(*this, vg);
    }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    if (rc < 0)
    {
        return -1;
    }

    uint64_t l_LBA_Start, l_Start, l_Length;
    if (numextents)
    {
        LOG(bb,debug) << "Start finding FS extents, fd=" << fd << ", numextents=" << numextents;
        for (unsigned x=0; x<numextents; x++)
        {
            rc = extlookup->get(x, l_LBA_Start, l_Start, l_Length);
            LOG(bb,debug) << "Found FS Extent fd=" << fd << ", extent #" << x << ", starting at 0x" << l_Start << " for 0x" << l_Length << " bytes.  Start LBA=0x" << l_LBA_Start << ", rc=" << rc;
            FL_Write6(FLExtents, FoundFSExtent, "Found FS Extent.  File descriptor %ld extent #%ld:  0x%lx for 0x%lx bytes.  Start LBA=0x%lx   rc=%ld", fd, x, l_Start, l_Length, l_LBA_Start, rc);
            if (rc == -1)
            {
                return -1;
            }
            else if(rc == -2)
            {
                continue;
            }

            Extent tmp = input;
            tmp.lba.start = l_LBA_Start;
            tmp.start = l_Start;
            tmp.len = l_Length;

            if ((tmp.flags & BBI_TargetPFS) != 0)
            {
                bytesRead += tmp.len;
            }

            if ((tmp.flags & BBI_TargetSSD) != 0)
            {
                bytesWritten += tmp.len;
            }

            // Lookup location(s) in LVM
            lookup.translate(tmp, result);
            LOG(bb,debug) << "lookup.translate returned for extent #" << x;
        }
    }
    else
    {
        // No extents for this file.  Build a 'dummy' extent to send to bbServer for this file.
        // bbServer needs this extent so that it can send the appropriate completion messages back
        // to bbProxy and update status for the transfer definition, LVKey, and handle.
        // Note that this dummy extent will be marked to actually transfer no data...
        Extent tmp = input;
        tmp.len = 0;
        tmp.flags |= BBI_First_Extent;
        tmp.flags |= BBI_Last_Extent;
        result.push_back(tmp);
    }

    proxy_BumpBBUsage(statinfo.st_dev, bytesWritten, bytesRead);
    LOG(bb,debug) << "proxy_BumpBBUsage, statinfo.st_dev=" << statinfo.st_dev << ", bytesWritten=" << bytesWritten << ", bytesRead=" << bytesRead;

    return 0;
}

#endif
