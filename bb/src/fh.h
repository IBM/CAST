/*******************************************************************************
 |    fh.h
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

#ifndef BB_FH_H_
#define BB_FH_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "util.h"

// Enumerators
enum BBFILESTATUS
{
    BBFILE_NONE             = 0x0000000000000000,
    BBFILE_SUCCESS          = 0x0000000000000001,
    BBFILE_FAILED           = 0x0000000000000002,
    BBFILE_CANCELED         = 0x0000000000000004,
    BBFILE_STOPPED          = 0x0000000000000008,
    BBFILE_NOT_TRANSFERRED  = 0x0000000000000010,   // NOTE: Only used for files where
                                                    //       a start transfer skipped the file.
};
typedef enum BBFILESTATUS BBFILESTATUS;


// Helper methods


inline int getBBFileStatusFromStr(BBFILESTATUS& pStatus, const char* pStatusStr)
{
    int rc = 0;

    pStatus = BBFILE_NONE;
    if (strcmp(pStatusStr, "BBFILE_SUCCESS") == 0)
        pStatus = BBFILE_SUCCESS;
    else if (strcmp(pStatusStr, "BBFILE_STOPPED") == 0)
        pStatus = BBFILE_STOPPED;
    else if (strcmp(pStatusStr, "BBFILE_FAILED") == 0)
        pStatus = BBFILE_FAILED;
    else if (strcmp(pStatusStr, "BBFILE_CANCELED") == 0)
        pStatus = BBFILE_CANCELED;
    else if (strcmp(pStatusStr, "BBFILE_NOT_TRANSFERRED") == 0)
        pStatus = BBFILE_NOT_TRANSFERRED;
    else
        rc = 1;

    return rc;
}

inline void getStrFromBBFileStatus(BBFILESTATUS pValue, char* pBuffer, const size_t pSize)
{
    if (pSize) {
        pBuffer[0] = '\0';
        switch (pValue) {
            case BBFILE_SUCCESS:
                strCpy(pBuffer, "BBFILE_SUCCESS", pSize);
                break;
            case BBFILE_STOPPED:
                strCpy(pBuffer, "BBFILE_STOPPED", pSize);
                break;
            case BBFILE_FAILED:
                strCpy(pBuffer, "BBFILE_FAILED", pSize);
                break;
            case BBFILE_CANCELED:
                strCpy(pBuffer, "BBFILE_CANCELED", pSize);
                break;
            case BBFILE_NOT_TRANSFERRED:
                strCpy(pBuffer, "BBFILE_NOT_TRANSFERRED", pSize);
                break;
            case BBFILE_NONE:
                strCpy(pBuffer, "BBFILE_NONE", pSize);
                break;

            default:
                snprintf(pBuffer, pSize, "%s (%u)", "UNDEFINED", pValue);
        }
    }

    return;
}

class filehandleData
{
  public:
    virtual ~filehandleData() {};
};


// Filehandle class
class filehandle
{
  private:
    int fd;
    std::string         filename;
    struct stat         statinfo;
    class extentLookup* extlookup;
    int                 fd_oflags;
    volatile uint32_t   numWritesNoSync;
    volatile size_t     totalSizeWritesNoSync;
    bool                isdevzero;
    filehandleData*     privateData;
    int openErrno;

  public:
    filehandle() :
        fd(-1),
        filename("")
        {
            memset((void*)&(statinfo), 0, sizeof(statinfo));
            extlookup = NULL;
            fd_oflags = 0;
            numWritesNoSync = 0;
            totalSizeWritesNoSync = 0;
            isdevzero = false;
            privateData = NULL;
            openErrno = 0;
        };
    filehandle(const std::string& fn) :
        fd(-1),
        filename(fn)
        {
            memset((void*)&(statinfo), 0, sizeof(statinfo));
            extlookup = NULL;
            fd_oflags = 0;
            numWritesNoSync = 0;
            totalSizeWritesNoSync = 0;
            isdevzero = false;
            privateData = NULL;
            openErrno = 0;
        };
    filehandle(const std::string& fn, int oflag, mode_t mode);
    ~filehandle();

//    NOTE: To house the stats, we can create filehandles without a valid file descriptor (i.e., no open file).
//          getfd() is used to simply print out the file descriptor for a handle.  We do not want to throw an exception
//          in that case.  This check doesn't catch 'bad' positive file descriptors.  If we attempt to use an invalid file
//          descriptor, we will get an exception anyway.
    inline int getfd() const { return fd; };
    inline bool validfd(){ return (fd>0);}
    inline int getOpenErrno(){return openErrno;}
    inline int getflags() const { return fd_oflags; };
    inline int getmode() const { return statinfo.st_mode; };
    inline uint32_t getNumWritesNoSync() { return numWritesNoSync; }
    inline size_t getsize() const { return statinfo.st_size; };
    inline uint32_t getTotalSizeWritesNoSync() { return totalSizeWritesNoSync; };
    inline void incrNumWritesNoSync(const uint32_t pValue=1) { numWritesNoSync += pValue; return; }
    inline void incrTotalSizeWritesNoSync(const size_t pValue) { totalSizeWritesNoSync += pValue; return; }
    inline void setNumWritesNoSync(const uint32_t pValue) { numWritesNoSync = pValue; return; }
    inline void setTotalSizeWritesNoSync(const size_t pValue) { totalSizeWritesNoSync = pValue; return; }
    inline void setPrivate(filehandleData* ptr) { privateData = ptr; };
    inline filehandleData* getPrivate() { return privateData; };

    int             close();
    void            dump(const char* pSev, const char* pPrefix=0);
    int             getstats(struct stat& statbuf);
    int             protect(off_t start, size_t len, bool writing, class Extent& input, vector<class Extent>& result);
    int             release(BBFILESTATUS completion_status);
    int             setsize(size_t newsize);
    void            updateStats(struct stat* stats);

    const std::string& getfn() const { return filename; };
};

extern int addFilehandle(filehandle* fh, uint64_t jobid, uint64_t handle, uint32_t contrib, uint32_t index);
extern void dumpFileHandleMap(const char* pSev, const char* pPrefix);
extern int fileHandleCount();
extern int findFilehandle(filehandle* &fh, uint64_t jobid, uint64_t handle, uint32_t contrib, uint32_t index);
extern int removeFilehandle(filehandle* &fh, uint64_t jobid, uint64_t handle, uint32_t contrib, uint32_t index);
extern void removeNextFilehandleByJobId(filehandle* &fh, uint64_t jobid);

#endif /* BB_FH_H_ */
