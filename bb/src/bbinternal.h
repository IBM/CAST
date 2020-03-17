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
class BBLocalAsync;
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
   AtomicCounter() : value(0) {};

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
extern BBLocalAsync g_LocalAsync;
extern WRKQMGR wrkqmgr;
extern WRKQE* HPWrkQE;
// extern Timer ResizeSSD_Timer;
extern Timer Throttle_Timer;
extern AtomicCounter metadataCounter;
extern bool g_AsyncRemoveJobInfo;
extern bool g_UseDirectIO;
extern int g_DiskStatsRate;
extern int g_DumpTransferMetadataAfterQueue;
extern int g_DumpStatsBeforeAddingToAllExtents;
extern int g_DumpExtentsBeforeAddingToAllExtents;
extern int g_DumpExtentsBeforeSort;
extern int g_DumpExtentsAfterSort;
extern uint32_t g_NumberOfAsyncRequestsThreads;
extern int64_t g_IBStatsLowActivityClipValue;
extern uint64_t g_ForceSSDReadError;
extern uint64_t g_ForceSSDWriteError;
extern uint64_t g_ForcePFSReadError;
extern uint64_t g_ForcePFSWriteError;
// extern double ResizeSSD_TimeInterval;
extern double Throttle_TimeInterval;
extern double AsyncRequestRead_TimeInterval;
extern string g_BBServer_Metadata_Path;
extern int l_SSD_Read_Governor_Active;
extern int l_SSD_Write_Governor_Active;
extern sem_t l_SSD_Read_Governor;
extern sem_t l_SSD_Write_Governor;

void setSsdWriteDirect(unsigned int pValue);
#endif

#if BBPROXY | BBSERVER
extern uint64_t g_TimeBaseScale;
#endif

/*******************************************************************************
 | External methods
 *******************************************************************************/
extern void contribToString(stringstream& pOutput, vector<uint32_t>& pContrib);
extern void flightlog_Backtrace(uint64_t key);
extern int logBacktrace();
extern int32_t readVar(const char* pVariable);
extern string resolveServerConfigKey(const string& pKey);
extern int sameHostName(const string& pHostName);
extern void writeVar(const char* pVariable, const char* pValue);

/*******************************************************************************
 | Constants
 *******************************************************************************/
const double DEFAULT_ASYNC_REMOVEJOBINFO_INTERVAL_VALUE = 180;                  // in seconds (3 minutes)
const double DEFAULT_ASYNC_REMOVEJOBINFO_MINIMUM_INTERVAL_VALUE = 60;           // in seconds (1 minute)
const double DEFAULT_BBSERVER_DUMP_COUNTERS_TIME_INTERVAL = 60;                 // in seconds (1 minute)
const double DEFAULT_BBSERVER_HEARTBEAT_DUMP_INTERVAL = 1800;                   // in seconds (30 minutes)
const double DEFAULT_BBSERVER_HEARTBEAT_TIME_INTERVAL = 180;                    // in seconds (3 minutes)
const double DEFAULT_BBSERVER_IBSTATS_TIME_INTERVAL = 60;                       // in seconds (1 minute)
const double DEFAULT_BBSERVER_IOSTATS_TIME_INTERVAL = 60;                       // in seconds (1 minute)
// const double DEFAULT_BBSERVER_RESIZE_SSD_TIME_INTERVAL = 8;                     // in seconds
const double DEFAULT_IBSTATS_LOW_ACTIVITY_RATE = 0.25;                          // NOTE: This value represents the
                                                                                //       rate in GB/sec
const double DEFAULT_LOG_UPDATE_HANDLE_STATUS_ELAPSED_TIME_CLIP_VALUE = 1.00;   // in seconds
const double DEFAULT_BBSERVER_THROTTLE_TIME_INTERVAL = 0.25;                    // in seconds
const double MAXIMUM_BBSERVER_THROTTLE_TIME_INTERVAL = 1.00;                    // in seconds
const double DEFAULT_BBSERVER_ASYNC_REQUEST_READ_TIME_INTERVAL = 25.00;         // in seconds
const double MAXIMUM_BBSERVER_ASYNC_REQUEST_READ_TIME_INTERVAL = 100.00;        // in seconds
const uint64_t MINIMUM_BBSERVER_DECLARE_SERVER_DEAD_VALUE = 240;                // in seconds (4 minutes)
const uint64_t MAXIMUM_BBSERVER_DECLARE_SERVER_DEAD_VALUE = 600;                // in seconds (10 minutes)
const unsigned int DEFAULT_BBSERVER_NUMBER_OF_LOCAL_ASYNC_REQUEST_THREADS = 48;
const unsigned int DEFAULT_BBSERVER_NUMBER_OF_TRANSFER_THREADS = 24;

// NOTE: If the BB throttling rate is used to limit the amount of
//       bandwidth BB consumes, the following default value should be
//       changed to the default rate/sec value used for BB throttling.
const uint64_t DEFAULT_MAXIMUM_TRANSFER_SIZE = 1*1024*1024*1024;
const uint64_t DEFAULT_NUMBER_OF_HANDLES = 1024;
const uint64_t DEFAULT_FORCE_SSD_READ_ERROR = 0;
const uint64_t DEFAULT_FORCE_SSD_WRITE_ERROR = 0;
const uint64_t DEFAULT_FORCE_PFS_READ_ERROR = 0;
const uint64_t DEFAULT_FORCE_PFS_WRITE_ERROR = 0;

const uint64_t DEFAULT_NUMBER_OF_HANDLEFILE_BUCKETS = 4096;

const uint64_t UNDEFINED_JOBID = 0;
const uint64_t UNDEFINED_JOBSTEPID = 0;
const uint64_t DEFAULT_JOBID = 1;
const uint64_t DEFAULT_JOBSTEPID = 1;
const uint64_t UNDEFINED_HANDLE = 0;

const int CONSECUTIVE_SUSPENDED_WORK_QUEUES_NOT_PROCESSED_THRESHOLD = 10;
const int DO_NOT_DUMP_QUEUES_ON_VALUE = -1;
const int ASYNC_REQUEST_HAS_NOT_BEEN_APPENDED = 0;
const int ASYNC_REQUEST_HAS_BEEN_APPENDED = 1;
const int MORE_EXTENTS_TO_TRANSFER = 1;
const int NO_MORE_EXTENTS_TO_TRANSFER = 0;
const int LOCK_TRANSFER_QUEUE = 1;
const int DO_NOT_LOCK_TRANSFER_QUEUE = 0;
const int FIRST_PASS = 1;
const int SECOND_PASS = 2;
const int THIRD_PASS = 3;
const int RESUME = 0;
const int SUSPEND = 1;

const int DEFAULT_TRANSFER_METADATA_AFTER_QUEUE_VALUE = 0;
const int DEFAULT_DUMP_STATS_BEFORE_ADDING_TO_ALLEXTENTS_VALUE = 1;
const int DEFAULT_DUMP_EXTENTS_BEFORE_ADDING_TO_ALLEXTENTS_VALUE = 0;
const int DEFAULT_DUMP_EXTENTS_BEFORE_SORT_VALUE = 0;
const int DEFAULT_DUMP_EXTENTS_AFTER_SORT_VALUE = 0;
const int DEFAULT_USE_ASYNC_REQUEST_READ_TURBO_MODE = 1;
const int DEFAULT_DISKSTATS_RATE = 60;

const uint32_t NO_CONTRIBID = 999999998;
const uint32_t CONTRIBID_AS_INITIALIZED = NO_CONTRIBID;
const uint32_t UNDEFINED_CONTRIBID = 999999999;
const uint32_t DEFAULT_CONTRIBID = 0;
const uint32_t MAX_NUMBER_OF_CONTRIBS = 1*64*1024;
const uint32_t DEFAULT_SSD_READ_GOVERNOR = 0;
const uint32_t DEFAULT_SSD_WRITE_GOVERNOR = 0;

const int32_t DEFAULT_LOGICAL_VOLUME_READAHEAD = -1;
const int32_t DO_NOT_TRANSFER_FILE = INT32_MAX;

const bool DEFAULT_USE_DISCARD_ON_MOUNT_OPTION = false;
const bool DEFAULT_REQUIRE_BBSERVER_METADATA_ON_PARALLEL_FILE_SYSTEM = true;
const bool DEFAULT_GENERATE_UUID_ON_CREATE_LOGICAL_VOLUME = true;
const bool DEFAULT_ABORT_ON_CRITICAL_ERROR = false;
const bool DEFAULT_LOG_ALL_ASYNC_REQUEST_ACTIVITY = false;
const bool DEFAULT_ASYNC_REMOVEJOBINFO_VALUE = true;
const bool DEFAULT_USE_DIRECT_IO_VALUE = true;

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
const string DEFAULT_LOCK_DEBUG_LEVEL = "debug";
const char DEVICE_DIRECTORY[] = "dev";
const char DEVICE_MAPPER_DIRECTORY[] = "dev/mapper";
const char ERROR_PREFIX[] = "ERROR - ";
const char LV_DISPLAY_PREFIX[] = "LV Path";
const char LV_DISPLAY_OPEN_PREFIX[] = "# open";
const char MOUNTS_DIRECTORY[] = "/proc/mounts";
const char TOPLEVEL_HANDLEFILE_NAME[] = "handle_";


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

enum LOCAL_METADATA_RELEASED
{
    LOCAL_METADATA_LOCK_NOT_RELEASED    = 0,
    LOCAL_METADATA_LOCK_RELEASED        = 1
};
typedef enum LOCAL_METADATA_RELEASED LOCAL_METADATA_RELEASED;

enum HANDLEFILE_LOCK_OPTION
{
    TEST_FOR_HANDLEFILE_LOCK        = -1,   // Not currently used
    DO_NOT_LOCK_HANDLEFILE          = 0,
    LOCK_HANDLEFILE                 = 1,
    LOCK_HANDLEFILE_WITH_TEST_FIRST = 2     // Not currently used
};
typedef enum HANDLEFILE_LOCK_OPTION HANDLEFILE_LOCK_OPTION;

enum HANDLEFILE_LOCK_FEEDBACK
{
    HANDLEFILE_WAS_NOT_LOCKED       = 0,
    HANDLEFILE_WAS_LOCKED           = 1
};
typedef enum HANDLEFILE_LOCK_FEEDBACK HANDLEFILE_LOCK_FEEDBACK;

enum HANDLEFILE_SCAN_OPTION
{
    NORMAL_SCAN = 0,
    FULL_SCAN   = 1
};
typedef enum HANDLEFILE_SCAN_OPTION HANDLEFILE_SCAN_OPTION;

enum ALLOW_BUMP_FOR_REPORTING_CONTRIBS_OPTION
{
    DO_NOT_ALLOW_BUMP_FOR_REPORTING_CONTRIBS = 0,
    ALLOW_BUMP_FOR_REPORTING_CONTRIBS        = 1
};
typedef enum ALLOW_BUMP_FOR_REPORTING_CONTRIBS_OPTION ALLOW_BUMP_FOR_REPORTING_CONTRIBS_OPTION;

enum DUMP_ALL_DATA_INDICATOR
{
    DO_NOT_DUMP_ALL_DATA    = 0,
    DUMP_ALL_DATA           = 1
};
typedef enum DUMP_ALL_DATA_INDICATOR DUMP_ALL_DATA_INDICATOR;

enum RETURN_REMOVED_JOBIDS_INDICATOR
{
    ONLY_RETURN_VALID_JOBIDS        = 1,
    ONLY_RETURN_REMOVED_JOBIDS      = 2
};
typedef enum RETURN_REMOVED_JOBIDS_INDICATOR RETURN_REMOVED_JOBIDS_INDICATOR;

enum VALIDATION_OPTION
{
    DO_NOT_PERFORM_VALIDATION = 0,
    PERFORM_VALIDATION        = 1
};
typedef enum VALIDATION_OPTION VALIDATION_OPTION;

enum XBBSERVER_JOB_EXISTS_OPTION
{
    XBBSERVER_JOB_EXISTS            = 0,
    XBBSERVER_JOB_DOES_NOT_EXIST    = 1
};
typedef enum XBBSERVER_JOB_EXISTS_OPTION XBBSERVER_JOB_EXISTS_OPTION;

enum PERFORM_CONTRIBID_CLEANUP_OPTION
{
    DO_NOT_PERFORM_CONTRIBID_CLEANUP  = 0,
    PERFORM_CONTRIBID_CLEANUP         = 1
};
typedef enum PERFORM_CONTRIBID_CLEANUP_OPTION PERFORM_CONTRIBID_CLEANUP_OPTION;

enum PERFORM_TAGINFO_CLEANUP_OPTION
{
    DO_NOT_PERFORM_TAGINFO_CLEANUP  = 0,
    PERFORM_TAGINFO_CLEANUP         = 1
};
typedef enum PERFORM_TAGINFO_CLEANUP_OPTION PERFORM_TAGINFO_CLEANUP_OPTION;

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

#ifdef __powerpc64__
#define  SPRN_TBRO                (0x10C)          // Time Base 64-bit, User Read-only
#endif

inline void BB_GetTime(uint64_t& pTime)
{

#ifdef __powerpc64__
    asm volatile("mfspr %0,%1;"
                 : "=&r" (pTime) : "i" (SPRN_TBRO) : "memory");
#elif __x86_64__
    unsigned hi, lo;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    pTime = ((uint64_t)hi << 32ull) | lo;
#else
#error not supported
#endif

    return;
}

inline void BB_GetTimeDifference(uint64_t& pTime)
{
    uint64_t l_EndTime;
    BB_GetTime(l_EndTime);

    pTime = l_EndTime - pTime;

    return;
}

/*******************************************************************************
 | External helper methods
 *******************************************************************************/
extern BBSTATUS getBBStatusFromStr(const char* pStatusStr);
extern void getStrFromBBStatus(BBSTATUS pValue, char* pBuffer, const size_t pSize);
extern void getStrFromTransferType(const uint64_t pFlags, char* pBuffer, const size_t pSize);
extern void BB_GetTime(uint64_t& pTime);
extern void BB_GetTimeDifference(uint64_t& pTime);

#endif /* BB_BBINERNAL_H_ */
