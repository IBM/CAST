/*================================================================================

    csmi/include/csm_api_version.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSM_API_VERSION_H__
#define __CSM_API_VERSION_H__

/** @file csm_api_version.h
 * @brief The listing of versions released and supported by CSM.
 */

#ifdef __cplusplus
extern "C" {    
#endif

/// TODO ADD MASKING MACROS!

///< The version id for CSM (uint64_t), this is the version counter for this API.
#define CSM_VERSION_ID CSM_VERSION_1_0_0

///< The development level.
#define CSM_DEVELOPMENT CSM_VERSION_ID

///< The minimum supported CSM level.
#define CSM_MIN_VERSION CSM_VERSION_0_4_1

/// VERSION START
#define CSM_VERSION_1_0_0 65536
#define CSM_VERSION_0_4_1 1025


#ifdef __cplusplus
}
#endif
#endif
