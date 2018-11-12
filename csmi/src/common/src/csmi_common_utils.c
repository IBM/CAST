/*================================================================================

    csmi/src/common/src/csmi_common_utils.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include "csmutil/include/csmutil_logging.h"
#include "csmnet/src/C/csm_network_internal_api_c.h"
#include "csmi/include/csm_api_common.h"
#include "csmi/src/common/include/csmi_common_utils.h"
#include "csmi/src/common/include/csmi_serialization.h"
#include "csmi/src/common/include/csmi_api_internal.h"

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

extern int csmi_net_unix_Send(csm_net_msg_t *);
extern csm_net_msg_t* csmi_net_unix_Recv(csmi_cmd_t cmd);


static pthread_mutex_t MSGID_MUTEX;
static uint64_t globalMsgId;
static pthread_mutex_t SEND_RECV_MUTEX;

// initialize the starting msg id
void csmi_set_init_msgId( int msgId)
{
  globalMsgId = msgId;
}

static uint64_t get_msgId()
{
    pthread_mutex_lock(&MSGID_MUTEX);
    globalMsgId++;
    pthread_mutex_unlock(&MSGID_MUTEX);
    return globalMsgId;
}

// return 0 if sucessfully get the "expected" response
int csmi_sendrecv_cmd_ext(
    csm_api_object *csm_obj, 
    csmi_cmd_t cmd, 
    uint8_t flags,
    uint8_t priority,
    const char *sendPayload, 
    uint32_t sendPayloadLen, 
    char **recvPayload, 
    uint32_t *recvPayloadLen )
//    uint64_t *recvMsgId)
{
    csm_net_msg_t *netMsg;        //<
    uint8_t recvCmd;              //<
    uint64_t recvMsgId;           //<
    csm_net_msg_t *recvNetMsg;    //<
    const char *recvData;         //<
    uint32_t recvDataLen;         //<

    uint32_t traceId;             ///< The trace id of the send recv command.
    uint64_t msgId;               ///< The message id of the send recv command.

    csmi_err_t *cdata_err;        //<
    int errcode=CSMERR_SENDRCV_ERROR;
    char *errmsg=NULL;            //<
    *recvPayload = NULL;          //<
    *recvPayloadLen = 0;          //<

    msgId = get_msgId();

    // TODO In the future we may want to reserve ~ 13 bits for the node information?
    // Add a trace id derived from the msg id 
    traceId = msgId % 0xFFFFFFFF;

    netMsg = csm_net_msg_Init( cmd, flags, priority,
                              msgId, geteuid(), getegid(), 
                              sendPayload, sendPayloadLen, traceId);

    if (netMsg == NULL) 
    {
      csmutil_logging(error, "%s-%d: csm_net_msg_Init return null\n", __FILE__, __LINE__);
      csm_api_object_trace_set(csm_obj, traceId);
      csm_api_object_errcode_set(csm_obj, CSMERR_SENDRCV_ERROR);
      csm_api_object_errmsg_set(csm_obj, strdup(csm_get_string_from_enum(csmi_cmd_err_t,CSMERR_SENDRCV_ERROR)));
      return errcode;
    }

    pthread_mutex_lock( &SEND_RECV_MUTEX );

    // csmi_net_unix_Send returns the number of bytes sent
    if (csmi_net_unix_Send(netMsg) <= 0) 
    {
      csmutil_logging(error, "%s-%d: csm_net_msg_Send failed\n", __FILE__, __LINE__);
      csm_api_object_trace_set(csm_obj, traceId);
      csm_api_object_errcode_set(csm_obj, CSMERR_SENDRCV_ERROR);
      csm_api_object_errmsg_set(csm_obj, strdup(csm_get_string_from_enum(csmi_cmd_err_t,CSMERR_SENDRCV_ERROR)));
      pthread_mutex_unlock( &SEND_RECV_MUTEX );
      csm_term_lib();
      return errcode;
    }

    csmutil_logging(trace, "%s-%d: The msg for cmd %d sent", __FILE__, __LINE__, cmd);

    free(netMsg);

    if ( (recvNetMsg = csmi_net_unix_Recv( cmd )) == NULL ) 
    {
      csmutil_logging(error, "%s-%d: csm_net_msg_Recv returned NULL\n", __FILE__, __LINE__);
      csm_api_object_trace_set(csm_obj, traceId);
      csm_api_object_errcode_set(csm_obj, CSMERR_SENDRCV_ERROR);
      csm_api_object_errmsg_set(csm_obj, strdup(csm_get_string_from_enum(csmi_cmd_err_t,CSMERR_SENDRCV_ERROR)));
      pthread_mutex_unlock( &SEND_RECV_MUTEX );
      return errcode;
    }

    csmutil_logging(trace, "%s-%d: Got response for cmd %d", __FILE__, __LINE__, cmd);

    // get the header and payload
    // check Ack; ack = csm_net_msg_GetAck(recvNetMsg);
    recvCmd = csm_net_msg_GetCommandType(recvNetMsg);
    recvMsgId = csm_net_msg_GetMessageID(recvNetMsg);
    recvData = csm_net_msg_GetData(recvNetMsg);
    recvDataLen = csm_net_msg_GetDataLen(recvNetMsg);

    csmutil_logging(info, "%s-%d: Recieve Payload len = %d", __FILE__, __LINE__, recvDataLen);

    csmutil_logging(debug, "%s-%d: recvMsgId: %d msgID: %d", __FILE__, __LINE__, recvMsgId, msgId);

    if (recvMsgId != msgId) // mismatch message id
    { 
      csmutil_logging(warning, "%s-%d: Mismatched msgId", __FILE__, __LINE__);

      errcode = CSMERR_MSGID_MISMATCH;
      errmsg = strdup(csm_get_string_from_enum(csmi_cmd_err_t,errcode));
      
    } 
    else if (csm_net_msg_GetErrorFlag(recvNetMsg)) // match MSG ID but error flag set
    {
      // the response indicates an error at the local daemon
      csmutil_logging(warning, "%s-%d: the Error Flag Set", __FILE__, __LINE__);
      
      // can try to unpack the error in this case. a valid rsp from db processor

      // to-do: do not check the RESP bit yet

      if (recvDataLen <= 0) {
          csmutil_logging(error, "%s-%d: Empty payload. Expected errcode and optional errmsg",
      		 __FILE__, __LINE__);
          errcode = CSMERR_SENDRCV_ERROR;
          errmsg = strdup(csm_get_string_from_enum(csmi_cmd_err_t,CSMERR_SENDRCV_ERROR));

      } 
      else //error payload not empty
      { 
          cdata_err = csmi_err_unpack(recvData, recvDataLen);
          if (cdata_err == NULL) { // fail to unpack error payload
              csmutil_logging(error, "%s-%d: Failed to do unpack the error payload",
      		    __FILE__, __LINE__);
              errcode = CSMERR_MSG_UNPACK_ERROR;
              errmsg = strdup(csm_get_string_from_enum(csmi_cmd_err_t,errcode));

          } 
          else // success to unpack error payload
          {
              errcode = cdata_err->errcode;
              if (errcode <= CSMI_SUCCESS || errcode >= csm_enum_max(csmi_cmd_err_t))
              {
                  csmutil_logging(error, "%s-%d: Invalid errcode: %d", __FILE__, __LINE__, errcode);
                  errcode = CSMERR_SENDRCV_ERROR;
                  errmsg = strdup(csm_get_string_from_enum(csmi_cmd_err_t,CSMERR_SENDRCV_ERROR));

                  if (cdata_err->errmsg) free(cdata_err->errmsg);

              } 
              else  // valid errcode
              {
                  if (cdata_err->errmsg) errmsg = cdata_err->errmsg;
                  else errmsg = strdup(csm_get_string_from_enum(csmi_cmd_err_t,errcode));

                  if( cdata_err->error_count > 0  && cdata_err->node_errors)
                  {
                        // Transfer ownership of the objects.
                        csm_api_object_errlist_set(csm_obj, cdata_err->error_count, cdata_err->node_errors);
                        cdata_err->node_errors = NULL;
                        cdata_err->error_count = 0;
                  }
              }
     
              // now can free cdata_err
              free(cdata_err);
          }
      }

    } 
    else if (recvCmd != cmd) // unmatch csmi command without error bit set
    {
      if (recvCmd == CSM_CMD_ERROR) // unknown recvd cmd to the daemon
      {
          csmutil_logging(warning, "%s-%d: CSM_CMD_ERROR", __FILE__, __LINE__);

          errcode = CSMERR_CMD_UNKNOWN;
      } 
      else // somehow recvd cmd (not CSM_CMD_ERROR) but mismatch to the requested cmd
      {
          csmutil_logging(warning, "%s-%d: Mismatched csmi cmd", __FILE__, __LINE__);

          errcode = CSMERR_CMD_MISMATCH;
      }
      errmsg = strdup(csm_get_string_from_enum(csmi_cmd_err_t,errcode));
    } 
    else 
    {
          //to-do: shall we check the ACK bit set?
      
          csmutil_logging(info, "%s-%d: Get an expected response", __FILE__, __LINE__);

          // finally, we can copy the received payload
          if (recvData && recvDataLen > 0) 
          {
              *recvPayload = (char*)malloc(sizeof(char)*recvDataLen);
              memcpy(*recvPayload, recvData, recvDataLen);
              *recvPayloadLen = recvDataLen;
          }
          errcode = CSMI_SUCCESS;
    }

    // FIXME This fails to free the rbuffer created in the mutex lock! - John Dunham
    // NOTE: the header and payload in recvNetMsg will be contiguous, so one free is enough.
    pthread_mutex_unlock( &SEND_RECV_MUTEX );

    // record the err/trace info before return
    csm_api_object_trace_set(csm_obj, traceId);
    csm_api_object_errcode_set(csm_obj, errcode);
    if (errmsg) csm_api_object_errmsg_set(csm_obj, errmsg);

    return errcode;
}


int csmi_sendrecv_cmd(
    csm_api_object *csm_obj,
    csmi_cmd_t cmd,
    const char *sendPayload,
    uint32_t sendPayloadLen,
    char **recvPayload,
    uint32_t *recvPayloadLen )
{
  uint8_t flags = 0;            //<
  uint8_t priority = CSM_PRIORITY_DEFAULT; //<

  return csmi_sendrecv_cmd_ext( csm_obj,
                                cmd,
                                flags,
                                priority,
                                sendPayload,
                                sendPayloadLen,
                                recvPayload,
                                recvPayloadLen );
}

