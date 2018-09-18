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
#include "tracksyscall.h"
#include "bbserver_flightlog.h"
#include "bberror.h"
#include <map>


uint64_t getTimeBaseScale();
static const uint64_t TimeBaseScale = getTimeBaseScale();


// record when to send a RAS message to warn or indicate stuck on file system IO.
// uint64_t(-1) is MAX ticks which is practically forever
// The scale multiple is the seconds multiplied by TimeBaseScale

static uint64_t warnSeconds=30;
static uint64_t stuckSeconds=60;
static uint64_t warnTicks=0;
static uint64_t stuckTicks=0;

void TrackSyscall::reset()
{
    _syscall = nosyscall;
    _timeStamp=0;
    _lineNumber=0;
    _tid=-1;
    _fd=-1;
    _fileName.clear();
}

thread_local TrackSyscallPtr threadLocalTrackSyscallPtr = new TrackSyscall();

pthread_mutex_t tidTrackerMutex = PTHREAD_MUTEX_INITIALIZER;
std::map<pthread_t, TrackSyscallPtr>   pthread_syscalltracker;
typedef std::pair<pthread_t, TrackSyscallPtr>  pthreadTrackPair;

void setThresholdTimes4Syscall(const uint64_t pWarnSeconds, const uint64_t pStuckSeconds)
{
    pthread_mutex_lock(&tidTrackerMutex);
    if (pWarnSeconds)
    {
        warnSeconds = pWarnSeconds;
        warnTicks = warnSeconds * TimeBaseScale;

    }
    else
    {
        warnSeconds = uint64_t(-1);
        warnTicks = uint64_t(-1);
    }
    if (pStuckSeconds){
        stuckSeconds = pStuckSeconds;
        stuckTicks = stuckSeconds * TimeBaseScale;
    }
    else
    {
        stuckSeconds=uint64_t(-1);
        stuckTicks=uint64_t(-1);
    }
    pthread_mutex_unlock(&tidTrackerMutex);
}

double ticks2seconds(uint64_t ticks) { return double(ticks)/double(TimeBaseScale); }

void checkForStuckSyscall()
{
    uint64_t nowTimeStamp = timeStamp();
    uint64_t tick_diff = 0;

    pthread_mutex_lock(&tidTrackerMutex);
    auto l_pthread_syscalltracker = pthread_syscalltracker; //make a local copy
    pthread_mutex_unlock(&tidTrackerMutex);

    for (auto iter : l_pthread_syscalltracker)
    {
        //auto l_tid = iter.first;
        TrackSyscallPtr l_ptr=iter.second;
        if( l_ptr-> _timeStamp)
        {
            tick_diff = nowTimeStamp - l_ptr->  _timeStamp;
            if ( (tick_diff >= stuckTicks) && (l_ptr->_rasCount < 2) )
            {
                 double secondsDiff = ticks2seconds(tick_diff);
                 FL_Write6(FLBBUsage, FLIOSTUCK, "Exceeded the stuck limit.  syscall=%ld fd=%ld line number=%ld offset=%ld  size=%ld seconds=%ld", l_ptr->_syscall,l_ptr->_fd,l_ptr->_lineNumber,l_ptr->_offset,l_ptr->_size,uint64_t(secondsDiff));
                 l_ptr-> _rasCount=2;
                 bberror << err("syscall.file.seconds",secondsDiff) << err("syscall.file.stuck",stuckSeconds);
                 l_ptr->doRASbberror(bb.sc.stuckIO);
            }
            else if ( (tick_diff >= warnTicks) && (!l_ptr->_rasCount) )
            {
                double secondsDiff = ticks2seconds(tick_diff);
                FL_Write6(FLBBUsage, FLIOWARN, "Exceeded the warn limit.  syscall=%ld fd=%ld line number=%ld offset=%ld  size=%ld seconds=%ld", l_ptr->_syscall,l_ptr->_fd,l_ptr->_lineNumber,l_ptr->_offset,l_ptr->_size,uint64_t(secondsDiff));
                l_ptr->_rasCount=1;
                bberror << err("syscall.file.seconds",secondsDiff) << err("syscall.file.warn",warnSeconds);
                l_ptr->doRASbberror(bb.sc.longIO);
            }
        }
    }
}

TrackSyscall::TrackSyscall()
{
    _tid = pthread_self();
    _timeStamp = 0;
    _syscall = nosyscall;
    _fd = -1;
    _timeStamp = 0;
    _lineNumber = 0;
    _rasCount = 0;
    _size=0;
    _offset=0;
    pthread_mutex_lock(&tidTrackerMutex);
    pthread_syscalltracker[_tid] = this;
    pthread_mutex_unlock(&tidTrackerMutex);
}


TrackSyscallPtr getSysCallTracker()
{
    return threadLocalTrackSyscallPtr; //thread local
}

uint64_t getTimeBaseScale()
{
	uint64_t timebaseScale = 1;
#ifdef __linux__
	FILE* f;
	char* ptr;
	char line[256];
	f = fopen("/proc/cpuinfo", "r");
	while(!feof(f))
	{
	    char* str = fgets(line, sizeof(line), f);
	    if(str == NULL)
  	        break;

	    if((ptr = strstr(line, "cpu MHz")) != 0)  // x86
	    {
		    ptr = strchr(ptr, ':');
		    ptr += 2;
		    sscanf(ptr, "%ld", &timebaseScale);
		    timebaseScale *= 1000000;
	    }
	    if((ptr = strstr(line, "timebase")) != 0) // powerpc
	    {
		    ptr = strchr(ptr, ':');
		    ptr += 2;
		    sscanf(ptr, "%ld", &timebaseScale);
            //printf("timebaseScale=%ld\n",timebaseScale);
	    }
	}
	fclose(f);

#endif
    warnTicks = warnSeconds * timebaseScale;
    stuckTicks = stuckSeconds * timebaseScale;
    return timebaseScale;
}

 void TrackSyscall::doRASbberror(const std::string& RASeventID){
    const int FDSYM=128;
    char buf2fd[FDSYM];
    buf2fd[0]=0;
    const int MAXBUFF=4096;
    char buff[MAXBUFF];
    buff[0]=0;

    bberror << err("syscall.mainpid",getpid() );
    bberror << err("syscall.pthread_t.decimal", _tid);
    int len = snprintf(buff,MAXBUFF-1,"%p",(void*)_tid);
    if (len>0) bberror<<err("syscall.pthread_t.hex",buff);
    buff[0]=0;
    if (_fileName.size() )
    {
        bberror << err("syscall.file.name", _fileName);
    }
    else if (_fd>0)
    {
        len = snprintf(buf2fd,FDSYM-1,"/proc/%d/fd/%d",getpid(), _fd);
        if (len>0) {
            int rc = readlink(buf2fd,buff,MAXBUFF);
            if (rc>0) {
                if (rc<MAXBUFF) buff[rc]=0;
                else buff[MAXBUFF-1]=0;
                _fileName = buff;
                bberror << err("syscall.file.name", _fileName);
            }
        }
    }
    else bberror << err("syscall.file.nameNA",_fd );

    switch ( _syscall )
    {
        case opensyscall: bberror << err("syscall.file.literal","opensyscall"); break;
        case preadsyscall: bberror << err("syscall.file.literal","preadsyscall"); break;
        case pwritesyscall:bberror << err("syscall.file.literal","pwritesyscall"); break;
        case statsyscall:bberror << err("syscall.file.literal","statsyscall"); break;
        case fstatsyscall:bberror << err("syscall.file.literal","fstatsyscall"); break;
        case fsyncsyscall:bberror << err("syscall.file.literal","fsyncsyscall"); break;
        case openexlayout: bberror << err("syscall.file.literal","openexlayout"); break;
        case setupexlayout: bberror << err("syscall.file.literal","setupexlayout"); break;
        case finalizeexlayout: bberror << err("syscall.file.literal","finalizeexlayout"); break;

        case SSDopenwritedirect:bberror << err("syscall.file.literal","SSDopenwritedirect"); break;
        case SSDopenwriteNOTdirect:bberror << err("syscall.file.literal","SSDopenwriteNOTdirect"); break;
        case SSDpreadsyscall:bberror << err("syscall.file.literal","SSDpreadsyscall"); break;
        case SSDpwritesyscall:bberror << err("syscall.file.literal","SSDpwritesyscall"); break;
        case SSDopenreaddirect:bberror << err("syscall.file.literal","SSDopenreaddirect"); break;
        case nosyscall:bberror << err("syscall.file.literal","nosyscall"); break;
        default: break;
    }
    bberror << err("syscall.file.enum",(int) _syscall);
    if (_fd>0) bberror << err("syscall.file.fd", _fd);
    else bberror<<err("syscall.file.fd","NA");
    if  (_size) bberror << err("syscall.file.rwsize",_size);
    if  (_offset) bberror << err("syscall.file.rwoffset",_offset);
    if (_lineNumber) bberror << err("syscall.file.linenumber",_lineNumber);
    bberror << RAS(RASeventID);
}
