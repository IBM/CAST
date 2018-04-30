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

//! \file  CoralNode.h
//! \brief class CoralNode

#ifndef CORALNODE_EMULATED_H
#define CORALNODE_EMULATED_H
#include "../include/NodeName.h"


/*
typedef NodeName * NodeNamePTR;
typedef std::pair<__u64,NodeNamePTR> inodePair;
typedef std::map<__u64,NodeNamePTR> NodeNameMap;
typedef NodeNameMap::iterator NodeNameIterator;
typedef NodeNameMap::const_iterator ConstNodeNameIterator;
typedef std::pair<NodeNameIterator,bool> NodeIteratorBool;
*/
class NodeNameEmulatedRoot : public NodeNameRoot  {
  public:

   NodeNameEmulatedRoot(char * mountDir,char * pEmulatePath):NodeNameRoot(mountDir){
       _emulatePathSize= strlen(pEmulatePath);
       _emulatePath = new char[strlen(pEmulatePath)+1];
       memcpy(_emulatePath,pEmulatePath,strlen(pEmulatePath)+1);
       //errno=0;
       //printf("doing stat of _emulatePath=%s pEmulatePath=%s \n",_emulatePath,pEmulatePath);
       //_statRC=stat(_emulatePath,&_stat);
       //_statErrno = errno;
       _stat.st_ino=FUSE_ROOT_ID;
       _FSflavor=EMULATED;
       _dirMap.insert(inodePair(this->getInode(),this) );
   }

  virtual ~NodeNameEmulatedRoot(){delete [] _emulatePath;}


size_t buildRemotePath(const __ino64_t pInode, char * const target, const size_t size){
     
       ConstNodeNameIterator it = _dirMap.find(pInode);
       if (it == _dirMap.end() ) {
        it = _FileMap.find(pInode);
        if (it == _FileMap.end() ) return 0;
       } 

       NodeNamePTR nnp = it->second;
       if (nnp->isRoot() ) {
          return snprintf(target, size, "%s", _emulatePath);
       }
       else {
          size_t length=buildRemotePath(nnp->getParentInode(),target,size);
          if (length){
            length += snprintf(target+length,size-length,"/%s",nnp->accessNamePTR() );
          }
          return length;
       }
          
   }


DIR * openRemoteDir(const __ino64_t pInode){
       int tempSize = 2048; //being lazy for emulated
       char temp[tempSize];
       size_t pathlength = buildRemotePath( pInode,temp,tempSize);
       printf("opendir pathlength=%d path=%s Inode=%lld  ,tempSize=%d\n",(int)pathlength,temp,(long long int)pInode,tempSize);
       DIR * dirp = NULL;
       if (pathlength){
          dirp = opendir(temp);
          
       }
       else errno=ENOENT;
       printf("dirp=%p\n",dirp);
       return dirp;
}

//On success, zero is returned.  On error, -1 is returned, and errno is
//     set appropriately.
int    lookupInRemoteDir(const __ino64_t pInode,char * const lookupName,struct stat * statptr){
       ConstNodeNameIterator it = _dirMap.find(pInode);
       if (lookupName){//looking for a name in a directory with node=pInode
          if (it == _dirMap.end() ) { errno=ENOENT;  return -1;}
       }
       else {
          if (it == _dirMap.end() ){
             it = _FileMap.find(pInode);
             if (it == _FileMap.end() ){ errno=ENOENT; return -1;}
          }
       }

       NodeNamePTR nnp = it->second;
       int lengthLookupName = 0;
       int tempSize = 2048; //being lazy for emulated
       char temp[tempSize];
       // \TODO: add looking for fd in parent directory or ancestor to cut down on pathlength
       // \TODO: consider cwd implementation possibilities??
       size_t pathlength = buildRemotePath( pInode,temp,tempSize); 
       if (!pathlength){
         errno=ENOENT;
         return -1;
       }
       if (lengthLookupName){
          pathlength += snprintf(temp+pathlength,tempSize-pathlength, "/%s",lookupName);//if nonnull lookupName, append
       }
       errno=0;
       int rcStat = fstatat(-1, temp, statptr,AT_SYMLINK_NOFOLLOW);
       if (!rcStat) if (lengthLookupName) updateChildNode(statptr,lookupName,nnp);
       printf("path=%s pathlength=%d rcStat=%d errno=%d method=%s \n",temp,(int)pathlength,rcStat,errno,__FUNCTION__);
       return rcStat;
}

//On success, zero is returned.  On error, -1 is returned, and errno is
//     set appropriately.
int    lookupInRemoteDir(const __ino64_t pInode,int fd,char * const lookupName,struct stat * statptr){
       if (!lookupName) { errno=ENOENT;  return -1;}
       ConstNodeNameIterator it = _dirMap.find(pInode);
       if (it == _dirMap.end() ) { errno=ENOENT;  return -1;}
       NodeNamePTR nnp = it->second;
       errno=0;
       int rcStat = fstat(fd, statptr);
       if (!rcStat)  updateChildNode(statptr,lookupName,nnp);
       
       return rcStat;
}

int closeRemoteDir(const __ino64_t pInode,DIR * dirp){
       int rc = 0;
       if (dirp) rc=closedir(dirp);
       return rc;   
}

int accessInRemoteDir( const __ino64_t pInode,const int mode){
       NodeNamePTR nnp = findNodeName(pInode);
       if (!nnp) {errno=ENOENT; return -1;}
       int tempSize = 2048; //being lazy for emulated
       char temp[tempSize];
       size_t pathlength = buildRemotePath( pInode,temp,tempSize);
       if (!pathlength){ errno=ENOENT;return -1; }
       int rcAccess = access(temp,mode);
       return rcAccess;
}

NodeNamePTR findNodeName( const __ino64_t pInode){
       
       ConstNodeNameIterator it = _FileMap.find(pInode);
       if (it == _FileMap.end() ) {         
         it = _dirMap.find(pInode);
         if (it == _dirMap.end() ) { return NULL;}
       }
       return it->second;
};

NodeNamePTR findDirNodeName( const __ino64_t pInode){
       
       ConstNodeNameIterator it = _dirMap.find(pInode);
       if (it == _dirMap.end() ) {         
          return NULL;
       }
       return it->second;
}


//returns file descriptor (fd) or -1 with errno set
int    openInRemoteDir(const __ino64_t pInode,char * const lookupName,int flags, mode_t mode){
       NodeNamePTR nnp = findNodeName(pInode);
       if (!nnp) {errno=ENOENT; return -1;}
       printf("inode=%lld with name=%s \n",(LLUS)pInode,nnp->accessNamePTR() );
       int lengthLookupName = 0;
       int tempSize = 2048; //being lazy for emulated
       char temp[tempSize];
       // \TODO: add looking for fd in parent directory or ancestor to cut down on pathlength--see open O_PATH in open man page
       // \TODO: consider cwd implementation possibilities??
       size_t pathlength = buildRemotePath( pInode,temp,tempSize);
       if (!pathlength){ errno=ENOENT;return -1; }
       if (lengthLookupName){
          pathlength += snprintf(temp+pathlength,tempSize-pathlength, "/%s",lookupName);//if nonnull lookupName, append
       }        
       errno=0;
       int rcOpen = openat(-1,temp,flags,mode);
       printf("path=%s pathlength=%d rcOpen=%d errno=%d flags=%x mode=%o method=%s \n",temp,(int)pathlength,rcOpen,errno,flags,(int)mode,__FUNCTION__);
       return rcOpen;
}

//returns file descriptor (fd) or -1 with errno set
int    mkdirInRemoteDir(const __ino64_t pInode,char * const lookupName, mode_t mode){
       
       if (!lookupName) {errno=-ENOENT; return -1;}
       NodeNamePTR nnp = findDirNodeName(pInode);
       if (!nnp) {errno=ENOENT; return -1;}
       
       int tempSize = 2048; //being lazy for emulated
       char temp[tempSize];
       size_t pathlength = buildRemotePath( pInode,temp,tempSize);
       pathlength += snprintf(temp+pathlength,tempSize-pathlength, "/%s",lookupName);//if nonnull lookupName, append   
       errno=0;
       int rcMkdir = mkdir(temp,mode);
       printf("path=%s pathlength=%d rcMkdir=%d errno=%d  mode=%o method=%s \n",temp,(int)pathlength,rcMkdir,errno,(int)mode,__FUNCTION__);
       return rcMkdir;
}

private:  
   char * _emulatePath;
   int    _emulatePathSize;
  
   struct stat _stat;
};//end class NodeNameEmulatedRoot



#endif //CORALNODE_EMULATED_H
