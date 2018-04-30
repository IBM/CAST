/*================================================================================
   
    csmi//src/common/include/csmi_common_generated.h

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
#ifndef _CSMI_COMMON_GENERATED_H_
#define _CSMI_COMMON_GENERATED_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
typedef enum {
    CSM_STRING_TYPE = 1,
    CSM_ENUM_TYPE = 2,
    CSM_CSM_BOOL_TYPE = 4,
    CSM_ENUM_BIT_TYPE = 3,
    CSM_MISC_TYPE = 0,
} csmi_type_resolver;

#define CSM_PRINT_PRIMATIVE( ptr, post)\
    case 11: printf("%d" post, *((int*)ptr));break;\
    case 5: printf("%"PRIu8  post, *((uint8_t*)ptr));break;\
    case 9: printf("%"PRId32 post, *((int32_t*)ptr));break;\
    case 15: printf("%zd" post, *((size_t*)ptr));break;\
    case 14: printf("%f" post, *((double*)ptr));break;\
    case 12: printf("%d" post, *((short*)ptr));break;\
    case 7: printf("%"PRIu64 post, *((uint64_t*)ptr));break;\
    case 13: printf("%ld" post, *((long*)ptr));break;\
    case 8: printf("%"PRId8  post, *((int8_t*)ptr));break;\
    case 10: printf("%"PRId64 post, *((int64_t*)ptr));break;\
    case 6: printf("%"PRIu32 post, *((uint32_t*)ptr));break;\
    case 16: printf("%d" post, *((pid_t*)ptr));break;\
    case 17: printf("%c" post, *((char*)ptr));break;\


#ifdef __cplusplus
}
#endif
#endif
