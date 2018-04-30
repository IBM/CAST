/*******************************************************************************
 |    ResponseHandler.h
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

#ifndef RESPONSE_HANDLER_H
#define RESPONSE_HANDLER_H
//! \file
//! \brief

// Includes

#include "CnxSock.h"
#include "Log.h"
#include "Msg.h"
#include "Thread.h"
#include "fshipcld.h"
#include <errno.h>
#include <linux/fuse.h>
#include "NodeNameNetworkedRoot.h"

/*
txp::Attribute* retrieveAttr(const int32_t pNameValue);
        txp::Attribute* retrieveAttr(txp::AttributeName &pName);
        std::map<txp::AttributeName*, txp::Attribute*>* retrieveAttrs();
*/
typedef txp::Msg::AttributeMap AttrMap;
typedef AttrMap::iterator AttrMapIterator;

class ResponseHandler {
public:
  ResponseHandler(NodeNameNetworkedRoot *pRootNodePtr,
                  txp::ConnexPtr pConnectPtr, int &pFuseFD,
                  int &pPipe2FuseReader)
      : _HEADERLENGTH(txp::OFFSET_TO_FIRST_ATTRIBUTE),
        _rootNodePtr(pRootNodePtr), _connectPtr(pConnectPtr),
        _deviceFD(pFuseFD), _pipe2FuseReader(pPipe2FuseReader) {
    txp::Log _txplog(txp::Log::OPEN); // writes to stdout
    _clientMajor4Msg = 0;
    _clientMinor4Msg = 0;
    _clientMajor = 0;
    _clientMinor = 0;
    _clientPid = -1;
    _mountPath = NULL;
  }

  ~ResponseHandler() {
    if (_mountPath)
      delete[] _mountPath;
  }

  int readInMessage();
  const int _HEADERLENGTH; // txp::OFFSET_TO_FIRST_ATTRIBUTE in Common.h is the
                           // header length.
  void *run(); //!< method for running a thread when threads get coded
  //    void * wait4data();  //!< method for waiting for message response from
  //    partner

private:
  __s64 Hello(txp::Msg *pMsg);
  __s64 opendirOp(txp::Msg *pMsg);
  __s64 openOp(txp::Msg *pMsg);
  __s64 createOp(txp::Msg *pMsg);
  __s64 mkdirOp(txp::Msg *pMsg);
  __s64 mknodOp(txp::Msg *pMsg);
  __s64 readdirOp(txp::Msg *pMsg);
  __s64 readdirPlusOp(txp::Msg *pMsg);

  __s64 getattrOp(txp::Msg *pMsg);
  __s64 lookupOp(txp::Msg *pMsg);
  __s64 rmdirOp(txp::Msg *pMsg);
  __s64 unlinkOp(txp::Msg *pMsg);
  __s64 linkOp(txp::Msg *pMsg);
  __s64 renameOp(txp::Msg *pMsg);
  __s64 readOp(txp::Msg *pMsg);
  __s64 writeOp(txp::Msg *pMsg);
  __s64 accessOp(txp::Msg *pMsg);
  __s64 fallocateOp(txp::Msg *pMsg);
  __s64 setattrOp(txp::Msg *pMsg);
  __s64 symlinkOp(txp::Msg *pMsg);
  __s64 readlinkOp(txp::Msg *pMsg);
  __s64 interruptOp(txp::Msg *pMsg);
  __s64 releasedirOp(txp::Msg *pMsg);
  __s64 releaseOp(txp::Msg *pMsg);
  __s64 fsyncOp(txp::Msg *pMsg);
  __s64 flushOp(txp::Msg *pMsg);
  __s64 pollOp(txp::Msg *pMsg);

  __s64 setxattrOp(txp::Msg *pMsg);
  __s64 getxattrOp(txp::Msg *pMsg);
  __s64 listxattrOp(txp::Msg *pMsg);
  __s64 removexattrOp(txp::Msg *pMsg);

  __s64 setlkOp(txp::Msg *pMsg);
  __s64 setlkwOp(txp::Msg *pMsg);
  __s64 getlkOp(txp::Msg *pMsg);
  __s64 signal(txp::Msg *pMsg);

  __s64 writedeviceFD(outMsgGeneric *outMsg, uint32_t opcode);

  void dump2txplog(char *buff, int buffSize) {
    txp::Log::dump_buffer_raw(_txplog, buff, buffSize, "dumpbuffer");
  };
  void dumpMsgHeader(char *buff) {
    txp::Log::dump_buffer_raw(_txplog, buff, _HEADERLENGTH, "header");
  }

  /* data */
  NodeNameNetworkedRoot *_rootNodePtr;
  txp::ConnexPtr _connectPtr;
  uint32_t _clientMajor;
  uint32_t _clientMinor;
  uint64_t _clientPid;
  int _clientMajor4Msg;
  int _clientMinor4Msg;
  int &_deviceFD;
  int &_pipe2FuseReader;
  char *_mountPath;
  txp::Log _txplog;

  // for using file in future
  // txp::Log d_log(txp::Log::LOGFILE, "fshipcld.log");
  // d_log.open();

}; // endclass ResponseHandler

#endif // RESPONSE_HANDLER_H
