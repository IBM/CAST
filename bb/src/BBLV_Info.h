/*******************************************************************************
 |    BBLV_Info.h
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

#ifndef BB_BBLVINFO_H_
#define BB_BBLVINFO_H_

#include "bbinternal.h"
#include "BBJob.h"
#include "BBLV_ExtentInfo.h"
#include "BBLV_Metadata.h"
#include "BBStatus.h"
#include "BBTagID.h"
#include "BBTagInfoMap.h"
#include "ExtentInfo.h"
#include "LVKey.h"

/*******************************************************************************
 | Forward declarations
 *******************************************************************************/
class BBTagInfo;
class BBTransferDef;
class Extent;

/*******************************************************************************
 | Enumerators
 *******************************************************************************/

/*******************************************************************************
 | Constants
 *******************************************************************************/
const int DO_NOT_REMOVE_TARGET_PFS_FILES = 0;
const int REMOVE_TARGET_PFS_FILES = 1;

/*******************************************************************************
 | Classes
 *******************************************************************************/
/**
 * \class BBLV_Info
 * Contains the extent information and map of BBTagID->BBTagInfo and used when transfer by_extent
 */
class BBLV_Info
{
  public:
    BBLV_Info() :
        jobid(0),
        connectionName(UNDEFINED_CONNECTION_NAME),
        hostname(UNDEFINED_HOSTNAME) {
    };

    BBLV_Info(const string& pConnectionName, const string& pHostName, const uint64_t pJobId) :
        jobid(pJobId),
        connectionName(pConnectionName),
        hostname(pHostName) {
    };

    BBLV_Info(const string& pConnectionName, const string& pHostName, const BBTagInfoMap& pTagInfoMap) :
        jobid(0),
        connectionName(pConnectionName),
        hostname(pHostName),
        tagInfoMap(pTagInfoMap) {
    };

    void accumulateTotalLocalContributorInfo(const uint64_t pHandle, size_t& pTotalContributors, size_t& pTotalLocalReportingContributors);
    int allContribsReported(const uint64_t pHandle, const BBTagID& pTagId);
    int allExtentsTransferred(const BBTagID& pTagId);
    void cancelExtents(const LVKey* pLVKey, uint64_t* pHandle, uint32_t* pContribId, uint32_t pNumberOfExpectedInFlight, LOCAL_METADATA_RELEASED& pLockWasReleased, const int pRemoveOption=DO_NOT_REMOVE_TARGET_PFS_FILES);
    void cleanUpAll(const LVKey* pLVKey);
    void dump(char* pSev, const char* pPrefix=0);
    void ensureStageOutEnded(const LVKey* pLVKey, LOCAL_METADATA_RELEASED& pLockWasReleased);
    BBSTATUS getStatus(const uint64_t pHandle, BBTagInfo* pTagInfo);
    BBSTATUS getStatus(const uint64_t pHandle, const uint32_t pContribId, BBTagInfo* pTagInfo);
    int getTransferHandle(uint64_t& pHandle, const LVKey* pLVKey, const BBJob pJob, const uint64_t pTag, const uint64_t pNumContrib, const uint32_t pContrib[]);
    int prepareForRestart(const string& pConnectionName, const LVKey* pLVKey, BBTagInfo* pTagInfo, const BBJob pJob, const uint64_t pHandle, const int32_t pContribId, BBTransferDef* pOrigTransferDef, BBTransferDef* pRebuiltTransferDef, const int pPass);
    int recalculateFlags(const string& pConnectionName, const LVKey* pLVKey, BBTagInfoMap* pTagInfoMap, BBTagInfo* pTagInfo, const int64_t pHandle, const int32_t pContribId);
    void removeFromInFlight(const string& pConnectionName, const LVKey* pLVKey, BBTagInfo* pTagInfo, ExtentInfo& pExtentInfo);
    int retrieveTransfers(BBTransferDefs& pTransferDefs);
    void sendTransferCompleteForContribIdMsg(const string& pConnectionName, const LVKey* pLVKey, const int64_t pHandle, const int32_t pContribId, BBTransferDef* pTransferDef);
    void sendTransferCompleteForFileMsg(const string& pConnectionName, const LVKey* pLVKey, ExtentInfo& pExtentInfo, BBTransferDef* pTransferDef);
    void sendTransferCompleteForHandleMsg(const string& pHostName, const string& pCN_HostName, const string& pConnectionName, const LVKey* pLVKey, const BBTagID pTagId, const uint64_t pHandle, int& pAppendAsyncRequestFlag, const BBSTATUS pStatus=BBNONE);
    void setAllExtentsTransferred(const LVKey* pLVKey, const uint64_t pHandle, ExtentInfo& pExtentInfo, const BBTagID pTagId, const int pValue=1);
    void setCanceled(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, uint64_t pHandle, LOCAL_METADATA_RELEASED& pLockWasReleased, const int pRemoveOption);
    int setSuspended(const LVKey* pLVKey, const string& pHostName, const int pValue);
    int stopTransfer(const LVKey* pLVKey, const string& pHostName, const string& pCN_HostName, const uint64_t pJobId, const uint64_t pJobStepId, uint64_t pHandle, uint32_t pContribId, LOCAL_METADATA_RELEASED& pLockWasReleased);
    void updateAllContribsReported(const LVKey* pLVKey);
    int updateAllTransferStatus(const string& pConnectionName, const LVKey* pLVKey, ExtentInfo& pExtentInfo, uint32_t pNumberOfExpectedInFlight);
    void updateTransferStatus(const LVKey* pLVKey, ExtentInfo& pExtentInfo, const BBTagID& pTagId, const int32_t pContribId, int& pNewStatus, uint32_t pNumberOfExpectedInFlight);

    inline int addExtents(const LVKey* pLVKey, const uint64_t pHandle, const uint32_t pContribId, BBTagInfo* pTagInfo, BBTransferDef* pTransfer, vector<struct stat*>* pStats) {
        return extentInfo.addExtents(pLVKey, pHandle, pContribId, pTagInfo, pTransfer, pStats);
    }

    inline void addToInFlight(const string& pConnectionName, const LVKey* pLVKey, ExtentInfo& pExtentInfo) {
        return extentInfo.addToInFlight(pConnectionName, pLVKey, pExtentInfo);
    }

    inline int allContribsReported() {
        return extentInfo.allContribsReported();
    }

    inline int allExtentsTransferred() {
        return extentInfo.allExtentsTransferred();
    }

    inline int allExtentsTransferred(BBTransferDef* pTransferDef) {
        return pTransferDef->allExtentsTransferred();
    }

    inline int BSCFS_InRequest() {
        return extentInfo.BSCFS_InRequest();
    }

    inline int canceled(BBTransferDef* pTransferDef) {
        return pTransferDef->canceled();
    }

    inline void dumpInFlight(const char* pSev) const {
        return extentInfo.dumpInFlight(pSev);
    }

    inline int failed(BBTransferDef* pTransferDef) {
        return pTransferDef->failed();
    }

    inline string getConnectionName() {
        return connectionName;
    }

    inline BBLV_ExtentInfo* getExtentInfo() {
        return &extentInfo;
    }

    inline string getHostName() {
        return hostname;
    }

    inline uint64_t getJobId() {
        return jobid;
    }

    inline Extent* getMinTrimAnchorExtent() {
        return extentInfo.getMinTrimAnchorExtent();
    }

    inline ExtentInfo getNextExtentInfo() {
        return extentInfo.getNextExtentInfo();
    }

    inline size_t getNumberOfExtents() const {
        return extentInfo.getNumberOfExtents();
    }

    inline size_t getNumberOfInFlightExtents() {
        return extentInfo.getNumberOfInFlightExtents();
    }

    inline size_t getNumberOfTransferDefsWithOutstandingWorkItems() {
        return extentInfo.getNumberOfTransferDefsWithOutstandingWorkItems();
    }

    inline BBTagInfoMap* getTagInfoMap() {
        return &tagInfoMap;
    }

    inline int getTagInfo(const uint64_t pHandle, const uint32_t pContribId, BBTagID& pTagId, BBTagInfo* &pTagInfo) {
        return tagInfoMap.getTagInfo(pHandle, pContribId, pTagId, pTagInfo);
    }

    inline BBTagInfo* getTagInfo(const BBTagID& pTagId) {
        return tagInfoMap.getTagInfo(pTagId);
    }

    inline int getTagInfo(BBTagInfo* pTagInfo, const BBJob pJob, const uint64_t pTag, const uint64_t pNumContrib, const uint32_t pContrib[]) {
        return tagInfoMap.getTagInfo(pTagInfo, pJob, pTag, pNumContrib, pContrib);
    }

    inline size_t getTotalTransferSize() {
        return tagInfoMap.getTotalTransferSize();
    }

    inline void getTransferHandles(vector<uint64_t>& pHandles, const BBJob pJob, const BBSTATUS pMatchStatus, const int pStageOutStarted) {
        return tagInfoMap.getTransferHandles(pHandles, pJob, pMatchStatus, pStageOutStarted);
    }

    inline int hasCanceledExtents() {
        return extentInfo.hasCanceledExtents();
    }

    inline int hasContribId(const uint32_t pContribId) {
        return tagInfoMap.hasContribId(pContribId);
    };

    inline int isSuspended() {
        return extentInfo.isSuspended();
    }

    inline void mergeFlags(const uint64_t pFlags) {
        return extentInfo.mergeFlags(pFlags);
    }

    inline void removeExtent(const Extent* pExtent) {
        return extentInfo.removeExtent(pExtent);
    }

    inline void removeTargetFiles(const LVKey* pLVKey, const uint64_t pHandle, const uint32_t pContribId) {
        tagInfoMap.removeTargetFiles(pLVKey, pHandle, pContribId);
    }

    inline void resetMinTrimAnchorExtent() {
        return extentInfo.resetMinTrimAnchorExtent();
    }

    inline int resizeLogicalVolumeDuringStageOut() {
        return extentInfo.resizeLogicalVolumeDuringStageOut();
    }

    inline void sendTransferCompleteForHandleMsg(const string& pHostName, const string& pCN_HostName, const LVKey* pLVKey, const uint64_t pHandle, int& pAppendAsyncRequestFlag, const BBSTATUS pStatus=BBNONE) {
        return tagInfoMap.sendTransferCompleteForHandleMsg(pHostName, pCN_HostName, connectionName, pLVKey, this, pHandle, pAppendAsyncRequestFlag, pStatus);
    }

    inline void setAllContribsReported(const LVKey* pLVKey, const int pValue=1) {
        return extentInfo.setAllContribsReported(pLVKey, pValue);
    }

    inline void setAllExtentsTransferred(const string& pConnectionName, const LVKey* pLVKey, const int pValue=1) {
        return extentInfo.setAllExtentsTransferred(pConnectionName, pLVKey, pValue);
    }

    inline void setJobId(const uint64_t pJobId) {
        jobid = pJobId;

        return;
    }

    inline void setStageOutEnded(const LVKey* pLVKey, const uint64_t pJobId, const int pValue=1) {
        return extentInfo.setStageOutEnded(pLVKey, pJobId, pValue);
    }

    inline void setStageOutEndedComplete(const LVKey* pLVKey, const uint64_t pJobId, const int pValue=1) {
        return extentInfo.setStageOutEndedComplete(pLVKey, pJobId, pValue);
    }

    inline void setStageOutStarted(const LVKey* pLVKey, const uint64_t pJobId, const int pValue=1) {
        return extentInfo.setStageOutStarted(pLVKey, pJobId, pValue);
    }

    inline int sortExtents(const LVKey* pLVKey) {
        size_t l_NumberOfNewCanceledExtents = 0;
        return extentInfo.sortExtents(pLVKey, l_NumberOfNewCanceledExtents);
    }

    inline int stageOutEnded() {
        return extentInfo.stageOutEnded();
    }

    inline int stageOutEndedComplete() {
        return extentInfo.stageOutEndedComplete();
    }

    inline int stageOutStarted() {
        return extentInfo.stageOutStarted();
    }

    inline int updateAllTransferHandleStatus(const string& pConnectionName, const LVKey* pLVKey, const uint64_t pJobId, BBLV_ExtentInfo& pLVKey_ExtentInfo, uint32_t pNumberOfExpectedInFlight) {
        return tagInfoMap.updateAllTransferHandleStatus(pConnectionName, pLVKey, pJobId, pLVKey_ExtentInfo, pNumberOfExpectedInFlight);
    }

    inline void updateTransferStatus(const string& pConnectionName, const LVKey* pLVKey, uint32_t pNumberOfExpectedInFlight) {
        string l_ConnectionName = string();
        if (!pConnectionName.empty())
        {
            l_ConnectionName = pConnectionName;
        }
        else
        {
            l_ConnectionName = connectionName;
        }

        return extentInfo.updateTransferStatus(l_ConnectionName, pLVKey, pNumberOfExpectedInFlight);
    }

    inline void updateTransferStatus(const LVKey* pLVKey, ExtentInfo& pExtentInfo, BBTransferDef* pTransferDef, int& pNewStatus, int& pExtentsRemainForSourceIndex, uint32_t pNumberOfExpectedInFlight) {
        return extentInfo.updateTransferStatus(pLVKey, pExtentInfo, pTransferDef, pNewStatus, pExtentsRemainForSourceIndex, pNumberOfExpectedInFlight);
    }

    uint64_t            jobid;
    string              connectionName; //  NOTE: Currently only used to send handle completions messages back to
                                        //        bbProxy via the async message file
    string              hostname;
    BBLV_ExtentInfo     extentInfo;
    BBTagInfoMap        tagInfoMap;
};

#endif /* BB_BBLVINFO_H_ */

