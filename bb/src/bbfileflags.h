/*******************************************************************************
 |    bbfileflags.h
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

#ifndef BB_BBFILEFLAGS_H_
#define BB_BBFILEFLAGS_H_

/*******************************************************************************
 | Forward declarations
 *******************************************************************************/

/*******************************************************************************
 | Typedefs
 *******************************************************************************/

/*******************************************************************************
 | External data
 *******************************************************************************/

/*******************************************************************************
 | External methods
 *******************************************************************************/

/*******************************************************************************
 | Constants
 *******************************************************************************/

/*******************************************************************************
 | Classes and structs
 *******************************************************************************/

/*******************************************************************************
 | Enumerators
 *******************************************************************************/
enum BBFILEFLAGS_INTERNAL
{
    BBTransferTypeMask   = 0x0030, ///< File bundle transfer type -- mask
    BBTransferTypeShift  = 4,      ///< File bundle transfer type -- shift
    BBTransferOrderMask  = 0x00c0, ///< File bundle transfer priority -- mask
    BBTransferOrderShift = 6,      ///< File bundle transfer priority -- shift
    BBFileBundleIDMask   = 0xff00, ///< File bundle identifier -- mask
    BBFileBundleIDShift  = 8,      ///< File bundle identifier -- shift
};
typedef enum BBFILEFLAGS_INTERNAL BBFILEFLAGS_INTERNAL;

/*******************************************************************************
 | Macro definitions
 *******************************************************************************/
#define BBTransferTypeRegular 0
#define BBTransferTypeBSCFS  1

#define BBTransferType(n) \
   (((n) << BBTransferTypeShift) & BBTransferTypeMask)

#define BBTransferOrder(n) \
   (((n) << BBTransferOrderShift) & BBTransferOrderMask)

#define BBFileBundleID(n) \
   (((n) << BBFileBundleIDShift) & BBFileBundleIDMask)

#define BBTransferTypeFromFlags(f) \
   (((f) & BBTransferTypeMask) >> BBTransferTypeShift)

#define BBTransferOrderFromFlags(f) \
   (((f) & BBTransferOrderMask) >> BBTransferOrderShift)

#define BBFileBundleIDFromFlags(f) \
   (((f) & BBFileBundleIDMask) >> BBFileBundleIDShift)

/*******************************************************************************
 | Helper methods
 *******************************************************************************/

#endif /* BB_BBFILEFLAGS_H_ */
