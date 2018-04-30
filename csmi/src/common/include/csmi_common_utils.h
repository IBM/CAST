/*================================================================================

    csmi/src/common/include/csmi_common_utils.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSMI_COMMON_UTILS_H
#define __CSMI_COMMON_UTILS_H

#include "csmi/src/common/include/csmi_cmds.h"
#include "csmi/include/csm_api_common.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** @brief Sends a command through the Infrastructure and waits for a response.
 *
 * @param csm_obj
 * @param cmd
 * @param sendPayload
 * @param sendPayloadLen
 * @param recvPayload
 * @param recvPayloadLen
 * 
 * @return 0 If everything went well.
 */
int csmi_sendrecv_cmd(csm_api_object *csm_obj, csmi_cmd_t cmd, const char *sendPayload, uint32_t sendPayloadLen, char **recvPayload, uint32_t *recvPayloadLen);

/** @brief TODO
 *
 * @param msgId 
 *
 */
void csmi_set_init_msgId( int );

/** @brief TODO
 *
 * @param buf_r
 * @param offset
 * @param rl
 *
 */
void csmi_util_write_rl(char *buf_r, uint32_t *offset, uint32_t rl);

/** @brief TODO
 *
 * @param buf_r
 * @param offset
 *
 * @return TODO
 *
 */
uint32_t csmi_util_read_rl(char *buf_r, uint32_t *offset);

/** @brief TODO
 * 
 * @param buf_r
 * @param buf_r
 * @param offset
 *
 */
void csmi_util_create_wr_str(char *buf_r, char const *src, uint32_t *offset);

// state structure for keeping track of 
// copying data and avoiding buffer overflow...
/** @brief TODO
 *
 */
typedef struct {
    char *buf_r;
    uint32_t offset;
    uint32_t max_offset;
} csmi_util_create_rd_str_t;

/** @brief TODO
 *
 * @param st
 * @param strptr
 *
 */
void csmi_util_create_rd_str(csmi_util_create_rd_str_t *st, char **strptr);

#ifdef __cplusplus
}
#endif

#endif
