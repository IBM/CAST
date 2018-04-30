/*******************************************************************************
 |    NodeNameRoot.cc
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

#include "../include/NodeNameRoot.h"
#include "fshipcld_flightlog.h"
#include "logging.h"

// globally initialize in gcc .init
uint64_t NodeNameRoot::entryValidsec = 0;
uint64_t NodeNameRoot::entryValidnsec = 0;
uint64_t NodeNameRoot::attrValidsec = 0;
uint64_t NodeNameRoot::attrValidnsec = 0;

int NodeNameRoot::forgetNode(const __ino64_t pInode, const uint64_t pNlookup) {
  if (pInode == FUSE_ROOT_ID)
    return 0;
  int l_RC = pthread_mutex_lock(&_mutex);
  _LOGLOCK;
  assert_perror(l_RC);
  int count = 0;
  NodeNameIterator it = _dirMap.find(pInode);
  if (__glibc_unlikely(it != _dirMap.end())) {
    NodeNamePTR nnp = it->second;
    /* clang-format off */ 
    FL_Write6(FL_NodeName, FORGET_DIR_, "pInode=%lld, pNlookup=%lld nnp=%llx _inode=%lld _parentInode=%lld nlookup=%lld", pInode, pNlookup,(uint64_t)nnp, nnp->getInode(), nnp->getParentInode(), nnp->getLookupCount() );
    /* clang-format on */
    _AliasMap.erase(nnp->getRemoteInode());
    _dirMap.erase(it); // removed from map
    delete nnp;
    count++;
  } else {
    it = _FileMap.find(pInode);
    if (__glibc_likely(it != _FileMap.end())) {
      NodeNamePTR nnp = it->second;
      /* clang-format off */ 
      FL_Write6(FL_NodeName, FORGET_FILE, "pInode=%lld, pNlookup=%lld nnp=%llx _inode=%lld _parentInode=%lld nlookup=%lld", pInode, pNlookup,(uint64_t)nnp, nnp->getInode(), nnp->getParentInode(), nnp->getLookupCount() );
      /* clang-format on */
      _AliasMap.erase(nnp->getRemoteInode());
      _FileMap.erase(it); // removed from map
      delete nnp;
      count++;
    } else {
      LOG(fshipcld, info) << "FORGET UNFOUND FuseInode=" << pInode;
      /* clang-format off */ 
      FL_Write(FL_NodeName, FORGET_UNFOUND, "pInode=%lld line=%ld", pInode, __LINE__,0,0);
      /* clang-format on */
    }
  }
  l_RC = pthread_mutex_unlock(&_mutex);
  _LOGUNLOCK;
  assert_perror(l_RC);
  return count;
}

NodeNameRoot::NodeNameRoot(char *dir) : NodeName(FUSE_ROOT_ID, dir) {
  _d_type = DT_DIR; // note--may need to check stat struct
  _parentInode = 0;
  if (_inodeForFuse != FUSE_ROOT_ID)
    abort();
  _readdir_d_type =
      0; // will do discovery on whether readdir type is UNKNOWN by file system
  _memChunkPtr = new txp::MemChunk(64, MAXCHUNKSIZE);
  LOG(fshipcld, debug) << "created FUSE_ROOT_ID=" << FUSE_ROOT_ID
                       << " dir=" << dir;
  int mutexRC = pthread_mutex_init(&_mutex, NULL);
  if (mutexRC)
    abort();
}

NodeNameRoot::~NodeNameRoot() {
  pthread_mutex_destroy(&_mutex);
  if (_memChunkPtr)
    delete _memChunkPtr;
  LOG(fshipcld, debug) << "Destroyed FUSE_ROOT_ID";
}

// assumes this is building a path of preceding directories
// presumes caller does locking
void NodeNameRoot::buildRemotePath(NodeNamePTR nnp, memItemPtr &miPtr) {

  if (!nnp) {
    miPtr->length = 0;
    return;
  }
  if (nnp->isRoot()) {
    int rc =
        add2PathPostSlash(miPtr->address, miPtr->length, nnp->accessNamePTR(),
                          nnp->getNameLength(), MAXCHUNKSIZE);
    if (__glibc_unlikely(rc <= 0))
      abort();
  } else {
    if (nnp == nnp->getParentInodePTR()) {
      LOG(fshipcld, always) << "node and parent node are=" << nnp
                            << " ABORT!!!!!!";
      abort();
    }
    buildRemotePath(nnp->getParentInodePTR(), miPtr);
    int rc =
        add2PathPostSlash(miPtr->address, miPtr->length, nnp->accessNamePTR(),
                          nnp->getNameLength(), MAXCHUNKSIZE);
    if (__glibc_unlikely(rc <= 0))
      abort();
  }
}

// locks object
void NodeNameRoot::buildpathb4lastname(const __ino64_t pInode,
                                       memItemPtr &miPtr) {
  // assume directory....
  int l_RC = pthread_mutex_lock(&_mutex);
  _LOGLOCK;
  assert_perror(l_RC);
  NodeNamePTR nnp = getFuseNodeDirNoLock(pInode);
  miPtr->length = 0;
  if (!nnp)
    return;

  if (nnp->isRoot()) {
    int rc =
        add2PathPostSlash(miPtr->address, miPtr->length, nnp->accessNamePTR(),
                          nnp->getNameLength(), MAXCHUNKSIZE);
    if (__glibc_unlikely(rc <= 0))
      abort();
  } else {
    buildRemotePath(nnp->getParentInodePTR(), miPtr);
    int rc =
        add2PathPostSlash(miPtr->address, miPtr->length, nnp->accessNamePTR(),
                          nnp->getNameLength(), MAXCHUNKSIZE);
    if (__glibc_unlikely(rc <= 0))
      abort();
  }
  l_RC = pthread_mutex_unlock(&_mutex);
  _LOGUNLOCK;
  assert_perror(l_RC);
  return;
}

// locks object
// return remote Node#
uint64_t NodeNameRoot::buildpathbWithLastname(const __ino64_t pInode,
                                              memItemPtr &miPtr) {
  int l_RC = pthread_mutex_lock(&_mutex);
  _LOGLOCK;
  assert_perror(l_RC);

  // dir or file.....
  NodeNamePTR nnp = getNodeFuse(pInode);
  miPtr->length = 0;
  if (!nnp) {
    l_RC = pthread_mutex_unlock(&_mutex);
    _LOGUNLOCK;
    assert_perror(l_RC);
    return 0;
  }

  if (nnp->isRoot()) {
    int rc =
        add2PathNoSlash(miPtr->address, miPtr->length, nnp->accessNamePTR(),
                        nnp->getNameLength(), MAXCHUNKSIZE);
    if (__glibc_unlikely(rc <= 0))
      abort();
  } else {
    buildRemotePath(nnp->getParentInodePTR(), miPtr);
    int rc =
        add2PathNoSlash(miPtr->address, miPtr->length, nnp->accessNamePTR(),
                        nnp->getNameLength(), MAXCHUNKSIZE);
    if (__glibc_unlikely(rc <= 0))
      abort();
  }
  uint64_t remoteInode = nnp->getRemoteInode();
  l_RC = pthread_mutex_unlock(&_mutex);
  _LOGUNLOCK;
  assert_perror(l_RC);
  // printf("pInode=%ld remoteInode=%ld\n",pInode,remoteInode);
  return remoteInode;
}

// locks object
NodeNamePTR NodeNameRoot::getNodeDir(const __ino64_t pInode) {
  if (pInode == _inodeForFuse)
    return this; // inode is root
  int l_RC = pthread_mutex_lock(&_mutex);
  _LOGLOCK;
  assert_perror(l_RC);
  NodeNamePTR nnp = NULL;
  ConstNodeNameIterator it = _dirMap.find(pInode);
  if (it != _dirMap.end())
    nnp = it->second;
  l_RC = pthread_mutex_unlock(&_mutex);
  _LOGUNLOCK;
  assert_perror(l_RC);
  return nnp;
}

// lock by caller
NodeNamePTR NodeNameRoot::getNodeDirNoLock(const __ino64_t pInode) {
  if (pInode == _inodeForFuse)
    return this; // inode is root
  NodeNamePTR nnp = NULL;
  ConstNodeNameIterator it = _dirMap.find(pInode);
  if (it != _dirMap.end())
    nnp = it->second;
  return nnp;
}

// lock presumed
void NodeNameRoot::insertIntoMap(NodeNamePTR nnp) {
  if (nnp->isDir()) {
    NodeIteratorBool ret = _dirMap.insert(inodePair(nnp->getInode(), nnp));
    if (ret.second) { // insert was successful
      /* clang-format off */ 
      FL_Write6(FL_NodeName, INSERT_DIR, " nnp=%llx _inode=%lld _parentInode=%lld parentNodePtr=%llx octal mode=%lo nlookup=%lld", (uint64_t)nnp, nnp->getInode(), nnp->getParentInode(),(uint64_t)nnp->getParentInodePTR(), (uint64_t)nnp->getMode(), nnp->getLookupCount() );
      /* clang-format on */
    } else {

      /* clang-format off */ 
      FL_Write6(FL_NodeName, INSERT_DFAIL, " nnp=%llx _inode=%lld _parentInode=%lld parentNodePtr=%llx octal mode=%lo nlookup=%lld", (uint64_t)nnp, nnp->getInode(), nnp->getParentInode(),(uint64_t)nnp->getParentInodePTR(), (uint64_t)nnp->getMode(), nnp->getLookupCount() );
      /* clang-format on */
      NodeNamePTR nnp2 = ret.first->second;
      /* clang-format off */ 
      FL_Write6(FL_NodeName, INSERT_DEXIST, " nnp2=%llx _inode=%lld _parentInode=%lld parentNodePtr=%llx octal mode=%lo nlookup=%lld", (uint64_t)nnp2, nnp2->getInode(), nnp2->getParentInode(),(uint64_t)nnp2->getParentInodePTR(), (uint64_t)nnp2->getMode(), nnp2->getLookupCount() );
      /* clang-format on */
      abort();
    }
  } else {
    _FileMap.insert(inodePair(nnp->getInode(), nnp));
    /* clang-format off */ 
    FL_Write6(FL_NodeName, INSERT_FILE, " nnp=%llx _inode=%lld _parentInode=%lld parentNodePtr=%llx octal mode=%lo nlookup=%lld", (uint64_t)nnp, nnp->getInode(), nnp->getParentInode(), (uint64_t)nnp->getParentInodePTR(), (uint64_t)nnp->getMode(), nnp->getLookupCount() );
    /* clang-format on */
  }
}
uint64_t
NodeNameRoot::updateChildNodeGetattrResponse(struct fuse_attr_out &pAttrOut,
                                             const uint64_t pInHdrnodeid) {

  return updateChildNodeAttrResponse(pAttrOut.attr, pInHdrnodeid);
}

// locked
uint64_t
NodeNameRoot::updateChildNodeAttrResponse(struct fuse_attr &pAttr,
                                          const uint64_t pInHdrnodeid) {
  int l_RC = pthread_mutex_lock(&_mutex);
  _LOGLOCK;
  assert_perror(l_RC);
  uint64_t getattrRC = 0;
  // Someone replaced the original entry name with a same name (create or move)
  // and need to deal with the inode change and possible change from file to dir
  // or dir to file
  NodeNamePTR nnp = getNodeFuse(pInHdrnodeid);
  NodeNamePTR nnpRemote = NULL;
  ConstNodeNameIterator it = _AliasMap.find(pAttr.ino);
  if (it != _AliasMap.end())
    nnpRemote = it->second;
  LOG(fshipcld, debug) << "updateChildNodeAttrResponsefuse nnp=" << nnp
                       << " fuse inode=" << pInHdrnodeid
                       << " nnpRemote=" << nnpRemote
                       << "remote inode=" << pAttr.ino;

  if (!nnp) {
    LOG(fshipcld, always)
        << "updateChildNodeAttrResponse has unexpected fuse inode not found";
    // Why would fuse send a FORGET in the middle of a getattr(pInHdrnodeid)?
    // Or did the code screwup
    getattrRC = ENOENT;
    assert(nnp != NULL); // need to investigate how this happened
  } else {
    LOG(fshipcld, debug)
        << "updateChildNodeGetattrResponse FuseNode Remote inode="
        << nnp->getRemoteInode();
    if (nnpRemote) { // this is non null if same name directory element was
                     // known but moved to the expected name target
      // remove old alias found
      _AliasMap.erase(nnpRemote->getRemoteInode());
      nnpRemote->setRemoteInode(0);
      nnpRemote = NULL; // lose addressing to the NodeName;
    }
    // check for same name but flipped from file to dir or dir to file (ouch)
    if ((nnp->getMode() ^ pAttr.mode) &
        S_IFMT) { // did the mode dir/file change?  need to drop the old and go
                  // with the new? how did this happen?
      LOG(fshipcld, always) << "file mode conflict nnp->getMode()=" << std::oct
                            << nnp->getMode() << " pAttr.mode=" << pAttr.mode
                            << std::dec;
      if (nnp != this) { // would be weird if the remote local directory became
                         // a file.  Other weirdness would ensue
        // create a new fuse node nodename entry and put on dir/file list and on
        // alias list
        nnp = (nnp->getParentInodePTR())
                  ->createChildNode(&pAttr, nnp->accessNamePTR());
        insertIntoMap(nnp);
      } else {
        LOG(fshipcld, always) << "mountpoint update";
      }
    } else { // need to remove previous alias entry and create a new one
      _AliasMap.erase(nnp->getRemoteInode()); // remove old alias
      nnp->setRemoteInode(pAttr.ino);
      _AliasMap.insert(inodePair(nnp->getRemoteInode(), nnp));
    }
    nnp->setRemoteInode(pAttr.ino);
    _AliasMap.insert(inodePair(nnp->getRemoteInode(), nnp));
    nnp->updateNode(&pAttr);
    pAttr.ino = nnp->getInode(); // need to use the local fuse inode value
  }

  l_RC = pthread_mutex_unlock(&_mutex);
  _LOGUNLOCK;
  assert_perror(l_RC);
  return getattrRC;
}

// locks object
void NodeNameRoot::renameNodeResponse(struct stat *s, char *name,
                                      const __ino64_t newParentInode) {
  int l_RC = pthread_mutex_lock(&_mutex);
  _LOGLOCK;
  assert_perror(l_RC);

  NodeNamePTR nnp = NULL;
  NodeNameIterator it = _AliasMap.find(s->st_ino);
  if (it != _AliasMap.end())
    nnp = it->second;
  if (!nnp) {   // \TODO Need to force an update of the dentry via fuse_notify??
                // Or should have had the old fuse nodeid?
    int it = 0; // trick to avoid iterator error
    if (it) {
    }
    // the reason we get here is because the existing name on remote was deleted
    // and remade

    // so some fuse operations will fail which try to use the old fuse inode ??
    // \TODO  need to pass the alias INODE values on the shipped operations as
    // well????
    // \TODO dentry to NodeNamePtr lookup??
    ConstNodeNameIterator it2newDir = _dirMap.find(newParentInode);
    NodeNamePTR parent = NULL;
    if (it2newDir != _dirMap.end())
      parent = it2newDir->second;
    nnp = parent->createChildNode(s, name);
    _AliasMap.insert(inodePair(nnp->getRemoteInode(), nnp));
    if (nnp->isDir()) {
      NodeIteratorBool aliasItBoolDirMap =
          _dirMap.insert(inodePair(nnp->getInode(), nnp));
      if (aliasItBoolDirMap.second == false)
        abort(); // MUST work!

    } else {
      NodeIteratorBool aliasItBoolFileMap =
          _FileMap.insert(inodePair(nnp->getInode(), nnp));
      if (aliasItBoolFileMap.second == false)
        abort(); // MUST work!
    }
  } // end if !nnp
  else {
    int it = 0; // trick to avoid iterator error
    if (it) {
    }
    if ((nnp->getMode() ^ s->st_mode) &
        S_IFMT) { // did the mode change?  need to drop the old and go with the
                  // new? how did this happen?
      NodeNamePTR parent = nnp->getParentInodePTR();
      if (newParentInode != nnp->getParentInode()) {
        ConstNodeNameIterator it2newDir = _dirMap.find(newParentInode);
        if (it2newDir != _dirMap.end())
          parent = it2newDir->second;
        else
          abort();
      }
      if (S_ISDIR(s->st_mode) && !(nnp->isDir())) { // was file, now a dir
        // call fuse notify on existing nnp?? Alias map will no longer touch
        // stale entry
        _AliasMap.erase(s->st_ino);
        nnp = parent->createChildNode(s, name);
        _AliasMap.insert(inodePair(nnp->getRemoteInode(), nnp));
        _dirMap.insert(inodePair(nnp->getInode(), nnp));
      }
      if (!S_ISDIR(s->st_mode) && nnp->isDir()) { // was a dir, now a file
        // call fuse notify on existing nnp?? Alias map will no longer touch
        // stale entry
        _AliasMap.erase(s->st_ino);
        nnp = parent->createChildNode(s, name);
        _AliasMap.insert(inodePair(nnp->getRemoteInode(), nnp));
        _FileMap.insert(inodePair(nnp->getInode(), nnp));
      }
    } else {
      if (newParentInode != nnp->getParentInode()) {
        ConstNodeNameIterator it2newDir = _dirMap.find(newParentInode);
        if (it2newDir != _dirMap.end())
          nnp->updateNode(s, name, it2newDir->second);
        else
          abort();
      } else {
        nnp->updateNode(s, name);
      }
    }
  }
  LOG(fshipcld, debug) << "s->st_ino=" << s->st_ino
                       << " nnp->getInode()=" << nnp->getInode()
                       << " nnp->getRemoteInode()=" << nnp->getRemoteInode();
  l_RC = pthread_mutex_unlock(&_mutex);
  _LOGUNLOCK;
  assert_perror(l_RC);
  return;
}

// locked
void NodeNameRoot::updateChildNodeForLookup(struct fuse_entry_out *pEntryOut,
                                            char *const name,
                                            const NodeNamePTR parent) {
  int l_RC = pthread_mutex_lock(&_mutex);
  _LOGLOCK;
  assert_perror(l_RC);

  NodeNamePTR nnp = NULL;
  ConstNodeNameIterator it = _AliasMap.find(pEntryOut->nodeid);
  if (it != _AliasMap.end())
    nnp = it->second;
  if (!nnp) {
    nnp = parent->createChildNode(pEntryOut, name);
    _AliasMap.insert(inodePair(nnp->getRemoteInode(), nnp));
    if (nnp->isDir()) {
      NodeIteratorBool aliasItBoolDirMap =
          _dirMap.insert(inodePair(nnp->getInode(), nnp));
      if (aliasItBoolDirMap.second == false)
        abort(); // MUST work!

    } else {
      NodeIteratorBool aliasItBoolFileMap =
          _FileMap.insert(inodePair(nnp->getInode(), nnp));
      if (aliasItBoolFileMap.second == false)
        abort(); // MUST work!
    }
  } // end if !nnp
  else {

    if ((nnp->getMode() ^ pEntryOut->attr.mode) &
        S_IFMT) { // did the mode change?  need to drop the old and go with the
                  // new? how did this happen?
      if (S_ISDIR(pEntryOut->attr.mode) &&
          !(nnp->isDir())) { // was file, now a dir
        // call fuse notify on existing nnp?? Alias map will no longer touch
        // stale entry
        _AliasMap.erase(pEntryOut->nodeid);
        nnp = parent->createChildNode(pEntryOut, name);
        _AliasMap.insert(inodePair(nnp->getRemoteInode(), nnp));
        _dirMap.insert(inodePair(nnp->getInode(), nnp));
      }
      if (!S_ISDIR(pEntryOut->attr.mode) &&
          nnp->isDir()) { // was a dir, now a file
        // call fuse notify on existing nnp?? Alias map will no longer touch
        // stale entry
        _AliasMap.erase(pEntryOut->nodeid);
        nnp = parent->createChildNode(pEntryOut, name);
        _AliasMap.insert(inodePair(nnp->getRemoteInode(), nnp));
        _FileMap.insert(inodePair(nnp->getInode(), nnp));
      }
    } else {
      nnp->updateNode(&pEntryOut->attr, name, parent);
    }
  }
  pEntryOut->nodeid = nnp->getInode(); // need to use the local fuse inode value
  pEntryOut->attr.ino =
      nnp->getInode(); // need to use the local fuse inode value

  l_RC = pthread_mutex_unlock(&_mutex);
  _LOGUNLOCK;
  assert_perror(l_RC);
  return;
}

// locked
void NodeNameRoot::updateChildNodeForResponse(
    struct fuse_direntplus *pFusedentPlus,
    const NodeNamePTR parent) { // readdirplus processing
  int l_RC = pthread_mutex_lock(&_mutex);
  _LOGLOCK;
  assert_perror(l_RC);

  NodeNamePTR nnp = NULL;
  LOG(fshipcld, debug) << "pFusedentPlus->entry_out.nodeid="
                       << pFusedentPlus->entry_out.nodeid;
  ConstNodeNameIterator it = _AliasMap.find(pFusedentPlus->entry_out.nodeid);
  if (it != _AliasMap.end())
    nnp = it->second;
  if (!nnp) {
    nnp = parent->createChildNode(pFusedentPlus);
    _AliasMap.insert(inodePair(nnp->getRemoteInode(), nnp));
    if (nnp->isDir()) {
      NodeIteratorBool aliasItBoolDirMap =
          _dirMap.insert(inodePair(nnp->getInode(), nnp));
      if (aliasItBoolDirMap.second == false)
        abort(); // MUST work!

    } else {
      NodeIteratorBool aliasItBoolFileMap =
          _FileMap.insert(inodePair(nnp->getInode(), nnp));
      if (aliasItBoolFileMap.second == false)
        abort(); // MUST work!
    }
  } else {

    if ((nnp->getMode() ^ pFusedentPlus->entry_out.attr.mode) &
        S_IFMT) { // did the mode change?  need to drop the old and go with the
                  // new? how did this happen?
      if (S_ISDIR(pFusedentPlus->entry_out.attr.mode) &&
          !(nnp->isDir())) { // was file, now a dir
        // call fuse notify on existing nnp?? Alias map will no longer touch
        // stale entry
        _AliasMap.erase(pFusedentPlus->entry_out.nodeid);
        nnp = parent->createChildNode(pFusedentPlus);
        _AliasMap.insert(inodePair(nnp->getRemoteInode(), nnp));
        _dirMap.insert(inodePair(nnp->getInode(), nnp));
      }
      if (!S_ISDIR(pFusedentPlus->entry_out.attr.mode) &&
          nnp->isDir()) { // was a dir, now a file
        // call fuse notify on existing nnp?? Alias map will no longer touch
        // stale entry
        _AliasMap.erase(pFusedentPlus->entry_out.nodeid);
        nnp = parent->createChildNode(pFusedentPlus);
        _AliasMap.insert(inodePair(nnp->getRemoteInode(), nnp));
        _FileMap.insert(inodePair(nnp->getInode(), nnp));
      }
    } else {
      nnp->updateNode(pFusedentPlus, parent);
    }
  }
  pFusedentPlus->dirent.ino = nnp->getInode();
  pFusedentPlus->entry_out.nodeid =
      nnp->getInode(); // need to use the local fuse inode value
  pFusedentPlus->entry_out.attr.ino =
      nnp->getInode(); // need to use the local fuse inode value
  pFusedentPlus->dirent.ino =
      nnp->getInode(); // need to use the local fuse inode value

  l_RC = pthread_mutex_unlock(&_mutex);
  _LOGUNLOCK;
  assert_perror(l_RC);
  return;
}

// locked
void NodeNameRoot::createChildNodeForResponse(struct fuse_entry_out *pEntryOut,
                                              char *const name,
                                              const NodeNamePTR parent) {

  int l_RC = pthread_mutex_lock(&_mutex);
  _LOGLOCK;
  assert_perror(l_RC);
  const NodeNamePTR nnp = parent->createChildNode(pEntryOut, name);
  NodeIteratorBool aliasItBool =
      _AliasMap.insert(inodePair(nnp->getRemoteInode(), nnp));

  if (aliasItBool.second == false) { // insert conflict, remote inode was reused
    // \TODOg old fuse inode candidate for fuse notify
    const NodeNamePTR nnpOld =
        aliasItBool.first->second;             // find the old NameNode
    _AliasMap.erase(nnpOld->getRemoteInode()); // removed from Alias map
    // For now, nnpOld is an Orphan until Forget or fuse_notify is sent
    aliasItBool = _AliasMap.insert(inodePair(nnp->getRemoteInode(), nnp));
    if (aliasItBool.second == false)
      abort(); // MUST work!
  }
  pEntryOut->nodeid = nnp->getInode(); // need to use the local fuse inode value
  pEntryOut->attr.ino =
      nnp->getInode(); // need to use the local fuse inode value
  if (nnp->isDir()) {
    NodeIteratorBool aliasItBoolDirMap =
        _dirMap.insert(inodePair(nnp->getInode(), nnp));
    if (aliasItBoolDirMap.second == false)
      abort(); // MUST work!
  } else {
    NodeIteratorBool aliasItBoolFileMap =
        _FileMap.insert(inodePair(nnp->getInode(), nnp));
    if (aliasItBoolFileMap.second == false)
      abort(); // MUST work!
  }

  pEntryOut->nodeid = nnp->getInode(); // need to use the local fuse inode value
  pEntryOut->attr.ino =
      nnp->getInode(); // need to use the local fuse inode value

  l_RC = pthread_mutex_unlock(&_mutex);
  _LOGUNLOCK;
  assert_perror(l_RC);
  return;
}
