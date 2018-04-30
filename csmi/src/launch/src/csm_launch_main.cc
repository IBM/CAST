/*================================================================================

    csmi/src/launch/src/csm_launch_main.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include "csmi/include/csm_api_workload_manager.h"

#include <iostream>
#include <list>
#include <string>
#include <sstream>

#include <string.h>
#include <stdint.h>
#include <stdlib.h>     // Provides getenv(), setenv()
#include <unistd.h>     // Provides gethostname()
#include <signal.h>     // Provides kill()
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>   // Provides waitpid()
#include <pwd.h>

using std::cout;
using std::endl;
using std::list;
using std::string;
using std::istringstream;
using std::ostringstream;

// LSF Environment Variable Definitions
#define NODES "LSB_MCPU_HOSTS"
#define JOBID "LSB_JOBID"
#define ALLOCATIONID "CSM_ALLOCATION_ID"

#define PMIX_SERVER_PATH "/opt/ibm/spectrum_mpi/jsm_pmix/bin/pmix_server"

extern char **environ;

int32_t ProcessArguments(int argc, char *argv[], string &o_user_script);
void DisplayUsage();

int32_t CreateAllocation(const list<string> &i_nodelist, const uint64_t &i_jobid, const string &i_user_script, uint64_t &o_allocationid);

volatile sig_atomic_t exit_signal_received = 0;
volatile sig_atomic_t _user_script_pid = 0;

void signal_handler(int32_t signal_number)
{
  exit_signal_received = signal_number;
  
  // Try to kill the user script as quickly as possible.
  if (_user_script_pid != 0)
  {
    kill(_user_script_pid, SIGTERM);
  }
}

int32_t main(int argc, char *argv[])
{
  int32_t retval;
  
  // Install a signal handler to catch SIGINT and SIGTERM
  // In the event of a bkill, LSF will send SIGINT, then SIGTERM, then SIGKILL
  // Catching SIGINT and SIGTERM to attempt to clean up before the SIGKILL is sent
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  string user_script("");
  retval = ProcessArguments(argc, argv, user_script);
  if (retval != 0)
  {
    return retval;
  }
 
  // Get the hostname and the short hostname
  char hostname[HOST_NAME_MAX];
  string long_hostname;
  string short_hostname;
  retval = gethostname(hostname, HOST_NAME_MAX);
  if (retval != 0)
  {
    cout << "Error: gethostname returned " << retval << endl;
    return retval;
  }

  long_hostname = hostname;
  short_hostname = long_hostname.substr(0, long_hostname.find("."));

  cout << "Launch node hostname is " << hostname << " (" << short_hostname << ")" << endl;

  // Read the node_list from the environment variable represented by NODES
  list<string> node_list;
  char *env_val(nullptr);

  env_val = getenv(NODES);
  
  if (env_val != nullptr)
  {
    istringstream nodes_in(env_val);
    string node;

    int32_t count(0);
    while (nodes_in >> node)
    {
      //cout << node << endl;
      
      // LSB_MCPU_HOSTS=c931f04p20-vm01 1 c931f04p20-vm25 2 c931f04p20-vm22 2 c931f04p20-vm03 2
      // Only save the node names, discard the slot count
      // LSF passes the launch node in the list, remove the launch node before creating the allocation
      if ( ( count % 2 == 0 ) &&
           ( node != long_hostname ) &&
           ( node != short_hostname) )
      {
        node_list.push_back(node);
      }
      count++;
    } 
  }
  else
  {
    DisplayUsage();
    return 1; 
  }

  // Read the jobid from the environment variable represented by JOBID
  uint64_t job_id;

  env_val = getenv(JOBID);
  
  if (env_val != nullptr)
  {
    istringstream job_id_in(env_val);
    job_id_in >> job_id;
  }
  else
  {
    DisplayUsage();
    return 1; 
  }

  // Try to initialize the CSM API library
  retval = csm_init_lib();
  if (retval != 0)
  { 
    cout << "Error: csm_init_lib() returned " << retval << endl;
    return retval;
  }

  // Try to create the allocation
  uint64_t allocation_id(0);
  retval = CreateAllocation(node_list, job_id, user_script, allocation_id);
  if (retval != 0)
  {
    exit(-1); 
  }

  // Set the allocation id into an environment variable
  ostringstream alloc_out;
  alloc_out << allocation_id;
  retval = setenv(ALLOCATIONID, alloc_out.str().c_str(), 1);

  // Create a temporary hostfile to pass to PMIx server
  char host_file[] = "/tmp/hostfileXXXXXX";
  int host_file_fd;
  host_file_fd = mkstemp(host_file);
  if (host_file_fd > 0)
  {
    FILE* host_file_ptr = fdopen(host_file_fd, "w");
    // PMIx requires the full hostname of the launch node to be the first line in the file
    fprintf(host_file_ptr, "%s\n", long_hostname.c_str());
    
    // Now print the short names of all nodes in the allocation
    for (list<string>::iterator node_itr = node_list.begin(); node_itr != node_list.end(); ++node_itr)
    {
      fprintf(host_file_ptr, "%s\n", node_itr->c_str());
    }
    fclose(host_file_ptr);
  }
  else
  {
    cout << "Error: could not create temporary host file." << endl;
  }

  // Try to fork and exec pmix_server
  pid_t pmix_server_pid;
  
  const char *const pmix_args[6] = {PMIX_SERVER_PATH, hostname, "-ptsargs", "-f", host_file, NULL};
  
  int32_t rc(0);

  pmix_server_pid = fork();

  if (pmix_server_pid > 0)
  {
    // In the parent
    cout << "PMIx server pid=" << pmix_server_pid << endl;
  }
  else if (pmix_server_pid == 0)
  {
    // In the child
    
    // Set actions for SIGINT and SIGTERM back to the default actions
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    
    // Close any open fds
    int maxfd=sysconf(_SC_OPEN_MAX);
    for (int fd = 3; fd < maxfd; fd++)
    {
      close(fd);
    }

    rc = execve(PMIX_SERVER_PATH, const_cast<char * const*>(pmix_args), environ);
    if (rc == -1)
    {
      cout << "execve() returned " << rc << ", errno=" << errno << endl;
      _Exit(127); 
    }
  }
  else
  {
    // fork() failed
    cout << "fork() returned " << pmix_server_pid << ", errno=" << errno << endl;
  }

  sleep(1);

  // Try to fork and exec the user script
  pid_t user_script_pid;

  user_script_pid = fork();

  if (user_script_pid > 0)
  {
    // In the parent
    cout << "User script pid=" << user_script_pid << endl;
    _user_script_pid = user_script_pid;
  }
  else if (user_script_pid == 0)
  {
    // In the child
   
    // We want the user script to exit first, if possible
    // However, there is currently no way to force that to happen
    // Set actions for SIGINT and SIGTERM back to the default actions
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
 
    // Close any open fds
    int maxfd=sysconf(_SC_OPEN_MAX);
    for (int fd = 3; fd < maxfd; fd++)
    {
      close(fd);
    }
    
    if (argc < 2)
    {
      cout << "Error: No command provided to launch." << endl;
      _Exit(126); 
    }

    rc = execve(argv[1], &argv[1], environ);
    if (rc == -1)
    {
      cout << "execve() returned " << rc << ", errno=" << errno << endl;
      _Exit(127); 
    }
  }
  else
  {
    // fork() failed
    cout << "fork() returned " << user_script_pid << ", errno=" << errno << endl;
  }

  // Wait for either the user script or PMIX server to exit
  // In normal operation the user script should exit and we will kill the PMIX server
  // During abnormal termination of PMIX server, we may need to kill the user script instead
  int32_t status(0);
  pid_t waitrc(0);

  while ((waitrc = waitpid(-1, &status, 0)) == -1)
  {
    if (errno != EINTR)
    {
      cout << "ERROR: waitpid() returned " << waitrc << ", errno=" << errno << endl;
      break;
    }
    else
    {
      if (exit_signal_received != 0)
      {
        cout << "Received signal " << strsignal(exit_signal_received) << " (" << exit_signal_received << ")" << endl;
        break;
      }
    }
  }
       
  if (exit_signal_received != 0)
  { 
    cout << "Received signal " << strsignal(exit_signal_received) << " (" << exit_signal_received << ")" << endl;
  }

  if (waitrc == user_script_pid)
  {
    cout << "user script (pid=" << user_script_pid << ") has terminated, killing pmix server" << endl;
    rc = kill(pmix_server_pid, SIGTERM);  
    if (rc != 0)
    {
      cout << "kill returned errno=" << errno << endl;
    }
  }
  else if (waitrc == pmix_server_pid)
  {
    cout << "pmix server (pid=" << pmix_server_pid << ") has terminated, killing user script" << endl;
    rc = kill(user_script_pid, SIGTERM);  
    if (rc != 0)
    {
      cout << "kill returned errno=" << errno << endl;
    }
  }
  
  // During a normal shutdown, wait for the other process to exit
  // If we have been killed, skip this step and try to exit quickly
  if (exit_signal_received == 0)
  {
    while ((waitrc = waitpid(-1, &status, 0)) == -1)
    {
      if (errno != EINTR)
      {
        cout << "ERROR: waitpid() returned " << rc << ", errno=" << errno << endl;
        break;
      }
    }
  }

  // Delete the temporary hostfile
  retval = unlink(host_file);
  if (retval != 0)
  {
    cout << "Error: unlink() returned " << retval << ", errno=" << errno << ", file=" << host_file << endl;
  }

  // Try to delete the allocation
  cout << "csm_allocation_delete: allocation_id=" << allocation_id << endl;
  csm_api_object *csm_obj = NULL;
  csm_allocation_delete_input_t delete_input;
  delete_input.allocation_id = allocation_id;

  retval = csm_allocation_delete(&csm_obj, &delete_input);

  if (retval == 0)
  {
    cout << "  allocation_id " << allocation_id << " successfully deleted." << endl;
  } 
  else
  {
    cout << "  errcode=" << csm_api_object_errcode_get(csm_obj)
         << " errmsg=\"" << csm_api_object_errmsg_get(csm_obj) << "\"" << endl;
  }

  // it's the csmi library's responsibility to free internal space
  csm_api_object_destroy(csm_obj);
  
  // Try to terminate the csm API library in an orderly fashion
  retval = csm_term_lib();
  if (retval != 0)
  { 
    cout << "Error: csm_term_lib() returned " << retval << endl;
    return retval;
  }

  return 0;
}

int32_t ProcessArguments(int argc, char *argv[], string &o_user_script)
{
  for (int i = 0; i < argc; i++)
  {
    //cout << "Arg " << i << ": " << argv[i] << endl;
  }

  if ( ( argc == 1 ) ||
       ( ( argc == 2 ) && (strcmp("-h", argv[1]) ==  0) ) ||
       ( ( argc == 2 ) && (strcmp("--help", argv[1]) == 0) ) )
  {
    DisplayUsage();
    return 1;
  }
  else
  {
    if (argc > 1)
    {
      o_user_script = argv[1];
    }
    return 0;
  }
}

void DisplayUsage()
{
  cout << "csmlaunch takes the name of the user script and it's arguments as arguments." << endl;
  cout << "Example:" << endl;
  cout << "csmlaunch /home/user/script.sh arg1 arg2 arg3" << endl;
  cout << endl;
  cout << "Additional required configuration settings are passed via environment variables." << endl;
  cout << "Set the list of nodes to use for the allocation using:" << endl;
  cout << "export " << NODES << "=\"node1 node1_slots node2 node2_slots node3 node3_slots\"" << endl;
  cout << endl;
  cout << "Set the primary job id for the allocation using:" << endl;
  cout << "export " << JOBID << "=1234" << endl;
}

int32_t CreateAllocation(const list<string> &i_nodelist, const uint64_t &i_jobid, const string &i_user_script, uint64_t &o_allocationid)
{
  int32_t retval;
  
  // Try to create an allocation
  cout << "csm_allocation_create:" << endl;
  csmi_allocation_t   allocation;
  char             **compute_nodes;
  csm_api_object    *csm_obj = NULL;
  time_t             t;
  struct tm         *tm;
  char               tbuf[32];
  
  char hostname[HOST_NAME_MAX];
  retval = gethostname(hostname, HOST_NAME_MAX);
  if (retval != 0)
  {
    cout << "Error: gethostname returned " << retval << endl;
    return retval;
  }

  // Get the user name
  string user_name;
  uid_t user_id(0);
  gid_t group_id(0); 
  struct passwd *passwd_ptr(nullptr);
  
  user_id = geteuid();
  passwd_ptr = getpwuid(user_id);
  if (passwd_ptr != nullptr)
  {
    user_name = passwd_ptr->pw_name;
    group_id = passwd_ptr->pw_gid;
  }
  else
  {
    user_name = "CSM_LAUNCH_UNKNOWN_USER";
  }
  endpwent();
      
  memset(&allocation, 0, sizeof(csmi_allocation_t));
  allocation.primary_job_id = i_jobid;
  allocation.launch_node_name = strdup(hostname);
  allocation.state = CSM_RUNNING;
  allocation.type = CSM_JSM;
  allocation.user_name = strdup(user_name.c_str());
  allocation.user_id = user_id;
  allocation.user_group_id = group_id;
  allocation.user_script = strdup(i_user_script.c_str());
  time(&t);
  tm = localtime(&t);
  strftime(tbuf, sizeof(tbuf), "%F %T", tm);
  allocation.job_submit_time = strdup(tbuf);
  
  allocation.num_nodes = i_nodelist.size();
  compute_nodes = (char **)malloc(sizeof(char *) * allocation.num_nodes);
  
  int j = 0;
  for (list<string>::const_iterator node_itr = i_nodelist.begin(); node_itr != i_nodelist.end(); ++node_itr)
  {
    compute_nodes[j] = strdup(node_itr->c_str());
    j++;
  }
  allocation.compute_nodes = compute_nodes;
  
  retval = csm_allocation_create(&csm_obj, &allocation);

  if (retval == 0)
  {
    cout << "  allocation " << allocation.allocation_id << " created successfully, user_name:" 
         << allocation.user_name << " num_nodes:" << allocation.num_nodes << endl;
  
    cout << "  nodelist for the allocation:" << endl;
    for (uint32_t i = 0; i < allocation.num_nodes; i++) 
    {
      cout << "    " << allocation.compute_nodes[i] << endl;
    }

  }
  else
  {
    cout << "  errcode=" << csm_api_object_errcode_get(csm_obj)
         << " errmsg=\"" << csm_api_object_errmsg_get(csm_obj) << "\"" << endl;
    
    return retval;
  }
    
  // Return the allocation id via o_allocationid
  o_allocationid = allocation.allocation_id;

  // it's the csmi library's responsibility to free internal space
  csm_api_object_destroy(csm_obj);
  
  // it's the csmi client's responsibility to free compute_nodes
  for (uint32_t i = 0; i < allocation.num_nodes; i++)
  {
    free(compute_nodes[i]);
  }
  free(compute_nodes);

  return 0;
}
