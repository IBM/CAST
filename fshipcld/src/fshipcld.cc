/*******************************************************************************
 |    fshipcld.cc
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

//!
//! \file   fshipcld.cc
//! \author Mike Aho <meaho@us.ibm.com>
//! \date   Wed Nov  9 12:59:04 2016
//! 
//! \brief  File for main invocation of fshipcld daemon
//! 
//! 
//!

#include "fshipcld.h"
#include "CnxSock.h"
#include "Msg.h"
#include "TrackPath.h"
#include "fshipcld_flightlog.h"
#include "logging.h"
#include "util.h"
#include <boost/program_options.hpp>
#include <dirent.h>
#include <errno.h>
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/mount.h>
#include <sys/poll.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../../transport/include/RDMACMfship.h"
#include "../include/FshipFuseOpsHandler.h"
#include "../include/NodeName.h"

namespace po = boost::program_options;

uint64_t fusemaxbuff = 65536;

std::string localAddr;
std::string configPath;

//! \brief Set fshipcld version
//!
//! \param pSize 
//! \param pVersion 
//!
//! \return 
//!
int fshipcld_GetVersion(size_t pSize, char *pVersion) {
  int l_RC = snprintf(pVersion, pSize, FSHIPCLD_VERSIONSTR);
  if (l_RC == sizeof(FSHIPCLD_VERSIONSTR)) {
    l_RC = 0;
  } else {
    l_RC = ENOSPC;
  }

  return l_RC;
}
//! \brief
//!
//! \param inString 
//!
//! \return 
//!
int LogString4main(char *inString) {
  long long int data[6];
  data[5] = 0;
  strncpy((char *)data, inString, 47);
  FL_Write6(FL_fshipcldmain, FCL_PARMS, "argv[]=%p%p%p%p%p%p", data[0], data[1],data[2], data[3], data[4], data[5]);
  return strlen(inString);
}
//! \brief Send Hello message to fshipmond
//!
//! \param pSendSock 
//! \param pLocalAddr 
//! \param pMode 
//!
//! \return -errno for error; 0=success
//!
int sayHelloToMonitor(txp::CnxSock &pSendSock, char *pLocalAddr, char *pMode) {
  int l_RC = 0;

  TrackPathFreeMsg trackPath(txp::CORAL_HELLO_MONITOR);
  trackPath.addString(txp::data, pLocalAddr);
  trackPath.addString(txp::mode, pMode);
  if (!configPath.empty())
    trackPath.addString(txp::configPath, (char *)configPath.c_str());
  LOG(fshipcld, always) << "sayHelloToMonitor with Return address: "
                        << pLocalAddr << " mode=" << pMode;
  ssize_t retSSize = trackPath.sendMsg((txp::Connex *)&pSendSock);
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    l_RC = -errno;
  }

  return l_RC;
}
//! \brief Networking to fshipd via sockets
//!
//! \param pSock Sockets object
//!
//! \return 0=success; nonzero is an error
//!
int doNetworked(txp::CnxSock &pSock) {
  int l_RC = 0;

  if (pSock.useMonitor()) {
    // Remote dial fshipmond with a copy of pSock.
    // First, make a copy of pSock.
    txp::CnxSock l_Sock(pSock);

    // pSock will be used for the return connection
    pSock.willDialRemote(0);
    l_RC = pSock.bindAndListenLocalPort();

    if (l_RC >= 0) {
      LOG(fshipcld, always) << "fshipd socket fd=" << l_RC;
      l_RC = 0;

      // Connect to fshipmond...
      l_RC = l_Sock.attachRemote();
      if (l_RC >= 0) {
        l_RC = 0;
        // Say 'Hello' to fshipmond with l_Sock,
        // passing pSock's local address.  That
        // address will be passed on the execv()
        // of fshipd.
        char l_LocalAddr[64] = {'\0'};
        snprintf(l_LocalAddr, sizeof(l_LocalAddr), "%s:%d", localAddr.c_str(),
                 pSock.getLocalPort());
        l_RC = sayHelloToMonitor(l_Sock, l_LocalAddr, (char *)"sockets");
        if (!l_RC) {
          // Accept the connection and read
          // in the 'Ready' message from fshipd...
          l_RC = pSock.acceptRemote();
          if (l_RC) {
            LOG(fshipcld, error) << "Accept from fshipd failed, l_RC=" << l_RC
                                 << " (" << strerror(-l_RC) << ")";
          }
        } else {
          LOG(fshipcld, error)
              << "Communication with fshipmond failed, l_RC=" << l_RC << " ("
              << strerror(-l_RC) << ")";
        }
      } else {
        LOG(fshipcld, error)
            << "Attach remote to fshipmond failed, l_RC=" << l_RC << " ("
            << strerror(-l_RC) << ")";
      }
    } else {
      LOG(fshipcld, error) << "Bind and listen failed, l_RC=" << l_RC << " ("
                           << strerror(-l_RC) << ")";
    }

    // Close the socket used to communicate to fshipmond
    // NOTE:  Wait until now...  We are sure fshipmond is
    //        also finished with this connection...
    l_Sock.closefd();

  } else {
    //  Remote dial fshipd directly...
    l_RC = pSock.attachRemote();
    if (l_RC >= 0) {
      LOG(fshipcld, always) << "fshipd socket fd=" << l_RC;
      l_RC = 0;
    } else {
      LOG(fshipcld, error) << "Attach remote to fshipd failed, l_RC=" << l_RC
                           << " (" << strerror(-l_RC) << ")";
    }
  }

  return l_RC;
}
//! \brief  Networking with fshipd via RDMA CM 
//!
//! \param pConnection Object of RDMA CM characteristics and actions
//! \param remoteAddr  Remote ipv4 address of fshipmond
//! \param remotePort  Remote port of fshipmond
//!
//! \return 
//!
int doNetworked(txp::RDMACMfship &pConnection, std::string &remoteAddr,
                uint16_t remotePort) {
  int l_RC = 0;

  if (pConnection.useMonitor()) {
    // \todo - This leg needs to be worked....  @DLH
    // Remote dial fshipmond.  First, create the socket
    // connection that will be used to connect to fshipmond.

    txp::CnxSock l_Sock = txp::CnxSock(AF_INET, SOCK_STREAM);

    in_addr_t ip4local;
    in_addr_t ip4remote;
    memset(&ip4local, 0, sizeof(struct in_addr));
    memset(&ip4remote, 0, sizeof(struct in_addr));

    // Set the local address/port
    l_Sock.setLocalAddr(ip4local, 0);

    if (remoteAddr.length()) {
      if (!inet_pton(AF_INET, remoteAddr.c_str(), &ip4remote)) {
        LOG(fshipcld, error)
            << "Remote IPV4 address is not in proper format. address="
            << remoteAddr.c_str();
        return 3;
      }
    } else {
      assert(remoteAddr.length() != 0);
      abort();
    }
    l_Sock.setRemoteAddr(ip4remote, remotePort);

    // Remote dial fshipmond...
    l_Sock.willDialRemote();

    // pConnection will be used for the return connection
    pConnection.willDialRemote(0);
    l_RC = pConnection.bindAndListenLocalPort();

    if (!l_RC) {
      // Connect to fshipmond...
      l_RC = l_Sock.attachRemote();
      if (l_RC >= 0) {
        l_RC = 0;
        // Say 'Hello' to fshipmond with l_Sock,
        // passing pConnection's local address.  That
        // address will be passed on the execv()
        // of fshipd.
        char l_LocalAddr[64] = {'\0'};
        snprintf(l_LocalAddr, sizeof(l_LocalAddr), "%s:%d", localAddr.c_str(),
                 pConnection.getLocalListenerPort());
        l_RC = sayHelloToMonitor(l_Sock, l_LocalAddr, (char *)"rdma");
        if (!l_RC) {
          // Accept the connection and read
          // in the 'Ready' message from fshipd...
          l_RC = pConnection.acceptRemote();
          if (l_RC) {
            LOG(fshipcld, error) << "Accept from fshipd failed, l_RC=" << l_RC;
          }
        } else {
          LOG(fshipcld, error)
              << "Communication with fshipmond failed, l_RC=" << l_RC << " ("
              << strerror(-l_RC) << ")";
        }
      } else {
        LOG(fshipcld, error)
            << "Attach remote to fshipmond failed, l_RC=" << l_RC << " ("
            << strerror(-l_RC) << ")";
      }
    } else {
      LOG(fshipcld, error) << "prepareServer failed, l_RC=" << l_RC;
    }

  } else {
    //  Remote dial fshipd directly...
    l_RC = pConnection.attachRemote();
    if (l_RC >= 0) {
      LOG(fshipcld, always) << "fshipd socket fd=" << l_RC;
      l_RC = 0;
    } else {
      LOG(fshipcld, error) << "Attach remote to fshipd failed, l_RC=" << l_RC
                           << " (" << strerror(-l_RC) << ")";
    }
  }
  return l_RC;
}
//! \brief
//!
//! \param argc standard main argc
//! \param argv standard main argv
//! \param rCON Networking object
//!
//! \return 0=success; nonzero is an error
//!
int processArgs(int argc, char *argv[], FshipFuseOpsHandler &rCON) {
  int l_RC = 0;

  // Declare the supported options.
  po::options_description desc("fshipcld allowed options");
  desc.add_options()(
      "config,c", po::value<std::string>()->default_value("coral.cfg"),
      "Path to configuration file")("help,h", "Display help message")
      //      ("version,V", po::value<int>(), "Version information")
      ("version,V", "Version information");

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
    fshipcld_GetVersion(sizeof(l_Version), l_Version);
    printf("Compiled %s on %s @ %s, version %s \n", argv[0], __DATE__, __TIME__,
           l_Version);
    return 1;
  }

  // Parse/populate options from the configuration file
  if (curConfig.load(vm["config"].as<std::string>())) {
    std::cerr << "Error loading configuration\n";
    exit(-1);
  }

  try {
    po::notify(vm);
  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    exit(-1);
  }

  config = curConfig.getTree();

  // Setup logging
  initializeLogging("fship.client.log", config);
  LOG(fshipcld, always) << "Starting fshipcld via " << argv[0] << " compiled "
                        << __DATE__ << "@" << __TIME__
                        << ((1 == htonl(1)) ? " (BIG ENDIEN)"
                                            : " (little endien)");
  LOG(fshipcld, always) << "Configuration file: " << curConfig.getPath();
  uint64_t passConfigPath2Fshipmond =
      config.get("fship.client.passconfigpath", 0);
  if (passConfigPath2Fshipmond)
    configPath = curConfig.getPath();
  LOG(fshipcld, always) << "passConfigPath2Fshipmond="
                        << passConfigPath2Fshipmond
                        << " configPath=" << configPath;

  fusemaxbuff = config.get("fship.fusemaxbuff", 65536);
  // Retrieve the local mount point
  std::string localMountStr = config.get("fship.client.mountlocal", "");
  if (!localMountStr.length()) {
    LOG(fshipcld, always)
        << "The local mount point fship.client.mountlocal is not specified";
    exit(EINVAL);
  }
  char *localMount = realpath(localMountStr.c_str(), NULL);
  if (!localMount) {
    LOG(fshipcld, always) << localMountStr.c_str()<<" mount realpath of local mount errno=" << errno << ":"
                          << strerror(errno);
    if (errno != ENOTCONN)
      exit(errno);
    l_RC = rCON.mountDevice(fusemaxbuff, localMountStr.c_str());
  } else {
    if (isFileSystemMounted(localMount)) {
            LOG(fshipcld,error) << "Already mounted "<<localMount;
            free(localMount);
            return 2;
    }
    l_RC = rCON.mountDevice(fusemaxbuff, localMount);
    free(localMount);
  }
  localMount = NULL;

  if (l_RC) {
    if (l_RC) {
      LOG(fshipcld, error) << "mountDevice failed, l_RC=" << l_RC
                           << " mount=" << localMountStr.c_str();
    }
    exit(l_RC);
  }
  LOG(fshipcld, always) << " mount=" << localMountStr.c_str()
                        << "fusemaxbuff=0x" << std::hex << fusemaxbuff
                        << std::dec << " decimal=" << fusemaxbuff;

  // Retrieve the remote mount point
  std::string remoteMountStr = config.get("fship.client.mountremote", "");
  if (!remoteMountStr.length()) {
    LOG(fshipcld, always)
        << "The remote target fship.client.mountremote is not specified";
    exit(EINVAL);
  }

  const char * remoteMount = remoteMountStr.c_str();
  LOG(fshipcld, always) << "Remote target:" << remoteMount;
  rCON.createNodeNameRoot(remoteMount);

  uint32_t numRDMAchunks = config.get("fship.numRDMAchunks", 8);
  LOG(fshipcld, always) << " numRDMAchunks=" << numRDMAchunks;
  rCON.setNumRDMAchunks(numRDMAchunks);

  // Setup flight log
  std::string flightlogdir =
      config.get("fship.client.flightlog", "/tmp/fshipcld/");
  LOG(fshipcld, always) << "Flightlog dir location: " << flightlogdir;
  int mkdirRC =
      mkdir(flightlogdir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  if (mkdirRC) {
    if (errno != EEXIST)
      LOG(fshipcld, critical) << "mkdir " << flightlogdir.c_str()
                              << " errno=" << strerror(errno);
  }
  std::string flightlog = flightlogdir + "fshipcld-";
  flightlog += std::to_string((int)getpid());
  LOG(fshipd, always) << "Flightlog current directory name=" << flightlog;
  mkdirRC = mkdir(flightlog.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  if (mkdirRC) {
    if (errno != EEXIST)
      LOG(fshipd, critical) << "mkdir " << flightlog.c_str()
                            << " errno=" << strerror(errno);
  }

  struct stat l_stat;
  int statRC = stat(flightlog.c_str(), &l_stat);
  if (statRC) {
    LOG(fshipcld, critical) << "flight log mkdir " << flightlog.c_str()
                            << " errno=" << strerror(errno);
  } else {
    if (!S_ISDIR(l_stat.st_mode))
      LOG(fshipcld, critical) << flightlog.c_str()
                              << " is not a directory for flight log";
  }

  l_RC = FL_CreateAll(flightlog.c_str());
  if (l_RC) {
    LOG(fshipcld, error) << "Unable to initialize flightlog (" << flightlog
                         << ") l_RC=" << l_RC;
    assert(l_RC == 0);
    abort();
  }
  FL_SetSize(FL_fshipcldmain, 262144);
  FL_SetName(FL_fshipcldmain, "fshipcld main processing");
  FL_SetSize(FL_NodeName, 262144);
  FL_SetName(FL_NodeName, "inode-name tracking");
  FL_SetSize(FL_fshipcldfuseop, 262144);
  FL_SetName(FL_fshipcldfuseop, "fshipcld fuse op");
  FL_SetSize(FL_fshipcldreadop, 262144);
  FL_SetName(FL_fshipcldreadop, "fshipcld fuse read op");
  FL_SetSize(FL_fshipcldwriteop, 262144);
  FL_SetName(FL_fshipcldwriteop, "fshipcld fuse write op");
  FL_Write6(FL_fshipcldmain, FCL_STARTUP,"fshipcld main pid=%lld, uid=%lld gid=%lld ppid=%lld pgid=%lld sid=%lld",getpid(), getuid(), getgid(), getppid(), getpgid(getpid()), getsid(getpid()));
  for (int i = 0; i < argc; i++)
    LogString4main(argv[i]);

  struct rlimit rlimit_infinity = {.rlim_cur = RLIM_INFINITY,
                                   .rlim_max = RLIM_INFINITY};
  int rlimitRC = setrlimit(RLIMIT_CORE, &rlimit_infinity);
  if (rlimitRC)
    LOG(fshipcld, error) << "for core rlimitRC=" << rlimitRC;
  rlimitRC = setrlimit(RLIMIT_MEMLOCK, &rlimit_infinity);
  if (rlimitRC)
    LOG(fshipcld, error) << "for memlock rlimitRC=" << rlimitRC;

  int xattrVal = config.get("fship.client.xattruse",
                            0); // default is to not use xattr and reply ENOSYS
  LOG(fshipcld, always) << "fship.client.xattruse=" << xattrVal;
  set_doXattr(xattrVal);

  int directIOVal = config.get("fship.client.fusedirectio",
                               0); // default is to NOT use fusediectio
  LOG(fshipcld, always) << "fship.client.fusedirectio=" << directIOVal;
  set_fuseDirectIO(directIOVal);

  NodeNameRoot::entryValidsec = config.get("fship.client.entry_valid_sec",
                                           1); // dentry valid time in seconds
  NodeNameRoot::entryValidnsec =
      config.get("fship.client.entry_valid_nsec", 0); // plus in nanoseconds
  NodeNameRoot::attrValidsec = config.get(
      "fship.client.attr_valid_sec", 1); // attributes valid time in seconds
  NodeNameRoot::attrValidnsec =
      config.get("fship.client.attr_valid_nsec", 0); // plus in nanoseconds
  LOG(fshipcld, always) << "fship.client.entry_valid_sec="
                        << NodeNameRoot::entryValidsec
                        << " fship.client.entry_valid_nsec="
                        << NodeNameRoot::entryValidnsec;
  LOG(fshipcld, always) << "fship.client.attr_valid_sec="
                        << NodeNameRoot::attrValidsec
                        << " fship.client.attr_valid_nsec="
                        << NodeNameRoot::attrValidnsec;

  uint16_t remotePort = config.get("fship.monitor.listenport", 5049);
  std::string remoteAddr = config.get("fship.client.remoteIPv4", "0.0.0.0");

  txp::ConnexSocketPtr oConnexPtr;
  std::string mode = config.get("fship.mode", "sockets");
  std::string l_Interface = config.get("fship.client.interface", "ib0");
  if (!mode.compare("rdma")) {
    //  RDMA connection
    oConnexPtr =
        new txp::RDMACMfship(AF_INET, SOCK_RDM, fusemaxbuff, numRDMAchunks);
    LOG(fshipcld, always) << "Adjusted values--fusemaxbuff:0x" << std::hex
                          << fusemaxbuff << std::dec
                          << " decimal=" << fusemaxbuff
                          << " numRDMAchunks=" << numRDMAchunks;
  } else {
    //  Socket connection
    oConnexPtr = new txp::CnxSock(AF_INET, SOCK_STREAM);
  }
  rCON.setConnectPtr(oConnexPtr);

  LOG(fshipcld, always) << "Mode:" << mode;
  in_addr_t ip4local;
  in_addr_t ip4remote;
  memset(&ip4local, 0, sizeof(struct in_addr));
  memset(&ip4remote, 0, sizeof(struct in_addr));

  l_RC = getIPAddrByInterface(l_Interface, localAddr);
  if (!l_RC) {

    // Determine local address
    if (localAddr.length()) {
      if (!inet_pton(AF_INET, localAddr.c_str(), &ip4local)) {
        LOG(fshipcld, error)
            << "Local IPV4 address is not in proper format. address="
            << localAddr.c_str();
        std::cout << desc << "\n";
        return 4;
      }
    }
    // Use generic port for connection that will be used with fshipd
    oConnexPtr->setLocalAddr(ip4local, 0);

    if (!remotePort) {
      assert(remotePort != 0);
      return 5;
    } else {
      oConnexPtr->willUseMonitor(1);
    }

    if (remoteAddr.length()) {
      if (!inet_pton(AF_INET, remoteAddr.c_str(), &ip4remote)) {
        LOG(fshipcld, error)
            << "Remote IPV4 address is not in proper format. address="
            << remoteAddr.c_str();
        std::cout << desc << "\n";
        return 3;
      }
    }
    oConnexPtr->setRemoteAddr(ip4remote, remotePort);
    oConnexPtr->willDialRemote();

    LOG(fshipcld, always) << "Interface: " << l_Interface;
    LOG(fshipcld, always) << "Use fshipmond: "
                          << (oConnexPtr->useMonitor() ? "true" : "false");
    LOG(fshipcld, always) << "Local address:  " << localAddr
                          << "  Local port: generic";
    LOG(fshipcld, always) << "Remote address: " << remoteAddr
                          << "  Remote port: " << remotePort;

    if (!mode.compare("rdma")) {
      //  RDMA connection
      l_RC = doNetworked(*((txp::RDMACMfship *)oConnexPtr), remoteAddr,
                         remotePort);
    } else {
      //  Socket connection
      l_RC = doNetworked(*((txp::CnxSock *)oConnexPtr));
    }
    if (l_RC) {
      LOG(fshipcld, error) << __PRETTY_FUNCTION__
                           << "Attach to remote failed, l_RC=" << l_RC;
    }
  } else {
    LOG(fshipcld, error)
        << "Failure when attemping to get the local IP address for interface '"
        << l_Interface << "', l_RC=" << l_RC << " (" << strerror(-l_RC) << ")";
  }

  return l_RC;
}
//! \brief fshipd entry
//!
//! \param argc Standard main parameter
//! \param argv Standard main parameter
//!
//! \return 
//!
int main(int argc, char *argv[]) {
  int l_RC = 0;

  if (getuid()) {
    printf("need to run as root \n");
    exit(-1);
  }

  /**  \TODO signal handlers
   **  \TODO loop to handle read from pipe
   **/
  signal(SIGCHLD, SIG_IGN);
  signal(SIGHUP, SIG_IGN);

  fflush(stdout);

  FshipFuseOpsHandler l_FshipOps;
  l_RC = processArgs(argc, argv, l_FshipOps);

  if (!l_RC) {
    l_RC = l_FshipOps.readMonitorFuseDevice();
  }

  if (l_RC) {
    if (l_RC > 0) {
      LOG(fshipcld, error) << "Ending with rc=" << l_RC;
    } else {
      LOG(fshipcld, error) << "fshipcld::main(): Ending with rc=" << l_RC
                           << " (" << strerror(-l_RC) << ")";
    }
  } else {
    LOG(fshipcld, always) << "Ending normally";
  }

  return (l_RC > 0 ? -l_RC : l_RC);
}
