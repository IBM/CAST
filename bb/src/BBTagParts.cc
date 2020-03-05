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

#include <map>

#include "bbinternal.h"
#include "bbserver_flightlog.h"
#include "BBLV_Info.h"
#include "BBTagInfo.h"
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
    int l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded(pLVKey, "BBTagParts::addTransferDef");

    tagParts[pContribId] = *pTransferDef;
    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata(pLVKey, "BBTagParts::addTransferDef");
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

int BBTagParts::allExtentsTransferred(BBTagInfo* pTagInfo, const uint32_t pContribId) {
    int rc = -1;

    int l_LocalMetadataWasLocked = 0;
    if (pTagInfo->localMetadataLockRequired())
    {
        l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded((LVKey*)0, "BBTagParts::allExtentsTransferred");
    }

    if (tagParts.find(pContribId) != tagParts.end()) {
        rc = tagParts[pContribId].allExtentsTransferred();
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata((LVKey*)0, "BBTagParts::allExtentsTransferred");
    }

    return rc;
}

int BBTagParts::anyCanceledTransferDefinitions(BBTagInfo* pTagInfo) {
    int rc = 0;

    int l_LocalMetadataWasLocked = 0;
    if (pTagInfo->localMetadataLockRequired())
    {
        l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded((LVKey*)0, "BBTagParts::anyCanceledTransferDefinitions");
    }

    // NOTE: It is only considered canceled if the stopped bit is also off
    for (auto it = tagParts.begin(); (!rc) && it != tagParts.end(); ++it) {
        rc = ((!(it->second).stopped()) && (it->second).canceled());
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata((LVKey*)0, "BBTagParts::anyCanceledTransferDefinitions");
    }

    return rc;
}

int BBTagParts::anyFailedTransferDefinitions(BBTagInfo* pTagInfo) {
    int rc = 0;

    int l_LocalMetadataWasLocked = 0;
    if (pTagInfo->localMetadataLockRequired())
    {
        l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded((LVKey*)0, "BBTagParts::anyFailedTransferDefinitions");
    }

    for (auto it = tagParts.begin(); (!rc) && it != tagParts.end(); ++it) {
        rc = (it->second).failed();
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata((LVKey*)0, "BBTagParts::anyFailedTransferDefinitions");
    }

    return rc;
}

int BBTagParts::anyStoppedTransferDefinitions(BBTagInfo* pTagInfo) {
    int rc = 0;

    int l_LocalMetadataWasLocked = 0;
    if (pTagInfo->localMetadataLockRequired())
    {
        l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded((LVKey*)0, "BBTagParts::anyStoppedTransferDefinitions");
    }

    for (auto it = tagParts.begin(); (!rc) && it != tagParts.end(); ++it) {
        rc = (it->second).stopped();
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata((LVKey*)0, "BBTagParts::anyStoppedTransferDefinitions");
    }

    return rc;
}

int BBTagParts::canceled(BBTagInfo* pTagInfo, const uint32_t pContribId) {
    int rc = -1;

    int l_LocalMetadataWasLocked = 0;
    if (pTagInfo->localMetadataLockRequired())
    {
        l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded((LVKey*)0, "BBTagParts::canceled");
    }

    if (tagParts.find(pContribId) != tagParts.end()) {
        rc = tagParts[pContribId].canceled();
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata((LVKey*)0, "BBTagParts::canceled");
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

void BBTagParts::cleanUpContribId(const LVKey* pLVKey, const BBTagID& pTagId, const uint64_t pHandle, const uint32_t pContribId)
{
    try
    {
        map<uint32_t, BBTransferDef>::iterator it;
        it = tagParts.find(pContribId);
        if (it != tagParts.end())
        {
            it->second.cleanUp();

            stringstream l_JobStr;
            pTagId.getJob().getStr(l_JobStr);
            LOG(bb,info) << "taginfo: ContribId " << pContribId << ", TagId(" << l_JobStr.str() \
                         << "," << pTagId.getTag() << ") with handle 0x" \
                         << hex << uppercase << setfill('0') << setw(16) << pHandle \
                         << setfill(' ') << nouppercase << dec << " (" << pHandle \
                         << ") removed from " << *pLVKey;

            tagParts.erase(it);
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        // Tolerate everything...
    }

    return;
}

void BBTagParts::dump(const char* pSev)
{
    if (wrkqmgr.checkLoggingLevel(pSev))
    {
        int l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded((LVKey*)0, "BBTagParts::dump");

        if (tagParts.size())
        {
            if (!strcmp(pSev,"debug"))
            {
                LOG(bb,debug) << ">>>>> Start: " << tagParts.size() \
                              << (tagParts.size()==1 ? " transfer definition <<<<<" : " transfer definitions <<<<<");
                for (auto& part : tagParts)
                {
                    LOG(bb,debug) << "Contrib: " << part.first;
                    part.second.dump(pSev);
                }
                LOG(bb,debug) << ">>>>>   End: " << tagParts.size() \
                              << (tagParts.size()==1 ? " transfer definition <<<<<" : " transfer definitions <<<<<");
            }
            else if (!strcmp(pSev,"info"))
            {
                LOG(bb,info) << ">>>>> Start: " << tagParts.size() \
                             << (tagParts.size()==1 ? " transfer definition <<<<<" : " transfer definitions <<<<<");
                for (auto& part : tagParts)
                {
                    LOG(bb,info) << "Contrib: " << part.first;
                    part.second.dump(pSev);
                }
                LOG(bb,info) << ">>>>>   End: " << tagParts.size() \
                             << (tagParts.size()==1 ? " transfer definition <<<<<" : " transfer definitions <<<<<");
            }
        }

        if (l_LocalMetadataWasLocked)
        {
            unlockLocalMetadata((LVKey*)0, "BBTagParts::dump");
        }
    }

    return;
}

int BBTagParts::failed(BBTagInfo* pTagInfo, const uint32_t pContribId) {
    int rc = -1;

    int l_LocalMetadataWasLocked = 0;
    if (pTagInfo->localMetadataLockRequired())
    {
        l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded((LVKey*)0, "BBTagParts::failed");
    }

    if (tagParts.find(pContribId) != tagParts.end()) {
        rc = tagParts[pContribId].failed();
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata((LVKey*)0, "BBTagParts::failed");
    }

    return rc;
}

BBJob BBTagParts::getJob(BBTagInfo* pTagInfo, const uint32_t pContribId) {
    BBJob l_Job = BBJob();

    int l_LocalMetadataWasLocked = 0;
    if (pTagInfo->localMetadataLockRequired())
    {
        l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded((LVKey*)0, "BBTagParts::getJob");
    }

    if (tagParts.find(pContribId) != tagParts.end()) {
        l_Job = tagParts[pContribId].getJob();
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata((LVKey*)0, "BBTagParts::getJob");
    }

    return l_Job;
}

BBSTATUS BBTagParts::getStatus(BBTagInfo* pTagInfo, const uint32_t pContribId) {
    BBSTATUS l_Status = BBNONE;

    int l_LocalMetadataWasLocked = 0;
    if (pTagInfo->localMetadataLockRequired())
    {
        l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded((LVKey*)0, "BBTagParts::getStatus");
    }

    if (tagParts.find(pContribId) != tagParts.end()) {
        l_Status = tagParts[pContribId].getStatus();
    } else {
        l_Status = BBNOTREPORTED;
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata((LVKey*)0, "BBTagParts::getStatus");
    }

    return l_Status;
}

uint64_t BBTagParts::getTag(BBTagInfo* pTagInfo, const uint32_t pContribId) {
    uint64_t l_Tag = 0;

    int l_LocalMetadataWasLocked = 0;
    if (pTagInfo->localMetadataLockRequired())
    {
        l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded((LVKey*)0, "BBTagParts::getTag");
    }

    if (tagParts.find(pContribId) != tagParts.end()) {
        l_Tag = tagParts[pContribId].getTag();
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata((LVKey*)0, "BBTagParts::getTag");
    }

    return l_Tag;
}

size_t BBTagParts::getTotalTransferSize(BBTagInfo* pTagInfo) {
    size_t l_TotalSize = 0;

    int l_LocalMetadataWasLocked = 0;
    if (pTagInfo->localMetadataLockRequired())
    {
        l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded((LVKey*)0, "BBTagParts::getTotalTransferSize_1");
    }

    for (auto it = tagParts.begin(); it != tagParts.end(); ++it) {
        l_TotalSize += BBTransferDef::getTotalTransferSize(&(it->second));
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata((LVKey*)0, "BBTagParts::getTotalTransferSize_1");
    }

    return l_TotalSize;
}

size_t BBTagParts::getTotalTransferSize(BBTagInfo* pTagInfo, const uint32_t pContribId) {
    size_t l_TotalSize = 0;

    int l_LocalMetadataWasLocked = 0;
    if (pTagInfo->localMetadataLockRequired())
    {
        l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded((LVKey*)0, "BBTagParts::getTotalTransferSize_2");
    }

    if (tagParts.find(pContribId) != tagParts.end()) {
        l_TotalSize = BBTransferDef::getTotalTransferSize(&(tagParts[pContribId]));
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata((LVKey*)0, "BBTagParts::getTotalTransferSize_2");
    }

    return l_TotalSize;
}

size_t BBTagParts::getTotalTransferSize(BBTagInfo* pTagInfo, const uint32_t pContribId, const uint32_t pSourceIndex) {
    size_t l_TotalSize = 0;

    int l_LocalMetadataWasLocked = 0;
    if (pTagInfo->localMetadataLockRequired())
    {
        l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded((LVKey*)0, "BBTagParts::getTotalTransferSize_3");
    }

    if (tagParts.find(pContribId) != tagParts.end()) {
        l_TotalSize = BBTransferDef::getTotalTransferSize(&(tagParts[pContribId]), pSourceIndex);
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata((LVKey*)0, "BBTagParts::getTotalTransferSize_3");
    }

    return l_TotalSize;
}

BBTransferDef* BBTagParts::getTransferDef(BBTagInfo* pTagInfo, const uint32_t pContribId) const {
    BBTransferDef* l_TransferDefPtr = 0;

    int l_TransferQueueWasUnlocked = 0;
    int l_LocalMetadataWasLocked = 0;
    if (pTagInfo->localMetadataLockRequired())
    {
        l_TransferQueueWasUnlocked = unlockTransferQueueIfNeeded((LVKey*)0, "BBTagParts::getTransferDef");
        l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded((LVKey*)0, "BBTagParts::getTransferDef");
    }

    for (auto it = tagParts.begin(); it != tagParts.end(); ++it) {
        if (it->first == pContribId) {
            l_TransferDefPtr = const_cast <BBTransferDef*> (&(it->second));
            break;
        }
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata((LVKey*)0, "BBTagParts::getTransferDef");
    }

    if (l_TransferQueueWasUnlocked)
    {
        lockTransferQueue((LVKey*)0, "BBTagParts::getTransferDef");
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

int BBTagParts::setCanceled(const LVKey* pLVKey, BBTagInfo* pTagInfo, uint64_t pHandle, const uint32_t pContribId, const int pValue) {
    int rc = -1;

    int l_LocalMetadataWasLocked = 0;
    if (pTagInfo->localMetadataLockRequired())
    {
        l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded(pLVKey, "BBTagParts::setCanceled");
    }

    for (auto it = tagParts.begin(); it != tagParts.end(); ++it) {
        if (it->first == pContribId) {
            BBTransferDef* l_TransferDef = const_cast <BBTransferDef*> (&(it->second));
            l_TransferDef->setCanceled(pLVKey, pHandle, pContribId, pValue);
            rc = 0;
        }
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata(pLVKey, "BBTagParts::setCanceled");
    }

    return rc;
}

int BBTagParts::setFailed(const LVKey* pLVKey, BBTagInfo* pTagInfo, uint64_t pHandle, const uint32_t pContribId, const int pValue)
{
    int rc = -1;

    int l_LocalMetadataWasLocked = 0;
    if (pTagInfo->localMetadataLockRequired())
    {
        l_LocalMetadataWasLocked = lockLocalMetadataIfNeeded(pLVKey, "BBTagParts::setFailed");
    }

    for (auto it = tagParts.begin(); it != tagParts.end(); ++it)
    {
        if (it->first == pContribId)
        {
            BBTransferDef* l_TransferDef = const_cast <BBTransferDef*> (&(it->second));
            l_TransferDef->setFailed(pLVKey, pHandle, pValue);
            rc = 0;
        }
    }

    if (l_LocalMetadataWasLocked)
    {
        unlockLocalMetadata(pLVKey, "BBTagParts::setFailed");
    }

    return rc;
}

int BBTagParts::stopTransfer(const LVKey* pLVKey, const string& pHostName, const string& pCN_HostName, BBLV_Info* pLV_Info, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId, LOCAL_METADATA_RELEASED& pLockWasReleased)
{
    int rc = 0;

    // NOTE: pLockWasReleased intentionally not initialized

    for (auto it = tagParts.begin(); ((!rc) && it != tagParts.end()); ++it)
    {
        // NOTE: Check for an already stopped transfer definition.  If so, then this transfer definition
        //       IS NOT associated with the correct LVKey.  This LVKey has already been failed over...
        if ((pContribId == UNDEFINED_CONTRIBID || it->first == pContribId) && (!(it->second).stopped()))
        {
            BBTransferDef* l_TransferDef = const_cast <BBTransferDef*> (&(it->second));
            rc = l_TransferDef->stopTransfer(pLVKey, pHostName, pCN_HostName, pJobId, pJobStepId, pHandle, it->first, pLockWasReleased);
        }
    }

    return rc;
}
