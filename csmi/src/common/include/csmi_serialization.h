/*================================================================================

    csmi/src/common/include/csmi_serialization.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef _CSMI_SERIALIZATION
#define _CSMI_SERIALIZATION

#include "csmi_cmds.h"

#include <stdint.h>

typedef struct {
  void *cdata;
  uint32_t cdataLen;
} csmi_cdata_t;

typedef struct {
  char *buf;
  uint32_t bufLen;
} csmi_buf_t;

typedef csmi_buf_t* (*packPrototype)(const csmi_cmd_t cmd, const csmi_cdata_t *inCstructData);
typedef csmi_cdata_t* (*unpackPrototype)(const csmi_cmd_t cmd, const char *inPayload, const uint32_t inPayloadLen);

typedef struct {
  csmi_cmd_t cmdType;
  const char *cmdName;
  const char *dbTabName;
  const char *className;
  packPrototype packFunc;
  unpackPrototype unpackFunc;
  // in case there are arguments attached to the API
  packPrototype argPackFunc;
  unpackPrototype argUnpackFunc;
} cmd_serialization_t;


#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_ERR_MSG_LEN 2000000
typedef struct {
  int errcode;
  char* errmsg;
} csmi_err_t;

void csmi_cmd_hdl_init(void);

packPrototype csmi_pack_get(csmi_cmd_t cmd);
unpackPrototype csmi_unpack_get(csmi_cmd_t cmd);
packPrototype csmi_argpack_get(csmi_cmd_t cmd);
unpackPrototype csmi_argunpack_get(csmi_cmd_t cmd);
const char *csmi_cmdname_get(csmi_cmd_t cmd);
csmi_cmd_t csmi_cmd_get(const char *name);
const char *csmi_classname_get(csmi_cmd_t cmd);
const char *csmi_dbtabname_get(csmi_cmd_t cmd);

char *csmi_err_pack(const int errcode, const char *errmsg, uint32_t *buf_len);
csmi_err_t* csmi_err_unpack(const char *buf, const uint32_t buf_len);
void csmi_err_free(csmi_err_t *err_obj);

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
int preventStringBufferOverRead(uint32_t offset, uint32_t bufferLength);

#ifdef __cplusplus
}
#endif


#endif
