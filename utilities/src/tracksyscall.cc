/*******************************************************************************
 |    tracksyscall.cc
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
std::map<TrackSyscallPtr, TrackSyscallPtr>   pthread_syscalltracker;

std::map<TrackSyscallPtr, TrackSyscallPtr> get_pthread_syscalltracker(){return pthread_syscalltracker;}


void TrackSyscall::makeEntry(){
    pthread_mutex_lock(&tidTrackerMutex);
    pthread_syscalltracker[this] = this;
    pthread_mutex_unlock(&tidTrackerMutex);
}
void TrackSyscall::removeEntry(){
    pthread_mutex_lock(&tidTrackerMutex);
    pthread_syscalltracker.erase(this);
    pthread_mutex_unlock(&tidTrackerMutex);
}
TrackSyscall::~TrackSyscall(){
    removeEntry();
}

void locktidTrackerMutex(){pthread_mutex_lock(&tidTrackerMutex);}
void unlocktidTrackerMutex(){pthread_mutex_unlock(&tidTrackerMutex);}

