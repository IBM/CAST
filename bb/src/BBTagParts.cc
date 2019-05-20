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

    // It is possible to enter this section of code without the transfer queue locked.
    // Inserting into a std::map is not thread safe, so we must acquire the lock around
    // the insert.
    int l_TransferQueueWasLocked = lockTransferQueueIfNeeded(pLVKey, "BBTagParts::addTransferDef");
    tagParts[pContribId] = *pTransferDef;
    if (l_TransferQueueWasLocked)
    {
        unlockTransferQueue(pLVKey, "BBTagParts::addTransferDef");
    }

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
    int rc = -1;

    int l_TransferQueueWasLocked = lockTransferQueueIfNeeded((LVKey*)0, "BBTagParts::allExtentsTransferred");

    if (tagParts.find(pContribId) != tagParts.end()) {
        rc = tagParts[pContribId].allExtentsTransferred();
    }

    if (l_TransferQueueWasLocked)
    {
        unlockTransferQueue((LVKey*)0, "BBTagParts::allExtentsTransferred");
    }

    return rc;
}

int BBTagParts::anyCanceledTransferDefinitions() {
    int rc = 0;

    int l_TransferQueueWasLocked = lockTransferQueueIfNeeded((LVKey*)0, "BBTagParts::anyCanceledTransferDefinitions");

    // NOTE: It is only considered canceled if the stopped bit is also off
    for (auto it = tagParts.begin(); (!rc) && it != tagParts.end(); ++it) {
        rc = ((!(it->second).stopped()) && (it->second).canceled());
    }

    if (l_TransferQueueWasLocked)
    {
        unlockTransferQueue((LVKey*)0, "BBTagParts::anyCanceledTransferDefinitions");
    }

    return rc;
}

int BBTagParts::anyFailedTransferDefinitions() {
    int rc = 0;

    int l_TransferQueueWasLocked = lockTransferQueueIfNeeded((LVKey*)0, "BBTagParts::anyFailedTransferDefinitions");

    for (auto it = tagParts.begin(); (!rc) && it != tagParts.end(); ++it) {
        rc = (it->second).failed();
    }

    if (l_TransferQueueWasLocked)
    {
        unlockTransferQueue((LVKey*)0, "BBTagParts::anyFailedTransferDefinitions");
    }

    return rc;
}

int BBTagParts::anyStoppedTransferDefinitions() {
    int rc = 0;

    int l_TransferQueueWasLocked = lockTransferQueueIfNeeded((LVKey*)0, "BBTagParts::anyStoppedTransferDefinitions");

    for (auto it = tagParts.begin(); (!rc) && it != tagParts.end(); ++it) {
        rc = (it->second).stopped();
    }

    if (l_TransferQueueWasLocked)
    {
        unlockTransferQueue((LVKey*)0, "BBTagParts::anyStoppedTransferDefinitions");
    }

    return rc;
}

int BBTagParts::canceled(const uint32_t pContribId) {
    int rc = -1;

    int l_TransferQueueWasLocked = lockTransferQueueIfNeeded((LVKey*)0, "BBTagParts::canceled");

    if (tagParts.find(pContribId) != tagParts.end()) {
        rc = tagParts[pContribId].canceled();
    }

    if (l_TransferQueueWasLocked)
    {
        unlockTransferQueue((LVKey*)0, "BBTagParts::canceled");
    }

    return rc;
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
        LOG(bb,debug) << "taginfo: Contrib(" << it->first << ") removed from TagId(" << l_JobStr.str() << "," << pTagId.getTag() \
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
    int rc = -1;

    int l_TransferQueueWasLocked = lockTransferQueueIfNeeded((LVKey*)0, "BBTagParts::failed");

    if (tagParts.find(pContribId) != tagParts.end()) {
        rc = tagParts[pContribId].failed();
    }

    if (l_TransferQueueWasLocked)
    {
        unlockTransferQueue((LVKey*)0, "BBTagParts::failed");
    }

    return rc;
}

BBJob BBTagParts::getJob(const uint32_t pContribId) {
    BBJob l_Job = BBJob();

    int l_TransferQueueWasLocked = lockTransferQueueIfNeeded((LVKey*)0, "BBTagParts::getJob");

    if (tagParts.find(pContribId) != tagParts.end()) {
        l_Job = tagParts[pContribId].getJob();
    }

    if (l_TransferQueueWasLocked)
    {
        unlockTransferQueue((LVKey*)0, "BBTagParts::getJob");
    }

    return l_Job;
}

BBSTATUS BBTagParts::getStatus(const uint32_t pContribId) {
    BBSTATUS l_Status = BBNONE;

    int l_TransferQueueWasLocked = lockTransferQueueIfNeeded((LVKey*)0, "BBTagParts::getStatus");

    if (tagParts.find(pContribId) != tagParts.end()) {
        l_Status = tagParts[pContribId].getStatus();
    } else {
        l_Status = BBNOTREPORTED;
    }

    if (l_TransferQueueWasLocked)
    {
        unlockTransferQueue((LVKey*)0, "BBTagParts::getStatus");
    }

    return l_Status;
}

uint64_t BBTagParts::getTag(const uint32_t pContribId) {
    uint64_t l_Tag = 0;

    int l_TransferQueueWasLocked = lockTransferQueueIfNeeded((LVKey*)0, "BBTagParts::getTag");

    if (tagParts.find(pContribId) != tagParts.end()) {
        l_Tag = tagParts[pContribId].getTag();
    }

    if (l_TransferQueueWasLocked)
    {
        unlockTransferQueue((LVKey*)0, "BBTagParts::getTag");
    }

    return l_Tag;
}

size_t BBTagParts::getTotalTransferSize() {
    size_t l_TotalSize = 0;

    int l_TransferQueueWasLocked = lockTransferQueueIfNeeded((LVKey*)0, "BBTagParts::getTotalTransferSize_1");

    for (auto it = tagParts.begin(); it != tagParts.end(); ++it) {
        l_TotalSize += BBTransferDef::getTotalTransferSize(&(it->second));
    }

    if (l_TransferQueueWasLocked)
    {
        unlockTransferQueue((LVKey*)0, "BBTagParts::getTotalTransferSize_1");
    }

    return l_TotalSize;
}

size_t BBTagParts::getTotalTransferSize(const uint32_t pContribId) {
    size_t l_TotalSize = 0;

    int l_TransferQueueWasLocked = lockTransferQueueIfNeeded((LVKey*)0, "BBTagParts::getTotalTransferSize_2");

    if (tagParts.find(pContribId) != tagParts.end()) {
        l_TotalSize = BBTransferDef::getTotalTransferSize(&(tagParts[pContribId]));
    }

    if (l_TransferQueueWasLocked)
    {
        unlockTransferQueue((LVKey*)0, "BBTagParts::getTotalTransferSize_2");
    }

    return l_TotalSize;
}

size_t BBTagParts::getTotalTransferSize(const uint32_t pContribId, const uint32_t pSourceIndex) {
    size_t l_TotalSize = 0;

    int l_TransferQueueWasLocked = lockTransferQueueIfNeeded((LVKey*)0, "BBTagParts::getTotalTransferSize_3");

    if (tagParts.find(pContribId) != tagParts.end()) {
        l_TotalSize = BBTransferDef::getTotalTransferSize(&(tagParts[pContribId]), pSourceIndex);
    }

    if (l_TransferQueueWasLocked)
    {
        unlockTransferQueue((LVKey*)0, "BBTagParts::getTotalTransferSize_3");
    }

    return l_TotalSize;
}

BBTransferDef* BBTagParts::getTransferDef(const uint32_t pContribId) const {
    BBTransferDef* l_TransferDefPtr = 0;

    int l_TransferQueueWasLocked = lockTransferQueueIfNeeded((LVKey*)0, "BBTagParts::getTransferDef");

    for (auto it = tagParts.begin(); it != tagParts.end(); ++it) {
        if (it->first == pContribId) {
            l_TransferDefPtr = const_cast <BBTransferDef*> (&(it->second));
            break;
        }
    }

    if (l_TransferQueueWasLocked)
    {
        unlockTransferQueue((LVKey*)0, "BBTagParts::getTransferDef");
    }

    return l_TransferDefPtr;
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
    int rc = -1;

    int l_TransferQueueWasLocked = lockTransferQueueIfNeeded(pLVKey, "BBTagParts::setCanceled");

    for (auto it = tagParts.begin(); it != tagParts.end(); ++it) {
        if (it->first == pContribId) {
            BBTransferDef* l_TransferDef = const_cast <BBTransferDef*> (&(it->second));
            l_TransferDef->setCanceled(pLVKey, pHandle, pContribId, pValue);
            rc = 0;
        }
    }

    if (l_TransferQueueWasLocked)
    {
        unlockTransferQueue(pLVKey, "BBTagParts::setCanceled");
    }

    return rc;
}

int BBTagParts::setFailed(const LVKey* pLVKey, uint64_t pHandle, const uint32_t pContribId, const int pValue)
{
    int rc = -1;

    int l_TransferQueueWasLocked = lockTransferQueueIfNeeded(pLVKey, "BBTagParts::setFailed");

    for (auto it = tagParts.begin(); it != tagParts.end(); ++it)
    {
        if (it->first == pContribId)
        {
            BBTransferDef* l_TransferDef = const_cast <BBTransferDef*> (&(it->second));
            l_TransferDef->setFailed(pLVKey, pHandle, pValue);
            rc = 0;
        }
    }

    if (l_TransferQueueWasLocked)
    {
        unlockTransferQueue(pLVKey, "BBTagParts::setFailed");
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
