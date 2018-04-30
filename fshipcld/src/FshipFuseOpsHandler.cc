/*******************************************************************************
 |    FshipFuseOpsHandler.cc
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

//! \file  FshipFuseOpsHandler.C
//! \brief

#include "SignalHandler.h"
#include "TrackPath.h"
#include "fshipcld_flightlog.h"
#include "fshipmount.h"
#include "logging.h"
#include <stdio.h>
#include <sys/mount.h>
#include "../include/FshipFuseOpsHandler.h"

static int doXattr = 0;
static int doFuseDirectIO = 0;

void set_doXattr(int pVal) { doXattr = pVal; }
void set_fuseDirectIO(int pVal) { doFuseDirectIO = pVal; }
FshipFuseOpsHandler::~FshipFuseOpsHandler() {
  if (_signalPipeThread)
    pthread_kill(_signalPipeThread, SIGTERM);
  if (_connectPtr)
    _connectPtr->disconnect();
  if (_responseHandlerPtr) {
    delete _responseHandlerPtr;
    _responseHandlerPtr = NULL;
  }
  if (_readChunksPtr) {
    delete _readChunksPtr;
    _readChunksPtr = NULL;
  }
  if (_remotePath) {
    delete[] _remotePath;
    _remotePath = 0;
  }
  if (_mountDir) {
    close(_deviceFD);
    unmountDevice();
    delete[] _mountDir;
    _mountDir = 0;
  }
  if (_rootNodePtr) {
    delete _rootNodePtr;
    _rootNodePtr = NULL;
  }
}

void FshipFuseOpsHandler::createNodeNameRoot(const char *pRemoteMount) {
  assert(_remotePath == NULL);
  assert(pRemoteMount != NULL);
  int length = strlen(pRemoteMount);
  _remotePath = new char[length + 1];
  memcpy(_remotePath, pRemoteMount, length + 1);
  _rootNodePtr = new NodeNameNetworkedRoot(
      _mountDir, _remotePath); // need for FshipFuseOpsHandler & ResponseHandler
}

FshipFuseOpsHandler::FshipFuseOpsHandler(txp::ConnexPtr pConnectPtr,
                                     char *pMountDir, char *pRemoteMount)
    : _connectPtr(pConnectPtr) 
      ,last_unique(0)
      ,_max_read(0)
      ,_deviceFD(-1)
{
  _rootNodePtr = new NodeNameNetworkedRoot(
      pMountDir, pRemoteMount); // need for FshipFuseOpsHandler & ResponseHandler
  const char *l_RemotePath = _rootNodePtr->getNetworkPath();
  int length = strlen(l_RemotePath);
  _remotePath = new char[length + 1];
  memcpy(_remotePath, l_RemotePath, length + 1);
  const char *const l_MountDir = _rootNodePtr->getMountDir();
  length = strlen(l_MountDir);
  _mountDir = new char[length + 1];
  memcpy(_mountDir, l_MountDir, length + 1);
  // printf("%s:%d _mountDir=%s _remotePath=%s
  // \n",__PRETTY_FUNCTION__,__LINE__,_mountDir,_remotePath);
  _numChunks = 8;
  _chunkSizeRead = 64 * 1024;
  _readChunksPtr = new txp::MemChunk(_numChunks, _chunkSizeRead);
  memset(&_init_in, 0, sizeof(_init_in));
  _responseHandlerPtr = NULL;

  _pipe_descriptor[0] = -1;
  _pipe_descriptor[1] = -1;

  _signalPipeThread = 0;
  _responseHandlerThread = 0;
  _fuseReadState = disconnectedState;
  for(unsigned int i=0;i<ARRAY_SIZE_FUNC_OP; i++) _FshipFuseOp[i]=&FshipFuseOpsHandler::errENOSYS;  //default to ENOSYS for a fuse opcode
  _FshipFuseOp[FUSE_LOOKUP] = &FshipFuseOpsHandler::lookup_op;
  _FshipFuseOp[FUSE_FORGET] = &FshipFuseOpsHandler::forget_op;
  _FshipFuseOp[FUSE_GETATTR] = &FshipFuseOpsHandler::getattr_op;
  _FshipFuseOp[FUSE_SETATTR] = &FshipFuseOpsHandler::setattr_op;

  _FshipFuseOp[FUSE_READLINK] = &FshipFuseOpsHandler::readlink_op;
  _FshipFuseOp[FUSE_SYMLINK] = &FshipFuseOpsHandler::symlink_op;
  _FshipFuseOp[FUSE_MKNOD] = &FshipFuseOpsHandler::mknod_op;
  _FshipFuseOp[FUSE_MKDIR] = &FshipFuseOpsHandler::mkdir_op;

  _FshipFuseOp[FUSE_UNLINK] = &FshipFuseOpsHandler::unlink_op;
  _FshipFuseOp[FUSE_RMDIR] = &FshipFuseOpsHandler::rmdir_op;
  _FshipFuseOp[FUSE_RENAME] = &FshipFuseOpsHandler::rename_op;
  _FshipFuseOp[FUSE_RENAME2] = &FshipFuseOpsHandler::rename_op;

  _FshipFuseOp[FUSE_LINK] = &FshipFuseOpsHandler::link_op;
  _FshipFuseOp[FUSE_OPEN] = &FshipFuseOpsHandler::open_op;
  _FshipFuseOp[FUSE_STATFS] = &FshipFuseOpsHandler::statfs_op;
  _FshipFuseOp[FUSE_RELEASE] = &FshipFuseOpsHandler::release_op;

   _FshipFuseOp[FUSE_FSYNC] = &FshipFuseOpsHandler::fsync_op;
   _FshipFuseOp[FUSE_FLUSH] = &FshipFuseOpsHandler::flush_op;
   _FshipFuseOp[FUSE_FSYNCDIR] = &FshipFuseOpsHandler::fsyncdir_op;

   _FshipFuseOp[FUSE_SETXATTR] = &FshipFuseOpsHandler::setxattr_op;
   _FshipFuseOp[FUSE_GETXATTR] = &FshipFuseOpsHandler::getxattr_op;
   _FshipFuseOp[FUSE_LISTXATTR] = &FshipFuseOpsHandler::listxattr_op;
   _FshipFuseOp[FUSE_REMOVEXATTR] = &FshipFuseOpsHandler::removexattr_op;

   _FshipFuseOp[FUSE_INIT] = &FshipFuseOpsHandler::init_op;
   _FshipFuseOp[FUSE_OPENDIR] = &FshipFuseOpsHandler::opendir_op;
   _FshipFuseOp[FUSE_READDIR] = &FshipFuseOpsHandler::readdir_op;
   _FshipFuseOp[FUSE_RELEASEDIR] = &FshipFuseOpsHandler::releasedir_op;

   _FshipFuseOp[FUSE_GETLK] = &FshipFuseOpsHandler::filelock_op;
   _FshipFuseOp[FUSE_SETLK] = &FshipFuseOpsHandler::filelock_op;
   _FshipFuseOp[FUSE_SETLKW] = &FshipFuseOpsHandler::filelock_op;

   _FshipFuseOp[FUSE_ACCESS] = &FshipFuseOpsHandler::access_op;
   _FshipFuseOp[FUSE_CREATE] = &FshipFuseOpsHandler::create_op;
   _FshipFuseOp[FUSE_INTERRUPT] = &FshipFuseOpsHandler::interrupt_op;
   _FshipFuseOp[FUSE_IOCTL] = &FshipFuseOpsHandler::ioctl_op;

   _FshipFuseOp[FUSE_POLL] = &FshipFuseOpsHandler::poll_op;
   _FshipFuseOp[FUSE_FALLOCATE] = &FshipFuseOpsHandler::fallocate_op;
   _FshipFuseOp[FUSE_READDIRPLUS] = &FshipFuseOpsHandler::readdirplus_op;
   _FshipFuseOp[FUSE_BATCH_FORGET] = &FshipFuseOpsHandler::batchforget_op;

}

FshipFuseOpsHandler::FshipFuseOpsHandler()
    : _mountDir(NULL), _connectPtr(NULL), _remotePath(NULL), _rootNodePtr(NULL)

{
  _numChunks = 8;
  _chunkSizeRead = 64 * 1024;
  _readChunksPtr = new txp::MemChunk(_numChunks, _chunkSizeRead);
  memset(&_init_in, 0, sizeof(_init_in));
  _responseHandlerPtr = NULL;

  _pipe_descriptor[0] = -1;
  _pipe_descriptor[1] = -1;

  _signalPipeThread = 0;
  _responseHandlerThread = 0;
  _fuseReadState = disconnectedState;
  for(unsigned int i=0;i<ARRAY_SIZE_FUNC_OP; i++) _FshipFuseOp[i]=&FshipFuseOpsHandler::errENOSYS;  //default to ENOSYS for a fuse opcode
  _FshipFuseOp[FUSE_LOOKUP] = &FshipFuseOpsHandler::lookup_op;
  _FshipFuseOp[FUSE_FORGET] = &FshipFuseOpsHandler::forget_op;
  _FshipFuseOp[FUSE_GETATTR] = &FshipFuseOpsHandler::getattr_op;
  _FshipFuseOp[FUSE_SETATTR] = &FshipFuseOpsHandler::setattr_op;

  _FshipFuseOp[FUSE_READLINK] = &FshipFuseOpsHandler::readlink_op;
  _FshipFuseOp[FUSE_SYMLINK] = &FshipFuseOpsHandler::symlink_op;
  _FshipFuseOp[FUSE_MKNOD] = &FshipFuseOpsHandler::mknod_op;
  _FshipFuseOp[FUSE_MKDIR] = &FshipFuseOpsHandler::mkdir_op;

  _FshipFuseOp[FUSE_UNLINK] = &FshipFuseOpsHandler::unlink_op;
  _FshipFuseOp[FUSE_RMDIR] = &FshipFuseOpsHandler::rmdir_op;
  _FshipFuseOp[FUSE_RENAME] = &FshipFuseOpsHandler::rename_op;
  _FshipFuseOp[FUSE_RENAME2] = &FshipFuseOpsHandler::rename_op;

  _FshipFuseOp[FUSE_LINK] = &FshipFuseOpsHandler::link_op;
  _FshipFuseOp[FUSE_OPEN] = &FshipFuseOpsHandler::open_op;
  _FshipFuseOp[FUSE_STATFS] = &FshipFuseOpsHandler::statfs_op;
  _FshipFuseOp[FUSE_RELEASE] = &FshipFuseOpsHandler::release_op;

   _FshipFuseOp[FUSE_FSYNC] = &FshipFuseOpsHandler::fsync_op;
   _FshipFuseOp[FUSE_FLUSH] = &FshipFuseOpsHandler::flush_op;
   _FshipFuseOp[FUSE_FSYNCDIR] = &FshipFuseOpsHandler::fsyncdir_op;

   _FshipFuseOp[FUSE_SETXATTR] = &FshipFuseOpsHandler::setxattr_op;
   _FshipFuseOp[FUSE_GETXATTR] = &FshipFuseOpsHandler::getxattr_op;
   _FshipFuseOp[FUSE_LISTXATTR] = &FshipFuseOpsHandler::listxattr_op;
   _FshipFuseOp[FUSE_REMOVEXATTR] = &FshipFuseOpsHandler::removexattr_op;

   _FshipFuseOp[FUSE_INIT] = &FshipFuseOpsHandler::init_op;
   _FshipFuseOp[FUSE_OPENDIR] = &FshipFuseOpsHandler::opendir_op;
   _FshipFuseOp[FUSE_READDIR] = &FshipFuseOpsHandler::readdir_op;
   _FshipFuseOp[FUSE_RELEASEDIR] = &FshipFuseOpsHandler::releasedir_op;

   _FshipFuseOp[FUSE_GETLK] = &FshipFuseOpsHandler::filelock_op;
   _FshipFuseOp[FUSE_SETLK] = &FshipFuseOpsHandler::filelock_op;
   _FshipFuseOp[FUSE_SETLKW] = &FshipFuseOpsHandler::filelock_op;

   _FshipFuseOp[FUSE_ACCESS] = &FshipFuseOpsHandler::access_op;
   _FshipFuseOp[FUSE_CREATE] = &FshipFuseOpsHandler::create_op;
   _FshipFuseOp[FUSE_INTERRUPT] = &FshipFuseOpsHandler::interrupt_op;
   _FshipFuseOp[FUSE_IOCTL] = &FshipFuseOpsHandler::ioctl_op;

   _FshipFuseOp[FUSE_POLL] = &FshipFuseOpsHandler::poll_op;
   _FshipFuseOp[FUSE_FALLOCATE] = &FshipFuseOpsHandler::fallocate_op;
   _FshipFuseOp[FUSE_READDIRPLUS] = &FshipFuseOpsHandler::readdirplus_op;
   _FshipFuseOp[FUSE_BATCH_FORGET] = &FshipFuseOpsHandler::batchforget_op;

}

__s64 FshipFuseOpsHandler::ioctl_op(inMsgGeneric *in) {
  inMsgIoctl *inMsg = (inMsgIoctl *)in;
  LOG(fshipcld, always) << std::hex
                        << "unexpected IOCTL cmd=x:" << inMsg->ioctl_in.cmd
                        << std::dec << " fh=" << inMsg->ioctl_in.fh << std::hex
                        << " flags=" << inMsg->ioctl_in.flags
                        << " arg=" << inMsg->ioctl_in.cmd
                        << " in_size=" << inMsg->ioctl_in.in_size
                        << " out_size=" << inMsg->ioctl_in.out_size << std::dec;

  return error_send(in, -ENOSYS);
}

__s64 FshipFuseOpsHandler::processMsg(inMsgGeneric *in) {

  __s64 retval = 0;

  if (in->hdr.opcode == FUSE_READ) {
    return read_op(in);
  }
  if (in->hdr.opcode == FUSE_WRITE) {
    return write_op(in);
  }
  switch (in->hdr.opcode) {
  case FUSE_LOOKUP:
    retval = lookup_op(in);
    break; /* no reply */
  case FUSE_FORGET:
    retval = forget_op(in);
    break;
  case FUSE_GETATTR:
    retval = getattr_op(in);
    break;
  case FUSE_SETATTR:
    retval = setattr_op(in);
    break;
  case FUSE_READLINK:
    retval = readlink_op(in);
    break;
  case FUSE_SYMLINK:
    retval = symlink_op(in);
    break;
  case FUSE_MKNOD:
    retval = mknod_op(in);
    break;
  case FUSE_MKDIR:
    retval = mkdir_op(in);
    break;
  case FUSE_UNLINK:
    retval = unlink_op(in);
    break;
  case FUSE_RMDIR:
    retval = rmdir_op(in);
    break;
  case FUSE_RENAME:
    retval = rename_op(in);
    break;
  case FUSE_RENAME2:
    retval = rename2_op(in);
    break;
  case FUSE_LINK:
    retval = link_op(in);
    break;
  case FUSE_OPEN:
    retval = open_op(in);
    break;
  case FUSE_STATFS:
    retval = statfs_op(in);
    break;
  case FUSE_RELEASE:
    retval = release_op(in);
    break;
  case FUSE_FSYNC:
    retval = fsync_op(in);
    break;
  case FUSE_SETXATTR:
    retval = setxattr_op(in);
    break;
  case FUSE_GETXATTR:
    retval = getxattr_op(in);
    break;
  case FUSE_LISTXATTR:
    retval = listxattr_op(in);
    break;
  case FUSE_REMOVEXATTR:
    retval = removexattr_op(in);
    break;
  case FUSE_FLUSH:
    retval = flush_op(in);
    break;
  case FUSE_INIT:
    retval = init_op(in);
    break;
  case FUSE_OPENDIR:
    retval = opendir_op(in);
    break;
  case FUSE_READDIR:
    retval = readdir_op(in);
    break;
  case FUSE_RELEASEDIR:
    retval = releasedir_op(in);
    break;
  case FUSE_FSYNCDIR:
    retval = fsyncdir_op(in);
    break;

  case FUSE_GETLK:
  case FUSE_SETLK:
  case FUSE_SETLKW:
    retval = filelock_op(in);
    break;

  case FUSE_ACCESS:
    retval = access_op(in);
    break;
  case FUSE_CREATE:
    retval = create_op(in);
    break;
  case FUSE_INTERRUPT:
    retval = interrupt_op(in);
    break;

  case FUSE_IOCTL:
    retval = ioctl_op(in);
    break;

  case FUSE_POLL:
    retval = poll_op(in);
    break;
  case FUSE_FALLOCATE:
    retval = fallocate_op(in);
    break;

  case FUSE_READDIRPLUS:
    retval = readdirplus_op(in);
    break;
  case FUSE_BATCH_FORGET:
    retval = batchforget_op(in);
    break;
  default:
   //case FUSE_BMAP:
   //case FUSE_DESTROY:

    if (in->hdr.opcode == 46) { // FUSE_LSEEK in 7.24
      LOG(fshipcld, always) << "LSEEK opcode=" << in->hdr.opcode;
      return error_send(in, -ENOSYS);
    } else {
      LOG(fshipcld, always) << "invalid opcode=" << in->hdr.opcode;
      assert(in->hdr.opcode == 0);
      return error_send(in, -ENOSYS);
    }
    break;
  } // end switch

  return retval;
};

__s64 FshipFuseOpsHandler::errENOSYS(inMsgGeneric *in){
   LOG(fshipcld, always) << "invalid opcode=" << in->hdr.opcode;
   //assert(in->hdr.opcode == 0);
   return error_send(in, -ENOSYS);
}

// return 0 if successful
int FshipFuseOpsHandler::mountDevice(int pMax_read, const char *pMountDir) {
  int length = strlen(pMountDir);
  _mountDir = new char[length + 1];
  memcpy(_mountDir, pMountDir, length + 1);
  _max_read = pMax_read;
  _deviceFD = mount_fship(_mountDir, pMax_read);
  int counter = 30;
  while (_deviceFD < 0) {
    _deviceFD = mount_fship(_mountDir, pMax_read);
    if (_deviceFD > 0)
      return 0;
    if (--counter < 0)
      return _deviceFD;
  }
  if (_deviceFD > 0)
    return 0;
  return _deviceFD;
};

__s64 FshipFuseOpsHandler::unmountDevice() { return unmountDeviceLazy(); };

__s64 FshipFuseOpsHandler::unmountDeviceLazy() {
  __s64 retval = umount2(_mountDir, MNT_DETACH);
  if (retval) {
    retval = -errno;
    printf("umount2/MNTDETACH for %s encountered errno=%d which is %s\n",
           _mountDir, errno, strerror(errno)); // need RAS log
  }
  return retval;
};

void *FshipFuseOpsHandler::startProcessSignalPipe(void *object) {
  ((FshipFuseOpsHandler *)object)->processSignalPipe();
  return NULL;
}

void FshipFuseOpsHandler::processSignalPipe() {
  struct pollfd pollSignals;
  SigWritePipe SigWritePipe(SIGINT);
  SigWritePipe.addSignal(SIGTERM);
  pollSignals.fd = SigWritePipe.readPipeFd();
  pollSignals.events = POLLIN;

  int rc = 0;
  int waitTimeOut = -1;
  while (!rc) {
    pollSignals.revents = 0;
    rc = poll(&pollSignals, 1, waitTimeOut);
    if (rc == -1) {
      LOG(fshipcld, always) << "pollSignals errno=" << errno << ":"
                            << strerror(errno);
      if (errno != EINTR) {
        _signalPipeThread = 0;
        return;
      }
      rc = 0;
    }
    if (pollSignals.revents & (POLLERR | POLLHUP | POLLNVAL)) {
      LOG(fshipcld, always) << "pollSignals (POLLERR|POLLHUP|POLLNVAL)";
      exit(-1);
    }
    if (pollSignals.revents) {
      _fuseReadState = signalingState;
      siginfo_t signalInfo;
      int pipeRead = read(pollSignals.fd, &signalInfo, sizeof(signalInfo));
      if (pipeRead > 0) {
        LOG(fshipcld, always) << ">>signal#=" << signalInfo.si_signo << "("
                              << strsignal(signalInfo.si_signo)
                              << ")  code=" << signalInfo.si_code
                              << " sendPid=" << signalInfo.si_pid
                              << "/realuid=" << signalInfo.si_uid;
        wakeupPipeMsg wPM(WAKEUP_SIGNAL, signalInfo.si_signo);
        int rcWrite = write(writePipeFd(), &wPM, sizeof(wPM));
        if (rcWrite == -1)
          LOG(fshipcld, always) << "rcWrite=" << rcWrite
                                << " sending wPM.id=" << wPM.id
                                << " errno=" << errno << ":" << strerror(errno)
                                << " writePipeFd()=" << writePipeFd();
        signal(signalInfo);
        if ((signalInfo.si_signo == SIGTERM) ||
            (signalInfo.si_signo == SIGINT)) {
          _signalPipeThread = 0;
          return; // need destructor of SigWritePipe to run
        }
      }
      rc = 0;
    }

  } // endwhile
}

void FshipFuseOpsHandler::responseHandler() {
  int l_RC = 0;

  while (
      (!l_RC) &&
      (_connectPtr->poll4DataIn() ==
       1)) { // TODO do another readInMessage attempt before going back to poll
    while (!l_RC) { // Keep getting messages
      l_RC = _responseHandlerPtr->readInMessage();
    }
    if (l_RC == 1)
      l_RC = 0;
  }

  LOG(fshipcld, error) << "ResponseHandler(): Ending with rc=" << l_RC;
  _responseHandlerThread = 0;
  wakeupPipeMsg wPM(WAKEUP_RESPONSETHREADEND);
  int rcWrite = write(writePipeFd(), &wPM, sizeof(wPM));
  if (rcWrite == -1)
    LOG(fshipcld, always) << "rcWrite=" << rcWrite
                          << " sending wPM.id=" << wPM.id << " errno=" << errno
                          << ":" << strerror(errno)
                          << " writePipeFd()=" << writePipeFd();
}

void *FshipFuseOpsHandler::startResponseHandler(void *object) {
  ((FshipFuseOpsHandler *)object)->responseHandler();
  return NULL;
}

int FshipFuseOpsHandler::readMonitorFuseDevice() {
  // Need a running ResponseHandler
  int RCpipe2 = pipe2(_pipe_descriptor, O_DIRECT | O_NONBLOCK);
  if (RCpipe2) {
    LOG(fshipcld, always) << "pipe2 errno=" << errno << ":" << strerror(errno);
    abort();
  }
  LOG(fshipcld, debug) << "internal pipe descriptors _pipe_descriptor[0]="
                       << _pipe_descriptor[0] << " [1]=" << _pipe_descriptor[1];
  if (!_responseHandlerPtr)
    _responseHandlerPtr = new ResponseHandler(_rootNodePtr, _connectPtr,
                                              _deviceFD, _pipe_descriptor[1]);
  assert(_pipe_descriptor[1] == writePipeFd());
  assert(_pipe_descriptor[1] > 0);

  int RCpthread = pthread_create(&_responseHandlerThread, NULL,
                                 FshipFuseOpsHandler::startResponseHandler, this);
  if (RCpthread)
    abort();

  _fuseReadState = activeState;

  int RCpthreadsignal =
      pthread_create(&_signalPipeThread, NULL,
                     FshipFuseOpsHandler::startProcessSignalPipe, this);
  if (RCpthreadsignal)
    abort();
  memItemPtr mip =
      _readChunksPtr->getChunk(); // get memorychunk for read of header

  const int fuseDevice = 0;
  const int wakeupPipe = 1;
  const int numFds = 2;

  struct pollfd pollInfo[numFds];
  pollInfo[fuseDevice].fd = _deviceFD;
  pollInfo[fuseDevice].events = POLLIN;
  pollInfo[fuseDevice].revents = 0;
  pollInfo[wakeupPipe].fd = readPipeFd();
  pollInfo[wakeupPipe].events = POLLIN;
  pollInfo[wakeupPipe].revents = 0;
  LOG(fshipcld, always) << "wakup fd =" << pollInfo[wakeupPipe].fd;
  int rc = 0;
  int rc_ssize = 0;
  int connect_wait_timeout =
      -1; // minimum number of milliseconds poll will block -1=forever
  while (rc >= 0) {

    rc = poll(pollInfo, numFds, connect_wait_timeout);
    if (_fuseReadState != activeState) {
      LOG(fshipcld, always) << "fuseReadState=" << _fuseReadState;
    }
    if (__glibc_unlikely(rc == -1)) {
      LOG(fshipcld, always) << "fuse poll fd with errno=" << errno << ":"
                            << strerror(errno);
      if (errno != EINTR) {
        rc = -errno;
        break;
      } else {
      }
      rc = 0;
    }
    if (__glibc_unlikely(pollInfo[wakeupPipe].revents)) {
      wakeupPipeMsg wPM;
      int pipeRead =
          read(pollInfo[wakeupPipe].fd, (void *)&wPM, sizeof(wakeupPipeMsg));
      LOG(fshipcld, always) << "wakeupPipe";
      if (__glibc_likely(pipeRead > 0)) {
        LOG(fshipcld, always) << "wakeupPipe id=" << wPM.id;
        if (wPM.id == WAKEUP_RESPONSETHREADEND) {
          return 0;
        } else {
          pollInfo[fuseDevice].events = 0; // stop polling the fuse device
          continue;
        }
      }
    }

    if (pollInfo[fuseDevice].revents &
        (POLLERR | POLLHUP | POLLNVAL)) { // error!
      /* clang-format off */ 
      FL_Write(FL_FUSE, FUSEPOLLEND, "Fuse  POLLERR|POLLHUP|POLLNVAL pollInfo[fuseDevice].revents=%lx", pollInfo[fuseDevice].revents, 0,0,0);
      /* clang-format on */
      LOG(fshipcld, always)
          << "Fuse  POLLERR|POLLHUP|POLLNVAL pollInfo[fuseDevice].revents="
          << std::hex << pollInfo[fuseDevice].revents << std::dec;
      rc = -1;
      break;
    }
    rc_ssize = 1;
    while (rc_ssize > 0) {
      if (!mip->next) {
        mip->next = _connectPtr->getRDMAChunk();
        if (__glibc_unlikely(!mip->next)) {
          sayError((char *)"NULL from getRDMAChunk--timeout");
          assert(mip != NULL);
        }
      }
      rc_ssize = readFuseMessage(mip);

      if (__glibc_unlikely(rc_ssize < 0)) {
        rc = 0;
        if (-rc_ssize == EWOULDBLOCK)
          break;
        if (-rc_ssize == EINTR) {

          break;
        }
        /* clang-format off */ 
        FL_Write(FL_FUSE, FUSEREADERR, "Fuse read error %ld line=%ld", rc_ssize, __LINE__,0,0);
        /* clang-format on */
        rc = rc_ssize;
        break;
      }
      if (__glibc_likely(rc_ssize > 0)) {
        pollInfo[fuseDevice].revents = 0;
        inMsgGeneric *in = (inMsgGeneric *)mip->address;
        /* clang-format off */ 
        FL_Write6(FL_FUSE, FUSEMSG, "Fuse Message opcode=%ld length=%ld unique=%ld nodeid=%ld uid=%ld pid=%ld", in->hdr.opcode, in->hdr.len, in->hdr.unique, in->hdr.nodeid, in->hdr.uid, in->hdr.pid);
        /* clang-format on */
        // if (rc_ssize>65536) LOG(fshipcld,always)<< "hex
        // len="<<std::hex<<in->hdr.len<<" dec="<<std::dec<<in->hdr.len;

        if (in->hdr.opcode == FUSE_WRITE)
          write_op(mip);
        else if (in->hdr.opcode == FUSE_READ)
          read_op(mip);
        else
          (this->*_FshipFuseOp[in->hdr.opcode])(in);  
          //processMsg(in);

        memset(in, 0, sizeof(inMsgGeneric));
      }
    }
  }

  LOG(fshipcld, error)
      << "FshipFuseOpsHandler::readMonitorFuseDevice(): Ending with rc=" << rc;
  return rc;
}

__s64 FshipFuseOpsHandler::poll_op(inMsgGeneric *in) {
  inMsgPoll *inMsg = (inMsgPoll *)in;
  inMsg->poll_in.events &= ~(
      POLLERR | POLLHUP | POLLNVAL); // these flags are ignored when doing poll;
  if (inMsg->poll_in.events & ~(POLLIN | POLLOUT)) {
    LOG(fshipcld, always) << "More than poll in or out, fuse_poll_in.fh="
                          << inMsg->poll_in.fh << std::hex << " flags=0x"
                          << inMsg->poll_in.flags << " events=0x"
                          << inMsg->poll_in.events << " kh=0x"
                          << inMsg->poll_in.kh << std::dec;
  }
  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, POLL, "fh=%ld  unique=%ld nodeid=%ld flags=%ld events=%lx kh=%lx", inMsg->poll_in.fh, in->hdr.unique, in->hdr.nodeid,inMsg->poll_in.flags,inMsg->poll_in.events,inMsg->poll_in.kh);
  /* clang-format on */
  TrackPathFreeMsg trackPath(txp::FUSE_POLL);
  trackPath.addStruct(txp::inMsgPoll, in, sizeof(inMsgPoll));
  trackPath.sendMsg(_connectPtr);
  return 0;
}

__s64 FshipFuseOpsHandler::getxattr_op(inMsgGeneric *in) {
  if (!doXattr)
    return error_send(in, -ENOSYS);
  inMsgGetXattr *inMsg = (inMsgGetXattr *)in;
  LOG(fshipcld, debug) << "getxattr for unique=" << in->hdr.unique
                       << " name=" << inMsg->nameOfAttribute
                       << " ::buffer size=" << inMsg->getxattr_in.size
                       << " nodeid=" << in->hdr.nodeid;
  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, GETXATTR, "getxattr_op=%ld  unique=%ld nodeid=%ld line=%ld,size=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,__LINE__,inMsg->getxattr_in.size,0);
  /* clang-format on */
  TrackPathFreeMsg trackPath(txp::FUSE_GETXATTR);
  trackPath.addFuseHeaderItems(in);
  trackPath.addUint32(txp::size, inMsg->getxattr_in.size);
  trackPath.addFQPN(_rootNodePtr, txp::name);
  trackPath.addString(txp::nameOfAttribute, inMsg->nameOfAttribute);
  trackPath.sendMsg(_connectPtr);
  return 0;
}

__s64 FshipFuseOpsHandler::setxattr_op(inMsgGeneric *in) {
  if (!doXattr)
    return error_send(in, -ENOSYS);

  inMsgSetXattr *inMsg = (inMsgSetXattr *)in;
  LOG(fshipcld, debug) << "setxattr for unique=" << in->hdr.unique
                       << " name=" << inMsg->nameOfAttribute
                       << " ::buffer size=" << inMsg->setxattr_in.size
                       << " nodeid=" << in->hdr.nodeid
                       << " flags=" << inMsg->setxattr_in.flags;
  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, SETXATTR, "setxattr_op=%ld  unique=%ld nodeid=%ld line=%ld,size=%ld flags=0x%lx", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,__LINE__,inMsg->setxattr_in.size,inMsg->setxattr_in.flags);
  /* clang-format on */
  TrackPathFreeMsg trackPath(txp::FUSE_SETXATTR);
  trackPath.addFuseHeaderItems(in);
  trackPath.addStruct(txp::fuse_setxattr_in, &inMsg->setxattr_in,
                      sizeof(inMsg->setxattr_in));
  trackPath.addFQPN(_rootNodePtr, txp::name);
  trackPath.addString(txp::nameOfAttribute, inMsg->nameOfAttribute);
  // access data for the attribute
  char *l_data = (char *)in + (in->hdr.len - inMsg->setxattr_in.size);
  trackPath.addStruct(txp::data, l_data, inMsg->setxattr_in.size);
  trackPath.sendMsg(_connectPtr);

  return 0;
}


__s64 FshipFuseOpsHandler::listxattr_op(inMsgGeneric *in) {

  if (!doXattr)
    return error_send(in, -ENOSYS);
  inMsgListXattr *inMsg = (inMsgListXattr *)in;
  LOG(fshipcld, debug) << "listxattr for unique=" << in->hdr.unique
                       << " ::buffer size=" << inMsg->getxattr_in.size
                       << " nodeid=" << in->hdr.nodeid;
  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, LISTXATTR, "listxattr_op=%ld  unique=%ld nodeid=%ld line=%ld size=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,__LINE__,inMsg->getxattr_in.size,0);
  /* clang-format on */
  TrackPathFreeMsg trackPath(txp::FUSE_LISTXATTR);
  trackPath.addFuseHeaderItems(in);
  trackPath.addUint32(txp::size, inMsg->getxattr_in.size);
  trackPath.addFQPN(_rootNodePtr, txp::name);
  trackPath.sendMsg(_connectPtr);
  return 0;
}
__s64 FshipFuseOpsHandler::removexattr_op(inMsgGeneric *in) {
  if (!doXattr)
    return error_send(in, -ENOSYS);
  inMsgRemoveXattr *inMsg = (inMsgRemoveXattr *)in;
  LOG(fshipcld, debug) << "removexattr for unique=" << in->hdr.unique
                       << " name=" << inMsg->nameOfAttribute
                       << " nodeid=" << in->hdr.nodeid;
  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, REMOVEXATTR, "removexattr_op=%ld  unique=%ld nodeid=%ld line=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,__LINE__,0,0);
  /* clang-format on */
  TrackPathFreeMsg trackPath(txp::FUSE_REMOVEXATTR);
  trackPath.addFuseHeaderItems(in);
  trackPath.addFQPN(_rootNodePtr, txp::name);
  trackPath.addString(txp::nameOfAttribute, inMsg->nameOfAttribute);
  trackPath.sendMsg(_connectPtr);
  return 0;
}

__s64 FshipFuseOpsHandler::sayError(char *errorString) {
  TrackPathFreeMsg trackPath(txp::CORAL_ERROR);
  trackPath.addString(txp::name, errorString);
  trackPath.sendMsg(_connectPtr);
  return 0;
}

__s64 FshipFuseOpsHandler::sayHello(inMsgInit *inMsg) {
  __s64 retval = 0;

  inHello l_Hello;
  l_Hello.max_read = _connectPtr->getRDMAchunkSize();
  l_Hello.daemon_pid = getpid();
  inMsgGeneric *in = (inMsgGeneric *)inMsg;
  l_Hello.version_major = inMsg->init_in.major;
  l_Hello.version_minor = inMsg->init_in.minor;

  l_Hello.generation =
      l_Hello
          .daemon_pid; /* Inode generation: nodeid:gen must be unique for the
                          fs's lifetime */
  l_Hello.entry_valid =
      NodeNameRoot::entryValidsec; /* Cache timeout for the name */
  l_Hello.entry_valid_nsec = NodeNameRoot::entryValidnsec;
  l_Hello.attr_valid =
      NodeNameRoot::attrValidsec; /* Cache timeout for the attributes */
  l_Hello.attr_valid_nsec = NodeNameRoot::attrValidnsec;
  l_Hello.openOutFlags = doFuseDirectIO;

  TrackPathFreeMsg trackPath(txp::CORAL_HELLO);
  trackPath.addFuseHeaderItems(in);

  trackPath.addStruct(txp::inHello, &l_Hello, sizeof(l_Hello));
  trackPath.addString(txp::name, _remotePath);

  ssize_t retSSize = trackPath.sendMsg(_connectPtr);
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    retval = error_send(in, -errno);
  }
  return retval;
}

int FshipFuseOpsHandler::signal(siginfo_t &pSignalInfo) {
  signalMsg gbM(pSignalInfo.si_signo);
  txp::Msg *l_Msg = 0;
  txp::Msg::buildMsg(txp::CORAL_SIGNAL, l_Msg);
  l_Msg->addAttribute(txp::signalMsg, (char *)&gbM, sizeof(gbM));
  ssize_t retSSize = _connectPtr->write(l_Msg);

  if (l_Msg) {
    delete l_Msg;
    l_Msg = NULL;
  }
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
  }

  return 0;
};

__s64 FshipFuseOpsHandler::init_op(inMsgGeneric *in) {
  __s64 retval = 0;

  inMsgInit *inMsg = (inMsgInit *)in;

  LOG(fshipcld, always) << "Fuse init inMsg major=" << inMsg->init_in.major
                        << " minor=" << inMsg->init_in.minor
                        << " readahead=" << inMsg->init_in.max_readahead
                        << " flags=" << std::hex << inMsg->init_in.flags
                        << std::dec;

  uint32_t flags = inMsg->init_in.flags;
  uint32_t residue = dump_init_flags(flags);
  if (residue)
    LOG(fshipcld, always) << "undocumented flags=" << std::hex << residue
                          << std::dec;

  if ((inMsg->init_in.major >
       FUSE_KERNEL_VERSION)) { // newer version of fuse module should be OK
  } else if ((inMsg->init_in.major == FUSE_KERNEL_VERSION) &&
             (inMsg->init_in.minor >=
              FUSE_KERNEL_MINOR_VERSION)) { // newer minor with major should be
                                            // OK
  } else {
    LOGERRNO(fshipcld, error, EBADE);
    return error_send(in, -EBADE);
    abort();
  }
  setInit(inMsg->init_in);
  retval = sayHello(inMsg);

  if (retval < 0)
    return errno;
  return retval;
}

__s64 FshipFuseOpsHandler::setattr_op(inMsgGeneric *in) {
  __s64 retval = 0;
  inMsgSetattr *inMsg = (inMsgSetattr *)in;
  uint64_t l_remoteNodeID = 0;
  TrackPathFreeMsg trackPath(txp::FUSE_SETATTR);
  trackPath.addFuseHeaderItems(in);
  if (!(inMsg->setattr_in.valid & FATTR_FH)) {
    l_remoteNodeID = trackPath.addFQPN(_rootNodePtr, txp::name);
    if (!l_remoteNodeID) {
      retval = error_send(in, -ENOENT);
      assert(l_remoteNodeID != 0);
      return retval;
    }
    trackPath.addUint64(txp::nodeid, l_remoteNodeID);
  }
  trackPath.addFuseAttrIn(&inMsg->setattr_in);

  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, SETATTR, "setattr_op=%ld  unique=%ld nodeid=%ld remoteNodeid=%ld fh=%ld line=%ld,", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,l_remoteNodeID,inMsg->setattr_in.fh,__LINE__);
  /* clang-format on */

  ssize_t retSSize = trackPath.sendMsg(_connectPtr);
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    retval = error_send(in, -errno);
  }
  return retval;
}

__s64 FshipFuseOpsHandler::getattr_op(inMsgGeneric *in) {
  __s64 retval = 0;
  inMsgGetattr *inMsg = (inMsgGetattr *)in;
  uint64_t l_remoteNodeID = 0;
  TrackPathFreeMsg trackPath(txp::FUSE_GETATTR);

  trackPath.addFuseHeaderItems(in);
  if (inMsg->getattr_in.getattr_flags & FUSE_GETATTR_FH) { // Use file-handle
    trackPath.addFD2msg(txp::fh, inMsg->getattr_in.fh);
  } else {
    l_remoteNodeID = trackPath.addFQPN(_rootNodePtr, txp::name);
    if (!l_remoteNodeID) {
      retval = error_send(in, -ENOENT);
      assert(l_remoteNodeID != 0);
      return retval;
    }
    trackPath.addUint64(txp::nodeid, l_remoteNodeID);
  }
  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, GETATTR, "getattr_op=%ld  unique=%ld nodeid=%ld remoteNodeid=%ld fh=%ld line=%ld,", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,l_remoteNodeID,inMsg->getattr_in.fh,__LINE__);
  /* clang-format on */

  ssize_t retSSize = trackPath.sendMsg(_connectPtr);
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    retval = error_send(in, -errno);
  }
  return retval;
}

__s64 FshipFuseOpsHandler::readlink_op(inMsgGeneric *in) {
  __s64 retval = 0;
  TrackPathFreeMsg trackPath(txp::FUSE_READLINK);
  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, READLINK, "readlink_op=%ld  unique=%ld nodeid=%ld line=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,__LINE__,0,0);
  /* clang-format on */
  trackPath.addFuseHeaderItems(in);
  trackPath.addFQPN(_rootNodePtr, txp::name);

  ssize_t retSSize = trackPath.sendMsg(_connectPtr);
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    retval = error_send(in, -errno);
  }
  return retval;
}

__s64 FshipFuseOpsHandler::release_op(inMsgGeneric *in) {

  inMsgRelease *inMsg = (inMsgRelease *)in;
  txp::Msg *l_Msg = 0;
  txp::Msg::buildMsg(txp::FUSE_RELEASE, l_Msg);
  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, RELEASE, "(close) release_op=%ld  unique=%ld nodeid=%ld line=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,__LINE__,0,0);
  /* clang-format on */
  int l_RC = l_Msg->addAttribute(txp::inMsgRelease, (char *)inMsg,
                                 sizeof(inMsgRelease));
  if (l_RC)
    abort();

  ssize_t retSSize = _connectPtr->write(l_Msg);
  if (l_Msg) {
    delete l_Msg;
    l_Msg = NULL;
  }

  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    return error_send(in, -errno);
  }

  return 0;
}

// ignore the flags and release flag (flush) for releasedir
__s64 FshipFuseOpsHandler::releasedir_op(inMsgGeneric *in) {

  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, RELEASEDIR, "releasedir_op=%ld  unique=%ld nodeid=%ld line=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,__LINE__,0,0);
  /* clang-format on */
  LOG(fshipcld, debug) << "ReleaseDir nodeid=" << in->hdr.nodeid;
  TrackPathFreeMsg trackPath(txp::FUSE_RELEASEDIR);
  trackPath.addFuseHeaderItems(in);
  inMsgReleasedir *inMsg = (inMsgReleasedir *)in;
  trackPath.addFD2msg(txp::fh, inMsg->release_in.fh);

  ssize_t retSSize = trackPath.sendMsg(_connectPtr);
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    return error_send(in, -errno);
  }
  return 0;
}

int FshipFuseOpsHandler::addFuseHeaderItems(txp::Msg *pMsg, inMsgGeneric *in) {
  txp::AttrPtr_char_array *l_Attr = NULL;
  int l_RC = txp::AttrPtr_char_array::buildAttrPtr_char_array(
      txp::fuse_in_header, (char *)in, sizeof(inMsgGeneric), l_Attr);
  if (!l_RC) {
    l_RC = pMsg->addAttribute(l_Attr);
  }
  if (l_RC)
    abort(); // messed up building attribue
  return l_RC;
}

int FshipFuseOpsHandler::addPath(txp::Msg *pMsg, char *pPath, int pLength,
                               txp::AttributeName attPathName) {
  txp::AttrPtr_char_array *l_Attr = NULL;
  int l_RC = txp::AttrPtr_char_array::buildAttrPtr_char_array(
      attPathName, pPath, pLength, l_Attr);
  if (!l_RC) {
    l_RC = pMsg->addAttribute(l_Attr);
  }
  if (l_RC)
    abort(); // messed up building message
  return l_RC;
}

__s64 FshipFuseOpsHandler::open_op(inMsgGeneric *in) {
  __s64 retval = 0;
  inMsgOpen *inMsg = (inMsgOpen *)in;
  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, OPEN, "open_op=%ld  unique=%ld nodeid=%ld line=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,__LINE__,0,0);
  /* clang-format on */
  TrackPathFreeMsg trackPath(txp::FUSE_OPEN);
  trackPath.addFuseHeaderItems(in);
  trackPath.addFQPN(_rootNodePtr, txp::name);
  trackPath.addUint32(txp::flags, inMsg->open_in.flags);

  ssize_t retSSize = trackPath.sendMsg(_connectPtr);
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    retval = error_send(in, -errno);
  }
  return retval;
};

__s64 FshipFuseOpsHandler::opendir_op(inMsgGeneric *in) {
  __s64 retval = 0;

  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, OPENDIR, "opendir_op=%ld  unique=%ld nodeid=%ld line=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,__LINE__,0,0);
  /* clang-format on */
  LOG(fshipcld, debug) << "OpenDir nodeid=" << in->hdr.nodeid;
  TrackPathFreeMsg trackPath(txp::FUSE_OPENDIR);
  trackPath.addFuseHeaderItems(in);
  trackPath.addFQPN(_rootNodePtr, txp::name);

  ssize_t retSSize = trackPath.sendMsg(_connectPtr);
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    retval = error_send(in, -errno);
  }
  return retval;
}

__s64 FshipFuseOpsHandler::lookup_op(inMsgGeneric *in) {
  __s64 retval = 0;
  inMsgLookup *inMsg = (inMsgLookup *)in;
  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, LOOKUP, "lookup_op=%ld  unique=%ld nodeid=%ld line=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,__LINE__,0,0);
  /* clang-format on */
  LOG(fshipcld, debug) << "lookupname=" << inMsg->lookupname;
  TrackPathFreeMsg trackPath(txp::FUSE_LOOKUP, inMsg->lookupname);
  trackPath.addFuseHeaderItems(in);
  trackPath.addPath2Msg4lookup(_rootNodePtr, txp::name);
  // trackPath.useMemStruct();
  ssize_t retSSize = trackPath.sendMsg(_connectPtr);
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    retval = error_send(in, -errno);
  }
  return retval;
}
__s64 FshipFuseOpsHandler::create_op(inMsgGeneric *in) {
  __s64 retval = 0;
  inMsgCreate *inMsg = (inMsgCreate *)in;
  mode_t mode = (inMsg->create_in.mode) & ~(inMsg->create_in.umask);
  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, CREATE, "create_op=%ld octal mode=%lo unique=%ld nodeid=%ld origMode=%lo umask=%lo", in->hdr.opcode, (uint64_t)mode, in->hdr.unique, in->hdr.nodeid, inMsg->create_in.mode,inMsg->create_in.umask);
  /* clang-format on */

  LOG(fshipcld, debug) << "create name=" << inMsg->name;
  TrackPathFreeMsg trackPath(txp::FUSE_CREATE, inMsg->name);
  trackPath.addFuseHeaderItems(in);
  trackPath.addPath2Msg4lookup(_rootNodePtr, txp::name);
  trackPath.addUint32(txp::mode, mode);
  trackPath.addUint32(txp::flags, inMsg->create_in.flags);

  ssize_t retSSize = trackPath.sendMsg(_connectPtr);
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    retval = error_send(in, -errno);
  }
  return retval;
}

__s64 FshipFuseOpsHandler::mknod_op(inMsgGeneric *in) {
  __s64 retval = 0;
  inMsgMknod *inMsg = (inMsgMknod *)in;
  mode_t mode = (inMsg->mknod_in.mode) & ~(inMsg->mknod_in.umask);
  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, MKNOD, "mknod_op=%ld octal mode=%lo unique=%ld nodeid=%ld origMode=%lo umask=%lo", in->hdr.opcode, (uint64_t)mode, in->hdr.unique, in->hdr.nodeid, inMsg->mknod_in.mode,inMsg->mknod_in.umask);
  /* clang-format on */

  LOG(fshipcld, debug) << "create name=" << inMsg->name;
  TrackPathFreeMsg trackPath(txp::FUSE_MKNOD, inMsg->name);
  trackPath.addFuseHeaderItems(in);
  trackPath.addPath2Msg4lookup(_rootNodePtr, txp::name);
  trackPath.addUint32(txp::mode, mode);
  trackPath.addUint32(txp::rdev, inMsg->mknod_in.rdev);

  ssize_t retSSize = trackPath.sendMsg(_connectPtr);
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    retval = error_send(in, -errno);
  }
  return retval;
}

__s64 FshipFuseOpsHandler::mkdir_op(inMsgGeneric *in) {
  __s64 retval = 0;
  inMsgMkdir *inMsg = (inMsgMkdir *)in;
  mode_t mode = (inMsg->mkdir_in.mode) & ~(inMsg->mkdir_in.umask);
  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, MKDIR, "mkdir_op=%ld octal mode=%lo unique=%ld nodeid=%ld origMode=%lo umask=%lo", in->hdr.opcode, (uint64_t)mode, in->hdr.unique, in->hdr.nodeid, inMsg->mkdir_in.mode,inMsg->mkdir_in.umask);
  /* clang-format on */
  LOG(fshipcld, debug) << "mkdir name=" << inMsg->name;
  TrackPathFreeMsg trackPath(txp::FUSE_MKDIR, inMsg->name);
  trackPath.addFuseHeaderItems(in);
  trackPath.addPath2Msg4lookup(_rootNodePtr, txp::name);
  trackPath.addUint32(txp::mode, mode);

  ssize_t retSSize = trackPath.sendMsg(_connectPtr);
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    retval = error_send(in, -errno);
  }
  return retval;
}

__s64 FshipFuseOpsHandler::rmdir_op(inMsgGeneric *in) {
  __s64 retval = 0;
  inMsgUnlinkAtName *inMsg = (inMsgUnlinkAtName *)in;

  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, RMDIR,  "rmdir_op=%ld  unique=%ld nodeid=%ld line=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,__LINE__,0,0);
  /* clang-format on */
  LOG(fshipcld, debug) << "rmdir name=" << inMsg->unlinkatname;
  TrackPathFreeMsg trackPath(txp::FUSE_RMDIR, inMsg->unlinkatname);
  trackPath.addFuseHeaderItems(in);
  trackPath.addPath2Msg4lookup(_rootNodePtr, txp::name);

  ssize_t retSSize = trackPath.sendMsg(_connectPtr);
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    retval = error_send(in, -errno);
  }
  return retval;
}

__s64 FshipFuseOpsHandler::unlink_op(inMsgGeneric *in) {
  __s64 retval = 0;
  inMsgUnlinkAtName *inMsg = (inMsgUnlinkAtName *)in;

  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, UNLINK,  "unlink_op=%ld  unique=%ld nodeid=%ld line=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,__LINE__,0,0);
  /* clang-format on */
  LOG(fshipcld, debug) << "unlink name=" << inMsg->unlinkatname;
  TrackPathFreeMsg trackPath(txp::FUSE_UNLINK, inMsg->unlinkatname);
  trackPath.addFuseHeaderItems(in);
  trackPath.addPath2Msg4lookup(_rootNodePtr, txp::name);

  ssize_t retSSize = trackPath.sendMsg(_connectPtr);
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    retval = error_send(in, -errno);
  }
  return retval;
}

__s64 FshipFuseOpsHandler::rename2_op(inMsgGeneric *in) {
  /* clang-format off */ 
   FL_Write6(FL_fshipcldfuseop, RENAME2,  "rename2_op=%ld  unique=%ld nodeid=%ld line=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,__LINE__,0,0);
  /* clang-format on */
  LOG(fshipcld, always) << "FUSE_RENAME2 invoked returning ENOSYS";
  return error_send(in, -ENOSYS);
}

__s64 FshipFuseOpsHandler::rename_op(inMsgGeneric *in) {
  __s64 retval = 0;
  inMsgRename *inMsg = (inMsgRename *)in;

  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, RENAME,  "rename_op=%ld  unique=%ld nodeid=%ld line=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,__LINE__,0,0);
  /* clang-format on */

  TrackPathFreeMsg trackPath(txp::FUSE_RENAME, inMsg->oldname);
  trackPath.setOffsetAttr2lastName(txp::offset2oldname);
  trackPath.addFuseHeaderItems(
      in); // in->hdr.nodeid is parent inode with oldname and path
  trackPath.addPath2Msg4lookup(_rootNodePtr, txp::oldname);

  int nextname = strlen(inMsg->oldname) + 1;
  char *newName = inMsg->oldname + nextname;

  TrackPathNoFreeMsg trackPath2(newName, trackPath.getMsgPtr());
  trackPath2.setOffsetAttr2lastName(txp::offset2newname);
  trackPath2.setParentInode(inMsg->newdirInode,
                            txp::newdirInode); // set parent inode and attr name
  trackPath2.addPath2Msg4lookup(_rootNodePtr, txp::newname);

  LOG(fshipcld, debug) << "RENAME in->hdr.nodeid olddir=" << in->hdr.nodeid
                       << " inMsg->newdirInode-" << inMsg->newdirInode
                       << " oldname=" << inMsg->oldname
                       << " newName=" << newName;

  ssize_t retSSize = trackPath.sendMsg(_connectPtr);
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    retval = error_send(in, -errno);
  }
  return retval;
}

__s64 FshipFuseOpsHandler::link_op(inMsgGeneric *in) {
  __s64 retval = 0;
  inMsgLink *inMsg = (inMsgLink *)in;

  LOG(fshipcld, debug) << "link_op new name=" << inMsg->newname;
  TrackPathFreeMsg trackPath(txp::FUSE_LINK, inMsg->newname);
  trackPath.addFuseHeaderItems(in);
  trackPath.addPath2Msg4lookup(_rootNodePtr, txp::newname);

  TrackPathNoFreeMsg trackPath2(trackPath.getMsgPtr());
  trackPath2.setNodeID(inMsg->link_in.oldnodeid);

  trackPath2.addUint64(txp::oldnodeid, inMsg->link_in.oldnodeid);
  uint64_t l_remoteNodeID = trackPath2.addFQPN(_rootNodePtr, txp::oldname);
  trackPath2.addUint64(txp::nodeid, l_remoteNodeID);
  LOG(fshipcld, debug) << "in->hdr.nodeid=" << in->hdr.nodeid
                       << " inMsg->link_in.oldnodeid="
                       << inMsg->link_in.oldnodeid
                       << " l_remoteNodeID=" << l_remoteNodeID;
  assert(l_remoteNodeID != 0);

  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, LINK,  "link_op=%ld  unique=%ld nodeid=%ld oldnodeid=%ld  remoteNodeID=%ld pid=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,inMsg->link_in.oldnodeid,l_remoteNodeID, in->hdr.pid);
  /* clang-format on */

  ssize_t retSSize = trackPath.sendMsg(_connectPtr);
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    retval = error_send(in, -errno);
  }
  return retval;
}

__s64 FshipFuseOpsHandler::symlink_op(inMsgGeneric *in) {
  __s64 retval = 0;
  inMsgSymlink *inMsg = (inMsgSymlink *)in;

  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, SYMLINK,  "link_op=%ld  unique=%ld nodeid=%ld line=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,__LINE__,0,0);
  /* clang-format on */
  LOG(fshipcld, debug) << "symlink_op new name=" << inMsg->newname;
  TrackPathFreeMsg trackPath(txp::FUSE_SYMLINK, inMsg->newname);
  trackPath.setOffsetAttr2lastName(txp::offset2newname);
  trackPath.addFuseHeaderItems(
      in); // in->hdr.nodeid is parent inode with oldname and path
  trackPath.addPath2Msg4lookup(_rootNodePtr, txp::newname);

  int nextname = strlen(inMsg->newname) + 1;
  char *oldName = inMsg->newname + nextname;
  trackPath.addString(txp::oldname, oldName);
  ssize_t retSSize = trackPath.sendMsg(_connectPtr);
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    retval = error_send(in, -errno);
  }
  return retval;
}
__s64 FshipFuseOpsHandler::forget_op(inMsgGeneric *in) {
  if (in->hdr.nodeid == FUSE_ROOT_ID)
    return 0;
  inMsgForget *inMsg = (inMsgForget *)in;
  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, FORGET,  "forget_op=%ld  unique=%ld nodeid=%ld line=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,__LINE__,0,0);
  /* clang-format on */
  _rootNodePtr->forgetNode(in->hdr.nodeid, inMsg->forget_in.nlookup);
  // no response sent for FORGET
  return 0;
}

__s64 FshipFuseOpsHandler::batchforget_op(inMsgGeneric *in) {
  inMsgBatchForget *inMsg = (inMsgBatchForget *)in;
  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, BATCHFORGET,  "batchforget_op=%ld  unique=%ld nodeid=%ld line=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,__LINE__,0,0);
  /* clang-format on */
  for (unsigned int i = 0; i < inMsg->forget_in.count; i++) {
    if (inMsg->forget[i].nodeid != FUSE_ROOT_ID) {
      _rootNodePtr->forgetNode(inMsg->forget[i].nodeid,
                               inMsg->forget[i].nlookup);
    }
  }
  // no response sent for FORGET
  return 0;
}
/*
struct fuse_kstatfs {
        uint64_t	blocks;
        uint64_t	bfree;
        uint64_t	bavail;
        uint64_t	files;
        uint64_t	ffree;
        uint32_t	bsize;
        uint32_t	namelen;
        uint32_t	frsize;
        uint32_t	padding;
        uint32_t	spare[6];
};
*/
__s64 FshipFuseOpsHandler::statfs_op(inMsgGeneric *in) {
  outMsgStatfs outMsg(in->hdr.unique);
  outMsg.kstatfs.blocks = ~0;
  outMsg.kstatfs.bfree = ~0;
  outMsg.kstatfs.bavail = ~0;
  outMsg.kstatfs.files = ~0;
  outMsg.kstatfs.namelen = 255;
  outMsg.kstatfs.frsize = 65536;
  outMsg.kstatfs.bsize = 65536;
  return (__s64)write(_deviceFD, &outMsg, outMsg.hdr.len);
}

__s64 FshipFuseOpsHandler::interrupt_op(inMsgGeneric *in) {
  __s64 retval = 0;
  inMsgInterrupt *inMsg = (inMsgInterrupt *)in;
  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, INTERRUPT,  "interrupt_op=%ld  unique=%ld nodeid=%ld interruptingUnique=%ld line=%ld ", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,inMsg->interrupt_in.unique,__LINE__,0);
  /* clang-format on */
  TrackPathFreeMsg trackPath(txp::FUSE_INTERRUPT);
  trackPath.addUint64(txp::unique, inMsg->interrupt_in.unique);
  trackPath.addFuseHeaderItems(in);

  ssize_t retSSize = trackPath.sendMsg(_connectPtr);
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    retval = error_send(in, -errno);
  }
  return retval;
  return 0;
};

__s64 FshipFuseOpsHandler::readdir_op(inMsgGeneric *in) {
  __s64 retval = 0;

  TrackPathFreeMsg trackPath(txp::FUSE_READDIR);
  trackPath.addFuseHeaderItems(in);

  inMsgReaddir *inMsg = (inMsgReaddir *)in;
  trackPath.addFuseReadIn(&inMsg->read_in);
  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, READDIR,  "readdir_op=%ld  unique=%ld nodeid=%ld line=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,__LINE__,0,0);
  /* clang-format on */

  ssize_t retSSize = trackPath.sendMsg(_connectPtr);
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    retval = error_send(in, -errno);
  }
  return retval;
}

__s64 FshipFuseOpsHandler::readdirplus_op(inMsgGeneric *in) {
  __s64 retval = 0;

  TrackPathFreeMsg trackPath(txp::FUSE_READDIRPLUS);
  trackPath.addFuseHeaderItems(in);

  inMsgReaddir *inMsg = (inMsgReaddir *)in;
  trackPath.addFuseReadIn(&inMsg->read_in);

  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, READDIRPLUS,  "readdirplus_op=%ld  unique=%ld nodeid=%ld line=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,__LINE__,0,0);
  /* clang-format on */
  ssize_t retSSize = trackPath.sendMsg(_connectPtr);
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    retval = error_send(in, -errno);
  }
  return retval;
}

__s64 FshipFuseOpsHandler::read_op(inMsgGeneric *in) {
  __s64 retval = 0;
  inMsgPread *inMsg = (inMsgPread *)in;

  TrackPathFreeMsg trackPath(txp::FUSE_READ);
  trackPath.addFuseHeaderItems(in);
  trackPath.addFuseReadIn(&inMsg->read_in);
  // if ( (int)inMsg->read_in.size>getpagesize() )
  // LOG(fshipcld,always)<<"inMsg->read_in.size
  // hex="<<std::hex<<inMsg->read_in.size<<std::dec<<"
  // dec="<<inMsg->read_in.size;
  /* clang-format off */ 
  FL_Write6(FL_fshipcldreadop, READOPIN,  "read_op=%ld  unique=%ld nodeid=%ld size=%d offset=%ld fh=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,inMsg->read_in.size,inMsg->read_in.offset,inMsg->read_in.fh);
  /* clang-format on */
  ssize_t retSSize = trackPath.sendMsg(_connectPtr);
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    retval = error_send(in, -errno);
  }
  return retval;
}

__s64 FshipFuseOpsHandler::read_op(memItemPtr mipfromRead) {
  __s64 retval = 0;

  inMsgGeneric *in = (inMsgGeneric *)mipfromRead->address;
  inMsgPread *inMsg = (inMsgPread *)in;

  if (_connectPtr->doSendImmediate(inMsg->read_in.size)) {
    return read_op(in);
  }
  /* clang-format off */ 
  FL_Write6(FL_fshipcldreadop, READOPMEM, "read_op=%ld  unique=%ld nodeid=%ld size=%d offset=%ld fh=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,inMsg->read_in.size,inMsg->read_in.offset,inMsg->read_in.fh);
  /* clang-format on */
  txp::Msg *l_Msg = 0;
  txp::Msg::buildMsg(txp::FUSE_READ, l_Msg);
  int l_RC = addFuseHeaderItems(l_Msg, in);

  memItemPtr mip4RDMAlocalBuffer = mipfromRead->next;
  mipfromRead->next = NULL;

  mip4RDMAlocalBuffer->next = mip4RDMAlocalBuffer; // put pointer in for
                                                   // response
  if (!l_RC)
    l_RC = l_Msg->addAttribute(txp::fuse_read_in, (char *)&inMsg->read_in,
                               sizeof(struct fuse_read_in));
  if (!l_RC)
    l_RC = l_Msg->addAttribute(txp::memItem, (char *)mip4RDMAlocalBuffer,
                               sizeof(memItem));
  if (l_RC)
    abort();

  ssize_t retSSize = _connectPtr->write(l_Msg);

  if (l_Msg) {
    delete l_Msg;
    l_Msg = NULL;
  }
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    return error_send(in, -errno);
  }

  return retval;
}

__s64 FshipFuseOpsHandler::write_op(inMsgGeneric *in) {
  abort();
  return 0;
}

__s64 FshipFuseOpsHandler::write_op(inMsgPwrite *inMsg, char *writeData) {
  inMsgGeneric *in = (inMsgGeneric *)inMsg;
  txp::Msg *l_Msg = 0;
  txp::Msg::buildMsg(txp::FUSE_WRITE, l_Msg);

  int l_RC = addFuseHeaderItems(l_Msg, in);
  // printf("write_op(inMsgPwrite>>>>write_op=%d  unique=%ld nodeid=%ld size=%d
  // offset=%ld fh=%ld\n", in->hdr.opcode, in->hdr.unique,
  // in->hdr.nodeid,inMsg->write_in.size,inMsg->write_in.offset,inMsg->write_in.fh);
  /* clang-format off */ 
  FL_Write6(fl_fshipcldwriteop, WRITEOPIN,  "write_op=%ld  unique=%ld nodeid=%ld size=%ld offset=%ld fh=%ld\n", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,inMsg->write_in.size,inMsg->write_in.offset,inMsg->write_in.fh);
  /* clang-format on */

  if (!l_RC)
    l_RC = l_Msg->addAttribute(txp::fuse_write_in, (char *)&inMsg->write_in,
                               sizeof(struct fuse_write_in));
  if (!l_RC)
    l_RC = l_Msg->addAttribute(txp::data, writeData, inMsg->write_in.size);
  if (l_RC)
    abort();

  ssize_t retSSize = _connectPtr->write(l_Msg);

  if (l_Msg) {
    delete l_Msg;
    l_Msg = NULL;
  }

  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    return error_send(in, -errno);
  }

  return 0;
}

__s64 FshipFuseOpsHandler::write_op(memItemPtr mipfromRead) {

  inMsgPwrite *inMsg = (inMsgPwrite *)mipfromRead->address;
  inMsgGeneric *in = (inMsgGeneric *)mipfromRead->address;
  txp::Msg *l_Msg = 0;
  txp::Msg::buildMsg(txp::FUSE_WRITE, l_Msg);
  int l_RC = addFuseHeaderItems(l_Msg, in);

  /* clang-format off */ 
  FL_Write6(FL_fshipcldwriteop, WRITEOPMEM,  "write_op=%ld  unique=%ld nodeid=%ld size=%ld offset=%ld fh=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,inMsg->write_in.size,inMsg->write_in.offset,inMsg->write_in.fh);
  /* clang-format on */

  if (inMsg->write_in.size > mipfromRead->next->length) {
    inMsg->write_in.size = mipfromRead->next->length;
    // \TODO: log
    abort(); // need to debug
  }

  memItemPtr mip4RDMAlocalBuffer = mipfromRead->next;
  mipfromRead->next = NULL;

  mip4RDMAlocalBuffer->next = mip4RDMAlocalBuffer; // put pointer in for
                                                   // response
  if (!l_RC)
    l_RC = l_Msg->addAttribute(txp::fuse_write_in, (char *)&inMsg->write_in,
                               sizeof(struct fuse_write_in));
  if (!l_RC)
    l_RC = l_Msg->addAttribute(txp::memItem, (char *)mip4RDMAlocalBuffer,
                               sizeof(memItem));
  if (l_RC)
    abort();

  ssize_t retval = 0;

  ssize_t retSSize = _connectPtr->writeWithBuffer(l_Msg, mip4RDMAlocalBuffer);

  if (l_Msg) {
    delete l_Msg;
    l_Msg = NULL;
  }
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    return error_send(in, -errno);
  }

  return retval;
}

__s64 FshipFuseOpsHandler::access_op(inMsgGeneric *in) {
  __s64 retval = 0;
  inMsgAccess *inMsg = (inMsgAccess *)in;

  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, ACCESSOP,  "access_op=%ld  unique=%ld nodeid=%ld line=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,__LINE__,0,0);
  /* clang-format on */
  TrackPathFreeMsg trackPath(txp::FUSE_ACCESS);
  trackPath.addFuseHeaderItems(in);
  if (trackPath.addFQPN(_rootNodePtr, txp::name) == 0) {
    retval = error_send(in, -ENOENT);
    return retval;
  }
  trackPath.addUint64(txp::mask, inMsg->access_in.mask);

  ssize_t retSSize = trackPath.sendMsg(_connectPtr);
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    retval = error_send(in, -errno);
  }
  return retval;
}

__s64 FshipFuseOpsHandler::fallocate_op(inMsgGeneric *in) {
  __s64 retval = 0;
  inMsgFallocate *inMsg = (inMsgFallocate *)in;
  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, FALLOCATEOP,  "fallocate_op=%ld  unique=%ld nodeid=%ld line=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,__LINE__,0,0);
  /* clang-format on */
  TrackPathFreeMsg trackPath(txp::FUSE_FALLOCATE);
  trackPath.addFuseHeaderItems(in);
  trackPath.addUint64(txp::fh, inMsg->fallocate_in.fh);
  trackPath.addUint64(txp::offset, inMsg->fallocate_in.offset);
  trackPath.addUint64(txp::length, inMsg->fallocate_in.length);
  trackPath.addUint32(txp::mode, inMsg->fallocate_in.mode);

  ssize_t retSSize = trackPath.sendMsg(_connectPtr);
  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    retval = error_send(in, -errno);
  }
  return retval;
}

__s64 FshipFuseOpsHandler::flush_op(inMsgGeneric *in) {
  inMsgFlush *inMsg = (inMsgFlush *)in;
  txp::Msg *l_Msg = 0;
  txp::Msg::buildMsg(txp::FUSE_FLUSH, l_Msg);
  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, FLUSHOP,  "flush_op=%ld  unique=%ld nodeid=%ld line=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,__LINE__,0,0);
  /* clang-format on */
  int l_RC =
      l_Msg->addAttribute(txp::inMsgFlush, (char *)inMsg, sizeof(inMsgFlush));
  if (l_RC)
    abort();
  ssize_t retSSize = _connectPtr->write(l_Msg);
  if (l_Msg) {
    delete l_Msg;
    l_Msg = NULL;
  }

  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    return error_send(in, -errno);
  }

  return 0;
}
__s64 FshipFuseOpsHandler::filelock_op(inMsgGeneric *in) {
  if (in)
    return error_send(in, -ENOSYS);
  inMsgLock *inMsg = (inMsgLock *)in;
  txp::Msg *l_Msg = 0;
  switch (in->hdr.opcode) {
  case FUSE_GETLK:
    txp::Msg::buildMsg(txp::FUSE_GETLK, l_Msg);
    /* clang-format off */ 
    FL_Write6(FL_fshipcldfuseop, GETLK,  "getlock_op=%ld  unique=%ld nodeid=%ld fh=%ld ownerkey=%ld flags=%ld(flock=1)", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,inMsg->lock_in.fh,inMsg->lock_in.owner,inMsg->lock_in.lk_flags);
    /* clang-format on */
    break;
  case FUSE_SETLK:
    txp::Msg::buildMsg(txp::FUSE_SETLK, l_Msg);
    /* clang-format off */ 
    FL_Write6(FL_fshipcldfuseop, SETLK,  "setlock_op=%ld  unique=%ld nodeid=%ld fh=%ld ownerkey=%ld flags=%ld(flock=1)", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,inMsg->lock_in.fh,inMsg->lock_in.owner,inMsg->lock_in.lk_flags);
    /* clang-format on */
    break;
  case FUSE_SETLKW:
    txp::Msg::buildMsg(txp::FUSE_SETLKW, l_Msg);
    /* clang-format off */ 
    FL_Write6(FL_fshipcldfuseop, SETLKW,  "setlockw_op=%ld  unique=%ld nodeid=%ld fh=%ld ownerkey=%ld flags=%ld(flock=1)", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,inMsg->lock_in.fh,inMsg->lock_in.owner,inMsg->lock_in.lk_flags);
    /* clang-format on */
    break;
  default:
    return error_send(in, -ENOSYS);
  }

  int l_RC =
      l_Msg->addAttribute(txp::inMsgLock, (char *)inMsg, sizeof(inMsgLock));
  if (l_RC)
    abort();
  ssize_t retSSize = _connectPtr->write(l_Msg);
  if (l_Msg) {
    delete l_Msg;
    l_Msg = NULL;
  }

  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    return error_send(in, -errno);
  }

  return 0;
}

__s64 FshipFuseOpsHandler::fsync_op(inMsgGeneric *in) {
  inMsgFsync *inMsg = (inMsgFsync *)in;
  txp::Msg *l_Msg = 0;
  txp::Msg::buildMsg(txp::FUSE_FSYNC, l_Msg);
  /* clang-format off */ 
  FL_Write6(FL_fshipcldfuseop, FSYNCOP,  "fsync_op=%ld  unique=%ld nodeid=%ld line=%ld", in->hdr.opcode, in->hdr.unique, in->hdr.nodeid,__LINE__,0,0);
  /* clang-format on */
  int l_RC =
      l_Msg->addAttribute(txp::inMsgFsync, (char *)inMsg, sizeof(inMsgSync));
  if (l_RC)
    abort();
  ssize_t retSSize = _connectPtr->write(l_Msg);
  if (l_Msg) {
    delete l_Msg;
    l_Msg = NULL;
  }

  if (retSSize == -1) {
    LOGERRNO(fshipcld, error, errno);
    return error_send(in, -errno);
  }

  return 0;
}

__s64 FshipFuseOpsHandler::fsyncdir_op(inMsgGeneric *in) {
  return error_send(in, -ENOSYS);
}

/* ssize_t read(int fd, void *buf, size_t count); */
ssize_t FshipFuseOpsHandler::readFuseMessage(char *buf, size_t bufSize) {

  inMsgGeneric *in = (inMsgGeneric *)buf;

  size_t readSize =
      sizeof(struct fuse_in_header) + sizeof(struct fuse_write_in);

  if (bufSize < readSize)
    return (-EINVAL);
  ssize_t retcode = read(_deviceFD, buf, bufSize);
  if (retcode == -1) {
    return -errno; // Let caller handle EINTR and EWOULDBLOCK
  }

  if (retcode == 0)
    return 0;
  // retcode > 0, of course
  last_unique++;
  if (retcode == in->hdr.len) {
    // clang-format off
        FL_Write6(FL_FUSE, FCL_INMSGFUSE,"op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid); 
    // clang-format on   
        if (last_unique != in->hdr.unique){
          // clang-format off 
          FL_Write(FL_FUSE, VFS_SKIPPED, "last_unique=%lld unique=%lld uid=%lld gid=%lld pid=%lld", last_unique,in->hdr.unique,0,0 );
      // clang-format on
    }
    last_unique = in->hdr.unique;
    return retcode;
  }
  if (retcode >= (signed)sizeof(struct fuse_in_header)) {
    if (last_unique != in->hdr.unique) {
      // clang-format off
          FL_Write(FL_FUSE, VFS_SKIPPED2, "last_unique=%lld unique=%lld uid=%lld gid=%lld pid=%lld", last_unique,in->hdr.unique,0,0 );
      // clang-format on
    }
    last_unique = in->hdr.unique;
    // clang-format off
        FL_Write6(FL_FUSE, VFS_READ2, "in->hdr.len=%lld, opcode=%lld unique=%lld uid=%lld gid=%lld pid=%lld", in->hdr.len, in->hdr.opcode,in->hdr.unique,in->hdr.uid,in->hdr.gid,in->hdr.pid );
    // clang-format on
  }

  if (in->hdr.len > bufSize) {
    // clang-format off
      FL_Write6(FL_FUSE, VFS_READE2BIG, "in->hdr.len=%lld, opcode=%lld unique=%lld uid=%lld gid=%lld pid=%lld", in->hdr.len, in->hdr.opcode,in->hdr.unique,in->hdr.uid,in->hdr.gid,in->hdr.pid );
    // clang-format on
    error_send(in, -E2BIG); /*interrupt finished */
    return -E2BIG;
  }
  bufSize = in->hdr.len - retcode;
  buf += retcode;
  retcode = read(_deviceFD, buf, bufSize);
  if (retcode == (ssize_t)bufSize)
    return (in->hdr.len);
  if ((retcode < 0) && (errno != EINTR))
    return -errno;
  printf("reading device is having troubles getting all the data.  Entering "
         "while loop\n");
  return -ENODATA;
};

ssize_t FshipFuseOpsHandler::readFuseMessage(memItemPtr mip) {
  struct iovec iov[2]; // expect 2 buffers
  int iovcnt = sizeof(iov) / sizeof(struct iovec);

  iov->iov_base = mip->address;
  iov->iov_len = sizeof(inMsgPwrite); // doing iovec to optimize for FUSE_WRITE
  inMsgGeneric *in = (inMsgGeneric *)mip->address;
  in->hdr.len = 0;
  in->hdr.opcode = ~0;

  memItemPtr l_mip2nd = mip->next;
  if (!l_mip2nd)
    abort(); // need the 2nd for pwrite

  iov[1].iov_base = l_mip2nd->address;
  iov[1].iov_len = l_mip2nd->chunkSize;

  ssize_t retcode = readv(_deviceFD, iov, iovcnt);

  if (retcode > 0) {
    if (retcode != in->hdr.len)
      abort(); // should not happen

    last_unique++; // use this to watch for missing operations since fuse will
                   // eat a unique ID if not enough buffer is provided and fails
                   // to dmesg as such
    if (last_unique != in->hdr.unique) {
      // clang-format off
      FL_Write(FL_FUSE, VFS_SKIPPED3, "last_unique=%lld unique=%lld uid=%lld gid=%lld pid=%lld", last_unique,in->hdr.unique,0,0 );
      // clang-format on
    }
    last_unique = in->hdr.unique;
    // clang-format off
    FL_Write6(FL_FUSE, FCL_INMSGFUSE2,"op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
    // clang-format on
    mip->length = sizeof(inMsgPwrite);
    // log, track unique, check for long enough header, etc.
    if (in->hdr.opcode == FUSE_WRITE) {
      l_mip2nd->length = retcode - mip->length;
    } else if (retcode > (signed)sizeof(inMsgPwrite)) { // fixup any overflow into 2nd buffer
      l_mip2nd->length = retcode - mip->length;
      memcpy(mip->address + mip->length, l_mip2nd->address, l_mip2nd->length);
      mip->length = retcode;
      l_mip2nd->length = 0;
    }
    return retcode;
  }
  if (retcode < 0) {
    if (errno != EINTR) {
      return -errno;
    }
  }
  return -ENODATA;
};

__s64 FshipFuseOpsHandler::writedeviceFD(outMsgGeneric *outMsg, uint32_t opcode) {
  // clang-format off
  FL_Write(FL_FUSE, FCL_BACK2FUSE,"op=%-3ld uniq=%-24ld len=%-12ld err=%-12ld  ", opcode,outMsg->hdr.unique,outMsg->hdr.len,outMsg->hdr.error);
  // clang-format on
  return (__s64)write(_deviceFD, outMsg, outMsg->hdr.len);
}
