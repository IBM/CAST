/*******************************************************************************
 |    fshipcld.h
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



//! \file   fshipcld.h
//! \author Mike Aho <meaho@us.ibm.com>
//! \date   Fri Nov 18 10:42:00 2016
//! 
//! \brief  Expected messages from/to fuse module 2.9.
//!         inMsg indicates inbound from fuse module (read of device).
//!         outMsg indicates outbound message to fuse module (write to device).
//!         Also has special messages between function-ship daemons.
//! \note   See fuse kernel module to understand fuse message formats
//! 
//! 
//!


#ifndef FSHIPCLD_H
#define FSHIPCLD_H


#include <stdlib.h>

#include <linux/types.h>
#include <linux/fuse.h>
#include <sys/stat.h>
#include <sys/vfs.h> 


#define FSHIPCLD_VERSIONSTR "Bringup" ///< Version string for fshipcld


/**
	\brief Fetch the version string
	\par Description
	The fshipcld_GetVersion routine returns the version string for fshipcld.
	This routine is intended for version mismatch debug.

	\param[in] pSize The amount of space provided to hold pVersion
	\param[out] pVersion The string containing the expected version.

	\return Error code
	\retval 0 Success
	\retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
*/
extern int fshipcld_GetVersion(size_t pSize, char* pVersion);
extern uint32_t dump_init_flags(uint32_t flags);


typedef struct fuse_out_header fuse_out_header;
typedef struct fuse_in_header fuse_in_header;
typedef struct fuse_out_header fuse_out_notify;

typedef struct outMsgGenericNotify{
        fuse_out_notify notify;
        outMsgGenericNotify(__s32 pNotifyCode){ notify.len=sizeof(outMsgGenericNotify); notify.unique=0; notify.error=pNotifyCode;}
}outMsgGenericNotify;

/*struct fuse_notify_inval_entry_out {
	uint64_t	parent;
	uint32_t	namelen;
	uint32_t	padding;
};*/

typedef struct OutMsgPollNotify{
  fuse_out_notify notify;
  struct fuse_notify_poll_wakeup_out poll_wakeup_out;
  OutMsgPollNotify(uint64_t in_kh){notify.len=sizeof(OutMsgPollNotify); notify.unique=0; notify.error=FUSE_NOTIFY_POLL; poll_wakeup_out.kh=in_kh;}
}OutMsgPollNotify;

// if the entry is found, will invalidate the entry (kernel dentry)
// will attempt to remove the dentry cache element in the kernel
// will NOT remove if the mountpoint (EBUSY returned)
// will NOT remove if the entry is for a nonempty directory, BUT will do a shrink_dcache_parent (ENOTEMPTY returned)
// if no kernel inode
typedef struct outMsgInvalEntryNotify{
        fuse_out_notify notify;
        fuse_notify_inval_entry_out inval_entry_out;
        char name[0];
        outMsgInvalEntryNotify(){ notify.len=sizeof(outMsgInvalEntryNotify); notify.unique=0; notify.error=FUSE_NOTIFY_INVAL_ENTRY;}
}outMsgInvalEntryNotify;

typedef struct inMsgGeneric {
	struct fuse_in_header hdr;
	char argsIn[0];
} inMsgGeneric;

typedef struct outMsgGeneric {
	struct fuse_out_header hdr;
	char argsOut[0];
inline outMsgGeneric(__u64 unique , __s32 error=0) {hdr.len=sizeof(fuse_out_header);hdr.error=error;hdr.unique=unique;}
} outMsgGeneric;

typedef  struct inMsgInit {//40+
	struct fuse_in_header hdr;
	struct fuse_init_in init_in;
} inMsgInit;

/**
 * INIT request/reply flags
 *
 * FUSE_ASYNC_READ: asynchronous read requests
 * FUSE_POSIX_LOCKS: remote locking for POSIX file locks
 * FUSE_FILE_OPS: kernel sends file handle for fstat, etc... (not yet supported)
 * FUSE_ATOMIC_O_TRUNC: handles the O_TRUNC open flag in the filesystem
 * FUSE_EXPORT_SUPPORT: filesystem handles lookups of "." and ".."
 * FUSE_BIG_WRITES: filesystem can handle write size larger than 4kB
 * FUSE_DONT_MASK: don't apply umask to file mode on create operations
 * FUSE_SPLICE_WRITE: kernel supports splice write on the device
 * FUSE_SPLICE_MOVE: kernel supports splice move on the device
 * FUSE_SPLICE_READ: kernel supports splice read on the device
 * FUSE_FLOCK_LOCKS: remote locking for BSD style file locks <<<Not supporting in CORAL
 * FUSE_HAS_IOCTL_DIR: kernel supports ioctl on directories
 * FUSE_AUTO_INVAL_DATA: automatically invalidate cached pages
 * FUSE_DO_READDIRPLUS: do READDIRPLUS (READDIR+LOOKUP in one)
 * FUSE_READDIRPLUS_AUTO: adaptive readdirplus
 * FUSE_ASYNC_DIO: asynchronous direct I/O submission
 * coming soon
 * FUSE_WRITEBACK_CACHE: use writeback cache for buffered writes
 * FUSE_NO_OPEN_SUPPORT: kernel supports zero-message opens
 * FUSE_PARALLEL_DIROPS: allow parallel lookups and readdir
 */
typedef struct outMsgInit {///< response to fuse_init from fuse module
	struct fuse_out_header hdr;
	struct fuse_init_out init_out;
	inline outMsgInit (__u64 unique ,__u32 pMaxWrite, __s32 error=0) {
		hdr.len=sizeof(outMsgInit);
		hdr.error=error;
		hdr.unique=unique;
		init_out.major = FUSE_KERNEL_VERSION;
		init_out.minor = FUSE_KERNEL_MINOR_VERSION;
		init_out.max_readahead = 0; /* No readahead caching by kernel */
		init_out.flags = FUSE_POSIX_LOCKS
					   | FUSE_FILE_OPS
					   | FUSE_DONT_MASK
					   | FUSE_AUTO_INVAL_DATA
					   | FUSE_ATOMIC_O_TRUNC    //pass any O_TRUNC flag on open; if not used, see ftruncate() before open()
					   | FUSE_DO_READDIRPLUS
	                                   | FUSE_BIG_WRITES
		;


		init_out.max_background = 0;            /* use fuse module default which would be FUSE_DEFAULT_MAX_BACKGROUND (inode.c) */
		init_out.congestion_threshold=0;        /* use module default FUSE_DEFAULT_CONGESTION_THRESHOLD (inode.c) */
		init_out.max_write = pMaxWrite;           /* default */

	}
} outMsgInit;

typedef  struct inMsgInterrupt {
	struct fuse_in_header hdr;
	struct fuse_interrupt_in interrupt_in;
} inMsgInterrupt;

typedef struct fuse_attr_out attr_out;

typedef struct inMsgGetattr {
	fuse_in_header hdr;
	struct fuse_getattr_in getattr_in;
} inMsgGetattr;

typedef  struct outMsgGetattr {
	fuse_out_header hdr;
	attr_out getattr_out;
	inline outMsgGetattr(__u64 unique , __s32 error=0) {hdr.len=sizeof(outMsgGetattr);hdr.error=error;hdr.unique=unique;}
} outMsgGetattr;



typedef  struct inMsgReaddir {
	struct fuse_in_header hdr;
	struct fuse_read_in read_in;
} inMsgReaddir;

typedef struct outMsgReaddir {
	struct fuse_out_header hdr;
	inline outMsgReaddir(__u64 unique , __s32 error=0) {hdr.len=sizeof( outMsgReaddir);hdr.error=error;hdr.unique=unique;}
} outMsgReaddir;

typedef struct outMsgReaddirPlus {
	struct fuse_out_header hdr;
	inline outMsgReaddirPlus(__u64 unique , __s32 error=0) {hdr.len=sizeof( outMsgReaddirPlus);hdr.error=error;hdr.unique=unique;}
} outMsgReaddirPlus;


/*
struct fuse_write_out {
	__u32	size;
	__u32	padding;
};
*/

typedef  struct inMsgReleasedir {
	struct fuse_in_header hdr;
	struct fuse_release_in release_in;
} inMsgReleasedir;

typedef  struct inMsgLookup{
	struct fuse_in_header hdr;
	char lookupname[0];
} inMsgLookup;

typedef  struct outMsgLookup{
	struct fuse_out_header hdr;
	struct fuse_entry_out entry_out;
	inline outMsgLookup(__u64 unique , __s32 error=0) {
            hdr.len=sizeof( outMsgLookup);
            hdr.error=error;
            hdr.unique=unique;
        }
} outMsgLookup;

typedef struct inMsgCreate{
	struct fuse_in_header hdr;
	struct fuse_create_in create_in;
	char name[0];
} inMsgCreate;


typedef  struct outMsgCreate {
	struct fuse_out_header hdr;
	struct fuse_entry_out entry_out;
	struct fuse_open_out open_out;
	inline outMsgCreate(__u64 unique , __s32 error=0) {hdr.len=sizeof(outMsgCreate);hdr.error=error;hdr.unique=unique; entry_out.nodeid=0;}
} outMsgCreate;

typedef struct inMsgMknod{
	struct fuse_in_header hdr;
	struct fuse_mknod_in mknod_in;
	char name[0];
} inMsgMknod;

typedef  struct outMsgMknod {
	struct fuse_out_header hdr;
	struct fuse_entry_out entry_out;
	inline outMsgMknod(__u64 unique , __s32 error=0) {hdr.len=sizeof(outMsgMknod);hdr.error=error;hdr.unique=unique; entry_out.nodeid=0;}
} outMsgMknod;

typedef  struct inMsgOpen {
	struct fuse_in_header hdr;
	struct fuse_open_in open_in;
} inMsgOpen;

typedef  struct outMsgOpen {
	struct fuse_out_header hdr;
	struct fuse_open_out open_out;
inline outMsgOpen(__u64 unique , __s32 error=0) {hdr.len=sizeof(outMsgOpen);hdr.error=error;hdr.unique=unique;}
} outMsgOpen;

typedef  struct inMsgUnlinkAtName{
	struct fuse_in_header hdr;
	char unlinkatname[0];
} inMsgUnlinkAtName;

typedef  struct outMsgUnlinkAtName{
	struct fuse_out_header hdr;
	__u64 unlinkedInode;
	inline outMsgUnlinkAtName(__u64 unique , __s32 error=0) {hdr.len=sizeof( outMsgLookup);hdr.error=error;hdr.unique=unique;unlinkedInode=0;}
} outMsgUnlinkAtName;

//  notes flags in fuse_release_in are from kernel "struct file" fs.h
//  release_flags has FUSE_RELEASE_FLUSH as a possible value (fuse.h)
typedef struct inMsgRelease{
	struct fuse_in_header hdr;
	struct fuse_release_in release_in;
} inMsgRelease;

typedef struct inMsgPread{//40+40
	struct fuse_in_header hdr;
	struct fuse_read_in read_in;
} inMsgPread;


typedef struct outMsgPread {
	struct fuse_out_header hdr;
	inline outMsgPread(__u64 unique , __s32 error=0) {hdr.len=sizeof( outMsgPread);hdr.error=error;hdr.unique=unique;}
} outMsgPread;

typedef struct inMsgPwrite{//40+40
	struct fuse_in_header hdr;
	struct fuse_write_in write_in;
	char data[0];
} inMsgPwrite;

typedef struct outMsgPwrite {
	struct fuse_out_header hdr;
	struct fuse_write_out write_out;
	inline outMsgPwrite(__u64 unique , __s32 error=0) {hdr.len=sizeof( outMsgPwrite);hdr.error=error;hdr.unique=unique;}
} outMsgPwrite;

typedef struct inMsgFlush{
	struct fuse_in_header hdr;
	struct fuse_flush_in flush_in;
} inMsgFlush;

typedef struct inMsgFsync{
	struct fuse_in_header hdr;
	struct fuse_fsync_in fsync_in;
} inMsgSync;

/* Unlock on close is handled by the flush method */
/* for FUSE_GETLK, FUSE_SETLK, and FUSE_SETLKW */
typedef struct inMsgLock{
	struct fuse_in_header hdr;
	struct fuse_lk_in lock_in;
} inMsgLock;

typedef struct  outMsgGetLock{
   struct fuse_out_header hdr;
   struct fuse_lk_out lock_out;
   inline outMsgGetLock(__u64 unique , __s32 error=0) {hdr.len=sizeof(outMsgGetLock);hdr.error=error;hdr.unique=unique;}
} outMsgGetLock;

typedef struct fuse_file_lock fflock;

typedef struct inMsgSetattr {
	struct fuse_in_header hdr;
	struct fuse_setattr_in setattr_in;
} inMsgSetattr;

typedef struct outMsgSetattr {
	struct fuse_out_header hdr;
	struct fuse_attr_out attr_out;
	inline outMsgSetattr(__u64 unique , __s32 error=0) {hdr.len=sizeof( outMsgSetattr);hdr.error=error;hdr.unique=unique;}
} outMsgSetattr;

typedef struct inMsgAccess{
	struct fuse_in_header hdr;
	struct fuse_access_in access_in;
} inMsgAccess;

typedef struct inMsgMkdir{
	struct fuse_in_header hdr;
	struct fuse_mkdir_in mkdir_in;
	char name[0];
} inMsgMkdir;

typedef  struct outMsgMkdir {
	struct fuse_out_header hdr;
	struct fuse_entry_out entry_out;
inline outMsgMkdir(__u64 unique , __s32 error=0) {hdr.len=sizeof(outMsgMkdir);hdr.error=error;hdr.unique=unique; entry_out.nodeid=0;}
} outMsgMkdir;

typedef struct memItem{
	char * address;
	uint64_t length;
        memItem* next;
        uint64_t chunkSize;
	uint64_t handle;
        uint32_t opcode;
	uint32_t lkey;
	uint32_t rkey;
	uint32_t crc32;

  memItem(char * pAddr, int pLength,uint64_t pChunkSize) : address(pAddr),length(pLength),next(NULL),chunkSize(pChunkSize),handle(0),opcode(0),lkey(0),rkey(0),crc32(0){};
  void reset2basic(){ length=0; next=NULL; crc32=0; opcode=0;};

} memItem;

typedef memItem * memItemPtr;

typedef struct inMsgRename{
	struct fuse_in_header hdr;
	__u64 newdirInode;  //nodeid in hdr is old dir Inode
	char oldname[0];
	char newname[0];
} inMsgRename;

typedef struct inMsgRename2{
	struct fuse_in_header hdr;
	struct fuse_rename2_in rename2_in;//newdirInode and flags
	char oldname[0];
	char newname[0];
} inMsgRename2;
/*
struct fuse_rename2_in {
	uint64_t	newdir;
	uint32_t	flags;
	uint32_t	padding;
};
struct fuse_getxattr_in {
	uint32_t	size; //size of buffer in getxattr
	uint32_t	padding;
};

struct fuse_getxattr_out {
	uint32_t	size;  //return value on xattr
	uint32_t	padding;
};
*/
// http://lxr.free-electrons.com/source/fs/fuse/dir.c#L1768 
// http://man7.org/linux/man-pages/man2/fgetxattr.2.html
typedef struct inMsgGetXattr{
	struct fuse_in_header hdr;
        struct fuse_getxattr_in getxattr_in;
        char nameOfAttribute[0];
	
} inMsgGetXattr;

typedef struct outMsgGetXattr{
	struct fuse_out_header hdr;
        struct fuse_getxattr_out getxattr_out;
        char attributeData[0]; //data follows of length getxattr_out.size
	inline outMsgGetXattr(__u64 unique , __s32 error=0) {hdr.len=sizeof(outMsgGetXattr);hdr.error=error;hdr.unique=unique;}
} outMsgGetXattr;

typedef struct inMsgListXattr{
	struct fuse_in_header hdr;
        struct fuse_getxattr_in getxattr_in;
} inMsgListXattr;

typedef struct inMsgRemoveXattr{
	struct fuse_in_header hdr;
        char nameOfAttribute[0];	
} inMsgRemoveXattr;

/*
struct fuse_setxattr_in {
	uint32_t	size;
	uint32_t	flags;
};
*/

typedef struct inMsgSetXattr{
	struct fuse_in_header hdr;
        struct fuse_setxattr_in setxattr_in;
        char nameOfAttribute[0];
        char data[0];
	
} inMsgSetXattr;

typedef struct outMsgSetXattr{
	struct fuse_out_header hdr;
	inline outMsgSetXattr(__u64 unique , __s32 error=0) {hdr.len=sizeof(outMsgSetXattr);hdr.error=error;hdr.unique=unique;}
} outMsgSetXattr;


typedef struct inMsgSymlink{
	struct fuse_in_header hdr;
	char oldname[0];
	char newname[0];
} inMsgSymlink;

typedef  struct outMsgSymlink {
	struct fuse_out_header hdr;
	struct fuse_entry_out entry_out;
	inline outMsgSymlink(__u64 unique , __s32 error=0) {hdr.len=sizeof(outMsgSymlink);hdr.error=error;hdr.unique=unique;}
} outMsgSymlink;

typedef  struct outMsgReadlink {
	struct fuse_out_header hdr;
	char link[0];
	inline outMsgReadlink(__u64 unique , __s32 error=0) {hdr.len=sizeof(outMsgReadlink);hdr.error=error;hdr.unique=unique;}
} outMsgReadlink;

typedef  struct outMsgStatfs {
	struct fuse_out_header hdr;
	struct fuse_kstatfs kstatfs;
	inline outMsgStatfs(__u64 unique , __s32 error=0) {hdr.len=sizeof(*this);hdr.error=error;hdr.unique=unique;}
} outMsgStatfs;

typedef struct stat4Name{
	struct stat nStat;
} stat4Name;

typedef struct renameOffset{
	__u64 offset2newname;
	__u64 offset2oldname;
} renameOffset;


typedef struct inMsgFallocate{
	struct fuse_in_header hdr;
	struct fuse_fallocate_in fallocate_in;
} inMsgFallocate;
/*
struct fuse_forget_in {
	uint64_t	nlookup;
};

struct fuse_batch_forget_in {
	uint32_t	count;
	uint32_t	dummy;
};
struct fuse_forget_one {
	uint64_t	nodeid;
	uint64_t	nlookup; 
};*/
//The nlookup parameter indicates the number of lookup
//previously performed on this inode.
//see fuse_lowlevel.h doxy 

typedef struct inMsgBatchForget{
	struct fuse_in_header hdr;
	struct fuse_batch_forget_in forget_in;
	struct fuse_forget_one forget[0];   //basing pointer available as forget address
} inMsgBatchForget;


typedef struct inMsgForget{
	struct fuse_in_header hdr;
	struct fuse_forget_in forget_in;
} inMsgForget;

typedef struct inHello{
	int max_read;
	int version_major;
	int version_minor;
	pid_t daemon_pid;
	uint64_t generation;	/* Inode generation: nodeid:gen must be unique for the fs's lifetime */
	uint64_t entry_valid;   /* Cache timeout for the name */
	uint64_t attr_valid;	/* Cache timeout for the attributes */
	uint32_t entry_valid_nsec;
	uint32_t attr_valid_nsec;
        uint32_t openOutFlags;
} inHello;

typedef struct outHello{
        int outHelloVersion;
	int fuse_version_major;
	int fuse_version_minor;
	pid_t daemon_pid;
        int statfsRC;
        struct statfs sfs;  
        outHello(pid_t inPid){ 
            fuse_version_major=FUSE_KERNEL_VERSION; 
            fuse_version_minor=FUSE_KERNEL_MINOR_VERSION;
             outHelloVersion=1;
        }
} outHello;
/*
struct fuse_link_in {
	uint64_t	oldnodeid;
};
http://linux.die.net/man/3/link
*/
typedef struct inMsgLink{
	struct fuse_in_header hdr;
	struct fuse_link_in link_in; //fuse_link_in.oldnodeid of previous file
        char   newname[0];
} inMsgLink;

typedef  struct outMsgLink{
	struct fuse_out_header hdr;
	struct fuse_entry_out entry_out;
	inline outMsgLink(__u64 unique , __s32 error=0) {hdr.len=sizeof( outMsgLink);hdr.error=error;hdr.unique=unique;}
} outMsgLink;
/*
struct fuse_ioctl_in {
	uint64_t	fh;
	uint32_t	flags;
	uint32_t	cmd;
	uint64_t	arg;
	uint32_t	in_size;
	uint32_t	out_size;
};

struct fuse_ioctl_iovec {
	uint64_t	base;
	uint64_t	len;
};

struct fuse_ioctl_out {
	int32_t		result;
	uint32_t	flags;
	uint32_t	in_iovs;
	uint32_t	out_iovs;
};
http://linux.die.net/man/2/ioctl
#include <sys/ioctl.h>
int ioctl(int d, int request, ...); 

http://man7.org/linux/man-pages/man2/ioctl_list.2.html
*/
typedef struct inMsgIoctl{
	struct fuse_in_header hdr;
	struct fuse_ioctl_in ioctl_in; //fuse_link_in.oldnodeid of previous file
        char data[0];
} inMsgIoctl;

/*--------------
struct fuse_poll_in {
	uint64_t	fh;
	uint64_t	kh;
	uint32_t	flags;
	uint32_t	events;
};
struct fuse_poll_out {
	uint32_t	revents;
	uint32_t	padding;
};
struct fuse_notify_poll_wakeup_out {
	uint64_t	kh;
};
 * Poll flags
 * FUSE_POLL_SCHEDULE_NOTIFY: request poll notify
#define FUSE_POLL_SCHEDULE_NOTIFY (1 << 0)
*/
typedef struct inMsgPoll{
	struct fuse_in_header hdr;
	struct fuse_poll_in poll_in; 
} inMsgPoll;

typedef  struct outMsgPoll{
	struct fuse_out_header hdr;
	struct fuse_poll_out poll_out;
	inline outMsgPoll(__u64 unique , __s32 error=0) {hdr.len=sizeof( outMsgPoll);hdr.error=error;hdr.unique=unique;}
} outMsgPoll;

typedef struct configurationPath {
  char configPath[0];
}configurationPath;

#define SIGNALMSG_RESPONSE_REQD 1
#define SIGNALMSG_TERMINATE_IMM 2
typedef struct signalMsg {
   int signal;
   int flags;
   int secondsCleanup;
   signalMsg(int pSig, int pFlags=SIGNALMSG_RESPONSE_REQD, int pSeconds=0){signal=pSig; flags=pFlags; secondsCleanup=pSeconds;}
} signalMsg;

#define WAKEUP_SIGNAL 1 
#define WAKEUP_RESPONSETHREADEND 2

typedef struct wakeupPipeMsg {
   int id;
   int signalSent;
   wakeupPipeMsg(){wakeupPipeMsg(0);}
   wakeupPipeMsg(int pID, int pSignalSent=0){id=pID; signalSent=pSignalSent;}
} wakeupPipeMsg;

#endif /* FSHIPCLD_H */
