/*******************************************************************************
 |    TrackPath.h
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

//! \file  TrackPath.h
//! \brief class TrackPath
#ifndef __TRACKPATH_H__
#define __TRACKPATH_H__

#include "CnxSock.h"
#include "Msg.h"
#include "fshipcld.h"
#include "NodeNameRoot.h"

class TrackPath {
public:
  TrackPath() {
    _memItemPtr = NULL;
    _RootNodePtr = NULL;
    _Msg = NULL;
    _length4when2useMemBufferForPathName = (unsigned int)(-1);
    _doPathWrite = 0;
    _index2LastName = 0;
    _lastName = NULL;
    _lookupName = NULL;
    _parentNodeId = 0;
    _nodeID = 0;
    _offsetAttr2lastName = txp::offset; // default
  }

  void useMemStruct() { _length4when2useMemBufferForPathName = 0; }

  txp::Msg *getMsgPtr() { return _Msg; }
  const char *getLookupName() { return _lookupName; }

  void addPath(txp::AttributeName attrPathName) {
    _Msg->addAttribute(attrPathName, _memItemPtr->address, _memItemPtr->length);
  }

  void addString(txp::AttributeName attrPathName, char *pString) {
    _Msg->addAttribute(attrPathName, pString, strlen(pString) + 1);
  }

  uint64_t addPath2Msg4lookup(NodeNameRootPTR pRootNodePtr,
                              txp::AttributeName attrPathName);

  uint64_t addFQPN(NodeNameRootPTR pRootNodePtr,
                   txp::AttributeName attrPathName);

  ssize_t sendMsg(txp::ConnexPtr _pConnectPtr) {
    ssize_t l_RC = 0;
    if (!_Msg) {
      errno = EIO;
      return -1;
    }

    if (_doPathWrite) {
      l_RC = _pConnectPtr->writeWithBuffer(_Msg, _memItemPtr->address,
                                           _memItemPtr->length, 0);
    } else {
      l_RC = _pConnectPtr->write(_Msg);
    }
    if (_Msg)
      delete _Msg;
    _Msg = NULL;
    return l_RC;
  }

  void addFuseHeaderItems(inMsgGeneric *in) {
    _Msg->addAttribute(txp::fuse_in_header, (char *)in, sizeof(inMsgGeneric));
    if (_lookupName) {
      _parentNodeId = in->hdr.nodeid;
    } else {
      _nodeID = in->hdr.nodeid;
    }

    return;
  }
  void setNodeID(uint64_t pNodeID) { _nodeID = pNodeID; }

  void setParentInode(uint64_t pParentNodeId, txp::AttributeName pAttrName) {
    _parentNodeId = pParentNodeId;
    addUint64(pAttrName, _parentNodeId);
  }

  void addFD2msg(txp::AttributeName pHandleAttr, uint64_t pHandle) {
    _Msg->addAttribute(pHandleAttr, pHandle);

    return;
  }

  void addUint32(txp::AttributeName pAttr, uint32_t pValue) {
    _Msg->addAttribute(pAttr, pValue);

    return;
  }

  void addUint64(txp::AttributeName pAttr, uint64_t pValue) {
    _Msg->addAttribute(pAttr, pValue);

    return;
  }

  void addFuseReadIn(struct fuse_read_in *read_in) {
    _Msg->addAttribute(txp::fuse_read_in, (char *)read_in,
                       sizeof(struct fuse_read_in));

    return;
  }

  void addStruct(txp::AttributeName pAttr, void *struct2Add, int structSizeof) {
    _Msg->addAttribute(pAttr, (char *)struct2Add, structSizeof);

    return;
  }

  void addFuseAttrIn(struct fuse_setattr_in *setattr_in) {
    txp::AttrPtr_char_array *l_Attr = NULL;
    txp::AttrPtr_char_array::buildAttrPtr_char_array(
        txp::fuse_setattr_in, (char *)setattr_in,
        sizeof(struct fuse_setattr_in), l_Attr);
    _Msg->addAttribute(l_Attr);

    return;
  }

  void setOffsetAttr2lastName(txp::AttributeName pAttrName) {
    _offsetAttr2lastName = pAttrName;

    return;
  }

protected:
  txp::Msg *_Msg;

  int _index2LastName;     // index into pathname referencing _lastName
  char *_lastName;         // just points at the last name in the Path
  const char *_lookupName; // references lookupName
  uint64_t _parentNodeId;  // know parent nodeID if nonzero
  uint64_t _nodeID;        // actual FUSE nodeid, zero if unknown
  uint64_t _remoteNodeID;  // remote node ID, zero if unknown
  txp::AttributeName _offsetAttr2lastName;
  memItemPtr _memItemPtr;
  NodeNameRootPTR _RootNodePtr;
  unsigned int _length4when2useMemBufferForPathName;
  signed int _doPathWrite;
  bool _removeMsg;

  virtual ~TrackPath() {
    if (_memItemPtr) {
      _RootNodePtr->putChunk(_memItemPtr);
    }
    // if (_Msg) {delete _Msg; _Msg=NULL;} //message is deleted in sendMsg
    // method
  }
};

//  Allow reuse of a Msg in a different TrackPath instance
class TrackPathNoFreeMsg : public TrackPath {
public:
  TrackPathNoFreeMsg(char *pLookupName, txp::Msg *pMsg) : TrackPath() {
    _Msg = pMsg;
    _lookupName = pLookupName;
  }

  TrackPathNoFreeMsg(txp::Msg *pMsg) : TrackPath() { _Msg = pMsg; }

  virtual ~TrackPathNoFreeMsg() {}
};

//  Allow reuse of a Msg in a different TrackPath instance
class TrackPathFreeMsg : public TrackPath {
public:
  TrackPathFreeMsg(txp::Id pAttrID) : TrackPath() {
    _offsetAttr2lastName = txp::offset; // default
    txp::Msg::buildMsg(pAttrID, _Msg);
  }

  TrackPathFreeMsg(txp::Id pAttrID, char *pLookupName) : TrackPath() {
    _offsetAttr2lastName = txp::offset; // default
    txp::Msg::buildMsg(pAttrID, _Msg);
    _lookupName = pLookupName;
  }

  virtual ~TrackPathFreeMsg() {}
};

#endif
