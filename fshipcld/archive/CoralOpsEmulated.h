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
/* (C) Copyright IBM Corp.  2015                                    */
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

//! \file  CoralOpsEmulated.h
//! \brief class CoralOps
#ifndef __CORALOPSEMULATED_H__
#define __CORALOPSEMULATED_H__

#include "util.h"
#include "CoralOps.h"
#include "CoralNodeEmulated.h"

class CoralOpsEmulated : public CoralOps
{
public:
   CoralOpsEmulated(char * pMountDir,char * pEmulatePath){
    int length = strlen(pEmulatePath); 
    _emulatePath = new char[length+1];
    memcpy(_emulatePath,pEmulatePath,length+1);
    length = strlen(pMountDir);
    _mountDir = new char[length+1];
    memcpy(_mountDir,pMountDir,length+1);
    _rootNodePtr = NULL;
    //printf("%s:%d _mountDir=%s _emulatePath=%s \n",__PRETTY_FUNCTION__,__LINE__,_mountDir,_emulatePath); 
    _memoryLength = 2*64*1024;//two pages on power 7
    memoryPtr = mmap(NULL, _memoryLength,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,0,0);
    if (memoryPtr==MAP_FAILED){
      memoryPtr=NULL;
    }
  }
  ~CoralOpsEmulated(){
     if (memoryPtr){
       int rc=munmap(memoryPtr, _memoryLength);
       if (rc) printf("munmap had errno=%u \n",errno);
     }
     if (_rootNodePtr) delete _rootNodePtr;
     if (_emulatePath) delete []_emulatePath;
     if (_mountDir){
       close(_deviceFD);
       unmountDevice();
       delete [] _mountDir;
     }
  }
private:
  char * _emulatePath;
  char * _mountDir;
  NodeNameEmulatedRoot* _rootNodePtr;
public: 
virtual int mountDevice();
virtual int readMonitorFuseDevice();
virtual __s64 unmountDevice();
virtual __s64 processMsg(inMsgGeneric * in );
virtual __s64 init_op(inMsgGeneric * in );


private:
CoralOpString opString;
void * memoryPtr;
int _memoryLength;
void logHeader(inMsgGeneric * in){
  fuse_in_header * h = (fuse_in_header *)in;
  char * opcode_str;
  opcode_str = opString[(unsigned int)h->opcode];
  printf("->inMsg: l=%d %s(%d) id=%lld nid=%lld uid=%d gid=%d pid=%d \n",(int)h->len,opcode_str,(int)h->opcode,(long long int)h->unique,(long long int)h->nodeid,(int)h->uid,(int)h->gid,(int)h->pid);
}

 
protected:


virtual __s64 lookup_op(inMsgGeneric * in ){
  __s64 retval = 0;
  inMsgLookup * inMsg = (inMsgLookup *)in;
  outMsgLookup outMsg(in->hdr.unique);;
  struct stat entry_stat;
  printf("lookup of name=%s\n",inMsg->lookupname);
  int rcStat = _rootNodePtr->lookupInRemoteDir(in->hdr.nodeid,inMsg->lookupname,&entry_stat);
  if (rcStat) printf("rcStat=%d errno=%d\n",rcStat,errno);
  if (!rcStat){  
     printf("processing stat entry\n");
     copyStat2attr_out(&entry_stat,&outMsg.entry_out.attr);
    
     outMsg.entry_out.nodeid=entry_stat.st_ino;
     if (!outMsg.entry_out.nodeid)printf("outMsg.entry_out.nodeid is zero \n");
     outMsg.entry_out.attr_valid=10; /*seconds*/
     outMsg.entry_out.attr_valid_nsec=0; 
     outMsg.entry_out.generation=1;
     outMsg.entry_out.entry_valid=10;	// Cache timeout for the name (seconds) see fuse module dir.c
     outMsg.entry_out.entry_valid_nsec=0;
     retval = (__s64 )write(_deviceFD, &outMsg, outMsg.hdr.len);
    
  }
  else{
     retval=error_send(in,-errno);
  }
  return retval;

}

/*
struct fuse_getattr_in {
	__u32	getattr_flags;
	__u32	dummy;
	__u64	fh;
};

 * Getattr flags #define FUSE_GETATTR_FH		(1 << 0)

*/

__s64 getattr_op(inMsgGeneric * in ){
  inMsgGetattr *inMsg = (inMsgGetattr *)in;
  __s64 retval = 0;
  if (inMsg->getattr_in.getattr_flags) printf("getattr_flags=%x fh=%p \n",inMsg->getattr_in.getattr_flags,(void*)inMsg->getattr_in.fh);
  struct stat entry_stat;
  int rcStat = _rootNodePtr->lookupInRemoteDir(in->hdr.nodeid,NULL,&entry_stat);
  if (!rcStat){ 
     outMsgGetattr outMsg(in->hdr.unique); 
     copyStat2attr_out(&entry_stat,&outMsg.getattr_out.attr);
     outMsg.getattr_out.attr_valid=1; /*seconds*/
     outMsg.getattr_out.attr_valid_nsec=0; 
     retval = sendOutMsg(outMsg.hdr);
  }
  else{
     retval=error_send(in,-errno);
  }
  return retval;
}

__s64 opendir_op(inMsgGeneric * in ){
  __s64 retval = 0;
  //inMsgOpen *inMsg = (inMsgOpen *)in;
  outMsgOpen outMsg(in->hdr.unique);
  
  DIR * dirp = _rootNodePtr->openRemoteDir(in->hdr.nodeid);
  if (dirp){
    outMsg.open_out.fh =  (__u64)dirp;
    outMsg.open_out.open_flags = FOPEN_DIRECT_IO; // FOPEN_DIRECT_IO;
    retval = (__s64 )write(_deviceFD, &outMsg, outMsg.hdr.len);
  }
  else retval = error_send(in,-errno);
   
  return retval;
}


__s64 readdir_op(inMsgGeneric * in ){
   __s64 retval = 0;
   inMsgReaddir *inMsg = (inMsgReaddir *)in;
   
   DIR * dirp = (DIR *)inMsg->read_in.fh;
   struct dirent dent;
   char * charMemoryPtr = (char *)memoryPtr;
  
   // printf("fh=%llx offset=%llu size=%llu read_flags=%llx lock_owner=%lld flags=%lld \n sizeof(struct dirent)=%lu sizeof(d_ino)=%lu sizeof(d_roff)=%lu sizeof(d_reclen)=%lu sizeof(d_type)=%lu nameOffset=%lu sizeof(d_name)=%lu \n", (LLUS)inMsg->read_in.fh, (LLUS)inMsg->read_in.offset,(LLUS)inMsg->read_in.size,(LLUS)inMsg->read_in.read_flags,(LLUS)inMsg->read_in.lock_owner,(LLUS)inMsg->read_in.flags,sizeof(struct dirent),sizeof(ino_t), sizeof(off_t),sizeof(dent.d_reclen),sizeof(dent.d_type),offsetof(struct dirent,d_name),sizeof(dent.d_name) );
  
   if ( memoryPtr==NULL ){
       printf("mmap failed with errno=%u(%s) \n",errno,strerror(errno)); //need RAS log
       return error_send(in,-ENOMEM);
   }

   outMsgReaddir outMsg(in->hdr.unique);
   printf("memoryPtr=%p \n",memoryPtr);
     
   struct fuse_dirent * l_direntPtr = ( struct fuse_dirent *)charMemoryPtr;
   __u32 bytes_written = 0;
   int bottomBits= 0;
   const int maxFuseDirentSize = FUSE_NAME_OFFSET + NAME_MAX;
   if (_memoryLength< maxFuseDirentSize){
      return error_send(in,-ENOMEM); 
   }
   struct dirent *result = NULL;
   int rc = readdir_r(dirp,&dent,&result);
        if (rc) { //nonzero if EBADF
          printf("readdir rc=%u  \n",rc);
          retval = error_send(in,-rc);       
   }
   const NodeNamePTR parent = _rootNodePtr->getNodeDir(in->hdr.nodeid);
   while (result){
      //if (result) printf("rc=%lld, ino=%llu offset=%llu reclen=%llu type=%llu filename=%s \n",(LLUS)rc,(LLUS)dent.d_ino,(LLUS)dent.d_off,(LLUS)dent.d_reclen,(LLUS)dent.d_type,dent.d_name);

         _rootNodePtr->updateChildNode(&dent,parent);

         l_direntPtr->ino = dent.d_ino; 
         l_direntPtr->type = dent.d_type;
         l_direntPtr->namelen=strlen(dent.d_name);
         strncpy(l_direntPtr->name,dent.d_name,l_direntPtr->namelen);
         l_direntPtr->off = sizeof(struct fuse_dirent) +  l_direntPtr->namelen;

         bottomBits= l_direntPtr->off & 7;
         if (bottomBits){
            l_direntPtr->off += 8-bottomBits;
         }
         charMemoryPtr+= l_direntPtr->off;      
         bytes_written += l_direntPtr->off;

         if ( (signed)(_memoryLength - bytes_written) < maxFuseDirentSize ){
            break;
         }
         l_direntPtr  = ( struct fuse_dirent *)charMemoryPtr;
         readdir_r(dirp,&dent,&result);
   };
   
  struct iovec iov[2];
  iov->iov_base=&outMsg;
  iov->iov_len=outMsg.hdr.len;
  outMsg.hdr.len += bytes_written;
  iov[1].iov_base=memoryPtr;
  iov[1].iov_len=bytes_written;
  int iovcnt = sizeof(iov)/sizeof(struct iovec);
  retval = writev(_deviceFD,iov,iovcnt);

   return retval;
}



/*
#include <sys/uio.h>
ssize_t writev(int fildes, const struct iovec *iov, int iovcnt); 
*/

// ignore the flags and release flag (flush) for releasedir 
__s64 releasedir_op(inMsgGeneric * in ){
   inMsgReleasedir * inMsg = (inMsgReleasedir *)in;
   errno=0;
   //printf("fh=%llx flags=%lx release_flags=%lx lockowner=%llu \n", (LLUS)inMsg->release_in.fh,(long int)inMsg->release_in.flags,(long int)inMsg->release_in.release_flags,(LLUS)inMsg->release_in.lock_owner);
   
   int rc =  _rootNodePtr->closeRemoteDir(in->hdr.nodeid,(DIR *)inMsg->release_in.fh);
   if (rc) return error_send(in,-errno);
   else return error_send(in,0);
}
/*
direct_io

  This option disables the use of page cache (file content cache) in
  the kernel for this filesystem.  This has several affects:

     - Each read() or write() system call will initiate one or more
       read or write operations, data will not be cached in the
       kernel.

     - The return value of the read() and write() system calls will
       correspond to the return values of the read and write
       operations.  This is useful for example if the file size is not
       known in advance (before reading it).
*/
virtual __s64 create_op(inMsgGeneric * in ){
  inMsgCreate * inMsg = (inMsgCreate *)in;
  mode_t mode = (inMsg->create_in.mode) & ~(inMsg->create_in.umask); 
  
  int fd=_rootNodePtr->openInRemoteDir(in->hdr.nodeid,inMsg->name,inMsg->create_in.flags,  mode);
  if (fd<0){
    return error_send(in,-errno); 
  }
  
  outMsgCreate outMsg(in->hdr.unique);
  outMsg.open_out.open_flags=FOPEN_DIRECT_IO; // FOPEN_DIRECT_IO;
  outMsg.open_out.fh=fd;

  struct stat entry_stat;
  int rcStat = _rootNodePtr->lookupInRemoteDir(in->hdr.nodeid,fd,inMsg->name,&entry_stat);
  if (rcStat) return error_send(in,-errno);
  //need to create a nodeid-name entry  
  copyStat2attr_out(&entry_stat,&outMsg.entry_out.attr);
  outMsg.entry_out.nodeid=entry_stat.st_ino;   
  outMsg.entry_out.attr_valid=600; /*seconds*/
  outMsg.entry_out.attr_valid_nsec=1; 
  outMsg.entry_out.generation=1;
  outMsg.entry_out.entry_valid=600;	// Cache timeout for the name (seconds) see fuse module dir.c
  outMsg.entry_out.entry_valid_nsec=1;
  __s64 retval = (__s64 )write(_deviceFD, &outMsg, outMsg.hdr.len);
  return retval;
 
}

virtual __s64 open_op(inMsgGeneric * in ){
  inMsgOpen * inMsg = (inMsgOpen *)in;
  int fd=_rootNodePtr->openInRemoteDir(in->hdr.nodeid,NULL,inMsg->open_in.flags, 0);
  if (-1==fd){
    return error_send(in,-errno); 
  }
  outMsgOpen outMsg(in->hdr.unique);
  outMsg.open_out.open_flags=FOPEN_DIRECT_IO; // FOPEN_DIRECT_IO;
  outMsg.open_out.fh=fd;
  __s64 retval = write(_deviceFD,&outMsg, outMsg.hdr.len);
  if (-1==retval)printf("%s errno=%d(%s)\n",__FUNCTION__,errno,strerror(errno) );
  return retval;
};

virtual __s64 release_op(inMsgGeneric * in ){
  inMsgRelease * inMsg = (inMsgRelease *)in;
  close(inMsg->release_in.fh);
  return error_send(in,0);
};

//not used in read_in struct: read_flags, lock_owner, flags
virtual __s64 read_op(inMsgGeneric * in ){
  inMsgPread * inMsg = (inMsgPread *)in;
  
  size_t nbyte = inMsg->read_in.size;
  if ((__u64)_memoryLength<inMsg->read_in.size)nbyte=_memoryLength;
  
  //ssize_t pread(int fildes, void *buf, size_t nbyte, off_t offset); 
  ssize_t ssizeRetval = pread(inMsg->read_in.fh, memoryPtr,nbyte,inMsg->read_in.offset);
  if (ssizeRetval == -1){
    return error_send(in,-errno);
  }
  
  if (!ssizeRetval)return error_send(in,0);

  outMsgPread outMsg(in->hdr.unique);
  struct iovec iov[2];
  iov->iov_base=&outMsg;
  iov->iov_len=outMsg.hdr.len;
  outMsg.hdr.len += ssizeRetval;
  iov[1].iov_base=memoryPtr;
  iov[1].iov_len=ssizeRetval;
  int iovcnt = sizeof(iov)/sizeof(struct iovec);
  __s64 retval = writev(_deviceFD,iov,iovcnt);
  //eventually need to look at using pipe and splice to reduce memory copy 
  return retval;
}
/////////////////////////////////////////////////////////////////
  //not used in read_in struct: write_flags, lock_owner, flags
virtual __s64 write_op(inMsgGeneric * in ){
   printf(" ENTER %s \n", __PRETTY_FUNCTION__);
   inMsgPwrite* inMsg = (inMsgPwrite*)in;
   size_t nbyte = inMsg->write_in.size;
   if ((__u64)_memoryLength<inMsg->write_in.size){
     nbyte=_memoryLength;
     printf("_memoryLength=%d<inMsg->write_in.size=%d\n",_memoryLength,inMsg->write_in.size);
   }

  //ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset); 
  ssize_t ssizeRetval = pwrite(inMsg->write_in.fh, inMsg->data,nbyte,inMsg->write_in.offset);
  if (ssizeRetval == -1){
    return error_send(in,-errno);
  }
  
  if (!ssizeRetval)return error_send(in,0);

  outMsgPwrite outMsg(in->hdr.unique);
  outMsg.write_out.size=(__u32)ssizeRetval;
  __s64 retval = write(_deviceFD,&outMsg, outMsg.hdr.len);
  //eventually need to look at using pipe and splice to reduce memory copy 
  return retval;
}
virtual __s64 flush_op(inMsgGeneric * in ){
   //inMsgFlush * inMsg = (inMsgFlush *)in;
   //int rc = fsync(inMsg->flush_in.fh);
   //if (rc) return error_send(in,-errno);
   return error_send(in,-ENOSYS); //No support of flush, fuse module will note and not send anymore
}
virtual __s64 fsync_op(inMsgGeneric * in ){
return error_send(in,ENOSYS);//No support of fsync, fuse module will note and not send anymore
}

virtual __s64 setattr_op(inMsgGeneric * in ){
  inMsgSetattr *inMsg = (inMsgSetattr *)in;
  
  __u32& valid = inMsg->setattr_in.valid;
  if (valid) printf("valid=%x\n",valid);

  struct stat entry_stat;
  if (valid & FATTR_FH){//have a file descriptor!  
    
    if (valid & FATTR_SIZE){
      int rcTruncate = ftruncate(inMsg->setattr_in.fh,inMsg->setattr_in.size);
      if (rcTruncate) return error_send(in,-errno);
    }
    int rcStat=fstat(inMsg->setattr_in.fh,&entry_stat);
    if (!rcStat){ 
      outMsgSetattr outMsg(in->hdr.unique);
      outMsg.attr_out.attr_valid=60; //seconds
      outMsg.attr_out.attr_valid_nsec=0; //nanoseconds 
      copyStat2attr_out(&entry_stat,&outMsg.attr_out.attr);
      return sendOutMsg(outMsg.hdr);
    }
    else return error_send(in,-errno);
      
  }
  else {
    if (valid & FATTR_SIZE){
      //int truncate(const char *path, off_t length);
      //int rcTruncate = truncate(
      // if (rcTruncate)error_send(in,-errno);
    }
    int rcStat = _rootNodePtr->lookupInRemoteDir(in->hdr.nodeid,NULL,&entry_stat);
    if (!rcStat){ 
      outMsgSetattr outMsg(in->hdr.unique);
      outMsg.attr_out.attr_valid=60; //seconds
      outMsg.attr_out.attr_valid_nsec=0; //nanoseconds 
      copyStat2attr_out(&entry_stat,&outMsg.attr_out.attr);
      return sendOutMsg(outMsg.hdr);
    }
    else return error_send(in,-errno);
  }
printf("need to add more settattr support\n");  
return error_send(in,-EIO);
}


virtual __s64 access_op(inMsgGeneric * in ){
  inMsgAccess *inMsg = (inMsgAccess *)in;
  int rcAccess = _rootNodePtr->accessInRemoteDir( in->hdr.nodeid,inMsg->access_in.mask);
  errno=EREMOTEIO; 
  if (rcAccess) return error_send(in,-errno);
  else return error_send(in,0);
}


virtual __s64 mkdir_op(inMsgGeneric * in ){
  inMsgMkdir * inMsg = (inMsgMkdir *)in;
  mode_t mode = (inMsg->mkdir_in.mode) & ~(inMsg->mkdir_in.umask); 
  printf("mkdir of %s \n",inMsg->name);
  int rcMkdir =_rootNodePtr->mkdirInRemoteDir(in->hdr.nodeid,inMsg->name,  mode);
  if (rcMkdir){
    return error_send(in,-errno); 
  }

  struct stat entry_stat;
  int rcStat = _rootNodePtr->lookupInRemoteDir(in->hdr.nodeid,inMsg->name,&entry_stat);
  if (rcStat) return error_send(in,-errno);
  outMsgMkdir outMsg(in->hdr.unique);
  //need to create a nodeid-name entry  
  copyStat2attr_out(&entry_stat,&outMsg.entry_out.attr);
  outMsg.entry_out.nodeid=entry_stat.st_ino;   
  outMsg.entry_out.attr_valid=600; /*seconds*/
  outMsg.entry_out.attr_valid_nsec=1; 
  outMsg.entry_out.generation=1;
  outMsg.entry_out.entry_valid=600;	// Cache timeout for the name (seconds) see fuse module dir.c
  outMsg.entry_out.entry_valid_nsec=1;
  __s64 retval = (__s64 )write(_deviceFD, &outMsg, outMsg.hdr.len);
  return retval;
 
}


};//class definition
#endif /* __CORALOPSEMULATED_H__ */
