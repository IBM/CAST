/*================================================================================

    csmnet/src/CPP/endpoint.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/** @file endpoint.h */

#ifndef CSM_NETWORK_SRC_CPP_ENDPOINT_H_
#define CSM_NETWORK_SRC_CPP_ENDPOINT_H_

#include <unistd.h>      // close()
#include <sys/socket.h>  // socket calls
#include <sys/un.h>      // struct sockaddr_un
#include <netinet/in.h>

#include <csm_network_config.h>
#include "address.h"
#include "csm_network_msg_cpp.h"
#include "csm_message_and_address.h"
#include "endpoint_buffer.h"
#include "endpoint_options.h"
#include "network_ctrl_path.h"
#include "csmi/src/common/include/csmi_serialization.h"

#include <atomic>
#include <memory>
#include <vector>

namespace csm {
namespace network {

/** \brief Available actions to trigger in a sync() call
 *
 */
typedef enum {
  SYNC_ACTION_READ,  ///< synchronize any read/recv operations
  SYNC_ACTION_WRITE, ///< synchronize any write/send operations
  SYNC_ACTION_MAINTENANCE,  ///< perform maintenance work (sync any connection states etc)
  SYNC_ACTION_ALL  ///< include all of the above
} SyncAction;


/** \class Endpoint
 * \brief Abstract base class for network endpoints
 */
class Endpoint {
protected:
  Address_sptr _LocalAddr;     ///< shared_ptr to the local address of this endpoint
  Address_sptr _RemoteAddr;    ///< shared_ptr to the remote address if endpoint is connected
  EndpointOptions_sptr _Options;   ///< endpoint-specific options/settings
  std::atomic<bool> _Connected;  ///< flag that shows whether endpoint is connected or not
  std::atomic<bool> _Verified; ///< flag that shows whether endpoint connection is version verified or not

  Endpoint()
  : _LocalAddr(nullptr),
    _RemoteAddr(nullptr),
    _Options(nullptr),
    _Connected(false),
    _Verified(false)
  {}
public:
  /** \brief construct unconnected endpoint from parameters
   *
   * \param [in]  i_LocalAddr   shared_ptr to local address
   * \param [in]  i_Options     shared_ptr to endpoint options
   *
   * \note this version without remote address creates an unconnected endpoint
   */
  Endpoint( const Address_sptr i_LocalAddr,
            const EndpointOptions_sptr i_Options )
  : _LocalAddr( (Address_sptr)i_LocalAddr ),
    _RemoteAddr( nullptr ),
    _Options( i_Options ),
    _Connected( false ),
    _Verified( false )
  {}
  /** \brief construct endpoint from existing endpoint ptr
   *
   * \param [in]  i_Endpoint    pointer to an existing endpoint
   */
  // todo: LS: will crash with nullptr
  Endpoint( const Endpoint * i_Endpoint )
  : _LocalAddr( i_Endpoint->GetLocalAddr() ),
    _RemoteAddr( i_Endpoint->GetRemoteAddr() ),
    _Options( i_Endpoint->GetOptions() ),
    _Connected( i_Endpoint->IsConnected() ),
    _Verified( i_Endpoint->IsVerified() )
  { }

  /** \brief destructor
   *
   */
  virtual ~Endpoint( ) { _RemoteAddr = nullptr; _LocalAddr = nullptr; _Options = nullptr; _Connected = 0; _Verified = 0; }

  /** \brief Retrieve a shared_ptr of the local address
   *
   * \return shared_ptr of local address
   */
  Address_sptr GetLocalAddr() const { return _LocalAddr; }
  /** \brief Set the local address
   *
   * \param [in] i_Addr    new address shared_ptr to set
   */
  void SetLocalAddr( Address_sptr i_Addr ) { _LocalAddr = i_Addr; }

  /** \brief Retrieve a shared_ptr of the remote address
   *
   * \return shared_ptr of the remote address
   */
  Address_sptr GetRemoteAddr() const { return _RemoteAddr; }
  /** \brief Set the remote address
   *
   * \param [in]  i_RemoteAddr    new remote address to set
   */
  void SetRemoteAddr( const Address_sptr i_RemoteAddr ) { _RemoteAddr = std::const_pointer_cast<Address>(i_RemoteAddr); }

  /** \brief Retrieve a shared_ptr to the options of this endpoint
   *
   * \return shared_ptr of the options
   */
  EndpointOptions_sptr GetOptions() const { return _Options; }
  /** \brief Set the endpoint options
   *
   * \param [in]   i_Options    new options to set
   */
  void SetOptions( const EndpointOptions_sptr i_Options ) { _Options = std::const_pointer_cast<EndpointOptions>(i_Options); }

  /** \brief Return connection status of this endpoint
   *
   * \return true if endpoint is connected, otherwise false
   */
  bool IsConnected() const { return _Connected; }

  /** \brief Return verification status of this endpoint
   *
   * \return true if endpoint's version has been verified
   */
  bool IsVerified() const { return _Verified; }

  /** \brief Set the verification status of this endpoint
   */
  void SetVerified() { _Verified = true; }

  /** \brief Return the address/endpoint type
   *
   * \return AddressType of the endpoint
   */
  csm::network::AddressType GetAddrType() const
  {
    if( _LocalAddr != nullptr )
      return _LocalAddr.get()->GetAddrType();
    if( _RemoteAddr != nullptr )
      return _RemoteAddr.get()->GetAddrType();
    return CSM_NETWORK_TYPE_UNKNOWN;
  }

  /** \brief Return whether this endpoint is a listening endpoint
   *
   * \return return true if endpoint is a server-side endpoint and is able to listen/accept
   */
  bool IsServerEndpoint() const { return _Options->_IsServer; };

  /** \brief Connects to a server to a server endpoint
   *
   * \param [in]  i_SrvAddr   shared_ptr of the server address
   *
   * \return int  0 on success, error code otherwise
   */
  virtual int Connect( const Address_sptr i_SrvAddr ) = 0;

  /** \brief Disconnect an endpoint
   *
   * \return  int  0 on success, error code otherwise
   *
   * \note currently, does nothing but setting member _Connected to false
   */
  virtual int Disconnect() { _Connected = false; return 0; }

  /** \brief Accept a new connection from a client
   *
   * \return  Endpoint*  Pointer to a new connected endpoint with the peer as remote address
   */
  virtual Endpoint* Accept( ) = 0;

  /** \brief Send a message to a remote address
   *
   * \param [in] i_Msg          Message to send
   * \param [in] i_RemoteAddr   peer address
   *
   * \return ssize_t  number of bytes sent, or negative error code
   *
   * \note  if used with connected endpoints, i_RemoteAddr has to match the peer-address
   */
  virtual ssize_t SendTo( const csm::network::Message &i_Msg,
                          const Address_sptr i_RemoteAddr ) = 0;

  /** \brief Send a message to the connected peer
   *
   * \param [in] i_Msg     Message to send
   *
   * \return ssize_t  number of bytes sent, or negative error code
   */
  virtual ssize_t Send( const csm::network::Message &i_Msg ) = 0;

  /** \brief Receive a message from any remote peer
   *
   * \param [out]  i_MsgAddr   received message and address of remote peer
   *
   * \return ssize_t  number of bytes received, throws exception on errors
   *
   * \note the address component of the output parameter will match
   *       the peer address of connected endpoints.  For unconnected
   *       endpoints, it will match the sender
   */
  virtual ssize_t RecvFrom( csm::network::MessageAndAddress &i_MsgAddr ) = 0;

  /** \brief Receive a message from connected peer
   *
   * \param [out]  i_Msg    received message
   *
   * \return ssize_t  number of bytes received, throws exception on errors
   */
  virtual ssize_t Recv( csm::network::Message &i_Msg ) = 0;

  /** \brief Determine whether any data is ready to be received
   *
   * \return bool  true if there's data available to be received
   */
  virtual bool DataPending() const = 0;

  /* data synchronization, e.g. flush any buffers */
  /** \brief  Synchronize/Maintenance of endpoint
   *
   * \param [in]  i_Sync   Which level/action of synchronization/maintenance to perform
   *
   * \return NetworkCtrlInfo*  a single linked list of NetworkCtrlInfo entries indicating
   *                           any errors or events detected during maintenance
   *                           (e.g. disconnects, timeouts, etc)
   */
  virtual NetworkCtrlInfo* Sync( const SyncAction i_Sync = SYNC_ACTION_ALL ) = 0;

  /** \brief Retrieve the socket number of an endpoint (if any)
   *
   * \return int socket number/file descriptor
   *
   */
  virtual int GetSocket() const = 0;
};

}  // namespace csm_network
}
#endif /* CSM_NETWORK_SRC_CPP_ENDPOINT_H_ */
