/*******************************************************************************
 |    fuse_getattr.cc
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
#define FUSE_USE_VERSION 28

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>


static int myf_getattr(const char *path, struct stat *stbuf)
{
    return 0;
}

static int myf_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi)
{
    return 0;
}

static int myf_open(const char *path, struct fuse_file_info *fi)
{
    
    return 0;
}

static int myf_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi)
{
    return 0;
}
static void* myf_init(struct fuse_conn_info *conn)
{
    printf("\nCurrent value of fuse_conn_info.max_write = %d\n\n",conn->max_write);
    return NULL;
}

static struct fuse_operations myf_oper = {
    myf_getattr,    // int (*getattr) (const char *, struct stat *);
    NULL,   // int (*readlink) (const char *, char *, size_t);
    NULL,             // int (*getdir) (const char *, fuse_dirh_t,
		      //                fuse_dirfil_t);
    NULL,      // int (*mknod) (const char *, mode_t, dev_t);
    NULL,      // int (*mkdir) (const char *, mode_t);
    NULL,     // int (*unlink) (const char *);
    NULL,      // int (*rmdir) (const char *);
    NULL,    // int (*symlink) (const char *, const char *);
    NULL,     // int (*rename) (const char *, const char *);
    NULL,       // int (*link) (const char *, const char *);
    NULL,      // int (*chmod) (const char *, mode_t);
    NULL,      // int (*chown) (const char *, uid_t, gid_t);
    NULL,   // int (*truncate) (const char *, off_t);
    NULL,             // int (*utime) (const char *, struct utimbuf *);
    myf_open,       // int (*open) (const char *, struct fuse_file_info *);
    myf_read,       // int (*read) (const char *, char *, size_t, off_t,
		      //              struct fuse_file_info *);
    NULL,      // int (*write) (const char *, const char *, size_t,
		      //               off_t, struct fuse_file_info *);
    NULL,     // int (*statfs) (const char *, struct statvfs *);
    NULL,      // int (*flush) (const char *, struct fuse_file_info *);
    NULL,    // int (*release) (const char *, struct fuse_file_info *);
    NULL,      // int (*fsync) (const char *, int,
		      //               struct fuse_file_info *);
    NULL,   // int (*setxattr) (const char *, const char *,
		      //                  const char *, size_t, int);
    NULL,   // int (*getxattr) (const char *, const char *, char *,
		      //                  size_t);
    NULL,  // int (*listxattr) (const char *, char *, size_t);
    NULL,// int (*removexattr) (const char *, const char *);
    NULL,    // int (*opendir) (const char *, struct fuse_file_info *);
    myf_readdir,    // int (*readdir) (const char *, void *, fuse_fill_dir_t,
		      //                 off_t, struct fuse_file_info *);
    NULL, // int (*releasedir) (const char *,
		      //                    struct fuse_file_info *);
    NULL,   // int (*fsyncdir) (const char *, int,
		      //                  struct fuse_file_info *);
    myf_init,       // void *(*init) (struct fuse_conn_info *conn);
    NULL,    // void (*destroy) (void *);
    NULL,     // int (*access) (const char *, int);
    NULL,     // int (*create) (const char *, mode_t,
		      //                struct fuse_file_info *);
    NULL,  // int (*ftruncate) (const char *, off_t,
		      //                   struct fuse_file_info *);
    NULL,   // int (*fgetattr) (const char *, struct stat *,
		      //                  struct fuse_file_info *);
    NULL,             // int (*lock) (const char *, struct fuse_file_info *,
		      //              int cmd, struct flock *);
    NULL,    // int (*utimens) (const char *,
		      //                 const struct timespec tv[2]);
    NULL,       // int (*bmap) (const char *, size_t blocksize,
		      //              uint64_t *idx);
    1,                // unsigned int flag_nullpath_ok:1;
    1,                // unsigned int flag_nopath:1;
    1,                // unsigned int flag_utime_omit_ok:1;
    29,               // unsigned int flag_reserved:29;
    NULL,      // int (*ioctl) (const char *, int cmd, void *arg,
		      //               struct fuse_file_info *,
		      //               unsigned int flags, void *data);
    NULL,       // int (*poll) (const char *, struct fuse_file_info *,
		      //              struct fuse_pollhandle *ph,
		      //              unsigned *reventsp);
    NULL,             // int (*write_buf) (const char *,
		      //                   struct fuse_bufvec *buf, off_t off,
		      //                   struct fuse_file_info *);
    NULL,             // int (*read_buf) (const char *,
		      //                  struct fuse_bufvec **bufp,
		      //                  size_t size, off_t off,
		      //                  struct fuse_file_info *);
    NULL,             // int (*flock) (const char *, struct fuse_file_info *,
		      //               int op);
    NULL,             // int (*fallocate) (const char *, int, off_t,
		      //                   off_t,struct fuse_file_info *);
};

int main(int argc, char *argv[])
{
    if (argc != 3)
        {
            printf("\n\t \"fuse_getattr /mount_point -d\"\n\n");
            return 0;
        }
    return fuse_main(argc, argv, &myf_oper, NULL);
}
