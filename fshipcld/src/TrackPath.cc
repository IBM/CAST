/*******************************************************************************
 |    TrackPath.cc
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

#include "TrackPath.h"

uint64_t TrackPath::addPath2Msg4lookup(NodeNameRootPTR pRootNodePtr,
                                       txp::AttributeName attrPathName) {
  if (!_memItemPtr) {
    _memItemPtr = pRootNodePtr->getChunk();
    _RootNodePtr = pRootNodePtr;
  }
  pRootNodePtr->buildpathb4lastname(_parentNodeId, _memItemPtr);
  if (!_memItemPtr->length)
    return 0;

  _memItemPtr->length--; // get the string length
  _index2LastName = _memItemPtr->length;
  int rc = snprintf(_memItemPtr->address + _memItemPtr->length,
                    pRootNodePtr->MAXCHUNKSIZE - _memItemPtr->length, "%s",
                    _lookupName);
  if (rc > 0)
    _memItemPtr->length += rc + 1;
  else
    return 0;
  txp::Attr_uint64 l_Attr64offset(_offsetAttr2lastName, _index2LastName);
  _Msg->addAttribute(&l_Attr64offset);
  if (_memItemPtr->length > _length4when2useMemBufferForPathName) {
    addStruct(txp::memItem, _memItemPtr, sizeof(*_memItemPtr));
    _doPathWrite = _length4when2useMemBufferForPathName;
  } else {
    addPath(attrPathName);
    _doPathWrite = 0;
  }

  // printf("addPath2Msg4lookup _pathName=%s length=%ld \noffset=%d
  // lookupName=%s
  // \n",_memItemPtr->address,_memItemPtr->length,_index2LastName,_memItemPtr->address+_index2LastName);
  return _memItemPtr->length;
}

uint64_t TrackPath::addFQPN(NodeNameRootPTR pRootNodePtr,
                            txp::AttributeName attrPathName) {
  if (!_memItemPtr) {
    _memItemPtr = pRootNodePtr->getChunk();
    _RootNodePtr = pRootNodePtr;
  }
  uint64_t l_remoteNodeID =
      pRootNodePtr->buildpathbWithLastname(_nodeID, _memItemPtr);
  assert(l_remoteNodeID != 0);
  // printf("_remoteNodeID=%ld\n",_remoteNodeID);
  if (!_memItemPtr->length) {
    return 0;
  }

  if (_memItemPtr->length > _length4when2useMemBufferForPathName) {
    addStruct(txp::memItem, _memItemPtr, sizeof(*_memItemPtr));
    _doPathWrite = _length4when2useMemBufferForPathName;
  } else {
    addPath(attrPathName);
    _doPathWrite = 0;
  }

  return l_remoteNodeID;
}
