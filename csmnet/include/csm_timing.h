/*================================================================================

    csmnet/include/csm_timing.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/**
 * @file csm_timing_internal_c.h
 * @brief Post-PRPQ EFIX: 11/13/2017. 
 *
 * This header is designed for the PRPQ efix solving internal git issues:  #775 #764
 * 
 * In this design the following assumptions are made:
 *
 * 1. An API with an extended timeout performs a multicast.
 * 2. An API with an extended timeout has two timeouts in the master state.
 * 3. There are 4 timouts: Client, Master, Aggregator, and Context Garbage Collection.
 *      a. The Client encapsultates up to two Master timeouts.
 *      b. The Master timeout is slightly longer than the Aggregator timeout.
 * 4. Context garabage collection should be longer than the longest timeout or at least 5 minutes.
 *      a. In the case of responses @ref CSM_NETWORK_ACK_TIMEOUT shall be used.
 *
 *  Timeout visualziation.
 *  ========= | // Client/Context timeout (CSM_EXTENDED_TIMEOUT_SECONDS)
 *  ---- ---- | // Master timeouts  (client / 2 ) - master_fudge_time 
 *  +++  +++  | // Aggregator timeouts ((client / 2 ) - master_fudge_time - agg_fudge_time
 */

#ifndef __CSM_TIMING_H__
#define __CSM_TIMING_H__

#include <string.h>
#include <stdio.h>
#include "csmi/src/common/include/csmi_cmds.h" // API command types

// MACRO DEFINES
// ================================================================

#define CSM_NETWORK_ACK_TIMEOUT ( 60 ) ///< ACK timeout in seconds.

// Extended timeout.
#define CSM_EXTENDED_TIMEOUT_SECONDS ( 120 ) // 2 Mins
#define CSM_EXTENDED_TIMEOUT_MILLISECONDS ( CSM_EXTENDED_TIMEOUT_SECONDS * 1000 )

// Default timeout.
#define CSM_RECV_TIMEOUT_GRANULARITY ( 5 ) // min and granularity
#define CSM_RECV_TIMEOUT_MIN ( CSM_RECV_TIMEOUT_GRANULARITY ) // shortest possible timeout
#define CSM_RECV_TIMEOUT_SECONDS ( CSM_RECV_TIMEOUT_GRANULARITY * 6 ) // 30 Secs
#define CSM_RECV_TIMEOUT_MILLISECONDS ( CSM_RECV_TIMEOUT_SECONDS * 1000 ) 
#define CSM_RECV_TIMEOUT_MILLI_HALF   ( CSM_RECV_TIMEOUT_MILLISECONDS / 2 )

// Timeout fudge time for client to master, subtractive.
#define CSM_CLIENT_TO_MASTER_FUDGE_SECONDS ( 1 ) // 1 Secs
#define CSM_CLIENT_TO_MASTER_FUDGE_MILLISECONDS ( CSM_CLIENT_TO_MASTER_FUDGE_SECONDS * 1000 )

// Timeout fudge time for master to aggregator, subtractive.
#define CSM_MASTER_TO_AGG_FUDGE_SECONDS ( 1 ) // 1 Secs
#define CSM_MASTER_TO_AGG_FUDGE_MILLISECONDS ( CSM_MASTER_TO_AGG_FUDGE_SECONDS * 1000 )

// Timeout fudge time for csm contexts, additive.
#define CSM_CONTEXT_FUDGE_SECONDS ( 5 ) // 5 Secs

// Minimm timeout time for context garbage collection.
#define CSM_CONTEXT_TIMEOUT_MIN_SECONDS ( 300 ) // 5 Mins

// Define a minimum Aggregator timeout time.
#define CSM_AGG_TIMEOUT_MIN_SECONDS ( 16 ) //
#define CSM_AGG_TIMEOUT_MIN_MILLISECONDS ( CSM_AGG_TIMEOUT_MIN_SECONDS * 1000 ) //

// ================================================================

// Functions
// ----------------------------------------------------------------
#ifdef __cplusplus
extern "C"
{
#endif

extern int csmi_cmd_timeouts[];

/** @brief Retrieves a timeout time in milliseconds for an api.
 *
 * @param[in] api_id The api id to request the api of.
 *
 * @return A timeout time in milliseconds.
 */
static inline
int csm_get_timeout( int api_id )
{
  if( api_id % CSM_CMD_INVALID == api_id )
    return csmi_cmd_timeouts[ api_id ] * 1000;
  else
    return CSM_RECV_TIMEOUT_MILLISECONDS;
}

/** @brief Updates the timeout values by parsing a ;-separated string of ints
 *
 * @param[in] data  The string/char* of ints
 * @param[in] len   The length of that string
 *
 * @return 0 on success, errno otherwise
 */
static inline
int csm_update_timeouts( const char *data,
                         const size_t len )
{
  if( data == NULL )
    return 0;
  char *pos = (char*)data;
  int secs;
  int api;
  while( sscanf( pos, "%d:%d;", &api, &secs ) == 2 )
  {
    if( secs > CSM_RECV_TIMEOUT_MIN )
      csmi_cmd_timeouts[ api ] = secs;

    pos = strchr( pos, ';' );
    if( pos == NULL )
      break;
    if( *pos == ';' )
      ++pos;
  }
  return 0;
}

/** @brief Determines the appropriate timeout time for a client.
 *
 *  @param[in] api    The API type the timeout is for. 
 *                      Should be of type @ref csmi_cmd_t, but int is accepted.
 *
 *  @return The length of the timeout, in milliseconds.
 */
#define csm_get_client_timeout( api ) \
    ( csm_get_timeout( api ) )


/** @brief Determines the appropriate timeout time for the master daemon.
 *
 *  @param[in] api    The API type the timeout is for. 
 *                      Should be of type @ref csmi_cmd_t, but int is accepted.
 *
 *  @return The length of the timeout, in milliseconds.
 */
#define csm_get_master_timeout( api ) \
    ( ( csm_get_timeout( api ) / 2 ) - CSM_CLIENT_TO_MASTER_FUDGE_MILLISECONDS )

/** @brief A simple max macro for timeouts, used to cleanup code.
 *
 * @param[in] timeoutA An integer timeout. 
 * @param[in] timeoutB An integer timeout.
 *
 * @return The largest value between @p timeoutA and @p timeoutB.
 */
#define csm_timeout_max( timeoutA,timeoutB )\
    timeoutA > timeoutB ? timeoutA : timeoutB

/** @brief Determines the appropriate timeout time for an aggregator.
 *
 *  @param[in] api    The API type the timeout is for. 
 *                      Should be of type @ref csmi_cmd_t, but int is accepted.
 *
 *  @return The length of the timeout, in milliseconds.
 */
#define csm_get_agg_timeout( api ) \
    ( csm_get_master_timeout( api ) - CSM_MASTER_TO_AGG_FUDGE_MILLISECONDS )

/** @brief Determines the appropriate timeout time for a context.
 *
 *  @param[in] api    The API type the timeout is for. 
 *                      Should be of type @ref csmi_cmd_t, but int is accepted.
 *  @param[in] isResp A flag indicating whether the context is for a response or not.
 *                      A response uses @ref CSM_NETWORK_ACK_TIMEOUT.
 *                      The greater value of either @ref csm_get_timeout or
 *                      @ref CSM_CONTEXT_TIMEOUT_MIN_SECONDS is used when not a response.
 *
 *  @return The length of the timeout, in seconds.
 */
#define csm_context_timeout( api, isResp )                                  \
    isResp ?                                                                \
        CSM_NETWORK_ACK_TIMEOUT + CSM_CONTEXT_FUDGE_SECONDS :               \
        csm_timeout_max(                                                    \
            ( csm_get_timeout( api ) / 1000 ) + CSM_CONTEXT_FUDGE_SECONDS,  \
            CSM_CONTEXT_TIMEOUT_MIN_SECONDS)

#ifdef __cplusplus
}
#endif
// ----------------------------------------------------------------


#endif

