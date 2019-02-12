/*******************************************************************************
 |    bbinternal.h
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

#ifndef BB_BBINERNAL_H_
#define BB_BBINERNAL_H_

#include <atomic>
#include <string>

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

using namespace std;

#include "bbapi_types.h"
#include "bbapi2.h"
#include "bbapiAdmin.h"
#include "bbdefaults.h"
#include "bberror.h"
#include "Connex.h"
#include "Msg.h"
#include "nodecontroller.h"

/*******************************************************************************
 | Forward declarations
 *******************************************************************************/
class BBLV_Metadata;
class WRKQMGR;
class WRKQE;

/*******************************************************************************
 | Typedefs
 *******************************************************************************/
typedef struct fiemap fiemap_t;
typedef struct fiemap_extent fiemap_extent_t;

/*******************************************************************************
 | Classes and structs
 *******************************************************************************/
// Metadata counter for flight logging
class AtomicCounter
{
public:
   AtomicCounter() : value(1) {};

   uint64_t getNext()
   {
      return ++value;
   };

private:
   std::atomic<uint64_t> value;
};

/*******************************************************************************
 | External data
 *******************************************************************************/
// NOTE:  The following variable is set in main() when bbproxy is started.
//        It identifies the name of the proxy daemon on this compute node.
//        If not the proxy daemon, this variable is set to the empty string.
extern string ProcessId;

extern thread_local std::string bbconnectionName;
extern BBLV_Metadata metadata;

#if BBSERVER
extern WRKQMGR wrkqmgr;
extern WRKQE* HPWrkQE;
extern double ResizeSSD_TimeInterval;
extern double Throttle_TimeInterval;
extern Timer ResizeSSD_Timer;
extern Timer Throttle_Timer;
extern AtomicCounter metadataCounter;

void setSsdWriteDirect(unsigned int pValue);

// NOTE: This lock is not used today.  Serialization of all metadata is controlled by the transfer queue lock
//extern pthread_mutex_t MetadataMutex;
#endif

/*******************************************************************************
 | External methods
 *******************************************************************************/
extern int logBacktrace();
extern int32_t readVar(const char* pVariable);
extern string resolveServerConfigKey(const string& pKey);
extern int sameHostName(const string& pHostName);
extern void writeVar(const char* pVariable, const char* pValue);
extern void flightlog_Backtrace(uint64_t key);

/*******************************************************************************
 | Constants
 *******************************************************************************/
const double DEFAULT_BBSERVER_CONSOLE_TRANSFER_STATUS_TIME_INTERVAL = 5;    // in seconds
const double DEFAULT_BBSERVER_HEARTBEAT_DUMP_INTERVAL = 3600;       // in seconds (1 hour)
const double DEFAULT_BBSERVER_HEARTBEAT_TIME_INTERVAL = 300;        // in seconds (5 minutes)
const double DEFAULT_BBSERVER_RESIZE_SSD_TIME_INTERVAL = 8;         // in seconds
const double DEFAULT_BBSERVER_THROTTLE_TIME_INTERVAL = 0.25;        // in seconds
const double MAXIMUM_BBSERVER_THROTTLE_TIME_INTERVAL = 1;           // in seconds
const uint64_t MINIMUM_BBSERVER_DECLARE_SERVER_DEAD_VALUE = 300;    // in seconds

const int CONSECUTIVE_SUSPENDED_WORK_QUEUES_NOT_PROCESSED_THRESHOLD = 10;
const int DO_NOT_DUMP_QUEUES_ON_VALUE = -1;
const int ASYNC_REQUEST_HAS_NOT_BEEN_APPENDED = 0;
const int ASYNC_REQUEST_HAS_BEEN_APPENDED = 1;
const int MORE_EXTENTS_TO_TRANSFER_FOR_FILE = 1;
const int NO_MORE_EXTENTS_TO_TRANSFER_FOR_FILE = 0;
const int LOCK_TRANSFER_QUEUE = 1;
const int DO_NOT_LOCK_TRANSFER_QUEUE = 0;
const int FIRST_PASS = 1;
const int SECOND_PASS = 2;
const int THIRD_PASS = 3;
const int RESUME = 0;
const int SUSPEND = 1;

const bool DEFAULT_USE_DISCARD_ON_MOUNT_OPTION = false;
const bool DEFAULT_REQUIRE_BBSERVER_METADATA_ON_PARALLEL_FILE_SYSTEM = true;

const uint64_t DEFAULT_JOBID = 1;
const uint64_t NO_JOBID = 0;
const uint64_t DEFAULT_JOBSTEPID = 1;
const uint64_t NO_JOBSTEPID = 0;

const int32_t DEFAULT_LOGICAL_VOLUME_READAHEAD = -1;
const int32_t DO_NOT_TRANSFER_FILE = INT32_MAX;

const uint32_t NO_CONTRIBID = 999999998;
const uint32_t CONTRIBID_AS_INITIALIZED = NO_CONTRIBID;
const uint32_t UNDEFINED_CONTRIBID = 999999999;
const uint32_t DEFAULT_CONTRIBID = 0;
const uint32_t MAX_NUMBER_OF_CONTRIBS = 1*64*1024;

const uint64_t DEFAULT_MAXIMUM_TRANSFER_SIZE = 1*1024*1024*1024;
const uint64_t DEFAULT_NUMBER_OF_HANDLES = 1024;

const string ALL = "*";

const string DEFAULT_JOBID_ENVVAR = "LSFJOBID";
const string DEFAULT_JOBSTEPID_ENVVAR = "LSFJOBSTEPID";
const string DEFAULT_PROXY_ALIAS = "bbproxy";
const string DEFAULT_SERVER_ALIAS = "bbserver";
const string DEFAULT_PROXY_NAME = "bb.proxy";
const string DEFAULT_SERVER_NAME = "bb.server0";
const string DEFAULT_SERVER_LEAVING_ALIAS = "bbserverX";
const string DEFAULT_SERVER_BACKUP_ALIAS = "bbserverbackup";
const string DEFAULT_UNIXPATH = "/run/bb_socket";
const string DEFAULT_JOBID_STR = to_string(DEFAULT_JOBID);
const string DEFAULT_JOBSTEPID_STR = to_string(DEFAULT_JOBSTEPID);
const string DEFAULT_CONTRIBID_STR = to_string(DEFAULT_CONTRIBID);
const string NO_CONTRIBID_STR = to_string(NO_CONTRIBID);
const string NO_HOSTNAME = "%%_NuLl_HoStNaMe_%%";
const string UNDEFINED_CONNECTION_NAME = "";
const string UNDEFINED_HOSTNAME = "";
const char DEVICE_DIRECTORY[] = "dev";
const char DEVICE_MAPPER_DIRECTORY[] = "dev/mapper";
const char ERROR_PREFIX[] = "ERROR - ";
const char LV_DISPLAY_PREFIX[] = "LV Path";
const char LV_DISPLAY_OPEN_PREFIX[] = "# open";
const char MOUNTS_DIRECTORY[] = "/proc/mounts";

/*******************************************************************************
 | Enumerators
 *******************************************************************************/
enum CHECK_FOR_RESTART_INDICATOR
{
    DO_NOT_CHECK_FOR_RESTART    = 0,
    CHECK_FOR_RESTART           = 1
};
typedef enum CHECK_FOR_RESTART_INDICATOR CHECK_FOR_RESTART_INDICATOR;

enum CLEAR_METADATA_ON_CANCEL
{
    DO_NOT_CLEAR_METADATA   = 0,
    CLEAR_METADATA          = 1
};
typedef enum CLEAR_METADATA_ON_CANCEL CLEAR_METADATA_ON_CANCEL;

enum ON_ERROR_FILL_IN_ERRSTATE
{
    DO_NOT_FILL_IN_ERRSTATE = 0,
    FILL_IN_ERRSTATE        = 1
};
typedef enum ON_ERROR_FILL_IN_ERRSTATE ON_ERROR_FILL_IN_ERRSTATE;

enum FIND_BB_DEVNAMES_OPTION
{
    FIND_ALL_BB_DEVICES             = 0,
    FIND_ALL_UNMOUNTED_BB_DEVICES   = 1,
    FIND_ALL_MOUNTED_BB_DEVICES     = 2
};
typedef enum FIND_BB_DEVNAMES_OPTION FIND_BB_DEVNAMES_OPTION;

enum TRANSFER_QUEUE_RELEASED
{
    TRANSFER_QUEUE_LOCK_NOT_RELEASED    = 0,
    TRANSFER_QUEUE_LOCK_RELEASED        = 1
};
typedef enum TRANSFER_QUEUE_RELEASED TRANSFER_QUEUE_RELEASED;

enum HANDLEFILE_LOCK_OPTION
{
    TEST_FOR_HANDLEFILE_LOCK        = -1,   // Not currently used
    DO_NOT_LOCK_HANDLEFILE          = 0,
    LOCK_HANDLEFILE                 = 1,
    LOCK_HANDLEFILE_WITH_TEST_FIRST = 2     // Not curently used
};
typedef enum HANDLEFILE_LOCK_OPTION HANDLEFILE_LOCK_OPTION;

enum HANDLEFILE_LOCK_FEEDBACK
{
    HANDLEFILE_WAS_NOT_LOCKED       = 0,
    HANDLEFILE_WAS_LOCKED           = 1
};
typedef enum HANDLEFILE_LOCK_FEEDBACK HANDLEFILE_LOCK_FEEDBACK;

/*******************************************************************************
 | Macro definitions
 *******************************************************************************/
#ifndef MIN
#define MIN(a,b) (((a)>(b))?(b):(a))
#endif
#ifndef MAX
#define MAX(a,b) (((a)<(b))?(b):(a))
#endif

#define REMOVE_LINE_FEED(BUFFER) \
    if (l_Line[strlen(l_Line)-1] == '\n') { \
        l_Line[strlen(l_Line)-1] = 0; \
    }

/*******************************************************************************
 | Helper methods
 *******************************************************************************/
inline BBCREATEFLAGS get_coptions(const char* pOption) {
    BBCREATEFLAGS l_Flags;

    if (pOption) {
        if (strcmp(pOption, "BBEXT4") == 0)
            l_Flags = BBEXT4;
        else if (strcmp(pOption, "BBXFS") == 0)
            l_Flags = BBXFS;
        else if (strcmp(pOption, "BBFSCUSTOM1") == 0)
            l_Flags = BBFSCUSTOM1;
        else if (strcmp(pOption, "BBFSCUSTOM2") == 0)
            l_Flags = BBFSCUSTOM2;
        else if (strcmp(pOption, "BBFSCUSTOM3") == 0)
            l_Flags = BBFSCUSTOM3;
        else if (strcmp(pOption, "BBFSCUSTOM4") == 0)
            l_Flags = BBFSCUSTOM4;
        else
            l_Flags = BBCREATEFLAGS(-1);
    } else {
        l_Flags = DEFAULT_COPTION;
    }

    return l_Flags;
}

inline int check_createlv_flags(const BBCREATEFLAGS pFlags) {
    int rc = 0;

    switch (pFlags) {
        case BBEXT4:
        case BBXFS:
            break;
        case BBFSCUSTOM1:
        case BBFSCUSTOM2:
        case BBFSCUSTOM3:
        case BBFSCUSTOM4:
        default:
            rc = -1;
    }

    return rc;
}

inline int get_coptionsFromStr(BBCREATEFLAGS& pFlags, const char* pCreateFlagsStr)
{
    int rc = 0;

    pFlags = BBCREATEFLAGS(0);

    if (strcmp(pCreateFlagsStr, "ext4") == 0)
        pFlags = BBEXT4;
    else if (strcmp(pCreateFlagsStr, "xfs") == 0)
        pFlags = BBXFS;
    else
        rc = -1;

    return rc;
}

inline BBCANCELSCOPE get_cancelScope(const char* pScope) {
    BBCANCELSCOPE l_Scope;

    if (pScope) {
        if (strcmp(pScope, "BBSCOPETRANSFER") == 0)
            l_Scope = BBSCOPETRANSFER;
        else if (strcmp(pScope, "BBSCOPETAG") == 0)
            l_Scope = BBSCOPETAG;
        else
            l_Scope = BBCANCELSCOPE(-1);
    } else {
        l_Scope = DEFAULT_CANCELSCOPE;
    }

    return l_Scope;
}

inline int check_cancel_scope(const BBCANCELSCOPE pScope) {
    int rc = 0;

    switch (pScope) {
        case BBSCOPETRANSFER:
        case BBSCOPETAG:
            break;
        default:
            rc = -1;
    }

    return rc;
}

inline int get_cancelScopeFromStr(BBCANCELSCOPE& pScope, const char* pCancelScopeStr)
{
    int rc = 0;

    pScope = BBCANCELSCOPE(0);

    if (strcmp(pCancelScopeStr, "BBSCOPETRANSFER") == 0)
        pScope = BBSCOPETRANSFER;
    else if (strcmp(pCancelScopeStr, "BBSCOPETAG") == 0)
        pScope = BBSCOPETAG;
    else
        rc = -1;

    return rc;
}

inline void getStrFromCancelScope(BBCANCELSCOPE pScope, char* pBuffer, const size_t pSize)
{
    if (pSize) {
        pBuffer[0] = '\0';
        switch (pScope) {
            case BBSCOPETAG:
                strCpy(pBuffer, "BBSCOPETAG", pSize);
                break;
            case BBSCOPETRANSFER:
                strCpy(pBuffer, "BBSCOPETRANSFER", pSize);
                break;

            default:
                snprintf(pBuffer, pSize, "%s (%d)", "UNDEFINED", pScope);
        }
    }

    return;
}

inline BBRESIZEFLAGS get_roptions(const char* pOption) {
    BBRESIZEFLAGS l_Flags;

    if (pOption) {
        if (strcmp(pOption, "BB_DO_NOT_PRESERVE_FS") == 0) {
            l_Flags = BB_DO_NOT_PRESERVE_FS;
        } else if (strcmp(pOption, "BB_NONE") == 0) {
            l_Flags = BB_NONE;
        } else {
            l_Flags = BBRESIZEFLAGS(-1);
        }
    } else {
        l_Flags = DEFAULT_ROPTION;
    }

    return l_Flags;
}

inline int check_resize_flags(const BBRESIZEFLAGS pFlags) {
    int rc = 0;

    switch (pFlags) {
        case BB_DO_NOT_PRESERVE_FS:
        case BB_NONE:
            break;
        default:
            rc = -1;
    }

    return rc;
}

inline BB_RTV_TRANSFERDEFS_FLAGS get_rtvoptions(const char* pOption) {
    BB_RTV_TRANSFERDEFS_FLAGS l_Flags;

    if (pOption) {
        if (strcmp(pOption, "ALL_DEFINITIONS") == 0)
            l_Flags = ALL_DEFINITIONS;
        else if (strcmp(pOption, "ONLY_DEFINITIONS_WITH_UNFINISHED_FILES") == 0)
            l_Flags = ONLY_DEFINITIONS_WITH_UNFINISHED_FILES;
        else if (strcmp(pOption, "ONLY_DEFINITIONS_WITH_STOPPED_FILES") == 0)
            l_Flags = ONLY_DEFINITIONS_WITH_STOPPED_FILES;
        else
            l_Flags = BB_RTV_TRANSFERDEFS_FLAGS(-1);
    } else {
        l_Flags = DEFAULT_RTV_TRANSFERDEFS_FLAGS;
    }

    return l_Flags;
}

inline int check_retrieve_transfer_defs_flags(const BB_RTV_TRANSFERDEFS_FLAGS pFlags) {
    int rc = 0;

    switch (pFlags) {
        case ALL_DEFINITIONS:
        case ONLY_DEFINITIONS_WITH_UNFINISHED_FILES:
        case ONLY_DEFINITIONS_WITH_STOPPED_FILES:
            break;
        default:
            rc = -1;
    }

    return rc;
}

inline int get_rtvoptionsFromStr(BB_RTV_TRANSFERDEFS_FLAGS& pFlags, const char* pRetrieveTransdefsStr)
{
    int rc = 0;

    pFlags = BB_RTV_TRANSFERDEFS_FLAGS(0);

    if (strcmp(pRetrieveTransdefsStr, "ALL_DEFINITIONS") == 0)
        pFlags = ALL_DEFINITIONS;
    else if (strcmp(pRetrieveTransdefsStr, "ONLY_DEFINITIONS_WITH_UNFINISHED_FILES") == 0)
        pFlags = ONLY_DEFINITIONS_WITH_UNFINISHED_FILES;
    else if (strcmp(pRetrieveTransdefsStr, "ONLY_DEFINITIONS_WITH_STOPPED_FILES") == 0)
        pFlags = ONLY_DEFINITIONS_WITH_STOPPED_FILES;
    else
        rc = -1;

    return rc;
}

inline void getStrFromRtvTransferDefsOptions(BB_RTV_TRANSFERDEFS_FLAGS pFlags, char* pBuffer, const size_t pSize)
{
    if (pSize) {
        pBuffer[0] = '\0';
        switch (pFlags) {
            case ALL_DEFINITIONS:
                strCpy(pBuffer, "ALL_DEFINITIONS", pSize);
                break;
            case ONLY_DEFINITIONS_WITH_UNFINISHED_FILES:
                strCpy(pBuffer, "ONLY_DEFINITIONS_WITH_UNFINISHED_FILES", pSize);
                break;
            case ONLY_DEFINITIONS_WITH_STOPPED_FILES:
                strCpy(pBuffer, "ONLY_DEFINITIONS_WITH_STOPPED_FILES", pSize);
                break;

            default:
                snprintf(pBuffer, pSize, "%s (%d)", "UNDEFINED", pFlags);
        }
    }

    return;
}

inline int check_match_status(const char* pMatchStatus) {
    int rc = 0;

    if (!((strstr(pMatchStatus, "BBNOTSTARTED")) ||
          (strstr(pMatchStatus, "BBINPROGRESS")) ||
          (strstr(pMatchStatus, "BBPARTIALSUCCESS")) ||
          (strstr(pMatchStatus, "BBFULLSUCCESS")) ||
          (strstr(pMatchStatus, "BBCANCELED")) ||
          (strstr(pMatchStatus, "BBFAILED")) ||
          (strstr(pMatchStatus, "BBSTOPPED")) ||
          (strstr(pMatchStatus, "BBNOTACONTRIB")) ||
          (strstr(pMatchStatus, "BBNOTREPORTED")) ||
          (strstr(pMatchStatus, "BBALL"))
       ))
    {
        rc = -1;
    }

    return rc;
}


/*******************************************************************************
 | External helper methods
 *******************************************************************************/
extern BBSTATUS getBBStatusFromStr(const char* pStatusStr);
extern void getStrFromBBStatus(BBSTATUS pValue, char* pBuffer, const size_t pSize);
extern void getStrFromTransferType(const uint64_t pFlags, char* pBuffer, const size_t pSize);

#endif /* BB_BBINERNAL_H_ */
