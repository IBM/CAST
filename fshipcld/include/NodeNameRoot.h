/*******************************************************************************
 |    NodeNameRoot.h
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


#ifndef NODENAMEROOT_H
#define NODENAMEROOT_H
#include "MemChunk.h"
#include "NodeName.h"

#if 0
#define _LOGLOCK                                                               \
  LOG(fshipcld, debug) << "lock line=" << __LINE__ << "@" << __PRETTY_FUNCTION__
#define _LOGUNLOCK                                                             \
  LOG(fshipcld, debug) << "UNLOCK line=" << __LINE__ << "@"                    \
                       << __PRETTY_FUNCTION__
#else
#define _LOGLOCK
#define _LOGUNLOCK
#endif

class NodeNameRoot : public NodeName {
public:
  static uint64_t entryValidsec;
  static uint64_t entryValidnsec;
  static uint64_t attrValidsec;
  static uint64_t attrValidnsec;
  static uint64_t rootEntryValidsec; // for root node ID=1
  static uint64_t rootEntryValidnsec;
  static uint64_t rootAttrValidsec;
  static uint64_t rootAttrValidnsec;

  const int MAXCHUNKSIZE = 64 * 1024;
  NodeNameRoot(char *dir);
  ~NodeNameRoot();

public:
  NodeNamePTR getNodeDir(const __ino64_t pInode);
  NodeNamePTR getNodeDirNoLock(const __ino64_t pInode);

private:
  NodeNamePTR getFuseNodeDirNoLock(const __ino64_t pInode) {
    if (pInode == _inodeForFuse)
      return this; // inode is root
    ConstNodeNameIterator it = _dirMap.find(pInode);
    if (it != _dirMap.end())
      return it->second;
    return NULL;
  }

private:
  // assumes caller does locking
  NodeNamePTR getNodeAlias(const __ino64_t pInode) {
    ConstNodeNameIterator it = _AliasMap.find(pInode);
    if (it != _AliasMap.end())
      return it->second;
    return NULL;
  }

  // assumes caller does locking
  NodeNamePTR getNodeFuse(const __ino64_t pInode) {
    if (pInode == _inodeForFuse)
      return this; // inode is root

    ConstNodeNameIterator it = _dirMap.find(pInode);
    if (it != _dirMap.end())
      return it->second;

    it = _FileMap.find(pInode);
    if (it != _FileMap.end())
      return it->second;

    return NULL;
  }

public:
  void updateChildNode(struct fuse_dirent *pFusedent,
                       const NodeNamePTR parent) {
    int l_RC = pthread_mutex_lock(&_mutex);
    _LOGLOCK;
    assert_perror(l_RC);
    NodeNamePTR nnp = getNodeAlias(pFusedent->ino);
    if (!nnp) {
      nnp = parent->createChildNode(pFusedent);
      if (nnp->isUnkFile())
        _readdir_d_type = 1; // need lookup or getattr to update type as readdir
                             // type is unknown for file system
      insertIntoMap(nnp);
    } else {
      nnp->updateNode(pFusedent, parent);
    }
    l_RC = pthread_mutex_unlock(&_mutex);
    _LOGUNLOCK;
    assert_perror(l_RC);
    return;
  }

  uint64_t updateChildNodeGetattrResponse(struct fuse_attr_out &pAttrOut,
                                          const uint64_t pInHdrnodeid);

  uint64_t updateChildNodeAttrResponse(struct fuse_attr &pAttr,
                                       const uint64_t pInHdrnodeid);
  void updateChildNodeForLookup(struct fuse_entry_out *pEntryOut,
                                char *const name, const NodeNamePTR parent);
  void createChildNodeForResponse(struct fuse_entry_out *pEntryOut,
                                  char *const name, const NodeNamePTR parent);
  void updateChildNodeForResponse(struct fuse_direntplus *pFusedentPlus,
                                  const NodeNamePTR parent);

  void updateChildNode(struct fuse_direntplus *pFusedentPlus,
                       const NodeNamePTR parent) {
    int l_RC = pthread_mutex_lock(&_mutex);
    _LOGLOCK;
    assert_perror(l_RC);
    NodeNamePTR nnp = getNodeAlias(pFusedentPlus->dirent.ino);
    if (!nnp) {
      nnp = parent->createChildNode(pFusedentPlus);
      insertIntoMap(nnp);
    } else {
      nnp->updateNode(pFusedentPlus, parent);
    }

    l_RC = pthread_mutex_unlock(&_mutex);
    _LOGUNLOCK;
    assert_perror(l_RC);
    return;
  }

  void updateChildNode(struct stat *statPtr, char *const name,
                       const NodeNamePTR parent) {
    int l_RC = pthread_mutex_lock(&_mutex);
    _LOGLOCK;
    assert_perror(l_RC);
    NodeNamePTR nnp = getNodeAlias(statPtr->st_ino);
    if (nnp) {
      if (nnp->isUnkFile()) {
        nnp->setType(statPtr->st_mode);
        if (nnp->isDir())
          MovetoDirMap(nnp);
      }
      nnp->updateNode(statPtr, name, parent);
    } else {
      nnp = parent->createChildNode(statPtr, name);
      insertIntoMap(nnp);
    }
    l_RC = pthread_mutex_unlock(&_mutex);
    _LOGUNLOCK;
    assert_perror(l_RC);
    return;
  }

  void updateChildNode(struct fuse_attr *pFuseAttr, char *const name,
                       const NodeNamePTR parent) {
    int l_RC = pthread_mutex_lock(&_mutex);
    _LOGLOCK;
    assert_perror(l_RC);
    NodeNamePTR nnp = getNodeAlias(pFuseAttr->ino);
    if (nnp) {
      if (nnp->isUnkFile()) {
        nnp->setType(pFuseAttr->mode);
        if (nnp->isDir())
          MovetoDirMap(nnp);
      }
      nnp->updateNode(pFuseAttr, name, parent);
    } else {
      nnp = parent->createChildNode(pFuseAttr, name);
      insertIntoMap(nnp);
    }
    l_RC = pthread_mutex_unlock(&_mutex);
    _LOGUNLOCK;
    assert_perror(l_RC);
    return;
  }

  int forgetNode(const __ino64_t pInode, const uint64_t pNlookup);
  void rmdirNode(const __ino64_t pInode);

  void renameNodeResponse(struct stat *s, char *name,
                          const __ino64_t newParentInode);

private:
  inline int add2PathPostSlash(char *const pBase, uint64_t &pStorageLength,
                               const char *const pSource, int pSourceLength,
                               const unsigned int pMaxStoragelength) const {
    if (__glibc_unlikely((pStorageLength + pSourceLength + 1) >
                         pMaxStoragelength))
      return -ENOMEM;
    if (__glibc_likely(pStorageLength > 0))
      pStorageLength--; // back up to Null char location, else start at
    memcpy(pBase + pStorageLength, pSource, pSourceLength);
    pStorageLength += pSourceLength;
    pBase[pStorageLength] = '/';
    pStorageLength++;
    pBase[pStorageLength] = 0;
    pStorageLength++;
    // printf(" base=%s addpath=%s \n",pBase,pSource);
    return pStorageLength;
  }

private:
  inline int add2PathNoSlash(char *const pBase, uint64_t &pStorageLength,
                             const char *const pSource, int pSourceLength,
                             const unsigned int pMaxStoragelength) const {
    if (__glibc_unlikely((pStorageLength + pSourceLength) > pMaxStoragelength))
      return -ENOMEM;
    if (__glibc_likely(pStorageLength > 0))
      pStorageLength--; // back up to Null char location, else start at
    memcpy(pBase + pStorageLength, pSource, pSourceLength);
    pStorageLength += pSourceLength;
    pBase[pStorageLength] = 0;
    pStorageLength++;
    return pStorageLength;
  }

public:
  void buildpathb4lastname(const __ino64_t pInode, memItemPtr &miPtr);
  // return nonzero remote inode value
  uint64_t buildpathbWithLastname(const __ino64_t pInode, memItemPtr &miPtr);
  unsigned char getAttrUpdateRequired() { return _readdir_d_type; }
  memItemPtr getChunk() { return _memChunkPtr->getChunk(); }
  void putChunk(memItemPtr pMi) { _memChunkPtr->putChunk(pMi); }

  // assumes this is building a path of preceding directories
private:
  void buildRemotePath(NodeNamePTR nnp, memItemPtr &miPtr);

private:
  // lock presumed
  void MovetoDirMap(NodeNamePTR nnp) {

    if (nnp->isDir()) {
      _dirMap.insert(inodePair(nnp->getInode(), nnp));
      _FileMap.erase(nnp->getInode()); // removed from map
    }
  }

private:
  // lock presumed
  NodeNamePTR findNodeName(const __ino64_t pInode) {

    ConstNodeNameIterator it = _FileMap.find(pInode);
    if (it == _FileMap.end()) {
      it = _dirMap.find(pInode);
      if (it == _dirMap.end()) {
        return NULL;
      }
    }
    return it->second;
  };

private:
  // lock presumed
  NodeNamePTR findDirNodeName(const __ino64_t pInode) {

    ConstNodeNameIterator it = _dirMap.find(pInode);
    if (it == _dirMap.end()) {
      return NULL;
    }
    return it->second;
  }

private:
  // lock presumed
  void insertIntoMap(NodeNamePTR nnp);

protected:
  pthread_mutex_t _mutex; // control access to avoid race conditions
  txp::MemChunkPtr _memChunkPtr;
  NodeNameMap _dirMap;
  NodeNameMap _FileMap;  // not a directory but a regular file or sym link
  NodeNameMap _AliasMap; // alias remote node ID to fshipcld nodeid
  unsigned char _readdir_d_type; // file system supports type field?
};

typedef NodeNameRoot *NodeNameRootPTR;

#endif // NODENAMEROOT_H
