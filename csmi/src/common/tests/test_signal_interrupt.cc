/*================================================================================

    csmi/src/common/tests/test_signal_interrupt.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include "csmi/include/csm_api_common.h"

#include <iostream>
#include <string>

#include <string.h>
#include <stdint.h>
#include <unistd.h>  
#include <signal.h>     // Provides kill()
#include <pthread.h>

using std::cout;
using std::endl;
using std::string;

volatile sig_atomic_t exit_signal_received = 0;

void* signal_thread(void*);
void* csm_init_thread(void*);

pthread_t signal_thread_id = 0;
pthread_t csm_init_thread_id = 0;

bool set_sa_restart(false);

void signal_handler(int32_t signal_number, siginfo_t *siginfo, void *context)
{
  exit_signal_received = signal_number;
  //cout << "Received signal " << strsignal(exit_signal_received) << " (" << exit_signal_received << ")" << endl;
}

int32_t main(int argc, char *argv[])
{
  int32_t retval(0);
    
  if (argc > 1)
  {
    if (string(argv[1]) == "-r")
    {
      set_sa_restart = true;
    }
    else
    {
      cout << "(no arguments) Running without sigaction flag SA_RESTART, EINTR expected." << endl;
      cout << "(-r flag)      Running with sigaction flag SA_RESTART, EINTR should not occur." << endl;
      exit(0);
    }
  }
 
  if (!set_sa_restart)
  {
    cout << "(no arguments) Running without sigaction flag SA_RESTART, EINTR expected." << endl;
  }
  else
  {
    cout << "(-r flag)      Running with sigaction flag SA_RESTART, EINTR should not occur." << endl;
  }
 
  // Create the csm_init thread
  retval = pthread_create(&csm_init_thread_id, NULL, &csm_init_thread, NULL);
  if (retval != 0)
  {
    cout << "Error: pthread_create() for csm_init_thread returned " << retval << endl;
    exit(retval);
  }

  sleep(1);

  // Create the signal thread
  retval = pthread_create(&signal_thread_id, NULL, &signal_thread, NULL);
  if (retval != 0)
  {
    cout << "Error: pthread_create() for signal_thread returned " << retval << endl;
    exit(retval);
  }

  // Wait forever
  pthread_join(csm_init_thread_id, NULL);  

  return 0;
}

void* signal_thread(void*)
{
  int32_t retval(0);
  
  while (true)
  {
    retval = pthread_kill(csm_init_thread_id, SIGUSR1);
    if (retval != 0)
    {
      cout << "kill returned errno=" << errno << endl;
      exit(retval);
    }
  }
}

void* csm_init_thread(void*)
{
  int32_t retval(0);
  
  struct sigaction newaction;
  memset(&newaction, 0, sizeof(newaction));

  newaction.sa_sigaction = &signal_handler;
  if (set_sa_restart)
  {
    // With SA_RESTART set, system calls should not receive EINTR
    newaction.sa_flags = SA_SIGINFO | SA_RESTART;
  }
  else
  {
    // Without SA_RESTART set, system calls should receive EINTR
    newaction.sa_flags = SA_SIGINFO;
  }
  
  // Install a signal handler to catch SIGUSR1
  retval = sigaction(SIGUSR1, &newaction, NULL);
  if (retval != 0)
  {
    cout << "Error: sigaction() returned " << retval << endl;
    exit(retval);
  }
  
  while (true)
  {
    // Try to initialize the CSM API library
    retval = csm_init_lib();
    if (retval != 0)
    { 
      cout << "Error: csm_init_lib() returned " << retval << endl;
      exit(retval);
    }
  
    // Try to terminate the csm API library in an orderly fashion
    retval = csm_term_lib();
    if (retval != 0)
    { 
      cout << "Error: csm_term_lib() returned " << retval << endl;
      exit(retval);
    }
  }
}
