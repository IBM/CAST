/*================================================================================

    csmnet/src/CPP/csm_version_msg.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "csm_version_msg.h"


csm::network::VersionMsg* csm::network::VersionMsg::_version = NULL;


csm::network::VersionMsg::VersionMsg( const std::string v,
            const std::string h )
: _Version( v ), _Hostname( h ), _Sequence(0)
{
}

uint8_t
csm::network::VersionMsg::GetVersionMajor() const
{
  size_t dotpos_major = 0;
  try
  {
    dotpos_major = _Version.find('.');
  }
  catch ( std::out_of_range &e )
  {
    return 0;
  }
  char *endp;
  std::string major_str = std::string( _Version, 0, dotpos_major );
  long int major = std::strtol( major_str.c_str(), &endp, 10 );
  if(( major == 0 ) && ( endp == major_str.c_str() ))
    return 0;
  return (uint8_t)major;
}

uint8_t
csm::network::VersionMsg::GetVersionCumulFix() const
{
  size_t dotpos_major = 0;
  size_t dotpos_cumulfix = 0;
  try
  {
    dotpos_major = _Version.find('.');
    dotpos_cumulfix = _Version.rfind('.');
  }
  catch ( std::out_of_range &e )
  {
    return 0;
  }
  char *endp;
  std::string cumul_str = std::string( _Version, dotpos_major+1, dotpos_cumulfix );
  long int cumul = std::strtol( cumul_str.c_str(), &endp, 10 );
  if(( cumul == 0 ) && ( endp == cumul_str.c_str() ))
    return 0;
  return (uint8_t)cumul;
}

uint8_t
csm::network::VersionMsg::GetVersionEfix() const
{
  size_t dotpos_cumulfix = 0;
  try
  {
    dotpos_cumulfix = _Version.rfind('.');
  }
  catch ( std::out_of_range &e )
  {
    return 0;
  }
  char *endp;
  std::string efix_str = std::string( _Version, dotpos_cumulfix+1, std::string::npos );
  long int efix = std::strtol( efix_str.c_str(), &endp, 10 );
  if(( efix == 0 ) && ( endp == efix_str.c_str() ))
    return 0;
  return (uint8_t)efix;
}
