/*================================================================================

    csmi/src/common/src/csmi_serialization.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "csmi/src/common/include/csmi_serialization.h"
// @Automated_Includes (Do Not Remove, for Automation! - John Dunham)

#include <stdlib.h>
#include <strings.h>
#include <string.h>

#include "csmutil/include/csmutil_logging.h"

static cmd_serialization_t CSMI_MAPPING[] = {
  #define cmd(n) {CSM_CMD_##n, "csm_"#n, NULL, NULL, NULL, NULL, NULL, NULL},
  #include "csmi/src/common/include/csmi_cmds_def.h"
  #undef cmd
  
  #define cmd(n) {n, #n, NULL, NULL, NULL, NULL, NULL, NULL},
  #include "csmi/src/common/include/csm_cmds_internal_def.h"
  #undef cmd
};

void csmi_callback_set(csmi_cmd_t cmd, packPrototype packFunc, unpackPrototype unpackFunc)
{
  if (csmi_cmd_is_valid( cmd )) {
    CSMI_MAPPING[cmd].packFunc = packFunc;
    CSMI_MAPPING[cmd].unpackFunc = unpackFunc;
  }
}

void csmi_arg_callback_set(csmi_cmd_t cmd, packPrototype packFunc, unpackPrototype unpackFunc)
{
  if (csmi_cmd_is_valid( cmd )) {
    CSMI_MAPPING[cmd].argPackFunc = packFunc;
    CSMI_MAPPING[cmd].argUnpackFunc = unpackFunc;
  }
}

packPrototype csmi_pack_get(csmi_cmd_t cmd)
{
  if (csmi_cmd_is_valid( cmd )) return CSMI_MAPPING[cmd].packFunc;
  else return NULL;
}

unpackPrototype csmi_unpack_get(csmi_cmd_t cmd)
{
  if (csmi_cmd_is_valid( cmd )) return CSMI_MAPPING[cmd].unpackFunc;
  else return NULL;
}

packPrototype csmi_argpack_get(csmi_cmd_t cmd)
{
  if (csmi_cmd_is_valid( cmd )) return CSMI_MAPPING[cmd].argPackFunc;
  else return NULL;
}

unpackPrototype csmi_argunpack_get(csmi_cmd_t cmd)
{
  if (csmi_cmd_is_valid( cmd )) return CSMI_MAPPING[cmd].argUnpackFunc;
  else return NULL;
}

const char *csmi_cmdname_get(csmi_cmd_t cmd)
{
  if (csmi_cmd_is_valid( cmd )) return CSMI_MAPPING[cmd].cmdName;
  else return NULL;
}

csmi_cmd_t csmi_cmd_get(const char *name)
{
  int ret;
  for (ret = 0; ret < CSM_CMD_MAX; ret++)
  {
    if( ! csmi_cmd_is_valid( ret ) ) continue;
    if ( strcasecmp(CSMI_MAPPING[(csmi_cmd_t) ret].cmdName, name) == 0 ) return (csmi_cmd_t) ret;
  }
    
  return CSM_CMD_MAX;
}

const char *csmi_classname_get(csmi_cmd_t cmd)
{
  if (csmi_cmd_is_valid( cmd )) return CSMI_MAPPING[cmd].className;
  else return NULL;
}

void csmi_classname_set(csmi_cmd_t cmd, const char *name) {
  if (csmi_cmd_is_valid( cmd )) {
    CSMI_MAPPING[cmd].className = name;
  }
}

const char *csmi_dbtabname_get(csmi_cmd_t cmd)
{
  if (csmi_cmd_is_valid( cmd )) return CSMI_MAPPING[cmd].dbTabName;
  else return NULL;
}

void csmi_dbtabname_set(csmi_cmd_t cmd, const char *name) {
  if (csmi_cmd_is_valid( cmd )) {
    CSMI_MAPPING[cmd].dbTabName = name;
  }
}

void csmi_cmd_hdl_init(void)
{
  /* The @ symbol is used for the sake of automation, please do not remove. -John Dunham 12-06-12*/
  /* @Workload Management */	
  /*
  csmi_callback_set(CSM_CMD_allocation_update_state, NULL, NULL);
  csmi_callback_set(CSM_CMD_allocation_query_active_all, NULL, NULL);
  csmi_callback_set(CSM_CMD_allocation_create, NULL, NULL);
  //csmi_arg_callback_set(CSM_CMD_allocation_create, allocation_create_arg_pack, allocation_create_arg_unpack);
  csmi_callback_set(CSM_CMD_allocation_delete, NULL, NULL );
  //csmi_arg_callback_set(CSM_CMD_allocation_delete, allocation_delete_arg_pack, allocation_delete_arg_unpack);
  csmi_callback_set(CSM_CMD_allocation_query, NULL, NULL );
  csmi_arg_callback_set(CSM_CMD_allocation_query, NULL, NULL );
  csmi_callback_set(CSM_CMD_allocation_query_details, NULL, NULL );
  csmi_arg_callback_set(CSM_CMD_allocation_query_details, NULL, NULL );
  */
  /* @Inventory */
  /*Old query test*/
  /*
  csmi_callback_set(CSM_CMD_node_attributes_query, node_attributes_pack, node_attributes_unpack);
  csmi_arg_callback_set(CSM_CMD_node_attributes_query, node_attributes_arg_pack, node_attributes_arg_unpack);
  csmi_dbtabname_set(CSM_CMD_node_attributes_query, "nodes");
  */
  
  /*node_attributes_query*/
  //csmi_callback_set(CSM_CMD_node_attributes_query, node_attributes_query_pack, node_attributes_query_unpack);
  csmi_dbtabname_set(CSM_CMD_node_attributes_query, "csm_node");
  
  /* @RAS */
  //csmi_callback_set(CSM_CMD_ras_event_create, ras_event_create_pack, ras_event_create_unpack);
  //csmi_arg_callback_set(CSM_CMD_ras_event_create, ras_event_create_arg_pack, ras_event_create_arg_unpack);
  csmi_classname_set(CSM_CMD_ras_event_create, "CSMIRasEventCreate");

  /*
  csmi_callback_set(CSM_CMD_ras_subscribe, ras_subscribe_pack, ras_subscribe_unpack);
  csmi_arg_callback_set(CSM_CMD_ras_subscribe, ras_subscribe_arg_pack, ras_subscribe_arg_unpack);

  csmi_callback_set(CSM_CMD_ras_unsubscribe, ras_unsubscribe_pack, ras_unsubscribe_unpack);
  csmi_arg_callback_set(CSM_CMD_ras_unsubscribe, ras_unsubscribe_arg_pack, ras_unsubscribe_arg_unpack);

  csmi_callback_set(CSM_CMD_ras_sub_event, ras_sub_event_pack, ras_sub_event_unpack);
  csmi_arg_callback_set(CSM_CMD_ras_sub_event, ras_sub_event_arg_pack, ras_sub_event_arg_unpack);

  csmi_callback_set(CSM_CMD_ras_event_query, ras_event_query_pack, ras_event_query_unpack);
  csmi_arg_callback_set(CSM_CMD_ras_event_query, ras_event_query_arg_pack, ras_event_query_arg_unpack);

  csmi_callback_set(CSM_CMD_ras_msg_type_get, ras_msg_type_get_pack, ras_msg_type_get_unpack);
  csmi_arg_callback_set(CSM_CMD_ras_msg_type_get, ras_msg_type_get_arg_pack, ras_msg_type_get_arg_unpack);
  */
  /* @Burst Buffer */
}

char *csmi_err_pack(const int errcode, const char *errmsg, uint32_t *buf_len)
{
  char *buf = NULL;
  int len;
  uint32_t offset = 0;
  
  *buf_len = sizeof(errcode);
  if (errmsg) *buf_len += strnlen(errmsg, MAX_ERR_MSG_LEN) + sizeof(len);

  if ( (buf = calloc(1, *buf_len)) == 0 ) {
    csmutil_logging(error, "%s-%d: calloc NULL", __FILE__, __LINE__);
    return NULL;
  }

  // add the errcode
  memcpy(buf + offset, &errcode, sizeof(errcode));
  offset += sizeof(errcode);

  // add the errmsg if not NULL
  if (errmsg) {
    len = strnlen(errmsg, MAX_ERR_MSG_LEN);
    memcpy(buf + offset, &len, sizeof(len));
    offset += sizeof(len);

    memcpy(buf + offset, errmsg, len);
    offset += len;
  }
  if (offset != *buf_len) {
    csmutil_logging(error, "%s-%d: packing error\n", __FILE__, __LINE__);
    free(buf);
    return NULL;
  }
  
  return buf;
}

csmi_err_t* csmi_err_unpack(const char *buf, const uint32_t buf_len) 
{
  int errcode;
  uint32_t offset = 0;
  csmi_err_t *cdata=NULL;
  static const size_t min_buf_size = sizeof(cdata->errcode);
  uint32_t len;

  if (buf == NULL || buf_len < min_buf_size) {
    csmutil_logging(warning,"csmi_err_unpack: buf not valid");
    return cdata;
  }

  // set up the return data
  cdata = calloc(1, sizeof(csmi_err_t));

  // first, get the errcode
  memcpy(&errcode, buf + offset, sizeof(errcode));
  offset += sizeof(errcode);
  cdata->errcode = errcode;

  if ((offset+sizeof(len)) < buf_len) {
    // get the errmsg length
    memcpy(&len, buf + offset, sizeof(len));
    offset += sizeof(len);

    // get the errmsg string
    if ((offset+len) == buf_len) {
      cdata->errmsg = calloc(1, len + 1);
      memcpy(cdata->errmsg, buf + offset, len);
      offset += len;
    }
  }

  if (offset != buf_len) {
    csmutil_logging(error, "%s-%d: unpacking error\n", __FILE__, __LINE__);
    free(cdata);
    return NULL;
  }
  return cdata;
}


void csmi_err_free(csmi_err_t *err_obj) 
{
  if (err_obj)
  {
    if (err_obj->errmsg) free(err_obj->errmsg);
    free(err_obj);
  }
}

/** @brief Used to make sure we don't read past the end of the string buffer.
 * @author: Nick Buonarota
 * @author_email: nbuonar@us.ibm.com
 * last edited: September 9, 2016
 *
 * @param offset Current position on the string buffer.
 * @param bufferLength The length of the string buffer.
 *
 * @return 0 Success
 * @return -1 Error: offset is greater than the buffer length.
 */
int preventStringBufferOverRead(uint32_t offset, uint32_t bufferLength)
{
	csmutil_logging(trace, "%s-%d", __FILE__, __LINE__);
	csmutil_logging(trace, "  In preventStringBufferOverRead()...");
	csmutil_logging(debug, "%s-%d", __FILE__, __LINE__);
	csmutil_logging(debug, "  values passed in:");
	csmutil_logging(debug, "    offset: %u", offset);
	csmutil_logging(debug, "    bufferLength: %u", bufferLength);
	if(offset > bufferLength){
		csmutil_logging(error, "%s-%d", __FILE__, __LINE__);
		csmutil_logging(error, "  offset is greater than the bufferLength.");
		csmutil_logging(error, "  preventing reading past the length of the string buffer.");
		csmutil_logging(error, "  offset: %u", offset);
		csmutil_logging(error, "  bufferLength: %u", bufferLength);
		return -1;
	}
	csmutil_logging(trace, "%s-%d", __FILE__, __LINE__);
	csmutil_logging(trace, "  Leaving preventStringBufferOverRead()...");
	return 0;
}
