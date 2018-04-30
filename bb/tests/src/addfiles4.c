/*******************************************************************************
 |    addfiles4.c
 |
 |   Copyright IBM Corporation 2017. All Rights Reserved.
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

int check(int rc)
{
    if(rc)
    {
        size_t buf_len = 65536;
        char errstring[buf_len];
        //errstring[0]=0;
        size_t numBytesAvail;
        
        printf("\nget error text with small buffer\n");
        int retcode = BB_GetLastErrorDetails(BBERRORJSON, &numBytesAvail, 200, errstring);
        printf("retcode from BB_GetLastErrorDetails retcode=%d buf_len=200 numBytesAvail=%d \n",retcode,  (int)numBytesAvail );
        printf("Error rc:       %d\n", rc);
        printf("Error details:  %s\n", errstring );

        printf("\nget error text with exact buffer \n");
        retcode = BB_GetLastErrorDetails(BBERRORJSON, &numBytesAvail, numBytesAvail, errstring);
        printf("retcode from BB_GetLastErrorDetails retcode=%d buf_len=%d numBytesAvail=%d \n",retcode, (int)numBytesAvail, (int)numBytesAvail );
        printf("Error rc:       %d\n", rc);
        printf("Error details:  %s\n", errstring );

        printf("\nget error text with overlarge buffer \n");
        retcode = BB_GetLastErrorDetails(BBERRORJSON, &numBytesAvail, numBytesAvail, errstring);
        printf("retcode from BB_GetLastErrorDetails retcode=%d buf_len=%d numBytesAvail=%d \n",retcode, (int)buf_len, (int)numBytesAvail );
        printf("Error rc:       %d\n", rc);
        printf("Error details:  %s\n", errstring );

        printf("\nget error text with overlarge buffer, -1 format selection \n");
        retcode = BB_GetLastErrorDetails(-1, &numBytesAvail, numBytesAvail, errstring);
        printf("retcode from BB_GetLastErrorDetails retcode=%d buf_len=%d numBytesAvail=%d \n",retcode, (int)buf_len, (int)numBytesAvail );
        printf("Error rc:       %d\n", rc);
        printf("Error details:  %s\n", errstring );
        
        printf("\nErrors tested.  Exiting\n");
        exit(0);
    }
    return rc;
}


int main(int argc, char** argv)
{
    int rc;
    unsigned int rank = 0;
    BBTransferDef_t tdef;
            
    if(argc < 3){
    
        printf("%s <source> <target>\n", argv[0]);
        exit(-1);
    }
    char* source = argv[1];
    char* target = argv[2];
        
    rc = BB_InitLibrary(rank, BBAPI_CLIENTVERSIONSTR);
    check(rc);
    
    printf("source file: %s\n", source);
    printf("target file: %s\n", target);
    
    
    BBTransferHandle_t thandle;
    printf("Obtaining transfer handle\n");
    rc = BB_GetTransferHandle(getpid(), 1, &rank, &thandle); /* \todo tag generation uses getpid() - need something better */
    check(rc);
    
    printf("Attempting to add files to (non-existent) transfer definition\n");
    rc = BB_AddFiles(&tdef, source, target, 0 );
    
    if( rc == 0 ) { // error did not happen
        fprintf( stderr, "%s\n",
"return from BB_AddFiles() should have indicated an error. 0 was returned." );

        fprintf( stderr, "Test Failed. \n" );
        
        fflush( stderr );
        exit( EXIT_FAILURE );
    }

    printf("Terminating BB library\n");
    rc = BB_TerminateLibrary();
    check(rc);
    
    printf( "Test Success!!!! \n" );
       
    return EXIT_SUCCESS;
}
    
