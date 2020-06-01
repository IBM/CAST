/*******************************************************************************
 |    BBLV_ExtentInfo.h
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

#ifndef BB_BBLVEXTENTINFO_H_
#define BB_BBLVEXTENTINFO_H_

#include <map>

#include "Connex.h"
#include "ExtentInfo.h"
#include "LVKey.h"

/*******************************************************************************
 | Forward declarations
 *******************************************************************************/
class BBTagID;
class BBLV_Info;
class BBTransferDef;
class Extent;

/*******************************************************************************
 | Type definitions
 *******************************************************************************/
typedef std::pair<uint32_t, BBTransferDef*> InFlightSubKey;     // First is targetindex
typedef std::pair<uint64_t, InFlightSubKey> InFlightKey;        // First is lba.maxkey

/**
 * \class BBLV_ExtentInfo
 * Defines the information related to all extents when transferring by_extent
 */
class BBLV_ExtentInfo
{
  public:
    BBLV_ExtentInfo() :
        flags(0) {
        allExtents = vector<ExtentInfo>();
        minTrimAnchorExtent = Extent();
        BB_GetTime(processingTime);
        inflight = map<InFlightKey, ExtentInfo>();
    }

    BBLV_ExtentInfo(const BBLV_ExtentInfo& src) {
        flags = src.flags;
        allExtents = src.allExtents;
        minTrimAnchorExtent = src.minTrimAnchorExtent;
        BB_GetTime(processingTime);
        inflight = src.inflight;
    }

    // Compare operators for sorting extents

    // Positive stride
    //
    // This compare operator is used when we are not shrinking a logical volume during stage out.
    //
    // For BSCFS files, extents for a given file are kept together as a group, in arrival order
    // as inserted into the extent vector by buildExtents().  The sort also ensures that the index
    // extents are transferred before the related data extents.  This aids in simplifying the merge
    // of extent data from the logs on the server.
    //
    // For non-BSCFS files, extents are all sorted together in ascending LBA order.
    static struct {
        inline bool operator()(ExtentInfo a, ExtentInfo b) {
            Extent* aa = a.extent;
            Extent* bb = b.extent;
            return (bb->lba.groupkey != aa->lba.groupkey ? bb->lba.groupkey > aa->lba.groupkey :
                    bb->lba.filekey != aa->lba.filekey ? bb->lba.filekey > aa->lba.filekey : aa->lba.maxkey < bb->lba.maxkey);
        }
    } compareOpPositiveStride;

    // Negative stride
    //
    // If we are to shrink a logical volume during stage out, we need to sort the extents so that we transfer data in such a way so we can
    // periodically resize and shrink the logical volume during the data transfers without destroying any data from extents not already transferred.
    //
    // For non-BSCFS files (only files supported for negative stride), extents are all sorted together in descending LBA order.
    static struct {
        inline bool operator()(ExtentInfo a, ExtentInfo b) {
            Extent* aa = a.extent;
            Extent* bb = b.extent;
            return (bb->lba.groupkey != aa->lba.groupkey ? bb->lba.groupkey > aa->lba.groupkey :
                    bb->lba.filekey != aa->lba.filekey ? bb->lba.filekey > aa->lba.filekey : bb->lba.maxkey < aa->lba.maxkey);
        }
    } compareOpNegativeStride;

    // Static methods

    // Non-static methods
    int addExtents(const LVKey* pLVKey, const uint64_t pHandle, const uint32_t pContribId, BBTagInfo* pTagInfo, BBTransferDef* pTransferDef, vector<struct stat*>* pStats);
    void addToInFlight(const string& pConnectionName, const LVKey* pLVKey, ExtentInfo& pExtentInfo);
    void dump(const char* pSev, const char* pPrefix=0) const;
    void dumpExtents(const char* pSev, const char* pPrefix=0) const;
    void dumpInFlight(const char* pSev) const;
    Extent* getAnySourceExtent(const uint64_t pHandle, const uint32_t pContribId, const uint32_t pSourceIndex);
    Extent* getMaxInFlightExtent();
    Extent* getMinimumTrimExtent();
    size_t getNumberOfTransferDefsWithOutstandingWorkItems();
    int hasCanceledExtents();
    int moreExtentsToTransfer(const int64_t pHandle, const int32_t pContrib, uint32_t pNumberOfExpectedInFlight, int pDumpQueuesOnValue=DO_NOT_DUMP_QUEUES_ON_VALUE);
    int moreExtentsToTransferForFile(const int64_t pHandle, const int32_t pContrib, const uint32_t pSourceIndex, uint32_t pNumberOfExpectedInFlight, int pDumpQueuesOnValue=DO_NOT_DUMP_QUEUES_ON_VALUE);
    int moreInFlightExtentsForTransferDefinition(const uint64_t pHandle, const uint32_t pContrib, int pDumpQueuesOnValue=DO_NOT_DUMP_QUEUES_ON_VALUE);
    void processLastExtent(Extent& pExtent, size_t pAccumulatedLength, vector<struct stat*>* pStats);
    void removeExtent(const Extent* pExtent);
    void removeFromInFlight(const LVKey* pLVKey, ExtentInfo& pExtentInfo);
    void sendAllTransfersCompleteMsg(const string& pConnectionName, const LVKey* pLVKey);
    void setAllContribsReported(const LVKey* pLVKey, const int pValue=1);
    void setAllExtentsTransferred(const string& pConnectionName, const LVKey* pLVKey, const int pValue=1);
    void setAllExtentsTransferred(const LVKey* pLVKey, const uint64_t pHandle, const uint32_t pContribId, BBTransferDef* pTransferDef, const int pValue=1);
    void setStageOutEnded(const LVKey* pLVKey, const uint64_t pJobId, const int pValue=1);
    void setStageOutEndedComplete(const LVKey* pLVKey, const uint64_t pJobId, const int pValue=1);
    void setStageOutStarted(const LVKey* pLVKey, const uint64_t pJobId, const int pValue=1);
    int setSuspended(const LVKey* pLVKey, const string& pHostName, const uint64_t pJobId, LOCAL_METADATA_RELEASED &pLocal_Metadata_Lock_Released, const int pValue=1);
    int sortExtents(const LVKey* pLVKey, size_t& pNumberOfNewExtentsCanceled, uint64_t* pHandle=0, uint32_t* pContribId=0);
    void updateTransferStatus(const string& pConnectionName, const LVKey* pLVKey, uint32_t pNumberOfExpectedInFlight);
    void updateTransferStatus(const LVKey* pLVKey, ExtentInfo& pExtentInfo, BBTransferDef* pTransferDef, int& pNewStatus, int& pExtentsRemainForSourceIndex, uint32_t pNumberOfExpectedInFlight);

    inline int allContribsReported() {
        RETURN_FLAG(BBTI_All_Contribs_Reported);
    }

    inline int allExtentsTransferred() {
        RETURN_FLAG(BBTD_All_Extents_Transferred);
    }

    inline int BSCFS_InRequest() {
        RETURN_FLAG(BBTD_BSCFS_In_Request);
    }

    inline void calcProcessingTime(uint64_t& pTime) {
        BB_GetTimeDifference(pTime);

        return;
    }

    inline size_t getNumberOfExtents() const {
        return allExtents.size();
    }

    inline Extent* getMinTrimAnchorExtent() {
        return &minTrimAnchorExtent;
    }

    inline size_t getNumberOfInFlightExtents() {
        return inflight.size();
    }

    inline ExtentInfo getNextExtentInfo() {
        if (allExtents.size()) {
            return allExtents.front();
        } else {
            return ExtentInfo();
        }
    }

    inline int isSuspended() {
        RETURN_FLAG(BBLV_Suspended);
    }

    inline void mergeFlags(const uint64_t pFlags) {
        flags |= (pFlags & 0xFFFF000000000000);

        if (flags & BBTD_Resize_Logical_Volume_During_Stageout && flags & BBTD_BSCFS_In_Request)
        {
            // BSCFS is in the request now...
            // Make sure we do not attempt to resize the logical volume during stageout.
            setResizeLogicalVolumeDuringStageout(0);
        }

        return;
    }

    inline void resetMinTrimAnchorExtent() {
        minTrimAnchorExtent = Extent();
    }

    inline int resizeLogicalVolumeDuringStageOut() {
        RETURN_FLAG(BBTD_Resize_Logical_Volume_During_Stageout);
    }

    inline void setResizeLogicalVolumeDuringStageout(const int pValue=1) {
        SET_FLAG_AND_RETURN(BBTD_Resize_Logical_Volume_During_Stageout,pValue);
    }

    inline int stageOutEnded() {
        RETURN_FLAG(BBLV_Stage_Out_End);
    }

    inline int stageOutEndedComplete() {
        RETURN_FLAG(BBLV_Stage_Out_End_Complete);
    }

    inline int stageOutStarted() {
        RETURN_FLAG(BBLV_Stage_Out_Start);
    }

    uint64_t            flags;
    vector<ExtentInfo>  allExtents;                 ///< All extents for the LVKey.  The pointer to the
                                                    ///< ExtentInfo in the original BBTransferDef is
                                                    ///< removed once a thread has dequeued the work
                                                    ///< entry to perform the copy.
    Extent              minTrimAnchorExtent;        ///< Extent that we can trim the LV back to during
                                                    ///< stage out.  Note, that the inflight extents
                                                    ///< must be taken into account when using this
                                                    ///< value (extent) to report status back to bbproxy.
    uint64_t            processingTime;             ///< Processing time from creation to final status for LVKey
    map<InFlightKey, ExtentInfo> inflight;          ///< Map of in-flight extents
};

#endif /* BB_BBLVEXTENTINFO_H_ */
