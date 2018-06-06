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

//! \file  NodeName.h
//! \brief class NodeName

#ifndef NODENAME_H
#define NODENAME_H
#include "logging.h"
#include <dirent.h>
#include <linux/fuse.h>
#include <map>
#include <stddef.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
//#include "LockCoral.h"

typedef struct dirent dirent;

#define SHORTNAMESIZE 32
class NodeName;
typedef NodeName *NodeNamePTR;
typedef std::pair<uint64_t, NodeNamePTR> inodePair;
typedef std::map<uint64_t, NodeNamePTR> NodeNameMap;
typedef NodeNameMap::iterator NodeNameIterator;
typedef NodeNameMap::const_iterator ConstNodeNameIterator;
typedef std::pair<NodeNameIterator, bool> NodeIteratorBool;

static volatile uint64_t staticValuesForFuseInode = 0;
class NodeName {

public:
  ~NodeName();
  /*
  struct fuse_dirent {
          __u64	ino;
          __u64	off;
          __u32	namelen;
          __u32	type;
          char name[0];
  */
  NodeNamePTR createChildNode(struct fuse_dirent *pFusedent) {
    LOG(fshipcld, debug) << "pFusedent->ino" << pFusedent->ino
                         << " name=" << pFusedent->name;
    NodeNamePTR child =
        new NodeName(pFusedent->ino, pFusedent->name, pFusedent->namelen);
    child->_d_type = pFusedent->type;
    child->attachToParentInode(this);
    return child;
  }

  NodeNamePTR createChildNode(struct fuse_direntplus *pFusedentPlus) {
    struct fuse_dirent *l_Fusedent = &pFusedentPlus->dirent;
    LOG(fshipcld, debug) << "l_Fusedent->ino" << l_Fusedent->ino
                         << " name=" << l_Fusedent->name;
    NodeNamePTR child =
        new NodeName(l_Fusedent->ino, l_Fusedent->name, l_Fusedent->namelen);
    child->_d_type = l_Fusedent->type;
    child->_mode = pFusedentPlus->entry_out.attr.mode;
    child->attachToParentInode(this);
    return child;
  }

  NodeNamePTR createChildNode(struct fuse_entry_out *pEntryOut, char *name) {
    struct fuse_attr *l_FuseAttr = &pEntryOut->attr;
    LOG(fshipcld, debug) << "l_FuseAttr->ino" << l_FuseAttr->ino
                         << " name=" << name;
    NodeNamePTR child = new NodeName(l_FuseAttr->ino, name);
    child->setType(l_FuseAttr->mode);
    child->attachToParentInode(this);
    return child;
  }

  NodeNamePTR createChildNode(struct stat *s, const char *name) {
    LOG(fshipcld, debug) << "s->st_ino" << s->st_ino << " name=" << name;
    NodeNamePTR child = new NodeName(s->st_ino, name);
    child->setType(s->st_mode);
    child->attachToParentInode(this);
    return child;
  }

  NodeNamePTR createChildNode(struct fuse_attr *pFuseAttr, const char *name) {
    LOG(fshipcld, debug) << "pFuseAttr->ino" << pFuseAttr->ino
                         << " name=" << name;
    NodeNamePTR child = new NodeName(pFuseAttr->ino, name);
    child->setType(pFuseAttr->mode);
    child->attachToParentInode(this);
    return child;
  }

  void updateNode(struct fuse_dirent *pFusedent, const NodeNamePTR parent);
  void updateNode(struct fuse_direntplus *pFusedentPlus,
                  const NodeNamePTR parent);
  void updateNode(struct stat *s, const char *name, const NodeNamePTR parent);
  void updateNode(struct stat *s, const char *name);
  void updateNode(struct fuse_attr *pFuseAttr, char *name,
                  const NodeNamePTR parent);
  void updateNode(struct fuse_attr *pFuseAttr);

  void setType(mode_t pMode) {
    // stat: S_IFMT     0170000   bit mask for the file type bit field
    _mode = pMode;
    if (S_ISDIR(_mode))
      _d_type = DT_DIR;
    else if (S_ISREG(_mode))
      _d_type = DT_REG;
    else if (S_ISLNK(_mode))
      _d_type = DT_LNK;
    else
      _d_type = DT_UNKNOWN; // Dont know
  }

  // return 1 if _dtype and mode disagree on whether belongs in dir map
  int setMode(mode_t pMode) {
    // stat: S_IFMT     0170000   bit mask for the file type bit field
    _mode = pMode;
    return ((_d_type == DT_DIR) ^ (S_ISDIR(_mode)));
  }

  uint64_t incrementLookupCount(const uint64_t pValue) {
    _nlookupCount += pValue;
    return _nlookupCount;
  }
  uint64_t decrementLookupCount(const uint64_t pValue) {
    _nlookupCount -= pValue;
    return _nlookupCount;
  }
  uint64_t addOneLookupCount() { return ++_nlookupCount; }
  uint64_t getLookupCount() { return _nlookupCount; }
  mode_t getMode() { return _mode; }

  NodeName(ino_t pRemoteInode, const char *pIname, int pLength = 0);

  void removeFromParentInode() {
    // if (_parentInodePTR) _parentInodePTR-> _numChildReferences--;
    _parentInodePTR = NULL;
  }

  int attachToParentInode(NodeNamePTR parent) {
    this->_parentInodePTR = parent;
    _parentInode = _parentInodePTR->_inodeForFuse;
    //_parentInodePTR-> _numChildReferences++;
    // return _parentInodePTR-> _numChildReferences;
    return 0;
  }

  int switchTo(NodeNamePTR nowParent) {
    if (this->_parentInodePTR == nowParent) {
      // return  _parentInodePTR -> _numChildReferences;
      return 0;
    }
    //_parentInodePTR->_numChildReferences--;
    return attachToParentInode(nowParent);
  }

  int setName(char *pIname) {
    _length = strlen(pIname);
    if (_name_ptr)
      delete[] _name_ptr;
    _name_ptr = NULL;
    if (_length < SHORTNAMESIZE) {
      _name_ptr = NULL;
      memcpy(_shortName, pIname, _length);
      _shortName[_length] = '\0';
    } else {
      _shortName[0] = 0;
      _name_ptr = new char[_length + 1];
      memcpy(_name_ptr, pIname, _length);
      _name_ptr[_length] = '\0';
    }
    return _length;
  }

  char isDir() { return (_d_type == DT_DIR); }
  char isLink() { return (_d_type == DT_LNK); }
  char isRegFile() { return (_d_type == DT_REG); }
  char isUnkFile() { return (_d_type == DT_UNKNOWN); }
  int isRoot() { return (_inodeForFuse == FUSE_ROOT_ID); }

  // XXX Not sure why it was complaining about const char* const
  const char * accessNamePTR() {
    if (_name_ptr)
      return _name_ptr;
    return _shortName;
  }
  uint64_t getInode() { return _inodeForFuse; }
  uint64_t getRemoteInode() { return _inoRemote; }
  void setRemoteInode(uint64_t pRemoteInode) { _inoRemote = pRemoteInode; }
  uint64_t getParentInode() { return _parentInode; }
  NodeNamePTR getParentInodePTR() { return _parentInodePTR; }
  int getNameLength() { return _length; }
  // \return negative, 0, or postive if pName is less, equal, or greater than
  // stored name
  // int   compareName(const char * pName){ return strcmp(pName, accessNamePTR()
  // );}

  int updateName(struct fuse_direntplus *pFusedentPlus) {
    return updateName(pFusedentPlus->dirent.name,
                      pFusedentPlus->dirent.namelen);
  }
  int updateName(struct fuse_dirent *pFusedent) {
    return updateName(pFusedent->name, pFusedent->namelen);
  }

  int updateName(const char *pIname, int pLength = 0) {
    if (!pLength)
      pLength = strlen(pIname);
    if (pLength == _length) {
      if (!strncmp(pIname, accessNamePTR(), _length))
        return 0;
    }
    if (_name_ptr)
      delete[] _name_ptr;
    _length = pLength;
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
    LOG(fshipcld, debug) << "now name=" << accessNamePTR();
    return _length;
  }
  void updateRemoteInodeVal(ino_t pIno);

protected:
  __ino64_t _inodeForFuse; // The value given to fuse
  __ino64_t _parentInode;
  __ino64_t _inoRemote; // remote inode number
  mode_t _mode;
  uint64_t _generation;
  unsigned char _d_type;       // like dirent d_type
  uint64_t _nlookupCount;      // number of lookups done
  NodeNamePTR _parentInodePTR; // use direct pointer instead of find of
                               // _parentInode (speedup)
  char _shortName[SHORTNAMESIZE];
  char *_name_ptr;
  int _length;

  // uint64_t  _numChildReferences;
  uint64_t _numHardlinks;
};

#endif // NODENAME_H
