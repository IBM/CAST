/*================================================================================

    csmnet/include/csm_network_header.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef __CSM_NETWORK_HEADER_H__
#define __CSM_NETWORK_HEADER_H__

#include <inttypes.h>

#include "csm_timing.h"
#include "csm_network_config.h"
#include "csmi/src/common/include/csmi_cmds.h"

/** @ingroup csm_net
 *  @file csm_network_header.h
 *  @brief network message definitions for @ref csm_net
 *
 *  This contains the definition of the network header that's
 *  part of every message between clients and servers as well
 *  as between servers and servers.
 *
 */

#define CSM_HEADER_ACK_BIT ( (uint8_t)0x01 ) ///< message is an ACK if set
#define CSM_HEADER_RESP_BIT ( (uint8_t)0x02 ) ///< message is a response if set
#define CSM_HEADER_ERR_BIT ( (uint8_t)0x04 )  ///< message is an error message if set
#define CSM_HEADER_CBK_BIT ( (uint8_t)0x08 ) ///< message is a callback message and will be sent to the callback socket
#define CSM_HEADER_INT_BIT ( (uint8_t)0x10 ) ///< message is an internal message between daemons
#define CSM_HEADER_MTC_BIT ( (uint8_t)0x20 ) ///< message is a multi-cast
#define CSM_HEADER_PCK_BIT ( (uint8_t)0x40 ) ///< message contains a request that requires a permission check by the API back-end

#define CSM_PRIORITY_NO_ACK ( (uint8_t)0x00 )  ///< priority for messages with no ACK requested
#define CSM_PRIORITY_WITH_ACK ( (uint8_t)0x01 )  ///< any message with higher priority above this requires an ACK
#define CSM_PRIORITY_RETAIN ( (uint8_t)0x02 )  ///< message will be retained (in mqtt) until first subscriber receives it!
#define CSM_PRIORITY_RETAIN_FOREVER ( (uint8_t)0x03 ) ///< persistent message (not in use)

#define CSM_NETWORK_MAX_PRIORITY ( (uint8_t)0x02 ) ///< max priority for message validation
#define CSM_NETWORK_PRIORITY_INVALID ( CSM_NETWORK_MAX_PRIORITY + 1 ) ///< anything larger or equal is invalid

#define CSM_PRIORITY_DEFAULT CSM_PRIORITY_WITH_ACK  ///< definition of the default message priority for convenience use

/** @def CSM_DISCONNECT_MSG
 * @brief String to be send between client and daemon to indicate a disconnect
 *
 * Disconnecting client/daemons are not noticed via Unix domain sockets. Therefore
 * either side sends a disconnect message when they go down.
 * Additionally to this message, there's a heartbeat protocol. However, the
 * disconnect message allows immediate notification of a communication endpoint
 * going down
 */
#define CSM_DISCONNECT_MSG "DISC0NNECT"
#define CSM_DISCONNECT_MSG_LEN 10 ///< Length of the @ref CSM_DISCONNECT_MSG

/** @def CSM_FAILOVER_MSG
 * @brief String to be sent between compute and aggregator to
 *  signal switch from secondary to primary connection
 */
#define CSM_FAILOVER_MSG "FAIL0VER"


/** @def CSM_RESTART_MSG
 * @brief String to be sent between compute and aggregator to
 * signal a restarted or "fresh from disconnect" state
 */
#define CSM_RESTART_MSG "R3ST4RT"


/** @def CSM_MSGID_BITS
 * @brief number of bits used for the sequence number component of the msgID
 */
#define CSM_MSGID_BITS ( 50 )

/** @def CSM_MSGID_MASK
 * @brief masks the range of bits that are usable for the sequence number component of the msgID
 * using 50 bit range, the remaining bits are reserved for a prefix to separate system/control
 * messsages from regular user requests
 */
#define CSM_MSGID_MASK (( 1ull << CSM_MSGID_BITS ) - 1)


/** @brief CSM network header specification
 *
 * Defines the network header for CSM messages.
 * It's organized in chunks of 64bit.
 *
 * message/command flags
 *
 *           name               | description
 *  ----------------------------|---------------
 *   @ref CSM_HEADER_ACK_BIT    | message is an acknowledgement
 *   @ref CSM_HEADER_RESP_BIT   | this is the response for a previous request
 *   @ref CSM_HEADER_ERR_BIT    | if set, the payload contains error info instead of regular data
 *   @ref CSM_HEADER_CBK_BIT    | if set, this message is marked as a callback message
 *   @ref CSM_HEADER_INT_BIT    | internal message type, e.g. sub-request to fulfill a CSMI call
 *   @ref CSM_HEADER_MTC_BIT    | multi-cast message indicator
 *   @ref CSM_HEADER_PCK_BIT    | private data check required for command
 *
 */
typedef struct csm_network_header {
// first 64bits:
  uint8_t _ProtocolVersion; ///< protocol/header version to prevent incompatible headers being accepted/processed

  uint8_t _CommandType;   ///< csmi api call or internal csm cmd number

  uint8_t _Flags;         ///< flags to identify the nature of the message
  uint8_t _Priority;      ///< message priority
  uint32_t _Reserved;     ///< Reserved space for CSM-internal use

// second 64bits:
  uint64_t _MessageID;    ///< message identifier to match requests and responses and ACKs

// third 64bits:
  uint32_t _DataLen;      ///< length of the payload data in bytes
  uint32_t _CheckSum;     ///< checksum of header and payload

// fourth 64bits:
  uint32_t _UserID;       ///< effective user ID of the original caller
  uint32_t _GroupID;      ///< effective group ID of the original caller
} csm_network_header_t;

#endif // __CSM_NETWORK_HEADER_H__
