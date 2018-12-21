/*******************************************************************************
 |    transfer_list.c
 |
 |    Copyright IBM Corporation 2016,2017. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/


#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>

#include "bb/include/bbapi.h"


void dumpJSON(char*  data, size_t size)
{
       char *buf;
       char *command;


       if ( size == 0 || data == 0 )
       {
           return;     //User called func with wrong parms.   Just return
       }

       buf = (char *)malloc(size*sizeof(char));
       command = (char *)malloc(size*sizeof(char)+100);
       memset(buf, '\0', size);
       memset(command, '\0', size+100);
       sprintf( command, "echo '%s' | python -mjson.tool", data );  /* Format JSON output */
       FILE *sz = popen(command, "r");
       while (fgets(buf, size*sizeof(char), sz) != 0) {
           printf("%s", buf);
       } /*..end of pstream */
       pclose(sz);

       free(buf);
       free(command);
       printf("Size of JSON: %lu chars\n", size);

} /* end of dumpJSON */


void getLastErrorDetails(BBERRORFORMAT pFormat, char** pBuffer, size_t *bufSize)
{
    int rc;
    size_t l_NumBytesAvailable;
    if(pBuffer)
    {
        rc = BB_GetLastErrorDetails(pFormat, &l_NumBytesAvailable, 0, NULL);
        if(rc == 0)
        {
            *pBuffer = (char*)malloc(l_NumBytesAvailable+1);
            memset(*pBuffer, '\0', l_NumBytesAvailable+1);
          //  BB_GetLastErrorDetails(pFormat, NULL, l_NumBytesAvailable, *pBuffer);
            BB_GetLastErrorDetails(pFormat, &l_NumBytesAvailable, l_NumBytesAvailable+1, *pBuffer);
            *bufSize = l_NumBytesAvailable;
          //  printf("Error details:  %s\n", *pBuffer);
        }
        else
        {
            *pBuffer = NULL;
            *bufSize = 0;
        }
    }
}  /*  ..end of getLastErrorDetails..  */


int check(int rc)
{
    if (rc)
    {
        char* errstring = 0;
        size_t  errSz = 0;
        getLastErrorDetails(BBERRORJSON, &errstring, &errSz);
        printf("Error rc:       %d\n", rc);
        printf("Error details:  %s\n", errstring);
        free(errstring);

        printf("Aborting due to failures\n");
        exit(-1);
    }
    return 0;
}

int main( int argc, char *argv[] ) {
    int rc;
    uint32_t rank = 0;
    FILE *file_list;
    char *file2open;
    char file_line[256];
    char *sfn, *tfn, *errString=0;
    size_t   errSz = 0;

    if(argc < 2)
    {
        printf("testcase <list_file>\n");
        exit(-1);
    }

    file2open = argv[1];

    printf( "opening %s\n", file2open );
    if( !(file_list = fopen(file2open,"r")) ) {
        fprintf( stderr, "failed to open %s\n", file2open );
        exit( EXIT_FAILURE );
    }

    rc = BB_InitLibrary(rank, BBAPI_CLIENTVERSIONSTR);
    check(rc);

    BBTransferDef_t* tdef;
    BBTransferHandle_t thandle;
    printf("Obtaining transfer handle\n");
    rc = BB_GetTransferHandle(getpid(), 1, &rank, &thandle); /* \todo tag generation uses getpid() - need something better */
    check(rc);

    printf("Creating transfer definition\n");
    rc = BB_CreateTransferDef(&tdef);
    check(rc);

    while( fgets( file_line, 255, file_list) ) {
        sfn = strtok( file_line, " \n\t" );
        if( !sfn )  continue;
        tfn = strtok( NULL, " \n\t" );

        printf("source file: %s\n", sfn);
        printf("target file: %s\n", tfn);

        printf("Adding files to transfer definition\n");
        rc = BB_AddFiles(tdef, sfn, tfn, 0);
        check(rc);
    }

    fclose(file_list );


    printf("Starting transfer\n");
    rc = BB_StartTransfer(tdef, thandle);
    check(rc);

    getLastErrorDetails(BBERRORJSON, &errString, &errSz);
    printf("Transfer  rc:       %d\n", rc);
    printf("Transfer JSON details:\n");
    dumpJSON(errString, errSz);

    free(errString);
    printf("Terminating BB library\n");
    rc = BB_TerminateLibrary();
    check(rc);

    return 0;
}

