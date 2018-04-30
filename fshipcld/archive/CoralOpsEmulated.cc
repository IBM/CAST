/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/* ================================================================ */
/*                                                                  */
/* Licensed Materials - Property of IBM                             */
/*                                                                  */
/* Blue Gene/Q                                                      */
/*                                                                  */
/* (C) Copyright IBM Corp.  2015                              */
/*                                                                  */
/* US Government Users Restricted Rights -                          */
/* Use, duplication or disclosure restricted                        */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/* This software is available to you under the                      */
/* --License--                                                      */
/*                                                                  */
/* ================================================================ */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */

//! \file  CoralOps.C
//! \brief class CoralOps

#include <stdio.h>
#include "CoralOpsEmulated.h"
#include "fshipMacros.h"
#include "fshipmount.h"

__s64 CoralOpsEmulated::processMsg(inMsgGeneric * in){
//printf("Method %s opcode=%lld\n",__PRETTY_FUNCTION__, (long long int)in->hdr.opcode);
__s64 retval=0;
switch(in->hdr.opcode){
  case FUSE_LOOKUP:    
    retval=lookup_op(in); break;  /* no reply */
  case FUSE_FORGET:    
    retval=forget_op(in); break;
  case FUSE_GETATTR:   
    retval=getattr_op(in); break;
  case FUSE_SETATTR:   
    retval=setattr_op(in); break; 
  case FUSE_READLINK:  
    retval=readlink_op(in); break; 
  case FUSE_SYMLINK:   
    retval=symlink_op(in); break;  
  case FUSE_MKNOD:	    
    retval=mknod_op(in); break;    
  case FUSE_MKDIR:	    
    retval=mkdir_op(in); break;
  case FUSE_UNLINK:    
    retval=unlink_op(in); break; 
  case FUSE_RENAME:    
    retval=rename_op(in); break; 
  case FUSE_LINK:	    
    retval=link_op(in); break;	
  case FUSE_OPEN:	    
    retval=open_op(in); break;
  case FUSE_READ:	    
    retval=read_op(in); break;
  case FUSE_WRITE:	    
    retval=write_op(in); break;
  case FUSE_STATFS:    
    retval=statfs_op(in); break;
  case FUSE_RELEASE:   
    retval=release_op(in); break;
  case FUSE_FSYNC:	    
    retval=fsync_op(in); break;  
  case FUSE_SETXATTR:  
    retval=setxattr_op(in); break; 
  case FUSE_GETXATTR:  
    retval=getxattr_op(in); break; 
  case FUSE_LISTXATTR: 
    retval=listxattr_op(in); break;
  case FUSE_REMOVEXATTR: 
    retval=removexattr_op(in); break; 
  case FUSE_FLUSH:	    
    retval=flush_op(in); break;
  case FUSE_INIT:	    
    retval=init_op(in); 
    printf("init retval=%lld opcode=%lld\n",(long long int)retval,(long long int)in->hdr.opcode);
    break;
  case FUSE_OPENDIR:   
    retval=opendir_op(in); break;
  case FUSE_READDIR:   
    retval=readdir_op(in); break;
  case FUSE_RELEASEDIR:  
    retval=releasedir_op(in); break;
  case FUSE_FSYNCDIR:  
    retval=fsyncdir_op(in); break;
  case FUSE_GETLK:	    
    retval=getlk_op(in); break;   
  case FUSE_SETLK:	    
    retval=setlk_op(in); break;      
  case FUSE_SETLKW:    
    retval=setlkw_op(in); break;    
  case FUSE_ACCESS:    
    retval=access_op(in); break;
  case FUSE_CREATE:    
    retval=create_op(in); break;
  case FUSE_INTERRUPT: 
    retval=interrupt_op(in); break;
  case FUSE_BMAP:	    
    retval=bmap_op(in); break;
  case FUSE_DESTROY:   
    retval=destroy_op(in); break;  
  case FUSE_IOCTL:	    
    retval=ioctl_op(in); break;
 
  case FUSE_POLL:	   
    retval=poll_op(in); break; 
  case FUSE_FALLOCATE: 
    retval=fallocate_op(in); break;	

  case FUSE_READDIRPLUS:  
    retval=readdirplus_op(in); break;
default:
      retval=opcode_error(in); 
      printf("invalid opcode=%lld\n",(long long int)in->hdr.opcode);
break;
}//end switch

return retval;
};

 int CoralOpsEmulated::mountDevice(){
    int returnCode=0;
    char temp[256];
    snprintf(temp,sizeof(temp),"emulatedOnDir=%s",_emulatePath);
    _deviceFD = mount_fship(_mountDir,temp);
    if (_deviceFD == -ENOTCONN){ 
      /*printf("retrying device open and mount. \n");*/ /* this could be from doing control-C in bash when running from typescript??? */
      _deviceFD = mount_fship(_mountDir,temp);
      /* \TODO should do RAS or logging on this */
    }
    //* \TODO need to put info into mtab so that command mount shows more info than just cat /proc/mounts
    if (_deviceFD > 0){
       //create root node
       _rootNodePtr = new NodeNameEmulatedRoot(_mountDir,_emulatePath);
    }
    return returnCode;
};
 __s64 CoralOpsEmulated::unmountDevice(){
    __s64 retval = umount2(_mountDir, 2); 
    if (retval) {
        retval = -errno;
        printf("umount2 encountered errno=%d which is %s\n",errno, strerror(errno)); //need RAS log
    }
   return retval;
};


 int CoralOpsEmulated::readMonitorFuseDevice(){
  const int MSGAREASIZE = 2*64*1024; // \TODO: put in memory management
  char msgArea[MSGAREASIZE];
 struct pollfd poll_device;
  poll_device.fd = _deviceFD;
  poll_device.events = POLLIN;
  int pollNow = 0;
  int rc = 0;
  int rc_ssize=0;
  int connect_wait_timeout = -1; //minimum number of milliseconds poll will block 
  for (;;){

    if (pollNow){
	    poll_device.revents = 0;
	    rc = poll(&poll_device,1,connect_wait_timeout);
	    if (rc == -1) {
		 int err = errno;
		 if (err == EINTR) continue;
		 else {
		    printf("died on poll_device errno=%d %s \n",err,strerror(err) ); 
		    return rc; /* \TODO: need to do umount */
		 }
	     }
	    if (poll_device.revents & (POLLERR|POLLHUP|POLLNVAL) ) {//error!
	      printf("died on poll_device with bits POLLERR|POLLHUP|POLLNVAL");
	      return -1;
	    }
    }
    rc_ssize =readFuseMessage(msgArea, MSGAREASIZE);
    
    if (rc_ssize < 0) {
      if (-rc_ssize == EWOULDBLOCK){
         pollNow=1;
         continue;
      }
      if (-rc_ssize == EINTR){
         pollNow=0;
         continue;
      }     
      printf("read_message had rc=%lld\n",(long long int)rc_ssize);   
      break;
    }
    else if (rc_ssize > 0){
        inMsgGeneric * in = (inMsgGeneric *)msgArea;
        logHeader(in);
        printf("size of message is rc_ssize=%lld\n",(long long int)rc_ssize);
        __s64 retcode =  processMsg( in );
        printf("processMsg retcode = %lld \n",(long long int)retcode); 
        //clear length and op code fields in region
        memset(in,0,sizeof(inMsgGeneric));
        pollNow=0;      
    }
    else (pollNow=1);
  } 
  return 0;
}

/** Maximum number of outstanding background requests */
#define FUSE_DEFAULT_MAX_BACKGROUND 12

/** Congestion starts at 75% of maximum */
#define FUSE_DEFAULT_CONGESTION_THRESHOLD (FUSE_DEFAULT_MAX_BACKGROUND * 3 / 4)
/**
 * INIT request/reply flags
 *
 * FUSE_EXPORT_SUPPORT: filesystem handles lookups of "." and ".."
 * FUSE_DONT_MASK: don't apply umask to file mode on create operations
 * FUSE_AUTO_INVAL_DATA: automatically invalidate cached pages
 * FUSE_DO_READDIRPLUS: do READDIRPLUS (READDIR+LOOKUP in one)
 * FUSE_READDIRPLUS_AUTO: adaptive readdirplus
 * FUSE_ASYNC_DIO: asynchronous direct I/O submission

https://bugzilla.redhat.com/show_bug.cgi?id=963258
fuse supports real async. direct I/O as of linux 3.10, controlled via the new FUSE_ASYNC_DIO init flag. This functionality improves performance of certain direct I/O workloads (even in the synchronous case by optimistically sending larger requests in parallel). Enable fuse async direct I/O for gluster.

 */

__s64 CoralOpsEmulated::init_op(inMsgGeneric * in ){
    ssize_t retval = 0;
    outMsgInit outMsg(in->hdr.unique);
    inMsgInit * inMsg = (inMsgInit *)in;
    printf("Fuse init inMsg major=%ld minor=%ld readahead=%ld flags=%ld \n",(_LI)inMsg->init_in.major,(_LI)inMsg->init_in.minor,(_LI)inMsg->init_in.max_readahead,(_LI)inMsg->init_in.flags);
    if ( (inMsg->init_in.major >= FUSE_KERNEL_VERSION) &&  (inMsg->init_in.minor >= FUSE_KERNEL_MINOR_VERSION) ){
    }
    else {
       return error_send(in,-ENOSYS);
       abort();
    }

    printf("Fuse init outMsg major=%ld minor=%ld readahead=%ld flags=%ld ",(_LI)outMsg.init_out.major,(_LI)outMsg.init_out.minor,(_LI)outMsg.init_out.max_readahead,(_LI)outMsg.init_out.flags);
    printf(" max_background=%ld congestion_threshold=%ld max_write=%ld \n",(_LI)outMsg.init_out.max_background,(_LI)outMsg.init_out.congestion_threshold,(_LI)outMsg.init_out.max_write);

    retval = write(_deviceFD, &outMsg, outMsg.hdr.len);
    if (retval < 0) return errno; 
    return retval;
}

