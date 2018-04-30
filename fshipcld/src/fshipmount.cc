/*******************************************************************************
 |    fshipmount.cc
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

//! \file  fshipcld.h
//! \brief Declarations for fshipcld
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include "fshipcld.h"
#include "logging.h"

/* return device file descriptor */
int mount_fship(char *target, int pMax_read) {
  const char *devname = "/dev/fuse";
  char type[64];
  snprintf(type, sizeof(type), "fuse.coral%d", getpid());
  // char *type = (char *) "fuse.coral";
  int save_errno = 0;
  int fdDev = -1;
  int retval;

  { // limit scope
    char buffer[128];
    FILE *checkModFileStream =
        popen("ls -ld /sys/module/fuse |grep fuse 2>&1", "r");
    fgets(buffer, 127, checkModFileStream);
    LOG(fshipcld, always) << buffer;
    pclose(checkModFileStream);
  }

  unsigned long mnt_flags = 0;
  char tmp[512];
  struct stat st_local;
  /* MS_NOATIME;  ?? No updating access times */

  errno = 0;
  retval = stat(target, &st_local);
  if (retval) {
    LOG(fshipcld, always) << "mountpoint=" << target << " errno=" << errno
                          << ":" << strerror(errno);

    if (errno ==
        ENOTCONN) { /* this can happen when control-C in an interactive session
                       */
      save_errno = ENOTCONN;
    }
  }

  fdDev =
      open(devname,
           O_RDWR | O_NONBLOCK); /* is device really able to do O_NONBLOCK?*/

  if (fdDev == -1) {
    LOG(fshipcld, always) << "failed to open devname=" << devname
                          << " errno=" << errno << ":" << strerror(errno);

    goto out;
    return -1;
  }

  /* to verify mount, do "cat /proc/mounts |grep fuse" */
  snprintf(tmp, sizeof(tmp),
           "fd=%i,rootmode=%o,user_id=%i,group_id=%i,allow_other,max_read=%i",
           fdDev, st_local.st_mode & S_IFMT, getuid(), getgid(), pMax_read);

  LOG(fshipcld, always) << tmp;
/* optionals???
const char default_permission[]="default_permissions";
const char allow_other[]="allow_other";
const char max_read="max_read=%u";
const char blksize="blksize=%u";
*/

#ifdef __APPLE__
  retval = mount(type, target, mnt_flags, tmp);
#else
  retval = mount(devname, target, type, mnt_flags, tmp);
#endif

  if (retval == -1) {
    LOG(fshipcld, always) << "failed tomount devname=" << devname
                          << " type=" << type << " errno=" << errno << ":"
                          << strerror(errno);
    if ((errno == EINVAL) && (save_errno == ENOTCONN)) {
      goto out_umount;
    } else
      save_errno = 0;
    LOG(fshipcld, always) << "failed to mount " << devname << " on " << target
                          << " errno=" << errno << ":" << strerror(errno);
    goto out_umount;
  }
  LOG(fshipcld, always) << "device name=" << devname << " on " << target;
  /*sleep(5);*/
  return fdDev;

out_umount:
  umount2(target, 2); /* lazy umount */
out:
  close(fdDev);
  umount2(target, 2);
  if (save_errno)
    return -save_errno;
  return -1;
}
