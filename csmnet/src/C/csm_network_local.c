/*================================================================================

    csmnet/src/C/csm_network_local.c

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <stdio.h>
#include <unistd.h>      // close()
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <sys/stat.h>    // stat()
#include <sys/types.h>
#include <sys/socket.h>  // socket calls
#include <sys/un.h>      // struct sockaddr_un
#include <sys/time.h>    // struct timeval, gettimeofday
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>

#include "csmutil/include/csmutil_logging.h"
#include "csmutil/include/csm_version.h"

#include "csmi/src/common/include/csmi_cmds.h"
#include "csm_timing.h"
#include "csm_network_config.h"
#include "csm_network_internal_api_c.h"

void * thread_callback_loop( void * aIn );

/* setting the default initial value for max transfer size
 * the actual value will be set via getsockopt()
 */
static size_t CSM_NETWORK_TRANSFER_MAX = 128 * 1024;

static inline
uint16_t getTimeStamp()
{
  struct timeval tp;
  gettimeofday( &tp, NULL );
  // use the last 12 bits of the seconds and bit 15-19 of the usecs to assemble a 16bit timestamp
  return (uint16_t)( (( tp.tv_sec & 0xFFF ) << 4) + ((tp.tv_usec >> 15) & 0xF) );
}

#define SQUISH_STDFDS
#ifdef SQUISH_STDFDS
int file_descriptor_check_and_fix( int fds[3] )
{
  int i;

  // first: check what we get when opening a file...
  int first = open("/dev/null", 0);
  if( first >= 0 )
    close( first );

  for( i = 0; i < 3; ++i )
    fds[ i ] = -1;

  // noone touched stdin/out/err, so, close and exit
  if( first > 2 )
    return 0;

  // check if we can open a replacement for stdin
  fds[0] = open("/dev/zero", O_RDONLY );
  if( fds[0] < 0 ) return errno;
  if( fds[0] != 0 )
  {
    close( fds[ 0 ] );
    fds[ 0 ] = -1;
  }

  // create a log file in /tmp
  char filename[ 64 ];
  bzero( filename, 64 );
//  sprintf( filename, "/tmp/libcsmi.%d.%d.log", getuid(), getpid() );
  sprintf( filename, "/dev/null" );

  // check if we can open a replacement for stdout
  fds[1] = open(filename, O_WRONLY | O_APPEND | O_CREAT, 0666 );
  if( fds[1] < 0 ) return errno;
  if( fds[1] != 1 )
  {
    close( fds[ 1 ] );
    fds[ 1 ] = -1;
  }

  // check if we can open a replacement for stderr
  fds[2] = open(filename, O_WRONLY | O_APPEND | O_CREAT, 0666 );
  if( fds[2] < 0 ) return errno;
  if( fds[2] != 2 )
  {
    close( fds[ 2 ] );
    fds[ 2 ] = -1;
  }

  return 0;
}
#else
int file_descriptor_check_and_fix( int fds[3] )
{
  int i;
  for( i = 0; i < 3; ++i )
    fds[ i ] = -1;

  return 0;
}
#endif

int csm_net_unix_ConfigureSocket( const int socket )
{
  // enable sockets to transmit user credentials
  int value = 1;
  int rc;

  errno = 0;
  rc = setsockopt( socket, SOL_SOCKET, SO_PASSCRED, &value, sizeof (value));
  if( rc )
  {
    perror("Setsockopt::SO_PASSCRED");
    csmutil_logging( error, "%s-%d: csm_net_unix_Init: socket %d: SO_PASSCRED setting failed rc=%d",
                     __FILE__, __LINE__, socket, errno );
    return -1;
  }

  struct timeval timeout;
  timeout.tv_sec = CSM_RECV_TIMEOUT_GRANULARITY;
  timeout.tv_usec = 0;
  errno = 0;
  rc = setsockopt( socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  if( rc )
  {
    perror("Setsockopt::SO_RCVTIMEO");
    csmutil_logging( error, "%s-%d: csm_net_unix_Init: socket %d: SO_RCVTIMEO setting failed rc=%d",
                     __FILE__, __LINE__, socket, errno );
    return -1;
  }

  rc = setsockopt( socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
  if( rc )
  {
    perror("Setsockopt::SO_SNDTIMEO");
    csmutil_logging( error, "%s-%d: csm_net_unix_Init: socket %d: SO_SNDTIMEO setting failed rc=%d",
                     __FILE__, __LINE__, socket, errno );
    return -1;
  }

  return 0;
}

static
void BufferStateReset( csm_net_unix_t *aEP )
{
  aEP->_BufferState._BufferedData = aEP->_DataBuffer;
  aEP->_BufferState._DataEnd = aEP->_DataBuffer;
  aEP->_BufferState._BufferedDataLen = 0;
  aEP->_BufferState._PartialMsg = 0;
}


/** @brief initializes a local endpoint
 *
 * creates 2 unix domain sockets (for default and callback)
 * removes any existing socket files and creates new files
 * if it's an active/client endpoint, the socket gets the PID as a suffix
 * this C-routine is intended for use with the client library, therefore the
 * server-side code only uses some hard-coded values for setup and is not
 * checking any configuration files or environment variables
 *
 * The callback socket, is suffixed with "_cb". There's a separate thread
 * spawned that's calling recv and just waits for any messages.
 * Incoming messages will call any registered functions in the context of that thread.
 *
 * @param aIsServer specify if this is a server/listen/passive socket or not
 *
 * @return pointer to the local endpoint structure
 * @return NULL if there are any errors (check errno for details)
 */
csm_net_endpoint_t * csm_net_unix_Init( const int aIsServer )
{
  int retry;
  int rc = 0;
  int std_fds[ 3 ];

  uint16_t timeStamp = getTimeStamp();

  // if a client has closed stdin/out/err, we need to make sure that we're not posting csmi output to the daemon
  if( (rc = file_descriptor_check_and_fix( std_fds )) != 0 )
  {
    errno = rc;
    return NULL;
  }

  // create the primary channel (regular msg)
  csm_net_unix_t *ep = (csm_net_unix_t*)malloc( sizeof( csm_net_unix_t ) );
  if( ep == NULL )
  {
    errno = ENOMEM;
    return NULL;
  }

  bzero( ep, sizeof( csm_net_unix_t ) );
  csm_net_unix_setServer( ep, aIsServer );

  // create the secondary channel (callback)
  csm_net_unix_t *cb = (csm_net_unix_t*)malloc( sizeof( csm_net_unix_t ) );
  if( cb == NULL )
  {
    free( ep );
    errno = ENOMEM;
    return NULL;

  }
  bzero( cb, sizeof( csm_net_unix_t ) );
  csm_net_unix_setServer( cb, aIsServer );

  if((posix_memalign( (void**)&(ep->_DataBuffer), getpagesize(), DGRAM_PAYLOAD_MAX ) != 0) ||
      (ep->_DataBuffer == NULL))
  {
    free( cb );
    free( ep );
    errno = ENOMEM;
    return NULL;
  }
  memset( ep->_DataBuffer, 0, DGRAM_PAYLOAD_MAX );

  ep->_BufferState._BufferedData = ep->_DataBuffer;
  ep->_BufferState._BufferedDataLen = 0;
  ep->_BufferState._DataEnd = ep->_BufferState._BufferedData;
  bzero( &(ep->_BufferState._SrcAddr), sizeof( struct sockaddr_un ) );
  bzero( &(ep->_OtherAddr), sizeof( struct sockaddr_un ) );


  if((posix_memalign( (void**)&(cb->_DataBuffer), getpagesize(), DGRAM_PAYLOAD_MAX ) != 0) ||
      (cb->_DataBuffer == NULL) )
  {
    free( ep->_DataBuffer );
    free( cb );
    free( ep );
    errno = ENOMEM;
    return NULL;
  }
  memset( cb->_DataBuffer, 0, DGRAM_PAYLOAD_MAX );

  cb->_BufferState._BufferedData = cb->_DataBuffer;
  cb->_BufferState._BufferedDataLen = 0;
  cb->_BufferState._DataEnd = cb->_BufferState._BufferedData;
  bzero( &(cb->_BufferState._SrcAddr), sizeof( struct sockaddr_un ) );
  bzero( &(cb->_OtherAddr), sizeof( struct sockaddr_un ) );

  // create socket
  ep->_Socket = socket(AF_UNIX, SOCK_DGRAM, 0);
  if( ep->_Socket < 0 )
  {
    free( ep->_DataBuffer );
    free( ep );
    free( cb->_DataBuffer );
    free( cb );
    
    csmutil_logging( error, "%s-%d: csm_net_unix_Init: ep->_Socket is 0", __FILE__, __LINE__);
    return NULL;
  }

  cb->_Socket = socket(AF_UNIX, SOCK_DGRAM, 0);
  if( cb->_Socket < 0 )
  {
    close( ep->_Socket );
    free( ep->_DataBuffer );
    free( ep );
    free( cb->_DataBuffer );
    free( cb );
    
    csmutil_logging( error, "%s-%d: csm_net_unix_Init: cb->_Socket is 0", __FILE__, __LINE__);
    return NULL;
  }

  // prepare address
  bzero( &ep->_Addr, sizeof( struct sockaddr_un ) );

  ep->_Addr.sun_family = AF_UNIX;
  cb->_Addr.sun_family = AF_UNIX;
  if( csm_net_unix_IsServer( ep ) )
  {
    strncpy( ep->_Addr.sun_path, CSM_NETWORK_LOCAL_SSOCKET, UNIX_PATH_MAX );
    ep->_Addr.sun_path[UNIX_PATH_MAX-1] = '\0';
    rc += unlink( CSM_NETWORK_LOCAL_SSOCKET );

    strncpy( cb->_Addr.sun_path, CSM_NETWORK_LOCALCB_SSOCKET, UNIX_PATH_MAX );
    cb->_Addr.sun_path[UNIX_PATH_MAX-1] = '\0';
    rc += unlink( CSM_NETWORK_LOCALCB_SSOCKET );
  }
  else
  {
    snprintf(ep->_Addr.sun_path, UNIX_PATH_MAX, "%s%d.%d.%d", CSM_NETWORK_LOCAL_CSOCKET, geteuid(), getpid(), timeStamp );
    ep->_Addr.sun_path[UNIX_PATH_MAX-1] = '\0';
    ep->_Addr.sun_path[0] = '\0';
    rc = 0;

    snprintf(cb->_Addr.sun_path, UNIX_PATH_MAX, "%s%d.%d.%d%s", CSM_NETWORK_LOCAL_CSOCKET, geteuid(), getpid(), timeStamp, CSM_SECONDARY_SOCKET_APPEND );
    cb->_Addr.sun_path[UNIX_PATH_MAX-1] = '\0';
    cb->_Addr.sun_path[0] = '\0';
    rc = 0;
  }
  if(( rc ) && ( errno != ENOENT ))
  {
    close( ep->_Socket );
    free( ep->_DataBuffer );
    free( ep );
    close( cb->_Socket );
    free( cb->_DataBuffer );
    free( cb );
    
    csmutil_logging( error, "%s-%d: csm_net_unix_Init: errno != ENOENT.", __FILE__, __LINE__);
    return NULL;
  }

  // enable sockets to transmit user credentials
  rc = csm_net_unix_ConfigureSocket( ep->_Socket );
  if( rc < 0 )
  {
    close( ep->_Socket );
    free( ep->_DataBuffer );
    free( ep );
    close( cb->_Socket );
    free( cb->_DataBuffer );
    free( cb );
    return NULL;
  }

  rc = csm_net_unix_ConfigureSocket( cb->_Socket );
  if( rc < 0 )
  {
    close( ep->_Socket );
    free( ep->_DataBuffer );
    free( ep );
    close( cb->_Socket );
    free( cb->_DataBuffer );
    free( cb );
    return NULL;
  }

  BufferStateReset( cb );
  BufferStateReset( ep );

  int value = 0;
  socklen_t vallen = sizeof( value );
  if (getsockopt( ep->_Socket, SOL_SOCKET, SO_SNDBUF, &value, &vallen) < 0)
  {
    value = 128 * 1024;
    csmutil_logging( warning, "Getting SO_SNDBUF failed, errno=%d; will use conservative size=%d", errno, value );
  }

  CSM_NETWORK_TRANSFER_MAX = value - CSM_UNIX_CREDENTIAL_LENGTH - sizeof( csm_network_header_t );

  // bind
  for( retry = CSM_INTERRUPT_RETRY; retry > 0; --retry )
  {
    errno = 0;
    rc = bind( ep->_Socket, (struct sockaddr *)&(ep->_Addr), sizeof( ep->_Addr ) );
    if( errno == EINTR )
    {
      csmutil_logging( warning, "bind syscall interrupted. Will retry." );
      continue;
    }
    else
      break;
  }
  if( rc )
  {
    perror("Socket.bind(): ");
    close( ep->_Socket );
    free( ep->_DataBuffer );
    free( ep );
    close( cb->_Socket );
    free( cb->_DataBuffer );
    free( cb );
    return NULL;
  }

  for( retry = CSM_INTERRUPT_RETRY; retry > 0; --retry )
  {
    errno = 0;
    rc = bind( cb->_Socket, (struct sockaddr *)&(cb->_Addr), sizeof( cb->_Addr ) );
    if( errno == EINTR )
    {
      csmutil_logging( warning, "bind syscall interrupted. Will retry." );
      continue;
    }
    else
      break;
  }
  if( rc )
  {
    perror("CB-Socket.Bind(): ");

    close( ep->_Socket );
    close( cb->_Socket );
    free( ep->_DataBuffer );
    free( ep );
    free( cb->_DataBuffer );
    free( cb );
    return NULL;
  }


  csm_net_endpoint_t *ret = (csm_net_endpoint_t*)malloc( sizeof ( csm_net_endpoint_t ) );
  if( ! ret )
  {
    close( ep->_Socket );
    close( cb->_Socket );
    free( ep->_DataBuffer );
    free( ep );
    free( cb->_DataBuffer );
    free( cb );
    errno = ENOMEM;
    return NULL;
  }

  ret->_sys_msg_base = ((uint64_t)random() + 1 ) << CSM_MSGID_BITS;
  ret->_last_msgID = (uint64_t)random();
  ret->_heartbeat_msg = csm_net_msg_Init( CSM_CMD_STATUS,
                                          CSM_HEADER_INT_BIT,
                                          CSM_PRIORITY_NO_ACK,
                                          ret->_sys_msg_base | ret->_last_msgID++,
                                          geteuid(), getegid(), "", 0, 0 );

  ret->_cb = cb;
  ret->_ep = ep;
  ret->_cbfun = NULL;
  ret->_cb_keep_running = 1;
  ret->_connected = 0;
  ret->_std_fd[ 0 ] = std_fds[ 0 ];
  ret->_std_fd[ 1 ] = std_fds[ 1 ];
  ret->_std_fd[ 2 ] = std_fds[ 2 ];
  rc = pthread_create( &ret->_cbthread, NULL, thread_callback_loop, (void*)ret );

  if( rc )
  {
    ret->_cbthread = 0;
  }
  return ret;
}

/** @brief destruction of the local endpoint
 *
 * Closes and cleans up the local endpoint structures, state, and files
 * Tells the callback thread to exit (or forces it to exit otherwise)
 *
 * @param aEP  pointer to the endpoint to destroy
 *
 * @return 0 if everything is successful
 * @return != 0 otherwise
 */
int csm_net_unix_Exit( csm_net_endpoint_t *aEP )
{
  int rc = 0;
  if( ! aEP ) return 0;

  if( aEP->_connected )
  {
    aEP->_connected = 0;
    char *msgData = strdup(CSM_DISCONNECT_MSG);
    csm_net_msg_t *DisconnectMsg;
    DisconnectMsg = csm_net_msg_Init( CSM_CMD_STATUS,
                                      CSM_HEADER_INT_BIT,
                                      CSM_PRIORITY_WITH_ACK,
                                      aEP->_sys_msg_base | aEP->_last_msgID++,
                                      geteuid(), getegid(),
                                      msgData, strnlen(msgData, 64),
                                      0);

    if( DisconnectMsg )
    {
      csmutil_logging(debug, "%s-%d: Sending disconnect message", __FILE__, __LINE__);
      rc = csm_net_unix_Send( aEP->_ep, DisconnectMsg );
      if( rc < 0 )
      {
        csmutil_logging(error, "%s-%d: Failed to send disconnect message. errno=%d", __FILE__, __LINE__, errno);
        rc = 1;
      }
      else
        rc = 0;

      free( DisconnectMsg );
    }
    free( msgData );
  }

  // cleanup CallBack socket
  if( aEP->_cb )
  {
    if(( aEP->_cbthread != 0 ) && ( pthread_self() != aEP->_cbthread ))
    {
      aEP->_cb_keep_running = 0;
      usleep(500);

      void * thres;
      rc = pthread_cancel(aEP->_cbthread );
      if( rc  && ( rc != ESRCH ) )
        perror("Thread cancellation: ");

      rc = pthread_join( aEP->_cbthread, &thres );
      if( rc )
        perror("Thread join: ");

      if (thres == PTHREAD_CANCELED)
        csmutil_logging( debug, "thread exited by cancellation (socket: %s)", aEP->_cb->_Addr.sun_path);
      else
        csmutil_logging( debug, "thread exited on its own (socket: %s)", aEP->_cb->_Addr.sun_path);
    }

    if( aEP->_cb->_Socket >= 0 )
    {
      close( aEP->_cb->_Socket );
      aEP->_cb->_Socket = -1;
    }

    BufferStateReset( aEP->_cb );
    if( aEP->_cb->_DataBuffer )
    {
      memset( aEP->_cb->_DataBuffer, 0, DGRAM_PAYLOAD_MAX );
      free( aEP->_cb->_DataBuffer );
    }

    memset( aEP->_cb, 0, sizeof( csm_net_unix_t ) );
    free( aEP->_cb );
  }

  // cleanup Regular socket
  if( aEP->_ep ) {
    if( aEP->_ep->_Socket >= 0 )
    {
      close( aEP->_ep->_Socket );
      aEP->_ep->_Socket = -1;
    }

    BufferStateReset( aEP->_ep );
    if( aEP->_ep->_DataBuffer )
    {
      memset( aEP->_ep->_DataBuffer, 0, DGRAM_PAYLOAD_MAX );
      free( aEP->_ep->_DataBuffer );
    }

    memset( aEP->_ep, 0, sizeof( csm_net_unix_t ) );
    free( aEP->_ep );
  }

  memset( aEP->_heartbeat_msg, 0, sizeof( csm_net_msg_t ) );
  free( aEP->_heartbeat_msg );

  int i;
  for( i = 0; i < 3; ++i )
    if( aEP->_std_fd[ i ] != -1 )
      close( aEP->_std_fd[ i ] );

  memset( aEP, 0, sizeof( csm_net_endpoint_t ) );
  free( aEP );
  return rc;
}

int csm_net_unix_SocketConnect( csm_net_unix_t *aEP,
                                const char* aSrvAddr )
{
  int rc;
  int retries;
  struct sockaddr_un SrvAddr;

  bzero( &SrvAddr, sizeof( struct sockaddr_un ) );
  SrvAddr.sun_family = AF_UNIX;
  strncpy( SrvAddr.sun_path, aSrvAddr, UNIX_PATH_MAX );
  SrvAddr.sun_path[ UNIX_PATH_MAX - 1 ] = '\0';

  rc = 1;
  for( retries = 1; ((rc != 0) && (retries < 10)); ++retries )
  {
    rc = connect( aEP->_Socket, (struct sockaddr *)&SrvAddr, sizeof(SrvAddr) );
    if( rc )
    {
      rc = errno;
      csmutil_logging( debug, "csmlib: Failed to connect to CSM_SSOCKET=%s; retrying %d of %d after %dus.",
                       SrvAddr.sun_path, retries, 9, retries * 250000);
      usleep( retries * 250000 );
    }
  }
  if( rc == 0 )
  {
    aEP->_OtherAddr.sun_family = AF_UNIX;
    strncpy( aEP->_OtherAddr.sun_path, aSrvAddr, UNIX_PATH_MAX );
    aEP->_OtherAddr.sun_path[ UNIX_PATH_MAX - 1 ] = '\0';
  }
  else
    csmutil_logging( error, "csmlib: Failed to connect to CSM_SSOCKET=%s; retried %d times.",
                     SrvAddr.sun_path, 9);

  return rc;
}


/** @brief connects a local endpoint to a local server endpoint
 *
 * Creates a connection-like status of the local endpoint, so that
 * send and recv don't have to provide a remote address explicitly
 *
 * @param aEP       pointer to the endpoint that's being connected
 * @param aSrcAddr  unix address/path of server endpoint
 *
 * @return 0 if everything is successful
 * @return errno otherwise
 */
int csm_net_unix_Connect( csm_net_endpoint_t *aEP,
                          const char* aSrvAddr )
{
  int rc = 0;

  if(( aEP == NULL ) || (aEP->_ep == NULL) || (aEP->_cb == NULL))
    return EINVAL;

  if( aEP->_ep->_Socket && ! csm_net_unix_IsServer( aEP->_ep ) )
  {
    rc = csm_net_unix_SocketConnect( aEP->_ep, aSrvAddr );
    if( rc == 0 )
    {
      // don't attempt to actually connect the cb socket for at least 2 reasons:
      // 1) the daemon won't be able to send from the regular socket to this one (gets EPERM)
      // 2) we're not sending anything back from the cb socket anyway
      aEP->_cb->_OtherAddr.sun_family = AF_UNIX;
      strncpy( aEP->_cb->_OtherAddr.sun_path, aEP->_ep->_OtherAddr.sun_path, UNIX_PATH_MAX );    }
      aEP->_cb->_OtherAddr.sun_path[ UNIX_PATH_MAX - 1 ] = '\0';
  }
  if( rc )
    return rc;

  char *versiondata = strdup(CSM_VERSION);
  csm_net_msg_t *VersionMsg;
  VersionMsg = csm_net_msg_Init( CSM_CMD_STATUS,
                                 CSM_HEADER_INT_BIT,
                                 CSM_PRIORITY_NO_ACK,
                                 aEP->_sys_msg_base | aEP->_last_msgID++,
                                 geteuid(), getegid(),
                                 versiondata, strnlen(versiondata, 64),
                                 0);

  if( !VersionMsg )
  {
    return errno;
  }

  csmutil_logging(debug, "%s-%d: Sending connection message", __FILE__, __LINE__);
  rc = csm_net_unix_Send( aEP->_ep, VersionMsg );
  if(( rc < (int)strnlen(CSM_VERSION, 64) ) || ( rc == -1))
  {
    csmutil_logging(error, "%s-%d: Failed to send connection message. errno=%d", __FILE__, __LINE__, errno);
    free( VersionMsg );
    return EACCES;
  }
  else
    rc = 0;

  free( VersionMsg );
  VersionMsg = csm_net_unix_BlockingRecv( aEP->_ep, CSM_CMD_STATUS);
  if( VersionMsg == NULL )
  {
    rc = errno;
    csmutil_logging( error, "%s-%d: Client-Daemon connection error. errno=%d", __FILE__, __LINE__, rc );
  }
  else if( csm_net_msg_GetErrorFlag( VersionMsg ) )
  {
    errno = EPROTO;
    rc = errno;
    csmutil_logging(error,"%s-%d: Version mismatch rc=%d...", __FILE__, __LINE__, rc);
  }
  free(versiondata);

  if( VersionMsg != NULL )
  {
    csm_update_timeouts( csm_net_msg_GetData( VersionMsg ),
                         csm_net_msg_GetDataLen( VersionMsg ) );
    bzero( VersionMsg, sizeof( csm_net_msg_t));
  }
  if( rc == 0 )
    aEP->_connected = 1;
  csmutil_logging(debug,"%s-%d: Completing connect with rc=%d...", __FILE__, __LINE__, rc);
  return rc;
}

static
csm_net_msg_t * csm_net_unix_RecvMain(
    csm_net_unix_t *aEP,
    int aBlocking,
    csmi_cmd_t cmd,
    int aReturnCtrlData );


/** @brief send a message from this endpoint to a connected endpoint
 *
 * Sends a formatted message from the given endpoint to the connected
 * endpoint. Requires the given endpoint to be connected and the message
 * to be valid.
 * Function also acquires user credentials and sends it as control data
 * via sendmsg()
 *
 * @param aEP   pointer to the endpoint
 * @param aMsg  pointer to the message to send
 *
 * @return number of bytes sent (excluding control data)
 * @return -1 on error (with errno set accordingly)
 */
ssize_t csm_net_unix_Send( csm_net_unix_t *aEP,
                          csm_net_msg_t *aMsg)
{
  ssize_t rc = 0;

  if( !aMsg )
  {
    errno = EBADR;
    return -1;
  }
  if( aEP->_Socket < 2 )
  {
    errno = ENOTCONN;
    return -1;
  }
  if( ! csm_header_validate( &(aMsg->_Header ) ) )
  {
    errno = EBADMSG;
    return -1;
  }

  if( csm_net_msg_GetDataLen( aMsg ) > DGRAM_PAYLOAD_MAX )
  {
    errno = EFBIG;
    return -1;
  }

  struct iovec iov[2];
  iov[0].iov_base = csm_net_msg_GetHeaderBuffer( aMsg );
  iov[0].iov_len = sizeof( csm_network_header_t );
  iov[1].iov_base = (void*)csm_net_msg_GetData( aMsg );
  iov[1].iov_len = csm_net_msg_GetDataLen( aMsg );

  if( ( iov[0].iov_base == NULL ) ||
      ((iov[1].iov_len > 0) && (iov[1].iov_base == NULL)) )
  {
    errno = ENODATA;
    return -1;
  }

  size_t totallen = iov[1].iov_len;
  char *next_ptr = (char*)iov[1].iov_base;
  if( totallen > CSM_NETWORK_TRANSFER_MAX )
  {
    iov[1].iov_len = CSM_NETWORK_TRANSFER_MAX;
    next_ptr += iov[1].iov_len;
  }

  char ctrl_buf[ 128 ];
  memset( ctrl_buf, 0, 128 );
  struct msghdr msg;
  memset( &msg, 0, sizeof( struct msghdr ) );
  msg.msg_name = NULL; // (void*)(&aEP->_Addr);
  msg.msg_namelen = 0; //sizeof( struct sockaddr_un );
  msg.msg_iov = iov;
  msg.msg_iovlen = ( iov[1].iov_len == 0 ) ? 1 : 2 ;
  msg.msg_control = ctrl_buf;
  msg.msg_controllen = 128;
  msg.msg_flags = 0;

  struct cmsghdr *cmsg = CMSG_FIRSTHDR( &msg );
  if( cmsg == NULL )
  {
    errno = ENOBUFS;
    csmutil_logging( error, "%s-%d: csm_net_unix_Send(): BUG: insufficient buffer space for control msg.", __FILE__, __LINE__ );
    return -1;
  }

  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_CREDENTIALS;
  cmsg->cmsg_len = CMSG_LEN( sizeof( struct ucred ) );

  struct ucred credentials;
  memset( &credentials, 0, sizeof( struct ucred ) );
  credentials.gid = getegid();
  credentials.uid = geteuid();
  credentials.pid = getpid();

  struct ucred *cptr = (struct ucred*)CMSG_DATA( cmsg );
  memcpy( cptr, &credentials, sizeof( struct ucred ) );
  msg.msg_controllen = CMSG_SPACE( sizeof( struct ucred ) );

  if( ! csm_net_unix_IsServer( aEP ) )
  {
    csmutil_logging(trace, "Socket: %d sending %" PRIu32" @buffer: %p to %s",
                    aEP->_Socket, csm_net_msg_GetDataLen( aMsg ), csm_net_msg_GetHeaderBuffer( aMsg ), aEP->_OtherAddr.sun_path );
    int retry;
    for( retry=CSM_INTERRUPT_RETRY; (retry>0); --retry )
    {
      errno = 0;
      rc = sendmsg( aEP->_Socket, &msg, 0 );
      if( errno == EINTR )
      {
        csmutil_logging( warning, "sendmsg syscall interrupted. Will retry." );
        if(rc > 0)
        {
          csmutil_logging( warning, "PARTIAL MESSAGE SENT DUE TO EINTR." );
          break; // after a partial msg sent, lets see if the large-msg protocol is able to complete the send
        }
        continue;
      }
      else
        break;
    }

    // todo: right now we fail if we can't even send the header
    if( rc < iov[0].iov_len )
    {
      csmutil_logging( error, "failed to send the msg header" );
      return -ENODATA;
    }

    // account for the header size because totallen excludes it, but rc includes it
    ssize_t remaining = totallen - rc + iov[0].iov_len;
    while(( remaining > 0 ) && ( rc > 0 ))
    {
      struct msghdr part;
      struct iovec iov;
      ssize_t tmp_rc;
      part.msg_control = ctrl_buf;
      part.msg_controllen = CSM_UNIX_CREDENTIAL_LENGTH;
      part.msg_flags = msg.msg_flags;
      part.msg_name = msg.msg_name;
      part.msg_namelen = msg.msg_namelen;
      part.msg_iovlen = 1;
      iov.iov_base = next_ptr;
      iov.iov_len = ( (size_t)remaining < CSM_NETWORK_TRANSFER_MAX ) ? (size_t)remaining : CSM_NETWORK_TRANSFER_MAX;
      part.msg_iov = &iov;

      csmutil_logging( debug ,"Unix::SendMsg: Partial total_data=%ld; this part=%ld; remaining=%ld",
                       totallen, part.msg_iov[0].iov_len, remaining - part.msg_iov[0].iov_len );

      for( retry=CSM_INTERRUPT_RETRY; (retry>0); --retry )
      {
        errno = 0;
        tmp_rc = sendmsg( aEP->_Socket, &part, 0 );
        if( errno == EINTR )
        {
          csmutil_logging( warning, "sendmsg syscall interrupted. Will retry." );
          if(tmp_rc > 0)
          {
            csmutil_logging( warning, "PARTIAL MESSAGE SENT DUE TO EINTR." );
            break;
          }
          continue;
        }
        else
        {
          rc = tmp_rc;
          break;
        }
      }
      if( tmp_rc > 0 )
      {
        rc += tmp_rc;
        remaining -= tmp_rc;
        next_ptr += tmp_rc;
      }
    }
  }
  else
  {
    // server in connection-less mode cannot just do send without remote address
    errno = EINVAL;
    return -1;
  }

  if( rc < 0 )
  {
    rc = errno;
    csmutil_logging( error, "Failed to send msg. rc=%d", errno );
    return -1;
  }

  // wait for an ACK if the message priority requires it
  // todo: the send will be considered failed if the ACK times out
  if( csm_net_msg_GetPriority( aMsg ) >= CSM_PRIORITY_WITH_ACK )
  {
    csmutil_logging( trace ,"Unix::SendMsg id=%ld: Waiting for ACK", csm_net_msg_GetMessageID( aMsg ) );

    csm_net_msg_t *ackmsg = NULL;

    struct timeval deadline, now;
    gettimeofday( &now, NULL );
    deadline.tv_sec = now.tv_sec + csm_get_client_timeout( csm_net_msg_GetCommandType( aMsg ) ) + 1;

    while( ackmsg == NULL )
    {
      ackmsg = csm_net_unix_RecvMain( aEP, 1, csm_net_msg_GetCommandType( aMsg ), 1 );
      int stored_errno = errno;
      if( ackmsg == NULL )
      {
        csmutil_logging( warning, "%s-%d: ACK Receive failed? cmd=%s rc=%d",
                         __FILE__, __LINE__, csmi_cmds_t_strs[ csm_net_msg_GetCommandType( ackmsg ) ], stored_errno );
        return -stored_errno;
      }

      gettimeofday( &now, NULL );
      if( deadline.tv_sec < now.tv_sec )
        return -ETIMEDOUT;

      if( csm_net_msg_GetMessageID( ackmsg ) != csm_net_msg_GetMessageID( aMsg ) )
      {
        csmutil_logging( warning, "%s-%d: Received old msg. Previous timeout? %ld vs. %ld, cmd=%s",
                         __FILE__, __LINE__, csm_net_msg_GetMessageID( ackmsg ), csm_net_msg_GetMessageID( aMsg ),
                         csmi_cmds_t_strs[ csm_net_msg_GetCommandType( ackmsg ) ] );
        ackmsg = NULL;
        continue;
      }

      if( !csm_net_msg_GetAck( ackmsg ) )
      {
        csmutil_logging( error, "%s-%d: EXPECTED ACK MESSAGE. Got msg with flags=0x%x, cmd=%d",
                         __FILE__, __LINE__,
                         csm_net_msg_GetFlags( ackmsg ), csm_net_msg_GetCommandType( ackmsg ) );
        errno = EPROTO;
        return -1;
      }
    }
  }
  return rc;
}

#define MIN( a, b ) ( (a)<(b)?(a):(b))

static
ssize_t FillReceiveBuffer( csm_net_unix_t *aEP, csmi_cmd_t cmd, const int partial )
{
    ssize_t rlen = 0;

    char *rbuf = aEP->_BufferState._DataEnd;
    char *data = rbuf + sizeof( csm_net_msg_t );

    struct iovec iov[2];
    struct msghdr msg;
    char cbuffer[ 2 * CMSG_SPACE( sizeof( struct ucred ) ) ];
    memset( cbuffer, 0, 2 * CMSG_SPACE( sizeof( struct ucred ) ) );

    iov[0].iov_base = rbuf;
    iov[0].iov_len = sizeof( csm_network_header_t );
    iov[1].iov_base = data;
    iov[1].iov_len = (DGRAM_PAYLOAD_MAX - (aEP->_BufferState._DataEnd - aEP->_DataBuffer));

    memset( &msg, 0, sizeof( struct msghdr ) );
    msg.msg_iov = iov;
    msg.msg_iovlen = 2;
    msg.msg_control = (void*)cbuffer;
    msg.msg_controllen = 2 * CMSG_SPACE( sizeof( struct ucred ) );

    // readjust the recv bufs if this is a continuation of a partial msg recv
    if( partial != 0 )
    {
      msg.msg_iovlen = 1;
      iov[0].iov_len = (DGRAM_PAYLOAD_MAX - (aEP->_BufferState._DataEnd - aEP->_DataBuffer));
      data = rbuf;
    }

    int retry;
    int stored_errno;

    int timeout_loops = csm_get_client_timeout(cmd) / (CSM_RECV_TIMEOUT_GRANULARITY * 1000) + 1;
    csmutil_logging( trace, "timeout (%s): %d; loops %d", csmi_cmds_t_strs[ cmd ], csm_get_timeout( cmd ), timeout_loops );
    for( retry = CSM_INTERRUPT_RETRY; retry > 0; --retry )
    {
        errno = 0;
        rlen = recvmsg( aEP->_Socket, &msg, 0 );
        stored_errno = errno;

        switch( stored_errno )
        {
            case EINTR:
            {
                csmutil_logging( debug, "recvmsg syscall interrupted. Will retry." );
                if(rlen > 0)
                {
                    csmutil_logging( critical, "TRUNCATED MESSAGE DUE TO EINTR."
                        " PROTOCOL NOT READY FOR THIS YET." );
                    errno = EPROTONOSUPPORT;
                    retry = 0;
                }
                break;
            }
            case EAGAIN:
            {
                // Verify that the API hasn't fully timed out. IF it hasn't back off
                // the retry decrement and attempt to receive the message again.
                // Otherwise zero the retry count and issue an error.
                csmutil_logging( trace, "recvmsg syscall timed out. Will retry. %d", timeout_loops );
                if( --timeout_loops > 0 )  
                {
                    ++retry; 
                    break;
                }

                csmutil_logging( error, "recvmsg timed out. rc=%d", rlen );
                retry = 0;
                break;
            }
            default:
                retry = 0;
                break;
        }
    };

    switch( msg.msg_flags )
    {
        case MSG_TRUNC:
            csmutil_logging( critical, "TRUNCATED INCOMING DATA!!!"
                " Part of data is lost!!! rlen=%d", rlen);
            errno = ENOBUFS;
            rlen = -1;
            break;
        case MSG_CTRUNC:
            csmutil_logging( critical, "TRUNCATED INCOMING CONTROL-DATA!!!"
                " Msg Control data is lost!!!");
            errno = ENOBUFS;
            rlen = -1;
            break;
        case MSG_OOB:
        case MSG_ERRQUEUE:
            csmutil_logging( critical, "OOB or ERROR DATA!!!"
                " TODO: need proper handling of that situation!!!");
        case MSG_EOR:
        default:
            break;
    }

    if( rlen >= 0 )
    {
      size_t newdata_count = (size_t)rlen;
      // accommodate the skipped space of the header vs. msg type
      if( (( partial == 0 ) || ( aEP->_BufferState._BufferedDataLen < sizeof( csm_network_header_t ) )) &&
          ( (size_t)rlen >= sizeof( csm_network_header_t ) ))
        newdata_count += (sizeof( csm_net_msg_t ) - sizeof( csm_network_header_t ));

      aEP->_BufferState._DataEnd += newdata_count;
      aEP->_BufferState._BufferedDataLen += newdata_count;
    }
    else
    {
      // reset only in error cases that
      if(( stored_errno != EAGAIN ) && ( stored_errno != EINTR ))
      {
        BufferStateReset( aEP );
        rlen = 0;
      }
    }
    return rlen;
}

static
csm_net_msg_t * csm_net_unix_RecvMain(
    csm_net_unix_t *aEP, 
    int aBlocking, 
    csmi_cmd_t cmd,
    int aReturnCtrlData )
{
    // input sanity check
    if( aEP == NULL )
    {
        errno = EINVAL;
        return NULL;
    }
    if( aEP->_Socket < 2 )
    {
      errno = ENOTCONN;
      return NULL;
    }
    if( aEP->_DataBuffer == NULL )
    {
        errno = ENOMEM;
        return NULL;
    }

    // sanity check of buffer status of EP
    csm_dgram_buffer_state_t *EPBS = &( aEP->_BufferState );
    if( ( EPBS->_BufferedData < aEP->_DataBuffer ) ||
        ( EPBS->_DataEnd < EPBS->_BufferedData ) ||
        ( EPBS->_BufferedDataLen >= DGRAM_PAYLOAD_MAX ) ||
        (   ( EPBS->_BufferedDataLen < DGRAM_PAYLOAD_MAX ) &&
            ( aEP->_BufferState._BufferedData > aEP->_DataBuffer + DGRAM_PAYLOAD_MAX - aEP->_BufferState._BufferedDataLen)
        ) )
    {
      errno = EOVERFLOW;
      return NULL;
    }

    // Server requires recvfrom to be called because it's not connected and thus doesn't have a remote address
    errno = 0;
    if( csm_net_unix_IsServer( aEP ) )
    {
        errno = EINVAL;
        return NULL;
    }

    // extract message, lengths, and buffer pointers from EP buffer status
    if( EPBS->_BufferedData == EPBS->_DataEnd )
      BufferStateReset( aEP );

    csm_net_msg_t * ret = (csm_net_msg_t*)EPBS->_BufferedData;
    char * data = (char*) EPBS->_BufferedData + sizeof(csm_net_msg_t);
    ssize_t rlen = 0;

    do
    {
        // check if we have any data in buffer already (received ACK + message, or ACK + Heartbeat, etc.)
        // otherwise, go try and receive some data
        if(( EPBS->_PartialMsg != 0 ) || ( EPBS->_BufferedData == aEP->_DataBuffer ))
        {
          char *contbuf = EPBS->_DataEnd;
          rlen = FillReceiveBuffer( aEP, cmd, EPBS->_PartialMsg );

          // when returning from recv, lets check if we're still connected
          if( aEP->_Socket == 0 )
          {
            csmutil_logging( warning, "Socket got shot down while in recv()" );
            errno = ENOTCONN;
            return NULL;
          }

          // update the actual message and data pointer
          ret = (csm_net_msg_t*)EPBS->_BufferedData;
          data = (char*) EPBS->_BufferedData + sizeof(csm_net_msg_t);
          if( EPBS->_PartialMsg != 0 )
            csmutil_logging( trace, "previousmsg: %15.15s____overlap: %15.15s___cont data starts with:%15.15s", contbuf - 15, contbuf - 8, contbuf );
        }
        else
        {
          ret = (csm_net_msg_t*)EPBS->_BufferedData;
          data = (char*) EPBS->_BufferedData + sizeof(csm_net_msg_t);
        }
        if( ret == NULL )
        {
          csmutil_logging( error, "Fatal protocol error. Endpoint Databuffer no longer initialized?\n");
          errno = ENOTCONN;
          return NULL;
        }
        ret->_Data = data;

        // any errors or nothing received?
        if( rlen < 0 )
        {
            csmutil_logging( error, "RECEIVE ERROR. rlen=%d", rlen );
            BufferStateReset( aEP );
            return NULL;
        }

        rlen = EPBS->_BufferedDataLen;

        // did we receive a whole header at all?
        if( (size_t)rlen < sizeof( csm_net_msg_t ) )
        {
            csmutil_logging( error, "INCOMPLETE HEADER received. rlen=%d", rlen );
            EPBS->_PartialMsg = 1;  // trigger recv next time we loop
            rlen = 0;  // set 0 to stay in loop
            ret = NULL;
            continue;
        }

        // is it a valid header (header only) ?
        if( csm_header_validate( &( ret->_Header ) ) == 0 )
        {
          // todo: better shut down socket than trying to recover
            csmutil_logging( critical, "INVALID HEADER. PROTOCOL BROKEN!!! need to flush remaining data from socket.");
            BufferStateReset( aEP );
            errno = EBADMSG;
            return NULL;
        }

        ssize_t parsed = EPBS->_BufferedData - aEP->_DataBuffer;

        // todo: we're currently limited in msg-size. So we have to check and make a fuzz about it when it happens
        if( csm_net_msg_GetDataLen( ret ) + sizeof( csm_network_header_t ) > (size_t)DGRAM_PAYLOAD_MAX - parsed )
        {
            csmutil_logging( critical, "MESSAGE PROTOCOL EXCEEDS CURRENT REMAINING BUFFER: %d BYTES.", DGRAM_PAYLOAD_MAX - parsed );
            BufferStateReset( aEP );
            errno = E2BIG;
            return NULL;
        }

        // determine the msg length which might be less than rlen
        size_t msg_len = sizeof( csm_net_msg_t ) + csm_net_msg_GetDataLen( ret );

        if( msg_len > EPBS->_BufferedDataLen )
        {
          csmutil_logging( debug, "MESSAGE PROTOCOL Partial msg received. Remaining %ld BYTES.", msg_len - EPBS->_BufferedDataLen );
          EPBS->_PartialMsg = 1;  // trigger recv next time we loop
          rlen = 0;  // set 0 to stay in loop
          ret = NULL;
          continue;
        }

        EPBS->_PartialMsg = 0;  // if we pass all previous checks, we have at least one complete message in the buffer (not partial)

        // is the complete msg valid with correct checksums
        if( ( csm_net_msg_GetDataLen( ret ) > 0 ) &&
            ( csm_header_check_sum( &( ret->_Header ), data ) != csm_net_msg_GetCheckSum( ret ) ))
        {
          // todo: better shut down socket than trying to recover
            csmutil_logging( critical, "INCORRECT CHECKSUM. PROTOCOL BROKEN!!! need to flush remaining data from socket.");
            BufferStateReset( aEP );
            errno = EBADMSG;
            return NULL;
        }

        // check for ACKs
        if( csm_net_msg_GetAck( ret ) )
        {
            csmutil_logging( trace, "Received an ACK.");
            msg_len = sizeof( csm_net_msg_t );
            if( aReturnCtrlData == 0 )
              goto ignore;
        }

        // check the message checksum
        uint32_t hdrchksum = csm_net_msg_GetCheckSum( ret );
        csm_net_msg_SetDataAndChksum( ret, data, csm_net_msg_GetDataLen( ret ) );
        if( hdrchksum != csm_net_msg_CheckSumCalculate( ret ) )
        {
          csmutil_logging( critical, "RECEIVED MSG WITH INVALID CHECKSUM" );
          BufferStateReset( aEP );
          errno = EBADMSG;
          return NULL;
        }

        // check for Heartbeat msg
        if(( csm_net_msg_GetFlags( ret ) & CSM_HEADER_INT_BIT )
            && ( csm_net_msg_GetCommandType(ret) == CSM_CMD_STATUS ) // ignore status msgs
            && ( ! csm_net_msg_GetResponseFlag( ret ) ))  // only if they are not "response for version check"
        {
            const char *msgData = csm_net_msg_GetData( ret );
            size_t dataLen = csm_net_msg_GetDataLen( ret );

            csmutil_logging( debug, "Received a STATUS MSG: len=%ld, %s", dataLen, msgData );

            if( csm_net_msg_GetCommandType(ret) == CSM_CMD_STATUS 
                && ( dataLen == CSM_DISCONNECT_MSG_LEN ) && ( strncmp( CSM_DISCONNECT_MSG, msgData, CSM_DISCONNECT_MSG_LEN ) == 0 ) )
            {
              csmutil_logging( debug, "Daemon sent disconnect msg." );
              if( aReturnCtrlData == 0 )
              {
                BufferStateReset( aEP );
                errno = ENOTCONN;
                return NULL;
              }
            }
            else
            {
                goto ignore;
            }
        }

        // does the receiver require an ACK: send it
        if( csm_net_msg_RequiresAck( &( ret->_Header ) ) )
        {
            csm_net_msg_t *ack = (csm_net_msg_t*)malloc( sizeof( csm_net_msg_t ) );
            bzero( ack, sizeof( csm_net_msg_t ) );
            csm_net_msg_InitHdr( ack, csm_net_msg_GetHeaderBuffer( ret ) );
            csm_net_msg_SetPriority( ack, CSM_PRIORITY_NO_ACK );
            csm_net_msg_SetAck( ack );
            csm_net_msg_ClrErrorFlag( ack );
            csm_net_msg_SetDataAndChksum( ack, "", 0 );
            csm_net_unix_Send( aEP, ack );
            free( ack );
        }

        // heartbeats might require an ACK but need to be ignored otherwise
        if(( csm_net_msg_GetFlags( ret ) & CSM_HEADER_INT_BIT ) &&
            ( csm_net_msg_GetCommandType( ret ) == CSM_CMD_HEARTBEAT ) &&
            ( ! csm_net_msg_GetResponseFlag( ret ) ))
        {
          // is no longer ignored - heartbeats end up at the CB socket and won't interfere with the regular commands
        }

        // if we go here and have an rlen and a valid return ptr, then update return
        if( msg_len && ret )
        {
            EPBS->_BufferedData += msg_len;
            if( EPBS->_BufferedData > EPBS->_DataEnd )
              return NULL;
            EPBS->_BufferedDataLen -= msg_len;
            csmutil_logging( debug, "%s-%d: Received msg: cmd=%d, msgId=%d, flags=%d", __FILE__, __LINE__,
                             csm_net_msg_GetCommandType( ret ),
                             csm_net_msg_GetMessageID( ret ),
                             csm_net_msg_GetFlags( ret ) );

            if( EPBS->_BufferedData == EPBS->_DataEnd )
              BufferStateReset( aEP );
            break;
        }
        else
        {
ignore:
            bzero( ret, msg_len );
            EPBS->_BufferedData += msg_len;
            if( EPBS->_BufferedData > EPBS->_DataEnd )
              return NULL;
            EPBS->_BufferedDataLen -= msg_len;
            if( EPBS->_BufferedDataLen > 0 )
              ret = (csm_net_msg_t*)EPBS->_BufferedData;
            else
              ret = NULL;
            rlen = 0;  // make sure we retry this loop
        }
    } while(( rlen == 0 ) && ( aBlocking ));
  
  return ret;
}

csm_net_msg_t * csm_net_unix_Recv( csm_net_unix_t *aEP )
{
  return csm_net_unix_RecvMain( aEP, 0, CSM_CMD_UNDEFINED, 0 ); /// TODO remove the cmd
}

csm_net_msg_t * csm_net_unix_BlockingRecv( csm_net_unix_t *aEP, csmi_cmd_t cmd)
{
  return csm_net_unix_RecvMain( aEP, 1, cmd, 0 ); /// TODO remove the cmd?
}

void csm_net_unix_FlushBuffer( csm_net_unix_t *aEP )
{
  aEP->_BufferState._BufferedData = aEP->_DataBuffer;
  aEP->_BufferState._BufferedDataLen = 0;
}

int csm_net_unix_RegisterCallback( csm_net_endpoint_t *aEP, csm_net_unix_CallBack aCallBack)
{
  aEP->_cbfun = aCallBack;
  return 0;
}

int csm_net_unix_UnRegisterCallback( csm_net_endpoint_t *aEP)
{
  aEP->_cbfun = NULL;
  return 0;
}

static
void thread_cancellation_handler( void * aIn )
{
  csmutil_logging( debug, "Executing callback thread cancellation handler");
}

static
int handle_internal_msg( csm_net_unix_t *ep, csm_net_msg_t const *aMsg )
{
  if( ! aMsg )
    return 0;
  switch( csm_net_msg_GetCommandType( aMsg ) )
  {
    case CSM_CMD_HEARTBEAT:
    {
      csm_net_msg_t heartbeat;
      csmutil_logging( trace, "Processing Heartbeat msg: %s", csm_net_msg_GetData( aMsg ));
      csm_net_msg_InitHdr( &heartbeat, (char*)&(aMsg->_Header) );
      csm_net_msg_SetAck( &heartbeat );
      csm_net_msg_SetData( &heartbeat, NULL, 0 );
      size_t len = csm_net_unix_Send( ep, &heartbeat );
      if( len <= 0 )
        return 1;
      break;
    }
    case CSM_CMD_STATUS:
    {
      csmutil_logging( debug, "Internal Ctrl msg: %s", csm_net_msg_GetData( aMsg ));
      size_t len = csm_net_msg_GetDataLen( aMsg );
      if( len != CSM_DISCONNECT_MSG_LEN )
        return 0;
      if( strncmp( csm_net_msg_GetData( aMsg ), CSM_DISCONNECT_MSG, len ) == 0 )
        return 1;
      break;
    }
    default:
      break;
  }
  return 0;
}

void * thread_callback_loop( void * aIn )
{
  csm_net_endpoint_t *ep = (csm_net_endpoint_t*)aIn;
  csm_net_unix_t *cb = ep->_cb;
  int rc = 0;
  int recvRetries = 2;

  pthread_cleanup_push( thread_cancellation_handler, aIn );

  csmutil_logging( debug, "starting CBThread on socket: %s", cb->_Addr.sun_path);
  // todo: instead of polling, we should wait for some data to arrive on the socket before calling recv
  while( ep->_cb_keep_running )
  {
    // don't attempt to recv anything before ep is actually connected
    if( ep->_connected == 0 )
    {
      usleep( 100000 );
      continue;
    }

    /* can be called without locking:
     *  - when tear-down happens in this thread, no issues with access after free
     *  - when tear-down happens in the main thread, pthread_cancel/join get called before cleanup
     */
    csm_net_msg_t *msg = csm_net_unix_RecvMain( cb, 1, CSM_CMD_UNDEFINED, 1 );
    int stored_errno = errno;
    pthread_testcancel();  // check for cancellation before attempting to process any received data

    // double check we're still connected before processing
    if( ep->_connected == 0 )
      continue;

    if( msg == NULL )
    {
      if(( stored_errno == EAGAIN ) || ( stored_errno == ETIMEDOUT ))
      {
        --recvRetries;
        if( recvRetries <= 0 )
        {
          ep->_connected = 0;
          ep->_cb_keep_running = 0;
          rc = ep->_on_disconnect( msg );
          break;
        }
      }
      continue;
    }

    recvRetries = 2;
    if( csm_net_msg_GetInternalFlag( msg ) )
    {
      if(( ep->_connected != 0 ) && ( handle_internal_msg( ep->_ep, msg ) == 1 ))
      {
        ep->_connected = 0;
        ep->_cb_keep_running = 0;
        rc = ep->_on_disconnect( msg );
        break;
      }
    }
    else
    {
      csm_net_msg_Dump( msg );

      if( ep->_cbfun != NULL )
      {
        rc = ep->_cbfun( msg );
        if( rc != 0 )
          csmutil_logging( debug, "CBThread received msg @0x%p; cb=0x%p; rc=%d", msg, ep->_cbfun, rc);
      }
      else
      {
        csmutil_logging( warning, "Callback triggered, but no registered callback found. dropping msg");
      }
    }
  }
  pthread_cleanup_pop( 0 );
  csmutil_logging( debug, "Exiting CBThread on socket: %s", cb->_Addr.sun_path);
  return NULL;
}
