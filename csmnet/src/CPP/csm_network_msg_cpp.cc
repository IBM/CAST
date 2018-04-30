/*================================================================================

    csmnet/src/CPP/csm_network_msg_cpp.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "csm_network_msg_cpp.h"

bool csm::network::Message::Init( const uint8_t aCmdType,
                                  const uint8_t aFlags,
                                  const uint8_t aPriority,
                                  const uint64_t aMessageID,
                                  const uint32_t aSrcAddr,
                                  const uint32_t aDstAddr,
                                  const uint32_t aUserID,
                                  const uint32_t aGroupID,
                                  const csm::network::MessageDataType &aData,
                                  const uint32_t aReserved )
{
  _Header._ProtocolVersion = CSM_NETWORK_PROTOCOL_VERSION;
  _Header._CommandType = aCmdType;

  SetPriority( aPriority );
  _Header._Flags = aFlags;

  _Header._MessageID = aMessageID;
  _Data = aData;
  _Header._DataLen = _Data.length();
  _Header._UserID = aUserID;
  _Header._GroupID = aGroupID;
  _Header._Reserved = aReserved;
  _Header._CheckSum = 0;
  _Header._CheckSum = csm_header_check_sum( &_Header, _Data.c_str() );

  return Validate();
}

bool csm::network::Message::InitHdr( const char * aInput )
{
  csm_network_header *in = (csm_network_header *)aInput;

  _Header = *in;

  return csm_header_validate( &_Header );
}
