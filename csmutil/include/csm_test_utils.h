/*================================================================================

    csmutil/include/csm_test_utils.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/** file csm_test_utils.h
 *
 ******************************************/


#ifndef __CSM_TEST_UTILS_H__
#define __CSM_TEST_UTILS_H__

#define TEST( function, expect ) ( !( (expect) == (function) ) )
#define TESTFAIL( function, expect ) ( !TEST(function, expect) )

#endif // __CSM_TEST_UTILS_H__
