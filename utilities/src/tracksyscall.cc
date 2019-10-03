/*******************************************************************************
 |    tracksyscall.h
 |
 |  © Copyright IBM Corporation 2015,2019. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/
#include "tracksyscall.h"


static pthread_mutex_t tidTrackerMutex = PTHREAD_MUTEX_INITIALIZER;
std::map<pthread_t, TrackSyscallPtr>   pthread_syscalltracker;

std::map<pthread_t, TrackSyscallPtr> get_pthread_syscalltracker(){return pthread_syscalltracker;}

TrackSyscall::TrackSyscall()
{
    _tid = pthread_self();
    _timeStamp = 0;
    _syscall = nosyscall;
    _fd = -1;
    _lineNumber = 0;
    _rasCount = 0;
    _size=0;
    _offset=0;
    pthread_mutex_lock(&tidTrackerMutex);
    pthread_syscalltracker[_tid] = this;
    pthread_mutex_unlock(&tidTrackerMutex);
}

void locktidTrackerMutex(){pthread_mutex_lock(&tidTrackerMutex);}
void unlocktidTrackerMutex(){pthread_mutex_unlock(&tidTrackerMutex);}


thread_local TrackSyscallPtr threadLocalTrackSyscallPtr=new TrackSyscall();
TrackSyscallPtr getSysCallTracker()
{
    return threadLocalTrackSyscallPtr; //thread local
}

