/*================================================================================

    csmnet/src/CPP/endpoint_buffer.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSM_NETWORK_SRC_CPP_ENDPOINT_BUFFER_H_
#define CSM_NETWORK_SRC_CPP_ENDPOINT_BUFFER_H_

#include "address.h"
#include "csm_message_and_address.h"

namespace csm {
namespace network {

class EndpointBuffer {
  enum EndpointBufferStates {
    BUFFER_EMPTY = 0,
    BUFFER_MSG_COMPLETE = 1,
    BUFFER_MSG_PARTIAL = 2,
    BUFFER_MSG_INVALID = 3,
    BUFFER_HDR_PARTIAL = 4
  };

  char *_BufferBase;
  char *_BufferHead;
  char *_BufferTail;
  size_t _DataLen;
  EndpointBufferStates _BufferState;

public:
  EndpointBuffer()
  : _BufferBase( new char[ DGRAM_PAYLOAD_MAX ] ),
    _BufferHead( _BufferBase ),
    _BufferTail( _BufferBase ),
    _DataLen( 0 ),
    _BufferState( BUFFER_EMPTY )
  {
      memset(_BufferBase, 0x0, DGRAM_PAYLOAD_MAX);
  }

  virtual ~EndpointBuffer()
  { delete[] _BufferBase; }

  ssize_t Recv( csm::network::Message &o_Msg )
  {
    switch( _BufferState )
    {
      case BUFFER_EMPTY:
        if( _DataLen != ProcessedData() )
          throw csm::network::ExceptionProtocol("RecvBuffer protocol problem: marked empty, but has data.");
        return 0;
      case BUFFER_MSG_COMPLETE:
        LOG( csmnet, debug ) << "RecvBuffer Valid-msg offset=" << ProcessedData();
        o_Msg.InitHdr( _BufferHead );
        o_Msg.SetData( std::string( _BufferHead+sizeof( csm_network_header ), o_Msg.GetDataLen() ) );
        break;
      case BUFFER_MSG_INVALID:
      {
        // consume the whole remaining buffer, since the datalen cannot be trusted
        LOG(csmnet, debug) << "RecvBuffer Invalid-msg offset=" << ProcessedData() << " total=" << _DataLen;
        o_Msg.InitHdr( _BufferHead );
        std::string data = std::string( _BufferHead + sizeof( csm_network_header ),
                                        _DataLen - ProcessedData() - sizeof( csm_network_header ));
        o_Msg.SetErr();  // make sure this message is marked as an error message
        o_Msg.SetData( data );
        break;
      }
      case BUFFER_MSG_PARTIAL:
      case BUFFER_HDR_PARTIAL:
        LOG( csmnet, debug ) << "RecvBuffer Partial content... needs recv+update.";
        return 0;
      default:
        throw csm::network::ExceptionProtocol("Buffer State error. Unknown state.");
    }
    Progress();
    return o_Msg.GetDataLen() + sizeof( csm_network_header );
  }

  inline bool IsEmpty() const { return _BufferTail == _BufferHead; }
  inline bool HasPartialMsg() const
  { return (_BufferState == BUFFER_MSG_PARTIAL) || ( _BufferState == BUFFER_HDR_PARTIAL ); }
  ssize_t GetRecvSpace() const { return DGRAM_PAYLOAD_MAX - ( _BufferTail - _BufferBase ); }
  char * GetRecvBufferPtr() const { return _BufferTail; }
  virtual void Update( const ssize_t i_Skip,  const struct sockaddr *i_SrcAddr = nullptr )
  {
    if( _DataLen + i_Skip > DGRAM_PAYLOAD_MAX )
      throw csm::network::ExceptionProtocol("Buffer State protocol failure: Buffer overflow.");
    else
    {
      _DataLen += i_Skip;
      _BufferTail += i_Skip;
      _BufferState = Transition();
      LOG( csmnet, trace ) << "Updating buffer. recvd=" << i_Skip
          << " data=" << _DataLen
          << " new state=" << _BufferState;
    }
  }

protected:
  inline size_t ProcessedData() const { return _BufferHead - _BufferBase; }
  virtual void Reset()
  {
    bzero( _BufferBase, _DataLen );
    _DataLen = 0;
    _BufferHead = _BufferBase;
    _BufferTail = _BufferBase;
    _BufferState = BUFFER_EMPTY;
    LOG( csmnet, trace ) << "Reset buffer.";
  }

  inline EndpointBufferStates Transition() const
  {
    csm_network_header *hdr = (csm_network_header*)_BufferHead;
    if( _DataLen - ProcessedData() >= sizeof( csm_network_header ) )
    {
      if( csm_header_validate( hdr ) )
      {
        if( _DataLen - ProcessedData() >= sizeof( csm_network_header ) + hdr->_DataLen )
        {
          LOG( csmnet, debug ) << "Buffer transition: There's at least one more complete message.";
          return BUFFER_MSG_COMPLETE;
        }
        else
        {
          LOG( csmnet, debug ) << "Buffer transition: There's a partial message pending. missing="
              << hdr->_DataLen - ( _DataLen - ProcessedData() - sizeof( csm_network_header_t ));
          return BUFFER_MSG_PARTIAL;
        }
      }
      else
      {
        LOG( csmnet, error ) << "Buffer transition: There's a message with invalid/checksum pending.";
        return BUFFER_MSG_INVALID;
      }
    }
    if( _DataLen == ProcessedData() )
      return BUFFER_EMPTY;
    if( _DataLen - ProcessedData() < sizeof( csm_network_header ) )
      return BUFFER_HDR_PARTIAL;
    return BUFFER_EMPTY;
  }

  void Progress()
  {
    csm_network_header *hdr = (csm_network_header*)_BufferHead;
    switch( _BufferState )
    {
      case BUFFER_EMPTY:
        break;
      case BUFFER_MSG_COMPLETE:
      {
        ssize_t skip = sizeof( csm_network_header ) + hdr->_DataLen;
        if( skip + ProcessedData() < DGRAM_PAYLOAD_MAX )
          _BufferHead += skip;
        else
        {
          Reset();
          throw csm::network::ExceptionProtocol("Buffer State protocol failure: Buffer overflow.");
        }

        if( ProcessedData() == _DataLen )
          Reset();
        _BufferState = Transition();
        LOG( csmnet, trace ) << "Updating buffer. new state=" << _BufferState;
        break;
      }
      case BUFFER_MSG_INVALID:
        Reset();
        break;
      case BUFFER_MSG_PARTIAL:

      case BUFFER_HDR_PARTIAL:
      default:
        break;
    }
  }
};

class EndpointStateUnix : public EndpointBuffer {
  AddressUnix_sptr _SrcAddr;

public:
  EndpointStateUnix( )
  : EndpointBuffer(),
    _SrcAddr()
  {}

  virtual ~EndpointStateUnix() {}

  ssize_t Recv( csm::network::MessageAndAddress &o_MsgAddr )
  {
    // preserve the current SrcAddress, because Recv() might call Reset() in case of invalid messages
    AddressUnix_sptr preserve = _SrcAddr;
    ssize_t rlen = EndpointBuffer::Recv( o_MsgAddr._Msg );

    // restore the previous src address if there was any data
    if( rlen != 0 )
    {
      o_MsgAddr.SetAddr( preserve );
      _SrcAddr = preserve;
    }
    return rlen;
  }
  inline AddressUnix_sptr GetAddr() const { return _SrcAddr; }

  inline void UpdateAddress( const struct sockaddr_un *i_Addr )
  {
    if( i_Addr == nullptr )
    {
      _SrcAddr = nullptr;
      return;
    }

    // create a new address if current address is null or different
    if( ( _SrcAddr == nullptr ) ||
        ( memcmp( i_Addr->sun_path, _SrcAddr.get()->_SockAddr.sun_path, UNIX_PATH_MAX ) ) )
    {
      _SrcAddr = std::make_shared<csm::network::AddressUnix>( i_Addr->sun_path );
    }
  }

  virtual void Update( const ssize_t i_Skip, const struct sockaddr *i_SrcAddr )
  {
    if(( HasPartialMsg() ) &&
        ( memcmp( ((const struct sockaddr_un*)i_SrcAddr)->sun_path, _SrcAddr.get()->_SockAddr.sun_path, UNIX_PATH_MAX ) ) )
      throw csm::network::ExceptionProtocol("CSMNET BROKEN PROTOCOL: (missing feature) Received Data from different client while in PARTIAL data state." );
    EndpointBuffer::Update( i_Skip );
    UpdateAddress( (sockaddr_un*)i_SrcAddr );
  }

private:
  virtual void Reset()
  {
    EndpointBuffer::Reset();
    _SrcAddr = nullptr;
  }
};

} // namespace csm_network
}
#endif /* CSM_NETWORK_SRC_CPP_ENDPOINT_STATE_H_ */
