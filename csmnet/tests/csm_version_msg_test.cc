/*================================================================================

    csmd/src/daemon/tests/csm_version_msg_test.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <iostream>

#include "logging.h"
#include "csmutil/include/csm_test_utils.h"

#define CSM_NETWORK_VERSION_MSG_TEST
#include "csmutil/include/csm_version.h"
#include "CPP/csm_version_msg.h"

int main( int argc, char **argv )
{
  int ret = 0;

  csm::network::VersionMsg *vmsg = csm::network::VersionMsg::Init("localhost");

  ret += TESTFAIL( vmsg, nullptr );
  if( ret != 0 )
  {
    LOG( csmd, error ) << "Failed to allocate/initialize version msg";
    return ret;
  }

  ret += TEST( vmsg->GetHostName().compare( "localhost" ), 0 );
  ret += TEST( vmsg->GetVersion().compare( CSM_VERSION ), 0 );
  ret += TEST( vmsg->GetSequence(), 0 );

  vmsg->Get(); // this should not touch any parts
  ret += TEST( vmsg->GetHostName().compare( "localhost" ), 0 );
  ret += TEST( vmsg->GetVersion().compare( CSM_VERSION ), 0 );
  ret += TEST( vmsg->GetSequence(), 0 );

  LOG( csmd, always ) << "Version: M=" << (int)vmsg->GetVersionMajor()
      << " C=" << (int)vmsg->GetVersionCumulFix()
      << " E=" << (int)vmsg->GetVersionEfix();
  ret += TEST( vmsg->GetVersionMajor(), 1 );
  ret += TEST( vmsg->GetVersionCumulFix(), 0 );
  ret += TEST( vmsg->GetVersionEfix(), 1 );

  // serialization would bump up the sequence #
  std::string archive = csm::network::VersionMsg::ConvertToBytes( vmsg );
  ret += TEST( vmsg->GetHostName().compare( "localhost" ), 0 );
  ret += TEST( vmsg->GetVersion().compare( CSM_VERSION ), 0 );
  ret += TEST( vmsg->GetSequence(), 1 );

  LOG( csmd, always ) << "Serialized: " << archive;

  csm::network::VersionStruct vstruct;
  csm::network::VersionMsg::ConvertToClass( archive, vstruct );
  ret += TEST( vmsg->GetHostName().compare( vstruct._Hostname ), 0);
  LOG( csmd, always ) << "vmsg.host: " << vmsg->GetHostName() << " vstruct.host: " << vstruct._Hostname;
  ret += TEST( vmsg->GetSequence() - 1, vstruct._Sequence );  // the archive contains the previous number
  LOG( csmd, always ) << "vmsg.seq: " << vmsg->GetSequence() << " vstruct.seq: " << vstruct._Sequence;
  ret += TEST( vmsg->GetVersion().compare( vstruct._Version ), 0 );
  LOG( csmd, always ) << "vmsg.ver: " << vmsg->GetVersion() << " vstruct.ver: " << vstruct._Version;

  vmsg->SetVersion( "UNKNOWN" );
  archive = csm::network::VersionMsg::ConvertToBytes( vmsg );
  LOG( csmd, always ) << "Serialized New: " << archive;

  ret += TEST( vmsg->GetVersionMajor(), 0 );
  ret += TEST( vmsg->GetVersionCumulFix(), 0 );
  ret += TEST( vmsg->GetVersionEfix(), 0 );

  LOG(csmd, info) << "Exit test: " << ret;

  return ret;
}
