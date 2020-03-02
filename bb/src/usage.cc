/*******************************************************************************
 |    usage.cc
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


#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>

#include <iostream>
#include <map>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/tokenizer.hpp>
#include <boost/regex.hpp>
#include <boost/property_tree/ptree.hpp>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/sysmacros.h>

#include "bbinternal.h"
#include "connections.h"
#include "HandleFile.h"

using namespace std;
using namespace boost;
namespace bfs = boost::filesystem;

#ifdef PROF_TIMING
#include <ctime>
#include <chrono>
#endif

#if BBSERVER
void checkForStuckSyscall();
#endif

#include "bbapi.h"
#include "bbproxy_flightlog.h"
#include "identity.h"
#include "logging.h"
#include "Msg.h"
#include "bberror.h"
#include "bbcounters.h"

pthread_mutex_t         monitorlock = PTHREAD_MUTEX_INITIALIZER;
map<string, BBUsage_t>  monitorlist;

pthread_mutex_t                     rwUsageLock    = PTHREAD_MUTEX_INITIALIZER;
map<dev_t, class BBUsageExtended>   bbxfer_usage;

static atomic<int64_t> g_Last_Port_Rcv_Data_Delta(-1);
static atomic<int64_t> g_Last_Port_Xmit_Data_Delta(-1);
static string g_IB_Adapter = "mlx5_0";


#define sectorsize 512  /// \todo calculate sector size using fdisk -l (or /sys/block/<block>/queue/hw_sector_size

class BBUsageExtended
{
  public:

    BBUsageExtended()
    {
        _localTareRead=0;
        _localTareWritten=0;
        _localTareReadCount=0;
        _localTareWriteCount=0;
        _burstBytesRead=0;
        _burstBytesWritten=0;
    }
    ~BBUsageExtended()
    {
        FL_Write(FLBBUsage,TareUsageFreed,"Freed localTareRead=%ld localTareWritten=%ld sectorsize=%lu", _localTareRead,_localTareWritten,sectorsize,0);
    }
    void    init(dev_t pDev)
    {
        _localTareRead=0;
        _localTareWritten=0;
        _localTareReadCount=0;
        _localTareWriteCount=0;
        _burstBytesRead=0;
        _burstBytesWritten=0;

        uint32_t major = major(pDev);
        uint32_t minor = minor(pDev);
        _statpath = string("/sys/dev/block/") + to_string(major) + ":" + to_string(minor) + "/stat";
        getLocalReadWrite(_localTareRead, _localTareWritten, _localTareReadCount, _localTareWriteCount);
        FL_Write6(FLBBUsage,SetUsageTare, "localTareRead=%ld localTareWritten=%ld major=%lu minor=%lu sectorsize=%lu", _localTareRead,_localTareWritten,major,minor,sectorsize,0);
    }

    int getlocalUsage(uint64_t& pLocalRead, uint64_t& pLocalWrite, uint64_t& pLocalReadCount, uint64_t& pLocalWriteCount)
    {
        pLocalRead=0;
        pLocalWrite=0;
        pLocalReadCount=0;
        pLocalWriteCount=0;

        int rc=getLocalReadWrite( pLocalRead, pLocalWrite, pLocalReadCount, pLocalWriteCount);
        if(rc) return rc;
        pLocalRead  -= _localTareRead;
        pLocalWrite -= _localTareWritten;
        pLocalReadCount  -= _localTareReadCount;
        pLocalWriteCount -= _localTareWriteCount;
        // FL_Write(FLBBUsage,GetUsageLocal, "LocalRead=%ld LocalWrite=%ld sectorsize=%lu", pLocalRead,pLocalWrite,sectorsize);
        return 0;
    }

    void bumpBurstUsage(uint64_t byteswritten, uint64_t bytesread)
    {
        _burstBytesRead    += bytesread;
        _burstBytesWritten += byteswritten;
    }
    int getburstUsage(uint64_t& pBurstRead, uint64_t& pBurstWrite)
    {
        pBurstRead = _burstBytesRead;
        pBurstWrite = _burstBytesWritten;
        return 0;
    }

private:
    int getLocalReadWrite(uint64_t& pLocalRead, uint64_t& pLocalWrite, uint64_t& pLocalReadCount, uint64_t& pLocalWriteCount)
    {
        int rc = 0;
        char*  line = NULL;
        size_t linelength;
        FILE* fd = fopen(_statpath.c_str(), "r");
        if(fd == NULL)
            return -1;

        ssize_t llen = getline(&line, &linelength, fd);
        fclose(fd);
        if(llen < 1)
            return -1;
        string line_str = line;
        free(line);

        vector<uint64_t> values;
        boost::char_separator<char> sep(" ");
        tokenizer< boost::char_separator<char>  > tok(line_str, sep);
        for(tokenizer< boost::char_separator<char> >::iterator beg=tok.begin(); beg!=tok.end();++beg)
            values.push_back(stoull(*beg));

        pLocalRead       = values[6-4] * sectorsize;
        pLocalWrite      = values[10-4] * sectorsize;
        pLocalReadCount  = values[4-4];
        pLocalWriteCount = values[8-4];
        return rc;
    }

    uint64_t _localTareWritten;
    uint64_t _localTareRead;
    uint64_t _localTareWriteCount;
    uint64_t _localTareReadCount;
    uint64_t _burstBytesRead;     ///< Number of bytes written to the logical volume via burst buffer transfers
    uint64_t _burstBytesWritten;  ///< Number of bytes read from the logical volume via burst buffer transfers

    std::string _statpath;
};

static int getDevice(const char* mountpoint, dev_t& devinfo)
{
    struct stat statbuf;
    int rc=stat(mountpoint,&statbuf);
    if(rc) return rc;

    devinfo = statbuf.st_dev;
    return 0;
}

#if BBSERVER
int highIB_Activity()
{
    int rc = 0;

    if (g_Last_Port_Rcv_Data_Delta > 0)
    {
        if (g_Last_Port_Rcv_Data_Delta < g_IBStatsLowActivityClipValue)
        {
            if (g_Last_Port_Xmit_Data_Delta > 0)
            {
                if (g_Last_Port_Xmit_Data_Delta >= g_IBStatsLowActivityClipValue)
                {
                    rc = 1;
                }
            }
            else
            {
                // Undetermined IB activity
                rc = -1;
            }
        }
        else
        {
            // High IB activity
            rc = 1;
        }
    }
    else
    {
        // Undetermined IB activity
        rc = -1;
    }

    return rc;
}
#endif

int   proxy_regLV4Usage(const char* mountpoint)
{
    int rc;
    dev_t dinfo;

    rc = getDevice(mountpoint, dinfo);
    if (rc) return rc;

    pthread_mutex_lock(&rwUsageLock);
    bbxfer_usage[dinfo].init(dinfo);
    pthread_mutex_unlock(&rwUsageLock);
    return 0;
}

int   proxy_deregLV4Usage(const char* mountpoint)
{
    int rc;
    dev_t dinfo;

    rc = getDevice(mountpoint, dinfo);
    if (rc) return rc;

    // remove monitor map entry
    pthread_mutex_lock(&monitorlock);
    monitorlist.erase(mountpoint);
    pthread_mutex_unlock(&monitorlock);

    pthread_mutex_lock(&rwUsageLock);
    bbxfer_usage.erase(dinfo);
    pthread_mutex_unlock(&rwUsageLock);
  return 0;
}


int proxy_GetUsage(const char* mountpoint, BBUsage_t& usage)
{
    int    rc = ENOENT;

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif

    memset((char*)&usage, 0, sizeof(usage));

    dev_t dinfo;
    rc = getDevice(mountpoint, dinfo);
    if(rc) return rc;

    if (bbxfer_usage.find(dinfo) != bbxfer_usage.end() )
    {
#if BBUSAGE_COUNT
        rc = bbxfer_usage[dinfo].getlocalUsage(usage.localBytesRead, usage.localBytesWritten, usage.localReadCount, usage.localWriteCount);
#else
        uint64_t dummy1,dummy2;
        rc = bbxfer_usage[dinfo].getlocalUsage(usage.localBytesRead, usage.localBytesWritten, dummy1, dummy2);
#endif
        if(rc == 0)
            rc = bbxfer_usage[dinfo].getburstUsage(usage.burstBytesRead, usage.burstBytesWritten);

        if(rc == 0)
        {
            usage.totalBytesRead    = usage.localBytesRead    + usage.burstBytesRead;
            usage.totalBytesWritten = usage.localBytesWritten + usage.burstBytesWritten;
            FL_Write6(FLBBUsage,GetUsageLV, "LocalRead=%ld LocalWrite=%ld BurstRead=%lu BurstWrite=%lu dev=%lu",usage.localBytesRead,usage.localBytesWritten,usage.burstBytesRead,usage.burstBytesWritten,0,0);
        }
    }
    else
    {
        rc=ENOENT;
        LOG(bb,error)<<"No usage entry for mountpoint="<<mountpoint;
    }
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return rc;
}


int proxy_BumpBBUsage(dev_t pDev, uint64_t byteswritten, uint64_t bytesread)
{
    pthread_mutex_lock(&rwUsageLock);
    bbxfer_usage[pDev].bumpBurstUsage(byteswritten,bytesread);
    pthread_mutex_unlock(&rwUsageLock);
    return 0;
}

extern string get_bb_nvmecliPath();
extern string getNVMeByIndex(uint32_t index);
int proxy_GetDeviceUsage(uint32_t devicenum, BBDeviceUsage_t& usage)
{
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif

    map<string,string> field;

    memset(&usage,0,sizeof(BBDeviceUsage_t) );
    string cmd = get_bb_nvmecliPath();
    string device = getNVMeByIndex(devicenum);
    cmd = cmd + " smart-log " + device;
    for(const auto& line : runCommand(cmd))
    {
        if(line.find("Smart Log") != string::npos)
            continue;

        auto tokens = buildTokens(line, ":");

        string name = tokens[0];
        string val  = tokens[1];
        size_t index;
        name.erase(name.find_last_not_of(" ")+1);
        while((index = val.find_first_not_of(".0123456789")) != string::npos)
        {
            val.erase(index,1);
        }

        LOG(bb,always) << "name=" << name << ".   val=" << val << ".";
        field[name] = val;
    }

    field["data_read"]    = to_string(stoull(field["data_units_read"])*512000);
    field["data_written"] = to_string(stoull(field["data_units_written"])*512000);
    field["num_read_commands"] = field["host_read_commands"];
    field["num_write_commands"] = field["host_write_commands"];
    field["busy_time"] = field["controller_busy_time"];

#define copyi(name) usage.name = stoull(field[#name]);
#define copyd(name) usage.name = stod(field[#name]);
    copyi(critical_warning);
    copyd(temperature);
    copyd(available_spare);
    copyd(percentage_used);
    copyi(data_read);
    copyi(data_written);
    copyi(num_read_commands);
    copyi(num_write_commands);
    copyi(busy_time);
    copyi(power_cycles);
    copyi(power_on_hours);
    copyi(unsafe_shutdowns);
    copyi(media_errors);
    copyi(num_err_log_entries);

    FL_Write6(FLBBUsage,GetDeviceUsage, "temperature=%ld percentage_used==%ld data_read=%ld  data_written==%ld num_read_commands=%ld num_write_commands=%ld ",usage.temperature,usage.percentage_used,usage.data_read,usage.data_written,usage.num_read_commands,usage.num_write_commands);

    LOG(bb,info) << "usage.temperature="<< usage.temperature<<" available_spare="<<usage.available_spare << " percentage_used="<<usage.percentage_used<<" critical_warning="<<usage.critical_warning<< " data_read="<< usage.data_read<< " data_written="<< usage.data_written<< " num_read_commands="<< usage.num_read_commands<< " num_write_commands="<< usage.num_write_commands;

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return 0;
}

int startMonitoringMount(const char* mountpoint, BBUsage_t limits)
{
    int rc = 0;
    string mountpoint_str = mountpoint;

    pthread_mutex_lock(&monitorlock);
    monitorlist[mountpoint_str] = limits;
    pthread_mutex_unlock(&monitorlock);

    return rc;
}

int stopMonitoringMount(const char* mountpoint)
{
    int rc = 0;
    string mountpoint_str = mountpoint;

    pthread_mutex_lock(&monitorlock);
    monitorlist.erase(mountpoint_str);
    pthread_mutex_unlock(&monitorlock);
    return rc;
}

struct rusage operator-(const struct rusage& lhs, const struct rusage& rhs)
{

    // removed  unmaintained per man page

    struct rusage temp;
    //struct timeval ru_utime; /* user CPU time used */
    /* This is the total amount of time spent executing in user mode,
              expressed in a timeval structure (seconds plus microseconds).
    */
    temp.ru_utime.tv_sec = lhs.ru_utime.tv_sec - rhs.ru_utime.tv_sec;
    temp.ru_utime.tv_usec = lhs.ru_utime.tv_usec - rhs.ru_utime.tv_usec;

    //struct timeval ru_stime; /* system CPU time used */
    /* This is the total amount of time spent executing in kernel
              mode, expressed in a timeval structure (seconds plus
              microseconds).
    */
    temp.ru_stime.tv_sec = lhs.ru_stime.tv_sec - rhs.ru_stime.tv_sec;
    temp.ru_stime.tv_usec = lhs.ru_stime.tv_usec - rhs.ru_stime.tv_usec;

    temp.ru_maxrss = lhs.ru_maxrss-rhs.ru_maxrss;        /* maximum resident set size used (in kilobytes).*/
    temp.ru_minflt = lhs.ru_minflt - rhs.ru_minflt;        /* number of page faults serviced without any I/O activity (reclaim pages) */
    temp.ru_majflt = lhs.ru_majflt - rhs.ru_majflt;        /* page faults (hard page faults) requiring I/O */
    temp.ru_inblock = lhs.ru_inblock - rhs.ru_inblock;       /* block input operations--number of times the filesystem had to perform input */
    temp.ru_oublock = lhs.ru_oublock - rhs.ru_oublock;       /* block output operations--number of times the filesystem had to perform output */
    temp.ru_nvcsw = lhs.ru_nvcsw - rhs.ru_nvcsw;         /* voluntary context switches (usually to await availability of a resource).*/
    temp.ru_nivcsw = lhs.ru_nivcsw - rhs.ru_nivcsw;        /* involuntary context switches--exceeded its time slice */
    return temp;
}

std::ostream & operator<<(std::ostream &os, const struct rusage& temp)
{
    // removed  unmaintained per man page
    os << " maximum resident set size="<< temp.ru_maxrss<<"(kB)";
    if (temp.ru_minflt) os << " page reclaims="<< temp.ru_minflt;
    if (temp.ru_majflt) os << " hard page faults="<< temp.ru_majflt;
    if (temp.ru_inblock) os << " block input op="<< temp.ru_inblock;
    if (temp.ru_oublock) os << " block output op="<< temp.ru_oublock;
    if (temp.ru_nvcsw)   os << " vol context switches="<< temp.ru_nvcsw;
    if (temp.ru_nivcsw)  os << " invol context switches="<< temp.ru_nivcsw;
    return os;
}

void* mountMonitorThread(void* ptr)
{
    int l_sleepValue = config.get(process_whoami+".ssdusagepollrate", 60);
    LOG(bb,always) << "mountMonitorThread sleep="<<l_sleepValue;
    int ras_max_rss_size = config.get(process_whoami+".ras_max_rss_size", 2*1024*1024);
    LOG(bb,always) << "ras_max_rss_size="<<ras_max_rss_size;

   int rusage_index=0;
   struct rusage l_rusage[2];
   struct rusage l_rusage_base;
   struct rusage l_rusageDelta;
#ifdef BBSERVER
   time_t lastCheck = 0;
#endif

   int getrusage_rc = getrusage(RUSAGE_SELF, &l_rusage_base  );
   if(getrusage_rc)
   {
       LOG(bb,error) << "mountMonitorThread:  initial getrusage() failed.  Exiting";
       return NULL;
   }

   LOG(bb,always) <<"MEM: "<< __PRETTY_FUNCTION__<<" base memory usage="<< l_rusage_base;
   l_rusage[rusage_index] = l_rusage_base;
   FL_Write6(FLMem,GetRuInit, " maximum resident set size=%ld(kB) hard page faults=%ld block input op=%ld lblock output op=%ld vol context switches=%ld invol context switches=%ld ",l_rusage[rusage_index].ru_maxrss,l_rusage[rusage_index].ru_majflt,l_rusage[rusage_index].ru_inblock,l_rusage[rusage_index].ru_oublock,l_rusage[rusage_index].ru_nvcsw,l_rusage[rusage_index].ru_nivcsw);
    while(1)
    {
        bberror.clear("mountMonitorThread");

#if BBPROXY
        pthread_mutex_lock(&monitorlock);
        // see idiom in www.cplusplus.com/reference/map/map/erase/ (stackoverflow)
        for (std::map<string,BBUsage_t>::iterator it=monitorlist.begin(); it!=monitorlist.end();) // no increment, must do in loop
        {
            BBUsage_t current;
            BBUsage_t limit = it->second;

            int errval = proxy_GetUsage(it->first.c_str(), current);
            if (errval==ENOENT){
               LOG(bb,info) << "ENOENT for "<< it->first.c_str()<<"--removing from monitor list";
               it=monitorlist.erase(it); //C++11  return an iterator to the element following element removed (or map::end, if last removed).
               continue;
            }
            else{
               LOG(bb,info) << "mountMonitorThread "<< it->first.c_str()<<" current.totalBytesWritten="<<current.totalBytesWritten << " limit.totalBytesWritten="<<limit.totalBytesWritten;
               LOG(bb,info) << "mountMonitorThread "<< it->first.c_str()<<" current.totalBytesRead="<<current.totalBytesRead << " limit.totalBytesRead="<<limit.totalBytesRead;
            }
            if((current.totalBytesWritten > limit.totalBytesWritten) && (limit.totalBytesWritten != 0))
            {
                FL_Write(FLBBUsage, FLSSDWriteExceeded, "Exceeded the file system limits.  Wrote=%ld  Limit=%ld",
                    current.totalBytesWritten, limit.totalBytesWritten, 0, 0);

                bberror << err("error.mountpoint", it->first);
                bberror << err("error.current_bytes_written", current.totalBytesWritten);
                bberror << err("error.limit_bytes_written", limit.totalBytesWritten);
                bberror << RAS(bb.usage.writeThreshExceeded);
            }

            if((current.totalBytesRead > limit.totalBytesRead) && (limit.totalBytesRead != 0))
            {
                FL_Write(FLBBUsage, FLSSDReadExceeded, "Exceeded the file system limits.  Read=%ld  Limit=%ld",
                    current.totalBytesRead, limit.totalBytesRead, 0, 0);
                bberror << err("error.mountpoint", it->first);
                bberror << err("error.current_bytes_read", current.totalBytesRead);
                bberror << err("error.limit_bytes_read", limit.totalBytesRead);
                bberror << RAS(bb.usage.readThreshExceeded);
            }
          ++it; //need to increment for the for loop (part of idiom with for-iterator-with-erase-inside-loop setup)
        }
        pthread_mutex_unlock(&monitorlock);
#endif
        //Note any increase in memory size
        //https://linux.die.net/man/2/getrusage  monitoring memory use
        if (ras_max_rss_size)
        {
           int rusage_index_prev = 0;
           if (rusage_index)
           {
              rusage_index=0;
              rusage_index_prev = 1;
           }
           else rusage_index=1;
           getrusage_rc = getrusage(RUSAGE_SELF, &l_rusage[rusage_index] );
           if (getrusage_rc) LOG(bb,error)<<"Error in getrusage errno="<<errno;
           if ( (ras_max_rss_size) && (ras_max_rss_size<=l_rusage[rusage_index].ru_maxrss) )
           {
                  LOG(bb,always) <<"MEM RAS thresholdMaxRssExceeded thresholdMaxRssExceeded= " << ras_max_rss_size << l_rusage[rusage_index];
                  bberror << err("error.maximum resident set size(kB)", l_rusage[rusage_index].ru_maxrss);
                  bberror << err("error.threshold",ras_max_rss_size);
                  bberror << RAS(bb.usage.maxRssThreshExceeded);
                  FL_Write(FLMem,RusageThr, "max rss threshold=%ld(kB) maximum resident set size=%ld(kB)",ras_max_rss_size,l_rusage[rusage_index].ru_maxrss,0,0);
                  FL_Write6(FLMem,RusageRAS, " maximum resident set size=%ld(kB) hard page faults=%ld block input op=%ld block output op=%ld vol context switches=%ld invol context switches=%ld ",l_rusage[rusage_index].ru_maxrss,l_rusage[rusage_index].ru_majflt,l_rusage[rusage_index].ru_inblock,l_rusage[rusage_index].ru_oublock,l_rusage[rusage_index].ru_nvcsw,l_rusage[rusage_index].ru_nivcsw);
              l_rusageDelta = l_rusage[rusage_index] - l_rusage[rusage_index_prev];
                  ras_max_rss_size=0; //RAS once for threshold
           }
           if (l_rusage[rusage_index].ru_maxrss - l_rusage[rusage_index_prev].ru_maxrss)
           {
              LOG(bb,debug) <<"MEM: "<< l_rusage[rusage_index];
              FL_Write6(FLMem,GetRusage, " maximum resident set size=%ld(kB) hard page faults=%ld block input op=%ld block output op=%ld vol context switches=%ld invol context switches=%ld ",l_rusage[rusage_index].ru_maxrss,l_rusage[rusage_index].ru_majflt,l_rusage[rusage_index].ru_inblock,l_rusage[rusage_index].ru_oublock,l_rusage[rusage_index].ru_nvcsw,l_rusage[rusage_index].ru_nivcsw);
              l_rusageDelta = l_rusage[rusage_index] - l_rusage[rusage_index_prev];
              FL_Write6(FLMem,GetRudelta, " maximum rss delta=%ld(kB) hard page faults delta=%ld blk input op delta=%ld blk output op delta=%ld vol ctxt sw delta=%ld invol ctxt sw delta=%ld ",l_rusageDelta.ru_maxrss, l_rusageDelta.ru_majflt,l_rusageDelta.ru_inblock,l_rusageDelta.ru_oublock,l_rusageDelta.ru_nvcsw,l_rusageDelta.ru_nivcsw);

           }
        }
#if MSG_STALE_CHECK
        // check for unfreed stale messages
        {
           checkForStaleMsg(600);  //a stale message is a message older than (seconds)
        }
#endif
#if BBSERVER
        {
            checkForStuckSyscall(); //any file system IO taking unduly long or stuck?

            time_t curTime   = time(NULL);
            int meminforate = config.get(process_whoami+".meminforate", 3600);

            if(curTime - lastCheck > meminforate)
            {
                lastCheck = curTime;
                for(const auto& line: runCommand(string("/proc/meminfo"), 1))
                {
                    LOG(bb,always) << "MEMINFO: " << line;
                }
            }
        }
#endif
        sleep(l_sleepValue);
    }

    return NULL;
}

unsigned long bbcounters[BB_COUNTER_MAX];

#if BBSERVER
static unsigned long bbcounters_shadow[BB_COUNTER_MAX];

void* diskstatsMonitorThread(void* ptr)
{
    string l_Port_Rcv_Data = "port_rcv_data";
    string l_Port_Xmit_Data = "port_xmit_data";

    time_t start, end;
    map<string,string> diskStatCache;
    string cmd = string("/usr/bin/timeout ") + to_string(g_DiskStatsRate+2) + string(" /usr/bin/iostat -xym -p ALL ") + to_string(g_DiskStatsRate);
    while(1)
    {
        start = time(NULL);
        try
        {
            for(const auto& line : runCommand(cmd))
            {
                auto tokens = buildTokens(line, " ");
                if(tokens.size() > 10)
                {
                    if(diskStatCache[tokens[0]] != line)
                    {
                        diskStatCache[tokens[0]] = line;
                        LOG(bb,always) << "IOSTAT:  " << line;
                    }
                }
            }
            for(const auto& line : runCommand("grep '' /sys/class/infiniband/*/ports/*/*counters/*"))
            {
                auto tokens = buildTokens(line, ":");
                if(tokens.size() == 2)
                {
                    if(diskStatCache[tokens[0]] != tokens[1])
                    {
                        if(!diskStatCache[tokens[0]].empty())
                        {
                            uint64_t l_Delta = stoll(tokens[1]) - stoll(diskStatCache[tokens[0]]);
                            LOG(bb,always) << "IBSTAT:  " << line << " (delta " << l_Delta << ")";
                            if (tokens[0].find(g_IB_Adapter) != string::npos)
                            {
                                if (tokens[0].find(l_Port_Rcv_Data) != string::npos)
                                {
                                    g_Last_Port_Rcv_Data_Delta = l_Delta;
                                }
                                else if (tokens[0].find(l_Port_Xmit_Data) != string::npos)
                                {
                                    g_Last_Port_Xmit_Data_Delta = l_Delta;
                                }
                            }
                        }
                        else
                        {
                            LOG(bb,always) << "IBSTAT:  " << line;
                        }

                        diskStatCache[tokens[0]] = tokens[1];
                    }
                }
            }
            #define MKBBCOUNTER(id) if(bbcounters[BB_COUNTERS_##id] != bbcounters_shadow[BB_COUNTERS_##id]) { LOG(bb,always) << "BB Counter '" #id "' = " << bbcounters[BB_COUNTERS_##id] << " (delta " << (bbcounters[BB_COUNTERS_##id] - bbcounters_shadow[BB_COUNTERS_##id]) << ")"; bbcounters[BB_COUNTERS_##id] = bbcounters_shadow[BB_COUNTERS_##id]; }
            #include "bbcounters.h"
        }
        catch(ExceptionBailout& e)
        {
            // NOTE:  Set the two static xmit/rcv values to -1 so we DO NOT
            //        attempt any additional async job prunes/removals of branches/trees.
            g_Last_Port_Rcv_Data_Delta = -1;
            g_Last_Port_Xmit_Data_Delta = -1;
            LOG(bb,error) << "IOSTAT: exception while executing iostat";
        }
        catch(std::exception& e)
        {
            // NOTE:  Set the two static xmit/rcv values to -1 so we DO NOT
            //        attempt any additional async job prunes/removals of branches/trees.
            g_Last_Port_Rcv_Data_Delta = -1;
            g_Last_Port_Xmit_Data_Delta = -1;
            LOG(bb,error) << "IOSTAT: std::exception while executing.  e.what=" << e.what();
        }

        end = time(NULL);  // safety incase timeout call fails
        if(end-start < g_DiskStatsRate)
        {
            sleep(g_DiskStatsRate - (end-start));
        }
    }
    return NULL;
}

#define SLEEP 60
void* asyncRemoveJobInfo(void* ptr)
{
    uint64_t l_Time;
    BB_GetTime(l_Time);

    vector<string> l_PathJobIds;
    l_PathJobIds.reserve(100);

    while (1)
    {
        try
        {
            if (g_AsyncRemoveJobInfoInterval)
            {
                uint64_t l_NewTime = l_Time;
                BB_GetTimeDifference(l_NewTime);
                double l_ElapsedTime = (double)l_NewTime/(double)g_TimeBaseScale;
                if (l_ElapsedTime >= g_AsyncRemoveJobInfoInterval)
                {
                    int rc = HandleFile::get_xbbServerGetCurrentJobIds(l_PathJobIds, ONLY_RETURN_REMOVED_JOBIDS);
                    if ((!rc) && l_PathJobIds.size() > 0)
                    {
                        if (highIB_Activity() >= 0)
                        {
                            for (size_t i=0; i<l_PathJobIds.size(); i++)
                            {
                                bfs::path job = bfs::path(l_PathJobIds[i]);
                                bfs::path l_PathToRemove = job.parent_path().string() + "/." + job.filename().string();
                                LOG(bb,debug) << "asyncRemoveJobInfo(): " << job.string() << " being renamed to " << l_PathToRemove.string();
                                try
                                {
                                    bfs::rename(job, l_PathToRemove);
                                }
                                catch (std::exception& e1)
                                {
                                    rc = -1;
                                }

                                if (!rc)
                                {
                                    try
                                    {
                                        LOG(bb,info) << "asyncRemoveJobInfo(): START: Removal of cross-bbServer metadata at " << l_PathJobIds[i];
                                        for (auto& jobstep : boost::make_iterator_range(bfs::directory_iterator(l_PathToRemove), {}))
                                        {
                                            if (!pathIsDirectory(jobstep)) continue;
                                            for (auto& handlebucket : boost::make_iterator_range(bfs::directory_iterator(jobstep), {}))
                                            {
                                                if (!pathIsDirectory(handlebucket)) continue;
                                                for (auto& handledir : boost::make_iterator_range(bfs::directory_iterator(handlebucket), {}))
                                                {
                                                    if (!pathIsDirectory(handledir)) continue;
                                                    string l_HandleDir = handledir.path().string();
                                                    bool l_AllDone = false;
                                                    while (!l_AllDone)
                                                    {
                                                        l_AllDone = true;
                                                        if (!highIB_Activity())
                                                        {
                                                            try
                                                            {
                                                                bfs::remove_all(handledir);
                                                                LOG(bb,debug) << "asyncRemoveJobInfo():   END: Successful prune of cross-bbServer metadata at " << l_HandleDir;
                                                            }
                                                            catch (std::exception& e1)
                                                            {
                                                                LOG(bb,warning) << "asyncRemoveJobInfo():   END: Unsuccessful prune of cross-bbServer metadata at " << l_HandleDir << ". Processing will continue...";
                                                            }

                                                        }
                                                        else
                                                        {
                                                            l_AllDone = false;
                                                            LOG(bb,info) << "asyncRemoveJobInfo(): IB activity too high to do prune at " << l_HandleDir \
                                                                         << ". Current " << g_IB_Adapter << " port_rcv_data delta " << g_Last_Port_Rcv_Data_Delta \
                                                                         << ", current " << g_IB_Adapter << " port_xmit_data delta " << g_Last_Port_Xmit_Data_Delta \
                                                                         << ", current IB stats low activity clip value " << g_IBStatsLowActivityClipValue \
                                                                         << ". Delaying " << g_DiskStatsRate << " seconds...";
                                                            sleep(g_DiskStatsRate);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        bool l_AllDone = false;
                                        while (!l_AllDone)
                                        {
                                            l_AllDone = true;
                                            if (!highIB_Activity())
                                            {
                                                try
                                                {
                                                    bfs::remove_all(l_PathToRemove);
                                                    LOG(bb,info) << "asyncRemoveJobInfo():   END: Successful removal of cross-bbServer metadata at " << l_PathJobIds[i];
                                                }
                                                catch (std::exception& e2)
                                                {
                                                    LOG(bb,error) << "asyncRemoveJobInfo():   END: Unsuccessful removal of cross-bbServer metadata at " << l_PathJobIds[i] \
                                                                  << ". This cross-bbServer metadata must be manually removed.";
                                                }
                                            }
                                            else
                                            {
                                                l_AllDone = false;
                                                LOG(bb,info) << "asyncRemoveJobInfo(): IB activity too high to do final removal at " << l_PathJobIds[i] \
                                                             << ". Current " << g_IB_Adapter << " port_rcv_data delta " << g_Last_Port_Rcv_Data_Delta \
                                                             << ", current " << g_IB_Adapter << " port_xmit_data delta " << g_Last_Port_Xmit_Data_Delta \
                                                             << ", current IB stats low activity clip value " << g_IBStatsLowActivityClipValue \
                                                             << ". Delaying " << g_DiskStatsRate << " seconds...";
                                                sleep(g_DiskStatsRate);
                                            }
                                        }
                                    }
                                    catch (std::exception& e3)
                                    {
                                        LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e3);
                                        continue;
                                    }
                                }
                                rc = 0;
                            }
                        }
                        else
                        {
                            LOG(bb,info) << "asyncRemoveJobInfo(): No removal of cross-bbServer metadata will be attempted during this interval as the IB activity is undetermined";
                        }
                    }
                    BB_GetTime(l_Time);
                }
            }
        }
        catch (std::exception& e4)
        {
            LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e4);
        }

        sleep(SLEEP);
    }

    return NULL;
}
#undef SLEEP
#endif

