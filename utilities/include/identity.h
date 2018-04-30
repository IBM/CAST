/*******************************************************************************
 |    identity.h
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


#ifndef COMMON_USERIDENTITY_H
#define COMMON_USERIDENTITY_H

#include <sys/types.h>

#ifdef __cplusplus
//extern "C" {
#endif
    
int becomeUser(uid_t user, gid_t group = ~((gid_t)0));
int checkUserGroup(uid_t user, gid_t group, bool* validGroup, bool* isPrimary);

#ifdef __cplusplus
//}
#endif

#endif // COMMON_USERIDENTITY_H
