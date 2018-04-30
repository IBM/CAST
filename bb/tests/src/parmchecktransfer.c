/*******************************************************************************
 |  parmchecktransfer.c
 |
 |   Copyright IBM Corporation 2017. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/
//  4.	ERROR: trasferDef was never created

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
    unsigned int rank = 0;
    BBTransferDef_t* tdefPtr=NULL;

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
   
    char* source = "x";
    char* target = "y";
    const char* key = "UnknownKey";
    const char* value = "UnknownValue";
        
    rc = BB_InitLibrary(rank, BBAPI_CLIENTVERSIONSTR);
    check(rc);
    
    //printf("source file: %s\n", source);
    //printf("target file: %s\n", target);
    
    tdefPtr=NULL;
    rc=BB_FreeTransferDef(tdefPtr);
    print_bberror(rc);
    if (!rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}

    tdefPtr=(BBTransferDef_t*)1;
    rc=BB_FreeTransferDef(tdefPtr);
    print_bberror(rc);
    if (!rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}

    tdefPtr=NULL;
    rc = BB_AddKeys(tdefPtr, key, value);
    print_bberror(rc);
    if (!rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}

    tdefPtr=(BBTransferDef_t*)1;
    rc = BB_AddKeys(tdefPtr, key, value);
    print_bberror(rc);
    if (!rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}

    tdefPtr=NULL;
    BB_StartTransfer(tdefPtr, 0);
    print_bberror(rc);
    if (!rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}

    tdefPtr=(BBTransferDef_t*)1;;
    BB_StartTransfer(tdefPtr, 0);
    print_bberror(rc);
    if (!rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}
    
    //printf("Attempting to add files to NULL transfer definition\n");
    tdefPtr=NULL;
    rc = BB_AddFiles(tdefPtr, source, target, 0 );
    print_bberror(rc);
    if (!rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}

    //printf("Attempting to add files to bad transfer definition\n");
    tdefPtr=(BBTransferDef_t*)1;
    rc = BB_AddFiles(tdefPtr, source, target, 0 );
    print_bberror(rc);
    if (!rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}

    rc=BB_CreateTransferDef(NULL);
    print_bberror(rc);
    if (!rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}

    //create a valid transfer def, test other parms
    rc=BB_CreateTransferDef(&tdefPtr);
    print_bberror(rc);
    if (rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}

    rc = BB_AddFiles(tdefPtr, NULL, target, 0 );
    print_bberror(rc);
    if (!rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}

    //invalid pHandle=0
    BB_StartTransfer(tdefPtr, 0);
    print_bberror(rc);
    if (!rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}

    rc = BB_AddFiles(tdefPtr, source, NULL, 0 );
    print_bberror(rc);
    if (!rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}

    rc = BB_AddKeys(tdefPtr, NULL, value);
    print_bberror(rc);
    if (!rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}

    rc = BB_AddKeys(tdefPtr, key, NULL);
    print_bberror(rc);
    if (!rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}


    //successful free
    rc=BB_FreeTransferDef(tdefPtr);
    print_bberror(rc);
    if (rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}

    //handle double free cases here until TerminateLibrary
    rc=BB_FreeTransferDef(tdefPtr);
    print_bberror(rc);
    if (!rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;} 

    //printf("StartTransfer of freed tdefPtr\n");
    BB_StartTransfer(tdefPtr, 0);
    print_bberror(rc);
    if (!rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}

    rc = BB_AddFiles(tdefPtr, source, target, 0 );
    print_bberror(rc);
    if (!rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}

    rc = BB_AddKeys(tdefPtr, key, value);
    print_bberror(rc);
    if (!rc) { printf("%s error at %d\n",argv[0],__LINE__); return __LINE__;}
   
    //printf("Terminating BB library\n");
    rc = BB_TerminateLibrary();
    check(rc);
    // printf( "Test Success!!!! \n" );
       
    return 0;
}
    
