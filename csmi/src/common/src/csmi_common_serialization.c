/*================================================================================

    csmi/src/common/src/csmi_common_serialization.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <strings.h>
#include <memory.h>
#include "../include/csmi_common_serialization.h"

// TODO fix the csm_* functions.
#include "csmutil/include/csmutil_logging.h"
#include "csmi/src/common/include/csmi_api_internal.h"
#include "csmi/src/common/include/csm_serialization_x_macros.h"
#include "csmi/include/csm_api_common.h"

// write 4 byte runlength code...
// read one byte at a time to avoid alignment issues...
void csmi_common_write_rl(char *buf_r, uint32_t *offset, uint32_t rl)
{
    unsigned char *p = (unsigned char *)(buf_r + *offset);
    int n;
    for (n = 0; n < 4; n++) {
        *p++ = rl & 0xFF; rl >>= 8;
    }
    *offset += 4;

}
// read 4 byte runlength code...  
uint32_t csmi_common_read_rl(char *buf_r, uint32_t *offset) 
{   
    uint32_t rl = 0;
    unsigned char *p = (unsigned char *)(buf_r + *offset + 4);
    unsigned n;
    for (n = 0; n < 4; n++) {
        p--; 
        rl <<= 8; 
        rl |= (uint32_t)(*p); 
    }
    *offset+=4;
    return(rl);
}
void csmi_common_create_wr_str(char *buf_r, char const *src, uint32_t *offset)
{
    // NOTE: we are deliberatly ignoring byte order, and assuming that both the sender and receiver
    // are matched..
    if (src) {
        uint32_t rl = strlen(src) + 1;
        csmi_common_write_rl(buf_r, offset, rl);
        memcpy(buf_r+(*offset), src, rl);
        *offset += rl;
    }
    else {
        uint32_t rl = 0;  
        csmi_common_write_rl(buf_r, offset, rl);
    }
}

void csmi_common_create_rd_str(csmi_common_create_rd_str_t *st,  const char **  strptr)
{
    uint32_t rl = csmi_common_read_rl(st->buf_r, &st->offset);

    // first word is the runlen...guard against overflow...
    if (st->offset+rl >= st->max_offset) {    
        assert(0);
        *strptr = NULL;
        return;
    }
    if (rl) {
        *strptr = st->buf_r + st->offset;
        st->offset += rl;
    }
    else {
        *strptr = NULL;
    }
}

void csmi_common_create_rd_str_arr(csmi_common_create_rd_str_t *st,
                                   const char **strptr, uint32_t n,
                                   uint32_t str_arr_offset)
{
  uint32_t i;
  uint32_t rl;
  char **str_arr;

  if (n == 0) {
    *strptr = NULL;
    return;
  }

  str_arr = (char **)(st->buf_r + str_arr_offset);
  *strptr = (char *)str_arr;
  for (i = 0; i < n; i++) {
    rl = csmi_common_read_rl(st->buf_r, &st->offset);
    if (rl) {
      str_arr[i] = st->buf_r + st->offset;
      st->offset += rl;
    }
    else {
      str_arr[i] = NULL;
    }
  }
}

int csm_serialize_str_array( char** array, uint32_t array_len, char** return_buffer, 
                         uint32_t* return_buffer_len )
{
    if ( !array && array_len != 0) return 1;

    int i;
    int offset = 0;
    int size_dump=0;
    *return_buffer_len = UINT32_T_SIZE;

    for( i=0; i < array_len; i++)               
        *return_buffer_len += UINT32_T_SIZE + STR_LENGTH(array[i]);

    // Create the buffer 
    char* buffer = (char*) malloc(*return_buffer_len);

    // Save the number of strings to the buffer first.
    memcpy( buffer, &array_len, UINT32_T_SIZE); 
    offset = UINT32_T_SIZE;

    // Actually pack the buffer.
    for( i=0; i < array_len; i++ )
    {
        if ( array[i] )
        {
            size_dump = strlen(array[i]) + 1;           
            memcpy(buffer + offset, &size_dump, sizeof(size_dump)); 
            offset += sizeof(size_dump);

            memcpy( buffer + offset, array[i], size_dump); 
            offset += size_dump;
        }                                                   
        else 
        {                                              
            size_dump = 1;                                  
            memcpy(buffer + offset, &size_dump, sizeof(size_dump)); 
            offset += sizeof(size_dump);

            size_dump = 0;                                  
            memcpy(buffer + offset, &size_dump, sizeof(char)); 
            offset += sizeof(char);
       }
    }

    *return_buffer = buffer;
    return 0;
}

int csm_deserialize_str_array( char** array[], uint32_t* array_len, const char* buffer, 
                            uint32_t buffer_len )
{
    // If the buffer is totally empty return a null array.
    if ( buffer_len < UINT32_T_SIZE )
    {
        array_len = 0;
        array = NULL;
        return 1;
    }

    uint32_t i;
    uint32_t offset = 0;
    uint32_t str_offset;

    memcpy( array_len, buffer, UINT32_T_SIZE );
    offset = UINT32_T_SIZE;
    
    // malloc the string array.
    *array = malloc( *array_len * sizeof(char*));
    
    for ( i = 0; i < *array_len; i++ )
    {
        memcpy( &str_offset, buffer + offset, sizeof(str_offset) ); 
        offset += sizeof(str_offset); 

	    csmutil_logging(debug, "%d Offset", str_offset);
        (*array)[i] = (char*) malloc(str_offset);
        memcpy((*array)[i], buffer + offset, str_offset);
        offset += str_offset;
    }
    return 0;
}

int csm_enum_from_string(char *enum_str, const char *enum_strs[])
{
    int retval = -1; ///< The enum value of the string.
    int i = 0;       ///< While loop counter.

    // If a null string was supplied return -1
    if (!enum_str || !enum_str[0])
        return retval;
    
    // While a match hasn't been found and the first character is non null.
    while(retval < 0 && enum_strs[i][0])
    {
        if (!strcasecmp(enum_str, enum_strs[i++]))
            retval = i - 1;     
    } 

    return retval;
}

