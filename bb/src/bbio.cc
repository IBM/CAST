/*******************************************************************************
 |    bbio.cc
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

#include <map>
#include <queue>

#include "bbio.h"
#include "bbinternal.h"
#include "bbserver_flightlog.h"
#include "Extent.h"
#include "tracksyscall.h"

extern size_t             transferBufferSize;
thread_local char*        threadTransferBuffer;

static pthread_mutex_t    freeTransferBuffers_mutex = PTHREAD_MUTEX_INITIALIZER;
static queue<void*>       freeTransferBuffers;
static sem_t              numFreeBuffers;

int setupTransferBuffer(int fileindex)
{
    if(threadTransferBuffer == NULL)
    {
        unsigned int backlog;
        sem_wait(&numFreeBuffers);
        pthread_mutex_lock(&freeTransferBuffers_mutex);
        threadTransferBuffer = (char*)freeTransferBuffers.front();
        freeTransferBuffers.pop();
        backlog = freeTransferBuffers.size();
        pthread_mutex_unlock(&freeTransferBuffers_mutex);

        FL_Write(FLXfer, FLXBufPop, "Using buffer %p  available buffers=%ld",(uint64_t)threadTransferBuffer,backlog,0,0);
    }
    return 0;
}
int returnTransferBuffer(void* buffer)
{
    unsigned int backlog;
    pthread_mutex_lock(&freeTransferBuffers_mutex);
    freeTransferBuffers.push(buffer);
    backlog = freeTransferBuffers.size();
    pthread_mutex_unlock(&freeTransferBuffers_mutex);
    FL_Write(FLXfer, FLXBufPush, "Using buffer %p  available buffers=%ld",(uint64_t)buffer,backlog,0,0);
    sem_post(&numFreeBuffers);
    return 0;
}



string getDeviceBySerial(string serial);

static map<string, int>   Serial2ssdReadFd;
static map<string, int>   Serial2ssdWriteFd;
static pthread_mutex_t    ssdReadFdMutex  = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t    ssdWriteFdMutex = PTHREAD_MUTEX_INITIALIZER;

const unsigned int PAGESIZE=getpagesize();
const unsigned int PAGESIZEOFFSETMASK=PAGESIZE-1;
const unsigned int FULLPAGESMASK=  ~PAGESIZEOFFSETMASK;

static unsigned int ssdwritedirect = 0;

unsigned int pageRounder(int length){
   unsigned int rounded=length;
   if (PAGESIZEOFFSETMASK & rounded) {
      rounded = (rounded & FULLPAGESMASK) + PAGESIZE;
   }
   return rounded;
}

unsigned int ssdWriteAdjust(int pLength){
   if (!ssdwritedirect) return pLength;
   return pageRounder(pLength);
}

void setSsdWriteDirect(unsigned int pValue){ ssdwritedirect = 0;  if (pValue) ssdwritedirect = 1;}
/*
 * Helper functions
 */
int getWriteFdByExtent(Extent* pExtent)
{
    errno=0;
    int fd = -1;
    pthread_mutex_lock(&ssdWriteFdMutex);
    auto it = Serial2ssdWriteFd.find(pExtent->serial);
    if (it !=  Serial2ssdWriteFd.end() ){
       fd = it->second; //second element is the fd
       pthread_mutex_unlock(&ssdWriteFdMutex);
       return fd;
    }
    LOG(bb,debug) << "Need to open write device by Serial="<<pExtent->serial<<".  In getWriteFdByExtent ExtentInfo: "<<*pExtent;
    pthread_mutex_unlock(&ssdWriteFdMutex); //give up the lock in case some other thread needs it
    string l_disk = getDeviceBySerial(pExtent->serial);
    if (!ssdwritedirect){
      threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::SSDopenwriteNOTdirect, l_disk.c_str(),__LINE__);
      fd = ::open(l_disk.c_str(), O_WRONLY);
      threadLocalTrackSyscallPtr->clearTrack();
      LOG(bb,info) << "OPEN O_WRONLY " << l_disk.c_str()<<" fd=" << fd << (fd==-1 ? (  string(strerror(errno)) +":"+to_string(errno) ) : " ");
    }
    else{
      threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::SSDopenwritedirect, l_disk.c_str(),__LINE__);
      fd = ::open(l_disk.c_str(), O_WRONLY | O_DIRECT );
      threadLocalTrackSyscallPtr->clearTrack();
      LOG(bb,info) << "OPEN O_WRONLY | O_DIRECT " << l_disk.c_str()<<" fd=" << fd << (fd==-1 ? (  string(strerror(errno)) +":"+to_string(errno) ) : " ");
    }

    if (fd < 0) return fd;
    pthread_mutex_lock(&ssdWriteFdMutex);
    Serial2ssdWriteFd[pExtent->serial]=fd;
    pthread_mutex_unlock(&ssdWriteFdMutex);
    return fd;
}


/*
 * Member methods
 */
void BBIO::closeAllFileHandles()
{
    return;
}

filehandle* BBIO::getFileHandle(const uint32_t pFileIndex)
{
    return (filehandle*)0;
}

uint32_t BBIO::getNumWritesNoSync(const uint32_t pFileIndex)
{
    return 0;
}

size_t BBIO::getTotalSizeWritesNoSync(const uint32_t pFileIndex)
{
    return 0;
}

int BBIO::getReadFdByExtent(Extent* pExtent)
{
    errno=0;
    int fd = -1;
    pthread_mutex_lock(&ssdReadFdMutex);
    auto it = Serial2ssdReadFd.find(pExtent->serial);
    if (it !=  Serial2ssdReadFd.end() ) {
       fd = it->second; //second element is the fd
       pthread_mutex_unlock(&ssdReadFdMutex);
       return fd;
    }
    LOG(bb,debug) << "Need to open read device by Serial=" <<pExtent->serial<<".  In BBIO::getReadFdByExtent ExtentInfo: "<<*pExtent;
    pthread_mutex_unlock(&ssdReadFdMutex);  //give up the lock in case some other thread needs it
    string l_disk = getDeviceBySerial(pExtent->serial);
    threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::SSDopenreaddirect, l_disk.c_str(),__LINE__);
    fd = ::open(l_disk.c_str(), O_RDONLY | O_DIRECT);
    threadLocalTrackSyscallPtr->clearTrack();
    LOG(bb,info) << "OPEN O_RDONLY | O_DIRECT " << l_disk.c_str()<<" fd=" << fd << (fd==-1 ? strerror(errno) : " ");
    if (fd < 0) return fd;
    pthread_mutex_lock(&ssdReadFdMutex);
    Serial2ssdReadFd[pExtent->serial]=fd;
    pthread_mutex_unlock(&ssdReadFdMutex);
    return fd;
}

void BBIO::incrNumWritesNoSync(const uint32_t pFileIndex, const uint32_t pValue)
{
    return;
}

void BBIO::incrTotalSizeWritesNoSync(const uint32_t pFileIndex, const size_t pValue)
{
    return;
}

int BBIO::performIO(LVKey& pKey, Extent* pExtent)
{
    int rc = 0;

    const ssize_t BLKSIZE = 65536-1;
    int ssd_fd = -1;
    off_t offset_src;
    off_t offset_dst;
    ssize_t bytesRead;
    ssize_t bytesWritten;
    size_t count = pExtent->len;

    FL_Write(FLXfer, BBIOPerformIO, "BBIO::performIO.  Length=%ld  Start=%lx  LBAStart=%lx  Flags=0x%lx", count, pExtent->start, pExtent->lba.start, pExtent->flags);

    // Perform the I/O
    if (pExtent->flags & BBI_TargetSSD)
    {
        if ( __glibc_likely(!pExtent->isCP_Transfer()) )
        {
            // Target is SSD...
            unlockTransferQueue(&pKey, "performIO - Before perform I/O to SSD for extent");

            try
            {
                LOG(bb,debug) << "Copying to SSD from sourceindex=" << pExtent->sourceindex;

                ssd_fd = getWriteFdByExtent(pExtent);
                if (ssd_fd < 0)
                {
                    FL_Write(FLXfer, SSDOpenFailed, "Opening the SSD for O_WRONLY failed.  rc=%ld, errno=%ld", ssd_fd, errno,0,0);
                    throw runtime_error(string("Unable to open the SSD for writing.  errno=") + to_string(errno));
                }

                offset_src = pExtent->start;
                offset_dst = pExtent->lba.start;

                LOG(bb,debug) << "Starting transfer to ssd fd=" << setw << ssd_fd \
                              << " from pfs " << transferDef->files[pExtent->sourceindex] \
                              << std::hex << std::uppercase << setfill('0') \
                              << " src offset=0x" << setw(8) << offset_src \
                              << " dst offset=0x" << setw(8) << offset_dst \
                              << " size=0x" << setw(8) << count \
                              << setfill(' ') << std::nouppercase << std::dec;

                while ((!rc) && count > 0)
                {
                    setupTransferBuffer(pExtent->sourceindex);

                    FL_Write(FLXfer, PREAD_PFS, "Reading from target index %ld into %p, len=%ld at offset 0x%lx", pExtent->sourceindex, (uint64_t)threadTransferBuffer, MIN(transferBufferSize, count), offset_src);
                    //pread is bbio::pread derived
                    bytesRead = pread(pExtent->sourceindex, threadTransferBuffer, MIN(transferBufferSize, count), offset_src);

                    FL_Write(FLXfer, PREAD_PFSCMP, "Reading from target index %ld.  bytesRead=%ld errno=%ld", pExtent->sourceindex, bytesRead,errno,0);

                    if ( __glibc_likely(bytesRead >= 0) )
                    {
                        if (offset_dst < 1024*1024)
                        {
                            // last ditch sanity check
                            throw runtime_error(string("Extent offset into SSD too low.  Offset=") + to_string(offset_dst));
                        }
                        FL_Write(FLXfer, PWRITE_SSD, "Writing to SSD.  File descriptor %ld, length %ld at offset 0x%lx", ssd_fd, bytesRead,offset_dst,0);
                        threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::SSDpwritesyscall, ssd_fd,__LINE__, ssdWriteAdjust(bytesRead),offset_dst);
                        bytesWritten = ::pwrite(ssd_fd, threadTransferBuffer, ssdWriteAdjust(bytesRead), offset_dst);
                        threadLocalTrackSyscallPtr->clearTrack();

                        if ( __glibc_likely(bytesWritten >= 0) )
                        {
                            FL_Write(FLXfer, PWRITE_SSDCMP, "Write file descriptor %ld complete.  rc=%ld offset_dst=0x%lx pExtent->sourceindex=%ld", ssd_fd, bytesWritten, offset_dst, pExtent->sourceindex);
                            if (bytesWritten > bytesRead)bytesWritten = bytesRead;  //fixup for page pad
                            offset_src += bytesWritten;
                            offset_dst += bytesWritten;
                            count      -= bytesWritten;
                        }
                        else
                        {
                            stringstream errorText;
                            FL_Write(FLXfer, PWRITE_SSDFAIL, "Write to remote SSD failed. fd=%ld  offset_dst=%ld  bytesWritten=%ld  errno=%ld", ssd_fd, offset_dst, bytesWritten, errno);
                            rc = -1;
                            errorText << "Write to remote SSD failed";
                            LOG_ERROR_TEXT_ERRNO_AND_RAS(errorText, errno, bb.sc.pwrite.ssd);
                            bberror << err("rc", rc);
                        }
                    }
                    else
                    {
                        FL_Write(FLXfer, PREAD_PFSFAIL, "Read from PFS file failed.  sourceindex=%ld  offset_src=%ld  bytesRead=%ld", pExtent->sourceindex, offset_src, bytesRead, 0);
                        rc = -1;
                        // BBIO subclass generated bberror and RAS
                    }
                }
                if (!rc)
                {
                    LOG(bb,debug) << "Transfer complete to ssd fd=" << setw(3) << ssd_fd << " from pfs " << transferDef->files[pExtent->sourceindex];
                }
            }
            catch (ExceptionBailout& e) { }
            catch (exception& e)
            {
                rc = -1;
                LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
            }

            lockTransferQueue(&pKey, "performIO - After perform I/O to SSD for extent");
        }
        else
        {
            LOG(bb,info) << "BBIO: No data needs to be transferred to the SSD from " << transferDef->files[pExtent->sourceindex];
        }
    }
    else if (pExtent->flags & BBI_TargetPFS)
    {
        // Target is PFS...
        size_t ssdReadCount;
        off_t ssdReadOffset;
        ssize_t l_Length;
        char* l_Buffer;

        if (!pExtent->isCP_Transfer())
        {
            // Target is PFS...
            unlockTransferQueue(&pKey, "performIO - Before perform I/O to PFS for extent");

            try
            {
                LOG(bb,debug) << "Copying to targetindex=" << pExtent->targetindex << " from SSD";

                ssd_fd = getReadFdByExtent(pExtent);
                if (ssd_fd < 0)
                    throw runtime_error(string("Unable to open the SSD for reading.  errno=") + to_string(errno));

                offset_src = pExtent->lba.start;
                offset_dst = pExtent->start;

                LOG(bb,debug) << "Starting transfer from ssd fd=" << setw(3) << ssd_fd \
                             << " to pfs " << transferDef->files[pExtent->targetindex] \
                             << std::hex << std::uppercase << setfill('0') \
                             << " src offset=0x" << setw(8) << offset_src \
                             << " dst offset=0x" << setw(8) << offset_dst \
                             << " size=0x" << setw(8) << count \
                             << setfill(' ') << std::nouppercase << std::dec;

                while ((!rc) && count > 0)
                {
                    setupTransferBuffer(pExtent->targetindex);

                    ssdReadCount  = MIN(transferBufferSize, (count+BLKSIZE)&(~BLKSIZE));
                    ssdReadOffset = offset_src & (~BLKSIZE);

                    FL_Write6(FLXfer, PREAD_SSD, "Reading from file descriptor %ld into %p, len=%ld at offset 0x%lx (pre-adjust was len=%ld offset=0x%lx)", ssd_fd, (uint64_t)threadTransferBuffer, ssdReadCount, ssdReadOffset, count, offset_src);
                    threadLocalTrackSyscallPtr->nowTrack(TrackSyscall::SSDpreadsyscall, ssd_fd,__LINE__, ssdReadCount,ssdReadOffset);
                    bytesRead = ::pread(ssd_fd, threadTransferBuffer, ssdReadCount, ssdReadOffset);
                    threadLocalTrackSyscallPtr->clearTrack();

                    if ( __glibc_likely(bytesRead >= 0) )
                    {
                        FL_Write6(FLXfer, PREAD_DATA, "Data bytes read from SSD: %lx %lx %lx %lx %lx %lx", ((uint64_t*)threadTransferBuffer)[0], ((uint64_t*)threadTransferBuffer)[1], ((uint64_t*)threadTransferBuffer)[2], ((uint64_t*)threadTransferBuffer)[3], ((uint64_t*)threadTransferBuffer)[4], ((uint64_t*)threadTransferBuffer)[5]);

                        l_Buffer = &threadTransferBuffer[offset_src&BLKSIZE];
                        l_Length = MIN((ssize_t)count, bytesRead - (offset_src&BLKSIZE));

                        FL_Write(FLXfer, PWRITE_PFS, "Writing to target index %ld into %p, len=%ld at offset 0x%lx", pExtent->targetindex, (uint64_t)l_Buffer, l_Length, offset_dst);
                        // bbio::pwrite derived
                        bytesWritten = pwrite(pExtent->targetindex, l_Buffer, l_Length, offset_dst);


                        if ( __glibc_likely(bytesWritten >= 0) )
                        {
                            FL_Write(FLXfer, PWRITE_PFSCMP, "Write to PFS complete.  Target index=%ld, offset=0x%lx, length=%ld, bytes_written=%ld", pExtent->targetindex, l_Length, offset_dst, bytesWritten);
                            offset_src += bytesWritten;
                            offset_dst += bytesWritten;
                            count      -= bytesWritten;
                        }
                        else
                        {
                            FL_Write(FLXfer, PWRITE_PFSFAIL, "Write to PFS file failed.  targetindex=%ld  offset_dst=%ld  bytesWritten=%ld", pExtent->targetindex, offset_dst, bytesWritten, 0);
                            rc = -1;
                            // BBIO subclass generated bberror and RAS
                        }

                    }
                    else // !(bytesRead>=0)
                    {
                        stringstream errorText;
                        FL_Write(FLXfer, PREAD_SSDFAIL, "Read from remote SSD failed.  fd=%ld  offset_src=%ld  bytesRead=%ld  errno=%ld", ssd_fd, offset_src, bytesRead, errno);
                        rc = -1;
                        errorText << "Read from remote SSD failed";
                        LOG_ERROR_TEXT_ERRNO_AND_RAS(errorText, errno, bb.sc.pread.ssd);
                        bberror << err("rc", rc);
                    }
                }
            }
            catch (ExceptionBailout& e) { }
            catch (exception& e)
            {
                rc = -1;
                LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
            }

            lockTransferQueue(&pKey, "performIO - After perform I/O to PFS for extent");

            if (!rc)
            {
                // Determine if we should do a periodic fsync...
                uint32_t l_FileIndex = pExtent->targetindex;
                incrNumWritesNoSync(l_FileIndex);
                incrTotalSizeWritesNoSync(l_FileIndex, pExtent->len);
                uint32_t l_NumberOfWrites = getNumWritesNoSync(l_FileIndex);
                size_t l_SizeOfWrites = getTotalSizeWritesNoSync(l_FileIndex);
                if ((getWritesBetweenSyncs() && l_NumberOfWrites >= getWritesBetweenSyncs()) || (getSizeBetweenSyncs() && l_SizeOfWrites >= getSizeBetweenSyncs()))
                {
                    // NOTE Set the counts to zero before the fsync so other threads
                    //      won't schedule another periodic fsync for this target file...
                    setNumWritesNoSync(l_FileIndex, 0);
                    setTotalSizeWritesNoSync(l_FileIndex, 0);
                    unlockTransferQueue(&pKey, "performIO - Before periodic fsync to PFS");
                    try
                    {
                        LOG(bb,info) << "Periodic PFS fsync start: targetindex=" << l_FileIndex << ", # writes " << l_NumberOfWrites << ", size of writes " << l_SizeOfWrites << ", triggered by extent " << *pExtent;
                        FL_Write(FLTInf2, PSYNC_PFS, "Performing periodic PFS fsync.  Target index=%ld", pExtent->targetindex,0,0,0);
                        fsync(l_FileIndex);
                        FL_Write(FLTInf2, PSYNC_PFSCMP, "Performed periodic PFS fsync.  Target index=%ld", pExtent->targetindex,0,0,0);
                        LOG(bb,info) << "Periodic PFS fsync end: targetindex=" << l_FileIndex;
                    }
                    catch (ExceptionBailout& e) { }
                    catch (exception& e)
                    {
                        rc = -1;
                        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
                    }
                    lockTransferQueue(&pKey, "performIO - After periodic fsync to PFS");
                }
                LOG(bb,debug) << "Transfer complete to pfs " << transferDef->files[l_FileIndex] << ", target index " << l_FileIndex \
                              << " from ssd fd " << setw(3) << ssd_fd << ", size " << pExtent->len \
                              << ", numWritesNoSync " << getNumWritesNoSync(l_FileIndex) << ", totalSizeWritesNoSync " << getTotalSizeWritesNoSync(l_FileIndex);
            }

        }
        else
        {
            LOG(bb,info) << "BBIO: No data needs to be transferred from the SSD to " << transferDef->files[pExtent->targetindex];
        }
    }
    else
    {
        LOG(bb,error) << "BBIO: Unknown extent flag specification 0x" << std::hex << std::uppercase << setfill('0') \
                      << pExtent->flags << setfill(' ') << std::nouppercase << std::dec;
    }

    return rc;
}

void BBIO::setNumWritesNoSync(const uint32_t pFileIndex, const uint32_t pValue)
{
    return;
}

void BBIO::setTotalSizeWritesNoSync(const uint32_t pFileIndex, const size_t pValue)
{
    return;
}
