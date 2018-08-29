/*******************************************************************************
 |    bbproxy.cc
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


#include <grp.h>
#include <limits.h>
#include <mntent.h>
#include <pwd.h>
#include <semaphore.h>
#include <stdio.h>
#include <sys/resource.h>

#include <iostream>
#include <fstream>
#include <map>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <boost/system/error_code.hpp>
#include <linux/magic.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <uuid/uuid.h>

#ifdef PROF_TIMING
#include <ctime>
#include <chrono>
#endif

#include "connections.h"
#include "bbconndata.h"
#include "bbapi.h"
#include "bbapiAdmin.h"
#include "bbflags.h"
#include "bbinternal.h"
#include "bbproxy.h"
#include "bbproxy_flightlog.h"
#include "BBTransferDef.h"
#include "fh.h"
#include "identity.h"
#include "logging.h"
#include "LVExtent.h"
#include "LVLookup.h"
#include "LVUtils.h"
#include "Msg.h"
#include "usage.h"
#include "nodecontroller.h"

namespace bfs = boost::filesystem;
namespace bs = boost::system;

typedef struct fiemap fiemap_t;
typedef struct fiemap_extent fiemap_extent_t;

extern thread_local uid_t threadLocaluid;
extern thread_local gid_t threadLocalgid;


//*****************************************************************************
//  Static data members
//*****************************************************************************

LogicalVolumeNumber MasterLogicalVolumeNumber(0);

// NOTE:  This mutex is acquired around the processing performed by resizeLogicalVolume()
pthread_mutex_t ResizeMutex = PTHREAD_MUTEX_INITIALIZER;

// Map of devname to logical volume data that is maintained
// by create/remove logical volume
std::map<std::string, LV_Data> logicalVolumeData;
std::map<std::string, std::string> mount2dev;
pthread_mutex_t logicalVolumeDataMutex = PTHREAD_MUTEX_INITIALIZER;


//*****************************************************************************
//  Static methods
//*****************************************************************************
bool isLocalRemoteNotSameAddress(const std::string& pConnectionName);
void set_use_NVMF(int pValue);
int bbproxy_SayHello(const string& pConnectionName)
{
    // NOTE: ENTRY() and EXIT() are not used in this method...  Little debug value and
    //       bberror is 'cleared' as part of establishing the connection...

    bberror.clear(pConnectionName);

    int rc = 0;
    stringstream errorText;
    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    try
    {
        txp::Msg::buildMsg(txp::CORAL_HELLO, msg);
        //msg->addAttribute(txp::version, BBAPI_CLIENTVERSIONSTR, strlen(BBAPI_CLIENTVERSIONSTR)+1);

        time_t l_seconds = time(NULL);
        msg->addAttribute(txp::epochtimeinseconds, (uint64_t)l_seconds);
        LOG(bb,info) << "Epoch time in seconds: " << l_seconds << " GMT="<<  asctime(gmtime(&l_seconds));

        //! \TODO remove after kernel fix for nvmf target/initiator on same system
        if ( isLocalRemoteNotSameAddress(pConnectionName)  ){
            set_use_NVMF(1);
        }
        else{
            set_use_NVMF(0);
        }

        txp::CharArray knownSerials_attr;
        findSerials();
        vector<string> knownSerials = getDeviceSerials();
        for(const auto& serial : knownSerials)
        {

            LOG(bb,info) << "Adding device serial: " << serial;
            knownSerials_attr.push_back(make_pair(serial.length()+1, (char*)serial.c_str()));
        }

        msg->addAttribute(txp::knownSerials, &knownSerials_attr);

        // Send the message to bbserver
        rc = sendMessage(pConnectionName, msg, reply,MUSTADDUIDGID);
        delete msg;
        msg=NULL;
        if (rc)
        {
            errorText << "sendMessage to server failed";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Wait for the response
        rc = waitReply(reply, msg);
        if (rc)
        {
            errorText << "waitReply failure";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        if (!msg)
        {
            rc = -1;
            errorText << "waitReply failure - null message returned";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Process response data
        rc = bberror.merge(msg);

        l_seconds = time(NULL);
        time_t remote_epoch = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::epochtimeinseconds))->getData();
        const uint64_t l_MAXDIFFSECONDS=1;
        const uint64_t l_DIFFSECONDS= abs(l_seconds - remote_epoch);
        if ( l_DIFFSECONDS > l_MAXDIFFSECONDS )
        {
            errorText << "Detectable time difference between bbserver and bbproxy";
            bberror << err("error.timediff",l_DIFFSECONDS) << err("error.remote.gmt", asctime(gmtime(&remote_epoch)) ) <<err("error.remote.epoch", remote_epoch) << err("error.local.gmt", asctime(gmtime(&l_seconds)) ) <<err("error.local.epoch", l_seconds)<<err("error.threshold.seconds",l_MAXDIFFSECONDS)<<RAS(bb.cfgerr.timeskew);
        }

        uint64_t numnacks           = ((txp::AttrPtr_array_of_char_arrays*)msg->retrieveAttrs()->at(txp::nackSerials))->getNumberOfElementsArrayOfCharArrays();
        txp::CharArray* nackSerials = (txp::CharArray*)msg->retrieveAttrs()->at(txp::nackSerials)->getDataPtr();
        for(uint64_t x=0; x<numnacks; x++)
        {
            LOG(bb,info) << "Serial (" << (*nackSerials)[x].second << ") not available on bbServer.  Removing";
            removeDeviceSerial((*nackSerials)[x].second);
        }

        delete msg;
        msg=NULL;
    }
    catch(ExceptionBailout& e) {
        LOG(bb,always)<<"catch(ExceptionBailout& e)"<<" @="<<__FILE__<<":"<< __FUNCTION__<<":"<<__LINE__;
    }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }
    return rc;
}


//*****************************************************************************
//  Support routines
//*****************************************************************************

void addLogicalVolumeData(const char* pDevName, const char* pMountPoint, Uuid& pLVUuid, const uint64_t pJobId, const gid_t pGroupId, const uid_t pUserId)
{
    pthread_mutex_lock(&logicalVolumeDataMutex);
    logicalVolumeData[string(pDevName)] = LV_Data(pMountPoint, pLVUuid, pJobId, pGroupId, pUserId);
    mount2dev[pMountPoint] = pDevName;
    pthread_mutex_unlock(&logicalVolumeDataMutex);
    return;
}

std::string getDevName4Mount(const std::string& pMountPoint){
    std::string l_DevName;
    pthread_mutex_lock(&logicalVolumeDataMutex);
    auto it = mount2dev.find(pMountPoint);
    if (it != mount2dev.end()){
        l_DevName = it->second;
    }
    pthread_mutex_unlock(&logicalVolumeDataMutex);
    return l_DevName;
}

LV_Data getLogicalVolumeData(std::string& pDevName)
{
    LV_Data l_LV_Data = LV_Data();

    pthread_mutex_lock(&logicalVolumeDataMutex);
    auto it = logicalVolumeData.find(pDevName);
    if (it != logicalVolumeData.end())
    {
       l_LV_Data = it->second;
    }
    pthread_mutex_unlock(&logicalVolumeDataMutex);
    return l_LV_Data;
}


Uuid  getLogicalVolumeDataUuid(std::string& pDevName){
    Uuid        l_lvuuid;
    pthread_mutex_lock(&logicalVolumeDataMutex);
    auto it = logicalVolumeData.find(pDevName);
    if (it != logicalVolumeData.end())
    {
        l_lvuuid = it->second.lvuuid;
    }
    pthread_mutex_unlock(&logicalVolumeDataMutex);
    return l_lvuuid;
}

void removeLVdata(const std::string pMountPoint)
{
    pthread_mutex_lock(&logicalVolumeDataMutex);
    std::string l_DevName = mount2dev[pMountPoint];
    logicalVolumeData.erase(l_DevName);
    mount2dev.erase(pMountPoint);
    pthread_mutex_unlock(&logicalVolumeDataMutex);
    return;
}

void ResizeLogicalVolumeLock()
{
    ENTRY_NO_CLOCK(__FILE__,__FUNCTION__);

    pthread_mutex_lock(&ResizeMutex);

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);
    return;
}


void ResizeLogicalVolumeUnlock()
{
    ENTRY_NO_CLOCK(__FILE__,__FUNCTION__);

    pthread_mutex_unlock(&ResizeMutex);

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);
    return;
}


int32_t LogicalVolumeNumber::incr()
{
    ENTRY_NO_CLOCK(__FILE__,__FUNCTION__);

    int32_t l_Value;
    if (value != INT_MAX) {
        l_Value = ++value;
    } else {
        value = 1;
        l_Value = value;
    }

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);
    return l_Value;
}


void checkForSuperUserPermission()
{
    const uid_t l_Owner = threadLocaluid;
    const gid_t l_Group = threadLocalgid;
    if ( (!l_Owner) || (!l_Group) )
    {
    }
    //else \todo missing check to see if root is in secondary groups
    else
    {
        //https://linux.die.net/man/7/capabilities  "capabilities(7) - Linux man page"
        //check
        //CAP_NET_ADMIN
        //CAP_NET_BIND_SERVICE
        stringstream errorText;
        errorText << "The checkForSuperUserPermission failed uid=" << l_Owner << " gid=" << l_Group;
        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, -1);
    }

    return;
}

void switchIds()
{
    const uid_t l_Owner = threadLocaluid;
    const gid_t l_Group = threadLocalgid;

    int rc = becomeUser(l_Owner, l_Group);
    if (!rc)
    {
        bberror << err("in.misc.uid", l_Owner) << err("in.misc.gid", l_Group);
    }
    else
    {
        stringstream errorText;
        errorText << "becomeUser failed";
        bberror << err("error.uid", l_Owner) << err("error.gid", l_Group);
        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
    }

    return;
}

void updateEnv(const string& pConnectionName)
{
    bberror.clear(pConnectionName);
    bberror << err("env.jobid", getJobId(pConnectionName)) \
            << err("env.jobstepid", getJobStepId(pConnectionName)) \
            << err("env.contribid", getContribId(pConnectionName));

    return;
}


//*****************************************************************************
//  bbapi -> bbProxy requests
//*****************************************************************************

void msgin_createdirectory(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc;
    stringstream errorText;

    const char* newpathname;

    FL_Write(FLProxy, Msg_CreateDir, "CreateDir command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "BB_CreateDirectory");
    try
    {
        newpathname = (const char*)msg->retrieveAttrs()->at(txp::newpathname)->getDataPtr();
        bberror << err("in.parms.path", newpathname);


        // Switch to the uid/gid of requester.
        switchIds();

        LOG(bb,info) << "msgin_createdirectory: pathname=" << newpathname;

        // Today, we specify a mode of 0777, which is further modified by the
        // applicable umask value for the uid...(mode & ~umask)
        rc = mkdir(newpathname, 0777);
        if (rc)
        {
            errorText << "mkdir failed";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, errno);
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;
}

void msgin_removedirectory(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc;
    stringstream errorText;

    const char* pathname;

    FL_Write(FLProxy, Msg_RemoveDir, "RemoveDir command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "BB_RemoveDirectory");
    try
    {
        pathname = (const char*)msg->retrieveAttrs()->at(txp::pathname)->getDataPtr();
        bberror << err("in.parms.path", pathname);

        // Switch to the uid/gid of requester.
        switchIds();

        LOG(bb,info) << "msgin_removedirectory: pathname=" << pathname;

        bfs::path l_PathName(pathname);
        bs::error_code l_ErrorCode;
        bfs::remove_all(l_PathName, l_ErrorCode);

        if (l_ErrorCode.value())
        {
            rc = -1;
            errorText << "bfs::remove_all failed";
            bberror << err("error.value", l_ErrorCode.value()) << err("error.message", l_ErrorCode.message());
            if (EBUSY==l_ErrorCode.value()) {
                lsofRunCmd(pathname);
            }
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;
}

void msgin_changeowner(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc;

    const char*    pathname;
    const char*    newowner;
    const char*    newgroup;

    FL_Write(FLProxy, Msg_CHOWN, "ChangeOwner command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "BB_ChangeOwner");
    try
    {
        pathname = (const char*)msg->retrieveAttrs()->at(txp::pathname)->getDataPtr();
        newowner = (const char*)msg->retrieveAttrs()->at(txp::newowner)->getDataPtr();
        newgroup = (const char*)msg->retrieveAttrs()->at(txp::newgroup)->getDataPtr();
        bberror << err("in.parms.path", pathname) << err("in.parms.newowner", newowner) << err("in.parms.newgroup", newgroup);

        // Switch to the uid/gid of requester.
        switchIds();

        LOG(bb,info) << "msgin_changeowner: pathname=" << pathname << ", newowner=" << newowner << ", newgroup=" << newgroup;

        // NOTE:  If doChangeOwner() fails, it fills in errstate...
        rc = doChangeOwner(pathname, newowner, newgroup);
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;
}

void msgin_changemode(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc;
    stringstream errorText;

    const char* pathname;
    mode_t      newmode;

    FL_Write(FLProxy, Msg_CHMOD, "ChangeMode command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "BB_ChangeMode");
    try
    {
        pathname = (const char*)msg->retrieveAttrs()->at(txp::pathname)->getDataPtr();
        newmode  = (mode_t)((txp::Attr_int32*)msg->retrieveAttrs()->at(txp::newmode))->getData();
        char l_Temp[16] = {'\0'};
        snprintf(l_Temp, sizeof(l_Temp), "%o", newmode);
        bberror << err("in.parms.path", pathname) << err("in.parms.newmode", l_Temp);

        // Switch to the uid/gid of requester.
        switchIds();

        LOG(bb,info) << "msgin_changemode: pathname=" << pathname << ", newmode=" << newmode;

        rc = chmod(pathname, newmode);
        if (rc)
        {
            errorText << "chmod failed";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, errno);
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;
}

void msgin_resizemountpoint(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc;
    stringstream errorText;

    const char* mountpoint;
    char*       logicalvolume = 0;
    const char* mountsize;
    uint64_t    resizeflags;

    FL_Write(FLProxy, Msg_ResizeMountPoint, "ResizeMountpoint command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "BB_ResizeMountpoint");
    try
    {
        mountpoint = (const char*)msg->retrieveAttrs()->at(txp::mountpoint)->getDataPtr();
        mountsize  = (const char*)msg->retrieveAttrs()->at(txp::mountsize)->getDataPtr();
        resizeflags = (uint64_t)((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::flags))->getData();
        bberror << err("in.parms.mount", mountpoint) << err("in.parms.size", mountsize) << err("in.parms.flags", resizeflags);

        // Switch to the uid/gid of requester.
        switchIds();

        // Check permissions
        checkForSuperUserPermission();

        LOG(bb,info) << "msgin_resizemountpoint: mountpoint=" << mountpoint << ", mountsize=" << mountsize << ", resizeflags=" << resizeflags;

        ResizeLogicalVolumeLock();

        {
            rc = resizeLogicalVolume(mountpoint, logicalvolume, mountsize, resizeflags);
        }

        ResizeLogicalVolumeUnlock();

        if (rc)
        {
            // NOTE: resizeLogicalVolume() filled in errstate
            errorText << "Resize operation for logical volume associated with " << mountpoint << " failed";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }
        LOG(bb,info) << "Resize operation for logical volume associated with " << mountpoint << " completed.";
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;
}

void msgin_getusage(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc = EINVAL;

    const char* mountpoint;
    BBUsage_t   usage;

    FL_Write(FLProxy, Msg_GetUsage, "GetUsage command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "BB_GetUsage");
    try
    {
        mountpoint = (const char*)msg->retrieveAttrs()->at(txp::mountpoint)->getDataPtr();
        bberror << err("in.parms.mount", mountpoint);

        // Switch to the uid/gid of requester.
        switchIds();

        LOG(bb,info) << "msgin_getusage: mountpoint=" << mountpoint;

        rc = proxy_GetUsage(mountpoint, usage);
        if (rc) LOG_RC_AND_BAIL(rc);
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);

    addBBErrorToMsg(response);
#define ADDFIELD(name) response->addAttribute(txp::name, usage.name)
    ADDFIELD(totalBytesRead);
    ADDFIELD(totalBytesWritten);
    ADDFIELD(localBytesRead);
    ADDFIELD(localBytesWritten);
    ADDFIELD(burstBytesRead);
    ADDFIELD(burstBytesWritten);
#undef ADDFIELD
    LOG(bb,info) << "Get usage.name totalBytesRead="<<usage.totalBytesRead<<" totalBytesWritten="<<usage.totalBytesWritten<<" localBytesRead="<< usage.localBytesRead<<" localBytesWritten="<<usage.localBytesWritten<<" burstBytesRead="<<usage.burstBytesRead<<" burstBytesWritten="<<usage.burstBytesWritten;

    sendMessage(pConnectionName,response);
    delete response;
    response=NULL;

#ifdef PROF_TIMING
    std::chrono::high_resolution_clock::time_point time_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return;
}

void msgin_getdeviceusage(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc = EINVAL;

    uint32_t devicenum;
    BBDeviceUsage_t usage;

    FL_Write(FLProxy, Msg_GetDeviceUsage, "GetDeviceUsage command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "BB_GetDeviceUsage");
    try
    {
        devicenum = ((txp::Attr_uint32*)msg->retrieveAttrs()->at(txp::devicenum))->getData();
        bberror << err("in.parms.device", devicenum);

        // Switch to the uid/gid of requester.
        switchIds();

        LOG(bb,info) << "msgin_getdeviceusage: devicenum=" << devicenum;

        rc = proxy_GetDeviceUsage(devicenum, usage);
        if (rc) LOG_RC_AND_BAIL(rc);
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);

    addBBErrorToMsg(response);
#define usageattr(name) response->addAttribute(txp::name,      usage.name);
#define dusageattr(name) response->addAttribute(txp::name,   (uint64_t)usage.name);
    usageattr(critical_warning);
    dusageattr(temperature);
    dusageattr(available_spare);
    dusageattr(percentage_used);
    usageattr(data_read);
    usageattr(data_written);
    usageattr(num_read_commands);
    usageattr(num_write_commands);
    usageattr(busy_time);
    usageattr(power_cycles);
    usageattr(power_on_hours);
    usageattr(unsafe_shutdowns);
    usageattr(media_errors);
    usageattr(num_err_log_entries);
#undef usageattr

    sendMessage(pConnectionName,response);
    delete response;
    response=NULL;

#ifdef PROF_TIMING
    std::chrono::high_resolution_clock::time_point time_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return;
}

void msgin_setusagelimit(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc = EINVAL;

    BBUsage_t usage;
    const char* mountpoint;

    FL_Write(FLProxy, Msg_SetUsageLimit, "SetUsageLimit command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "BB_SetUsageLimit");
    try
    {
        mountpoint = (const char*)msg->retrieveAttrs()->at(txp::mountpoint)->getDataPtr();
        bberror << err("in.parms.mount", mountpoint);

        // Switch to the uid/gid of requester.
        switchIds();

        LOG(bb,info) << "msgin_setusagelimit: mountpoint=" << mountpoint;

#define ADDFIELD(name) usage.name = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::name))->getData();
        ADDFIELD(totalBytesRead);
        ADDFIELD(totalBytesWritten);
        ADDFIELD(localBytesRead);
        ADDFIELD(localBytesWritten);
        ADDFIELD(burstBytesRead);
        ADDFIELD(burstBytesWritten);
#undef ADDFIELD
        FL_Write6(FLProxy,SetUsageLimit, "usage.name totalBytesRead=%ld totalBytesWritten=%ld localBytesRead=%ld localBytesWritten=%ld burstBytesRead=%ld burstBytesWritten=%ld ",usage.totalBytesRead,usage.totalBytesWritten,usage.localBytesRead,usage.localBytesWritten,usage.burstBytesRead,usage.burstBytesWritten);
        LOG(bb,info) << "Set usage.name totalBytesRead="<<usage.totalBytesRead<<" totalBytesWritten="<<usage.totalBytesWritten<<" localBytesRead="<< usage.localBytesRead<<" localBytesWritten="<<usage.localBytesWritten<<" burstBytesRead="<<usage.burstBytesRead<<" burstBytesWritten="<<usage.burstBytesWritten;

        if(usage.totalBytesWritten || usage.totalBytesRead)
        {
            rc = startMonitoringMount(mountpoint, usage);
            if(rc) LOG_RC_AND_BAIL(rc);
        }
        else
        {
            rc = stopMonitoringMount(mountpoint);
            if(rc) LOG_RC_AND_BAIL(rc);
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;
}

void msgin_change_server(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc = 0;
    stringstream errorText;

    char* mountpoint = 0;
    char* l_MountPoint = 0;
    Uuid l_lvuuid;
    ResponseDescriptor reply;
    txp::Msg* msgserver = 0;
    char* l_VolumeGroupName = 0;

    FL_Write(FLProxy, Msg_ChgServer, "Coral_ChgServer command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "Coral_ChgServer");
    try
    {
        mountpoint = (char*)(msg->retrieveAttrs()->at(txp::mountpoint)->getDataPtr());
        bberror << err("in.parms.mount", mountpoint);

        // Switch to the uid/gid of requester.
        switchIds();

        // Check permissions
        checkForSuperUserPermission();

        LOG(bb,info) << "msgin_change_server: mountpoint=" << mountpoint;

        size_t l_VolumeGroupNameLen = config.get(process_whoami+".volumegroup", DEFAULT_VOLUME_GROUP_NAME).length();
        l_VolumeGroupName = new char[l_VolumeGroupNameLen+1];
        strncpy(l_VolumeGroupName, config.get(process_whoami+".volumegroup", DEFAULT_VOLUME_GROUP_NAME).c_str(), l_VolumeGroupNameLen);
        l_VolumeGroupName[l_VolumeGroupNameLen] = 0;

        l_MountPoint = realpath(mountpoint, NULL);

        if (l_MountPoint) {
            char l_DevName[PATH_MAX] = {'\0'};
            size_t l_DevNameLength = sizeof(l_DevName);
            char l_FileSysType[PATH_MAX] = {'\0'};
            size_t l_FileSysTypeLength = sizeof(l_FileSysType);
            rc = isMounted(l_MountPoint, l_DevName, l_DevNameLength, l_FileSysType, l_FileSysTypeLength);
            if (rc == 1) {
                rc = 0;
                if (strncmp(l_DevName, l_VolumeGroupName, l_VolumeGroupNameLen) == 0) {
                    rc = getLogicalVolumeUUID(l_MountPoint, l_lvuuid);
                    if (rc) {
                        rc = -1;
                        errorText << "Could not determine the uuid of the logical volume associated with " << l_MountPoint << ". The mount point may no longer have a mounted file system or the directory for the mount point may not exist.";
                        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                    }
                } else {
                    rc = -1;
                    errorText << "Mount point " << l_MountPoint << " is associated with device " << l_DevName << " which is not associated with volume group " << l_VolumeGroupName;
                    LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                }
            } else {
                if (rc == 0) {
                    rc = -1;
                    errorText << "Mount point " << l_MountPoint << " is not currently mounted.";
                    LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                } else {
                    errorText << "Error occurred when trying to determine if " << l_MountPoint << " is mounted.";
                    LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                }
            }
            free(l_MountPoint);
            l_MountPoint=NULL;
        }

        // Build the message to send to bbserver
        txp::Msg::buildMsg(txp::CORAL_CHANGESERVER, msgserver);
        char l_lvuuid_str[LENGTH_UUID_STR] = {'\0'};
        l_lvuuid.copyTo(l_lvuuid_str);
        bberror << err("out.lvuuid", l_lvuuid_str);
        // NOTE:  The char array is copied to heap by addAttribute and the storage for
        //        the logical volume uuid attribute is owned by the message facility.
        //        Our copy can then go out of scope...
        msgserver->addAttribute(txp::uuid, l_lvuuid_str, sizeof(l_lvuuid_str), txp::COPY_TO_HEAP);

        // Send the message to bbserver
        rc=sendMessage(DEFAULT_SERVER_ALIAS, msgserver, reply);
        delete msgserver;
        msgserver = NULL;
        if (rc)
        {
            errorText << "sendMessage to server failed";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Wait for the response
        rc = waitReply(reply, msgserver);
        if (rc)
        {
            errorText << "waitReply failure";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        if (!msgserver)
        {
            rc = -1;
            errorText << "waitReply failure - null message returned";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Process response data
        rc = bberror.merge(msgserver);
        delete msgserver;
        msgserver = NULL;

    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    if (l_VolumeGroupName) {
        delete[] l_VolumeGroupName;
    }
    if (l_MountPoint){
        free(l_MountPoint);
        l_MountPoint=NULL;
    }

    addReply(msg, response);

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;
}

void msgin_getvar(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc = 0;

    char* l_Variable = 0;
    int64_t l_Value = -1;

    FL_Write(FLProxy, Msg_GetVar, "Coral_GetVar command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "Coral_GetVar");
    try
    {
        l_Variable = (char*)msg->retrieveAttrs()->at(txp::variable)->getDataPtr();

        // Switch to the uid/gid of requester.
        switchIds();

        if ((strstr(l_Variable, "jobid")) || (strstr(l_Variable, "jobstepid")) || (strstr(l_Variable, "contribid")))
        {
	        if (!bbconnectionName.size())
	        {
	            stringstream errorText;
	            rc = ENOTCONN;
	            errorText << "NULL connection name";
                LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
            }
            if (strstr(l_Variable, "jobid"))
            {
                l_Value = getJobId(bbconnectionName);
            }
            else if (strstr(l_Variable, "jobstepid"))
            {
                l_Value = getJobStepId(bbconnectionName);
            }
            else
            {
                l_Value = getContribId(bbconnectionName);
            }
            LOG(bb,debug) << "GetVar: Variable: " << l_Variable << " => " << l_Value;
        }
        else
        {
            l_Value = readVar(l_Variable);
            LOG(bb,info) << "GetVar: Variable: " << l_Variable << " => " << l_Value;
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);

    addBBErrorToMsg(response);
    response->addAttribute(txp::value64, l_Value);

    sendMessage(pConnectionName,response);
    delete response;
    response=NULL;

#ifdef PROF_TIMING
    std::chrono::high_resolution_clock::time_point time_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return;
}

// NOTE:  Currently only supports writing ascii values to be read as positive integers
void msgin_setvar(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    bberror.clear(pConnectionName);

    int rc;

    const char* l_Variable;
    const char* l_Value;

    FL_Write(FLProxy, Msg_SetVar, "Coral_SetVar command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "Coral_SetVar");
    try
    {
        l_Variable = (const char*)msg->retrieveAttrs()->at(txp::variable)->getDataPtr();
        l_Value = (const char*)msg->retrieveAttrs()->at(txp::value)->getDataPtr();

        // Switch to the uid/gid of requester.
        switchIds();

        // Variables jobid, jobstepid, and contribid are stored in a connection name mapping.
        // Other variables are used for test purposes and stored in a file at the path specified
        // in the configuration file.
        if (strstr(l_Variable, "jobid"))
        {   uint64_t value = stoull(l_Value);
            rc = setJobId(pConnectionName, value);
            LOG(bb,debug) << "SetVar: Variable: " << l_Variable << " = " << l_Value;
        }
        else if (strstr(l_Variable, "jobstepid"))
        {
            uint64_t value = stoull(l_Value);
            rc = setJobStepId(pConnectionName, value);
            LOG(bb,debug) << "SetVar: Variable: " << l_Variable << " = " << l_Value;
        }
        else if (strstr(l_Variable, "contribid"))
        {
            uint32_t value = stoul(l_Value);
            rc = setContribId(pConnectionName, value);
            LOG(bb,debug) << "SetVar: Variable: " << l_Variable << " = " << l_Value;
        }
        else
        {
            writeVar(l_Variable, l_Value);

            int64_t l_NewValue = readVar(l_Variable);
            LOG(bb,info) << "SetVar: Variable: " << l_Variable << " => " << l_NewValue;
        }
    }

    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;
}



//*****************************************************************************
//  bbapi -> bbProxy - >bbServer requests
//*****************************************************************************

void msgin_canceltransfer(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc;
    stringstream errorText;

    uint64_t l_Handle = 0;
    uint64_t l_JobId = UNDEFINED_JOBID;
    uint64_t l_JobStepId = UNDEFINED_JOBSTEPID;
    uint32_t l_ContribId = UNDEFINED_CONTRIBID;

    txp::Msg* msgserver = 0;
    ResponseDescriptor reply;

    FL_Write(FLProxy, Msg_CancelTransfer, "CancelTransfer command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "BB_CancelTransfer");

    try
    {
        // Retrieve the data from the message
        l_Handle = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::handle))->getData();
        uint64_t l_CancelScope = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::flags))->getData();

        // Resolve the job, jobstep, and contribid identifiers
        if (bbconnectionName.size())
        {
            l_JobId = getJobId(bbconnectionName);
            l_JobStepId = getJobStepId(bbconnectionName);
            l_ContribId = getContribId(bbconnectionName);
        }
        else
        {
	        stringstream errorText;
	        rc = ENOTCONN;
	        errorText << "NULL connection name";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }
        // Insert input values into errstate...
        bberror << err("in.parms.handle", l_Handle) << err("in.parms.cancelscope", l_CancelScope);

        // Switch to the uid/gid of requester.
        switchIds();

        LOG(bb,info) << "Cancel transfer: jobid=" << l_JobId << ", jobstepid=" << l_JobStepId << ", handle=" << l_Handle << ", contribid=" << l_ContribId << ", cancelscope=" << l_CancelScope;

        // Build the message to send to bbserver
        txp::Msg::buildMsg(txp::BB_CANCELTRANSFER, msgserver);
        msgserver->addAttribute(txp::jobid, l_JobId);
        msgserver->addAttribute(txp::jobstepid, l_JobStepId);
        msgserver->addAttribute(txp::contribid, l_ContribId);
        msgserver->addAttribute(txp::handle, l_Handle);
        msgserver->addAttribute(txp::flags, l_CancelScope);

        // Send the message to bbserver
        rc=sendMessage(DEFAULT_SERVER_ALIAS, msgserver, reply);
        delete msgserver;
        msgserver = NULL;
        if (rc)
        {
            errorText << "sendMessage to server failed";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Wait for the response
        rc = waitReply(reply, msgserver);
        if (rc)
        {
            errorText << "waitReply failure";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        if (!msgserver)
        {
            rc = -1;
            errorText << "waitReply failure - null message returned";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Process response data
        rc = bberror.merge(msgserver);
        delete msgserver;
        msgserver = NULL;

    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;
}

void msgin_createlogicalvolume(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc=0;
    stringstream errorText;

    const char* mountpoint=NULL;
    const char* mountsize;
    uint64_t    jobid;
    uint64_t    createflags;
    Uuid        l_lvuuid;
    char        l_lvuuid_str[LENGTH_UUID_STR] = {'\0'};
    BBUsage_t   usage;

    bool onErrorRemoveNewLogicalVolume = false;

    txp::Msg* msgserver = 0;
    ResponseDescriptor reply;

    FL_Write(FLProxy, Msg_CreateLV, "CreateLogicalVolume command received  id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "BB_CreateLogicalVolume");

    try
    {
        // Retrieve the data from the message
        mountpoint = (const char*)msg->retrieveAttrs()->at(txp::mountpoint)->getDataPtr();
        mountsize  = (const char*)msg->retrieveAttrs()->at(txp::mountsize)->getDataPtr();
        createflags = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::flags))->getData();

        // Resolve the job, jobstep, and contribid identifiers
        if (bbconnectionName.size())
        {
            jobid = getJobId(bbconnectionName);
        }
        else
        {
            stringstream errorText;
            rc = ENOTCONN;
            errorText << "NULL connection name";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        // Insert input values into errstate...
        bberror << err("in.parms.mount", mountpoint) << err("in.parms.size", mountsize) << err("in.parms.flags", createflags);

        // Switch to the uid/gid of requester.
        switchIds();

        // Check permissions
        checkForSuperUserPermission();

        LOG(bb,info) << "msgin_createlogicalvolume: mountpoint=" << mountpoint << ", mountsize=" << mountsize << ", createflags=" << createflags;

        // Validate the size value
        ssize_t lvsize = 0;
        convertLVMSizeToBytes(mountsize, lvsize);
        if(lvsize < MINIMUM_LOGICAL_VOLUME_NUMBER_OF_SECTORS*SECTOR_SIZE)
        {
            rc = -1;
            errorText << "Requested size of " << mountsize << " is either invalid or too small. The minimum size that can be specified is " << MINIMUM_LOGICAL_VOLUME_NUMBER_OF_SECTORS*SECTOR_SIZE << " bytes. Suffixes of 'B', 'S', 'K', 'M', 'G', and 'T' are honored.";
            bberror << err("error.lvsize", lvsize);
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Retrieve the uid/gid of the mount point
        uid_t l_UserId;
        gid_t l_GroupId;
        rc = getUserAndGroupIds(mountpoint, l_UserId, l_GroupId);
        if (rc)
        {
            // NOTE: errstate already filled in...
            errorText << "Could not retrieve the userid and groupid for the mountpoint";
            bberror << err("error.jobid",jobid);
            LOG_ERROR_AND_BAIL(errorText);
        }

        if (getSuspendState(DEFAULT_SERVER_ALIAS) == SUSPENDED)
        {
            // A retry could be attempted in this suspended scenario.  Return -2.
            rc = -2;
            errorText << "Connection to the active server is suspended. Attempt to retry the create logical volume request when the connection is not suspended.";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        LOG(bb,info) << "Create: " << mountpoint << ", " << mountsize << ", " << createflags << ", " << l_UserId << ":" << l_GroupId << ", jobid=" << jobid;

        rc = createLogicalVolume(l_UserId, l_GroupId, mountpoint, mountsize, createflags);
        if (rc)
        {
            // NOTE: createLogicalVolume() filled in errstate
            BAIL;
        }

        // Any error from this point forward must remove the newly created logical volume...
        onErrorRemoveNewLogicalVolume = true;

        // Retrieve the logical volume UUID so that it can be registered with bbserver...
        rc = getLogicalVolumeUUID(mountpoint, l_lvuuid);
        if (rc)
        {
            rc = -1;
            errorText << "Retrieving the uuid for the newly created logical volume failed, rc=" << rc << ". The mount point may no longer have a mounted file system or the directory for the mount point may not exist.";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Build the message to send to bbserver
        txp::Msg::buildMsg(txp::BB_CREATELOGICALVOLUME, msgserver);
        l_lvuuid.copyTo(l_lvuuid_str);
        string l_HostName;
        activecontroller->gethostname(l_HostName);
        bberror << err("out.lvuuid", l_lvuuid_str) << err("in.misc.hostname", l_HostName);
        LOG(bb,info) << "Created LV Uuid = " << l_lvuuid_str << ", mountpoint = " << mountpoint;
        // NOTE:  The char arrays are copied to heap by addAttribute and the storage for
        //        the logical volume uuid and hostname attributes are owned by the
        //        message facility.  Our copies can then go out of scope...
        msgserver->addAttribute(txp::uuid, l_lvuuid_str, sizeof(l_lvuuid_str), txp::COPY_TO_HEAP);
        msgserver->addAttribute(txp::hostname, l_HostName.c_str(), l_HostName.size()+1, txp::COPY_TO_HEAP);
        msgserver->addAttribute(txp::jobid, jobid);
        msgserver->addAttribute(txp::mntptuid, l_UserId);
        msgserver->addAttribute(txp::mntptgid, l_GroupId);
        msgserver->addAttribute(txp::option, (uint32_t)0);

        // Send the message to bbserver
        rc=sendMessage(DEFAULT_SERVER_ALIAS, msgserver, reply);
        delete msgserver;
        msgserver = NULL;
        if (rc)
        {
            errorText << "sendMessage to server failed";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Wait for the response
        rc = waitReply(reply, msgserver);
        if (rc)
        {
            errorText << "waitReply failure";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        if (!msgserver)
        {
            rc = -1;
            errorText << "waitReply failure - null message returned";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Process response data
        bberror << err("out.lvuuid", l_lvuuid_str);

        rc = bberror.merge(msgserver);

        delete msgserver;
        msgserver=NULL;

        if (!rc){
            // Maintain the logicaVolumeData map
            char l_DevName[1024] = {'\0'};
            getLogicalVolumeDevName(l_lvuuid, l_DevName, sizeof(l_DevName));
            addLogicalVolumeData(l_DevName, mountpoint, l_lvuuid, jobid, l_GroupId, l_UserId);
        }

        // Notify control system
        if (!rc) activecontroller->lvcreate(l_lvuuid_str, LVStateMounted, lvsize, mountpoint, "xfs");

        LOG(bb,info) << "Create operation for logical volume completed and mounted at " << mountpoint << ".";

        // Setup usage for this mount point
        errno = 0;
        if (!rc)
        {
            rc = proxy_regLV4Usage(mountpoint);
            if (rc)
            {
                rc = -1;
                errorText << "usage registration failed for mountpoint "<< mountpoint;
                LOG_ERROR_TEXT_ERRNO(errorText, errno);
                FL_Write(FLBBUsage, USAGEREGFAIL, "usage registration failed for mountpoint errno=%ld create_flags=%lx user=%ld group=%ld",errno,createflags,l_UserId,l_GroupId);
                LOG_RC_AND_BAIL(rc);
            }
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    if (rc && onErrorRemoveNewLogicalVolume)
    {
        LOG(bb,info) << "Attempting backout via removeLogicalVolume of mountpoint = " << mountpoint;

        // De-register the LV Usage...
        int rc2 = proxy_deregLV4Usage(mountpoint);
        if (rc2)
        {
            errorText << "Backout of LV usage registration via dereg failed for mountpoint " << mountpoint;
            LOG_ERROR_TEXT(errorText);
        }

        // Notify the active controller
        activecontroller->lvremove(l_lvuuid_str, usage);

        // Maintain the logicaVolumeData map
        removeLVdata(mountpoint);

        // Remove the logical volume
        int rc3 = removeLogicalVolume(mountpoint, l_lvuuid, DO_NOT_FILL_IN_ERRSTATE);
        if (rc3)
        {
            errorText << "Backout removal of the logical volume associated with " << mountpoint << " failed";
            LOG_ERROR_TEXT(errorText);
        }
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;
}

void msgin_gettransferhandle(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc = 0;
    stringstream errorText;

    uint64_t l_Handle = 0;
    uint64_t l_JobId = UNDEFINED_JOBID;
    uint64_t l_JobStepId = UNDEFINED_JOBSTEPID;
    uint64_t l_Tag = 0;
    uint64_t l_NumContrib = 0;
    uint32_t* l_Contrib = 0;
    uint32_t l_ContribId = UNDEFINED_CONTRIBID;

    txp::Msg* msgserver = 0;
    ResponseDescriptor reply;

    FL_Write(FLProxy, Msg_GetTransferHandle, "GetTransferHandle command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());

    bberror << err("in.apicall", "BB_GetTransferHandle");

    try
    {
        // Retrieve the data from the message
        l_Tag = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::tag))->getData();
        l_NumContrib = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::numcontrib))->getData();
        if (l_NumContrib) {
            l_Contrib = (uint32_t*)msg->retrieveAttrs()->at(txp::contrib)->getDataPtr();
        }

        // Switch to the uid/gid of requester.
        switchIds();

        // Resolve the job and jobstep identifiers
        if (bbconnectionName.size())
        {
            l_JobId = getJobId(bbconnectionName);
            l_JobStepId = getJobStepId(bbconnectionName);
        }
        else
        {
            rc = ENOTCONN;
	        errorText << "NULL connection name";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        // If bbapi specified 1 contributor but no contrib list -or-
        //    bbcmd specified 0 contributors -then-
        // Build a contrib list consisting of the contribid.
        bool l_UseDefaultContrib = false;
        if (!l_Contrib) {
            l_UseDefaultContrib = true;
            l_NumContrib = 1;
            l_ContribId = getContribId(bbconnectionName);
            l_Contrib = &l_ContribId;
        }

        if (l_JobStepId == 0) {
            rc = -1;
            errorText << "A transfer handle cannot be created with a job step of 0, rc=" << rc << ".";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Insert input values into errstate...
        stringstream l_ContribStr;
        uint32_t* l_ContribPtr = l_Contrib;
        l_ContribStr << "(";
        for(uint64_t i=0; i<l_NumContrib; ++i) {
            LOOP_COUNT(__FILE__,__FUNCTION__,"contributors");
            if (i!=l_NumContrib-1) {
                l_ContribStr << *l_ContribPtr << ",";
            } else {
                l_ContribStr << *l_ContribPtr;
            }
            ++l_ContribPtr;
        }
        l_ContribStr << ")";
        bberror << err("in.parms.tag", l_Tag) << err("in.parms.numcontrib", l_NumContrib);
        if (l_UseDefaultContrib) {
            bberror << err("dft.parms.contrib", l_ContribStr.str());
        } else {
            bberror << err("in.parms.contrib", l_ContribStr.str());
        }

        LOG(bb,info) << "msgin_gettransferhandle: jobid=" << l_JobId << ", jobstepid=" << l_JobStepId << ", tag=" << l_Tag << ", numcontrib=" << l_NumContrib << ", contrib=" << l_ContribStr;

        // NOTE: If processContrib() fails, it fills in errstate...
        rc = processContrib(l_NumContrib, l_Contrib);
        if (rc) BAIL;

        // Build the message to send to bbserver
        txp::Msg::buildMsg(txp::BB_GETTRANSFERHANDLE, msgserver);
        string l_HostName;
        activecontroller->gethostname(l_HostName);
        msgserver->addAttribute(txp::hostname, l_HostName.c_str(), l_HostName.size()+1, txp::COPY_TO_HEAP);
        msgserver->addAttribute(txp::jobid, l_JobId);
        msgserver->addAttribute(txp::jobstepid, l_JobStepId);
        msgserver->addAttribute(txp::tag, l_Tag);
        msgserver->addAttribute(txp::numcontrib, l_NumContrib);
        msgserver->addAttribute(txp::contrib, (const char*)l_Contrib, sizeof(uint32_t) * l_NumContrib);

        // Send the message to bbserver
        rc=sendMessage(DEFAULT_SERVER_ALIAS, msgserver, reply);
        delete msgserver;
        msgserver=NULL;
        if (rc)
        {
            errorText << "sendMessage to server failed";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Wait for the response
        rc = waitReply(reply, msgserver);
        if (rc)
        {
            errorText << "waitReply failure";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        if (!msgserver)
        {
            rc = -1;
            errorText << "waitReply failure - null message returned";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Process response data
        rc = bberror.merge(msgserver);

        if (!rc)
        {
            Uuid l_lvuuid = Uuid((char*)(msgserver->retrieveAttrs()->at(txp::uuid)->getDataPtr()));
            l_Handle = ((txp::Attr_uint64*)msgserver->retrieveAttrs()->at(txp::transferHandle))->getData();
            char l_lvuuid_str[LENGTH_UUID_STR] = {'\0'};
            l_lvuuid.copyTo(l_lvuuid_str);
            LOG(bb,info) << "msgin_gettransferhandle: LVUuid " << l_lvuuid_str << ", job(" << l_JobId << "," << l_JobStepId << "), tag " << l_Tag << ", numcontrib " << l_NumContrib << ", contrib " << l_ContribStr.str() << ", returning handle = " << l_Handle << " jobid="<< l_JobId<<" jobstepid="<<l_JobStepId<<", rc = " << rc;
            bberror << err("out.lvuuid", l_lvuuid_str) << err("out.handle", l_Handle);
        }

        delete msgserver;
        msgserver=NULL;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    // Build/send the response message
    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);

    addBBErrorToMsg(response);
    response->addAttribute(txp::transferHandle, l_Handle);

    sendMessage(pConnectionName,response);
    delete response;
    response=NULL;

#ifdef PROF_TIMING
    std::chrono::high_resolution_clock::time_point time_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return;
}

void msgin_gettransferinfo(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc = 0;
    stringstream errorText;

    uint64_t l_JobId = UNDEFINED_JOBID;
    uint64_t l_JobStepId = UNDEFINED_JOBSTEPID;
    uint64_t l_Handle = 0;
    uint64_t l_Tag = 0;
    uint64_t l_NumContrib = 0;
    uint32_t l_ContribId = UNDEFINED_CONTRIBID;
    uint32_t* l_Contrib = 0;
    BBSTATUS l_Status = BBNONE;
    uint64_t l_NumReportingContribs = 0;
    uint64_t l_TotalTransferKeyLength = 0;
    uint64_t l_TotalTransferSize = 0;
    BBSTATUS l_LocalStatus = BBNONE;
    uint64_t l_LocalTransferSize = 0;

    txp::Msg* msgserver = 0;
    ResponseDescriptor reply;

    FL_Write(FLProxy, Msg_GetTransferInfo, "GetTransferInfo command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "BB_GetTransferInfo");

    try
    {
        // Retrieve the data from the message
        l_Handle = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::handle))->getData();
        bberror << err("in.parms.handle", l_Handle);

        // Switch to the uid/gid of requester.
        switchIds();

        // Resolve the contribid value
        l_ContribId = getContribId(bbconnectionName);

        LOG(bb,debug) << "msgin_gettransferinfo: handle=" << l_Handle << ", contribid=" << l_ContribId;

        // Build the message to send to bbserver
        txp::Msg::buildMsg(txp::BB_GETTRANSFERINFO, msgserver);
        msgserver->addAttribute(txp::handle, l_Handle);
        msgserver->addAttribute(txp::contribid, l_ContribId);

        // Send the message to bbserver
        rc=sendMessage(DEFAULT_SERVER_ALIAS, msgserver, reply);
        delete msgserver;
        msgserver = NULL;
        if (rc)
        {
            errorText << "sendMessage to server failed";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Wait for the response
        rc = waitReply(reply, msgserver);
        if (rc)
        {
            errorText << "waitReply failure";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        if (!msgserver)
        {
            rc = -1;
            errorText << "waitReply failure - null message returned";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        rc = bberror.merge(msgserver);

        // Process response data
        // NOTE: If there is a failure indicated by a non-zero return code, the only fields that are returned
        //       are handle, contribid, status and local status.  In that case, the status values will be BBNONE.
        if (!rc)
        {
            l_JobId = ((txp::Attr_uint64*)msgserver->retrieveAttrs()->at(txp::jobid))->getData();
            l_JobStepId = ((txp::Attr_uint64*)msgserver->retrieveAttrs()->at(txp::jobstepid))->getData();
            l_Tag = ((txp::Attr_uint64*)msgserver->retrieveAttrs()->at(txp::tag))->getData();
            l_NumContrib = ((txp::Attr_uint64*)msgserver->retrieveAttrs()->at(txp::numcontrib))->getData();
            l_Contrib = (uint32_t*)msgserver->retrieveAttrs()->at(txp::contrib)->getDataPtr();
            l_NumReportingContribs = ((txp::Attr_uint64*)msgserver->retrieveAttrs()->at(txp::numreportingcontribs))->getData();
            l_TotalTransferKeyLength = ((txp::Attr_uint64*)msgserver->retrieveAttrs()->at(txp::totalTransferKeyLength))->getData();
            l_TotalTransferSize = ((txp::Attr_uint64*)msgserver->retrieveAttrs()->at(txp::totalTransferSize))->getData();
            l_LocalTransferSize = ((txp::Attr_uint64*)msgserver->retrieveAttrs()->at(txp::localTransferSize))->getData();
        }

        l_Status = (BBSTATUS)((txp::Attr_uint64*)msgserver->retrieveAttrs()->at(txp::status))->getData();
        l_LocalStatus = (BBSTATUS)((txp::Attr_uint64*)msgserver->retrieveAttrs()->at(txp::localstatus))->getData();

        delete msgserver;
        msgserver = NULL;

        char l_LocalStatusStr[64] = {'\0'};
        char l_StatusStr[64] = {'\0'};
        getStrFromBBStatus(l_LocalStatus, l_LocalStatusStr, sizeof(l_LocalStatusStr));
        getStrFromBBStatus(l_Status, l_StatusStr, sizeof(l_StatusStr));
        LOG(bb,info) << "msgin_gettransferinfo: handle " << l_Handle << ", contribid " << l_ContribId << " returning local status = " << l_LocalStatusStr << ", overall status = " << l_StatusStr << ", rc = " << rc;
        bberror << err("out.localstatus", l_LocalStatusStr) << err("out.status", l_StatusStr) \
                << err("out.localTransferSize", l_LocalTransferSize) << err("out.totalTransferSize", l_TotalTransferSize);
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    // Build/send the response message
    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);

    addBBErrorToMsg(response);
    if (!rc)
    {
        response->addAttribute(txp::jobid, l_JobId);
        response->addAttribute(txp::jobstepid, l_JobStepId);
        response->addAttribute(txp::contribid, l_ContribId);
        response->addAttribute(txp::tag, l_Tag);
        response->addAttribute(txp::numcontrib, l_NumContrib);
        response->addAttribute(txp::contrib, (const char*)l_Contrib, sizeof(uint32_t) * l_NumContrib);
        response->addAttribute(txp::numreportingcontribs, l_NumReportingContribs);
        response->addAttribute(txp::totalTransferKeyLength, l_TotalTransferKeyLength);
        response->addAttribute(txp::totalTransferSize, l_TotalTransferSize);
        response->addAttribute(txp::localTransferSize, l_LocalTransferSize);
    }

    response->addAttribute(txp::status, (uint64_t)l_Status);
    response->addAttribute(txp::localstatus, (uint64_t)l_LocalStatus);

    sendMessage(pConnectionName,response);
    delete response;
    response=NULL;

#ifdef PROF_TIMING
    std::chrono::high_resolution_clock::time_point time_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return;
}

void msgin_gettransferkeys(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc = 0;
    stringstream errorText;

    uint64_t l_Handle = 0;
    uint64_t l_JobId = UNDEFINED_JOBID;
    uint32_t l_ContribId = UNDEFINED_CONTRIBID;
    uint64_t l_BufferSize = 0;
    char*    l_Buffer = 0;

    txp::Msg* msgserver = 0;
    ResponseDescriptor reply;

    FL_Write(FLProxy, Msg_GetTransferKeys, "GetTransferKeys command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "BB_GetTransferKeys");

    try
    {
        // Retrieve the data from the message
        l_Handle = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::handle))->getData();
        bberror << err("in.parms.handle", l_Handle);

        // Switch to the uid/gid of requester.
        switchIds();

        // Resolve the job and contribid identifiers
        if (bbconnectionName.size())
        {
            l_JobId = getJobId(bbconnectionName);
            l_ContribId = getContribId(bbconnectionName);
        }
        else
        {
            rc = ENOTCONN;
	        errorText << "NULL connection name";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        LOG(bb,info) << "msgin_gettransferkeys: jobid=" << l_JobId << ", handle=" << l_Handle << ", contribid=" << l_ContribId;

        // Build the message to send to bbserver
        txp::Msg::buildMsg(txp::BB_GETTRANSFERKEYS, msgserver);
        msgserver->addAttribute(txp::jobid, l_JobId);
        msgserver->addAttribute(txp::contribid, l_ContribId);
        msgserver->addAttribute(txp::handle, l_Handle);

        // Send the message to bbserver
        rc=sendMessage(DEFAULT_SERVER_ALIAS, msgserver, reply);
        delete msgserver;
        msgserver = NULL;
        if (rc)
        {
            errorText << "sendMessage to server failed";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Wait for the response
        rc = waitReply(reply, msgserver);
        if (rc)
        {
            errorText << "waitReply failure";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        if (!msgserver)
        {
            rc = -1;
            errorText << "waitReply failure - null message returned";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        rc = bberror.merge(msgserver);

        if(rc == 0)
        {
            // Process response data
            l_BufferSize = ((txp::Attr_uint64*)msgserver->retrieveAttrs()->at(txp::buffersize))->getData();
            l_Buffer = (char*)(msgserver->retrieveAttrs()->at(txp::buffer)->getDataPtr());
        }
        delete msgserver;
        msgserver = NULL;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    // Build/send the response message
    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);

    addBBErrorToMsg(response);
    if (!rc)
    {
        response->addAttribute(txp::buffersize, l_BufferSize);
        response->addAttribute(txp::buffer, l_Buffer, l_BufferSize);
    }

    sendMessage(pConnectionName,response);
    delete response;
    response=NULL;

#ifdef PROF_TIMING
    std::chrono::high_resolution_clock::time_point time_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return;
}

void msgin_getthrottlerate(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc = 0;
    stringstream errorText;

    const char* mountpoint;
    char* l_MountPoint = 0;
    Uuid l_lvuuid;

    uint64_t l_Rate;

    txp::Msg* msgserver = 0;
    ResponseDescriptor reply;

    FL_Write(FLProxy, Msg_GetThrottleRate, "GetThrottleRate command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "BB_GetThrottleRate");

    try
    {
        // Retrieve the data from the message
        mountpoint = (const char*)msg->retrieveAttrs()->at(txp::mountpoint)->getDataPtr();

        // Insert input values into errstate...
        bberror << err("in.parms.mount", mountpoint);

        // Switch to the uid/gid of requester.
        switchIds();

        LOG(bb,info) << "msgin_getthrottlerate: mountpoint=" << mountpoint;

        l_MountPoint = realpath(mountpoint, NULL);

        if (l_MountPoint) {
            rc = getLogicalVolumeUUID(l_MountPoint, l_lvuuid);
            if (rc) {
                rc = -1;
                errorText << "Retrieving the uuid for the logical volume to get the throttle rate for failed, rc=" << rc << ". The mount point may not exist.";
                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
            }
        } else {
            rc = -1;
            errorText << "Could not determine an absolute path for " << mountpoint << ".  The mountpoint directory may not exist.";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Build the message to send to bbserver
        txp::Msg::buildMsg(txp::BB_GETTHROTTLERATE, msgserver);
        char l_lvuuid_str[LENGTH_UUID_STR] = {'\0'};
        l_lvuuid.copyTo(l_lvuuid_str);
        // NOTE:  The char array is copied to heap by addAttribute and the storage for
        //        the logical volume uuid attribute is owned by the message facility.
        //        Our copy can then go out of scope...
        msgserver->addAttribute(txp::uuid, l_lvuuid_str, sizeof(l_lvuuid_str), txp::COPY_TO_HEAP);

        // Send the message to bbserver
        rc=sendMessage(DEFAULT_SERVER_ALIAS, msgserver, reply);
        delete msgserver;
        msgserver=NULL;
        if (rc)
        {
            errorText << "sendMessage to server failed";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Wait for the response
        rc = waitReply(reply, msgserver);
        if (rc)
        {
            errorText << "waitReply failure";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        if (!msgserver)
        {
            rc = -1;
            errorText << "waitReply failure - null message returned";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Process response data
        rc = bberror.merge(msgserver);
        if (!rc)
        {
            l_Rate = ((txp::Attr_uint64*)msgserver->retrieveAttrs()->at(txp::rate))->getData();
            bberror << err("out.rate", l_Rate);
        }
        delete msgserver;
        msgserver=NULL;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    if (l_MountPoint){
        free(l_MountPoint);
        l_MountPoint=NULL;
    }

    // Build/send the response message
    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);

    addBBErrorToMsg(response);
    if (!rc)
    {
        response->addAttribute(txp::rate, l_Rate);
    }

    sendMessage(pConnectionName,response);
    delete response;
    response=NULL;

#ifdef PROF_TIMING
    std::chrono::high_resolution_clock::time_point time_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return;
}

void msgin_gettransferlist(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc = 0;
    stringstream errorText;

    uint64_t l_JobId = UNDEFINED_JOBID;
    uint64_t l_JobStepId = UNDEFINED_JOBSTEPID;
    uint64_t l_NumHandles = 0;
    BBSTATUS l_MatchStatus = BBNONE;
    uint64_t l_NumAvailHandles = 0;
    uint64_t* l_Handles = 0;

    txp::Msg* msgserver = 0;
    ResponseDescriptor reply;

    FL_Write(FLProxy, Msg_GetTransferList, "GetTransferList command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "BB_GetTransferList");

    try
    {
        // Retrieve the data from the message
        l_NumHandles = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::numhandles))->getData();
        l_MatchStatus = (BBSTATUS)((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::matchstatus))->getData();

        // Insert input values into errstate...
        errorText << std::hex << std::uppercase << setfill('0') \
                  << "0x" << (uint64_t)l_MatchStatus \
                  << setfill(' ') << std::nouppercase << std::dec;
        bberror << err("in.parms.numhandles", l_NumHandles) << err("in.parms.matchstatus", errorText.str());

        // Switch to the uid/gid of requester.
        switchIds();

        // Resolve the job and jobstep identifiers
        if (bbconnectionName.size())
        {
            l_JobId = getJobId(bbconnectionName);
            l_JobStepId = getJobStepId(bbconnectionName);
        }
        else
        {
            rc = ENOTCONN;
	        errorText << "NULL connection name";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        LOG(bb,info) << "msgin_gettransferlist: jobid=" << l_JobId << ", jobstepid=" << l_JobStepId << ", numhandles=" << l_NumHandles << ", matchstatus=" << l_MatchStatus;

        // Build the message to send to bbserver
        txp::Msg::buildMsg(txp::BB_GETTRANSFERLIST, msgserver);
        msgserver->addAttribute(txp::jobid, l_JobId);
        msgserver->addAttribute(txp::jobstepid, l_JobStepId);
        msgserver->addAttribute(txp::numhandles, l_NumHandles);
        msgserver->addAttribute(txp::matchstatus, (uint64_t)l_MatchStatus);

#ifndef __clang_analyzer__  // keep for debug
        l_NumHandles=0;
        l_NumAvailHandles=0;
#endif

        // Send the message to bbserver
        rc=sendMessage(DEFAULT_SERVER_ALIAS, msgserver, reply);
        delete msgserver;
        msgserver=NULL;
        if (rc)
        {
            errorText << "sendMessage to server failed";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Wait for the response
        rc = waitReply(reply, msgserver);
        if (rc)
        {
            errorText << "waitReply failure";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        if (!msgserver)
        {
            rc = -1;
            errorText << "waitReply failure - null message returned";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Process response data
        rc = bberror.merge(msgserver);
        if (rc) {
            l_NumHandles=0;
            l_NumAvailHandles=0;
        }
        else {
            l_NumHandles = ((txp::Attr_uint64*)msgserver->retrieveAttrs()->at(txp::numhandles))->getData();
            l_NumAvailHandles = ((txp::Attr_uint64*)msgserver->retrieveAttrs()->at(txp::numavailhandles))->getData();
            l_Handles = (uint64_t*)msgserver->retrieveAttrs()->at(txp::handles)->getDataPtr();
        }

        bberror << err("out.numhandles", l_NumHandles) << err("out.numavailhandles", l_NumAvailHandles);


        delete msgserver;
        msgserver=NULL;

    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    // Build/send the response message
    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);

    LOG(bb,info) << "msgin_gettransferlist handles: jobid=" << l_JobId << ", jobstepid=" << l_JobStepId << ", numhandles=" << l_NumHandles << ", matchstatus=" << l_MatchStatus;
    uint64_t* l_Temp = l_Handles;
    for(size_t i=0; i<l_NumHandles; ++i) {
        string l_HandleStr = "out.handle_" + to_string(i);
        bberror << err(l_HandleStr.c_str(), *l_Temp);
        LOG(bb,info) << "msgin_gettransferlist: i=" << i << ", handle=" << *l_Temp;
        ++l_Temp;
    }

    addBBErrorToMsg(response);
    if (!rc){
        response->addAttribute(txp::numhandles, l_NumHandles);
        response->addAttribute(txp::numavailhandles, l_NumAvailHandles);
        response->addAttribute(txp::handles, (const char*)l_Handles, sizeof(uint64_t) * l_NumHandles);
    }


    sendMessage(pConnectionName,response);
    delete response;
    response=NULL;

#ifdef PROF_TIMING
    std::chrono::high_resolution_clock::time_point time_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return;
}

void msgin_removejobinfo(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc = 0;
    stringstream errorText;

    uint64_t l_JobId = UNDEFINED_JOBID;

    ResponseDescriptor reply;
    txp::Msg* msgserver = 0;

    FL_Write(FLProxy, Msg_RemoveJobInfo, "RemoveJobInfo command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "BB_RemoveJobInfo");

    try
    {
        // Switch to the uid/gid of requester.
        switchIds();

        // Check permissions
        checkForSuperUserPermission();

        // Resolve the jobid
        if (bbconnectionName.size())
        {
            l_JobId = getJobId(bbconnectionName);
        }
        else
        {
            rc = ENOTCONN;
            errorText << "NULL connection name";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        LOG(bb,info) << "msgin_removejobinfo: jobid=" << l_JobId;

        // Build the message to send to bbserver
        txp::Msg::buildMsg(txp::BB_REMOVEJOBINFO, msgserver);
        msgserver->addAttribute(txp::jobid, l_JobId);

        // Send the message to bbserver
        rc=sendMessage(DEFAULT_SERVER_ALIAS, msgserver, reply);
        delete msgserver;
        msgserver=NULL;
        if (rc)
        {
            errorText << "sendMessage to server failed";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Wait for the response
        rc = waitReply(reply, msgserver);
        if (rc)
        {
            errorText << "waitReply failure";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        if (!msgserver)
        {
            rc = -1;
            errorText << "waitReply failure - null message returned";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Process response data
        rc = bberror.merge(msgserver);
        delete(msgserver);
        msgserver=NULL;

    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;
}

void msgin_removelogicalvolume(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc;
    stringstream errorText;

    const char* mountpoint = NULL;
    char* l_MountPoint = 0;
    Uuid l_lvuuid;
    uint32_t contribid = UNDEFINED_CONTRIBID;

    BBUsage_t usage;

    txp::Msg* msgserver = 0;
    ResponseDescriptor reply;

    std::string theDevName;

    size_t l_VolumeGroupNameLen = config.get(process_whoami+".volumegroup", DEFAULT_VOLUME_GROUP_NAME).length();
    char* l_VolumeGroupName = new char[l_VolumeGroupNameLen+1];
    strncpy(l_VolumeGroupName, config.get(process_whoami+".volumegroup", DEFAULT_VOLUME_GROUP_NAME).c_str(), l_VolumeGroupNameLen);
    l_VolumeGroupName[l_VolumeGroupNameLen] = 0;

    FL_Write(FLProxy, Msg_RemovePart, "RemoveLogicalVolume command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "BB_RemoveLogicalVolume");
    try
    {
        mountpoint = (const char*)msg->retrieveAttrs()->at(txp::mountpoint)->getDataPtr();
        bberror << err("in.parms.mount", mountpoint);

        // Resolve the contribid
        if (bbconnectionName.size())
        {
            contribid = getContribId(bbconnectionName);
        }
        else
        {
            rc = ENOTCONN;
            errorText << "NULL connection name";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        // Switch to the uid/gid of requester.
        switchIds();

        // Check permissions
        checkForSuperUserPermission();

        LOG(bb,info) << "msgin_removelogicalvolume: mountpoint=" << mountpoint;

        l_MountPoint = realpath(mountpoint, NULL);
        int isMountedRc = -1;

        // Determine lvuuid
        if (l_MountPoint)
        {
            char l_FileSysType[LINE_MAX] = {'\0'};
            size_t l_FileSysTypeLength = sizeof(l_FileSysType);
            char l_DevName[LINE_MAX] = {'\0'};
            size_t l_DevNameLength = sizeof(l_DevName);
            isMountedRc = isMounted(l_MountPoint, l_DevName, l_DevNameLength, l_FileSysType, l_FileSysTypeLength);

            if (isMountedRc == 1)
            {
                rc = getLogicalVolumeUUID(l_MountPoint, l_lvuuid);
                theDevName = l_DevName;
                if (rc)
                {
                    l_lvuuid = getLogicalVolumeDataUuid(theDevName);
                }
            }
            else
            {
                theDevName = getDevName4Mount(l_MountPoint);
                l_lvuuid = getLogicalVolumeDataUuid(theDevName);
            }
        }
        else
        {
            rc = -2;
            errorText << "Could not determine an absolute path for " << mountpoint << ".  The mountpoint directory may not exist.";
            LOG(bb,info) << errorText.str();
            bberror << err("error.text", errorText.str()) << errloc(rc) << bailout;
        }


        if (!l_lvuuid.is_null())
        {
            // Build the message to send to bbserver
            txp::Msg::buildMsg(txp::BB_REMOVELOGICALVOLUME, msgserver);
            char l_lvuuid_str[LENGTH_UUID_STR] = {'\0'};
            l_lvuuid.copyTo(l_lvuuid_str);
            // NOTE:  The char array is copied to heap by addAttribute and the storage for
            //        the logical volume uuid attribute is owned by the message facility.
            //        Our copy can then go out of scope...
            msgserver->addAttribute(txp::uuid, l_lvuuid_str, sizeof(l_lvuuid_str), txp::COPY_TO_HEAP);
            msgserver->addAttribute(txp::contribid, contribid);

            // Send the message to bbserver
            rc=sendMessage(DEFAULT_SERVER_ALIAS, msgserver, reply);
            delete msgserver;
            msgserver=NULL;
            if (rc)
            {
                errorText << "sendMessage to server failed";
                LOG_ERROR_TEXT_RC(errorText, rc);
            }
            else {
                // Wait for the response
                rc = waitReply(reply, msgserver);
                if (rc)
                {
                    errorText << "waitReply failure";
                    LOG_ERROR_TEXT_RC(errorText, rc);
                }
                else if (!msgserver)
                {
                    rc = -1;
                    errorText << "waitReply failure - null message returned";
                    LOG_ERROR_TEXT_RC(errorText, rc);
                }
                else
                {
                    // Process response data
                    bberror << err("out.lvuuid", l_lvuuid_str);
                    rc = bberror.merge(msgserver);
                    LOG(bb,info) << "Removed logical volume associated with " << mountpoint;
                    delete msgserver;
                    msgserver=NULL;
                }
            }


            proxy_GetUsage(l_MountPoint, usage);

            if (!isMountedRc )
            {
                rc = doRemoveLogicalVolume(0, theDevName.c_str());
                if (rc)
                {
                    errorText << "Attempt to remove logical volume " << theDevName << " that was once associated with " << l_MountPoint \
                              << " was not successful.  The logical volume may no longer exist.";
                    LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                }
            }
            else
            {
                rc = removeLogicalVolume(l_MountPoint, l_lvuuid, FILL_IN_ERRSTATE);
                if (rc)
                {
                    // NOTE: removeLogicalVolume() filled in bberror
                    bberror<<err("err.isMountedRc",isMountedRc);
                    LOG_RC_AND_BAIL(rc);
                }
            }

            // Notify the node controller
            activecontroller->lvremove(l_lvuuid_str, usage);

            // Maintain the logicaVolumeData map
            removeLVdata(mountpoint);
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    if(mountpoint)
    {
        rc = proxy_deregLV4Usage(mountpoint);
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    if (l_MountPoint)
    {
        free(l_MountPoint);
    }

    if (l_VolumeGroupName)
    {
        delete[] l_VolumeGroupName;
    }

    addReply(msg, response);

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;
}

void msgin_restarttransfers(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc = 0;
    stringstream errorText;

    uint64_t jobid = UNDEFINED_JOBID;
    uint64_t jobstepid = UNDEFINED_JOBSTEPID;
    uint64_t handle = UNDEFINED_HANDLE;
    uint32_t contribid = UNDEFINED_CONTRIBID;
    string l_HostName;

    ResponseDescriptor reply;

    string l_TransferDefsStr;
    BBTransferDefs* l_TransferDefs = 0;

    uint32_t l_NumRestartedTransferDefs = 0;

    FL_Write(FLProxy, Msg_RestartTransfers, "RestartTransfers command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "BB_RestartTransfers");
    try
    {
        // Retrieve the data from the message
        string hostname = (const char*)msg->retrieveAttrs()->at(txp::hostname)->getDataPtr();
        handle = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::handle))->getData();
        // NOTE: archive string is already null terminated and the length accounts for the null terminator
        l_TransferDefsStr.assign((const char*)msg->retrieveAttrs()->at(txp::transferdefs)->getDataPtr(), (uint64_t)(msg->retrieveAttrs()->at(txp::transferdefs)->getDataLength()));

        // Resolve the jobid/jobstepid/contribid
        if (bbconnectionName.size())
        {
            jobid = getJobId(bbconnectionName);
            jobstepid = getJobStepId(bbconnectionName);
            contribid = getContribId(bbconnectionName);
        }
        else
        {
            rc = ENOTCONN;
            errorText << "NULL connection name";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        activecontroller->gethostname(l_HostName);

        string l_HostNamePrt1 = hostname;
        string l_HostNamePrt2 = l_HostNamePrt1;
        if (hostname == UNDEFINED_HOSTNAME)
        {
            // If not specified, the default hostname is this compute node...
            hostname = l_HostName;
            l_HostNamePrt2 = hostname;
        }
        else if (hostname == NO_HOSTNAME)
        {
            hostname = UNDEFINED_HOSTNAME;
            l_HostNamePrt1 = "''";
            l_HostNamePrt2 = l_HostNamePrt1;
        }

        // Insert input/dft values into errstate...
        bberror << err("in.parms.hostname", l_HostNamePrt1) << err("in.parms.handle", handle) << err("in.parms.transferdefs", l_TransferDefsStr)
                << err("in.dft.hostname", l_HostNamePrt2) << err("in.dft.jobid", jobid) << err("in.dft.jobstepid", jobstepid) << err("in.dft.contribid", contribid);

        // Switch to the uid/gid of requester.
        switchIds();

        // Check permissions
        checkForSuperUserPermission();

        LOG(bb,info) << "msgin_restarttransfers: hostname=" << l_HostNamePrt1 << ", jobid=" << jobid << ", jobstepid=" << jobstepid << ", handle=" << handle << ", contribid=" << contribid;

        if (getSuspendState(DEFAULT_SERVER_ALIAS) == SUSPENDED)
        {
            // A retry could be attempted in this suspended scenario.  Return -2.
            rc = -2;
            errorText << "Connection to the active server is suspended. Attempt to retry the restart transfers request when the connection is not suspended.";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        l_TransferDefs = new BBTransferDefs();

        // Demarshall the archive into the transfer definitions object
        l_TransferDefs->demarshall(l_TransferDefsStr);

        // Process the transfer definitions object for the restart transfers operation
        l_TransferDefs->restartTransfers(hostname, jobid, jobstepid, handle, contribid, l_NumRestartedTransferDefs);
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    if (l_TransferDefs)
    {
        delete l_TransferDefs;
        l_TransferDefs = 0;
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);

    addBBErrorToMsg(response);
    if (!rc)
    {
        response->addAttribute(txp::numTransferDefs, l_NumRestartedTransferDefs);
    }

    sendMessage(pConnectionName, response);
    delete response;
    response=NULL;

#ifdef PROF_TIMING
    std::chrono::high_resolution_clock::time_point time_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return;
}

void msgin_resume(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc = 0;
    stringstream errorText;
    CONNECTION_SUSPEND_OPTION l_EntrySuspendState = UNDEFINED;

    string l_HostName;
    string l_HostNamePrt1;
    string l_HostNamePrt2;

    ResponseDescriptor reply;
    txp::Msg* msgserver = 0;

    FL_Write(FLProxy, Msg_Resume, "Resume command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "BB_Resume");
    try
    {
        // Retrieve the on-entry suspend state
        l_EntrySuspendState = getSuspendState(DEFAULT_SERVER_ALIAS);

        // Retrieve the data from the message
        string hostname = (const char*)msg->retrieveAttrs()->at(txp::hostname)->getDataPtr();

        activecontroller->gethostname(l_HostName);

        l_HostNamePrt1 = hostname;
        l_HostNamePrt2 = l_HostNamePrt1;
        if (hostname == UNDEFINED_HOSTNAME)
        {
            // If not specified, the default hostname is this compute node...
            hostname = l_HostName;
            l_HostNamePrt2 = hostname;
        }
        else if (hostname == NO_HOSTNAME)
        {
            hostname = UNDEFINED_HOSTNAME;
            l_HostNamePrt1 = "''";
            l_HostNamePrt2 = l_HostNamePrt1;
        }

        // Insert input/dft values into errstate...
        bberror << err("in.parms.hostname", l_HostNamePrt1) << err("in.dft.hostname", l_HostNamePrt2);

        // Switch to the uid/gid of requester.
        switchIds();

        // Check permissions
        checkForSuperUserPermission();

        LOG(bb,info) << "msgin_resume: hostname=" << l_HostNamePrt1;

        if (hostname == UNDEFINED_HOSTNAME || hostname == l_HostName)
        {
            // Update the suspend state before sending the request to bbServer.
            // This is done here for inverse symmetry with msgin_suspend().  The
            // suspend state in that routine is set after a successful completion
            // on bbServer.
            updateSuspendMap(DEFAULT_SERVER_ALIAS, NOT_SUSPENDED);
        }

        // Build the message to send to bbserver
        txp::Msg::buildMsg(txp::BB_RESUME, msgserver);
        // NOTE:  The char array is copied to heap by addAttribute and the storage for
        //        the hostname attribute is owned by the message facility.
        //        Our copy can then go out of scope...
        msgserver->addAttribute(txp::hostname, hostname.c_str(), hostname.size()+1, txp::COPY_TO_HEAP);

        // Send the message to bbserver
        rc=sendMessage(DEFAULT_SERVER_ALIAS, msgserver, reply);
        delete msgserver;
        msgserver=NULL;
        if (rc)
        {
            errorText << "sendMessage to server failed";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Wait for the response
        rc = waitReply(reply, msgserver);
        if (rc)
        {
            errorText << "waitReply failure";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        if (!msgserver)
        {
            rc = -1;
            errorText << "waitReply failure - null message returned";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Process response data
        rc = bberror.merge(msgserver);

        delete msgserver;
        msgserver = NULL;

    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    if (!rc)
    {
        LOG(bb,info) << "Connection from the CN hostname " << l_HostNamePrt2 << " is now marked as resumed to the active bbServer";
    }
    else
    {
        // On any error, restore the on-entry suspend state
        updateSuspendMap(DEFAULT_SERVER_ALIAS, l_EntrySuspendState);
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;
}

void msgin_retrievetransfers(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc = 0;
    stringstream errorText;

    uint64_t jobid = UNDEFINED_JOBID;
    uint64_t jobstepid = UNDEFINED_JOBSTEPID;
    uint64_t handle = UNDEFINED_HANDLE;
    uint64_t retrieveflags;
    uint32_t contribid = UNDEFINED_CONTRIBID;
    string l_HostName;

    ResponseDescriptor reply;
    txp::Msg* msgserver = 0;

    uint32_t l_DataObtainedLocally = 0;
    uint32_t l_NumTransferDefs = 0;
    uint64_t l_NumBytesAvailable = 0;
    string l_TransferDefs;

    FL_Write(FLProxy, Msg_RetrieveTransfers, "RetrieveTransfers command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "BB_RetrieveTransfers");
    try
    {
        // Retrieve the data from the message
        string hostname = (const char*)msg->retrieveAttrs()->at(txp::hostname)->getDataPtr();
        handle = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::handle))->getData();
        retrieveflags = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::flags))->getData();

        // Resolve the jobid/jobstepid/contribid
        if (bbconnectionName.size())
        {
            jobid = getJobId(bbconnectionName);
            jobstepid = getJobStepId(bbconnectionName);
            contribid = getContribId(bbconnectionName);
        }
        else
        {
            rc = ENOTCONN;
            errorText << "NULL connection name";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        activecontroller->gethostname(l_HostName);

        string l_HostNamePrt1 = hostname;
        string l_HostNamePrt2 = l_HostNamePrt1;
        if (hostname == UNDEFINED_HOSTNAME)
        {
            // If not specified, the default hostname is this compute node...
            hostname = l_HostName;
            l_HostNamePrt2 = hostname;
        }
        else if (hostname == NO_HOSTNAME)
        {
            hostname = UNDEFINED_HOSTNAME;
            l_HostNamePrt1 = "''";
            l_HostNamePrt2 = l_HostNamePrt1;
        }

        // Insert input/dft values into errstate...
        bberror << err("in.parms.hostname", l_HostNamePrt1) << err("in.parms.handle", handle) << err("in.parms.rtvflags", retrieveflags) \
                << err("in.dft.hostname", l_HostNamePrt2) << err("in.dft.jobid", jobid) << err("in.dft.jobstepid", jobstepid) << err("in.dft.contribid", contribid);

        // Switch to the uid/gid of requester.
        switchIds();

        // Check permissions
        checkForSuperUserPermission();

        LOG(bb,info) << "msgin_retrievetransfers: hostname=" << l_HostNamePrt1 << ", jobid=" << jobid << ", jobstepid=" << jobstepid << ", handle=" << handle << ", contribid=" << contribid;

        // Build the message to send to bbserver
        txp::Msg::buildMsg(txp::BB_RETRIEVE_TRANSFERS, msgserver);
        msgserver->addAttribute(txp::jobid, jobid);
        msgserver->addAttribute(txp::jobstepid, jobstepid);
        msgserver->addAttribute(txp::handle, handle);
        msgserver->addAttribute(txp::contribid, contribid);
        msgserver->addAttribute(txp::flags, retrieveflags);
        // NOTE:  The char array is copied to heap by addAttribute and the storage for
        //        the hostname attribute is owned by the message facility.
        //        Our copy can then go out of scope...
        msgserver->addAttribute(txp::hostname, hostname.c_str(), hostname.size()+1, txp::COPY_TO_HEAP);

        // Send the message to bbserver
        rc=sendMessage(DEFAULT_SERVER_ALIAS, msgserver, reply);
        delete msgserver;
        msgserver=NULL;
        if (rc)
        {
            errorText << "sendMessage to server failed";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Wait for the response
        rc = waitReply(reply, msgserver);
        if (rc)
        {
            errorText << "waitReply failure";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        if (!msgserver)
        {
            rc = -1;
            errorText << "waitReply failure - null message returned";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Process response data
        rc = bberror.merge(msgserver);
        if (!rc)
        {
            l_DataObtainedLocally = ((txp::Attr_uint32*)msgserver->retrieveAttrs()->at(txp::dataObtainedLocally))->getData();
            l_NumTransferDefs = ((txp::Attr_uint32*)msgserver->retrieveAttrs()->at(txp::numTransferDefs))->getData();
            l_NumBytesAvailable = (uint64_t)(msgserver->retrieveAttrs()->at(txp::transferdefs)->getDataLength());
            // NOTE: archive string is already null terminated and the length accounts for the null terminator
            l_TransferDefs.assign((const char*)msgserver->retrieveAttrs()->at(txp::transferdefs)->getDataPtr(), l_NumBytesAvailable);
            LOG(bb,info) << "l_DataObtainedLocally = " << l_DataObtainedLocally << ", l_NumTransferDefs = " << l_NumTransferDefs
                         << ", l_NumBytesAvailable = " << l_NumBytesAvailable;
            LOG(bb,debug) << "l_TransferDefs = |" << l_TransferDefs << "|";
        }
        delete(msgserver);
        msgserver=NULL;

    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);

    addBBErrorToMsg(response);
    if (!rc)
    {
        response->addAttribute(txp::dataObtainedLocally, l_DataObtainedLocally);
        response->addAttribute(txp::numTransferDefs, l_NumTransferDefs);
        response->addAttribute(txp::transferdefs, l_TransferDefs.c_str(), l_NumBytesAvailable);
    }

    sendMessage(pConnectionName,response);
    delete response;
    response=NULL;

#ifdef PROF_TIMING
    std::chrono::high_resolution_clock::time_point time_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return;
}

void msgin_setthrottlerate(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc = 0;
    stringstream errorText;

    const char* mountpoint;
    char* l_MountPoint = 0;
    Uuid l_lvuuid;
    uint64_t rate;

    txp::Msg* msgserver = 0;
    ResponseDescriptor reply;

    FL_Write(FLProxy, Msg_SetThrottleRate, "SetThrottleRate command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "BB_SetThrottleRate");

    try
    {
        // Retrieve the data from the message
        mountpoint = (const char*)msg->retrieveAttrs()->at(txp::mountpoint)->getDataPtr();
        rate = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::rate))->getData();

        // Insert input values into errstate...
        bberror << err("in.parms.mount", mountpoint) << err("in.parms.rate", rate);

        // Switch to the uid/gid of requester.
        switchIds();

        LOG(bb,info) << "msgin_setthrottlerate: mountpoint=" << mountpoint << ", rate=" << rate;

        l_MountPoint = realpath(mountpoint, NULL);

        if (l_MountPoint) {
            rc = getLogicalVolumeUUID(l_MountPoint, l_lvuuid);
            if (rc) {
                rc = -1;
                errorText << "Retrieving the uuid for the logical volume to to set the throttle rate for failed, rc=" << rc << ". The mount point may not exist.";
                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
            }
        } else {
            rc = -1;
            errorText << "Could not determine an absolute path for " << mountpoint << ".  The mountpoint directory may not exist.";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Build the message to send to bbserver
        txp::Msg::buildMsg(txp::BB_SETTHROTTLERATE, msgserver);
        char l_lvuuid_str[LENGTH_UUID_STR] = {'\0'};
        l_lvuuid.copyTo(l_lvuuid_str);
        // NOTE:  The char array is copied to heap by addAttribute and the storage for
        //        the logical volume uuid attribute is owned by the message facility.
        //        Our copy can then go out of scope...
        msgserver->addAttribute(txp::uuid, l_lvuuid_str, sizeof(l_lvuuid_str), txp::COPY_TO_HEAP);
        msgserver->addAttribute(txp::rate, rate);

        // Send the message to bbserver
        rc=sendMessage(DEFAULT_SERVER_ALIAS, msgserver, reply);
        delete msgserver;
        msgserver=NULL;
        if (rc)
        {
            errorText << "sendMessage to server failed";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Wait for the response
        rc = waitReply(reply, msgserver);
        if (rc)
        {
            errorText << "waitReply failure";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        if (!msgserver)
        {
            rc = -1;
            errorText << "waitReply failure - null message returned";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Process response data
        rc = bberror.merge(msgserver);
        delete msgserver;
        msgserver=NULL;

    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    if (l_MountPoint){
        free(l_MountPoint);
        l_MountPoint=NULL;
    }

    // Build/send the response message
    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;
}

void msgin_stageout_start(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc = 0;
    stringstream errorText;

    const char* mountpoint = 0;
    char* l_MountPoint = 0;
    Uuid l_lvuuid;
    ResponseDescriptor reply;
    txp::Msg* msgserver = 0;
    char* l_VolumeGroupName = 0;
    char* l_DevName = 0;

    FL_Write(FLProxy, Msg_StageoutStarted, "StageoutStarted command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "Coral_StageOutStart");
    try
    {
        mountpoint = (char*)(msg->retrieveAttrs()->at(txp::mountpoint)->getDataPtr());
        bberror << err("in.parms.mount", mountpoint);


        // Switch to the uid/gid of requester.
        switchIds();

        // Check permissions
        checkForSuperUserPermission();

        LOG(bb,info) << "msgin_stageout_start: mountpoint=" << mountpoint;

        size_t l_VolumeGroupNameLen = config.get(process_whoami+".volumegroup", DEFAULT_VOLUME_GROUP_NAME).length();
        l_VolumeGroupName = new char[l_VolumeGroupNameLen+1];
        strncpy(l_VolumeGroupName, config.get(process_whoami+".volumegroup", DEFAULT_VOLUME_GROUP_NAME).c_str(), l_VolumeGroupNameLen);
        l_VolumeGroupName[l_VolumeGroupNameLen] = 0;

        l_MountPoint = realpath(mountpoint, NULL);

        if (l_MountPoint) {
            char l_DevName[PATH_MAX] = {'\0'};
            size_t l_DevNameLength = sizeof(l_DevName);
            char l_FileSysType[PATH_MAX] = {'\0'};
            size_t l_FileSysTypeLength = sizeof(l_FileSysType);
            rc = isMounted(l_MountPoint, l_DevName, l_DevNameLength, l_FileSysType, l_FileSysTypeLength);
            if (rc == 1) {
                rc = 0;
                if (strncmp(l_DevName, l_VolumeGroupName, l_VolumeGroupNameLen) == 0) {
                    rc = getLogicalVolumeUUID(l_MountPoint, l_lvuuid);
                    if (rc) {
                        rc = -1;
                        errorText << "Could not determine the uuid of the logical volume associated with " << l_MountPoint << ". The mount point may no longer have a mounted file system or the directory for the mount point may not exist.";
                        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                    }
                } else {
                    rc = -1;
                    errorText << "Mount point " << l_MountPoint << " is associated with device " << l_DevName << " which is not associated with volume group " << l_VolumeGroupName;
                    LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                }
            } else {
                if (rc == 0) {
                    rc = -1;
                    errorText << "Mount point " << l_MountPoint << " is not currently mounted.";
                    LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                } else {
                    errorText << "Error occurred when trying to determine if " << l_MountPoint << " is mounted.";
                    LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                }
            }
            free(l_MountPoint);
            l_MountPoint=NULL;
        }

        txp::Msg::buildMsg(txp::CORAL_STAGEOUT_START, msgserver);

        char l_lvuuid_str[LENGTH_UUID_STR] = {'\0'};
        l_lvuuid.copyTo(l_lvuuid_str);
        // NOTE:  The char array is copied to heap by addAttribute and the storage for
        //        the logical volume uuid attribute is owned by the message facility.
        //        Our copy can then go out of scope...
        LOG(bb,debug) << "msgin_stageout_start: LV Uuid = " << l_lvuuid_str;
        msgserver->addAttribute(txp::uuid, l_lvuuid_str, sizeof(l_lvuuid_str), txp::COPY_TO_HEAP);

        // Send the message to bbserver
        rc=sendMessage(DEFAULT_SERVER_ALIAS, msgserver, reply);
        delete msgserver;
        msgserver=NULL;
        if (rc)
        {
            errorText << "sendMessage to server failed";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Wait for the response
        rc = waitReply(reply, msgserver);
        if (rc)
        {
            errorText << "waitReply failure";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        if (!msgserver)
        {
            rc = -1;
            errorText << "waitReply failure - null message returned";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Process response data
        rc = bberror.merge(msgserver);
        delete msgserver;
        msgserver=NULL;

    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }
    if (l_MountPoint){
        free(l_MountPoint);
        l_MountPoint=NULL;
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    if (l_DevName) {
        delete[] l_DevName;
        l_DevName = 0;
    }

    if (l_VolumeGroupName) {
        delete[] l_VolumeGroupName;
    }

    addReply(msg, response);

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;
}


void msgin_starttransfer(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc = 0;
    stringstream errorText;

    uint64_t  l_JobId = UNDEFINED_JOBID;
    uint64_t  l_JobStepId = UNDEFINED_JOBSTEPID;
    uint64_t  l_Handle = 0;
    uint32_t  l_ContribId = 0;
    BBTransferDef l_Transfer;

    FL_Write(FLProxy, Msg_StartTransfer, "StartTransfer command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "BB_StartTransfer");

    try
    {
        // Retrieve the data from the message
        l_Handle = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::handle))->getData();

        // Resolve the jobid, jobstepid, and contribid identifiers
        if (bbconnectionName.size())
        {
            l_JobId = getJobId(bbconnectionName);
            l_JobStepId = getJobStepId(bbconnectionName);
            l_ContribId = getContribId(bbconnectionName);
        }
        else
        {
            rc = ENOTCONN;
            errorText << "NULL connection name";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        BBTransferDef::demarshallTransferDef(msg, &l_Transfer);

        // NOTE:  If this is a new transfer definition (not rebuilt from the metadata), the contribid,
        //        tag, and handle values are not yet set in the transfer definition that was
        //        demarshalled above.  Those are set in bbserver, before the transfer definition
        //        is stored into the metadata.
        l_Transfer.uid = threadLocaluid;
        l_Transfer.gid = threadLocalgid;
        l_Transfer.setJob(l_JobId, l_JobStepId);

        if (config.get(process_whoami+".bringup.dumpTransferDefinitionAfterDemarshall", 0)) {
            l_Transfer.dump("info", "demarshallTransferDef");
        }

        // Insert the parms into errstate...
        bberror << err("in.parms.handle", l_Handle) << err("dft.jobid",l_JobId);
        bberror << err("in.misc.uid", l_Transfer.uid) << err("in.misc.gid", l_Transfer.gid);

        // Switch to the uid/gid of requester.
        switchIds();

        LOG(bb,info) << "msgin_starttransfer: jobid=" << l_JobId << ", jobstepid=" << l_JobStepId << ", handle=" << l_Handle << ", contribid=" << l_ContribId;

        if (getSuspendState(DEFAULT_SERVER_ALIAS) == SUSPENDED)
        {
            // A retry could be attempted in this suspended scenario.  Return -2.
            rc = -2;
            errorText << "Connection to the active server is suspended. Attempt to retry the start transfer request when the connection is not suspended.";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        if (startTransfer(&l_Transfer, l_JobId, l_JobStepId, l_Handle, l_ContribId))
        {
            BAIL;
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;
}


void msgin_stoptransfers(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc = 0;
    stringstream errorText;

    uint64_t jobid = UNDEFINED_JOBID;
    uint64_t jobstepid = UNDEFINED_JOBSTEPID;
    uint64_t handle = UNDEFINED_HANDLE;
    uint32_t contribid = UNDEFINED_CONTRIBID;
    string l_HostName;

    ResponseDescriptor reply;
    txp::Msg* msgserver = 0;

    string l_TransferDefs;
    uint32_t l_NumStoppedTransferDefs = 0;

    FL_Write(FLProxy, Msg_StopTransfers, "StopTransfers command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "BB_StopTransfers");
    try
    {
        // Retrieve the data from the message
        string hostname = (const char*)msg->retrieveAttrs()->at(txp::hostname)->getDataPtr();
        handle = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::handle))->getData();
        // NOTE: archive string is already null terminated and the length accounts for the null terminator
        l_TransferDefs.assign((const char*)msg->retrieveAttrs()->at(txp::transferdefs)->getDataPtr(), (uint64_t)(msg->retrieveAttrs()->at(txp::transferdefs)->getDataLength()));

        // Resolve the jobid/jobstepid/contribid
        if (bbconnectionName.size())
        {
            jobid = getJobId(bbconnectionName);
            jobstepid = getJobStepId(bbconnectionName);
            contribid = getContribId(bbconnectionName);
        }
        else
        {
            rc = ENOTCONN;
            errorText << "NULL connection name";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        activecontroller->gethostname(l_HostName);

        string l_HostNamePrt1 = hostname;
        string l_HostNamePrt2 = l_HostNamePrt1;
        if (hostname == UNDEFINED_HOSTNAME)
        {
            // If not specified, the default hostname is this compute node...
            hostname = l_HostName;
            l_HostNamePrt2 = hostname;
        }
        else if (hostname == NO_HOSTNAME)
        {
            hostname = UNDEFINED_HOSTNAME;
            l_HostNamePrt1 = "''";
            l_HostNamePrt2 = l_HostNamePrt1;
        }

        // Insert input/dft values into errstate...
        bberror << err("in.parms.hostname", l_HostNamePrt1) << err("in.parms.handle", handle) << err("in.parms.transferdefs", l_TransferDefs)
                << err("in.dft.hostname", l_HostNamePrt2) << err("in.dft.jobid", jobid) << err("in.dft.jobstepid", jobstepid) << err("in.dft.contribid", contribid);

        // Switch to the uid/gid of requester.
        switchIds();

        // Check permissions
        checkForSuperUserPermission();

        LOG(bb,info) << "msgin_stoptransfers: hostname=" << l_HostNamePrt1 << ", jobid=" << jobid << ", jobstepid=" << jobstepid << ", handle=" << handle << ", contribid=" << contribid;

        // Build the message to send to bbserver
        txp::Msg::buildMsg(txp::BB_STOP_TRANSFERS, msgserver);
        msgserver->addAttribute(txp::jobid, jobid);
        msgserver->addAttribute(txp::jobstepid, jobstepid);
        msgserver->addAttribute(txp::handle, handle);
        msgserver->addAttribute(txp::contribid, contribid);
        // NOTE:  The char array is copied to heap by addAttribute and the storage for
        //        the hostname/archive attributes are owned by the message facility.
        //        Our copy can then go out of scope...
        msgserver->addAttribute(txp::hostname, hostname.c_str(), hostname.size()+1, txp::COPY_TO_HEAP);
        msgserver->addAttribute(txp::transferdefs, (const char*)msg->retrieveAttrs()->at(txp::transferdefs)->getDataPtr(), (uint64_t)(msg->retrieveAttrs()->at(txp::transferdefs)->getDataLength()), txp::COPY_TO_HEAP);

        // Send the message to bbserver
        rc=sendMessage(DEFAULT_SERVER_ALIAS, msgserver, reply);
        delete msgserver;
        msgserver=NULL;
        if (rc)
        {
            errorText << "sendMessage to server failed";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Wait for the response
        rc = waitReply(reply, msgserver);
        if (rc)
        {
            errorText << "waitReply failure";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        if (!msgserver)
        {
            rc = -1;
            errorText << "waitReply failure - null message returned";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Process response data
        rc = bberror.merge(msgserver);
        if (!rc)
        {
            l_NumStoppedTransferDefs = ((txp::Attr_uint32*)msgserver->retrieveAttrs()->at(txp::numTransferDefs))->getData();
        }
        delete(msgserver);
        msgserver=NULL;

    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);


    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);

    addBBErrorToMsg(response);
    if (!rc)
    {
        response->addAttribute(txp::numTransferDefs, l_NumStoppedTransferDefs);
    }

    rc=sendMessage(pConnectionName,response);
    delete response;
    response=NULL;
    if (rc)
    {
        errorText << "sendMessage to server failed";
        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
    }

#ifdef PROF_TIMING
    std::chrono::high_resolution_clock::time_point time_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return;
}

void msgin_suspend(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc = 0;
    stringstream errorText;

    string l_HostName;

    ResponseDescriptor reply;
    txp::Msg* msgserver = 0;

    FL_Write(FLProxy, Msg_Suspend, "Suspend command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("in.apicall", "BB_Suspend");
    try
    {
        // Retrieve the data from the message
        string hostname = (const char*)msg->retrieveAttrs()->at(txp::hostname)->getDataPtr();

        activecontroller->gethostname(l_HostName);

        string l_HostNamePrt1 = hostname;
        string l_HostNamePrt2 = l_HostNamePrt1;
        if (hostname == UNDEFINED_HOSTNAME)
        {
            // If not specified, the default hostname is this compute node...
            hostname = l_HostName;
            l_HostNamePrt2 = hostname;
        }
        else if (hostname == NO_HOSTNAME)
        {
            hostname = UNDEFINED_HOSTNAME;
            l_HostNamePrt1 = "''";
            l_HostNamePrt2 = l_HostNamePrt1;
        }

        // Insert input/dft values into errstate...
        bberror << err("in.parms.hostname", l_HostNamePrt1) << err("in.dft.hostname", l_HostNamePrt2);


        // Switch to the uid/gid of requester.
        switchIds();

        // Check permissions
        checkForSuperUserPermission();

        LOG(bb,info) << "msgin_suspend: hostname=" << l_HostNamePrt1;

        // Build the message to send to bbserver
        txp::Msg::buildMsg(txp::BB_SUSPEND, msgserver);
        // NOTE:  The char array is copied to heap by addAttribute and the storage for
        //        the hostname attribute is owned by the message facility.
        //        Our copy can then go out of scope...
        msgserver->addAttribute(txp::hostname, hostname.c_str(), hostname.size()+1, txp::COPY_TO_HEAP);

        // Send the message to bbserver
        rc=sendMessage(DEFAULT_SERVER_ALIAS, msgserver, reply);
        delete msgserver;
        msgserver=NULL;
        if (rc)
        {
            errorText << "sendMessage to server failed";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Wait for the response
        rc = waitReply(reply, msgserver);
        if (rc)
        {
            errorText << "waitReply failure";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        if (!msgserver)
        {
            rc = -1;
            errorText << "waitReply failure - null message returned";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Process response data
        rc = bberror.merge(msgserver);

        if (!rc)
        {
            if (hostname == UNDEFINED_HOSTNAME || hostname == l_HostName)
            {
                updateSuspendMap(DEFAULT_SERVER_ALIAS, SUSPENDED);
                LOG(bb,info) << "Connection from the CN hostname " << l_HostNamePrt2 << " is now marked as suspended to the active bbServer";
            }
        }

        delete msgserver;
        msgserver = NULL;

    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;
}


//*****************************************************************************
//  bbServer -> bbProxy messages
//*****************************************************************************

void msgin_all_file_transfers_complete(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    FL_Write(FLProxy, Msg_AllXferDone, "file transfers complete for file id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    // Stageout is/has ended on the compute node and all file transfers are complete
    // for the job associated with a logical volume...

    Uuid lv_uuid = Uuid((char*)(msg->retrieveAttrs()->at(txp::uuid)->getDataPtr()));
    char l_DevName[1024] = {'\0'};
    getLogicalVolumeDevName(lv_uuid, l_DevName, sizeof(l_DevName));
    LOG(bb,info) << "All file transfers complete: LV device = " << l_DevName << ", LV uuid = " << lv_uuid
                 << ". See individual handle/contributor/file messages for additional status.";

    EXIT(__FILE__,__FUNCTION__);
    return;
}

void msgin_all_file_transfers_complete_for_contribid(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    FL_Write(FLProxy, Msg_ContribIDxferDone, "file transfers complete for contribid--Msg id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    // All file transfers are complete for the contributor id...

    Uuid lv_uuid = Uuid((char*)(msg->retrieveAttrs()->at(txp::uuid)->getDataPtr()));
    char l_DevName[1024] = {'\0'};
    getLogicalVolumeDevName(lv_uuid, l_DevName, sizeof(l_DevName));
    uint64_t l_Handle = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::handle))->getData();
    uint32_t l_ContribId = ((txp::Attr_uint32*)msg->retrieveAttrs()->at(txp::contribid))->getData();
    BBSTATUS l_Status = (BBSTATUS)((txp::Attr_int64*)msg->retrieveAttrs()->at(txp::status))->getData();
    char l_StatusStr[64] = {'\0'};
    getStrFromBBStatus(l_Status, l_StatusStr, sizeof(l_StatusStr));
    char l_TransferStatusStr[64] = {'\0'};
    switch (l_Status) {
        case BBSTOPPED:
        {
            strCpy(l_TransferStatusStr, "stopped", sizeof(l_TransferStatusStr));
            break;
        }
        case BBFAILED:
        {
            strCpy(l_TransferStatusStr, "failed", sizeof(l_TransferStatusStr));
            break;
        }
        case BBCANCELED:
        {
            strCpy(l_TransferStatusStr, "canceled", sizeof(l_TransferStatusStr));
            break;
        }
        default:
        {
            strCpy(l_TransferStatusStr, "completed", sizeof(l_TransferStatusStr));
            break;
        }
    }
    LOG(bb,info) << "Transfer " << l_TransferStatusStr << " for contribid " << l_ContribId << ": LV device = " \
                 << l_DevName << ", handle = " << l_Handle << ", status " << l_StatusStr;

    EXIT(__FILE__,__FUNCTION__);
    return;
}

void msgin_file_transfer_complete_for_file(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    int rc = 0;

    filehandle* fh = NULL;

    ENTRY(__FILE__,__FUNCTION__);
    FL_Write(FLProxy, Msg_xferFileComplete, "file transfers complete for file id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    // All extents for a given sourceindex have been transferred to the I/O node...

    Uuid lv_uuid = Uuid((char*)(msg->retrieveAttrs()->at(txp::uuid)->getDataPtr()));
    char l_DevName[1024] = {'\0'};
    getLogicalVolumeDevName(lv_uuid, l_DevName, sizeof(l_DevName));
    uint64_t l_JobId = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::jobid))->getData();
    uint64_t l_Handle = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::handle))->getData();
    uint32_t l_ContribId = ((txp::Attr_uint32*)msg->retrieveAttrs()->at(txp::contribid))->getData();
    uint32_t l_SourceIndex = ((txp::Attr_uint32*)msg->retrieveAttrs()->at(txp::sourceindex))->getData();
    uint32_t l_TargetIndex = (BBTransferDef::getTargetIndex(l_SourceIndex));
    char* l_SourceFile = (char*)(msg->retrieveAttrs()->at(txp::sourcefile))->getDataPtr();
    BBFILESTATUS l_FileStatus = (BBFILESTATUS)((txp::Attr_int64*)msg->retrieveAttrs()->at(txp::status))->getData();
    char l_TransferType[64] = {'\0'};
    getStrFromTransferType(((txp::Attr_int64*)msg->retrieveAttrs()->at(txp::flags))->getData(), l_TransferType, sizeof(l_TransferType));
    uint64_t l_SizeTransferred = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::sizetransferred))->getData();

    // NOTE: No processing to perform for a local cp transfer...
    if (!((((txp::Attr_int64*)msg->retrieveAttrs()->at(txp::flags))->getData()) & BBI_TargetSSDSSD))
    {
        bool l_StageoutStarted = (((txp::Attr_int64*)msg->retrieveAttrs()->at(txp::flags))->getData()) & BBLVK_Stage_Out_Start ? true : false;

        if ((((txp::Attr_int64*)msg->retrieveAttrs()->at(txp::flags))->getData()) & BBI_TargetSSD ||
            (((txp::Attr_int64*)msg->retrieveAttrs()->at(txp::flags))->getData()) & BBI_TargetPFS ||
            (((txp::Attr_int64*)msg->retrieveAttrs()->at(txp::flags))->getData()) & BBI_TargetPFSPFS)
        {
            try
            {
                // Remove source file (if open)
                if (!removeFilehandle(fh, l_JobId, l_Handle, l_ContribId, l_SourceIndex))
                {
                    LOG(bb,info) << "Removing filehandle '" << fh->getfn() << "'";
                    if (fh->release(l_FileStatus))
                    {
                        throw runtime_error(string("Unable to release filehandle"));
                    }
                }
                else
                {
                    // NOTE:  fh will be NULL in this leg
                    if (!l_StageoutStarted)
                    {
                        LOG(bb,debug) << "Unable to find file associated with handle " << l_Handle << ", contrib " << l_ContribId << ", source index " << l_SourceIndex;
                    }
                    else
                    {
                        // NOTE:  If stageout has started, the handle has already been cleaned up when the file system was torn down...
                    }
                }
            }
            catch(exception& e)
            {
                rc = -1;
                LOG(bb,warning) << "Exception thrown when processing transfer complete for file: " << e.what();
            }

            if (fh)
            {
                delete fh;
                fh = NULL;
            }

            if ((((txp::Attr_int64*)msg->retrieveAttrs()->at(txp::flags))->getData()) & BBI_TargetSSD ||
                (((txp::Attr_int64*)msg->retrieveAttrs()->at(txp::flags))->getData()) & BBI_TargetPFS)
            {
                try
                {
                    // Remove target file (if open)
                    if (!removeFilehandle(fh, l_JobId, l_Handle, l_ContribId, l_TargetIndex))
                    {
                        LOG(bb,info) << "Removing filehandle '" << fh->getfn() << "'";
                        if (fh->release(l_FileStatus))
                        {
                            throw runtime_error(string("Unable to release filehandle"));
                        }
                    }
                    else
                    {
                        // NOTE:  fh will be NULL in this leg
                        if (!l_StageoutStarted)
                        {
                            LOG(bb,debug) << "Unable to find file associated with handle " << l_Handle << ", contrib " << l_ContribId << ", target index " << l_TargetIndex;
                        }
                        else
                        {
                            // NOTE:  If stageout has started, the handle has already been cleaned up when the file system was torn down...
                        }
                    }
                }
                catch(exception& e)
                {
                    rc = -1;
                    LOG(bb,warning) << "Exception thrown when processing transfer complete for file: " << e.what();
                }

                if (fh)
                {
                    delete fh;
                    fh = NULL;
                }
            }
        }
    }

    // Build the response message
    txp::Msg* response;
    msg->buildResponseMsg(response);

    // NOTE:  rc is the return code being put into the response message as a simple attribute.
    //        It is NOT being inserted into the bberror information here...
    txp::Attr_int32 returncode(txp::returncode, rc);
    response->addAttribute(&returncode);
    addReply(msg, response);

    if (rc)
    {
        l_FileStatus = BBFILE_FAILED;
        l_SizeTransferred = 0;
        LOG(bb,info) << "For jobid " << l_JobId << ", handle " << l_Handle << ", contribid " << l_ContribId << ", source index " << l_SourceIndex << ", target index " << l_TargetIndex \
                     << ", failure during close related processing for the source/target file has caused the file transfer status to be changed to BBFILE_FAILED";
    }

    char l_FileStatusStr[64] = {'\0'};
    getStrFromBBFileStatus(l_FileStatus, l_FileStatusStr, sizeof(l_FileStatusStr));
    char l_TransferStatusStr[64] = {'\0'};
    switch (l_FileStatus) {
        case BBFILE_STOPPED:
        {
            strCpy(l_TransferStatusStr, "stopped", sizeof(l_TransferStatusStr));
            break;
        }
        case BBFILE_FAILED:
        {
            strCpy(l_TransferStatusStr, "failed", sizeof(l_TransferStatusStr));
            break;
        }
        case BBFILE_CANCELED:
        {
            strCpy(l_TransferStatusStr, "canceled", sizeof(l_TransferStatusStr));
            break;
        }
        default:
        {
            strCpy(l_TransferStatusStr, "completed", sizeof(l_TransferStatusStr));
            break;
        }
    }
    LOG(bb,info) << "Transfer " << l_TransferStatusStr << " for source file " << l_SourceFile << ", LV device " << l_DevName \
                 << ", jobid " << l_JobId << ", handle " << l_Handle << ", contribid " << l_ContribId << ", sourceindex " << l_SourceIndex \
                 << ", file status " << l_FileStatusStr << ", transfer type " << l_TransferType \
                 << ", size transferred " << l_SizeTransferred;

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);

    return;
}

void msgin_all_file_transfers_complete_for_handle(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    FL_Write(FLProxy, Msg_HdlXferDone, "all file transfers complete for handle id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    // All file transfers are complete for the handle on the I/O node...

    Uuid lv_uuid = Uuid((char*)(msg->retrieveAttrs()->at(txp::uuid)->getDataPtr()));
    char l_DevName[1024] = {'\0'};
    getLogicalVolumeDevName(lv_uuid, l_DevName, sizeof(l_DevName));
    uint64_t l_Handle = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::handle))->getData();
    BBSTATUS l_Status = (BBSTATUS)((txp::Attr_int64*)msg->retrieveAttrs()->at(txp::status))->getData();
    char l_StatusStr[64] = {'\0'};
    getStrFromBBStatus(l_Status, l_StatusStr, sizeof(l_StatusStr));
    char l_TransferStatusStr[64] = {'\0'};
    switch (l_Status) {
        case BBSTOPPED:
        {
            strCpy(l_TransferStatusStr, "stopped, but may be restarted", sizeof(l_TransferStatusStr));
            break;
        }
        case BBFAILED:
        {
            // NOTE: BBFAILED is never set as a handle status today...
            strCpy(l_TransferStatusStr, "failed", sizeof(l_TransferStatusStr));
            break;
        }
        case BBPARTIALSUCCESS:
        {
            strCpy(l_TransferStatusStr, "ended, but may be restarted", sizeof(l_TransferStatusStr));
            break;
        }
        case BBCANCELED:
        {
            strCpy(l_TransferStatusStr, "canceled", sizeof(l_TransferStatusStr));
            break;
        }
        default:
        {
            strCpy(l_TransferStatusStr, "completed", sizeof(l_TransferStatusStr));
            break;
        }
    }
    LOG(bb,info) << "Transfer " << l_TransferStatusStr << " for handle " << l_Handle \
                 << ": LV device = " << l_DevName << ", status " << l_StatusStr;

    EXIT(__FILE__,__FUNCTION__);
    return;
}


void msgin_transfer_progress(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    FL_Write(FLProxy, Msg_XferProg, "Transfer Progress command received id=%ld, number=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    uint64_t l_LVStartingByte = 0;
    uint64_t l_LVCurrentSize = 0;
    uint64_t l_LastByteTransferred = 0;

    Uuid lv_uuid = Uuid((char*)(msg->retrieveAttrs()->at(txp::uuid)->getDataPtr()));
    uint64_t l_JobId = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::jobid))->getData();
    uint32_t l_Count = ((txp::Attr_uint32*)msg->retrieveAttrs()->at(txp::count))->getData();
    uint64_t l_MaxLBA = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::maxlba))->getData();
    uint64_t l_Offset = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::offset))->getData();
    uint64_t l_Length = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::length))->getData();
    char l_DevName[1024] = {'\0'};
    char l_DevName2[1024] = {'\0'};
    getLogicalVolumeDevName(lv_uuid, l_DevName, sizeof(l_DevName));
    if (l_MaxLBA && l_Offset && l_Length)
    {
        l_LastByteTransferred = l_Offset + l_Length;
        LOG(bb,info) << "Progress: LV = " << l_DevName // << ", LV uuid = " << lv_uuid
                     << std::hex << std::uppercase << setfill('0') \
                     << ", lba.maxkey=0x" << setw(16) << l_MaxLBA \
                     << ", lba.start=0x" << setw(16) << l_Offset \
                     << ", len=0x" << setw(8) << l_Length \
                     << setfill(' ') << std::nouppercase << std::dec \
                     << ". " << l_Count << " additional extents left to be scheduled.";
    } else {
        LOG(bb,info) << "Progress: LV device = " << l_DevName << ", LV uuid = " << lv_uuid << ", " << l_Count << " additional extents left to be scheduled.";
    }

    ResizeLogicalVolumeLock();

    {
        try
        {
            // Parse out the volume group and LV device names...
            strCpy(l_DevName2, l_DevName, sizeof(l_DevName2));
            char* l_VolumeGroupName = strchr(l_DevName2, '/')+1;
            l_VolumeGroupName = strchr(l_VolumeGroupName, '/');
            *l_VolumeGroupName++ = '\0';
            char* l_LogicalVolumeDevName = strchr(l_VolumeGroupName, '/');
            *l_LogicalVolumeDevName++ = '\0';

            // Build a lookup vector with the LVM extent data...
            LVLookup l_LVLookup;
            string l_VolumeGroupNameStr(l_VolumeGroupName);
            string l_LogicalVolumeDevNameStr(l_LogicalVolumeDevName);
            int rc = l_LVLookup.build(l_VolumeGroupNameStr, l_LogicalVolumeDevNameStr);
            if (rc >= 0)
            {
                // Retrieve the starting location for the logical volume and the current size...
                rc = l_LVLookup.getData(l_LVStartingByte, l_LVCurrentSize);
                if (!rc)
                {
                    // Invoke timeToResizeLogicalVolume() to see if a resize operation should be performed...
                    char l_LVNewSize[16] = {'\0'};
                    if (timeToResizeLogicalVolume(l_LastByteTransferred, l_LVStartingByte, l_LVCurrentSize, l_LVNewSize, sizeof(l_LVNewSize)))
                    {
                        // Attempt the resize opertion with the returned new size...
                        LOG(bb,info) << "Progress: l_DevName=" << l_DevName << ", l_LVStartingByte=" << l_LVStartingByte << ", l_LVCurrentSize=" << l_LVCurrentSize << ", l_LastByteTransferred=" << l_LastByteTransferred << ", l_LVNewSize=" << l_LVNewSize;

                        char l_MountPointPath[PATH_MAX+1] = {'\0'};
                        char* l_MountPoint = l_MountPointPath;
                        if (isMounted(l_VolumeGroupName, l_LogicalVolumeDevName, l_MountPoint, sizeof(l_MountPointPath)))
                        {
                            // Mounted file system...
                            l_LogicalVolumeDevName = 0;
                            char l_FileSystemType[32] = {'\0'};
                            if (!(getFileSystemType(l_DevName, l_FileSystemType, sizeof(l_FileSystemType))))
                            {
                                if (strstr(l_FileSystemType, "xfs"))
                                {
                                    // Close out all the file handles for the jobid...
                                    filehandle* l_FileHandle = 0;
                                    removeNextFilehandleByJobId(l_FileHandle, l_JobId);
                                    while (l_FileHandle)
                                    {
                                        LOOP_COUNT(__FILE__,__FUNCTION__,"removed_filehandles");
                                        LOG(bb,info) << "Found filehandle for removal";
                                        rc = l_FileHandle->release(BBFILE_NONE);
                                        LOG(bb,info) << "Releasing fd rc=" << rc;
                                        delete l_FileHandle;
                                        l_FileHandle=NULL;

                                        removeNextFilehandleByJobId(l_FileHandle, l_JobId);
                                    }

                                    rc = resizeLogicalVolume(l_MountPoint, l_LogicalVolumeDevName, l_LVNewSize, (uint64_t)BB_DO_NOT_PRESERVE_FS);
                                    if (rc)
                                    {
                                        LOG(bb,error) << "Resize of logical volume failed (1), rc = " << rc;
                                    }

                                    LOG(bb,info) << "Leaving LVM breadcrumb of " << lv_uuid << " for " << l_DevName << " at " << l_MountPoint;
                                    int rc2 = leaveLVM_BreadCrumb(l_MountPoint, lv_uuid, l_DevName);
                                    if (rc2)
                                    {
                                   	    LOG(bb,error) << "Leaving LVM breadcrumb failed, rc = " << rc2;
                                    }

                                } else if (strstr(l_FileSystemType, "ext4")) {
                                    rc = resizeLogicalVolume(l_MountPoint, l_LogicalVolumeDevName, l_LVNewSize, (uint64_t)BB_NONE);
                                    if (rc)
                                    {
                                        LOG(bb,error) << "Resize of logical volumn failed (2), rc = " << rc;
                                    }
                                } else {
                                    LOG(bb,error) << "Unexpected and unsupported file system type of " << l_FileSystemType << " discovered for mountpoint " << l_MountPoint;
                                }
                            }
                        } else {
                            // Not currently mounted...
                            l_MountPoint = 0;
                            rc = resizeLogicalVolume(l_MountPoint, l_LogicalVolumeDevName, l_LVNewSize, (uint64_t)BB_DO_NOT_PRESERVE_FS);
                            if (rc)
                            {
                                LOG(bb,error) << "Resize of logical volumn failed (3), rc = " << rc;
                            }
                        }
                    }
                } else {
                    LOG(bb,error) << "Failure when attempting to retrieve the starting location for the logical volume device name " << l_LogicalVolumeDevNameStr;
                }
            } else {
                LOG(bb,error) << "Failure when attempting to build the lookup table to retrieve the starting location for the logical volume device name " << l_LogicalVolumeDevNameStr;
            }
        }
        catch(exception& e)
        {
            LOG(bb,warning) << "Exception thrown when attempting to resize logical volume: " << e.what();
        }
    }

    ResizeLogicalVolumeUnlock();

    EXIT(__FILE__,__FUNCTION__);
    return;
}

void msgin_getserverbyname(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);
    int rc=0;
    FL_Write(FLProxy, GetByServerName, " id=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());

    bberror << err("in.apicall", "msgin_getserverbyname");
    int count=0;
    std::string result="none";
    try
    {
        // Switch to the uid/gid of requester.
        switchIds();
        string serverName = (const char*)msg->retrieveAttrs()->at(txp::hostname)->getDataPtr();
        int64_t l_Value = ((txp::Attr_int64*)msg->retrieveAttrs()->at(txp::value64))->getData();
        BBServerQuery l_query = (BBServerQuery)l_Value;
        //BBWAITFOREPLYCOUNT
        if (l_query==BBWAITFOREPLYCOUNT){
            count = countWaitReplyList(serverName);
            if (count<0) { rc=-1; count=0;}
            result=std::to_string(count);
            std::string temp="out."+serverName+".waitforreplycount";
            bberror<<err("in.serverName",serverName)<<err("in.actionName","waitforreplycount")<<err(temp.c_str(),count);
        }
        else{
            rc=-1;
            bberror<<err("invalid.option",l_Value);;
        }
        if (rc) {
            stringstream errorText;
            errorText << "The getbyservername request failed";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }
        LOG(bb,info)<<"msgin_getserverbyname: l_query="<<l_query<<" serverName="<<serverName<<" waitforreplycount="<<count;
    }
    catch(ExceptionBailout& e) { LOG(bb,always)<<"msgin_getserverbyname: ExceptionBailout caught";}
    catch(exception& e)
    {
        LOG(bb,always)<<"msgin_getserver: exception caught";
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);
    addReply(msg, response);
    if (!rc)
    {
        response->addAttribute(txp::buffer, result.c_str(), result.length() + 1 );
    }

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;

    EXIT(__FILE__,__FUNCTION__);
    return;
}

void msgin_getserver(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);
    int rc=0;
    FL_Write(FLProxy, ListServers, "Query BBserver info id=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());

    std::string result="none";

    bberror << err("in.apicall", "msgin_getserver");

    try
    {

        // Switch to the uid/gid of requester.
        switchIds();

        // Check permissions
        checkForSuperUserPermission();

        int64_t l_Value = ((txp::Attr_int64*)msg->retrieveAttrs()->at(txp::value64))->getData();
        BBServerQuery l_query = (BBServerQuery)l_Value;

        switch(l_query){
            case BBALLCONNECTED:
                result= connectionNameFromAlias() + " " + readyBBserverList();
                break;
            case BBACTIVE:
                result= connectionNameFromAlias();
                break;
            case BBREADY:
                result = readyBBserverList();
                break;
            case BBBACKUP:
                result = getBACKUP();
                break;
            case BBPRIMARY:
                result = getPRIMARY();
                break;
            default:
                rc=EINVAL;
                if (rc) {
                    stringstream errorText;
                    errorText << "The getserver request failed for an invalid option="<<l_query;
                    LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                }
                break;
        }
        LOG(bb,info)<<"msgin_getserver: l_query="<<l_query<<" result="<<result;
    }
    catch(ExceptionBailout& e) { LOG(bb,always)<<"msgin_getserver: ExceptionBailout caught";}
    catch(exception& e)
    {
        LOG(bb,always)<<"msgin_getserver: exception caught";
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    if (!rc)
    {
        response->addAttribute(txp::buffer, result.c_str(), result.length() + 1 );
    }

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;

    EXIT(__FILE__,__FUNCTION__);
    return;
}

void msgin_setserver(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);

    int rc=0;
    stringstream errorText;

    FL_Write(FLProxy, SetServer, "setserver request id=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("inbbproxy.apicall", "SetServer");
    try
    {
        // Switch to the uid/gid of requester.
        switchIds();

        // Check permissions
        checkForSuperUserPermission();

        string serverName = (const char*)msg->retrieveAttrs()->at(txp::hostname)->getDataPtr();
        string actionName = (const char*)msg->retrieveAttrs()->at(txp::variable)->getDataPtr();
        bberror << err("inbbproxy.actionName", actionName)<<err("inbbproxy.serverName",serverName);

        if (serverName=="primary")
        {
            serverName=getPRIMARY();
            bberror << err("inbbproxy.primary",serverName);
        }
        else if (serverName=="backup")
        {
           serverName=getBACKUP();
           bberror << err("inbbproxy.backup",serverName);
        }

        LOG(bb,info) << "setserver action=" << actionName << " for serverName=" << serverName;
        if (actionName=="activate")
        {
            std::string nowActive = connectionNameFromAlias();
            bberror << err("inbbproxy.activeServerBeforeActivate",nowActive);
            if (nowActive!=serverName)
            {
                rc = makeActivebbserver(serverName);
                bberror << err("inbbproxy.activeServerAfterActivate",connectionNameFromAlias());
                if (!rc)
                {
                    // Now register the known Burst Buffer logical volumes to the new bbServer.
                    // We register them here so that an LVKey exists in the new bbServer metadata
                    // and an ensuing start transfer request can be started properly using that
                    // LVKey.
                    string l_HostName;
                    activecontroller->gethostname(l_HostName);

                    vector<string> l_DevNames;
                    findBB_DevNames(l_DevNames, FIND_ALL_MOUNTED_BB_DEVICES);
                    // NOTE:  We pass the uuid of each LV to the new bbServer with a BB_CREATELOGICALVOLUME
                    //        message.  This causes the new bbServer to register the LVKey in it's
                    //        metadata.  This allows an ensuing gethandle/start transfer to be successful
                    //        using that 'registered' logical volume, just as if the 'real' create
                    //        logical volume were performed on the CN when the new bbServer was
                    //        already servicing the CN.
                    for (size_t i=0; i<l_DevNames.size(); ++i)
                    {
                        Uuid l_lvuuid;
                        rc = getUUID(l_DevNames[i].c_str(), l_lvuuid);
                        if (!rc)
                        {
                            char l_lvuuid_str[LENGTH_UUID_STR] = {'\0'};
                            l_lvuuid.copyTo(l_lvuuid_str);
                            LV_Data l_LV_Data = getLogicalVolumeData(l_DevNames[i]);
                            if (l_LV_Data.jobid != UNDEFINED_JOBID)
                            {
                                txp::Msg* msgserver = 0;
                                ResponseDescriptor reply;

                                // NOTE:  We pass the LVs to the new bbServer with a BB_CREATELOGICALVOLUME
                                //        message.  This causes the new bbServer to register the LVKey in it's
                                //        metadata.
                                txp::Msg::buildMsg(txp::BB_CREATELOGICALVOLUME, msgserver);
                                msgserver->addAttribute(txp::uuid, l_lvuuid_str, sizeof(l_lvuuid_str), txp::COPY_TO_HEAP);
                                msgserver->addAttribute(txp::hostname, l_HostName.c_str(), l_HostName.size()+1, txp::COPY_TO_HEAP);
                                msgserver->addAttribute(txp::jobid, l_LV_Data.jobid);
                                msgserver->addAttribute(txp::mntptgid, l_LV_Data.groupid);
                                msgserver->addAttribute(txp::mntptuid, l_LV_Data.userid);
                                msgserver->addAttribute(txp::option, (uint32_t)1);

                                // Send the message to bbserver
                                rc=sendMessage(DEFAULT_SERVER_ALIAS, msgserver, reply);
                                delete msgserver;
                                msgserver = NULL;
                                if (rc)
                                {
                                    errorText << "sendMessage to server failed";
                                    LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                                }

                                // Wait for the response
                                rc = waitReply(reply, msgserver);
                                if (rc)
                                {
                                    errorText << "waitReply failure when processing device " << l_DevNames[i];
                                    LOG_ERROR_TEXT_RC_AND_BAIL(errorText,rc);
                                }

                                if (msgserver)
                                {
                                    delete msgserver;
                                    msgserver = NULL;
                                }
                            }
                            else
                            {
                                // No logical volume currently defined for this connection
                                // NOTE:  This is the case where getLogicalVolumeData() passed back a
                                //        default constructed LV_Data().
                            }
                        }
                        else
                        {
                            rc = 0;
                            errorText << "Could not determine the uuid of the logical volume associated with device " << l_DevNames[i] \
                                      << ". The logical volume associated with this device will not be registered to the new bbServer." \
                                      << ". Processing continues for additional burst buffers logical volumes.";
                            LOG_ERROR_TEXT(errorText);
                        }
                    }
                }
                else
                {
                    stringstream errorText;
                    errorText << "The setserver request activate failed for serverName="<<serverName;
                    LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                }
            }
        }
        else if (actionName=="offline")
        {
            rc =takeActivebbserverOffline(serverName);
            if (rc)
            {
                stringstream errorText;
                errorText << "The setserver request offline failed for serverName="<<serverName;
                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
            }
        }
        else
        {
            rc=EINVAL;
            if (rc)
            {
                stringstream errorText;
                errorText << "The setserver request failed for an invalid action="<<actionName;
                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
            }
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;

    EXIT(__FILE__,__FUNCTION__);
    return;
}

void msgin_openserver(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);
    int rc=0;
    FL_Write(FLProxy, OpenServer, "Open BBserver request id=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("inbbproxy.apicall", "OpenServer");
    try
    {
        // Switch to the uid/gid of requester.
        switchIds();

        // Check permissions
        checkForSuperUserPermission();
        if ( (!threadLocaluid) || (!threadLocalgid) );
        else{
            rc=-1;
            stringstream errorText;
            errorText << "Root user or primary group required";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        string serverName = (const char*)msg->retrieveAttrs()->at(txp::hostname)->getDataPtr();
        bberror << err("inbbproxy.actionName", "close")<<err("inbbproxy.serverName",serverName);
        if ( serverName=="primary")
        {
            serverName=getPRIMARY();
            bberror << err("inbbproxy.primary",serverName);
        }
        else if ( serverName=="backup")
        {
            serverName=getBACKUP();
            bberror << err("inbbproxy.backup",serverName);
        }
        LOG(bb,info) << "Open request for serverName="<<serverName<<" uid="<<threadLocaluid<<" gid="<<threadLocalgid;
        rc = makeConnection2bbserver(serverName);
        if (rc) {
            stringstream errorText;
            errorText << "The open request failed for the bbserver serverName="<<serverName;
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;

    EXIT(__FILE__,__FUNCTION__);
    return;
    }

void msgin_closeserver(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    updateEnv(pConnectionName);
    int rc=0;
    FL_Write(FLProxy, CloseServer, "Close BBserver request id=%ld, request=%ld, len=%ld",msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(),msg->getSerializedLen());
    bberror << err("inbbproxy.apicall", "CloseServer");
    try
    {
        // Switch to the uid/gid of requester.
        switchIds();
        // Check permissions
        checkForSuperUserPermission();
        if ( (!threadLocaluid) || (!threadLocalgid) );
        else{
            rc=-1;
            stringstream errorText;
            errorText << "Root user or primary group required";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        string serverName = (const char*)msg->retrieveAttrs()->at(txp::hostname)->getDataPtr();
        bberror << err("inbbproxy.actionName", "close")<<err("inbbproxy.serverName",serverName);
        if ( serverName=="primary")
        {
            serverName=getPRIMARY();
            bberror << err("inbbproxy.primary",serverName);
        }
        else if ( serverName=="backup")
        {
            serverName=getBACKUP();
            bberror << err("inbbproxy.backup",serverName);
        }
        LOG(bb,info) << "Close request for serverName="<<serverName;
        std::string active = connectionNameFromAlias();
        if (serverName == active){
            rc=EBUSY;
            stringstream errorText;
            errorText << "The bbserver is active for serverName="<<serverName;
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }
        else
        {
            //  NOTE:  We wait up to 2 minutes for the fh map to become empty so that all files closes are
            //         first processed from the 'old' server.  In the case of cancel/stop, we want to process
            //         all closes for those transfer definitions before the connection is closed.  Otherwise,
            //         nothing other than a remove logical volume will close the files.
            //
            //         It is possible for new start transfers to be initialted to the new server and those
            //         file handles would be inserted into the fh map.  But, we don't care about those file
            //         handles and the only harm is the 2 minute wait.
            int l_Continue = 120;
            rc = -1;
            while ((rc) && (l_Continue--))
            {
                rc = fileHandleCount();
                if (rc)
                {
                    usleep((useconds_t)1000000);    // Delay 1 second
                    if (l_Continue % 10 == 0)
                    {
                        dumpFileHandleMap("info", "msgin_closeserver() - Waiting for all files to be closed, ");
                    }
                }
            }

            rc = closeConnectionFD(serverName);
            if (rc)
            {
                stringstream errorText;
                errorText << "The close request failed for the bbserver serverName="<<serverName;
                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
            }
        }

    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;

    EXIT(__FILE__,__FUNCTION__);
    return;
}
//*****************************************************************************
//  Main routines
//*****************************************************************************

int registerHandlers()
{
    registerMessageHandler(txp::BB_ALL_FILE_TRANSFERS_COMPLETE, msgin_all_file_transfers_complete);
    registerMessageHandler(txp::BB_CANCELTRANSFER, msgin_canceltransfer);
    registerMessageHandler(txp::BB_CHMOD, msgin_changemode);
    registerMessageHandler(txp::BB_CHOWN, msgin_changeowner);
    registerMessageHandler(txp::BB_CREATEDIR, msgin_createdirectory);
    registerMessageHandler(txp::BB_CREATELOGICALVOLUME, msgin_createlogicalvolume);
    registerMessageHandler(txp::BB_GETDEVICEUSAGE, msgin_getdeviceusage);
    registerMessageHandler(txp::BB_GETTHROTTLERATE, msgin_getthrottlerate);
    registerMessageHandler(txp::BB_GETTRANSFERHANDLE, msgin_gettransferhandle);
    registerMessageHandler(txp::BB_GETTRANSFERINFO, msgin_gettransferinfo);
    registerMessageHandler(txp::BB_GETTRANSFERKEYS, msgin_gettransferkeys);
    registerMessageHandler(txp::BB_GETTRANSFERLIST, msgin_gettransferlist);
    registerMessageHandler(txp::BB_GETUSAGE, msgin_getusage);
    registerMessageHandler(txp::BB_REMOVEDIR, msgin_removedirectory);
    registerMessageHandler(txp::BB_REMOVEJOBINFO, msgin_removejobinfo);
    registerMessageHandler(txp::BB_REMOVELOGICALVOLUME, msgin_removelogicalvolume);
    registerMessageHandler(txp::BB_RESIZEMOUNTPOINT, msgin_resizemountpoint);
    registerMessageHandler(txp::BB_RESTART_TRANSFERS, msgin_restarttransfers);
    registerMessageHandler(txp::BB_RESUME, msgin_resume);
    registerMessageHandler(txp::BB_RETRIEVE_TRANSFERS, msgin_retrievetransfers);
    registerMessageHandler(txp::BB_SETTHROTTLERATE, msgin_setthrottlerate);
    registerMessageHandler(txp::BB_SETUSAGELIMIT, msgin_setusagelimit);
    registerMessageHandler(txp::BB_STARTTRANSFER, msgin_starttransfer);
    registerMessageHandler(txp::BB_TRANSFER_COMPLETE_FOR_CONTRIBID, msgin_all_file_transfers_complete_for_contribid);
    registerMessageHandler(txp::BB_TRANSFER_COMPLETE_FOR_FILE, msgin_file_transfer_complete_for_file);
    registerMessageHandler(txp::BB_TRANSFER_COMPLETE_FOR_HANDLE, msgin_all_file_transfers_complete_for_handle);
    registerMessageHandler(txp::BB_TRANSFER_PROGRESS, msgin_transfer_progress);
    registerMessageHandler(txp::BB_STOP_TRANSFERS, msgin_stoptransfers);
    registerMessageHandler(txp::BB_SUSPEND, msgin_suspend);
    registerMessageHandler(txp::BB_GETSERVER, msgin_getserver);
    registerMessageHandler(txp::BB_GETSERVERBYNAME, msgin_getserverbyname);
    registerMessageHandler(txp::BB_SETSERVER, msgin_setserver);
    registerMessageHandler(txp::BB_OPENSERVER, msgin_openserver);
    registerMessageHandler(txp::BB_CLOSESERVER, msgin_closeserver);
    registerMessageHandler(txp::CORAL_CHANGESERVER, msgin_change_server);
    registerMessageHandler(txp::CORAL_GETVAR, msgin_getvar);
    registerMessageHandler(txp::CORAL_SETVAR, msgin_setvar);
    registerMessageHandler(txp::CORAL_STAGEOUT_START, msgin_stageout_start);

    return 0;
}


char LVM_SUPPRESS[] = "LVM_SUPPRESS_FD_WARNINGS=1";  // note: ownership of this string is moved to ENV during putenv() call.

int bb_main(std::string who)
{
    ENTRY_NO_CLOCK(__FILE__,__FUNCTION__);

    int            rc;
    pthread_t      tid;
    pthread_attr_t attr;
    // Increase the number of allowed file descriptors...
    struct rlimit l_Limits;
    rc = getrlimit(RLIMIT_NOFILE, &l_Limits);
    if (rc == 0)
    {
        l_Limits.rlim_cur = l_Limits.rlim_max;
        rc = setrlimit(RLIMIT_NOFILE, &l_Limits);
        if (rc == 0)
        {
            LOG(bb,always) << "Maximum number of file descriptors set to " << l_Limits.rlim_cur;
        }
    }
    // Workaround for lvs warning messages on leaked file descriptors.
    putenv(LVM_SUPPRESS);

    LOG(bb,always) << "bbproxy bb_main BBAPI_CLIENTVERSIONSTR="<<BBAPI_CLIENTVERSIONSTR;

    /* Start SSD mount & memory monitor */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid, &attr, mountMonitorThread, NULL);

    /* Open connection to bbServer */
    rc = openConnectionToBBserver();
    if(rc)
    {
        LOG(bb,warning) << "Connection to bbServer failed to open.  rc=" << rc;
    }

    /* Set the master logical volume number from the configuration */
    MasterLogicalVolumeNumber.set(config.get(process_whoami+".startingvolgrpnbr", DEFAULT_MASTER_LOGICAL_VOLUME_NUMBER));

    /* Initialze the mutex for resizing logical volumes */
//    pthread_mutex_init(&ResizeMutex, NULL);

    /* Perform any clean-up of existing logical volumes */
    logicalVolumeCleanup();

    LOG(bb,always) << "bbProxy completed initialization";

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);
    return 0;
}

int bb_exit(std::string who)
{
    ENTRY_NO_CLOCK(__FILE__,__FUNCTION__);

//    printf("bb_exit: who = %s\n", who.c_str());
    if (!who.empty() )
    {
        string upath = config.get("bb.unixpath", DEFAULT_UNIXPATH);
        LOG(bb,info) << "Process " << who << " have upath " << upath;
        if (upath != NO_CONFIG_VALUE)
        {
            struct stat sb;
            LOG(bb,info) << "Cleaning upath";

            if ((stat (upath.c_str(), &sb) == 0) && S_ISSOCK (sb.st_mode))
            {
                LOG(bb,info) << "Unlinking upath";
                unlink (upath.c_str());
            }
        }
        else
        {
            LOG(bb,info) << "No file to unlink";
        }
    }

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);
    return 0;
}

int doAuthenticate(const string& name){
    int rc=0;
    txp::Msg* msg = 0;
    txp::Msg::buildMsg(txp::CORAL_AUTHENTICATE, msg);
    ResponseDescriptor resp;

    LOG(bb,info) << "==> Sending msg to " << name.c_str() << ": CORAL_AUTHENTICATE(SSL) process_whoami=" << \
    process_whoami.c_str() << ", process_instance=" << process_instance.c_str() \
    << ", msg#=" << msg->getMsgNumber() << ", rqstmsg#=" << msg->getRequestMsgNumber();


    txp::AttrPtr_char_array whoami(txp::whoami, process_whoami.c_str(), process_whoami.length()+1);
    txp::AttrPtr_char_array instance(txp::instance, process_instance.c_str(), process_instance.length()+1);
    msg->addAttribute(&whoami);
    msg->addAttribute(&instance);
    uint32_t contribid = UNDEFINED_CONTRIBID;
    msg->addAttribute(txp::contribid, contribid);
    msg->addAttribute(txp::version, BBAPI_CLIENTVERSIONSTR, strlen(BBAPI_CLIENTVERSIONSTR)+1);

    // Send the message to bbserver
    rc=sendMessage(name, msg, resp);
    delete msg;
    msg=NULL;
    if (rc)
    {
        stringstream errorText;
        errorText << "sendMessage to server failed";
        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
    }

    waitReply(resp, msg);
    rc = ((txp::Attr_int32*)msg->retrieveAttrs()->at(txp::resultCode))->getData();
    if (rc) {
        stringstream errorText;
        errorText <<"CORAL_AUTHENICATE(SSL) failed contribid=" << contribid \
        << ", process_whoami=" << process_whoami.c_str() << ", process_instance=" << process_instance.c_str() \
        << ", msg#=" << msg->getMsgNumber() << ", rqstmsg#=" << msg->getRequestMsgNumber() << ", rc=" << rc;
        LOG_ERROR_TEXT_ERRNO(errorText, EINVAL);
        LOG_RC_AND_RAS(rc, bb.net.authfailed);
    }

    delete msg;
    return rc;
}
