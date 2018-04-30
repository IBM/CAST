/*******************************************************************************
 |    fshipmond.cc
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


//! \file  fshipmond.c
//! \brief fshipmond main execution


#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>

#include <exception>
#include <iostream>
#include <string>

#include <arpa/inet.h>

#include <boost/program_options.hpp>

#include <netinet/in.h>

#include <sys/mount.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "CnxSock.h"
#include "fshipmond_flightlog.h"
#include "logging.h"
#include "MonitorHandler.h"
#include "Msg.h"
#include "util.h"
//#include "Interface.h"

FL_SetSize(FLUtility, 16384);

namespace po = boost::program_options;


int fshipmond_GetVersion(size_t pSize, char* pVersion) {
    int l_RC = snprintf(pVersion, pSize, FSHIPMOND_VERSIONSTR);
    if (l_RC == sizeof(FSHIPMOND_VERSIONSTR)) {
        l_RC = 0;
    } else {
        l_RC = ENOSPC;
    }
    return l_RC;
}

int processArgs(int argc, char *argv[], txp::CnxSock* pSock) {
    int l_RC = 0;

    // Declare the supported options.
    po::options_description desc("fshipmond allowed options");
    desc.add_options()
        ("config,c", po::value<std::string>()->default_value("coral.cfg"), "Path to configuration file")
        ("help,h", "Display help message")
//      ("version,V", po::value<int>(), "Version information")
        ("version,V", "Version information")
    ;

    // First, populate options from the command line
    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        exit(-1);
    }

    // Next, parse/process any command line options that can be processed without
    // any parsed values from the configuration file.
    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    if (vm.count("version")) {
        char l_Version[64] = {'\0'};
        fshipmond_GetVersion(sizeof(l_Version), l_Version);
        printf("Compiled %s on %s @ %s, version %s \n", argv[0], __DATE__, __TIME__, l_Version);
        return 1;
    }

    // Parse/populate options from the configuration file
    if (curConfig.load(vm["config"].as<std::string>()))
    {
        std::cerr << "Error loading configuration\n";
        exit(-1);
    }

    try {
        po::notify(vm);
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        exit(-1);
    }

    config = curConfig.getTree();

    uint16_t localPort= config.get("fship.monitor.listenport", 5049);
    std::string localAddr = config.get("fship.monitor.listenIPv4","0.0.0.0");

    in_addr_t ip4local;
    in_addr_t ip4remote;
    memset(&ip4local, 0, sizeof(struct in_addr));
    memset(&ip4remote, 0, sizeof(struct in_addr));


    // Retrieve/set local port
    //getIPPort(config.get("fship.monitor.address", "0.0.0.0:5049"), localAddr, localPort);
    if (localAddr.length()) {
        if (!inet_pton(AF_INET, localAddr.c_str(), &ip4local)) {
            printf("Local IPV4 address is not in proper format. address=%s\n", localAddr.c_str());
            std::cout << desc << "\n";
            return -2;
        }
    }
    pSock->setLocalAddr(ip4local, localPort);

    // Set remote port
    pSock->setRemoteAddr(ip4remote, 0);

    std::string flightlog = config.get("fship.monitor.flightlog", "/tmp/fshipmond/");

    // Setup logging
    initializeLogging("fship.monitor.log", config);
    LOG(fshipmond,info) << "Starting fshipmond";
    LOG(fshipmond,info) << "Configuration file: " << curConfig.getPath();
    LOG(fshipmond,info) << "Flightlog location: " << flightlog;
    LOG(fshipmond,info) << "Local address: " << localAddr;
    LOG(fshipmond,info) << "Local port: " << localPort;

    // Setup flight log
    int mkdirRC = mkdir( flightlog.c_str() , S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (mkdirRC) {
        if (errno != EEXIST)   LOG(fshipmond,critical) << "mkdir "<< flightlog.c_str() << " errno="<<strerror(errno);
    }
    struct stat l_stat;
    int statRC = stat(flightlog.c_str(), &l_stat);
    if (statRC) {
        LOG(fshipmond,critical) << "flight log mkdir "<< flightlog.c_str() << " errno="<<strerror(errno);
    }
    else {
        if ( !S_ISDIR(l_stat.st_mode) ) LOG(fshipmond,critical) << flightlog.c_str()<< " is not a directory for flight log";
    }

    l_RC = FL_CreateAll(flightlog.c_str());
    if(l_RC) {
        LOG(fshipmond,error) << "Unable to initialize flightlog (" << flightlog << ") l_RC=" << l_RC;
        abort();
    }
    FL_SetSize(fl_fshipmondmain,1024);
    FL_SetName(fl_fshipmondmain, "fshipmond main processing");
    FL_Write6(fl_fshipmondmain, FD_STARTUP, "fshipmond main pid=%lld, uid=%lld gid=%lld ppid=%lld pgid=%lld sid=%lld", getpid(), getuid(),getgid(),getppid(),getpgid(getpid()),getsid(getpid()) );
    LOG(fshipmond,info)<<"Process info "<< "fshipmond main pid="<<getpid()<<" uid="<<getuid()<<" gid="<<getgid()<<" ppid="<<getppid()<<" pgid="<<getpgid(getpid())<<" sid="<<getsid(getpid());

    return l_RC;
}

int main(int argc, char *argv[]) {
    int l_RC = 0;

    if (getuid()) {
      printf("need to run as root \n");
      exit(-1);
    }
    
    fflush(stdout);
     //getInterfaceAddresses();

    txp::CnxSock uSock(AF_INET, SOCK_STREAM);
    l_RC = processArgs(argc, argv, &uSock);

    if (!l_RC) {
        l_RC = uSock.bindLocalPort();
        if (l_RC >= 0) {
            l_RC = 0;
            txp::ConnexPtr l_Cnxptr = &uSock;
            MonitorHandler mh(l_Cnxptr);
            mh.run();   //  Monitor for incoming messages
        } else {
            printf("%s l_RC=%d errno=%d(%s)\n", __PRETTY_FUNCTION__, l_RC, errno, strerror(errno));
            l_RC = -errno;
        }
    }

    if (!l_RC) {
        LOG(fshipmond,info)<<"fshipmond::main(): Ending normally";
    } else {
        // \todo - Need to work on the error codes...  A errno string is created, but may not always apply...
        LOG(fshipmond,error)<<"fshipmond::main(): Ending with rc=" << l_RC << ", " << strerror(std::abs(l_RC));
    }

    return (l_RC>0 ? -l_RC : l_RC);
};
