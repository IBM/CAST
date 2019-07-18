/*******************************************************************************
 |    identity.cc
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


#include <pthread.h>
#include <stdio.h>
#include <map>
#include <list>
#include <grp.h>
#include <string.h>
#include <stdlib.h>
#include <sys/fsuid.h>
#include <sys/syscall.h>
#include <errno.h>
#include "fsutil_flightlog.h"
#include "identity.h"
#include "identity_private.h"

using namespace std;

pthread_mutex_t          uidMap_lock = PTHREAD_MUTEX_INITIALIZER;
map<uid_t, UserIdentity> uidMap;   ///< protected by uidMap_lock
list<uid_t>              ageList;  ///< protected by uidMap_lock

#define UIDCACHE_AGELIMIT 60 ///< number of seconds to retain UID cache

FL_SetSize(FLUtility, 16384);
FL_SetName(FLUtility, "Utilities");

static int setgroups_thread(int numGroups, gid_t* groupList)
{
    // glibc wrappers setgroups for POSIX requirement that all threads have the same view.
    // for per-thread, we'll need to call the syscall directly.
    return syscall(SYS_setgroups, numGroups, groupList);
}

static int updateUserIdentityCache(uid_t user, UserIdentity*& ui)
{
    int rc;
    int x;
    time_t curtime = time(NULL);
    map<uid_t, UserIdentity>::iterator it;
    struct passwd* result;
    ssize_t buflen;

    /* Remove UID from cache since its been sitting there too long */
    if(ageList.size() > 0)
    {
        if(uidMap[ageList.front()].expireTime < curtime)
        {
            uid_t evictuid = ageList.front();
            FL_Write(FLUtility, FL_AGEUIDCACHE, "Aging-out user from identity cache.  User %ld.  ExpireTime=%ld  CurTime=%ld",
                     evictuid, uidMap[evictuid].expireTime, curtime, 0);
            uidMap.erase(evictuid);
            ageList.pop_front();
        }
    }

    it = uidMap.find(user);
    if(it != uidMap.end())
    {
        ui = &it->second;
        return 0;
    }

    FL_Write(FLUtility, FL_LOOKUPUSER, "User %ld is not in the UID cache.  Looking up identity and secondary groups", user, 0, 0, 0);

    // build UserIdentity
    buflen = sysconf(_SC_GETPW_R_SIZE_MAX);
    if(buflen == -1)
        buflen = 16384;

    uidMap[user].buf = malloc(buflen);
    uidMap[user].expireTime = curtime + UIDCACHE_AGELIMIT;
    uidMap[user].numGroups = MaxGroups;

    rc = getpwuid_r(user, &uidMap[user].pwd, (char*)uidMap[user].buf, buflen, &result);
    if(rc || (result == NULL))
    {
        FL_Write6(FLUtility, FL_GETPWUIDFAIL, "Failed to retrieve user %ld passwd entry.  rc=%ld, buflen=%ld, errno=%ld, result=%p",
                  user, rc, buflen, errno, (uint64_t)result, 0);
        uidMap.erase(user);
        return -1;
    }
    rc = getgrouplist(uidMap[user].pwd.pw_name, uidMap[user].pwd.pw_gid, uidMap[user].groupList, &uidMap[user].numGroups);
    if (rc == -1)
    {
        FL_Write(FLUtility, FL_GETGRPSFAIL, "Failed to retrieve secondary groups for user %ld.  rc=%ld, buflen=%ld, errno=%ld",
                 user, rc, buflen, errno);
        uidMap.erase(user);
        return -1;
    }
    ageList.push_back(user);
    for(x=0; x<uidMap[user].numGroups; x++)
    {
        uidMap[user].groupMap[uidMap[user].groupList[x]] = false;
    }
    uidMap[user].groupMap[uidMap[user].pwd.pw_gid] = true;

    ui = &uidMap[user];
    return 0;
}

int becomeUser(uid_t user, gid_t primaryGroup)
{
    static __thread uid_t currentThreadUID = ~0;
    if(currentThreadUID == user)
    {
        return 0;
    }

    FL_Write(FLUtility, FL_SWITCHUSER, "Thread switching FS context to userid %ld, primaryGroup %ld", user, primaryGroup, 0, 0);

    // map is not thread-safe, create thread lock
    pthread_mutex_lock(&uidMap_lock);

    int rc;
    uid_t olduid, curuid;
    gid_t oldgid, curgid;
    gid_t GID;
    UserIdentity* ui;

    /* Lookup UID credentials */
    if(updateUserIdentityCache(user, ui) != 0)
    {
        pthread_mutex_unlock(&uidMap_lock);
        return -1;
    }

    /*  Switch context */
    if(primaryGroup != ~((gid_t)0))
    {
        map<gid_t, bool>::iterator it;
        it = ui->groupMap.find(primaryGroup);
        if (it == ui->groupMap.end())
        {
            FL_Write(FLUtility, FL_BADPRIMARYGROUP, "Specified primary group is not a member of the user's group list", user, primaryGroup, 0, 0);

            pthread_mutex_unlock(&uidMap_lock);
            return -1;
        }

        GID = primaryGroup;
    }
    else
    {
        GID = ui->pwd.pw_gid;
    }

    /* set file system userid */
    olduid = setfsuid(ui->pwd.pw_uid);
    curuid = setfsuid(ui->pwd.pw_uid);
    if(curuid != ui->pwd.pw_uid) // setfsuid error happened
    {
        FL_Write(FLUtility, FL_SETFSUIDFAIL, "Failed to setfsuid for user %ld.  Current userid %ld", ui->pwd.pw_uid, curuid, 0, 0);
        pthread_mutex_unlock(&uidMap_lock);
        return -1;
    }
    FL_Write(FLUtility, FL_SETFSUIDPASS, "Set fsuid to user %ld, previously set to userid %ld.", ui->pwd.pw_uid, olduid, 0, 0);

    /* set file system groupid */
    oldgid = setfsgid(GID);
    curgid = setfsgid(GID);
    if(curgid != GID) // setfsgid error happened
    {
        FL_Write(FLUtility, FL_SETFSGIDFAIL, "Failed to setfsgid for group %ld.  Current groupid %ld",
                 GID, curgid, 0, 0);

        user = currentThreadUID;
        olduid = setfsuid(user);
        curuid = setfsuid(user);
        FL_Write(FLUtility, FL_SETFSUIDFAIL2, "Reverted setfsuid back to user %ld due to setfsgid failure.  Current userid %ld",
                 user, curuid, 0, 0);

        pthread_mutex_unlock(&uidMap_lock);
        (void)olduid;
        return -1;
    }
    FL_Write(FLUtility, FL_SETFSGIDPASS, "Set fsgid to group %ld, previously set to groupid %ld.", GID, oldgid, 0, 0);

    /* set secondary groups for thread */
    rc = setgroups_thread(uidMap[user].numGroups, uidMap[user].groupList);
    if(rc)
    {
        FL_Write(FLUtility, FL_SETGROUPSFAIL, "Failed to set %ld secondary groups for user %ld.  rc=%ld, errno=%ld",
                 uidMap[user].numGroups, user, rc, errno);

        user = currentThreadUID;
        oldgid = setfsgid(GID);
        curgid = setfsgid(GID);
        FL_Write(FLUtility, FL_SETFSGIDFAIL3, "Reverted setfsgid back to user %ld due to setgroups failure.  Current gid %ld",
                 user, curgid, 0, 0);

        olduid = setfsuid(user);
        curuid = setfsuid(user);
        FL_Write(FLUtility, FL_SETFSUIDFAIL3, "Reverted setfsuid back to user %ld due to setgroups failure.  Current uid %ld",
                 user, curuid, 0, 0);

        pthread_mutex_unlock(&uidMap_lock);
        (void)oldgid;
        (void)olduid;
        return -1;
    }
    currentThreadUID = user;
    FL_Write(FLUtility, FL_BECAMEUSER, "Successfully switched file system to user %ld.", user, 0, 0, 0);

    pthread_mutex_unlock(&uidMap_lock);

    return 0;
}

int checkUserGroup(uid_t user, gid_t group, bool* validGroup, bool* isPrimary)
{
    UserIdentity* ui;
    map<gid_t, bool>::iterator it;

    if(validGroup)
        *validGroup = false;
    if(isPrimary)
        *isPrimary = false;

    pthread_mutex_lock(&uidMap_lock);

    if(updateUserIdentityCache(user, ui) != 0)
    {
        pthread_mutex_unlock(&uidMap_lock);
        return -1;
    }

    it = ui->groupMap.find(group);
    if(it != ui->groupMap.end())
    {
        if(validGroup)
            *validGroup = true;
        if(isPrimary)
            *isPrimary = ui->groupMap[group];
    }

    pthread_mutex_unlock(&uidMap_lock);

    return 0;
}

