/*================================================================================

    csmutil/src/csmutil_logging.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "csmutil_logging.h"

static csmutil_logging_level default_log_level = warning;


static const char* csmutil_logging_str[] =
{
  #define SEVERITY(n) #n,
  #include "utilities/include/severity.h"
  #undef SEVERITY
};

csmutil_logging_level csmutil_logging_level_get()
{
    return default_log_level;
}

void csmutil_logging_level_set(char *aLevelStr)
{
  int i;
  FILE *fp = stderr;
  for (i=0; i<NUM_SEVERITIES;i++)
    if (strcasecmp(csmutil_logging_str[i], aLevelStr) == 0) break;
  if (i < NUM_SEVERITIES)
  {
     //fprintf( fp, "[csmapi]: The default log level: %s...\n", aLevelStr);
     default_log_level = (csmutil_logging_level) i;
  }
  else
    fprintf(fp, "[csmapi]: Unknown logging level: %s!\n", aLevelStr);
}

void csmutil_logging(csmutil_logging_level level, const char *fmt, ...)
{
  va_list ap;
  char buf[CSMI_BUF_LEN];
  FILE *fp;
 
  if (level >= NUM_SEVERITIES) {
    fprintf(stderr, "[csmapi:cirtical]: logging level not valid\n");
    exit(-1);
  }

  if (level < default_log_level) return;
  
  fp = stderr;

  va_start(ap, fmt);
  vsnprintf(buf, CSMI_BUF_LEN, fmt, ap);
  fprintf(fp,"[csmapi][%s]\t%s\n", csmutil_logging_str[level], buf);
  fflush(fp);
  va_end(ap);

}
