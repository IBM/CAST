/*******************************************************************************
 |    BBTagParts.h
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

#ifndef BB_BBTAGPARTS_H_
#define BB_BBTAGPARTS_H_

#include <map>

#include "BBTagID.h"
#include "BBTransferDef.h"

/*******************************************************************************
 | Forward Declarations
 *******************************************************************************/
class BBLV_Info;
class BBTagInfo;

/*******************************************************************************
 | Classes
 *******************************************************************************/

/**
 * \class BBTagParts
 * Defines a map of transfer definitions to contributors
 */
class BBTagParts
{
  public:
    int allExtentsTransferred(BBTagInfo* pTagInfo, const uint32_t pContribId);
    int anyCanceledTransferDefinitions(BBTagInfo* pTagInfo);
    int anyFailedTransferDefinitions(BBTagInfo* pTagInfo);
    int anyStoppedTransferDefinitions(BBTagInfo* pTagInfo);
    int addTransferDef(const LVKey* pLVKey, const uint64_t pHandle, const uint32_t pContribId, BBTransferDef* &pTransferDef);
    int canceled(BBTagInfo* pTagInfo, const uint32_t pContribId);
    void cleanUpAll(const LVKey* pLVKey, const BBTagID pTagId);
    void dump(const char* pSev);
    int failed(BBTagInfo* pTagInfo, const uint32_t pContribId);
    BBJob getJob(BBTagInfo* pTagInfo, const uint32_t pContribId);
    BBSTATUS getStatus(BBTagInfo* pTagInfo, const uint32_t pContribId);
    uint64_t getTag(BBTagInfo* pTagInfo, const uint32_t pContribId);
    size_t getTotalTransferSize(BBTagInfo* pTagInfo);
    size_t getTotalTransferSize(BBTagInfo* pTagInfo, const uint32_t pContribId);
    size_t getTotalTransferSize(BBTagInfo* pTagInfo, const uint32_t pContribId, const uint32_t pSourceIndex);
    BBTransferDef* getTransferDef(BBTagInfo* pTagInfo, const uint32_t pContribId) const;
    void removeTargetFiles(const LVKey* pLVKey, const uint32_t pContribId);
    int retrieveTransfers(BBTransferDefs& pTransferDefs, BBLV_ExtentInfo* pExtentInfo);
    int setCanceled(const LVKey* pLVKey, BBTagInfo* pTagInfo, uint64_t pHandle, const uint32_t pContribId, const int pValue=1);
    int setFailed(const LVKey* pLVKey, BBTagInfo* pTagInfo, uint64_t pHandle, const uint32_t pContribId, const int pValue=1);
    int stopTransfer(const LVKey* pLVKey, const string& pHostName, const string& pCN_HostName, BBLV_Info* pLV_Info, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId, LOCAL_METADATA_RELEASED& pLockWasReleased);

    inline size_t getNumberOfParts() const
    {
        return tagParts.size();
    }

    inline void removeTransferDef(const uint32_t pContribId)
    {
        tagParts.erase(pContribId);

        return;
    }

    inline int replaceExtentVector(BBTagInfo* pTagInfo, const uint32_t pContribId, BBTransferDef* pTransferDef) {
        return getTransferDef(pTagInfo, pContribId)->replaceExtentVector(pTransferDef);
    }

    map<uint32_t, BBTransferDef> tagParts;
};

#endif /* BB_BBTAGPARTS_H_ */
