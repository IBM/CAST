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

  vmsg->SetVersion( "1.0.0" );

  ret += TEST( vmsg->GetHostName().compare( "localhost" ), 0 );
  ret += TEST( vmsg->GetVersion().compare( "1.0.0" ), 0 );
  ret += TEST( vmsg->GetSequence(), 0 );

  vmsg->Get(); // this should not touch any parts
  ret += TEST( vmsg->GetHostName().compare( "localhost" ), 0 );
  ret += TEST( vmsg->GetVersion().compare( "1.0.0" ), 0 );
  ret += TEST( vmsg->GetSequence(), 0 );

  LOG( csmd, always ) << "Version: M=" << csm::network::ExtractVersionMajor( vmsg->GetVersion() )
      << " C=" << csm::network::ExtractVersionCumulFix( vmsg->GetVersion() )
      << " E=" << csm::network::ExtractVersionEfix( vmsg->GetVersion() );
  ret += TEST( csm::network::ExtractVersionMajor( vmsg->GetVersion() ), 1 );
  ret += TEST( csm::network::ExtractVersionCumulFix( vmsg->GetVersion() ), 0 );
  ret += TEST( csm::network::ExtractVersionEfix( vmsg->GetVersion() ), 0 );

  // serialization would bump up the sequence #
  std::string archive = csm::network::VersionMsg::ConvertToBytes( vmsg );
  ret += TEST( vmsg->GetHostName().compare( "localhost" ), 0 );
  ret += TEST( vmsg->GetVersion().compare( "1.0.0" ), 0 );
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

  // set a supported cumulative fix id
  vmsg->SetVersion( "1.1.0" );
  ret += TEST( csm::network::ExtractVersionCumulFix( vmsg->GetVersion() ), 1 );
  ret += TEST( vmsg->Acceptable( vstruct ), true );
  LOG( csmd, always ) << "Version supported: " << vmsg->Acceptable( vstruct ) << " current: " << vmsg->GetVersion();

  // set a different efix id
  vmsg->SetVersion( "1.0.53" );
  ret += TEST( csm::network::ExtractVersionEfix( vmsg->GetVersion() ), 53 );
  ret += TEST( vmsg->Acceptable( vstruct ), true );
  LOG( csmd, always ) << "Version supported: " << vmsg->Acceptable( vstruct ) << " current: " << vmsg->GetVersion();

  // set an unsupported cumulative fix id
  vmsg->SetVersion( "1.5.0" );
  ret += TEST( csm::network::ExtractVersionCumulFix( vmsg->GetVersion() ), 5 );
  ret += TEST( vmsg->Acceptable( vstruct ), false );
  LOG( csmd, always ) << "Version supported: " << vmsg->Acceptable( vstruct ) << " current: " << vmsg->GetVersion();

  // set a different major version
  vmsg->SetVersion( "2.0.0" );
  ret += TEST( csm::network::ExtractVersionMajor( vmsg->GetVersion() ), 2 );
  ret += TEST( vmsg->Acceptable( vstruct ), false );
  LOG( csmd, always ) << "Version supported: " << vmsg->Acceptable( vstruct ) << " current: " << vmsg->GetVersion();


  // set an old version string to test rebustness
  vmsg->SetVersion( "abcdefghikjlmnop" );
  archive = csm::network::VersionMsg::ConvertToBytes( vmsg );
  LOG( csmd, always ) << "Serialized with old string: " << archive;

  // The old-format version string should return all zero
  ret += TEST( csm::network::ExtractVersionMajor( vmsg->GetVersion() ), 0 );
  ret += TEST( csm::network::ExtractVersionCumulFix( vmsg->GetVersion() ), 0 );
  ret += TEST( csm::network::ExtractVersionEfix( vmsg->GetVersion() ), 0 );

  // test support for the earliest CAST GA 1.0.0 versions with git commit
  // should fail because of API shift/incompatability
  vmsg->SetVersion( "1.0.0" );
  ret += TEST( csm::network::ExtractVersionMajor( vmsg->GetVersion() ), 1 );
  vstruct._Version = "675a5de990b20fcd7adac14ded0410b0c9047d0c";
  ret += TEST( vmsg->Acceptable( vstruct ), false );
  LOG( csmd, always ) << "Version supported: " << vmsg->Acceptable( vstruct ) << " current: " << vmsg->GetVersion();

  // only support the git commits as long as major version is 1 (if at all)
  vmsg->SetVersion( "2.0.0" );
  ret += TEST( csm::network::ExtractVersionMajor( vmsg->GetVersion() ), 2 );
  ret += TEST( vmsg->Acceptable( vstruct ), false );
  LOG( csmd, always ) << "Version supported: " << vmsg->Acceptable( vstruct ) << " current: " << vmsg->GetVersion();

  // check for unsupported version
  vmsg->SetVersion( "1.0.0" );
  ret += TEST( csm::network::ExtractVersionMajor( vmsg->GetVersion() ), 1 );
  vstruct._Version = "abcdefghikjlmnopqrstuvw";
  ret += TEST( vmsg->Acceptable( vstruct ), false );
  LOG( csmd, always ) << "Version supported: " << vmsg->Acceptable( vstruct ) << " current: " << vmsg->GetVersion();

  LOG(csmd, info) << "Exit test: " << ret;

  return ret;
}
