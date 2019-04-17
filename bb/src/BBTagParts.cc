/*******************************************************************************
 |    BBTagParts.cc
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

#include "bbinternal.h"
#include "bbserver_flightlog.h"
#include "BBLV_Info.h"
#include "BBTagParts.h"
#include "logging.h"

//
// BBTagParts class
//

int BBTagParts::addTransferDef(const LVKey* pLVKey, const uint64_t pHandle, const uint32_t pContribId, BBTransferDef* &pTransferDef) {
    int rc = 0;

    tagParts[pContribId] = *pTransferDef;

    // Return the transfer definition just copied into tagparts...
    pTransferDef = &tagParts[pContribId];

    uint64_t l_NumberOfExtents = pTransferDef->getNumberOfExtents();
    LOG(bb,debug) << "BBTagParts::addTransferDef: For " << *pLVKey << ", handle " << pHandle
                  << ", contribid " << pContribId << ", number of extents " << l_NumberOfExtents;

    if (!l_NumberOfExtents) {
        pTransferDef->setAllExtentsTransferred(pLVKey, pHandle, pContribId);
    }

    return rc;
}

int BBTagParts::allExtentsTransferred(const uint32_t pContribId) {
    if (tagParts.find(pContribId) != tagParts.end()) {
        return tagParts[pContribId].allExtentsTransferred();
    } else {
        return -1;
    }
}

int BBTagParts::anyCanceledTransferDefinitions() {
    int rc = 0;

    for (auto it = tagParts.begin(); (!rc) && it != tagParts.end(); ++it) {
        rc = (it->second).canceled();
    }

    return rc;
}

int BBTagParts::anyFailedTransferDefinitions() {
    int rc = 0;

    for (auto it = tagParts.begin(); (!rc) && it != tagParts.end(); ++it) {
        rc = (it->second).failed();
    }

    return rc;
}

int BBTagParts::anyStoppedTransferDefinitions() {
    int rc = 0;

    for (auto it = tagParts.begin(); (!rc) && it != tagParts.end(); ++it) {
        rc = (it->second).stopped();
    }

    return rc;
}

int BBTagParts::canceled(const uint32_t pContribId) {
    if (tagParts.find(pContribId) != tagParts.end()) {
        return tagParts[pContribId].canceled();
    } else {
        return -1;
    }
}

void BBTagParts::cleanUpAll(const LVKey* pLVKey, const BBTagID pTagId)
{
    Uuid lv_uuid = pLVKey->second;
    char lv_uuid_str[LENGTH_UUID_STR] = {'\0'};
    lv_uuid.copyTo(lv_uuid_str);

    stringstream l_JobStr;
    pTagId.getJob().getStr(l_JobStr);

    auto it = tagParts.begin();
    while (it != tagParts.end()) {
        it->second.cleanUp();
        LOG(bb,info) << "taginfo: Contrib(" << it->first << ") removed from TagId(" << l_JobStr.str() << "," << pTagId.getTag() \
                     << ") for " << *pLVKey;
        it = tagParts.erase(it);
    }

    return;
}

void BBTagParts::dump(const char* pSev) {
    if (tagParts.size()) {
        if (!strcmp(pSev,"debug")) {
            LOG(bb,debug) << ">>>>> Start: " << tagParts.size() \
                          << (tagParts.size()==1 ? " transfer definition <<<<<" : " transfer definitions <<<<<");
            for (auto& part : tagParts) {
                LOG(bb,debug) << "Contrib: " << part.first;
                part.second.dump(pSev);
            }
            LOG(bb,debug) << ">>>>>   End: " << tagParts.size() \
                          << (tagParts.size()==1 ? " transfer definition <<<<<" : " transfer definitions <<<<<");
        } else if (!strcmp(pSev,"info")) {
            LOG(bb,info) << ">>>>> Start: " << tagParts.size() \
                         << (tagParts.size()==1 ? " transfer definition <<<<<" : " transfer definitions <<<<<");
            for (auto& part : tagParts) {
                LOG(bb,info) << "Contrib: " << part.first;
                part.second.dump(pSev);
            }
            LOG(bb,info) << ">>>>>   End: " << tagParts.size() \
                         << (tagParts.size()==1 ? " transfer definition <<<<<" : " transfer definitions <<<<<");
        }
    }

    return;
}

int BBTagParts::failed(const uint32_t pContribId) {
    if (tagParts.find(pContribId) != tagParts.end()) {
        return tagParts[pContribId].failed();
    } else {
        return -1;
    }
}

BBJob BBTagParts::getJob(const uint32_t pContribId) {
    if (tagParts.find(pContribId) != tagParts.end()) {
        return tagParts[pContribId].getJob();
    } else {
        return 0;
    }
}

BBSTATUS BBTagParts::getStatus(const uint32_t pContribId) {
    BBSTATUS l_Status = BBNONE;

    if (tagParts.find(pContribId) != tagParts.end()) {
        l_Status = tagParts[pContribId].getStatus();
    } else {
        l_Status = BBNOTREPORTED;
    }

    return l_Status;
}

uint64_t BBTagParts::getTag(const uint32_t pContribId) {
    if (tagParts.find(pContribId) != tagParts.end()) {
        return tagParts[pContribId].getTag();
    } else {
        return 0;
    }
}

size_t BBTagParts::getTotalTransferSize() {
    size_t l_TotalSize = 0;

    for (auto it = tagParts.begin(); it != tagParts.end(); ++it) {
        l_TotalSize += BBTransferDef::getTotalTransferSize(&(it->second));
    }

    return l_TotalSize;
}

size_t BBTagParts::getTotalTransferSize(const uint32_t pContribId) {
    if (tagParts.find(pContribId) != tagParts.end()) {
        return BBTransferDef::getTotalTransferSize(&(tagParts[pContribId]));
    } else {
        return 0;
    }
}

size_t BBTagParts::getTotalTransferSize(const uint32_t pContribId, const uint32_t pSourceIndex) {
    if (tagParts.find(pContribId) != tagParts.end()) {
        return BBTransferDef::getTotalTransferSize(&(tagParts[pContribId]), pSourceIndex);
    } else {
        return 0;
    }
}

BBTransferDef* BBTagParts::getTransferDef(const uint32_t pContribId) const {
    for (auto it = tagParts.begin(); it != tagParts.end(); ++it) {
        if (it->first == pContribId)
            return const_cast <BBTransferDef*> (&(it->second));
    }

    return (BBTransferDef*)0;
}

int BBTagParts::retrieveTransfers(BBTransferDefs& pTransferDefs, BBLV_ExtentInfo* pExtentInfo)
{
    int rc = 0;

    for (auto it = tagParts.begin(); (!rc) && it != tagParts.end(); ++it) {
        if (pTransferDefs.getContribId() == UNDEFINED_CONTRIBID || pTransferDefs.getContribId() == it->first)
        {
            rc = (it->second).retrieveTransfers(pTransferDefs, pExtentInfo);
        }
    }

    return rc;
}

void BBTagParts::removeTargetFiles(const LVKey* pLVKey, const uint32_t pContribId)
{
    for (auto it = tagParts.begin(); it != tagParts.end(); ++it) {
        if (pContribId == UNDEFINED_CONTRIBID || it->first == pContribId) {
            BBTransferDef* l_TransferDef = const_cast <BBTransferDef*> (&(it->second));
            l_TransferDef->removeTargetFiles(pLVKey);
        }
    }

    return;
}

int BBTagParts::setCanceled(const LVKey* pLVKey, uint64_t pHandle, const uint32_t pContribId, const int pValue) {
    int rc =-1;

    for (auto it = tagParts.begin(); it != tagParts.end(); ++it) {
        if (it->first == pContribId) {
            BBTransferDef* l_TransferDef = const_cast <BBTransferDef*> (&(it->second));
            l_TransferDef->setCanceled(pLVKey, pHandle, pContribId, pValue);
            rc = 0;
        }
    }

    return rc;
}

int BBTagParts::setFailed(const LVKey* pLVKey, uint64_t pHandle, const uint32_t pContribId, const int pValue)
{
    int rc =-1;

    for (auto it = tagParts.begin(); it != tagParts.end(); ++it)
    {
        if (it->first == pContribId)
        {
            BBTransferDef* l_TransferDef = const_cast <BBTransferDef*> (&(it->second));
            l_TransferDef->setFailed(pLVKey, pHandle, pValue);
            rc = 0;
        }
    }

    return rc;
}

int BBTagParts::stopTransfer(const LVKey* pLVKey, const string& pHostName, const string& pCN_HostName, BBLV_Info* pLV_Info, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId, TRANSFER_QUEUE_RELEASED& pLockWasReleased)
{
    int rc = 0;

    // NOTE: pLockWasReleased intentionally not initialized

    for (auto it = tagParts.begin(); ((!rc) && it != tagParts.end()); ++it)
    {
        // NOTE: Check for an already stopped transfer definition.  If so, then this transfer definition
        //       IS NOT associated with the correct LVKey.  This LVKey has already been failed over...
        if ((pContribId == UNDEFINED_CONTRIBID || it->first == pContribId) && (!(it->second).stopped()))
        {
            uint32_t l_ContribId = it->first;
            // NOTE: If we are using multiple transfer threads, we have to make sure that there are
            //       no extents for this transfer definition currently in-flight on this bbServer...
            //       If so, delay for a bit...
            // NOTE: In the normal case, the work queue for the CN hostname on this bbServer should
            //       be suspended, so no new extents should start processing when we release/re-acquire
            //       the lock below...
            uint32_t i = 0;
            while (pLV_Info->getExtentInfo()->moreInFlightExtentsForTransferDefinition(pHandle, l_ContribId))
            {
                unlockTransferQueue(pLVKey, "stopTransfer - Waiting for inflight queue to clear");
                {
                    pLockWasReleased = TRANSFER_QUEUE_LOCK_RELEASED;
                    // NOTE: Currently set to send info to console after 3 seconds of not being able to clear, and every 10 seconds thereafter...
                    if ((i++ % 40) == 12)
                    {
                        FL_Write(FLDelay, StopTransfer, "Waiting for in-flight queue to clear of extents for handle %ld, contribid %ld.",
                                 pHandle, l_ContribId, 0, 0);
                        LOG(bb,info) << ">>>>> DELAY <<<<< stopTransfer(): Waiting for in-flight queue to clear of extents for handle " << pHandle \
                                     << ", contribid " << l_ContribId;
                        pLV_Info->getExtentInfo()->dumpInFlight("info");
                    }
                    usleep((useconds_t)250000);
                }
                lockTransferQueue(pLVKey, "stopTransfer - Waiting for inflight queue to clear");
            }

            BBTransferDef* l_TransferDef = const_cast <BBTransferDef*> (&(it->second));
            rc = l_TransferDef->stopTransfer(pLVKey, pHostName, pCN_HostName, pJobId, pJobStepId, pHandle, l_ContribId, pLockWasReleased);
        }
    }

    return rc;
}
