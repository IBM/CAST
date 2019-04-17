/*******************************************************************************
 |    BBTagInfoMap.h
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

#ifndef BB_BBTAGINFOMAP_H_
#define BB_BBTAGINFOMAP_H_

#include <map>
#include <vector>

#include <boost/filesystem.hpp>

#include "BBStatus.h"
#include "BBJob.h"
#include "BBTagInfo.h"
#include "BBTransferDef.h"
#include "HandleFile.h"
#include "LVKey.h"
#include "WorkID.h"
#include "bbinternal.h"

/*******************************************************************************
 | Forward declarations
 *******************************************************************************/
class BBLV_ExtentInfo;
class BBTagParts;
class ContribIdFile;
class HandleFile;

/*******************************************************************************
 | Constants
 *******************************************************************************/


/*******************************************************************************
 | Classes
 *******************************************************************************/

/**
 * \class BBTagInfoMap
 * Defines the map of BBTagID->BBTagInfo
 */
class BBTagInfoMap
{
  public:
    // Static methods

    // Non-static methods
    void accumulateTotalLocalContributorInfo(const uint64_t pHandle, size_t& pTotalContributors, size_t& pTotalLocalReportingContributors);
    int addTagInfo(const LVKey* pLVKey, const BBJob pJob, const BBTagID pTagId, BBTagInfo& pTagInfo, int& pGeneratedHandle);
    void cleanUpAll(const LVKey* pLVKey);
    void dump(char* pSev, const char* pPrefix=0);
    size_t getNumberOfTransferDefs(const BBTagID& pTagId);
    BBTagParts* getParts(const BBTagID& pTagId);
    int getTagInfo(const uint64_t pHandle, const uint32_t pContribId, BBTagID& pTagId, BBTagInfo* &pTagInfo);
    BBTagInfo* getTagInfo(const BBTagID& pTagId);
    int getTagInfo(BBTagInfo* &pTagInfo, const BBJob pJob, const uint64_t pTag, const uint64_t pNumContrib, const uint32_t pContrib[]);
    size_t getTotalTransferSize();
    void getTransferHandles(vector<uint64_t>& pHandles, const BBJob pJob, const BBSTATUS pMatchStatus, const int pStageOutStarted);
    int hasContribId(const uint32_t pContribId);
    int isUniqueHandle(uint64_t pHandle);
    void removeTargetFiles(const LVKey* pLVKey, const uint64_t pHandle, const uint32_t pContribId);
    void removeTransferDef(const BBTagID& pTagId, const uint32_t pContribId);
    int retrieveTransfers(BBTransferDefs& pTransferDefs, BBLV_ExtentInfo* pExtentInfo);
    void sendTransferCompleteForHandleMsg(const string& pHostName, const string& pCN_HostName, const string& pConnectionName, const LVKey* pLVKey, BBLV_Info* pLV_Info, const uint64_t pHandle, int& pAppendAsyncRequestFlag, const BBSTATUS pStatus=BBNONE);
    void setCanceled(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle);
    int stopTransfer(const LVKey* pLVKey, BBLV_Info* pLV_Info, const string& pHostName, const string& pCN_HostName, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId, TRANSFER_QUEUE_RELEASED& pLockWasReleased);
    void updateAllContribsReported(const LVKey* pLVKey, int& pAllReported);
    int updateAllTransferHandleStatus(const string& pConnectionName, const LVKey* pLVKey, const uint64_t pJobId, BBLV_ExtentInfo& pLVKey_ExtentInfo, uint32_t pNumberOfExpectedInFlight);
    int update_xbbServerAddData(const LVKey* pLVKey, const BBJob pJob, const uint64_t pTag, BBTagInfo& pTagInfo);

    inline void removeTagInfo(const BBTagID& pTagId) {
        tagInfoMap.erase(pTagId);

        return;
    }

    inline size_t size() {
        return tagInfoMap.size();
    }

    map<BBTagID, BBTagInfo, BBTagID_Compare> tagInfoMap;
};

extern BBTagInfoMap taginfo;

#endif /* BB_BBTAGINFOMAP_H_ */
