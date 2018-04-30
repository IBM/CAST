/*******************************************************************************
 |    identity_private.h
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


#ifndef __IDENTITY_PRIVATE_H__
#define __IDENTITY_PRIVATE_H__
#include <map>
#include <pwd.h>


#define MaxGroups 1024

class UserIdentity
{
  public:
    UserIdentity() {buf=NULL;};
    
    time_t          expireTime;
    struct passwd   pwd;
    int             numGroups;
    gid_t           groupList[MaxGroups];
    std::map<gid_t,bool> groupMap;
    
    void*           buf;  // storage for pwd strings
    ~UserIdentity() { if(buf)free(buf); }
};

#endif //__IDENTITY_PRIVATE_H__
