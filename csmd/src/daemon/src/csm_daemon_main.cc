/*================================================================================

    csmd/src/daemon/src/csm_daemon_main.cc

  © Copyright IBM Corporation 2015-2019. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <signal.h>
#include <include/csm_daemon.h>

csm::daemon::Daemon *Daemon = nullptr;

void ExitHandler( int sig )
{
  if( Daemon != nullptr )
    Daemon->Stop();
  else
    raise( sig );
}

int main( int argc, char **argv )
{
  Daemon = new csm::daemon::Daemon();

  signal( SIGTERM, ExitHandler );
  signal( SIGINT, ExitHandler );

  int rc = 0;
  try {
    rc = Daemon->Run( argc, argv );
  }
  catch ( ... ) {}

  if( Daemon != nullptr )
    delete Daemon;

  return rc;
}


