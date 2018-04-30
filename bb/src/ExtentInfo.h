/*******************************************************************************
 |    ExtentInfo.h
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

#ifndef BB_EXTENTINFO_H_
#define BB_EXTENTINFO_H_

#include "Extent.h"

/*******************************************************************************
 | Forward declarations
 *******************************************************************************/
class BBTransferDef;

/**
 * \class ExtentInfo
 * Defines the information necessary to transfer by_extent
 */
class ExtentInfo
{
  public:
    ExtentInfo() :
        handle(0),
        contrib(0),
        extent(0),
        transferDef(0) {}

    ExtentInfo(uint64_t pHandle, uint32_t pContrib, Extent* pExtent, BBTransferDef* pTransferDef) :
        handle(pHandle),
        contrib(pContrib),
        extent(pExtent),
        transferDef(pTransferDef) {}

    ExtentInfo(const ExtentInfo& pSrc) {
        handle = pSrc.handle;
        contrib = pSrc.contrib;
        extent = pSrc.extent;
        transferDef = pSrc.transferDef;
    }

    inline uint32_t getContrib() {
        return contrib;
    }

    inline Extent* getExtent() {
        return extent;
    }

    inline uint64_t getHandle() {
        return handle;
    }

    inline uint32_t getSourceIndex() {
        return extent->getSourceIndex();
    }

    inline BBTransferDef* getTransferDef() {
        return transferDef;
    }

    uint64_t        handle;
    uint32_t        contrib;
    Extent*         extent;
    BBTransferDef*  transferDef;
};

#endif /* BB_EXTENTINFO_H_ */
