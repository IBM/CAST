/*******************************************************************************
 |    ResponseHandler.cc
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

//! \file
//! \brief
// Includes
#include "ResponseHandler.h"
#include "CnxSock.h"
#include "Msg.h"
#include "fshipcld.h"
#include "fshipcld_flightlog.h"
#include "identity.h"

#ifndef GPFS_SUPER_MAGIC
#define GPFS_SUPER_MAGIC 0x47504653
#endif

__s64 ResponseHandler::opendirOp(txp::Msg *pMsg) {
  txp::Attribute *l_inHdrAttribute = pMsg->retrieveAttr(txp::outMsgOpen);
  if (l_inHdrAttribute) {
    outMsgOpen *outMsg = (outMsgOpen *)l_inHdrAttribute->getDataPtr();
    return writedeviceFD((outMsgGeneric *)outMsg, FUSE_OPENDIR);
  }
  l_inHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  outMsgGeneric *outMsg = (outMsgGeneric *)l_inHdrAttribute->getDataPtr();
  LOG(fshipcld, debug) << "OpenDir response errno=" << outMsg->hdr.error;
  __s64 retval = writedeviceFD((outMsgGeneric *)outMsg, FUSE_OPENDIR);
  return retval;
}

__s64 ResponseHandler::openOp(txp::Msg *pMsg) {
  txp::Attribute *l_inHdrAttribute = pMsg->retrieveAttr(txp::outMsgOpen);
  if (l_inHdrAttribute) {
    outMsgOpen *outMsg = (outMsgOpen *)l_inHdrAttribute->getDataPtr();
    return writedeviceFD((outMsgGeneric *)outMsg, FUSE_OPEN);
  }
  l_inHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  outMsgGeneric *outMsg = (outMsgGeneric *)l_inHdrAttribute->getDataPtr();
  __s64 retval = writedeviceFD((outMsgGeneric *)outMsg, FUSE_OPEN);
  return retval;
}

__s64 ResponseHandler::createOp(txp::Msg *pMsg) {
  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  if (l_outHdrAttribute) {
    outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();
    return writedeviceFD(outMsg, FUSE_CREATE);
  }
  txp::Attribute *l_inHdrAttribute = pMsg->retrieveAttr(txp::fuse_in_header);
  if (!l_inHdrAttribute) {
    LOGERRNO(fshipcld, error, ENOSYS);
    return -ENOSYS;
  }
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();

  l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgCreate);
  outMsgCreate *outMsg = (outMsgCreate *)l_outHdrAttribute->getDataPtr();

  // pick off inode numbers and names of directory entries
  const NodeNamePTR parent = _rootNodePtr->getNodeDir(in->hdr.nodeid);
  if (!parent)
    abort();

  txp::Attribute *createNameAttribute = pMsg->retrieveAttr(txp::lookupname);
  char *createName = NULL;

  if (createNameAttribute) {
    createName = (char *)createNameAttribute
                     ->getDataPtr(); // to get addressability to the data
  }
  if (createName) {
    _rootNodePtr->createChildNodeForResponse(&outMsg->entry_out, createName,
                                             parent);
  } else {
    abort();
  }
  /* clang-format off */ 
  FL_Write6(fl_fshipcldfusetx, FCL_CREATERESP,"create nodeid=%ld  generation=%ld  attr_ino=%ld  attr_size=%ld  fh=%ld  flags=0x%lx", outMsg->entry_out.nodeid, outMsg->entry_out.generation, outMsg->entry_out.attr.ino, outMsg->entry_out.attr.size, outMsg->open_out.fh, outMsg->open_out.open_flags);
  /* clang-format on */
  return writedeviceFD((outMsgGeneric *)outMsg, FUSE_CREATE);
}

__s64 ResponseHandler::renameOp(txp::Msg *pMsg) {
  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();
  if (outMsg->hdr.error) {
    return writedeviceFD(outMsg, FUSE_RENAME);
  }

  txp::Attribute *l_inHdrAttribute = pMsg->retrieveAttr(txp::fuse_in_header);
  if (!l_inHdrAttribute) {
    LOGERRNO(fshipcld, error, EIO);
    return writedeviceFD(outMsg, -EIO);
  }
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  if (!in)
    abort(); // shut up compile warning for non-use of in--but log in the flight
             // log that we got a response here...

  // uint64_t olddirinode = in->hdr.nodeid;
  uint64_t newdirInode = 0;
  txp::Attribute *l_newdirInode = pMsg->retrieveAttr(txp::newdirInode);
  if (l_newdirInode) {
    int l_RC = l_newdirInode->cpyData(&newdirInode, sizeof(newdirInode));
    if (l_RC)
      abort();
    if (!newdirInode)
      abort();
  } else
    abort();

  txp::Attribute *l_statAttribute = pMsg->retrieveAttr(txp::stat4Name);
  struct stat *l_statPtr = (struct stat *)l_statAttribute->getDataPtr();

  txp::Attribute *l_newNameAttribute = pMsg->retrieveAttr(txp::newname);
  char *name = (char *)l_newNameAttribute->getDataPtr();

  _rootNodePtr->renameNodeResponse(l_statPtr, name, newdirInode);

  __s64 retval = writedeviceFD(outMsg, in->hdr.opcode);
  return retval;
}

__s64 ResponseHandler::symlinkOp(txp::Msg *pMsg) {

  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  if (l_outHdrAttribute) {
    outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();
    return (__s64)write(_deviceFD, outMsg, outMsg->hdr.len);
  }

  txp::Attribute *l_inHdrAttribute = pMsg->retrieveAttr(txp::fuse_in_header);
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();

  l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgSymlink);
  outMsgSymlink *outMsg = (outMsgSymlink *)l_outHdrAttribute->getDataPtr();
  l_outHdrAttribute = pMsg->retrieveAttr(txp::newname);
  char *createName = (char *)l_outHdrAttribute->getDataPtr();
  if (createName) {
    const NodeNamePTR parent = _rootNodePtr->getNodeDir(in->hdr.nodeid);
    _rootNodePtr->createChildNodeForResponse(&outMsg->entry_out, createName,
                                             parent);
  } else {
    abort();
  }
  __s64 retval = (__s64)write(_deviceFD, outMsg, outMsg->hdr.len);
  return retval;
}

__s64 ResponseHandler::setxattrOp(txp::Msg *pMsg) {

  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  if (l_outHdrAttribute) {
    outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();
    LOG(fshipcld, debug) << "setxattr response unique=" << outMsg->hdr.unique
                         << " errno=" << (-outMsg->hdr.error) << ":"
                         << strerror(-outMsg->hdr.error);
    /* clang-format off */ 
    FL_Write(fl_fshipcldfusetx, FCL_SETXATTRRESP,"setxattr unique==%ld len=%ld err=%ld LINE=%ld" , outMsg->hdr.unique, outMsg->hdr.len, outMsg->hdr.error,__LINE__);
    /* clang-format on */
    return (__s64)write(_deviceFD, outMsg, outMsg->hdr.len);
  } else
    abort();
  return 0;
}

__s64 ResponseHandler::getxattrOp(txp::Msg *pMsg) {
  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  if (l_outHdrAttribute) {
    outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();
    // if (ENODATA != abs(outMsg->hdr.error) ) LOG(fshipcld,info)<<"getxattr
    // unique="<<outMsg->hdr.unique<<"
    // errno="<<(-outMsg->hdr.error)<<":"<<strerror(-outMsg->hdr.error);
    /* clang-format off */ 
    FL_Write(fl_fshipcldfusetx, FCL_GETXATTRRESP,"getxattr unique==%ld len=%ld err=%ld LINE=%ld" , outMsg->hdr.unique, outMsg->hdr.len, outMsg->hdr.error,__LINE__);
    /* clang-format on */
    return (__s64)write(_deviceFD, outMsg, outMsg->hdr.len);
  }
  __s64 retval = 0;

  txp::Attribute *l_outMsgGetXattrAttribute =
      pMsg->retrieveAttr(txp::outMsgGetXattr);
  outMsgGetXattr *outMsgPtr =
      (outMsgGetXattr *)l_outMsgGetXattrAttribute->getDataPtr();

  txp::Attribute *l_outMsgGetXattrData = NULL;
  if (outMsgPtr->getxattr_out.size)
    l_outMsgGetXattrData = pMsg->retrieveAttr(txp::attributeData);

  if (l_outMsgGetXattrData) {
    char *l_xattrData = (char *)l_outMsgGetXattrData->getDataPtr();

    // Send response to fuse module using message followed by data
    struct iovec iov[2];
    iov->iov_base = outMsgPtr;
    iov->iov_len = sizeof(
        outMsgGeneric); // fuse kernel not using gettattr_out in this case
    iov[1].iov_base = l_xattrData;
    iov[1].iov_len = outMsgPtr->getxattr_out.size;
    outMsgPtr->hdr.len =
        iov->iov_len +
        iov[1].iov_len; // correct header len for not including getxattr_out
    int iovcnt = sizeof(iov) / sizeof(struct iovec);
    retval = writev(_deviceFD, iov, iovcnt);
  } else {
    retval = (__s64)write(_deviceFD, outMsgPtr, outMsgPtr->hdr.len);
  }
  return retval;
}

__s64 ResponseHandler::removexattrOp(txp::Msg *pMsg) {

  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  if (l_outHdrAttribute) {
    outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();
    // LOG(fshipcld,debug)<<"removexattr response
    // unique="<<outMsg->hdr.unique<<"
    // errno="<<(-outMsg->hdr.error)<<":"<<strerror(-outMsg->hdr.error);
    /* clang-format off */ 
    FL_Write(fl_fshipcldfusetx, FCL_RMVXATTRRESP,"removexattr unique==%ld len=%ld err=%ld LINE=%ld" , outMsg->hdr.unique, outMsg->hdr.len, outMsg->hdr.error,__LINE__);
    /* clang-format on */
    return (__s64)write(_deviceFD, outMsg, outMsg->hdr.len);
  } else
    abort();
  return 0;
}

__s64 ResponseHandler::listxattrOp(txp::Msg *pMsg) {

  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  if (l_outHdrAttribute) {
    outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();
    if (ENODATA != abs(outMsg->hdr.error))
      LOG(fshipcld, info) << "listxattr unique=" << outMsg->hdr.unique
                          << " errno=" << (-outMsg->hdr.error) << ":"
                          << strerror(-outMsg->hdr.error);
    return (__s64)write(_deviceFD, outMsg, outMsg->hdr.len);
  }
  __s64 retval = 0;

  txp::Attribute *l_outMsgGetXattrAttribute =
      pMsg->retrieveAttr(txp::outMsgGetXattr);
  outMsgGetXattr *outMsgPtr =
      (outMsgGetXattr *)l_outMsgGetXattrAttribute->getDataPtr();

  txp::Attribute *l_outMsgGetXattrData = NULL;
  if (outMsgPtr->getxattr_out.size)
    l_outMsgGetXattrData = pMsg->retrieveAttr(txp::attributeData);

  if (l_outMsgGetXattrData) {
    char *l_xattrData = (char *)l_outMsgGetXattrData->getDataPtr();
    LOG(fshipcld, debug) << "listxattr for unique=" << outMsgPtr->hdr.unique
                         << " value=" << l_xattrData << " ::buffer size="
                         << " size=" << outMsgPtr->getxattr_out.size;
    // Send response to fuse module using message followed by data
    struct iovec iov[2];
    iov->iov_base = outMsgPtr;
    iov->iov_len = sizeof(outMsgGetXattr);
    iov[1].iov_base = l_xattrData;
    iov[1].iov_len = outMsgPtr->getxattr_out.size;
    outMsgPtr->hdr.len += outMsgPtr->getxattr_out.size;
    int iovcnt = sizeof(iov) / sizeof(struct iovec);
    retval = writev(_deviceFD, iov, iovcnt);
  } else {
    retval = (__s64)write(_deviceFD, outMsgPtr, outMsgPtr->hdr.len);
  }
  return retval;
}

__s64 ResponseHandler::rmdirOp(txp::Msg *pMsg) {
  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();
  if (outMsg->hdr.error) {
    return (__s64)write(_deviceFD, outMsg, outMsg->hdr.len);
  }
  txp::Attribute *l_inHdrAttribute = pMsg->retrieveAttr(txp::fuse_in_header);
  if (!l_inHdrAttribute) {
    LOGERRNO(fshipcld, error, ENOSYS);
    return -ENOSYS;
  }

  if (!outMsg->hdr.error) { // get the nodeid and remove from the map
    uint64_t oldnodeid = 0;
    txp::Attribute *l_inFlagsAttribute = pMsg->retrieveAttr(txp::oldnodeid);
    if (l_inFlagsAttribute) {
      int l_RC = l_inFlagsAttribute->cpyData(&oldnodeid, sizeof(oldnodeid));
      if (l_RC)
        abort();
    }
  }

  __s64 retval = (__s64)write(_deviceFD, outMsg, outMsg->hdr.len);
  return retval;
}

__s64 ResponseHandler::unlinkOp(txp::Msg *pMsg) {

  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();
  __s64 retval = (__s64)write(_deviceFD, outMsg, outMsg->hdr.len);
  return retval;
}

__s64 ResponseHandler::linkOp(txp::Msg *pMsg) {

  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  if (l_outHdrAttribute) {
    outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();
    return writedeviceFD(outMsg, FUSE_LINK);
  }
  txp::Attribute *l_inHdrAttribute = pMsg->retrieveAttr(txp::fuse_in_header);
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();

  txp::Attribute *l_inOutMsgAttribute = pMsg->retrieveAttr(txp::outMsgLink);
  outMsgLink *outMsg = (outMsgLink *)l_inOutMsgAttribute->getDataPtr();

  uint64_t prevRemoteInode = 0;
  txp::Attribute *l_AttrPrevRemoteInode = pMsg->retrieveAttr(txp::nodeid);
  int l_RC =
      l_AttrPrevRemoteInode->cpyData(&prevRemoteInode, sizeof(prevRemoteInode));
  assert(prevRemoteInode != 0);
  assert(l_RC == 0);

  uint64_t oldInode = 0;
  txp::Attribute *l_AttrFuseOldInode = pMsg->retrieveAttr(txp::oldnodeid);
  l_RC = l_AttrFuseOldInode->cpyData(&oldInode, sizeof(oldInode));
  assert(oldInode != 0);

#if 0
     if (prevRemoteInode != outMsg->entry_out.attr.ino){//lock and update if inode number changed (funky)
           uint64_t  updateRC = _rootNodePtr->updateChildNodeEntryResponse(outMsg->entry_out,oldInode);//struct fuse_attr attr;
           if (updateRC){
             outMsgGeneric outMsgError(in->hdr.unique,-EIO);
             writedeviceFD ( &outMsgError, in->hdr.opcode);
             assert(updateRC==0);
           }
        }
        else { // need to use the local fuse inode value
           outMsg->entry_out.attr.ino = in->hdr.nodeid;  
           outMsg->entry_out.nodeid = outMsg->entry_out.attr.ino;
        }
     }
     else {// need to use the local fuse inode value
        outMsg->entry_out.attr.ino = in->hdr.nodeid; 
        outMsg->entry_out.nodeid = outMsg->entry_out.attr.ino;
     }
#endif
  outMsg->entry_out.attr.ino = oldInode;
  outMsg->entry_out.nodeid = oldInode;

  // presume oldinode still exists in map entries for old name ....
  __s64 retval = writedeviceFD((outMsgGeneric *)outMsg, in->hdr.opcode);
  return retval;
}

__s64 ResponseHandler::mkdirOp(txp::Msg *pMsg) {
  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  if (l_outHdrAttribute) {
    outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();
    /* clang-format off */ 
    FL_Write(fl_fshipcldfusetx, MKDIRRESP,"setattr unique==%ld len=%ld err=%ld LINE=%ld" , outMsg->hdr.unique, outMsg->hdr.len, outMsg->hdr.error,__LINE__);
    /* clang-format on */
    return (__s64)write(_deviceFD, outMsg, outMsg->hdr.len);
  }
  txp::Attribute *l_inHdrAttribute = pMsg->retrieveAttr(txp::fuse_in_header);
  if (!l_inHdrAttribute) {
    LOGERRNO(fshipcld, error, ENOSYS);
    return -ENOSYS;
  }
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();

  l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgCreate);
  outMsgMkdir *outMsg = (outMsgMkdir *)l_outHdrAttribute->getDataPtr();

  // pick off inode numbers and names of directory entries
  const NodeNamePTR parent = _rootNodePtr->getNodeDir(in->hdr.nodeid);
  if (!parent)
    abort();

  txp::Attribute *createNameAttribute = pMsg->retrieveAttr(txp::lookupname);
  char *createName = NULL;

  if (createNameAttribute) {
    createName = (char *)createNameAttribute
                     ->getDataPtr(); // to get addressability to the data
  }
  if (createName) {
    _rootNodePtr->createChildNodeForResponse(&outMsg->entry_out, createName,
                                             parent);

  } else {
    abort();
  }
  /* clang-format off */ 
  FL_Write6(fl_fshipcldfusetx, FCL_MKDIRRESP,"mkdir fuseNodeid==%ld  entry_valid=%ld nsec=%ld  attr_valid=%ld  nsec=%ld attr.ino=%ld", outMsg->entry_out.nodeid, outMsg->entry_out.entry_valid, outMsg->entry_out.entry_valid_nsec, outMsg->entry_out.attr_valid, outMsg->entry_out.attr_valid_nsec, outMsg->entry_out.attr.ino);
  /* clang-format on */

  __s64 retval = (__s64)writedeviceFD((outMsgGeneric *)outMsg, in->hdr.opcode);
  return retval;
}

__s64 ResponseHandler::mknodOp(txp::Msg *pMsg) {
  txp::Attribute *l_inHdrAttribute = pMsg->retrieveAttr(txp::fuse_in_header);
  if (!l_inHdrAttribute) {
    l_inHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
    outMsgGeneric *outMsg = (outMsgGeneric *)l_inHdrAttribute->getDataPtr();
    return writedeviceFD(outMsg, FUSE_MKNOD);
  }

  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();

  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgMknod);
  outMsgMknod *outMsg = (outMsgMknod *)l_outHdrAttribute->getDataPtr();

  // pick off inode numbers and names of directory entries
  const NodeNamePTR parent = _rootNodePtr->getNodeDir(in->hdr.nodeid);
  if (!parent)
    abort();

  txp::Attribute *createNameAttribute = pMsg->retrieveAttr(txp::lookupname);
  char *createName = NULL;

  if (createNameAttribute) {
    createName = (char *)createNameAttribute
                     ->getDataPtr(); // to get addressability to the data
  }
  if (createName) {
    _rootNodePtr->createChildNodeForResponse(&outMsg->entry_out, createName,
                                             parent);

  } else {
    abort();
  }
  /* clang-format off */ 
  FL_Write6(fl_fshipcldfusetx, FCL_MKNODRESP,"mknod fuseNodeid==%ld  entry_valid=%ld nsec=%ld  attr_valid=%ld  nsec=%ld attr.ino=%ld", outMsg->entry_out.nodeid, outMsg->entry_out.entry_valid, outMsg->entry_out.entry_valid_nsec, outMsg->entry_out.attr_valid, outMsg->entry_out.attr_valid_nsec, outMsg->entry_out.attr.ino);
  /* clang-format on */

  __s64 retval = (__s64)writedeviceFD((outMsgGeneric *)outMsg, in->hdr.opcode);
  return retval;
}
__s64 ResponseHandler::interruptOp(txp::Msg *pMsg) {
  txp::Attribute *l_inHdrAttribute = pMsg->retrieveAttr(txp::fuse_in_header);
  if (!l_inHdrAttribute) {
    LOGERRNO(fshipcld, error, ENOSYS);
    return -ENOSYS;
  }
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  outMsgGeneric outMsg(in->hdr.unique);
  __s64 retval = (__s64)write(_deviceFD, &outMsg, outMsg.hdr.len);
  return retval;
}

__s64 ResponseHandler::fsyncOp(txp::Msg *pMsg) {
  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();
  __s64 retval = writedeviceFD(outMsg, FUSE_FSYNC);
  return retval;
}

__s64 ResponseHandler::flushOp(txp::Msg *pMsg) {
  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();
  __s64 retval = writedeviceFD(outMsg, FUSE_FLUSH);
  return retval;
}

__s64 ResponseHandler::pollOp(txp::Msg *pMsg) {
  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgPoll);
  outMsgPoll *outMsgP = (outMsgPoll *)l_outHdrAttribute->getDataPtr();
  if (outMsgP->hdr.error) {
    /* clang-format off */ 
    FL_Write(fl_fshipcldfusetx, POLLENO,"op=%-3ld uniq=%-24ld len=%-12ld error=%-12ld",FUSE_POLL,outMsgP->hdr.unique,outMsgP->hdr.len,outMsgP->hdr.error);
    /* clang-format on */
  } else {
    /* clang-format off */ 
    FL_Write(fl_fshipcldfusetx, POLLRSP,"op=%-3ld uniq=%-24ld len=%-12ld revents=%-12ld",FUSE_POLL,outMsgP->hdr.unique,outMsgP->hdr.len,outMsgP->poll_out.revents);
    /* clang-format on */
  }
  outMsgGeneric *outMsg = (outMsgGeneric *)outMsgP;
  __s64 retval = writedeviceFD(outMsg, FUSE_POLL);
  if (!retval) {
    l_outHdrAttribute = pMsg->retrieveAttr(txp::fuse_notify_poll_wakeup_out);
    if (l_outHdrAttribute) {
      struct fuse_notify_poll_wakeup_out *wakeup_outPtr =
          (struct fuse_notify_poll_wakeup_out *)l_outHdrAttribute->getDataPtr();
      OutMsgPollNotify outMsgPollNotify(wakeup_outPtr->kh);
      retval =
          writedeviceFD((outMsgGeneric *)&outMsgPollNotify, FUSE_NOTIFY_POLL);
    }
  }
  return retval;
}

__s64 ResponseHandler::Hello(txp::Msg *pMsg) {
  __s64 retval = 0;
  txp::Version l_Version = pMsg->getVersion();
  _clientMajor4Msg = l_Version.getMajor();
  _clientMinor4Msg = l_Version.getMinor();
  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  if (l_outHdrAttribute) {
    outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();
    retval = writedeviceFD(outMsg, FUSE_INIT);
    LOG(fshipcld, always) << "FUSE_INIT failed with fshipd error="
                          << abs(outMsg->hdr.error) << ":"
                          << strerror(abs(outMsg->hdr.error));
    assert_perror(abs(outMsg->hdr.error));
    exit(outMsg->hdr.error);
    return retval;
  }
  txp::Attribute *l_inHdrAttribute = pMsg->retrieveAttr(txp::fuse_in_header);
  if (!l_inHdrAttribute) {
    LOGERRNO(fshipcld, error, ENOSYS);
    return -ENOSYS;
  }
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  l_inHdrAttribute = pMsg->retrieveAttr(txp::outHello);
  outHello *HelloResponse = NULL;
  if (l_inHdrAttribute) {
    HelloResponse = (outHello *)l_inHdrAttribute->getDataPtr();
    LOG(fshipcld, always) << "Hello Response Fuse major="
                          << HelloResponse->fuse_version_major
                          << " minor=" << HelloResponse->fuse_version_minor;
    if (HelloResponse->statfsRC) {
      LOG(fshipcld, critical)
          << "remote statfs errno=" << HelloResponse->statfsRC << ":"
          << strerror(HelloResponse->statfsRC);
    } else {
      std::string fstype(":UNKNOWN");
      if (HelloResponse->sfs.f_type == GPFS_SUPER_MAGIC)
        fstype = ":GPFS";
      LOG(fshipcld, always)
          << std::hex << " fs type=0x" << HelloResponse->sfs.f_type << fstype
          << " Optimal xfer block size=0x" << (int)HelloResponse->sfs.f_bsize
          << " mount flags=0x" << (int)HelloResponse->sfs.f_flags << std::dec;
    }
  }
  outMsgInit outMsg(in->hdr.unique, _connectPtr->getRDMAchunkSize());
  // outMsg.init_out.max_write = _connectPtr->getRDMAchunkSize();
  LOG(fshipcld, always) << "Hello outMsgInit Fuse major="
                        << outMsg.init_out.major
                        << " minor=" << outMsg.init_out.minor;
  LOG(fshipcld, always) << "max_readahead=" << outMsg.init_out.max_readahead
                        << " max_background=" << outMsg.init_out.max_background
                        << " congestion_threshold="
                        << outMsg.init_out.congestion_threshold
                        << " max_write=" << outMsg.init_out.max_write;
  dump_init_flags(outMsg.init_out.flags);
  retval = writedeviceFD((outMsgGeneric *)&outMsg, FUSE_INIT);
  return retval;
};

__s64 ResponseHandler::setattrOp(txp::Msg *pMsg) {
  __s64 retval = 0;
  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  if (l_outHdrAttribute) {
    outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();
    /* clang-format off */ 
    FL_Write(fl_fshipcldfusetx, FCL_SETATTRRESP,"setattr unique==%ld len=%ld err=%ld LINE=%ld" , outMsg->hdr.unique, outMsg->hdr.len, outMsg->hdr.error,__LINE__);
    /* clang-format on */
    return (__s64)write(_deviceFD, outMsg, outMsg->hdr.len);
  }
  // bounce back fuse_in_header for nodeid and perhaps other tracking
  txp::Attribute *l_inHdrAttribute = pMsg->retrieveAttr(txp::fuse_in_header);
  if (!l_inHdrAttribute) {
    LOGERRNO(fshipcld, error, ENOSYS);
    return -ENOSYS;
  }
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  l_inHdrAttribute = pMsg->retrieveAttr(txp::outMsgSetattr);
  if (l_inHdrAttribute) {
    outMsgSetattr *outMsg = (outMsgSetattr *)l_inHdrAttribute->getDataPtr();
    uint64_t prevRemoteInode = 0;
    txp::Attribute *l_AttrPrevRemoteInode = pMsg->retrieveAttr(txp::nodeid);
    if (l_AttrPrevRemoteInode) {
      int l_RC = l_AttrPrevRemoteInode->cpyData(&prevRemoteInode,
                                                sizeof(prevRemoteInode));
      if (l_RC)
        abort();
    }
    // LOG(fshipcld,always)<<"Getattr inode="<<outMsg->getattr_out.attr.ino<<"
    // prevRemoteInode="<<prevRemoteInode;
    if (prevRemoteInode !=
        outMsg->attr_out.attr
            .ino) { // lock and update if inode number changed (funky)
      uint64_t updateRC = _rootNodePtr->updateChildNodeGetattrResponse(
          outMsg->attr_out, in->hdr.nodeid); // struct fuse_attr attr;
      if (updateRC) {
        outMsg->hdr.error = updateRC;
      }
    } else {
      outMsg->attr_out.attr.ino =
          in->hdr.nodeid; // need to use the local fuse inode value
    }
    if (in->hdr.nodeid == FUSE_ROOT_ID) {
      outMsg->attr_out.attr_valid =
          600; // 10 minutes on attribute refreshes for root node
      outMsg->attr_out.attr_valid_nsec = 1;
    }
    retval = writedeviceFD((outMsgGeneric *)outMsg, FUSE_GETATTR);
  } else {
    l_inHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
    outMsgGeneric outMsg(in->hdr.unique, -EIO);
    retval = writedeviceFD(&outMsg, FUSE_GETATTR);
    LOGERRNO(fshipcld, error, EIO);
    abort();
  }
  return retval;
}

__s64 ResponseHandler::getattrOp(txp::Msg *pMsg) {
  __s64 retval = 0;
  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  if (l_outHdrAttribute) {
    outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();
    /* clang-format off */ 
    FL_Write(fl_fshipcldfusetx, FCL_GETATTRRESP,"getattr unique==%ld len=%ld err=%ld LINE=%ld" , outMsg->hdr.unique, outMsg->hdr.len, outMsg->hdr.error,__LINE__);
    /* clang-format on */
    return (__s64)write(_deviceFD, outMsg, outMsg->hdr.len);
  }
  // bounce back fuse_in_header for nodeid and perhaps other tracking
  txp::Attribute *l_inHdrAttribute = pMsg->retrieveAttr(txp::fuse_in_header);
  if (!l_inHdrAttribute) {
    LOGERRNO(fshipcld, error, ENOSYS);
    return -ENOSYS;
  }
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  l_inHdrAttribute = pMsg->retrieveAttr(txp::outMsgGetattr);
  if (l_inHdrAttribute) {
    outMsgGetattr *outMsg = (outMsgGetattr *)l_inHdrAttribute->getDataPtr();
    uint64_t prevRemoteInode = 0;
    txp::Attribute *l_AttrPrevRemoteInode = pMsg->retrieveAttr(txp::nodeid);
    if (l_AttrPrevRemoteInode) {
      int l_RC = l_AttrPrevRemoteInode->cpyData(&prevRemoteInode,
                                                sizeof(prevRemoteInode));
      if (l_RC)
        abort();
    }
    // LOG(fshipcld,always)<<"Getattr inode="<<outMsg->getattr_out.attr.ino<<"
    // prevRemoteInode="<<prevRemoteInode;
    if (prevRemoteInode !=
        outMsg->getattr_out.attr
            .ino) { // lock and update if inode number changed (funky)
      uint64_t updateRC = _rootNodePtr->updateChildNodeGetattrResponse(
          outMsg->getattr_out, in->hdr.nodeid); // struct fuse_attr attr;
      if (updateRC) {
        outMsg->hdr.error = updateRC;
      }
    } else {
      outMsg->getattr_out.attr.ino =
          in->hdr.nodeid; // need to use the local fuse inode value
    }
    if (in->hdr.nodeid == FUSE_ROOT_ID) {
      outMsg->getattr_out.attr_valid =
          600; // 10 minutes on attribute refreshes for root node
      outMsg->getattr_out.attr_valid_nsec = 1;
    }
    retval = writedeviceFD((outMsgGeneric *)outMsg, FUSE_GETATTR);
  } else {
    l_inHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
    outMsgGeneric outMsg(in->hdr.unique, -EIO);
    retval = writedeviceFD(&outMsg, FUSE_GETATTR);
    abort();
  }
  return retval;
}

__s64 ResponseHandler::lookupOp(txp::Msg *pMsg) {
  // bounce back fuse_in_header for nodeid and perhaps other tracking
  txp::Attribute *l_inHdrAttribute = pMsg->retrieveAttr(txp::fuse_in_header);
  if (!l_inHdrAttribute) {
    l_inHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
    outMsgGeneric *outMsg = (outMsgGeneric *)l_inHdrAttribute->getDataPtr();
    return writedeviceFD(outMsg, FUSE_LOOKUP);
  }
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  // pick off inode numbers and names of directory entries
  const NodeNamePTR parent = _rootNodePtr->getNodeDir(in->hdr.nodeid);
  if (!parent)
    abort();

  txp::Attribute *l_inOutMsgAttribute = pMsg->retrieveAttr(txp::outMsgLookup);
  outMsgLookup *outMsg = (outMsgLookup *)l_inOutMsgAttribute->getDataPtr();

  if (outMsg->hdr.error) {
    outMsgGeneric outMsgError(outMsg->hdr.unique, outMsg->hdr.error);
    __s64 local_val = (__s64)writedeviceFD(&outMsgError, in->hdr.opcode);
    return local_val;
  }

  txp::Attribute *lookUpNameAttribute = pMsg->retrieveAttr(txp::lookupname);
  char *lookUpName = NULL;

  if (lookUpNameAttribute) {
    lookUpName = (char *)lookUpNameAttribute
                     ->getDataPtr(); // to get addressability to the data
  }
  if (lookUpName) {
    _rootNodePtr->updateChildNodeForLookup(&outMsg->entry_out, lookUpName,
                                           parent);
  }
  // ensure the inode/name is in the list of directories or regular files
  // need parent inode, file inode, and name
  __s64 retval = writedeviceFD((outMsgGeneric *)outMsg, in->hdr.opcode);
  return retval;
}

__s64 ResponseHandler::writedeviceFD(outMsgGeneric *outMsg, uint32_t opcode) {
  /* clang-format off */ 
  FL_Write(fl_fshipcldfusetx, FCL_SEND2FUSE,"op=%-3ld uniq=%-24ld len=%-12ld err=%-12ld  ", opcode,outMsg->hdr.unique,outMsg->hdr.len,outMsg->hdr.error);
  /* clang-format on */
  return (__s64)write(_deviceFD, outMsg, outMsg->hdr.len);
}

__s64 ResponseHandler::readdirOp(txp::Msg *pMsg) {

  // bounce back fuse_in_header for nodeid and perhaps other tracking
  txp::Attribute *l_inHdrAttribute = pMsg->retrieveAttr(txp::fuse_in_header);
  if (!l_inHdrAttribute) {
    LOGERRNO(fshipcld, error, ENOSYS);
    return -ENOSYS;
  }
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();

  txp::Attribute *l_outMsgGenericAttr = pMsg->retrieveAttr(txp::outMsgGeneric);
  if (!l_outMsgGenericAttr) {
    LOGERRNO(fshipcld, error, ENOSYS);
    return -ENOSYS;
  }
  outMsgGeneric *outMsgPtr = (outMsgGeneric *)l_outMsgGenericAttr->getDataPtr();

  uint32_t sizeOfDataBuffer =
      outMsgPtr->hdr.len -
      sizeof(outMsgGeneric); // hdr.len is hdr size + bytes written

  if (sizeOfDataBuffer > 0) {
    char *charMemoryPtr = _connectPtr->accessBuffer(pMsg, sizeOfDataBuffer);

    // pick off inode numbers and names of directory entries
    const NodeNamePTR parent = _rootNodePtr->getNodeDir(in->hdr.nodeid);
    if (!parent)
      abort();

    struct fuse_dirent *l_Fusedent = (struct fuse_dirent *)charMemoryPtr;
    unsigned int offset = 0;

    while (offset < sizeOfDataBuffer) {
      // for (unsigned int
      // i=0;i<l_Fusedent->namelen;i++)printf("%c",l_Fusedent->name[i]);
      // printf("\n");
      if (l_Fusedent->namelen > 2) {
        _rootNodePtr->updateChildNode(l_Fusedent, parent);
      } else if (l_Fusedent->name[0] != '.') {
        _rootNodePtr->updateChildNode(l_Fusedent, parent);
      } else if ((l_Fusedent->namelen == 2) && (l_Fusedent->name[1] != '.')) {
        _rootNodePtr->updateChildNode(l_Fusedent, parent);
      }

      if (l_Fusedent->off) {
        offset += l_Fusedent->off; /* offset to next entry within the buffer*/
        l_Fusedent = (struct fuse_dirent *)(charMemoryPtr + offset);
      } else
        break;
    }

    // Send response to fuse module using message followed by data
    struct iovec iov[2];
    iov->iov_base = outMsgPtr;
    iov->iov_len = sizeof(outMsgGeneric);
    iov[1].iov_base = charMemoryPtr;
    iov[1].iov_len = sizeOfDataBuffer;
    int iovcnt = sizeof(iov) / sizeof(struct iovec);
    /* clang-format off */ 
    FL_Write(fl_fshipcldfusetx, FCL_READDIR__,"op=%-3ld uniq=%-24ld len=%-12ld err=%-12ld  ", FUSE_READDIR, outMsgPtr->hdr.unique,outMsgPtr->hdr.len,outMsgPtr->hdr.error);
    /* clang-format on */
    int retval = writev(_deviceFD, iov, iovcnt);
    _connectPtr->releaseBuffer(charMemoryPtr);
    return retval;
  } else {
    int retval = writedeviceFD(outMsgPtr, in->hdr.opcode);
    return retval;
  }
  return 0;
}

__s64 ResponseHandler::readdirPlusOp(txp::Msg *pMsg) {

  // bounce back fuse_in_header for nodeid and perhaps other tracking
  txp::Attribute *l_inHdrAttribute = pMsg->retrieveAttr(txp::fuse_in_header);
  if (!l_inHdrAttribute) {
    LOGERRNO(fshipcld, error, ENOSYS);
    return -ENOSYS;
  }
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();

  txp::Attribute *l_outMsgGenericAttr = pMsg->retrieveAttr(txp::outMsgGeneric);
  if (!l_outMsgGenericAttr) {
    LOGERRNO(fshipcld, error, ENOSYS);
    return -ENOSYS;
  }
  outMsgGeneric *outMsgPtr = (outMsgGeneric *)l_outMsgGenericAttr->getDataPtr();

  uint32_t sizeOfDataBuffer =
      outMsgPtr->hdr.len -
      sizeof(outMsgGeneric); // hdr.len is hdr size + bytes written

  size_t nbytes = 0;
  size_t reclen = 0;
  LOG(fshipcld, debug) << "readdirplus pNid=" << in->hdr.nodeid
                       << " sizeOfDataBuffer=" << sizeOfDataBuffer;
  if (sizeOfDataBuffer > 0) {
    char *charMemoryPtr = _connectPtr->accessBuffer(pMsg, sizeOfDataBuffer);

    // pick off inode numbers and names of directory entries
    const NodeNamePTR parent = _rootNodePtr->getNodeDir(in->hdr.nodeid);
    if (!parent)
      abort();

    struct fuse_direntplus *l_Fusedentplus =
        (struct fuse_direntplus *)charMemoryPtr;

    while (nbytes < sizeOfDataBuffer) {
      if (l_Fusedentplus->dirent.namelen > 2) {
        _rootNodePtr->updateChildNodeForResponse(l_Fusedentplus, parent);
      } else if (l_Fusedentplus->dirent.name[0] != '.') {
        _rootNodePtr->updateChildNodeForResponse(l_Fusedentplus, parent);
      } else if ((l_Fusedentplus->dirent.namelen == 2) &&
                 (l_Fusedentplus->dirent.name[1] != '.')) {
        _rootNodePtr->updateChildNodeForResponse(l_Fusedentplus, parent);
      }

      reclen = FUSE_DIRENTPLUS_SIZE(l_Fusedentplus);
      nbytes += reclen;

      l_Fusedentplus = (struct fuse_direntplus *)(charMemoryPtr + nbytes);
    }

    // Send response to fuse module using message followed by data
    struct iovec iov[2];
    iov->iov_base = outMsgPtr;
    iov->iov_len = sizeof(outMsgGeneric);
    iov[1].iov_base = charMemoryPtr;
    iov[1].iov_len = sizeOfDataBuffer;
    int iovcnt = sizeof(iov) / sizeof(struct iovec);
    /* clang-format off */ 
    FL_Write(fl_fshipcldfusetx, FCL_READDIRPLUS,"op=%-3ld uniq=%-24ld len=%-12ld err=%-12ld  ", FUSE_READDIR, outMsgPtr->hdr.unique,outMsgPtr->hdr.len,outMsgPtr->hdr.error);
    /* clang-format on */
    int retval = writev(_deviceFD, iov, iovcnt);
    _connectPtr->releaseBuffer(charMemoryPtr);
    return retval;
  } else {
    int retval = writedeviceFD(outMsgPtr, in->hdr.opcode);
    return retval;
  }
  return 0;
}

__s64 ResponseHandler::readOp(txp::Msg *pMsg) {

  txp::Attribute *l_outMsgGenericAttr = pMsg->retrieveAttr(txp::outMsgGeneric);
  if (!l_outMsgGenericAttr) {
    LOGERRNO(fshipcld, error, ENOSYS);
    return -ENOSYS;
  }
  outMsgGeneric *outMsgPtr = (outMsgGeneric *)l_outMsgGenericAttr->getDataPtr();
  if (outMsgPtr->hdr.error)
    return write(_deviceFD, outMsgPtr, outMsgPtr->hdr.len);

  txp::Attribute *size32Attr = pMsg->retrieveAttr(txp::size);
  uint32_t sizeOfDataBuffer = 0;
  int l_RC = size32Attr->cpyData(&sizeOfDataBuffer, sizeof(sizeOfDataBuffer));
  if (l_RC)
    abort();
  /* clang-format off */ 
  FL_Write(FL_fshipcldreadop, READOPRSP,"unique=%ld error=%ld sizeRead=%ld line=%ld\n", outMsgPtr->hdr.unique, outMsgPtr->hdr.error,sizeOfDataBuffer,__LINE__);
  /* clang-format on */
  if (!sizeOfDataBuffer)
    return write(_deviceFD, outMsgPtr, outMsgPtr->hdr.len);

  memItemPtr mipRDMAfromOriginal = NULL;
  txp::Attribute *l_memItemAttribute = pMsg->retrieveAttr(txp::memItem);
  if (l_memItemAttribute) {
    memItemPtr mip = (memItemPtr)l_memItemAttribute->getDataPtr();
    mipRDMAfromOriginal =
        mip->next; // get the saved mip address used in fshipcld
  }
  if (mipRDMAfromOriginal) {
    char *charMemoryPtr = mipRDMAfromOriginal->address;

    // Send response to fuse module using message followed by data
    struct iovec iov[2];
    iov->iov_base = outMsgPtr;
    iov->iov_len = sizeof(outMsgGeneric);
    iov[1].iov_base = charMemoryPtr;
    iov[1].iov_len = sizeOfDataBuffer;
    int iovcnt = sizeof(iov) / sizeof(struct iovec);
    int retval = writev(_deviceFD, iov, iovcnt);
    _connectPtr->freeRDMAChunk(mipRDMAfromOriginal);
    return retval;

  } else {
    char *charMemoryPtr = _connectPtr->accessBuffer(pMsg, sizeOfDataBuffer);
    // printf("charMemoryPtr=%s",charMemoryPtr);
    // Send response to fuse module using message followed by data
    struct iovec iov[2];
    iov->iov_base = outMsgPtr;
    iov->iov_len = sizeof(outMsgGeneric);
    iov[1].iov_base = charMemoryPtr;
    iov[1].iov_len = sizeOfDataBuffer;
    int iovcnt = sizeof(iov) / sizeof(struct iovec);
    int retval = writev(_deviceFD, iov, iovcnt);
    _connectPtr->releaseBuffer(charMemoryPtr);
    return retval;
  }

  return 0;
}

__s64 ResponseHandler::writeOp(txp::Msg *pMsg) {
  txp::Attribute *l_memItemAttribute = pMsg->retrieveAttr(txp::memItem);
  if (l_memItemAttribute) {
    memItemPtr l_localMip4RDMA = (memItemPtr)l_memItemAttribute->getDataPtr();
    memItemPtr mipRDMAfromOriginal = l_localMip4RDMA->next;
    mipRDMAfromOriginal->next = NULL;
    _connectPtr->freeRDMAChunk(mipRDMAfromOriginal);
  }

  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgPwrite);
  if (l_outHdrAttribute) {
    outMsgPwrite *outMsg = (outMsgPwrite *)l_outHdrAttribute->getDataPtr();
    /* clang-format off */ 
    FL_Write(FL_fshipcldwriteop, WRITEOPRSP,  "unique=%ld error=%ld sizeWritten=%ld line=%ld\n", outMsg->hdr.unique, outMsg->hdr.error,outMsg->write_out.size,__LINE__);
    /* clang-format on */
    if (outMsg->hdr.error) {
      outMsg->hdr.len = sizeof(outMsgGeneric);
    }
    // printf("Response writeOp(txp::Msg*>>>> wroteSize=%d
    // error=%d\n",outMsg->write_out.size,outMsg->hdr.error);
    return write(_deviceFD, outMsg, outMsg->hdr.len);
  }
  l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  if (l_outHdrAttribute) {
    outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();
    return write(_deviceFD, outMsg, outMsg->hdr.len);
  } else
    abort();
  return 0;
}

__s64 ResponseHandler::accessOp(txp::Msg *pMsg) {

  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();

  __s64 retval = writedeviceFD(outMsg, FUSE_ACCESS);
  return retval;
}

__s64 ResponseHandler::fallocateOp(txp::Msg *pMsg) {

  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();

  __s64 retval = (__s64)write(_deviceFD, outMsg, outMsg->hdr.len);
  return retval;
}

__s64 ResponseHandler::releasedirOp(txp::Msg *pMsg) {
  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();

  __s64 retval = writedeviceFD(outMsg, FUSE_RELEASEDIR);
  return retval;
}

__s64 ResponseHandler::releaseOp(txp::Msg *pMsg) {
  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();

  __s64 retval = writedeviceFD(outMsg, FUSE_RELEASE);
  return retval;
}

__s64 ResponseHandler::signal(txp::Msg *pMsg) {
  _connectPtr
      ->disconnect(); // \todo Need to move to clearing outstanding messages
  return 0;
}

__s64 ResponseHandler::setlkOp(txp::Msg *pMsg) {
  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();

  __s64 retval = writedeviceFD(outMsg, FUSE_SETLK);
  return retval;
}

__s64 ResponseHandler::setlkwOp(txp::Msg *pMsg) {
  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();

  if (outMsg->hdr.error == EAGAIN)
    outMsg->hdr.error = EINTR;

  __s64 retval = writedeviceFD(outMsg, FUSE_SETLKW);
  return retval;
}
__s64 ResponseHandler::getlkOp(txp::Msg *pMsg) {

  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  if (l_outHdrAttribute) {
    outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();
    return writedeviceFD(outMsg, FUSE_READLINK);
  }
  l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGetLock);
  outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();
  return writedeviceFD(outMsg, FUSE_GETLK);
}

__s64 ResponseHandler::readlinkOp(txp::Msg *pMsg) {

  txp::Attribute *l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgGeneric);
  if (l_outHdrAttribute) {
    outMsgGeneric *outMsg = (outMsgGeneric *)l_outHdrAttribute->getDataPtr();
    return writedeviceFD(outMsg, FUSE_READLINK);
  }
  l_outHdrAttribute = pMsg->retrieveAttr(txp::outMsgReadlink);
  outMsgReadlink *outMsgPtr = (outMsgReadlink *)l_outHdrAttribute->getDataPtr();
  if (outMsgPtr->hdr.error)
    printf("readlinkOp error=%d\n", outMsgPtr->hdr.error);
  txp::Attribute *size64Attr = pMsg->retrieveAttr(txp::length);
  uint64_t sizeOfDataBuffer = 0;
  int l_RC = size64Attr->cpyData(&sizeOfDataBuffer, sizeof(sizeOfDataBuffer));
  if (l_RC)
    abort();

  if (!sizeOfDataBuffer) {
    int retval = write(_deviceFD, outMsgPtr, outMsgPtr->hdr.len);
    return retval;
  }

  txp::Attribute *l_linkDataAttr = pMsg->retrieveAttr(txp::link);
  char *charMemoryPtr = (char *)l_linkDataAttr->getDataPtr();

  // Send response to fuse module using message followed by data
  outMsgPtr->hdr.len = sizeof(outMsgReadlink) + sizeOfDataBuffer;
  struct iovec iov[2];
  iov->iov_base = outMsgPtr;
  iov->iov_len = sizeof(outMsgReadlink);
  iov[1].iov_base = charMemoryPtr;
  iov[1].iov_len = sizeOfDataBuffer;
  int iovcnt = sizeof(iov) / sizeof(struct iovec);
  /* clang-format off */ 
  FL_Write(fl_fshipcldfusetx, FCL_READLINK_, "op=%-3ld uniq=%-24ld len=%-12ld err=%-12ld  ", FUSE_READLINK, outMsgPtr->hdr.unique,outMsgPtr->hdr.len,outMsgPtr->hdr.error);
  /* clang-format on */
  int retval = writev(_deviceFD, iov, iovcnt);

  return retval;
}

int ResponseHandler::readInMessage() {
  int l_RC = 0;

  txp::Msg *l_MsgPtr = 0;
  memItemPtr l_memItemPtr = _connectPtr->getReadChunk();
  if (!l_memItemPtr)
    return 1;
  if (l_memItemPtr->next)
    abort(); // expect a single chunk

  l_RC =
      _connectPtr->read(l_MsgPtr, l_memItemPtr->address, l_memItemPtr->length);

  if (!l_RC) {
    txp::Id l_Id = l_MsgPtr->getMsgId();
    switch (l_Id) {
    case txp::FUSE_LOOKUP:
      lookupOp(l_MsgPtr);
      break;
    case txp::FUSE_FORGET:
      LOGERRNO(fshipcld, error, EINVAL);
      break;
    case txp::FUSE_GETATTR:
      getattrOp(l_MsgPtr);
      break;
    case txp::FUSE_SETATTR:
      setattrOp(l_MsgPtr);
      break;
    case txp::FUSE_READLINK:
      readlinkOp(l_MsgPtr);
      break;
    case txp::FUSE_SYMLINK:
      symlinkOp(l_MsgPtr);
      break;
    case txp::FUSE_MKNOD:
      mknodOp(l_MsgPtr);
      break;
    case txp::FUSE_MKDIR:
      mkdirOp(l_MsgPtr);
      break;
    case txp::FUSE_UNLINK:
      unlinkOp(l_MsgPtr);
      break;
    case txp::FUSE_RMDIR:
      rmdirOp(l_MsgPtr);
      break;

    case txp::FUSE_RENAME:
      renameOp(l_MsgPtr);
      break;

    case txp::FUSE_OPEN:
      openOp(l_MsgPtr);
      break;

    case txp::FUSE_READ:
      readOp(l_MsgPtr);
      break;
    case txp::FUSE_WRITE:
      writeOp(l_MsgPtr);
      break;
    case txp::FUSE_STATFS:
      abort();
      break;
    case txp::FUSE_RELEASE:
      releaseOp(l_MsgPtr);
      break;
    case txp::FUSE_FLUSH:
      flushOp(l_MsgPtr);
      break;
    case txp::FUSE_INIT:
      break;
    case txp::FUSE_OPENDIR:
      opendirOp(l_MsgPtr);
      break;
    case txp::FUSE_READDIR:
      readdirOp(l_MsgPtr);
      break;
    case txp::FUSE_RELEASEDIR:
      releasedirOp(l_MsgPtr);
      break;
    case txp::FUSE_FSYNC:
      fsyncOp(l_MsgPtr);
      break;
    case txp::FUSE_GETLK:
      getlkOp(l_MsgPtr);
      break;
    case txp::FUSE_SETLK:
      setlkOp(l_MsgPtr);
      break;
    case txp::FUSE_SETLKW:
      setlkwOp(l_MsgPtr);
      break;

    case txp::FUSE_ACCESS:
      accessOp(l_MsgPtr);
      break;
    case txp::FUSE_CREATE:
      createOp(l_MsgPtr);
      break;
    case txp::FUSE_INTERRUPT:
      interruptOp(l_MsgPtr);
      break;
    case txp::FUSE_BMAP:
      abort();
      break;
    case txp::FUSE_DESTROY:
      abort();
      break;
    case txp::FUSE_IOCTL:
      abort();
      break;
    case txp::FUSE_POLL:
      pollOp(l_MsgPtr);
      break;
    case txp::FUSE_FALLOCATE:
      fallocateOp(l_MsgPtr);
      break;
    case txp::FUSE_READDIRPLUS:
      readdirPlusOp(l_MsgPtr);
      break;
    case txp::FUSE_LINK:
      linkOp(l_MsgPtr);
      break;

    case txp::CORAL_HELLO:
      Hello(l_MsgPtr);
      break;
    case txp::CORAL_NO_OP:
      break;
    case txp::CORAL_READY:
      break;
    case txp::CORAL_SIGNAL:
      signal(l_MsgPtr);
      break;
    case txp::CORAL_AUTHENTICATE:
      abort();
      break;
    case txp::CORAL_GOODBYE:
      _connectPtr->disconnect();
      break;
    case txp::CORAL_ERROR:
      abort();
      break;

    case txp::FUSE_SETXATTR:
      setxattrOp(l_MsgPtr);
      break;
    case txp::FUSE_GETXATTR:
      getxattrOp(l_MsgPtr);
      break;
    case txp::FUSE_LISTXATTR:
      listxattrOp(l_MsgPtr);
      break;
    case txp::FUSE_REMOVEXATTR:
      removexattrOp(l_MsgPtr);
      break;

    // note: unsupported list
    // case txp::CUSE_INIT:abort(); break;
    // case txp::FUSE_FSYNCDIR:abort();break;

    default:
      LOG(fshipcld, always) << " unsupported msg ID=" << l_Id;
      abort();
      break;
    }
  }
  _connectPtr->freeReadChunk(l_memItemPtr);
  if (l_MsgPtr)
    delete l_MsgPtr;
  l_MsgPtr = NULL;
  return l_RC;
};
