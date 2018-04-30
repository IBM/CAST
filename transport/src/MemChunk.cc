/*******************************************************************************
 |    MemChunk.cc
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


//! \file  MemChunk.cc
//! \brief Get a big piece of memory and chunk it up

#include <exception>
#include "MemChunk.h"


using namespace  txp;

MemChunk::MemChunk(int pNumChunks, uint64_t pChunkSize):MemChunkBase(pNumChunks,pChunkSize) {
       
      _memLength = (uint64_t)_numChunks * _chunkSize;
      _memMapPtr = (char*)mmap(NULL, _memLength,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS|MAP_LOCKED,0,0);
      if (_memMapPtr==MAP_FAILED)
      {
         _memMapPtr=NULL;
          throw std::runtime_error(std::string("MemChunk: mmap returned MAP_FAILED"));
         return;
      }
      _memList = new memChunk[_numChunks];
      for(int i=0; i<_numChunks;i++){
         _memList[i].startAddress = _memMapPtr + _chunkSize*i;
         _memList[i].memLength = _chunkSize;
         _memList[i].regMemPtr = NULL;
         _memList[i].mi.reset2basic();
         _memList[i].mi.address = _memList[i].startAddress;
         _memList[i].mi.rkey = 0;
         _memList[i].mi.lkey = 0;
         _memList[i].mi.handle = (uint64_t)this;
        
         _memList[i].mi.chunkSize = _chunkSize;
         _chunkListFree.push_back(_memList+i);
      }
      int semRC=sem_init(&_memSemaphore,0,_numChunks);
      if (semRC) abort();
      int mutexRC = pthread_mutex_init(&_mutex,NULL);
      if (mutexRC) abort();
   }

MemChunkRDMA::MemChunkRDMA(int pNumChunks, uint64_t pChunkSize,struct ibv_pd * pd):MemChunkBase(pNumChunks,pChunkSize) 
{
     _memChunkWaitSeconds = 60;
      _regMemPtr = NULL;
      _memLength = (uint64_t)_numChunks * _chunkSize;
      _memMapPtr = (char*)mmap(NULL, _memLength,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS|MAP_LOCKED,0,0);
      if (_memMapPtr==MAP_FAILED)
      {
          _memMapPtr=NULL;
          throw std::runtime_error(std::string("Unable to map memory"));
      }
      if(!pd) 
      {
          throw std::runtime_error(std::string("ibv_pd::pd is NULL"));
      }
      
      _regMemPtr = ibv_reg_mr(pd, _memMapPtr, _memLength, IBV_ACCESS_LOCAL_WRITE|IBV_ACCESS_REMOTE_WRITE|IBV_ACCESS_REMOTE_READ);
      
      _memList = new memChunk[_numChunks];
      for(int i=0; i<_numChunks;i++){
         _memList[i].startAddress = _memMapPtr + _chunkSize*i;
         _memList[i].memLength = _chunkSize;
         _memList[i].regMemPtr = _regMemPtr;
         _memList[i].mi.reset2basic();
         _memList[i].mi.address = _memList[i].startAddress;
         _memList[i].mi.chunkSize = _chunkSize;
         _memList[i].mi.lkey = _regMemPtr->lkey;
         _memList[i].mi.rkey = _regMemPtr->rkey;
         _memList[i].mi.handle = (uint64_t)this;

         _chunkListFree.push_back(_memList+i);
      }
      int semRC=sem_init(&_memSemaphore,0,_numChunks);
      if (semRC)
      {
          throw std::runtime_error(std::string("MemChunkRDMA: sem_init failed"));
      }
      int mutexRC = pthread_mutex_init(&_mutex,NULL);
      if (mutexRC)
      {
          throw std::runtime_error(std::string("MemChunkRDMA: pthread_mutex_init failed"));
      }
}
