/*================================================================================

    csmnet/include/csm_network_config.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef __CSM_NETWORK_CONFIG_H__
#define __CSM_NETWORK_CONFIG_H__

/** @ingroup csm_net
 *  @file csm_network_config.h
 *  @brief Definitions and default settings for the CSM network component
 *
 *  This contains the basic definitions and default settings for the network component
 *
 */

/** @defgroup csm_net  CSM network component
 *
 *  Everything that's related to transfer of data and requests between CSM
 *  daemons and/or clients
 *
 */


/** @def CSM_NETWORK_PROTOCOL_VERSION
 * @brief specification of protocol version
 *
 * Specifies the version of the network header format.
 * An inbound or outbound message is considered invalid
 * if the protocol version doesn't match. Will only be changed
 * after substantial changes in the header format that cause
 * the data exchange between new and old protocol to become
 * incompatible.
 */
#define CSM_NETWORK_PROTOCOL_VERSION ( (uint8_t)3 )


/** @def CSM_NETWORK_LOCAL_SSOCKET
 * @brief The default Unix socket path
 *
 * Specifies the default Unix socket path that the CSM daemon would create
 * to enable clients to connect and send requests into the CSM infrastructure
 */
#define CSM_NETWORK_LOCAL_SSOCKET "/run/csmd.sock"


/** @def CSM_NETWORK_LOCALCB_SSOCKET
 * @brief The default Unix socket path for callbacks
 *
 * Specifies the default Unix socket path that the CSM daemon would create.
 * It is used as a secondary link between client and daemon to send callback
 * messages and trigger registered functions at the client.
 */
#define CSM_NETWORK_LOCALCB_SSOCKET "/run/csmd.sock_cb"


/** @def CSM_NETWORK_LOCAL_CSOCKET
 * @brief client socket base name
 *
 * Defines the base name of the client-side Unix socket path.
 * It is only the prefix of the full name.
 * It has no actual effect on the file system because the client
 * uses abstract socket paths.
 */
#define CSM_NETWORK_LOCAL_CSOCKET "/tmp/csmi"


/** @def CSM_SECONDARY_SOCKET_APPEND
 * @brief Extension to derive the callback socket path
 *
 * The extension is appended to the full name of the Unix
 * socket path to create the secondary socket for callback
 * communication.
 */
#define CSM_SECONDARY_SOCKET_APPEND "_cb"


#ifndef UNIX_PATH_MAX

/** @def UNIX_PATH_MAX
 * @brief UNIX_PATH_MAX definition in case it's not defined
 *
 * On some systems, it wasn't defined. So if needed, we define it here.
 */
#define UNIX_PATH_MAX ( sizeof( ((struct sockaddr_un *)0x0)->sun_path ) )
#endif


#ifndef DGRAM_PAYLOAD_MAX

/** @def DGRAM_PAYLOAD_MAX
 * @brief Maximim payload that can be transferred
 *
 * Maximim payload to be transferred with one message transfer.
 *
 */
#define DGRAM_PAYLOAD_MAX ( 16777216 )
//#define DGRAM_PAYLOAD_MAX ( 1048576 )
#endif


/** @def CSM_CREDENTIAL_ID_UNKNOWN
 * @brief Placeholder ID for unknown/invalid credentials
 *
 * If userID or groupID are either unknown or need to be set invalid,
 * this value will be used.
 */
#define CSM_CREDENTIAL_ID_UNKNOWN ((uint32_t)0xFFFFFFFF)


/** @def CSM_INTERRUPT_RETRY
 * @brief Number of send or receive retries when interrupted
 *
 * Send/Recv calls in the CSM client library might get interrupted
 * by timers set by the user application. If this happens, the library
 * attempts this number of retries.
 */
#define CSM_INTERRUPT_RETRY ( 100 )


/** @def CSM_NETWORK_HEARTBEAT_INTERVAL
 * @brief Number of seconds between heartbeats if a connection
 * between 2 elements of the CSM infrastructure is idle
 */
#define CSM_NETWORK_HEARTBEAT_INTERVAL ( 15 )


#ifndef CSM_VERSION_ID
/** @def CSM_VERSION_ID
 * @brief Version identification
 *
 * Contains the version ID string for CSM to assure compatibility
 * between communication partners. If the build system is not
 * setting this ID, we set a default here.
 */
//#define CSM_VERSION_ID "unknown"
#endif

#endif // __CSM_NETWORK_CONFIG_H__
