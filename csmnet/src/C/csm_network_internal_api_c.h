/*================================================================================

    csmnet/src/C/csm_network_internal_api_c.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef __CSM_NETWORK_INTERNAL_API_C_H__
#define __CSM_NETWORK_INTERNAL_API_C_H__

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "csm_network_header.h"
#include "csm_network_msg_c.h"

#ifndef DGRAM_PAYLOAD_MAX
#define DGRAM_PAYLOAD_MAX ( 65535 )
#endif

typedef int (*csm_net_unix_CallBack)(csm_net_msg_t*);
int csmi_register_Callback( csm_net_unix_CallBack aCallBack);


typedef struct {
  char * _BufferedData;   ///< current pointer to start of next message to parse
  char * _DataEnd; ///< points to the end of the valid data in the buffer
  size_t _BufferedDataLen;   ///< remaining number of bytes in the buffer
  int _PartialMsg; ///< indicates whether a partial message is in the buffer and needs recv call to complete
  struct sockaddr_un _SrcAddr;   ///< source address of the origin of the data
} csm_dgram_buffer_state_t;

/* Abstract network API class */
typedef struct {
    int _IsServer;
    int _Socket;
    struct sockaddr_un _Addr;         // stores the local address that it's bound to

    struct sockaddr_un _OtherAddr; // for client sockets this holds the connected addr

    char * _DataBuffer;
    csm_dgram_buffer_state_t _BufferState;

} csm_net_unix_t;

typedef struct {
  csm_net_unix_t *_ep;
  csm_net_unix_t *_cb;
  pthread_t _cbthread;
  csm_net_unix_CallBack _cbfun;
  csm_net_unix_CallBack _on_disconnect;
  uint64_t _sys_msg_base;
  uint64_t _last_msgID;
  csm_net_msg_t *_heartbeat_msg; // a pre-allocated heartbeat message to avoid alloc and dealloc
  //int (*_cbfun)(csm_net_msg_t*);
  volatile int _cb_keep_running;
  volatile int _connected;
  int _std_fd[3]; // keep track of stdin/out/err
} csm_net_endpoint_t;

static inline
void csm_net_unix_setServer( csm_net_unix_t *aEP,
                const int aIsServer )
{ aEP->_IsServer= aIsServer; }

static inline
int csm_net_unix_IsServer( csm_net_unix_t *aEP ) { return aEP->_IsServer; }

/* Construct a network interface
 *  e.g. create and bind a server socket
 */
#ifdef __cplusplus
extern "C" {
#endif

csm_net_endpoint_t* csm_net_unix_Init( const int aIsServer );

int csm_net_unix_Exit( csm_net_endpoint_t *aEP );

int csm_net_unix_Connect( csm_net_endpoint_t *aEP,
                          const char* aSrvAddr );

ssize_t csm_net_unix_Send( csm_net_unix_t *aEP,
                          csm_net_msg_t *aMsg);

// allocates memory for the message, if return != NULL, user needs to free!
csm_net_msg_t * csm_net_unix_Recv( csm_net_unix_t *aEP );
csm_net_msg_t * csm_net_unix_BlockingRecv( csm_net_unix_t *aEP, csmi_cmd_t cmd );

void csm_net_unix_FlushBuffer( csm_net_unix_t *aEP );

// register a new callback function with this endpoint
int csm_net_unix_RegisterCallback( csm_net_endpoint_t *aEP,
                                   csm_net_unix_CallBack aCallBack);

// remove function from registered callbacks
int csm_net_unix_UnRegisterCallback( csm_net_endpoint_t *aEP);

#ifdef __cplusplus
}
#endif

#endif // __CSM_NETWORK_INTERNAL_API_C_H__
