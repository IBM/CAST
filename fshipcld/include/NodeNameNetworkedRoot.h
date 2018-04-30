/*******************************************************************************
 |    NodeNameNetworkedRooth
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


#ifndef NODNAME_NETWORKED_H
#define NODNAME_NETWORKED_H
#include "NodeNameRoot.h"

/*
typedef NodeName * NodeNamePTR;
typedef std::pair<__u64,NodeNamePTR> inodePair;
typedef std::map<__u64,NodeNamePTR> NodeNameMap;
typedef NodeNameMap::iterator NodeNameIterator;
typedef NodeNameMap::const_iterator ConstNodeNameIterator;
typedef std::pair<NodeNameIterator,bool> NodeIteratorBool;
*/
class NodeNameNetworkedRoot : public NodeNameRoot {
public:
  NodeNameNetworkedRoot(char *mountDir, char *pNetworkPath)
      : NodeNameRoot(pNetworkPath) {
    _mountDirSize = strlen(mountDir);
    _mountDir = new char[strlen(mountDir) + 1];
    memcpy(_mountDir, mountDir, _mountDirSize + 1);
    _stat.st_ino = FUSE_ROOT_ID;
    _dirMap.insert(inodePair(this->getInode(), this));
  }

  virtual ~NodeNameNetworkedRoot() { delete[] _mountDir; }

  const char *getNetworkPath() { return accessNamePTR(); }
  const char *getMountDir() const { return _mountDir; }

private:
  char *_mountDir;
  int _mountDirSize;

  struct stat _stat;
}; // end class NodeNameNetworkedRoot

#endif // NODNAME_NETWORKED_H
