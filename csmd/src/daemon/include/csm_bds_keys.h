/*================================================================================

    csmd/src/daemon/include/csm_bds_keys.h

  © Copyright IBM Corporation 2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSM_BDS_KEYS_H
#define __CSM_BDS_KEYS_H

////////////////////////////////////////////////////////////////////////////////////////////////////
// CSM_BDS_KEY_TYPE - the type of data being reported 
////////////////////////////////////////////////////////////////////////////////////////////////////
#define CSM_BDS_KEY_TYPE "type"

// Known values for CSM_BDS_KEY_TYPE
#define CSM_BDS_TYPE_GPU_COUNTERS  "csm-gpu-counters"

#define CSM_BDS_TYPE_NODE_ENV      "csm-node-env"
#define CSM_BDS_TYPE_PROCESSOR_ENV "csm-processor-env"
#define CSM_BDS_TYPE_GPU_ENV       "csm-gpu-env"
#define CSM_BDS_TYPE_DIMM_ENV      "csm-dimm-env"
#define CSM_BDS_TYPE_TEST_ENV      "csm-test-env"

////////////////////////////////////////////////////////////////////////////////////////////////////
// CSM_BDS_KEY_SOURCE - the source from which the reported data was collected 
////////////////////////////////////////////////////////////////////////////////////////////////////
#define CSM_BDS_KEY_SOURCE "source"

////////////////////////////////////////////////////////////////////////////////////////////////////
// CSM_BDS_KEY_TIME_STAMP - time of data collection 
////////////////////////////////////////////////////////////////////////////////////////////////////
#define CSM_BDS_KEY_TIME_STAMP "timestamp"

////////////////////////////////////////////////////////////////////////////////////////////////////
// CSM_BDS_SECTION_DATA - hierarchical section containing multiple type specific key/value pairs 
////////////////////////////////////////////////////////////////////////////////////////////////////
#define CSM_BDS_SECTION_DATA "data"

#endif
