/*================================================================================

    csmi/src/common/tests/test_csmi_srv_cli.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/* csm_csmi_srv_cli.cc
 *
 ******************************************/

#include "csm_daemon_config.h"
#include <sys/types.h>
#include <sys/wait.h>

extern int server( pid_t aClient, int argc, char *argv[]);
extern int client( pid_t aServer, int argc, char *argv[] );

int main( int argc, char **argv )
{
  int rc = 0;


  pid_t pid = getpid();
  pid_t child = fork();

  if( child < 0 )
  {
    perror("Forking child failed");
    rc = child;
  }
  else
  {
    if ( child == 0 ) // child, forked process
    {
      // need a client that feeds network messages into a server
      rc += client( pid, argc, argv );
    }
    else // parent process, main thread
    {
      rc += server( child, argc, argv);

      // come here if server exits before client.
      // use the server exit code for terminating as server exits abnormally
      printf( "SERVER FINISHED: rc=%d\n", rc );
      int status;
      waitpid( child, &status, 0 );
      rc += (unsigned) WEXITSTATUS( status );
    }
  }

  printf( "TEST EXIT: rc=%d\n", rc );
  return rc;
}
