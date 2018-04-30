/*******************************************************************************
 |    bbio.h
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


#ifndef BB_BBIO_H_
#define BB_BBIO_H_

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <string>

using namespace std;

#include "BBTransferDef.h"
#include "fh.h"
#include "LVKey.h"


/*******************************************************************************
 | External methods
 *******************************************************************************/
extern string resolveServerConfigKey(const string& pKey);


/*******************************************************************************
 | Forward declarations
 *******************************************************************************/
class Extent;


/*******************************************************************************
 | Constants
 *******************************************************************************/
const uint32_t DEFAULT_WRITES_BETWEEN_SYNCS = 0;
//const uint32_t DEFAULT_WRITES_BETWEEN_SYNCS = 16;
//const uint32_t DEFAULT_WRITES_BETWEEN_SYNCS = 1024;
const size_t DEFAULT_SIZE_BETWEEN_SYNCS = 0;
//const size_t DEFAULT_SIZE_BETWEEN_SYNCS = 2 * 1024 * 1024;
//const size_t DEFAULT_SIZE_BETWEEN_SYNCS = 256 * 1024 * 1024;


/*******************************************************************************
 | External functions
 *******************************************************************************/
extern void lockTransferQueue(const LVKey* pLVKey, const char* pMethod);
extern void unlockTransferQueue(const LVKey* pLVKey, const char* pMethod);
extern int  returnTransferBuffer(void* buffer);


//
// BBIO class
//

class BBIO
{
  public:
    /**
     * \brief Destructor
     */
    virtual ~BBIO() {};

    // Virtual methods

    // Returns the filehandle for the input file index
    virtual filehandle* getFileHandle(const uint32_t pFileIndex);

    // Returns the number of writes performed for the file without an fsync
    virtual uint32_t getNumWritesNoSync(const uint32_t pFileIndex);

    // Returns the total size of data that has been writen to the file without an fsync
    virtual size_t getTotalSizeWritesNoSync(const uint32_t pFileIndex);

    // Increments the number of writes performed for the file without an fsync
    virtual void incrNumWritesNoSync(const uint32_t pFileIndex, const uint32_t pValue=1);

    // Increments the total size of data that has been writen to the file without an fsync
    virtual void incrTotalSizeWritesNoSync(const uint32_t pFileIndex, const size_t pValue);

    // Sets the number of writes performed for the file without an fsync
    virtual void setNumWritesNoSync(const uint32_t pFileIndex, const uint32_t pValue);

    // Sets the total size of data that has been writen to the file without an fsync
    virtual void setTotalSizeWritesNoSync(const uint32_t pFileIndex, const size_t pValue);

    // Pure virtual method implemented by derived class to open a file in the parallel file system.
    virtual int open(uint32_t pFileIndex, uint64_t pBBFileFlags, const string& pFileName, const mode_t pMode) = 0;

    // Pure virtual method implemented by derived class to close a file in the parallel file system.
    virtual int close(uint32_t pFileIndex) = 0;

    // Pure virtual method implemented by derived class to fstat a file in the parallel file system.
    virtual int fstat(uint32_t pFileIndex, struct stat* pStats) = 0;

    // Pure virtual method implemented by derived class to synchronize a file that was written to the parallel file system.
    virtual int fsync(uint32_t pFileIndex) = 0;

    // Non-virtual methods

    // Inline methods
    inline uint32_t getWritesBetweenSyncs() { return writesBetweenSyncs; };
    inline size_t getSizeBetweenSyncs() { return sizeBetweenSyncs; };

    // Non-inline methods

    // Returns the fd for the associated SSD
    int getReadFdByExtent(Extent* pExtent);

    virtual unsigned int getBacklog(uint32_t pFileIndex) { return 0; };

    // Invoked by doTransfer() by a worker thread to perform the I/O for a given extent taken from the work queue.
    int performIO(LVKey& pKey, Extent* pExtent);

  protected:
    BBIO(int32_t pContribId, BBTransferDef* pTransferDef) :
        contribId(pContribId), transferDef(pTransferDef) {
        writesBetweenSyncs = config.get(resolveServerConfigKey("writesBetweenSyncs"), DEFAULT_WRITES_BETWEEN_SYNCS);
        sizeBetweenSyncs = config.get(resolveServerConfigKey("sizeBetweenSyncs"), DEFAULT_SIZE_BETWEEN_SYNCS);
    };

    // Pure virtual method implemented by read from the parallel file system.  Return value is number of bytes read.
    virtual ssize_t pread(uint32_t pFileIndex, char* pBuffer, size_t pMaxBytesToRead, off_t& pOffset) = 0;

    // Pure virtual method implemented by derived class to write to the parallel file system.  Return value is number of bytes written.
    virtual ssize_t pwrite(uint32_t pFileIndex, const char* pBuffer, size_t pMaxBytesToWrite, off_t& pOffset) = 0;

    int32_t             contribId;          ///<  Contributor for this transfer
    uint32_t            writesBetweenSyncs; ///<  If supported, number of writes between periodic fsyncs for PFS
    size_t              sizeBetweenSyncs;   ///<  If supported, size of writes between periodic fsyncs for PFS
    BBTransferDef*      transferDef;        ///<  Associated transfer definition
};

#endif /* BB_BBIO_H_ */
