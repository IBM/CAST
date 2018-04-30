/*******************************************************************************
 |    FshipFuseOpsHandler.h
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
//! \file   FshipFuseOpsHandler.h
//! \author Mike Aho <meaho@us.ibm.com>
//! \date   Fri Nov 18 13:47:48 2016
//! 
//! \brief  Handle file operation requests from fuse module
//! 
//! 
//!

#ifndef __FSHIP_FUSE_OPS_HANDLER_H__
#define __FSHIP_FUSE_OPS_HANDLER_H__

#include "CnxSock.h"
#include "MemChunk.h"
#include "Msg.h"
#include "NodeNameNetworkedRoot.h"
#include "ResponseHandler.h"
#include "util.h"

const unsigned int MAX_INDEX_FUNC_OP = FUSE_RENAME2;
const unsigned int ARRAY_SIZE_FUNC_OP = MAX_INDEX_FUNC_OP + 1;

void set_doXattr(int pVal);

void set_fuseDirectIO(int pVal);



class FshipFuseOpsHandler  {
using FshipFuseOpfunc = __s64 (FshipFuseOpsHandler::*)(inMsgGeneric *in);
FshipFuseOpfunc _FshipFuseOp[ARRAY_SIZE_FUNC_OP]; 
public:
  FshipFuseOpsHandler(txp::ConnexPtr pConnectPtr, char *pMountDir,
                    char *pRemoteMount);
  FshipFuseOpsHandler();

  ~FshipFuseOpsHandler();

  char *_mountDir;

private:
  txp::ConnexPtr _connectPtr;
  char *_remotePath;

private:
  int _numChunks;
  uint32_t _numRDMAchunks;
  uint64_t _chunkSizeRead;

  txp::MemChunk *_readChunksPtr;

  NodeNameNetworkedRoot *_rootNodePtr;

  ResponseHandler *_responseHandlerPtr;

  __s64 sayHello(inMsgInit *inMsg);
  struct fuse_init_in
      _init_in; //!< fuse module init in of major, minor,flags, & max_readahed
public:
  void setConnectPtr(txp::ConnexPtr pConnectPtr) { _connectPtr = pConnectPtr; }
  void createNodeNameRoot(const char *pRemoteMount);
  void setNumRDMAchunks(uint32_t pNumRDMAchunks) {
    _numRDMAchunks = pNumRDMAchunks;
  }

  void setInit(struct fuse_init_in &pInit) {
    _init_in.major = pInit.major;
    _init_in.minor = pInit.minor;
    _init_in.max_readahead = pInit.max_readahead;
    _init_in.flags = pInit.flags;
  }
  int mountDevice(int pMax_read, const char *pMountDir);
  int readMonitorFuseDevice();
  __s64 unmountDevice();
  __s64 unmountDeviceLazy();
  __s64 processMsg(inMsgGeneric *in);
  __s64 init_op(inMsgGeneric *in);

 inline __s64 opcode_error(inMsgGeneric *in) {
    return error_send(in, -ENOSYS);
  }
 inline int getDeviceFD() { return _deviceFD; }
 
  __u64 last_unique;
  int _max_read;
  int _deviceFD;
  std::string _mountName;
  char mountParameters[512];

  inline __s64 error_send(inMsgGeneric *in, __s32 error) {
    outMsgGeneric err(in->hdr.unique, error);
    return writedeviceFD(&err, in->hdr.opcode);
  }
  inline __s64 sendOutMsg(const fuse_out_header &hdr) {
    __s64 retval = 0;
    retval = (__s64)write(_deviceFD, &hdr, hdr.len);
    return retval;
  }

  ssize_t readFuseMessage(char *buf, size_t bufSize);
  ssize_t readFuseMessage(memItemPtr mi);

  //! \brief  Send error back to module for request
  //! \param  unique ID of request
  //! \param  error  Send -errno back
  //! \return
  //! \note  The kernel module expects errno to be negative

  __s64 writedeviceFD(outMsgGeneric *outMsg, uint32_t opcode);

private:
  int addFuseHeaderItems(txp::Msg *pMsg, inMsgGeneric *in);
  int addPath(txp::Msg *pMsg, char *pPath, int pLength,
              txp::AttributeName attPathName = txp::name);
  int addFuseReadIn(txp::Msg *pMsg, struct fuse_read_in *read_in);

  __s64 lookup_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 getattr_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 readdir_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 readdirplus_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 opendir_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 releasedir_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 release_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 open_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 create_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 mknod_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 mkdir_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 rmdir_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 unlink_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 forget_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 rename_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 rename2_op(inMsgGeneric *in);//!< \brief Handle message named by method

  __s64 interrupt_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 read_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 read_op(memItemPtr mip);//!< \brief Handle message named by method
  __s64 write_op(inMsgGeneric *in);//!< \brief Handle message named by method

  __s64 write_op(memItemPtr mip);
  __s64 write_op(inMsgPwrite *inMsg, char *writeData);
  __s64 access_op(inMsgGeneric *in);//!< \brief Handle message named by method

  __s64 fsync_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 filelock_op(inMsgGeneric *in); //!<  get, set, setw

  __s64 setxattr_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 getxattr_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 listxattr_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 removexattr_op(inMsgGeneric *in);//!< \brief Handle message named by method

  __s64 fsyncdir_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 fallocate_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 setattr_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 batchforget_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 symlink_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 readlink_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 link_op(inMsgGeneric *in);//!< \brief Handle message named by method

  __s64 statfs_op(inMsgGeneric *in);//!< \brief Handle message named by method

  __s64 flush_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 ioctl_op(inMsgGeneric *in);//!< \brief Handle message named by method
  __s64 errENOSYS(inMsgGeneric *in);//!< \brief ENOSYS this message 

  int signal(siginfo_t &pSignalInfo);

  __s64 poll_op(inMsgGeneric *in);

  __s64 sayError(char *errorString);
  int _pipe_descriptor[2];

  //! \brief Return read descriptor of pipe
  int readPipeFd() { return _pipe_descriptor[0]; }

  //! \brief Return write descriptor of pipe
  int writePipeFd() { return _pipe_descriptor[1]; }

  static void *startProcessSignalPipe(void *object);
  void processSignalPipe();
  pthread_t _signalPipeThread;

  static void *startResponseHandler(void *object);
  void responseHandler();
  pthread_t _responseHandlerThread;

  enum readState {
    activeState = 0,
    quiescingState = 1,
    signalingState,
    disconnectPending,
    disconnectingState,
    disconnectedState
  };
  volatile int _fuseReadState;

};     // class definition
#endif /* __FSHIP_FUSE_OPS_HANDLER_H__ */
