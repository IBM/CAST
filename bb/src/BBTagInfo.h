/*******************************************************************************
 |    BBTagInfo.h
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

#ifndef BB_BBTAGINFO_H_
#define BB_BBTAGINFO_H_

#include <map>

#include <boost/filesystem.hpp>

#include "bberror.h"
#include "BBTagParts.h"
#include "BBTransferDef.h"
#include "ContribFile.h"
#include "ContribIdFile.h"

namespace bfs = boost::filesystem;

/*******************************************************************************
 | Forward declarations
 *******************************************************************************/
class BBLV_Info;
class BBTagInfoMap;
class HandleFile;
class LVUuidFile;

/*******************************************************************************
 | Enumerators
 *******************************************************************************/

/*******************************************************************************
 | Constants
 *******************************************************************************/

/*******************************************************************************
 | Classes
 *******************************************************************************/

/**
 * \class BBTagInfo
 * Defines the information for a given BBTagID
 */
class BBTagInfo
{
  public:
    BBTagInfo() :
        flags(0),
        transferHandle(0) {};

    BBTagInfo(BBTagInfoMap* pTagInfo, const uint64_t pNumContrib, const uint32_t pContrib[], const BBJob pJob, const uint64_t pTag, int& pGeneratedHandle);

    // Static methods
    static int compareContrib(const uint64_t pNumContrib, const uint32_t pContrib[], vector<uint32_t>& pContribVector);
    static void genTransferHandle(uint64_t& pHandle, const BBJob pJob, const uint64_t pTag, vector<uint32_t>& pContrib);

    // Non-static methods
    void accumulateTotalLocalContributorInfo(const uint64_t pHandle, size_t& pTotalContributors, size_t& pTotalLocalContributors);
    int addTransferDef(const std::string& pConnectionName, const LVKey* pLVKey, const BBJob pJob, BBLV_Info* pLV_Info, const BBTagID pTagId, const uint32_t pContribId, const uint64_t pHandle, BBTransferDef* &pTransferDef);
    void bumpTransferHandle(uint64_t& pHandle);
    void calcCanceled(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle);
    void calcStopped(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle);
    void dump(const char* pSev);
    void expectContribToSS(stringstream& pSS) const;
    uint64_t get_xbbServerHandle(const BBJob& pJob, const uint64_t pTag);
    BBSTATUS getStatus(const int pStageOutStarted);
    void getTransferHandle(uint64_t& pHandle, BBTagInfoMap* pTagInfo, const BBJob pJob, const uint64_t pTag, int& pGeneratedHandle);
    int inExpectContrib(const uint32_t pContribId);
    int prepareForRestart(const std::string& pConnectionName, const LVKey* pLVKey, const BBJob pJob, const uint64_t pHandle, const int32_t pContribId, BBTransferDef* l_OrigTransferDef, BBTransferDef* pRebuiltTransferDef, const int pPass);
    int retrieveTransfers(BBTransferDefs& pTransferDefs, BBLV_ExtentInfo* pExtentInfo);
    void sendTransferCompleteForHandleMsg(const string& pHostName, const string& pCN_HostName, const string& pConnectionName, const LVKey* pLVKey, BBLV_Info* pLV_Info, const BBTagID pTagId, const uint64_t pHandle, int& pAppendAsyncRequestFlag, const BBSTATUS pStatus);
    void setAllContribsReported(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const int pValue=1);
    void setAllExtentsTransferred(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const int pValue=1);
    void setCanceledForHandle(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId, const int pValue=1);
    void setStopped(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId, const int pValue=1);
    int stopTransfer(const LVKey* pLVKey, BBLV_Info* pLV_Info, const string& pHostName, const string& pCN_HostName, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId, TRANSFER_QUEUE_RELEASED& pLockWasReleased);
    int update_xbbServerAddData(const LVKey* pLVKey, HandleFile* pHandleFile, const char* pHandleFileName, const BBJob pJob, BBLV_Info* pLV_Info, const uint32_t pContribId, const uint64_t pHandle, BBTransferDef* &pTransferDef);
    int xbbServerIsHandleUnique(const BBJob& pJob, const uint64_t pHandle);

    inline int allContribsReported() {
        RETURN_FLAG(BBTI_All_Contribs_Reported);
    }

    inline int allExtentsTransferred() {
        RETURN_FLAG(BBTD_All_Extents_Transferred);
    }

    inline int allExtentsTransferred(const uint32_t pContribId) {
        return parts.allExtentsTransferred(pContribId);
    }

    inline int canceled() {
        RETURN_FLAG(BBTD_Canceled);
    }

    inline int canceled(const uint32_t pContribId) {
        return parts.canceled(pContribId);
    }

    inline void cleanUpAll(const LVKey* pLVKey, const BBTagID& pTagId) {
        parts.cleanUpAll(pLVKey, pTagId);

        return;
    }

    inline int compareContrib(const uint64_t pNumContrib, const uint32_t pContrib[]) {
        return compareContrib(pNumContrib, pContrib, expectContrib);
    }

    inline int failed() {
        RETURN_FLAG(BBTD_Failed);
    }

    inline int failed(const uint32_t pContribId) {
        return parts.failed(pContribId);
    }

    inline void genTransferHandle(uint64_t& pHandle, const BBJob pJob, const uint64_t pTag) {
        return genTransferHandle(pHandle, pJob, pTag, expectContrib);
    }

    inline vector<uint32_t>* getExpectContrib() {
        return &expectContrib;
    }

    inline BBJob getJob(const uint32_t pContribId) {
        return parts.getJob(pContribId);
    }

    inline size_t getTotalContributors() const {
        return expectContrib.size();
    }

    inline size_t getTotalLocalReportingContributors() const {
        return parts.getNumberOfParts();
    }

    inline size_t getNumberOfTransferDefs() const {
        return parts.getNumberOfParts();
    }

    inline BBTagParts* getParts() {
        return &parts;
    }

    inline BBSTATUS getStatus(const uint32_t pContribId) {
        return parts.getStatus(pContribId);
    }

    inline uint64_t getTag(const uint32_t pContribId) {
        return parts.getTag(pContribId);
    }

    inline size_t getTotalTransferSize(const uint32_t pContribId) {
        return parts.getTotalTransferSize(pContribId);
    }

    inline size_t getTotalTransferSize() {
        return parts.getTotalTransferSize();
    }

    inline BBTransferDef* getTransferDef(const uint32_t pContribId) {
        return parts.getTransferDef(pContribId);
    }

    inline uint64_t getTransferHandle() {
        return transferHandle;
    }

    inline void removeTargetFiles(const LVKey* pLVKey, const uint64_t pHandle, const uint32_t pContribId) {
        if (pHandle == transferHandle)
        {
            return parts.removeTargetFiles(pLVKey, pContribId);
        }
    }

    inline void removeTransferDef(const uint32_t pContribId) {
        return parts.removeTransferDef(pContribId);
    }

    inline int replaceExtentVector(const uint32_t pContribId, BBTransferDef* pTransferDef) {
        return parts.replaceExtentVector(pContribId, pTransferDef);
    }

    inline int setCanceled(const LVKey* pLVKey, uint64_t pHandle, const uint32_t pContribId, const int pValue=1) {
        return parts.setCanceled(pLVKey, pHandle, pContribId, pValue);
    }

    inline int setFailed(const LVKey* pLVKey, uint64_t pHandle, const uint32_t pContribId, const int pValue=1) {
        return parts.setFailed(pLVKey, pHandle, pContribId, pValue);
    }

    inline void setTransferHandle(const uint64_t pHandle) {
        transferHandle = pHandle;

        return;
    }

    inline int stopped() {
        RETURN_FLAG(BBTD_Stopped);
    }

    uint64_t            flags;
    uint64_t            transferHandle;
    vector<uint32_t>    expectContrib;
    BBTagParts          parts;
};

#endif /* BB_BBTAGINFO_H_ */
