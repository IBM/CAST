/*================================================================================

    csmi/src/ras/src/csm_api_ras.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/* ================================================================ */
/*                                                                  */
/* Licensed Materials - Property of IBM                             */
/*                                                                  */
/* (C) Copyright IBM Corp.  2015                                    */
/*                                                                  */
/* US Government Users Restricted Rights -                          */
/* Use, duplication or disclosure restricted                        */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/* This software is available to you under the                      */
/* Eclipse Public License (EPL).                                    */
/*                                                                  */
/* ================================================================ */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */

// expermental ras source scafolding...

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "csmi/include/csm_api_ras.h"


static int csm_ras_quote_json_str(char *outbuf, unsigned maxlen,
                                  const char *str);

/**
 * Quote a json string and protect against buffer overflow.. 
 * 
 * @param outbuf
 * @param maxlen 
 * @param str 
 * 
 * @return returns number of characters in the output string 
 *                 not including the null terminator. 
 */
static int csm_ras_quote_json_str(char *outbuf, unsigned maxlen,
                                  const char *str)
{
    unsigned n;
    unsigned adjmax = maxlen-6;
    char *p = outbuf;
    const char *s = str;

    if ((maxlen < 6) || (outbuf == NULL)) {
        return(-EINVAL);        // invalid max len...
    }
    *p++ = '\"';
    if (str) {
        
        for (n = 0; n < adjmax; n++) {   // allways allow enough space to escape one character.
            if (*s == 0)
                break;
            switch (*s) {
                case '\\': 
                case '"':  
                case '/':  
                case '\b': 
                case '\f': 
                case '\n': 
                case '\r': 
                case '\t': *p++ = '\\'; *p++ = *s; n++; break;
                default: *p++ = *s; break;
            }
            s++;
        }
    }
    *p++ = '\"';
    *p++ = 0; 
    return((p-outbuf)-1);
}




static int csm_ras_add_kv_pair(char *outbuf, unsigned maxlen,
                           char const *key, char const *value,
                           char const *sep)
{
    int nc;
    unsigned lenremaining = maxlen;
    char *jb = outbuf;

    if (sep) {          // do we need a leading separator...
        if (lenremaining < strlen(sep)+1) return(-E2BIG);
        strcpy(jb, sep);
        jb += strlen(sep);
    }

    nc = csm_ras_quote_json_str(jb, lenremaining, key); 
    if (nc < 0) return(nc);  
    lenremaining -= nc; jb += nc;
    if (lenremaining < 2) return(-E2BIG);
    *jb++ = ':'; lenremaining--;
    nc = csm_ras_quote_json_str(jb, lenremaining, value); 
    if (nc < 0) return(nc);  
    lenremaining -= nc; jb += nc;

    return(jb-outbuf);


}
int csm_ras_create_event_kv_json(char const *msg_id, 
                         char const *time_stamp,
						 char const *location_name,
                         char const *raw_data,
                         char const *kvcsv,
                         char *jsonbuffer, unsigned maxlen)
{
    int nc = 0;
    unsigned lenremaining = maxlen;
    char *jb = jsonbuffer;
    char *kbuff = NULL;


    if ((kvcsv) && (strlen(kvcsv) > 0))
    {
        kbuff = malloc(strlen(kvcsv)+1);
        if (! kbuff)  {
            nc =(-ENOMEM);
            goto fail;
        }
        strcpy(kbuff, kvcsv);
    }
    if ((jb == NULL) || (maxlen < 10)) {
        nc = -EINVAL; goto fail;
    }

     *jb++ = '{'; *jb++ = '\n';
     lenremaining -= 2;

     nc = csm_ras_add_kv_pair(jb, lenremaining, CSM_RAS_FKEY_MSG_ID, msg_id, NULL); 
     if (nc < 0) goto fail;
     lenremaining -= nc; jb += nc;
     nc = csm_ras_add_kv_pair(jb, lenremaining, CSM_RAS_FKEY_TIME_STAMP, time_stamp,",\n"); 
     if (nc < 0) goto fail;
     lenremaining -= nc; jb += nc;
     nc = csm_ras_add_kv_pair(jb, lenremaining, CSM_RAS_FKEY_LOCATION_NAME, location_name,",\n"); 
     if (nc < 0) goto fail;
     lenremaining -= nc; jb += nc;
     if (raw_data) {
         nc = csm_ras_add_kv_pair(jb, lenremaining, CSM_RAS_FKEY_RAW_DATA, raw_data,",\n"); 
         if (nc < 0) goto fail;
         lenremaining -= nc; jb += nc;
     }

     // now parse off the key value pairs...
     if (kbuff) {
         char *s1;
         char *sp1, *sp2;
         for (s1 = kbuff; ; s1= NULL) {
            char *kv;
            char *key, *value;
            kv = strtok_r(s1, ",", &sp1);       // comma separated first
            if (kv == NULL)
                break;
           key = strtok_r(kv, "=", &sp2);   // key=value next...

           if (key == NULL) {nc =  -EINVAL; goto fail; }
           value = strtok_r(NULL, "=", &sp2);   // key=value next...
           nc = csm_ras_add_kv_pair(jb, lenremaining, key, value,",\n");

           if (nc < 0) goto fail;
           lenremaining -= nc; jb += nc;
         }
     }

     if (lenremaining < 3) return(-E2BIG);
     *jb++ = '\n'; *jb++ = '}'; *jb++=0;
     lenremaining -= 3;
     return(0);

fail:
     // failure path, do any memory cleanups...
    if (kbuff)
        free(kbuff);
    return(nc);

}
    

