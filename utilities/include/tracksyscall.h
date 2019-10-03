/*******************************************************************************
 |    tracksyscall.h
 |
 |  © Copyright IBM Corporation 2015,2017. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/syscall.h>
#include <iostream>
#include <string>
#include <map>


#ifndef TRACKSYSCALL_H_
#define TRACKSYSCALL_H_


inline uint64_t timeStamp()
{
uint64_t timebase;
#ifdef __powerpc64__
#define  SPRN_TBRO                (0x10C)          // Time Base 64-bit                             User Read-only

    asm volatile(
                 "mfspr %0,%1;"
                 : "=&r" (timebase) : "i" (SPRN_TBRO) : );
    return timebase;
#else
    timebase=0;
    return timebase;
#endif

}
class TrackSyscall;
typedef TrackSyscall* TrackSyscallPtr;
typedef std::pair<pthread_t, TrackSyscallPtr>  pthreadTrackPair;
extern thread_local TrackSyscallPtr threadLocalTrackSyscallPtr;
extern void locktidTrackerMutex();
extern void unlocktidTrackerMutex();
extern TrackSyscallPtr getSysCallTracker();
extern std::map<pthread_t, TrackSyscallPtr>   pthread_syscalltracker;


 class TrackSyscall{
   public:
   enum tracking{nosyscall=0, opensyscall=1, preadsyscall=2, pwritesyscall=3, statsyscall=4,
                 fstatsyscall=5, fsyncsyscall=6, fcntlsyscall=7, openexlayout=8, setupexlayout=9, finalizeexlayout=10,
                 fopensyscall=11, freadsyscall=12, fseeksyscall=13, ftellsyscall=14, fwritesyscall=15,
                 SSDopenwritedirect=32, SSDopenwriteNOTdirect=33, SSDpreadsyscall=34, SSDpwritesyscall=35, SSDopenreaddirect=48,
                 Runcommandfopen=64,Runcommandpopen=65};

   TrackSyscall();

inline uint64_t   nowTrack(tracking pSyscall,const std::string& pFilename, int pLineNumber=0)
   {
    _syscall = pSyscall;
    _fd = -1;
    _timeStamp = timeStamp();
    _fileName=pFilename;
    _lineNumber = pLineNumber;
    _rasCount = 0;
    return _timeStamp;
   }
inline uint64_t nowTrack(tracking pSyscall, int pFd=-1,int pLineNumber=0, uint64_t pSize=0, uint64_t pOffset=0 )
   {
    _syscall = pSyscall;
    _lineNumber = pLineNumber;
    _fd = pFd;
    _size=pSize;
    _offset=pOffset;
    _fileName.clear();
    _rasCount = 0;
    _timeStamp = timeStamp();
    return _timeStamp;
   }
inline uint64_t nowTrack(tracking pSyscall, FILE* pFile=(FILE*)0, int pLineNumber=0, uint64_t pSize=0, uint64_t pOffset=0)
   {
    _syscall = pSyscall;
    _lineNumber = pLineNumber;
    _fd = 0;
    if (pFile != 0) _fd = pFile->_fileno;
    _size=pSize;
    _offset=pOffset;
    _fileName.clear();
    _rasCount = 0;
    _timeStamp = timeStamp();
    return _timeStamp;
   }
inline void  clearTrack(){
/*
      if (_syscall == nosyscall);
      else if (_syscall == SSDopenwritedirect);
      else  if (_syscall == SSDopenwriteNOTdirect);
      else  if (_syscall == SSDopenreaddirect);
      else  if (_syscall == opensyscall);
      else  if (_syscall == fstatsyscall);
      else sleep(120);
    if (_syscall==fsyncsyscall) sleep(120);
*/
     _timeStamp=0;
     _syscall = nosyscall;
   }
inline bool isClear(){ return  !_timeStamp;}

   tracking _syscall;
   int      _lineNumber;
   int      _fd;
   int      _rasCount;
   uint64_t _size;
   uint64_t _offset;
   uint64_t _timeStamp;
   pthread_t _tid;
   std::string _fileName;

};


#endif /*TRACKSYSCALL_H_*/
