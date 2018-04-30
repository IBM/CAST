/*================================================================================

    csmd/src/daemon/src/thread_pool.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "include/thread_pool.h"
#include "src/csmi_request_handler/csmi_base.h"

#include <csignal>
#include <ucontext.h>
#include <iomanip>

namespace csm {
namespace daemon {

std::map<boost::thread::id, boost::thread* > csm::daemon::ThreadPool::TidToThreadMap;
std::vector<boost::thread::id> csm::daemon::ThreadPool::CrashedThreadList;
volatile int csm::daemon::ThreadPool::CrashedThreadCount;
boost::thread::id csm::daemon::ThreadPool::_main_loop_thread_id;

std::map<boost::thread::id, std::string> csm::daemon::ThreadPool::TidToHandlerMap;

std::mutex csm::daemon::ThreadPool::LockForCrashedThreadList;
std::mutex csm::daemon::ThreadPool::LockForTidToHandlerMap;

std::map<CSMI_BASE *, csm::daemon::ThreadPool::strand_sptr> csm::daemon::ThreadPool::HandlerToStrand;
 
void csm::daemon::ThreadPool::Recover(bool doRecreate)
{
  LOG(csmd, debug) << "ThreadPool::Recover(): try to recreate " << CrashedThreadCount << " thread...";

  std::vector<boost::thread*> threadList;

  { // start locking
  std::lock_guard<std::mutex> guard(LockForCrashedThreadList);
  
  if (CrashedThreadCount > 0) threadList.resize(CrashedThreadCount, nullptr);
  for (size_t i=0; i<CrashedThreadList.size(); i++)
  {
    auto it = TidToThreadMap.find( CrashedThreadList[i] );
    if ( it != TidToThreadMap.end() )
      threadList[i] = it->second;
  }
  CrashedThreadList.clear();
  CrashedThreadCount = 0;
  } // end locking
  
  for (size_t i=0; i<threadList.size(); i++)
  {
    boost::thread* thd = threadList[i];
    if (thd == nullptr)
    {
      LOG(csmd, warning) << "ThreadPool::Recover(): Cannot find the thread in the TidToThreadMap...";
      continue;
    }
    boost::thread::id tid = thd->get_id();
    LOG(csmd, debug) << "ThreadPool::Recover(): Next, recreate the thread for the crashed one " << tid;
    
    TidToThreadMap.erase(tid);
    // sanity check: before removing it from thread_group, check if the thread is in
    if ( !_thread_grp.is_thread_in( thd ) )
    {
        LOG(csmd, critical) << "ThreadPool::Recover(): Cannot find the thread " << tid << " in the thread group";
    }
    _thread_grp.remove_thread(thd);
    
    if (doRecreate) addThread();
    
    { // start locking
    std::lock_guard<std::mutex> guard(LockForTidToHandlerMap);
    TidToHandlerMap.erase(tid);
    } // end locking
  }

  _thread_grp_size = _thread_grp.size();
  LOG(csmd, debug) << "ThreadPool::Recover() thread group size = " << _thread_grp_size;
}

void csm::daemon::ThreadPool::MarkHandler(boost::thread::id tid, std::string handler)
{
  std::lock_guard<std::mutex> guard(LockForTidToHandlerMap);
  TidToHandlerMap[tid] = handler;
  //LOG(csmd, info) << "!!!!!! # of active threads: " << TidToHandlerMap.size() << " !!!!!!";
}

void csm::daemon::ThreadPool::RemoveHandler(boost::thread::id tid)
{
  std::lock_guard<std::mutex> guard(LockForTidToHandlerMap);
  TidToHandlerMap.erase(tid);
}
  
void csm::daemon::ThreadPool::segfault_handler(int sig)
{
  std::string handler;
  boost::thread::id tid= boost::this_thread::get_id();

  if ( _main_loop_thread_id == tid )
  {
    std::cerr << "ERROR: segfault_handler() - The main loop thread has segmentation fault!" << std::endl;
    pthread_exit(NULL);
  }
    
  { // start locking
  std::lock_guard<std::mutex> guard(LockForCrashedThreadList);
  CrashedThreadList.push_back(tid);
  CrashedThreadCount++;
  } // end locking
  
  { // start locking
  std::lock_guard<std::mutex> guard(LockForTidToHandlerMap);
  auto it = TidToHandlerMap.find(tid);
  if ( it != TidToHandlerMap.end() )
    handler = it->second;
  } // end locking

  
  if ( !handler.empty() )
  {
    LOG(csmd, critical) << "SEGFAULT: thread_id = " << tid << " handler_name = " << handler;
  }
  else
  {
    LOG(csmd, critical) << "SEGFAULT: Cannot find the info. for this thread_id: " << tid;
  }
  pthread_exit(NULL);
}


void csm::daemon::ThreadPool::segfault_sigaction_handler(int sig, siginfo_t *si, void *arg)
{
  // const char *str = "Into handler\n";
  // write(1, str, strlen(str)); // async signal safe
  LOG(csmd, critical) << "Caught segfault when accessing data at address: " << std::hex << si->si_addr << std::dec;
  
  // code below is not portable. for ppc only
#if defined(__ppc__) || defined(__powerpc__) || defined(_ARCH_PPC)
  ucontext_t *context = (ucontext_t *)arg;
  // should we do + 0x04 here?
  LOG(csmd, critical) << "Instruction Address from where the crash happened: 0x" << std::hex << context->uc_mcontext.regs->link << std::dec;
#endif

  std::string handler;
  boost::thread::id tid= boost::this_thread::get_id();

  if ( _main_loop_thread_id == tid )
  {
    std::cerr << "ERROR: segfault_handler() - The main loop thread has segmentation fault!" << std::endl;
  }
  else
  {
    auto it = TidToHandlerMap.find(tid);
    if ( it != TidToHandlerMap.end() )
      handler = it->second;

    if ( !handler.empty() )
    {
      LOG(csmd, critical) << "SEGFAULT: thread_id = " << tid << " handler_name = " << handler;
    }
    else
    {
      LOG(csmd, critical) << "SEGFAULT: Cannot find the info. for this thread_id: " << tid;
    }
   }
   
    // resume to the segfault default signal handling
    signal(sig, SIG_DFL);
    raise(sig);
}


} // end namespace daemon
} // end namespace csm
