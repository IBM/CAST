/*******************************************************************************
 |    bbio_BSCFS.h
 |
 |    Copyright IBM Corporation 2016,2017. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/

#ifndef BB_BBIO_BSCFS_H_
#define BB_BBIO_BSCFS_H_

#include "bbio.h"
#include "bscfs_index.h"
#include "fh.h"

class BBIO_BSCFS : public BBIO
{
  public:
    //  A BBIO_BSCFS() object is instantiated for each distinct bundleId defined
    //  in the list of files in the associated transfer definition.
    BBIO_BSCFS(int32_t pContribId, BBTransferDef* pTransferDef) :
        BBIO(pContribId, pTransferDef),
	mapFileIndex(-1),
	sharedFileIndex(-1),
	mapFileHandle(NULL),
	sharedFileHandle(NULL),
	index(NULL),
	indexBytes(0),
	dataBytes(0),
	writing(0),
	indexComplete(false) {
    };

    /**
     * \brief Destructor
     */
    virtual ~BBIO_BSCFS() {
	if (index != NULL) free(index);
	delete mapFileHandle;
	delete sharedFileHandle;
    };

    // Virtual method to close a file in the parallel file system.
    virtual int close(uint32_t pFileIndex);

    // Virtual method to close all file handles (Clean-up for abnormal start transfer)
    virtual void closeAllFileHandles();

    // Member method to perform cleanup after any close processing.
    void closeCleanUp();

    // Member method to perform index file processing as part of the close of a file in the parallel file system.
    int closeFileIndexProcessing(uint32_t pFileIndex);

    // Member method to perform shared file processing as part of the close of a file in the parallel file system.
    int closeSharedFileProcessing(uint32_t pFileIndex);

    // Virtual method to open a file in the parallel file system.
    virtual int open(uint32_t pFileIndex, uint64_t pBBFileFlags, const string& pFileName, const mode_t pMode=0);

    // Virtual method to fstat a file in the parallel file system.
    virtual int fstat(uint32_t pFileIndex, struct stat* pStats);

    // Virtual method to read from the parallel file system.  Return value is number of bytes read.
    virtual ssize_t pread(uint32_t pFileIndex, char* pBuffer, size_t pMaxBytesToRead, off_t& pOffset);

    // Virtual method to synchronize a file that was written to the parallel file system.
    virtual int fsync(uint32_t pFileIndex);

    // Virtual method to write to the parallel file system.  Return value is number of bytes written.
    virtual ssize_t pwrite(uint32_t pFileIndex, const char* pBuffer, size_t pMaxBytesToWrite, off_t& pOffset);

  private:
    const uint64_t MAX_INDEX_SIZE = 0x100000000ul;

    uint32_t       mapFileIndex;     ///< Index in transferDef of the mapfile
    uint32_t       sharedFileIndex;  ///< Index in transferDef of the shared file
    filehandle*    mapFileHandle;    ///< File handler for the mapfile
    filehandle*    sharedFileHandle; ///< File handler for the shared file
    bscfs_index_t* index;            ///< Index mapping data file to shared file
    size_t         indexBytes;       ///< Size of the index file
    size_t         dataBytes;        ///< Size of the data file
    bool           writing;          ///< Are we writing or reading?
    bool           indexComplete;    ///< Has index been fully received?
};

#endif /* BB_BBIO_BSCFS_H_ */
