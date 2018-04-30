/*================================================================================

    csmd/src/daemon/include/thread_manager.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_THREAD_MANAGER_H_
#define CSMD_SRC_DAEMON_INCLUDE_THREAD_MANAGER_H_

#include <vector>

#include <logging.h>

namespace csm {
namespace daemon {
typedef std::vector<csm::daemon::EventManager*> EventManagerPool;

class ThreadManager
{
  csm::daemon::ThreadPool *_ThreadPool;
  csm::daemon::EventManagerPool *_EventMgrPool;

public:
  ThreadManager( const csm::daemon::ThreadPool *i_ThreadPool = nullptr,
                 const csm::daemon::EventManagerPool *i_EventMgrPool = nullptr )
  : _ThreadPool( const_cast<csm::daemon::ThreadPool*>( i_ThreadPool ) ),
    _EventMgrPool( const_cast<csm::daemon::EventManagerPool*>( i_EventMgrPool ) )
  {}
  ThreadManager( const csm::daemon::ThreadManager &i_TM )
  : _ThreadPool( i_TM._ThreadPool ),
    _EventMgrPool( i_TM._EventMgrPool )
  {}
  ~ThreadManager()
  {
    // make sure everything is running before we exit
    StartThreads();
    _EventMgrPool->clear();
  }

  int StopThreads()
  {
    if( _ThreadPool )
      _ThreadPool->InterruptHandlers();

    if( _EventMgrPool )
      for( auto & it: *_EventMgrPool )
        it->Freeze();

    return 0;
  }
  int StartThreads()
  {
    if( _EventMgrPool )
      for( auto & it: *_EventMgrPool )
        it->Unfreeze();
    return 0;
  }



};

}   // namespace daemon
}  // namespace csm

#endif /* CSMD_SRC_DAEMON_INCLUDE_THREAD_MANAGER_H_ */
