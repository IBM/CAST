/*******************************************************************************
 |    flightlog.c
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


#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h> /* mmap() is defined in this header */
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <fcntl.h>
#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <sys/sysctl.h>
#endif
#include "flightlog.h"

#ifndef MAX
#define MAX(a,b) ((a>b)?a:b)
#endif

static int FL_RAWTIME = 0;

extern int processPlugin(size_t bufferSize, char* buffer, FlightRecorderRegistry_t* reg, FlightRecorderLog_t* log);
extern int loadPlugin(FlightRecorderRegistry_t* reg);


int FL_CreateRegistries(const char* rootpath, unsigned int numreg, FlightRecorderCreate_t* flcreate, uint64_t csum)
{
    int rc;
    unsigned int x;
    char filename[256];
	for (x = 0; x < numreg; x++)
	{
		snprintf(filename, sizeof(filename), "%s/%s", rootpath, flcreate[x].filename);
		rc = FL_CreateRegistry(flcreate[x].reg, flcreate[x].name, filename, flcreate[x].decoderName, flcreate[x].size, (FlightRecorderFormatter_t *)flcreate[x].fmt, flcreate[x].numenums, csum);
		if (rc)
			return rc;
	}
	return 0;
}

int FL_AttachRegistry(FlightRecorderRegistryList_t** reglist, const char* filename, FlightRecorderRegistry_t* reg)
{
	int rc;
	int fdout;
	struct stat statbuffer;
	if(reg == NULL)
	{
		fdout = open(filename, O_RDWR | O_CLOEXEC, S_IRWXU);
		if(fdout == -1)
		    return -1;
		
		rc = fstat(fdout, &statbuffer);
		if(rc != 0)
		{
			close(fdout);
			return -1;
		}
		reg = (FlightRecorderRegistry_t*)mmap(NULL, (size_t)statbuffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FILE, fdout, 0);
	}
	
	FlightRecorderRegistryList_t* tmp = (FlightRecorderRegistryList_t*)malloc(sizeof(FlightRecorderRegistryList_t));
	memset(tmp, 0, sizeof(FlightRecorderRegistryList_t));
	
	tmp->nextRegistry = *reglist;
	tmp->reg = reg;
	tmp->flightformatter = (FlightRecorderFormatter_t*)((char*)reg + FLIGHTLOG_OFFSET + reg->flightsize * sizeof(FlightRecorderLog_t));
	tmp->flightlog = (FlightRecorderLog_t*)((char*)reg + FLIGHTLOG_OFFSET);
	
	unsigned int x;
	for(tmp->maxidlen=0, x=0; x<reg->num_ids; x++)
	{
	    tmp->maxidlen = MAX(tmp->maxidlen, strlen(tmp->flightformatter[x].id_str));
	}

	*reglist = tmp;
	rc = loadPlugin(reg);
	return rc;
}

int FL_CreateRegistry(FlightRecorderRegistry_t** reghandle, const char* name, const char* filename, const char* decoder, uint64_t length, FlightRecorderFormatter_t* fmt, uint64_t numids, uint64_t csum)
{
        int rc = 0;
	FlightRecorderRegistry_t* reg;
	
#if __APPLE__
	int private_mapflags = MAP_PRIVATE | MAP_ANON;
#else
	int private_mapflags = MAP_PRIVATE | MAP_ANONYMOUS;
#endif

	int mapflags;
	int fdout = -1;
	int doRegistrySetup = 0;
	size_t mmapsize = 0;
	size_t pagesize =  (size_t)sysconf(_SC_PAGESIZE);
	mmapsize = sizeof(FlightRecorderRegistry_t) + 
		sizeof(FlightRecorderLog_t) * length + 
		sizeof(FlightRecorderFormatter_t) * numids +
		64;
	mmapsize = ((mmapsize+pagesize-1) & (~(pagesize-1)));
	if((filename != NULL) && (strncmp(filename, "none/",5) != 0))
	{
		mapflags = MAP_SHARED | MAP_FILE;
		fdout = open(filename, O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, S_IRWXU);
		if(fdout != -1)
		{
			doRegistrySetup = 1;
		    int rc = ftruncate(fdout, (off_t)mmapsize);
			if (rc)
			{
				close(fdout);
				fdout = -1;
			}
		}
		else
		{
			fdout = open(filename, O_RDWR | O_CLOEXEC, S_IRWXU);
			if(fdout != -1)
			{	
				// unable to open flightlog at the path specified.  Fail but anonymous mmap structures (but not backed by a file)
				doRegistrySetup = 0;
			}
		}
	}
	if(fdout == -1)
	{
		doRegistrySetup = 1;
		mapflags = private_mapflags;
		fdout = -1;
	}
	
	*reghandle = reg = (FlightRecorderRegistry_t*)mmap(NULL, mmapsize, PROT_READ | PROT_WRITE, mapflags, fdout, 0);
	if((char*)reg == (char*)-1)
	{
		printf("Error mapping flightlog.   errno=%d\n", errno);
		exit(-1);
	}

	unsigned long bootid = 0;
#ifdef __linux__
	char buffer[1024];
	FILE* fdbootid = fopen("/proc/sys/kernel/random/boot_id", "r");
	fgets(buffer, sizeof(buffer), fdbootid);
	bootid = strtoul(buffer, NULL, 16);
	fclose(fdbootid);
#endif

	if(doRegistrySetup == 0)
	{
		if((reg->flightchecksum != csum) || 
		   (reg->bootid != bootid))
		{
			// slight race condition whereby same flightlog is generated in parallel.  
			close(fdout);
			unlink(filename);
			return FL_CreateRegistry(reghandle, name, filename, decoder, length, fmt, numids, csum); // try again
		}
	}

	if(doRegistrySetup)
	{
		reg->flightchecksum = csum;
		reg->flightlock    = 0;
		reg->flightsize    = length;
		reg->bootid        = bootid;

		// Registry Formatting data, not used by runtime:
#ifdef __linux__
		FILE *f;
		char *ptr;
		char line[256];
		float tmpf;
		f = fopen("/proc/cpuinfo", "r");
		if(f == NULL)
		{
			printf("Unable to open /proc/cpuinfo, failing.  errno=%d", errno);
			exit(-1);
		}
		while (!feof(f))
		{
			char *str = fgets(line, sizeof(line), f);
			if (str == NULL)
				break;

			if ((ptr = strstr(line, "cpu MHz")) != 0) // x86
			{
				ptr = strchr(ptr, ':');
				ptr += 2;
				sscanf(ptr, "%g", &tmpf);
				reg->timebaseScale = ((double)tmpf * 1000000.0);
			}
			if ((ptr = strstr(line, "timebase")) != 0) // powerpc
			{
				ptr = strchr(ptr, ':');
				ptr += 2;
				sscanf(ptr, "%g", &tmpf);
				reg->timebaseScale = (double)tmpf;
			}
		}
		fclose(f);
#endif
#ifdef __APPLE__
		uint64_t speed = 0;
		size_t   len = sizeof(speed);
		mach_timebase_info_data_t info;
		mach_timebase_info(&info);
		sysctlbyname("hw.cpufrequency_max", &speed, &len, NULL, 0);
		reg->timebaseScale = speed / ((double)info.numer / (double)info.denom);
#endif

		uint64_t tb = 0;
#ifdef __powerpc64__
		asm volatile("mfspr %0,%1" : "=&r" (tb) : "i" (SPRN_TBRO) : "cc", "memory", "r3");
#elif __x86_64__
		unsigned hi, lo;
		__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
		tb = ((uint64_t)hi << 32ull) | lo;
#endif
		struct timeval tv;
		gettimeofday(&tv, NULL);
		double initEpoch = tv.tv_sec + tv.tv_usec/1000000.0;
		reg->timebaseAdjust = initEpoch * reg->timebaseScale - tb;
		
		FlightRecorderFormatter_t* tmpfmt = (FlightRecorderFormatter_t*)((char*)reg + FLIGHTLOG_OFFSET + reg->flightsize * sizeof(FlightRecorderLog_t));
		memcpy(tmpfmt, fmt, sizeof(FlightRecorderFormatter_t) * numids);
		reg->num_ids = numids;
		((char*)reg->registryName)[sizeof(reg->registryName)-1] = 0;
		strncpy((char*)reg->registryName, name, sizeof(reg->registryName)-1);	
		
		((char*)reg->decoderName)[sizeof(reg->decoderName)-1] = 0;
		strncpy((char*)reg->decoderName, decoder, sizeof(reg->decoderName)-1);
	}

//	rc = loadPlugin(reg);
	return rc;
}

int displayTime(char* buffer, size_t bufferSize, uint64_t timebase, double scale, uint64_t timebaseAdjust)
{
    if(FL_RAWTIME)
    {
        return snprintf(buffer, bufferSize, "TB=%016llx", (unsigned long long)timebase);
    }
    double wallTime = (timebase / scale);
    time_t wallTimeSeconds  = wallTime;
	double initEpoch = (timebaseAdjust / scale);  // add the timebaseAdjust separately due to precision limits on 'double' type
    time_t initEpochSeconds = initEpoch;
    uint64_t wallTimeFraction = (wallTime - wallTimeSeconds)*1000000000.0;
    wallTimeSeconds  += initEpochSeconds;
    wallTimeFraction += (initEpoch - initEpochSeconds) * 1000000000.0;
    if(wallTimeFraction >= 1000000000)
    {
        wallTimeSeconds++;
        wallTimeFraction -= 1000000000;
    }
    struct tm result;
    localtime_r(&wallTimeSeconds, &result);
    ssize_t rc =strftime(buffer, bufferSize, "%F %T.000000000 %Z", &result);
    if(rc >= 0)
	{
		buffer = strstr(buffer, ".0000");
		if(buffer)
		{
			char tmpbuffer[16];
			snprintf(tmpbuffer, sizeof(tmpbuffer), ".%09llu", (unsigned long long)wallTimeFraction);
			memcpy(buffer, tmpbuffer, 10);
		}
	}
	return rc;
}

int FL_Decode(FlightRecorderRegistryList_t* logregistryHead, size_t bufferSize, char* buffer, uint64_t* moreData, uint64_t flags)
{
	uint32_t set;
	uint64_t x;
	uint64_t length;
	uint64_t nexttime;
	uint64_t offset;
	uint64_t currid = 0;
	size_t   maxidsize = sizeof("FL_BEGIN_LOG");
	FlightRecorderRegistryList_t* logregistry;
	FlightRecorderRegistryList_t* lsel;
	FlightRecorderFormatter_t* fmt;
	FlightRecorderLog_t* log;
	char     fmt1str[64];
	char     fmt2str[64];
	
    if((flags & FLDECODEFLAGS_RAWMODE) != 0)
    {
        FL_RAWTIME = 1;
        length = snprintf(buffer, bufferSize, "Scale=%g  adjustTimebase=%ld\n", logregistryHead->reg->timebaseScale, logregistryHead->reg->timebaseAdjust);
        bufferSize -= length;
        buffer     += length;
    }
    else
        FL_RAWTIME = 0;
    
    // Acquire current flight recorder lock pointers
	for(logregistry=logregistryHead; logregistry != NULL; logregistry=logregistry->nextRegistry)
	{
	        logregistry->id = currid++;
		maxidsize = MAX(maxidsize, logregistry->maxidlen);
		
		if(logregistry->lastStateSet == 0)
		{
#if 0
			offset = L2_AtomicLoad(&logregistry->flightlock);
			logregistry->count = offset % logregistry->flightsize;
#else
			offset = logregistry->reg->flightlock;
			logregistry->count = offset % logregistry->reg->flightsize;
#endif
			logregistry->lastOffset = offset;
			logregistry->counttotal = 0;
            
			for(x=0; x<logregistry->reg->flightsize; x++)
			{
				if((logregistry->flightlog[ logregistry->count ].id) != 0)
					break;
				logregistry->count = (logregistry->count+1) % logregistry->reg->flightsize;
				logregistry->counttotal++;
			}
		}
		else
		{
			logregistry->count      = logregistry->lastState;
			logregistry->counttotal = logregistry->lastStateTotal;
		}
	}
	snprintf(fmt1str, sizeof(fmt1str), " %%%lds:%%-2d ", maxidsize);
	snprintf(fmt2str, sizeof(fmt2str), " %%%lds:-- ", maxidsize);
	
	do
	{
		*moreData = 0;
		nexttime = 0xffffffffffffffffull;
		lsel     = NULL;
		for(logregistry=logregistryHead; logregistry != NULL; logregistry=logregistry->nextRegistry)
		{
			if((logregistry->flightlog[logregistry->count].timestamp < nexttime) && (logregistry->counttotal < logregistry->reg->flightsize))
			{
				nexttime = logregistry->flightlog[logregistry->count].timestamp;
				lsel = logregistry;
			}
		}
		
		if(lsel != NULL)
		{
		    *moreData = lsel->lastOffset + lsel->counttotal + (lsel->id<<48) + 1;
		    if(bufferSize < 256)
		    {
			    return 0;
		    }
		    set = lsel->lastStateSet;
		    log = &lsel->flightlog[lsel->count];
		    lsel->count = (lsel->count+1) % lsel->reg->flightsize;
		    lsel->counttotal++;
		    lsel->lastStateSet = 1;
		    lsel->lastState = lsel->count;
		    lsel->lastStateTotal = lsel->counttotal;
		    
		    if(log->id >= lsel->reg->num_ids)
		    {
  		        length = displayTime(buffer, bufferSize, log->timestamp, logregistryHead->reg->timebaseScale, logregistryHead->reg->timebaseAdjust);
                bufferSize -= length;
                buffer     += length;
			
                length = (uint64_t)snprintf(buffer, bufferSize, fmt1str, "FL_INVALDLOG", log->hwthread);
                bufferSize -= length;
                buffer     += length;
                
                length = (uint64_t)snprintf(buffer, bufferSize, "An invalid entry with registry=\"%s\"  id=%d was detected (valid ID range 0-%lld)\n", 
                          lsel->reg->registryName, log->id, (unsigned long long)lsel->reg->num_ids);
                bufferSize -= length;
                buffer     += length;
                (void)bufferSize;
                (void)buffer;
                return 0;
		    }
		    
		    fmt = &lsel->flightformatter[log->id];
		    if(set == 0)
		    {
  		        length = displayTime(buffer, bufferSize, log->timestamp, logregistryHead->reg->timebaseScale, logregistryHead->reg->timebaseAdjust);
                bufferSize -= length;
                buffer     += length;
                
                length = (uint64_t)snprintf(buffer, bufferSize, fmt2str, "FL_BEGIN_LOG");
                bufferSize -= length;
                buffer     += length;
                
                length = (uint64_t)snprintf(buffer, bufferSize, "Starting log \"%s\"\n", lsel->reg->registryName);
                bufferSize -= length;
                buffer     += length;
            }
            length = displayTime(buffer, bufferSize, log->timestamp, logregistryHead->reg->timebaseScale, logregistryHead->reg->timebaseAdjust);
            bufferSize -= length;
            buffer     += length;
            
            length = (uint64_t)snprintf(buffer, bufferSize, fmt1str, fmt->id_str, log->hwthread);
            bufferSize -= length;
            buffer     += length;
	    
	    if(processPlugin(bufferSize, buffer, lsel->reg, log) == 0)
	    {
                length = strlen(buffer);
                bufferSize -= length;
                buffer     += length;
	    }
	    else
	    {
                length = (uint64_t)snprintf(buffer, bufferSize, fmt->formatString, log->data[0], log->data[1], log->data[2], log->data[3], log->data[4], log->data[5]);
                bufferSize -= length;
                buffer += length;
	    }
	    
            length = (uint64_t)snprintf(buffer, bufferSize, "\n");
            bufferSize -= length;
            buffer     += length;
        }
    }
    while(*moreData);
    return 0;
}
