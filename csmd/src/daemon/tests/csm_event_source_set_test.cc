/*================================================================================

    csmd/src/daemon/tests/csm_event_source_set_test.cc

  © Copyright IBM Corporation 2015-2019. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <stdlib.h>
#include <string>

#include <logging.h>
#include "csm_test_utils.h"
#include "csm_daemon_config.h"
#include "include/csm_event_source_set.h"


class EventSourceDummy : public csm::daemon::EventSource
{
public:
  EventSourceDummy( csm::daemon::EventManager const * aEvMgr = nullptr,
                    csm::daemon::RetryBackOff *aRetryBackoff = new csm::daemon::RetryBackOff() )
  : EventSource( aRetryBackoff, 0, false )
  {}

  virtual ~EventSourceDummy()
  {
  }

  virtual csm::daemon::CoreEvent* GetEvent( const std::vector<uint64_t> &i_BucketList )
  {
    return nullptr;
  }
  virtual bool QueueEvent( const csm::daemon::CoreEvent *i_Event )
  {
    return WakeUpMainLoop();
  }
};


csm::daemon::Configuration* getConfig( int argc, char **argv )
{
  char **nargv = argv;
  int nargc = argc;

  if( argc < 5 )
  {
    nargc = 5;
    nargv = new char*[nargc];
    nargv[1] = strdup("-r");
    nargv[2] = strdup("u");
    nargv[3] = strdup("-f");
    char *basepath = getenv("CORAL_HOME");
    if( basepath == nullptr )
    {
      LOG(csmd, warning) << "${CORAL_HOME} undefined, using ${HOME} !!!";
      basepath = getenv("HOME");
    }
    std::string filepath = std::string( basepath );
    filepath.append("/csm_utility.cfg");
    nargv[4] = strdup( (char*)filepath.c_str() );
    LOG(csmd, error ) << "add config path: " << nargv[4];
  }
//  for( auto i=0; i<nargc, )
  try
  {
    csm::daemon::RunMode runmode;
    csm::daemon::Configuration* conf = csm::daemon::Configuration::Instance( nargc, nargv, &runmode );

    for( int n=0; n<nargc; ++n )
      if( nargv[n] ) free( nargv[n] );
    if(nargv) delete [] nargv;

    return conf;
  }
  catch( csm::daemon::Exception &e )
  {
    LOG( csmd, error ) << "Failed configuration: " << e.what();
    return nullptr;
  }
}

int main( int argc, char **argv )
{
  int rc = 0;

  csm::daemon::Configuration *config = getConfig( argc, argv );
  rc += TESTFAIL( config, nullptr );
  if( rc > 0 )
    return rc;

  LOG( csmd, always ) << "configured, now creating source-set rc=" << rc;
  csm::daemon::EventSourceSet *set = new csm::daemon::EventSourceSet();
  rc += TESTFAIL( set, nullptr);
  if( rc > 0 )
  {
    config->Cleanup();
    return rc;
  }

  LOG( csmd, always ) << "source-set created, adding sources. rc=" << rc;
  EventSourceDummy *source = nullptr;
  try
  {
    source = new EventSourceDummy( nullptr );
    rc += TESTFAIL( source, nullptr );
    rc += TEST( set->Add( source, 815, 0, 10 ), 0 );

    rc += TEST( set->Remove( source, 0 ), 0 );
    if( source != nullptr )
      delete source;
  }
  catch (csm::daemon::Exception &e )
  {
    rc += 1;
    LOG( csmd, error ) << "EventSource creation: " << e.what();
  }

  delete set;

  config->Cleanup();
  LOG(csmd, always) << "Test complete rc=" << rc;
  return rc;
}

