/*******************************************************************************
 |    xfer.h
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


#ifndef BB_XFER_H_
#define BB_XFER_H_

#include "bbinternal.h"
#include "BBTagInfo.h"


/*******************************************************************************
 | External variables
 *******************************************************************************/
extern pthread_mutex_t lock_transferqueue;
extern sem_t sem_workqueue;

extern pthread_mutex_t lock_metadata;

extern string l_LockDebugLevel;


/*******************************************************************************
 | Constants
 *******************************************************************************/


/*******************************************************************************
 | Enumerators
 *******************************************************************************/
enum FORCE_OPTION
{
    NOT_FORCED = 0,
    FORCED = 1
};
typedef enum FORCE_OPTION FORCE_OPTION;

enum TOLERATE_ALREADY_EXISTS_OPTION
{
    DO_NOT_TOLERATE_ALREADY_EXISTS = 0,
    TOLERATE_ALREADY_EXISTS = 1
};
typedef enum TOLERATE_ALREADY_EXISTS_OPTION TOLERATE_ALREADY_EXISTS_OPTION;


/*******************************************************************************
 | External functions
 *******************************************************************************/
extern int addLogicalVolume(const std::string& pConnectionName, const string& pHostName, txp::Msg* pMsg, const LVKey* pLVKey, const uint64_t pJobId, const TOLERATE_ALREADY_EXISTS_OPTION pTolerateAlreadyExists);

extern int cancelTransferForHandle(const string& pHostName, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const int pRemoveOption);

extern int doForceStopTransfer(const LVKey* pLVKey, ContribIdFile* pContribIdFile, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId);

extern int forceStopTransfer(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId);

extern int getHandle(const std::string& pConnectionName, LVKey* &pLVKey, BBJob pJob, const uint64_t pTag, uint64_t pNumContrib, uint32_t pContrib[], uint64_t& pHandle);

extern int getThrottleRate(const std::string& pConnectionName, LVKey* pLVKey, uint64_t& pRate);

extern int jobStillExists(const std::string& pConnectionName, const LVKey* pLVKey, BBLV_Info* pLV_Info, BBTagInfo* pTagInfo, const uint64_t pJobId, const uint32_t pContribId);

// Use this method to lock the local metadata if it is known that this thread
// does not already own the lock.
extern void lockLocalMetadata(const LVKey* pLVKey, const char* pMethod);

// Use this method to lock the local metadata if it is possible that this thread
// already owns the lock.  The return value indicates whether the lock needed to
// be obtained and was obtained.
extern int lockLocalMetadataIfNeeded(const LVKey* pLVKey, const char* pMethod);

extern bool localMetadataIsLocked();

// Use this method to lock the transfer queue if it is known that this thread
// does not already own the lock.
extern void lockTransferQueue(const LVKey* pLVKey, const char* pMethod);

// Use this method to lock the transfer queue if it is possible that this thread
// already owns the lock.  The return value indicates whether the lock needed to
// be obtained and was obtained.
extern int lockTransferQueueIfNeeded(const LVKey* pLVKey, const char* pMethod);

extern void markTransferFailed(const LVKey* pLVKey, BBTransferDef* pTransferDef, BBLV_Info* pLV_Info, uint64_t pHandle, uint32_t pContribId);

extern int prepareForRestartOriginalServerDead(const std::string& pConnectionName, const LVKey* pLVKey, const uint64_t pHandle, BBJob pJob, const int32_t pContribId);

extern int queueTransfer(const std::string& pConnectionName, LVKey* pLVKey, BBJob pJob, const uint64_t pTag, BBTransferDef* &pTransferDef, \
                         const int32_t pContribId, uint64_t pNumContrib, uint32_t pContrib[], uint64_t& pHandle, const uint32_t pPerformOperation, \
                         uint32_t &pMarkFailedFromProxy, vector<struct stat*>* pStats);

extern int removeJobInfo(const string& pHostName, const uint64_t pJobId);

extern int removeLogicalVolume(const std::string& pConnectionName, const LVKey* pLVKey);

extern int setThrottleRate(const std::string& pConnectionName, LVKey* pLVKey, uint64_t pRate);

extern int stageoutEnd(const std::string& pConnectionName, const LVKey* pLVKey, const FORCE_OPTION pForced=NOT_FORCED);

extern int stageoutStart(const std::string& pConnectionName, const LVKey* pLVKey);

extern void startTransferThreads();

extern void switchIdsToMountPoint(txp::Msg* pMsg);

extern void unlockLocalMetadata(const LVKey* pLVKey, const char* pMethod);

// Use this method to unlock the local metadata if it is possible that this thread
// may not own the lock.  The return value indicates whether the lock needed to
// be released and was released.
extern int unlockLocalMetadataIfNeeded(const LVKey* pLVKey, const char* pMethod);

// Use this method to unlock the transfer queue if it is known that this thread
// already owns the lock.
extern void unlockTransferQueue(const LVKey* pLVKey, const char* pMethod);

// Use this method to unlock the transfer queue if it is possible that this thread
// may not own the lock.  The return value indicates whether the lock needed to
// be released and was released.
extern int unlockTransferQueueIfNeeded(const LVKey* pLVKey, const char* pMethod);

#endif /* BB_XFER_H_ */
