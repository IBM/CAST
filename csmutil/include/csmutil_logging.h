/*================================================================================

    csmutil/include/csmutil_logging.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef _CSMI_LOG_H
#define _CSMI_LOG_H

#include <stdio.h>
#include <stdlib.h>

#define CSMI_BUF_LEN 1000

typedef enum {
  #define SEVERITY(n) n,
  #include "utilities/include/severity.h"
  #undef SEVERITY

  NUM_SEVERITIES
} csmutil_logging_level;


#define csmutil_set_msg(buf, msg) \
	sprintf(buf, "[%s-%d]: %s", __FILE__, __LINE__, msg); 

#define csmutil_set_msg_code(buf, msg, code) \
	sprintf(buf, "[%s-%d]: %s %d", __FILE__, __LINE__, msg, code); 

/*
#define csmutil_logging(level, msg) \
	if (level <= WARNING) fprintf(stdout, "CSMI: %s!\n", msg); \
	else { \
		fprintf(stderr, "CSMI: %s!\n", msg);\
		exit(-1); \
	}
*/

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Retrieves the current logging level, used in conditional code paths.
 * @return The @ref csmutil_logging_level currently set by the client.
 */
csmutil_logging_level csmutil_logging_level_get();
void csmutil_logging_level_set(char *aLevelStr);
void csmutil_logging(csmutil_logging_level level, const char *fmt, ...);

#ifdef __cplusplus
}
#endif
#endif
