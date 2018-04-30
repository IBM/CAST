/*******************************************************************************
 |    Extent.h
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

#ifndef BB_EXTENT_H_
#define BB_EXTENT_H_

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "bbapi2.h"
#include "bbapi_types.h"
#include "bbfileflags.h"
#include "bbflags.h"
#include "logging.h"

// #include <boost/archive/binary_oarchive.hpp>
// #include <boost/archive/binary_iarchive.hpp>
// #include <boost/serialization/binary_object.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
// #include <boost/filesystem.hpp>
// #include <boost/serialization/map.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>

using namespace std;

/*******************************************************************************
 | Classes
 *******************************************************************************/

/**
 * \class Extent
 * Defines a unit of storage for transfer
 */
class Extent
{
  public:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& pArchive, const uint32_t pVersion)
    {
        pArchive & sourceindex;
        pArchive & targetindex;
        pArchive & flags;
    }

    Extent() :
        sourceindex(0),
        targetindex(0),
        handle(UNDEFINED_HANDLE),
        flags(0),
        contribid(UNDEFINED_CONTRIBID) {
        lba.groupkey = 0;
        lba.filekey = 0;
        lba.maxkey = 0;
        lba.start = 0;
        start = 0;
        len = 0 ;
        memset((void*)serial, 0, sizeof(serial));
    }

    // Constructor used when creating single file extents for each source/target file pair
    // of a transfer definition being retrieved from bbServer metadata
    Extent(const uint32_t pSourceIndex, const uint32_t pTargetIndex, const uint64_t pFlags) :
        sourceindex(pSourceIndex),
        targetindex(pTargetIndex),
        handle(UNDEFINED_HANDLE),
        flags(pFlags),
        contribid(UNDEFINED_CONTRIBID) {
        lba.groupkey = 0;
        lba.filekey = 0;
        lba.maxkey = 0;
        lba.start = 0;
        start = 0;
        len = 0;
        memset((void*)serial, 0, sizeof(serial));
    }

    Extent(const Extent &src) :
        sourceindex(src.sourceindex),
        targetindex(src.targetindex),
        handle(src.handle),
        flags(src.flags),
        contribid(src.contribid) {
        lba.groupkey = src.lba.groupkey;
        lba.filekey = src.lba.filekey;
        lba.maxkey = src.lba.maxkey;
        lba.start = src.lba.start;
        start = src.start;
        len = src.len;
        memcpy((void*)serial, (void*)&src.serial, sizeof(serial));
    }

    void clear()
    {
        sourceindex = 0;
        targetindex = 0;
        handle = UNDEFINED_HANDLE;
        flags = 0;
        contribid = UNDEFINED_CONTRIBID;
        lba.groupkey = 0;
        lba.filekey = 0;
        lba.maxkey = 0;
        lba.start = 0;
        start = 0;
        len = 0 ;
        memset((void*)serial, 0, sizeof(serial));
    }

    inline void dump(const char* pSev, const char* pPrefix) const {
        if (!strcmp(pSev,"debug")) {
            LOG(bb,debug) << ">>>>> Start: " << (pPrefix ? pPrefix : "Dump Extent <<<<<");
            LOG(bb,debug) << "srcidx=" << setw(3) << sourceindex \
                          << hex << uppercase << ", flgs=0x" << setfill('0') << setw(16) << flags \
                          << ", lba.key=0x" << setw(4) << lba.groupkey << setw(4) << lba.filekey << setw(16) << lba.maxkey \
                          << ", lba.start=0x" << setw(16) << lba.start << ", start=0x" << setw(8) << start \
                          << ", len=0x" << setw(8) << len \
                          << setfill(' ') << nouppercase << dec \
                          << ", tgtidx=" << setw(3) << targetindex \
                          << ", handle=" << handle \
                          << ", contribid=" << contribid \
                          << ", serial=" << serial;
            LOG(bb,debug) << ">>>>>   End: " << (pPrefix ? pPrefix : "Dump Extent <<<<<");
        } else if (!strcmp(pSev,"info")) {
            LOG(bb,info) << ">>>>> Start: " << (pPrefix ? pPrefix : "Dump Extent <<<<<");
            LOG(bb,info) << "srcidx=" << setw(3) << sourceindex \
                          << hex << uppercase << ", flgs=0x" << setfill('0') << setw(16) << flags \
                          << ", lba.key=0x" << setw(4) << lba.groupkey << setw(4) << lba.filekey << setw(16) << lba.maxkey \
                          << ", lba.start=0x" << setw(16) << lba.start << ", start=0x" << setw(8) << start \
                          << ", len=0x" << setw(8) << len \
                          << setfill(' ') << nouppercase << dec \
                          << ", tgtidx=" << setw(3) << targetindex \
                          << ", handle=" << handle \
                          << ", contribid=" << contribid \
                          << ", serial=" << serial;
            LOG(bb,info) << ">>>>>   End: " << (pPrefix ? pPrefix : "Dump Extent <<<<<");
        }

        return;
    }

    inline uint16_t getBundleType() {
        return (uint16_t)(BBTransferTypeFromFlags(flags));
    }

    inline uint16_t getBundleID() {
        return (uint16_t)((flags & BBFileBundleIDMask) >> BBFileBundleIDShift);
    }

    inline uint32_t getContribId() {
        return contribid;
    }

    inline uint64_t getFlags() {
        return flags;
    }

    inline uint64_t getHandle() {
        return handle;
    }

    inline size_t getLength() {
        return len;
    }

    inline uint32_t getSourceIndex() {
        return sourceindex;
    }

    inline uint32_t getTargetIndex() {
        return targetindex;
    }

    inline uint16_t getTransferOrder() {
        return (uint16_t)((flags & BBTransferOrderMask) >> BBTransferOrderShift);
    }

    inline int isBSCFS_Extent() {
        RETURN_FLAG(BBI_BSCFS_File_Transfer);
    }

    inline int isCP_Transfer() {
        RETURN_FLAG(BBI_CP_Transfer);
    }

    inline int isRegularExtent() {
        return (BBTransferTypeFromFlags(flags) == BBTransferTypeRegular) ? 1 : 0;
    }

    inline int isTrimAnchor() {
        RETURN_FLAG(BBI_Trim_Anchor);
    }

    inline void resetForSort() {
        // Reset the group/file keys
        lba.groupkey = 0;
        lba.filekey = 0;

        // Reset the trim anchor
        setTrimAnchor(0);

        // Turn off any existing first/last flags
        flags &= (~BBI_First_Extent);
        flags &= (~BBI_Last_Extent);

        return;
    }

    inline int sameBundleID(const uint16_t pBundleID) {
        return pBundleID == getBundleID();
    }

    inline int sameTransferOrder(const uint16_t pTransferOrder) {
        return pTransferOrder == getTransferOrder();
    }

    inline void setCanceled(const int pValue=1) {
        SET_FLAG_AND_RETURN(BBTD_Canceled,pValue);
    }

    inline void setCP_Tansfer(const int pValue=1) {
        SET_FLAG_AND_RETURN(BBI_CP_Transfer,pValue);
    }

    inline void setTrimAnchor(const int pValue=1) {
        SET_FLAG_AND_RETURN(BBI_Trim_Anchor,pValue);
    }

    uint32_t sourceindex;
    uint32_t targetindex;
    uint64_t handle;
    uint64_t flags;
    uint32_t contribid;
    struct {
        uint64_t groupkey;  ///< leading sort key for extents
        uint64_t filekey;   ///< secondary sort key for extents
        uint64_t maxkey;    ///< maximum logical block address
        uint64_t start;     ///< logical block address on the SSD
    } lba;
    off_t    start;         ///< offset into the file
    size_t   len;           ///< length of extent in bytes
    char     serial[40];
};

inline std::ostream & operator<<(std::ostream &os, const Extent& p)
{
    return os << "srcidx=" << setw(3) << p.sourceindex
              << hex << uppercase << ", flgs=0x" << setfill('0') << setw(16) << p.flags
//              << ", lba.key=0x" << setw(4) << p.lba.groupkey << setw(4) << p.lba.filekey << setw(16) << p.lba.maxkey
              << ", lba.start=0x" << setw(16) << p.lba.start << ", start=0x" << setw(8) << p.start
              << ", len=0x" << setw(8) << p.len
              << setfill(' ') << nouppercase << dec
              << ", tgtidx=" << setw(3) << p.targetindex;
//              << ", handle=" << p.handle;
//              << ", contribid=" << p.contribid;
//              << ", serial=" << p.serial;
}

#endif /* BB_EXTENT_H_ */
