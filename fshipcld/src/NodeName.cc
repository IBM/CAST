/*******************************************************************************
 |    NodeName.h
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

#include "../include/NodeName.h"
#include "fshipcld_flightlog.h"
#include "logging.h"
//! \file  NodeName.cc
//! \brief class NodeName

NodeName::NodeName(ino_t pRemoteInode, const char *pIname, int pLength)
    : _inoRemote(pRemoteInode), _length(pLength) {
  _inodeForFuse =
      ++staticValuesForFuseInode; // \TODO up to a 2**64 (~18EE18) values,
  if (!staticValuesForFuseInode)
    abort(); // die on wrap to zero
  _parentInode = 0;
  _parentInodePTR = NULL;
  _nlookupCount = 0;
  //_numChildReferences=0;
  _numHardlinks = 0;
  _d_type = DT_UNKNOWN;
  _mode = 0;
  _generation = 1;
  if (!_length)
    _length = strlen(pIname);
  if (_length < SHORTNAMESIZE) {
    _name_ptr = NULL;
    memcpy(_shortName, pIname, _length);
    _shortName[_length] = '\0';
    // printf("_shortName=%s ",_shortName);
  } else {
    _shortName[0] = 0;
    _name_ptr = new char[_length + 1];
    memcpy(_name_ptr, pIname, _length);
    _name_ptr[_length] = '\0';
    // printf("_name_ptr=%s ",_name_ptr);
  }
  LOG(fshipcld, debug) << "create inodeForFuse=" << _inodeForFuse
                       << " remoteInode=" << _inoRemote
                       << " name=" << (_name_ptr ? _name_ptr : _shortName);
}

NodeName::~NodeName() {
  LOG(fshipcld, debug) << "delete inodeForFuse=" << _inodeForFuse
                       << " remoteInode=" << _inoRemote
                       << " name=" << (_name_ptr ? _name_ptr : _shortName);
  if (_name_ptr)
    delete[] _name_ptr;
  _name_ptr = NULL;
  _shortName[0] = 0;
  // if (_parentInodePTR) _parentInodePTR-> _numChildReferences--;
  _length = 0;
  _parentInodePTR = NULL;
}

void NodeName::updateRemoteInodeVal(ino_t pInoRemote) {
  if (_inoRemote != pInoRemote)
    printf("inode value changed from _inoRemote=%ld to pInoRemote=%ld\n",
           _inoRemote, pInoRemote);
  _inoRemote = pInoRemote;
}

void NodeName::updateNode(struct fuse_attr *pFuseAttr, char *name,
                          const NodeNamePTR parent) {
  setMode(pFuseAttr->mode);
  updateName(name);

  updateRemoteInodeVal(pFuseAttr->ino);

  _parentInode = parent->_inodeForFuse;
  _parentInodePTR = parent;
}

void NodeName::updateNode(struct fuse_attr *pFuseAttr) {
  setMode(pFuseAttr->mode);
  updateRemoteInodeVal(pFuseAttr->ino);
}

void NodeName::updateNode(struct stat *s, const char *name,
                          const NodeNamePTR parent) {

  setMode(s->st_mode);
  updateName(name);

  updateRemoteInodeVal(s->st_ino);

  _parentInode = parent->_inodeForFuse;
  _parentInodePTR = parent;
  if (parent == this)
    abort();
}

void NodeName::updateNode(struct stat *s, const char *name) {

  setMode(s->st_mode);
  updateName(name);

  updateRemoteInodeVal(s->st_ino);
}

void NodeName::updateNode(struct fuse_direntplus *pFusedentPlus,
                          const NodeNamePTR parent) {
  setMode(pFusedentPlus->entry_out.attr.mode);
  updateName(pFusedentPlus);
  updateRemoteInodeVal(pFusedentPlus->dirent.ino);
  _parentInode = parent->_inodeForFuse;
  _parentInodePTR = parent;
}

void NodeName::updateNode(struct fuse_dirent *pFusedent,
                          const NodeNamePTR parent) {
  _d_type = pFusedent->type;
  updateName(pFusedent);
  updateRemoteInodeVal(pFusedent->ino);
  _parentInode = parent->_inodeForFuse;
  _parentInodePTR = parent;
}
