/*******************************************************************************
 |    bbio_regular.cc
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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "bbinternal.h"
#include "bbio.h"
#include "bbio_regular.h"
#include "bbserver_flightlog.h"
#include "fh.h"
#include <boost/filesystem.hpp>
#include "tracksyscall.h"

namespace bfs = boost::filesystem;

extern thread_local char* threadTransferBuffer;

unsigned int BBIO_Regular::getBacklog(uint32_t fileindex)
{
    bbioRegularData* regdata = (bbioRegularData*)fhmap[fileindex]->getPrivate();
    pthread_mutex_lock(&regdata->lock);
    unsigned int backlog = regdata->pendingops.size();
    pthread_mutex_unlock(&regdata->lock);

    return backlog;
}

int bbioRegularData::addOp(bbioop& op)
{
    bool incPending = false;
    pthread_mutex_lock(&lock);
    if(pendingops.find(op.offset) == pendingops.end())
    {
        pendingops[op.offset] = op;
        incPending = true;
    }
    pthread_mutex_unlock(&lock);
    if(incPending)
    {
        sem_post(&numPendingOps);
    }
    else
    {
        // Duplicate offsets shouldn't occur for a given file.
        LOG_RC_AND_RAS(-1, bb.internal.dupkey);
    }

    FL_Write6(FLXfer, BBIORegOpAdd, "Queued operation fd=%ld  offset=%ld  isWrite=%ld  len=%ld op.completion=%ld incPending=%ld", op.fd, op.offset, op.isWrite, op.len,(uint64_t)op.completion,incPending);

    if(op.completion)
    {
        sem_wait(op.completion);
        FL_Write6(FLXfer, BBIORegOpCmpl, "Operation completed fd=%ld  offset=%ld  isWrite=%ld  len=%ld", op.fd, op.offset, op.isWrite, op.len,0,0);
    }

    return 0;
}

void* bbioRegFileManager(void* vptr)
{
    bbioRegularData* ptr = (bbioRegularData*)vptr;
    off_t lastoffset = 0;
    threadLocalTrackSyscallPtr = getSysCallTracker();
    while(1)
    {
        sem_wait(&ptr->numPendingOps);
        pthread_mutex_lock(&ptr->lock);
        unsigned int backlog = ptr->pendingops.size();
        auto opiter = ptr->pendingops.begin();
        auto op = opiter->second;
        ptr->pendingops.erase(opiter->first);
        pthread_mutex_unlock(&ptr->lock);

        ssize_t result;
        FL_Write6(FLXfer, BBIORegOp, "Performing I/O op on fd=%ld  offset=%ld  isWrite=%ld  len=%ld  backlog=%ld  stride=%ld", op.fd, op.offset, op.isWrite, op.len,backlog,op.offset - lastoffset);
        lastoffset = op.offset;
        if(op.isWrite)
        {
            threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::pwritesyscall, op.fd,__LINE__,op.len ,op.offset);
            result = ::pwrite(op.fd, op.buffer, op.len, op.offset);
            threadLocalTrackSyscallPtr->clearTrack();
        }
        else
        {
            threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::preadsyscall, op.fd,__LINE__,op.len ,op.offset);
            result = ::pread(op.fd, op.buffer, op.len, op.offset);
            threadLocalTrackSyscallPtr->clearTrack();
        }
        if(op.xferbuffer)    returnTransferBuffer((char*)op.xferbuffer);
        if(op.completion_rc) *op.completion_rc = result;
        if(op.completion)    sem_post(op.completion);
    }

    return NULL;
}

int BBIO_Regular::close(uint32_t pFileIndex)
{
    int rc = 0;

    filehandle* fh = getFileHandle(pFileIndex);
    if (fh)
    {
        FL_Write(FLXfer, BBIORegClose, "pFileIndex=%ld fh=%p fd=%ld",pFileIndex,(uint64_t)fh,fh->getfd(),0);
        LOG(bb,debug) << "Close for " << fh->getfn() << ", file index " << pFileIndex << ", fd " << fh->getfd();
        delete fhmap[pFileIndex];
        fhmap.erase(pFileIndex);
    }
    else
    {
        LOG(bb,error) << __PRETTY_FUNCTION__ << " NULL file handle for index="<<pFileIndex;
        rc = -1;
    }

    return rc;
}

void BBIO_Regular::closeAllFileHandles()
{
    filehandle* fh = 0;

    for (size_t l_FileIndex=0; l_FileIndex<transferDef->files.size(); l_FileIndex++)
    {
        // NOTE:  We first resolve to the file handle so that
        //        we don't put out an error message via the close()
        //        method which does a getFileHandle().
        fh = fhmap[(uint32_t)l_FileIndex];
        if (fh)
        {
            close((uint32_t)l_FileIndex);
            fh = 0;
        }
    }

    return;
}

int BBIO_Regular::fstat(uint32_t pFileIndex, struct stat* pStats)
{
    int rc = 0;

    filehandle* fh = getFileHandle(pFileIndex);
    if (fh)
    {
        LOG(bb,debug) << "BBIO_Regular(): fstat for " << fh->getfn() << ", file index " << pFileIndex;
        rc = fh->getstats(*pStats);
        FL_Write(FLXfer, BBIORegFstat, "pFileIndex=%ld fd=%ld rc=%ld errno=%ld",pFileIndex,fh->getfd(),rc,errno);
        if(rc == 0)
        {
            LOG(bb,debug) << "BBIO_Regular::fstat(): " << fh->getfn() << ": st_dev=" << pStats->st_dev << ", st_mode=0" << std::oct << pStats->st_mode << std::dec << ", st_size=" << pStats->st_size;
        }
    }
    else
    {
        LOG(bb,error) << __PRETTY_FUNCTION__ << " NULL file handle for index="<<pFileIndex;
        rc = -1;
    }

    return rc;
}

int BBIO_Regular::fsync(uint32_t pFileIndex)
{
    int rc = 0;

    filehandle* fh = getFileHandle(pFileIndex);
    if (fh)
    {
        threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::fsyncsyscall, fh->getfd(),__LINE__);
        rc=::fsync(fh->getfd());
        threadLocalTrackSyscallPtr->clearTrack();
        FL_Write(FLXfer, BBIORegFsync, "pFileIndex=%ld fd=%ld rc=%ld errno=%ld",pFileIndex,fh->getfd(),rc,errno);
    }
    else
    {
        LOG(bb,error) << __PRETTY_FUNCTION__ << " NULL file handle for index="<<pFileIndex;
        rc = -1;
    }

    return rc;
}

filehandle* BBIO_Regular::getFileHandle(const uint32_t pFileIndex)
{
    filehandle* fh = fhmap[pFileIndex];
    if (!fh)
    {
        LOG(bb,error) << "BBIO_Regular::getFileHandle(): File handle for file index " << pFileIndex << " does not exist";
    }

    return fh;
}

uint32_t BBIO_Regular::getNumWritesNoSync(const uint32_t pFileIndex)
{
    uint32_t l_NumWrites = 0;

    filehandle* fh = getFileHandle(pFileIndex);
    if (fh)
    {
        l_NumWrites = fh->getNumWritesNoSync();
    }
    else
    {
        LOG(bb,error) << __PRETTY_FUNCTION__ << " NULL file handle for index="<<pFileIndex;
    }

    return l_NumWrites;
}

size_t BBIO_Regular::getTotalSizeWritesNoSync(const uint32_t pFileIndex)
{
    size_t l_TotalSize = 0;

    filehandle* fh = getFileHandle(pFileIndex);
    if (fh)
    {
        l_TotalSize = fh->getTotalSizeWritesNoSync();
    }
    else
    {
        LOG(bb,error) << __PRETTY_FUNCTION__ << " NULL file handle for index="<<pFileIndex;
    }

    return l_TotalSize;
}

void BBIO_Regular::incrNumWritesNoSync(const uint32_t pFileIndex, const uint32_t pValue)
{
    filehandle* fh = getFileHandle(pFileIndex);
    if (fh)
    {
        fh->incrNumWritesNoSync(pValue);
    }
    else
    {
        LOG(bb,error) << __PRETTY_FUNCTION__ << " NULL file handle for index="<<pFileIndex;
    }

    return;
}

void BBIO_Regular::incrTotalSizeWritesNoSync(const uint32_t pFileIndex, const size_t pValue)
{
    filehandle* fh = getFileHandle(pFileIndex);
    if (fh)
    {
        fh->incrTotalSizeWritesNoSync(pValue);
    }
    else
    {
        LOG(bb,error) << __PRETTY_FUNCTION__ << " NULL file handle for index="<<pFileIndex;
    }

    return;
}

void BBIO_Regular::setNumWritesNoSync(const uint32_t pFileIndex, const uint32_t pValue)
{
    filehandle* fh = getFileHandle(pFileIndex);
    if (fh)
    {
        fh->setNumWritesNoSync(pValue);
    }
    else
    {
       LOG(bb,error) << __PRETTY_FUNCTION__ << " NULL file handle for index="<<pFileIndex;
    }

    return;
}

void BBIO_Regular::setTotalSizeWritesNoSync(const uint32_t pFileIndex, const size_t pValue)
{
    filehandle* fh = getFileHandle(pFileIndex);
    if (fh)
    {
        fh->setTotalSizeWritesNoSync(pValue);
    }
    else
    {
        LOG(bb,error) << __PRETTY_FUNCTION__ << " NULL file handle for index="<<pFileIndex;
    }

    return;
}

int BBIO_Regular::open(uint32_t pFileIndex, uint64_t pBBFileFlags, const string& pFileName, const mode_t pMode)
{
    int oflags = 0;
    int rc = 0;
    stringstream errorText;

    if (pBBFileFlags & BBRecursive)
    {
        bfs::create_directories(bfs::path(pFileName).parent_path());
    }

    if(pBBFileFlags & BBI_TargetSSD)
    {
        oflags = O_RDONLY;
    }

    if (pBBFileFlags & BBI_TargetPFS)
    {
        oflags = O_CREAT | O_TRUNC | O_WRONLY;
    }

    //LOG(bb,info) << "filename:  " << pFileName << ", oflags=" << oflags << ", mode=" << pMode;
    filehandle* l_fhPtr = new filehandle(pFileName, oflags, pMode);
    if (l_fhPtr->validfd())
    {
        fhmap[pFileIndex] = l_fhPtr;
        if (SINGLETHREADIO)
        {
           fhmap[pFileIndex]->setPrivate(new bbioRegularData);
        }
        FL_Write(FLXfer, BBIORegOpen, "pFileIndex=%ld fd=%ld oflags=%ld pMode=%ld",pFileIndex,l_fhPtr->getfd(),oflags,pMode);
    }
    else
    {
       rc = l_fhPtr->getOpenErrno();
       errorText << "Open failed for " << pFileName;
       bberror << err("error.filename", pFileName);
       LOG_ERROR_TEXT_ERRNO(errorText, rc);
       FL_Write(FLXfer, BBIORegOpenErrno, "pFileIndex=%ld errno=%ld oflags=%ld pMode=%ld", pFileIndex, errno, oflags, pMode);
       delete l_fhPtr;
    }

    return rc;
}


ssize_t BBIO_Regular::pread(uint32_t pFileIndex, char* pBuffer, size_t pMaxBytesToRead, off_t& pOffset)
{
    ssize_t bytesRead = -1;

    filehandle* fh = getFileHandle(pFileIndex);
    if (fh)
    {
        int fd = fh->getfd();
        if(SINGLETHREADIO)
        {
            bbioop  op;
            sem_t   done;
            ssize_t result;

            sem_init(&done,0,0);
            op.fd            = fd;
            op.isWrite       = false;
            op.offset        = pOffset;
            op.len           = pMaxBytesToRead;
            op.buffer        = pBuffer;
            op.xferbuffer    = NULL;
            op.completion    = &done;
            op.completion_rc = &result;

            bbioRegularData* regdata = (bbioRegularData*)fh->getPrivate();
            regdata->addOp(op);

            sem_destroy(&done);
            bytesRead = *op.completion_rc;
        }
        else
        {
            threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::preadsyscall, fd,__LINE__, pMaxBytesToRead,pOffset);
            bytesRead = ::pread(fd, pBuffer, pMaxBytesToRead, pOffset);
            threadLocalTrackSyscallPtr->clearTrack();
            if (bytesRead < 0)
            {
                stringstream errorText;
                errorText << "BBIO_Regular::pread: Read from PFS file failed, file index " << pFileIndex << ", max bytes to read " << pMaxBytesToRead << ", offset " << pOffset;
                bberror << err("error.fileindex", pFileIndex);
                LOG_ERROR_TEXT_ERRNO(errorText, errno);
                LOG_RC_AND_RAS(bytesRead, bb.sc.pread.pfs);
            }
        }
    }
    else
    {
        stringstream errorText;
        errorText << "BBIO_Regular::pread: Read from PFS file failed, file index " << pFileIndex << ", max bytes to read " << pMaxBytesToRead << ", offset " << pOffset << ", no file handle";
        bberror << err("error.fileindex", pFileIndex);
        LOG_ERROR_TEXT_ERRNO(errorText, errno);
        LOG_RC_AND_RAS(bytesRead, bb.sc.pread.bbio);
    }

    return bytesRead;
}

ssize_t BBIO_Regular::pwrite(uint32_t pFileIndex, const char* pBuffer, size_t pMaxBytesToWrite, off_t& pOffset)
{
    ssize_t bytesWritten = -1;

    filehandle* fh = getFileHandle(pFileIndex);
    if (fh)
    {
        int fd = fh->getfd();
        if(SINGLETHREADIO)
        {
            bbioop op;
            op.fd            = fd;
            op.isWrite       = true;
            op.offset        = pOffset;
            op.len           = pMaxBytesToWrite;
            op.buffer        = (char*)pBuffer;
            op.xferbuffer    = threadTransferBuffer;
            op.completion    = NULL;
            op.completion_rc = NULL;

            threadTransferBuffer = NULL;

            bbioRegularData* regdata = (bbioRegularData*)fh->getPrivate();
            regdata->addOp(op);
            bytesWritten = op.len;
        }
        else
        {
            threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::pwritesyscall, fd,__LINE__,pMaxBytesToWrite ,pOffset);
            bytesWritten = ::pwrite(fd, pBuffer, pMaxBytesToWrite, pOffset);
            threadLocalTrackSyscallPtr->clearTrack();

            if (bytesWritten < 0)
            {
                stringstream errorText;
                errorText << "BBIO_Regular::pwrite: Write to PFS file failed, file index " << pFileIndex << ", max bytes to write " << pMaxBytesToWrite << ", offset " << pOffset;
                bberror << err("error.fileindex", pFileIndex);
                LOG_ERROR_TEXT_ERRNO(errorText, errno);
                LOG_RC_AND_RAS(bytesWritten, bb.sc.pwrite.pfs);
            }
        }
    }
    else
    {
        stringstream errorText;
        errorText << "BBIO_Regular::pwrite: Write to PFS file failed, file index " << pFileIndex << ", max bytes to write " << pMaxBytesToWrite << ", offset " << pOffset << ", no file handle";
        bberror << err("error.fileindex", pFileIndex);
        LOG_ERROR_TEXT_ERRNO(errorText, errno);
        LOG_RC_AND_RAS(bytesWritten, bb.sc.pwrite.bbio);
    }

    return bytesWritten;
}
