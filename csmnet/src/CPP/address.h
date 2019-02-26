/*================================================================================

    csmnet/src/CPP/address.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/** @file address.h
 * @ingroup csm_net
 * @brief defines the address types
 *
 * CSM uses several different network protocols and types. This file defines the
 * type of addresses required to cover the handling of these networks
 *
 */

#ifndef CSM_NETWORK_SRC_CPP_ADDRESS_H_
#define CSM_NETWORK_SRC_CPP_ADDRESS_H_

#include <iostream>
#include <iomanip>
#include <memory>
#include <algorithm>
#include <string>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/un.h>      // struct sockaddr_un
#include <arpa/inet.h>
#include <inttypes.h>
#include <boost/crc.hpp>

#include "csm_network_exception.h"
#include "csm_network_config.h"

#include "logging.h"

namespace csm {
namespace network {

/** @brief Available types of addresses
 *
 *  We have different types of endpoints and thus different major types of addresses
 *  An additional address type is ABSTRACT to generalize addressing of MASTER, AGGREGATOR, etc..
 */
enum AddressType {
  CSM_NETWORK_TYPE_UNKNOWN,     ///< Unknown address type
  CSM_NETWORK_TYPE_LOCAL,       ///< Local address type (currently UNIX DOMAIN SOCKET)
  CSM_NETWORK_TYPE_PTP,         ///< Point-2-Point address type for Compute-to-Aggregator connections
  CSM_NETWORK_TYPE_AGGREGATOR,  ///< dedicated connectian between AGG and MASTER daemons
  CSM_NETWORK_TYPE_UTILITY,     ///< dedicated connection between UTL and MASTER daemons
  CSM_NETWORK_TYPE_ABSTRACT,    ///< Abstract address type for general access to MASTER, AGGREGATOR
  CSM_NETWORK_TYPE_MAX          ///< Limiter for sanity checks
};

/** @brief output operator for address type */
template<class stream>
static stream&
operator<<( stream &out, const csm::network::AddressType &aType )
{
  switch( aType )
  {
    case CSM_NETWORK_TYPE_UNKNOWN:
      out << "CSM_NETWORK_TYPE_UNKNOWN";
      break;
    case CSM_NETWORK_TYPE_LOCAL:
      out << "CSM_NETWORK_TYPE_LOCAL";
      break;
    case CSM_NETWORK_TYPE_PTP:
      out << "CSM_NETWORK_TYPE_COMPUTE";
      break;
    case CSM_NETWORK_TYPE_AGGREGATOR:
      out << "CSM_NETWORK_TYPE_AGGREGATOR";
      break;
    case CSM_NETWORK_TYPE_UTILITY:
      out << "CSM_NETWORK_TYPE_UTILITY";
      break;
    case CSM_NETWORK_TYPE_MAX:
      out << "CSM_NETWORK_TYPE_MAX";
      break;
    default:
      out << "ERROR: !!!OUT-OF-RANGE!!!";
  }
  return (out);
}

/** @typedef AddressCode
 * @brief code used as key to keep track of known addresses in maps
 *
 * AddressCode is generated from address data depending on address type
 */
typedef uint64_t AddressCode;

/** @def PUB_SUB_MAGIC
 * @brief Magic value for MQTT address, since there's only one connection
 * @note last 2 bits have to match the address type to prevent collisions with other address types
 */
#define PUB_SUB_MAGIC ( (csm::network::AddressCode)0x13ull )


/** @class Address
 * @brief Abstract base class for network Addresses
 *
 * The base class is an abstract type because it will not have
 * sufficient informations to be an address for any endpoint
 */
class Address {
protected:
  AddressType _AddrType;  ///< Type of address
  bool _Empty;            ///< Whether address is empty or not
public:
  /** @brief construct address from parameters
   *
   * @param [in]  aType    address type to initialize, default value: CSM_NETWORK_TYPE_UNKNOWN
   * @param [in]  aEmpty   whether address is empty, default value: true
   */
  Address( const AddressType aType = CSM_NETWORK_TYPE_UNKNOWN, const bool aEmpty = true )
  : _AddrType( aType ), _Empty( aEmpty ) {}
  /** @brief copy constructor
   *
   * @param [in]   aAddr   address object to copy
   */
  Address( const Address &aAddr )
  : _AddrType( aAddr._AddrType ), _Empty( aAddr._Empty ) {}
  virtual ~Address() {}
  /** @brief Get the address type
   * @return address type constant
   */
  inline AddressType GetAddrType() const { return _AddrType; }
  /** @brief Set the address type
   * @param[in]  aType  the new type to set
   */
  inline void SetAddrType( const AddressType aType ) { _AddrType = aType; }
  /** @brief pure virtual to create a string with address type specific information
   * @return string of address data
   */
  virtual std::string Dump() const = 0;
  /** @brief pure virtual copy function
   * @return pointer to copy of this address object
   */
  virtual Address* copy() const = 0;
  /** @brief Set the address empty flag
   * @param[in]  aEmpty  new status of the flag, default value: true
   */
  inline void SetEmpty( const bool aEmpty = true ) { _Empty = aEmpty; }
  /** @brief Get whether the address is empty or not
   * @return constant bool whether address is empty or not
   */
  inline bool IsEmpty() const { return _Empty; }
  /** @brief pure virtual generate address code
   * @return constant address code that can be used as key/unique identifier
   */
  virtual AddressCode MakeKey() const = 0;

  /** @brief calculates the hash based on an input buffer
   *
   * @param[in]  aPath  pointer to beginning of buffer
   * @param[in]  len    length of the buffer, since there maybe 0-bytes it cannot be interpreted as string
   * @return crc64 code
   */
  inline AddressCode KeyFromString( const char* aPath, const int len ) const
  {
    boost::crc_optimal<64, 0x42F0E1EBA9EA3693, 0, 0, false, false> crc;
    AddressCode *ptr = (AddressCode*)aPath;

    crc.process_bytes( ptr, len );
    return crc.checksum();
  }
};

/** @typedef Address_sptr
 * @brief shared_ptr definition of Address
 */
typedef std::shared_ptr<csm::network::Address> Address_sptr;

/** @class AddressUnix
 * @brief local Unix address type for client-daemon interaction
 */
class AddressUnix : public Address {
public:
  sockaddr_un _SockAddr;  ///< UNIX DOMAIN SOCKET address information

  /** @brief construct unix address based on socket path
   *
   *  Supports abstract addresses that start with 0-byte and may have additional 0-bytes
   *  in the sequence. So if the input starts with 0-byte, the next UNIX_PATH_MAX bytes are
   *  taken as the full abstract address
   *
   * @param[in]  aSocketPath   the path data for the socket path, default value: empty string
   *
   */
  AddressUnix( const char *aSocketPath = " " )
  : Address( CSM_NETWORK_TYPE_LOCAL, ( aSocketPath == nullptr ) || ( std::string( aSocketPath ) == " " ) )
  {
    if( aSocketPath == nullptr )
      throw csm::network::Exception("Unable to create address from nullptr socket path.");
    // abstract addresses consist of the full UNIX_PATH_MAX and start with 0-byte
    // they can have 0-bytes in between too, that's why length has to be UNIX_MAX_PATH for abstract addresses
    unsigned length = UNIX_PATH_MAX;
    if( aSocketPath[0] != 0 )
    {
      length = strnlen( aSocketPath, UNIX_PATH_MAX );
      _Empty = ( (length == 0) || (std::string( aSocketPath ) == " ") );
    }

    uint64_t sum = 0;
    for( uint64_t i=0; (sum == 0) && (i < UNIX_PATH_MAX / sizeof( uint64_t )); ++i )
      sum += *(uint64_t*)&(aSocketPath[ i * sizeof( uint64_t ) ]);

    if( sum==0 )
      throw csm::network::Exception("Invalid Unix Socket Path");

    bzero( &_SockAddr, sizeof( struct sockaddr_un ) );

    _SockAddr.sun_family = AF_UNIX;

    memcpy( _SockAddr.sun_path, aSocketPath, length );
    LOG(csmnet,debug) << "Created UnixAddress: " << aSocketPath
        << " AddrFam=" << _SockAddr.sun_family
        << " Addr=" << Dump();
  }
  /** @brief copy constructor
   * @param[in] aAddr  address object to copy
   */
  AddressUnix( const AddressUnix &aAddr )
  : Address( CSM_NETWORK_TYPE_LOCAL, aAddr.IsEmpty() )
  {
    if( aAddr.GetAddrType() != CSM_NETWORK_TYPE_LOCAL )
      throw csm::network::Exception("Invalid Address Type. Expecting LOCAL.");

    memcpy( &_SockAddr, &aAddr._SockAddr, sizeof( sockaddr_un ) );
  }
  /** @brief copy function
   * @return a new copy of this address
   */
  Address* copy() const
  {
    return new AddressUnix( *this );
  }
  /** @brief assignment operator
   * @param[in]  aAddr  address object to assign to this object
   */
  AddressUnix& operator=( const AddressUnix &aAddr )
  {
    if( aAddr.GetAddrType() != CSM_NETWORK_TYPE_LOCAL )
      throw csm::network::Exception("Invalid Address Type. Expecting LOCAL.");
    SetAddrType( CSM_NETWORK_TYPE_LOCAL );
    SetEmpty( aAddr.IsEmpty() );
    memcpy( &_SockAddr, &aAddr._SockAddr, sizeof( sockaddr_un ) );
    return *this;
  }
  /** @brief destructor */
  virtual ~AddressUnix() {}
  /** @brief Dump of address information into string
   *
   * Tries to dump current address information in socketpath as a string.
   * If this is an abstract address, the first 0-byte is skipped and the
   * returned string is created from the remaining data.  Note that this
   * can have strange consequences depending on the data in the abstract
   * address.
   *
   * @return String with address information
   */
  virtual std::string Dump() const
  {
    return _SockAddr.sun_path[0] ? std::string( _SockAddr.sun_path ) : std::string( &(_SockAddr.sun_path[1]) );
  }
  /**@brief generate address code/key
   *
   * Generates an address code based on the socket path data and address type.
   * It takes a hash of the socket path and adds the address type
   *
   * @return address code/key
   */
  virtual AddressCode MakeKey() const
  {
    return (KeyFromString( _SockAddr.sun_path, UNIX_PATH_MAX ) << 2) + _AddrType;
  }

  /** @brief comparison operator ==
   *
   * Compares this address with another one
   *
   * @param[in]  aAddr  address to compare with
   * @return true if addresses match
   */
  bool operator==( const AddressUnix &aAddr ) const
  {
    return (aAddr.GetAddrType() == GetAddrType() )
        && ( memcmp( aAddr._SockAddr.sun_path, _SockAddr.sun_path, UNIX_PATH_MAX ) == 0 );
  }
  /** @brief comparison operator !=
   *
   * @param[in]  aAddr  address to compare with
   * @return true if addresses don't match
   */
  bool operator!=( const AddressUnix &aAddr ) const
  {
    return ! operator==( aAddr );
  }
};

/** @typedef AddressUnix_sptr
 * @brief shared_ptr definition of unix address class
 */
typedef std::shared_ptr<csm::network::AddressUnix> AddressUnix_sptr;

/**@brief output operator for unix address */
template<class stream>
static stream&
operator<<( stream &out, const csm::network::AddressUnix &addr )
{
  out << addr._SockAddr.sun_path;
  return (out);
}


/** @class AddressPTP
 * @brief Point-to-Point address type for compute-aggregator connection/interaction
 */
class AddressPTP : public Address {
public:
  sockaddr_in _SockAddr;   ///<  INET address information

  /** @brief construct from IP and Port
   *
   * Constructor converts input parameters into network byte order before creating the address
   *
   * @param[in]   aIP    IP address in host byte order
   * @param[in]   aPort  Port number in host byte order
   */
  AddressPTP( const uint32_t aIP = 0,
              const in_port_t aPort = 0 )
  : Address( CSM_NETWORK_TYPE_PTP,
             ((aIP == 0) || (aPort == 0)) )
  {
    bzero( &_SockAddr, sizeof( sockaddr_in ) );
    _SockAddr.sin_family = AF_INET;
    _SockAddr.sin_port = htons( aPort );
    _SockAddr.sin_addr.s_addr = htonl( aIP );
  }
  /** @brief copy constructor
   * @param[in]  aAddr   address to be copied
   */
  AddressPTP( const AddressPTP &aAddr )
  : Address( CSM_NETWORK_TYPE_PTP,
             aAddr.IsEmpty() )
  {
    if( aAddr.GetAddrType() != CSM_NETWORK_TYPE_PTP )
      throw csm::network::Exception("AddressPTP::AddressPTP(): Invalid Address Type. Expecting PTP");
    memcpy( &_SockAddr, &(aAddr._SockAddr), sizeof( sockaddr_in ) );
  }

  /** @brief construct object based on INET address struct
   * @param[in]  aAddr  INET address struct with information to create this address object
   */
  AddressPTP( const sockaddr_in &aAddr )
  : Address( CSM_NETWORK_TYPE_PTP,
             ((aAddr.sin_addr.s_addr == 0) || (aAddr.sin_port == 0)) )
  {
    memcpy( &_SockAddr, &aAddr, sizeof( sockaddr_in ) );
  }

  /** @brief destructor */
  virtual ~AddressPTP() {}

  /** @brief copy function to retrieve a copy of this object
   * @return a copy of this PTP address
   */
  Address* copy() const
  {
    return new AddressPTP( *this );
  }
  /** @brief assignment operator
   * @param[in]  aAddr  PTP address to assign to this
   */
  AddressPTP& operator=( const AddressPTP &aAddr )
  {
    if( aAddr.GetAddrType() != CSM_NETWORK_TYPE_PTP )
      throw csm::network::Exception("AddressPTP::operator=(): Invalid Address Type. Expecting PTP.");
    SetAddrType( CSM_NETWORK_TYPE_PTP);
    SetEmpty( aAddr.IsEmpty() );
    memcpy( &_SockAddr, &aAddr._SockAddr, sizeof( sockaddr_in ) );
    return *this;
  }
  /** @brief generate address code from address information
   * @return address code/key
   */
  virtual AddressCode MakeKey() const
  {
    AddressCode ip = (AddressCode)_IP();
    // !!! IP is in network byte order, so higher bits will have the high frequency !!!
    return ( ip << 20 ) + ( (AddressCode)_Port() << 2 ) + (AddressCode)_AddrType;
  }
  /** @brief comparison operator ==
   * @param[in]  aAddr   address to compare with
   * @return  true if addresses match
   */
  bool operator==( const AddressPTP &aAddr ) const
  {
    return ( aAddr._IP() == _IP() ) && ( aAddr._Port() == _Port() );
  }
  /** @brief comparison operator !=
   * @param[in]  aAddr  address to compare with
   * @return  true if addresses don't match
   */
  bool operator!=( const AddressPTP aAddr ) const
  {
    return ! operator==( aAddr );
  }

  /** @brief dump a string composed of IP and Port in host byte order
   * @return string of address information
   */
  virtual std::string Dump() const
  {
    return std::string( inet_ntoa( _SockAddr.sin_addr ) ) + ':' + std::to_string( htons( _Port() ) );
  }

  /** @brief retrieve the IP address
   * @return IP address without conversion to host or network byte order
   */
  in_addr_t _IP() const { return _SockAddr.sin_addr.s_addr; }

  /** @brief retrieve the Port number
   * @return Port number without conversion to host or network byte order
   */
  in_port_t _Port() const { return _SockAddr.sin_port; }

};

/** @typedef AddressPTP_sptr
 * @brief shared_ptr definition of PTP address class
 */
typedef std::shared_ptr<csm::network::AddressPTP> AddressPTP_sptr;

/** \brief output operator for PTP addresses */
template<class stream>
static stream&
operator<<( stream &out, const csm::network::AddressPTP &addr )
{
  out << "0x" << std::hex << std::setw(8) << ntohl( addr._IP() )
      << ":" << std::dec << std::setw(5) << ntohs( addr._Port() );
  return (out);
}



/** @class AddressUtility
 * @brief Address type to hold utility node addresses
 */
class AddressUtility : public Address {

public:
  sockaddr_in _SockAddr;   ///<  INET address information

  /** @brief construct object from topic and type
   * @param[in]  aTopic    string to point to the topic
   * @param[in]  aMsgType  msg type associated with this topic/address
   */
  AddressUtility( const uint32_t aIP = 0,
                  const in_port_t aPort = 0 )
  : Address( CSM_NETWORK_TYPE_UTILITY,
             ((aIP == 0) || (aPort == 0)) )
  {
    bzero( &_SockAddr, sizeof( sockaddr_in ) );
    _SockAddr.sin_family = AF_INET;
    _SockAddr.sin_port = htons( aPort );
    _SockAddr.sin_addr.s_addr = htonl( aIP );
  }
  /** @brief copy constructor
   * @param[in]  aAddr   address to be copied
   */
  AddressUtility( const AddressPTP &aAddr )
  : Address( CSM_NETWORK_TYPE_UTILITY,
             aAddr.IsEmpty() )
  {
    if( aAddr.GetAddrType() != CSM_NETWORK_TYPE_UTILITY )
      throw csm::network::Exception("AddressUtility::AddressUtility(): Invalid Address Type. Expecting UTILITY");
    memcpy( &_SockAddr, &(aAddr._SockAddr), sizeof( sockaddr_in ) );
  }

  /** @brief construct object based on INET address struct
   * @param[in]  aAddr  INET address struct with information to create this address object
   */
  AddressUtility( const sockaddr_in &aAddr )
  : Address( CSM_NETWORK_TYPE_UTILITY,
             ((aAddr.sin_addr.s_addr == 0) || (aAddr.sin_port == 0)) )
  {
    memcpy( &_SockAddr, &aAddr, sizeof( sockaddr_in ) );
  }

  /** @brief destructor */
  virtual ~AddressUtility() {}

  /** @brief copy function to retrieve a copy of this object
   * @return a copy of this PTP address
   */
  Address* copy() const
  {
    return new AddressUtility( *this );
  }
  /** @brief assignment operator
   * @param[in]  aAddr  PTP address to assign to this
   */
  AddressUtility& operator=( const AddressUtility &aAddr )
  {
    if( aAddr.GetAddrType() != CSM_NETWORK_TYPE_UTILITY )
      throw csm::network::Exception("AddressPTP::operator=(): Invalid Address Type. Expecting PTP.");
    SetAddrType( CSM_NETWORK_TYPE_UTILITY);
    SetEmpty( aAddr.IsEmpty() );
    memcpy( &_SockAddr, &aAddr._SockAddr, sizeof( sockaddr_in ) );
    return *this;
  }
  /** @brief generate address code from address information
   * @return address code/key
   */
  virtual AddressCode MakeKey() const
  {
    AddressCode ip = (AddressCode)_IP();
    // !!! IP is in network byte order, so higher bits will have the high frequency !!!
    return ( ip << 20 ) + ( (AddressCode)_Port() << 2 ) + (AddressCode)_AddrType;
  }
  /** @brief comparison operator ==
   * @param[in]  aAddr   address to compare with
   * @return  true if addresses match
   */
  bool operator==( const AddressUtility &aAddr ) const
  {
    return ( aAddr._IP() == _IP() ) && ( aAddr._Port() == _Port() );
  }
  /** @brief comparison operator !=
   * @param[in]  aAddr  address to compare with
   * @return  true if addresses don't match
   */
  bool operator!=( const AddressUtility aAddr ) const
  {
    return ! operator==( aAddr );
  }

  /** @brief dump a string composed of IP and Port in host byte order
   * @return string of address information
   */
  virtual std::string Dump() const
  {
    return std::string( inet_ntoa( _SockAddr.sin_addr ) ) + ':' + std::to_string( htons( _Port() ) );
  }

  /** @brief retrieve the IP address
   * @return IP address without conversion to host or network byte order
   */
  in_addr_t _IP() const { return _SockAddr.sin_addr.s_addr; }

  /** @brief retrieve the Port number
   * @return Port number without conversion to host or network byte order
   */
  in_port_t _Port() const { return _SockAddr.sin_port; }

};

/** @typedef AddressUtility_sptr
 * @brief shared_ptr definition of AddressUtility
 */
typedef std::shared_ptr<csm::network::AddressUtility> AddressUtility_sptr;

template<class stream>
static stream&
operator<<( stream &out, const csm::network::AddressUtility &addr )
{
  out << "0x" << std::hex << std::setw(8) << ntohl( addr._IP() )
      << ":" << std::dec << std::setw(5) << ntohs( addr._Port() );
  return (out);
}



/** @class AddressAggregator
 * @brief Address type to hold aggregator node addresses
 */
class AddressAggregator: public Address {

public:
  sockaddr_in _SockAddr;   ///<  INET address information

  /** @brief construct object from topic and type
   * @param[in]  aTopic    string to point to the topic
   * @param[in]  aMsgType  msg type associated with this topic/address
   */
  AddressAggregator( const uint32_t aIP = 0,
                  const in_port_t aPort = 0 )
  : Address( CSM_NETWORK_TYPE_AGGREGATOR,
             ((aIP == 0) || (aPort == 0)) )
  {
    bzero( &_SockAddr, sizeof( sockaddr_in ) );
    _SockAddr.sin_family = AF_INET;
    _SockAddr.sin_port = htons( aPort );
    _SockAddr.sin_addr.s_addr = htonl( aIP );
  }
  /** @brief copy constructor
   * @param[in]  aAddr   address to be copied
   */
  AddressAggregator( const AddressPTP &aAddr )
  : Address( CSM_NETWORK_TYPE_AGGREGATOR,
             aAddr.IsEmpty() )
  {
    if( aAddr.GetAddrType() != CSM_NETWORK_TYPE_AGGREGATOR )
      throw csm::network::Exception("AddressAggregator::AddressAggregator(): Invalid Address Type. Expecting AGGREGATOR");
    memcpy( &_SockAddr, &(aAddr._SockAddr), sizeof( sockaddr_in ) );
  }

  /** @brief construct object based on INET address struct
   * @param[in]  aAddr  INET address struct with information to create this address object
   */
  AddressAggregator( const sockaddr_in &aAddr )
  : Address( CSM_NETWORK_TYPE_AGGREGATOR,
             ((aAddr.sin_addr.s_addr == 0) || (aAddr.sin_port == 0)) )
  {
    memcpy( &_SockAddr, &aAddr, sizeof( sockaddr_in ) );
  }

  /** @brief destructor */
  virtual ~AddressAggregator() {}

  /** @brief copy function to retrieve a copy of this object
   * @return a copy of this PTP address
   */
  Address* copy() const
  {
    return new AddressAggregator( *this );
  }
  /** @brief assignment operator
   * @param[in]  aAddr  PTP address to assign to this
   */
  AddressAggregator& operator=( const AddressAggregator &aAddr )
  {
    if( aAddr.GetAddrType() != CSM_NETWORK_TYPE_AGGREGATOR )
      throw csm::network::Exception("AddressAggregator::operator=(): Invalid Address Type. Expecting Aggregator.");
    SetAddrType( CSM_NETWORK_TYPE_AGGREGATOR);
    SetEmpty( aAddr.IsEmpty() );
    memcpy( &_SockAddr, &aAddr._SockAddr, sizeof( sockaddr_in ) );
    return *this;
  }
  /** @brief generate address code from address information
   * @return address code/key
   */
  virtual AddressCode MakeKey() const
  {
    AddressCode ip = (AddressCode)_IP();
    // !!! IP is in network byte order, so higher bits will have the high frequency !!!
    return ( ip << 20 ) + ( (AddressCode)_Port() << 2 ) + (AddressCode)_AddrType;
  }
  /** @brief comparison operator ==
   * @param[in]  aAddr   address to compare with
   * @return  true if addresses match
   */
  bool operator==( const AddressAggregator &aAddr ) const
  {
    return ( aAddr._IP() == _IP() ) && ( aAddr._Port() == _Port() );
  }
  /** @brief comparison operator !=
   * @param[in]  aAddr  address to compare with
   * @return  true if addresses don't match
   */
  bool operator!=( const AddressAggregator aAddr ) const
  {
    return ! operator==( aAddr );
  }

  /** @brief dump a string composed of IP and Port in host byte order
   * @return string of address information
   */
  virtual std::string Dump() const
  {
    return std::string( inet_ntoa( _SockAddr.sin_addr ) ) + ':' + std::to_string( htons( _Port() ) );
  }

  /** @brief retrieve the IP address
   * @return IP address without conversion to host or network byte order
   */
  in_addr_t _IP() const { return _SockAddr.sin_addr.s_addr; }

  /** @brief retrieve the Port number
   * @return Port number without conversion to host or network byte order
   */
  in_port_t _Port() const { return _SockAddr.sin_port; }

};

/** @typedef AddressAggregator_sptr
 * @brief shared_ptr definition of AddressAggregator
 */
typedef std::shared_ptr<csm::network::AddressAggregator> AddressAggregator_sptr;

template<class stream>
static stream&
operator<<( stream &out, const csm::network::AddressAggregator &addr )
{
  out << "0x" << std::hex << std::setw(8) << ntohl( addr._IP() )
      << ":" << std::dec << std::setw(5) << ntohs( addr._Port() );
  return (out);
}




/** @typedef AddressAbstractType
 * @brief address type of abstract address
 */
typedef enum {
  ABSTRACT_ADDRESS_NONE,         ///<  no type, not initialized
  ABSTRACT_ADDRESS_MASTER,       ///<  abstract master address
  ABSTRACT_ADDRESS_AGGREGATOR,   ///<  abstract aggregator address
  ABSTRACT_ADDRESS_BROADCAST,    ///<  broadcast address
  ABSTRACT_ADDRESS_SELF,         ///<  self-send address
  ABSTRACT_ADDRESS_MAX           ///<  limiter for sanity check
} AddressAbstractType;

/** @brief string conversion for abstract address type */
static std::string
AddressAbstractType_to_string( const AddressAbstractType aName )
{
  switch( aName )
  {
    case ABSTRACT_ADDRESS_NONE:       return "ABSTRACT_ADDRESS_NONE";
    case ABSTRACT_ADDRESS_MASTER:     return "ABSTRACT_ADDRESS_MASTER";
    case ABSTRACT_ADDRESS_AGGREGATOR: return "ABSTRACT_ADDRESS_AGGREGATOR";
    case ABSTRACT_ADDRESS_BROADCAST:  return "ABSTRACT_ADDRESS_BROADCAST";
    case ABSTRACT_ADDRESS_SELF:       return "ABSTRACT_ADDRESS_SELF";
    case ABSTRACT_ADDRESS_MAX:        return "ABSTRACT_ADDRESS_MAX";
    default:                          return "ERROR: !!!OUT-OF-RANGE!!!";
  }
}

/** @brief output operator for abstract address type */
template<class stream>
static stream&
operator<<( stream &out, const csm::network::AddressAbstractType &aType )
{
  out << AddressAbstractType_to_string( aType );
  return (out);
}

/** @class AddressAbstract
 * @brief Address type to handle abstract addresses
 */
class AddressAbstract : public Address
{
public:
  /** @brief construct abstract address from type name
   * @param[in]  aName  abstract address name, default value: ABSTRACT_ADDRESS_NONE
   */
  AddressAbstract( const AddressAbstractType aName = ABSTRACT_ADDRESS_NONE )
  : Address( CSM_NETWORK_TYPE_ABSTRACT, ( aName != ABSTRACT_ADDRESS_NONE ) ),
    _AbstractName( aName )
  {}
  /** @brief copy constructor
   * @param[in]  aAddr  abstract address to copy
   */
  AddressAbstract( const AddressAbstract &aAddr )
  : Address( CSM_NETWORK_TYPE_ABSTRACT, ( aAddr.IsEmpty() ) ),
    _AbstractName( aAddr._AbstractName )
  {}
  /** @brief destructor */
  virtual ~AddressAbstract() {}

  /** @brief copy function
   * @return copy of this object
   */
  Address* copy() const
  {
    return new AddressAbstract( *this );
  }

  /** @brief assignment operator
   * @param[in]  aAddr  address to assign
   */
  AddressAbstract& operator=( const AddressAbstract &aAddr )
  {
    if( aAddr.GetAddrType() != CSM_NETWORK_TYPE_ABSTRACT )
      throw csm::network::Exception("Invalid Address Type. Expecting ABSTRACT.");
    SetAddrType( CSM_NETWORK_TYPE_ABSTRACT );
    SetEmpty( aAddr.IsEmpty() );
    _AbstractName = aAddr._AbstractName;
    return *this;
  }

  /** @brief generate address code
   * @return 0 since this isn't used
   */
  virtual AddressCode MakeKey() const
  {
    return 0;
  }

  /** @brief dump the abstract address information
   * @return string with address information (type of abstract address)
   */
  virtual std::string Dump() const
  {
    return AddressAbstractType_to_string( _AbstractName );
  }

public:
  csm::network::AddressAbstractType _AbstractName;   ///<  abstract address type/name
};

/** @typedef AddressAbstract_sptr
 * @brief shared_ptr definition of AddressAbstract
 */
typedef std::shared_ptr<csm::network::AddressAbstract> AddressAbstract_sptr;

/** @brief output operator for abstract addresses */
template<class stream>
static stream&
operator<<( stream &out, const AddressAbstract &addr )
{
  out << addr._AbstractName;
  return (out);
}

} // namespace csm_network
}
#endif /* CSM_NETWORK_SRC_CPP_ADDRESS_H_ */
