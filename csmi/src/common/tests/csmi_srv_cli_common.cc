/*================================================================================

    csmi/src/common/tests/csmi_srv_cli_common.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

// test source, sink, and processor for network handling

#include <errno.h>
#include <iostream>
#include <assert.h>
#include <signal.h>

#include <logging.h>
#include "csm_daemon_config.h"

#include "include/csm_core_event.h"
#include "include/csm_daemon_core.h"

#include "src/csmi_request_handler/csmi_base.h"

#include "csmnet/src/C/csm_network_internal_api_c.h"
#include "include/csm_daemon.h"

extern "C" int csmi_client(int argc, char *argv[]);

#define MQTTBROKER "/work/extras/mosquitto/usr/local/sbin/mosquitto"

static pid_t child_proc=-1;
csm::daemon::Daemon *gDaemon = nullptr;

void exit_handler(int signum)
{
   
  std::cout << ">>> Server exited..." << std::endl;

  // server runs normally as client exits first.
  // use the client exit code when terminating
  int rc = 0;

  if( gDaemon != nullptr )
  {
    gDaemon->Stop();
    std::cout << " Sent STOP signal to server main loop." << std::endl;
  }
  else
  {
    if (child_proc != -1)
    {
      int status;
      waitpid( child_proc, &status, 0 );
      rc = WEXITSTATUS(status);
    }
    exit( rc );
  }
}

int mqtt_broker( pid_t aClient)
{
  // the server should exit if the csmi client exits.
  //signal(SIGCHLD, exit_handler);
  
  char *pwd;
  if ( (pwd = getenv("PWD")) == NULL) exit(-1);
  
  char *ptr;
  if ( (ptr = strstr(pwd, "bluecoral")) == NULL) exit(-1);
  ptr += strlen("bluecoral");
  
  int len = strlen(pwd) - strlen(ptr) + strlen(MQTTBROKER) + 1;
  *ptr = '\0';
  
  char *fullpath = (char *) malloc(len);
  sprintf(fullpath, "%s%s", pwd, MQTTBROKER);
  
  std::cout << ">>> Strated MQTT broker(" << getpid() << "): " << fullpath << std::endl;
  
  execl(fullpath, "mosquitto", 0, 0);
  perror("mqtt_broker execl failure!\n");
  
  return -1;
}

int server( pid_t aClient, int argc, char *argv[])
{
  int rc=0;
  std::cout << ">>> Started server with Master role " << std::endl;

  gDaemon = new csm::daemon::Daemon();

  // supress the logging in csmd and csmnet
  setLoggingLevel(csmd, error);
  setLoggingLevel(csmnet, error);

  // remember the child process
  child_proc = aClient;
  
  // the server should exit if the csmi client exits.
  signal(SIGCHLD, exit_handler);

//  Daemon.Stop();   // tell the daemon to just to a single iteration
  rc = gDaemon->Run( argc, argv );
  
  return rc;
}

int client( pid_t aServer, int argc, char *argv[] )
{
  int rc = 1;
  csm_net_endpoint_t *ep=NULL;


  std::cout << ">>> Started client..." << std::endl;
  if ( (ep = csm_net_unix_Init(0)) == NULL) {
    std::cout << ">>> Client failed to create a socket..." << std::endl;
    exit(-1);
  }

  // wait for the server is up running
  for( int retries = 1; ((rc > 0) && (retries < 10)); ++retries )
  {
      rc = csm_net_unix_Connect(ep, CSM_NETWORK_LOCAL_SSOCKET);
      if( rc )
      {
        usleep( retries * 500000 );
        rc = 1;
      }
  }
  csm_net_unix_Exit(ep);

  if( rc )
   std::cout << ">>> Client Failed to Connected to Server..." << std::endl;
  else {
   rc = csmi_client(argc, argv);
  }

  std::cout << ">>> Client exited (rc= " << rc << ")" << std::endl;
  return rc;
}
