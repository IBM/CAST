/*******************************************************************************
 |    fshipd.cc
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

//! \file  fshipd.c
//! \brief fshipd main execution

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <exception>
#include <string>

#include <arpa/inet.h>

#include <boost/program_options.hpp>

#include <netinet/in.h>

#include <sys/mount.h>
#include <sys/poll.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>

#include "../../transport/include/RDMACMfship.h"
#include "CnxSock.h"
#include "MessageHandler.h"
#include "Msg.h"
#include "fshipcld.h"
#include "fshipd_flightlog.h"
#include "logging.h"
#include "util.h"

FL_SetSize(FLUtility, 16384);

namespace po = boost::program_options;

uint64_t fusemaxbuff = 65536;
uint32_t numRDMAchunks = 8;

int fshipd_GetVersion(size_t pSize, char *pVersion) {
  int l_RC = snprintf(pVersion, pSize, FSHIPD_VERSIONSTR);
  if (l_RC == sizeof(FSHIPD_VERSIONSTR)) {
    l_RC = 0;
  } else {
    l_RC = ENOSPC;
  }

  return l_RC;
}

int sayReady(txp::CnxSock *pSock) {
  ssize_t l_RC = 0;

  txp::Msg *l_Msg;
  txp::Msg::buildMsg(txp::CORAL_READY, l_Msg);
  l_RC = pSock->write(l_Msg);

  return (int)l_RC;
}

int sayReady(txp::RDMACMfship *pConnection) {
  ssize_t l_RC = 0;

  txp::Msg *l_Msg;
  txp::Msg::buildMsg(txp::CORAL_READY, l_Msg);
  l_RC = pConnection->write(l_Msg);

  return (int)l_RC;
}

int processArgs(int argc, char *argv[], txp::ConnexPtr &oConnexPtr) {

  int l_RC = 0;

  // Declare the supported options.
  po::options_description desc("fshipd allowed options");
  desc.add_options()("config,c", po::value<std::string>()->default_value(""),
                     "Path to configuration file")("help,h",
                                                   "Display help message")(
      "remoteAddress,r", po::value<std::string>()->default_value(""),
      "Remote address")
      //      ("version,V", po::value<int>(), "Version information")
      ("version,V", "Version information")(
          "mode,m", po::value<std::string>()->default_value(""),
          "mode of sockets or rdma");

  // Populate options from the command line
  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    exit(-1);
  }

  // Parse/process any command line options that can be processed without
  // any parsed values from the configuration file.
  if (vm.count("help")) {
    std::cout << desc << "\n";
    return 1;
  }

  if (vm.count("version")) {
    char l_Version[64] = {'\0'};
    fshipd_GetVersion(sizeof(l_Version), l_Version);
    printf("Compiled %s on %s @ %s, version %s \n", argv[0], __DATE__, __TIME__,
           l_Version);
    return 1;
  }

  // Parse/populate options from the configuration file
  std::string l_Config = vm["config"].as<std::string>();
  if (!l_Config.empty()) {
    if (curConfig.load(l_Config)) {
      std::cerr << "Error loading configuration\n";
      exit(-1);
    }
    config = curConfig.getTree();
  } else {
    std::cerr << "Configuration file must be specified on the command line or "
                 "as a program parameter\n";
    exit(-1);
  }

  // Parse/populate options from the configuration file
  std::string mode = vm["mode"].as<std::string>();

  try {
    po::notify(vm);
  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    exit(-1);
  }

  config = curConfig.getTree();

  // Setup logging
  initializeLogging("fship.server.log", config);
  LOG(fshipd, always) << "Starting fshipd via " << argv[0] << " compiled "
                      << __DATE__ << "@" << __TIME__
                      << ((1 == htonl(1)) ? " (BIG ENDIEN)"
                                          : " (little endien)");
  LOG(fshipd, always) << "Configuration file: " << curConfig.getPath();

  // Setup flight log
  std::string flightlogdir =
      config.get("fship.server.flightlog", "/tmp/fshipd/");
  const char &lastchar = flightlogdir.back();
  if (lastchar != '/')
    flightlogdir += "/";
  int mkdirRC =
      mkdir(flightlogdir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  if (mkdirRC) {
    if (errno != EEXIST)
      LOG(fshipd, critical) << "mkdir " << flightlogdir.c_str()
                            << " errno=" << strerror(errno);
  }
  std::string flightlog = flightlogdir + "fshipd-";
  flightlog += std::to_string((int)getpid());
  LOG(fshipd, always) << "(RENAME pending) Flightlog current directory name="
                      << flightlog;
  mkdirRC = mkdir(flightlog.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  if (mkdirRC) {
    if (errno != EEXIST)
      LOG(fshipd, critical) << "mkdir " << flightlog.c_str()
                            << " errno=" << strerror(errno);
  }
  struct stat l_stat;
  int statRC = stat(flightlog.c_str(), &l_stat);
  if (statRC) {
    LOG(fshipd, critical) << "flight log mkdir " << flightlog.c_str()
                          << " errno=" << strerror(errno);
  } else {
    if (!S_ISDIR(l_stat.st_mode))
      LOG(fshipd, critical) << flightlog.c_str()
                            << " is not a directory for flight log";
  }

  l_RC = FL_CreateAll(flightlog.c_str());
  if (l_RC) {
    LOG(fshipd, error) << "Unable to initialize flightlog (" << flightlog
                       << ") l_RC=" << l_RC;
    abort();
  }
  FL_SetSize(fl_fshipdmain, 1024);
  FL_SetName(fl_fshipdmain, "fshipd main processing");
  FL_SetSize(fl_fshipdtx, 16384);
  FL_SetName(fl_fshipdtx, "fshipd transactions");
  /* clang-format off */ 
    FL_Write6(fl_fshipdmain, FD_STARTUP, "fshipd main pid=%lld, uid=%lld gid=%lld ppid=%lld pgid=%lld sid=%lld", getpid(), getuid(),getgid(),getppid(),getpgid(getpid()),getsid(getpid()) );
  /* clang-format on */
  /* clang-format off */ 
    FL_Write6(fl_fshiptx, TX_STARTUP, "fshipd main pid=%lld, uid=%lld gid=%lld ppid=%lld pgid=%lld sid=%lld", getpid(), getuid(),getgid(),getppid(),getpgid(getpid()),getsid(getpid()) );
  /* clang-format on */
  LOG(fshipd, always) << "Process info "
                      << "fshipd main pid=" << getpid() << " uid=" << getuid()
                      << " gid=" << getgid() << " ppid=" << getppid()
                      << " pgid=" << getpgid(getpid())
                      << " sid=" << getsid(getpid());

  struct rlimit rlimit_infinity = {.rlim_cur = RLIM_INFINITY,
                                   .rlim_max = RLIM_INFINITY};
  int rlimitRC = setrlimit(RLIMIT_CORE, &rlimit_infinity);
  if (rlimitRC)
    printf("for core rlmiitRC=%d \n", rlimitRC);
  rlimitRC = setrlimit(RLIMIT_MEMLOCK, &rlimit_infinity);
  if (rlimitRC)
    printf("for memlock rlmiitRC=%d \n", rlimitRC);
  rlimitRC =
      setrlimit(RLIMIT_NOFILE, &rlimit_infinity); // unlimited file descriptors
  if (rlimitRC)
    printf("for file descriptors rlmiitRC=%d \n", rlimitRC);

  fusemaxbuff = config.get("fship.fusemaxbuff", 65536);
  numRDMAchunks = config.get("fship.numRDMAchunks", 8);
  LOG(fshipd, always) << "fusemaxbuff:0x" << std::hex << fusemaxbuff << std::dec
                      << " decimal=" << fusemaxbuff
                      << " numRDMAchunks=" << numRDMAchunks;

  uint16_t localPort;
  uint16_t remotePort = 0;
  std::string localAddr;
  std::string remoteAddr;
  in_addr_t ip4local;
  in_addr_t ip4remote;
  memset(&ip4local, 0, sizeof(struct in_addr));
  memset(&ip4remote, 0, sizeof(struct in_addr));

  if (mode.empty()) {
    mode = config.get("fship.mode", "sockets");
  } else {
    LOG(fshipd, always) << "override mode=" << mode;
  }

  std::string l_Interface = config.get("fship.server.interface", "ib0");
  LOG(fshipd, always) << "Mode: " << mode << ", Interface: " << l_Interface;
  std::string l_RemoteAddr = vm["remoteAddress"].as<std::string>();
  if (!mode.compare("rdma")) {
    //  RDMA connection
    txp::RDMACMfship *uSockPtr =
        new txp::RDMACMfship(AF_INET, SOCK_RDM, fusemaxbuff, numRDMAchunks);
    LOG(fshipd, always) << "Adjusted values--fusemaxbuff:0x" << std::hex
                        << fusemaxbuff << std::dec << " decimal=" << fusemaxbuff
                        << " numRDMAchunks=" << numRDMAchunks;
    oConnexPtr = uSockPtr;

    // Retrieve/set local address/port
    if (!l_RemoteAddr.empty()) {
      std::string flightlogdirRename = flightlogdir + "fshipd-" + l_RemoteAddr;
      rename(flightlog.c_str(), flightlogdirRename.c_str());
      LOG(fshipd, always) << "(RENAMED) Flightlog directory name="
                          << flightlogdirRename;
      // Remote address was passed...  Use the interface in the configuration to
      // remote dial the client address...
      // First, determine the local address
      l_RC = getIPAddrByInterface(l_Interface, localAddr);
      if (!l_RC) {
        if (localAddr.length()) {
          if (!inet_pton(AF_INET, localAddr.c_str(), &ip4local)) {
            LOG(fshipd, error)
                << "Local IPV4 address is not in proper format. address="
                << localAddr.c_str();
            std::cout << desc << "\n";
            return 4;
          }
        }
        // Use generic port for connection that will be used with fshipd
        uSockPtr->setLocalAddr(ip4local, 0);
      } else {
        LOG(fshipd, error) << "Failure when attemping to get the local IP "
                              "address for interface '"
                           << l_Interface << "', l_RC=" << l_RC << " ("
                           << strerror(-l_RC) << ")";
      }
    } else {
      // Remote address was not passed...  fshipd should listen on address/port
      // in the configuration
      // Retrieve/set local address/port
      getIPPort(config.get("fship.server.address", "127.0.0.1:5050"), localAddr,
                localPort);
      LOG(fshipd, always) << "Local address: " << localAddr
                          << " Local Port: " << localPort;
      if (localAddr.length()) {
        if (!inet_pton(AF_INET, localAddr.c_str(), &ip4local)) {
          printf("Local IPV4 address is not in proper format. address=%s\n",
                 localAddr.c_str());
          std::cout << desc << "\n";
          return 2;
        }
      }
      uSockPtr->setLocalAddr(ip4local, localPort);
    }

    if (!l_RC) {
      // Retrieve/set remote port...
      // NOTE: fshipmond passes --remoteAddress for this instance
      //       of fshipd to dial remotely.  If remoteAddress is not
      //       set, then this is a test environment where fshipcld is
      //       directly dialing fshipd.   @DLH
      if (!l_RemoteAddr.empty()) {
        getIPPort(l_RemoteAddr, remoteAddr, remotePort);
      }
      if (remoteAddr.length()) {
        if (!inet_pton(AF_INET, remoteAddr.c_str(), &ip4remote)) {
          LOG(fshipd, error)
              << "Remote IPV4 address is not in proper format. address="
              << remoteAddr.c_str();
          std::cout << desc << "\n";
          return 3;
        }
        LOG(fshipd, always) << "Remote address: "
                            << ((!l_RemoteAddr.empty()) ? remoteAddr : "None")
                            << "  Remote port: " << remotePort;

        uSockPtr->willDialRemote();
      }
      uSockPtr->setRemoteAddr(ip4remote, remotePort);

      l_RC = uSockPtr->attachRemote();
      if (l_RC >= 0) {
        LOG(fshipd, always) << "attachRemote fd=" << l_RC;
        l_RC = 0;
        if (uSockPtr->dialRemote()) {
          // If fshipd dialed fshipcld, then
          // it owes a 'Ready' message to fshipcld.
          // fshipcld will NOT reply to the 'Ready'
          // message, but rather, will send it's own
          // 'Hello' message that will identify itself
          // to this instance of fshipd.
          l_RC = sayReady(uSockPtr);
          if (l_RC >= 0) {
            l_RC = 0;
          } else {
            LOG(fshipd, always) << __PRETTY_FUNCTION__ << " l_RC=" << l_RC;
          }
        }
      } else {
        LOG(fshipd, always) << __PRETTY_FUNCTION__ << " l_RC=" << abs(l_RC)
                            << ":" << strerror(abs(l_RC));
        return l_RC;
      }
    }
  } else {

    //  Socket connection
    txp::CnxSock *uSockPtr = new txp::CnxSock(AF_INET, SOCK_STREAM);
    oConnexPtr = uSockPtr;

    if (!l_RemoteAddr.empty()) {
      // Remote address was passed...  Use the interface in the configuration to
      // remote dial the address...
      // First, determine local address
      std::string flightlogdirRename = flightlogdir + "fshipd-" + l_RemoteAddr;
      rename(flightlog.c_str(), flightlogdirRename.c_str());
      LOG(fshipd, always) << "(RENAMED) Flightlog directory name="
                          << flightlogdirRename;
      l_RC = getIPAddrByInterface(l_Interface, localAddr);
      if (!l_RC) {
        if (localAddr.length()) {
          if (!inet_pton(AF_INET, localAddr.c_str(), &ip4local)) {
            LOG(fshipd, error)
                << "Local IPV4 address is not in proper format. address="
                << localAddr.c_str();
            std::cout << desc << "\n";
            return 4;
          }
        }
        // Use generic port for connection that will be used with fshipd
        uSockPtr->setLocalAddr(ip4local, 0);
      } else {
        LOG(fshipd, error) << "Failure when attemping to get the local IP "
                              "address for interface '"
                           << l_Interface << "', l_RC=" << l_RC << " ("
                           << strerror(-l_RC) << ")";
      }
    } else {
      // Remote address was not passed...  fshipd should listen on address/port
      // in the configuration
      // Retrieve/set local address/port
      getIPPort(config.get("fship.server.address", "127.0.0.1:5050"), localAddr,
                localPort);
      LOG(fshipd, always) << "Local address: " << localAddr
                          << " Local Port: " << localPort;
      if (localAddr.length()) {
        if (!inet_pton(AF_INET, localAddr.c_str(), &ip4local)) {
          printf("Local IPV4 address is not in proper format. address=%s\n",
                 localAddr.c_str());
          std::cout << desc << "\n";
          return 2;
        }
      }
      uSockPtr->setLocalAddr(ip4local, localPort);
    }

    if (!l_RC) {

      // Retrieve/set remote port...
      // NOTE: fshipmond passes --remoteAddress for this instance
      //       of fshipd to dial remotely.  If remoteAddress is not
      //       set, then this is a test environment where fshipcld is
      //       directly dialing fshipd.   @DLH
      if (!l_RemoteAddr.empty()) {
        getIPPort(l_RemoteAddr, remoteAddr, remotePort);
      }
      if (remoteAddr.length()) {
        if (!inet_pton(AF_INET, remoteAddr.c_str(), &ip4remote)) {
          printf("Remote IPV4 address is not in proper format. address=%s\n",
                 remoteAddr.c_str());
          std::cout << desc << "\n";
          return 3;
        }
        LOG(fshipd, always) << "Remote address: "
                            << (vm.count("remoteAddress") ? remoteAddr : "None")
                            << "  Remote port: " << remotePort;

        uSockPtr->willDialRemote();
      }
      uSockPtr->setRemoteAddr(ip4remote, remotePort);

      l_RC = uSockPtr->attachRemote();
      if (l_RC >= 0) {
        LOG(fshipd, always) << "attachRemote fd=" << l_RC;
        l_RC = 0;
        if (uSockPtr->dialRemote()) {
          // If fshipd dialed fshipcld, then
          // it owes a 'Ready' message to fshipcld.
          // fshipcld will NOT reply to the 'Ready'
          // message, but rather, will send it's own
          // 'Hello' message that will identify itself
          // to this instance of fshipd.
          l_RC = sayReady(uSockPtr);
          if (l_RC >= 0) {
            l_RC = 0;
          } else {
            LOG(fshipd, always) << __PRETTY_FUNCTION__ << " l_RC=" << l_RC;
          }
        }
      } else {
        LOG(fshipd, always) << __PRETTY_FUNCTION__ << " l_RC=" << abs(l_RC)
                            << ":" << strerror(abs(l_RC));
        return l_RC;
      }
    }
  }

  return l_RC;
}

int main(int argc, char *argv[]) {
  int l_RC = 0;

  if (getuid()) {
    printf("need to run as root \n");
    exit(-1);
  }

  /**  \TODO signal handlers
   **  \TODO loop to handle read from pipe
             args to pass
   **/
  signal(SIGCHLD, SIG_IGN);
  signal(SIGHUP, SIG_IGN);

  fflush(stdout);

  txp::ConnexPtr l_CnxPtr = NULL;

  l_RC = processArgs(argc, argv, l_CnxPtr);
  //    LOG(fshipd,always) << "processArgs return code=" << l_RC;

  if (!l_RC) {
    MessageHandler mh(l_CnxPtr);
    mh.run(config.get("fship.server.numthreads",
                      1)); // monitor for incoming messages
  } else {
    printf("%s l_RC=%d errno=%d(%s)\n", __PRETTY_FUNCTION__, l_RC, errno,
           strerror(errno));
    l_RC = -errno;
  }

  if (!l_RC) {
    LOG(fshipd, info) << "fshipd::main(): Ending normally";
  } else {
    // \todo - Need to work on the error codes...  A errno string is created,
    // but may not always apply...
    LOG(fshipd, error) << "fshipd::main(): Ending with rc=" << l_RC << ", "
                       << strerror(std::abs(l_RC));
  }

  return (l_RC > 0 ? -l_RC : l_RC);
};
