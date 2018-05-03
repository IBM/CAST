/*================================================================================

    csmnet/src/CPP/csm_version_msg.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <algorithm>

#include "logging.h"
#include "csm_network_config.h"
#include "csm_version_msg.h"


csm::network::VersionMsg* csm::network::VersionMsg::_version = NULL;

#define SUPPORTED_CAST_100_COMMIT_PUB "675a5de990b20fcd"
#define SUPPORTED_CAST_100_COMMIT_IBM "12194309a3636ef6"

unsigned
csm::network::ExtractVersionMajor( const std::string &vStr )
{
  size_t dotpos_major = 0;
  try
  {
    dotpos_major = vStr.find('.');
  }
  catch ( std::out_of_range &e )
  {
    return 0;
  }
  if( dotpos_major == std::string::npos )
    return 0;
  char *endp;
  std::string major_str = std::string( vStr, 0, dotpos_major );
  long int major = std::strtol( major_str.c_str(), &endp, 10 );
  if(( major == 0 ) && ( endp == major_str.c_str() ))
    return 0;
  return (uint8_t)major;
}

unsigned
csm::network::ExtractVersionCumulFix( const std::string &vStr )
{
  size_t dotpos_major = 0;
  size_t dotpos_cumulfix = 0;
  try
  {
    dotpos_major = vStr.find('.');
    dotpos_cumulfix = vStr.rfind('.');
  }
  catch ( std::out_of_range &e )
  {
    return 0;
  }
  if(( dotpos_major == std::string::npos ) || ( dotpos_cumulfix == std::string::npos ))
    return 0;
  char *endp;
  size_t len = dotpos_cumulfix - dotpos_major - 1;
  std::string cumul_str = std::string( vStr, dotpos_major+1, len );
  long int cumul = std::strtol( cumul_str.c_str(), &endp, 10 );
  if(( cumul == 0 ) && ( endp == cumul_str.c_str() ))
    return 0;
  return (uint8_t)cumul;
}

unsigned
csm::network::ExtractVersionEfix( const std::string &vStr )
{
  size_t dotpos_cumulfix = 0;
  try
  {
    dotpos_cumulfix = vStr.rfind('.');
  }
  catch ( std::out_of_range &e )
  {
    return 0;
  }
  if( dotpos_cumulfix == std::string::npos )
    return 0;
  char *endp;
  std::string efix_str = std::string( vStr, dotpos_cumulfix+1, std::string::npos );
  long int efix = std::strtol( efix_str.c_str(), &endp, 10 );
  if(( efix == 0 ) && ( endp == efix_str.c_str() ))
    return 0;
  return (uint8_t)efix;
}

csm::network::VersionMsg::VersionMsg( const std::string v,
            const std::string h )
: _Hostname( h ), _Sequence(0)
{
  SetVersion( v );
}

void
csm::network::VersionMsg::SetVersion( const std::string newVersion )
{
  _Version = newVersion;
  _VersionNumbers[0] = csm::network::ExtractVersionMajor( _Version );
  _VersionNumbers[1] = csm::network::ExtractVersionCumulFix( _Version );
  _VersionNumbers[2] = csm::network::ExtractVersionEfix( _Version );
}

bool
csm::network::VersionMsg::Acceptable( const csm::network::VersionStruct &vStruct )
{
  unsigned major = csm::network::ExtractVersionMajor( vStruct._Version );
  bool supported = true;
  supported &= ( major == _VersionNumbers[ 0 ] );

  unsigned cumul = csm::network::ExtractVersionCumulFix( vStruct._Version );
  LOG( csmnet, trace ) << "VersionMsg::Acceptable: this: " << _VersionNumbers[0] << "." << _VersionNumbers[1] << "." << _VersionNumbers[2]
    << " comp: " << major << "." << cumul << "." << csm::network::ExtractVersionEfix( vStruct._Version );
  supported &= (( cumul >= (unsigned)std::max( (int)_VersionNumbers[ 1 ] - CSM_CUMULATIVE_FIX_MAX_BACK_LEVEL_SUPPORT, 0) ) && ( cumul <= _VersionNumbers[ 1 ] ));

  /*
   * This code is to temporarily support the last CAST versions that used the git commit for the version between daemons
   */
  if(( supported == false ) && ( major == 0 ) && ( _VersionNumbers[0] == 1 ))
  {
    std::string shortVersion = std::string( vStruct._Version, 0, strlen( SUPPORTED_CAST_100_COMMIT_PUB ) );
    LOG( csmnet, trace ) << "ShortIn: " << shortVersion << " pub: " << SUPPORTED_CAST_100_COMMIT_PUB;
    int pubv = shortVersion.compare( SUPPORTED_CAST_100_COMMIT_PUB );
    int ibmv = shortVersion.compare( SUPPORTED_CAST_100_COMMIT_IBM );
    supported = (( 0 == pubv ) || ( 0 == ibmv ));
  }

  return supported;
}
