/*******************************************************************************
 |    serial.cc
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


#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>

#if BBSERVER
#include "bbserver_flightlog.h"
#define NAME "bbServer"
#endif

#if BBPROXY
#include "bbproxy_flightlog.h"
#define NAME "bbProxy"
int USE_NVMF=1; //! \TODO remove after kernel fix for nvmf target/initiator on same system
void set_use_NVMF(int pValue){USE_NVMF=pValue;} 
#endif

using namespace std;
namespace bfs = boost::filesystem;

#include "bbinternal.h"

static map<string, string>               DriveBySerial;
static vector<string>                    SerialOrder;
static map<string, string>               SerialByDrive;
static vector<string>                    nvme_devices;
static map<string, map<string, string> > nvmeDeviceInfo;
static map<string, string>               KeyByHostname;
static pthread_once_t  findSerialInit  = PTHREAD_ONCE_INIT;
static pthread_mutex_t findSerialMutex = PTHREAD_MUTEX_INITIALIZER;

static int genSerialByDrive()
{
    SerialByDrive.clear();
    for(const auto& serial : SerialOrder)
    {
        if(DriveBySerial.find(serial) != DriveBySerial.end())
        {
            SerialByDrive[DriveBySerial[serial]] = serial;
        }
    }
    return 0;
}

static string bb_nvmecliPath;
string get_bb_nvmecliPath(){return bb_nvmecliPath;}

int set_bb_nvmecliPath(const string& executable)
{
    LOG(bb,always)  <<  "nvme executable path =" << executable;
    int rc = access(executable.c_str(), X_OK);
    if (rc) {
        rc = errno;
        stringstream errorText;
        errorText << "Access failed to fully qualified nvme executable path for X_OK";
        bberror << err("error.executable", executable) << err("error.config","bb.nvmecliPath");
        LOG_ERROR_TEXT_ERRNO_AND_RAS(errorText, rc, bb.cfgerr.noNVMeCLI);
    }
    bb_nvmecliPath = executable;

    return rc;
}

static string bb_nvmfConnectPath;

int set_bb_nvmfConnectPath(const string& executable)
{
    LOG(bb,always)  <<  "nvmef executable script/bin path =" << executable;
    int rc = access(executable.c_str(), X_OK);
    if (rc) {
        rc = errno;
        stringstream errorText;
        errorText << "Access failed to fully qualified nvmf script/bin executable path for X_OK";
        bberror << err("error.executable", executable) << err("error.config","bb.nvmfConnectPath");
        LOG_ERROR_TEXT_ERRNO_AND_RAS(errorText, 0, bb.cfgerr.noconnectscript);
    }
    else {
        bb_nvmfConnectPath = executable;
    }
    
    return rc;
}

#if BBSERVER
void look4iSCSIinitiator(){
    LOG(bb, debug) << "Searching for iSCSI initiator devices";

    try{
        string target;
        for(const auto& line : runCommand("iscsiadm -m session -o show -P 3"))
            {
            if(line.find("Target:") != string::npos)
            {
                    vector<string> values = buildTokens(line, " ");
                target = values[1];
            }
            if(line.find("Attached scsi disk") != string::npos)
            {
                    vector<string> values = buildTokens(line, " \t");
                string drive = values[3];
                    SerialOrder.push_back(target);
                    DriveBySerial[target] = string("/dev/") + drive;
                    LOG(bb, info) << "Found iSCSI device (" << target << ") associated with /dev/" << drive;
            }
        }
    }//end try block
    catch(exception& e){
        int rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
        throw;
    }
 
}
#endif
#if BBSERVER
void look4NVMFinitiator(){
    LOG(bb, debug) << "Searching for NVMe over Fabrics initiator block devices";
    string cmd;
    try{       
        for(auto device: nvme_devices)
        {
            if(nvmeDeviceInfo[device]["mn"] == "Linux")
            {
                // NVMe over Fabrics device
                string nvmet_pseudoserial;
                vector<string> files = {"subsysnqn", "transport"};
                for(unsigned int index=0; index<files.size(); index++)
                {
                cmd = string("/sys/block/") + device.substr(5) + string("/device/") + files[index];
                for(auto line : runCommand(cmd, true))
                {
                    if(files[index] == "transport")
                    {
                    if(line == string("loop"))
                    {
                    }
                    else if(line == string("rdma"))
                    {
                        files.push_back("address");
                    }
                    }
                    if(files[index] == "address")
                    {
                    line.erase(line.find("traddr="), 7);
                    line.erase(line.find("trsvcid="), 8);
                    }
                    nvmet_pseudoserial += ((nvmet_pseudoserial != "") ? ",": "") + line;
                }
                }
                nvmeDeviceInfo[device]["pseudosn"]                = nvmet_pseudoserial;
                    SerialOrder.push_back(nvmet_pseudoserial);
                DriveBySerial[nvmeDeviceInfo[device]["pseudosn"]] = device;
                LOG(bb,info) << "NVMe device " << device << " : pseudosn=" << nvmet_pseudoserial;
            }
        }
    }//end try block
    catch(exception& e){
        int rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
        throw;
    }
}
#endif
#if BBPROXY
void look4iSCSItargetDevices(){
    LOG(bb, debug) << "Searching for iSCSI target devices";
    try{
        string target;
        for(auto line : runCommand("tgtadm --lld iscsi --mode target --op show"))
        {
            if(line.find("Target ") != string::npos)
                {
                    vector<string> values = buildTokens(line, " \t");
                    target = values[2];
                }
            if(line.find("Backing store path:") != string::npos)
            {
                    vector<string> values = buildTokens(line, " \t");
                    string drive = values[3];
                    SerialOrder.push_back(target);
                    DriveBySerial[target]  = drive;
                    LOG(bb, info) << "Found iSCSI drive: " << target << " associated with " << drive;
            }
        }
    }//end try block
    catch(exception& e){
        int rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
        throw;
    }   
}       
#endif
#if BBPROXY
void look4NVMFtargetDevices(){
    LOG(bb, debug) << "Searching for NVMe over Fabrics target block devices";
    string cmd;
    try{    
        if (USE_NVMF) // \TODO remove in an updated
        for(auto device: nvme_devices)
        {
            if (nvmeDeviceInfo[device]["mn"] != "Linux")
            {
                // This is a potential target device for NVMe over Fabrics.
                // If NVMe over Fabrics target is configured, fabricate a serial number using connectivity info.
                for(auto& hostpath : boost::make_iterator_range(bfs::directory_iterator(bfs::path("/sys/kernel/config/nvmet/hosts")), {}))
                {
                    vector<string> values = buildTokens(hostpath.path().filename().string(), "#");
                    KeyByHostname[values[0]] = hostpath.path().filename().string();
                    const string bbserverConst = "bb.server";
                    string temp = values[0].substr (bbserverConst.length());
                    size_t colonSpot = temp.find (":");
                    temp = temp.substr(0, colonSpot );
                    vector<string> valuesDot = buildTokens(temp, "_");
                    temp = valuesDot[0]
                          + "." + valuesDot[1]
                          + "." + valuesDot[2]
                          + "." + valuesDot[3]
                          ;
                    KeyByHostname[temp] = hostpath.path().filename().string();
                }

                cmd = string("grep -l ") + device + string(" /sys/kernel/config/nvmet/ports/*/subsystems/*/namespaces/*/device_path");
                for(auto line : runCommand(cmd))
                {
                    vector<string> values = buildTokens(line, "/");

                    string port = values[5];
                    string subsysnqn = values[7];
                    string nvmenamespace = values[9];
                    string nvmet_pseudoserial = subsysnqn;
                    vector<string> files = {"addr_trtype"};
                    for(unsigned int index = 0; index < files.size(); index++)
                    {
                        cmd = string("/sys/kernel/config/nvmet/ports/") + port + string("/") + files[index];
                        for(auto line : runCommand(cmd, true))
                        {
                            if(files[index] == "addr_trtype")
                            {
                                if(line == string("loop"))
                                {
                                }
                                else if(line == string("rdma"))
                                {
                                    files.push_back("addr_traddr");
                                    files.push_back("addr_trsvcid");
                                }
                            }
                            nvmet_pseudoserial = nvmet_pseudoserial + "," + line;
                        }
                    }
                    LOG(bb,info) << "NVMe device " << device << " : port=" << port << "  subsystem=" << subsysnqn << "   namespace=" << nvmenamespace << "   pseudosn=" << nvmet_pseudoserial;

                    nvmeDeviceInfo[device]["pseudosn"] = nvmet_pseudoserial;
                    SerialOrder.push_back(nvmeDeviceInfo[device]["pseudosn"]);
                    DriveBySerial[nvmeDeviceInfo[device]["pseudosn"]] = device;
                }
            }
        }
    }//end try block
    catch(exception& e){
        int rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
        throw;
    }   
}       
    #endif

void findSerials(void)
{
    map<string, bool> validDrives;
    string cmd;
    pthread_mutex_lock(&findSerialMutex);

    try{
        // clear structures
        DriveBySerial.clear();
        SerialOrder.clear();
        SerialByDrive.clear();
        nvme_devices.clear();
        nvmeDeviceInfo.clear();
        KeyByHostname.clear();
        
        // find valid drives
        cmd = config.get("bb.lsblkPath", "lsblk") + " -n --nodeps -o name,type,serial";
        for(auto line : runCommand(cmd))
        {
            vector<string> tok = buildTokens(line, " ");
            LOG(bb,info) << "Device " << tok[0] << " found, type=" << tok[1];
            if(tok[1] == "disk")
            {
                validDrives[string("/dev/") + tok[0]] = true;
                if(tok.size() > 2)  // lsblk is also providing the serial number.  (3.10 kernels may not provide NVMe serials on lsblk)
                {
                    SerialOrder.push_back(tok[2]);
                    DriveBySerial[tok[2]] = string("/dev/") + tok[0];
                    LOG(bb,info) << "Device " << tok[0] << " has serial '" << tok[2] << "'";
                }
            }
        }


        if (bb_nvmecliPath.size()) //nvme command is available?
        {
                // obtain list of NVMe devices (both PCIe-attached or RDMA-attached)
            cmd = bb_nvmecliPath + " list 2>&1; echo rc=$?";
            for(auto line : runCommand(cmd,false,false))//run cmd as flatfile = false, bool noException=false
            {
            vector<string> tok = buildTokens(line, " ");
            if(tok[0].at(0) != '/') continue;
            if(validDrives.find(tok[0]) == validDrives.end())
            {
                LOG(bb,info) << "NVMe device '" << tok[0] << "', but does not appear to be a block device";
                continue;
            }
            LOG(bb,info) << "NVMe device found: '" << tok[0] << "'";
            nvme_devices.push_back(tok[0]);
            }

            // obtain details on each NVMe device
            for(auto device: nvme_devices)
            {
                cmd = bb_nvmecliPath + " id-ctrl " + device;
                for(auto line : runCommand(cmd,false,false)) //run cmd as flatfile = false, bool noException=false
                {
                    if(line.find(": ") == string::npos) continue;   // \todo:  switch to nvme-cli's json format  --output=json

                    string name  = line.substr(0,line.find(" "));
                    string value = line.substr(line.find(":")+2, line.find_last_not_of(" \t") - line.find(":")-1);
                    LOG(bb,info) << "NVMe device " << device << " : " << name << " = " << value;
                    nvmeDeviceInfo[device][name] = value;
                }

                // This is an NVMe over Fabrics device.  Fabricate a serial number using connectivity info.
                if(nvmeDeviceInfo[device]["mn"] != "Linux")
                {
                    LOG(bb,info) << "device:" << device << "   sn='" << nvmeDeviceInfo[device]["sn"] << "'";
                    SerialOrder.push_back(nvmeDeviceInfo[device]["sn"]);
                    DriveBySerial[nvmeDeviceInfo[device]["sn"]] = device;
                }
            }
        }
        else
        {
        LOG(bb,info) << "No valid nvme command to do nvme list. Reference the configuration for bb.nvmecliPath.";
        }

    #if BBSERVER
        look4iSCSIinitiator();
        look4NVMFinitiator();
    #endif

    #if BBPROXY
        look4iSCSItargetDevices();
        look4NVMFtargetDevices();
    #endif
        genSerialByDrive();
        pthread_mutex_unlock(&findSerialMutex);
    }//end try block
    catch(exception& e){
        pthread_mutex_unlock(&findSerialMutex);
        int rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
        throw;
    }
}

bool foundDeviceBySerial(const string& serial)
{
    bool foundIt=0;
    pthread_once(&findSerialInit, findSerials);
    pthread_mutex_lock(&findSerialMutex);
    try{
      foundIt=(DriveBySerial.find(serial) != DriveBySerial.end());
      pthread_mutex_unlock(&findSerialMutex);
    }//end try block
    catch(exception& e){
        pthread_mutex_unlock(&findSerialMutex);
        int rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
        throw;
    }
    return foundIt;
}

// handle nvmf connect for pseudo serial such as serial: burstbuffer,rdma,20.7.6.27,1023
int nvmfConnectPath(const string& serial, const string& connectionKey)
{
    if (!bb_nvmfConnectPath.size()) //check for 0 length string
    {
        //Already did RAS, automatic kickback
        return -1;
    }
    vector<string> ele = buildTokens(serial, ",");
    
    if(ele.size() != 4) 
        return -1;
    string nameSpace=ele[0];
    string network = ele[1];
    string port = ele[3];
    string ipAddr = ele[2];
    vector<string> ipv4_ele = buildTokens(ipAddr,".");
    
    size_t keyenvsize = 16 + connectionKey.size();
    char*  keyenv     = (char*)malloc(keyenvsize);
    snprintf(keyenv, keyenvsize, "NVMEKEY=%s", connectionKey.c_str());
    putenv(keyenv);

    string cmd =  bb_nvmfConnectPath  +" "+network+" " + nameSpace+" "+ipAddr+" "+ port +" 2>&1; echo  rc=$?;";
    LOG(bb,info) << " cmd=" << cmd;
    bool success = false;
    stringstream errorText;
    for(auto line : runCommand(cmd))
    {
        errorText << line;
        LOG(bb,info) << "output: " << line;
        if(line == "rc=0")
        {
            success = true;
        }
    }
    if(!success)
    {
        bberror << err("error.executable",bb_nvmfConnectPath );
        LOG_ERROR_TEXT_ERRNO_AND_RAS(errorText, 0, bb.net.nvmfConnectFail);
        return -1;
    }
    findSerials();
    int whileCount=0;
    while ( !foundDeviceBySerial(serial) )
    {
        whileCount++;
        if (whileCount>60)break;
        sleep(1);
        findSerials();
    }
    if (whileCount) LOG(bb,always)  << " whileCount="<<whileCount<< "in nvmfConnectPath ";
    if (!foundDeviceBySerial(serial) ){
        LOG(bb,always)  <<  "poll discovery of serial NOT found in list, serial="<<serial;
        errorText << "poll discovery of serial NOT found in list, serial="<<serial;
        LOG_ERROR_TEXT_ERRNO_AND_RAS(errorText, 0, bb.net.noNVMfDevices);
    }

    return 0;
}

string getDeviceBySerial(string serial)
{
    pthread_once(&findSerialInit, findSerials);
    
    pthread_mutex_lock(&findSerialMutex);
    try{
        if(DriveBySerial.find(serial) == DriveBySerial.end())
        {
            throw runtime_error(string("unable to getDeviceBySerial(\"") + serial + "\")");
        }
        auto tmp = DriveBySerial[serial];

        struct stat l_stat;
        int stat_rc = stat(tmp.c_str() , &l_stat);
        if (stat_rc){
            //remove device and then throw
            DriveBySerial.erase(serial);
            SerialByDrive.erase(tmp);
            throw runtime_error(string("unable to stat getDeviceBySerial(\"") + serial + "\") " + strerror(errno) + " drive: " + tmp.c_str() ) ;
        }
        pthread_mutex_unlock(&findSerialMutex);
        return tmp;
    }//end try block
    catch(runtime_error& e){  
        pthread_mutex_unlock(&findSerialMutex);
        LOG(bb,info) << "Exception caught "<<__func__<<"@"<<__FILE__<<":"<<__LINE__<<" what="<<e.what();
        throw;
    }  
    catch(exception& e){
        pthread_mutex_unlock(&findSerialMutex);
        int rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
        throw;
    }
}

string getSerialByDevice(string device)
{
    pthread_once(&findSerialInit, findSerials);
    
    pthread_mutex_lock(&findSerialMutex);
    try{
        if(SerialByDrive.find(device) == SerialByDrive.end())
        {          
            throw runtime_error(string("unable to getSerialByDevice(\"") + device + "\")");
        }
        auto tmp = SerialByDrive[device];
        pthread_mutex_unlock(&findSerialMutex);
        return tmp;
    }//end try block  
    catch(runtime_error& e){  
        pthread_mutex_unlock(&findSerialMutex);
        LOG(bb,info) << "Exception caught "<<__func__<<"@"<<__FILE__<<":"<<__LINE__<<" what="<<e.what();
        throw;
    }
    catch(exception& e){
        pthread_mutex_unlock(&findSerialMutex);
        int rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
        throw;
    }
}

vector<string> getDeviceSerials()
{
    pthread_once(&findSerialInit, findSerials);
    
    vector<string> lst;
    pthread_mutex_lock(&findSerialMutex);
    try{
        for(const auto& e : DriveBySerial)
        {
            lst.push_back(e.first);
        }
        pthread_mutex_unlock(&findSerialMutex);
        return lst;
    }//end try block    
    catch(exception& e){
        pthread_mutex_unlock(&findSerialMutex);
        int rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
        throw;
    }
}

int removeDeviceSerial(string serial)
{
    pthread_once(&findSerialInit, findSerials);
    
    pthread_mutex_lock(&findSerialMutex);
    try{
        DriveBySerial.erase(serial);
        if(DriveBySerial.empty())
        {
            stringstream errorText;
            errorText << "There are zero remaining viable devices between bbProxy and bbServer";
            bberror << err("error.serial.num", "0");
            LOG_ERROR_TEXT_RC_AND_RAS(errorText, -1, bb.net.noViableDevices);
            throw runtime_error(string("There are zero remaining viable devices between bbProxy and bbServer"));
        }
        genSerialByDrive();
        pthread_mutex_unlock(&findSerialMutex);
        return 0;
    }//end try block   
    catch(runtime_error& e){  
        pthread_mutex_unlock(&findSerialMutex);
        LOG(bb,info) << "Exception caught "<<__func__<<"@"<<__FILE__<<":"<<__LINE__<<" what="<<e.what();
        throw;
    } 
    catch(exception& e){
        pthread_mutex_unlock(&findSerialMutex);
        int rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
        throw;
    }

}

string getNVMeByIndex(uint32_t index)
{
    pthread_once(&findSerialInit, findSerials);
    pthread_mutex_lock(&findSerialMutex);
    try{
        if(nvme_devices.size() <= index)
        {
            throw runtime_error(string("unable to getNVMeByIndex(") + to_string(index) + ")");
        }
        auto tmp = nvme_devices[index];
        pthread_mutex_unlock(&findSerialMutex);
        return tmp;
    }//end try block  
    catch(runtime_error& e){  
        pthread_mutex_unlock(&findSerialMutex);
        LOG(bb,info) << "Exception caught "<<__func__<<"@"<<__FILE__<<":"<<__LINE__<<" what="<<e.what();
        throw;
    }  
    catch(exception& e){
        pthread_mutex_unlock(&findSerialMutex);
        int rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
        throw;
    }
}

string getNVMeDeviceInfo(string device, string key)
{
    pthread_once(&findSerialInit, findSerials);
    
    pthread_mutex_lock(&findSerialMutex);
    try{
        auto tmp = nvmeDeviceInfo[device][key];
        pthread_mutex_unlock(&findSerialMutex);
        return tmp;
    }//end try block    
    catch(exception& e){
        pthread_mutex_unlock(&findSerialMutex);
        int rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
        throw;
    }
}

extern string getRemoteAddrString(const std::string& pConnectionName);
string getKeyByHostname(string hostname)
{
    pthread_once(&findSerialInit, findSerials);
    
    pthread_mutex_lock(&findSerialMutex);
    try
    {
        auto tmp = KeyByHostname[hostname];
        if (tmp.empty()) {
            string l_string = getRemoteAddrString(hostname);
            tmp=KeyByHostname[  l_string ];
        }
        pthread_mutex_unlock(&findSerialMutex);
        return tmp;
    }
    catch(exception& e)
    {
        pthread_mutex_unlock(&findSerialMutex);
        int rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
        throw;
    }
}
