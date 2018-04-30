/*================================================================================

    csmi/include/csm_api.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/** @file csm_api.h
 *  @brief Function prototypes and structures for the CSM APIs.
 *
 *  This contains the struct definitions for the CSM APIs and any macros,
 *  constants, or global variables you will need.
 *  
 *  
 *	@author Nick Buonarota (nbuonar@us.ibm.com)
 *	@author John Dunham (jdunham@us.ibm.com)
 */

/** @defgroup csm_apis  CSM APIs
 * APIs interact with the CSM database and are grouped based on API functionality. 
 * 
 * @warning CSM APIs **DO NOT** currently support multi-threading.
 * @{
 *      @defgroup bb_apis Burst Buffer APIs
 *          An API facilitating integration with Burst Buffer.
 *      
 *      @defgroup wm_apis Workload Manager APIs
 *          An API for creating, modifying, reading and deleting jobs on a cluster.
 *      
 *      @defgroup ras_apis RAS APIs
 *          An API for interacting with RAS events.
 *
 *      @defgroup inv_apis Inventory APIs
 *          An API for creating, modifying, reading and deleting inventory records in the 
 *          CSM Database.
 *
 *      @defgroup diag_apis Diagnostics/Health Check APIs
 *          An API facilitating Diagnostics and Health Check.
 *
 *      @defgroup common_apis Common APIs
 *          Common API structs and functions for CSM.
 *
 *      @defgroup bb_apis Burst Buffer APIs
 *          An API facilitating integration with Burst Buffer.
 *
 *      @defgroup c_serialization Serialization Functions
 *          The following Macros should be used in lieu of the generated function
 *          names, this allows for easier modification of struct names.
 *
 *      @defgroup undefined  Not Yet Implemented
 *          APIs that haven't yet been implemented.
 * @}
 */

#ifndef __CSM_API_H__
#define __CSM_API_H__

#ifdef __cplusplus
extern "C" {    
#endif

#include "csmi_type_wm_funct.h"
#include "csmi_type_ras_funct.h"
#include "csmi_type_inv_funct.h"
#include "csmi_type_bb_funct.h"
#include "csmi_type_diag_funct.h"

#ifdef __cplusplus
}
#endif

#endif
