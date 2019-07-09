/*******************************************************************************
 |    BBLV_ExtentInfo.cc
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
#include "BBLV_Info.h"
#include "BBLV_ExtentInfo.h"
#include "bbserver_flightlog.h"
#include "BBTagInfo.h"
#include "BBTagInfoMap.h"
#include "BBTransferDef.h"
#include "bbwrkqmgr.h"
#include "HandleFile.h"
#include "LVUuidFile.h"

typedef std::pair<BBTransferDef*, uint32_t> FileKey;
typedef std::pair<Extent*,Extent*> FileValue;
typedef std::pair<BBTransferDef*, uint16_t> BSCFS_SortKey;
typedef std::pair<uint64_t, uint64_t> BSCFS_SortValue;


//
// BBLV_ExtentInfo class
//

//
// BBLV_ExtentInfo - Static methods
//

//
// BBLV_ExtentInfo - Non-static methods
//

int BBLV_ExtentInfo::addExtents(const LVKey* pLVKey, const uint64_t pHandle, const uint32_t pContribId, BBTagInfo* pTagInfo, BBTransferDef* pTransferDef, vector<struct stat*>* pStats) {
    int rc = 0;
    stringstream errorText;

//    LOG(bb,debug) << "Before addExtents: allExtents.capacity() = " << allExtents.capacity() << ", allExtents.size() = " << allExtents.size() << ", pTransferDef->getNumberOfExtents() = " << pTransferDef->getNumberOfExtents();

    int32_t l_PrevSourceIndex = -1;
    uint64_t l_MaxTransferSize = config.get(process_whoami+".maxTransferSize", DEFAULT_MAXIMUM_TRANSFER_SIZE);

#define PFSBLOCKBOUNDRY 65536
    l_MaxTransferSize = ((l_MaxTransferSize+(PFSBLOCKBOUNDRY-1))/PFSBLOCKBOUNDRY)*PFSBLOCKBOUNDRY;
    LOG(bb,debug) << "Maximum transfer size = 0x" << hex << uppercase << setfill('0') << setw(8) << l_MaxTransferSize << setfill(' ') << nouppercase << dec;

    if (pTransferDef->extents.size())
    {
        if (config.get(process_whoami+".bringup.dumpStatsBeforeAddingToAllExtents", 1))
        {
            for (size_t i = 0; i < pStats->size(); i=i+2)
            {
                if ((*pStats)[i])
                {
                  LOG(bb,debug) << "addExtents(): sourceindex = " << i << ", stats size = 0x" << hex << uppercase << setfill('0') << setw(8) << ((*pStats)[i])->st_size << setfill(' ') << nouppercase << dec;
                }
                else
                {
                    // NOTE: Not all source file array positions need to have a stats struct.
                    //       All source files need not participate in a restart operation...
                }
            }
        }

        Extent tmp;
        vector<Extent> l_NewList;
        size_t l_StatSize = 0;
        size_t l_AccumulatedLength = 0;
        size_t l_LastExtentLength = 0;
        for (std::vector<Extent>::size_type i = 0; i < pTransferDef->extents.size(); ++i)
        {
            if (l_PrevSourceIndex >= 0)
            {
                if (l_PrevSourceIndex != (int32_t)pTransferDef->extents[i].sourceindex)
                {
                    tmp.flags |= BBI_Last_Extent;
                }
                l_NewList.push_back(tmp);
            }

            tmp = pTransferDef->extents[i];
            tmp.handle = pHandle;
            tmp.contribid = pContribId;
            if (l_PrevSourceIndex != (int32_t)tmp.sourceindex)
            {
                tmp.flags |= BBI_First_Extent;
                l_AccumulatedLength = 0;
                l_StatSize = ((*pStats)[tmp.sourceindex])->st_size;
            }
            else
            {
                l_AccumulatedLength += l_LastExtentLength;
                if (l_AccumulatedLength > l_StatSize)
                {
                    rc = -1;
                    errorText << "addExtents(): sourceindex = " << tmp.sourceindex << ", vector of extents and their sizes is not consistent with stat size of " << l_StatSize;
                    LOG_ERROR_TEXT_RC(errorText, rc);
                    break;
                }
            }
            l_LastExtentLength = tmp.len;
            l_PrevSourceIndex = (int32_t)tmp.sourceindex;

            uint64_t l_Length = tmp.len;
            if (l_Length && (!(tmp.flags & BBI_CP_Transfer)))
            {
                uint64_t l_LBA_Start = tmp.lba.start;
                uint64_t l_Start = tmp.start;
                size_t l_AccumulatedLength2 = 0;
                bool l_PushExtent = false;
                while (l_Length)
                {
                    if (l_PushExtent)
                    {
                        l_NewList.push_back(tmp);
                        tmp = pTransferDef->extents[i];
                    }
                    l_PushExtent = true;

                    tmp.lba.start = l_LBA_Start;
                    tmp.start = l_Start;

                    if ((l_MaxTransferSize) && (l_Length > l_MaxTransferSize))
                    {
                        tmp.len = l_MaxTransferSize;
                    }
                    else
                    {
                        tmp.len = l_Length;
                    }

                    if ((tmp.start % PFSBLOCKBOUNDRY != 0) && (tmp.len > l_MaxTransferSize))     // Keep pread/pwrite alignment to PFS block boundary
                    {
                        tmp.len = PFSBLOCKBOUNDRY - (tmp.start % PFSBLOCKBOUNDRY);
                    }
#undef PFSBLOCKBOUNDRY

                    l_LBA_Start += tmp.len;
                    l_Start += tmp.len;
                    l_Length -= tmp.len;

                    if (l_AccumulatedLength + l_AccumulatedLength2 + tmp.len > l_StatSize)  // Extent file size is block granularity.  Truncate for files that aren't multiples of page size.
                    {
                        LOG(bb,info) << "addExtents(): sourceindex = " << tmp.sourceindex << ", last extent truncated from " << tmp.len << " to " << l_StatSize - (l_AccumulatedLength + l_AccumulatedLength2) \
                                     << " to not copy past end of file. Starting address is 0x" << hex << uppercase << setfill('0') << setw(16) << tmp.start << nouppercase << dec;
                        tmp.len = l_StatSize - (l_AccumulatedLength + l_AccumulatedLength2);
                        l_Length = 0;
                    }
                    tmp.lba.maxkey = tmp.lba.start + tmp.len;
                    l_AccumulatedLength2 += tmp.len;
                }
            }
        }
        tmp.flags |= BBI_Last_Extent;
        l_NewList.push_back(tmp);

        if (!rc)
        {
            // NOTE:  The final list of extents must reside in the transfer definition
            rc = pTransferDef->replaceExtentVector(&l_NewList);
            if (!rc)
            {
                if (config.get(process_whoami+".bringup.dumpExtentsBeforeAddingToAllExtents", 0)) {
                    pTransferDef->dumpExtents("info", "New vector of extents before being added to allExtents");
                }

                lockTransferQueue(pLVKey, "BBLV_ExtentInfo::addExtents");
                {
                    for(std::vector<Extent>::size_type i = 0; i < pTransferDef->extents.size(); ++i)
                    {
                        allExtents.push_back(ExtentInfo(pHandle, pContribId, &(pTransferDef->extents[i]), pTagInfo, pTransferDef));
                    }
                }
                unlockTransferQueue(pLVKey, "BBLV_ExtentInfo::addExtents");
            }
            else
            {
                rc = -1;
                errorText << "addExtents(): Could not replace vector of new extents into the transfer definition";
                LOG_ERROR_TEXT_RC(errorText, rc);
            }
        }
    }

//    LOG(bb,debug) << "After addExtents: allExtents.capacity() = " << allExtents.capacity() << ", allExtents.size() = " << allExtents.size();

    return rc;
}

void BBLV_ExtentInfo::addToInFlight(const string& pConnectionName, const LVKey* pLVKey, ExtentInfo& pExtentInfo) {
    InFlightSubKey l_SubKey = make_pair((pExtentInfo.extent)->targetindex, pExtentInfo.getTransferDef());
    InFlightKey l_Key = make_pair((pExtentInfo.extent)->lba.maxkey, l_SubKey);
    inflight.insert(make_pair(l_Key, pExtentInfo));
    LOG(bb,debug)  << "Add to inflight: tdef: " << hex << uppercase << setfill('0') \
                   << pExtentInfo.getTransferDef() << ", lba.maxkey: 0x" << pExtentInfo.getExtent()->lba.maxkey << setfill(' ') << nouppercase << dec \
                   << ", handle: " << pExtentInfo.getHandle() << ", contribid: " << pExtentInfo.getContrib() \
                   << ", srcidx: " << pExtentInfo.getSourceIndex() << ", " << inflight.size() << " extent(s) inflight";

//    dump("info", "After insert");
//    dumpInFlight("info");

    // NOTE: We used to update the metadata handle status here if this was the first extent for a transfer definition
    //       and for the first contributor, the handle status would transition from BBNOTSTARTED to BBINPROGRESS.
    //       However, it was later changed to where the handle status immediately transitions to BBINPROGRESS as
    //       part of start transfer.

    return;
}

void BBLV_ExtentInfo::dump(const char* pSev, const char* pPrefix) const {
    if (!strcmp(pSev,"debug")) {
        LOG(bb,debug) << ">>>>> Start: ExtentInfo <<<<<";
        LOG(bb,debug) << hex << uppercase << setfill('0') << "Flags: 0x" << setw(4) << flags << setfill(' ') << nouppercase << dec;
        if (minTrimAnchorExtent.lba.maxkey != 0) {
            LOG(bb,debug) << ">>>>> Start: Minimum Trim Anchor Extent <<<<<";
            LOG(bb,debug) << minTrimAnchorExtent;
            LOG(bb,debug) << ">>>>>   End: Minimum Trim Anchor Extent <<<<<";
        }
        dumpInFlight(pSev);
        dumpExtents(pSev, pPrefix);
        LOG(bb,debug) << ">>>>>   End: ExtentInfo <<<<<";
    } else if (!strcmp(pSev,"info")) {
        LOG(bb,info) << ">>>>> Start: ExtentInfo <<<<<";
        LOG(bb,info) << hex << uppercase << setfill('0') << "Flags: 0x" << setw(4) << flags << setfill(' ') << nouppercase << dec;
        if (minTrimAnchorExtent.lba.maxkey != 0) {
            LOG(bb,info) << ">>>>> Start: Minimum Trim Anchor Extent <<<<<";
            LOG(bb,info) << minTrimAnchorExtent;
            LOG(bb,info) << ">>>>>   End: Minimum Trim Anchor Extent <<<<<";
        }
        dumpInFlight(pSev);
        dumpExtents(pSev, pPrefix);
        LOG(bb,info) << ">>>>>   End: ExtentInfo <<<<<";
    }

    return;
}

void BBLV_ExtentInfo::dumpExtents(const char* pSev, const char* pPrefix) const {
    if (allExtents.size()) {
        if (!strcmp(pSev,"debug")) {
            LOG(bb,debug) << ">>>>> Start: " << (pPrefix ? pPrefix : "AllExtents") << ", " \
                          << allExtents.size() << (allExtents.size()==1 ? " extent <<<<<" : " extents <<<<<");
            for (auto& e : allExtents) {
                LOG(bb,debug) << "transdef: " << e.transferDef << hex << uppercase << setfill('0') << "  handle: 0x" << setw(16) << e.handle << setfill(' ') \
                             << nouppercase << dec << " (" << e.handle << ")  contrib: " << e.contrib;
                LOG(bb,debug) << *(e.extent);
            }
            LOG(bb,debug) << ">>>>>   End: " << (pPrefix ? pPrefix : "AllExtents") << ", " \
                          << allExtents.size() << (allExtents.size()==1 ? " extent <<<<<" : " extents <<<<<");
        } else if (!strcmp(pSev,"info")) {
            LOG(bb,info) << ">>>>> Start: " << (pPrefix ? pPrefix : "AllExtents") << ", " \
                         << allExtents.size() << (allExtents.size()==1 ? " extent <<<<<" : " extents <<<<<");
            for (auto& e : allExtents) {
                LOG(bb,info) << "transdef: " << e.transferDef << hex << uppercase << setfill('0') << "  handle: 0x" << setw(16) << e.handle << setfill(' ') \
                             << nouppercase << dec << " (" << e.handle << ")  contrib: " << e.contrib;
                LOG(bb,info) << *(e.extent);
            }
            LOG(bb,info) << ">>>>>   End: " << (pPrefix ? pPrefix : "AllExtents") << ", " \
                         << allExtents.size() << (allExtents.size()==1 ? " extent <<<<<" : " extents <<<<<");
        }
    }

    return;
}

void BBLV_ExtentInfo::dumpInFlight(const char* pSev) const {
    if (inflight.size()) {
        int i = 1;
        if (!strcmp(pSev,"debug")) {
            LOG(bb,debug) << ">>>>> Start: " << inflight.size() \
                          << (inflight.size()==1 ? " in-flight extent <<<<<" : " in-flight extents <<<<<");
            for (auto it=inflight.begin(); it!=inflight.end(); ++it) {
                LOG(bb,debug) << i << ")  lba.maxkey: 0x" << hex << uppercase << setfill('0') << setw(16) \
                              << (it->first).first << setfill(' ') << nouppercase << dec \
                              << "  transdef: " << (it->second).transferDef << "  targetindex: " \
                              << (it->second).extent->targetindex \
                              << hex << uppercase << setfill('0') << "    handle: 0x" << setw(16) << (it->second).handle \
                              << setfill(' ') << nouppercase << dec << " (" << (it->second).handle \
                              << ")  contrib: " << (it->second).contrib;
                LOG(bb,debug) << "    " << *((it->second).extent);
                ++i;
            }
            LOG(bb,debug) << ">>>>>   End: " << inflight.size() \
                          << (inflight.size()==1 ? " in-flight extent <<<<<" : " in-flight extents <<<<<");
        } else if (!strcmp(pSev,"info")) {
            LOG(bb,info) << ">>>>> Start: " << inflight.size() \
                          << (inflight.size()==1 ? " in-flight extent <<<<<" : " in-flight extents <<<<<");
            for (auto it=inflight.begin(); it!=inflight.end(); ++it) {
                LOG(bb,info) << i << ")  lba.maxkey: 0x" << hex << uppercase << setfill('0') << setw(16) \
                             << (it->first).first << setfill(' ') << nouppercase << dec \
                             << "  transdef: " << (it->second).transferDef << "  targetindex: " \
                             << (it->second).extent->targetindex \
                             << hex << uppercase << setfill('0') << "    handle: 0x" << setw(16) << (it->second).handle \
                             << setfill(' ') << nouppercase << dec << " (" << (it->second).handle \
                             << ")  contrib: " << (it->second).contrib;
                LOG(bb,info) << "    " << *((it->second).extent);
                ++i;
            }
            LOG(bb,info) << ">>>>>   End: " << inflight.size() \
                         << (inflight.size()==1 ? " in-flight extent <<<<<" : " in-flight extents <<<<<");
        }
    }

    return;
}

Extent* BBLV_ExtentInfo::getAnySourceExtent(const uint64_t pHandle, const uint32_t pContribId, const uint32_t pSourceIndex)
{
    Extent* l_Extent = 0;

    for (size_t i=0; i<allExtents.size(); ++i)
    {
        if (pHandle == allExtents[i].handle && pContribId == allExtents[i].contrib && pSourceIndex == allExtents[i].getSourceIndex())
        {
            l_Extent = allExtents[i].getExtent();
            break;
        }
    }

    return l_Extent;
}

Extent* BBLV_ExtentInfo::getMaxInFlightExtent() {
    uint64_t l_MaxLBA = 0;
    Extent* l_MaxExtent = 0;
    for (auto it=inflight.begin(); it!=inflight.end(); ++it) {
        uint64_t l_Temp = (it->first).first;
        if (l_MaxLBA < l_Temp) {
            l_MaxLBA = l_Temp;
            l_MaxExtent = it->second.extent;
        }
    }

    return l_MaxExtent;
}

Extent* BBLV_ExtentInfo::getMinimumTrimExtent() {
    Extent* l_Extent = 0;

    //  NOTE: We cannot trim back to less than the minimum trim anchor point.  Therefore,
    //        if the inflight is to the left of the min trim anchor, we can only trim to the
    //        minimum trim anchor.  Otherwise, we can only trim to the maximum inflight.
    Extent* l_Extent1 = getMinTrimAnchorExtent();
    if (l_Extent1) {
        if (l_Extent1->lba.maxkey) {
            l_Extent = l_Extent1;
            Extent* l_Extent2 = getMaxInFlightExtent();
            if (l_Extent2) {
                if (l_Extent->lba.maxkey < l_Extent2->lba.maxkey) {
                    l_Extent = l_Extent2;
                }
            }
        }
    }

    return l_Extent;
}

size_t BBLV_ExtentInfo::getNumberOfTransferDefsWithOutstandingWorkItems()
{
    map<BBTransferDef*, uint32_t> l_UniqueTransferDefs;

    for (auto& e : allExtents)
    {
        BBTransferDef* l_TransferDef = e.getTransferDef();
        if (l_UniqueTransferDefs.find(l_TransferDef) == l_UniqueTransferDefs.end())
        {
            l_UniqueTransferDefs[l_TransferDef] = 1;
        }
        else
        {
            l_UniqueTransferDefs[l_TransferDef] += 1;
        }
    }

    for (auto it=inflight.begin(); it!=inflight.end(); ++it)
    {
        BBTransferDef* l_TransferDef = (it->second).getTransferDef();
        if (l_UniqueTransferDefs.find(l_TransferDef) == l_UniqueTransferDefs.end())
        {
            l_UniqueTransferDefs[l_TransferDef] = 1;
        }
        else
        {
            l_UniqueTransferDefs[l_TransferDef] += 1;
        }
    }

    return l_UniqueTransferDefs.size();
}


int BBLV_ExtentInfo::hasCanceledExtents()
{
    int rc = 0;
    Extent* l_Extent;

    // NOTE:  Must bypass all groupkey/filekey extents with a zero value.  These are extent entries for non-transfers.
    //        Once we get past all zeroed groupkey/filekey entries, all canceled extents are next.  So the first of
    //        the non-zero groupkey/filekey extents indicates whether any canceled extents exist.
    for (size_t i=0; i<allExtents.size(); ++i)
    {
        l_Extent = allExtents[i].getExtent();
        if (l_Extent->lba.groupkey && l_Extent->lba.filekey)
        {
            rc = l_Extent->isCanceled();
            break;
        }
    }

    return rc;
}

int BBLV_ExtentInfo::moreExtentsToTransfer(const int64_t pHandle, const int32_t pContrib, uint32_t pNumberOfExpectedInFlight)
{
    int rc = 0;
    uint32_t l_NumberInFlight = 0;

    LOG(bb,debug) << "moreExtentsToTransfer: pHandle: " << pHandle << ", pContrib: " << pContrib << ", pNumberOfExpectedInFlight: " << pNumberOfExpectedInFlight;

    int l_TransferQueueLocked = lockTransferQueueIfNeeded((LVKey*)0, "BBLV_ExtentInfo::moreExtentsToTransfer");

    // Check for in-flight extents with the same handle/contrib
    if (inflight.size())
    {
        for (auto it=inflight.begin(); it!=inflight.end(); ++it)
        {
            if ((pHandle <= 0 || (uint64_t)pHandle == (it->second).handle) && (pContrib < 0 || pContrib == UNDEFINED_CONTRIBID || (uint32_t)pContrib == (it->second).contrib))
            {
                LOG(bb,debug) << "moreExtentsToTransfer: Matched handle: " << (it->second).handle << ", contribid: " << (it->second).contrib << ", extent: " << *((it->second).extent);
                if (++l_NumberInFlight > pNumberOfExpectedInFlight)
                {
                    rc = 1;
                    break;
                }
            }
        }
    }

    // Check for not yet scheduled extents with the same handle/contrib
    if ((!rc) && (allExtents.size()))
    {
        for (auto& e : allExtents)
        {
            if ((pHandle <= 0 || (uint64_t)pHandle == e.handle) && (pContrib < 0 || pContrib == UNDEFINED_CONTRIBID || (uint32_t)pContrib == e.contrib))
            {
                rc = 1;
                break;
            }
        }
    }

    if (l_TransferQueueLocked)
    {
        unlockTransferQueue((LVKey*)0, "BBLV_ExtentInfo::moreExtentsToTransfer");
    }

    return rc;
}

int BBLV_ExtentInfo::moreExtentsToTransferForFile(const int64_t pHandle, const int32_t pContrib, const uint32_t pSourceIndex, uint32_t pNumberOfExpectedInFlight, int pDumpQueuesOnValue)
{
    int rc = 0;
    uint32_t l_NumberInFlight = 0;
    bool l_DumpInFlight = false;
    bool l_DumpAllExtents = false;

    LOG(bb,debug) << "moreExtentsToTransferForFile: pHandle: " << pHandle << ", pContrib: " << pContrib << ", pSourceIndex: " << pSourceIndex << ", pNumberOfExpectedInFlight: " << pNumberOfExpectedInFlight;

    int l_TransferQueueLocked = lockTransferQueueIfNeeded((LVKey*)0, "BBLV_ExtentInfo::moreExtentsToTransferForFile");

    // Check for in-flight extents with the same handle/contrib/sourceindex
    if (inflight.size())
    {
        for (auto it=inflight.begin(); it!=inflight.end(); ++it)
        {
            if ((pHandle <= 0 || (uint64_t)pHandle == (it->second).handle) && (pContrib < 0 || pContrib == UNDEFINED_CONTRIBID || (uint32_t)pContrib == (it->second).contrib) && (pSourceIndex == (it->second).getSourceIndex()))
            {
                LOG(bb,debug) << "moreExtentsToTransferForFile: Matched handle: " << (it->second).handle << ", contribid: " << (it->second).contrib << ", source index: " << (it->second).getSourceIndex() << ", extent: " << *((it->second).extent);
                if (++l_NumberInFlight > pNumberOfExpectedInFlight)
                {
                    rc = 1;
                    l_DumpInFlight = true;
                }
            }
        }
    }

    // Check for not yet scheduled extents with the same handle/contrib/sourceindex
    if ((!rc) && (allExtents.size()))
    {
        for (auto& e : allExtents)
        {
            if ((pHandle <= 0 || (uint64_t)pHandle == e.handle) && (pContrib < 0 || pContrib == UNDEFINED_CONTRIBID || (uint32_t)pContrib == e.contrib) && (pSourceIndex == e.getSourceIndex()))
            {
                rc = 1;
                l_DumpAllExtents = true;
                break;
            }
        }
    }

    if (pDumpQueuesOnValue != DO_NOT_DUMP_QUEUES_ON_VALUE)
    {
        if (rc == pDumpQueuesOnValue)
        {
            LOG(bb,info) << "moreExtentsToTransferForFile: rc: " << rc << " pHandle: " << pHandle << " pContrib: " << pContrib << " pSourceIndex: " << pSourceIndex << " pNumberOfExpectedInFlight: " << pNumberOfExpectedInFlight << " pNumberInFlight: " << l_NumberInFlight;
            if (l_DumpInFlight)
            {
                dumpInFlight("info");
            }
            if (l_DumpAllExtents)
            {
                dumpExtents("info", "moreExtentsToTransferForFile()");
            }
        }
    }

    if (l_TransferQueueLocked)
    {
        unlockTransferQueue((LVKey*)0, "BBLV_ExtentInfo::moreExtentsToTransferForFile");
    }

    return rc;
}

int BBLV_ExtentInfo::moreInFlightExtentsForTransferDefinition(const uint64_t pHandle, const uint32_t pContrib)
{
    int rc = 0;

    LOG(bb,debug) << "moreInFlightExtentsForTransferDefinition: pHandle: " << pHandle << ", pContrib: " << pContrib;

    int l_TransferQueueLocked = lockTransferQueueIfNeeded((LVKey*)0, "BBLV_ExtentInfo::moreInFlightExtentsForTransferDefinition");

    // Check for in-flight extents with the same handle/contrib
    if (inflight.size())
    {
        for (auto it=inflight.begin(); it!=inflight.end(); ++it)
        {
            if ((pHandle == 0 || pHandle == (it->second).handle) && (pContrib == UNDEFINED_CONTRIBID || (pContrib == (it->second).contrib)))
            {
                rc = 1;
                break;
            }
        }
    }

    if (l_TransferQueueLocked)
    {
        unlockTransferQueue((LVKey*)0, "BBLV_ExtentInfo::moreInFlightExtentsForTransferDefinition");
    }

    return rc;
}

void BBLV_ExtentInfo::removeExtent(const Extent* pExtent) {
    for (auto it=allExtents.begin(); it!=allExtents.end(); ++it) {
        if (it->extent == pExtent) {
            allExtents.erase(it);
            break;
        }
    }

    return;
}

void BBLV_ExtentInfo::removeFromInFlight(const LVKey* pLVKey, ExtentInfo& pExtentInfo) {
    Extent* l_Extent = pExtentInfo.extent;

    InFlightSubKey l_SubKey = make_pair(l_Extent->targetindex, pExtentInfo.getTransferDef());
    InFlightKey l_Key = make_pair(l_Extent->lba.maxkey, l_SubKey);
    map<InFlightKey, ExtentInfo>::iterator it = inflight.find(l_Key);
    if (it!=inflight.end())
    {
        //  Only update the minimum trim anchor if we do not have one yet -or-
        //  the current minimum trim anchor is to the right of the current extent.
        if (l_Extent->isTrimAnchor() &&
            ((!minTrimAnchorExtent.lba.maxkey) ||
             (minTrimAnchorExtent.lba.maxkey > l_Extent->lba.maxkey)))
        {
            minTrimAnchorExtent = *l_Extent;
        }
        inflight.erase(l_Key);

        FL_Write6(FLWrkQMgr, RmvInFlight, "Jobid %ld, handle %ld, contribid %ld, source index %ld, extent %p, in-flight depth %ld.",
                  pExtentInfo.getTransferDef()->getJobId(), pExtentInfo.getHandle(), (uint64_t)pExtentInfo.getContrib(),
                  (uint64_t)pExtentInfo.getSourceIndex(), (uint64_t)pExtentInfo.getExtent(), (uint64_t)inflight.size());

        LOG(bb,debug)  << "Remove from inflight: tdef: " << hex << uppercase << setfill('0') \
                       << pExtentInfo.getTransferDef() << ", lba.maxkey: 0x" << pExtentInfo.getExtent()->lba.maxkey \
                       << setfill(' ') << nouppercase << dec \
                       << ", handle: " << pExtentInfo.getHandle() << ", contribid: " << pExtentInfo.getContrib() \
                       << ", srcidx: " << pExtentInfo.getSourceIndex() << ", " << inflight.size() << " extent(s) inflight";
    }

//    dumpInFlight("info");
//    dump("info", "After remove");

    return;
}

void BBLV_ExtentInfo::sendAllTransfersCompleteMsg(const string& pConnectionName, const LVKey* pLVKey) {
    txp::Msg* l_Complete = 0;
    txp::Msg::buildMsg(txp::BB_ALL_FILE_TRANSFERS_COMPLETE, l_Complete);

    Uuid lv_uuid = pLVKey->second;
    char lv_uuid_str[LENGTH_UUID_STR] = {'\0'};
    lv_uuid.copyTo(lv_uuid_str);

    LOG(bb,info) << "->bbproxy: All transfers complete:  " << *pLVKey << ".";
    LOG(bb,info) << "           See individual handle/contributor/file messages for additional status.";

    // NOTE:  The char array is copied to heap by addAttribute and the storage for
    //        the logical volume uuid attribute is owned by the message facility.
    //        Our copy can then go out of scope...
    l_Complete->addAttribute(txp::uuid, lv_uuid_str, sizeof(lv_uuid_str), txp::COPY_TO_HEAP);

    // Send the all transfers complete message

    try
    {
        int rc = sendMessage(pConnectionName,l_Complete);
        if (rc) LOG(bb,info) << "l_Complete sendMessage rc="<<rc<<" name="<<pConnectionName<<" @"<<  __func__;
    }
    catch(exception& e)
    {
        LOG(bb,warning) << "Exception thrown when attempting to send completion for "<< *pLVKey << ": " << e.what();
        assert(strlen(e.what())==0);
    }

    delete l_Complete;

    return;
}

void BBLV_ExtentInfo::setAllContribsReported(const LVKey* pLVKey, const int pValue)
{
    if (pValue) {
        LOG(bb,info) << "All contributors reported for " << *pLVKey;
    }

    if ((((flags & BBTI_All_Contribs_Reported) == 0) && pValue) || ((flags & BBTI_All_Contribs_Reported) && (!pValue)))
    {
        LOG(bb,info) << "BBLV_ExtentInfo::setAllContribsReported(): For " << *pLVKey \
                     << " -> Changing from: " << ((flags & BBTI_All_Contribs_Reported) ? "true" : "false") << " to " << (pValue ? "true" : "false");
    }
    SET_FLAG_AND_RETURN(BBTI_All_Contribs_Reported, pValue);
}

void BBLV_ExtentInfo::setAllExtentsTransferred(const string& pConnectionName, const LVKey* pLVKey, const int pValue)
{
    if (pValue && (!allExtentsTransferred())) {
        LOG(bb,debug) << "Processing complete for " << *pLVKey;
        sendAllTransfersCompleteMsg(pConnectionName, pLVKey);
    }

    if ((((flags & BBTD_All_Extents_Transferred) == 0) && pValue) || ((flags & BBTD_All_Extents_Transferred) && (!pValue)))
    {
        LOG(bb,info) << "BBLV_ExtentInfo::setAllExtentsTransferred(): For " << *pLVKey \
                     << " -> Changing from: " << ((flags & BBTD_All_Extents_Transferred) ? "true" : "false") << " to " << (pValue ? "true" : "false");
    }
    SET_FLAG_AND_RETURN(BBTD_All_Extents_Transferred, pValue);
}

void BBLV_ExtentInfo::setAllExtentsTransferred(const LVKey* pLVKey, const uint64_t pHandle, const uint32_t pContribId, BBTransferDef* pTransferDef, const int pValue)
{
    return pTransferDef->setAllExtentsTransferred(pLVKey, pHandle, pContribId, pValue);
}

void BBLV_ExtentInfo::setStageOutEnded(const LVKey* pLVKey, const uint64_t pJobId, const int pValue)
{
    if ((((flags & BBLV_Stage_Out_End) == 0) && pValue) || ((flags & BBLV_Stage_Out_End) && (!pValue)))
    {
        LOG(bb,info) << "BBLV_ExtentInfo::setStageOutEnded(): For " << *pLVKey \
                     << " -> Changing from: " << ((flags & BBLV_Stage_Out_End) ? "true" : "false") << " to " << (pValue ? "true" : "false");
    }
    SET_FLAG(BBLV_Stage_Out_End, pValue);

    int rc = LVUuidFile::update_xbbServerLVUuidFile(pLVKey, pJobId, BBLV_Stage_Out_End, pValue);
    if (rc)
    {
        if (rc != -2)
        {
            LOG(bb,error) << "BBLV_ExtentInfo::setStageOutEnded():  Failure when attempting to update the cross bbServer LVUuid file for LVKey " << *pLVKey << ", jobid " << pJobId;
        }
        else
        {
            // NOTE:  Tolerate that the cross-bbServer metadata is already removed for this job...
        }
    }

    return;
}

void BBLV_ExtentInfo::setStageOutEndedComplete(const LVKey* pLVKey, const uint64_t pJobId, const int pValue)
{
    if ((((flags & BBLV_Stage_Out_End_Complete) == 0) && pValue) || ((flags & BBLV_Stage_Out_End_Complete) && (!pValue)))
    {
        LOG(bb,debug) << "BBLV_ExtentInfo::setStageOutEndedComplete(): For " << *pLVKey \
                      << " -> Changing from: " << ((flags & BBLV_Stage_Out_End_Complete) ? "true" : "false") << " to " << (pValue ? "true" : "false");
    }
    SET_FLAG(BBLV_Stage_Out_End_Complete, pValue);

    int rc = LVUuidFile::update_xbbServerLVUuidFile(pLVKey, pJobId, BBLV_Stage_Out_End_Complete, pValue);
    if (rc)
    {
        if (rc != -2)
        {
            LOG(bb,error) << "BBLV_ExtentInfo::setStageOutEndedComplete():  Failure when attempting to update the cross bbServer LVUuid file for LVKey " << *pLVKey << ", jobid " << pJobId;
        }
        else
        {
            // NOTE:  Tolerate that the cross-bbServer metadata is already removed for this job...
        }
    }

    return;
}

void BBLV_ExtentInfo::setStageOutStarted(const LVKey* pLVKey, const uint64_t pJobId, const int pValue)
{
    if ((((flags & BBLV_Stage_Out_Start) == 0) && pValue) || ((flags & BBLV_Stage_Out_Start) && (!pValue)))
    {
        LOG(bb,info) << "BBLV_ExtentInfo::setStageOutStarted(): For " << *pLVKey \
                     << " -> Changing from: " << ((flags & BBLV_Stage_Out_Start) ? "true" : "false") << " to " << (pValue ? "true" : "false");
    }
    SET_FLAG(BBLV_Stage_Out_Start, pValue);

    if (LVUuidFile::update_xbbServerLVUuidFile(pLVKey, pJobId, BBLV_Stage_Out_Start, pValue))
    {
        LOG(bb,error) << "BBLV_ExtentInfo::setStageOutStarted():  Failure when attempting to update the cross bbServer LVUuid file for LVKey " << *pLVKey << ", jobid " << pJobId;
    }

    return;
}

int BBLV_ExtentInfo::setSuspended(const LVKey* pLVKey, const string& pHostName, const uint64_t pJobId, const int pValue)
{
    int rc = 0;

    if (!stageOutStarted())
    {
        rc = wrkqmgr.setSuspended(pLVKey, pValue);
        switch (rc)
        {
            // NOTE: Even if the work queue had the suspend bit on (rc=2), we still unconditionally set the suspend
            //       bit in BBLV_ExtentInfo flags as in the restart case when registering LVKeys from the "old server"
            //       the work queues are constructed with the suspend bit on.
            case 0:
            case 2:
            {
                if ((((flags & BBLV_Suspended) == 0) && pValue) || ((flags & BBLV_Suspended) && (!pValue)))
                {
                    LOG(bb,info) << "BBLV_Info::setSuspended(): For hostname " << pHostName << ", " << *pLVKey << ", jobid " << pJobId \
                                 << " -> Changing from: " << ((flags & BBLV_Suspended) ? "true" : "false") << " to " << (pValue ? "true" : "false");
                }
                SET_FLAG(BBLV_Suspended, pValue);
            }
            break;

            case -2:
            {
                // NOTE: For failover cases, it is possible for a setSuspended() request to be issued to this bbServer before any request
                //       has 'used' the LVKey and required the work queue to be present.  We simply tolerate the condition...
                SET_FLAG(BBLV_Suspended, pValue);

                string l_Temp = "resume";
                if (pValue)
                {
                    // Connection being suspended
                    l_Temp = "suspend";
                }
                LOG(bb,info) << "BBLV_Info::setSuspended(): For hostname " << pHostName << ", jobid " << pJobId \
                             << ", work queue not present for " << *pLVKey << ". Tolerated condition for a " << l_Temp << " operation.";
            }
            break;

            default:
                LOG(bb,info) << "BBLV_Info::setSuspended(): Unexpected return code " << rc \
                             << " received for hostname " << pHostName << ", jobid " << pJobId << ", " << *pLVKey \
                             << " when attempting the suspend or resume operation on the work queue.";
                rc = -1;
                break;
        }
    }
    else
    {
        // Stageout end processing has started.  Therefore, the ability to do anything using this LVKey
        // will soon be, or has already, been removed.  (i.e. the local cache of data is being/or has been
        // torn down...)  Therefore, the only meaningful thing left to be done is remove job information.
        // Return an error message.
        rc = -1;
        LOG(bb,error) << "BBLV_Info::setSuspended(): For hostname " << pHostName << ", jobid " << pJobId \
                      << ", the remove logical volume request has been run, or is currently running" \
                      << " for " << *pLVKey << ". Suspend or resume operations are not allowed for this environment.";
    }

    return rc;
}

int BBLV_ExtentInfo::sortExtents(const LVKey* pLVKey, size_t& pNumberOfNewExtentsCanceled, uint64_t* pHandle, uint32_t* pContribId)
{
    const uint64_t l_CanceledGroupKey = 1;
    const uint64_t l_CanceledFileKey = 1;
    const uint64_t l_NonCanceledGroupKey = 2;
    const uint64_t l_NonCanceledFileKey = 1;
    int rc = 0;

    pNumberOfNewExtentsCanceled = 0;

    int l_TransferQueueLocked = lockTransferQueueIfNeeded(pLVKey, "sort extents");
    {
        if ((pHandle && pContribId) || resizeLogicalVolumeDuringStageOut() || BSCFS_InRequest())
        {
            try
            {
                if (allExtents.size())
                {
                    // Extents exist...

                    // NOTE: The way the groupkeys and filekeys are set, normal files will
                    //       always sort in front of all BSCFS files.  This should not be a
                    //       problem as ther are no plans to mix normal and BSCFS extents
                    //       for a given work queue.
                    uint64_t l_BSCFS_GroupKey = 2;
                    Extent* l_ExtentPtr = 0;
                    map<BSCFS_SortKey, uint64_t> BSCFS_SortMap;
                    map<BSCFS_SortKey, uint64_t>::iterator it_BSCFS_SortMap;

                    // When entering, we are assured there are no in-flight extents for this LVKey

                    size_t l_AlreadyMarkedAsCanceled = 0;
                    size_t l_NotMarkedAsCanceled = 0;
                    size_t l_NotMarkedAsCanceledBSCFS = 0;
                    bool l_CheckForNewlyCanceledExtents = ((pHandle && pContribId) ? true : false);

                    LOG(bb,debug) << "sortExtents(): For " << *pLVKey << ", " << allExtents.size() << " extent(s) on the queue upon entry to sort";

                    for (size_t i=0; i<allExtents.size(); ++i)
                    {
                        Extent* l_Extent = allExtents[i].extent;

                        // Reset the extent for sort processing
                        l_Extent->resetForSort();

                        if (l_Extent->flags & BBI_TargetSSDSSD || (!l_Extent->len))
                        {
                            // Dummy extent for local SSD cp or zero length file.
                            // Leave the group key and file key as zero so it sorts
                            // to the front.

                            ++l_NotMarkedAsCanceled;
                        }
                        else if (l_Extent->flags & BBTD_Canceled)
                        {
                            // Extent is already marked as being canceled
                            l_Extent->lba.groupkey = l_CanceledGroupKey;
                            l_Extent->lba.filekey = l_CanceledFileKey;

                            ++l_AlreadyMarkedAsCanceled;
                        }
                        else if (l_CheckForNewlyCanceledExtents &&
                                 (*pHandle == l_Extent->getHandle() && (*pContribId == UNDEFINED_CONTRIBID || *pContribId == l_Extent->getContribId())))
                        {
                            // This is an extent to be newly marked as canceled
                            l_Extent->lba.groupkey = l_CanceledGroupKey;
                            l_Extent->lba.filekey = l_CanceledFileKey;

                            l_Extent->setCanceled();
                            ++pNumberOfNewExtentsCanceled;
                        }
                        else if (l_Extent->isRegularExtent())
                        {
                            // This is a non-canceled regular extent for transfer
                            l_Extent->lba.groupkey = l_NonCanceledGroupKey;
                            l_Extent->lba.filekey = l_NonCanceledFileKey;

                            //  Since we know this is the maximum LBA of those extents left, we can use
                            //  this extent as a location that we can trim the logical volume back to during
                            //  stage out.
                            //  NOTE: This value will only be used if we sort in descending LBA order.
                            //  NOTE: Can only possibly be used if this is an extent for stageout...
                            l_Extent->setTrimAnchor();

                            ++l_NotMarkedAsCanceled;
                        }
                        else
                        {
                            // This is a non-canceled BSCFS extent
                            uint64_t l_GroupKey;
                            BSCFS_SortKey l_Key = make_pair(allExtents[i].getTransferDef(), l_Extent->getBundleID());
                            it_BSCFS_SortMap = BSCFS_SortMap.find(l_Key);
                            if (it_BSCFS_SortMap != BSCFS_SortMap.end())
                            {
                                // Use the group key as found in the map
                                l_GroupKey = BSCFS_SortMap[l_Key];
                            }
                            else
                            {
                                // Increment the group key
                                l_GroupKey = ++l_BSCFS_GroupKey;
                                // Insert the new key
                                BSCFS_SortMap[l_Key] = l_GroupKey;
                            }
                            l_Extent->lba.groupkey = l_GroupKey;
                            l_Extent->lba.filekey = l_Extent->getTransferOrder();

                            // NOTE: No trim anchor needs to be set as we do not resize the logical volume if BSCFS is in the request.

                            ++l_NotMarkedAsCanceledBSCFS;
                        }
                    }

                    if (config.get(resolveServerConfigKey("bringup.dumpExtentsBeforeSort"), 0))
                    {
                        dumpExtents("info", "Before sort/copy processing");
                    }

                    // Perform the sort of the extents vector...
                    if (!resizeLogicalVolumeDuringStageOut())
                    {
                        LOG(bb,debug) << "sortExtents(): For " << *pLVKey << ", start sorting " << allExtents.size() << " extent(s) for the LVKey, positive SSD stride";
                        sort(allExtents.begin(), allExtents.end(), compareOpPositiveStride);
                    }
                    else
                    {
                        LOG(bb,debug) << "sortExtents(): For " << *pLVKey << ", start sorting " << allExtents.size() << " extent(s) for the LVKey, negative SSD stride";
                        sort(allExtents.begin(), allExtents.end(), compareOpNegativeStride);
                    }
                    LOG(bb,debug) << "sortExtents(): For " << *pLVKey << ", end sorting " << allExtents.size() << " extent(s) for the LVKey";

                    // Find the first/last extent for each (transfer definition,sourceindex)
                    BBTransferDef* l_TransferDef;
                    FileValue l_Value;
                    map<FileKey, FileValue> l_FileMap;
                    map<FileKey, FileValue>::iterator it_FileMap;
                    for (size_t i=0; i<allExtents.size(); ++i)
                    {
                        l_ExtentPtr = allExtents[i].getExtent();
                        l_TransferDef = allExtents[i].getTransferDef();
                        // Now bookkeep the first/last per (BBTransferDef,sourceindex)
                        FileKey l_Key = make_pair(l_TransferDef, l_ExtentPtr->getSourceIndex());
                        it_FileMap = l_FileMap.find(l_Key);
                        if (it_FileMap != l_FileMap.end())
                        {
                            // Maintain the existing entry for this (transfer definition, sourceindex)
                            l_Value = make_pair((it_FileMap->second).first, l_ExtentPtr);
                        }
                        else
                        {
                            // Create a new entry for this (transfer definition, sourceindex)
                            l_Value = make_pair(l_ExtentPtr, l_ExtentPtr);
                        }
                        l_FileMap[l_Key] = l_Value;
                    }

                    // Set the first/last values in the sorted vector
                    for (size_t i=0; i<allExtents.size(); ++i)
                    {
                        l_ExtentPtr = allExtents[i].getExtent();
                        l_TransferDef = allExtents[i].getTransferDef();
                        // Now find the entry in the map giving the first/last extents for this (transfer definition,sourceindex)
                        FileKey l_Key = make_pair(l_TransferDef, l_ExtentPtr->getSourceIndex());
                        it_FileMap = l_FileMap.find(l_Key);
                        if ((it_FileMap->second).first == l_ExtentPtr)
                        {
                            l_ExtentPtr->flags |= BBI_First_Extent;
                        }
                        if ((it_FileMap->second).second == l_ExtentPtr)
                        {
                            l_ExtentPtr->flags |= BBI_Last_Extent;
                        }
                    }

                    LOG(bb,info) << "sortExtents(): For " << *pLVKey << ", " << l_AlreadyMarkedAsCanceled << " extent(s) were already marked as canceled, " \
                                 << pNumberOfNewExtentsCanceled << " additional extent(s) were newly marked as canceled, " \
                                 << l_NotMarkedAsCanceled << " regular and " << l_NotMarkedAsCanceledBSCFS << " BSCFS extent(s) remain to be transfered";
                }
                else
                {
                    LOG(bb,info) << "sortExtents(): For " << *pLVKey << ", no extents on the work queue for the LVKey";
                }
            }
            catch(ExceptionBailout& e) { }
            catch(exception& e)
            {
                rc = -1;
                LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
            }

#if BBSERVER
            // Cause the ResizeSSD_Timer to pop on the first transfered extent.
            // NOTE:  We set it whether or not we are resizing the SSD during stageout.
            ResizeSSD_Timer.forcePop();
#endif
            LOG(bb,debug) << "sortExtents(): For " << *pLVKey << ", " << allExtents.size() << " extent(s) on the queue when exiting sort.";

            if (config.get(resolveServerConfigKey("bringup.dumpExtentsAfterSort"), 0))
            {
                dumpExtents("info", "After sort/copy processing");
            }
        }
    }
    if (l_TransferQueueLocked)
    {
        unlockTransferQueue(pLVKey, "sort extents");
    }

    return rc;
}

void BBLV_ExtentInfo::updateTransferStatus(const string& pConnectionName, const LVKey* pLVKey, uint32_t pNumberOfExpectedInFlight)
{
    if (stageOutStarted()) {
        if (allContribsReported()) {
            if (!moreExtentsToTransfer((int64_t)(-1), (int32_t)(-1), pNumberOfExpectedInFlight)) {
                setAllExtentsTransferred(pConnectionName, pLVKey);
            }
        }
    }

    return;
}

void BBLV_ExtentInfo::updateTransferStatus(const LVKey* pLVKey, ExtentInfo& pExtentInfo, BBTransferDef* pTransferDef, int& pNewStatus, int& pExtentsRemainForSourceIndex, uint32_t pNumberOfExpectedInFlight)
{
    pNewStatus = 0;

    pExtentsRemainForSourceIndex = moreExtentsToTransferForFile((int64_t)pExtentInfo.handle, (int32_t)pExtentInfo.contrib, pExtentInfo.getSourceIndex(), pNumberOfExpectedInFlight);
    if (!moreExtentsToTransfer((int64_t)pExtentInfo.handle, (int32_t)pExtentInfo.contrib, pNumberOfExpectedInFlight)) {
        pNewStatus = 1;
        setAllExtentsTransferred(pLVKey, pExtentInfo.getHandle(), pExtentInfo.getContrib(), pTransferDef);
    }
    LOG(bb,debug) << "updateTransferStatus(): tdef " << hex << uppercase << setfill('0') \
                  << pTransferDef << setfill(' ') << nouppercase << dec \
                  << ", handle " << pExtentInfo.getHandle() << ", contribid " << pExtentInfo.getContrib() \
                  << ", srcidx " << pExtentInfo.getSourceIndex() \
                  << ", new status=" << pNewStatus << ", extents remain for srcidx=" << pExtentsRemainForSourceIndex;

    return;
}
