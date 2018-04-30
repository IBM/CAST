/*================================================================================

    csmi/src/common/include/csmi_common_serialization.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSMI_COMMON_SERIALIZATON_H
#define __CSMI_COMMON_SERIALIZATON_H

#include "csmi_serialization.h"

#ifdef __cplusplus
extern "C"
{
#endif

void csmi_common_write_rl(char *buf_r, uint32_t *offset, uint32_t rl);
uint32_t csmi_common_read_rl(char *buf_r, uint32_t *offset);
void csmi_common_create_wr_str(char *buf_r, char const *src, uint32_t *offset);
// state structure for keeping track of 
// copying data and avoiding buffer overflow...
typedef struct {
    char *buf_r;
    uint32_t offset;
    uint32_t max_offset;
} csmi_common_create_rd_str_t;
void csmi_common_create_rd_str(csmi_common_create_rd_str_t *st, const char ** strptr);
void csmi_common_create_rd_str_arr(csmi_common_create_rd_str_t *st,
                                   const char **strptr, uint32_t n,
                                   uint32_t str_arr_offset);

#ifdef __cplusplus
}
#endif

#endif


