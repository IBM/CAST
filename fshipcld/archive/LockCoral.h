/*******************************************************************************
 |    LockCoral.h
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


//! \file  CoralNode.h
//! \brief class CoralNode

#ifndef LOCKCORAL_H
#define LOCKCORAL_H

#include <stddef.h>
#include <string>
#include <map>
#include <linux/fuse.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include "logging.h"
#include <fcntl.h>
#include "fshipcld.h"


/*
struct fuse_file_lock {
	uint64_t	start;
	uint64_t	end;
	uint32_t	type;
	uint32_t	pid; // tgid 
};
struct fuse_lk_in {
	uint64_t	FH2;
	uint64_t	owner;
	struct fuse_file_lock lk;
	uint32_t	lk_flags;
	uint32_t	padding;
};
*/

/* A single process can hold only one
       type of lock on a file region; if a new lock is applied to an
       already-locked region, then the existing lock is converted to the new
       lock type.  (Such conversions may involve splitting, shrinking, or
       coalescing with an existing lock if the byte range specified by the
       new lock does not precisely coincide with the range of the existing
       lock.)
*/
typedef struct fuse_lk_in fuseFileLock;


class LockRegion {

public:
  uint64_t start;
  uint64_t end;
public:
  LockRegion(uint64_t pStart=0, uint64_t pEnd=0):start(pStart),end(pEnd){};
  LockRegion(inMsgLock* inMsg) 
  :start(inMsg->lock_in.lk.start),end(inMsg->lock_in.lk.end){}

bool isSubsetOf(LockRegion& pLR){ 
   return ( (pLR.start <= start) && (end <= pLR.end) );}

bool operator !=(const LockRegion& pLR)const{
    return ( ( start >  pLR.end) || (pLR.start > end) );  
  }  

bool operator ==(const LockRegion& pLR)const{//exact match
    return ( ( start == pLR.start) && (pLR.end == end) );  
  }  

//required for std::map definition
bool operator <(const LockRegion& pLR)const {//overlap is equivalent
    return ( (end<pLR.start) );
  }
bool operator >(const LockRegion& pLR)const{//overlap is equivalent
    return ( (start>pLR.end) );
  }
LockRegion& operator|= (const LockRegion& pLR){
    if (*this != pLR) return *this; //no overlap, so no contiguous union
    if (start > pLR.start) start = pLR.start;
    if (end < pLR.end) end = pLR.end;
    return *this;
  } 

};



class LockCoral : public LockRegion  {

public:
  LockCoral(inMsgLock* inMsg) 
  :LockRegion(inMsg->lock_in.lk.start,inMsg->lock_in.lk.end) 
  {
      
     _type = inMsg->lock_in.lk.type;
     _pid = inMsg->lock_in.lk.pid;
     _fh = inMsg->lock_in.fh;
     _owner = inMsg->lock_in.lk.type;
     _lk_flags = inMsg->lock_in.lk_flags;

    _unique = inMsg->hdr.unique;
    _nodeid=inMsg->hdr.nodeid;
  }
  int getLock(fflock * pfflock)const { 
      pfflock->pid=_pid;
      pfflock->type=_type;
      pfflock->start = start;
      pfflock->end = end;
      return _type;
   }
   uint32_t getPID(){return _pid;}

protected:
   uint32_t   _type;
   uint32_t   _pid; // tgid 
   uint64_t   _fh; 
   uint64_t   _owner;
   uint32_t   _lk_flags;
 
   uint64_t _unique;
   uint64_t _nodeid;
};

typedef LockCoral * FileLockPTR;
typedef std::pair<LockRegion,FileLockPTR> RegionLockPair;
typedef std::map<LockRegion,FileLockPTR> RegionLockMap;
typedef RegionLockMap::iterator RegionLockIterator;
typedef RegionLockMap::const_iterator ConstRegionLockIterator;
typedef RegionLockMap::reverse_iterator ReverseRegionLockIterator;
typedef std::pair<RegionLockIterator,bool> RegionLockIteratorBool;


typedef RegionLockMap* RegionLockMapPtr;
typedef std::pair<uint32_t,RegionLockMapPtr> PID2LockMapPair;
typedef std::map<uint32_t,RegionLockMapPtr> PID2LockMapInodeMap;
typedef PID2LockMapInodeMap::iterator PID2LockMapInodeMapIterator;
typedef PID2LockMapInodeMap::const_iterator ConstPID2LockMapInodeMapIterator;
typedef PID2LockMapInodeMap * PID2LockMapInodeMapPtr;



class FileLocksPosix {
  private:
   int noLock(const fflock& pfflock, fflock* pfflockPtr)const { 
      
      memcpy(pfflockPtr,&pfflock,sizeof(fflock) );
      pfflockPtr->type=F_UNLCK;
      return F_UNLCK;
   }
   
   public:

     FileLocksPosix(){
       _PID2LockMapInodeMapPtr=NULL;
       _RegionWriteLockMapPtr = NULL;
     }

     int removeReadLock(inMsgLock* inMsg);
     int removeWriteLock(inMsgLock* inMsg);

////////////////////////////////////////
// Check pid on this compute node whether file lock would block
// true=return getlock()
//other = ok locally, need to check remote fshipd
int GETLKcheck4WriteLockBlocked(inMsgLock* inMsg, fflock* pfflPtr){
       LockCoral l_LC(inMsg);
       if (_RegionWriteLockMapPtr){ 
         RegionLockIterator it = _RegionWriteLockMapPtr->find(l_LC);
         if (it!=_RegionWriteLockMapPtr->end() ){
           if (l_LC.getPID()!= it->second->getPID() ){
              return  it->second->getLock(pfflPtr);
           }
           else if (l_LC.isSubsetOf(*it->second) ){//collision on same PID
              //do nothing
           }
           else 
           { 
                //do a brute force look until something better comes along
                for (auto value : *_RegionWriteLockMapPtr){
                   if (l_LC != *value.second) continue; //ignore nonoverlap
                   if (l_LC.getPID()!= value.second->getPID() ) return  value.second->getLock(pfflPtr);
                }
           }
         }
       }
       if (_PID2LockMapInodeMapPtr){
         for (auto value : *_PID2LockMapInodeMapPtr){
           if (l_LC.getPID()== value.first) continue; //allow for change from read lock to write lock
           RegionLockIterator it = value.second->find(l_LC);
           if (it!=value.second->end() ) return  it->second->getLock(pfflPtr);
         }//endfor
       }
       return noLock(inMsg->lock_in.lk,pfflPtr);
     }


//////////////////////////////////////////
// Check pid on this compute node whether file lock would block
//true = return getlock()
//other = ok locally, need to check remote fshipd
int GETLKcheck4ReadLockBlocked(inMsgLock* inMsg, fflock* pfflPtr){
       if (!_RegionWriteLockMapPtr) return noLock(inMsg->lock_in.lk,pfflPtr);
       LockCoral l_LC(inMsg);
       RegionLockIterator it = _RegionWriteLockMapPtr->find(l_LC);
       if (it==_RegionWriteLockMapPtr->end() ) return noLock(inMsg->lock_in.lk,pfflPtr);
       // need to confirm this is held by a differnt process
       if (l_LC.getPID()== it->second->getPID() ){
        if (l_LC.isSubsetOf(*(it->second)) ){
          return noLock(inMsg->lock_in.lk,pfflPtr);
        }
        //do a brute force look until something better comes along
        for (auto value : *_RegionWriteLockMapPtr){
           if (l_LC != *value.second) continue; //ignore nonoverlap
           if (l_LC.getPID()!= value.second->getPID() ) return  value.second->getLock(pfflPtr);
        } 
       }
       return  it->second->getLock(pfflPtr);
     }
     
int SETLKcheckIfBlocked(inMsgLock* inMsg){
    fflock l_fflock;
    if (inMsg->lock_in.lk.type==F_WRLCK) {
       return GETLKcheck4WriteLockBlocked(inMsg,&l_fflock);
    }
    else {
       return GETLKcheck4ReadLockBlocked(inMsg,&l_fflock);
    }
    return 0;
}

int addWriteLock(inMsgLock* inMsg){
   if ( !( F_UNLCK == SETLKcheckIfBlocked(inMsg) )  ) return 0;

   LockCoral* writeLC = new LockCoral(inMsg);
   if (!_RegionWriteLockMapPtr) _RegionWriteLockMapPtr = new RegionLockMap;
   RegionLockIteratorBool itbool = _RegionWriteLockMapPtr->insert( RegionLockPair( LockRegion(inMsg),writeLC) );
   if (itbool.second) return 1;
   if (writeLC->getPID() != itbool.first->second->getPID() ){
      delete writeLC;
      return 0;
   }
   RegionLockIterator l_RLI = itbool.first;
   if (writeLC->isSubsetOf(*(l_RLI->second)) ){
      delete writeLC;
      return 4;
   }
   while (!itbool.second){ //insertion was not succesful
     if (writeLC->getPID() != itbool.first->second->getPID() ){
      delete writeLC;
      return 0;
     }
     *writeLC |= *(l_RLI->second);  //Colesce regions for the same PID
     delete itbool.first->second;
     l_RLI = _RegionWriteLockMapPtr->erase(itbool.first); //points to next
     itbool = _RegionWriteLockMapPtr->insert( RegionLockPair( LockRegion(inMsg),writeLC) );
   }
   PID2LockMapInodeMapIterator it = _PID2LockMapInodeMapPtr->find( writeLC->getPID() );
   if (it==_PID2LockMapInodeMapPtr->end() ) return 1;
   RegionLockMapPtr l_RLM = it->second;
   RegionLockIterator l_RLIreadlist = l_RLM->find(*writeLC);
   if (l_RLIreadlist==l_RLM->end() ) return 2;
   if (l_RLIreadlist->second->isSubsetOf(*writeLC) ){ 
     delete l_RLIreadlist->second;
     l_RLIreadlist = l_RLM->erase(l_RLIreadlist);
   }
   printf("need to check read pid map for additional conversions of pid read lock on overlapping region");
   return 1;
   }

int addReadLock(inMsgLock* inMsg){

     if ( !( F_UNLCK == SETLKcheckIfBlocked(inMsg) )  ) return 0;

       LockCoral l_LC(inMsg);
       if (_RegionWriteLockMapPtr){ 
         RegionLockIterator it = _RegionWriteLockMapPtr->find(l_LC);
         if (it!=_RegionWriteLockMapPtr->end() ) { 
            if (l_LC.getPID()==it->second->getPID() ){
              if (it->second->isSubsetOf(l_LC) ){//remove
                 delete it->second;
                 it = _RegionWriteLockMapPtr->erase(it);                 
              }
              else {
                printf("addReadLock case yet to handle \n");
              }
              
            }
            return 0; // it->second->getLock();
         }
       }
       // find the read region list for the pid
       if (!_PID2LockMapInodeMapPtr){
           _PID2LockMapInodeMapPtr = new PID2LockMapInodeMap;
            RegionLockMapPtr l_RLM = new RegionLockMap;
           
           _PID2LockMapInodeMapPtr->insert( PID2LockMapPair(l_LC.getPID(),l_RLM) );
           LockCoral* readLC = new LockCoral(inMsg);
           l_RLM->insert( RegionLockPair( LockRegion(inMsg),readLC) );
           return 1;
       }
       PID2LockMapInodeMapIterator it = _PID2LockMapInodeMapPtr->find( l_LC.getPID() );
       if (it==_PID2LockMapInodeMapPtr->end() ){//pid not found
            RegionLockMapPtr l_RLM = new RegionLockMap;          
           _PID2LockMapInodeMapPtr->insert( PID2LockMapPair(l_LC.getPID(),l_RLM) );
           LockCoral* readLC = new LockCoral(inMsg);
           l_RLM->insert( RegionLockPair( LockRegion(inMsg),readLC) );
           return 2;
       }
       RegionLockMapPtr l_RLM = it->second; 
       LockCoral* readLC = new LockCoral(inMsg);
       RegionLockIteratorBool itbool = l_RLM->insert( RegionLockPair( LockRegion(inMsg),readLC) );
       if (itbool.second) return 3;//new element was inserted
       //overlap conflict and need to merge (to keep read list regions well-ordered
       
       RegionLockIterator l_RLI = itbool.first;
       if (readLC->isSubsetOf(*(l_RLI->second)) ){
          delete readLC;
          return 4;
       }
       while (!itbool.second){ //insertion was not succesful
          *readLC |= *(l_RLI->second);
           delete l_RLI->second;
           l_RLI = l_RLM->erase(l_RLI); //points to next
           itbool = l_RLM->insert( RegionLockPair( LockRegion(inMsg),readLC) );
       }
       return 5;
   } 

   private:
     PID2LockMapInodeMapPtr _PID2LockMapInodeMapPtr;
     RegionLockMapPtr   _RegionWriteLockMapPtr;

};



#endif //LOCKCORAL_H
