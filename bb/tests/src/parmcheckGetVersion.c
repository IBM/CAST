/*******************************************************************************
 |    pamrcheckGetVersion.c
 |
 |   Copyright IBM Corporation 2017 All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>


#include "bb/include/bbapi.h"
#include "bb/include/bbapi_types.h"

static int DOPRINT=BBERRORJSON;

int print_bberror(int rc)
{
    if(rc)
    {
        char errstring[4096];
        size_t numBytesAvail;
        size_t buf_len = sizeof(errstring);
        if (DOPRINT){    
          BB_GetLastErrorDetails(DOPRINT, &numBytesAvail, buf_len, errstring);
          //printf("Error rc:       %d\n", rc);
          printf("Error details:  %s\n", errstring );
        }
    }
    return rc;
}

int check(int rc)
{
    if(rc)
    {
        print_bberror(rc);
        printf("Aborting due to failures\n");
        exit(-1);
    }
    return rc;
}

int main(int argc, char** argv)
{
    int rc;
    int i=0;

    if (argc==2){
      if ( strcmp(argv[1],"NOPRINT")==0) DOPRINT=0;
      else if ( strcmp(argv[1],"BBERRORJSON")==0) DOPRINT=BBERRORJSON;
      else if ( strcmp(argv[1],"BBERRORXML")==0) DOPRINT=BBERRORXML;
      else if ( strcmp(argv[1],"BBERRORFLAT")==0) DOPRINT=BBERRORFLAT;
      else {
        printf("%s [printoption]\n",argv[0]);
        printf("printoption= one of BBERRORJSON (default), NOPRINT, BBERRORXML, BBERRORFLAT\n");
        printf("Examples:\n");
        printf("%s \n",argv[0]);
        printf("%s NOPRINT\n",argv[0]);
        printf("%s BBERRORJSON\n",argv[0]);
        printf("%s BBERRORXML\n",argv[0]);
        printf("%s BBERRORFLAT\n",argv[0]);
        exit(-1);
      }
    } 
    if(argc > 2)
    {   
        printf("argc=%d\n",argc);
        printf("%s [printoption]\n",argv[0]);
        printf("printoption= one of BBERRORJSON (default), NOPRINT, BBERRORXML, BBERRORFLAT\n");
        printf("Examples:\n");
        printf("%s \n",argv[0]);
        printf("%s NOPRINT\n",argv[0]);
        printf("%s BBERRORJSON\n",argv[0]);
        printf("%s BBERRORXML\n",argv[0]);
        printf("%s BBERRORFLAT\n",argv[0]);
        exit(-1);
    }
   
    char buffer[4096];
    size_t size=4095;

    rc=BB_GetVersion(0, buffer);
    if (rc) print_bberror(rc);
    if (!rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}

    rc=BB_GetVersion(size, NULL);
    if (rc) print_bberror(rc);
    if (!rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}

    /* this should be valid */
    rc=BB_GetVersion(size, buffer);
    //printf("version \n %s \n",buffer);
    if (rc) print_bberror(rc);
    if (rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}

    size=strlen(buffer);
    for (i=0; i<size; buffer[i++]=0);

    /* this should be too small */
    rc=BB_GetVersion(size, buffer);
    //printf("version \n %s \n",buffer);
    if (rc) print_bberror(rc);
    if (!rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}

    for (i=0; i<size; buffer[i++]=0);

    /* this should be exact */
    size++;
    rc=BB_GetVersion(size, buffer);
    //printf("version \n %s \n",buffer);
    if (rc) print_bberror(rc);
    if (rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}
   
            
    rc = BB_InitLibrary(0, BBAPI_CLIENTVERSIONSTR);
    if (rc) printf("bbProxy not running or version mismatch\n");
    check(rc);

    size=4095;
    for (i=0; i<size; buffer[i++]=0);

    rc=BB_GetVersion(0, buffer);
    if (rc) print_bberror(rc);
    if (!rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}

    rc=BB_GetVersion(size, NULL);
    if (rc) print_bberror(rc);
    if (!rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}

    /* this should be valid */
    rc=BB_GetVersion(size, buffer);
    //printf("version \n %s \n",buffer);
    if (rc) print_bberror(rc);
    if (rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}

    size=strlen(buffer);
    for (i=0; i<size; buffer[i++]=0);

    /* this should be too small */
    rc=BB_GetVersion(size, buffer);
    //printf("version \n %s \n",buffer);
    if (rc) print_bberror(rc);
    if (!rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}

    for (i=0; i<size; buffer[i++]=0);

    /* this should be exact */
    size++;
    rc=BB_GetVersion(size, buffer);
    //printf("version \n %s \n",buffer);
    if (rc) print_bberror(rc);
    if (rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}
         
    return 0;
}
    
