/*================================================================================

    csmnet/src/CPP/csm_network_msg_cpp.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSM_NETWORK_SRC_CSM_NETWORK_MSG_CPP_H_
#define CSM_NETWORK_SRC_CSM_NETWORK_MSG_CPP_H_

#include <string>
#include <iostream>
#include <bitset>
#include "csmi/src/common/include/csmi_cmds.h"
#include "csm_network_header.h"
#include "../C/csm_network_msg_c.h"

namespace csm {
namespace network {

typedef std::string MessageDataType;

class Message
{
  csm_network_header_t _Header;
  csm::network::MessageDataType _Data;

public:
  Message( ) : _Header(), _Data() {}
  Message( csm_network_header const &aHeader,
           csm::network::MessageDataType const &aData ) : _Data( aData )
  {
    memcpy( &_Header, &aHeader, sizeof( csm_network_header_t ) );
  }
  Message( const Message &in )
  : _Data ( in._Data )
  {
    memcpy( &_Header, &( in._Header ), sizeof( csm_network_header_t ) );
  }
  ~Message() {}

  Message& operator=( const Message& aIn )
  {
    _Data = aIn.GetData();
    memcpy( &_Header, aIn.GetHeaderBuffer(), sizeof( csm_network_header_t ) );
    return *this;
  }
  bool Init( const uint8_t aCmdType,
             const uint8_t aFlags,
             const uint8_t aPriority,
             const uint64_t aMessageID,
             const uint32_t aSrcAddr,
             const uint32_t aDstAddr,
             const uint32_t aUserID,
             const uint32_t aGroupID,
             const csm::network::MessageDataType &aData,
             const uint32_t aReserved = 0 );
  bool InitHdr( const char * aInput );

  // overrides certain values to create a valid header to signal an error
  // e.g. after an invalid header was detected
  inline void CreateError( const bool aAck,
                           const uint8_t aPriority,
                           const csm::network::MessageDataType &aData = "" )
  {
    _Header._ProtocolVersion = CSM_NETWORK_PROTOCOL_VERSION;
//    SetCommandType( CSM_CMD_ERROR );
    SetErr();
    SetPriority( aPriority );
    SetData( aData );
  }

  inline bool Validate() const
  {
    return ( csm_header_validate( &_Header ) ) && ( CheckSumCalculate() == GetCheckSum() );
  }

  // calling the c-impl functions/macros here to make sure
  // we handle peculiarities about encoding in one single place
  inline bool GetAck() const { return CSM_HEADER_GET_ACK( &_Header ); }
  inline void SetAck() { CSM_HEADER_SET_ACK( &_Header ); }
  inline void ClrAck() { CSM_HEADER_CLR_ACK( &_Header ); }
  inline bool GetResp() const { return CSM_HEADER_GET_RESP( &_Header ); }
  inline void SetResp() { CSM_HEADER_SET_RESP( &_Header ); }
  inline void ClrResp() { CSM_HEADER_CLR_RESP( &_Header ); }
  inline bool GetErr() const { return CSM_HEADER_GET_ERR( &_Header ); }
  inline void SetErr() { CSM_HEADER_SET_ERR( &_Header ); }
  inline void ClrErr() { CSM_HEADER_CLR_ERR( &_Header ); }
  inline bool GetCbk() const { return CSM_HEADER_GET_CBK( &_Header ); }
  inline void SetCbk() { CSM_HEADER_SET_CBK( &_Header ); }
  inline void ClrCbk() { CSM_HEADER_CLR_CBK( &_Header ); }
  inline bool GetInt() const { return CSM_HEADER_GET_INT( &_Header ); }
  inline void SetInt() { CSM_HEADER_SET_INT( &_Header ); }
  inline void ClrInt() { CSM_HEADER_CLR_INT( &_Header ); }
  inline bool GetMulticast() const { return CSM_HEADER_GET_MTC( &_Header ); }
  inline void SetMulticast() { CSM_HEADER_SET_MTC( &_Header ); }
  inline void ClrMulticast() { CSM_HEADER_CLR_MTC( &_Header ); }
  inline bool GetPrivateCheck() const { return CSM_HEADER_GET_PCK( &_Header ); }
  inline void SetPrivateCheck() { CSM_HEADER_SET_PCK( &_Header ); }
  inline void ClrPrivateCheck() { CSM_HEADER_CLR_PCK( &_Header ); }

  inline uint8_t GetFlags() const { return _Header._Flags; };
  inline void SetFlags( const uint8_t aFlags ) { _Header._Flags = aFlags; }

  inline uint8_t GetPriority() const { return _Header._Priority; }
  inline void SetPriority( const uint8_t aNewPrio ) { _Header._Priority = aNewPrio; }

  inline csmi_cmd_t GetCommandType() const { return (const csmi_cmd_t)_Header._CommandType; }
  inline void SetCommandType( const uint8_t aCmd ) { _Header._CommandType = aCmd; }

  inline uint64_t GetMessageID() const { return _Header._MessageID; }
  inline void SetMessageID( const uint64_t aMsgID ) { _Header._MessageID = aMsgID; }

  /** @brief Retrieves the Reserved ID for tracing job runs.
   * @return The contents of the header stored in the _Reserved bits.
   */
  inline uint32_t GetReservedID() const { return _Header._Reserved; }

  /** @brief Sets a reserved ID for tracing a handler run.
   * @param[in] aReservedID The new id for the header.
   */
  inline void  SetReservedID(const uint32_t aReservedID) {_Header._Reserved = aReservedID; }


  inline uint32_t GetCheckSum() const { return _Header._CheckSum; }
  inline uint32_t CheckSumCalculate() const { return csm_header_check_sum( &_Header, _Data.c_str() ); }
  inline uint32_t CheckSumUpdate() { _Header._CheckSum = CheckSumCalculate(); return _Header._CheckSum; }

  inline uint32_t GetDataLen() const { return _Header._DataLen; }
  inline void SetSrcAddr( const uint32_t aSrcAddr ) { }
  inline uint32_t GetSrcAddr() const { return 0; }
  inline void SetDstAddr( const uint32_t aDstAddr ) { }
  inline uint32_t GetDstAddr() const { return 0; }

  inline uint32_t GetUserID() const { return _Header._UserID; }
  inline void SetUserID( const uint32_t aUserID ) { _Header._UserID = aUserID; }
  inline uint32_t GetGroupID() const { return _Header._GroupID; }
  inline void SetGroupID( const uint32_t aGroupID ) { _Header._GroupID = aGroupID; }

  inline void SwapSrcDst() { }

  inline const csm::network::MessageDataType& GetData() const { return _Data; }
  inline const char* GetDataPtr() const { return _Data.c_str(); }
  inline void SetDataOnly( const csm::network::MessageDataType& data ) { _Data = data; _Header._DataLen = data.length(); }
  inline void SetData( const csm::network::MessageDataType& data )
  {
    SetDataOnly( data );
    CheckSumUpdate();
  }

  inline char * GetHeaderBuffer() const { return (char*)&_Header; }
  inline bool RequiresACK() const { return ( csm_net_msg_RequiresAck( &_Header )); }
  inline bool PrivateRequiresUserCompare() const { return ( GetPrivateCheck() || GetInt() ); }

  bool operator==( const csm::network::Message &msg ) const
  {
    return ( 0 == memcmp( &( msg._Header ), &( _Header ), sizeof( csm_network_header_t ) ) )
            && ( 0 == _Data.compare( _Data ) );
  }

};

inline std::string
cmd_to_string( const csmi_cmd_t cmd )
{
  if( cmd < CSM_CMD_INVALID )
    return csmi_cmds_t_strs[ cmd ];
  else
    return "Unknown command.";
}


template<class stream>
static stream&
operator<<( stream &out, const csm::network::Message &data )
{
  out << "**** ID:" << data.GetMessageID()
      << " cmd: " << cmd_to_string( data.GetCommandType() ) << "(" << (int)data.GetCommandType() << ")"
      << "; flags: b(" << std::bitset<8>( data.GetFlags() ) << ")"
      << " uid|gid: " << data.GetUserID() << ":" << data.GetGroupID()
      << "; datalen: " << data.GetDataLen() << " Data: " << std::string( data.GetData(), 0, CSM_LOG_RAW_MSG_LIMIT );
  if( data.GetDataLen() > CSM_LOG_RAW_MSG_LIMIT )
    out << "...(total: " << data.GetDataLen() << ")";
  out << "****";
  return (out);
}

}  // namespace network
} // namespace csm

#endif /* CSM_NETWORK_SRC_CSM_NETWORK_MSG_CPP_H_ */
