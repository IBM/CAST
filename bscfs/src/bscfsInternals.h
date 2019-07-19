/*******************************************************************************
 |    bscfsInternals.h
 |
 |  © Copyright IBM Corporation 2015,2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 ******************************************************************************/


#ifndef __BSCFS_INTERNALS_H__
#define __BSCFS_INTERNALS_H__


/****************************************
 * structures for handling files in bbfs
 *
 ****************************************/
#include <pthread.h>
#include <map>
#include <aio.h>

#include "bbapi.h"

#include "bscfs_ioctl.h"
#include "bscfs_index.h"

/* a memory aligned buffer
 */
typedef struct data_buffer {
    int busy;
    struct aiocb io_req;
    char *buffer;
} data_buffer_t;

/* structure of an opened or active shared file */
typedef struct shared_file {
    pthread_mutex_t lock;          // lock on the shared file (local to
				   //     the node)
    int open_count;                // reference count for the number of
				   //     active opens 
    struct stat pfsstatcache;      // cached stat of pfs file at open
    bscfs_file_state_t state;      // file state
    int accmode;                   // flags used to open pfs_fd
    char *file_name;               // file name from mount point
    char *pfs_file_name;           // pfs shared file name
    char *map_file_name;           // map file name (in the PFS)
    char *cleanup_file_name;       // cleanup script name (in the PFS)
    char *data_file_name;          // name of the SSD data file
    char *index_file_name;         // name of the SSD index file
    int pfs_fd;                    // file desc of the shared file in the PFS,
				   //     -1 if not open
    int data_fd_write;             // file desc of the data file for writing,
				   //     -1 if not open
    int data_fd_read;              // file desc of the data file for reading,
				   //     -1 if not open
    int index_fd;                  // file desc of the index file,
				   //     -1 if not open
    uint64_t pfs_file_size;        // best local guess for the size of the
				   //     shared file
    uint64_t data_file_space;      // size of SSD space allocated for the data
				   //     file so far
    uint64_t data_file_size;       // number of bytes written to data file
				   //     (at least started)
    uint64_t data_file_offset;     // next offset in data file to be allocated
    uint64_t index_file_space;     // size of SSD space allocated for the index
				   //     file so far
    bscfs_index_t *index;          // mapping from the SSD data file to the
				   //     PFS shared file
    int current_write_buffer;      // selector for one of the two write buffers,
				   //     -1 means partial buffer is on SSD
    uint64_t write_buffer_offset;  // offset for next copy into current write
				   //     buffer
    int data_file_write_error;     // negative errno if we've had a write fail
    data_buffer_t write_buffer[2]; // buffers for writing
    data_buffer_t read_buffer[2];  // buffers for reading
    BBTransferHandle_t
	transfer_handle;           // transfer handle for flush or prefetch
} shared_file_t;

/* definition of a user data structure to keeps command line parameters
 * 
 */
typedef struct bscfs_data {
    char *bb_path;                 // path to user root directory on local SSD
    char *pfs_path;                // path to the user directory on the
				   //     parallel file system
    char *mount_path;              // path to the local mount point in the
				   //     local OS file system
    char *config_file;             // path to configuration file
    char *pre_install_list;        // path to file listing prefetched files
				   //     to install
    char *cleanup_list;            // path to file where post-draining files
				   //     are to be listed
    uint64_t write_buffer_size;    // size of in-memory data buffers used for
				   //     writing
    uint64_t read_buffer_size;     // size of in-memory data buffers used for
				   //     reading
    uint64_t data_falloc_size;     // chunk size for pre-allocating data file
				   //     SSD space
    uint64_t max_index_size;       // maximum index file size
    uint64_t node_number;          // node number within job's allocation
    uint64_t node_count;           // number of nodes in job's allocation
    uint64_t bb_path_len;          // strlen of bb_path
    uint64_t pfs_path_len;         // strlen of pfs_path
    uint64_t max_path_len;         // maximum length of BSCFS paths, starting
				   //     from the mount point
    int mount_fd;                  // file descriptor to the mount point,
				   //     used for chdir the fuse engine
    pthread_mutex_t
	shared_files_lock;         // lock that protects shared_files
    std::map<std::string, shared_file_t*>
	shared_files;              // list of all open or active shared files  
    BBTAG next_transfer_tag;       // tag to be used for next flush or prefetch
} bscfs_data_t;

extern bscfs_data_t bscfs_data; // global data

/*******************************************************************************
 * General definitions of all operations to implement for matching fuse FS
 * operations. Comments copied from fuse.h
 ******************************************************************************/

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 28
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <stddef.h>
#include <fuse.h>

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored. The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int bscfs_getattr(const char *path, struct stat *stbuf);

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
int bscfs_readdir(const char *path, void *buffer, fuse_fill_dir_t file_filler,
		  off_t offset, struct fuse_file_info *file_info);

/** Create a file node
 *
 * This is called for creation of all non-directory, non-symlink
 * nodes.  If the filesystem defines a create() method, then for
 * regular files that will be called instead.
 */
int bscfs_mknod(const char *name, mode_t mode, dev_t dev);

/** Remove a file */
int bscfs_unlink(const char *name);

/** Create a hard link to a file */
int bscfs_link (const char *source, const char *dest);


/** File open operation
 *
 * No creation (O_CREAT, O_EXCL) and by default also no
 * truncation (O_TRUNC) flags will be passed to open(). If an
 * application specifies O_TRUNC, fuse first calls truncate()
 * and then open(). Only if 'atomic_o_trunc' has been
 * specified and kernel version is 2.6.24 or later, O_TRUNC is
 * passed on to open.
 *
 * Unless the 'default_permissions' mount option is given,
 * open should check if the operation is permitted for the
 * given flags. Optionally open may also return an arbitrary
 * filehandle in the fuse_file_info structure, which will be
 * passed to all file operations.
 *
 * Changed in 2.2
 */
int bscfs_open(const char *name, struct fuse_file_info *file_info);

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes. An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
int bscfs_read(const char *name , char *buffer, size_t size, off_t offset,
		     struct fuse_file_info *file_info);

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error. An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
int bscfs_write(const char *name, const char *buffer, size_t size, off_t offset,
		      struct fuse_file_info *file_info);

/** Get file system statistics
 *
 * The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' fields are ignored
 *
 * Replaced 'struct statfs' parameter with 'struct statvfs' in
 * version 2.5
 */
int bscfs_statfs(const char *path, struct statvfs *stat_fs);

/** Possibly flush cached data
 *
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 *
 * NOTE: The flush() method may be called more than once for each
 * open().      This happens if more than one file descriptor refers
 * to an opened file due to dup(), dup2() or fork() calls.      It is
 * not possible to determine if a flush is final, so each flush
 * should be treated equally.  Multiple write-flush sequences are
 * relatively rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 *
 * Changed in version 2.2
 */
int bscfs_flush(const char *path, struct fuse_file_info *info );

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.      It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int bscfs_release(const char *path, struct fuse_file_info *file_info);

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */
void *bscfs_init(struct fuse_conn_info *conn);

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void bscfs_destroy(void *file_sys);


/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
int bscfs_create(const char *name, mode_t mode,
		 struct fuse_file_info *file_info);

/** Synchronize file contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data.
 *
 * Changed in version 2.2
 */
int bscfs_fsync(const char *name, int datasync,
		struct fuse_file_info *file_info);

/** Change the permission bits of a file */
int bscfs_chmod(const char *name, mode_t mode);

/** Change the owner and group of a file */
int bscfs_chown(const char *name, uid_t uid, gid_t gid);

/** Set extended attributes */
int bscfs_setxattr(const char *path, const char *name, const char *value,
		   size_t size, int flags);

/** Get extended attributes */
int bscfs_getxattr(const char *path, const char *name, char *value,
		   size_t size);

/** List extended attributes */
int bscfs_listxattr(const char *name, char *list, size_t size);

/** Remove extended attributes */
int bscfs_removexattr(const char *path, const char *name);

/** Open directory
 *
 * Unless the 'default_permissions' mount option is given,
 * this method should check if opendir is permitted for this
 * directory. Optionally opendir may also return an arbitrary
 * filehandle in the fuse_file_info structure, which will be
 * passed to readdir, closedir and fsyncdir.
 *
 * Introduced in version 2.3
 */
int bscfs_opendir(const char *name, struct fuse_file_info *file_info);

/** Release directory
 *
 * Introduced in version 2.3
 */
int bscfs_releasedir(const char *name, struct fuse_file_info *file_info);

/**
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 * This method is not called under Linux kernel versions 2.4.x
 *
 * Introduced in version 2.5
 */
int bscfs_access(const char *name, int mask);

/**
 * Get attributes from an open file
 *
 * This method is called instead of the getattr() method if the
 * file information is available.
 *
 * Currently this is only called after the create() method if that
 * is implemented (see above).  Later it may be called for
 * invocations of fstat() too.
 *
 * Introduced in version 2.5
 */
int bscfs_fgetattr(const char *name, struct stat *statbuf,
		   struct fuse_file_info *file_info);

/**
 * Perform POSIX file locking operation
 *
 * The cmd argument will be either F_GETLK, F_SETLK or F_SETLKW.
 *
 * For the meaning of fields in 'struct flock' see the man page
 * for fcntl(2).  The l_whence field will always be set to
 * SEEK_SET.
 *
 * For checking lock ownership, the 'fuse_file_info->owner'
 * argument must be used.
 *
 * For F_GETLK operation, the library will first check currently
 * held locks, and if a conflicting lock is found it will return
 * information without calling this method.      This ensures, that
 * for local locks the l_pid field is correctly filled in.      The
 * results may not be accurate in case of race conditions and in
 * the presence of hard links, but it's unlikly that an
 * application would rely on accurate GETLK results in these
 * cases.  If a conflicting lock is not found, this method will be
 * called, and the filesystem may fill out l_pid by a meaningful
 * value, or it may leave this field zero.
 *
 * For F_SETLK and F_SETLKW the l_pid field will be set to the pid
 * of the process performing the locking operation.
 *
 * Note: if this method is not implemented, the kernel will still
 * allow file locking to work locally.  Hence it is only
 * interesting for network filesystems and similar.
 *
 * Introduced in version 2.6
 */
// Intentionally not implemented. Fuse will use mknod/open instead.
//int bscfs_lock(const char *name, struct fuse_file_info *file_info, int cmd,
//              struct flock *lock);

/** Change the size of a file */
int bscfs_truncate(const char *name, off_t offset);

/**
 * Change the size of an open file
 *
 * This method is called instead of the truncate() method if the
 * truncation was invoked from an ftruncate() system call.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the truncate() method will be
 * called instead.
 *
 * Introduced in version 2.5
 */
int bscfs_ftruncate (const char *name, off_t offset,
		     struct fuse_file_info *file_info);

/** Read the target of a symbolic link
 *
 * The buffer should be filled with a null terminated string.  The
 * buffer size argument includes the space for the terminating
 * null character.      If the linkname is too long to fit in the
 * buffer, it should be truncated.      The return value should be 0
 * for success.
 */
int bscfs_readlink (const char *name, char *buffer, size_t size);

/** Rename a file */
int bscfs_rename(const char *oldname, const char *newname);

/** Create a directory
 *
 * Note that the mode argument may not have the type specification
 * bits set, i.e. S_ISDIR(mode) can be false.  To obtain the
 * correct directory type bits use  mode|S_IFDIR
 * */
int bscfs_mkdir(const char *name, mode_t mode);

/** Remove a directory */
int bscfs_rmdir(const char *name);

/** Create a symbolic link */
int bscfs_symlink(const char *dest_name , const char *src_name);

/** Synchronize directory contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data
 *
 * Introduced in version 2.3
 */
int bscfs_fsyncdir(const char *name, int sync,
		   struct fuse_file_info *file_info);


/**
 * Change the access and modification times of a file with
 * nanosecond resolution
 *
 * Introduced in version 2.6
 */
int bscfs_utimens(const char *name , const struct timespec tv[2]);

/**
 * Map block index within file to block index within device
 *
 * Note: This makes sense only for block device backed filesystems
 * mounted with the 'blkdev' option
 *
 * Introduced in version 2.6
 */
int bscfs_bmap(const char *name, size_t blocksize, uint64_t *idx);

/**
 * Ioctl
 *
 * flags will have FUSE_IOCTL_COMPAT set for 32bit ioctls in
 * 64bit environment.  The size and direction of data is
 * determined by _IOC_*() decoding of cmd.  For _IOC_NONE,
 * data will be NULL, for _IOC_WRITE data is out area, for
 * _IOC_READ in area and if both are set in/out area.  In all
 * non-NULL cases, the area is of _IOC_SIZE(cmd) bytes.
 *
 * Introduced in version 2.8
 */
int bscfs_ioctl(const char *name , int cmd, void *arg,
		struct fuse_file_info *file_info,
		unsigned int flags, void *data);

/**
 * Poll for IO readiness events
 *
 * Note: If ph is non-NULL, the client should notify
 * when IO readiness events occur by calling
 * fuse_notify_poll() with the specified ph.
 *
 * Regardless of the number of times poll with a non-NULL ph
 * is received, single notification is enough to clear all.
 * Notifying more times incurs overhead but doesn't harm
 * correctness.
 *
 * The callee is responsible for destroying ph with
 * fuse_pollhandle_destroy() when no longer in use.
 *
 * Introduced in version 2.8
 */
int bscfs_poll(const char *name, struct fuse_file_info *file_info,
              struct fuse_pollhandle *ph, unsigned *reventsp);

/*
 * Explicit BSCFS operations
 */
int bscfs_start_local_flush(void *data);
int bscfs_prepare_local_flush(void *data);
int bscfs_start_local_prefetch(void *data);
int bscfs_check_local_transfer(void *data);
int bscfs_global_flush_completed(void *data);
int bscfs_forget(void *data);
int bscfs_query_internal_files(void *data);
int bscfs_install_internal_files(void *data);
int bscfs_get_parameter(void *data);

#endif //__BSCFS_INTERNALS_H__
