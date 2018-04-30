/*================================================================================

    csmd/src/daemon/include/thread_pool.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef CSMD_SRC_DAEMON_INCLUDE_THREAD_POOL_H_
#define CSMD_SRC_DAEMON_INCLUDE_THREAD_POOL_H_
#include "logging.h"

#include <atomic>
#include <mutex>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

class CSMI_BASE;

namespace csm {
namespace daemon {

struct ThreadPool {
  typedef std::unique_ptr<boost::asio::io_service::work> asio_worker;
  typedef std::shared_ptr<boost::asio::io_service::strand> strand_sptr;
  
private:
  boost::asio::io_service _service;
  asio_worker _service_worker;
  boost::thread_group _thread_grp;
  int _thread_grp_size;
  
  static std::vector<boost::thread::id> CrashedThreadList;
  static std::map<boost::thread::id, boost::thread* > TidToThreadMap;
  
  static std::map<boost::thread::id, std::string> TidToHandlerMap;
  
  static std::map<CSMI_BASE *, strand_sptr> HandlerToStrand;

  static std::mutex LockForCrashedThreadList;
  static std::mutex LockForTidToHandlerMap;
    
  volatile static int CrashedThreadCount;
  
  static boost::thread::id _main_loop_thread_id;

public:
  ThreadPool(int threads) :_service(), _service_worker(new asio_worker::element_type(_service)) {
    for (int i = 0; i < threads; ++i) {
      addThread();
    }
    _thread_grp_size = threads;
    CrashedThreadCount = 0;
  }
    
  ~ThreadPool() {
    _service_worker.reset();
    _thread_grp.join_all();
    LOG(csmd, debug) << "~ThreadPool(): done with join_all...";
    _service.stop();
  }
  
  /* 
   * we should pass the non-nullptr handler if the handler is not multi-threaded safe
   * e.g. for one request to multiple replies scenarios, it leads to the same shared EventContext
   * in the handlers. Proper locking is needed if each reply may update the content in the context
   * in order to be multi-threaded safe
   */
  template<typename F>
  void enqueue(F f, CSMI_BASE *handler = nullptr)
  {
    if (handler == nullptr)
    {
      _service.post( f );
    }
    else
    {
      //added the boost strand support for the handlers which may not be multi-thread safe
      //solution: the tasks belonging to the same strand can only be executed sequentially
      auto it = HandlerToStrand.find(handler);
      strand_sptr strand;
      if (it == HandlerToStrand.end())
      {
        strand = std::make_shared<boost::asio::io_service::strand>(_service);
        HandlerToStrand[handler] = strand;
      }
      else strand = it->second;
    
      _service.post( strand->wrap(f) );
    }
  }
  
  // if doRecreate is true, we will re-create the new threads.
  void Recover(bool doRecreate = true);
   
  bool HasThreadCrashed()
  {
    std::lock_guard<std::mutex> guard(LockForCrashedThreadList);
    return (CrashedThreadCount > 0);
  }
    
  void MarkHandler(boost::thread::id tid, std::string handler);
  void RemoveHandler(boost::thread::id tid);
  
  void InterruptHandlers()
  {
    std::lock_guard<std::mutex> guard(LockForTidToHandlerMap);
    std::vector<boost::thread::id> list;
    for (auto it = TidToHandlerMap.begin(); it != TidToHandlerMap.end(); ++it)
    {
      // now the dbMgr is not in the TidToThreadMap though they are recorded in TidToHandlerMap.
      // so we are not able to find dbMgr/netMgr in TidToThreadMap
      if (TidToThreadMap.find(it->first) != TidToThreadMap.end())
      {
        LOG(csmd, debug) << "Interrupting the thread: tid= " << it->first << " name=" << it->second;
        boost::thread * thd = TidToThreadMap[it->first];
        thd->interrupt();
        list.push_back(it->first);
      }
    }
    for (size_t i=0; i<list.size(); i++) TidToHandlerMap.erase(list[i]);
  }
  
  inline void SetMainLoopThreadId(boost::thread::id tid) { _main_loop_thread_id = tid; }
  static void segfault_handler(int sig);
  static void segfault_sigaction_handler(int sig, siginfo_t *si, void *arg);
  
private:
  inline void addThread()
  {
    auto worker = [this] { return _service.run(); };
      
    boost::thread* thd = new boost::thread(worker);
      
    _thread_grp.add_thread( thd );
    TidToThreadMap[thd->get_id()] = thd;
      
    LOG(csmd, debug) << "ThreadPool(): new thread = " << thd->get_id();
  }
  
};

} // namespace daemon
} // namespace csm

#endif
