/*******************************************************************************
 |    weak.cc
 |
 |  © Copyright IBM Corporation 2017,2017. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
*******************************************************************************/

#define _GNU_SOURCE 1
#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <identity.h>
#include <string.h>
#include <grp.h>
#include <sys/fsuid.h>

const char* hitname = "/var/log/";

typedef FILE* (*fopentype)(const char*, const char*);
typedef int (*renametype)(const char*, const char*);
typedef int (*unlinktype)(const char*);

/***
Override fopen64, rename, and unlink symbols for console logrotate capability.
Because bbProxy and bbServer periodically switches its file system uid/gid context, the boost::log
will occasionally close/open a new log file under the FS context of the logging thread.  This
needs to be root to be successful.  boost::log does not provide hooks to override its open/rename/unlink
methods, so we're using weak symbols to catch the syscalls and switch to the correct (root) user, and then back 
when complete.  
 ***/

extern "C" FILE* fopen64(const char* path, const char* mode)
{
    FILE* f;
    uid_t uid = 0;
    gid_t gid = 0;
    bool  switchuid = false;
    char  tmpmode[64];
    if(strncmp(path, hitname, strlen(hitname)) == 0)
    {
        snprintf(tmpmode, sizeof(tmpmode), "e%s", mode);   // add O_CLOEXEC for console.log
        uid = setfsuid(~0);
        gid = setfsgid(~0);
        if((uid != 0) || (gid != 0))
        {
            becomeUser(0,0);
            switchuid = true;
        }
    }
    static fopentype orig_fopen = NULL;
    if(orig_fopen == NULL)
    {
        orig_fopen = (fopentype)dlsym(RTLD_NEXT, "fopen64");
    }
    f = (orig_fopen)(path, mode);
    int savedErrno = 0;
    if (f==NULL) savedErrno=errno;
    if(switchuid)
    {
        becomeUser(uid, gid);
    }
    if (f==NULL) errno=savedErrno;
    return f;
}

extern "C" int rename(const char* oldname, const char* newname)
{
    int   rc;
    uid_t uid = 0;
    gid_t gid = 0;
    bool  switchuid = false;
    if(strncmp(oldname, hitname, strlen(hitname)) == 0)
    {
        uid = setfsuid(~0);
        gid = setfsgid(~0);
        if((uid != 0) || (gid != 0))
        {
            becomeUser(0,0);
            switchuid = true;
        }
    }
    static renametype orig_rename = NULL;
    if(orig_rename == NULL)
    {
        orig_rename = (renametype)dlsym(RTLD_NEXT, "rename");
    }
    rc = (orig_rename)(oldname, newname);
    int savedErrno = 0;
    if (rc) savedErrno=errno;
    if(switchuid)
    {
        becomeUser(uid, gid);
    }
    if (rc) errno=savedErrno;
    return rc;
}

extern "C" int unlink(const char* path)
{
    int   rc;
    uid_t uid = 0;
    gid_t gid = 0;
    bool  switchuid = false;
    if(strncmp(path, hitname, strlen(hitname)) == 0)
    {
        uid = setfsuid(~0);
        gid = setfsgid(~0);
        if((uid != 0) || (gid != 0))
        {
            becomeUser(0,0);
            switchuid = true;
        }
    }
    static unlinktype orig_unlink = NULL;
    if(orig_unlink == NULL)
    {
        orig_unlink = (unlinktype)dlsym(RTLD_NEXT, "unlink");
    }
    rc = (orig_unlink)(path);
    int savedErrno = 0;
    if (rc) savedErrno=errno;
    if(switchuid)
    {
        becomeUser(uid, gid);
    }
    if (rc) errno=savedErrno;
    return rc;
}
/***
 * The override of the stat library call is done differently since it is not called out as a weak symbol.  
 * The stat library call is overridden here and then calls into fstatat to get the struct stat.
 ***/
extern "C" int stat(const char* path, struct stat *buf)
{
    int   rc;
    uid_t uid = 0;
    gid_t gid = 0;
    bool  switchuid = false;
    if(strncmp(path, hitname, strlen(hitname)) == 0)
    {
        uid = setfsuid(~0);
        gid = setfsgid(~0);
        if((uid != 0) || (gid != 0))
        {
            becomeUser(0,0);
            switchuid = true;
        }
    }
    int filehandle = -1;
    int fstatatflags = AT_NO_AUTOMOUNT;
    rc = fstatat(filehandle, path, buf, fstatatflags);
    int savedErrno = 0;
    if (rc) savedErrno=errno;
    if(switchuid)
    {
        becomeUser(uid, gid);
    }
    if (rc) errno=savedErrno;
    return rc;
}
