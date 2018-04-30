/*******************************************************************************
 |    MessageHandler.h
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

#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H
//!
//! \file   MessageHandler.h
//! \author Mike Aho <meaho@us.ibm.com>
//! \date   Fri Nov 11 09:13:58 2016
//! 
//! \brief  Handle incoming messages for function-shipping.
//! \defgroup fshipdMessageHandler fshipd Messagehandler
//! 
//! 
//!

/**
 \page fshipdMessageHandler_hl fshipd Messagehandler

The fshipd Messagehandler proceses function-ship request messages and sends a response.  The function-ship request will result in one or more
system calls (a/k/a syscalls) issued against a target directory as requested by the remote function-ship requestor (fshipd).

The requests are file-system function-ship requests.  These include:
(list to be added)

 \verbatim

 \endverbatim

 */


#include "CnxSock.h"
#include "Log.h"
#include "Msg.h"
#include "fshipcld.h"
#include <errno.h>
#include <linux/fuse.h>
#include <stddef.h>
#include <stdlib.h>

/*
txp::Attribute* retrieveAttr(const int32_t pNameValue);
        txp::Attribute* retrieveAttr(txp::AttributeName &pName);
        std::map<txp::AttributeName*, txp::Attribute*>* retrieveAttrs();
*/

#define FSHIPD_VERSIONSTR "Bringup" ///< Version string for fshipd

typedef txp::Msg::AttributeMap AttrMap;
typedef AttrMap::iterator AttrMapIterator;

/**
   \brief Fetch the version string
   \par Description
   The fshipd_GetVersion routine returns the version string for fshipd.
   This routine is intended for version mismatch debug.

   \param[in] pSize The amount of space provided to hold pVersion
   \param[out] pVersion The string containing the expected version.

   \return Error code
   \retval 0 Success
   \retval errno Positive non-zero values correspond with errno.  strerror() can
   be used to interpret.
*/
extern int fshipd_GetVersion(size_t pSize, char *pVersion);

//! \brief Handles messages from remote function-shipping daemon fshipcld
//!        and messages from any special interfaces like communications monitoring
class MessageHandler {
public:
  //! \brief Constructor using communications connector
  //! \param pConnectPtr [in] Communications connector
  //!
  MessageHandler(txp::ConnexPtr pConnectPtr)
      : _HEADERLENGTH(txp::OFFSET_TO_FIRST_ATTRIBUTE),
        _connectPtr(pConnectPtr) {
    txp::Log _txplog(txp::Log::OPEN); // writes to stdout
    _clientMajor4Msg = 0;
    _clientMinor4Msg = 0;
    _clientMajor = 0;
    _clientMinor = 0;
    _clientPid = -1;
    _mountPath = NULL;
    _generation =
        1; /* Inode generation: nodeid:gen must be unique for the fs's lifetime
              */
    _entry_valid = 60; /* Cache timeout for the name */
    _attr_valid = 120; /* Cache timeout for the attributes */
    _entry_valid_nsec = 1;
    _attr_valid_nsec = 1;
    sem_init(&threadSerialSem, 0, 1);
    _openOutFlags = FOPEN_DIRECT_IO; // FOPEN_DIRECT_IO;;// choices are in
                                     // fuse.h,.e.g FOPEN_DIRECT_IO
                                     // FOPEN_KEEP_CACHE FOPEN_NONSEEKABLE
  }
  //! \brief Destroy MessageHandler Object with appropriate cleanup
  //!
  ~MessageHandler() {
    if (_mountPath) {
      delete[] _mountPath;
      _mountPath = 0;
    }
  }
  //! \brief  Read in a message and handle
  //!
  //! \return 
  //!
  int readInMessage();
  const int _HEADERLENGTH; // txp::OFFSET_TO_FIRST_ATTRIBUTE in Common.h is the
                           // header length.
  void *run(); //!< method for running a thread when threads get coded
  void *run(unsigned numthreads); //!< method for running a thread when threads
                                  //!get coded
 private:
  //! \brief Take a stat struct and build a fuse_attr_out
  //! \param pStat [in] stat struct
  //! \param attrOut [out] fuse_attr_out 
  //!
  inline void setAttrOut(const struct stat &pStat, fuse_attr_out &attrOut)const {
    copyStat2attr_out(pStat, &attrOut.attr);
    attrOut.attr_valid = _attr_valid; /*seconds*/
    attrOut.attr_valid_nsec = _attr_valid_nsec;
    return;
  }
  //! \brief Take a stat struct and build a fuse_entry_out
  //! \param pStat [in] stat struct
  //! \param entry_out [out] fuse_entry_out
  //!
  inline void setEntryOut(const struct stat &pStat,
                          struct fuse_entry_out &entry_out)const {
    copyStat2attr_out(pStat, &entry_out.attr);
    entry_out.nodeid = pStat.st_ino;
    entry_out.attr_valid = _attr_valid; /*seconds*/
    entry_out.attr_valid_nsec = _attr_valid_nsec;
    entry_out.generation = _generation;
    entry_out.entry_valid = _entry_valid; // Cache timeout for the name
                                          // (seconds) see fuse module dir.c
    entry_out.entry_valid_nsec = _entry_valid_nsec;
    return;
  }
  //! \brief Handle message named by method
  //! \param pMsg 
  //!
  //! \return 0=Success, nonzero for failures
  //!
  int Hello(txp::Msg *pMsg); //!< \brief Handle message named by method
  int unsupportedOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  int opendirOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  int openOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  int releasedirOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  int releaseOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  int readdirOp(txp::Msg *pMsg);//!< \brief Handle message named by method

  int errOp(txp::Msg *pMsg, __u64 pUnique, int pErrno);

  int getattrOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  int lookupOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  int creatOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  int mknodOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  int mkdirOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  int renameOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  int readOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  int readOp(memItemPtr &pMip, txp::Msg *pMsg);//!< \brief Handle message named by method
  int readdirPlusOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  int writeOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  int writeOp(memItemPtr &pMip, txp::Msg *pMsg);//!< \brief Handle message named by method
  int accessOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  int unlinkatOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  int fallocateOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  int setattrOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  int symlinkOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  int linkOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  int readlinkOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  int interruptOp(txp::Msg *pMsg);//!< \brief Handle message named by method

  //! \brief Send an error response message for the inbound message
  //! \param pMsg [in] Message received
  //! \param in [in] Pointer to fuse_in_hdr
  //! \param error [in] errno to send back to message sender fshipcld
  //!
  //! \return 
  //!
  int sendErrorResponse(txp::Msg *pMsg, inMsgGeneric *in, int error);

  //! \brief Add lookup path name to the message with attribute
  //! \param pMsg The message where the lookup name is appending
  //! \param pPath The pathname 
  //! \param pLength The length of the pathname
  //! \param attrName txp attribute name to attach to the lookup 
  //!
  //! \return 
  //!
  int addlookupname(txp::Msg *pMsg, char *pPath, int pLength,
                    txp::AttributeName attrName);

  int fsyncOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  int flushOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  int pollOp(txp::Msg *pMsg);//!< \brief Handle message named by method

  int setlkOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  int getlkOp(txp::Msg *pMsg);

  __s64 setxattrOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  __s64 getxattrOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  __s64 listxattrOp(txp::Msg *pMsg); //!< \brief Handle message named by method
  __s64 removexattrOp(txp::Msg *pMsg);//!< \brief Handle message named by method
  int signal(txp::Msg *pMsg);//!< \brief Handle signal message sent by remote (not a function-ship/syscall)
  /* data */
  txp::ConnexPtr _connectPtr;
  uint32_t _clientMajor;
  uint32_t _clientMinor;
  uint64_t _clientPid;
  int _clientMajor4Msg;
  int _clientMinor4Msg;
  char *_mountPath;
  txp::Log _txplog;
  sem_t threadSerialSem;
  uint32_t _openOutFlags; //!< FOPEN_DIRECT_IO, etc settings back to fuse kernel

  //! \brief
  //! \param buff 
  //! \param buffSize 
  //!
  void dump2txplog(char *buff, int buffSize) {
    txp::Log::dump_buffer_raw(_txplog, buff, buffSize, "dumpbuffer");
  };
  //! \brief
  //! \param buff 
  //!
  void dumpMsgHeader(char *buff) {
    txp::Log::dump_buffer_raw(_txplog, buff, _HEADERLENGTH, "header");
  }

  uint64_t _generation;  /**< Inode generation: nodeid:gen must
                            be unique for the fs's lifetime */
  uint64_t _entry_valid; /**< Cache timeout for the name */
  uint64_t _attr_valid;  /**< Cache timeout for the attributes */
  uint32_t _entry_valid_nsec;
  uint32_t _attr_valid_nsec;
  // for using file in future
  // txp::Log d_log(txp::Log::LOGFILE, "fshipcld.log");
  // d_log.open();

}; // endclass MessageHandler

#endif // MESSAGE_HANDLER_H
