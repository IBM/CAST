/*================================================================================

    csmi/src/ras/src/csmi_ras_subscribe_callback.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

/*Needed for CSM*/
#include "csm_api_ras.h"
#include "csmi/include/csmi_type_common.h"
#include "csmnet/src/C/csm_network_internal_api_c.h"
#include "csmutil/include/csmutil_logging.h"


typedef struct {
    ras_event_subscribe_callback _cbfun;
} CsmRasSubscribeCtl;

// static function should initialize this to NULLS...
static CsmRasSubscribeCtl csmRasSubcribeCtl;

int csmi_ll_ras_callback(csm_net_msg_t* recvNetMsg) 
{
    const char *recvData =  csm_net_msg_GetData(recvNetMsg);
    uint32_t recvDataLen = csm_net_msg_GetDataLen(recvNetMsg);
    csmi_ras_event_vector_t *event_vect = NULL;;

    // unpack
    if ( !(csmRasSubcribeCtl._cbfun == NULL || recvData == NULL) )
    {
        csm_deserialize_struct( 
            csmi_ras_event_vector_t, &event_vect, (const char *)recvData, recvDataLen);

        (*csmRasSubcribeCtl._cbfun)(event_vect);
        csm_free_struct_ptr(csmi_ras_event_vector_t,event_vect);
    }

    return(CSMI_SUCCESS);
}

int csmi_ll_callback(csm_net_msg_t* recvNetMsg) 
{
    uint8_t recvCmd;
    int rc = CSMI_SUCCESS;

    recvCmd = csm_net_msg_GetCommandType(recvNetMsg);
    switch (recvCmd) {
        case CSM_CMD_ras_sub_event:
            rc  = csmi_ll_ras_callback(recvNetMsg);
            break;
        default: 
            csmutil_logging(error, "unrecognized callback command id");
            break;
    }

    free(recvNetMsg);       // dispose of the message, end of the line...
    return(rc);
}

//typedef void (*ras_event_subscribe_callback)(csm_api_object *csm_obj,
//                                             csmi_ras_event_vector_t *event_vect);
int csm_ras_subscribe_callback(ras_event_subscribe_callback rasEventCallback)
{
    csmRasSubcribeCtl._cbfun = rasEventCallback;
    return(csmi_register_Callback(csmi_ll_callback));
}





