/*================================================================================

    csmd/src/daemon/tests/csm_daemon_config_test.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/** file csm_daemon_config_test.cc
 *
 ******************************************/

#include <errno.h>
#include <iostream>

#include <logging.h>
#include <csm_daemon_config.h>

bool RoleMakesSense( const std::string &aRoleName )
{
  switch( aRoleName[0] )
  {
    case 'M':
    case 'm':
    case 'A':
    case 'a':
    case 'C':
    case 'c':
    case 'U':
    case 'u':
      return true;
    default:
      return false;
  }
}

int ConfigTest( const std::string &aRoleName, const CSMDaemonRole aExpectedRole )
{
  try {
    csm::daemon::Configuration good = *csm::daemon::Configuration::Instance();

    // was the role setting successful?
    if( good.GetRole() != aExpectedRole )
      return EINVAL;
  }
  catch (csm::daemon::Exception &e)
  {
    LOG(csmd,error) << "Expected exception. We're good..." << e.what();

    // This exception would be expected behavior unless the requested role was valid
    if( RoleMakesSense( aRoleName ) )
      return EINVAL;
    else
      return 0;
  }
  catch (...)
  {
    LOG(csmd,error) << "Unexpected exception";
    return EINVAL;
  }
  return 0;
}

int SetRoleTest( csm::daemon::Configuration &aConfig,
                 const CSMDaemonRole aRole )
{
  try
  {
    aConfig.SetRole( aRole );
    // check if role was set
    if( aConfig.GetRole() != aRole )
      return EINVAL;
  }
  catch ( csm::daemon::Exception &e)
  {
    LOG(csmd,error) << "Requested Role out of range: " << e.what();
    return 0;
  }
  catch (...)
  {
    return EINVAL;
  }
  return 0;
}

int main( int argc, char **argv )
{
  int rc = 0;
  csm::daemon::Configuration *testconf = nullptr;

  // needs to fails because config singleton hasn't been created
  try {
    testconf = csm::daemon::Configuration::Instance();

  }
  catch (csm::daemon::Exception &e)
  {
    LOG(csmd,error) << "Expected exception. We're good..." << e.what();
  }
  catch (...)
  {
    rc += 1;
  }

  // SetRole Testing
  csm::daemon::RunMode runmode( csm::daemon::RUN_MODE::STARTED );

  testconf = csm::daemon::Configuration::Instance( argc, argv, &runmode);

  rc += SetRoleTest( *testconf, CSM_DAEMON_ROLE_UNKNOWN );
  rc += SetRoleTest( *testconf, CSM_DAEMON_ROLE_MASTER );
  rc += SetRoleTest( *testconf, CSM_DAEMON_ROLE_AGGREGATOR );
  rc += SetRoleTest( *testconf, CSM_DAEMON_ROLE_AGENT );
  rc += SetRoleTest( *testconf, CSM_DAEMON_ROLE_UTILITY );
  rc += SetRoleTest( *testconf, CSM_DAEMON_ROLE_MAX );

  return rc;
}
