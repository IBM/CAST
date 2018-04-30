/*******************************************************************************
 |    MemChunk.h
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


//! \file  MemChunk.h
//! \brief class MemChunk
#ifndef __MEMCHUNK_H__
#define __MEMCHUNK_H__
#include <list>
#include <map>
#include <semaphore.h>
#include <pthread.h>
#include <sys/mman.h>
#include "fshipcld.h"
#include "logging.h"
#include <infiniband/verbs.h>

//! Transport component uses txp namespace
namespace txp {

typedef struct memChunk{
  ::memItem mi;
  char * startAddress;
  uint64_t memLength;
  struct ibv_mr * regMemPtr;
  memChunk(): mi(NULL,0,0),startAddress(0),memLength(0),regMemPtr(NULL){};
} memChunk;

typedef memChunk * memChunkPtr;


typedef std::pair<memChunkPtr,memChunk> pair4memChunkPtr;

class MemChunkBase {
public:
MemChunkBase(int pNumChunks, uint64_t pChunkSize){
   _numChunks=pNumChunks;
   _chunkSize=pChunkSize;
}
virtual memItemPtr getChunk()=0;
virtual void putChunk(memItemPtr pMi)=0;
virtual ~MemChunkBase(){};
const uint64_t getChunkSize()const{return _chunkSize;}
const int getNumChunks()const{return _numChunks;}
protected:
   uint64_t _chunkSize;
   int _numChunks;
};

typedef MemChunkBase * MemChunkPtr;

class MemChunk : public MemChunkBase {
private:   
   uint64_t _memLength;
   char * _memMapPtr; //pointer to big memory chunk which is subdivided
   memChunkPtr _memList; //array of memChunk, _numChunk elements 
   std::list<memChunkPtr> _chunkListFree;
   std::map<memChunkPtr,memChunk> _inUseMap;
   sem_t _memSemaphore;
   pthread_mutex_t _mutex;  //control access to list processing
public:
   MemChunk(int pNumChunks, uint64_t pChunkSize);
virtual ~MemChunk(){
  pthread_mutex_destroy(&_mutex);
  int rc = munmap(_memMapPtr, _memLength);
  if (rc) LOG(txp,warning) <<  "munmap error " <<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<errno<<", "<<strerror(errno);

}

virtual memItemPtr getChunk(){//consumer responsible for ensuring unique
      sem_wait(&_memSemaphore);
      pthread_mutex_lock(&_mutex);
      memChunkPtr mcptr = _chunkListFree.front();
      mcptr->mi.reset2basic();
      mcptr->mi.chunkSize= _chunkSize;
      mcptr->mi.lkey = 0;
      mcptr->mi.rkey = 0;
      mcptr->mi.handle = (uint64_t)this;

      _chunkListFree.pop_front();     
      _inUseMap.insert(pair4memChunkPtr(mcptr,*mcptr) );
      pthread_mutex_unlock(&_mutex);
      return &mcptr->mi;
   }

virtual void putChunk(memItemPtr pMi){
     pthread_mutex_lock(&_mutex);
     const std::map<memChunkPtr,memChunk>::iterator it = _inUseMap.find( (memChunkPtr)pMi );
     if (it !=_inUseMap.end() ){
        _inUseMap.erase(it);
        _chunkListFree.push_back( (memChunkPtr)pMi );
        pthread_mutex_unlock(&_mutex);
        sem_post(&_memSemaphore);
     }
     else {
       abort();
       pthread_mutex_unlock(&_mutex);
     }
      return;
   }


};

class MemChunkRDMA : public MemChunkBase {
private:   
   uint64_t _memLength;
   char * _memMapPtr; //pointer to big memory chunk which is subdivided
   memChunkPtr _memList; //array of memChunk, _numChunk elements 
   std::list<memChunkPtr> _chunkListFree;
   std::map<memChunkPtr,memChunk> _inUseMap;
   sem_t _memSemaphore;
   pthread_mutex_t _mutex;  //control access to list processing
   struct ibv_mr * _regMemPtr;
   int _memChunkWaitSeconds; //! seconds to wait for a chunk (hang detection)
public:
   MemChunkRDMA(int pNumChunks, uint64_t pChunkSize,struct ibv_pd * pd=NULL);
virtual ~MemChunkRDMA(){
  pthread_mutex_destroy(&_mutex);
  if (_regMemPtr) {
    int errnoDeregMemPtr = ibv_dereg_mr(_regMemPtr);
    if (errnoDeregMemPtr) LOG(txp,warning)<< "ibv_dereg_mr regMemPtr failed"<<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<errnoDeregMemPtr<<", "<<strerror(errnoDeregMemPtr);
    _regMemPtr=NULL;
  }
  
  int rc = munmap(_memMapPtr, _memLength);
  if (rc) LOG(txp,warning) <<  "munmap error " <<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<errno<<", "<<strerror(errno);

}

void setMemChunkWaitSeconds(int pMemChunkWaitSeconds){_memChunkWaitSeconds=pMemChunkWaitSeconds;}

virtual memItemPtr getChunk(){//consumer responsible for ensuring unique
      struct timespec l_timespec; 
      clock_gettime(CLOCK_REALTIME,&l_timespec);
      l_timespec.tv_sec += _memChunkWaitSeconds;
      // if performance is required, can do sem_trywait and then branch off into doing a sem_timed_wait
      // http://linux.die.net/man/3/sem_timedwait
      int l_semRC = sem_timedwait(&_memSemaphore,&l_timespec);
      if (l_semRC){
        LOG(txp,always)<<"sem_timedwait "<<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<errno<<", "<<strerror(errno)<<" waited seconds="<<_memChunkWaitSeconds;
        return NULL;
      }
      pthread_mutex_lock(&_mutex);
      if (_chunkListFree.empty() ) abort(); // how did I get here?
      memChunkPtr mcptr = _chunkListFree.front();
      mcptr->mi.reset2basic();
      mcptr->mi.chunkSize= _chunkSize;
      mcptr->mi.handle = (uint64_t)this;
      _chunkListFree.pop_front();     
      _inUseMap.insert(pair4memChunkPtr(mcptr,*mcptr) );
      pthread_mutex_unlock(&_mutex);
      return &mcptr->mi;
   }

virtual void putChunk(memItemPtr pMi){
     pthread_mutex_lock(&_mutex);
     const std::map<memChunkPtr,memChunk>::iterator it = _inUseMap.find( (memChunkPtr)pMi );
     if (it !=_inUseMap.end() ){
        _inUseMap.erase(it);
        _chunkListFree.push_back( (memChunkPtr)pMi );
        pthread_mutex_unlock(&_mutex);
        sem_post(&_memSemaphore);
     }
     else {
       abort();
       pthread_mutex_unlock(&_mutex);
     }
      return;
   }

};



typedef MemChunkRDMA * MemChunkRDMAptr;


};   // namespace
#endif //__MEMCHUNK_H__
