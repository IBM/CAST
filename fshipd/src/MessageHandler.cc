/*******************************************************************************
 |    MessageHandler.cc
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
#include "MessageHandler.h"
#include "CnxSock.h"
#include "Msg.h"
#include "fshipd_flightlog.h"
#include "identity.h"
#include "logging.h"
#include "util.h"
#include <dirent.h>
#include <linux/stat.h>
#include <string>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <vector>

#ifndef GPFS_SUPER_MAGIC
#define GPFS_SUPER_MAGIC 0x47504653
#endif

static const char empty[] = "";
// getData() methods on derived classes of Attribute().  For char_array, use
// getDataPtr() and getDataLength()...

int MessageHandler::openOp(txp::Msg *pMsg) {
  txp::Attribute *pathAttribute = pMsg->retrieveAttr(txp::name);
  //    txp::AttributeType l_TypeOfAttribute = pathAttribute->getAttrType() ;
  char *l_Path =
      (char *)pathAttribute->getDataPtr(); // to get addressability to the data

  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_OPEN, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  int buRC = becomeUser(in->hdr.uid, in->hdr.gid); // becomeUser(uid_t user)
                                                   // ensure identity is correct
                                                   // for authority check
  if (buRC)
    return sendErrorResponse(pMsg, in, EOWNERDEAD);
  int flags = 0;
  txp::Attribute *l_inFlagsAttribute = pMsg->retrieveAttr(txp::flags);
  int l_RC = l_inFlagsAttribute->cpyData(&flags, sizeof(flags));
  if (l_RC)
    abort();

  outMsgOpen outMsg(in->hdr.unique, 0);
  /* clang-format off */ 
  FL_Write(fl_fshipdposix, POSIX_OPENBEG,  "Starting open() operation.  PathnameLen=%ld  Flags=0x%lx", pathAttribute->getDataLength(), flags,0,0);
  /* clang-format on */
  outMsg.open_out.fh = ::open(l_Path, flags);
  /* clang-format off */ 
  FL_Write(fl_fshipdposix, POSIX_OPENFIN, "Completed open() operation.  PathnameLen=%ld  Flags=0x%lx  FH=%ld  errno=%d", pathAttribute->getDataLength(), flags,outMsg.open_out.fh, errno);
  /* clang-format on */

  int writeRequest = flags & (O_WRONLY | O_RDWR);

  if ((signed)outMsg.open_out.fh == -1) {
    if (errno != ENOENT)
      return sendErrorResponse(pMsg, in, errno);
    else if (!writeRequest)
      return sendErrorResponse(pMsg, in, errno);
    else { // retry mechanism
      flags |= O_CREAT;
      outMsg.open_out.fh = ::openat(-1, l_Path, flags, S_IRWXU);
      if ((signed)outMsg.open_out.fh == -1)
        sendErrorResponse(pMsg, in, errno);
    }
  }

  outMsg.open_out.open_flags =
      _openOutFlags; // set according to object FOPEN fuse flags

  // Build response Msg and then pick up elements from original message
  txp::Msg *l_ResponseMsg = 0;
  l_RC = pMsg->buildResponseMsg(l_ResponseMsg);
  if (!l_RC)
    l_RC = l_ResponseMsg->addAttribute(txp::outMsgOpen, (char *)&outMsg,
                                       sizeof(outMsgOpen));
  if (l_RC)
    abort(); // messed up building attribue

  ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
  if (retSSize == -1) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -errno;
  }

  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }
  return 0;
}

int MessageHandler::mknodOp(txp::Msg *pMsg) {
  txp::Attribute *pathAttribute = pMsg->retrieveAttr(txp::name);
  txp::Attribute *l_Attr64offset = pMsg->retrieveAttr(txp::offset);

  uint64_t indexToLookupName = 0;
  char *l_Path = NULL;
  char *lookUpName = NULL;

  if (l_Attr64offset) {
    int l_RC =
        l_Attr64offset->cpyData(&indexToLookupName, sizeof(indexToLookupName));
    if (l_RC)
      abort();
  }
  if (pathAttribute) {
    l_Path =
        (char *)
            pathAttribute->getDataPtr(); // to get addressability to the data
    if (l_Attr64offset)
      lookUpName = l_Path + indexToLookupName;
  }

  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_MKNOD, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  int buRC = becomeUser(in->hdr.uid, in->hdr.gid); // becomeUser(uid_t user)
                                                   // ensure identity is correct
                                                   // for authority check
  if (buRC)
    return sendErrorResponse(pMsg, in, EOWNERDEAD);

  int rdev = 0;
  int mode = 0;
  txp::Attribute *l_inFlagsAttribute = pMsg->retrieveAttr(txp::rdev);
  txp::Attribute *l_inModeAttribute = pMsg->retrieveAttr(txp::mode);
  int l_RC = l_inFlagsAttribute->cpyData(&rdev, sizeof(rdev));
  if (!l_RC)
    l_RC = l_inModeAttribute->cpyData(&mode, sizeof(mode));
  if (l_RC)
    abort();

  outMsgMknod outMsg(in->hdr.unique, 0);
  memset(&outMsg.entry_out, 0, sizeof(outMsg.entry_out));
  int RCmknod = ::mknod(l_Path, mode, rdev);
  /* clang-format off */ 
  FL_Write(fl_fshipdposix, POSIX_MKNOD, "Completed mknod() operation.  PathnameLen=%ld  rdev=0x%lx  errno=%d", pathAttribute->getDataLength(), rdev, errno,0);
  /* clang-format on */
  if (-1 == RCmknod) {
    return sendErrorResponse(pMsg, in, errno);
  } else {
    struct stat l_stat;
    int l_statRC = stat(l_Path, &l_stat);

    if (l_statRC) {
      return sendErrorResponse(pMsg, in, errno);
    } else {
      setEntryOut(l_stat, outMsg.entry_out);
    }
  }

  // Build response Msg and then pick up elements from original message
  txp::Msg *l_ResponseMsg = 0;
  l_RC = pMsg->buildResponseMsg(l_ResponseMsg);

  l_RC = l_ResponseMsg->addAttribute(txp::fuse_in_header, (const char *)in,
                                     l_inHdrAttribute->getDataLength());

  txp::AttrPtr_char_array *l_Attr = NULL;
  l_RC = txp::AttrPtr_char_array::buildAttrPtr_char_array(
      txp::outMsgMknod, (char *)&outMsg, sizeof(outMsgMknod), l_Attr);
  if (!l_RC) {
    l_RC = l_ResponseMsg->addAttribute(l_Attr);
  }

  if (lookUpName) {
    int l_lookUpNameLength = strlen(lookUpName) + 1; // get null terminator
    l_RC = addlookupname(l_ResponseMsg, lookUpName, l_lookUpNameLength,
                         txp::lookupname);
  };

  if (l_RC)
    abort(); // messed up building attribue

  ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
  if (retSSize == -1) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -errno;
  }

  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }
  return 0;
}

int MessageHandler::creatOp(txp::Msg *pMsg) {
  txp::Attribute *pathAttribute = pMsg->retrieveAttr(txp::name);
  txp::Attribute *l_Attr64offset = pMsg->retrieveAttr(txp::offset);

  uint64_t indexToLookupName = 0;
  char *l_Path = NULL;
  char *lookUpName = NULL;

  if (l_Attr64offset) {
    int l_RC =
        l_Attr64offset->cpyData(&indexToLookupName, sizeof(indexToLookupName));
    if (l_RC)
      abort();
  }
  if (pathAttribute) {
    l_Path =
        (char *)
            pathAttribute->getDataPtr(); // to get addressability to the data
    if (l_Attr64offset)
      lookUpName = l_Path + indexToLookupName;
  }

  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_CREATE, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  int buRC = becomeUser(in->hdr.uid, in->hdr.gid); // becomeUser(uid_t user)
                                                   // ensure identity is correct
                                                   // for authority check
  if (buRC)
    return sendErrorResponse(pMsg, in, EOWNERDEAD);

  int flags = 0;
  int mode = 0;
  txp::Attribute *l_inFlagsAttribute = pMsg->retrieveAttr(txp::flags);
  txp::Attribute *l_inModeAttribute = pMsg->retrieveAttr(txp::mode);
  int l_RC = l_inFlagsAttribute->cpyData(&flags, sizeof(flags));
  if (!l_RC)
    l_RC = l_inModeAttribute->cpyData(&mode, sizeof(mode));
  if (l_RC)
    abort();

  outMsgCreate outMsg(in->hdr.unique, 0);
  memset(&outMsg.entry_out, 0, sizeof(outMsg.entry_out));
  /* clang-format off */ 
  FL_Write(fl_fshipdposix, POSIX_OPENATBEG, "Starting openat() operation.  PathnameLen=%ld  Flags=0x%lx  FH=%ld  errno=%d", pathAttribute->getDataLength(), flags,mode,0);
  /* clang-format on */
  int rcOpen = ::openat(-1, l_Path, flags, mode);
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_OPENATFIN, "Completed openat() operation.  PathnameLen=%ld  Flags=0x%lx  FH=%ld  errno=%d rcOpen=%d", pathAttribute->getDataLength(), flags,mode,rcOpen, errno,rcOpen);
  /* clang-format on */

  outMsg.open_out.fh = rcOpen;
  if (-1 == rcOpen) {
    return sendErrorResponse(pMsg, in, errno);
  } else {

    outMsg.open_out.open_flags =
        _openOutFlags; // set according to object FOPEN fuse flags

    struct stat l_stat;
    int fstatatflags = AT_SYMLINK_NOFOLLOW;
    int filehandle = -1;
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_STATATBEG, "Starting statat() operation.  FH=%ld Flags=%ld ", filehandle, pathAttribute->getDataLength(), fstatatflags,0,0,0);
    /* clang-format on */
    int l_statRC = fstatat(filehandle, l_Path, &l_stat, fstatatflags);
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_STATATFIN, "Completed statat() operation. rc=%ld  errno=%ld st_ino=%ld mode=%o uid=%ld gid=%ld", l_statRC, errno,l_stat.st_ino,l_stat.st_mode, l_stat.st_uid,l_stat.st_gid);
    /* clang-format on */

    if (l_statRC) {
      return sendErrorResponse(pMsg, in, errno);
    } else {
      setEntryOut(l_stat, outMsg.entry_out);
    }
  }

  // Build response Msg and then pick up elements from original message
  txp::Msg *l_ResponseMsg = 0;
  l_RC = pMsg->buildResponseMsg(l_ResponseMsg);

  l_RC = l_ResponseMsg->addAttribute(txp::fuse_in_header, (const char *)in,
                                     l_inHdrAttribute->getDataLength());

  txp::AttrPtr_char_array *l_Attr = NULL;
  l_RC = txp::AttrPtr_char_array::buildAttrPtr_char_array(
      txp::outMsgCreate, (char *)&outMsg, sizeof(outMsgCreate), l_Attr);
  if (!l_RC) {
    l_RC = l_ResponseMsg->addAttribute(l_Attr);
  }

  if (lookUpName) {
    int l_lookUpNameLength = strlen(lookUpName) + 1; // get null terminator
    l_RC = addlookupname(l_ResponseMsg, lookUpName, l_lookUpNameLength,
                         txp::lookupname);
  };

  if (l_RC)
    abort(); // messed up building attribue

  ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
  if (retSSize == -1) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -errno;
  }

  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }
  return 0;
}

int MessageHandler::mkdirOp(txp::Msg *pMsg) {
  txp::Attribute *pathAttribute = pMsg->retrieveAttr(txp::name);
  txp::Attribute *l_Attr64offset = pMsg->retrieveAttr(txp::offset);
  txp::Attribute *l_AttrMemItemPtr = pMsg->retrieveAttr(txp::memItem);
  int l_RC = 0;
  uint64_t indexToLookupName = 0;
  char *l_Path = NULL;
  char *lookUpName = NULL;
  memItemPtr l_memItemPtr = NULL;

  if (l_Attr64offset) {
    l_RC =
        l_Attr64offset->cpyData(&indexToLookupName, sizeof(indexToLookupName));
    if (l_RC)
      abort();
  }
  if (l_AttrMemItemPtr) {
    l_memItemPtr = (memItemPtr)l_AttrMemItemPtr->getDataPtr();
    LOG(fshipd, debug) << "l_memItemPtr=" << l_memItemPtr;
    // printf("l_memItemPtr=%p \n",l_memItemPtr);
  }
  if (pathAttribute) {
    l_Path =
        (char *)
            pathAttribute->getDataPtr(); // to get addressability to the data
    if (l_Attr64offset)
      lookUpName = l_Path + indexToLookupName;
  }

  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_MKDIR, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  int buRC = becomeUser(in->hdr.uid, in->hdr.gid); // becomeUser(uid_t user)
                                                   // ensure identity is correct
                                                   // for authority check
  if (buRC)
    return sendErrorResponse(pMsg, in, EOWNERDEAD);

  int mode = 0;

  txp::Attribute *l_inModeAttribute = pMsg->retrieveAttr(txp::mode);
  if (!l_RC)
    l_RC = l_inModeAttribute->cpyData(&mode, sizeof(mode));
  if (l_RC)
    abort();

  outMsgMkdir outMsg(in->hdr.unique, 0);
  memset(&outMsg.entry_out, 0, sizeof(outMsg.entry_out));

  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_MKDIRATBEG, "Starting mkdirat() operation.  -1, pathlength=%ld  mode=%ld", pathAttribute->getDataLength(), mode,0,0,0,0);
  /* clang-format on */
  int rcMkdir = mkdirat(-1, l_Path, mode);
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_MKDIRATFIN, "Completed mkdirat() operation.  -1, pathlength=%ld  mode=%ld  rc=%ld  errno=%ld", pathAttribute->getDataLength(), mode,rcMkdir, errno,0,0);
  /* clang-format on */

  if (-1 == rcMkdir) {
    outMsg.hdr.error = -errno;
  } else {
    struct stat l_stat;
    int fstatatflags = AT_SYMLINK_NOFOLLOW;
    int filehandle = -1;
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_STATATBEG2, "Starting statat() operation.  FH=%ld PathLength=%ld Flags=0x%lx", filehandle, pathAttribute->getDataLength(), fstatatflags,0,0,0);
    /* clang-format on */
    int l_statRC = fstatat(filehandle, l_Path, &l_stat, fstatatflags);
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_STATATFIN2, "Completed statat() operation.  rc=%ld  errno=%ld st_ino=%ld mode=%o uid=%ld gid=%ld", l_statRC, errno,l_stat.st_ino,l_stat.st_mode, l_stat.st_uid,l_stat.st_gid);
    /* clang-format on */

    if (l_statRC) {
      outMsg.hdr.error = -errno;
      memset(&outMsg.entry_out, 0, sizeof(outMsg.entry_out));
    } else {
      setEntryOut(l_stat, outMsg.entry_out);
    }
  }

  // Build response Msg and then pick up elements from original message
  txp::Msg *l_ResponseMsg = 0;
  l_RC = pMsg->buildResponseMsg(l_ResponseMsg);

  l_RC = l_ResponseMsg->addAttribute(txp::fuse_in_header, (const char *)in,
                                     l_inHdrAttribute->getDataLength());

  txp::AttrPtr_char_array *l_Attr = NULL;
  l_RC = txp::AttrPtr_char_array::buildAttrPtr_char_array(
      txp::outMsgCreate, (char *)&outMsg, sizeof(outMsgCreate), l_Attr);
  if (!l_RC) {
    l_RC = l_ResponseMsg->addAttribute(l_Attr);
  }

  if (lookUpName) {
    int l_lookUpNameLength = strlen(lookUpName) + 1; // get null terminator
    l_RC = addlookupname(l_ResponseMsg, lookUpName, l_lookUpNameLength,
                         txp::lookupname);
  };

  if (l_RC)
    abort(); // messed up building attribue

  LOG(fshipd, debug) << "mkdir l_Path=" << l_Path;

  ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
  if (retSSize == -1) {
    LOG(fshipd, error) << "mkdir write retSSize=" << retSSize
                       << " errno=" << errno;
    LOGERRNO(fshipd, error, ENOSYS);
    if (l_ResponseMsg) {
      delete l_ResponseMsg;
      l_ResponseMsg = 0;
    }
    return -errno;
  }

  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }
  return 0;
}
__s64 MessageHandler::getxattrOp(txp::Msg *pMsg) {
  txp::Attribute *pathAttribute = pMsg->retrieveAttr(txp::name);
  char *l_Path =
      (char *)pathAttribute->getDataPtr(); // to get addressability to the data

  txp::Attribute *xattrName = pMsg->retrieveAttr(txp::nameOfAttribute);
  char *l_nameOfAttribute = (char *)xattrName->getDataPtr();

  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_GETXATTR, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  txp::Attribute *size32Attr = pMsg->retrieveAttr(txp::size);
  uint32_t sizeOfDataBuffer = 0;
  int l_RC = size32Attr->cpyData(&sizeOfDataBuffer, sizeof(sizeOfDataBuffer));

  int buRC = becomeUser(in->hdr.uid, in->hdr.gid); // becomeUser(uid_t user)
                                                   // ensure identity is correct
                                                   // for authority check
  if (buRC)
    return sendErrorResponse(pMsg, in, EOWNERDEAD);

  char value[sizeOfDataBuffer];

  // lgetxattr is like getxattr except will do a symlink itself instead of its
  // reference
  ssize_t sizeOut =
      lgetxattr(l_Path, l_nameOfAttribute, value, sizeOfDataBuffer);

  if (sizeOut == (-1)) {
    if (ENODATA != errno)
      LOG(fshipd, info) << "lgetxattr l_Path=" << l_Path << " value="
                        << " sizeOfDataBuffer=" << sizeOfDataBuffer
                        << " sizeOut=" << sizeOut << " errno=" << errno;
    return sendErrorResponse(pMsg, in, errno);
  }
  LOG(fshipd, debug) << "lgetxattr l_Path=" << l_Path
                     << " name=" << l_nameOfAttribute
                     << " value=" << (sizeOfDataBuffer ? value : empty)
                     << " sizeOfDataBuffer=" << sizeOfDataBuffer
                     << " sizeOut=" << sizeOut;
  outMsgGetXattr outMsg(in->hdr.unique);
  outMsg.getxattr_out.size = sizeOut;

  // Build response Msg and then pick up elements from original message
  txp::Msg *l_ResponseMsg = 0;
  l_RC = pMsg->buildResponseMsg(l_ResponseMsg);
  assert(l_RC == 0);

  l_ResponseMsg->addAttribute(txp::outMsgGetXattr, (char *)&outMsg,
                              sizeof(outMsgGetXattr));
  if ((sizeOfDataBuffer > 0) && (sizeOut > 0))
    l_ResponseMsg->addAttribute(txp::attributeData, value, sizeOut);

  ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
  if (retSSize == -1) {
    LOGERRNO(fshipd, error, ENOSYS);
    if (l_ResponseMsg) {
      delete l_ResponseMsg;
      l_ResponseMsg = 0;
    }
    return -errno;
  }

  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }
  return 0;
}

__s64 MessageHandler::listxattrOp(txp::Msg *pMsg) {
  txp::Attribute *pathAttribute = pMsg->retrieveAttr(txp::name);
  char *l_Path =
      (char *)pathAttribute->getDataPtr(); // to get addressability to the data

  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_LISTXATTR, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  txp::Attribute *size32Attr = pMsg->retrieveAttr(txp::size);
  uint32_t sizeOfDataBuffer = 0;
  int l_RC = size32Attr->cpyData(&sizeOfDataBuffer, sizeof(sizeOfDataBuffer));

  int buRC = becomeUser(in->hdr.uid, in->hdr.gid); // becomeUser(uid_t user)
                                                   // ensure identity is correct
                                                   // for authority check
  if (buRC)
    return sendErrorResponse(pMsg, in, EOWNERDEAD);

  char value[sizeOfDataBuffer];

  // llistxattr is like listxattr except will do a symlink itself instead of its
  // reference
  ssize_t sizeOut = llistxattr(l_Path, value, sizeOfDataBuffer);

  if (sizeOut == (-1)) {
    if (ENODATA != errno)
      LOG(fshipd, info) << "llistxattr l_Path=" << l_Path << " value="
                        << " sizeOfDataBuffer=" << sizeOfDataBuffer
                        << " sizeOut=" << sizeOut << " errno=" << errno;
    return sendErrorResponse(pMsg, in, errno);
  }
  LOG(fshipd, debug) << "llistxattr l_Path=" << l_Path
                     << " value=" << (sizeOfDataBuffer ? value : empty)
                     << " sizeOfDataBuffer=" << sizeOfDataBuffer
                     << " sizeOut=" << sizeOut;
  outMsgGetXattr outMsg(in->hdr.unique);
  outMsg.getxattr_out.size = sizeOut;

  // Build response Msg and then pick up elements from original message
  txp::Msg *l_ResponseMsg = 0;
  l_RC = pMsg->buildResponseMsg(l_ResponseMsg);
  assert(l_RC == 0);

  l_ResponseMsg->addAttribute(txp::outMsgGetXattr, (char *)&outMsg,
                              sizeof(outMsgGetXattr));
  if ((sizeOfDataBuffer > 0) && (sizeOut > 0)) {
    l_ResponseMsg->addAttribute(txp::attributeData, value, sizeOut);
  }
  ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
  if (retSSize == -1) {
    LOGERRNO(fshipd, error, ENOSYS);
    if (l_ResponseMsg) {
      delete l_ResponseMsg;
      l_ResponseMsg = 0;
    }
    return -errno;
  }

  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }
  return 0;
}

__s64 MessageHandler::setxattrOp(txp::Msg *pMsg) {

  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_SETXATTR, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  int buRC = becomeUser(in->hdr.uid, in->hdr.gid); // becomeUser(uid_t user)
                                                   // ensure identity is correct
                                                   // for authority check
  if (buRC)
    return sendErrorResponse(pMsg, in, EOWNERDEAD);

  txp::Attribute *pathAttribute = pMsg->retrieveAttr(txp::name);
  char *l_Path =
      (char *)pathAttribute->getDataPtr(); // to get addressability to the data

  txp::Attribute *xattrName = pMsg->retrieveAttr(txp::nameOfAttribute);
  char *l_nameOfAttribute = (char *)xattrName->getDataPtr();

  txp::Attribute *xattrData = pMsg->retrieveAttr(txp::data);
  void *l_data = (void *)xattrData->getDataPtr();

  txp::Attribute *xattrIn = pMsg->retrieveAttr(txp::fuse_setxattr_in);
  struct fuse_setxattr_in *l_setxattr_in =
      (struct fuse_setxattr_in *)xattrIn->getDataPtr();

  // lsetxattr is like setxattr except will do on symlink instead of reference
  int rc = lsetxattr(l_Path, l_nameOfAttribute, l_data, l_setxattr_in->size,
                     l_setxattr_in->flags);

  if (rc == (-1)) {
    return sendErrorResponse(pMsg, in, errno);
  } else {
    return sendErrorResponse(pMsg, in, 0);
  }
  return 0;
}

__s64 MessageHandler::removexattrOp(txp::Msg *pMsg) {

  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_REMOVEXATTR, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  int buRC = becomeUser(in->hdr.uid, in->hdr.gid); // becomeUser(uid_t user)
                                                   // ensure identity is correct
                                                   // for authority check
  if (buRC)
    return sendErrorResponse(pMsg, in, EOWNERDEAD);

  txp::Attribute *pathAttribute = pMsg->retrieveAttr(txp::name);
  char *l_Path =
      (char *)pathAttribute->getDataPtr(); // to get addressability to the data

  txp::Attribute *xattrName = pMsg->retrieveAttr(txp::nameOfAttribute);
  char *l_nameOfAttribute = (char *)xattrName->getDataPtr();

  // lremovexattr is like removexattr except will do on symlink if symlink
  // instead of reference
  int rc = lremovexattr(l_Path, l_nameOfAttribute);

  if (rc == (-1)) {
    return sendErrorResponse(pMsg, in, errno);
  } else {
    return sendErrorResponse(pMsg, in, 0);
  }
  return 0;
}

int MessageHandler::opendirOp(txp::Msg *pMsg) {
  txp::Attribute *pathAttribute = pMsg->retrieveAttr(txp::name);
  char *l_Path =
      (char *)pathAttribute->getDataPtr(); // to get addressability to the data

  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_OPENDIR, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  int buRC = becomeUser(in->hdr.uid, in->hdr.gid); // becomeUser(uid_t user)
                                                   // ensure identity is correct
                                                   // for authority check
  if (buRC)
    return sendErrorResponse(pMsg, in, EOWNERDEAD);

  int saved_errno = 0;
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_OPENDIRBEG, "Starting opendir() operation.  Pathlength=%ld", pathAttribute->getDataLength(), 0,0,0,0,0);
  /* clang-format on */
  DIR *dirp = opendir(l_Path);
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_OPENDIRFIN, "Completed opendir() operation.  Pathlength=%ld  DIR=%p errno=%ld", pathAttribute->getDataLength(), (uint64_t)((void*)dirp), errno,0,0,0);
  /* clang-format on */
  if (!dirp) {
    LOG(fshipd, always) << "uid:gid:pid=" << in->hdr.uid << ":" << in->hdr.gid
                        << ":" << in->hdr.pid << " opendir l_Path=" << l_Path
                        << " errno=" << errno << ":" << strerror(errno);
    return sendErrorResponse(pMsg, in, errno);
  }
  outMsgOpen outMsg(in->hdr.unique, saved_errno);
  outMsg.open_out.fh = (__u64)dirp;
  outMsg.open_out.open_flags = FOPEN_DIRECT_IO;

  // Build response Msg and then pick up elements from original message
  txp::Msg *l_ResponseMsg = 0;
  int l_RC = pMsg->buildResponseMsg(l_ResponseMsg);

  txp::AttrPtr_char_array *l_Attr = NULL;
  l_RC = txp::AttrPtr_char_array::buildAttrPtr_char_array(
      txp::outMsgOpen, (char *)&outMsg, sizeof(outMsgOpen), l_Attr);
  if (!l_RC) {
    l_RC = l_ResponseMsg->addAttribute(l_Attr);
  }
  if (l_RC)
    abort(); // messed up building attribue

  ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
  if (retSSize == -1) {
    LOGERRNO(fshipd, error, ENOSYS);
    if (l_ResponseMsg) {
      delete l_ResponseMsg;
      l_ResponseMsg = 0;
    }
    return -errno;
  }

  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }
  return 0;
}

int MessageHandler::readlinkOp(txp::Msg *pMsg) {
  txp::Attribute *pathAttribute = pMsg->retrieveAttr(txp::name);
  // txp::AttributeType l_TypeOfAttribute = pathAttribute->getAttrType() ;
  char *l_Path =
      (char *)pathAttribute->getDataPtr(); // to get addressability to the data
  // printf("readlink l_Path=%s\n",l_Path);

  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_READLINK, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  int buRC = becomeUser(in->hdr.uid, in->hdr.gid); // becomeUser(uid_t user)
                                                   // ensure identity is correct
                                                   // for authority check
  if (buRC)
    return sendErrorResponse(pMsg, in, EOWNERDEAD);

  // linux/limits.h:12:#define PATH_MAX        4096	/* # chars in a path name
  // including nul */
  size_t bufsiz = PATH_MAX;
  char buf[bufsiz];
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_READLINKBEG, "Starting readlink() operation.  Pathlength=%ld  Bufsize=%ld", pathAttribute->getDataLength(), bufsiz,0,0,0,0);
  /* clang-format on */
  ssize_t readlinkRCssize = readlink(l_Path, buf, bufsiz);
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_READLINKFIN, "Completed readlink() operation.  Pathlength=%ld  Bufsize=%ld  rcsize=%ld  errno=%ld", pathAttribute->getDataLength(), bufsiz,readlinkRCssize,errno,0,0);
  /* clang-format on */
  if (readlinkRCssize == -1)
    return sendErrorResponse(pMsg, in, errno);
  // readlink() does not append a null
  buf[readlinkRCssize] = 0; // add null terminator
  // printf("readlink buf=%s\n",buf);

  // Build response Msg and then pick up elements from original message
  txp::Msg *l_ResponseMsg = 0;
  int l_RC = pMsg->buildResponseMsg(l_ResponseMsg);
  outMsgReadlink outMsg(in->hdr.unique);

  l_ResponseMsg->addAttribute(txp::outMsgReadlink, (char *)&outMsg,
                              sizeof(outMsgReadlink));
  readlinkRCssize++; // include ending null in length
  l_ResponseMsg->addAttribute(txp::length, (uint64_t)readlinkRCssize);

  l_ResponseMsg->addAttribute(txp::link, buf, readlinkRCssize);

  if (l_RC)
    abort(); // messed up building attribue

  ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
  if (retSSize == -1) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -errno;
  }

  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }
  return 0;
}

int MessageHandler::releasedirOp(txp::Msg *pMsg) {
  txp::Attribute *l_Attr64fh = pMsg->retrieveAttr(txp::fh);
  DIR *dirp = NULL;
  int savedErrno = 0;
  int l_RC = l_Attr64fh->cpyData(&dirp, sizeof(dirp));
  if (!l_RC) {
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_CLOSEDIRBEG, "Starting closedir() operation.  dirp=%p ", (uint64_t)((void*)dirp),0,0,0,0,0);
    /* clang-format on */
    l_RC = closedir(dirp);
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_CLOSEDIRFIN, "Completed closedir() operation.  dirp=%p  rc=%ld  errno=%ld", (uint64_t)((void*)dirp), l_RC, errno,0,0,0);
    /* clang-format on */
    if (l_RC == -1)
      savedErrno = errno;
  }
  /* clang-format off */
  FL_Write(fl_fshiptx, TX_RELEASEDIR, "op=%-3ld dirp=%-24ld savedErrno=%-24ld rcClose=%ld", FUSE_RELEASEDIR, (uint64_t)dirp, savedErrno,l_RC);
  /* clang-format on */

  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  return sendErrorResponse(pMsg, in, 0);
}

// Release flags
//#define FUSE_RELEASE_FLUSH	(1 << 0)
//#define FUSE_RELEASE_FLOCK_UNLOCK	(1 << 1)
int MessageHandler::releaseOp(txp::Msg *pMsg) {

  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::inMsgRelease);
  inMsgRelease *inMsg = (inMsgRelease *)l_inHdrAttribute->getDataPtr();
  inMsgGeneric *in = (inMsgGeneric *)inMsg;

  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_CLOSEBEG, "Starting close() operation.  fh=%ld flags=%lx release_flags=%lx lock_owner=%ld" , inMsg->release_in.fh, inMsg->release_in.flags,inMsg->release_in.release_flags,inMsg->release_in.lock_owner,0,0);
  /* clang-format on */
  int l_RC = close(inMsg->release_in.fh);
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_CLOSEFIN, "Completed close() operation.  fh=%ld  rc=%ld  errno=%ld", inMsg->release_in.fh, l_RC, errno,0,0,0);
  /* clang-format on */
  if (l_RC == -1)
    return sendErrorResponse(pMsg, in, errno);

  return sendErrorResponse(pMsg, in, 0);
}

int MessageHandler::signal(txp::Msg *pMsg) {
  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::signalMsg);
  signalMsg *gbmPtr = (signalMsg *)l_inHdrAttribute->getDataPtr();
  LOG(fshipd, always) << "received signal=" << gbmPtr->signal;
  // Build response Msg
  txp::Msg *l_ResponseMsg = 0;
  int l_RC = pMsg->buildResponseMsg(l_ResponseMsg);
  if (l_RC)
    abort(); // messed up building attribute

  ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
  if (retSSize == -1) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -errno;
  }
  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }

  return 0;
}

int MessageHandler::unsupportedOp(txp::Msg *pMsg) {
  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  if (!l_inHdrAttribute) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -ENOSYS;
  }
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_UNSUPPORTED, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  outMsgGeneric outMsg(in->hdr.unique, -ENOSYS);

  // Build response Msg
  txp::Msg *l_ResponseMsg = 0;
  int l_RC = pMsg->buildResponseMsg(l_ResponseMsg);
  txp::AttrPtr_char_array *l_Attr = NULL;
  l_RC = txp::AttrPtr_char_array::buildAttrPtr_char_array(
      txp::outMsgGeneric, (char *)&outMsg, sizeof(outMsgGeneric), l_Attr);
  if (!l_RC) {
    l_RC = l_ResponseMsg->addAttribute(l_Attr);
  }
  if (l_RC)
    abort(); // messed up building attribute

  ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
  if (retSSize == -1) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -errno;
  }
  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }

  return 0;
}

int MessageHandler::errOp(txp::Msg *pMsg, __u64 pUnique, int pErrno) {

  outMsgGeneric outMsg(pUnique, -pErrno);

  // Build response Msg
  txp::Msg *l_ResponseMsg = 0;
  int l_RC = pMsg->buildResponseMsg(l_ResponseMsg);
  txp::AttrPtr_char_array *l_Attr = NULL;
  l_RC = txp::AttrPtr_char_array::buildAttrPtr_char_array(
      txp::outMsgGeneric, (char *)&outMsg, sizeof(outMsgGeneric), l_Attr);
  if (!l_RC) {
    l_RC = l_ResponseMsg->addAttribute(l_Attr);
  }
  if (l_RC)
    abort(); // messed up building attribute

  ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
  if (retSSize == -1) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -errno;
  }
  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }

  return 0;
}

int MessageHandler::readdirOp(txp::Msg *pMsg) {

  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  if (!l_inHdrAttribute) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -ENOSYS;
  }
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_READDIR, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  txp::Attribute *l_fuseReadInAttribute = pMsg->retrieveAttr(txp::fuse_read_in);
  if (!l_fuseReadInAttribute) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -ENOSYS;
  }
  struct fuse_read_in *l_fuse_read_in =
      (struct fuse_read_in *)l_fuseReadInAttribute->getDataPtr();
  int buRC = becomeUser(in->hdr.uid, in->hdr.gid); // becomeUser(uid_t user)
                                                   // ensure identity is correct
                                                   // for authority check
  if (buRC)
    return sendErrorResponse(pMsg, in, EOWNERDEAD);

  __u32 maxMemSize = l_fuse_read_in->size;
  DIR *dirp = (DIR *)l_fuse_read_in->fh;
  outMsgGeneric outMsg(in->hdr.unique);

  // Set up response message with pointers in order to compute size early
  txp::Msg *l_ResponseMsg = 0;
  int l_RC = pMsg->buildResponseMsg(l_ResponseMsg);
  l_RC = l_ResponseMsg->addAttribute(txp::fuse_in_header, (const char *)in,
                                     l_inHdrAttribute->getDataLength());
  txp::AttrPtr_char_array *l_Attr = NULL;
  l_RC = txp::AttrPtr_char_array::buildAttrPtr_char_array(
      txp::outMsgGeneric, (char *)&outMsg, sizeof(outMsgGeneric), l_Attr);
  if (!l_RC) {
    l_RC = l_ResponseMsg->addAttribute(l_Attr);
  }
  const uint64_t MessageReserveBuff = pMsg->getMsgLengthWithDataValues(64);

  memItemPtr mip = _connectPtr->getSendChunk();

  if (maxMemSize > (mip->chunkSize - MessageReserveBuff))
    maxMemSize = mip->chunkSize - MessageReserveBuff;

  char *charMemoryPtr = mip->address;
  void *memoryPtr = charMemoryPtr;
  struct fuse_dirent *l_direntPtr = (struct fuse_dirent *)memoryPtr;
  __u32 bytes_written = 0;

  const unsigned int maxFuseDirentSize = FUSE_NAME_OFFSET + NAME_MAX;
  if (maxMemSize < maxFuseDirentSize) {
    errOp(pMsg, in->hdr.unique, -ENOMEM);
  }
  size_t bottomBits = 0;
  struct dirent *result = NULL;
  struct dirent dent;

  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_READDIRBEG, "Starting readdir() operation.", 0,0,0,0,0,0);
  /* clang-format on */
  int rc = readdir_r(dirp, &dent, &result);
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_READDIRFIN, "Completed readdir() operation.  rc=%ld  errno=%ld", rc,errno,0,0,0,0);
  /* clang-format on */

  if (rc) { // nonzero if EBADF
    LOGERRNO(fshipd, error, errno);
    return -ENOMEM; //! \TODO send back an error message, build a common routine
  }

  while (result) {
    // printf("ino=%ld type=%d name=%s\n",dent.d_ino, dent.d_type, dent.d_name);
    l_direntPtr->ino = dent.d_ino;
    l_direntPtr->type = dent.d_type;
    l_direntPtr->namelen = strlen(dent.d_name);
    strncpy(l_direntPtr->name, dent.d_name, l_direntPtr->namelen);
    l_direntPtr->off = sizeof(struct fuse_dirent) + l_direntPtr->namelen;

    bottomBits = l_direntPtr->off & 7;
    if (bottomBits) {
      l_direntPtr->off += 8 - bottomBits;
    }
    charMemoryPtr += l_direntPtr->off;
    bytes_written += l_direntPtr->off;

    if ((maxMemSize - bytes_written) < maxFuseDirentSize) {
      break;
    }
    l_direntPtr = (struct fuse_dirent *)charMemoryPtr;
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_READDIRBEG2, "Starting readdir() operation.", 0,0,0,0,0,0);
    /* clang-format on */
    rc = readdir_r(dirp, &dent, &result);
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_READDIRFIN2, "Completed readdir() operation.  rc=%ld  errno=%ld", rc,errno,0,0,0,0);
    /* clang-format on */
  };

  // update a value included in the response message, then send
  outMsg.hdr.len += bytes_written;
  mip->length = bytes_written;

  ssize_t retSSize = _connectPtr->writeWithBuffer(l_ResponseMsg, mip);
  if ((uint64_t)retSSize > MessageReserveBuff + bytes_written) {
    LOG(fshipd, debug) << "readdir retSSize=" << retSSize << " msg length="
                       << l_ResponseMsg->getMsgLengthWithDataValues()
                       << " MessageReserveBuff=" << MessageReserveBuff;
    abort();
  }
  if (retSSize == -1) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -errno;
  }

  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }
  _connectPtr->sentSendChunk(mip);

  return 0;
}

int MessageHandler::readdirPlusOp(txp::Msg *pMsg) {

  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  if (!l_inHdrAttribute) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -ENOSYS;
  }
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_READDIRPLUS, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  txp::Attribute *l_fuseReadInAttribute = pMsg->retrieveAttr(txp::fuse_read_in);
  if (!l_fuseReadInAttribute) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -ENOSYS;
  }
  struct fuse_read_in *l_fuse_read_in =
      (struct fuse_read_in *)l_fuseReadInAttribute->getDataPtr();
  int buRC = becomeUser(in->hdr.uid, in->hdr.gid); // becomeUser(uid_t user)
                                                   // ensure identity is correct
                                                   // for authority check
  if (buRC)
    return sendErrorResponse(pMsg, in, EOWNERDEAD);

  /*
  struct fuse_read_in {
          uint64_t	fh;
          uint64_t	offset;
          uint32_t	size;
          uint32_t	read_flags;
          uint64_t	lock_owner;
          uint32_t	flags;
          uint32_t	padding;
  };*/

  __u32 maxMemSize = l_fuse_read_in->size;
  DIR *dirp = (DIR *)l_fuse_read_in->fh;
  const int l_dirfd = dirfd(dirp); // need for fstatat
  outMsgGeneric outMsg(in->hdr.unique);

  // Set up response message with pointers in order to compute size early
  txp::Msg *l_ResponseMsg = 0;
  int l_RC = pMsg->buildResponseMsg(l_ResponseMsg);

  l_RC = l_ResponseMsg->addAttribute(txp::fuse_in_header, (const char *)in,
                                     l_inHdrAttribute->getDataLength());
  txp::AttrPtr_char_array *l_Attr = NULL;
  l_RC = txp::AttrPtr_char_array::buildAttrPtr_char_array(
      txp::outMsgGeneric, (char *)&outMsg, sizeof(outMsgGeneric), l_Attr);
  if (!l_RC) {
    l_RC = l_ResponseMsg->addAttribute(l_Attr);
  }
  const uint64_t MessageReserveBuff = pMsg->getMsgLengthWithDataValues(64);

  memItemPtr mip = _connectPtr->getSendChunk(maxMemSize);
  if (maxMemSize > (mip->chunkSize - MessageReserveBuff))
    maxMemSize = mip->chunkSize - MessageReserveBuff;

  char *charMemoryPtr = mip->address;
  void *memoryPtr = charMemoryPtr;
  struct fuse_direntplus *l_direntPlusPtr = (struct fuse_direntplus *)memoryPtr;
  __u32 bytes_written = 0;

  const unsigned int maxFusePlusDirentSize =
      sizeof(struct fuse_entry_out) + FUSE_NAME_OFFSET + NAME_MAX;
  if (maxMemSize < maxFusePlusDirentSize) {
    errOp(pMsg, in->hdr.unique, -ENOMEM);
    abort();
  }

  struct dirent *result = NULL;
  struct dirent dent;
  struct stat l_stat;
  int flags = AT_SYMLINK_NOFOLLOW;
  int l_statRC = 0;
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_READDIRBEG3, "Starting readdir() operation.", 0,0,0,0,0,0);
  /* clang-format on */
  int rc = readdir_r(dirp, &dent, &result);
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_READDIRFIN3, "Completed readdir() operation.  rc=%ld  errno=%ld", rc,errno,0,0,0,0);
  /* clang-format on */

  size_t reclen = 0;

  if (rc) { // nonzero if EBADF
    LOGERRNO(fshipd, error, errno);
    return -ENOMEM; //! \TODO send back an error message, build a common routine
  }

  while (result) {
    // printf("ino=%ld type=%d name=%s\n",dent.d_ino, dent.d_type, dent.d_name);
    { // to scope fuse_dirent
      memset(l_direntPlusPtr, 0, sizeof(struct fuse_direntplus));
      struct fuse_dirent *l_direntPtr = &l_direntPlusPtr->dirent;
      l_direntPtr->ino = dent.d_ino;
      l_direntPtr->type = dent.d_type;
      l_direntPtr->namelen = strlen(dent.d_name);
      strncpy(l_direntPtr->name, dent.d_name, l_direntPtr->namelen);
      l_direntPtr->off = 0; // not used by fuse module
      // printf("dent.d_ino=%ld dent.d_type=%d dent.d_name=%s
      // \n",dent.d_ino,dent.d_type, dent.d_name);
    }
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_STATATBEGDP, "Start statat DIRFD=%ld, flags=0x%lx.", l_dirfd,flags,0,0,0,0);
    /* clang-format on */
    memset(&l_stat, 0, sizeof(l_stat));
    l_statRC = fstatat(l_dirfd, dent.d_name, &l_stat, flags);
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_STATATFINDP, "End statat dirplus.  rc=%ld  errno=%ld st_ino=%ld mode=%o uid=%ld gid=%ld", l_statRC, errno,l_stat.st_ino,l_stat.st_mode, l_stat.st_uid,l_stat.st_gid);
    /* clang-format on */

    if (l_statRC == 0) {
      setEntryOut(l_stat, l_direntPlusPtr->entry_out);
      // printf("dirplus st_ino=%ld mode-%o \n",l_stat.st_ino,l_stat.st_mode);
      if (!dent.d_type) { // file system does not support doing type, so grab
                          // from stat
        if (S_ISDIR(l_stat.st_mode))
          l_direntPlusPtr->dirent.type = DT_DIR;
        else if (S_ISREG(l_stat.st_mode))
          l_direntPlusPtr->dirent.type = DT_REG;
        else if (S_ISLNK(l_stat.st_mode))
          l_direntPlusPtr->dirent.type = DT_LNK;
      }
    } else {
      l_direntPlusPtr->entry_out.attr_valid = 0;
      l_direntPlusPtr->entry_out.attr_valid_nsec = 1; // nanosecond
      l_direntPlusPtr->entry_out.entry_valid =
          0; // Cache timeout for the name (seconds) see fuse module dir.c
      l_direntPlusPtr->entry_out.entry_valid_nsec = 1;
      memset(&l_direntPlusPtr->entry_out.attr, 0,
             sizeof(l_direntPlusPtr->entry_out.attr));
    }

    reclen = FUSE_DIRENTPLUS_SIZE(l_direntPlusPtr);
    charMemoryPtr += reclen;
    bytes_written += reclen;
    if ((maxMemSize - bytes_written) < MAXDIRENTPLUSENTRY) {
      break;
    }
    l_direntPlusPtr = (struct fuse_direntplus *)charMemoryPtr;

    readdir_r(dirp, &dent, &result);
  };

  // change value in Response header for bytes written to buffer
  outMsg.hdr.len += bytes_written;
  mip->length = bytes_written;

  ssize_t retSSize = _connectPtr->writeWithBuffer(l_ResponseMsg, mip);
  if ((uint64_t)retSSize > MessageReserveBuff + bytes_written) {
    LOG(fshipd, debug) << "readdirPlusOp retSSize=" << retSSize
                       << " msg length="
                       << l_ResponseMsg->getMsgLengthWithDataValues()
                       << " MessageReserveBuff=" << MessageReserveBuff;
    abort();
  }
  if (retSSize == -1) {
    LOGERRNO(fshipd, error, ENOSYS);
    if (l_ResponseMsg) {
      delete l_ResponseMsg;
      l_ResponseMsg = 0;
    }
    return -errno;
  }

  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }
  _connectPtr->sentSendChunk(mip);

  return 0;
}

int MessageHandler::getattrOp(txp::Msg *pMsg) {
  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  if (!l_inHdrAttribute) {
    LOGERRNO(fshipd, error, ENOSYS);
    abort();
  }
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_GETATTR, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  int buRC = becomeUser(in->hdr.uid, in->hdr.gid); // becomeUser(uid_t user)
                                                   // ensure identity is correct
                                                   // for authority check
  if (buRC)
    return sendErrorResponse(pMsg, in, EOWNERDEAD);

  txp::Attribute *l_Attr64fh = pMsg->retrieveAttr(txp::fh);
  txp::Attribute *pathAttribute = pMsg->retrieveAttr(txp::name);
  char *l_Path = NULL;
  uint64_t filehandle = -1;

  if (l_Attr64fh) {
    int l_RC = l_Attr64fh->cpyData(&filehandle, sizeof(filehandle));
    if (l_RC)
      abort();
  }
  if (pathAttribute) {
    l_Path =
        (char *)
            pathAttribute->getDataPtr(); // to get addressability to the data
  }

  struct stat l_stat;

  int l_statRC = 0;
  if (l_Path != NULL) {
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_STATATBEG4, "Starting statat(FH=%ld) operation.", filehandle,0,0,0,0,0);
    /* clang-format on */
    l_statRC =
        fstatat(filehandle, l_Path, &l_stat,
                AT_SYMLINK_NOFOLLOW); // 0 or int flags= AT_SYMLINK_NOFOLLOW;
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_STATATFIN4, "Completed statat() operation. rc=%ld  errno=%ld st_ino=%ld mode=%o uid=%ld gid=%ld", l_statRC, errno,l_stat.st_ino,l_stat.st_mode, l_stat.st_uid,l_stat.st_gid);
    /* clang-format on */

  } else { // try filehandle by itself
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_STATBEG, "Starting stat(FH=%ld) operation.", filehandle,0,0,0,0,0);
    /* clang-format on */
    l_statRC = fstat(filehandle, &l_stat);
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_STATFIN, "Completed stat() operation.  rc=%ld  errno=%ld st_ino=%ld mode=%o uid=%ld gid=%ld", l_statRC, errno,l_stat.st_ino,l_stat.st_mode, l_stat.st_uid,l_stat.st_gid);
    /* clang-format on */
  }

  if (l_statRC)
    return sendErrorResponse(pMsg, in, errno);
  outMsgGetattr outMsg(in->hdr.unique);
  setAttrOut(l_stat, outMsg.getattr_out);

  uint64_t prevRemoteInode = 0;
  txp::Attribute *l_AttrPrevRemoteInode = pMsg->retrieveAttr(txp::nodeid);
  if (l_AttrPrevRemoteInode) {
    int l_RC = l_AttrPrevRemoteInode->cpyData(&prevRemoteInode,
                                              sizeof(prevRemoteInode));
    if (l_RC)
      abort();
  }

  // Build response Msg and then pick up elements from original message
  txp::Msg *l_ResponseMsg = 0;
  int l_RC = pMsg->buildResponseMsg(l_ResponseMsg);
  if (l_RC) {
    printf("%s l_RC=%d\n", __PRETTY_FUNCTION__, l_RC);
    return l_RC;
  }
  l_RC = l_ResponseMsg->addAttribute(txp::fuse_in_header, (const char *)in,
                                     l_inHdrAttribute->getDataLength());
  l_ResponseMsg->addAttribute(txp::outMsgGetattr, (char *)&outMsg,
                              sizeof(outMsg));
  if (!l_RC)
    l_RC = l_ResponseMsg->addAttribute(txp::nodeid, prevRemoteInode);

  if (!l_RC) {
    ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
    if (retSSize == -1) {
      LOGERRNO(fshipd, error, errno);
      if (l_ResponseMsg) {
        delete l_ResponseMsg;
        l_ResponseMsg = 0;
      }
      abort();
    }
  }

  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }
  return 0;
}

int MessageHandler::addlookupname(txp::Msg *pMsg, char *pPath, int pLength,
                                  txp::AttributeName attrName) {
  txp::AttrPtr_char_array *l_Attr = NULL;
  int l_RC = txp::AttrPtr_char_array::buildAttrPtr_char_array(attrName, pPath,
                                                              pLength, l_Attr);
  if (!l_RC) {
    l_RC = pMsg->addAttribute(l_Attr);
  }
  if (l_RC)
    abort(); // messed up building message

  return l_RC;
}

int MessageHandler::lookupOp(txp::Msg *pMsg) {
  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  if (!l_inHdrAttribute) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -ENOSYS;
  }
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */
  FL_Write6(fl_fshiptx, TX_LOOKUP, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  int buRC = becomeUser(in->hdr.uid, in->hdr.gid); // becomeUser(uid_t user)
                                                   // ensure identity is correct
                                                   // for authority check
  if (buRC)
    return sendErrorResponse(pMsg, in, EOWNERDEAD);

  txp::Attribute *l_Attr64fh = pMsg->retrieveAttr(txp::fh);
  txp::Attribute *l_Attr64offset = pMsg->retrieveAttr(txp::offset);

  txp::Attribute *pathAttribute = pMsg->retrieveAttr(txp::name);
  char *l_Path = NULL;
  uint64_t filehandle = -1;

  uint64_t indexToLookupName = 0;
  char *lookUpName = NULL;

  if (l_Attr64offset) {
    int l_RC =
        l_Attr64offset->cpyData(&indexToLookupName, sizeof(indexToLookupName));
    if (l_RC)
      abort();
  }

  if (l_Attr64fh) {
    int l_RC = l_Attr64fh->cpyData(&filehandle, sizeof(filehandle));
    if (l_RC)
      abort();
  }
  if (pathAttribute) {
    //        txp::AttributeType l_TypeOfAttribute =
    //        pathAttribute->getAttrType() ;
    l_Path =
        (char *)
            pathAttribute->getDataPtr(); // to get addressability to the data

    if (l_Attr64offset)
      lookUpName = l_Path + indexToLookupName;
  }

  struct stat l_stat;
  int flags = AT_SYMLINK_NOFOLLOW;
  int l_statRC = 0;
  int saved_errno = 0;
  if (l_Path != NULL) {
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_STATATBEG5, "Completed statat(FH=%ld, flags=0x%lx) operation.", filehandle, flags,0,0,0,0);
    /* clang-format on */
    l_statRC = fstatat(filehandle, l_Path, &l_stat, flags);
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_STATATFIN5, "Completed statat() operation.rc=%ld  errno=%ld st_ino=%ld mode=%o uid=%ld gid=%ld", l_statRC, errno,l_stat.st_ino,l_stat.st_mode, l_stat.st_uid,l_stat.st_gid);
    /* clang-format on */
    if (l_statRC)
      saved_errno = errno;
  } else { // try filehandle by itself
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_STATBEG2, "Completed stat(FH=%ld) operation.", filehandle,0,0,0,0,0);
    /* clang-format on */
    l_statRC = fstat(filehandle, &l_stat);
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_STATFIN2, "Completed stat() operationrc=%ld  errno=%ld st_ino=%ld mode=%o uid=%ld gid=%ld", l_statRC, errno,l_stat.st_ino,l_stat.st_mode, l_stat.st_uid,l_stat.st_gid);
    /* clang-format on */
    if (l_statRC)
      saved_errno = errno;
  }
  outMsgLookup outMsg(in->hdr.unique, -saved_errno);
  memset(&outMsg.entry_out, 0, sizeof(struct fuse_entry_out));
  if (!l_statRC) {
    setEntryOut(l_stat, outMsg.entry_out);
  }

  // Build response Msg and then pick up elements from original message
  txp::Msg *l_ResponseMsg = 0;
  int l_RC = pMsg->buildResponseMsg(l_ResponseMsg);

  assert_perror(l_RC);

  l_RC = l_ResponseMsg->addAttribute(txp::fuse_in_header, (const char *)in,
                                     l_inHdrAttribute->getDataLength());
  txp::AttrPtr_char_array *l_Attr = NULL;
  l_RC = txp::AttrPtr_char_array::buildAttrPtr_char_array(
      txp::outMsgLookup, (char *)&outMsg, sizeof(outMsg), l_Attr);
  if (!l_RC) {
    l_RC = l_ResponseMsg->addAttribute(l_Attr);
  }
  if (lookUpName) {
    int l_lookUpNameLength = strlen(lookUpName) + 1; // get null terminator
    l_RC = l_ResponseMsg->addAttribute(txp::lookupname, lookUpName,
                                       l_lookUpNameLength);
    // l_RC = addlookupname(l_ResponseMsg,lookUpName,
    // l_lookUpNameLength,txp::lookupname);
  };

  if (!l_RC) {
    ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
    if (retSSize == -1) {
      LOGERRNO(fshipd, error, errno);
      abort();
    }
  }

  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }
  return 0;
}

int MessageHandler::Hello(txp::Msg *pMsg) {
  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  if (!l_inHdrAttribute) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -ENOSYS;
  }
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_HELLO, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  txp::AttrPtr_char_array *l_inHelloAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::inHello);
  inHello *l_HelloPtr = (inHello *)l_inHelloAttribute->getDataPtr();

  txp::Version l_Version = pMsg->getVersion();
  _clientMajor4Msg = l_Version.getMajor();
  _clientMinor4Msg = l_Version.getMinor();

  // Save off the path to the directory the server will use on behalf of the
  // client
  txp::Attribute *mountAttribute = pMsg->retrieveAttr(txp::name);
  //    txp::AttributeType l_TypeOfAttribute = mountAttribute->getAttrType() ;
  int32_t l_LengthOfAttribute =
      mountAttribute->getDataLength(); // to get the length of the value
  _mountPath = new char[l_LengthOfAttribute + 1];
  //    void* getDataPtr()    to get addressability to the data
  char *mountPath =
      (char *)mountAttribute->getDataPtr(); // to get addressability to the data
  strncpy(_mountPath, mountPath, l_LengthOfAttribute);
  _mountPath[l_LengthOfAttribute] = '\0';
  LOG(fshipd, always) << "_mountPath=" << _mountPath
                      << " Hello remote pid=" << l_HelloPtr->daemon_pid;

  _generation =
      l_HelloPtr
          ->generation; /* Inode generation: nodeid:gen must be unique for the
                           fs's lifetime */
  _entry_valid = l_HelloPtr->entry_valid; /* Cache timeout for the name */
  _attr_valid = l_HelloPtr->attr_valid;   /* Cache timeout for the attributes */
  _entry_valid_nsec = l_HelloPtr->entry_valid_nsec;
  _attr_valid_nsec = l_HelloPtr->attr_valid_nsec;
  LOG(fshipd, always) << "_entry_valid=" << _entry_valid
                      << " _entry_valid_nsec=" << _entry_valid_nsec;
  LOG(fshipd, always) << " _attr_valid=" << _attr_valid
                      << " _attr_valid_nsec=" << _attr_valid_nsec;
  LOG(fshipd, always) << " _generation=" << _generation;

  _openOutFlags = l_HelloPtr->openOutFlags; // includes decision on DIRECT_IO
  LOG(fshipd, always) << " _openOutFlags=" << std::hex << _openOutFlags
                      << std::dec;
  int chdirRC = chdir(_mountPath);
  if (chdirRC) {
    LOG(fshipd, critical) << "_mountPath=" << _mountPath
                          << " chdir errno=" << errno << ":" << strerror(errno);
    return sendErrorResponse(pMsg, in, errno);
  }
  outHello outMsg(getpid());
  outMsg.statfsRC = statfs(_mountPath, &outMsg.sfs);
  if (outMsg.statfsRC) {

    LOG(fshipd, critical) << "statfs errno=" << errno << ":" << strerror(errno);
  } else {
    std::string fstype(" ");
    if (outMsg.sfs.f_type == GPFS_SUPER_MAGIC)
      fstype = ":GPFS";
    LOG(fshipd, always) << std::hex << " fs type=0x" << outMsg.sfs.f_type
                        << fstype << " Optimal xfer block size=0x"
                        << (int)outMsg.sfs.f_bsize << " mount flags=0x"
                        << (int)outMsg.sfs.f_flags << std::dec;
  }

  // Build response Msg and then pick up elements from original message
  txp::Msg *l_ResponseMsg = 0;
  int l_RC = pMsg->buildResponseMsg(l_ResponseMsg);
  if (l_RC) {
    abort();
  }
  l_RC = l_ResponseMsg->addAttribute(txp::fuse_in_header, (const char *)in,
                                     l_inHdrAttribute->getDataLength());
  if (!l_RC)
    l_RC = l_ResponseMsg->addAttribute(txp::outHello, (const char *)&outMsg,
                                       sizeof(outHello));
  if (!l_RC) {

    ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
    if (retSSize == -1) {
      LOGERRNO(fshipd, error, errno);
      return -errno;
    }
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  } else
    abort();

  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }
  return 0; // version check....
};

void *workerThread(void *MHPtr) {
  MessageHandler *mythis = (MessageHandler *)MHPtr;
  mythis->run();
  return NULL;
}

void *MessageHandler::run() {
  int rc;
  pid_t tid = syscall(SYS_gettid);
  LOG(fshipd, always) << "fshipd thread lwp pid=" << tid;
  do {
    sem_wait(&threadSerialSem);
    rc = _connectPtr->poll4DataIn();
    if (rc != 1) {
      sem_post(&threadSerialSem);
      break;
    }
    rc = readInMessage();
  } while (1);

  if (rc == 0) {
    LOG(fshipd, always) << "MessageHandler::run(): Ending normally";
  } else {
    LOG(fshipd, always) << "MessageHandler::run(): Ending with rc=" << rc;
  }
  return NULL;
}

void *MessageHandler::run(unsigned numthreads) {
  unsigned x;
  int rc;
  pthread_t tid;
  void *rtnvalue;
  std::vector<pthread_t> blocklist;
  LOG(fshipd, always) << "numthreads=" << x << " main pid=" << getpid();
  for (x = 0; x < numthreads; x++) {
    rc = pthread_create(&tid, NULL, workerThread, this);
    if (rc != 0) {
      LOG(fshipd, always) << "MessageHandler::run(unsigned): break with rc="
                          << rc;
      break;
    }
    blocklist.push_back(tid);
  }
  for (auto tid : blocklist) {
    rc = pthread_join(tid, &rtnvalue);
  }
  return NULL;
}

int MessageHandler::readInMessage() {
  int l_RC = 0;

  txp::Msg *l_MsgPtr = 0;
  memItemPtr l_memItemPtr = _connectPtr->getReadChunk();
  if (!l_memItemPtr) {
    sem_post(&threadSerialSem);
    return 1;
  }
  l_RC =
      _connectPtr->read(l_MsgPtr, l_memItemPtr->address, l_memItemPtr->length);
  sem_post(&threadSerialSem);

  if (l_RC == 0) {
    txp::Id l_Id = l_MsgPtr->getMsgId();
    // LOG(info) << "FUSE ID="<<l_Id;
    switch (l_Id) {
    case txp::FUSE_LOOKUP:
      lookupOp(l_MsgPtr);
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
    case txp::FUSE_MKDIR:
      mkdirOp(l_MsgPtr);
      break;
    case txp::FUSE_UNLINK:
      unlinkatOp(l_MsgPtr);
      break;
    case txp::FUSE_RMDIR:
      unlinkatOp(l_MsgPtr);
      break;
    case txp::FUSE_RENAME:
      renameOp(l_MsgPtr);
      break;
    case txp::FUSE_LINK:
      linkOp(l_MsgPtr);
      break;
    case txp::FUSE_OPEN:
      openOp(l_MsgPtr);
      break;
    case txp::FUSE_READ:
      // readOp(l_MsgPtr);
      readOp(l_memItemPtr, l_MsgPtr);
      break;
    case txp::FUSE_WRITE:
      // writeOp(l_MsgPtr);
      writeOp(l_memItemPtr, l_MsgPtr);
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
      abort();
      break; // comes over as Hello instead
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
    case txp::FUSE_SETLKW: // no blocking a worker thread!
      setlkOp(l_MsgPtr);
      break;
    case txp::FUSE_ACCESS:
      accessOp(l_MsgPtr);
      break;
    case txp::FUSE_CREATE:
      creatOp(l_MsgPtr);
      break;
    case txp::FUSE_MKNOD:
      mknodOp(l_MsgPtr);
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
    case txp::CUSE_INIT:
      abort();
      break; // init is handled by CORAL_HELLO
      break;
    case txp::CORAL_HELLO:
      Hello(l_MsgPtr);
      break;
    case txp::CORAL_NO_OP:
      break;

    case txp::CORAL_AUTHENTICATE:
      abort();
      break;
    case txp::CORAL_GOODBYE:
      _connectPtr->disconnect();
      break;
    case txp::CORAL_SIGNAL:
      signal(l_MsgPtr);
      break;
    case txp::CORAL_ERROR:
      return -1;
      abort();
      break;
    case txp::FUSE_GETXATTR:
      getxattrOp(l_MsgPtr);
      break;

    case txp::FUSE_SETXATTR:
      setxattrOp(l_MsgPtr);
      break;
    case txp::FUSE_LISTXATTR:
      listxattrOp(l_MsgPtr);
      break;
    case txp::FUSE_REMOVEXATTR:
      removexattrOp(l_MsgPtr);
      break;
    // not supported
    // case txp::FUSE_FSYNCDIR:abort();break;
    default:
      abort();
      break;
    }
  }

  if (l_MsgPtr) {
    delete l_MsgPtr;
    l_MsgPtr = 0;
  }
  if (l_memItemPtr)
    _connectPtr->freeReadChunk(l_memItemPtr);

  return l_RC;
};

int MessageHandler::unlinkatOp(txp::Msg *pMsg) { // remove dir or file
  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  if (!l_inHdrAttribute) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -ENOSYS;
  }
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_UNLINK, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */
  int buRC = becomeUser(in->hdr.uid, in->hdr.gid); // becomeUser(uid_t user)
                                                   // ensure identity is correct
                                                   // for authority check
  if (buRC)
    return sendErrorResponse(pMsg, in, EOWNERDEAD);

  txp::Attribute *l_Attr64fh = pMsg->retrieveAttr(txp::fh);

  txp::Attribute *pathAttribute = pMsg->retrieveAttr(txp::name);
  char *l_Path = NULL;
  uint64_t filehandle = -1;

  if (l_Attr64fh) {
    int l_RC = l_Attr64fh->cpyData(&filehandle, sizeof(filehandle));
    if (l_RC)
      abort();
  }

  if (pathAttribute) {
    l_Path =
        (char *)
            pathAttribute->getDataPtr(); // to get addressability to the data
  }

  outMsgGeneric outMsg(in->hdr.unique);
  int unlinkatflags = 0;
  if (in->hdr.opcode == FUSE_RMDIR)
    unlinkatflags = AT_REMOVEDIR;
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_UNLINKATBEG, "Starting unlinkat() operation.", 0,0,0,0,0,0);
  /* clang-format on */
  int l_unlinkatRC = unlinkat(filehandle, l_Path, unlinkatflags);
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_UNLINKATFIN, "Completed unlinkat() operation.  rc=%ld  errno=%ld", l_unlinkatRC,errno,0,0,0,0);
  /* clang-format on */

  if (l_unlinkatRC)
    outMsg.hdr.error = -errno;

  // Build response Msg and then pick up elements from original message
  txp::Msg *l_ResponseMsg = 0;
  int l_RC = pMsg->buildResponseMsg(l_ResponseMsg);
  if (l_RC) {
    abort();
  }
  l_RC = l_ResponseMsg->addAttribute(txp::fuse_in_header, (const char *)in,
                                     l_inHdrAttribute->getDataLength());
  txp::AttrPtr_char_array *l_Attr = NULL;
  l_RC = txp::AttrPtr_char_array::buildAttrPtr_char_array(
      txp::outMsgGeneric, (char *)&outMsg, sizeof(outMsg), l_Attr);
  if (!l_RC) {
    l_RC = l_ResponseMsg->addAttribute(l_Attr);
  }

  if (!l_RC) {
    ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
    if (retSSize == -1) {
      LOGERRNO(fshipd, error, errno);
      abort();
    }
  }

  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }
  return 0;
}

int MessageHandler::symlinkOp(txp::Msg *pMsg) {
  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  if (!l_inHdrAttribute) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -ENOSYS;
  }
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_SYMLINKOP, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  int buRC = becomeUser(in->hdr.uid, in->hdr.gid); // becomeUser(uid_t user)
                                                   // ensure identity is correct
                                                   // for authority check
  if (buRC)
    return sendErrorResponse(pMsg, in, EOWNERDEAD);

  txp::Attribute *oldpathAttribute = pMsg->retrieveAttr(txp::oldname);
  txp::Attribute *newpathAttribute = pMsg->retrieveAttr(txp::newname);
  char *l_oldPath = NULL;
  char *l_newPath = NULL;

  if (!oldpathAttribute)
    abort();
  if (!newpathAttribute)
    abort();

  l_oldPath =
      (char *)
          oldpathAttribute->getDataPtr(); // to get addressability to the data
  l_newPath =
      (char *)
          newpathAttribute->getDataPtr(); // to get addressability to the data

  // int symlink(const char *oldpath, const char *newpath);
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_SYMLINKBEG, "Starting symlink() operation.", 0,0,0,0,0,0);
  /* clang-format on */
  int symlinkRC = symlink(l_oldPath, l_newPath);
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_SYMLINKFIN, "Completed symlink() operation symlinkRc=%ld errno=%ld", symlinkRC, errno,0,0,0,0);
  /* clang-format on */

  if (symlinkRC)
    return sendErrorResponse(pMsg, in, errno);

  struct stat l_stat;
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_LSTATBEG, "Starting lstat() operation.", 0,0,0,0,0,0);
  /* clang-format on */
  int lstatRC = lstat(l_newPath, &l_stat);
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_LSTATFIN, "Completed lstat() operation.", lstatRC, errno,0,0,0,0);
  /* clang-format on */

  if (lstatRC)
    return sendErrorResponse(pMsg, in, errno);

  uint64_t offset2newname = 0;
  txp::Attribute *l_offset2newnameAttribute =
      pMsg->retrieveAttr(txp::offset2newname);
  int l_RC = l_offset2newnameAttribute->cpyData(&offset2newname,
                                                sizeof(offset2newname));
  if (l_RC)
    abort();
  char *newName = l_newPath + offset2newname;

  outMsgSymlink outMsg(in->hdr.unique);
  setEntryOut(l_stat, outMsg.entry_out);

  // Build response Msg and then pick up elements from original message
  txp::Msg *l_ResponseMsg = 0;
  l_RC = pMsg->buildResponseMsg(l_ResponseMsg);
  if (l_RC) {
    abort();
  }
  l_RC = l_ResponseMsg->addAttribute(txp::fuse_in_header, (const char *)in,
                                     l_inHdrAttribute->getDataLength());
  l_RC = l_ResponseMsg->addAttribute(txp::outMsgSymlink, (char *)&outMsg,
                                     sizeof(outMsgSymlink));

  if (l_RC)
    abort(); // messed up building attribute
  // send back last known inode number for cleanup

  if (newName) {
    int l_lookUpNameLength = strlen(newName) + 1; // get null terminator
    l_RC =
        addlookupname(l_ResponseMsg, newName, l_lookUpNameLength, txp::newname);
  };

  if (!l_RC) {
    ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
    if (retSSize == -1) {
      LOGERRNO(fshipd, error, errno);
      abort();
    }
  }

  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }
  return 0;
}

int MessageHandler::linkOp(txp::Msg *pMsg) {
  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  if (!l_inHdrAttribute) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -ENOSYS;
  }
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_LINKOP, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  int buRC = becomeUser(in->hdr.uid, in->hdr.gid); // becomeUser(uid_t user)
                                                   // ensure identity is correct
                                                   // for authority check
  if (buRC)
    return sendErrorResponse(pMsg, in, EOWNERDEAD);

  txp::Attribute *oldpathAttribute = pMsg->retrieveAttr(txp::oldname);
  txp::Attribute *newpathAttribute = pMsg->retrieveAttr(txp::newname);
  char *l_oldPath = NULL;
  char *l_newPath = NULL;

  if (!oldpathAttribute)
    abort();
  if (!newpathAttribute)
    abort();

  l_oldPath =
      (char *)
          oldpathAttribute->getDataPtr(); // to get addressability to the data
  l_newPath =
      (char *)
          newpathAttribute->getDataPtr(); // to get addressability to the data

  // int symlink(const char *oldpath, const char *newpath);
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_LINKBEG, "Starting link() operation.", 0,0,0,0,0,0);
  /* clang-format on */
  int linkRC = link(l_oldPath, l_newPath);
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_LINKFIN, "Completed link() operation.", linkRC, errno,0,0,0,0);
  /* clang-format on */

  if (linkRC)
    return sendErrorResponse(pMsg, in, errno);

  struct stat l_stat;
  int flags = AT_SYMLINK_NOFOLLOW;
  int l_statRC = 0;

  int filehandle = -1;
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_STATATBEG6, "Completed statat(FH=%ld, flags=0x%lx) operation.", filehandle,flags,0,0,0,0);
  /* clang-format on */
  l_statRC = fstatat(filehandle, l_oldPath, &l_stat, flags);
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_STATATFIN6, "Completed statat() operation. rc=%ld  errno=%ld st_ino=%ld mode=%o uid=%ld gid=%ld", l_statRC, errno,l_stat.st_ino,l_stat.st_mode, l_stat.st_uid,l_stat.st_gid);
  /* clang-format on */
  if (l_statRC)
    sendErrorResponse(pMsg, in, errno);

  outMsgLink outMsg(in->hdr.unique, 0);
  setEntryOut(l_stat, outMsg.entry_out);

  uint64_t prevRemoteInode = 0;
  txp::Attribute *l_AttrPrevRemoteInode = pMsg->retrieveAttr(txp::nodeid);
  assert(l_AttrPrevRemoteInode != NULL);
  int l_RC =
      l_AttrPrevRemoteInode->cpyData(&prevRemoteInode, sizeof(prevRemoteInode));
  assert(l_RC == 0);
  assert(prevRemoteInode != 0);

  txp::Attribute *l_AttrOldFuseInode = pMsg->retrieveAttr(txp::oldnodeid);
  uint64_t oldInode = 0;
  assert(l_AttrPrevRemoteInode != NULL);
  l_RC = l_AttrOldFuseInode->cpyData(&oldInode, sizeof(oldInode));
  assert(l_RC == 0);
  assert(oldInode != 0);

  // Build response Msg and then pick up elements from original message
  txp::Msg *l_ResponseMsg = 0;
  l_RC = pMsg->buildResponseMsg(l_ResponseMsg);
  assert(l_RC == 0);
  l_RC = l_ResponseMsg->addAttribute(txp::fuse_in_header, (const char *)in,
                                     l_inHdrAttribute->getDataLength());
  if (!l_RC)
    l_RC = l_ResponseMsg->addAttribute(txp::outMsgLink, (const char *)&outMsg,
                                       sizeof(outMsg));
  if (!l_RC)
    l_RC = l_ResponseMsg->addAttribute(txp::nodeid, prevRemoteInode);
  if (!l_RC)
    l_RC = l_ResponseMsg->addAttribute(txp::oldnodeid, oldInode);
  if (l_RC)
    abort();

  if (!l_RC) {
    ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
    if (retSSize == -1) {
      LOGERRNO(fshipd, error, errno);
      abort();
    }
  }

  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }
  return 0;
}

int MessageHandler::renameOp(txp::Msg *pMsg) {
  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  if (!l_inHdrAttribute) {
    LOGERRNO(fshipd, error, ENOSYS);
    abort();
    return -ENOSYS;
  }
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_RENAME, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  outMsgGeneric outMsg(in->hdr.unique);
  int buRC = becomeUser(in->hdr.uid, in->hdr.gid); // becomeUser(uid_t user)
                                                   // ensure identity is correct
                                                   // for authority check
  if (buRC)
    return sendErrorResponse(pMsg, in, EOWNERDEAD);

  txp::Attribute *oldpathAttribute = pMsg->retrieveAttr(txp::oldname);
  txp::Attribute *newpathAttribute = pMsg->retrieveAttr(txp::newname);
  char *l_oldPath = NULL;
  char *l_newPath = NULL;
  char *oldName = NULL;
  char *newName = NULL;

  if (!oldpathAttribute)
    abort();
  if (!newpathAttribute)
    abort();

  l_oldPath =
      (char *)
          oldpathAttribute->getDataPtr(); // to get addressability to the data

  l_newPath =
      (char *)
          newpathAttribute->getDataPtr(); // to get addressability to the data

  // int renameat(int olddirfd, const char *oldpath, int newdirfd, const char
  // *newpath);
  /* clang-format off */ 
  FL_Write(fl_fshipdposix, POSIX_RENAMEATBEG, "Starting renameat() operation.  OldPathnameLen=%ld  NewPathnameLen=%ld unique=%ld  errno=%d hdr.nodeid=%ld", oldpathAttribute->getDataLength(), newpathAttribute->getDataLength(),in->hdr.unique,in->hdr.nodeid);
  /* clang-format on */
  int l_RCrenameat = renameat(
      -1, l_oldPath, -1, l_newPath); // olddirfd=-1, newdirfd=-1, respectively
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_RENAMEATFIN, "Completed renameat() operation.  rc=%ld  errno=%ld", l_RCrenameat, errno,0,0,0,0);
  /* clang-format on */

  if (l_RCrenameat)
    return sendErrorResponse(pMsg, in, errno);

  struct stat l_stat;
  int statflags =
      AT_SYMLINK_NOFOLLOW; // unlink does not dereference symbolic links
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_STATATRENAMBEG, "Starting rename statat(FH=%ld, flags=0x%lx) operation.", -1, statflags,0,0,0,0);
  /* clang-format on */
  int l_statRC = fstatat(-1, l_newPath, &l_stat, statflags);
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_STATATRENAMFIN, "Completed renam statat() operation.  rc=%ld  errno=%ld st_ino=%ld mode=%o uid=%ld gid=%ld", l_statRC, errno,l_stat.st_ino,l_stat.st_mode, l_stat.st_uid,l_stat.st_gid);
  /* clang-format on */

  if (l_statRC)
    return sendErrorResponse(pMsg, in, errno);

  int l_RC = 0;
  uint64_t offset2newname = 0;

  txp::Attribute *l_offset2newnameAttribute =
      pMsg->retrieveAttr(txp::offset2newname);
  l_RC = l_offset2newnameAttribute->cpyData(&offset2newname,
                                            sizeof(offset2newname));
  if (l_RC)
    abort();
  if (l_newPath)
    newName = l_newPath + offset2newname;

  uint64_t offset2oldname = 0;
  txp::Attribute *l_offset2oldnameAttribute =
      pMsg->retrieveAttr(txp::offset2oldname);
  l_RC = l_offset2oldnameAttribute->cpyData(&offset2oldname,
                                            sizeof(offset2oldname));
  if (l_RC)
    abort();
  if (l_oldPath)
    oldName = l_oldPath + offset2oldname;

  // need to send back newdirInode
  txp::Attribute *l_newdirInode = pMsg->retrieveAttr(txp::newdirInode);

  // Build response Msg and then pick up elements from original message
  txp::Msg *l_ResponseMsg = 0;
  l_RC = pMsg->buildResponseMsg(l_ResponseMsg);
  if (l_RC)
    abort();
  l_RC = l_ResponseMsg->addAttribute(txp::fuse_in_header, (const char *)in,
                                     l_inHdrAttribute->getDataLength());
  if (!l_RC)
    if (l_newdirInode)
      l_RC = l_ResponseMsg->addAttribute(l_newdirInode);
  if (l_RC)
    abort();
  txp::AttrPtr_char_array *l_Attr = NULL;
  l_RC = txp::AttrPtr_char_array::buildAttrPtr_char_array(
      txp::outMsgGeneric, (char *)&outMsg, sizeof(outMsgGeneric), l_Attr);
  if (!l_RC) {
    l_RC = l_ResponseMsg->addAttribute(l_Attr);
  }

  // l_stat.st_ino;
  LOG(fshipd, debug) << "in->hdr.nodeid=" << in->hdr.nodeid
                     << " st_ino=" << l_stat.st_ino << " oldPath=" << l_oldPath
                     << " newPath=" << l_newPath;
  txp::AttrPtr_char_array *l_AttrStat = NULL;
  l_RC = txp::AttrPtr_char_array::buildAttrPtr_char_array(
      txp::stat4Name, (char *)&l_stat, sizeof(l_stat), l_AttrStat);
  if (!l_RC) {
    l_RC = l_ResponseMsg->addAttribute(l_AttrStat);
  }
  if (l_RC)
    abort(); // messed up building attribute
  // send back last known inode number for cleanup

  if (oldName) {
    int l_lookUpNameLength = strlen(oldName) + 1; // get null terminator
    l_RC =
        addlookupname(l_ResponseMsg, oldName, l_lookUpNameLength, txp::oldname);
  };
  if (newName) {
    int l_lookUpNameLength = strlen(newName) + 1; // get null terminator
    l_RC =
        addlookupname(l_ResponseMsg, newName, l_lookUpNameLength, txp::newname);
  };

  if (!l_RC) {
    ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
    if (retSSize == -1) {
      LOGERRNO(fshipd, error, errno);
      abort();
    }
  }

  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }
  return 0;
}

int MessageHandler::readOp(txp::Msg *pMsg) {
  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  if (!l_inHdrAttribute) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -ENOSYS;
  }
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_READ, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */
  // no becomeUser here

  txp::Attribute *l_fuseReadInAttribute = pMsg->retrieveAttr(txp::fuse_read_in);
  struct fuse_read_in *l_fuse_read_in =
      (struct fuse_read_in *)l_fuseReadInAttribute->getDataPtr();
  memItemPtr mip = _connectPtr->getSendChunk(); // get send message chunk
  outMsgGeneric outMsg(in->hdr.unique);

  char *charMemoryPtr = mip->address;
  void *memoryPtr = charMemoryPtr;
  size_t nbyte = l_fuse_read_in->size; // previously  on RHS
  if (nbyte > mip->chunkSize)
    abort(); // should not have gotten here
  uint32_t totalRead = 0;

  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_PREADBEG, "Starting pread() operation.  fh=%ld  Address=%p  Len=%ld  Offset=%ld chunkSize=%ld", l_fuse_read_in->fh, (uint64_t)memoryPtr,nbyte, l_fuse_read_in->offset,mip->chunkSize,0);
  /* clang-format on */
  ssize_t ssizeRetval =
      pread(l_fuse_read_in->fh, memoryPtr, nbyte, l_fuse_read_in->offset);
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_PREADFIN, "Completed pread() operation.  fh=%ld  Address=%p  Len=%ld  Offset=%ld  rc=%ld  errno=%ld", l_fuse_read_in->fh, (uint64_t)memoryPtr,nbyte, l_fuse_read_in->offset,ssizeRetval,errno);
  /* clang-format on */
  if (ssizeRetval == -1) {
    outMsg.hdr.error = -errno;
  } else {
    totalRead += (uint32_t)ssizeRetval;
  }
  mip->length = totalRead;

  // Build response message
  txp::Msg *l_ResponseMsg = 0;
  int l_RC = pMsg->buildResponseMsg(l_ResponseMsg);
  l_RC = l_ResponseMsg->addAttribute(
      txp::fuse_read_in, (const char *)l_fuseReadInAttribute->getDataPtr(),
      l_fuseReadInAttribute->getDataLength());

  txp::AttrPtr_char_array *l_Attr = NULL;
  l_RC = txp::AttrPtr_char_array::buildAttrPtr_char_array(
      txp::outMsgGeneric, (char *)&outMsg, sizeof(outMsgGeneric), l_Attr);
  if (!l_RC) {
    l_RC = l_ResponseMsg->addAttribute(l_Attr);
  }
  txp::Attr_uint32 l_Attr32size(txp::size, totalRead);
  l_RC = l_ResponseMsg->addAttribute(&l_Attr32size);

  outMsg.hdr.len += totalRead;

  ssize_t retSSize = _connectPtr->writeWithBuffer(l_ResponseMsg, mip);

  if (retSSize == -1) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -errno;
  }
  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }
  _connectPtr->sentSendChunk(mip); // \TODO changes later for RW chunk

  return 0;
}

int MessageHandler::readOp(memItemPtr &pMip, txp::Msg *pMsg) {
  memItemPtr l_remoteMip4RDMA = NULL;
  txp::Attribute *l_memItemAttribute = pMsg->retrieveAttr(txp::memItem);
  if (l_memItemAttribute) {
    l_remoteMip4RDMA = (memItemPtr)l_memItemAttribute->getDataPtr();
  }
  if (!l_remoteMip4RDMA)
    return readOp(pMsg);

  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  if (!l_inHdrAttribute) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -ENOSYS;
  }
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_BIGREAD, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */
  // No becomeUser

  outMsgGeneric outMsg(in->hdr.unique);
  txp::Attribute *l_fuseReadInAttribute = pMsg->retrieveAttr(txp::fuse_read_in);
  struct fuse_read_in *l_fuse_read_in =
      (struct fuse_read_in *)l_fuseReadInAttribute->getDataPtr();

  memItemPtr l_localMip4RDMA = NULL;
  if (!pMip->next) { // no attached RDMA buffer yet processed

    // need to get a local RDMA buffer to RDMA read from l_remoteMip to local
    // RDMA buffer
    l_localMip4RDMA = _connectPtr->getRDMAChunk();

    char *charMemoryPtr = l_localMip4RDMA->address;
    void *memoryPtr = charMemoryPtr;
    size_t nbyte = l_fuse_read_in->size;
    if (nbyte > l_localMip4RDMA->chunkSize)
      nbyte = l_localMip4RDMA->chunkSize;
    uint32_t totalRead = 0;

    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_BIGPREADBEG, "Starting pread() operation.  fh=%ld  Address=%p  Len=%ld  Offset=%ld chunkSize=%ld", l_fuse_read_in->fh, (uint64_t)memoryPtr,nbyte, l_fuse_read_in->offset,l_localMip4RDMA->chunkSize,0);
    /* clang-format on */
    ssize_t ssizeRetval =
        pread(l_fuse_read_in->fh, memoryPtr, nbyte, l_fuse_read_in->offset);
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_BIGPREADFIN, "Completed pread() operation.  fh=%ld  Address=%p  Len=%ld  Offset=%ld  rc=%ld  errno=%ld", l_fuse_read_in->fh, (uint64_t)memoryPtr,nbyte, l_fuse_read_in->offset,ssizeRetval,errno);
    /* clang-format on */

    if (ssizeRetval == -1) {
      outMsg.hdr.error = -errno;
    } else {
      totalRead += (uint32_t)ssizeRetval;
    }

    pMip->next = l_localMip4RDMA;
    l_localMip4RDMA->length = totalRead;
    l_localMip4RDMA->next = NULL; // Safety

    if (totalRead) { // do RDMA and set pMip to NULL so that pass in by
                     // reference does not free on return....
      int RC = _connectPtr->rdmaWriteToFrom(l_remoteMip4RDMA, pMip);
      if (!RC) {
        pMip = NULL; // do not free pMip for reuse.
        return 0;    // waiting for RDMA to complete
      }
      // fall through and continue for error or 0 read
    }
  }

  l_localMip4RDMA = pMip->next;
  pMip->next = NULL;

  outMsg.hdr.len += l_localMip4RDMA->length;

  // Build response message
  txp::Msg *l_ResponseMsg = 0;
  int l_RC = pMsg->buildResponseMsg(l_ResponseMsg);
  l_RC = l_ResponseMsg->addAttribute(
      txp::fuse_read_in, (const char *)l_fuseReadInAttribute->getDataPtr(),
      l_fuseReadInAttribute->getDataLength());

  if (!l_RC)
    l_RC = l_ResponseMsg->addAttribute(txp::outMsgGeneric, (char *)&outMsg,
                                       sizeof(outMsgGeneric));
  if (!l_RC)
    l_RC = l_ResponseMsg->addAttribute(txp::memItem, (char *)l_remoteMip4RDMA,
                                       sizeof(memItem));
  txp::Attr_uint32 l_Attr32size(txp::size, l_localMip4RDMA->length);
  l_RC = l_ResponseMsg->addAttribute(&l_Attr32size);

  ssize_t retSSize = _connectPtr->write(l_ResponseMsg);

  if (retSSize == -1) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -errno;
  }
  _connectPtr->freeRDMAChunk(l_localMip4RDMA);
  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }

  return 0;
}

int MessageHandler::writeOp(memItemPtr &pMip, txp::Msg *pMsg) {
  int saved_errno = 0;
  uint32_t l_msgLengthWithData = pMsg->getMsgLengthWithDataValues();
  if (_connectPtr->dataRcvImmediate(pMip, l_msgLengthWithData))
    return writeOp(pMsg); // need to handle immediate data right after -- use
                          // immediate data for predicate
  if (!pMip->next) { // no attached RDMA buffer yet processed

    txp::AttrPtr_char_array *l_inHdrAttribute =
        (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
    if (!l_inHdrAttribute) {
      LOGERRNO(fshipd, error, ENOSYS);
      return -ENOSYS;
    }
    inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
    /* clang-format off */ 
    FL_Write6(fl_fshiptx, TX_BIGWRITE, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
    /* clang-format on */
    // no becomeUser

    txp::Attribute *l_fuseReadInAttribute =
        pMsg->retrieveAttr(txp::fuse_write_in);
    struct fuse_write_in *l_fuse_write_in =
        (struct fuse_write_in *)l_fuseReadInAttribute->getDataPtr();

    txp::Attribute *l_memItemAttribute = pMsg->retrieveAttr(txp::memItem);
    memItemPtr l_remoteMip4RDMA = (memItemPtr)l_memItemAttribute->getDataPtr();

    // need to get a local RDMA buffer to RDMA read from l_remoteMip to local
    // RDMA buffer
    memItemPtr l_localMip4RDMA = _connectPtr->getRDMAChunk();

    pMip->next = l_localMip4RDMA;
    l_localMip4RDMA->length = l_fuse_write_in->size;
    l_localMip4RDMA->next = NULL; // Safety
    // do RDMA and set pMip to NULL so that pass in by reference does not free
    // on return....
    int RC = _connectPtr->rdmaReadFromTo(l_remoteMip4RDMA, pMip);
    if (RC)
      abort(); // \TODO need to handle RC error more gracefully
    if (!RC) {
      pMip = NULL; // do not free pMip for reuse.
      return 0;
    }
    saved_errno = RC;
    // fall through
  }
  if (pMip->next) { // RDMA attached, so do the write
    memItemPtr l_localMip4RDMA = pMip->next;
    char *memoryPtr = l_localMip4RDMA->address;
    pMip->next = NULL;

    txp::AttrPtr_char_array *l_inHdrAttribute =
        (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
    if (!l_inHdrAttribute) {
      LOGERRNO(fshipd, error, ENOSYS);
      return -ENOSYS;
    }
    inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
    /* clang-format off */ 
    FL_Write6(fl_fshiptx, TX_WRITEMEMITEM, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
    /* clang-format on */

    txp::Attribute *l_fuseReadInAttribute =
        pMsg->retrieveAttr(txp::fuse_write_in);
    struct fuse_write_in *l_fuse_write_in =
        (struct fuse_write_in *)l_fuseReadInAttribute->getDataPtr();

    // size_t nbyte =  l_localMip4RDMA->length;
    size_t nbyte = l_fuse_write_in->size;
    uint32_t totalWritten = 0;
    // ssize_t pwrite(int fildes, void *buf, size_t nbyte, off_t offset);
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_BIGPWRITEBEG, "Starting pwrite() operation.  fh=%ld  Address=%p  Len=%ld  Offset=%ld", l_fuse_write_in->fh, (uint64_t)memoryPtr,nbyte, l_fuse_write_in->offset,0,0);
    /* clang-format on */

    ssize_t ssizeRetval =
        pwrite(l_fuse_write_in->fh, memoryPtr, nbyte, l_fuse_write_in->offset);
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_BIGPWRITEFIN, "Completed pwrite() operation.  fh=%ld  Address=%p  Len=%ld  Offset=%ld  rc=%ld  errno=%ld", l_fuse_write_in->fh, (uint64_t)memoryPtr,nbyte, l_fuse_write_in->offset,ssizeRetval, errno);
    /* clang-format on */

    if (ssizeRetval == -1) {
      saved_errno = errno;
    } else {
      totalWritten += (uint32_t)ssizeRetval;
    }

    // need to send back in order to free remote RDMA buffer
    txp::Attribute *l_memItemAttribute = pMsg->retrieveAttr(txp::memItem);
    memItemPtr l_remoteMip4RDMA = (memItemPtr)l_memItemAttribute->getDataPtr();

    // Build response message
    txp::Msg *l_ResponseMsg = 0;
    int l_RC = pMsg->buildResponseMsg(l_ResponseMsg);
    outMsgPwrite outMsg(in->hdr.unique, saved_errno);
    outMsg.write_out.size = totalWritten;
    if (!l_RC)
      l_RC = l_ResponseMsg->addAttribute(txp::outMsgPwrite, (char *)&outMsg,
                                         sizeof(outMsgPwrite));
    if (!l_RC)
      l_RC = l_ResponseMsg->addAttribute(txp::memItem, (char *)l_remoteMip4RDMA,
                                         sizeof(memItem));
    if (l_RC)
      abort();
    if (!l_RC) {
      ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
      if (retSSize == -1) {
        LOGERRNO(fshipd, error, errno);
        abort();
      }
    }

    _connectPtr->freeRDMAChunk(l_localMip4RDMA);

    if (l_ResponseMsg) {
      delete l_ResponseMsg;
      l_ResponseMsg = 0;
    }
  }
  return 0;
}

int MessageHandler::writeOp(txp::Msg *pMsg) {
  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  if (!l_inHdrAttribute) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -ENOSYS;
  }
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_WRITE, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  txp::Attribute *l_fuseReadInAttribute =
      pMsg->retrieveAttr(txp::fuse_write_in);
  struct fuse_write_in *l_fuse_write_in =
      (struct fuse_write_in *)l_fuseReadInAttribute->getDataPtr();

  const __u32 maxMemSize = l_fuse_write_in->size;

  char *charMemoryPtr = _connectPtr->accessBuffer(pMsg, maxMemSize);
  void *memoryPtr = charMemoryPtr;
  size_t nbyte = l_fuse_write_in->size;
  uint32_t totalWritten = 0;

  // no becomeUser

  // ssize_t pwrite(int fildes, void *buf, size_t nbyte, off_t offset);
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_PWRITEBEG, "Starting pwrite() operation.  fh=%ld  Address=%p  Len=%ld  Offset=%ld", l_fuse_write_in->fh, (uint64_t)memoryPtr,nbyte, l_fuse_write_in->offset,0,0);
  /* clang-format on */

  ssize_t ssizeRetval =
      pwrite(l_fuse_write_in->fh, memoryPtr, nbyte, l_fuse_write_in->offset);
  /* clang-format off */
  FL_Write6(fl_fshipdposix, POSIX_PWRITEFIN, "Completed pwrite() operation.  fh=%ld  Address=%p  Len=%ld  Offset=%ld  rc=%ld  errno=%ld", l_fuse_write_in->fh, (uint64_t)memoryPtr,nbyte, l_fuse_write_in->offset,ssizeRetval, errno);
  /* clang-format on */

  if (ssizeRetval == -1) {
    return sendErrorResponse(pMsg, in, errno);
  } else {
    totalWritten += (uint32_t)ssizeRetval;
  }

  // Build response message
  txp::Msg *l_ResponseMsg = 0;
  int l_RC = pMsg->buildResponseMsg(l_ResponseMsg);
  outMsgPwrite outMsg(in->hdr.unique);
  outMsg.write_out.size = totalWritten;
  if (!l_RC)
    l_RC = l_ResponseMsg->addAttribute(txp::outMsgPwrite, (char *)&outMsg,
                                       sizeof(outMsgPwrite));
  if (l_RC)
    abort();
  if (!l_RC) {
    ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
    if (retSSize == -1) {
      LOGERRNO(fshipd, error, errno);
      abort();
    }
  }

  _connectPtr->releaseBuffer(charMemoryPtr);
  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }
  return 0;
}

// defines used from linux/stat.h in kernel
#define S_IRUGO (S_IRUSR | S_IRGRP | S_IROTH)
#define S_IWUGO (S_IWUSR | S_IWGRP | S_IWOTH)
#define S_IXUGO (S_IXUSR | S_IXGRP | S_IXOTH)
int MessageHandler::accessOp(txp::Msg *pMsg) {
  txp::Attribute *pathAttribute = pMsg->retrieveAttr(txp::name);

  char *l_Path =
      (char *)pathAttribute->getDataPtr(); // to get addressability to the data

  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_ACCESS, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  uint64_t mask = 0;

  txp::Attribute *l_Attr64mask = pMsg->retrieveAttr(txp::mask);
  if (l_Attr64mask) {
    int l_RC = l_Attr64mask->cpyData(&mask, sizeof(mask));
    if (l_RC)
      abort();
  }

  int buRC = becomeUser(in->hdr.uid, in->hdr.gid); // becomeUser(uid_t user)
                                                   // ensure identity is correct
                                                   // for authority check
  if (buRC)
    return sendErrorResponse(pMsg, in, EOWNERDEAD);
  // need to emulate access under fshipd which runs as root
  struct stat l_stat;

  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_STATBEG3, "Starting stat() operation.", 0,0,0,0,0,0);
  /* clang-format on */
  int l_statRC = stat(l_Path, &l_stat);
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_STATFIN3, "Completed stat() operation.  rc=%ld  errno=%ld st_ino=%ld mode=%o uid=%ld gid=%ld", l_statRC, errno,l_stat.st_ino,l_stat.st_mode, l_stat.st_uid,l_stat.st_gid);
  /* clang-format on */
  mode_t tempMode = l_stat.st_mode;
  if (l_statRC)
    return sendErrorResponse(pMsg, in, errno);

  if (mask & F_OK) { // need only know stat shows existence and have access
    return sendErrorResponse(pMsg, in, 0);
  }

  bool validGroup = 0;
  bool isPrimary = 0;
  int cugRC = checkUserGroup(in->hdr.uid, l_stat.st_gid, &validGroup,
                             &isPrimary); // is group on node OK for user?
  if (cugRC)
    return sendErrorResponse(pMsg, in,
                             EACCES); // only a problem if the uid is a problem

  if (in->hdr.uid != l_stat.st_uid)
    tempMode &= ~S_IRWXU; // turn off user mode bits
  if (validGroup || isPrimary)
    tempMode &= ~S_IRWXG;       // turn off group mode bits
  mask &= (X_OK | W_OK | R_OK); // ensure only valid checks turned on
  if ((mask & X_OK) && (tempMode & S_IXUGO))
    mask &= ~X_OK;
  if ((mask & W_OK) && (tempMode & S_IWUGO))
    mask &= ~W_OK;
  if ((mask & R_OK) && (tempMode & S_IRUGO))
    mask &= ~R_OK;

  if (!mask) {
    return sendErrorResponse(pMsg, in, 0);
  }

  return sendErrorResponse(pMsg, in, EACCES);
}

int MessageHandler::fallocateOp(txp::Msg *pMsg) {
  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_FALLOC, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  int buRC = becomeUser(in->hdr.uid, in->hdr.gid); // becomeUser(uid_t user)
                                                   // ensure identity is correct
                                                   // for authority check
  if (buRC)
    return sendErrorResponse(pMsg, in, EOWNERDEAD);
  uint64_t fh = 0;
  uint64_t offset;
  uint64_t length;
  uint32_t mode;

  txp::Attribute *l_Attr64fh = pMsg->retrieveAttr(txp::fh);
  l_Attr64fh->cpyData(&fh, sizeof(fh));
  txp::Attribute *l_Attr64offset = pMsg->retrieveAttr(txp::offset);
  l_Attr64offset->cpyData(&offset, sizeof(offset));
  txp::Attribute *l_Attr64length = pMsg->retrieveAttr(txp::length);
  l_Attr64length->cpyData(&length, sizeof(length));
  txp::Attribute *l_Attr32mode = pMsg->retrieveAttr(txp::mode);
  l_Attr32mode->cpyData(&mode, sizeof(mode));
  int saved_errno = 0;
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_FALLOCATEBEG, "Starting fallocate() operation. FH=%ld  mode=o:%lo  offset=%ld  length=%ld",fh, mode, offset, length,0,0);
  /* clang-format on */
  int fallocateRC = fallocate(fh, mode, offset, length);
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_FALLOCATEFIN, "Completed fallocate() operation.  FH=%ld  mode=o:%lo  offset=%ld  length=%ld  rc=%ld  errno=%ld", fh, mode, offset, length, fallocateRC, errno);
  /* clang-format on */

  if (fallocateRC)
    saved_errno = errno;

  // Build response Msg and then pick up elements from original message
  txp::Msg *l_ResponseMsg = 0;
  int l_RC = pMsg->buildResponseMsg(l_ResponseMsg);
  l_RC = l_ResponseMsg->addAttribute(txp::fuse_in_header, (const char *)in,
                                     l_inHdrAttribute->getDataLength());
  outMsgGeneric outMsg(in->hdr.unique, -saved_errno);
  txp::AttrPtr_char_array *l_Attr = NULL;
  if (!l_RC)
    l_RC = txp::AttrPtr_char_array::buildAttrPtr_char_array(
        txp::outMsgGeneric, (char *)&outMsg, sizeof(outMsgGeneric), l_Attr);
  if (!l_RC) {
    l_RC = l_ResponseMsg->addAttribute(l_Attr);
  }
  if (l_RC)
    abort();

  ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
  if (retSSize == -1) {
    LOGERRNO(fshipd, error, ENOSYS);
    if (l_ResponseMsg) {
      delete l_ResponseMsg;
      l_ResponseMsg = 0;
    }
    return -errno;
  }

  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }
  return 0;
}

int MessageHandler::setattrOp(txp::Msg *pMsg) {
  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_SETATTR, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  int buRC = becomeUser(in->hdr.uid, in->hdr.gid); // becomeUser(uid_t user)
                                                   // ensure identity is correct
                                                   // for authority check
  if (buRC)
    return sendErrorResponse(pMsg, in, EOWNERDEAD);

  txp::Attribute *pathAttribute = pMsg->retrieveAttr(txp::name);

  char *l_Path = NULL;
  if (pathAttribute) {
    l_Path =
        (char *)
            pathAttribute->getDataPtr(); // to get addressability to the data
  }
  txp::Attribute *attrInAttribute = pMsg->retrieveAttr(txp::fuse_setattr_in);
  struct fuse_setattr_in *attr_inPtr =
      (struct fuse_setattr_in *)
          attrInAttribute->getDataPtr(); // to get addressability to the data

  uint32_t bitmask = attr_inPtr->valid;

  if (bitmask & FATTR_SIZE) {
    int truncRC = 0;
    /* clang-format off */ 
           FL_Write6(fl_fshipdposix, POSIX_TRUNCATEBEG, "Starting truncate() operation", 0,0,0,0,0,0);
    /* clang-format on */

    if (bitmask & FATTR_FH)
      truncRC = ftruncate(attr_inPtr->fh, attr_inPtr->size);
    else
      truncRC = truncate(l_Path, attr_inPtr->size);
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_TRUNCATEFIN, "Completed truncate() operation.  rc=%ld  errno=%ld", truncRC, errno,0,0,0,0);
    /* clang-format on */

    if (truncRC)
      return sendErrorResponse(pMsg, in, errno);
  }
  if (bitmask & FATTR_MODE) {
    int fchmodRC = 0;
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_CHMODBEG, "Starting chmod() operation.", 0,0,0,0,0,0);
    /* clang-format on */

    if (bitmask & FATTR_FH)
      fchmodRC = fchmod(attr_inPtr->fh, attr_inPtr->mode);
    else
      fchmodRC = chmod(l_Path, attr_inPtr->mode);
    if (fchmodRC)
      return sendErrorResponse(pMsg, in, errno);
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_CHMODFIN, "Completed chmod() operation.  rc=%ld  errno=%ld", fchmodRC, errno,0,0,0,0);
    /* clang-format on */
  }
  if (bitmask & (FATTR_UID | FATTR_GID)) {
    uid_t l_uid = -1; // default to not change
    gid_t l_gid = -1;
    if (bitmask & FATTR_UID)
      l_uid = attr_inPtr->uid;
    if (bitmask & FATTR_GID)
      l_gid = attr_inPtr->gid;
    int chownRC = 0;
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_CHOWNBEG, "Starting chown() operation.", 0,0,0,0,0,0);
    /* clang-format on */

    if (bitmask & FATTR_FH)
      chownRC = fchown(attr_inPtr->fh, l_uid, l_gid);
    else
      chownRC = chown(l_Path, l_uid, l_gid);
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_CHOWNFIN, "Completed chown() operation.  rc=%ld  errno=%ld", chownRC, errno,0,0,0,0);
    /* clang-format on */

    if (chownRC)
      return sendErrorResponse(pMsg, in, errno);
  }

  if (bitmask &
      (FATTR_ATIME | FATTR_ATIME_NOW | FATTR_MTIME | FATTR_MTIME_NOW)) {
    // times[0] specifies the new "last access time" (atime)
    // times[1] specifies the new "last modification time" (mtime).
    struct timespec times[2];
    if (bitmask & FATTR_ATIME_NOW)
      times[0].tv_nsec = UTIME_NOW;
    else if (bitmask & FATTR_ATIME) {
      times[0].tv_sec = attr_inPtr->atime;
      times[0].tv_nsec = attr_inPtr->atimensec;
    } else
      times[0].tv_nsec = UTIME_OMIT;

    if (bitmask & FATTR_MTIME_NOW)
      times[1].tv_nsec = UTIME_NOW;
    else if (bitmask & FATTR_MTIME) {
      times[1].tv_sec = attr_inPtr->mtime;
      times[1].tv_nsec = attr_inPtr->mtimensec;
    } else
      times[1].tv_nsec = UTIME_OMIT;
    int utimeRC = 0;
    if (bitmask & FATTR_FH)
      utimeRC = futimens(attr_inPtr->fh, times);
    else
      utimeRC =
          utimensat(-1, l_Path, times, 0); // flags=0 or AT_SYMLINK_NOFOLLOW
    if (utimeRC)
      return sendErrorResponse(pMsg, in, errno);
  }

  if (bitmask & FATTR_LOCKOWNER) {
    LOG(fshipd, warning) << "settatr LOCKOWNER not processed";
    // int fcntl(int fd, int cmd, ... /* arg */ );
  }

  struct stat l_stat;
  int l_statRC = 0;

  if (bitmask & FATTR_FH) {
    l_statRC = fstat(attr_inPtr->fh, &l_stat);
    if (l_statRC)
      return sendErrorResponse(pMsg, in, errno);
  } else {
    l_statRC = fstatat(-1, l_Path, &l_stat, 0); // 0 or AT_SYMLINK_NOFOLLOW
    if (l_statRC)
      return sendErrorResponse(pMsg, in, errno);
  }
  outMsgSetattr outMsg(in->hdr.unique);
  setAttrOut(l_stat, outMsg.attr_out);
  uint64_t prevRemoteInode = 0;
  txp::Attribute *l_AttrPrevRemoteInode = pMsg->retrieveAttr(txp::nodeid);
  if (l_AttrPrevRemoteInode) {
    int l_RC = l_AttrPrevRemoteInode->cpyData(&prevRemoteInode,
                                              sizeof(prevRemoteInode));
    if (l_RC)
      abort();
  }

  // Build normal ResponseMsg
  txp::Msg *l_ResponseMsg = 0;
  int l_RC = pMsg->buildResponseMsg(l_ResponseMsg);
  l_RC = l_ResponseMsg->addAttribute(txp::fuse_in_header, (const char *)in,
                                     l_inHdrAttribute->getDataLength());
  txp::AttrPtr_char_array *l_Attr = NULL;
  if (!l_RC)
    l_RC = txp::AttrPtr_char_array::buildAttrPtr_char_array(
        txp::outMsgSetattr, (char *)&outMsg, sizeof(outMsgSetattr), l_Attr);
  if (!l_RC) {
    l_RC = l_ResponseMsg->addAttribute(l_Attr);
  }
  if (!l_RC)
    l_RC = l_ResponseMsg->addAttribute(txp::nodeid, prevRemoteInode);
  if (l_RC)
    abort();

  ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
  if (retSSize == -1) {
    LOGERRNO(fshipd, error, ENOSYS);
    if (l_ResponseMsg) {
      delete l_ResponseMsg;
      l_ResponseMsg = 0;
    }
    return -errno;
  }
  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }

  return 0;
}

int MessageHandler::sendErrorResponse(txp::Msg *pMsg, inMsgGeneric *in,
                                      int error) {
  /* clang-format off */ 
  FL_Write6(fl_fshipdmain, FS_SNDERR,"op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld err=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, error);
  /* clang-format on */
  if (error > 0)
    error = -error;
  // Build error response Msg and then pick up elements from original message
  txp::Msg *l_ResponseMsg = 0;
  pMsg->buildResponseMsg(l_ResponseMsg);
  // l_ResponseMsg->addAttribute(l_inHdrAttribute);
  outMsgGeneric outMsg(in->hdr.unique, error);

  int l_RC = l_ResponseMsg->addAttribute(
      txp::outMsgGeneric, (const char *)&outMsg, sizeof(outMsgGeneric));
  // if (!l_RC)l_RC = l_ResponseMsg->addAttribute(txp::fuse_in_header, (const
  // char*)in, sizeof(inMsgGeneric) );
  if (l_RC)
    abort();
  ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
  if (retSSize == -1) {
    LOGERRNO(fshipd, error, ENOSYS);
    if (l_ResponseMsg) {
      delete l_ResponseMsg;
      l_ResponseMsg = 0;
    }
    return -errno;
  }
  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }

  return 0;
}

int MessageHandler::interruptOp(txp::Msg *pMsg) {
  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::fuse_in_header);
  inMsgGeneric *in = (inMsgGeneric *)l_inHdrAttribute->getDataPtr();
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_INTERRUPT, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  txp::Attribute *l_Attr64unique = pMsg->retrieveAttr(txp::unique);
  // \TODO special handling goes here to interrupt operation of request for
  // unique
  // Build normal ResponseMsg
  txp::Msg *l_ResponseMsg = 0;
  pMsg->buildResponseMsg(l_ResponseMsg);
  l_ResponseMsg->addAttribute(txp::fuse_in_header, (const char *)in,
                              l_inHdrAttribute->getDataLength());
  l_ResponseMsg->addAttribute(txp::unique,
                              (const char *)l_Attr64unique->getDataPtr(),
                              l_Attr64unique->getDataLength());

  ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
  if (retSSize == -1) {
    LOGERRNO(fshipd, error, ENOSYS);
    if (l_ResponseMsg) {
      delete l_ResponseMsg;
      l_ResponseMsg = 0;
    }
    return -errno;
  }
  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }

  return 0;
}

int MessageHandler::flushOp(txp::Msg *pMsg) {
  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::inMsgFlush);
  inMsgFlush *inMsg = (inMsgFlush *)l_inHdrAttribute->getDataPtr();
  inMsgGeneric *in = (inMsgGeneric *)inMsg;
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_FLUSH, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_FLUSH2FSYNCBEG, "Starting flush fsync operation.  FH=%ld lockowner=%ld ", inMsg->flush_in.fh, inMsg->flush_in.lock_owner,0,0,0,0);
  /* clang-format on */
  int rc = fsync(inMsg->flush_in.fh);
  /* clang-format off */ 
  FL_Write6(fl_fshipdposix, POSIX_FLUSH2FSYNCFIN, "Completed flush fsynch() operation.  FH=%ld lockowner=%ld   rc=%ld  errno=%ld", inMsg->flush_in.fh, inMsg->flush_in.lock_owner,rc,errno,0,0);
  /* clang-format on */
  if (rc)
    return sendErrorResponse(pMsg, in, -errno);
  sendErrorResponse(pMsg, in, 0);
  return 0;
}

int MessageHandler::fsyncOp(txp::Msg *pMsg) {
  txp::AttrPtr_char_array *l_inHdrAttribute =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::inMsgFsync);
  inMsgFsync *inMsg = (inMsgFsync *)l_inHdrAttribute->getDataPtr();
  inMsgGeneric *in = (inMsgGeneric *)inMsg;
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_FSYNC, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */
  if (inMsg->fsync_in
          .fsync_flags) { // per fuse file.h fsync_flags = datasync ? 1 : 0;
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_FDATASYNCBEG, "Starting fdatasync operation.  FH=%ld fsync_flags=%ld ", inMsg->fsync_in.fh, inMsg->fsync_in.fsync_flags,0,0,0,0);
    /* clang-format on */
    int rc = fdatasync(inMsg->fsync_in.fh);
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_FDATASYNCEND, "Completed fdatasync operation.  FH=%ld fsync_flags=%ld    rc=%ld  errno=%ld", inMsg->fsync_in.fh, inMsg->fsync_in.fsync_flags,rc,errno,0,0);
    /* clang-format on */
    if (rc)
      return sendErrorResponse(pMsg, in, -errno);
  } else { // flush meta data AND data
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_FSYNCBEG, "Starting fdatasync operation.  FH=%ld fsync_flags=%ld ", inMsg->fsync_in.fh, inMsg->fsync_in.fsync_flags,0,0,0,0);
    /* clang-format on */
    int rc = fsync(inMsg->fsync_in.fh);
    /* clang-format off */ 
    FL_Write6(fl_fshipdposix, POSIX_FSYNCEND, "Completed fdatasync operation.  FH=%ld fsync_flags=%ld    rc=%ld  errno=%ld", inMsg->fsync_in.fh, inMsg->fsync_in.fsync_flags,rc,errno,0,0);
    /* clang-format on */
    if (rc)
      return sendErrorResponse(pMsg, in, -errno);
  }
  sendErrorResponse(pMsg, in, 0);

  return 0;
}

/*
struct fuse_poll_in {
        uint64_t	fh;
        uint64_t	kh;
        uint32_t	flags;
        uint32_t	events;
};

struct fuse_poll_out {
        uint32_t	revents;
        uint32_t	padding;
};

struct fuse_notify_poll_wakeup_out {
        uint64_t	kh;
};*/

/**
 * Poll flags
 *
 * FUSE_POLL_SCHEDULE_NOTIFY: request poll notify

#define FUSE_POLL_SCHEDULE_NOTIFY (1 << 0)
FUSE_NOTIFY_POLL   = 1,
*/
// need to setup poll/epoll and send fuse_notify_poll when poll is satisfied
int MessageHandler::pollOp(txp::Msg *pMsg) {
  txp::AttrPtr_char_array *l_inMsg =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::inMsgPoll);
  inMsgPoll *inMsg = (inMsgPoll *)l_inMsg->getDataPtr();
  inMsgGeneric *in = (inMsgGeneric *)inMsg;
  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_POLL, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */
  LOG(fshipd, debug) << "fuse_poll_in.fh=" << inMsg->poll_in.fh << std::hex
                     << " flags=0x" << inMsg->poll_in.flags << " events=0x"
                     << inMsg->poll_in.events << " kh=0x" << inMsg->poll_in.kh
                     << std::dec;

  // \TODO put on poll queue thread
  // test prototype start
  struct pollfd pfd;
  pfd.fd = inMsg->poll_in.fh;
  pfd.events = inMsg->poll_in.events;
  pfd.revents = 0;
  int pollRC = poll(&pfd, 1, 0);

  // test prototype end
  outMsgPoll outMsg(inMsg->hdr.unique);
  outMsg.poll_out.revents = pfd.revents;

  // Build error response Msg --TEMP ACTION until poller thread set up
  txp::Msg *l_ResponseMsg = 0;
  int l_RC = pMsg->buildResponseMsg(l_ResponseMsg);
  struct fuse_notify_poll_wakeup_out wakeup_out;
  wakeup_out.kh = inMsg->poll_in.kh;

  if (!l_RC)
    l_RC = l_ResponseMsg->addAttribute(txp::outMsgPoll, (char *)&outMsg,
                                       sizeof(outMsgPoll));

  if (l_RC)
    abort(); // messed up building attribue
  if (FUSE_POLL_SCHEDULE_NOTIFY & inMsg->poll_in.flags) {
    l_RC = l_ResponseMsg->addAttribute(txp::fuse_notify_poll_wakeup_out,
                                       (char *)&wakeup_out, sizeof(wakeup_out));
  }
  if (pollRC == -1)
    outMsg.hdr.error = -errno;
  ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
  if (retSSize == -1) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -errno;
  }

  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }
  return 0;
}

int MessageHandler::setlkOp(txp::Msg *pMsg) {
  txp::AttrPtr_char_array *l_inMsgLockAttr =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::inMsgLock);
  if (!l_inMsgLockAttr) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -ENOSYS;
  }
  inMsgGeneric *in = (inMsgGeneric *)l_inMsgLockAttr->getDataPtr();

  int buRC = becomeUser(in->hdr.uid, in->hdr.gid); // becomeUser(uid_t user)
                                                   // ensure identity is correct
                                                   // for authority check
  if (buRC)
    return sendErrorResponse(pMsg, in, EOWNERDEAD);

  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_SETLK, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  inMsgLock *inMsg = (inMsgLock *)in;

  struct flock l_flock;
  memset(&l_flock, 0, sizeof(l_flock));
  l_flock.l_type = inMsg->lock_in.lk.type;
  l_flock.l_whence = SEEK_SET; // from beginning of file
  l_flock.l_start = inMsg->lock_in.lk.start;
  l_flock.l_len = (inMsg->lock_in.lk.end - inMsg->lock_in.lk.start) + 1;

#ifdef F_OFD_SETLK
  int rc_fcntl = fcntl(inMsg->lock_in.fh, F_OFD_SETLK, &l_flock);
#else
  int rc_fcntl = fcntl(inMsg->lock_in.fh, F_SETLK, &l_flock);
#endif
  if (rc_fcntl == -1)
    sendErrorResponse(pMsg, in, -errno);

  return sendErrorResponse(pMsg, in, 0);
}
int MessageHandler::getlkOp(txp::Msg *pMsg) {
  txp::AttrPtr_char_array *l_inMsgLockAttr =
      (txp::AttrPtr_char_array *)pMsg->retrieveAttr(txp::inMsgLock);
  if (!l_inMsgLockAttr) {
    LOGERRNO(fshipd, error, ENOSYS);
    return -ENOSYS;
  }
  inMsgGeneric *in = (inMsgGeneric *)l_inMsgLockAttr->getDataPtr();

  int buRC = becomeUser(in->hdr.uid, in->hdr.gid); // becomeUser(uid_t user)
                                                   // ensure identity is correct
                                                   // for authority check
  if (buRC)
    return sendErrorResponse(pMsg, in, EOWNERDEAD);

  /* clang-format off */ 
  FL_Write6(fl_fshiptx, TX_GETLK, "op=%-3ld uniq=%-24ld nID=%-24ld uID=%-9ld gID=%-9ld pID=%ld", in->hdr.opcode,in->hdr.unique,in->hdr.nodeid,in->hdr.uid,in->hdr.gid, in->hdr.pid);
  /* clang-format on */

  inMsgLock *inMsg = (inMsgLock *)in;

  struct flock l_flock;
  memset(&l_flock, 0, sizeof(l_flock));
  l_flock.l_type = inMsg->lock_in.lk.type;
  l_flock.l_whence = SEEK_SET; // from beginning of file
  l_flock.l_start = inMsg->lock_in.lk.start;
  l_flock.l_len = (inMsg->lock_in.lk.end - inMsg->lock_in.lk.start) + 1;

#ifdef F_OFD_GETLK
  int rc_fcntl = fcntl(inMsg->lock_in.fh, F_OFD_GETLK, &l_flock);
#else
  int rc_fcntl = fcntl(inMsg->lock_in.fh, F_GETLK, &l_flock);
#endif
  if (rc_fcntl == -1)
    sendErrorResponse(pMsg, in, -errno);

  // Build error response Msg --TEMP ACTION until poller thread set up
  outMsgGetLock outMsgLock(in->hdr.unique);
  outMsgLock.lock_out.lk.type = l_flock.l_type;
  outMsgLock.lock_out.lk.pid = l_flock.l_pid;
  outMsgLock.lock_out.lk.start = l_flock.l_start;
  outMsgLock.lock_out.lk.end = outMsgLock.lock_out.lk.start + l_flock.l_len - 1;

  txp::Msg *l_ResponseMsg = 0;
  int l_RC = pMsg->buildResponseMsg(l_ResponseMsg);

  if (!l_RC)
    l_RC = l_ResponseMsg->addAttribute(txp::outMsgGetLock, (char *)&outMsgLock,
                                       sizeof(outMsgGetLock));

  if (l_RC)
    abort(); // messed up building attribue
  ssize_t retSSize = _connectPtr->write(l_ResponseMsg);
  if (retSSize == -1) {
    LOGERRNO(fshipd, error, ENOSYS);
    if (l_ResponseMsg) {
      delete l_ResponseMsg;
      l_ResponseMsg = 0;
    }
    return -errno;
  }
  if (l_ResponseMsg) {
    delete l_ResponseMsg;
    l_ResponseMsg = 0;
  }

  return 0;
}
