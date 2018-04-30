/*******************************************************************************
 |    BBSTATUS.h
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

#ifndef BB_BBSTATUS_H_
#define BB_BBSTATUS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>

/**
 *  \brief Burst Buffer status values
 *  \note Bit encoded values
 *  \ingroup bbapi
 */
enum BBSTATUS
{
    BBNONE              = 0x0000000000000000, ///< No status (used by BB_GetTransferInfo)
    BBNOTSTARTED        = 0x0000000000000001, ///< The tag has been defined to the system
                                              ///< but none of the contributors have yet
                                              ///< reported.
    BBINPROGRESS        = 0x0000000000000002, ///< The tag has been defined to the system
                                              ///< and data for the associated transfer
                                              ///< definitions is actively being transferred.
    BBPARTIALSUCCESS    = 0x0000000000000004, ///< Less than the full number of contributors
                                              ///< reported for the tag, but all of the
                                              ///< data for those reported transfer definitions
                                              ///< has been transferred successfully.  However,
                                              ///< some contributors never reported, and
                                              ///< therefore, not all data was transferred.
    BBFULLSUCCESS       = 0x0000000000000008, ///< All contributors have reported for the tag
                                              ///< and all data for all of the associated transfer
                                              ///< definitions has been transferred successfully.
    BBCANCELED          = 0x0000000000000010, ///< All data transfers associated with the tag
                                              ///< have been cancelled by the user.  No or
                                              ///< some data may have been transferred.
    BBFAILED            = 0x0000000000000020, ///< One or more data transfers have failed for
                                              ///< one or more of the associated transfer
                                              ///< definitions for the tag.  No or some data
                                              ///< may have been transferred successfully.
    BBSTOPPED           = 0x0000000000000040, ///< All data transfers associated with the tag
                                              ///< have been stopped.  No or some data may
                                              ///< have been transferred.
    BBNOTACONTRIB       = 0x0000000000000100, ///< The contributor making the API request is
                                              ///< NOT in the array of contributors for this tag.
                                              ///< Therefore, localStatus and localTransferSize
                                              ///< cannot be returned.
    BBNOTREPORTED       = 0x0000000000000200, ///< The contributor making the API request is
                                              ///< in the array of contributors for this tag.
                                              ///< However, it has not yet reported in with it's
                                              ///< transfer definition.
    BBALL               = 0xFFFFFFFFFFFFFFFF  ///< All status values (used by BB_GetTransferList)
};
typedef enum BBSTATUS BBSTATUS;

#define BBSTATUS_OR(OP1,OP2) (BBSTATUS)((uint64_t)OP1 | (uint64_t)OP2)
#define BBSTATUS_AND(OP1,OP2) (BBSTATUS)((uint64_t)OP1 & (uint64_t)OP2)

#ifdef __cplusplus
}
#endif

#endif /* BB_BBSTATUS_H_ */
